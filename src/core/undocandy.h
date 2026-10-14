#ifndef UNDOCANDY_H
#define UNDOCANDY_H

// ═══════════════════════════════════════════════════════════════════════
//  undoCandy.h — Undo por tiles 256×256 con LZ4 + zlib tiered
//  ─────────────────────────────────────────────────────────────────────
//  • RAM tier: LZ4 (rápido, ratio 1.5-2×)
//  • Disco tier: zlib nivel 6 (denso, se paga una sola vez)
//  • Respeta estrictamente ramStates; el resto a /tmp
// ═══════════════════════════════════════════════════════════════════════

#include <QImage>
#include <QSize>
#include <QList>
#include <QVector>
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>
#include <QCoreApplication>
#include <QDebug>
#include <functional>
#include <cstring>

#include <lz4.h>

namespace UndoCandy {

// ─────────────────────────────────────────────────────────────────────
//  LZ4 helpers (header-only)
// ─────────────────────────────────────────────────────────────────────
inline QByteArray lz4Compress(const QByteArray &src) {
    if (src.isEmpty()) return QByteArray();
    const int bound = LZ4_compressBound(src.size());
    QByteArray out;
    out.resize(4 + bound);
    qint32 origSize = src.size();
    std::memcpy(out.data(), &origSize, 4);
    const int written = LZ4_compress_default(src.constData(),
                                             out.data() + 4,
                                             src.size(), bound);
    if (written <= 0) {
        // Fallback: guardar sin comprimir (header con size=0)
        QByteArray raw;
        raw.resize(4 + src.size());
        qint32 zero = 0;
        std::memcpy(raw.data(), &zero, 4);
        std::memcpy(raw.data() + 4, src.constData(), src.size());
        return raw;
    }
    out.resize(4 + written);
    return out;
}

inline QByteArray lz4Decompress(const QByteArray &src) {
    if (src.size() < 4) return src;
    qint32 origSize = 0;
    std::memcpy(&origSize, src.constData(), 4);
    if (origSize == 0) {
        // Fallback: estaba sin comprimir
        return src.mid(4);
    }
    if (origSize < 0 || origSize > (1 << 30)) return QByteArray();
    QByteArray out;
    out.resize(origSize);
    const int decoded = LZ4_decompress_safe(src.constData() + 4,
                                             out.data(),
                                             src.size() - 4,
                                             origSize);
    if (decoded != origSize) return QByteArray();
    return out;
}

// ─────────────────────────────────────────────────────────────────────
struct TilePatch {
    int        tileIdx = 0;
    QByteArray data;                 // píxeles viejos del tile, LZ4 en RAM
};

struct Entry {
    bool             isDelta = false;
    QByteArray       fullData;       // si !isDelta (LZ4 en RAM, zlib en disco)
    QList<TilePatch> patches;        // si isDelta
    QSize            canvasSize;
    int              layerIndex = 0;
    QString          label;
    QString          diskPath;

    bool hasRam() const {
        if (!fullData.isEmpty()) return true;
        for (const auto &p : patches) if (!p.data.isEmpty()) return true;
        return false;
    }
    bool hasDisk() const { return !diskPath.isEmpty(); }
};

struct Result {
    QImage  image;
    int     layerIndex = 0;
    QString label;
    bool    valid = false;
};

// ─────────────────────────────────────────────────────────────────────
class Manager {
public:
    struct Config {
        int    ramStates      = 5;
        int    maxDiskStates  = 50;
        int    zlibDiskLevel  = 6;    // compresión al evictar a disco
        int    tileSize       = 256;
        double deltaThreshold = 0.60;
    };

    using LayerImageProvider = std::function<QImage(int layerIndex)>;

    Manager() { initSession(); }
    explicit Manager(const Config &cfg) : m_cfg(cfg) { initSession(); }
    ~Manager() { clearAndRemoveSession(); }

    Manager(const Manager &)            = delete;
    Manager &operator=(const Manager &) = delete;

    void setLayerImageProvider(LayerImageProvider p) { m_layerProvider = std::move(p); }
    void setConfig(const Config &c)                  { m_cfg = c; }
    const Config &config() const                     { return m_cfg; }
    void setTileSize(int px)                         { m_cfg.tileSize = qBound(64, px, 1024); }
    void setDeltaThreshold(double t)                 { m_cfg.deltaThreshold = qBound(0.1, t, 0.95); }

    bool canUndo() const { return !m_undoStack.isEmpty() || m_hasPending; }
    bool canRedo() const { return !m_redoStack.isEmpty(); }
    int  undoCount() const { return m_undoStack.size() + (m_hasPending ? 1 : 0); }
    int  redoCount() const { return m_redoStack.size(); }

    int  maxDiskStates() const { return m_cfg.maxDiskStates; }
    int  ramStates() const     { return m_cfg.ramStates; }
    bool hasPending() const    { return m_hasPending; }

    int ramCacheCount() const {
        int n = 0;
        for (const auto &e : m_undoStack) if (e.hasRam()) ++n;
        for (const auto &e : m_redoStack) if (e.hasRam()) ++n;
        return n;
    }

    qint64 ramBytes() const {
        qint64 t = 0;
        for (const auto &e : m_undoStack) {
            t += e.fullData.size();
            for (const auto &p : e.patches) t += p.data.size();
        }
        for (const auto &e : m_redoStack) {
            t += e.fullData.size();
            for (const auto &p : e.patches) t += p.data.size();
        }
        return t;
    }

    qint64 diskBytes() const {
        qint64 t = 0;
        for (const auto &e : m_undoStack) if (e.hasDisk()) t += QFileInfo(e.diskPath).size();
        for (const auto &e : m_redoStack) if (e.hasDisk()) t += QFileInfo(e.diskPath).size();
        return t;
    }

    QString nextUndoLabel() const {
        if (m_hasPending) return m_pendingLabel;
        return m_undoStack.isEmpty() ? QString() : m_undoStack.last().label;
    }
    QString nextRedoLabel() const {
        return m_redoStack.isEmpty() ? QString() : m_redoStack.last().label;
    }

    void setMaxDiskStates(int n) {
        m_cfg.maxDiskStates = qBound(5, n, 2000);
        enforceDiskLimit();
    }
    void setRamStates(int n) {
        m_cfg.ramStates = qBound(1, n, 200);
        enforceRamLimit();
    }

    void push(const QImage &img, int layerIndex, const QString &label = QString()) {
        if (img.isNull() || layerIndex < 0) return;
        truncateRedo();
        finalizePendingIfAny();

        m_pendingImage = img;          // COW
        m_pendingLayer = layerIndex;
        m_pendingLabel = label;
        m_hasPending   = true;
    }

    void push(const QImage &img, int layerIndex) {
        push(img, layerIndex, QStringLiteral("Edición"));
    }

    Result undo(const QString &currentLabel = QString()) {
        Result r;
        finalizePendingIfAny();
        if (m_undoStack.isEmpty()) return r;

        Entry e = m_undoStack.takeLast();

        QImage baseImg;
        if (m_layerProvider) baseImg = m_layerProvider(e.layerIndex);
        if (baseImg.isNull()) { m_undoStack.append(e); return r; }

        Entry redoEntry;
        redoEntry.isDelta    = false;
        redoEntry.fullData   = compressForRam(baseImg);
        redoEntry.canvasSize = baseImg.size();
        redoEntry.layerIndex = e.layerIndex;
        redoEntry.label      = currentLabel.isEmpty() ? QStringLiteral("Cambio") : currentLabel;
        m_redoStack.append(redoEntry);

        QImage target = materializeEntry(e, baseImg);
        r.image      = target;
        r.layerIndex = e.layerIndex;
        r.label      = e.label;
        r.valid      = !target.isNull();

        removeDiskFile(e);
        enforceRamLimit();
        return r;
    }

    Result redo(const QString &currentLabel = QString()) {
        Result r;
        finalizePendingIfAny();
        if (m_redoStack.isEmpty()) return r;

        Entry e = m_redoStack.takeLast();

        QImage baseImg;
        if (m_layerProvider) baseImg = m_layerProvider(e.layerIndex);
        if (baseImg.isNull()) { m_redoStack.append(e); return r; }

        Entry undoEntry;
        undoEntry.isDelta    = false;
        undoEntry.fullData   = compressForRam(baseImg);
        undoEntry.canvasSize = baseImg.size();
        undoEntry.layerIndex = e.layerIndex;
        undoEntry.label      = currentLabel.isEmpty() ? QStringLiteral("Cambio") : currentLabel;
        m_undoStack.append(undoEntry);

        QImage target = materializeEntry(e, baseImg);
        r.image      = target;
        r.layerIndex = e.layerIndex;
        r.label      = e.label;
        r.valid      = !target.isNull();

        removeDiskFile(e);
        enforceRamLimit();
        return r;
    }

    void clear() {
        for (auto &e : m_undoStack) removeDiskFile(e);
        for (auto &e : m_redoStack) removeDiskFile(e);
        m_undoStack.clear();
        m_redoStack.clear();
        m_pendingImage = QImage();
        m_pendingLayer = -1;
        m_pendingLabel.clear();
        m_hasPending = false;

        QDir dir(m_sessionDir);
        if (dir.exists()) {
            for (const QFileInfo &fi : dir.entryInfoList({"*.z"}, QDir::Files))
                QFile::remove(fi.absoluteFilePath());
        }
    }

    void materialize() { finalizePendingIfAny(); }

private:
    Config  m_cfg;
    QString m_sessionDir;
    quint64 m_nextId = 0;

    QVector<Entry> m_undoStack;
    QVector<Entry> m_redoStack;

    QImage  m_pendingImage;
    int     m_pendingLayer = -1;
    QString m_pendingLabel;
    bool    m_hasPending = false;

    LayerImageProvider m_layerProvider;

    void initSession() {
        m_sessionDir = QDir::tempPath()
                     + QStringLiteral("/undocandy-")
                     + QString::number(QCoreApplication::applicationPid())
                     + QStringLiteral("-")
                     + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QDir().mkpath(m_sessionDir);
    }

    void clearAndRemoveSession() {
        clear();
        QDir dir(m_sessionDir);
        if (dir.exists()) dir.removeRecursively();
    }

    QString nextPath() {
        return m_sessionDir + QStringLiteral("/%1.z")
               .arg(m_nextId++, 8, 10, QChar('0'));
    }

    // ── Serialización base ──
    static QImage toArgb32(const QImage &src) {
        if (src.isNull()) return QImage();
        if (src.format() == QImage::Format_ARGB32 ||
            src.format() == QImage::Format_ARGB32_Premultiplied)
            return src;
        return src.convertToFormat(QImage::Format_ARGB32);
    }

    static QByteArray serializeRaw(const QImage &img) {
        const int w = img.width();
        const int h = img.height();
        const int stride = w * 4;
        QByteArray raw;
        raw.resize(8 + h * stride);
        char *p = raw.data();
        qint32 ww = w, hh = h;
        std::memcpy(p, &ww, 4); p += 4;
        std::memcpy(p, &hh, 4); p += 4;
        for (int y = 0; y < h; ++y) {
            std::memcpy(p, img.constScanLine(y), stride);
            p += stride;
        }
        return raw;
    }

    static QImage deserializeRaw(const QByteArray &raw) {
        if (raw.size() < 8) return QImage();
        const char *p = raw.constData();
        qint32 w = 0, h = 0;
        std::memcpy(&w, p, 4); p += 4;
        std::memcpy(&h, p, 4); p += 4;
        if (w <= 0 || h <= 0) return QImage();
        const int stride = w * 4;
        if (raw.size() < 8 + h * stride) return QImage();
        QImage img(w, h, QImage::Format_ARGB32);
        for (int y = 0; y < h; ++y) {
            std::memcpy(img.scanLine(y), p, stride);
            p += stride;
        }
        return img;
    }

    // RAM usa LZ4 (rápido)
    QByteArray compressForRam(const QImage &img) const {
        if (img.isNull()) return QByteArray();
        return lz4Compress(serializeRaw(toArgb32(img)));
    }

    int tileSize() const { return m_cfg.tileSize; }

    int numTiles(const QSize &sz) const {
        const int T = tileSize();
        const int cols = (sz.width()  + T - 1) / T;
        const int rows = (sz.height() + T - 1) / T;
        return cols * rows;
    }

    QList<TilePatch> computeTileDiff(const QImage &oldImg, const QImage &newImg) const {
        QList<TilePatch> patches;
        if (oldImg.size() != newImg.size()) return patches;

        QImage o = oldImg;
        QImage n = newImg;
        if (o.format() != QImage::Format_ARGB32) o = o.convertToFormat(QImage::Format_ARGB32);
        if (n.format() != QImage::Format_ARGB32) n = n.convertToFormat(QImage::Format_ARGB32);

        const int T = tileSize();
        const int W = o.width();
        const int H = o.height();
        const int cols = (W + T - 1) / T;
        const int rows = (H + T - 1) / T;

        for (int ty = 0; ty < rows; ++ty) {
            for (int tx = 0; tx < cols; ++tx) {
                const int x0 = tx * T;
                const int y0 = ty * T;
                const int tw = qMin(T, W - x0);
                const int th = qMin(T, H - y0);

                bool differs = false;
                for (int y = 0; y < th && !differs; ++y) {
                    const QRgb *oL = reinterpret_cast<const QRgb*>(o.constScanLine(y0 + y)) + x0;
                    const QRgb *nL = reinterpret_cast<const QRgb*>(n.constScanLine(y0 + y)) + x0;
                    if (std::memcmp(oL, nL, tw * 4) != 0) differs = true;
                }
                if (!differs) continue;

                TilePatch p;
                p.tileIdx = ty * cols + tx;
                QByteArray raw;
                raw.resize(tw * th * 4);
                char *dst = raw.data();
                for (int y = 0; y < th; ++y) {
                    const QRgb *oL = reinterpret_cast<const QRgb*>(o.constScanLine(y0 + y)) + x0;
                    std::memcpy(dst + y * tw * 4, oL, tw * 4);
                }
                p.data = lz4Compress(raw);
                patches.append(p);
            }
        }
        return patches;
    }

    // Aplica patches; descomprime LZ4 si hace falta (marca _isZlib=false)
    QImage applyPatches(const QImage &base,
                        const QList<TilePatch> &patches,
                        const QSize &canvasSize,
                        bool patchesAreZlib = false) const
    {
        QImage result = base;
        if (result.format() != QImage::Format_ARGB32)
            result = result.convertToFormat(QImage::Format_ARGB32);
        if (!result.isDetached()) result.detach();

        const int T = tileSize();
        const int cols = (canvasSize.width() + T - 1) / T;

        for (const auto &p : patches) {
            const int tx = p.tileIdx % cols;
            const int ty = p.tileIdx / cols;
            const int x0 = tx * T;
            const int y0 = ty * T;
            const int tw = qMin(T, canvasSize.width() - x0);
            const int th = qMin(T, canvasSize.height() - y0);

            QByteArray raw = patchesAreZlib ? qUncompress(p.data)
                                            : lz4Decompress(p.data);
            if (raw.size() < tw * th * 4) continue;

            for (int y = 0; y < th; ++y) {
                QRgb *dstL = reinterpret_cast<QRgb*>(result.scanLine(y0 + y)) + x0;
                const QRgb *srcL = reinterpret_cast<const QRgb*>(raw.constData() + y * tw * 4);
                std::memcpy(dstL, srcL, tw * 4);
            }
        }
        return result;
    }

    // ═══════════════════════════════════════════════════════════════
    //  Serialización para DISCO (zlib nivel alto)
    //  Formato:
    //    quint8 isDelta
    //    si delta: qint32 n, y por cada uno: qint32 tileIdx + QByteArray zlib
    //    si full:  QByteArray zlib
    //    qint32 canvasW, canvasH, layerIndex
    //    QString label
    // ═══════════════════════════════════════════════════════════════
    QByteArray serializeEntryForDisk(const Entry &e) const {
        QByteArray out;
        QDataStream s(&out, QIODevice::WriteOnly);
        s.setVersion(QDataStream::Qt_6_0);
        s << quint8(e.isDelta ? 1 : 0);

        if (e.isDelta) {
            s << qint32(e.patches.size());
            for (const auto &p : e.patches) {
                s << qint32(p.tileIdx);
                // RAM: LZ4 → descomprimir → recomprimir zlib
                QByteArray raw = lz4Decompress(p.data);
                QByteArray zlib = qCompress(raw, m_cfg.zlibDiskLevel);
                s << zlib;
            }
        } else {
            QByteArray raw = lz4Decompress(e.fullData);
            QByteArray zlib = qCompress(raw, m_cfg.zlibDiskLevel);
            s << zlib;
        }

        s << qint32(e.canvasSize.width());
        s << qint32(e.canvasSize.height());
        s << qint32(e.layerIndex);
        s << e.label;
        return out;
    }

    void finalizePendingIfAny() {
        if (!m_hasPending || m_pendingImage.isNull()) {
            m_hasPending = false;
            m_pendingImage = QImage();
            return;
        }

        QImage nextImg;
        if (m_layerProvider) nextImg = m_layerProvider(m_pendingLayer);

        Entry e;
        e.canvasSize = m_pendingImage.size();
        e.layerIndex = m_pendingLayer;
        e.label      = m_pendingLabel;

        bool useDelta = false;
        if (!nextImg.isNull() && nextImg.size() == m_pendingImage.size()) {
            QList<TilePatch> patches = computeTileDiff(m_pendingImage, nextImg);
            const int total = numTiles(m_pendingImage.size());
            const int threshold = qMax(1, (int)(total * m_cfg.deltaThreshold));
            if (!patches.isEmpty() && patches.size() < threshold) {
                e.isDelta = true;
                e.patches = patches;
                useDelta = true;
            }
        }

        if (!useDelta) {
            e.isDelta  = false;
            e.fullData = compressForRam(m_pendingImage);
        }

        m_undoStack.append(e);
        m_hasPending   = false;
        m_pendingImage = QImage();
        m_pendingLayer = -1;
        m_pendingLabel.clear();

        enforceRamLimit();
        enforceDiskLimit();
    }

    QImage materializeEntry(const Entry &e, const QImage &baseImg) const {
        // RAM
        if (e.isDelta && !e.patches.isEmpty()) {
            return applyPatches(baseImg, e.patches, e.canvasSize, false);
        }
        if (!e.isDelta && !e.fullData.isEmpty()) {
            QByteArray raw = lz4Decompress(e.fullData);
            return deserializeRaw(raw);
        }

        // Disco
        if (e.hasDisk()) {
            QFile f(e.diskPath);
            if (!f.open(QIODevice::ReadOnly)) return QImage();
            QByteArray diskBlob = f.readAll();
            f.close();

            QDataStream s(diskBlob);
            s.setVersion(QDataStream::Qt_6_0);
            quint8 flag = 0;
            s >> flag;

            if (flag == 1) {
                qint32 n = 0;
                s >> n;
                QList<TilePatch> patches;
                patches.reserve(n);
                for (qint32 i = 0; i < n; ++i) {
                    qint32 idx = 0;
                    QByteArray zlib;
                    s >> idx >> zlib;
                    TilePatch p;
                    p.tileIdx = idx;
                    p.data = zlib;
                    patches.append(p);
                }
                // Leer canvasSize/layerIndex/label (no hacen falta)
                qint32 w = 0, h = 0, li = 0;
                QString lbl;
                s >> w >> h >> li >> lbl;
                QSize cs(w, h);
                return applyPatches(baseImg, patches, cs, true);
            } else {
                QByteArray zlib;
                s >> zlib;
                QByteArray raw = qUncompress(zlib);
                return deserializeRaw(raw);
            }
        }
        return QImage();
    }

    void evictToDisk(Entry &e) {
        if (!e.hasRam()) return;
        if (e.diskPath.isEmpty()) e.diskPath = nextPath();

        QByteArray blob = serializeEntryForDisk(e);
        QFile f(e.diskPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(blob);
            f.close();
        }

        e.fullData.clear();
        e.fullData.squeeze();
        e.patches.clear();
        e.patches.squeeze();
    }

    void removeDiskFile(const Entry &e) {
        if (!e.diskPath.isEmpty()) QFile::remove(e.diskPath);
    }

    void truncateRedo() {
        for (auto &e : m_redoStack) removeDiskFile(e);
        m_redoStack.clear();
    }

    void enforceDiskLimit() {
        while (m_undoStack.size() > m_cfg.maxDiskStates) {
            Entry e = m_undoStack.takeFirst();
            removeDiskFile(e);
        }
    }

    void enforceRamLimit() {
        int total = ramCacheCount();
        while (total > m_cfg.ramStates) {
            bool evicted = false;
            for (auto &e : m_undoStack) {
                if (e.hasRam()) { evictToDisk(e); --total; evicted = true; break; }
            }
            if (!evicted) {
                for (auto &e : m_redoStack) {
                    if (e.hasRam()) { evictToDisk(e); --total; evicted = true; break; }
                }
            }
            if (!evicted) break;
        }
    }
};

} // namespace UndoCandy

#endif // UNDOCANDY_H