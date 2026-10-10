#ifndef LAYERSTACK_H
#define LAYERSTACK_H

#include <QObject>
#include <QImage>
#include <QList>
#include <QMap>
#include <QSet>
#include <QRect>
#include <QSize>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QTransform>
#include <QVector>
#include <QPointF>
#include <QPixmap>
#include <QRandomGenerator>
#include "filter/ImageFilters.h"
#include "core/TileSystem.h"

struct Layer {
    QImage image;
    QString name;
    bool visible;
    double opacity;
    int blendMode;
    bool locked;

    Layer(const QImage &img = QImage(), const QString &n = "Capa")
        : image(img), name(n), visible(true), opacity(1.0), blendMode(0), locked(false) {}
};

class LayerStack {
public:
    LayerStack() = default;
    static QPainter::CompositionMode blendToCompositionMode(int m) {
        switch (m) {
            case 0:  return QPainter::CompositionMode_SourceOver;
            case 1:  return QPainter::CompositionMode_Multiply;
            case 2:  return QPainter::CompositionMode_Screen;
            case 3:  return QPainter::CompositionMode_Overlay;
            case 4:  return QPainter::CompositionMode_Darken;
            case 5:  return QPainter::CompositionMode_Lighten;
            case 6:  return QPainter::CompositionMode_ColorDodge;
            case 7:  return QPainter::CompositionMode_HardLight;
            case 8:  return QPainter::CompositionMode_SoftLight;
            case 9:  return QPainter::CompositionMode_Difference;
            case 10: return QPainter::CompositionMode_Exclusion;
            case 11: return QPainter::CompositionMode_SourceOver;
            default: return QPainter::CompositionMode_SourceOver;
        }
    }

    static QImage applyDissolve(const QImage &src, double opacity) {
        if (src.isNull()) return src;
        QImage img = src;
        if (img.format() != QImage::Format_ARGB32 &&
            img.format() != QImage::Format_ARGB32_Premultiplied) {
            img = img.convertToFormat(QImage::Format_ARGB32);
        }
        const double o = qBound(0.0, opacity, 1.0);
        if (o >= 0.999) return img;
        if (o <= 0.001) {
            QImage empty(img.size(), QImage::Format_ARGB32);
            empty.fill(Qt::transparent);
            return empty;
        }
        for (int y = 0; y < img.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                QRgb px = line[x];
                int a = qAlpha(px);
                if (a == 0) continue;
                double r = QRandomGenerator::global()->generateDouble();
                if (r < o) {
                    line[x] = qRgba(qRed(px), qGreen(px), qBlue(px), 255);
                } else {
                    line[x] = qRgba(qRed(px), qGreen(px), qBlue(px), 0);
                }
            }
        }
        return img;
    }

    void suspendCompose(bool suspend) {
        m_suspendCompose = suspend;
        if (!suspend) m_compositedValid = false;
    }
    bool isComposeSuspended() const { return m_suspendCompose; }

    QList<Layer>& layers() { return m_layers; }
    const QList<Layer>& layers() const { return m_layers; }
    int count() const { return m_layers.size(); }
    bool isEmpty() const { return m_layers.isEmpty(); }
    bool validIndex(int i) const { return i >= 0 && i < m_layers.size(); }
    Layer& layerAt(int i) { return m_layers[i]; }
    const Layer& layerAt(int i) const { return m_layers[i]; }

    int currentIndex() const { return m_current; }
    bool currentValid() const { return validIndex(m_current); }
    Layer& currentLayer() { return m_layers[m_current]; }
    const Layer& currentLayer() const { return m_layers[m_current]; }
    QImage& currentImage() { return m_layers[m_current].image; }
    bool currentLocked() const { return currentValid() && m_layers[m_current].locked; }
    void setCurrentIndex(int i) { if (validIndex(i)) m_current = i; }

    QSize canvasSize() const { return m_layers.isEmpty() ? QSize() : m_layers[0].image.size(); }
    int width() const { return canvasSize().width(); }
    int height() const { return canvasSize().height(); }
    QRect canvasRect() const { return QRect(QPoint(0, 0), canvasSize()); }

    QImage compositedImage() { ensureComposited(); return m_image; }

    void recompose() {
        if (m_suspendCompose) return;
        if (m_layers.isEmpty()) return;
        QSize s = m_layers[0].image.size();
        if (m_image.size() != s || m_image.format() != QImage::Format_ARGB32) {
            m_image = QImage(s, QImage::Format_ARGB32);
            m_image.fill(Qt::transparent);
        }
        if (!m_tiles.isValid() ||
            m_tiles.width()  != s.width() ||
            m_tiles.height() != s.height()) {
            m_tiles.init(s.width(), s.height());
        }
        m_tiles.markAllDirty();
        m_compositedValid = false;
    }

    void recomposeRegion(const QRect &dirtyCanvasRect) {
        if (m_suspendCompose) return;
        if (m_layers.isEmpty()) return;
        QSize s = m_layers[0].image.size();

        const bool sizeChanged =
            (m_image.size() != s) ||
            (m_image.format() != QImage::Format_ARGB32) ||
            !m_tiles.isValid() ||
            m_tiles.width()  != s.width() ||
            m_tiles.height() != s.height();

        if (sizeChanged) {
            m_image = QImage(s, QImage::Format_ARGB32);
            m_image.fill(Qt::transparent);
            m_tiles.init(s.width(), s.height());
            m_tiles.markAllDirty();
        } else if (!dirtyCanvasRect.isEmpty()) {
            m_tiles.markDirty(dirtyCanvasRect);
        }
        m_compositedValid = false;
    }

    void ensureComposited() {
        if (m_suspendCompose) return;
        if (m_compositedValid) return;
        if (m_layers.isEmpty()) return;
        if (m_image.size() != m_layers[0].image.size()) {
            m_image = QImage(m_layers[0].image.size(), QImage::Format_ARGB32);
            m_image.fill(Qt::transparent);
        }
        QPainter p(&m_image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(m_image.rect(), Qt::transparent);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        for (int i = 0; i < m_layers.size(); ++i) {
            const Layer &L = m_layers[i];
            if (!L.visible) continue;
            QImage layerImage = L.image;
            applyMasksToLayer(i, layerImage);
            if (L.blendMode == 11) {
                layerImage = applyDissolve(layerImage, L.opacity);
                p.setOpacity(1.0);
            } else {
                p.setOpacity(L.opacity);
            }
            p.setCompositionMode(blendToCompositionMode(L.blendMode));
            p.drawImage(0, 0, layerImage);
        }
        p.end();
        m_compositedValid = true;
    }

    QPixmap composeTilePixmap(int idx) {
        QRect tr = m_tiles.tileRect(idx);
        QImage tileImg(tr.size(), QImage::Format_ARGB32);
        tileImg.fill(Qt::transparent);
        QPainter p(&tileImg);
        for (int i = 0; i < m_layers.size(); ++i) {
            const Layer &L = m_layers[i];
            if (!L.visible) continue;
            QRect src = tr.intersected(L.image.rect());
            if (src.isEmpty()) continue;
            if (L.blendMode == 11) {
                QImage sub = L.image.copy(src);
                if (isColorMaskActive(i) || isGrayMaskActive(i))
                    sub = applyMasksToRegion(i, L.image, src);
                sub = applyDissolve(sub, L.opacity);
                p.setOpacity(1.0);
                p.setCompositionMode(QPainter::CompositionMode_SourceOver);
                p.drawImage(src.topLeft() - tr.topLeft(), sub);
            } else {
                p.setOpacity(L.opacity);
                p.setCompositionMode(blendToCompositionMode(L.blendMode));
                if (isColorMaskActive(i) || isGrayMaskActive(i)) {
                    QImage sub = applyMasksToRegion(i, L.image, src);
                    p.drawImage(src.topLeft() - tr.topLeft(), sub);
                } else {
                    p.drawImage(src.topLeft() - tr.topLeft(), L.image, src);
                }
            }
        }
        p.end();
        return QPixmap::fromImage(tileImg);
    }

    void markDirty(const QRect &r) {
        m_tiles.markDirty(r);
        m_compositedValid = false;
    }
    void markAllDirty() { m_tiles.markAllDirty(); m_compositedValid = false; }

    bool tilesValid() const { return m_tiles.isValid(); }
    QVector<int> tilesIntersecting(const QRect &r) const { return m_tiles.tilesIntersecting(r); }
    bool isTileDirty(int idx) const { return m_tiles.isDirty(idx); }
    void refreshTile(int idx) { m_tiles.setPixmap(idx, composeTilePixmap(idx)); }
    QPixmap tilePixmap(int idx) const { return m_tiles.pixmap(idx); }
    QRect tileRect(int idx) const { return m_tiles.tileRect(idx); }

    void appendFirstLayer(const QImage &img, const QString &name) {
        m_layers.append(Layer(img, name));
        m_current = 0;
        recompose();
    }

    void addLayer(const QString &name) {
        if (m_layers.isEmpty()) return;
        QImage newLayer(m_layers[0].image.size(), QImage::Format_ARGB32);
        newLayer.fill(Qt::transparent);
        m_layers.append(Layer(newLayer, name));
        m_current = m_layers.size() - 1;
        recompose();
    }

    void addLayerSilent(const QString &name) {
        if (m_layers.isEmpty()) return;
        QImage newLayer(m_layers[0].image.size(), QImage::Format_ARGB32);
        newLayer.fill(Qt::transparent);
        m_layers.append(Layer(newLayer, name));
        m_current = m_layers.size() - 1;
    }

    void addImageLayer(const QImage &img, const QString &name) {
        if (m_layers.isEmpty()) return;
        QImage newLayer(m_layers[0].image.size(), QImage::Format_ARGB32);
        newLayer.fill(Qt::transparent);
        QPainter p(&newLayer);
        p.drawImage((newLayer.width() - img.width()) / 2,
                    (newLayer.height() - img.height()) / 2, img);
        p.end();
        m_layers.append(Layer(newLayer, name));
        m_current = m_layers.size() - 1;
        recompose();
    }

    bool duplicateCurrent() {
        if (m_layers.isEmpty()) return false;
        Layer dup = m_layers[m_current];
        dup.name += QObject::tr(" (copia)");
        m_layers.insert(m_current + 1, dup);
        duplicateMaskData(m_current, m_current + 1);
        m_current++;
        invalidateMaskCaches();
        recompose();
        return true;
    }

    bool deleteCurrent() {
        if (m_layers.size() <= 1) return false;
        removeLayerMasksAt(m_current);
        m_layers.removeAt(m_current);
        if (m_current >= m_layers.size()) m_current = m_layers.size() - 1;
        recompose();
        return true;
    }

    bool mergeDown() {
        if (m_current <= 0) return false;
        QImage upperImage = m_layers[m_current].image;
        applyCommittedMasks(m_current, upperImage);
        QPainter painter(&m_layers[m_current - 1].image);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setOpacity(m_layers[m_current].opacity);
        painter.drawImage(0, 0, upperImage);
        painter.end();
        removeLayerMasksAt(m_current);
        m_layers.removeAt(m_current);
        m_current--;
        recompose();
        return true;
    }

    bool moveCurrentUp() {
        if (m_current >= m_layers.size() - 1) return false;
        m_layers.swapItemsAt(m_current, m_current + 1);
        swapMaskData(m_current, m_current + 1);
        m_current++;
        recompose();
        return true;
    }

    bool moveCurrentDown() {
        if (m_current <= 0) return false;
        m_layers.swapItemsAt(m_current, m_current - 1);
        swapMaskData(m_current, m_current - 1);
        m_current--;
        recompose();
        return true;
    }

    bool reorder(int fromIndex, int toIndex) {
        if (!validIndex(fromIndex) || !validIndex(toIndex) || fromIndex == toIndex) return false;
        Layer layer = m_layers.takeAt(fromIndex);
        m_layers.insert(toIndex, layer);
        reorderMaskData(fromIndex, toIndex);
        if (m_current == fromIndex) m_current = toIndex;
        else {
            if (fromIndex < m_current && toIndex >= m_current) m_current--;
            else if (fromIndex > m_current && toIndex <= m_current) m_current++;
        }
        recompose();
        return true;
    }

    void setVisibility(int i, bool v) {
        if (validIndex(i)) { m_layers[i].visible = v; recompose(); }
    }
    void setLayerOpacity(int i, double o) {
        if (validIndex(i)) { m_layers[i].opacity = qBound(0.0, o, 1.0); recompose(); }
    }
    void setBlendMode(int i, int m) {
        if (validIndex(i)) { m_layers[i].blendMode = m; recompose(); }
    }
    void setLocked(int i, bool l) {
        if (validIndex(i)) m_layers[i].locked = l;
    }

    void flipCurrent(bool horizontal, bool vertical) {
        if (!currentValid()) return;
        m_layers[m_current].image = m_layers[m_current].image.mirrored(horizontal, vertical);
        flipMask(m_current, horizontal, vertical);
        recompose();
    }

    void rotateCurrent(int angle) {
        if (!currentValid()) return;
        QTransform transform; transform.rotate(angle);
        QImage rotated = m_layers[m_current].image.transformed(transform, Qt::SmoothTransformation);
        QImage newImg(m_layers[m_current].image.size(), QImage::Format_ARGB32);
        newImg.fill(Qt::transparent);
        QPainter p(&newImg);
        p.drawImage((newImg.width() - rotated.width()) / 2,
                    (newImg.height() - rotated.height()) / 2, rotated);
        p.end();
        m_layers[m_current].image = newImg;
        rotateMask(m_current, angle);
        recompose();
    }

    void resizeCanvas(int nuevoW, int nuevoH) {
        for (Layer &layer : m_layers) {
            QImage nueva(nuevoW, nuevoH, QImage::Format_ARGB32);
            nueva.fill(Qt::transparent);
            QPainter p(&nueva); p.drawImage(0, 0, layer.image); p.end();
            layer.image = nueva;
        }
        resizeAllMasks(QSize(nuevoW, nuevoH));
        recompose();
    }

    void clearEverything() {
        m_layers.clear();
        clearAllMasks();
        m_current = 0;
        m_compositedValid = false;
    }

    void resetWithSingleLayer(const QImage &img, const QString &name) {
        clearAllMasks();
        m_layers.clear();
        m_layers.append(Layer(img, name));
        m_current = 0;
        recompose();
        ensureComposited();
    }

    bool hasMask(int layerIndex) const { return m_masks.contains(layerIndex); }
    bool isMaskEnabled(int layerIndex) const { return m_masksEnabled.contains(layerIndex); }
    bool isGrayMaskActive(int layerIndex) const {
        return m_masksEnabled.contains(layerIndex) && m_masks.contains(layerIndex);
    }
    QImage& maskRef(int layerIndex) { return m_masks[layerIndex]; }
    QImage mask(int layerIndex) const { return m_masks.value(layerIndex); }

    int maskEditLayer() const { return m_maskEditLayer; }
    bool isEditingMask() const {
        return m_maskEditLayer >= 0 && m_masks.contains(m_maskEditLayer);
    }

    void addMask(int layerIndex, const QSize &size) {
        if (!m_masks.contains(layerIndex)) {
            QImage mask(size, QImage::Format_Grayscale8);
            mask.fill(255);
            m_masks.insert(layerIndex, mask);
            m_masksEnabled.insert(layerIndex);
        }
        m_maskEditLayer = layerIndex;
        invalidateMaskCaches();
    }

    void removeMask(int layerIndex) {
        if (!m_masks.contains(layerIndex)) return;
        m_masks.remove(layerIndex);
        m_masksEnabled.remove(layerIndex);
        if (m_maskEditLayer == layerIndex) m_maskEditLayer = -1;
        invalidateMaskCaches();
    }

    bool toggleMaskEnabled(int layerIndex) {
        if (!m_masks.contains(layerIndex)) return false;
        if (m_masksEnabled.contains(layerIndex)) {
            m_masksEnabled.remove(layerIndex);
            invalidateMaskCaches();
            return false;
        }
        m_masksEnabled.insert(layerIndex);
        invalidateMaskCaches();
        return true;
    }

    void invertMask(int layerIndex) {
        if (!m_masks.contains(layerIndex)) return;
        QImage &m = m_masks[layerIndex];
        if (m.format() == QImage::Format_Grayscale8) {
            for (int y = 0; y < m.height(); ++y) {
                uchar *line = m.scanLine(y);
                for (int x = 0; x < m.width(); ++x) line[x] = 255 - line[x];
            }
        } else {
            for (int y = 0; y < m.height(); ++y) {
                QRgb *line = (QRgb*)m.scanLine(y);
                for (int x = 0; x < m.width(); ++x) {
                    int g = 255 - qGray(line[x]);
                    line[x] = qRgba(g, g, g, 255);
                }
            }
        }
        invalidateMaskCaches();
    }

    void flipMask(int layerIndex, bool horizontal, bool vertical) {
        if (!m_masks.contains(layerIndex)) return;
        m_masks[layerIndex] = m_masks[layerIndex].mirrored(horizontal, vertical);
        invalidateMaskCaches();
    }

    void rotateMask(int layerIndex, int angle) {
        if (!m_masks.contains(layerIndex)) return;
        QImage &old = m_masks[layerIndex];
        QTransform transform; transform.rotate(angle);
        QImage mRot = old.transformed(transform, Qt::SmoothTransformation);
        QImage newMask(old.size(), QImage::Format_Grayscale8);
        newMask.fill(255);
        QPainter mp(&newMask);
        mp.drawImage((newMask.width() - mRot.width()) / 2,
                     (newMask.height() - mRot.height()) / 2, mRot);
        mp.end();
        m_masks[layerIndex] = newMask;
        invalidateMaskCaches();
    }

    QImage applyMaskAndRemove(int layerIndex, const QImage &src) {
        if (!m_masks.contains(layerIndex)) return src;
        QImage result = applyMaskGrayToImage(src, m_masks[layerIndex]);
        removeMask(layerIndex);
        return result;
    }

    void setEditLayer(int layerIndex) {
        if (!m_masks.contains(layerIndex)) return;
        m_maskEditLayer = layerIndex;
        invalidateMaskCaches();
    }

    void exitMaskEdit() {
        m_maskEditLayer = -1;
        invalidateMaskCaches();
    }

    void invalidateMaskCaches() {
        m_maskPreviewCache.clear();
        m_rubylithCache = QImage();
    }
    void clearPreviewCache() { m_maskPreviewCache.clear(); }

    QImage getMaskPreview(int layerIndex) const {
        if (!m_masks.contains(layerIndex)) return QImage();
        if (m_maskPreviewCache.contains(layerIndex)) return m_maskPreviewCache.value(layerIndex);
        QImage disp = m_masks[layerIndex].convertToFormat(QImage::Format_RGB32);
        m_maskPreviewCache.insert(layerIndex, disp);
        return disp;
    }

    const QImage& ensureRubylith(int layerIndex, const QSize &canvasSize) {
        if (m_masks.contains(layerIndex) &&
            (m_rubylithCache.isNull() || m_rubylithCache.size() != canvasSize)) {
            m_rubylithCache = buildRubylith(m_masks[layerIndex]);
        }
        return m_rubylithCache;
    }

    void syncRubylithRegion(int layerIndex, const QRect &region) {
        if (!m_masks.contains(layerIndex)) return;
        const QImage &gray = m_masks[layerIndex];
        if (m_rubylithCache.isNull() || m_rubylithCache.size() != gray.size()) return;
        if (m_rubylithCache.format() != QImage::Format_ARGB32)
            m_rubylithCache = m_rubylithCache.convertToFormat(QImage::Format_ARGB32);
        QRect r = region.intersected(gray.rect());
        if (r.isEmpty()) return;

        const bool gray8 = (gray.format() == QImage::Format_Grayscale8);
        for (int y = r.top(); y <= r.bottom(); ++y) {
            const uchar *srcU = gray.constScanLine(y);
            const QRgb *srcR = reinterpret_cast<const QRgb*>(srcU);
            QRgb *dst = (QRgb*)m_rubylithCache.scanLine(y);
            for (int x = r.left(); x <= r.right(); ++x) {
                int lum = gray8 ? srcU[x] : qGray(srcR[x]);
                dst[x] = qRgba(220, 40, 60, (255 - lum) / 2);
            }
        }
    }

    bool hasColorMask(int layerIndex) const { return m_colorMaskParams.contains(layerIndex); }
    bool isColorMaskEnabled(int layerIndex) const { return m_colorMasksEnabled.contains(layerIndex); }
    bool hasLiveColorMaskPreview(int layerIndex) const { return m_liveColorPreview.contains(layerIndex); }

    bool isColorMaskActive(int layerIndex) const {
        if (m_liveColorPreview.contains(layerIndex)) return true;
        return m_colorMasksEnabled.contains(layerIndex) && m_colorMaskParams.contains(layerIndex);
    }

    FilterParams colorMaskParams(int layerIndex) const {
        return m_colorMaskParams.value(layerIndex);
    }

    FilterParams activeColorMaskParams(int layerIndex) const {
        if (m_liveColorPreview.contains(layerIndex)) return m_liveColorPreview.value(layerIndex);
        return m_colorMaskParams.value(layerIndex);
    }

    void addColorMask(int layerIndex, const FilterParams &fp) {
        m_colorMaskParams[layerIndex] = fp;
        m_colorMasksEnabled.insert(layerIndex);
        m_liveColorPreview.remove(layerIndex);
    }

    void removeColorMask(int layerIndex) {
        m_colorMaskParams.remove(layerIndex);
        m_colorMasksEnabled.remove(layerIndex);
        m_liveColorPreview.remove(layerIndex);
    }

    bool toggleColorMaskEnabled(int layerIndex) {
        if (!m_colorMaskParams.contains(layerIndex)) return false;
        if (m_colorMasksEnabled.contains(layerIndex)) {
            m_colorMasksEnabled.remove(layerIndex);
            return false;
        }
        m_colorMasksEnabled.insert(layerIndex);
        return true;
    }

    void setLiveColorMaskPreview(int layerIndex, const FilterParams &fp) {
        m_liveColorPreview[layerIndex] = fp;
    }

    void clearLiveColorMaskPreview(int layerIndex) {
        m_liveColorPreview.remove(layerIndex);
    }

    QImage getColorMaskPreview(int layerIndex, const QImage &layerImage) const {
        if (!m_colorMaskParams.contains(layerIndex)) return QImage();
        QImage small = layerImage.scaled(76, 76, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                                 .convertToFormat(QImage::Format_ARGB32);
        ImageFiltersDialog::applyFilterParams(small, m_colorMaskParams.value(layerIndex));
        return small;
    }

    void applyMasksToLayer(int layerIndex, QImage &layerImage) {
        if (isColorMaskActive(layerIndex)) {
            FilterParams fp = activeColorMaskParams(layerIndex);
            if (!fp.isIdentity())
                ImageFiltersDialog::applyFilterParams(layerImage, fp);
        }
        if (isGrayMaskActive(layerIndex)) {
            applyMaskGrayInPlace(layerImage, m_masks[layerIndex]);
        }
    }

    void applyCommittedMasks(int layerIndex, QImage &layerImage) {
        if (m_colorMasksEnabled.contains(layerIndex) && m_colorMaskParams.contains(layerIndex)) {
            FilterParams fp = m_colorMaskParams.value(layerIndex);
            if (!fp.isIdentity())
                ImageFiltersDialog::applyFilterParams(layerImage, fp);
        }
        if (isGrayMaskActive(layerIndex)) {
            applyMaskGrayInPlace(layerImage, m_masks[layerIndex]);
        }
    }

    QImage applyMasksToRegion(int layerIndex, const QImage &layerImage, const QRect &src) {
        QImage sub = layerImage.copy(src);
        if (isColorMaskActive(layerIndex)) {
            FilterParams fp = activeColorMaskParams(layerIndex);
            if (!fp.isIdentity())
                ImageFiltersDialog::applyFilterParams(sub, fp, src.topLeft(), layerImage.size());
        }
        if (isGrayMaskActive(layerIndex)) {
            QImage subMask = m_masks[layerIndex].copy(src);
            applyMaskGrayInPlace(sub, subMask);
        }
        return sub;
    }

private:

    static void applyMaskGrayInPlace(QImage &result, const QImage &grayMask) {
        if (grayMask.isNull() || grayMask.size() != result.size()) return;
        if (result.format() != QImage::Format_ARGB32 &&
            result.format() != QImage::Format_ARGB32_Premultiplied) {
            result = result.convertToFormat(QImage::Format_ARGB32);
        }
        const bool gray8 = (grayMask.format() == QImage::Format_Grayscale8);
        const int w = result.width();
        const int h = result.height();
        for (int y = 0; y < h; ++y) {
            QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
            const uchar *srcU = grayMask.constScanLine(y);
            const QRgb *srcR = reinterpret_cast<const QRgb*>(srcU);
            for (int x = 0; x < w; ++x) {
                const int g = gray8 ? srcU[x] : qGray(srcR[x]);
                const QRgb px = dst[x];
                dst[x] = qRgba(qRed(px), qGreen(px), qBlue(px),
                               (qAlpha(px) * g) / 255);
            }
        }
    }

    static QImage applyMaskGrayToImage(const QImage &src, const QImage &grayMask) {
        if (grayMask.isNull() || grayMask.size() != src.size()) return src;
        QImage result = src.copy();
        applyMaskGrayInPlace(result, grayMask);
        return result;
    }

    static QImage buildRubylith(const QImage &gray) {
        QImage ov(gray.size(), QImage::Format_ARGB32);
        const bool gray8 = (gray.format() == QImage::Format_Grayscale8);
        for (int y = 0; y < gray.height(); ++y) {
            const uchar *srcU = gray.constScanLine(y);
            const QRgb *srcR = reinterpret_cast<const QRgb*>(srcU);
            QRgb *dst = (QRgb*)ov.scanLine(y);
            for (int x = 0; x < gray.width(); ++x) {
                int lum = gray8 ? srcU[x] : qGray(srcR[x]);
                int a = (255 - lum) / 2;
                dst[x] = qRgba(220, 40, 60, a);
            }
        }
        return ov;
    }

    void duplicateMaskData(int fromIndex, int toIndex) {
        if (m_masks.contains(fromIndex)) {
            m_masks.insert(toIndex, m_masks.value(fromIndex).copy());
            if (m_masksEnabled.contains(fromIndex)) m_masksEnabled.insert(toIndex);
        }
        if (m_colorMaskParams.contains(fromIndex)) {
            m_colorMaskParams.insert(toIndex, m_colorMaskParams.value(fromIndex));
            if (m_colorMasksEnabled.contains(fromIndex)) m_colorMasksEnabled.insert(toIndex);
        }
    }

    void removeLayerMasksAt(int layerIndex) {
        m_masks.remove(layerIndex);
        m_masksEnabled.remove(layerIndex);
        m_colorMaskParams.remove(layerIndex);
        m_colorMasksEnabled.remove(layerIndex);
        m_liveColorPreview.remove(layerIndex);
        if (m_maskEditLayer == layerIndex) m_maskEditLayer = -1;

        QMap<int, QImage> nm; QSet<int> ne;
        for (auto it = m_masks.constBegin(); it != m_masks.constEnd(); ++it)
            nm.insert((it.key() > layerIndex) ? it.key() - 1 : it.key(), it.value());
        for (int i : m_masksEnabled) ne.insert((i > layerIndex) ? i - 1 : i);
        m_masks = nm; m_masksEnabled = ne;

        QMap<int, FilterParams> ncm; QSet<int> nce;
        for (auto it = m_colorMaskParams.constBegin(); it != m_colorMaskParams.constEnd(); ++it)
            ncm.insert((it.key() > layerIndex) ? it.key() - 1 : it.key(), it.value());
        for (int i : m_colorMasksEnabled) nce.insert((i > layerIndex) ? i - 1 : i);
        m_colorMaskParams = ncm; m_colorMasksEnabled = nce;

        QMap<int, FilterParams> nlp;
        for (auto it = m_liveColorPreview.constBegin(); it != m_liveColorPreview.constEnd(); ++it)
            nlp.insert((it.key() > layerIndex) ? it.key() - 1 : it.key(), it.value());
        m_liveColorPreview = nlp;

        if (m_maskEditLayer > layerIndex) m_maskEditLayer--;
        invalidateMaskCaches();
    }

    void swapMaskData(int a, int b) {
        bool ha = m_masks.contains(a), hb = m_masks.contains(b);
        QImage va, vb;
        if (ha) va = m_masks.take(a);
        if (hb) vb = m_masks.take(b);
        if (ha) m_masks.insert(b, va);
        if (hb) m_masks.insert(a, vb);

        bool ea = m_masksEnabled.contains(a), eb = m_masksEnabled.contains(b);
        m_masksEnabled.remove(a); m_masksEnabled.remove(b);
        if (ea) m_masksEnabled.insert(b);
        if (eb) m_masksEnabled.insert(a);

        bool ca = m_colorMaskParams.contains(a), cb = m_colorMaskParams.contains(b);
        FilterParams cva, cvb;
        if (ca) cva = m_colorMaskParams.take(a);
        if (cb) cvb = m_colorMaskParams.take(b);
        if (ca) m_colorMaskParams.insert(b, cva);
        if (cb) m_colorMaskParams.insert(a, cvb);

        bool eca = m_colorMasksEnabled.contains(a), ecb = m_colorMasksEnabled.contains(b);
        m_colorMasksEnabled.remove(a); m_colorMasksEnabled.remove(b);
        if (eca) m_colorMasksEnabled.insert(b);
        if (ecb) m_colorMasksEnabled.insert(a);

        bool lpa = m_liveColorPreview.contains(a), lpb = m_liveColorPreview.contains(b);
        FilterParams lpva, lpvb;
        if (lpa) lpva = m_liveColorPreview.take(a);
        if (lpb) lpvb = m_liveColorPreview.take(b);
        if (lpa) m_liveColorPreview.insert(b, lpva);
        if (lpb) m_liveColorPreview.insert(a, lpvb);

        if (m_maskEditLayer == a) m_maskEditLayer = b;
        else if (m_maskEditLayer == b) m_maskEditLayer = a;

        invalidateMaskCaches();
    }

    void reorderMaskData(int fromIndex, int toIndex) {
        auto mapNew = [fromIndex, toIndex](int i) {
            if (i == fromIndex) return toIndex;
            if (fromIndex < i && i <= toIndex) return i - 1;
            if (toIndex <= i && i < fromIndex) return i + 1;
            return i;
        };

        QMap<int, QImage> nm; QSet<int> ne;
        for (auto it = m_masks.constBegin(); it != m_masks.constEnd(); ++it)
            nm.insert(mapNew(it.key()), it.value());
        for (int i : m_masksEnabled) ne.insert(mapNew(i));
        m_masks = nm; m_masksEnabled = ne;

        QMap<int, FilterParams> ncm; QSet<int> nce;
        for (auto it = m_colorMaskParams.constBegin(); it != m_colorMaskParams.constEnd(); ++it)
            ncm.insert(mapNew(it.key()), it.value());
        for (int i : m_colorMasksEnabled) nce.insert(mapNew(i));
        m_colorMaskParams = ncm; m_colorMasksEnabled = nce;

        QMap<int, FilterParams> nlp;
        for (auto it = m_liveColorPreview.constBegin(); it != m_liveColorPreview.constEnd(); ++it)
            nlp.insert(mapNew(it.key()), it.value());
        m_liveColorPreview = nlp;

        if (m_maskEditLayer >= 0) m_maskEditLayer = mapNew(m_maskEditLayer);
        invalidateMaskCaches();
    }

    void resizeAllMasks(const QSize &newSize) {
        for (auto it = m_masks.begin(); it != m_masks.end(); ++it) {
            QImage nm(newSize, QImage::Format_Grayscale8);
            nm.fill(255);
            QPainter mp(&nm);
            mp.drawImage(0, 0, it.value());
            mp.end();
            it.value() = nm;
        }
        invalidateMaskCaches();
    }

    void clearAllMasks() {
        m_masks.clear();
        m_masksEnabled.clear();
        m_colorMaskParams.clear();
        m_colorMasksEnabled.clear();
        m_liveColorPreview.clear();
        m_maskEditLayer = -1;
        invalidateMaskCaches();
    }

    QList<Layer> m_layers;
    int m_current = 0;
    QImage m_image;
    bool m_compositedValid = false;
    bool m_suspendCompose = false;
    TilCach m_tiles;

    QMap<int, QImage> m_masks;
    QSet<int> m_masksEnabled;
    int m_maskEditLayer = -1;

    QMap<int, FilterParams> m_colorMaskParams;
    QSet<int> m_colorMasksEnabled;
    QMap<int, FilterParams> m_liveColorPreview;

    mutable QMap<int, QImage> m_maskPreviewCache;
    mutable QImage m_rubylithCache;
};

#endif // LAYERSTACK_H
