#include "paintarea.h"
#include "utils/ExifLoader.h"

static QRect computeDiffRect(const QImage &a, const QImage &b) {
    if (a.isNull() || b.isNull()) return QRect();
    if (a.size() != b.size() || a.format() != b.format()) return QRect(QPoint(0,0), a.size());
    const int w = a.width(), h = a.height(), rowBytes = w * 4;
    int minY = -1, maxY = -1;
    for (int y = 0; y < h; ++y)
        if (std::memcmp(a.constScanLine(y), b.constScanLine(y), rowBytes) != 0) {
            if (minY < 0) minY = y; maxY = y;
        }
    if (minY < 0) return QRect();
    int minX = w, maxX = -1;
    for (int y = minY; y <= maxY; ++y) {
        const QRgb *ra = reinterpret_cast<const QRgb*>(a.constScanLine(y));
        const QRgb *rb = reinterpret_cast<const QRgb*>(b.constScanLine(y));
        for (int x = 0; x < minX; ++x) if (ra[x] != rb[x]) { minX = x; break; }
        for (int x = w - 1; x > maxX; --x) if (ra[x] != rb[x]) { maxX = x; break; }
    }
    if (maxX < 0 || minX > maxX) return QRect();
    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

FrameThumbnail::FrameThumbnail(const QImage &img, int index, QWidget *parent)
    : QFrame(parent), frameImage(img), frameIndex(index), isSelected(false) {
    setFixedSize(THUMB_SIZE + 8, THUMB_SIZE + 20);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Frame %1").arg(index + 1));
}
void FrameThumbnail::setSelected(bool s) { isSelected = s; update(); }
void FrameThumbnail::setFrameImage(const QImage &i) { frameImage = i; update(); }
int  FrameThumbnail::getFrameIndex() const { return frameIndex; }
void FrameThumbnail::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), isSelected ? QColor("#1a4a7c") : QColor("#2b2b2b"));
    painter.setPen(QPen(isSelected ? QColor("#0066cc") : QColor("#444444"), isSelected ? 2 : 1));
    painter.drawRect(1, 1, width() - 2, height() - 2);
    QPixmap checker(8, 8); checker.fill(Qt::white);
    QPainter p(&checker);
    p.fillRect(0,0,4,4, QColor(210,210,210)); p.fillRect(4,4,4,4, QColor(210,210,210));
    p.end();
    painter.fillRect(2, 5, width() - 4, THUMB_SIZE - 2, QBrush(checker));
    if (!frameImage.isNull()) {
        QImage scaled = frameImage.scaled(THUMB_SIZE, THUMB_SIZE, Qt::KeepAspectRatio, Qt::FastTransformation);
        painter.drawImage((width() - scaled.width()) / 2, 4, scaled);
    }
    painter.setPen(isSelected ? Qt::white : QColor("#aaa"));
    painter.setFont(QFont("Adwaita Sans", 8));
    painter.drawText(QRect(0, THUMB_SIZE + 6, width(), 14), Qt::AlignCenter, QString::number(frameIndex + 1));
}
void FrameThumbnail::mousePressEvent(QMouseEvent *) { emit clicked(frameIndex); }

QColor PaintArea::selBlue() const {
    if (m_accentColor.isValid()) return m_accentColor;
    return darkModeActive ? QColor("#60a5fa") : QColor("#2563eb");
}
QColor PaintArea::selBlueLight() const {
    if (m_accentColor.isValid()) return m_accentColor.lighter(130);
    return darkModeActive ? QColor("#93c5fd") : QColor("#3b82f6");
}

const BrushSettings& PaintArea::activePreset() const {
    if (ToolManager::isArtistic(currentTool)) return classicToolPreset;
    if (currentTool == ToolType::CustomBrush) return customBrushPresets[activeCustomBrushIndex];
    static BrushSettings empty; return empty;
}
const QImage& PaintArea::activeStamp() const {
    if (ToolManager::isArtistic(currentTool))
        return (activeMouseButton == Qt::RightButton) ? classicToolStampRight : classicToolStamp;
    if (currentTool == ToolType::CustomBrush)
        return (activeMouseButton == Qt::RightButton) ? customBrushStampRight : customBrushStamp;
    static QImage empty; return empty;
}

void PaintArea::refreshAndNotify(bool rec) {
    if (rec) recomponerImagen();
    emit layersChanged(); update();
}
void PaintArea::bakeAllPending() {
    bakeSelection(); bakeActivePath(); bakeTextFrame(); bakeAllObjects();
    cancelSelectFreeVector(); cancelSelectFreeElement();
}
void PaintArea::beginEdit() { bakeAllPending(); saveHistoryState(QStringLiteral("Editar")); }

void PaintArea::beginStroke(const ToolCtx &ctx) {
    Q_UNUSED(ctx);
    QString label;
    switch (currentTool) {
    case ToolType::Pencil: label = QStringLiteral("Lápiz"); break;
    case ToolType::Eraser: label = QStringLiteral("Goma"); break;
    case ToolType::Bucket: label = QStringLiteral("Cubeta"); break;
    case ToolType::Brush: label = QStringLiteral("Pincel"); break;
    case ToolType::Spray: label = QStringLiteral("Spray"); break;
    case ToolType::Crayon: label = QStringLiteral("Crayón"); break;
    case ToolType::Marker: label = QStringLiteral("Marcador"); break;
    case ToolType::Watercolor: label = QStringLiteral("Acuarela"); break;
    case ToolType::OilBrush: label = QStringLiteral("Óleo"); break;
    case ToolType::Calligraphy: label = QStringLiteral("Caligrafía"); break;
    case ToolType::Highlighter: label = QStringLiteral("Resaltador"); break;
    case ToolType::CustomBrush: label = QStringLiteral("Pincel personal"); break;
    case ToolType::Blur: label = QStringLiteral("Desenfoque"); break;
    case ToolType::Heal: label = QStringLiteral("Corrector"); break;
    case ToolType::ShadowBurn: label = QStringLiteral("Subexponer"); break;
    case ToolType::Line: label = QStringLiteral("Línea"); break;
    case ToolType::Rectangle: label = QStringLiteral("Rectángulo"); break;
    case ToolType::Ellipse: label = QStringLiteral("Elipse"); break;
    case ToolType::RoundRect: label = QStringLiteral("Rect redondeado"); break;
    case ToolType::Triangle: label = QStringLiteral("Triángulo"); break;
    case ToolType::RightTriangle: label = QStringLiteral("Triángulo rect"); break;
    case ToolType::Diamond: label = QStringLiteral("Rombo"); break;
    case ToolType::Pentagon: label = QStringLiteral("Pentágono"); break;
    case ToolType::Hexagon: label = QStringLiteral("Hexágono"); break;
    case ToolType::ArrowRight: label = QStringLiteral("Flecha der"); break;
    case ToolType::ArrowLeft: label = QStringLiteral("Flecha izq"); break;
    case ToolType::Star: label = QStringLiteral("Estrella"); break;
    case ToolType::Heart: label = QStringLiteral("Corazón"); break;
    case ToolType::Cube: label = QStringLiteral("Cubo"); break;
    case ToolType::PixelStroke: label = QStringLiteral("Trazo Pixel"); break;
    case ToolType::MirrorPen: label = QStringLiteral("Espejo"); break;
    case ToolType::Lighten: label = QStringLiteral("Aclarar"); break;
    case ToolType::Gradient: label = QStringLiteral("Gradiente"); break;
    case ToolType::Clone: label = QStringLiteral("Clonar"); break;
    case ToolType::Deform: label = QStringLiteral("Deformar"); break;
    case ToolType::Text: label = QStringLiteral("Texto"); break;
    default: label = QStringLiteral("Trazo"); break;
    }
    saveHistoryState(label);
    startPoint = ctx.pos; lastPoint = ctx.pos; currentMousePos = ctx.pos;
    drawing = true;
}
bool PaintArea::endStroke() {
    if (!drawing) return false;
    drawing = false; activeMouseButton = Qt::NoButton;
    emit layersChanged(); update(); return true;
}

void PaintArea::configureMaskEditController() {
    MaskEditController::Context ctx;
    ctx.maskRef = [this]() -> QImage& { return stack.maskRef(stack.maskEditLayer()); };
    ctx.isEditingMask = [this]() { return stack.isEditingMask(); };
    ctx.maskEditLayer = [this]() { return stack.maskEditLayer(); };
    ctx.brushColorFor = [this](Qt::MouseButton b) { return obtenerColorDeTrabajo(b); };
    ctx.brushPresetFor = [this]() -> BrushSettings {
        if (ToolManager::isArtistic(currentTool)) return classicToolPreset;
        if (currentTool == ToolType::CustomBrush) return customBrushPresets[activeCustomBrushIndex];
        BrushSettings s; s.shape = ShapeType::Circle; s.dragMode = DragMode::Continuous;
        s.rotationMode = RotationMode::Fixed;
        s.size = qMax(5, (int)(penWidth * mouseSensitivity * 2));
        return s;
    };
    ctx.isPixelArtMode = [this]() { return pixelOptions.getIsPixelArtMode(); };
    m_maskEdit.setContext(ctx);
    m_maskEdit.onRepaint = [this](const QRect &r) {
        QRect rr = r.intersected(stack.canvasRect());
        if (rr.isEmpty()) return;
        stack.syncRubylithRegion(stack.maskEditLayer(), rr);
        stack.markDirty(rr); stack.clearPreviewCache(); repintarZonaCanvas(rr);
    };
    m_maskEdit.onMaskChanged = [this]() { emit layersChanged(); };
    m_maskEdit.onStatusMessage = [this](const QString &m) { emit statusBarMessage(m); };
}

int PaintArea::margenHerramienta() const {
    int sw = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    if (usaStampDePincel()) {
        const BrushSettings &c = activePreset();
        int base = qMax(c.size, sw);
        int spread = c.shapeElements.isEmpty() ? 0 : base * 2;
        return (int)((base + 12) * 1.25) + spread + c.scatter + 8;
    }
    int mult = ToolManager::marginMultiplier(currentTool);
    if (mult < 0) return m_deform.options().radius + 10;
    return sw * mult + ToolManager::marginOffset(currentTool);
}
QRect PaintArea::rectCanvasAWidget(const QRect &r) const {
    if (r.isEmpty()) return QRect();
    int x1 = (int)floor(r.left() * zoomFactor), y1 = (int)floor(r.top() * zoomFactor);
    int x2 = (int)ceil(r.right() * zoomFactor), y2 = (int)ceil(r.bottom() * zoomFactor);
    return QRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}
void PaintArea::repintarZonaCanvas(const QRect &r) {
    QRect c = r.intersected(stack.canvasRect());
    if (!c.isEmpty()) update(rectCanvasAWidget(c).adjusted(-3, -3, 3, 3));
}
QRect PaintArea::rectSiluetaWidget(const QPoint &widgetPos) const {
    int m = (int)(margenHerramienta() * zoomFactor) + 10;
    return QRect(widgetPos.x() - m, widgetPos.y() - m, m * 2 + 1, m * 2 + 1);
}
void PaintArea::invalidarTrazo(const QPoint &a, const QPoint &b, const QRect &extra) {
    int m = margenHerramienta();
    QRect r = QRect(a, b).normalized().adjusted(-m, -m, m, m).intersected(stack.canvasRect());
    if (!r.isEmpty()) stack.markDirty(r);
    QRect wr = rectCanvasAWidget(r);
    if (!extra.isEmpty()) {
        QRect re = extra.intersected(stack.canvasRect());
        if (!re.isEmpty()) wr = wr.united(rectCanvasAWidget(re));
    }
    if (!wr.isEmpty()) update(wr.adjusted(-2, -2, 2, 2));
}
void PaintArea::invalidarPreviewClone(const QPoint &pos) {
    if (!cloneSourceSet) return;
    int bs = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    QRect sa(cloneSource.x() - bs - 4, cloneSource.y() - bs - 4, bs * 2 + 8, bs * 2 + 8);
    QRect ca(pos.x() - bs - 4, pos.y() - bs - 4, bs * 2 + 8, bs * 2 + 8);
    QRect la = QRect(cloneSource, pos).normalized().adjusted(-4, -4, 4, 4);
    QRect tot = sa.united(ca).united(la).intersected(stack.canvasRect());
    if (!tot.isEmpty()) update(rectCanvasAWidget(tot).adjusted(-4, -4, 4, 4));
}
void PaintArea::renderTiles(QPainter &painter, const QRect &vr) {
    if (!stack.tilesValid()) return;
    QRect cv((int)floor(vr.x()/zoomFactor), (int)floor(vr.y()/zoomFactor),
             (int)ceil(vr.width()/zoomFactor)+2, (int)ceil(vr.height()/zoomFactor)+2);
    cv = cv.intersected(stack.canvasRect());
    if (cv.isEmpty()) return;
    QVector<int> tiles = stack.tilesIntersecting(cv);
    for (int idx : tiles) if (stack.isTileDirty(idx)) stack.refreshTile(idx);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int idx : tiles) {
        QRect tr = stack.tileRect(idx);
        int wx = (int)floor(tr.x()*zoomFactor), wy = (int)floor(tr.y()*zoomFactor);
        int ww = (int)ceil((tr.x()+tr.width())*zoomFactor)-wx, wh = (int)ceil((tr.y()+tr.height())*zoomFactor)-wy;
        painter.save(); painter.resetTransform();
        painter.drawPixmap(wx, wy, ww, wh, stack.tilePixmap(idx));
        painter.restore();
    }
    stack.releasePixmapsOutside(cv, 1);
}
void PaintArea::sincronizarCapasConFrameActual() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    QImage f = animManager.getCurrentFrameImage();
    if (f.isNull()) {
        f = QImage(pixelOptions.getResolution(), pixelOptions.getResolution(), QImage::Format_ARGB32);
        f.fill(Qt::transparent);
    }
    stack.resetWithSingleLayer(f, tr("Capa 1")); update();
}
void PaintArea::guardarFrameActualEnAnimador() {
    if (pixelOptions.getIsPixelArtMode()) animManager.setCurrentFrameImage(stack.compositedImage());
}
void PaintArea::procesarDibujoContinuo() {
    if (!drawing || !usaStampDePincel()) return;
    const BrushSettings &p = activePreset();
    if (!p.isAirbrush && p.dragMode != DragMode::Scattered) return;
    ToolCtx ctx; ctx.action = ToolAction::Move;
    ctx.pos = currentMousePos; ctx.rawPos = hoverPos; ctx.button = activeMouseButton;
    ctx.continuous = true;
    ctx.scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    handleTool(ctx);
}
QColor PaintArea::obtenerColorDeTrabajo(Qt::MouseButton b) {
    if (currentTool == ToolType::Eraser) return Qt::transparent;
    if (activeColorTarget == 2) return (b == Qt::LeftButton) ? penColor2 : penColor1;
    return (b == Qt::LeftButton) ? penColor1 : penColor2;
}
void PaintArea::applyClonStamp(QImage &t, const QPoint &p) {
    if (!cloneSourceSet || cloneBuffer.isNull()) return;
    QPoint off = cloneSource - cloneInitialDest;
    int bs = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    PixelArt::stampClone(t, cloneBuffer, off, p, bs);
}
void PaintArea::aplicarGradienteConfigurado(const QPoint &p1, const QPoint &p2) {
    if (!puedeEditarCapaActual()) return;
    GradientTools::applyGradient(stack.currentImage(), p1, p2, penColor1, penColor2,
        (int)gradientType, gradientOpacity, gradientAngle, gradientReverse, gradientDither,
        gradientUseSecondColor, gradientBlendMode);
}
void PaintArea::openGradientSettings() {
    GradientDialog dlg(darkModeActive, penColor1, penColor2, (int)gradientType, gradientOpacity,
        gradientAngle, gradientReverse, gradientDither, gradientBlendMode, gradientUseSecondColor, this);
    if (dlg.exec() == QDialog::Accepted) {
        gradientType = (GradientType)dlg.getGradientType();
        gradientOpacity = dlg.getOpacity(); gradientAngle = dlg.getAngle();
        gradientReverse = dlg.getReverse(); gradientDither = dlg.getDither();
        gradientBlendMode = dlg.getBlendMode(); gradientUseSecondColor = dlg.getUseSecondColor();
    }
}
void PaintArea::bakeObjectIntoLayer(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    const PaintObject &o = selMgr.objectAt(idx);
    int li = o.layerIndex;
    if (!capaValida(li)) { li = stack.currentIndex(); if (!capaValida(li)) return; }
    selMgr.bakeObjectInto(idx, stack.layerAt(li).image, !pixelOptions.getIsPixelArtMode());
}
void PaintArea::bakeAllObjects() {
    if (!selMgr.hasObjects()) return;
    for (int i = 0; i < selMgr.objectCount(); ++i) bakeObjectIntoLayer(i);
    selMgr.clearObjects(); recomponerImagen();
}
void PaintArea::convertSelectionToObject() {
    if (!selMgr.isActive() || !selMgr.hasBuffer()) { emit statusBarMessage(tr("Sin selección")); return; }
    if (!capaValida()) return;
    saveHistoryState(QStringLiteral("Selección a objeto"));
    PaintObject o;
    o.type = ObjectType::SelectionImage; o.bounds = QRectF(selMgr.rect());
    o.layerIndex = stack.currentIndex(); o.selected = true;
    o.rotation = selMgr.rotation(); o.scaleX = 1.0; o.scaleY = 1.0;
    o.selectionBufferId = selMgr.registerBufferForLoad(selMgr.buffer());
    int idx = selMgr.addObject(o); selMgr.setActiveObjectIndex(idx); selMgr.discard();
    setTool(ToolType::Move); emit statusBarMessage(tr("Convertido en objeto")); refreshAndNotify();
}
void PaintArea::integrateSelectedObjects() {
    QList<int> sel = selMgr.selectedObjectIndices();
    if (sel.isEmpty()) {
        if (selMgr.hasObjects()) {
            saveHistoryState(QStringLiteral("Integrar al lienzo")); bakeAllObjects();
            emit statusBarMessage(tr("Integrado al lienzo")); refreshAndNotify(false);
        } else emit statusBarMessage(tr("Sin objetos"));
        return;
    }
    if (!capaValida()) return;
    saveHistoryState(QStringLiteral("Integrar objetos"));
    for (int i : sel) bakeObjectIntoLayer(i);
    std::sort(sel.begin(), sel.end(), std::greater<int>());
    for (int i : sel) selMgr.removeObjectAt(i);
    selMgr.setActiveObjectIndex(-1);
    emit statusBarMessage(tr("Integrado al lienzo")); refreshAndNotify();
}
void PaintArea::loadTextObjectForEditing(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    if (selMgr.objectAt(idx).type != ObjectType::Text) return;
    PaintObject o = selMgr.takeObjectAt(idx);
    textEdit.loadExisting(o.bounds.toRect(), o.textContent, o.textFont, o.textColor);
    textEdit.setLineSpacing(o.lineSpacing); textEdit.setLetterSpacing(o.letterSpacing);
    lastUsedTextFont = o.textFont; emit textFrameClicked(textEdit.rect.topLeft()); update();
}
void PaintArea::loadShapeObjectForEditing(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    if (selMgr.objectAt(idx).type != ObjectType::Shape) return;
    PaintObject &o = selMgr.objectAt(idx);
    ShapePropertiesDialog dlg(darkModeActive, o.shapeTool, o.fillColor, o.strokeColor, o.strokeWidth,
        o.hollow, o.isFrame, o.frameImage, o.frameImageScale, o.frameImageOffset, this);
    if (dlg.exec() == QDialog::Accepted) {
        saveHistoryState(QStringLiteral("Editar figura"));
        o.fillColor = dlg.getFillColor(); o.strokeColor = dlg.getStrokeColor();
        o.strokeWidth = dlg.getStrokeWidth(); o.hollow = dlg.getHollow();
        o.isFrame = dlg.getIsFrame(); o.frameImage = dlg.getFrameImage();
        o.frameImageScale = dlg.getFrameImageScale(); o.frameImageOffset = dlg.getFrameImageOffset();
        emit statusBarMessage(tr("Figura actualizada")); refreshAndNotify();
    }
}
QRect PaintArea::getRightHandle() const {
    return QRect(stack.width()*zoomFactor, stack.height()*zoomFactor/2 - HANDLE_SIZE/2, HANDLE_SIZE, HANDLE_SIZE);
}
QRect PaintArea::getBottomHandle() const {
    return QRect(stack.width()*zoomFactor/2 - HANDLE_SIZE/2, stack.height()*zoomFactor, HANDLE_SIZE, HANDLE_SIZE);
}
QRect PaintArea::getBottomRightHandle() const {
    return QRect(stack.width()*zoomFactor, stack.height()*zoomFactor, HANDLE_SIZE, HANDLE_SIZE);
}
void PaintArea::updateClassicToolStamp() {
    QColor l = obtenerColorDeTrabajo(Qt::LeftButton), r = obtenerColorDeTrabajo(Qt::RightButton);
    QColor sL = classicToolPreset.mixSecondColor ? r : QColor();
    QColor sR = classicToolPreset.mixSecondColor ? l : QColor();
    classicToolStamp = PaintEngine::generateBrushStamp(classicToolPreset, l, penOpacity, pixelOptions.getIsPixelArtMode(), sL);
    classicToolStampRight = PaintEngine::generateBrushStamp(classicToolPreset, r, penOpacity, pixelOptions.getIsPixelArtMode(), sR);
}
void PaintArea::updateCustomBrushStamp() {
    const BrushSettings &c = customBrushPresets[activeCustomBrushIndex];
    QColor l = obtenerColorDeTrabajo(Qt::LeftButton), r = obtenerColorDeTrabajo(Qt::RightButton);
    QColor sL = c.mixSecondColor ? r : QColor(), sR = c.mixSecondColor ? l : QColor();
    customBrushStamp = PaintEngine::generateBrushStamp(c, l, penOpacity, pixelOptions.getIsPixelArtMode(), sL);
    customBrushStampRight = PaintEngine::generateBrushStamp(c, r, penOpacity, pixelOptions.getIsPixelArtMode(), sR);
}

PaintArea::PaintArea(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StaticContents); setMouseTracking(true); setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus); setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    customBrushPresets[0] = BrushSettings();
    customBrushPresets[1] = BrushSettings();
    customBrushPresets[1].shape = ShapeType::Square;
    classicToolPreset = ArtisticPresets::softBrush();
    lastUsedTextFont = QFont("Adwaita Sans", 24);
    continuousDrawTimer = new QTimer(this);
    connect(continuousDrawTimer, &QTimer::timeout, this, &PaintArea::procesarDibujoContinuo);
    undoPendingFlushTimer = new QTimer(this);
    undoPendingFlushTimer->setSingleShot(true);
    undoPendingFlushTimer->setInterval(800);
    connect(undoPendingFlushTimer, &QTimer::timeout, this, [this]() {
        m_undo.materialize(); stack.flushComposite();
        emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount());
    });
    connect(&textEdit, &TextEngine::EditSession::needsRepaint, this, [this]() { update(); });
    configureMaskEditController();
    QRect sg = QApplication::primaryScreen()->geometry();
    int w = qMin(1280, sg.width() - 200), h = qMin(720, sg.height() - 200);
    QImage img(w, h, QImage::Format_ARGB32); img.fill(Qt::white);
    stack.appendFirstLayer(img, tr("Fondo"));
    m_undo.setLayerImageProvider([this](int idx) -> QImage {
        if (stack.validIndex(idx)) return stack.layerAt(idx).image; return QImage();
    });
    actualizarDimensionesFisicas(); updateClassicToolStamp(); updateCustomBrushStamp();
}

void PaintArea::setActiveColorTarget(int t) { activeColorTarget = t; if (usaStampDePincel()) updateClassicToolStamp(); }
void PaintArea::setMouseSensitivity(double s) { mouseSensitivity = s; m_maskEdit.setMouseSensitivity(s); }
double PaintArea::getMouseSensitivity() const { return mouseSensitivity; }

void PaintArea::bakeActivePath() {
    if (bezierTool.isEmpty()) return;
    if (!capaValida()) { bezierTool.reset(); return; }
    QPainterPath path = bezierTool.hasCompletion() ? bezierTool.takeCompletedPath() : QPainterPath();
    if (path.isEmpty()) { bezierTool.reset(); return; }
    saveHistoryState(QStringLiteral("Trazar path"));
    QPainter p(&stack.currentImage());
    p.setRenderHint(QPainter::Antialiasing, !pixelOptions.getIsPixelArtMode());
    QColor c = obtenerColorDeTrabajo(Qt::LeftButton); c.setAlpha(penOpacity);
    int sw = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    p.setPen(QPen(c, sw, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(QBrush(c)); p.drawPath(path); p.end();
    bezierTool.reset(); emit layersChanged(); recomponerImagen(); update();
}
void PaintArea::setCustomBrushPresets(const BrushSettings &p1, const BrushSettings &p2, int i) {
    customBrushPresets[0] = p1; customBrushPresets[1] = p2;
    activeCustomBrushIndex = i; updateCustomBrushStamp();
}
BrushSettings PaintArea::getCustomBrush(int i) const { return customBrushPresets[i]; }
int PaintArea::getActiveCustomBrushIndex() const { return activeCustomBrushIndex; }
bool PaintArea::hasLayerMask(int i) const { return stack.hasMask(i); }
bool PaintArea::isLayerMaskEnabled(int i) const { return stack.isMaskEnabled(i); }
int  PaintArea::getMaskEditLayer() const { return stack.maskEditLayer(); }
bool PaintArea::isEditingMask() const { return stack.isEditingMask(); }
QImage PaintArea::getMaskPreview(int i) const { return stack.getMaskPreview(i); }

void PaintArea::addLayerMask(int i) {
    if (!stack.validIndex(i)) return;
    saveHistoryState(QStringLiteral("Añadir máscara"));
    stack.addMask(i, stack.layerAt(i).image.size()); refreshAndNotify();
}
void PaintArea::removeLayerMask(int i) {
    if (!stack.hasMask(i)) return;
    saveHistoryState(QStringLiteral("Eliminar máscara"));
    stack.removeMask(i); m_maskEdit.resetBezier(); refreshAndNotify();
}
void PaintArea::toggleLayerMaskEnabled(int i) {
    if (!stack.hasMask(i)) return; stack.toggleMaskEnabled(i); refreshAndNotify();
}
void PaintArea::selectMaskForEditing(int i) {
    if (!stack.hasMask(i)) return;
    stack.setEditLayer(i); emit layersChanged(); update();
}
void PaintArea::selectLayerContentForEditing() {
    if (stack.maskEditLayer() < 0) return;
    stack.exitMaskEdit(); m_maskEdit.resetBezier(); emit layersChanged(); update();
}
void PaintArea::applyMaskToLayer(int i) {
    if (!stack.hasMask(i) || !stack.validIndex(i)) return;
    saveHistoryState(QStringLiteral("Aplicar máscara"));
    stack.layerAt(i).image = stack.applyMaskAndRemove(i, stack.layerAt(i).image); refreshAndNotify();
}
void PaintArea::invertLayerMask(int i) {
    if (!stack.hasMask(i)) return;
    saveHistoryState(QStringLiteral("Invertir máscara")); stack.invertMask(i); refreshAndNotify();
}
bool PaintArea::hasLayerColorMask(int i) const { return stack.hasColorMask(i); }
bool PaintArea::isLayerColorMaskEnabled(int i) const { return stack.isColorMaskEnabled(i); }
FilterParams PaintArea::getLayerColorMaskParams(int i) const { return stack.colorMaskParams(i); }
QImage PaintArea::getColorMaskPreview(int i) const {
    if (!stack.validIndex(i)) return QImage();
    return stack.getColorMaskPreview(i, stack.layerAt(i).image);
}
void PaintArea::addLayerColorMask(int i, const FilterParams &fp) {
    if (!stack.validIndex(i)) return;
    saveHistoryState(QStringLiteral("Añadir máscara color"));
    stack.addColorMask(i, fp); refreshAndNotify();
}
void PaintArea::removeLayerColorMask(int i) {
    if (!stack.hasColorMask(i)) return;
    saveHistoryState(QStringLiteral("Eliminar máscara color"));
    stack.removeColorMask(i); refreshAndNotify();
}
void PaintArea::toggleLayerColorMaskEnabled(int i) {
    if (!stack.hasColorMask(i)) return; stack.toggleColorMaskEnabled(i); refreshAndNotify();
}
void PaintArea::setLiveColorMaskPreview(int i, const FilterParams &fp) {
    if (!stack.validIndex(i)) return; stack.setLiveColorMaskPreview(i, fp); refreshAndNotify();
}
void PaintArea::clearLiveColorMaskPreview(int i) {
    if (!stack.hasLiveColorMaskPreview(i)) return;
    stack.clearLiveColorMaskPreview(i); refreshAndNotify();
}
void PaintArea::addLayer() {
    if (stack.isEmpty()) return;
    saveHistoryState(QStringLiteral("Añadir capa"));
    stack.addLayer(tr("Capa %1").arg(stack.count() + 1)); refreshAndNotify(false);
}
void PaintArea::addImageLayer(const QImage& img) {
    beginEdit(); if (stack.isEmpty()) return;
    stack.addImageLayer(img, tr("Capa Importada")); refreshAndNotify(false);
}
void PaintArea::insertImageAsObject(const QImage &img) {
    if (img.isNull()) return;
    bakeSelection(); bakeTextFrame(); bakeActivePath();
    saveHistoryState(QStringLiteral("Insertar imagen objeto"));
    int cw = stack.width(), ch = stack.height(), iw = img.width(), ih = img.height();
    double ms = qMin((double)cw/iw, (double)ch/ih);
    if (ms < 1.0) { iw = (int)(iw*ms*0.9); ih = (int)(ih*ms*0.9); }
    QRect r((cw-iw)/2, (ch-ih)/2, iw, ih);
    PaintObject o;
    o.type = ObjectType::SelectionImage; o.bounds = QRectF(r);
    o.layerIndex = stack.currentIndex(); o.selected = true;
    o.rotation = 0.0; o.scaleX = 1.0; o.scaleY = 1.0;
    o.selectionBufferId = selMgr.registerBufferForLoad(img);
    int idx = selMgr.addObject(o); selMgr.setActiveObjectIndex(idx);
    setTool(ToolType::Move); emit statusBarMessage(tr("Imagen insertada como objeto."));
    refreshAndNotify();
}
void PaintArea::duplicateLayer() { if (stack.duplicateCurrent()) { saveHistoryState(QStringLiteral("Duplicar capa")); refreshAndNotify(false); } }
void PaintArea::deleteLayer()    { if (stack.deleteCurrent())    { saveHistoryState(QStringLiteral("Eliminar capa")); refreshAndNotify(false); } }
void PaintArea::mergeDown()      { if (stack.mergeDown())        { saveHistoryState(QStringLiteral("Combinar capas")); refreshAndNotify(false); } }
void PaintArea::moveLayerUp()    { if (stack.moveCurrentUp())    { saveHistoryState(QStringLiteral("Mover capa arriba")); refreshAndNotify(false); } }
void PaintArea::moveLayerDown()  { if (stack.moveCurrentDown())  { saveHistoryState(QStringLiteral("Mover capa abajo")); refreshAndNotify(false); } }
void PaintArea::reorderLayer(int f, int t) {
    if (!stack.reorder(f, t)) return;
    saveHistoryState(QStringLiteral("Reordenar capas")); refreshAndNotify(false);
}
void PaintArea::setCurrentLayer(int i)      { if (stack.validIndex(i)) { stack.setCurrentIndex(i); refreshAndNotify(false); } }
void PaintArea::setLayerVisibility(int i, bool v) { if (stack.validIndex(i)) { stack.setVisibility(i, v); refreshAndNotify(false); } }
void PaintArea::setLayerOpacity(int i, double o)  { if (stack.validIndex(i)) { stack.setLayerOpacity(i, o); refreshAndNotify(false); } }
void PaintArea::setLayerBlendMode(int i, int m)   { if (stack.validIndex(i)) { stack.setBlendMode(i, m); refreshAndNotify(false); } }
void PaintArea::setLayerLocked(int i, bool l)     { if (stack.validIndex(i)) { stack.setLocked(i, l); emit layersChanged(); } }
const QList<Layer>& PaintArea::getLayers() const { return stack.layers(); }
int PaintArea::getCurrentLayerIndex() const { return stack.currentIndex(); }
QImage PaintArea::getImage() { return stack.compositedImage(); }
void PaintArea::applyImageFilters(const QImage &img) {
    if (!puedeEditarCapaActual()) { if (capaValida()) emit statusBarMessage(tr("Capa bloqueada")); return; }
    saveHistoryState(QStringLiteral("Aplicar filtros")); stack.currentImage() = img; refreshAndNotify();
}
void PaintArea::flipCurrentLayer(bool h, bool v) {
    if (!puedeEditarCapaActual()) { if (capaValida()) emit statusBarMessage(tr("Capa bloqueada")); return; }
    saveHistoryState(QStringLiteral("Voltear capa")); stack.flipCurrent(h, v); refreshAndNotify(false);
}
void PaintArea::rotateCurrentLayer(int a) {
    if (!puedeEditarCapaActual()) { if (capaValida()) emit statusBarMessage(tr("Capa bloqueada")); return; }
    saveHistoryState(QStringLiteral("Rotar capa")); stack.rotateCurrent(a); refreshAndNotify(false);
}
void PaintArea::magicWandSelect(const QPoint &pos, int tol) {
    if (!puedeEditarCapaActual()) { if (capaValida()) emit statusBarMessage(tr("Capa bloqueada")); return; }
    bakeSelection();
    QImage &img = stack.currentImage();
    MagicWandResult r = MagicWandTools::applyMagicWand(img, pos, tol, true, true);
    if (r.pixelsSelected == 0 || r.boundingBox.isEmpty()) {
        emit statusBarMessage(tr("Varita: sin píxeles similares")); return;
    }
    saveHistoryState(QStringLiteral("Varita mágica"));
    QImage ex = MagicWandTools::extractMaskedRegion(img, r.mask, r.boundingBox);
    MagicWandTools::clearMaskedRegion(img, r.mask);
    selMgr.applyMagicWand(r.boundingBox, ex); refreshAndNotify();
}
void PaintArea::setPixelArtMode(bool active, int res) {
    pixelOptions.setPixelArtMode(active, res);
    clearHistory(); selMgr.clearObjects(); stack.clearEverything();
    m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        animManager.init(res); sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        animManager.deinit();
        QRect sg = QApplication::primaryScreen()->geometry();
        int w = qMin(1280, sg.width() - 200), h = qMin(720, sg.height() - 200);
        QImage img(w, h, QImage::Format_ARGB32); img.fill(Qt::white);
        stack.appendFirstLayer(img, tr("Fondo"));
    }
    actualizarDimensionesFisicas();
    emit layersChanged(); emit resolutionChanged(stack.width(), stack.height()); update();
}
void PaintArea::setGridSize(int s) { pixelOptions.setGridSize(s); update(); }
void PaintArea::setGridActive(bool a) { pixelOptions.setGridActive(a); update(); }
bool PaintArea::isGridActive() const { return pixelOptions.isGridActive(); }
void PaintArea::setPixelArtResolution(int r) {
    if (r == pixelOptions.getResolution() || !pixelOptions.getIsPixelArtMode()) return;
    pixelOptions.setPixelArtResolution(r);
    animManager.init(r); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    actualizarDimensionesFisicas(); emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height()); update();
}
void PaintArea::addFrame() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    saveHistoryState(QStringLiteral("Añadir frame"));
    guardarFrameActualEnAnimador(); animManager.addFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::duplicateFrame() {
    if (!pixelOptions.getIsPixelArtMode() || animManager.getFrames().isEmpty()) return;
    saveHistoryState(QStringLiteral("Duplicar frame"));
    guardarFrameActualEnAnimador(); animManager.duplicateFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::deleteFrame() {
    if (!pixelOptions.getIsPixelArtMode() || animManager.getFrames().size() <= 1) return;
    saveHistoryState(QStringLiteral("Eliminar frame"));
    guardarFrameActualEnAnimador(); animManager.deleteFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::goToFrame(int i) {
    if (!pixelOptions.getIsPixelArtMode() || i < 0 || i >= animManager.getFrames().size()) return;
    if (i == animManager.getCurrentFrameIndex()) return;
    guardarFrameActualEnAnimador(); animManager.goToFrame(i); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::nextFrame() { if (pixelOptions.getIsPixelArtMode()) { int n = animManager.getCurrentFrameIndex()+1; if (n < animManager.getFrames().size()) goToFrame(n); } }
void PaintArea::prevFrame() { if (pixelOptions.getIsPixelArtMode()) { int p = animManager.getCurrentFrameIndex()-1; if (p >= 0) goToFrame(p); } }
void PaintArea::clearHistory() { m_undo.clear(); emit historyChanged(0, 0, 0); }
void PaintArea::saveHistoryState() { saveHistoryState(QStringLiteral("Edición")); }
void PaintArea::saveHistoryState(const QString &l) {
    if (!capaValida()) return;
    m_undo.push(stack.currentImage(), stack.currentIndex(), l);
    emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount());
    undoPendingFlushTimer->start();
}
void PaintArea::undo() {
    if (!m_undo.canUndo()) return;
    UndoCandy::Result r = m_undo.undo(QStringLiteral("Estado actual"));
    if (!r.valid || r.image.isNull()) return;
    if (!stack.validIndex(r.layerIndex)) {
        emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount()); return;
    }
    const QImage prev = stack.layerAt(r.layerIndex).image;
    QRect dirty = stack.canvasRect();
    if (!prev.isNull() && prev.size() == r.image.size() && prev.format() == r.image.format())
        dirty = computeDiffRect(prev, r.image);
    stack.layerAt(r.layerIndex).image = r.image;
    stack.setCurrentIndex(r.layerIndex); stack.recomposeRegion(dirty);
    actualizarDimensionesFisicas(); emit layersChanged();
    emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount()); update();
}
void PaintArea::redo() {
    if (!m_undo.canRedo()) return;
    UndoCandy::Result r = m_undo.redo(QStringLiteral("Estado actual"));
    if (!r.valid || r.image.isNull()) return;
    if (!stack.validIndex(r.layerIndex)) {
        emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount()); return;
    }
    const QImage prev = stack.layerAt(r.layerIndex).image;
    QRect dirty = stack.canvasRect();
    if (!prev.isNull() && prev.size() == r.image.size() && prev.format() == r.image.format())
        dirty = computeDiffRect(prev, r.image);
    stack.layerAt(r.layerIndex).image = r.image;
    stack.setCurrentIndex(r.layerIndex); stack.recomposeRegion(dirty);
    actualizarDimensionesFisicas(); emit layersChanged();
    emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount()); update();
}
int PaintArea::undoAvailableCount() const { return m_undo.undoCount(); }
int PaintArea::redoAvailableCount() const { return m_undo.redoCount(); }
int PaintArea::undoRamCachedCount() const { return m_undo.ramCacheCount(); }
int PaintArea::undoMaxStates() const { return m_undo.maxDiskStates(); }
int PaintArea::undoRamStates() const { return m_undo.ramStates(); }
QString PaintArea::nextUndoLabel() const { return m_undo.nextUndoLabel(); }
QString PaintArea::nextRedoLabel() const { return m_undo.nextRedoLabel(); }
void PaintArea::setUndoMaxStates(int n) { m_undo.setMaxDiskStates(n); emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount()); }
void PaintArea::setUndoRamStates(int n) { m_undo.setRamStates(n); emit historyChanged(m_undo.undoCount(), m_undo.redoCount(), m_undo.ramCacheCount()); }

void PaintArea::setPenColor1(const QColor &c) { penColor1 = c; refreshBrushStamps(); }
void PaintArea::setPenColor2(const QColor &c) { penColor2 = c; refreshBrushStamps(); }
void PaintArea::refreshBrushStamps() {
    if (ToolManager::isArtistic(currentTool)) updateClassicToolStamp();
    else if (currentTool == ToolType::CustomBrush) updateCustomBrushStamp();
}
QColor PaintArea::getPenColor1() const { return penColor1; }
QColor PaintArea::getPenColor2() const { return penColor2; }
void PaintArea::setPenWidth(int w) {
    penWidth = w; m_maskEdit.setPenWidth(w);
    if (ToolManager::isArtistic(currentTool)) { classicToolPreset.size = qMax(5, w*4); updateClassicToolStamp(); }
    else if (currentTool == ToolType::CustomBrush) { customBrushPresets[activeCustomBrushIndex].size = qMax(5, w*4); updateCustomBrushStamp(); }
}
void PaintArea::setPenOpacity(int o) {
    penOpacity = o;
    if (ToolManager::isArtistic(currentTool)) updateClassicToolStamp();
    else if (currentTool == ToolType::CustomBrush) updateCustomBrushStamp();
}
double PaintArea::getZoomFactor() const { return zoomFactor; }
void PaintArea::setZoomFactor(double f) {
    if (f < 0.125) f = 0.125; if (f > 32.0) f = 32.0;
    zoomFactor = f; actualizarDimensionesFisicas(); emit zoomChanged(zoomFactor); update();
}
void PaintArea::actualizarDimensionesFisicas() {
    setFixedSize((int)(stack.width()*zoomFactor)+HANDLE_SIZE, (int)(stack.height()*zoomFactor)+HANDLE_SIZE);
}
QSize PaintArea::canvasSize() const { return stack.canvasSize(); }
void PaintArea::setDarkMode(bool e) { darkModeActive = e; update(); }
bool PaintArea::getDarkMode() const { return darkModeActive; }
void PaintArea::setGradientType(int t) { gradientType = (GradientType)qBound(0, t, 2); }
int  PaintArea::getGradientType() const { return (int)gradientType; }
int  PaintArea::getGradientOpacity() const { return gradientOpacity; }
void PaintArea::setGradientOpacity(int o) { gradientOpacity = qBound(0, o, 255); }
int  PaintArea::getGradientAngle() const { return gradientAngle; }
void PaintArea::setGradientAngle(int a) { gradientAngle = qBound(0, a, 360); }
bool PaintArea::getGradientReverse() const { return gradientReverse; }
void PaintArea::setGradientReverse(bool r) { gradientReverse = r; }
bool PaintArea::getGradientDither() const { return gradientDither; }
void PaintArea::setGradientDither(bool d) { gradientDither = d; }
int  PaintArea::getGradientBlendMode() const { return gradientBlendMode; }
void PaintArea::setGradientBlendMode(int m) { gradientBlendMode = m; }
bool PaintArea::getGradientUseSecondColor() const { return gradientUseSecondColor; }
void PaintArea::setGradientUseSecondColor(bool v) { gradientUseSecondColor = v; }
bool PaintArea::isCloneSourceSet() const { return cloneSourceSet; }
void PaintArea::resetCloneSource() { cloneSourceSet = false; cloneBuffer = QImage(); cloneIsStamping = false; }
void PaintArea::setSelectFreeSubMode(SelectionManager::FreeSubMode m) {
    if (m != SelectionManager::FreeVector) cancelSelectFreeVector();
    if (m != SelectionManager::FreeElement) cancelSelectFreeElement();
    selMgr.setFreeSubMode(m); emit selectFreeSubModeChanged((int)m); update();
}
SelectionManager::FreeSubMode PaintArea::getSelectFreeSubMode() const { return selMgr.getFreeSubMode(); }
void PaintArea::cancelSelectFreeVector()  { if (selMgr.isFreeVectorMode()) { selMgr.cancelFreeVectorMode(); update(); } }
void PaintArea::cancelSelectFreeElement() { selMgr.clearElementHover(); update(); }
void PaintArea::setSelectionShape(SelectionManager::SelectionShape s) { selMgr.setShape(s); update(); }
SelectionManager::SelectionShape PaintArea::getSelectionShape() const { return selMgr.shape(); }

void PaintArea::setTool(ToolType t) {
    if (handleGradientToolReentry(t)) return;
    resetStateForToolSwitch(t); currentTool = t; m_maskEdit.setCurrentTool(t);
    if (t == ToolType::Gradient) openGradientSettings();
    applyToolPreset(t); update();
}
bool PaintArea::handleGradientToolReentry(ToolType t) {
    if (t == ToolType::Gradient && currentTool == ToolType::Gradient) { openGradientSettings(); return true; }
    return false;
}
bool PaintArea::toolKeepsSelectionAlive(ToolType t) const {
    return t == ToolType::Select || t == ToolType::SelectFree || t == ToolType::MagicWand
        || t == ToolType::LassoExtract || t == ToolType::LassoDelete;
}
bool PaintArea::toolKeepsFreeModeAlive(ToolType t) const {
    return t == ToolType::SelectFree || t == ToolType::LassoExtract || t == ToolType::LassoDelete;
}
void PaintArea::resetSelectionForNewTool(ToolType t) {
    if (preserveSelectionOnToolSwitch) preserveSelectionOnToolSwitch = false;
    else if (!toolKeepsSelectionAlive(t)) bakeSelection();
    if (!toolKeepsFreeModeAlive(t)) { cancelSelectFreeVector(); cancelSelectFreeElement(); }
}
void PaintArea::resetToolModes(ToolType t) {
    if (t != ToolType::Text) bakeTextFrame();
    if (t != ToolType::Gradient) drawingGradient = false;
    if (t != ToolType::Move) { movingLayer = false; moveLayerIdx = -1; }
    if (t != ToolType::Clone) cloneIsStamping = false;
    if (t != ToolType::Deform) m_deform.reset();
}
void PaintArea::resetStateForToolSwitch(ToolType t) {
    if (t != ToolType::PenBezier) { bakeActivePath(); m_maskEdit.resetBezier(); }
    resetSelectionForNewTool(t); resetToolModes(t);
    if (stack.isEditingMask() && !herramientaDePintura() && t != ToolType::PenBezier) stack.exitMaskEdit();
}
void PaintArea::applyToolPreset(ToolType t) {
    if (ToolManager::isArtistic(t)) {
        classicToolPreset = ArtisticPresets::presetForTool(t);
        classicToolPreset.size = qMax(5, penWidth * 4); updateClassicToolStamp();
    } else if (t == ToolType::CustomBrush) updateCustomBrushStamp();
}
void PaintArea::clearImage() {
    saveHistoryState(QStringLiteral("Limpiar imagen")); bakeAllPending();
    QSize prev = stack.canvasSize();
    stack.clearEverything(); m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        animManager.init(pixelOptions.getResolution()); sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        QImage img(prev.width(), prev.height(), QImage::Format_ARGB32); img.fill(Qt::white);
        stack.appendFirstLayer(img, tr("Fondo"));
    }
    emit layersChanged(); emit resolutionChanged(stack.width(), stack.height()); update();
}
void PaintArea::crearNuevoLienzo(int w, int h, bool t) {
    if (w < 50) w = 50; if (h < 50) h = 50;
    saveHistoryState(QStringLiteral("Nuevo lienzo")); bakeAllPending();
    stack.clearEverything(); clearHistory(); m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) setPixelArtResolution(qMax(8, qMin(512, qMax(w, h))));
    else {
        QImage img(w, h, QImage::Format_ARGB32); img.fill(t ? Qt::transparent : Qt::white);
        stack.appendFirstLayer(img, tr("Fondo"));
        actualizarDimensionesFisicas(); emit layersChanged();
        emit resolutionChanged(stack.width(), stack.height()); update();
    }
}
bool PaintArea::abrirImagen(const QString &f) {
    bakeAllPending();
    QImage ni = ExifLoader::loadRespectingExif(f);
    if (ni.isNull()) return false;
    saveHistoryState(QStringLiteral("Abrir imagen"));
    stack.clearEverything(); m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        QImage es = ni.scaled(pixelOptions.getResolution(), pixelOptions.getResolution(),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);
        QImage il(pixelOptions.getResolution(), pixelOptions.getResolution(), QImage::Format_ARGB32);
        il.fill(Qt::transparent);
        QPainter p(&il); p.drawImage(0, 0, es); p.end();
        animManager.init(pixelOptions.getResolution()); animManager.setCurrentFrameImage(il);
        sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else stack.appendFirstLayer(ni.convertToFormat(QImage::Format_ARGB32), tr("Fondo"));
    actualizarDimensionesFisicas(); emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height()); update();
    return true;
}
bool PaintArea::guardarImagen(const QString &f, const char *fmt) {
    bakeAllPending(); if (pixelOptions.getIsPixelArtMode()) guardarFrameActualEnAnimador();
    return stack.compositedImage().save(f, fmt);
}
bool PaintArea::guardarComoSvg(const QString &f) {
    bakeAllPending(); if (pixelOptions.getIsPixelArtMode()) guardarFrameActualEnAnimador();
    QImage img = stack.compositedImage(); if (img.isNull()) return false;
    QSvgGenerator g; g.setFileName(f); g.setSize(img.size());
    g.setViewBox(QRect(0, 0, img.width(), img.height()));
    g.setTitle(tr("Dibujo Paint-UX")); g.setDescription(tr("Exportado desde Paint-UX Studio"));
    QPainter p(&g); p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(0, 0, img); p.end();
    return true;
}
bool PaintArea::guardarComoGif(const QString &f, int delay, int scale) {
    if (!pixelOptions.getIsPixelArtMode()) return false;
    guardarFrameActualEnAnimador();
    const QList<QImage> &fr = animManager.getFrames();
    if (fr.isEmpty()) return false;
    GifEncoder enc; return enc.save(f, fr, delay, true, scale);
}
void PaintArea::bakeSelection() {
    if (selMgr.isActive() && selMgr.hasBuffer() && puedeEditarCapaActual()) {
        selMgr.bake(stack.currentImage(), !pixelOptions.getIsPixelArtMode()); recomponerImagen();
    }
    selMgr.discard(); cancelSelectFreeVector(); cancelSelectFreeElement();
    emit layersChanged(); update();
}
void PaintArea::bakeTextFrame() {
    if (textEdit.active && !textEdit.isEmpty()) {
        lastUsedTextFont = textEdit.font;
        PaintObject o;
        o.type = ObjectType::Text; o.bounds = QRectF(textEdit.rect);
        o.textContent = textEdit.text; o.textFont = textEdit.font; o.textColor = textEdit.color;
        o.lineSpacing = qBound(0.1, textEdit.lineSpacing, 30.0);
        o.letterSpacing = qBound(-20.0, textEdit.letterSpacing, 100.0);
        o.layerIndex = stack.currentIndex(); o.selected = false;
        selMgr.addObject(o);
    }
    textEdit.end(); update();
}
void PaintArea::cancelTextFrame() { textEdit.end(); update(); }
void PaintArea::updateTextFrame(const QRect &r, const QFont &f, const QColor &c, double ls, double lt) {
    if (!textEdit.active) textEdit.beginNew(r, f, c);
    else { textEdit.rect = r; textEdit.applyFormat(f, c); }
    textEdit.setLineSpacing(ls); textEdit.setLetterSpacing(lt);
    lastUsedTextFont = f; update();
}
bool PaintArea::isTextFrameActive() const { return textEdit.active; }
QRect PaintArea::getTextFrameRect() const { return textEdit.rect; }
QString PaintArea::getTextFrameContent() const { return textEdit.text; }
QFont PaintArea::getTextFrameFont() const { return textEdit.font; }
QColor PaintArea::getTextFrameColor() const { return textEdit.color; }
void PaintArea::insertTextChar(const QString &c) { textEdit.insert(c); }
void PaintArea::deleteTextChar() { textEdit.backspace(); }
void PaintArea::copiarSeleccion() {
    QList<int> sel = selMgr.selectedObjectIndices();
    if (!sel.isEmpty()) {
        QImage r = selMgr.renderSelectedObjects();
        if (!r.isNull()) { selMgr.setClipboardBuffer(r); QApplication::clipboard()->setImage(r); return; }
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.setClipboardBuffer(selMgr.buffer());
        QApplication::clipboard()->setImage(selMgr.buffer());
    }
}
void PaintArea::cortarSeleccion() {
    QList<int> sel = selMgr.selectedObjectIndices();
    if (!sel.isEmpty()) {
        QImage r = selMgr.renderSelectedObjects();
        if (!r.isNull()) {
            selMgr.setClipboardBuffer(r); QApplication::clipboard()->setImage(r);
            saveHistoryState(QStringLiteral("Cortar objetos")); selMgr.deleteSelectedObjects(); return;
        }
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.setClipboardBuffer(selMgr.buffer());
        QApplication::clipboard()->setImage(selMgr.buffer());
        selMgr.discard(); refreshAndNotify();
    }
}
void PaintArea::pegarClipboard() {
    bakeSelection(); bakeActivePath(); bakeTextFrame();
    QImage img = selMgr.getImageToPaste();
    if (!img.isNull()) {
        saveHistoryState(QStringLiteral("Pegar")); selMgr.pasteAsSelection(img);
        setTool(ToolType::Select); update();
    }
}
void PaintArea::borrarSeleccion() {
    QList<int> sel = selMgr.selectedObjectIndices();
    if (!sel.isEmpty()) { saveHistoryState(QStringLiteral("Eliminar objetos")); selMgr.deleteSelectedObjects(); return; }
    if (selMgr.isActive() && selMgr.hasBuffer()) { selMgr.discard(); refreshAndNotify(); }
}
void PaintArea::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls() || e->mimeData()->hasImage()) e->acceptProposedAction();
}
void PaintArea::dropEvent(QDropEvent *e) {
    QImage img;
    if (e->mimeData()->hasUrls()) img = ExifLoader::loadRespectingExif(e->mimeData()->urls().first().toLocalFile());
    else if (e->mimeData()->hasImage()) img = qvariant_cast<QImage>(e->mimeData()->imageData());
    if (img.isNull()) return;
    QPoint cp((int)(e->position().x()/zoomFactor), (int)(e->position().y()/zoomFactor));
    int hit = selMgr.findObjectAt(QPointF(cp));
    if (hit >= 0 && selMgr.objectAt(hit).type == ObjectType::Shape) {
        bakeSelection(); bakeActivePath(); bakeTextFrame();
        saveHistoryState(QStringLiteral("Imagen en figura"));
        PaintObject &o = selMgr.objectAt(hit);
        o.isFrame = true; o.hollow = false; o.frameImage = img;
        o.frameImageScale = 1.0; o.frameImageOffset = QPointF(0, 0);
        selMgr.selectObject(hit); refreshAndNotify(); return;
    }
    bakeSelection(); bakeActivePath(); bakeTextFrame();
    saveHistoryState(QStringLiteral("Insertar imagen"));
    selMgr.pasteAsSelection(img, QPoint(50, 50)); setTool(ToolType::Select); update();
}
void PaintArea::cambiarDimensionesLienzo(int w, int h) {
    if (pixelOptions.getIsPixelArtMode()) { setPixelArtResolution(qMax(8, qMin(64, w))); return; }
    if (w < 50) w = 50; if (h < 50) h = 50;
    bakeAllPending(); saveHistoryState(QStringLiteral("Cambiar tamaño lienzo"));
    stack.resizeCanvas(w, h); actualizarDimensionesFisicas(); emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height()); update();
}
const QList<QImage>& PaintArea::getFrames() const { return animManager.getFrames(); }
int PaintArea::getCurrentFrameIndex() const { return animManager.getCurrentFrameIndex(); }
bool PaintArea::getIsPixelArtMode() const { return pixelOptions.getIsPixelArtMode(); }
int PaintArea::getPixelGridSize() const { return pixelOptions.getGridSize(); }
int PaintArea::getPixelResolution() const { return pixelOptions.getResolution(); }
void PaintArea::editTextObject(int idx) { loadTextObjectForEditing(idx); }

void PaintArea::paintEvent(QPaintEvent *e) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    const int sw = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    paintCheckerboard(p, QRect(0, 0, (int)(stack.width()*zoomFactor), (int)(stack.height()*zoomFactor)));
    renderTiles(p, e->rect());
    p.save(); p.scale(zoomFactor, zoomFactor);
    if (pixelOptions.isGridActive()) PixelArt::drawGrid(p, stack.compositedImage(), pixelOptions.getGridSize(), zoomFactor);
    selMgr.drawAllObjects(p);
    paintMaskOverlay(p); paintBezierOverlay(p); paintCanvasResizePreview(p);
    paintShapePreview(p, sw); paintActiveSelection(p);
    paintSelectFreeVectorOverlay(p); paintSelectFreeElementOverlay(p);
    paintSelectionPreview(p); paintTextFrame(p); paintGradientPreview(p);
    paintCloneOverlay(p); paintSelectionGizmos(p);
    p.restore();
    paintCanvasHandles(p); paintCursorSilhouette(p, sw);
    if (!stack.isEditingMask()) stack.releaseRubylith();
}
void PaintArea::paintCheckerboard(QPainter &p, const QRect &r) {
    static const QPixmap c = []() {
        QPixmap x(16, 16); x.fill(QColor(255, 255, 255));
        QPainter pc(&x);
        pc.fillRect(0,0,8,8, QColor(210,210,210)); pc.fillRect(8,8,8,8, QColor(210,210,210));
        pc.end(); return x;
    }();
    p.fillRect(r, QBrush(c));
}
void PaintArea::paintMaskOverlay(QPainter &p) {
    if (!stack.isEditingMask()) return;
    const QImage &rub = stack.ensureRubylith(stack.maskEditLayer(), stack.canvasSize());
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(0, 0, rub);
    p.setPen(QPen(QColor(220, 40, 60), 2.0/zoomFactor, Qt::DashLine));
    p.setBrush(Qt::NoBrush); p.drawRect(stack.canvasRect());
    if (currentTool == ToolType::PenBezier && m_maskEdit.hasBezierNodes())
        m_maskEdit.paintBezierOverlay(p, zoomFactor);
}
void PaintArea::paintBezierOverlay(QPainter &p) {
    if (currentTool == ToolType::PenBezier && !stack.isEditingMask()) bezierTool.paint(p, zoomFactor);
}
void PaintArea::paintCanvasResizePreview(QPainter &p) {
    if (!resizingCanvas) return;
    p.setPen(QPen(darkModeActive ? Qt::white : Qt::black, 1.5/zoomFactor, Qt::DashLine));
    p.drawRect(0, 0, previewCanvasSize.x(), previewCanvasSize.y());
}
void PaintArea::paintShapePreview(QPainter &p, int sw) {
    if (!drawing) return;
    if (!ToolManager::isShape(currentTool) && currentTool != ToolType::PixelStroke) return;
    QColor c = obtenerColorDeTrabajo(activeMouseButton); c.setAlpha(penOpacity);
    if ((currentTool == ToolType::PixelStroke || currentTool == ToolType::Line) && pixelOptions.getIsPixelArtMode()) {
        QImage prev = stack.currentImage().copy();
        PixelArt::drawShape(prev, startPoint, lastPoint, c, currentTool);
        p.drawImage(0, 0, prev);
    } else {
        p.setPen(QPen(c, sw, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush); PaintEngine::drawGeometry(p, startPoint, lastPoint, currentTool);
    }
}
void PaintArea::paintActiveSelection(QPainter &p) {
    if (selMgr.isActive() && selMgr.hasBuffer()) selMgr.drawSelectionOverlay(p, zoomFactor);
}
void PaintArea::paintSelectFreeVectorOverlay(QPainter &p) {
    if (currentTool != ToolType::SelectFree || selMgr.getFreeSubMode() != SelectionManager::FreeVector || !selMgr.isFreeVectorMode()) return;
    QPointF h(hoverPos.x()/zoomFactor, hoverPos.y()/zoomFactor);
    selMgr.paintFreeVectorOverlay(p, h, zoomFactor, selBlue());
}
void PaintArea::paintSelectFreeElementOverlay(QPainter &p) {
    if (currentTool != ToolType::SelectFree || selMgr.getFreeSubMode() != SelectionManager::FreeElement || !selMgr.isElementHoverActive()) return;
    selMgr.paintElementHoverOverlay(p, zoomFactor, selBlue());
}
void PaintArea::paintSelectionPreview(QPainter &p) {
    if (!drawing) return;
    if (currentTool != ToolType::Select && currentTool != ToolType::SelectFree) return;
    p.setPen(QPen(selBlue(), 1.5/zoomFactor, Qt::DashLine)); p.setBrush(Qt::NoBrush);
    if (currentTool == ToolType::Select) {
        const QRect r = selMgr.rect();
        switch (selMgr.shape()) {
        case SelectionManager::ShapeEllipse: p.drawEllipse(r); break;
        case SelectionManager::ShapeTriangle: {
            QPolygon t; t << QPoint(r.center().x(), r.top()) << QPoint(r.left(), r.bottom()) << QPoint(r.right(), r.bottom());
            p.drawPolygon(t); break;
        }
        default: p.drawRect(r); break;
        }
    } else p.drawPath(selMgr.path());
}
void PaintArea::paintTextFrame(QPainter &p) {
    if (!textEdit.active) return;
    TextEngine::Style ts = currentTextStyle(); ts.handleSize = textEdit.handleSize;
    textEdit.paint(p, zoomFactor, ts);
}
void PaintArea::paintGradientPreview(QPainter &p) {
    if (!drawingGradient || currentTool != ToolType::Gradient) return;
    auto g = buildGradientForPreview();
    if (g) paintGradientFill(p, g.get());
    paintGradientHandleOverlay(p);
}
std::unique_ptr<QGradient> PaintArea::buildGradientForPreview() const {
    QColor c1 = gradientReverse ? penColor2 : penColor1, c2;
    if (gradientUseSecondColor) c2 = gradientReverse ? penColor1 : penColor2;
    else { c2 = c1; c2.setAlpha(0); }
    c1.setAlpha(gradientOpacity); c2.setAlpha(gradientUseSecondColor ? gradientOpacity : 0);
    std::unique_ptr<QGradient> g;
    switch (gradientType) {
    case GradientLinear: g = std::make_unique<QLinearGradient>(gradientStart, gradientEnd); break;
    case GradientRadial: {
        int r = qMax(1, (int)sqrt(pow(gradientEnd.x()-gradientStart.x(), 2) + pow(gradientEnd.y()-gradientStart.y(), 2)));
        g = std::make_unique<QRadialGradient>(gradientStart, r); break;
    }
    case GradientConic: g = std::make_unique<QConicalGradient>(gradientStart, gradientAngle); break;
    default: return nullptr;
    }
    g->setColorAt(0.0, c1); g->setColorAt(1.0, c2); return g;
}
void PaintArea::paintGradientFill(QPainter &p, QGradient *g) {
    if (!g) return;
    p.setPen(Qt::NoPen); p.setBrush(*g);
    p.setOpacity(0.85); p.drawRect(stack.canvasRect()); p.setOpacity(1.0);
}
void PaintArea::paintGradientHandleOverlay(QPainter &p) {
    p.setPen(QPen(QColor(255, 80, 80), 2.0/zoomFactor, Qt::DashLine)); p.setBrush(Qt::NoBrush);
    p.drawLine(gradientStart, gradientEnd);
    p.setBrush(QColor(255, 80, 80, 220));
    p.drawEllipse(QPointF(gradientStart), 6.0/zoomFactor, 6.0/zoomFactor);
    p.drawEllipse(QPointF(gradientEnd), 6.0/zoomFactor, 6.0/zoomFactor);
}
void PaintArea::paintCloneOverlay(QPainter &p) {
    if (currentTool != ToolType::Clone || !cloneSourceSet) return;
    QPoint off = cloneSource - cloneInitialDest;
    QPoint cs = currentMousePos + off;
    int bs = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    p.setPen(QPen(QColor(100, 200, 255), 2.0/zoomFactor, Qt::SolidLine));
    p.setBrush(QColor(100, 200, 255, 40)); p.drawEllipse(cloneSource, bs, bs);
    p.setPen(QPen(QColor(100, 200, 255), 1.0/zoomFactor, Qt::DashLine));
    p.setBrush(Qt::NoBrush); p.drawLine(cloneSource, currentMousePos);
    if (cloneIsStamping) {
        p.setPen(QPen(QColor(255, 200, 100), 1.5/zoomFactor, Qt::DotLine));
        p.setBrush(Qt::NoBrush); p.drawEllipse(cs, bs, bs);
    }
}
void PaintArea::paintSelectionGizmos(QPainter &p) {
    if (currentTool == ToolType::Move) selMgr.drawObjectGizmos(p, zoomFactor, darkModeActive);
    if (selMgr.isActive() && selMgr.hasBuffer())
        selMgr.drawSelectionGizmo(p, zoomFactor, darkModeActive, selBlue(), selBlueLight());
}
void PaintArea::paintCanvasHandles(QPainter &p) {
    p.setPen(QPen(darkModeActive ? Qt::white : QColor("#404040"), 1));
    p.setBrush(Qt::white);
    p.drawRect(getRightHandle()); p.drawRect(getBottomHandle()); p.drawRect(getBottomRightHandle());
}
void PaintArea::paintCursorSilhouette(QPainter &p, int sw) {
    if (!rect().contains(hoverPos) || textEdit.active) return;
    if (stack.isEditingMask() && herramientaDePintura()) { paintMaskBrushSilhouette(p); return; }
    switch (ToolManager::silhouetteKind(currentTool)) {
    case ToolManager::SilRetouch: paintRetouchSilhouette(p, sw); break;
    case ToolManager::SilDeform: paintDeformSilhouette(p); break;
    case ToolManager::SilClone: if (cloneSourceSet) paintCloneSilhouette(p, sw); break;
    case ToolManager::SilBrush: paintBrushStampSilhouette(p); break;
    default: break;
    }
}
void PaintArea::paintMaskBrushSilhouette(QPainter &p) {
    p.setPen(QPen(darkModeActive ? QColor(255,255,255,140) : QColor(0,0,0,120), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    BrushSettings c;
    if (usaStampDePincel()) c = activePreset();
    else { c.shape = ShapeType::Circle; c.dragMode = DragMode::Continuous; c.rotationMode = RotationMode::Fixed;
           c.size = qMax(5, (int)(penWidth * mouseSensitivity * 2)); }
    double s = qMax(4, c.size) * zoomFactor;
    p.save(); p.setRenderHint(QPainter::Antialiasing, true);
    PaintEngine::drawBrushSilhouette(p, c, s, hoverPos); p.restore();
}
void PaintArea::paintRetouchSilhouette(QPainter &p, int sw) {
    p.setPen(QPen(darkModeActive ? QColor(255,255,255,180) : QColor(0,0,0,150), 1));
    p.setBrush(Qt::NoBrush);
    double r = (sw*2+2) * zoomFactor;
    p.drawEllipse(QPointF(hoverPos), r, r);
}
void PaintArea::paintDeformSilhouette(QPainter &p) { m_deform.paintOverlay(p, hoverPos, zoomFactor, darkModeActive); }
void PaintArea::paintCloneSilhouette(QPainter &p, int sw) {
    p.setPen(QPen(darkModeActive ? QColor(255,255,255,200) : QColor(0,0,0,180), 1.5));
    p.setBrush(Qt::NoBrush);
    int bs = (sw*2) * zoomFactor;
    p.drawEllipse(QPointF(hoverPos), (double)bs, (double)bs);
    p.drawLine(hoverPos.x()-4, hoverPos.y(), hoverPos.x()+4, hoverPos.y());
    p.drawLine(hoverPos.x(), hoverPos.y()-4, hoverPos.x(), hoverPos.y()+4);
}
void PaintArea::paintBrushStampSilhouette(QPainter &p) {
    p.setPen(QPen(darkModeActive ? QColor(255,255,255,120) : QColor(0,0,0,100), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    double s = activePreset().size * zoomFactor;
    p.save(); p.setRenderHint(QPainter::Antialiasing, true);
    PaintEngine::drawBrushSilhouette(p, activePreset(), s, hoverPos); p.restore();
}

#include "moc_paintarea.cpp"