#ifndef COLRS_H
#define COLRS_H

#include <QDialog>
#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QConicalGradient>
#include <QLinearGradient>
#include <QColor>
#include <QColorDialog>
#include <QCursor>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QVector>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QFrame>
#include <QSize>
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QSettings>
#include <QVariantList>
#include <QListWidget>
#include <QListView>
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QTextStream>
#include <QFileInfo>
#include <QToolTip>
#include <QTimer>
#include <QScrollArea>
#include <QToolButton>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QWindow>
#include <QAction>
#include <QPixmap>
#include <QObject>
#include <QPair>
#include <QList>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Colrs {

// ═══════════════════════════════════════════════════════════════════════
//  DragHelper — arrastre de ventanas sin marco
// ═══════════════════════════════════════════════════════════════════════
class DragHelper {
public:
    virtual ~DragHelper() = default;

protected:
    static bool tryStartDrag(QWidget *dialog, QMouseEvent *e) {
        if (!dialog || !e || e->button() != Qt::LeftButton) return false;

        if (QWindow *wh = dialog->windowHandle()) {
            if (wh->startSystemMove()) {
                e->accept();
                return true;
            }
        }

        dialog->setProperty(
            "_dragOffset",
            e->globalPosition().toPoint() - dialog->frameGeometry().topLeft());
        dialog->setProperty("_dragging", true);
        e->accept();
        return true;
    }

    static bool handleDragMove(QWidget *dialog, QMouseEvent *e) {
        if (!dialog) return false;
        if (!dialog->property("_dragging").toBool()) return false;
        if (!(e->buttons() & Qt::LeftButton)) return false;

        const QPoint offset = dialog->property("_dragOffset").toPoint();
        dialog->move(e->globalPosition().toPoint() - offset);
        e->accept();
        return true;
    }

    static void endDrag(QWidget *dialog) {
        if (dialog) dialog->setProperty("_dragging", false);
    }
};

// ═══════════════════════════════════════════════════════════════════════
//  TriangleColorPicker — triángulo HSV + anillo de tono
// ═══════════════════════════════════════════════════════════════════════
class TriangleColorPicker : public QWidget {
    Q_OBJECT
public:
    explicit TriangleColorPicker(QWidget *parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor &c);

signals:
    void colorChanged(const QColor &color);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    QColor  m_color = Qt::red;
    double  m_hue = 0.0;
    double  m_sat = 1.0;
    double  m_val = 1.0;
    bool    m_dragging = false;
    int     m_dragMode = 0;

    QPointF m_center;
    double  m_ringOuter = 0.0;
    double  m_ringInner = 0.0;
    double  m_triRadius = 0.0;

    QPointF m_vWhite;
    QPointF m_vBlack;
    QPointF m_vHue;

    QImage  m_cache;
    double  m_cachedHue = -1.0;

    void recalcGeometry();
    void invalidateCache();

    static bool barycentric(const QPointF &p,
                            const QPointF &a,
                            const QPointF &b,
                            const QPointF &c,
                            double &wa,
                            double &wb,
                            double &wc);

    void applyClick(const QPointF &pos);
    void emitColor();
    void regenerateCache();
    void paintHueRing(QPainter &p);
    void paintTriangle(QPainter &p);
    void paintMarkers(QPainter &p);
};

inline TriangleColorPicker::TriangleColorPicker(QWidget *parent) : QWidget(parent) {
    setMinimumSize(200, 200);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    recalcGeometry();
}

inline void TriangleColorPicker::setColor(const QColor &c) {
    if (!c.isValid()) return;

    float h = 0.0f, s = 0.0f, v = 0.0f, a = 0.0f;
    c.getHsvF(&h, &s, &v, &a);

    if (h >= 0.0f) m_hue = static_cast<double>(h);
    m_sat = qBound(0.0, static_cast<double>(s), 1.0);
    m_val = qBound(0.0, static_cast<double>(v), 1.0);
    m_color = QColor::fromHsvF(m_hue, m_sat, m_val, 1.0);

    invalidateCache();
    update();
}

inline void TriangleColorPicker::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    recalcGeometry();
    invalidateCache();
}

inline void TriangleColorPicker::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) return;

    const QPointF pos = e->position();
    const double dx = pos.x() - m_center.x();
    const double dy = pos.y() - m_center.y();
    const double dist = std::sqrt(dx * dx + dy * dy);

    if (dist >= m_ringInner && dist <= m_ringOuter) m_dragMode = 1;
    else m_dragMode = 2;

    m_dragging = true;
    applyClick(pos);
}

inline void TriangleColorPicker::mouseMoveEvent(QMouseEvent *e) {
    if (!m_dragging) return;
    applyClick(e->position());
}

inline void TriangleColorPicker::mouseReleaseEvent(QMouseEvent *) {
    m_dragging = false;
    m_dragMode = 0;
}

inline void TriangleColorPicker::recalcGeometry() {
    const double side = qMin<double>(width(), height());

    m_center    = QPointF(width() / 2.0, height() / 2.0);
    m_ringOuter = side * 0.5 - 4.0;
    m_ringInner = m_ringOuter * 0.82;
    m_triRadius = m_ringInner - 6.0;

    auto vertexAt = [&](double deg) -> QPointF {
        const double rad = deg * M_PI / 180.0;
        return QPointF(m_center.x() + m_triRadius * std::cos(rad),
                       m_center.y() - m_triRadius * std::sin(rad));
    };

    m_vWhite = vertexAt(90.0);
    m_vBlack = vertexAt(210.0);
    m_vHue   = vertexAt(330.0);
}

inline void TriangleColorPicker::invalidateCache() {
    m_cachedHue = -1.0;
}

inline bool TriangleColorPicker::barycentric(const QPointF &p,
                                             const QPointF &a,
                                             const QPointF &b,
                                             const QPointF &c,
                                             double &wa,
                                             double &wb,
                                             double &wc) {
    const double denom = (b.y() - c.y()) * (a.x() - c.x())
                       + (c.x() - b.x()) * (a.y() - c.y());

    if (std::abs(denom) < 1e-9) return false;

    wa = ((b.y() - c.y()) * (p.x() - c.x()) + (c.x() - b.x()) * (p.y() - c.y())) / denom;
    wb = ((c.y() - a.y()) * (p.x() - c.x()) + (a.x() - c.x()) * (p.y() - c.y())) / denom;
    wc = 1.0 - wa - wb;

    return wa >= -0.02 && wb >= -0.02 && wc >= -0.02;
}

inline void TriangleColorPicker::applyClick(const QPointF &pos) {
    const double dx = pos.x() - m_center.x();
    const double dy = pos.y() - m_center.y();
    const double dist = std::sqrt(dx * dx + dy * dy);

    if (m_dragMode == 1 || (m_dragMode == 0 && dist >= m_ringInner && dist <= m_ringOuter)) {
        double raw = std::atan2(-dy, dx) * 180.0 / M_PI;
        double hueDeg = std::fmod(90.0 - raw + 720.0, 360.0);
        m_hue = hueDeg / 360.0;

        invalidateCache();
        update();
        emitColor();
        return;
    }

    if (m_dragMode == 2 || m_dragMode == 0) {
        double w, b, s;
        if (barycentric(pos, m_vWhite, m_vBlack, m_vHue, w, b, s)) {
            m_val = qBound(0.0, w + s, 1.0);
            m_sat = (m_val > 0.0001) ? qBound(0.0, s / m_val, 1.0) : 0.0;
            update();
            emitColor();
        }
    }
}

inline void TriangleColorPicker::emitColor() {
    QColor c = QColor::fromHsvF(m_hue, m_sat, m_val, 1.0);
    if (c != m_color) {
        m_color = c;
        emit colorChanged(m_color);
    }
}

inline void TriangleColorPicker::regenerateCache() {
    const int size = int(m_triRadius * 2.0) + 4;
    if (size <= 8) return;

    m_cache = QImage(size, size, QImage::Format_ARGB32);
    m_cache.fill(Qt::transparent);

    const QPointF lc(size / 2.0, size / 2.0);
    const QPointF lw = m_vWhite - m_center + lc;
    const QPointF lb = m_vBlack - m_center + lc;
    const QPointF lh = m_vHue   - m_center + lc;

    const double hue = m_hue;

    for (int y = 0; y < size; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(m_cache.scanLine(y));

        for (int x = 0; x < size; ++x) {
            const QPointF pt(x + 0.5, y + 0.5);

            double w, b, s;
            if (!barycentric(pt, lw, lb, lh, w, b, s)) continue;

            const double V = w + s;
            const double S = (V > 0.0001) ? (s / V) : 0.0;

            line[x] = QColor::fromHsvF(hue,
                                       qBound(0.0, S, 1.0),
                                       qBound(0.0, V, 1.0),
                                       1.0).rgba();
        }
    }

    m_cachedHue = hue;
}

inline void TriangleColorPicker::paintHueRing(QPainter &p) {
    const QRectF ringRect(m_center.x() - m_ringOuter,
                          m_center.y() - m_ringOuter,
                          m_ringOuter * 2,
                          m_ringOuter * 2);

    QConicalGradient grad(m_center, 90.0);
    for (int i = 0; i <= 72; ++i) {
        const double t = i / 72.0;
        grad.setColorAt(t, QColor::fromHsvF(1.0 - t, 1.0, 1.0));
    }

    QPainterPath outer;
    outer.addEllipse(ringRect);

    QPainterPath inner;
    inner.addEllipse(m_center, m_ringInner, m_ringInner);

    QPainterPath ring = outer.subtracted(inner);

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawPath(ring);
    p.restore();
}

inline void TriangleColorPicker::paintTriangle(QPainter &p) {
    if (m_cache.isNull() || m_cachedHue != m_hue) regenerateCache();
    if (m_cache.isNull()) return;

    p.drawImage(m_center.x() - m_cache.width() / 2.0,
                m_center.y() - m_cache.height() / 2.0,
                m_cache);
}

inline void TriangleColorPicker::paintMarkers(QPainter &p) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    const double qtAngle = (90.0 - m_hue * 360.0) * M_PI / 180.0;
    const double midR = (m_ringOuter + m_ringInner) / 2.0;

    const QPointF huePos(m_center.x() + midR * std::cos(qtAngle),
                         m_center.y() - midR * std::sin(qtAngle));

    p.setBrush(Qt::NoBrush);

    p.setPen(QPen(Qt::white, 2.5));
    p.drawEllipse(huePos, 7.0, 7.0);

    p.setPen(QPen(Qt::black, 1.0));
    p.drawEllipse(huePos, 7.0, 7.0);

    const double w = m_val * (1.0 - m_sat);
    const double b = 1.0 - m_val;
    const double s = m_val * m_sat;

    const QPointF svPos = w * m_vWhite + b * m_vBlack + s * m_vHue;

    p.setPen(QPen(Qt::white, 2.5));
    p.drawEllipse(svPos, 6.5, 6.5);

    p.setPen(QPen(Qt::black, 1.0));
    p.drawEllipse(svPos, 6.5, 6.5);

    p.restore();
}

inline void TriangleColorPicker::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    paintHueRing(p);
    paintTriangle(p);
    paintMarkers(p);
}

// ═══════════════════════════════════════════════════════════════════════
//  ColorSlotButton — slot primario / secundario
// ═══════════════════════════════════════════════════════════════════════
class ColorSlotButton : public QPushButton {
    Q_OBJECT
public:
    explicit ColorSlotButton(int slotIndex, QWidget *parent = nullptr);

    void setColor(const QColor &c) { m_color = c; update(); }
    QColor color() const { return m_color; }
    int slotIndex() const { return m_slotIndex; }
    void setActive(bool a) { m_active = a; update(); }

signals:
    void slotClicked(int slotIndex);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    int    m_slotIndex;
    QColor m_color = Qt::black;
    bool   m_active = false;
};

inline ColorSlotButton::ColorSlotButton(int slotIndex, QWidget *parent)
    : QPushButton(parent), m_slotIndex(slotIndex) {
    setFixedSize(34, 56);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

inline void ColorSlotButton::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect().adjusted(2, 2, -2, -2);

    QPixmap checker(8, 8);
    checker.fill(Qt::white);

    QPainter cp(&checker);
    cp.fillRect(0, 0, 4, 4, QColor(200, 200, 200));
    cp.fillRect(4, 4, 4, 4, QColor(200, 200, 200));
    cp.end();

    p.save();
    QPainterPath clip;
    clip.addRoundedRect(r, 4, 4);
    p.setClipPath(clip);
    p.drawTiledPixmap(r, checker);
    p.fillRect(r, m_color);
    p.restore();

    if (m_active) {
        p.setPen(QPen(QColor("#3b82f6"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(-1, -1, 1, 1), 5, 5);
    } else {
        p.setPen(QPen(QColor(120, 120, 120), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 4, 4);
    }
}

inline void ColorSlotButton::mousePressEvent(QMouseEvent *e) {
    emit slotClicked(m_slotIndex);
    QPushButton::mousePressEvent(e);
}

// ═══════════════════════════════════════════════════════════════════════
//  PaletteStore — persistencia JSON de paletas
// ═══════════════════════════════════════════════════════════════════════
class PaletteStore {
public:
    struct Palette {
        QString name;
        QVector<QColor> colors;
    };

    static QString dir() {
        QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (base.isEmpty()) base = QDir::homePath() + "/.paintux";

        QDir d(base);
        if (!d.exists("palettes")) d.mkpath("palettes");

        return d.filePath("palettes");
    }

    static QString sanitize(const QString &n) {
        QString s = n.trimmed();
        static const QRegularExpression re("[^A-Za-z0-9_\\- ]");
        s.replace(re, "_");
        if (s.isEmpty()) s = "palette";
        return s;
    }

    static bool save(const Palette &p) {
        if (p.name.trimmed().isEmpty()) return false;

        QJsonObject obj;
        obj["name"] = p.name;

        QJsonArray arr;
        for (const QColor &c : p.colors) arr.append(c.name(QColor::HexArgb));
        obj["colors"] = arr;

        QFile f(QDir(dir()).filePath(sanitize(p.name) + ".json"));
        if (!f.open(QIODevice::WriteOnly)) return false;

        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        return true;
    }

    static Palette load(const QString &filePath) {
        Palette p;

        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) return p;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) return p;

        QJsonObject obj = doc.object();
        p.name = obj.value("name").toString();

        for (const QJsonValue &v : obj.value("colors").toArray()) {
            QColor c(v.toString());
            if (c.isValid()) p.colors.append(c);
        }

        return p;
    }

    static QVector<Palette> loadAll() {
        QVector<Palette> out;

        QDir d(dir());
        const QStringList files = d.entryList({"*.json"}, QDir::Files, QDir::Name);

        for (const QString &fn : files) {
            Palette p = load(d.filePath(fn));
            if (!p.name.isEmpty()) out.append(p);
        }

        return out;
    }

    static bool remove(const QString &name) {
        QFile f(QDir(dir()).filePath(sanitize(name) + ".json"));
        return f.exists() && f.remove();
    }

    static bool exists(const QString &name) {
        return QFile::exists(QDir(dir()).filePath(sanitize(name) + ".json"));
    }

    static QVector<QColor> defaultMSPaint() {
        return {
            Qt::black, QColor(127,127,127), QColor(136,0,21), Qt::red,
            QColor(255,127,39), Qt::yellow, Qt::green, QColor(0,162,232),
            QColor(63,72,204), QColor(163,73,164), Qt::white, QColor(195,195,195),
            QColor(185,122,87), QColor(255,174,201), QColor(255,201,14),
            QColor(239,228,176), QColor(181,230,29), QColor(153,217,234),
            QColor(112,146,190), QColor(200,191,231)
        };
    }

    static QVector<QColor> defaultClassicArt() {
        return {
            QColor(255,255,255), QColor(240,230,210), QColor(220,190,150),
            QColor(180,120,70), QColor(120,70,40), QColor(60,30,15),
            QColor(0,0,0), QColor(80,80,80), QColor(160,160,160),
            QColor(220,40,40), QColor(240,120,120), QColor(255,200,200),
            QColor(255,180,40), QColor(255,220,120), QColor(240,240,180),
            QColor(40,120,60), QColor(90,180,90), QColor(180,220,140),
            QColor(30,90,160), QColor(80,150,220)
        };
    }

    static QVector<QColor> defaultRetroNeon() {
        return {
            QColor(10,5,20), QColor(40,10,60), QColor(90,20,110),
            QColor(180,20,140), QColor(255,40,180), QColor(255,120,200),
            QColor(0,220,220), QColor(0,180,255), QColor(0,120,220),
            QColor(60,80,220), QColor(140,60,220), QColor(200,60,240),
            QColor(255,180,0), QColor(255,100,20), QColor(255,60,60),
            QColor(180,255,0), QColor(0,255,150), QColor(100,255,220),
            QColor(240,240,255), QColor(120,120,160)
        };
    }

    static QVector<QColor> defaultAurora() {
        return {
            QColor(8,12,30), QColor(16,24,54), QColor(24,40,82),
            QColor(32,64,120), QColor(30,100,160), QColor(30,140,190),
            QColor(50,180,200), QColor(90,220,210), QColor(140,240,190),
            QColor(190,250,170), QColor(240,250,180), QColor(250,220,160),
            QColor(250,180,140), QColor(230,130,140), QColor(190,90,160),
            QColor(140,60,170), QColor(90,40,150), QColor(50,24,100),
            QColor(28,16,60), QColor(12,10,30)
        };
    }

    static QVector<QColor> defaultPastel() {
        return {
            QColor(255,250,245), QColor(255,235,235), QColor(255,214,222),
            QColor(255,183,205), QColor(255,154,188), QColor(250,120,160),
            QColor(255,183,162), QColor(255,214,165), QColor(255,236,179),
            QColor(214,255,190), QColor(168,230,170), QColor(120,200,190),
            QColor(137,220,235), QColor(120,180,255), QColor(170,160,255),
            QColor(214,170,255), QColor(255,190,255), QColor(240,240,240),
            QColor(180,180,190), QColor(70,70,80)
        };
    }
};

// ═══════════════════════════════════════════════════════════════════════
//  PaletteController — fuente única de verdad de la paleta activa
// ═══════════════════════════════════════════════════════════════════════
class PaletteController : public QObject {
    Q_OBJECT
public:
    static PaletteController &instance() {
        static PaletteController s;
        return s;
    }

    QString activeName() const { return m_name; }
    QVector<QColor> activeColors() const { return m_colors; }

    void setActive(const QString &name, const QVector<QColor> &colors) {
        if (colors.isEmpty()) return;

        m_name = name.isEmpty() ? QStringLiteral("Personalizada") : name;
        m_colors = colors;

        persist();
        emit activePaletteChanged(m_name, m_colors);
    }

    void setActiveByName(const QString &name) {
        for (const auto &p : PaletteStore::loadAll()) {
            if (p.name == name) {
                setActive(p.name, p.colors);
                return;
            }
        }

        for (const auto &p : builtins()) {
            if (p.first == name) {
                setActive(p.first, p.second);
                return;
            }
        }

        setActive(QStringLiteral("MS Paint"), PaletteStore::defaultMSPaint());
    }

    QList<QPair<QString, QVector<QColor>>> allPalettes() const {
        QList<QPair<QString, QVector<QColor>>> out = builtins();

        for (const auto &p : PaletteStore::loadAll()) {
            if (p.colors.isEmpty()) continue;

            bool replaced = false;
            for (auto &b : out) {
                if (b.first == p.name) {
                    b.second = p.colors;
                    replaced = true;
                    break;
                }
            }

            if (!replaced) out.append({ p.name, p.colors });
        }

        return out;
    }

    static QList<QPair<QString, QVector<QColor>>> builtins() {
        return {
            { QStringLiteral("MS Paint"),        PaletteStore::defaultMSPaint() },
            { QStringLiteral("Clásica artista"), PaletteStore::defaultClassicArt() },
            { QStringLiteral("Retro neón"),      PaletteStore::defaultRetroNeon() },
            { QStringLiteral("Aurora boreal"),   PaletteStore::defaultAurora() },
            { QStringLiteral("Pastel suave"),    PaletteStore::defaultPastel() },
        };
    }

signals:
    void activePaletteChanged(const QString &name, const QVector<QColor> &colors);

private:
    PaletteController() { load(); }
    PaletteController(const PaletteController &) = delete;
    PaletteController &operator=(const PaletteController &) = delete;

    QString m_name;
    QVector<QColor> m_colors;

    void load() {
        QSettings s("Paintux", "PaintuxStudio");

        m_name = s.value("palette/activeName", QStringLiteral("MS Paint")).toString();

        QVariantList stored = s.value("palette/activeColors").toList();
        QVector<QColor> fromSettings;

        for (const QVariant &v : stored) {
            QColor c(v.toString());
            if (c.isValid()) fromSettings.append(c);
        }

        for (const auto &p : PaletteStore::loadAll()) {
            if (p.name == m_name && !p.colors.isEmpty()) {
                m_colors = p.colors;
                return;
            }
        }

        for (const auto &p : builtins()) {
            if (p.first == m_name) {
                m_colors = p.second;
                return;
            }
        }

        if (!fromSettings.isEmpty()) {
            m_colors = fromSettings;
            return;
        }

        m_name = QStringLiteral("MS Paint");
        m_colors = PaletteStore::defaultMSPaint();
    }

    void persist() {
        QSettings s("Paintux", "PaintuxStudio");

        s.setValue("palette/activeName", m_name);

        QVariantList stored;
        for (const QColor &c : m_colors) stored.append(c.name(QColor::HexArgb));

        s.setValue("palette/activeColors", stored);
    }
};

// ═══════════════════════════════════════════════════════════════════════
//  PaletteEditorWindow — editor flotante de paleta
// ═══════════════════════════════════════════════════════════════════════
class PaletteEditorWindow : public QWidget, public DragHelper {
    Q_OBJECT
public:
    PaletteEditorWindow(const QString &name,
                        const QVector<QColor> &colors,
                        bool darkMode,
                        TriangleColorPicker *sharedPicker = nullptr,
                        QWidget *parent = nullptr)
        : QWidget(parent),
          m_dark(darkMode),
          m_picker(sharedPicker) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                       Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_DeleteOnClose, false);
        setMouseTracking(true);

        setWindowTitle(tr("Editar paleta"));
        setFixedSize(560, 500);

        QVBoxLayout *root = new QVBoxLayout(this);
        root->setContentsMargins(14, 12, 14, 12);
        root->setSpacing(10);
        root->addSpacing(18);

        QHBoxLayout *nameRow = new QHBoxLayout();
        nameRow->setSpacing(8);

        QLabel *lblName = new QLabel(tr("Nombre:"), this);
        lblName->setStyleSheet("color:#e5e5e5;font-size:12px;font-weight:600;background:transparent;");
        lblName->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        m_nameEdit = new QLineEdit(name, this);
        m_nameEdit->setPlaceholderText(tr("Mi paleta"));
        m_nameEdit->setStyleSheet(
            "QLineEdit{background-color:rgba(255,255,255,25);color:#e5e5e5;"
            "border:1px solid rgba(255,255,255,50);border-radius:5px;"
            "padding:5px 8px;font-size:12px;}"
            "QLineEdit:focus{border:1px solid #3b82f6;}");

        nameRow->addWidget(lblName);
        nameRow->addWidget(m_nameEdit, 1);
        root->addLayout(nameRow);

        QHBoxLayout *colorRow = new QHBoxLayout();
        colorRow->setSpacing(8);

        QLabel *lblCur = new QLabel(tr("Color actual:"), this);
        lblCur->setStyleSheet("color:#e5e5e5;font-size:12px;background:transparent;");
        lblCur->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        m_previewBtn = new QPushButton(this);
        m_previewBtn->setFixedSize(52, 28);
        m_previewBtn->setCursor(Qt::PointingHandCursor);
        m_previewBtn->setToolTip(m_picker
            ? tr("El color actual viene del triángulo del selector")
            : tr("Click para elegir un color"));

        colorRow->addWidget(lblCur);
        colorRow->addWidget(m_previewBtn);

        if (m_picker) {
            QLabel *hint = new QLabel(tr("(usa el triángulo de la ventana principal)"), this);
            hint->setStyleSheet("color:#8a8a8a;font-size:10px;background:transparent;");
            hint->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            colorRow->addWidget(hint);
        }

        colorRow->addStretch();
        root->addLayout(colorRow);

        QLabel *lblColors = new QLabel(tr("Colores de la paleta:"), this);
        lblColors->setStyleSheet("color:#b0b0b0;font-size:11px;font-weight:600;background:transparent;");
        root->addWidget(lblColors);

        m_grid = new QListWidget(this);
        m_grid->setViewMode(QListView::IconMode);
        m_grid->setIconSize(QSize(32, 32));
        m_grid->setGridSize(QSize(42, 42));
        m_grid->setResizeMode(QListView::Adjust);
        m_grid->setMovement(QListView::Static);
        m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
        m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
        m_grid->setSpacing(2);
        m_grid->setStyleSheet(
            "QListWidget{background-color:rgba(255,255,255,10);"
            "border:1px solid rgba(255,255,255,40);border-radius:6px;"
            "padding:4px;outline:none;}"
            "QListWidget::item{border-radius:4px;}"
            "QListWidget::item:selected{background:rgba(59,130,246,60);}"
            "QListWidget::item:hover{background:rgba(255,255,255,20);}");
        root->addWidget(m_grid, 1);

        QHBoxLayout *actions = new QHBoxLayout();
        actions->setSpacing(6);

        QString btnStyle =
            "QPushButton{background-color:rgba(59,130,246,40);color:#e5e5e5;"
            "border:1px solid rgba(59,130,246,80);border-radius:5px;"
            "padding:5px 12px;font-size:11px;}"
            "QPushButton:hover{background-color:rgba(59,130,246,80);border-color:#3b82f6;}";

        QPushButton *btnAdd     = new QPushButton(tr("+ Añadir"), this);
        QPushButton *btnReplace = new QPushButton(tr("Reemplazar"), this);
        QPushButton *btnRemove  = new QPushButton(tr("Quitar"), this);

        for (auto *b : { btnAdd, btnReplace, btnRemove }) {
            b->setCursor(Qt::PointingHandCursor);
            b->setStyleSheet(btnStyle);
        }

        actions->addWidget(btnAdd);
        actions->addWidget(btnReplace);
        actions->addWidget(btnRemove);
        actions->addStretch();
        root->addLayout(actions);

        QHBoxLayout *bottom = new QHBoxLayout();
        bottom->addStretch();

        QPushButton *btnCancel = new QPushButton(tr("Cancelar"), this);
        QPushButton *btnSave   = new QPushButton(tr("Guardar"), this);

        btnCancel->setFixedHeight(30);
        btnSave->setFixedHeight(30);

        btnCancel->setStyleSheet(
            "QPushButton{background-color:transparent;color:#e5e5e5;"
            "padding:5px 12px;border:1px solid rgba(255,255,255,50);"
            "border-radius:5px;font-size:12px;}"
            "QPushButton:hover{border-color:#3b82f6;}");

        btnSave->setStyleSheet(
            "QPushButton{background-color:#2563eb;color:white;font-weight:bold;"
            "padding:5px 16px;border-radius:5px;border:none;font-size:12px;}"
            "QPushButton:hover{background-color:#1d4ed8;}");
        btnSave->setDefault(true);

        bottom->addWidget(btnCancel);
        bottom->addWidget(btnSave);
        root->addLayout(bottom);

        for (const QColor &c : colors) addColorItem(c);
        if (m_grid->count() > 0) m_grid->setCurrentRow(0);

        m_currentColor = colors.isEmpty() ? Qt::black : colors.first();

        connect(btnAdd,     &QPushButton::clicked, this, &PaletteEditorWindow::onAdd);
        connect(btnReplace, &QPushButton::clicked, this, &PaletteEditorWindow::onReplace);
        connect(btnRemove,  &QPushButton::clicked, this, &PaletteEditorWindow::onRemove);
        connect(btnSave,    &QPushButton::clicked, this, &PaletteEditorWindow::onSave);
        connect(btnCancel,  &QPushButton::clicked, this, &PaletteEditorWindow::onCancel);

        connect(m_grid, &QListWidget::itemClicked, this, [this](QListWidgetItem *it) {
            if (!it) return;

            QColor c = it->data(Qt::UserRole).value<QColor>();
            if (c.isValid()) {
                m_currentColor = c;
                updatePreview();
            }
        });

        connect(m_grid, &QListWidget::customContextMenuRequested,
                this, &PaletteEditorWindow::onContextMenu);

        connect(m_previewBtn, &QPushButton::clicked, this, &PaletteEditorWindow::onPickColor);

        if (m_picker) {
            m_currentColor = m_picker->color();
            connect(m_picker, &TriangleColorPicker::colorChanged,
                    this, &PaletteEditorWindow::setCurrentColor);
        }

        updatePreview();
    }

    void setCurrentColor(const QColor &c) {
        if (!c.isValid()) return;
        m_currentColor = c;
        updatePreview();
    }

    QColor currentColor() const { return m_currentColor; }
    QString paletteName() const { return m_nameEdit->text().trimmed(); }

    QVector<QColor> paletteColors() const {
        QVector<QColor> out;
        for (int i = 0; i < m_grid->count(); ++i)
            out.append(m_grid->item(i)->data(Qt::UserRole).value<QColor>());
        return out;
    }

signals:
    void accepted(const QString &name, const QVector<QColor> &colors);
    void cancelled();

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);

        p.setPen(QPen(QColor(255, 255, 255, 45), 1));
        p.setBrush(QColor(24, 24, 28, 240));
        p.drawRoundedRect(r, 10, 10);
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(e);
            return;
        }

        QWidget *child = childAt(e->position().toPoint());
        if (child && child != this) {
            QWidget::mousePressEvent(e);
            return;
        }

        DragHelper::tryStartDrag(this, e);
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (DragHelper::handleDragMove(this, e)) return;
        QWidget::mouseMoveEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        DragHelper::endDrag(this);
        QWidget::mouseReleaseEvent(e);
    }

    void closeEvent(QCloseEvent *e) override {
        emit cancelled();
        QWidget::closeEvent(e);
    }

private:
    bool m_dark;
    TriangleColorPicker *m_picker = nullptr;

    QLineEdit *m_nameEdit = nullptr;
    QListWidget *m_grid = nullptr;
    QPushButton *m_previewBtn = nullptr;

    QColor m_currentColor = Qt::black;

    static QPixmap makeSwatchPix(const QColor &c) {
        QPixmap pm(32, 32);
        pm.fill(Qt::transparent);

        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        for (int y = 0; y < 32; y += 5) {
            for (int x = 0; x < 32; x += 5) {
                p.fillRect(x, y, 5, 5,
                           ((x / 5 + y / 5) % 2) ? QColor(200,200,200) : QColor(240,240,240));
            }
        }

        p.setBrush(c);
        p.setPen(QPen(QColor(0,0,0,90), 1));
        p.drawRoundedRect(QRectF(0.5, 0.5, 31, 31), 4, 4);
        p.end();

        return pm;
    }

    void addColorItem(const QColor &c) {
        QListWidgetItem *it = new QListWidgetItem();
        it->setData(Qt::UserRole, c);
        it->setIcon(QIcon(makeSwatchPix(c)));
        it->setToolTip(c.name(QColor::HexArgb));
        m_grid->addItem(it);
    }

    void replaceColorItem(QListWidgetItem *it, const QColor &c) {
        if (!it) return;

        it->setData(Qt::UserRole, c);
        it->setIcon(QIcon(makeSwatchPix(c)));
        it->setToolTip(c.name(QColor::HexArgb));
    }

    void updatePreview() {
        m_previewBtn->setStyleSheet(QString(
            "QPushButton{border:1px solid rgba(255,255,255,50);border-radius:4px;"
            "background-color:%1;}").arg(m_currentColor.name(QColor::HexRgb)));
    }

    void onAdd() {
        addColorItem(m_currentColor);
    }

    void onRemove() {
        QListWidgetItem *it = m_grid->currentItem();
        if (it) delete m_grid->takeItem(m_grid->row(it));
    }

    void onReplace() {
        QListWidgetItem *it = m_grid->currentItem();
        if (it) replaceColorItem(it, m_currentColor);
    }

    void onPickColor() {
        if (m_picker) return;

        QColor c = QColorDialog::getColor(m_currentColor, this, tr("Elegir color"));
        if (c.isValid()) {
            m_currentColor = c;
            updatePreview();
        }
    }

    void onContextMenu(const QPoint &pos) {
        QListWidgetItem *it = m_grid->itemAt(pos);
        if (!it) return;

        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu{background-color:#1e1e22;color:#e5e5e5;"
            "border:1px solid rgba(255,255,255,40);border-radius:5px;padding:4px;}"
            "QMenu::item{padding:5px 20px;border-radius:3px;}"
            "QMenu::item:selected{background-color:#3b82f6;}");

        QAction *aEdit = menu.addAction(tr("Reemplazar"));
        QAction *aDup  = menu.addAction(tr("Duplicar"));
        QAction *aDel  = menu.addAction(tr("Eliminar"));

        QAction *chosen = menu.exec(m_grid->mapToGlobal(pos));
        if (!chosen) return;

        if (chosen == aEdit) {
            replaceColorItem(it, m_currentColor);
        } else if (chosen == aDup) {
            addColorItem(it->data(Qt::UserRole).value<QColor>());
        } else if (chosen == aDel) {
            delete m_grid->takeItem(m_grid->row(it));
        }
    }

    void onSave() {
        const QString n = paletteName();

        if (n.isEmpty()) {
            QMessageBox::warning(this, tr("Nombre vacío"),
                                 tr("Escribe un nombre para la paleta."));
            return;
        }

        emit accepted(n, paletteColors());
        close();
    }

    void onCancel() {
        emit cancelled();
        close();
    }
};

// ═══════════════════════════════════════════════════════════════════════
//  PaletteStripWidget — solo swatches, sin menú contextual
// ═══════════════════════════════════════════════════════════════════════
class PaletteStripWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaletteStripWidget(QWidget *parent = nullptr);

    void setColumns(int c) {
        m_cols = qMax(1, c);
        updateSize();
        update();
    }

    void setCellSize(int px) {
        m_cell = qBound(8, px, 60);
        updateSize();
        update();
    }

    void setGap(int g) {
        m_gap = qMax(0, g);
        updateSize();
        update();
    }

    void setPalette(const QVector<QColor> &colors) {
        m_colors = colors;
        m_selected = -1;
        updateSize();
        update();
    }

    const QVector<QColor> &palette() const { return m_colors; }

    QColor selectedColor() const {
        if (m_selected >= 0 && m_selected < m_colors.size())
            return m_colors[m_selected];
        return QColor();
    }

    QSize sizeHint() const override;

signals:
    void colorClicked(const QColor &color);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *) override;

private:
    QVector<QColor> m_colors;

    int m_cols = 10;
    int m_cell = 20;
    int m_gap = 3;

    int m_hover = -1;
    int m_selected = -1;

    void updateSize() {
        setFixedSize(sizeHint());
    }

    QRect cellRect(int idx) const;
    int indexAt(const QPoint &p) const;

    void drawSwatch(QPainter &p,
                    const QRect &r,
                    const QColor &c,
                    bool selected,
                    bool hover);
};

inline PaletteStripWidget::PaletteStripWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_colors = PaletteStore::defaultMSPaint();
    updateSize();
}

inline QSize PaletteStripWidget::sizeHint() const {
    if (m_colors.isEmpty()) return QSize(0, 0);

    const int rows = (m_colors.size() + m_cols - 1) / m_cols;
    const int w = m_cols * (m_cell + m_gap) + m_gap;
    const int h = rows * (m_cell + m_gap) + m_gap;

    return QSize(w, h);
}

inline QRect PaletteStripWidget::cellRect(int idx) const {
    if (idx < 0 || idx >= m_colors.size()) return QRect();

    const int row = idx / m_cols;
    const int col = idx % m_cols;

    return QRect(m_gap + col * (m_cell + m_gap),
                 m_gap + row * (m_cell + m_gap),
                 m_cell,
                 m_cell);
}

inline int PaletteStripWidget::indexAt(const QPoint &p) const {
    for (int i = 0; i < m_colors.size(); ++i) {
        if (cellRect(i).contains(p)) return i;
    }
    return -1;
}

inline void PaletteStripWidget::drawSwatch(QPainter &p,
                                           const QRect &r,
                                           const QColor &c,
                                           bool selected,
                                           bool hover) {
    if (c.alpha() < 255) {
        for (int y = 0; y < r.height(); y += 5) {
            for (int x = 0; x < r.width(); x += 5) {
                p.fillRect(r.x() + x, r.y() + y, 5, 5,
                           ((x / 5 + y / 5) % 2) ? QColor(190,190,190) : QColor(230,230,230));
            }
        }
    }

    p.fillRect(r, c);

    if (selected) {
        p.setPen(QPen(QColor("#3b82f6"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0, 0, -1, -1));

        p.setPen(QPen(Qt::white, 1));
        p.drawRect(r.adjusted(1, 1, -2, -2));
    } else if (hover) {
        p.setPen(QPen(QColor("#60a5fa"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0, 0, -1, -1));
    } else {
        p.setPen(QPen(QColor(0, 0, 0, 60), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0, 0, -1, -1));
    }
}

inline void PaletteStripWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    for (int i = 0; i < m_colors.size(); ++i) {
        drawSwatch(p,
                   cellRect(i),
                   m_colors[i],
                   i == m_selected,
                   i == m_hover);
    }
}

inline void PaletteStripWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }

    const int idx = indexAt(e->pos());
    if (idx >= 0) {
        m_selected = idx;
        update();
        emit colorClicked(m_colors[idx]);
    }
}

inline void PaletteStripWidget::mouseMoveEvent(QMouseEvent *e) {
    const int idx = indexAt(e->pos());
    if (idx != m_hover) {
        m_hover = idx;
        update();
    }
}

inline void PaletteStripWidget::leaveEvent(QEvent *) {
    if (m_hover != -1) {
        m_hover = -1;
        update();
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  ColorPickerDialog — diálogo principal
// ═══════════════════════════════════════════════════════════════════════
class ColorPickerDialog : public QDialog, public DragHelper {
    Q_OBJECT
public:
    explicit ColorPickerDialog(const QColor &initial, QWidget *parent = nullptr);

    QColor primaryColor() const   { return m_slots[0]; }
    QColor secondaryColor() const { return m_slots[1]; }
    QColor selectedColor() const  { return m_slots[m_activeSlot]; }

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    struct PaletteRow {
        QString name;
        QVector<QColor> colors;
        QPushButton *radio = nullptr;
        QLabel *label = nullptr;
        PaletteStripWidget *strip = nullptr;
    };

    TriangleColorPicker *m_triangle = nullptr;

    QVector<PaletteRow> m_paletteRows;
    QVBoxLayout *m_paletteListLayout = nullptr;
    QScrollArea *m_paletteScroll = nullptr;
    QWidget *m_paletteListWidget = nullptr;

    QPushButton *m_btnNew = nullptr;
    QPushButton *m_btnEdit = nullptr;
    QPushButton *m_btnDel = nullptr;

    ColorSlotButton *m_slotBtn[2] = { nullptr, nullptr };
    QLineEdit *m_hexEdit = nullptr;
    QLabel *m_preview = nullptr;

    QColor m_slots[2];
    int m_activeSlot = 0;
    bool m_syncing = false;

    void buildUi();
    void rebuildPaletteList();
    void appendPaletteRow(const QString &name, const QVector<QColor> &colors);
    void clearPaletteList();

    void onTriangleChanged(const QColor &c);
    void onPaletteColorClicked(const QColor &c);
    void onSlotClicked(int idx);
    void onHexEdited();

    void onAddPalette();
    void onEditPalette();
    void onDeletePalette();

    void updateActiveSlotVisuals();
    void syncTriangleFromActiveSlot();
    void syncHexFromColor();
    void updatePreview();
    void refreshActiveMarkers();

    void openEditor(const QString &name, const QVector<QColor> &colors);
};

inline ColorPickerDialog::ColorPickerDialog(const QColor &initial, QWidget *parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    setWindowTitle(tr("Selector de Color"));
    setMouseTracking(true);
    setFixedSize(660, 560);

    m_slots[0] = initial.isValid() ? initial : QColor(Qt::black);
    m_slots[1] = Qt::white;

    buildUi();
    rebuildPaletteList();
    updateActiveSlotVisuals();
    syncTriangleFromActiveSlot();
    syncHexFromColor();
    updatePreview();

    connect(&PaletteController::instance(), &PaletteController::activePaletteChanged,
            this, [this](const QString &, const QVector<QColor> &) {
                refreshActiveMarkers();
            });

    if (parent) {
        QPoint p = parent->mapToGlobal(QPoint(0, 0));
        QSize ps = parent->size();

        move(p.x() + (ps.width() - width()) / 2,
             p.y() + (ps.height() - height()) / 2);
    } else if (QScreen *scr = QGuiApplication::primaryScreen()) {
        QRect g = scr->availableGeometry();
        move(g.center().x() - width() / 2, g.center().y() - height() / 2);
    }
}

inline void ColorPickerDialog::buildUi() {
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(8);
    root->addSpacing(20);

    auto styledLabel = [this](const QString &txt) -> QLabel* {
        QLabel *l = new QLabel(txt, this);
        l->setStyleSheet("color:#e5e5e5;background:transparent;font-size:11px;");
        return l;
    };

    QHBoxLayout *mainContent = new QHBoxLayout();
    mainContent->setSpacing(14);

    // ── Columna izquierda: paletas ─────────────────────────────────────
    QVBoxLayout *leftCol = new QVBoxLayout();
    leftCol->setSpacing(6);
    leftCol->setContentsMargins(0, 0, 0, 0);

    QLabel *sectionTitle = styledLabel(tr("Paletas"));
    sectionTitle->setStyleSheet(
        "color:rgba(255,255,255,180);font-size:11px;font-weight:700;"
        "letter-spacing:0.8px;background:transparent;");
    leftCol->addWidget(sectionTitle);

    m_paletteScroll = new QScrollArea(this);
    m_paletteScroll->setWidgetResizable(true);
    m_paletteScroll->setFrameShape(QFrame::NoFrame);
    m_paletteScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paletteScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_paletteScroll->setAttribute(Qt::WA_TranslucentBackground, true);
    m_paletteScroll->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    m_paletteScroll->viewport()->setAutoFillBackground(false);
    m_paletteScroll->setFixedWidth(268);
    m_paletteScroll->setStyleSheet(
        "QScrollArea{background:transparent;border:none;}"
        "QScrollArea > QWidget > QWidget{background:transparent;}"
        "QScrollBar:vertical{background:transparent;width:8px;margin:0;}"
        "QScrollBar::handle:vertical{background:rgba(255,255,255,40);"
        "border-radius:4px;min-height:20px;}"
        "QScrollBar::handle:vertical:hover{background:#3b82f6;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}");

    m_paletteListWidget = new QWidget();
    m_paletteListWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    m_paletteListWidget->setAutoFillBackground(false);

    m_paletteListLayout = new QVBoxLayout(m_paletteListWidget);
    m_paletteListLayout->setContentsMargins(0, 0, 6, 0);
    m_paletteListLayout->setSpacing(6);
    m_paletteListLayout->addStretch();

    m_paletteScroll->setWidget(m_paletteListWidget);
    leftCol->addWidget(m_paletteScroll, 1);

    QHBoxLayout *manageRow = new QHBoxLayout();
    manageRow->setSpacing(4);
    manageRow->setContentsMargins(0, 0, 0, 0);

    m_btnNew  = new QPushButton(tr("+ Nueva paleta"), this);
    m_btnEdit = new QPushButton(tr("Ajustar"), this);
    m_btnDel  = new QPushButton(tr("X"), this);

    const QString btnBase =
        "QPushButton{background-color:rgba(255,255,255,15);color:#e0e0e0;"
        "border:1px solid rgba(255,255,255,30);border-radius:6px;"
        "font-size:10px;font-weight:700;padding:0px 6px;}"
        "QPushButton:hover{background-color:rgba(59,130,246,60);"
        "border-color:#3b82f6;color:white;}";

    const QString btnDanger =
        "QPushButton{background-color:rgba(255,255,255,15);color:#e0e0e0;"
        "border:1px solid rgba(255,255,255,30);border-radius:6px;"
        "font-size:10px;font-weight:900;padding:0px 6px;}"
        "QPushButton:hover{background-color:rgba(239,68,68,60);"
        "border-color:#ef4444;color:white;}";

    for (auto *b : { m_btnNew, m_btnEdit, m_btnDel }) {
        b->setFixedHeight(26);
        b->setCursor(Qt::PointingHandCursor);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    m_btnNew->setStyleSheet(btnBase);
    m_btnEdit->setStyleSheet(btnBase);
    m_btnDel->setStyleSheet(btnDanger);

    m_btnNew->setToolTip(tr("Crear una paleta nueva"));
    m_btnEdit->setToolTip(tr("Ajustar / editar la paleta activa"));
    m_btnDel->setToolTip(tr("Eliminar la paleta activa"));

    manageRow->addWidget(m_btnNew);
    manageRow->addWidget(m_btnEdit);
    manageRow->addWidget(m_btnDel);

    leftCol->addLayout(manageRow);
    leftCol->addStretch();

    mainContent->addLayout(leftCol, 0);

    // ── Columna derecha: triángulo + preview + hex ─────────────────────
    QVBoxLayout *rightCol = new QVBoxLayout();
    rightCol->setSpacing(10);
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->addStretch();

    m_triangle = new TriangleColorPicker(this);
    m_triangle->setFixedSize(230, 230);
    rightCol->addWidget(m_triangle, 0, Qt::AlignCenter);

    QHBoxLayout *previewRow = new QHBoxLayout();
    previewRow->setSpacing(8);
    previewRow->setContentsMargins(0, 0, 0, 0);

    previewRow->addStretch();
    previewRow->addWidget(styledLabel(tr("Preview:")));

    m_preview = new QLabel(this);
    m_preview->setFixedSize(44, 26);
    m_preview->setStyleSheet("border:1px solid rgba(255,255,255,50);border-radius:4px;");
    previewRow->addWidget(m_preview);

    previewRow->addSpacing(4);
    previewRow->addWidget(styledLabel(tr("Hex:")));

    m_hexEdit = new QLineEdit(this);
    m_hexEdit->setMaxLength(9);
    m_hexEdit->setFixedWidth(90);
    m_hexEdit->setPlaceholderText("#RRGGBB");
    m_hexEdit->setStyleSheet(
        "QLineEdit{background-color:rgba(255,255,255,25);color:#e5e5e5;"
        "border:1px solid rgba(255,255,255,50);border-radius:5px;"
        "padding:3px 6px;font-size:11px;}"
        "QLineEdit:focus{border:1px solid #3b82f6;}");

    previewRow->addWidget(m_hexEdit);
    previewRow->addStretch();

    rightCol->addLayout(previewRow);
    rightCol->addStretch();

    mainContent->addLayout(rightCol, 1);
    root->addLayout(mainContent, 1);

    // ── Fila inferior: slots + OK/Cancel ───────────────────────────────
    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(8);
    bottomRow->setContentsMargins(0, 0, 0, 0);

    bottomRow->addWidget(styledLabel(tr("Slots:")));

    for (int i = 0; i < 2; ++i) {
        m_slotBtn[i] = new ColorSlotButton(i, this);
        m_slotBtn[i]->setFixedSize(28, 44);
        m_slotBtn[i]->setColor(m_slots[i]);
        bottomRow->addWidget(m_slotBtn[i]);

        connect(m_slotBtn[i], &ColorSlotButton::slotClicked,
                this, &ColorPickerDialog::onSlotClicked);
    }

    bottomRow->addStretch();

    QPushButton *btnCancel = new QPushButton(tr("Cancelar"), this);
    QPushButton *btnOk     = new QPushButton(tr("Aplicar"), this);

    btnOk->setDefault(true);

    btnCancel->setFixedHeight(30);
    btnOk->setFixedHeight(30);

    btnOk->setStyleSheet(
        "QPushButton{background-color:#2563eb;color:white;font-weight:bold;"
        "padding:5px 12px;border-radius:5px;border:none;font-size:12px;}"
        "QPushButton:hover{background-color:#1d4ed8;}");

    btnCancel->setStyleSheet(
        "QPushButton{background-color:transparent;color:#e5e5e5;"
        "padding:5px 10px;border:1px solid rgba(255,255,255,50);"
        "border-radius:5px;font-size:12px;}"
        "QPushButton:hover{border-color:#3b82f6;}");

    bottomRow->addWidget(btnCancel);
    bottomRow->addWidget(btnOk);

    root->addLayout(bottomRow);

    connect(btnOk,     &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(m_triangle, &TriangleColorPicker::colorChanged,
            this, &ColorPickerDialog::onTriangleChanged);

    connect(m_hexEdit, &QLineEdit::editingFinished,
            this, &ColorPickerDialog::onHexEdited);

    connect(m_btnNew,  &QPushButton::clicked, this, &ColorPickerDialog::onAddPalette);
    connect(m_btnEdit, &QPushButton::clicked, this, &ColorPickerDialog::onEditPalette);
    connect(m_btnDel,  &QPushButton::clicked, this, &ColorPickerDialog::onDeletePalette);
}

inline void ColorPickerDialog::clearPaletteList() {
    if (!m_paletteListLayout) return;

    while (m_paletteListLayout->count() > 1) {
        QLayoutItem *item = m_paletteListLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    m_paletteRows.clear();
}

inline void ColorPickerDialog::appendPaletteRow(const QString &name,
                                                const QVector<QColor> &colors) {
    if (!m_paletteListLayout) return;

    PaletteRow row;
    row.name = name;
    row.colors = colors;

    row.radio = new QPushButton(m_paletteListWidget);
    row.radio->setFixedSize(18, 18);
    row.radio->setCursor(Qt::PointingHandCursor);
    row.radio->setToolTip(tr("Activar esta paleta"));

    row.label = new QLabel(name, m_paletteListWidget);
    row.label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    row.label->setMaximumWidth(210);
    row.label->setToolTip(name);
    row.label->setStyleSheet(
        "color:rgba(255,255,255,220);font-size:11px;font-weight:600;"
        "background:transparent;");

    row.strip = new PaletteStripWidget(m_paletteListWidget);
    row.strip->setColumns(10);
    row.strip->setCellSize(20);
    row.strip->setGap(3);
    row.strip->setPalette(colors);

    QWidget *container = new QWidget(m_paletteListWidget);
    container->setAttribute(Qt::WA_TranslucentBackground, true);
    container->setAutoFillBackground(false);
    container->setStyleSheet(
        "QWidget{background-color:rgba(255,255,255,8);"
        "border:1px solid rgba(255,255,255,25);border-radius:8px;}");

    QVBoxLayout *box = new QVBoxLayout(container);
    box->setContentsMargins(6, 5, 6, 6);
    box->setSpacing(4);

    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(6);

    titleRow->addWidget(row.radio, 0, Qt::AlignVCenter);
    titleRow->addWidget(row.label);
    titleRow->addStretch();

    box->addLayout(titleRow);
    box->addWidget(row.strip, 0, Qt::AlignLeft);

    connect(row.radio, &QPushButton::clicked, this, [this, name, colors]() {
        PaletteController::instance().setActive(name, colors);
    });

    connect(row.strip, &PaletteStripWidget::colorClicked, this,
            [this](const QColor &c) {
                onPaletteColorClicked(c);
            });

    m_paletteListLayout->insertWidget(m_paletteListLayout->count() - 1, container);
    m_paletteRows.append(row);
}

inline void ColorPickerDialog::rebuildPaletteList() {
    clearPaletteList();

    const auto all = PaletteController::instance().allPalettes();
    for (const auto &p : all)
        appendPaletteRow(p.first, p.second);

    refreshActiveMarkers();
}

inline void ColorPickerDialog::refreshActiveMarkers() {
    const QString activeName = PaletteController::instance().activeName();

    for (PaletteRow &row : m_paletteRows) {
        if (!row.radio || !row.label) continue;

        const bool on = (row.name == activeName);

        const QString bg = row.colors.isEmpty()
            ? QStringLiteral("#888888")
            : row.colors.first().name(QColor::HexArgb);

        const QString border = on
            ? QStringLiteral("#3b82f6")
            : QStringLiteral("rgba(255,255,255,40)");

        row.radio->setStyleSheet(QString(
            "QPushButton{background-color:%1;border:2px solid %2;"
            "border-radius:4px;color:white;font-size:9px;font-weight:900;padding:0px;}")
            .arg(bg, border));

        row.radio->setText(on ? QString::fromUtf8("\xE2\x9C\x93") : QString());

        row.label->setStyleSheet(on
            ? QStringLiteral("color:#93c5fd;font-size:11px;font-weight:700;background:transparent;")
            : QStringLiteral("color:rgba(255,255,255,220);font-size:11px;font-weight:600;background:transparent;"));
    }
}

inline void ColorPickerDialog::onTriangleChanged(const QColor &c) {
    if (m_syncing) return;

    m_slots[m_activeSlot] = c;
    m_slotBtn[m_activeSlot]->setColor(c);

    syncHexFromColor();
    updatePreview();
}

inline void ColorPickerDialog::onPaletteColorClicked(const QColor &c) {
    m_syncing = true;

    m_slots[m_activeSlot] = c;
    m_slotBtn[m_activeSlot]->setColor(c);
    m_triangle->setColor(c);

    syncHexFromColor();
    updatePreview();

    m_syncing = false;
}

inline void ColorPickerDialog::onSlotClicked(int idx) {
    if (idx < 0 || idx > 1) return;

    m_activeSlot = idx;

    updateActiveSlotVisuals();
    syncTriangleFromActiveSlot();
    syncHexFromColor();
    updatePreview();
}

inline void ColorPickerDialog::onHexEdited() {
    QString text = m_hexEdit->text().trimmed();
    if (!text.startsWith('#')) text.prepend('#');

    QColor c(text);
    if (!c.isValid()) {
        syncHexFromColor();
        return;
    }

    m_syncing = true;

    m_slots[m_activeSlot] = c;
    m_slotBtn[m_activeSlot]->setColor(c);
    m_triangle->setColor(c);

    updatePreview();

    m_syncing = false;
}

inline void ColorPickerDialog::openEditor(const QString &name,
                                          const QVector<QColor> &colors) {
    auto *editor = new PaletteEditorWindow(name, colors, true, m_triangle, this);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->setCurrentColor(m_slots[m_activeSlot]);

    connect(editor, &PaletteEditorWindow::accepted, this,
            [this](const QString &n, const QVector<QColor> &c) {
                if (n.trimmed().isEmpty() || c.isEmpty()) return;

                PaletteStore::Palette p;
                p.name = n;
                p.colors = c;

                if (PaletteStore::save(p)) {
                    PaletteController::instance().setActive(n, c);
                    rebuildPaletteList();
                }
            });

    QPoint g = mapToGlobal(QPoint(width() - 20, 40));
    editor->move(g);
    editor->show();
    editor->raise();
}

inline void ColorPickerDialog::onAddPalette() {
    bool ok = false;

    QString name = QInputDialog::getText(this,
                                         tr("Nueva paleta"),
                                         tr("Nombre de la paleta:"),
                                         QLineEdit::Normal,
                                         tr("Mi paleta"),
                                         &ok);

    if (!ok || name.trimmed().isEmpty()) return;

    if (PaletteStore::exists(name)) {
        if (QMessageBox::question(this,
                                  tr("Ya existe"),
                                  tr("Ya existe '%1'. Sobrescribir?").arg(name))
            != QMessageBox::Yes) {
            return;
        }
    }

    QVector<QColor> base = PaletteController::instance().activeColors();

    while (base.size() < 20)
        base.append(Qt::white);

    if (base.size() > 20)
        base.resize(20);

    openEditor(name, base);
}

inline void ColorPickerDialog::onEditPalette() {
    const QString name = PaletteController::instance().activeName();
    const QVector<QColor> colors = PaletteController::instance().activeColors();

    if (colors.isEmpty()) return;

    openEditor(name, colors);
}

inline void ColorPickerDialog::onDeletePalette() {
    const QString name = PaletteController::instance().activeName();

    if (!PaletteStore::exists(name)) {
        QMessageBox::information(this,
                                 tr("Paleta integrada"),
                                 tr("Las paletas integradas no se pueden eliminar."));
        return;
    }

    if (QMessageBox::question(this,
                              tr("Eliminar paleta"),
                              tr("Eliminar la paleta '%1'?").arg(name))
        != QMessageBox::Yes) {
        return;
    }

    bool isBuiltin = false;
    for (const auto &b : PaletteController::builtins()) {
        if (b.first == name) {
            isBuiltin = true;
            break;
        }
    }

    PaletteStore::remove(name);

    if (isBuiltin)
        PaletteController::instance().setActiveByName(name);
    else
        PaletteController::instance().setActiveByName(QStringLiteral("MS Paint"));

    rebuildPaletteList();
}

inline void ColorPickerDialog::updateActiveSlotVisuals() {
    m_slotBtn[0]->setActive(m_activeSlot == 0);
    m_slotBtn[1]->setActive(m_activeSlot == 1);
}

inline void ColorPickerDialog::syncTriangleFromActiveSlot() {
    m_syncing = true;
    m_triangle->setColor(m_slots[m_activeSlot]);
    m_syncing = false;
}

inline void ColorPickerDialog::syncHexFromColor() {
    const QColor &c = m_slots[m_activeSlot];

    m_hexEdit->blockSignals(true);
    m_hexEdit->setText(c.name(QColor::HexRgb).toUpper());
    m_hexEdit->blockSignals(false);
}

inline void ColorPickerDialog::updatePreview() {
    const QColor &c = m_slots[m_activeSlot];

    m_preview->setStyleSheet(QString(
        "background-color:%1;border:1px solid rgba(255,255,255,50);border-radius:4px;")
        .arg(c.name(QColor::HexRgb)));
}

inline void ColorPickerDialog::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);

    QColor bg(26, 26, 30, 230);

    p.setPen(QPen(QColor(255, 255, 255, 45), 1));
    p.setBrush(bg);
    p.drawRoundedRect(r, 10, 10);

    QLinearGradient glow(r.topLeft(), r.bottomLeft());
    glow.setColorAt(0.0, QColor(255, 255, 255, 12));
    glow.setColorAt(0.35, QColor(255, 255, 255, 0));

    p.setPen(Qt::NoPen);
    p.setBrush(glow);

    QPainterPath clip;
    clip.addRoundedRect(r, 10, 10);
    p.setClipPath(clip);
    p.drawRect(r);

    p.setClipping(false);
}

inline void ColorPickerDialog::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) {
        QDialog::mousePressEvent(e);
        return;
    }

    QWidget *child = childAt(e->position().toPoint());
    if (child && child != this) {
        QDialog::mousePressEvent(e);
        return;
    }

    DragHelper::tryStartDrag(this, e);
}

inline void ColorPickerDialog::mouseMoveEvent(QMouseEvent *e) {
    if (DragHelper::handleDragMove(this, e)) return;
    QDialog::mouseMoveEvent(e);
}

inline void ColorPickerDialog::mouseReleaseEvent(QMouseEvent *e) {
    DragHelper::endDrag(this);
    QDialog::mouseReleaseEvent(e);
}

} // namespace Colrs

#endif // COLRS_H