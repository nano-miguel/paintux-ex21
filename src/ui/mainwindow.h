#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QToolButton>
#include <QScrollArea>
#include <QFrame>
#include <QInputDialog>
#include <QButtonGroup>
#include <QDir>
#include <QActionGroup>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTranslator>
#include <QSettings>
#include <QProcess>
#include <QStyleFactory>
#include <QSvgRenderer>
#include <QDebug>
#include <QColorDialog>
#include <QStyleHints>
#include <QMimeData>
#include <QDrag>
#include <QPalette>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWidgetAction>
#include <QScrollBar>
#include <QCloseEvent>
#include <QWindow>
#include <QHash>
#include "paintarea.h"
#include "core/ToolManager.h"
#include "filter/ImageFilters.h"
#include "core/CustomBrushes.h"
#include "core/colrs.h"

// ═══════════════════════════════════════════════════════════════════════
//  Tema "openSUSE Neutral": grises fríos neutros, azules de acento intactos
//  ─────────────────────────────────────────────────────────────────────
//  • Mantiene exactamente los mismos azules de acento (#3B82F6 etc)
//  • Solo cambiaron los grises/blancos: de cálidos (Braun House) a
//    neutros fríos estilo openSUSE Aegean / Adwaita openSUSE.
// ═══════════════════════════════════════════════════════════════════════
struct ThemeColors {
    bool dark;
    QString bgDialog, bgPanel, bgPreview, bgInput, bgHover, bgSelected;
    QString textPrimary, textSecondary, textMuted, textAccent;
    QString border, borderStrong, borderAccent;
    QString accent, accentHover, accentPressed;
    QString success, warning, danger;

    ThemeColors(bool isDark) : dark(isDark) {
        if (isDark) {
            // openSUSE Dark: grises neutros fríos (sin tinte marrón)
            bgDialog     = "#1C1C1C";   // fondo raíz
            bgPanel      = "#242424";   // superficie de panel
            bgPreview    = "#121212";   // canvas, lo más oscuro
            bgInput      = "#2D2D2D";   // inputs, combos
            bgHover      = "#383838";   // hover
            bgSelected   = "#1E3A5F";   // selección (azul muy sutil)
            textPrimary  = "#E8E8E8";   // blanco frío neutro
            textSecondary= "#B0B0B0";   // gris medio neutro
            textMuted    = "#808080";   // gris apagado neutro
            textAccent   = "#7FB0E0";   // azul suavizado (igual que antes)
            border       = "#333333";   // borde sutil neutro
            borderStrong = "#4A4A4A";   // borde visible neutro
            borderAccent = "#3B82F6";   // azul de acento (igual)
            accent       = "#3B82F6";   // igual
            accentHover  = "#5A95F5";   // igual
            accentPressed= "#2E6BC8";   // igual
            success      = "#6BA368";   // igual
            warning      = "#C89B4A";   // igual
            danger       = "#C05A55";   // igual
        } else {
            // openSUSE Light: blancos/grises neutros fríos (sin tinte crema)
            bgDialog     = "#F5F5F5";
            bgPanel      = "#FAFAFA";
            bgPreview    = "#EEEEEE";
            bgInput      = "#FFFFFF";
            bgHover      = "#E8E8E8";
            bgSelected   = "#D9E3EE";   // igual que antes
            textPrimary  = "#1C1C1C";
            textSecondary= "#555555";
            textMuted    = "#888888";
            textAccent   = "#2E6BC8";   // igual
            border       = "#DCDCDC";
            borderStrong = "#BBBBBB";
            borderAccent = "#3B82F6";   // igual
            accent       = "#3B82F6";   // igual
            accentHover  = "#2E6BC8";   // igual
            accentPressed= "#1E4A8F";   // igual
            success      = "#4E8B4A";   // igual
            warning      = "#B8862E";   // igual
            danger       = "#B04540";   // igual
        }
    }
};

bool detectarTemaOscuroSistema();
QIcon crearIconoMascara(bool dark);
QIcon crearIconoDeformacion(bool dark);
QIcon crearIconoSelectFree(int subMode, bool dark);

class NewCanvasDialog : public QDialog {
    Q_OBJECT
private:
    QSpinBox *sbWidth, *sbHeight;
    QPushButton *btnWhite, *btnTransparent;
    QLabel *previewLabel, *lblInfo;
    int selectedBg = 0;
    QList<QPushButton*> presetButtons;
    QPushButton *activePresetBtn = nullptr;
    ThemeColors colors;
    void updatePresetStyle(QPushButton *btn, bool active);
    void setActivePreset(QPushButton *btn);
    void applyPreset(int w, int h);
    void updatePreview();
public:
    NewCanvasDialog(bool darkMode, QWidget *parent = nullptr);
    int getWidth() const;
    int getHeight() const;
    bool isTransparent() const;
};

class MaskThumbButton : public QPushButton {
    Q_OBJECT
public:
    MaskThumbButton(QWidget *parent = nullptr);
signals:
    void maskClicked(bool shiftHeld);
protected:
    void mousePressEvent(QMouseEvent *e) override;
};

class DraggableLayerItem : public QFrame {
    Q_OBJECT
public:
    int layerIndex;
    bool isSelected, layerVisible;
    bool isDarkMode;
    QPoint dragStartPosition;
    QPushButton *btnEye;
    QLabel *thumbLabel;
    QFrame *maskThumbFrame;
    MaskThumbButton *btnMaskThumb;
    QFrame *colorMaskThumbFrame;
    MaskThumbButton *btnColorMaskThumb;
    QLabel *nameLabel;
    DraggableLayerItem(int idx, const QImage &img, const QString &name,
                       bool vis, bool dark, QWidget *parent = nullptr);
    void updateMaskState(bool hasMask, bool enabled, bool editing, const QImage &preview);
    void updateColorMaskState(bool hasMask, bool enabled, const QImage &preview);
    void refreshFrom(const QImage &img, const QString &name, bool vis, bool dark);
    void updateDarkMode(bool dark);
    void setSelected(bool selected);
private:
    void updateEyeButton();
    void updateStyle();
    qint64  m_cachedImgKey = -1;
    QPixmap m_cachedThumb;
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
signals:
    void clicked(int layerIndex);
    void layerMoved(int fromIndex, int toIndex);
    void visibilityToggled(int layerIndex, bool visible);
    void maskClicked(int layerIndex, bool shiftHeld);
    void colorMaskClicked(int layerIndex, bool shiftHeld);
};

class mainwind : public QMainWindow {
    Q_OBJECT
private:
    int currentWorkspaceAlpha = 191;
    PaintArea *paintArea = nullptr;
    QScrollArea *scrollArea = nullptr;
    QString currentProjectPath;
    QString lastImagePath;
    QString lastImageExtension = "png";
    bool hasUnsavedChanges = false;
    bool suppressUnsavedWarning = false;
    QWidget *titleBarWidget = nullptr;
    QWidget *sysBarWidget = nullptr;
    QWidget *ribbonWidget = nullptr;
    QWidget *bottomBarWidget = nullptr;
    QWidget *leftSidebarWidget = nullptr;
    QWidget *contentAreaWidget = nullptr;
    QPushButton *btnArchivoMenu = nullptr;
    QPushButton *btnViewMenu = nullptr;
    QPushButton *btnConfiguraciones = nullptr;
    QPushButton *btnQuickSave = nullptr;
    QPushButton *btnUndo = nullptr;
    QPushButton *btnRedo = nullptr;
    QPushButton *btnRotateFlip = nullptr;
    QPushButton *btnThemeToggle = nullptr;
    QPushButton *btnImageFilters = nullptr;
    QPushButton *btnCustomBrushesTop = nullptr;
    QPushButton *btnCanvasSize = nullptr;
    QMenu *menuArchivoDesplegable = nullptr;
    QMenu *menuConfiguracionesDesplegable = nullptr;
    QMenu *menuCanvasSizeStd = nullptr;
    QMenu *menuCanvasSizePixel = nullptr;
    QAction *actNuevo = nullptr;
    QAction *actAbrir = nullptr;
    QAction *actGuardarImagen = nullptr;
    QAction *actGuardarImagenComo = nullptr;
    QAction *actGuardarProyecto = nullptr;
    QAction *actGuardarProyectoComo = nullptr;
    QAction *actInsertarImagen = nullptr;
    QAction *actInsertarComoCapa = nullptr;
    QAction *actAbrirFondo = nullptr;
    QAction *actExportGif = nullptr;
    QAction *actSalir = nullptr;
    QAction *actUndoLight = nullptr;
    QAction *actUndoNormal = nullptr;
    QAction *actUndoWide = nullptr;
    QAction *actUndoDeep = nullptr;
    QAction *actUndoExtreme = nullptr;
    QAction *actUndoCustom = nullptr;
    QActionGroup *undoPresetsGroup = nullptr;
    QAction *actUndoRam3 = nullptr;
    QAction *actUndoRam5 = nullptr;
    QAction *actUndoRam8 = nullptr;
    QAction *actUndoRam15 = nullptr;
    QActionGroup *undoRamGroup = nullptr;
    QAction *actShowUndoIndicator = nullptr;
    QGroupBox *boxShapes = nullptr;
    QGroupBox *boxBrushesContainer = nullptr;
    QGroupBox *boxClipboard = nullptr;
    QGroupBox *boxAnimation = nullptr;
    QPushButton *btnPrevFrame = nullptr;
    QPushButton *btnNextFrame = nullptr;
    QPushButton *btnAddFrame = nullptr;
    QPushButton *btnDupFrame = nullptr;
    QPushButton *btnDelFrame = nullptr;
    QLabel *lblFrameIndicator = nullptr;
    QScrollArea *framesScrollArea = nullptr;
    QWidget *framesContainer = nullptr;
    QHBoxLayout *framesLayout = nullptr;
    QList<FrameThumbnail*> frameThumbnails;
    QPushButton *btnPlayAnimation = nullptr;
    QSlider *animSpeedSlider = nullptr;
    QTimer *animTimer = nullptr;
    bool isAnimating = false;
    int animSpeed = 200;
    QGroupBox *boxAdvTools = nullptr;
    QGroupBox *boxVectorTools = nullptr;
    QGroupBox *boxLayersContainer = nullptr;
    QPushButton *btnMagicWand = nullptr;
    QPushButton *btnBlurTool = nullptr;
    QPushButton *btnHealTool = nullptr;
    QPushButton *btnShadowTool = nullptr;
    QPushButton *btnGradientTool = nullptr;
    QPushButton *btnCloneTool = nullptr;
    QPushButton *btnZoomToolSidebar = nullptr;
    QToolButton *btnPenBezier = nullptr;
    QPushButton *btnLassoTool = nullptr;
    QMenu *menuLassoModos = nullptr;
    QAction *actLassoAdentro = nullptr;
    QAction *actLassoAfuera = nullptr;
    int lassoModoActual = 0;
    QPushButton *btnSelectBox = nullptr;
    QPushButton *btnSelectFree = nullptr;
    QMenu *menuSelectFreeModos = nullptr;
    QAction *actFreeLasso = nullptr;
    QAction *actFreeVector = nullptr;
    QAction *actFreeElement = nullptr;
    int selectFreeMode = 0;
    QMenu *menuSelectShape = nullptr;
    QAction *actShapeRect = nullptr;
    QAction *actShapeEllipse = nullptr;
    QAction *actShapeTriangle = nullptr;
    int selectShapeMode = 0;
    QPushButton *btnDeformTool = nullptr;
    DeformSettingsBar *deformSettingsBar = nullptr;
    QScrollArea *layersScrollArea = nullptr;
    QWidget *layersContainer = nullptr;
    QVBoxLayout *layersLayout = nullptr;
    QList<DraggableLayerItem*> draggableLayerItems;
    QPushButton *btnAddLayer = nullptr;
    QPushButton *btnDupLayer = nullptr;
    QPushButton *btnDelLayer = nullptr;
    QPushButton *btnLayerMask = nullptr;
    QPushButton *btnMoveLayerUp = nullptr;
    QPushButton *btnMoveLayerDown = nullptr;
    QMenu *menuLayerMask = nullptr;
    QAction *actAddMask = nullptr;
    QAction *actDelMask = nullptr;
    QAction *actToggleMask = nullptr;
    QAction *actInvertMask = nullptr;
    QAction *actApplyMask = nullptr;
    QAction *actAddColorMask = nullptr;
    QAction *actDelColorMask = nullptr;
    QAction *actToggleColorMask = nullptr;
    QLabel *lblCurrentLayer = nullptr;
    QSlider *sliderLayerOpacity = nullptr;
    QComboBox *comboBlendMode = nullptr;
    QCheckBox *chkLayerLocked = nullptr;
    QFrame *frameSelectorsColor = nullptr;
    QPushButton *btnColor1 = nullptr;
    QPushButton *btnColor2 = nullptr;
    QToolButton *btnEditColors = nullptr;
    int colorObjetivoActivo = 1;
    bool darkMode = false;
    QList<QAbstractButton*> listaBotonesHerramientas;
    QList<QPushButton*> listaBotonesRecientes;
    int indiceRecienteActual = 0;
    QComboBox *comboZoom = nullptr;
    QPushButton *btnZoomLess = nullptr;
    QPushButton *btnZoomMore = nullptr;
    QLabel *lblZoomIndicator = nullptr;
    QLabel *lblUndoIndicator = nullptr;
    QLabel *lblResolutionIndicator = nullptr;
    QString modoActual = "Normal";
    QPushButton *btnPencil = nullptr;
    QPushButton *btnBucket = nullptr;
    QPushButton *btnText = nullptr;
    QPushButton *btnEraser = nullptr;
    QPushButton *btnPicker = nullptr;
    QPushButton *btnMoveTool = nullptr;
    QToolButton *btnMirrorPen = nullptr;
    QToolButton *btnBrushTool = nullptr;
    QToolButton *btnSprayTool = nullptr;
    QToolButton *btnCustomToolAction = nullptr;
    QToolButton *btnCrayonTool = nullptr;
    QToolButton *btnLighten = nullptr;
    QToolButton *btnMarkerTool = nullptr;
    QToolButton *btnPixelStroke = nullptr;
    QToolButton *btnWatercolor = nullptr;
    QToolButton *btnOilBrush = nullptr;
    QToolButton *btnCalligraphy = nullptr;
    QToolButton *btnHighlighter = nullptr;
    QFrame *separadorBarra = nullptr;
    QActionGroup *transparencyGroup = nullptr;
    TextEngine::FormatBar *textFormatBar = nullptr;
    bool actualizandoPanelCapas = false;
    QTimer *layerRefreshTimer = nullptr;
    QFrame *brushesGridFrame = nullptr;
    QFrame *pixelToolsPanel = nullptr;

    mutable QHash<QString, QString> m_assetPathCache;
    mutable QHash<QString, QIcon>   m_iconCache;
    QString m_lastAppliedQss;
    bool    m_lastQssDarkMode = false;
    int     m_lastQssAlpha    = -1;

    QString resolveAssetPath(const QString &filename);
    QIcon   getAssetIcon(const QString &filename);
    QPushButton *makeWindowCtrlButton(const QString &text, const QString &objName, const QString &tooltip);
    void    wireToolButton(QAbstractButton *btn, ToolType tool, const QString &statusMsg = QString());
    void    resetToolButtons(QAbstractButton *activo);
    void    styleColorSwatch(QPushButton *btn, const QColor &color, bool isActive);

    void abrirEditorMascaraColor(int idx);
    void actualizarBotonLazo();
    void actualizarBotonSelectFree();
    void aplicarSelectFreeMode(int mode);
    void aplicarSelectShapeMode(int mode);
    void aplicarVisibilidadModo();
    void handleZoomRequest(double newFactor, QPoint viewportPos);
    void solicitarRefreshCapas();
    void cambiarIdioma(const QString &langCode);
    void actualizarPanelCapas();
    void iniciarAnimacion();
    void detenerAnimacion();
    void actualizarMiniaturasFrames(const QList<QImage> &frames, int currentIndex);
    bool preguntarGuardarCambios();
    bool cambiarModo(const QString &nuevoModo);
    void refrescarDatosZoomUI(double factor);
    void compilarHojasDeEstiloGlobales();
    void inyectarColorAObjeto(const QColor &color);
    void sincronizarGoteroUI(int target, const QColor &color);
    void actualizarEstilosDePrevisualizacion();
    void abrirPaletaAvanzada();
    void subirImagenDisco();
    void exportarGif();
    void nuevoLienzo();
    void abrirArchivo();
    bool abrirProyectoDesdeArchivo(const QString &fileName);
    bool abrirImagenDesdeArchivo(const QString &fileName);
    bool guardarImagen();
    bool guardarImagenComo();
    bool guardarProyecto();
    bool guardarProyectoComo();
    void actualizarTitulo();
    void aplicarUndoPreset(int maxStates);
    void aplicarUndoRam(int ramStates);
    void guardarUndoSettings();
    void cargarUndoSettings();
protected:
    void closeEvent(QCloseEvent *event) override;
public:
    mainwind();
};

#endif // MAINWINDOW_H