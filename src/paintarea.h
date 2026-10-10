#ifndef PAINTAREA_H
#define PAINTAREA_H

#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QPoint>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QToolButton>
#include <QFrame>
#include <QCursor>
#include <QRandomGenerator>
#include <QButtonGroup>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QCheckBox>
#include <QSpinBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QFile>
#include <QDir>
#include <QUuid>
#include <QFileInfo>
#include <QCoreApplication>
#include <QHash>
#include <vector>
#include <cmath>
#include <cstring>
#include <memory>
#include <functional>
#include <QTransform>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QConicalGradient>
#include <QGradient>
#include <QSvgGenerator>
#include <QPaintEvent>
#include <algorithm>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QDrag>
#include <QSet>
#include <QColor>
#include <QMap>
#include <QClipboard>
#include <QApplication>

#include "core/ToolManager.h"
#include "tools/PixelArtTools.h"
#include "tools/PixelAnimationTools.h"
#include "tools/LassoTools.h"
#include "filter/ImageFilters.h"
#include "tools/RetouchTools.h"
#include "core/LayerStack.h"
#include "core/CustomBrushes.h"
#include "core/ShapeObjects.h"
#include "tools/TextEngine.h"
#include "core/MaskEditController.h"
#include "tools/GradientTools.h"
#include "tools/MagicWandTools.h"
#include "tools/DeformTools.h"
#include "tools/SelectTools.h"
#include "tools/BezierPathTool.h"
#include "utils/ExifLoader.h"

class mainwind;

class FrameThumbnail : public QFrame {
    Q_OBJECT
private:
    QImage frameImage;
    bool isSelected;
    int frameIndex;
    static const int THUMB_SIZE = 48;
public:
    FrameThumbnail(const QImage &img, int index, QWidget *parent = nullptr);
    void setSelected(bool selected);
    void setFrameImage(const QImage &img);
    int getFrameIndex() const;
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
signals:
    void clicked(int frameIndex);
};

enum class ToolAction { Press, Move, Release };

struct ToolCtx {
    ToolAction action = ToolAction::Press;
    QMouseEvent *mouse = nullptr;
    QKeyEvent   *key   = nullptr;
    QPoint  pos;
    QPoint  rawPos;
    Qt::MouseButton button = Qt::NoButton;
    bool shift = false;
    bool ctrl  = false;
    bool alt   = false;
    bool continuous = false;
    int  scaledWidth = 1;
};

class DualUndoManager {
public:
    struct Config {
        int ramStates     = 5;
        int maxDiskStates = 50;
        int zlibLevel     = 1;
    };

    struct Entry {
        QImage  ramImage;
        QString diskPath;
        int     layerIndex = 0;
        QString label;
        bool hasRam()   const { return !ramImage.isNull(); }
        bool hasDisk()  const { return !diskPath.isEmpty(); }
    };

    struct Result {
        QImage  image;
        int     layerIndex = 0;
        QString label;
        bool    valid = false;
    };

    DualUndoManager() {
        m_sessionDir = QDir::tempPath()
                     + QStringLiteral("/paintlux-undo-")
                     + QString::number(QCoreApplication::applicationPid())
                     + QStringLiteral("-")
                     + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QDir().mkpath(m_sessionDir);
    }

    ~DualUndoManager() { clearAndRemoveDir(); }

    DualUndoManager(const DualUndoManager&) = delete;
    DualUndoManager& operator=(const DualUndoManager&) = delete;

    void clear() {
        for (auto &e : m_undoStack) removeDiskFile(e);
        for (auto &e : m_redoStack) removeDiskFile(e);
        m_undoStack.clear();
        m_redoStack.clear();
        QDir dir(m_sessionDir);
        if (dir.exists()) {
            for (const QFileInfo &fi : dir.entryInfoList({"*.z"}, QDir::Files))
                QFile::remove(fi.absoluteFilePath());
        }
    }

    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }
    int  undoCount() const { return m_undoStack.size(); }
    int  redoCount() const { return m_redoStack.size(); }

    int  ramCacheCount() const {
        int n = 0;
        for (const auto &e : m_undoStack) if (e.hasRam()) ++n;
        for (const auto &e : m_redoStack) if (e.hasRam()) ++n;
        return n;
    }

    int  maxStates() const { return m_cfg.maxDiskStates; }
    int  ramStates() const { return m_cfg.ramStates; }

    QString nextUndoLabel() const {
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

    void push(const QImage &img, int layerIndex, const QString &label) {
        if (img.isNull() || layerIndex < 0) return;
        truncateRedo();
        Entry e;
        e.ramImage   = toArgb32(img);
        e.layerIndex = layerIndex;
        e.label      = label;
        m_undoStack.append(e);
        enforceRamLimit();
        enforceDiskLimit();
    }

    Result undo(const QImage &currentImg, int currentLayerIndex, const QString &currentLabel) {
        Result r;
        if (m_undoStack.isEmpty()) return r;
        if (!currentImg.isNull() && currentLayerIndex >= 0) {
            Entry e;
            e.ramImage   = toArgb32(currentImg);
            e.layerIndex = currentLayerIndex;
            e.label      = currentLabel.isEmpty() ? QStringLiteral("Cambio") : currentLabel;
            m_redoStack.append(e);
            enforceRamLimit();
        }
        Entry e = m_undoStack.takeLast();
        r.image      = loadEntryImage(e);
        r.layerIndex = e.layerIndex;
        r.label      = e.label;
        r.valid      = !r.image.isNull();
        removeDiskFile(e);
        return r;
    }

    Result redo(const QImage &currentImg, int currentLayerIndex, const QString &currentLabel) {
        Result r;
        if (m_redoStack.isEmpty()) return r;
        if (!currentImg.isNull() && currentLayerIndex >= 0) {
            Entry e;
            e.ramImage   = toArgb32(currentImg);
            e.layerIndex = currentLayerIndex;
            e.label      = currentLabel.isEmpty() ? QStringLiteral("Cambio") : currentLabel;
            m_undoStack.append(e);
            enforceRamLimit();
        }
        Entry e = m_redoStack.takeLast();
        r.image      = loadEntryImage(e);
        r.layerIndex = e.layerIndex;
        r.label      = e.label;
        r.valid      = !r.image.isNull();
        removeDiskFile(e);
        return r;
    }

private:
    Config m_cfg;
    QString m_sessionDir;
    quint64 m_nextId = 0;
    QVector<Entry> m_undoStack;
    QVector<Entry> m_redoStack;

    void clearAndRemoveDir() {
        clear();
        QDir dir(m_sessionDir);
        if (dir.exists()) dir.removeRecursively();
    }

    QString nextPath() {
        return m_sessionDir + QStringLiteral("/%1.z")
               .arg(m_nextId++, 8, 10, QChar('0'));
    }

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

    void evictToDisk(Entry &e) {
        if (!e.hasRam()) return;
        if (e.diskPath.isEmpty()) e.diskPath = nextPath();
        QByteArray raw = serializeRaw(e.ramImage);
        QByteArray compressed = qCompress(raw, m_cfg.zlibLevel);
        QFile f(e.diskPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(compressed);
            f.close();
        }
        e.ramImage = QImage();
    }

    QImage loadEntryImage(const Entry &e) {
        if (e.hasRam()) return e.ramImage;
        if (!e.hasDisk()) return QImage();
        QFile f(e.diskPath);
        if (!f.open(QIODevice::ReadOnly)) return QImage();
        QByteArray compressed = f.readAll();
        f.close();
        return deserializeRaw(qUncompress(compressed));
    }

    void removeDiskFile(const Entry &e) {
        if (e.hasDisk()) QFile::remove(e.diskPath);
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
                if (e.hasRam() && m_undoStack.size() > 1) {
                    evictToDisk(e);
                    --total;
                    evicted = true;
                    break;
                }
            }
            if (!evicted) {
                for (auto &e : m_redoStack) {
                    if (e.hasRam()) {
                        evictToDisk(e);
                        --total;
                        evicted = true;
                        break;
                    }
                }
            }
            if (!evicted) break;
        }
    }
};

class PaintArea : public QWidget {
    Q_OBJECT
public:
    explicit PaintArea(QWidget *parent = nullptr);

    LayerStack& getLayerStack() { return stack; }
    const LayerStack& getLayerStack() const { return stack; }
    SelectionManager& getSelectionManager() { return selMgr; }
    const SelectionManager& getSelectionManager() const { return selMgr; }
    PixelAnimationManager& getAnimationManager() { return animManager; }
    const PixelAnimationManager& getAnimationManager() const { return animManager; }

    void setActiveColorTarget(int target);
    void setMouseSensitivity(double sens);
    double getMouseSensitivity() const;
    void setDeformOptc(const DeformOptc &o) { m_deform.setOptions(o); }
    DeformOptc getDeformOptc() const { return m_deform.options(); }
    void bakeActivePath();

    void setCustomBrushPresets(const BrushSettings &p1, const BrushSettings &p2, int activeIndex);
    BrushSettings getCustomBrush(int index) const;
    int getActiveCustomBrushIndex() const;

    bool hasLayerMask(int layerIndex) const;
    bool isLayerMaskEnabled(int layerIndex) const;
    int getMaskEditLayer() const;
    bool isEditingMask() const;
    QImage getMaskPreview(int layerIndex) const;
    void addLayerMask(int layerIndex);
    void removeLayerMask(int layerIndex);
    void toggleLayerMaskEnabled(int layerIndex);
    void selectMaskForEditing(int layerIndex);
    void selectLayerContentForEditing();
    void applyMaskToLayer(int layerIndex);
    void invertLayerMask(int layerIndex);

    bool hasLayerColorMask(int layerIndex) const;
    bool isLayerColorMaskEnabled(int layerIndex) const;
    FilterParams getLayerColorMaskParams(int layerIndex) const;
    QImage getColorMaskPreview(int layerIndex) const;
    void addLayerColorMask(int layerIndex, const FilterParams &fp);
    void removeLayerColorMask(int layerIndex);
    void toggleLayerColorMaskEnabled(int layerIndex);
    void setLiveColorMaskPreview(int layerIndex, const FilterParams &fp);
    void clearLiveColorMaskPreview(int layerIndex);

    void addLayer();
    void addImageLayer(const QImage &img);
    void insertImageAsObject(const QImage &img);
    void duplicateLayer();
    void deleteLayer();
    void mergeDown();
    void moveLayerUp();
    void moveLayerDown();
    void reorderLayer(int fromIndex, int toIndex);
    void setCurrentLayer(int index);
    void setLayerVisibility(int index, bool visible);
    void setLayerOpacity(int index, double opacity);
    void setLayerBlendMode(int index, int mode);
    void setLayerLocked(int index, bool locked);
    const QList<Layer>& getLayers() const;
    int getCurrentLayerIndex() const;

    QImage getImage();
    void applyImageFilters(const QImage &filteredImage);
    void flipCurrentLayer(bool horizontal, bool vertical);
    void rotateCurrentLayer(int angle);
    void magicWandSelect(const QPoint &pos, int tolerance = 32);

    void setPixelArtMode(bool active, int resolution = 32);
    void setGridSize(int size);
    void setGridActive(bool active);
    bool isGridActive() const;
    void setPixelArtResolution(int resolution);

    void addFrame();
    void duplicateFrame();
    void deleteFrame();
    void goToFrame(int index);
    void nextFrame();
    void prevFrame();

    void clearHistory();
    void saveHistoryState();
    void saveHistoryState(const QString &label);
    void undo();
    void redo();
    int undoAvailableCount() const;
    int redoAvailableCount() const;
    int undoRamCachedCount() const;
    int undoMaxStates() const;
    int undoRamStates() const;
    void setUndoMaxStates(int maxStates);
    void setUndoRamStates(int ramStates);
    QString nextUndoLabel() const;
    QString nextRedoLabel() const;

    void setPenColor1(const QColor &c);
    void setPenColor2(const QColor &c);
    void refreshBrushStamps();
    QColor getPenColor1() const;
    QColor getPenColor2() const;
    void setPenWidth(int newWidth);
    void setPenOpacity(int opacity);

    double getZoomFactor() const;
    void setZoomFactor(double factor);
    void actualizarDimensionesFisicas();
    QSize canvasSize() const;

    void setDarkMode(bool enabled);
    bool getDarkMode() const;

    void setGradientType(int t);
    int getGradientType() const;
    int getGradientOpacity() const;
    void setGradientOpacity(int o);
    int getGradientAngle() const;
    void setGradientAngle(int a);
    bool getGradientReverse() const;
    void setGradientReverse(bool r);
    bool getGradientDither() const;
    void setGradientDither(bool d);
    int getGradientBlendMode() const;
    void setGradientBlendMode(int m);
    bool getGradientUseSecondColor() const;
    void setGradientUseSecondColor(bool v);

    bool isCloneSourceSet() const;
    void resetCloneSource();

    void setTool(ToolType tool);
    void clearImage();
    void crearNuevoLienzo(int w, int h, bool transparent);
    bool abrirImagen(const QString &fileName);
    bool guardarImagen(const QString &fileName, const char *fileFormat);
    bool guardarComoSvg(const QString &fileName);
    bool guardarComoGif(const QString &fileName, int delayMs = 100, int scale = 1);

    void bakeSelection();
    void bakeTextFrame();
    void cancelTextFrame();
    // ← CORREGIDO: añadido parámetro letterSpacing con default
    void updateTextFrame(const QRect &rect, const QFont &font, const QColor &color,
                         double lineSpacing = 1.0, double letterSpacing = 0.0);
    bool isTextFrameActive() const;
    QRect getTextFrameRect() const;
    QString getTextFrameContent() const;
    QFont getTextFrameFont() const;
    QColor getTextFrameColor() const;
    void insertTextChar(const QString &ch);
    void deleteTextChar();

    void copiarSeleccion();
    void cortarSeleccion();
    void pegarClipboard();
    void borrarSeleccion();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void cambiarDimensionesLienzo(int nuevoW, int nuevoH);

    const QList<QImage>& getFrames() const;
    int getCurrentFrameIndex() const;
    bool getIsPixelArtMode() const;
    int getPixelGridSize() const;
    int getPixelResolution() const;

    void editTextObject(int idx);

    void setSelectFreeSubMode(SelectionManager::FreeSubMode mode);
    SelectionManager::FreeSubMode getSelectFreeSubMode() const;
    void cancelSelectFreeVector();
    void cancelSelectFreeElement();

    void setSelectionShape(SelectionManager::SelectionShape s);
    SelectionManager::SelectionShape getSelectionShape() const;

    bool handleTool(const ToolCtx &ctx);

signals:
    void colorPicked(int target, const QColor &color);
    void zoomChanged(double factor);
    void zoomRequested(double newFactor, QPoint viewportPos);
    void resolutionChanged(int width, int height);
    void framesChanged(const QList<QImage> &frames, int currentIndex);
    void layersChanged();
    void statusBarMessage(const QString &msg);
    void textFrameClicked(const QPoint &canvasPos);
    void textFrameCancelled();
    void selectFreeSubModeChanged(int mode);
    void selectionFinalizedByElement();
    void historyChanged(int undoCount, int redoCount, int ramCount);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    LayerStack stack;
    void recomponerImagen() { stack.recompose(); }

    QPoint startPoint, lastPoint, hoverPos;
    bool drawing = false;
    Qt::MouseButton activeMouseButton = Qt::NoButton;

    bool resizingCanvas = false;
    int resizeMode = 0;
    const int HANDLE_SIZE = 8;
    QPoint previewCanvasSize;

    QColor penColor1 = Qt::black, penColor2 = Qt::white;
    int penWidth = 3, penOpacity = 255;
    ToolType currentTool = ToolType::Pencil;
    double zoomFactor = 0.50;

    BrushSettings customBrushPresets[2];
    int activeCustomBrushIndex = 0;
    QImage customBrushStamp, customBrushStampRight;

    BrushSettings classicToolPreset;
    QImage classicToolStamp, classicToolStampRight;
    QPointF lastClassicPoint;
    QColor strokeCanvasFallback;
    double strokeTotalLength = 0.0;
    double strokeAccumulatedLength = 0.0;

    QTimer *continuousDrawTimer;
    QPoint currentMousePos;
    QPoint previousMousePos;

    SelectionManager selMgr;
    MaskEditController m_maskEdit;

    bool darkModeActive = false;
    PixelArtOptions pixelOptions;
    PixelAnimationManager animManager;
    BezierPathTool bezierTool;
    double mouseSensitivity = 1.0;
    int activeColorTarget = 1;
    TextEngine::EditSession textEdit;
    DualUndoManager m_undo;
    QString m_pendingLabel;

    enum GradientType { GradientLinear = 0, GradientRadial = 1, GradientConic = 2 };
    GradientType gradientType = GradientLinear;
    QPoint gradientStart, gradientEnd;
    bool drawingGradient = false;
    int gradientOpacity = 255;
    int gradientAngle = 0;
    bool gradientReverse = false;
    bool gradientDither = false;
    int gradientBlendMode = 0;
    bool gradientUseSecondColor = false;

    bool cloneSourceSet = false;
    QPoint cloneSource;
    QPoint cloneInitialDest;
    QImage cloneBuffer;
    bool cloneIsStamping = false;

    DeformController m_deform;

    bool movingLayer = false;
    QPoint moveStartPos;
    QImage moveLayerBackup;
    int moveLayerIdx = -1;

    QFont lastUsedTextFont;
    QPoint elementLastHover;
    bool preserveSelectionOnToolSwitch = false;

    friend class mainwind;

    QColor selBlue() const;
    QColor selBlueLight() const;

    bool capaValida(int idx = -1) const {
        return (idx >= 0) ? stack.validIndex(idx) : stack.currentValid();
    }
    bool puedeEditarCapaActual() const {
        return capaValida() && !stack.currentLocked();
    }
    bool herramientaDePintura() const { return ToolManager::isPainting(currentTool); }
    bool usaStampDePincel() const    { return ToolManager::usesBrushStamp(currentTool); }
    const BrushSettings& activePreset() const;
    const QImage& activeStamp() const;

    void refreshAndNotify(bool recompose = true);
    void bakeAllPending();
    void beginEdit();
    void beginStroke(const ToolCtx &ctx);
    bool endStroke();

    void configureMaskEditController();
    void renderTiles(QPainter &painter, const QRect &visibleWidgetRect);
    void sincronizarCapasConFrameActual();
    void guardarFrameActualEnAnimador();
    void procesarDibujoContinuo();
    QColor obtenerColorDeTrabajo(Qt::MouseButton button);
    void applyClonStamp(QImage &target, const QPoint &destPos);
    void aplicarGradienteConfigurado(const QPoint &p1, const QPoint &p2);
    void openGradientSettings();

    int margenHerramienta() const;
    QRect rectCanvasAWidget(const QRect &r) const;
    void repintarZonaCanvas(const QRect &canvasRect);
    QRect rectSiluetaWidget(const QPoint &widgetPos) const;
    void invalidarTrazo(const QPoint &a, const QPoint &b, const QRect &extraCanvas = QRect());
    void invalidarPreviewClone(const QPoint &cursorPos);

    void convertSelectionToObject();
    void integrateSelectedObjects();
    void loadTextObjectForEditing(int idx);
    void loadShapeObjectForEditing(int idx);
    void bakeObjectIntoLayer(int idx);
    void bakeAllObjects();

    QRect getRightHandle() const;
    QRect getBottomHandle() const;
    QRect getBottomRightHandle() const;

    void updateClassicToolStamp();
    void updateCustomBrushStamp();

    TextEngine::Style currentTextStyle() const {
        return darkModeActive ? TextEngine::Style::forDarkMode()
                              : TextEngine::Style::forLightMode();
    }

    ToolCtx buildMouseCtx(QMouseEvent *event) const;
    ToolCtx buildKeyCtx(QKeyEvent *event) const;

    using DispatchFn = bool (PaintArea::*)(const ToolCtx &);
    bool dispatchPress  (const ToolCtx &ctx);
    bool dispatchMove   (const ToolCtx &ctx);
    bool dispatchRelease(const ToolCtx &ctx);
    bool dispatchKey    (const ToolCtx &ctx);

    bool onPressMaskBezier(const ToolCtx &ctx);
    bool onPressMaskPaint(const ToolCtx &ctx);
    bool onPressTextActive(const ToolCtx &ctx);
    bool onPressCanvasHandles(const ToolCtx &ctx);
    bool onPressSelectionGizmo(const ToolCtx &ctx);

    bool onMoveMaskBezier(const ToolCtx &ctx);
    bool onMoveMaskPaint(const ToolCtx &ctx);
    bool onMoveSelectFreeVector(const ToolCtx &ctx);
    bool onMoveSelectFreeElement(const ToolCtx &ctx);
    bool onMoveTextActive(const ToolCtx &ctx);
    bool onMoveTextHover(const ToolCtx &ctx);
    bool onMoveCanvasResize(const ToolCtx &ctx);
    bool onMoveSelectionRotate(const ToolCtx &ctx);
    bool onMoveSelectionResize(const ToolCtx &ctx);
    bool onMoveSelectionDrag(const ToolCtx &ctx);

    bool onReleaseMaskPaint(const ToolCtx &ctx);
    bool onReleaseSelectFreeVector(const ToolCtx &ctx);
    bool onReleaseTextDragResize(const ToolCtx &ctx);
    bool onReleaseCanvasResize(const ToolCtx &ctx);
    bool onReleaseSelectionGizmo(const ToolCtx &ctx);
    bool onReleaseMove(const ToolCtx &ctx);

    bool onKeySelectFree(const ToolCtx &ctx);
    bool onKeyMaskBezier(const ToolCtx &ctx);
    bool onKeyClipboard(const ToolCtx &ctx);
    bool onKeyDelete(const ToolCtx &ctx);
    bool onKeyObjectShortcuts(const ToolCtx &ctx);
    bool onKeyTextEditing(const ToolCtx &ctx);
    bool onKeyToolSpecific(const ToolCtx &ctx);
    bool onKeyBezierPen(const ToolCtx &ctx);

    void pressPenBezier(const QPoint &pos);
    void pressSelectRect(const QPoint &pos);
    void pressSelectFree(const QPoint &pos);
    void pressSelectFreeVector(const QPoint &pos);
    void pressSelectFreeElement(const QPoint &pos);
    void pressTextNew(const QPoint &pos, const QColor &colorDeUso);
    void pressMove(QMouseEvent *event, const QPoint &pos);

    bool tryGrabActiveObjectGizmo(const QPointF &canvasPos);
    void beginObjectSelectionDrag(int idx, const QPointF &canvasPos);
    void beginLayerMove(const QPoint &pos);

    bool moveObjectManipulation(const ToolCtx &ctx);
    bool moveMovingLayer(const ToolCtx &ctx);
    bool moveToolHover(const ToolCtx &ctx);

    void moveCursorUpdate(const QPoint &pos);
    void applyToolCursor();
    void updateSilhouetteOnHover();
    void updateBezierPreviewOnHover(const QPoint &pos);

    void releaseFinishSelection(const QPoint &finalPoint);

    bool toolPencil    (const ToolCtx &ctx);
    bool toolEraser    (const ToolCtx &ctx);
    bool toolPicker    (const ToolCtx &ctx);
    bool toolBucket    (const ToolCtx &ctx);
    bool toolLasso     (const ToolCtx &ctx);
    bool toolGradient  (const ToolCtx &ctx);
    bool toolMagicWand (const ToolCtx &ctx);
    bool toolRetouch   (const ToolCtx &ctx);
    bool toolClone     (const ToolCtx &ctx);
    bool toolDeform    (const ToolCtx &ctx);
    bool toolZoom      (const ToolCtx &ctx);
    bool toolShape     (const ToolCtx &ctx);
    bool toolBrushStamp(const ToolCtx &ctx);
    bool toolPixelArt  (const ToolCtx &ctx);
    bool toolPenBezier (const ToolCtx &ctx);
    bool toolSelect    (const ToolCtx &ctx);
    bool toolSelectFree(const ToolCtx &ctx);
    bool toolText      (const ToolCtx &ctx);
    bool toolMove      (const ToolCtx &ctx);

    bool handleGradientToolReentry(ToolType tool);
    void resetStateForToolSwitch(ToolType tool);
    void applyToolPreset(ToolType tool);
    bool toolKeepsSelectionAlive(ToolType tool) const;
    bool toolKeepsFreeModeAlive(ToolType tool) const;
    void resetSelectionForNewTool(ToolType tool);
    void resetToolModes(ToolType tool);

    void paintCheckerboard(QPainter &painter, const QRect &canvasRect);
    void paintMaskOverlay(QPainter &painter);
    void paintBezierOverlay(QPainter &painter);
    void paintCanvasResizePreview(QPainter &painter);
    void paintShapePreview(QPainter &painter, int scaledWidth);
    void paintActiveSelection(QPainter &painter);
    void paintSelectionPreview(QPainter &painter);
    void paintTextFrame(QPainter &painter);
    void paintGradientPreview(QPainter &painter);
    void paintCloneOverlay(QPainter &painter);
    void paintSelectionGizmos(QPainter &painter);
    void paintCanvasHandles(QPainter &painter);
    void paintCursorSilhouette(QPainter &painter, int scaledWidth);
    void paintSelectFreeVectorOverlay(QPainter &painter);
    void paintSelectFreeElementOverlay(QPainter &painter);
    void paintMaskBrushSilhouette(QPainter &painter);
    void paintRetouchSilhouette(QPainter &painter, int scaledWidth);
    void paintDeformSilhouette(QPainter &painter);
    void paintCloneSilhouette(QPainter &painter, int scaledWidth);
    void paintBrushStampSilhouette(QPainter &painter);

    std::unique_ptr<QGradient> buildGradientForPreview() const;
    void paintGradientFill(QPainter &painter, QGradient *grad);
    void paintGradientHandleOverlay(QPainter &painter);
};

#endif // PAINTAREA_H