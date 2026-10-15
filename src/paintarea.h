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
#include "core/undocandy.h"

class mainwind;

class FrameThumbnail : public QFrame {
    Q_OBJECT
    QImage frameImage; bool isSelected; int frameIndex;
    static const int THUMB_SIZE = 48;
public:
    FrameThumbnail(const QImage &img, int index, QWidget *parent = nullptr);
    void setSelected(bool s); void setFrameImage(const QImage &i); int getFrameIndex() const;
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
signals:
    void clicked(int frameIndex);
};

enum class ToolAction { Press, Move, Release };

struct ToolCtx {
    ToolAction action = ToolAction::Press;
    QMouseEvent *mouse = nullptr; QKeyEvent *key = nullptr;
    QPoint pos, rawPos;
    Qt::MouseButton button = Qt::NoButton;
    bool shift = false, ctrl = false, alt = false, continuous = false;
    int scaledWidth = 1;
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
    void setMouseSensitivity(double sens); double getMouseSensitivity() const;
    void setDeformOptc(const DeformOptc &o) { m_deform.setOptions(o); }
    DeformOptc getDeformOptc() const { return m_deform.options(); }
    void bakeActivePath();

    void setCustomBrushPresets(const BrushSettings &p1, const BrushSettings &p2, int i);
    BrushSettings getCustomBrush(int i) const; int getActiveCustomBrushIndex() const;

    bool hasLayerMask(int i) const; bool isLayerMaskEnabled(int i) const;
    int getMaskEditLayer() const; bool isEditingMask() const;
    QImage getMaskPreview(int i) const;
    void addLayerMask(int i); void removeLayerMask(int i);
    void toggleLayerMaskEnabled(int i); void selectMaskForEditing(int i);
    void selectLayerContentForEditing(); void applyMaskToLayer(int i); void invertLayerMask(int i);

    bool hasLayerColorMask(int i) const; bool isLayerColorMaskEnabled(int i) const;
    FilterParams getLayerColorMaskParams(int i) const;
    QImage getColorMaskPreview(int i) const;
    void addLayerColorMask(int i, const FilterParams &fp); void removeLayerColorMask(int i);
    void toggleLayerColorMaskEnabled(int i);
    void setLiveColorMaskPreview(int i, const FilterParams &fp); void clearLiveColorMaskPreview(int i);

    void addLayer(); void addImageLayer(const QImage &img); void insertImageAsObject(const QImage &img);
    void duplicateLayer(); void deleteLayer(); void mergeDown();
    void moveLayerUp(); void moveLayerDown();
    void reorderLayer(int from, int to);
    void setCurrentLayer(int i); void setLayerVisibility(int i, bool v);
    void setLayerOpacity(int i, double o); void setLayerBlendMode(int i, int m);
    void setLayerLocked(int i, bool l);
    const QList<Layer>& getLayers() const; int getCurrentLayerIndex() const;

    QImage getImage(); void applyImageFilters(const QImage &img);
    void flipCurrentLayer(bool h, bool v); void rotateCurrentLayer(int a);
    void magicWandSelect(const QPoint &pos, int tol = 32);

    void setPixelArtMode(bool a, int r = 32);
    void setGridSize(int s); void setGridActive(bool a); bool isGridActive() const;
    void setPixelArtResolution(int r);

    void addFrame(); void duplicateFrame(); void deleteFrame();
    void goToFrame(int i); void nextFrame(); void prevFrame();

    void clearHistory(); void saveHistoryState(); void saveHistoryState(const QString &l);
    void undo(); void redo();
    int undoAvailableCount() const; int redoAvailableCount() const; int undoRamCachedCount() const;
    int undoMaxStates() const; int undoRamStates() const;
    void setUndoMaxStates(int n); void setUndoRamStates(int n);
    QString nextUndoLabel() const; QString nextRedoLabel() const;

    void setPenColor1(const QColor &c); void setPenColor2(const QColor &c);
    void refreshBrushStamps(); QColor getPenColor1() const; QColor getPenColor2() const;

    void setAccentColor(const QColor &c) { m_accentColor = c; update(); }
    QColor accentColor() const { return m_accentColor; }

    void setPenWidth(int w); void setPenOpacity(int o);
    double getZoomFactor() const; void setZoomFactor(double f);
    void actualizarDimensionesFisicas(); QSize canvasSize() const;
    void setDarkMode(bool e); bool getDarkMode() const;

    void setGradientType(int t); int getGradientType() const;
    int getGradientOpacity() const; void setGradientOpacity(int o);
    int getGradientAngle() const; void setGradientAngle(int a);
    bool getGradientReverse() const; void setGradientReverse(bool r);
    bool getGradientDither() const; void setGradientDither(bool d);
    int getGradientBlendMode() const; void setGradientBlendMode(int m);
    bool getGradientUseSecondColor() const; void setGradientUseSecondColor(bool v);

    bool isCloneSourceSet() const; void resetCloneSource();

    void setTool(ToolType t); void clearImage();
    void crearNuevoLienzo(int w, int h, bool transp);
    bool abrirImagen(const QString &f);
    bool guardarImagen(const QString &f, const char *fmt);
    bool guardarComoSvg(const QString &f);
    bool guardarComoGif(const QString &f, int delay = 100, int scale = 1);

    void bakeSelection(); void bakeTextFrame(); void cancelTextFrame();
    void updateTextFrame(const QRect &r, const QFont &f, const QColor &c,
                         double ls = 1.0, double lt = 0.0);
    bool isTextFrameActive() const; QRect getTextFrameRect() const;
    QString getTextFrameContent() const; QFont getTextFrameFont() const;
    QColor getTextFrameColor() const;
    void insertTextChar(const QString &c); void deleteTextChar();

    void copiarSeleccion(); void cortarSeleccion(); void pegarClipboard(); void borrarSeleccion();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void cambiarDimensionesLienzo(int w, int h);

    const QList<QImage>& getFrames() const; int getCurrentFrameIndex() const;
    bool getIsPixelArtMode() const; int getPixelGridSize() const; int getPixelResolution() const;
    void editTextObject(int idx);

    void setSelectFreeSubMode(SelectionManager::FreeSubMode m);
    SelectionManager::FreeSubMode getSelectFreeSubMode() const;
    void cancelSelectFreeVector(); void cancelSelectFreeElement();
    void setSelectionShape(SelectionManager::SelectionShape s);
    SelectionManager::SelectionShape getSelectionShape() const;
    bool handleTool(const ToolCtx &ctx);

signals:
    void colorPicked(int target, const QColor &color);
    void zoomChanged(double f);
    void zoomRequested(double f, QPoint vp);
    void resolutionChanged(int w, int h);
    void framesChanged(const QList<QImage> &f, int idx);
    void layersChanged();
    void statusBarMessage(const QString &m);
    void textFrameClicked(const QPoint &p);
    void textFrameCancelled();
    void selectFreeSubModeChanged(int m);
    void selectionFinalizedByElement();
    void historyChanged(int u, int r, int rc);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

private:
    LayerStack stack;
    void recomponerImagen() { stack.recompose(); }
    QPoint startPoint, lastPoint, hoverPos;
    bool drawing = false;
    Qt::MouseButton activeMouseButton = Qt::NoButton;
    bool resizingCanvas = false; int resizeMode = 0;
    const int HANDLE_SIZE = 8; QPoint previewCanvasSize;
    QColor penColor1 = Qt::black, penColor2 = Qt::white;
    int penWidth = 3, penOpacity = 255;
    ToolType currentTool = ToolType::Pencil;
    double zoomFactor = 0.50;
    BrushSettings customBrushPresets[2]; int activeCustomBrushIndex = 0;
    QImage customBrushStamp, customBrushStampRight;
    BrushSettings classicToolPreset; QImage classicToolStamp, classicToolStampRight;
    QPointF lastClassicPoint; QColor strokeCanvasFallback;
    double strokeTotalLength = 0.0, strokeAccumulatedLength = 0.0;
    QTimer *continuousDrawTimer = nullptr, *undoPendingFlushTimer = nullptr;
    QPoint currentMousePos, previousMousePos;
    SelectionManager selMgr; MaskEditController m_maskEdit;
    bool darkModeActive = false; QColor m_accentColor;
    PixelArtOptions pixelOptions; PixelAnimationManager animManager;
    BezierPathTool bezierTool; double mouseSensitivity = 1.0;
    int activeColorTarget = 1;
    TextEngine::EditSession textEdit;
    UndoCandy::Manager m_undo;
    enum GradientType { GradientLinear = 0, GradientRadial = 1, GradientConic = 2 };
    GradientType gradientType = GradientLinear;
    QPoint gradientStart, gradientEnd; bool drawingGradient = false;
    int gradientOpacity = 255, gradientAngle = 0;
    bool gradientReverse = false, gradientDither = false;
    int gradientBlendMode = 0; bool gradientUseSecondColor = false;
    bool cloneSourceSet = false; QPoint cloneSource, cloneInitialDest;
    QImage cloneBuffer; bool cloneIsStamping = false;
    DeformController m_deform;
    bool movingLayer = false; QPoint moveStartPos; QImage moveLayerBackup; int moveLayerIdx = -1;
    QFont lastUsedTextFont; QPoint elementLastHover; bool preserveSelectionOnToolSwitch = false;

    friend class mainwind;

    QColor selBlue() const; QColor selBlueLight() const;
    bool capaValida(int i = -1) const { return (i >= 0) ? stack.validIndex(i) : stack.currentValid(); }
    bool puedeEditarCapaActual() const { return capaValida() && !stack.currentLocked(); }
    bool herramientaDePintura() const { return ToolManager::isPainting(currentTool); }
    bool usaStampDePincel() const { return ToolManager::usesBrushStamp(currentTool); }
    const BrushSettings& activePreset() const; const QImage& activeStamp() const;

    void refreshAndNotify(bool rec = true);
    void bakeAllPending(); void beginEdit();
    void beginStroke(const ToolCtx &ctx); bool endStroke();
    void configureMaskEditController();
    void renderTiles(QPainter &p, const QRect &wr);
    void sincronizarCapasConFrameActual(); void guardarFrameActualEnAnimador();
    void procesarDibujoContinuo();
    QColor obtenerColorDeTrabajo(Qt::MouseButton b);
    void applyClonStamp(QImage &t, const QPoint &p);
    void aplicarGradienteConfigurado(const QPoint &p1, const QPoint &p2);
    void openGradientSettings();
    int margenHerramienta() const;
    QRect rectCanvasAWidget(const QRect &r) const;
    void repintarZonaCanvas(const QRect &r);
    QRect rectSiluetaWidget(const QPoint &widgetPos) const;
    void invalidarTrazo(const QPoint &a, const QPoint &b, const QRect &e = QRect());
    void invalidarPreviewClone(const QPoint &p);

    void convertSelectionToObject(); void integrateSelectedObjects();
    void loadTextObjectForEditing(int i); void loadShapeObjectForEditing(int i);
    void bakeObjectIntoLayer(int i); void bakeAllObjects();
    QRect getRightHandle() const; QRect getBottomHandle() const; QRect getBottomRightHandle() const;
    void updateClassicToolStamp(); void updateCustomBrushStamp();

    TextEngine::Style currentTextStyle() const {
        return darkModeActive ? TextEngine::Style::forDarkMode() : TextEngine::Style::forLightMode();
    }
    ToolCtx buildMouseCtx(QMouseEvent *e) const; ToolCtx buildKeyCtx(QKeyEvent *e) const;
    using DispatchFn = bool (PaintArea::*)(const ToolCtx &);
    bool dispatchPress(const ToolCtx &ctx); bool dispatchMove(const ToolCtx &ctx);
    bool dispatchRelease(const ToolCtx &ctx); bool dispatchKey(const ToolCtx &ctx);

    bool onPressMaskBezier(const ToolCtx&); bool onPressMaskPaint(const ToolCtx&);
    bool onPressTextActive(const ToolCtx&); bool onPressCanvasHandles(const ToolCtx&);
    bool onPressSelectionGizmo(const ToolCtx&);
    bool onMoveMaskBezier(const ToolCtx&); bool onMoveMaskPaint(const ToolCtx&);
    bool onMoveSelectFreeVector(const ToolCtx&); bool onMoveSelectFreeElement(const ToolCtx&);
    bool onMoveTextActive(const ToolCtx&); bool onMoveTextHover(const ToolCtx&);
    bool onMoveCanvasResize(const ToolCtx&); bool onMoveSelectionRotate(const ToolCtx&);
    bool onMoveSelectionResize(const ToolCtx&); bool onMoveSelectionDrag(const ToolCtx&);
    bool onReleaseMaskPaint(const ToolCtx&); bool onReleaseSelectFreeVector(const ToolCtx&);
    bool onReleaseTextDragResize(const ToolCtx&); bool onReleaseCanvasResize(const ToolCtx&);
    bool onReleaseSelectionGizmo(const ToolCtx&); bool onReleaseMove(const ToolCtx&);
    bool onKeySelectFree(const ToolCtx&); bool onKeyMaskBezier(const ToolCtx&);
    bool onKeyClipboard(const ToolCtx&); bool onKeyDelete(const ToolCtx&);
    bool onKeyObjectShortcuts(const ToolCtx&); bool onKeyTextEditing(const ToolCtx&);
    bool onKeyToolSpecific(const ToolCtx&); bool onKeyBezierPen(const ToolCtx&);

    void pressPenBezier(const QPoint &p); void pressSelectRect(const QPoint &p);
    void pressSelectFree(const QPoint &p); void pressSelectFreeVector(const QPoint &p);
    void pressSelectFreeElement(const QPoint &p); void pressTextNew(const QPoint &p, const QColor &c);
    void pressMove(QMouseEvent *e, const QPoint &p);
    bool tryGrabActiveObjectGizmo(const QPointF &p);
    void beginObjectSelectionDrag(int i, const QPointF &p); void beginLayerMove(const QPoint &p);
    bool moveObjectManipulation(const ToolCtx&); bool moveMovingLayer(const ToolCtx&);
    bool moveToolHover(const ToolCtx&);
    void moveCursorUpdate(const QPoint &p); void applyToolCursor();
    void updateSilhouetteOnHover(); void updateBezierPreviewOnHover(const QPoint &p);
    void releaseFinishSelection(const QPoint &p);

    bool toolPencil(const ToolCtx&); bool toolEraser(const ToolCtx&); bool toolPicker(const ToolCtx&);
    bool toolBucket(const ToolCtx&); bool toolLasso(const ToolCtx&); bool toolGradient(const ToolCtx&);
    bool toolMagicWand(const ToolCtx&); bool toolRetouch(const ToolCtx&); bool toolClone(const ToolCtx&);
    bool toolDeform(const ToolCtx&); bool toolZoom(const ToolCtx&); bool toolShape(const ToolCtx&);
    bool toolBrushStamp(const ToolCtx&); bool toolPixelArt(const ToolCtx&); bool toolPenBezier(const ToolCtx&);
    bool toolSelect(const ToolCtx&); bool toolSelectFree(const ToolCtx&); bool toolText(const ToolCtx&);
    bool toolMove(const ToolCtx&);

    bool handleGradientToolReentry(ToolType t);
    void resetStateForToolSwitch(ToolType t); void applyToolPreset(ToolType t);
    bool toolKeepsSelectionAlive(ToolType t) const; bool toolKeepsFreeModeAlive(ToolType t) const;
    void resetSelectionForNewTool(ToolType t); void resetToolModes(ToolType t);

    void paintCheckerboard(QPainter &p, const QRect &r); void paintMaskOverlay(QPainter &p);
    void paintBezierOverlay(QPainter &p); void paintCanvasResizePreview(QPainter &p);
    void paintShapePreview(QPainter &p, int w); void paintActiveSelection(QPainter &p);
    void paintSelectionPreview(QPainter &p); void paintTextFrame(QPainter &p);
    void paintGradientPreview(QPainter &p); void paintCloneOverlay(QPainter &p);
    void paintSelectionGizmos(QPainter &p); void paintCanvasHandles(QPainter &p);
    void paintCursorSilhouette(QPainter &p, int w);
    void paintSelectFreeVectorOverlay(QPainter &p); void paintSelectFreeElementOverlay(QPainter &p);
    void paintMaskBrushSilhouette(QPainter &p); void paintRetouchSilhouette(QPainter &p, int w);
    void paintDeformSilhouette(QPainter &p); void paintCloneSilhouette(QPainter &p, int w);
    void paintBrushStampSilhouette(QPainter &p);

    std::unique_ptr<QGradient> buildGradientForPreview() const;
    void paintGradientFill(QPainter &p, QGradient *g);
    void paintGradientHandleOverlay(QPainter &p);
};

#endif