#include "paintarea.h"
#include "utils/ExifLoader.h"

FrameThumbnail::FrameThumbnail(const QImage &img, int index, QWidget *parent)
    : QFrame(parent), frameImage(img), frameIndex(index), isSelected(false) {
    setFixedSize(THUMB_SIZE + 8, THUMB_SIZE + 20);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Frame %1").arg(index + 1));
}

void FrameThumbnail::setSelected(bool selected) { isSelected = selected; update(); }
void FrameThumbnail::setFrameImage(const QImage &img) { frameImage = img; update(); }
int  FrameThumbnail::getFrameIndex() const { return frameIndex; }

void FrameThumbnail::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), isSelected ? QColor("#1a4a7c") : QColor("#2b2b2b"));
    painter.setPen(QPen(isSelected ? QColor("#0066cc") : QColor("#444444"), isSelected ? 2 : 1));
    painter.drawRect(1, 1, width() - 2, height() - 2);
    QPixmap checker(8, 8);
    checker.fill(Qt::white);
    QPainter p(&checker);
    p.fillRect(0, 0, 4, 4, QColor(210, 210, 210));
    p.fillRect(4, 4, 4, 4, QColor(210, 210, 210));
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

QColor PaintArea::selBlue() const { return darkModeActive ? QColor("#60a5fa") : QColor("#2563eb"); }
QColor PaintArea::selBlueLight() const { return darkModeActive ? QColor("#93c5fd") : QColor("#3b82f6"); }

const BrushSettings& PaintArea::activePreset() const {
    if (ToolManager::isArtistic(currentTool)) return classicToolPreset;
    if (currentTool == ToolType::CustomBrush) return customBrushPresets[activeCustomBrushIndex];
    static BrushSettings empty;
    return empty;
}

const QImage& PaintArea::activeStamp() const {
    if (ToolManager::isArtistic(currentTool))
        return (activeMouseButton == Qt::RightButton) ? classicToolStampRight : classicToolStamp;
    if (currentTool == ToolType::CustomBrush)
        return (activeMouseButton == Qt::RightButton) ? customBrushStampRight : customBrushStamp;
    static QImage empty;
    return empty;
}

void PaintArea::refreshAndNotify(bool recompose) {
    if (recompose) recomponerImagen();
    emit layersChanged();
    update();
}

void PaintArea::bakeAllPending() {
    bakeSelection(); bakeActivePath(); bakeTextFrame(); bakeAllObjects();
    cancelSelectFreeVector(); cancelSelectFreeElement();
}

void PaintArea::beginEdit() { bakeAllPending(); saveHistoryState(); }

void PaintArea::beginStroke(const ToolCtx &ctx) {
    saveHistoryState();
    startPoint = ctx.pos;
    lastPoint = ctx.pos;
    currentMousePos = ctx.pos;
    drawing = true;
}

bool PaintArea::endStroke() {
    if (!drawing) return false;
    drawing = false;
    activeMouseButton = Qt::NoButton;
    emit layersChanged();
    update();
    return true;
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
        BrushSettings s;
        s.shape = ShapeType::Circle; s.dragMode = DragMode::Continuous;
        s.rotationMode = RotationMode::Fixed;
        s.size = qMax(5, (int)(penWidth * mouseSensitivity * 2));
        return s;
    };
    ctx.isPixelArtMode = [this]() { return pixelOptions.getIsPixelArtMode(); };
    m_maskEdit.setContext(ctx);
    m_maskEdit.onRepaint = [this](const QRect &r) {
        QRect rr = r.intersected(stack.canvasRect());
        if (rr.isEmpty()) return;
        stack.syncMaskAlphaRegion(stack.maskEditLayer(), rr);
        stack.syncRubylithRegion(stack.maskEditLayer(), rr);
        stack.markDirty(rr);
        stack.clearPreviewCache();
        repintarZonaCanvas(rr);
    };
    m_maskEdit.onMaskChanged = [this]() { emit layersChanged(); };
    m_maskEdit.onStatusMessage = [this](const QString &msg) { emit statusBarMessage(msg); };
}

int PaintArea::margenHerramienta() const {
    int sw = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    if (usaStampDePincel()) {
        const BrushSettings &c = activePreset();
        int base = qMax(c.size, sw);
        int compositeSpread = c.shapeElements.isEmpty() ? 0 : base * 2;
        return (int)((base + 12) * 1.25) + compositeSpread + c.scatter + 8;
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

void PaintArea::repintarZonaCanvas(const QRect &canvasRect) {
    QRect c = canvasRect.intersected(stack.canvasRect());
    if (c.isEmpty()) return;
    update(rectCanvasAWidget(c).adjusted(-3, -3, 3, 3));
}

QRect PaintArea::rectSiluetaWidget(const QPoint &widgetPos) const {
    int m = (int)(margenHerramienta() * zoomFactor) + 10;
    return QRect(widgetPos.x() - m, widgetPos.y() - m, m * 2 + 1, m * 2 + 1);
}

void PaintArea::invalidarTrazo(const QPoint &a, const QPoint &b, const QRect &extraCanvas) {
    int m = margenHerramienta();
    QRect r = QRect(a, b).normalized().adjusted(-m, -m, m, m).intersected(stack.canvasRect());
    if (!r.isEmpty()) stack.markDirty(r);
    QRect wr = rectCanvasAWidget(r);
    if (!extraCanvas.isEmpty()) {
        QRect re = extraCanvas.intersected(stack.canvasRect());
        if (!re.isEmpty()) wr = wr.united(rectCanvasAWidget(re));
    }
    if (!wr.isEmpty()) update(wr.adjusted(-2, -2, 2, 2));
}

void PaintArea::invalidarPreviewClone(const QPoint &cursorPos) {
    if (!cloneSourceSet) return;
    int brushSize = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    QRect sourceArea(cloneSource.x() - brushSize - 4, cloneSource.y() - brushSize - 4, brushSize * 2 + 8, brushSize * 2 + 8);
    QRect cursorArea(cursorPos.x() - brushSize - 4, cursorPos.y() - brushSize - 4, brushSize * 2 + 8, brushSize * 2 + 8);
    QRect lineArea = QRect(cloneSource, cursorPos).normalized().adjusted(-4, -4, 4, 4);
    QRect totalArea = sourceArea.united(cursorArea).united(lineArea).intersected(stack.canvasRect());
    if (!totalArea.isEmpty()) update(rectCanvasAWidget(totalArea).adjusted(-4, -4, 4, 4));
}

void PaintArea::renderTiles(QPainter &painter, const QRect &visibleWidgetRect) {
    if (!stack.tilesValid()) return;
    QRect canvasVisible(
        (int)floor(visibleWidgetRect.x() / zoomFactor), (int)floor(visibleWidgetRect.y() / zoomFactor),
        (int)ceil(visibleWidgetRect.width() / zoomFactor) + 2, (int)ceil(visibleWidgetRect.height() / zoomFactor) + 2);
    canvasVisible = canvasVisible.intersected(stack.canvasRect());
    if (canvasVisible.isEmpty()) return;
    QVector<int> tiles = stack.tilesIntersecting(canvasVisible);
    for (int idx : tiles) { if (stack.isTileDirty(idx)) stack.refreshTile(idx); }
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int idx : tiles) {
        QRect tr = stack.tileRect(idx);
        int wx = (int)floor(tr.x() * zoomFactor), wy = (int)floor(tr.y() * zoomFactor);
        int ww = (int)ceil((tr.x() + tr.width()) * zoomFactor) - wx;
        int wh = (int)ceil((tr.y() + tr.height()) * zoomFactor) - wy;
        painter.save(); painter.resetTransform();
        painter.drawPixmap(wx, wy, ww, wh, stack.tilePixmap(idx));
        painter.restore();
    }
}

void PaintArea::sincronizarCapasConFrameActual() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    QImage frameImg = animManager.getCurrentFrameImage();
    if (frameImg.isNull()) {
        frameImg = QImage(pixelOptions.getResolution(), pixelOptions.getResolution(), QImage::Format_ARGB32);
        frameImg.fill(Qt::transparent);
    }
    stack.resetWithSingleLayer(frameImg, tr("Capa 1"));
    update();
}

void PaintArea::guardarFrameActualEnAnimador() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    animManager.setCurrentFrameImage(stack.compositedImage());
}

void PaintArea::procesarDibujoContinuo() {
    if (!drawing || !usaStampDePincel()) return;
    const BrushSettings &preset = activePreset();
    if (!preset.isAirbrush && preset.dragMode != DragMode::Scattered) return;

    ToolCtx ctx;
    ctx.action = ToolAction::Move;
    ctx.pos = currentMousePos;
    ctx.rawPos = hoverPos;
    ctx.button = activeMouseButton;
    ctx.continuous = true;
    ctx.scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));

    handleTool(ctx);
}

QColor PaintArea::obtenerColorDeTrabajo(Qt::MouseButton button) {
    if (currentTool == ToolType::Eraser) return Qt::transparent;
    if (activeColorTarget == 2) return (button == Qt::LeftButton) ? penColor2 : penColor1;
    return (button == Qt::LeftButton) ? penColor1 : penColor2;
}

void PaintArea::applyClonStamp(QImage &target, const QPoint &destPos) {
    if (!cloneSourceSet || cloneBuffer.isNull()) return;
    QPoint offset = cloneSource - cloneInitialDest;
    int brushSize = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    PixelArt::stampClone(target, cloneBuffer, offset, destPos, brushSize);
}

void PaintArea::aplicarGradienteConfigurado(const QPoint &p1, const QPoint &p2) {
    if (!puedeEditarCapaActual()) return;
    GradientTools::applyGradient(stack.currentImage(), p1, p2, penColor1, penColor2,
                                 (int)gradientType, gradientOpacity, gradientAngle,
                                 gradientReverse, gradientDither, gradientUseSecondColor, gradientBlendMode);
}

void PaintArea::openGradientSettings() {
    GradientDialog dlg(darkModeActive, penColor1, penColor2, (int)gradientType, gradientOpacity,
                       gradientAngle, gradientReverse, gradientDither, gradientBlendMode, gradientUseSecondColor, this);
    if (dlg.exec() == QDialog::Accepted) {
        gradientType = (GradientType)dlg.getGradientType();
        gradientOpacity = dlg.getOpacity();
        gradientAngle = dlg.getAngle();
        gradientReverse = dlg.getReverse();
        gradientDither = dlg.getDither();
        gradientBlendMode = dlg.getBlendMode();
        gradientUseSecondColor = dlg.getUseSecondColor();
        QString typeStr = gradientType == GradientLinear ? tr("Lineal") :
                          gradientType == GradientRadial ? tr("Radial") : tr("Cónico");
        QString colorStr = gradientUseSecondColor ? tr("2 colores") : tr("Color → Transp.");
        emit statusBarMessage(tr("Gradiente: %1 | %2° | %3% | %4")
                              .arg(typeStr).arg(gradientAngle).arg(qRound(gradientOpacity / 255.0 * 100)).arg(colorStr));
    }
}

void PaintArea::bakeObjectIntoLayer(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    const PaintObject &obj = selMgr.objectAt(idx);
    int layerIdx = obj.layerIndex;
    if (!capaValida(layerIdx)) {
        layerIdx = stack.currentIndex();
        if (!capaValida(layerIdx)) return;
    }
    selMgr.bakeObjectInto(idx, stack.layerAt(layerIdx).image, !pixelOptions.getIsPixelArtMode());
}

void PaintArea::bakeAllObjects() {
    if (!selMgr.hasObjects()) return;
    for (int i = 0; i < selMgr.objectCount(); ++i) bakeObjectIntoLayer(i);
    selMgr.clearObjects();
    recomponerImagen();
}

void PaintArea::convertSelectionToObject() {
    if (!selMgr.isActive() || !selMgr.hasBuffer()) { emit statusBarMessage(tr("Sin selección")); return; }
    if (!capaValida()) return;
    saveHistoryState();
    selMgr.registerSelectionImageObject(selMgr.rect(), selMgr.buffer(), selMgr.rotation(), stack.currentIndex());
    selMgr.discard();
    setTool(ToolType::Move);
    emit statusBarMessage(tr("Convertido en objeto"));
    refreshAndNotify();
}

void PaintArea::integrateSelectedObjects() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (selected.isEmpty()) {
        if (selMgr.hasObjects()) {
            saveHistoryState(); bakeAllObjects();
            emit statusBarMessage(tr("Integrado al lienzo"));
            refreshAndNotify(false);
        } else emit statusBarMessage(tr("Sin objetos"));
        return;
    }
    if (!capaValida()) return;
    saveHistoryState();
    for (int idx : selected) bakeObjectIntoLayer(idx);
    std::sort(selected.begin(), selected.end(), std::greater<int>());
    for (int idx : selected) selMgr.removeObjectAt(idx);
    selMgr.setActiveObjectIndex(-1);
    emit statusBarMessage(tr("Integrado al lienzo"));
    refreshAndNotify();
}

void PaintArea::loadTextObjectForEditing(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    if (selMgr.objectAt(idx).type != ObjectType::Text) return;
    PaintObject obj = selMgr.takeObjectAt(idx);
    textEdit.loadExisting(obj.bounds.toRect(), obj.textContent, obj.textFont, obj.textColor);
    lastUsedTextFont = obj.textFont;
    emit textFrameClicked(textEdit.rect.topLeft());
    update();
}

void PaintArea::loadShapeObjectForEditing(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    if (selMgr.objectAt(idx).type != ObjectType::Shape) return;
    PaintObject &obj = selMgr.objectAt(idx);
    ShapePropertiesDialog dlg(darkModeActive, obj.shapeTool, obj.fillColor, obj.strokeColor,
                              obj.strokeWidth, obj.hollow, obj.isFrame, obj.frameImage,
                              obj.frameImageScale, obj.frameImageOffset, this);
    if (dlg.exec() == QDialog::Accepted) {
        saveHistoryState();
        obj.fillColor = dlg.getFillColor(); obj.strokeColor = dlg.getStrokeColor();
        obj.strokeWidth = dlg.getStrokeWidth(); obj.hollow = dlg.getHollow();
        obj.isFrame = dlg.getIsFrame(); obj.frameImage = dlg.getFrameImage();
        obj.frameImageScale = dlg.getFrameImageScale(); obj.frameImageOffset = dlg.getFrameImageOffset();
        emit statusBarMessage(tr("Figura actualizada"));
        refreshAndNotify();
    }
}

QRect PaintArea::getRightHandle() const {
    return QRect(stack.width() * zoomFactor, stack.height() * zoomFactor / 2 - HANDLE_SIZE / 2, HANDLE_SIZE, HANDLE_SIZE);
}
QRect PaintArea::getBottomHandle() const {
    return QRect(stack.width() * zoomFactor / 2 - HANDLE_SIZE / 2, stack.height() * zoomFactor, HANDLE_SIZE, HANDLE_SIZE);
}
QRect PaintArea::getBottomRightHandle() const {
    return QRect(stack.width() * zoomFactor, stack.height() * zoomFactor, HANDLE_SIZE, HANDLE_SIZE);
}

void PaintArea::updateClassicToolStamp() {
    QColor leftBase = obtenerColorDeTrabajo(Qt::LeftButton);
    QColor rightBase = obtenerColorDeTrabajo(Qt::RightButton);
    QColor secondL = classicToolPreset.mixSecondColor ? rightBase : QColor();
    QColor secondR = classicToolPreset.mixSecondColor ? leftBase : QColor();
    classicToolStamp = PaintEngine::generateBrushStamp(classicToolPreset, leftBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondL);
    classicToolStampRight = PaintEngine::generateBrushStamp(classicToolPreset, rightBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondR);
}

void PaintArea::updateCustomBrushStamp() {
    const BrushSettings &config = customBrushPresets[activeCustomBrushIndex];
    QColor leftBase = obtenerColorDeTrabajo(Qt::LeftButton);
    QColor rightBase = obtenerColorDeTrabajo(Qt::RightButton);
    QColor secondL = config.mixSecondColor ? rightBase : QColor();
    QColor secondR = config.mixSecondColor ? leftBase : QColor();
    customBrushStamp = PaintEngine::generateBrushStamp(config, leftBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondL);
    customBrushStampRight = PaintEngine::generateBrushStamp(config, rightBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondR);
}

PaintArea::PaintArea(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StaticContents); setMouseTracking(true); setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus); setAttribute(Qt::WA_TranslucentBackground, true); setAutoFillBackground(false);
    customBrushPresets[0] = BrushSettings();
    customBrushPresets[1] = BrushSettings();
    customBrushPresets[1].shape = ShapeType::Square;
    classicToolPreset = ArtisticPresets::softBrush();
    lastUsedTextFont = QFont("Adwaita Sans", 24);
    continuousDrawTimer = new QTimer(this);
    connect(continuousDrawTimer, &QTimer::timeout, this, &PaintArea::procesarDibujoContinuo);
    connect(&textEdit, &TextEngine::EditSession::needsRepaint, this, [this]() { update(); });
    configureMaskEditController();
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int initW = qMin(1280, screenGeometry.width() - 200);
    int initH = qMin(720, screenGeometry.height() - 200);
    QImage defaultImg(initW, initH, QImage::Format_ARGB32);
    defaultImg.fill(Qt::white);
    stack.appendFirstLayer(defaultImg, tr("Fondo"));
    actualizarDimensionesFisicas();
    updateClassicToolStamp();
    updateCustomBrushStamp();
}

void PaintArea::setActiveColorTarget(int target) { activeColorTarget = target; if (usaStampDePincel()) updateClassicToolStamp(); }
void PaintArea::setMouseSensitivity(double sens) { mouseSensitivity = sens; m_maskEdit.setMouseSensitivity(sens); }
double PaintArea::getMouseSensitivity() const { return mouseSensitivity; }

void PaintArea::bakeActivePath() {
    if (bezierTool.isEmpty()) return;
    if (!capaValida()) { bezierTool.reset(); return; }
    QPainterPath path = bezierTool.hasCompletion() ? bezierTool.takeCompletedPath() : QPainterPath();
    if (path.isEmpty()) { bezierTool.reset(); return; }
    saveHistoryState();
    QPainter painter(&stack.currentImage());
    painter.setRenderHint(QPainter::Antialiasing, !pixelOptions.getIsPixelArtMode());
    QColor colorDeUso = obtenerColorDeTrabajo(Qt::LeftButton);
    colorDeUso.setAlpha(penOpacity);
    int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    painter.setPen(QPen(colorDeUso, scaledWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(colorDeUso));
    painter.drawPath(path);
    painter.end();
    bezierTool.reset();
    emit layersChanged(); recomponerImagen(); update();
}

void PaintArea::setCustomBrushPresets(const BrushSettings &p1, const BrushSettings &p2, int activeIndex) {
    customBrushPresets[0] = p1; customBrushPresets[1] = p2;
    activeCustomBrushIndex = activeIndex; updateCustomBrushStamp();
}
BrushSettings PaintArea::getCustomBrush(int index) const { return customBrushPresets[index]; }
int PaintArea::getActiveCustomBrushIndex() const { return activeCustomBrushIndex; }

bool PaintArea::hasLayerMask(int layerIndex) const { return stack.hasMask(layerIndex); }
bool PaintArea::isLayerMaskEnabled(int layerIndex) const { return stack.isMaskEnabled(layerIndex); }
int  PaintArea::getMaskEditLayer() const { return stack.maskEditLayer(); }
bool PaintArea::isEditingMask() const { return stack.isEditingMask(); }
QImage PaintArea::getMaskPreview(int layerIndex) const { return stack.getMaskPreview(layerIndex); }

void PaintArea::addLayerMask(int layerIndex) {
    if (!stack.validIndex(layerIndex)) return;
    stack.addMask(layerIndex, stack.layerAt(layerIndex).image.size());
    emit statusBarMessage(tr("Máscara: NEGRO oculta · BLANCO revela · Goma revela · X intercambia"));
    refreshAndNotify();
}
void PaintArea::removeLayerMask(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    stack.removeMask(layerIndex); m_maskEdit.resetBezier();
    emit statusBarMessage(tr("Máscara eliminada")); refreshAndNotify();
}
void PaintArea::toggleLayerMaskEnabled(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    bool enabled = stack.toggleMaskEnabled(layerIndex);
    emit statusBarMessage(enabled ? tr("Máscara activada") : tr("Máscara desactivada")); refreshAndNotify();
}
void PaintArea::selectMaskForEditing(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    stack.setEditLayer(layerIndex);
    emit layersChanged();
    emit statusBarMessage(tr("Editando MÁSCARA: negro oculta / blanco revela")); update();
}
void PaintArea::selectLayerContentForEditing() {
    if (stack.maskEditLayer() < 0) return;
    stack.exitMaskEdit(); m_maskEdit.resetBezier();
    emit layersChanged(); update();
}
void PaintArea::applyMaskToLayer(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    if (!stack.validIndex(layerIndex)) return;
    saveHistoryState();
    stack.layerAt(layerIndex).image = stack.applyMaskAndRemove(layerIndex, stack.layerAt(layerIndex).image);
    emit statusBarMessage(tr("Máscara aplicada a la capa")); refreshAndNotify();
}
void PaintArea::invertLayerMask(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    stack.invertMask(layerIndex);
    emit statusBarMessage(tr("Máscara invertida")); refreshAndNotify();
}

bool PaintArea::hasLayerColorMask(int layerIndex) const { return stack.hasColorMask(layerIndex); }
bool PaintArea::isLayerColorMaskEnabled(int layerIndex) const { return stack.isColorMaskEnabled(layerIndex); }
FilterParams PaintArea::getLayerColorMaskParams(int layerIndex) const { return stack.colorMaskParams(layerIndex); }

QImage PaintArea::getColorMaskPreview(int layerIndex) const {
    if (!stack.validIndex(layerIndex)) return QImage();
    return stack.getColorMaskPreview(layerIndex, stack.layerAt(layerIndex).image);
}
void PaintArea::addLayerColorMask(int layerIndex, const FilterParams &fp) {
    if (!stack.validIndex(layerIndex)) return;
    saveHistoryState();
    stack.addColorMask(layerIndex, fp);
    emit statusBarMessage(tr("Máscara de COLOR añadida")); refreshAndNotify();
}
void PaintArea::removeLayerColorMask(int layerIndex) {
    if (!stack.hasColorMask(layerIndex)) return;
    saveHistoryState(); stack.removeColorMask(layerIndex);
    emit statusBarMessage(tr("Máscara de color eliminada")); refreshAndNotify();
}
void PaintArea::toggleLayerColorMaskEnabled(int layerIndex) {
    if (!stack.hasColorMask(layerIndex)) return;
    bool enabled = stack.toggleColorMaskEnabled(layerIndex);
    emit statusBarMessage(enabled ? tr("Máscara de color ACTIVADA") : tr("Máscara de color DESACTIVADA")); refreshAndNotify();
}
void PaintArea::setLiveColorMaskPreview(int layerIndex, const FilterParams &fp) {
    if (!stack.validIndex(layerIndex)) return;
    stack.setLiveColorMaskPreview(layerIndex, fp); refreshAndNotify();
}
void PaintArea::clearLiveColorMaskPreview(int layerIndex) {
    if (!stack.hasLiveColorMaskPreview(layerIndex)) return;
    stack.clearLiveColorMaskPreview(layerIndex); refreshAndNotify();
}

void PaintArea::addLayer() { if (stack.isEmpty()) return; stack.addLayer(tr("Capa %1").arg(stack.count() + 1)); refreshAndNotify(false); }
void PaintArea::addImageLayer(const QImage& img) { beginEdit(); if (stack.isEmpty()) return; stack.addImageLayer(img, tr("Capa Importada")); refreshAndNotify(false); }

void PaintArea::insertImageAsObject(const QImage &img) {
    if (img.isNull()) return;
    bakeSelection(); bakeTextFrame(); bakeActivePath(); saveHistoryState();
    int canvasW = stack.width(), canvasH = stack.height();
    int imgW = img.width(), imgH = img.height();
    double maxScale = qMin((double)canvasW / imgW, (double)canvasH / imgH);
    if (maxScale < 1.0) { imgW = (int)(imgW * maxScale * 0.9); imgH = (int)(imgH * maxScale * 0.9); }
    int posX = (canvasW - imgW) / 2, posY = (canvasH - imgH) / 2;
    QRect objRect(posX, posY, imgW, imgH);
    selMgr.registerSelectionImageObject(objRect, img, 0.0, stack.currentIndex());
    setTool(ToolType::Move);
    emit statusBarMessage(tr("Imagen insertada como objeto."));
    refreshAndNotify();
}

void PaintArea::duplicateLayer() { if (!stack.duplicateCurrent()) return; refreshAndNotify(false); }
void PaintArea::deleteLayer()    { if (!stack.deleteCurrent())    return; refreshAndNotify(false); }
void PaintArea::mergeDown()      { if (!stack.mergeDown())        return; refreshAndNotify(false); }
void PaintArea::moveLayerUp()    { if (!stack.moveCurrentUp())    return; refreshAndNotify(false); }
void PaintArea::moveLayerDown()  { if (!stack.moveCurrentDown())  return; refreshAndNotify(false); }
void PaintArea::reorderLayer(int fromIndex, int toIndex) { if (!stack.reorder(fromIndex, toIndex)) return; refreshAndNotify(false); }
void PaintArea::setCurrentLayer(int index) { if (!stack.validIndex(index)) return; stack.setCurrentIndex(index); refreshAndNotify(false); }
void PaintArea::setLayerVisibility(int index, bool visible) { if (!stack.validIndex(index)) return; stack.setVisibility(index, visible); refreshAndNotify(false); }
void PaintArea::setLayerOpacity(int index, double opacity) { if (!stack.validIndex(index)) return; stack.setLayerOpacity(index, opacity); refreshAndNotify(false); }
void PaintArea::setLayerBlendMode(int index, int mode) { if (!stack.validIndex(index)) return; stack.setBlendMode(index, mode); refreshAndNotify(false); }
void PaintArea::setLayerLocked(int index, bool locked) { if (!stack.validIndex(index)) return; stack.setLocked(index, locked); emit layersChanged(); }
const QList<Layer>& PaintArea::getLayers() const { return stack.layers(); }
int PaintArea::getCurrentLayerIndex() const { return stack.currentIndex(); }
QImage PaintArea::getImage() { return stack.compositedImage(); }

void PaintArea::applyImageFilters(const QImage &filteredImage) {
    if (!puedeEditarCapaActual()) { if (capaValida()) emit statusBarMessage(tr("Capa bloqueada")); return; }
    saveHistoryState(); stack.currentImage() = filteredImage; refreshAndNotify();
}

void PaintArea::flipCurrentLayer(bool horizontal, bool vertical) {
    if (!puedeEditarCapaActual()) {
        if (capaValida()) emit statusBarMessage(tr("Capa bloqueada"));
        return;
    }
    saveHistoryState(); stack.flipCurrent(horizontal, vertical); refreshAndNotify(false);
}

void PaintArea::rotateCurrentLayer(int angle) {
    if (!puedeEditarCapaActual()) {
        if (capaValida()) emit statusBarMessage(tr("Capa bloqueada"));
        return;
    }
    saveHistoryState(); stack.rotateCurrent(angle); refreshAndNotify(false);
}

void PaintArea::magicWandSelect(const QPoint &pos, int tolerance) {
    if (!puedeEditarCapaActual()) { if (capaValida()) emit statusBarMessage(tr("Capa bloqueada")); return; }
    bakeSelection();
    QImage &layerImg = stack.currentImage();
    MagicWandResult result = MagicWandTools::applyMagicWand(layerImg, pos, tolerance, true, true);
    if (result.pixelsSelected == 0 || result.boundingBox.isEmpty()) {
        emit statusBarMessage(tr("Varita: sin píxeles similares")); return;
    }
    saveHistoryState();
    QImage extracted = MagicWandTools::extractMaskedRegion(layerImg, result.mask, result.boundingBox);
    MagicWandTools::clearMaskedRegion(layerImg, result.mask);
    selMgr.applyMagicWand(result.boundingBox, extracted);
    emit statusBarMessage(tr("Varita: %1 píxeles seleccionados").arg(result.pixelsSelected));
    refreshAndNotify();
}

void PaintArea::setPixelArtMode(bool active, int resolution) {
    pixelOptions.setPixelArtMode(active, resolution);
    clearHistory(); selMgr.clearObjects(); stack.clearEverything();
    m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        animManager.init(resolution); sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        animManager.deinit();
        QRect screenGeometry = QApplication::primaryScreen()->geometry();
        int w = qMin(1280, screenGeometry.width() - 200), h = qMin(720, screenGeometry.height() - 200);
        QImage nuevaImagen(w, h, QImage::Format_ARGB32); nuevaImagen.fill(Qt::white);
        stack.appendFirstLayer(nuevaImagen, tr("Fondo"));
    }
    actualizarDimensionesFisicas();
    emit layersChanged(); emit resolutionChanged(stack.width(), stack.height()); update();
}
void PaintArea::setGridSize(int size) { pixelOptions.setGridSize(size); update(); }
void PaintArea::setGridActive(bool active) { pixelOptions.setGridActive(active); update(); }
bool PaintArea::isGridActive() const { return pixelOptions.isGridActive(); }

void PaintArea::setPixelArtResolution(int resolution) {
    if (resolution != pixelOptions.getResolution() && pixelOptions.getIsPixelArtMode()) {
        pixelOptions.setPixelArtResolution(resolution);
        animManager.init(resolution); sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
        actualizarDimensionesFisicas(); emit layersChanged();
        emit resolutionChanged(stack.width(), stack.height()); update();
    }
}

void PaintArea::addFrame() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    saveHistoryState(); guardarFrameActualEnAnimador();
    animManager.addFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::duplicateFrame() {
    if (!pixelOptions.getIsPixelArtMode() || animManager.getFrames().isEmpty()) return;
    saveHistoryState(); guardarFrameActualEnAnimador();
    animManager.duplicateFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::deleteFrame() {
    if (!pixelOptions.getIsPixelArtMode() || animManager.getFrames().size() <= 1) return;
    saveHistoryState(); guardarFrameActualEnAnimador();
    animManager.deleteFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::goToFrame(int index) {
    if (!pixelOptions.getIsPixelArtMode() || index < 0 || index >= animManager.getFrames().size()) return;
    if (index == animManager.getCurrentFrameIndex()) return;
    guardarFrameActualEnAnimador(); animManager.goToFrame(index); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::nextFrame() {
    if (pixelOptions.getIsPixelArtMode()) {
        int nextIdx = animManager.getCurrentFrameIndex() + 1;
        if (nextIdx < animManager.getFrames().size()) goToFrame(nextIdx);
    }
}
void PaintArea::prevFrame() {
    if (pixelOptions.getIsPixelArtMode()) {
        int prevIdx = animManager.getCurrentFrameIndex() - 1;
        if (prevIdx >= 0) goToFrame(prevIdx);
    }
}

void PaintArea::clearHistory() { undoStack.clear(); redoStack.clear(); }
void PaintArea::saveHistoryState() {
    if (capaValida()) {
        undoStack.append(stack.currentImage());
        if (undoStack.size() > MAX_HISTORY) undoStack.removeFirst();
    }
    redoStack.clear();
}
void PaintArea::undo() {
    if (undoStack.isEmpty()) return;
    bakeAllPending();
    redoStack.append(stack.currentImage());
    stack.currentImage() = undoStack.takeLast();
    recomponerImagen(); actualizarDimensionesFisicas();
    emit layersChanged(); emit resolutionChanged(stack.width(), stack.height()); update();
}
void PaintArea::redo() {
    if (redoStack.isEmpty()) return;
    bakeAllPending();
    undoStack.append(stack.currentImage());
    stack.currentImage() = redoStack.takeLast();
    recomponerImagen(); actualizarDimensionesFisicas();
    emit layersChanged(); emit resolutionChanged(stack.width(), stack.height()); update();
}

void PaintArea::setPenColor1(const QColor &c) { penColor1 = c; refreshBrushStamps(); }
void PaintArea::setPenColor2(const QColor &c) { penColor2 = c; refreshBrushStamps(); }
void PaintArea::refreshBrushStamps() {
    if (ToolManager::isArtistic(currentTool)) updateClassicToolStamp();
    else if (currentTool == ToolType::CustomBrush) updateCustomBrushStamp();
}
QColor PaintArea::getPenColor1() const { return penColor1; }
QColor PaintArea::getPenColor2() const { return penColor2; }
void PaintArea::setPenWidth(int newWidth) {
    penWidth = newWidth; m_maskEdit.setPenWidth(newWidth);
    if (ToolManager::isArtistic(currentTool)) { classicToolPreset.size = qMax(5, newWidth * 4); updateClassicToolStamp(); }
    else if (currentTool == ToolType::CustomBrush) { customBrushPresets[activeCustomBrushIndex].size = qMax(5, newWidth * 4); updateCustomBrushStamp(); }
}
void PaintArea::setPenOpacity(int opacity) {
    penOpacity = opacity;
    if (ToolManager::isArtistic(currentTool)) updateClassicToolStamp();
    else if (currentTool == ToolType::CustomBrush) updateCustomBrushStamp();
}

double PaintArea::getZoomFactor() const { return zoomFactor; }
void PaintArea::setZoomFactor(double factor) {
    if (factor < 0.125) factor = 0.125; if (factor > 32.0) factor = 32.0;
    zoomFactor = factor; actualizarDimensionesFisicas(); emit zoomChanged(zoomFactor); update();
}
void PaintArea::actualizarDimensionesFisicas() {
    setFixedSize((int)(stack.width() * zoomFactor) + HANDLE_SIZE, (int)(stack.height() * zoomFactor) + HANDLE_SIZE);
}
QSize PaintArea::canvasSize() const { return stack.canvasSize(); }
void PaintArea::setDarkMode(bool enabled) { darkModeActive = enabled; update(); }
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

void PaintArea::setSelectFreeSubMode(SelectionManager::FreeSubMode mode) {
    if (mode != SelectionManager::FreeVector) cancelSelectFreeVector();
    if (mode != SelectionManager::FreeElement) cancelSelectFreeElement();
    selMgr.setFreeSubMode(mode);
    emit selectFreeSubModeChanged((int)mode); update();
}
SelectionManager::FreeSubMode PaintArea::getSelectFreeSubMode() const { return selMgr.getFreeSubMode(); }
void PaintArea::cancelSelectFreeVector() { if (selMgr.isFreeVectorMode()) { selMgr.cancelFreeVectorMode(); update(); } }
void PaintArea::cancelSelectFreeElement() { selMgr.clearElementHover(); update(); }

void PaintArea::setTool(ToolType tool) {
    if (handleGradientToolReentry(tool)) return;
    resetStateForToolSwitch(tool);
    currentTool = tool;
    m_maskEdit.setCurrentTool(tool);
    if (tool == ToolType::Gradient) openGradientSettings();
    applyToolPreset(tool);
    update();
}
bool PaintArea::handleGradientToolReentry(ToolType tool) {
    if (tool == ToolType::Gradient && currentTool == ToolType::Gradient) { openGradientSettings(); return true; }
    return false;
}

bool PaintArea::toolKeepsSelectionAlive(ToolType tool) const {
    return tool == ToolType::Select
        || tool == ToolType::SelectFree
        || tool == ToolType::MagicWand
        || tool == ToolType::LassoExtract
        || tool == ToolType::LassoDelete;
}

bool PaintArea::toolKeepsFreeModeAlive(ToolType tool) const {
    return tool == ToolType::SelectFree
        || tool == ToolType::LassoExtract
        || tool == ToolType::LassoDelete;
}

void PaintArea::resetSelectionForNewTool(ToolType tool) {
    if (preserveSelectionOnToolSwitch) {
        preserveSelectionOnToolSwitch = false;
    } else if (!toolKeepsSelectionAlive(tool)) {
        bakeSelection();
    }
    if (!toolKeepsFreeModeAlive(tool)) {
        cancelSelectFreeVector();
        cancelSelectFreeElement();
    }
}

void PaintArea::resetToolModes(ToolType tool) {
    if (tool != ToolType::Text)     bakeTextFrame();
    if (tool != ToolType::Gradient) drawingGradient = false;
    if (tool != ToolType::Move)     { movingLayer = false; moveLayerIdx = -1; }
    if (tool != ToolType::Clone)    cloneIsStamping = false;
    if (tool != ToolType::Deform)   m_deform.reset();
}

void PaintArea::resetStateForToolSwitch(ToolType tool) {
    if (tool != ToolType::PenBezier) {
        bakeActivePath();
        m_maskEdit.resetBezier();
    }
    resetSelectionForNewTool(tool);
    resetToolModes(tool);
    if (stack.isEditingMask() && !herramientaDePintura() && tool != ToolType::PenBezier)
        stack.exitMaskEdit();
}

void PaintArea::applyToolPreset(ToolType tool) {
    if (ToolManager::isArtistic(tool)) {
        classicToolPreset = ArtisticPresets::presetForTool(tool);
        classicToolPreset.size = qMax(5, penWidth * 4);
        updateClassicToolStamp();
    } else if (tool == ToolType::CustomBrush) updateCustomBrushStamp();
}

void PaintArea::clearImage() {
    saveHistoryState(); bakeAllPending();
    QSize prevSize = stack.canvasSize();
    stack.clearEverything(); m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        animManager.init(pixelOptions.getResolution()); sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        QImage nuevaImagen(prevSize.width(), prevSize.height(), QImage::Format_ARGB32);
        nuevaImagen.fill(Qt::white);
        stack.appendFirstLayer(nuevaImagen, tr("Fondo"));
    }
    emit layersChanged(); emit resolutionChanged(stack.width(), stack.height()); update();
}

void PaintArea::crearNuevoLienzo(int w, int h, bool transparent) {
    if (w < 50) w = 50; if (h < 50) h = 50;
    saveHistoryState(); bakeAllPending();
    stack.clearEverything(); clearHistory(); m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        setPixelArtResolution(qMax(8, qMin(512, qMax(w, h))));
    } else {
        QImage nuevaImagen(w, h, QImage::Format_ARGB32);
        nuevaImagen.fill(transparent ? Qt::transparent : Qt::white);
        stack.appendFirstLayer(nuevaImagen, tr("Fondo"));
        actualizarDimensionesFisicas(); emit layersChanged();
        emit resolutionChanged(stack.width(), stack.height()); update();
    }
}

bool PaintArea::abrirImagen(const QString &fileName) {
    bakeAllPending();
    QImage nuevaImagen = ExifLoader::loadRespectingExif(fileName);
    if (nuevaImagen.isNull()) return false;
    saveHistoryState(); stack.clearEverything(); m_maskEdit.resetAll(); bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        QImage imagenEscalada = nuevaImagen.scaled(pixelOptions.getResolution(), pixelOptions.getResolution(),
                                                   Qt::KeepAspectRatio, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);
        QImage imgLimpia(pixelOptions.getResolution(), pixelOptions.getResolution(), QImage::Format_ARGB32);
        imgLimpia.fill(Qt::transparent);
        QPainter p(&imgLimpia); p.drawImage(0, 0, imagenEscalada); p.end();
        animManager.init(pixelOptions.getResolution()); animManager.setCurrentFrameImage(imgLimpia);
        sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        stack.appendFirstLayer(nuevaImagen.convertToFormat(QImage::Format_ARGB32), tr("Fondo"));
    }
    actualizarDimensionesFisicas(); emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height()); update();
    return true;
}

bool PaintArea::guardarImagen(const QString &fileName, const char *fileFormat) {
    bakeAllPending();
    if (pixelOptions.getIsPixelArtMode()) guardarFrameActualEnAnimador();
    return stack.compositedImage().save(fileName, fileFormat);
}
bool PaintArea::guardarComoSvg(const QString &fileName) {
    bakeAllPending();
    if (pixelOptions.getIsPixelArtMode()) guardarFrameActualEnAnimador();
    QImage image = stack.compositedImage();
    if (image.isNull()) return false;
    QSvgGenerator generator;
    generator.setFileName(fileName); generator.setSize(image.size());
    generator.setViewBox(QRect(0, 0, image.width(), image.height()));
    generator.setTitle(tr("Dibujo Paint-UX"));
    generator.setDescription(tr("Exportado desde Paint-UX Studio"));
    QPainter painter(&generator);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(0, 0, image);
    painter.end();
    return true;
}
bool PaintArea::guardarComoGif(const QString &fileName, int delayMs, int scale) {
    if (!pixelOptions.getIsPixelArtMode()) return false;
    guardarFrameActualEnAnimador();
    const QList<QImage> &frames = animManager.getFrames();
    if (frames.isEmpty()) return false;
    GifEncoder encoder;
    return encoder.save(fileName, frames, delayMs, true, scale);
}

void PaintArea::bakeSelection() {
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        if (puedeEditarCapaActual()) {
            selMgr.bake(stack.currentImage(), !pixelOptions.getIsPixelArtMode());
            recomponerImagen();
        }
    }
    selMgr.discard(); cancelSelectFreeVector(); cancelSelectFreeElement();
    emit layersChanged(); update();
}
void PaintArea::bakeTextFrame() {
    if (textEdit.active && !textEdit.isEmpty()) {
        lastUsedTextFont = textEdit.font;
        selMgr.registerTextObject(textEdit.rect, textEdit.text, textEdit.font, textEdit.color, stack.currentIndex());
    }
    textEdit.end(); update();
}
void PaintArea::cancelTextFrame() { textEdit.end(); update(); }
void PaintArea::updateTextFrame(const QRect &rect, const QFont &font, const QColor &color) {
    if (!textEdit.active) textEdit.beginNew(rect, font, color);
    else { textEdit.rect = rect; textEdit.applyFormat(font, color); }
    lastUsedTextFont = font; update();
}
bool PaintArea::isTextFrameActive() const { return textEdit.active; }
QRect PaintArea::getTextFrameRect() const { return textEdit.rect; }
QString PaintArea::getTextFrameContent() const { return textEdit.text; }
QFont PaintArea::getTextFrameFont() const { return textEdit.font; }
QColor PaintArea::getTextFrameColor() const { return textEdit.color; }
void PaintArea::insertTextChar(const QString &ch) { textEdit.insert(ch); }
void PaintArea::deleteTextChar() { textEdit.backspace(); }

void PaintArea::copiarSeleccion() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (!selected.isEmpty()) {
        QImage rendered = selMgr.renderSelectedObjects();
        if (!rendered.isNull()) {
            selMgr.setClipboardBuffer(rendered);
            QApplication::clipboard()->setImage(rendered);
            emit statusBarMessage(tr("Objetos copiados (%1)").arg(selected.size()));
            return;
        }
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.setClipboardBuffer(selMgr.buffer());
        QApplication::clipboard()->setImage(selMgr.buffer());
        emit statusBarMessage(tr("Copiado al portapapeles"));
    } else emit statusBarMessage(tr("No hay selección ni objetos"));
}
void PaintArea::cortarSeleccion() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (!selected.isEmpty()) {
        QImage rendered = selMgr.renderSelectedObjects();
        if (!rendered.isNull()) {
            selMgr.setClipboardBuffer(rendered);
            QApplication::clipboard()->setImage(rendered);
            saveHistoryState(); selMgr.deleteSelectedObjects();
            emit statusBarMessage(tr("Objetos cortados (%1)").arg(selected.size()));
            return;
        }
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.setClipboardBuffer(selMgr.buffer());
        QApplication::clipboard()->setImage(selMgr.buffer());
        selMgr.discard();
        emit statusBarMessage(tr("Cortado al portapapeles")); refreshAndNotify();
    } else emit statusBarMessage(tr("No hay selección ni objetos"));
}
void PaintArea::pegarClipboard() {
    bakeSelection(); bakeActivePath(); bakeTextFrame();
    QImage imgToPaste = selMgr.getImageToPaste();
    if (!imgToPaste.isNull()) {
        saveHistoryState(); selMgr.pasteAsSelection(imgToPaste);
        setTool(ToolType::Select); update();
        emit statusBarMessage(tr("Pegado desde portapapeles"));
    } else emit statusBarMessage(tr("Portapapeles vacío"));
}
void PaintArea::borrarSeleccion() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (!selected.isEmpty()) { saveHistoryState(); selMgr.deleteSelectedObjects(); emit statusBarMessage(tr("Objetos eliminados")); return; }
    if (selMgr.isActive() && selMgr.hasBuffer()) { selMgr.discard(); emit statusBarMessage(tr("Selección borrada")); refreshAndNotify(); }
    else emit statusBarMessage(tr("No hay selección ni objetos"));
}

void PaintArea::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) event->acceptProposedAction();
}
void PaintArea::dropEvent(QDropEvent *event) {
    QImage droppedImage;
    if (event->mimeData()->hasUrls()) {
        QString filePath = event->mimeData()->urls().first().toLocalFile();
        droppedImage = ExifLoader::loadRespectingExif(filePath);
    } else if (event->mimeData()->hasImage()) {
        droppedImage = qvariant_cast<QImage>(event->mimeData()->imageData());
    }
    if (!droppedImage.isNull()) {
        QPoint canvasPos((int)(event->position().x() / zoomFactor), (int)(event->position().y() / zoomFactor));
        int hitObj = selMgr.findObjectAt(QPointF(canvasPos));
        if (hitObj >= 0 && selMgr.objectAt(hitObj).type == ObjectType::Shape) {
            bakeSelection(); bakeActivePath(); bakeTextFrame(); saveHistoryState();
            PaintObject &obj = selMgr.objectAt(hitObj);
            obj.isFrame = true; obj.hollow = false; obj.frameImage = droppedImage;
            obj.frameImageScale = 1.0; obj.frameImageOffset = QPointF(0, 0);
            selMgr.selectObject(hitObj);
            emit statusBarMessage(tr("Imagen colocada dentro de la figura")); refreshAndNotify();
            return;
        }
        bakeSelection(); bakeActivePath(); bakeTextFrame(); saveHistoryState();
        selMgr.pasteAsSelection(droppedImage, QPoint(50, 50));
        setTool(ToolType::Select);
        emit statusBarMessage(tr("Imagen insertada")); update();
    }
}

void PaintArea::cambiarDimensionesLienzo(int nuevoW, int nuevoH) {
    if (pixelOptions.getIsPixelArtMode()) { setPixelArtResolution(qMax(8, qMin(64, nuevoW))); return; }
    if (nuevoW < 50) nuevoW = 50; if (nuevoH < 50) nuevoH = 50;
    bakeAllPending(); saveHistoryState();
    stack.resizeCanvas(nuevoW, nuevoH);
    actualizarDimensionesFisicas(); emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height()); update();
}

const QList<QImage>& PaintArea::getFrames() const { return animManager.getFrames(); }
int PaintArea::getCurrentFrameIndex() const { return animManager.getCurrentFrameIndex(); }
bool PaintArea::getIsPixelArtMode() const { return pixelOptions.getIsPixelArtMode(); }
int PaintArea::getPixelGridSize() const { return pixelOptions.getGridSize(); }
int PaintArea::getPixelResolution() const { return pixelOptions.getResolution(); }
void PaintArea::editTextObject(int idx) { loadTextObjectForEditing(idx); }

ToolCtx PaintArea::buildMouseCtx(QMouseEvent *event) const {
    ToolCtx ctx;
    ctx.mouse = event;
    ctx.rawPos = event->position().toPoint();
    ctx.pos = QPoint(ctx.rawPos.x() / zoomFactor, ctx.rawPos.y() / zoomFactor);
    ctx.button = event->button();
    ctx.shift = (event->modifiers() & Qt::ShiftModifier);
    ctx.ctrl  = (event->modifiers() & Qt::ControlModifier);
    ctx.alt   = (event->modifiers() & Qt::AltModifier);
    ctx.scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    return ctx;
}

ToolCtx PaintArea::buildKeyCtx(QKeyEvent *event) const {
    ToolCtx ctx;
    ctx.key = event;
    ctx.shift = (event->modifiers() & Qt::ShiftModifier);
    ctx.ctrl  = (event->modifiers() & Qt::ControlModifier);
    ctx.alt   = (event->modifiers() & Qt::AltModifier);
    return ctx;
}

bool PaintArea::dispatchPress(const ToolCtx &ctx) {
    static const DispatchFn kPress[] = {
        &PaintArea::onPressCanvasHandles,
        &PaintArea::onPressMaskBezier,
        &PaintArea::onPressMaskPaint,
        &PaintArea::onPressTextActive,
        &PaintArea::onPressSelectionGizmo,
    };
    for (DispatchFn fn : kPress) { if ((this->*fn)(ctx)) return true; }
    return false;
}

bool PaintArea::dispatchMove(const ToolCtx &ctx) {
    static const DispatchFn kMove[] = {
        &PaintArea::onMoveSelectionRotate,
        &PaintArea::onMoveSelectionResize,
        &PaintArea::onMoveCanvasResize,
        &PaintArea::onMoveMaskBezier,
        &PaintArea::onMoveMaskPaint,
        &PaintArea::onMoveSelectFreeVector,
        &PaintArea::onMoveSelectFreeElement,
        &PaintArea::onMoveTextActive,
        &PaintArea::onMoveTextHover,
        &PaintArea::onMoveSelectionDrag,
    };
    for (DispatchFn fn : kMove) { if ((this->*fn)(ctx)) return true; }
    return false;
}

bool PaintArea::dispatchRelease(const ToolCtx &ctx) {
    static const DispatchFn kRelease[] = {
        &PaintArea::onReleaseMaskPaint,
        &PaintArea::onReleaseSelectFreeVector,
        &PaintArea::onReleaseTextDragResize,
        &PaintArea::onReleaseCanvasResize,
        &PaintArea::onReleaseSelectionGizmo,
    };
    for (DispatchFn fn : kRelease) { if ((this->*fn)(ctx)) return true; }
    return false;
}

bool PaintArea::dispatchKey(const ToolCtx &ctx) {
    static const DispatchFn kKey[] = {
        &PaintArea::onKeySelectFree,
        &PaintArea::onKeyMaskBezier,
        &PaintArea::onKeyClipboard,
        &PaintArea::onKeyDelete,
        &PaintArea::onKeyObjectShortcuts,
        &PaintArea::onKeyTextEditing,
        &PaintArea::onKeyToolSpecific,
        &PaintArea::onKeyBezierPen,
    };
    for (DispatchFn fn : kKey) { if ((this->*fn)(ctx)) return true; }
    return false;
}

bool PaintArea::handleTool(const ToolCtx &ctx) {
    if (ToolManager::needsEditableLayer(currentTool) && !puedeEditarCapaActual()) {
        if (ctx.action == ToolAction::Press)
            emit statusBarMessage(tr("Capa bloqueada"));
        return true;
    }

    struct Entry { ToolType tool; DispatchFn fn; };
    static const Entry kTable[] = {
        { ToolType::Pencil,        &PaintArea::toolPencil     },
        { ToolType::Eraser,        &PaintArea::toolEraser     },
        { ToolType::Picker,        &PaintArea::toolPicker     },
        { ToolType::Bucket,        &PaintArea::toolBucket     },
        { ToolType::Zoom,          &PaintArea::toolZoom       },
        { ToolType::MagicWand,     &PaintArea::toolMagicWand  },
        { ToolType::PenBezier,     &PaintArea::toolPenBezier  },
        { ToolType::Select,        &PaintArea::toolSelect     },
        { ToolType::SelectFree,    &PaintArea::toolSelectFree },
        { ToolType::Text,          &PaintArea::toolText       },
        { ToolType::Move,          &PaintArea::toolMove       },
        { ToolType::Gradient,      &PaintArea::toolGradient   },
        { ToolType::Clone,         &PaintArea::toolClone      },
        { ToolType::Deform,        &PaintArea::toolDeform     },
        { ToolType::LassoExtract,  &PaintArea::toolLasso      },
        { ToolType::LassoDelete,   &PaintArea::toolLasso      },
        { ToolType::Blur,          &PaintArea::toolRetouch    },
        { ToolType::Heal,          &PaintArea::toolRetouch    },
        { ToolType::ShadowBurn,    &PaintArea::toolRetouch    },
        { ToolType::MirrorPen,     &PaintArea::toolPixelArt   },
        { ToolType::Lighten,       &PaintArea::toolPixelArt   },
        { ToolType::Brush,         &PaintArea::toolBrushStamp },
        { ToolType::Spray,         &PaintArea::toolBrushStamp },
        { ToolType::Crayon,        &PaintArea::toolBrushStamp },
        { ToolType::Marker,        &PaintArea::toolBrushStamp },
        { ToolType::Watercolor,    &PaintArea::toolBrushStamp },
        { ToolType::OilBrush,      &PaintArea::toolBrushStamp },
        { ToolType::Calligraphy,   &PaintArea::toolBrushStamp },
        { ToolType::Highlighter,   &PaintArea::toolBrushStamp },
        { ToolType::CustomBrush,   &PaintArea::toolBrushStamp },
        { ToolType::Line,          &PaintArea::toolShape      },
        { ToolType::Rectangle,     &PaintArea::toolShape      },
        { ToolType::Ellipse,       &PaintArea::toolShape      },
        { ToolType::RoundRect,     &PaintArea::toolShape      },
        { ToolType::Triangle,      &PaintArea::toolShape      },
        { ToolType::RightTriangle, &PaintArea::toolShape      },
        { ToolType::Diamond,       &PaintArea::toolShape      },
        { ToolType::Pentagon,      &PaintArea::toolShape      },
        { ToolType::Hexagon,       &PaintArea::toolShape      },
        { ToolType::ArrowRight,    &PaintArea::toolShape      },
        { ToolType::ArrowLeft,     &PaintArea::toolShape      },
        { ToolType::Star,          &PaintArea::toolShape      },
        { ToolType::Heart,         &PaintArea::toolShape      },
        { ToolType::Cube,          &PaintArea::toolShape      },
        { ToolType::PixelStroke,   &PaintArea::toolShape      },
    };

    DispatchFn fn = nullptr;
    for (const Entry &e : kTable) {
        if (e.tool == currentTool) { fn = e.fn; break; }
    }

    bool handled = false;
    if (fn) handled = (this->*fn)(ctx);

    if (ctx.action == ToolAction::Move && !drawing && !handled && !ctx.continuous) {
        moveCursorUpdate(ctx.pos);
    }
    return handled;
}

void PaintArea::keyPressEvent(QKeyEvent *event) {
    ToolCtx ctx = buildKeyCtx(event);
    ctx.action = ToolAction::Press;
    if (dispatchKey(ctx)) return;
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) { bakeActivePath(); bakeSelection(); return; }
    QWidget::keyPressEvent(event);
}

void PaintArea::mousePressEvent(QMouseEvent *event) {
    setFocus();
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    activeMouseButton = event->button();
    m_maskEdit.setActiveMouseButton(activeMouseButton);
    ToolCtx ctx = buildMouseCtx(event);
    ctx.action = ToolAction::Press;
    if (dispatchPress(ctx)) return;
    if (!stack.canvasRect().contains(ctx.pos)) return;
    handleTool(ctx);
}

void PaintArea::mouseMoveEvent(QMouseEvent *event) {
    ToolCtx ctx = buildMouseCtx(event);
    ctx.action = ToolAction::Move;
    hoverPos = ctx.rawPos;
    previousMousePos = currentMousePos;
    currentMousePos = ctx.pos;
    if (dispatchMove(ctx)) return;
    handleTool(ctx);
}

void PaintArea::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != activeMouseButton) return;
    continuousDrawTimer->stop();
    ToolCtx ctx = buildMouseCtx(event);
    ctx.action = ToolAction::Release;
    m_maskEdit.endBezierDrag();
    if (dispatchRelease(ctx)) return;
    handleTool(ctx);
}

void PaintArea::wheelEvent(QWheelEvent *event) {
    QPoint localPos = event->position().toPoint();
    QPoint viewportPos = mapToParent(localPos);
    double factor = 1.15;
    double newZoom = zoomFactor;
    if (event->angleDelta().y() > 0) newZoom = zoomFactor * factor;
    else if (event->angleDelta().y() < 0) newZoom = zoomFactor / factor;
    else { event->accept(); return; }
    emit zoomRequested(newZoom, viewportPos);
    event->accept();
}

void PaintArea::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    const int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    paintCheckerboard(painter, QRect(0, 0, (int)(stack.width() * zoomFactor), (int)(stack.height() * zoomFactor)));
    renderTiles(painter, event->rect());
    painter.save();
    painter.scale(zoomFactor, zoomFactor);
    if (pixelOptions.isGridActive())
        PixelArt::drawGrid(painter, stack.compositedImage(), pixelOptions.getGridSize(), zoomFactor);
    selMgr.drawAllObjects(painter);
    paintMaskOverlay(painter);
    paintBezierOverlay(painter);
    paintCanvasResizePreview(painter);
    paintShapePreview(painter, scaledWidth);
    paintActiveSelection(painter);
    paintSelectFreeVectorOverlay(painter);
    paintSelectFreeElementOverlay(painter);
    paintSelectionPreview(painter);
    paintTextFrame(painter);
    paintGradientPreview(painter);
    paintCloneOverlay(painter);
    paintSelectionGizmos(painter);
    painter.restore();
    paintCanvasHandles(painter);
    paintCursorSilhouette(painter, scaledWidth);
}

void PaintArea::paintCheckerboard(QPainter &painter, const QRect &canvasRect) {
    static const QPixmap checker = []() {
        QPixmap c(16, 16); c.fill(QColor(255, 255, 255));
        QPainter pc(&c);
        pc.fillRect(0, 0, 8, 8, QColor(210, 210, 210));
        pc.fillRect(8, 8, 8, 8, QColor(210, 210, 210));
        pc.end();
        return c;
    }();
    painter.fillRect(canvasRect, QBrush(checker));
}

void PaintArea::paintMaskOverlay(QPainter &painter) {
    if (!stack.isEditingMask()) return;
    const QImage &rubylithImg = stack.ensureRubylith(stack.maskEditLayer(), stack.canvasSize());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(0, 0, rubylithImg);
    painter.setPen(QPen(QColor(220, 40, 60), 2.0 / zoomFactor, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(stack.canvasRect());
    painter.setPen(Qt::white);
    QFont labFont; labFont.setPixelSize(qMax(10, (int)(12 / zoomFactor))); labFont.setBold(true);
    painter.setFont(labFont);
    painter.fillRect(0, 0, 320 / zoomFactor, 20 / zoomFactor, QColor(220, 40, 60, 200));
    painter.drawText(4 / zoomFactor, 15 / zoomFactor, tr("MÁSCARA: negro oculta / blanco revela"));
    if (currentTool == ToolType::PenBezier && m_maskEdit.hasBezierNodes())
        m_maskEdit.paintBezierOverlay(painter, zoomFactor);
}

void PaintArea::paintBezierOverlay(QPainter &painter) {
    if (currentTool == ToolType::PenBezier && !stack.isEditingMask())
        bezierTool.paint(painter, zoomFactor);
}

void PaintArea::paintCanvasResizePreview(QPainter &painter) {
    if (!resizingCanvas) return;
    painter.setPen(QPen(darkModeActive ? Qt::white : Qt::black, 1.5 / zoomFactor, Qt::DashLine));
    painter.drawRect(0, 0, previewCanvasSize.x(), previewCanvasSize.y());
}

void PaintArea::paintShapePreview(QPainter &painter, int scaledWidth) {
    if (!drawing) return;
    if (!ToolManager::isShape(currentTool) && currentTool != ToolType::PixelStroke) return;
    QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton); colorDeUso.setAlpha(penOpacity);
    if ((currentTool == ToolType::PixelStroke || currentTool == ToolType::Line) && pixelOptions.getIsPixelArtMode()) {
        QImage preview = stack.currentImage().copy();
        PixelArt::drawShape(preview, startPoint, lastPoint, colorDeUso, currentTool);
        painter.drawImage(0, 0, preview);
    } else {
        painter.setPen(QPen(colorDeUso, scaledWidth, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        PaintEngine::drawGeometry(painter, startPoint, lastPoint, currentTool);
    }
}

void PaintArea::paintActiveSelection(QPainter &painter) {
    if (selMgr.isActive() && selMgr.hasBuffer())
        selMgr.drawSelectionOverlay(painter, zoomFactor);
}

void PaintArea::paintSelectFreeVectorOverlay(QPainter &painter) {
    if (currentTool != ToolType::SelectFree || selMgr.getFreeSubMode() != SelectionManager::FreeVector || !selMgr.isFreeVectorMode()) return;
    QPointF hoverF(hoverPos.x() / zoomFactor, hoverPos.y() / zoomFactor);
    selMgr.paintFreeVectorOverlay(painter, hoverF, zoomFactor, selBlue());
}

void PaintArea::paintSelectFreeElementOverlay(QPainter &painter) {
    if (currentTool != ToolType::SelectFree || selMgr.getFreeSubMode() != SelectionManager::FreeElement || !selMgr.isElementHoverActive()) return;
    selMgr.paintElementHoverOverlay(painter, zoomFactor, selBlue());
}

void PaintArea::paintSelectionPreview(QPainter &painter) {
    if (!drawing) return;
    if (currentTool != ToolType::Select && currentTool != ToolType::SelectFree) return;
    painter.setPen(QPen(selBlue(), 1.5 / zoomFactor, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    if (currentTool == ToolType::Select) painter.drawRect(selMgr.rect());
    else painter.drawPath(selMgr.path());
}

void PaintArea::paintTextFrame(QPainter &painter) {
    if (!textEdit.active) return;
    TextEngine::Style ts = currentTextStyle();
    ts.handleSize = textEdit.handleSize;
    textEdit.paint(painter, zoomFactor, ts);
}

void PaintArea::paintGradientPreview(QPainter &painter) {
    if (!drawingGradient || currentTool != ToolType::Gradient) return;
    auto grad = buildGradientForPreview();
    if (grad) paintGradientFill(painter, grad.get());
    paintGradientHandleOverlay(painter);
}

std::unique_ptr<QGradient> PaintArea::buildGradientForPreview() const {
    QColor c1 = gradientReverse ? penColor2 : penColor1;
    QColor c2;
    if (gradientUseSecondColor) c2 = gradientReverse ? penColor1 : penColor2;
    else { c2 = c1; c2.setAlpha(0); }
    c1.setAlpha(gradientOpacity);
    c2.setAlpha(gradientUseSecondColor ? gradientOpacity : 0);
    std::unique_ptr<QGradient> grad;
    switch (gradientType) {
    case GradientLinear: grad = std::make_unique<QLinearGradient>(gradientStart, gradientEnd); break;
    case GradientRadial: {
        const int radius = qMax(1, (int)sqrt(pow(gradientEnd.x() - gradientStart.x(), 2) + pow(gradientEnd.y() - gradientStart.y(), 2)));
        grad = std::make_unique<QRadialGradient>(gradientStart, radius); break;
    }
    case GradientConic: grad = std::make_unique<QConicalGradient>(gradientStart, gradientAngle); break;
    default: return nullptr;
    }
    grad->setColorAt(0.0, c1);
    grad->setColorAt(1.0, c2);
    return grad;
}

void PaintArea::paintGradientFill(QPainter &painter, QGradient *grad) {
    if (!grad) return;
    painter.setPen(Qt::NoPen); painter.setBrush(*grad);
    painter.setOpacity(0.85); painter.drawRect(stack.canvasRect()); painter.setOpacity(1.0);
}

void PaintArea::paintGradientHandleOverlay(QPainter &painter) {
    painter.setPen(QPen(QColor(255, 80, 80), 2.0 / zoomFactor, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(gradientStart, gradientEnd);
    painter.setBrush(QColor(255, 80, 80, 220));
    painter.drawEllipse(QPointF(gradientStart), 6.0 / zoomFactor, 6.0 / zoomFactor);
    painter.drawEllipse(QPointF(gradientEnd), 6.0 / zoomFactor, 6.0 / zoomFactor);
    painter.setPen(Qt::white);
    QFont infoFont; infoFont.setPixelSize(qMax(10, (int)(12 / zoomFactor)));
    painter.setFont(infoFont);
    const QString info = QString("%1 | %2° | %3% | %4")
        .arg(gradientType == GradientLinear ? "Lineal" : gradientType == GradientRadial ? "Radial" : "Cónico")
        .arg(gradientAngle).arg(qRound(gradientOpacity / 255.0 * 100))
        .arg(gradientUseSecondColor ? "2col" : "→ Transp");
    painter.drawText(gradientStart.x() + 10 / zoomFactor, gradientStart.y() - 10 / zoomFactor, info);
}

void PaintArea::paintCloneOverlay(QPainter &painter) {
    if (currentTool != ToolType::Clone || !cloneSourceSet) return;
    QPoint offset = cloneSource - cloneInitialDest;
    QPoint currentSource = currentMousePos + offset;
    int brushSize = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    painter.setPen(QPen(QColor(100, 200, 255), 2.0 / zoomFactor, Qt::SolidLine));
    painter.setBrush(QColor(100, 200, 255, 40));
    painter.drawEllipse(cloneSource, brushSize, brushSize);
    painter.setPen(QPen(QColor(100, 200, 255), 1.0 / zoomFactor, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(cloneSource, QPoint(currentMousePos.x(), currentMousePos.y()));
    if (cloneIsStamping) {
        painter.setPen(QPen(QColor(255, 200, 100), 1.5 / zoomFactor, Qt::DotLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(currentSource, brushSize, brushSize);
    }
}

void PaintArea::paintSelectionGizmos(QPainter &painter) {
    if (currentTool == ToolType::Move)
        selMgr.drawObjectGizmos(painter, zoomFactor, darkModeActive);
    if (selMgr.isActive() && selMgr.hasBuffer())
        selMgr.drawSelectionGizmo(painter, zoomFactor, darkModeActive, selBlue(), selBlueLight());
}

void PaintArea::paintCanvasHandles(QPainter &painter) {
    painter.setPen(QPen(darkModeActive ? Qt::white : QColor("#404040"), 1));
    painter.setBrush(Qt::white);
    painter.drawRect(getRightHandle());
    painter.drawRect(getBottomHandle());
    painter.drawRect(getBottomRightHandle());
}

void PaintArea::paintCursorSilhouette(QPainter &painter, int scaledWidth) {
    if (!rect().contains(hoverPos) || textEdit.active) return;
    if (stack.isEditingMask() && herramientaDePintura()) {
        paintMaskBrushSilhouette(painter);
        return;
    }
    switch (ToolManager::silhouetteKind(currentTool)) {
    case ToolManager::SilRetouch: paintRetouchSilhouette(painter, scaledWidth); break;
    case ToolManager::SilDeform:  paintDeformSilhouette(painter); break;
    case ToolManager::SilClone:   if (cloneSourceSet) paintCloneSilhouette(painter, scaledWidth); break;
    case ToolManager::SilBrush:   paintBrushStampSilhouette(painter); break;
    case ToolManager::SilNone:
    default: break;
    }
}

void PaintArea::paintMaskBrushSilhouette(QPainter &painter) {
    painter.setPen(QPen(darkModeActive ? QColor(255,255,255,140) : QColor(0,0,0,120), 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    BrushSettings cfg;
    if (usaStampDePincel()) cfg = activePreset();
    else {
        cfg.shape = ShapeType::Circle; cfg.dragMode = DragMode::Continuous;
        cfg.rotationMode = RotationMode::Fixed;
        cfg.size = qMax(5, (int)(penWidth * mouseSensitivity * 2));
    }
    const double sSize = qMax(4, cfg.size) * zoomFactor;
    painter.save(); painter.setRenderHint(QPainter::Antialiasing, true);
    PaintEngine::drawBrushSilhouette(painter, cfg, sSize, hoverPos);
    painter.restore();
}

void PaintArea::paintRetouchSilhouette(QPainter &painter, int scaledWidth) {
    painter.setPen(QPen(darkModeActive ? QColor(255,255,255,180) : QColor(0,0,0,150), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(hoverPos), (double)((scaledWidth*2+2)*zoomFactor), (double)((scaledWidth*2+2)*zoomFactor));
}

void PaintArea::paintDeformSilhouette(QPainter &painter) {
    m_deform.paintOverlay(painter, hoverPos, zoomFactor, darkModeActive);
}

void PaintArea::paintCloneSilhouette(QPainter &painter, int scaledWidth) {
    painter.setPen(QPen(darkModeActive ? QColor(255,255,255,200) : QColor(0,0,0,180), 1.5));
    painter.setBrush(Qt::NoBrush);
    const int brushSize = (scaledWidth * 2) * zoomFactor;
    painter.drawEllipse(QPointF(hoverPos), (double)brushSize, (double)brushSize);
    painter.drawLine(hoverPos.x()-4, hoverPos.y(), hoverPos.x()+4, hoverPos.y());
    painter.drawLine(hoverPos.x(), hoverPos.y()-4, hoverPos.x(), hoverPos.y()+4);
}

void PaintArea::paintBrushStampSilhouette(QPainter &painter) {
    painter.setPen(QPen(darkModeActive ? QColor(255,255,255,120) : QColor(0,0,0,100), 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    const double sSize = activePreset().size * zoomFactor;
    painter.save(); painter.setRenderHint(QPainter::Antialiasing, true);
    PaintEngine::drawBrushSilhouette(painter, activePreset(), sSize, hoverPos);
    painter.restore();
}

#include "moc_paintarea.cpp"