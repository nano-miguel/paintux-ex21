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
#include <QMap>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTime>
#include <QImageReader>
#include <QListWidget>
#include <QTabWidget>
#include <QFileInfo>
#include <QFile>
#include <QUrl>
#include <QStandardPaths>

#include "paintarea.h"
#include "core/ToolManager.h"
#include "filter/ImageFilters.h"
#include "core/CustomBrushes.h"
#include "core/colrs.h"

// ═══════════════════════════════════════════════════════════════════════
//  ComponentSkin — una imagen (PNG/SVG) aplicada a un componente UI
// ═══════════════════════════════════════════════════════════════════════
struct ComponentSkin {
    QString filePath;                       // ruta absoluta al PNG/SVG (vacío = sin skin)
    QColor  tintColor = QColor(0, 0, 0, 0); // tinte opcional (alpha=0 = sin tinte)
    int     opacity = 100;                  // 0-100
    bool    stretch = false;                // false = tile · true = stretch
    bool    enabled = false;                // activo/inactivo

    bool isActive() const { return enabled && !filePath.isEmpty() && QFile::exists(filePath); }

    QVariantMap toVariantMap() const {
        QVariantMap m;
        m["filePath"]  = filePath;
        m["tintColor"] = tintColor.name(QColor::HexArgb);
        m["opacity"]   = opacity;
        m["stretch"]   = stretch;
        m["enabled"]   = enabled;
        return m;
    }
    static ComponentSkin fromVariantMap(const QVariantMap &m) {
        ComponentSkin s;
        s.filePath  = m.value("filePath").toString();
        s.tintColor = QColor(m.value("tintColor", "#00000000").toString());
        s.opacity   = m.value("opacity", 100).toInt();
        s.stretch   = m.value("stretch", false).toBool();
        s.enabled   = m.value("enabled", false).toBool();
        return s;
    }
};

// Claves estándar de componentes con skin
namespace SkinKey {
    inline const char *WorkArea    = "workArea";     // fondo del área de dibujo
    inline const char *Window      = "window";       // fondo raíz del programa
    inline const char *TitleBar    = "titleBar";
    inline const char *SysBar      = "sysBar";
    inline const char *Ribbon      = "ribbon";
    inline const char *Sidebar     = "sidebar";
    inline const char *BottomBar   = "bottomBar";
    inline const char *Panel       = "panel";        // group boxes / menús
    inline const char *Input       = "input";        // botones / combos / line edits
    inline const char *Preview     = "preview";      // miniaturas / scrolls internos
    inline const char *Hover       = "hover";        // hover de botones
    inline const char *Accent      = "accent";       // botones de acento

    inline QStringList all() {
        return { QString::fromLatin1(WorkArea), QString::fromLatin1(Window),
                 QString::fromLatin1(TitleBar), QString::fromLatin1(SysBar),
                 QString::fromLatin1(Ribbon),   QString::fromLatin1(Sidebar),
                 QString::fromLatin1(BottomBar),QString::fromLatin1(Panel),
                 QString::fromLatin1(Input),    QString::fromLatin1(Preview),
                 QString::fromLatin1(Hover),    QString::fromLatin1(Accent) };
    }

    inline QString label(const QString &key) {
        if (key == QLatin1String(WorkArea))  return QObject::tr("Área de trabajo (canvas)");
        if (key == QLatin1String(Window))    return QObject::tr("Fondo de ventana");
        if (key == QLatin1String(TitleBar))  return QObject::tr("Barra de título");
        if (key == QLatin1String(SysBar))    return QObject::tr("Barra superior (sys)");
        if (key == QLatin1String(Ribbon))    return QObject::tr("Ribbon (herramientas)");
        if (key == QLatin1String(Sidebar))   return QObject::tr("Barra lateral");
        if (key == QLatin1String(BottomBar)) return QObject::tr("Barra inferior");
        if (key == QLatin1String(Panel))     return QObject::tr("Paneles / tarjetas / menús");
        if (key == QLatin1String(Input))     return QObject::tr("Entradas / botones");
        if (key == QLatin1String(Preview))   return QObject::tr("Zonas de previsualización");
        if (key == QLatin1String(Hover))     return QObject::tr("Hover de botones");
        if (key == QLatin1String(Accent))    return QObject::tr("Botones de acento");
        return key;
    }
}

struct ThemeColors {
    bool dark;
    QString bgDialog, bgPanel, bgPreview, bgInput, bgHover, bgSelected;
    QString textPrimary, textSecondary, textMuted, textAccent;
    QString border, borderStrong, borderAccent;
    QString accent, accentHover, accentPressed;
    QString success, warning, danger;
    ThemeColors(bool isDark) : dark(isDark) {
        if (isDark) {
            bgDialog="#1C1C1C"; bgPanel="#242424"; bgPreview="#121212";
            bgInput="#2D2D2D"; bgHover="#383838"; bgSelected="#1E3A5F";
            textPrimary="#E8E8E8"; textSecondary="#B0B0B0"; textMuted="#808080";
            textAccent="#7FB0E0"; border="#333333"; borderStrong="#4A4A4A";
            borderAccent="#3B82F6"; accent="#3B82F6"; accentHover="#5A95F5";
            accentPressed="#2E6BC8"; success="#6BA368"; warning="#C89B4A"; danger="#C05A55";
        } else {
            bgDialog="#F5F5F5"; bgPanel="#FAFAFA"; bgPreview="#EEEEEE";
            bgInput="#FFFFFF"; bgHover="#E8E8E8"; bgSelected="#D9E3EE";
            textPrimary="#1C1C1C"; textSecondary="#555555"; textMuted="#888888";
            textAccent="#2E6BC8"; border="#DCDCDC"; borderStrong="#BBBBBB";
            borderAccent="#3B82F6"; accent="#3B82F6"; accentHover="#2E6BC8";
            accentPressed="#1E4A8F"; success="#4E8B4A"; warning="#B8862E"; danger="#B04540";
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════
//  AppTheme — un tema completo (colores + skins)
// ═══════════════════════════════════════════════════════════════════════
struct AppTheme {
    QString name; QString family;
    bool isDark = true; bool isGlass = false;
    QColor bgWindow, bgTitleBar, bgTitleBarHover, bgSysBar, bgRibbon;
    QColor bgSidebar, bgBottomBar, bgPanel, bgInput, bgPreview;
    QColor bgHover, bgSelected;
    QColor textPrimary, textSecondary, textMuted, textOnAccent;
    QColor border, borderStrong;
    QColor accent, accentHover, accentPressed;
    QColor danger, dangerHover;
    int radius = 4; int borderWidth = 1;
    QString fontFamily = "Adwaita Sans";

    // Skins por componente (PNG/SVG)
    QMap<QString, ComponentSkin> skins;

    static AppTheme oscuro();
    static AppTheme claro();
    static AppTheme minimal();
    static AppTheme minimalClaro();
    static AppTheme oneDark(); static AppTheme oneLight();
    static AppTheme github(); static AppTheme githubClaro();
    static AppTheme nord(); static AppTheme nordClaro();
    static AppTheme catppuccin(); static AppTheme catppuccinClaro();
    static AppTheme tokyoNight(); static AppTheme tokyoNightDia();
    static AppTheme dracula(); static AppTheme alucard();
    static AppTheme everforest(); static AppTheme everforestClaro();
    static AppTheme rosePine(); static AppTheme rosePineDawn();
    static AppTheme ayu(); static AppTheme ayuClaro();
    static AppTheme kanagawa(); static AppTheme kanagawaClaro();
    static AppTheme gruvbox(); static AppTheme gruvboxClaro();
    static AppTheme solarized(); static AppTheme solarizedClaro();
    static AppTheme monokai(); static AppTheme monokaiClaro();

    static QList<AppTheme> presets();
    static AppTheme presetByName(const QString &n);
    static AppTheme findFamily(const QString &family, bool dark);

    QVariantMap toVariantMap() const {
        QVariantMap m;
        m["name"]=name; m["family"]=family; m["isDark"]=isDark; m["isGlass"]=isGlass;
        auto addColor=[&](const QString &k,const QColor &c){ m[k]=c.name(QColor::HexArgb); };
        addColor("bgWindow",bgWindow); addColor("bgTitleBar",bgTitleBar);
        addColor("bgTitleBarHover",bgTitleBarHover); addColor("bgSysBar",bgSysBar);
        addColor("bgRibbon",bgRibbon); addColor("bgSidebar",bgSidebar);
        addColor("bgBottomBar",bgBottomBar); addColor("bgPanel",bgPanel);
        addColor("bgInput",bgInput); addColor("bgPreview",bgPreview);
        addColor("bgHover",bgHover); addColor("bgSelected",bgSelected);
        addColor("textPrimary",textPrimary); addColor("textSecondary",textSecondary);
        addColor("textMuted",textMuted); addColor("textOnAccent",textOnAccent);
        addColor("border",border); addColor("borderStrong",borderStrong);
        addColor("accent",accent); addColor("accentHover",accentHover);
        addColor("accentPressed",accentPressed);
        addColor("danger",danger); addColor("dangerHover",dangerHover);
        m["radius"]=radius; m["borderWidth"]=borderWidth; m["fontFamily"]=fontFamily;

        // Skins
        QVariantMap skinsMap;
        for (auto it = skins.constBegin(); it != skins.constEnd(); ++it)
            skinsMap[it.key()] = it.value().toVariantMap();
        m["skins"] = skinsMap;
        return m;
    }
    static AppTheme fromVariantMap(const QVariantMap &m) {
        AppTheme t = AppTheme::oscuro();
        if (m.isEmpty()) return t;
        t.name=m.value("name",t.name).toString();
        t.family=m.value("family",t.family).toString();
        t.isDark=m.value("isDark",t.isDark).toBool();
        t.isGlass=m.value("isGlass",t.isGlass).toBool();
        auto getColor=[&](const QString &k,QColor &c){
            if (m.contains(k)){ QColor v(m.value(k).toString()); if (v.isValid()) c=v; }
        };
        getColor("bgWindow",t.bgWindow); getColor("bgTitleBar",t.bgTitleBar);
        getColor("bgTitleBarHover",t.bgTitleBarHover); getColor("bgSysBar",t.bgSysBar);
        getColor("bgRibbon",t.bgRibbon); getColor("bgSidebar",t.bgSidebar);
        getColor("bgBottomBar",t.bgBottomBar); getColor("bgPanel",t.bgPanel);
        getColor("bgInput",t.bgInput); getColor("bgPreview",t.bgPreview);
        getColor("bgHover",t.bgHover); getColor("bgSelected",t.bgSelected);
        getColor("textPrimary",t.textPrimary); getColor("textSecondary",t.textSecondary);
        getColor("textMuted",t.textMuted); getColor("textOnAccent",t.textOnAccent);
        getColor("border",t.border); getColor("borderStrong",t.borderStrong);
        getColor("accent",t.accent); getColor("accentHover",t.accentHover);
        getColor("accentPressed",t.accentPressed);
        getColor("danger",t.danger); getColor("dangerHover",t.dangerHover);
        t.radius=m.value("radius",t.radius).toInt();
        t.borderWidth=m.value("borderWidth",t.borderWidth).toInt();
        t.fontFamily=m.value("fontFamily",t.fontFamily).toString();

        // Skins
        t.skins.clear();
        const QVariantMap skinsMap = m.value("skins").toMap();
        for (auto it = skinsMap.constBegin(); it != skinsMap.constEnd(); ++it)
            t.skins.insert(it.key(), ComponentSkin::fromVariantMap(it.value().toMap()));
        return t;
    }
};

// ═══════════════════════════════════════════════════════════════════════
//  SkinHelper — renderizar PNG/SVG en QPixmap con tinte
// ═══════════════════════════════════════════════════════════════════════
namespace SkinHelper {
    inline QPixmap renderPixmap(const ComponentSkin &s, bool isDark, const QSize &defaultSize = QSize(96, 96)) {
        if (!s.isActive()) return QPixmap();
        QFileInfo fi(s.filePath);
        const QString ext = fi.suffix().toLower();
        QPixmap pm;
        if (ext == "svg") {
            QSvgRenderer r(s.filePath);
            if (!r.isValid()) return QPixmap();
            QSize sz = r.defaultSize();
            if (sz.isEmpty()) sz = defaultSize;
            QImage img(sz, QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter p(&img);
            r.render(&p);
            p.end();
            pm = QPixmap::fromImage(img);
        } else {
            if (!pm.load(s.filePath)) return QPixmap();
        }
        if (pm.isNull()) return QPixmap();

        // Tinte
        if (s.tintColor.alpha() > 0) {
            QImage tinted = pm.toImage();
            QPainter tp(&tinted);
            tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tp.fillRect(tinted.rect(), s.tintColor);
            tp.end();
            pm = QPixmap::fromImage(tinted);
        }
        // Opacidad
        if (s.opacity < 100) {
            QImage img = pm.toImage();
            QPainter p(&img);
            p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            p.fillRect(img.rect(), QColor(0, 0, 0, (int)(255.0 * s.opacity / 100.0)));
            p.end();
            pm = QPixmap::fromImage(img);
        }
        Q_UNUSED(isDark);
        return pm;
    }

    // Genera reglas QSS para un selector dado
    inline QString qssFor(const ComponentSkin &s, const QString &selector, bool isDark) {
        if (!s.isActive()) return QString();
        QString urlPath = QUrl::fromLocalFile(s.filePath).toString();
        const QString repeat = s.stretch ? QStringLiteral("no-repeat") : QStringLiteral("repeat");
        const QString size   = s.stretch ? QStringLiteral("100% 100%") : QStringLiteral("auto");
        const QString pos    = QStringLiteral("0 0");
        return QString("%1 { background-image: url(\"%2\"); "
                       "background-repeat: %3; background-position: %4; "
                       "background-size: %5; background-attachment: local; }")
                .arg(selector, urlPath, repeat, pos, size);
    }
}

class ThemeEditorDialog : public QDialog {
    Q_OBJECT
public:
    ThemeEditorDialog(const AppTheme &initial, QWidget *parent = nullptr);
    AppTheme currentTheme() const { return m_theme; }
signals:
    void themeChanged(const AppTheme &t);
private:
    AppTheme m_theme;
    QHash<QString, QPushButton*> m_colorButtons;
    QSlider *m_radiusSlider = nullptr;
    QLabel *m_radiusLabel = nullptr;
    QComboBox *m_presetCombo = nullptr;
    QLabel *m_previewTitle = nullptr;
    QWidget *m_previewBox = nullptr;
    QTabWidget *m_tabs = nullptr;

    // Pestaña de skins
    struct SkinRow {
        QLabel *thumb = nullptr;
        QLabel *pathLabel = nullptr;
        QPushButton *btnPick = nullptr;
        QPushButton *btnClear = nullptr;
        QCheckBox *chkEnabled = nullptr;
        QCheckBox *chkStretch = nullptr;
        QSlider *opacity = nullptr;
        QPushButton *btnTint = nullptr;
        QString key;
    };
    QHash<QString, SkinRow> m_skinRows;

    void buildUI();
    void buildColorsTab();
    void buildSkinsTab();
    void refreshSwatches();
    void refreshSkins();
    void updateTheme();
    void applyPreset();
    void onColorClicked(const QString &key);
    void updatePreviewStyle();
    void pickSkinFile(const QString &key);
    void clearSkin(const QString &key);
    void onSkinTint(const QString &key);
};

// ═══════════════════════════════════════════════════════════════════════
//  Diálogos y widgets auxiliares
// ═══════════════════════════════════════════════════════════════════════

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
    int getWidth() const; int getHeight() const; bool isTransparent() const;
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
    bool isSelected, layerVisible, isDarkMode;
    QColor m_accentColor;
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
    void setAccentColor(const QColor &c) { m_accentColor = c; updateStyle(); }
private:
    void updateEyeButton(); void updateStyle();
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

// ═══════════════════════════════════════════════════════════════════════
//  mainwind
// ═══════════════════════════════════════════════════════════════════════
class mainwind : public QMainWindow {
    Q_OBJECT
private:
    AppTheme currentTheme;
    bool darkMode = true;
    int currentWorkspaceAlpha = 191;
    PaintArea *paintArea = nullptr;
    QScrollArea *scrollArea = nullptr;
    QString currentProjectPath, lastImagePath, lastImageExtension = "png";
    bool hasUnsavedChanges = false, suppressUnsavedWarning = false;

    QWidget *m_centralWidget = nullptr;
    QVBoxLayout *m_centralLayout = nullptr;
    QWidget *titleBarWidget = nullptr, *sysBarWidget = nullptr, *ribbonWidget = nullptr;
    QWidget *bottomBarWidget = nullptr, *leftSidebarWidget = nullptr, *contentAreaWidget = nullptr;
    QWidget *m_workAreaContainer = nullptr;

    QPushButton *btnArchivoMenu=nullptr, *btnViewMenu=nullptr, *btnConfiguraciones=nullptr;
    QPushButton *btnQuickSave=nullptr, *btnUndo=nullptr, *btnRedo=nullptr;
    QPushButton *btnRotateFlip=nullptr, *btnThemeToggle=nullptr;
    QPushButton *btnImageFilters=nullptr, *btnCustomBrushesTop=nullptr, *btnCanvasSize=nullptr;
    QFrame *separadorBarra = nullptr;

    QMenu *menuArchivoDesplegable=nullptr, *menuViewDesplegable=nullptr, *menuGrid=nullptr;
    QMenu *menuConfiguracionesDesplegable=nullptr, *menuSensibilidad=nullptr;
    QMenu *menuTransparencia=nullptr, *menuApariencia=nullptr, *menuUndo=nullptr;
    QMenu *menuUndoRam=nullptr, *menuIdiomas=nullptr;
    QMenu *menuCanvasSizeStd=nullptr, *menuCanvasSizePixel=nullptr;

    QAction *actNuevo=nullptr, *actAbrir=nullptr, *actGuardarImagen=nullptr;
    QAction *actGuardarImagenComo=nullptr, *actGuardarProyecto=nullptr;
    QAction *actGuardarProyectoComo=nullptr, *actInsertarImagen=nullptr;
    QAction *actInsertarComoCapa=nullptr, *actAbrirFondo=nullptr;
    QAction *actExportGif=nullptr, *actSalir=nullptr;
    QAction *actNormal=nullptr, *actPixelArt=nullptr, *actAvanzado=nullptr;
    QAction *actUndoLight=nullptr, *actUndoNormal=nullptr, *actUndoWide=nullptr;
    QAction *actUndoDeep=nullptr, *actUndoExtreme=nullptr, *actUndoCustom=nullptr;
    QAction *actUndoRam3=nullptr, *actUndoRam5=nullptr, *actUndoRam8=nullptr, *actUndoRam15=nullptr;
    QAction *actShowUndoIndicator = nullptr;

    QActionGroup *undoPresetsGroup=nullptr, *undoRamGroup=nullptr, *gridGroup=nullptr;
    QActionGroup *modeGroup=nullptr, *sensGroup=nullptr, *themeGroup=nullptr;
    QActionGroup *langGroup=nullptr, *transparencyGroup=nullptr;

    QGroupBox *boxShapes=nullptr, *boxBrushesContainer=nullptr, *boxClipboard=nullptr, *boxAnimation=nullptr;
    QPushButton *btnCopyClipboard=nullptr, *btnCutClipboard=nullptr;
    QToolButton *btnPasteClipboard=nullptr;
    QComboBox *comboPenSize=nullptr, *comboPenOpacity=nullptr;

    QList<QPushButton*> m_shapeButtons;
    QList<ToolType>     m_shapeTools;

    QPushButton *btnPrevFrame=nullptr, *btnNextFrame=nullptr, *btnAddFrame=nullptr;
    QPushButton *btnDupFrame=nullptr, *btnDelFrame=nullptr, *btnPlayAnimation=nullptr;
    QLabel *lblFrameIndicator=nullptr;
    QScrollArea *framesScrollArea=nullptr;
    QWidget *framesContainer=nullptr;
    QHBoxLayout *framesLayout=nullptr;
    QList<FrameThumbnail*> frameThumbnails;
    QSlider *animSpeedSlider=nullptr;
    QTimer *animTimer=nullptr;
    bool isAnimating=false; int animSpeed=200;

    QGroupBox *boxAdvTools=nullptr, *boxVectorTools=nullptr, *boxLayersContainer=nullptr;
    QPushButton *btnMagicWand=nullptr, *btnBlurTool=nullptr, *btnHealTool=nullptr;
    QPushButton *btnShadowTool=nullptr, *btnGradientTool=nullptr, *btnCloneTool=nullptr;
    QPushButton *btnZoomToolSidebar=nullptr, *btnLassoTool=nullptr;
    QToolButton *btnPenBezier=nullptr;
    QMenu *menuLassoModos=nullptr;
    QAction *actLassoAdentro=nullptr, *actLassoAfuera=nullptr;
    int lassoModoActual=0;
    QPushButton *btnSelectBox=nullptr, *btnSelectFree=nullptr;
    QMenu *menuSelectFreeModos=nullptr;
    QAction *actFreeLasso=nullptr, *actFreeVector=nullptr, *actFreeElement=nullptr;
    int selectFreeMode=0;
    QMenu *menuSelectShape=nullptr;
    QAction *actShapeRect=nullptr, *actShapeEllipse=nullptr, *actShapeTriangle=nullptr;
    int selectShapeMode=0;
    QPushButton *btnDeformTool=nullptr;
    DeformSettingsBar *deformSettingsBar=nullptr;

    QScrollArea *layersScrollArea=nullptr;
    QWidget *layersContainer=nullptr;
    QVBoxLayout *layersLayout=nullptr;
    QList<DraggableLayerItem*> draggableLayerItems;
    QPushButton *btnAddLayer=nullptr, *btnDupLayer=nullptr, *btnDelLayer=nullptr;
    QPushButton *btnLayerMask=nullptr, *btnMoveLayerUp=nullptr, *btnMoveLayerDown=nullptr;
    QMenu *menuLayerMask=nullptr;
    QAction *actAddMask=nullptr, *actDelMask=nullptr, *actToggleMask=nullptr;
    QAction *actInvertMask=nullptr, *actApplyMask=nullptr;
    QAction *actAddColorMask=nullptr, *actDelColorMask=nullptr, *actToggleColorMask=nullptr;
    QLabel *lblCurrentLayer=nullptr;
    QSlider *sliderLayerOpacity=nullptr;
    QComboBox *comboBlendMode=nullptr;
    QCheckBox *chkLayerLocked=nullptr;

    QFrame *frameSelectorsColor=nullptr;
    QPushButton *btnColor1=nullptr, *btnColor2=nullptr;
    QToolButton *btnEditColors=nullptr;
    QButtonGroup *groupTargets=nullptr;
    int colorObjetivoActivo=1;
    QList<QAbstractButton*> listaBotonesHerramientas;
    QList<QPushButton*> listaBotonesRecientes, listaBotonesPaleta;
    int indiceRecienteActual=0;

    QComboBox *comboZoom=nullptr;
    QPushButton *btnZoomLess=nullptr, *btnZoomMore=nullptr;
    QLabel *lblZoomIndicator=nullptr, *lblUndoIndicator=nullptr, *lblResolutionIndicator=nullptr;

    QString modoActual = "Normal";
    QPushButton *btnPencil=nullptr, *btnBucket=nullptr, *btnText=nullptr;
    QPushButton *btnEraser=nullptr, *btnPicker=nullptr, *btnMoveTool=nullptr;
    QToolButton *btnMirrorPen=nullptr, *btnBrushTool=nullptr, *btnSprayTool=nullptr;
    QToolButton *btnCustomToolAction=nullptr, *btnCrayonTool=nullptr, *btnLighten=nullptr;
    QToolButton *btnMarkerTool=nullptr, *btnPixelStroke=nullptr;
    QToolButton *btnWatercolor=nullptr, *btnOilBrush=nullptr;
    QToolButton *btnCalligraphy=nullptr, *btnHighlighter=nullptr;

    TextEngine::FormatBar *textFormatBar = nullptr;
    bool actualizandoPanelCapas = false;
    QTimer *layerRefreshTimer = nullptr;
    QFrame *brushesGridFrame = nullptr, *pixelToolsPanel = nullptr;

    mutable QHash<QString, QString> m_assetPathCache;
    mutable QHash<QString, QIcon>   m_iconCache;
    QString m_lastAppliedQss, m_lastThemeSignature;
    int m_lastQssAlpha = -1;

    void setupWindow(); void setupCentralLayout();
    void buildTitleBar(); void buildSysBar();
    void buildMenus(); void buildFileMenu(); void buildViewMenu(); void buildSettingsMenu();
    void buildRibbon(); void buildClipboardGroup(QHBoxLayout*);
    void buildImageGroup(QHBoxLayout*); void buildBrushesGroup(QHBoxLayout*);
    void buildShapesGroup(QHBoxLayout*); void buildPropertiesGroup(QHBoxLayout*);
    void buildAnimationGroup(QHBoxLayout*); void buildColorsGroup(QHBoxLayout*);
    void buildLeftSidebar(); void buildAdvancedToolsGroup(QVBoxLayout*);
    void buildVectorToolsGroup(QVBoxLayout*); void buildLayersGroup(QVBoxLayout*);
    void buildWorkArea(); void buildBottomBar();
    void wireAllSignals(); void wireRibbonSignals(); void wireSidebarSignals();
    void wireCanvasSignals(); void wireMenuSignals();
    void applyInitialState();

    QString buildQssFromTheme(const AppTheme &t, int alpha);
    void applyTheme(const AppTheme &t, bool persist = true);
    void openThemeEditor();
    void loadThemeFromSettings(); void saveThemeToSettings();

    QString resolveAssetPath(const QString &filename);
    QIcon getAssetIcon(const QString &filename);
    QPushButton *makeWindowCtrlButton(const QString &text, const QString &objName, const QString &tooltip);
    void wireToolButton(QAbstractButton *btn, ToolType tool, const QString &statusMsg = QString());
    void resetToolButtons(QAbstractButton *activo);
    void styleColorSwatch(QPushButton *btn, const QColor &color, bool isActive);

    void abrirEditorMascaraColor(int idx);
    void actualizarBotonLazo(); void actualizarBotonSelectFree();
    void aplicarSelectFreeMode(int mode); void aplicarSelectShapeMode(int mode);
    void aplicarVisibilidadModo();
    void handleZoomRequest(double newFactor, QPoint viewportPos);
    void solicitarRefreshCapas(); void cambiarIdioma(const QString &langCode);
    void actualizarPanelCapas();
    void iniciarAnimacion(); void detenerAnimacion();
    void actualizarMiniaturasFrames(const QList<QImage> &frames, int currentIndex);
    bool preguntarGuardarCambios(); bool cambiarModo(const QString &nuevoModo);
    void refrescarDatosZoomUI(double factor);
    void compilarHojasDeEstilosGlobales();
    void compilarHojasDeEstiloGlobales();
    void inyectarColorAObjeto(const QColor &color);
    void sincronizarGoteroUI(int target, const QColor &color);
    void actualizarEstilosDePrevisualizacion();
    void abrirPaletaAvanzada(); void subirImagenDisco(); void exportarGif();
    void nuevoLienzo(); void abrirArchivo();
    bool abrirProyectoDesdeArchivo(const QString &fileName);
    bool abrirImagenDesdeArchivo(const QString &fileName);
    bool guardarImagen(); bool guardarImagenComo();
    bool guardarProyecto(); bool guardarProyectoComo();
    void actualizarTitulo();
    void aplicarUndoPreset(int maxStates); void aplicarUndoRam(int ramStates);
    void guardarUndoSettings(); void cargarUndoSettings();
    void aplicarPaletaDelController(const QString &name, const QVector<QColor> &colors);
    void refrescarAccentEnCapas();

public:
    void toggleTheme();
protected:
    void closeEvent(QCloseEvent *event) override;
public:
    mainwind();
};

#endif // MAINWINDOW_H