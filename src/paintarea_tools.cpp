#include "paintarea.h"

bool PaintArea::toolPencil(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        beginStroke(ctx);
        QColor c = obtenerColorDeTrabajo(activeMouseButton); c.setAlpha(penOpacity);
        if (capaValida()) {
            PaintEngine::applyGraphitePencil(stack.currentImage(), ctx.pos, ctx.pos, c, ctx.scaledWidth, penOpacity);
            invalidarTrazo(ctx.pos, ctx.pos); emit layersChanged();
        }
        return true;
    }
    case ToolAction::Move: {
        if (!drawing) return false;
        QColor c = obtenerColorDeTrabajo(activeMouseButton); c.setAlpha(penOpacity);
        QPoint prev = lastPoint;
        if (capaValida()) PaintEngine::applyGraphitePencil(stack.currentImage(), lastPoint, ctx.pos, c, ctx.scaledWidth, penOpacity);
        lastPoint = ctx.pos; invalidarTrazo(prev, ctx.pos);
        return true;
    }
    case ToolAction::Release:
        return endStroke();
    default: return false;
    }
}

bool PaintArea::toolEraser(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        beginStroke(ctx);
        if (capaValida()) {
            PaintEngine::applyEraserLine(stack.currentImage(), ctx.pos, ctx.pos, ctx.scaledWidth, true);
            invalidarTrazo(ctx.pos, ctx.pos); emit layersChanged();
        }
        return true;
    }
    case ToolAction::Move: {
        if (!drawing) return false;
        QPoint prev = lastPoint;
        if (capaValida()) PaintEngine::applyEraserLine(stack.currentImage(), lastPoint, ctx.pos, ctx.scaledWidth, true);
        lastPoint = ctx.pos; invalidarTrazo(prev, ctx.pos);
        return true;
    }
    case ToolAction::Release:
        return endStroke();
    default: return false;
    }
}

bool PaintArea::toolPicker(const ToolCtx &ctx) {
    if (ctx.action != ToolAction::Press) return true;
    QColor picked = stack.compositedImage().pixelColor(ctx.pos);
    emit colorPicked((activeMouseButton == Qt::LeftButton) ? 1 : 2, picked);
    return true;
}

bool PaintArea::toolBucket(const ToolCtx &ctx) {
    if (ctx.action != ToolAction::Press) return true;
    QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton); colorDeUso.setAlpha(penOpacity);
    saveHistoryState();
    if (capaValida()) { PaintEngine::floodFill(stack.currentImage(), ctx.pos, colorDeUso); refreshAndNotify(); }
    return true;
}

bool PaintArea::toolZoom(const ToolCtx &ctx) {
    if (ctx.action != ToolAction::Press) return true;
    QPoint viewportPos = mapToParent(ctx.rawPos);
    if (ctx.button == Qt::LeftButton) emit zoomRequested(zoomFactor * 2.0, viewportPos);
    else emit zoomRequested(zoomFactor / 2.0, viewportPos);
    return true;
}

bool PaintArea::toolRetouch(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        beginStroke(ctx);
        if (capaValida()) {
            RetouchTools::applyRetouchAlongLine(stack.currentImage(), ctx.pos, ctx.pos,
                                                penWidth, mouseSensitivity, penOpacity, ToolManager::retouchCode(currentTool));
            invalidarTrazo(ctx.pos, ctx.pos); emit layersChanged();
        }
        return true;
    }
    case ToolAction::Move: {
        if (!drawing) return false;
        QPoint prev = lastPoint;
        if (capaValida()) {
            RetouchTools::applyRetouchAlongLine(stack.currentImage(), lastPoint, ctx.pos,
                                                penWidth, mouseSensitivity, penOpacity, ToolManager::retouchCode(currentTool));
        }
        lastPoint = ctx.pos; invalidarTrazo(prev, ctx.pos);
        return true;
    }
    case ToolAction::Release:
        return endStroke();
    default: return false;
    }
}

bool PaintArea::toolPixelArt(const ToolCtx &ctx) {
    QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton); colorDeUso.setAlpha(penOpacity);
    switch (ctx.action) {
    case ToolAction::Press: {
        beginStroke(ctx);
        if (capaValida()) {
            PixelArt::drawPixel(stack.currentImage(), ctx.pos, colorDeUso, currentTool, true);
            invalidarTrazo(ctx.pos, ctx.pos); emit layersChanged();
        }
        return true;
    }
    case ToolAction::Move: {
        if (!drawing) return false;
        QPoint prev = lastPoint;
        if (capaValida()) PixelArt::drawLine(stack.currentImage(), lastPoint, ctx.pos, colorDeUso, currentTool, true);
        lastPoint = ctx.pos; invalidarTrazo(prev, ctx.pos);
        return true;
    }
    case ToolAction::Release:
        return endStroke();
    default: return false;
    }
}

bool PaintArea::toolBrushStamp(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        beginStroke(ctx);
        strokeTotalLength = 0.0; strokeAccumulatedLength = 0.0;
        QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton); colorDeUso.setAlpha(penOpacity);
        QColor colorOpuesto = obtenerColorDeTrabajo(activeMouseButton == Qt::LeftButton ? Qt::RightButton : Qt::LeftButton);
        strokeCanvasFallback = QColor();
        if (ToolManager::isArtistic(currentTool) && activePreset().wetMix)
            strokeCanvasFallback = PaintEngine::sampleCanvasColor(stack.compositedImage(), ctx.pos, qMax(3, ctx.scaledWidth));
        const BrushSettings &preset = activePreset();
        lastClassicPoint = ctx.pos;
        if (preset.isAirbrush || preset.dragMode == DragMode::Scattered) continuousDrawTimer->start(16);
        if (capaValida()) {
            PaintEngine::applyCustomBrushStroke(stack.currentImage(), ctx.pos, activeStamp(), preset, mouseSensitivity,
                                                0.0, 1.0, 1.0, colorDeUso, colorOpuesto, strokeCanvasFallback);
            invalidarTrazo(ctx.pos, ctx.pos); emit layersChanged();
        }
        return true;
    }
    case ToolAction::Move: {
        if (!drawing) return false;
        QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton); colorDeUso.setAlpha(penOpacity);
        QColor colorOpuesto = obtenerColorDeTrabajo(activeMouseButton == Qt::LeftButton ? Qt::RightButton : Qt::LeftButton);

        if (ctx.continuous) {
            if (capaValida()) {
                PaintEngine::applyCustomBrushStroke(stack.currentImage(), ctx.pos, activeStamp(),
                                                    activePreset(), mouseSensitivity,
                                                    0.0, 1.0, 1.0, colorDeUso, colorOpuesto, strokeCanvasFallback);
                invalidarTrazo(ctx.pos, ctx.pos);
            }
            return true;
        }

        QPoint prev = lastPoint;
        if (capaValida()) {
            const double segLen = QLineF(lastClassicPoint, QPointF(ctx.pos)).length();
            strokeAccumulatedLength += segLen;
            const double estimatedTotal = strokeAccumulatedLength + segLen * 10.0;
            if (estimatedTotal > strokeTotalLength) strokeTotalLength = estimatedTotal;
            PaintEngine::applyCustomBrushLine(stack.currentImage(), lastClassicPoint, ctx.pos, activeStamp(), activePreset(),
                                              mouseSensitivity, lastClassicPoint, colorDeUso, colorOpuesto, strokeCanvasFallback,
                                              strokeTotalLength, strokeAccumulatedLength - segLen);
        }
        lastClassicPoint = ctx.pos; lastPoint = ctx.pos;
        invalidarTrazo(prev, ctx.pos);
        return true;
    }
    case ToolAction::Release: {
        if (!drawing) return false;
        strokeTotalLength = strokeAccumulatedLength; strokeAccumulatedLength = 0.0;
        return endStroke();
    }
    default: return false;
    }
}

bool PaintArea::toolShape(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        beginStroke(ctx);
        return true;
    }
    case ToolAction::Move: {
        if (!drawing) return false;
        QPoint prev = lastPoint; lastPoint = ctx.pos;
        invalidarTrazo(prev, ctx.pos, QRect(startPoint, prev).normalized());
        return true;
    }
    case ToolAction::Release: {
        if (!drawing) return false;
        drawing = false;
        QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton); colorDeUso.setAlpha(penOpacity);
        if (pixelOptions.getIsPixelArtMode()) {
            if (!capaValida()) { activeMouseButton = Qt::NoButton; return true; }
            if (currentTool == ToolType::PixelStroke || currentTool == ToolType::Line)
                PixelArt::drawShape(stack.currentImage(), startPoint, ctx.pos, colorDeUso, currentTool);
            else {
                QPainter painter(&stack.currentImage());
                painter.setRenderHint(QPainter::Antialiasing, false);
                painter.setPen(QPen(colorDeUso, ctx.scaledWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                PaintEngine::drawGeometry(painter, startPoint, ctx.pos, currentTool);
                painter.end();
            }
            recomponerImagen();
        } else if (ToolManager::isShape(currentTool)) {
            selMgr.registerShapeObject(currentTool, startPoint, ctx.pos, colorDeUso, colorDeUso, ctx.scaledWidth, stack.currentIndex());
        } else {
            if (capaValida()) {
                QPainter painter(&stack.currentImage());
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(QPen(colorDeUso, ctx.scaledWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                PaintEngine::drawGeometry(painter, startPoint, ctx.pos, currentTool);
                painter.end();
                recomponerImagen();
            }
        }
        activeMouseButton = Qt::NoButton;
        emit layersChanged(); update();
        return true;
    }
    default: return false;
    }
}

bool PaintArea::toolPenBezier(const ToolCtx &ctx) {
    if (ctx.action != ToolAction::Press) return true;
    pressPenBezier(ctx.pos);
    return true;
}

bool PaintArea::toolLasso(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        saveHistoryState();
        startPoint = ctx.pos; lastPoint = ctx.pos; drawing = true;
        selMgr.startFree(ctx.pos);
        return true;
    }
    case ToolAction::Move: {
        if (!drawing) return false;
        selMgr.updateFree(ctx.pos, stack.canvasSize());
        lastPoint = ctx.pos;
        repintarZonaCanvas(selMgr.rect().adjusted(-6, -6, 6, 6));
        return true;
    }
    case ToolAction::Release: {
        if (!drawing) return false;
        drawing = false;
        selMgr.closeFreePath();
        QPainterPath lassoPath = selMgr.path();
        if (puedeEditarCapaActual()) {
            if (currentTool == ToolType::LassoExtract) {
                stack.currentImage() = LassoProcessor::applyLassoExtract(stack.currentImage(), lassoPath, pixelOptions.getIsPixelArtMode());
                emit statusBarMessage(tr("Fondo recortado"));
            } else {
                LassoProcessor::applyLassoDelete(stack.currentImage(), lassoPath, pixelOptions.getIsPixelArtMode());
                emit statusBarMessage(tr("Objeto borrado"));
            }
            refreshAndNotify();
        }
        selMgr.discard(); activeMouseButton = Qt::NoButton; update();
        return true;
    }
    default: return false;
    }
}

bool PaintArea::toolGradient(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        if (ctx.alt) {
            gradientType = (GradientType)(((int)gradientType + 1) % 3);
            emit statusBarMessage(tr("Gradiente: %1").arg(
                gradientType == GradientLinear ? tr("Lineal") : gradientType == GradientRadial ? tr("Radial") : tr("Cónico")));
            return true;
        }
        if (ctx.ctrl) { openGradientSettings(); return true; }
        gradientStart = ctx.pos; gradientEnd = ctx.pos; drawingGradient = true;
        return true;
    }
    case ToolAction::Move: {
        if (!drawingGradient) return false;
        gradientEnd = ctx.pos; update();
        return true;
    }
    case ToolAction::Release: {
        if (!drawingGradient) return false;
        drawingGradient = false;
        if (gradientStart != gradientEnd) {
            saveHistoryState();
            aplicarGradienteConfigurado(gradientStart, gradientEnd);
            emit statusBarMessage(tr("Gradiente aplicado")); refreshAndNotify();
        } else update();
        activeMouseButton = Qt::NoButton;
        return true;
    }
    default: return false;
    }
}

bool PaintArea::toolMagicWand(const ToolCtx &ctx) {
    if (ctx.action != ToolAction::Press) return true;
    magicWandSelect(ctx.pos, static_cast<int>(32 * mouseSensitivity));
    return true;
}

bool PaintArea::toolClone(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        if (ctx.alt || !cloneSourceSet) {
            cloneSource = ctx.pos; cloneSourceSet = true;
            if (capaValida()) cloneBuffer = stack.currentImage().copy();
            cloneInitialDest = ctx.pos;
            emit statusBarMessage(tr("Fuente fijada")); update();
            return true;
        }
        saveHistoryState();
        cloneInitialDest = ctx.pos; cloneIsStamping = true;
        if (capaValida()) { applyClonStamp(stack.currentImage(), ctx.pos); invalidarTrazo(ctx.pos, ctx.pos); emit layersChanged(); }
        return true;
    }
    case ToolAction::Move: {
        if (!cloneIsStamping || !cloneSourceSet) { invalidarPreviewClone(ctx.pos); return false; }
        if (!ctx.mouse || !(ctx.mouse->buttons() & Qt::LeftButton)) return true;
        if (capaValida()) { applyClonStamp(stack.currentImage(), ctx.pos); invalidarTrazo(ctx.pos, ctx.pos); emit layersChanged(); }
        invalidarPreviewClone(ctx.pos);
        return true;
    }
    case ToolAction::Release: {
        if (!cloneIsStamping) return false;
        cloneIsStamping = false; update();
        return true;
    }
    default: return false;
    }
}

bool PaintArea::toolDeform(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        saveHistoryState();
        m_deform.begin(ctx.pos, &stack.currentImage(), activeMouseButton == Qt::RightButton);
        return true;
    }
    case ToolAction::Move: {
        if (!m_deform.isActive()) return false;
        const QPoint prev = previousMousePos;
        m_deform.continueStroke(ctx.pos);
        invalidarTrazo(prev, ctx.pos);
        return true;
    }
    case ToolAction::Release: {
        if (!m_deform.isActive()) return false;
        m_deform.end(); activeMouseButton = Qt::NoButton;
        emit layersChanged(); update();
        return true;
    }
    default: return false;
    }
}

bool PaintArea::toolSelect(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press:
        pressSelectRect(ctx.pos);
        return true;
    case ToolAction::Move:
        if (drawing) {
            selMgr.updateRect(startPoint, ctx.pos, stack.canvasSize());
            repintarZonaCanvas(selMgr.rect().adjusted(-6, -6, 6, 6));
            return true;
        }
        return false;
    case ToolAction::Release:
        if (drawing) {
            releaseFinishSelection(ctx.pos);
            emit layersChanged();
            drawing = false;
            activeMouseButton = Qt::NoButton;
            update();
            return true;
        }
        return false;
    default: return false;
    }
}

bool PaintArea::toolSelectFree(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press:
        pressSelectFree(ctx.pos);
        return true;
    case ToolAction::Move:
        if (selMgr.getFreeSubMode() == SelectionManager::FreeLasso && drawing) {
            selMgr.updateFree(ctx.pos, stack.canvasSize());
            repintarZonaCanvas(selMgr.rect().adjusted(-6, -6, 6, 6));
            return true;
        }
        return false;
    case ToolAction::Release:
        if (selMgr.getFreeSubMode() == SelectionManager::FreeLasso && drawing) {
            releaseFinishSelection(ctx.pos);
            emit layersChanged();
            drawing = false;
            activeMouseButton = Qt::NoButton;
            update();
            return true;
        }
        return false;
    default: return false;
    }
}

bool PaintArea::toolText(const ToolCtx &ctx) {
    if (ctx.action != ToolAction::Press) return true;
    QColor c = obtenerColorDeTrabajo(activeMouseButton);
    c.setAlpha(penOpacity);
    pressTextNew(ctx.pos, c);
    return true;
}

bool PaintArea::toolMove(const ToolCtx &ctx) {
    switch (ctx.action) {
    case ToolAction::Press: {
        pressMove(ctx.mouse, ctx.pos);
        return true;
    }
    case ToolAction::Move: {
        if (ctx.mouse && (ctx.mouse->buttons() & Qt::LeftButton)) {
            if (moveObjectManipulation(ctx)) return true;
            if (moveMovingLayer(ctx)) return true;
            return true;
        }
        return moveToolHover(ctx);
    }
    case ToolAction::Release:
        return onReleaseMove(ctx);
    default:
        return false;
    }
}

void PaintArea::pressPenBezier(const QPoint &pos) {
    bool completed = bezierTool.click(pos, zoomFactor, activeMouseButton == Qt::RightButton);
    if (completed) bakeActivePath(); else update();
}

void PaintArea::pressSelectRect(const QPoint &pos) {
    startPoint = pos; selMgr.startRect(pos); drawing = true;
}

void PaintArea::pressSelectFree(const QPoint &pos) {
    auto mode = selMgr.getFreeSubMode();
    if (mode == SelectionManager::FreeVector)  { pressSelectFreeVector(pos);  return; }
    if (mode == SelectionManager::FreeElement) { pressSelectFreeElement(pos); return; }
    startPoint = pos; selMgr.startFree(pos); drawing = true;
}

void PaintArea::pressSelectFreeVector(const QPoint &pos) {
    if (activeMouseButton != Qt::LeftButton) return;
    QPointF cp(pos.x(), pos.y());
    if (selMgr.isNearFirstFreeVectorPoint(cp, zoomFactor) && selMgr.freeVectorNodes().size() >= 3) {
        if (selMgr.closeFreeVectorPath(stack.canvasSize())) {
            saveHistoryState(); selMgr.finalizeFree(stack.currentImage());
            emit statusBarMessage(tr("Selección vectorial cerrada"));
            preserveSelectionOnToolSwitch = true; emit selectionFinalizedByElement();
            refreshAndNotify(); setTool(ToolType::Move);
        }
        return;
    }
    int hit = selMgr.findFreeVectorPointAt(cp, zoomFactor);
    if (hit >= 0) { selMgr.setFreeVectorDragIndex(hit); setCursor(Qt::ClosedHandCursor); update(); return; }
    if (!selMgr.isFreeVectorMode()) selMgr.startFreeVectorMode();
    selMgr.addFreeVectorPoint(cp); update();
}

void PaintArea::pressSelectFreeElement(const QPoint &pos) {
    if (activeMouseButton != Qt::LeftButton) return;
    QImage &layerImg = stack.currentImage();
    ElementSelectOptions opts; opts.hoverDownsample = 1;
    ElementSelectResult res = SelectionManager::selectElement(layerImg, pos, opts);
    if (!res.isValid()) { emit statusBarMessage(tr("No se detectó ningún elemento en ese punto")); return; }
    bakeSelection(); saveHistoryState();
    if (selMgr.finalizeElement(res, stack.currentImage())) {
        emit statusBarMessage(tr("Elemento seleccionado: %1 px").arg(res.pixelsSelected));
        refreshAndNotify(); preserveSelectionOnToolSwitch = true;
        emit selectionFinalizedByElement(); setTool(ToolType::Move);
    } else emit statusBarMessage(tr("No se pudo finalizar la selección"));
    update();
}

void PaintArea::pressTextNew(const QPoint &pos, const QColor &colorDeUso) {
    if (textEdit.active) return;
    const int defaultW = qMax(100, stack.width() / 4), defaultH = qMax(60, stack.height() / 8);
    QRect r(pos.x(), pos.y(), defaultW, defaultH);
    textEdit.beginNew(r, lastUsedTextFont, colorDeUso);
    emit textFrameClicked(pos); update();
}

void PaintArea::pressMove(QMouseEvent *event, const QPoint &pos) {
    const QPointF canvasPos(pos.x(), pos.y());
    const bool shiftHeld = event && (event->modifiers() & Qt::ShiftModifier);
    if (!shiftHeld && tryGrabActiveObjectGizmo(canvasPos)) return;
    int hitObj = selMgr.findObjectAt(canvasPos);
    if (hitObj >= 0) {
        if (shiftHeld) selMgr.selectObject(hitObj, true);
        else beginObjectSelectionDrag(hitObj, canvasPos);
        return;
    }
    if (!selMgr.selectedObjectIndices().isEmpty()) { selMgr.deselectAllObjects(); return; }
    beginLayerMove(pos);
}

bool PaintArea::tryGrabActiveObjectGizmo(const QPointF &canvasPos) {
    const int activeIdx = selMgr.activeObjectIndex();
    if (activeIdx < 0 || activeIdx >= selMgr.objectCount()) return false;
    if (!selMgr.objectAt(activeIdx).selected) return false;
    ObjectHandle h = selMgr.findObjectGizmoHandleAt(activeIdx, canvasPos, zoomFactor);
    if (h == ObjectHandle::IntegrateToCanvas) { integrateSelectedObjects(); return true; }
    if (h == ObjectHandle::EditText) { loadTextObjectForEditing(activeIdx); return true; }
    if (h == ObjectHandle::EditShape) { loadShapeObjectForEditing(activeIdx); return true; }
    if (h == ObjectHandle::None) return false;
    selMgr.setObjectActiveHandle(h); selMgr.setObjectDragStart(canvasPos);
    selMgr.setObjectRotationStart(selMgr.objectAt(activeIdx).rotation);
    selMgr.setObjectScaleStart(selMgr.objectAt(activeIdx).scaleX, selMgr.objectAt(activeIdx).scaleY);
    selMgr.setObjectBoundsStart(selMgr.objectAt(activeIdx).bounds);
    if (h == ObjectHandle::Move) selMgr.setObjectDragging(true);
    else if (h == ObjectHandle::Rotate) selMgr.setObjectRotating(true);
    else selMgr.setObjectScaling(true);
    setCursor(cursorForHandle(h));
    return true;
}

void PaintArea::beginObjectSelectionDrag(int idx, const QPointF &canvasPos) {
    if (!selMgr.objectAt(idx).selected) selMgr.selectObject(idx, false);
    selMgr.setObjectActiveHandle(ObjectHandle::Move); selMgr.setActiveObjectIndex(idx);
    selMgr.setObjectDragStart(canvasPos); selMgr.setObjectBoundsStart(selMgr.objectAt(idx).bounds);
    selMgr.setObjectDragging(true); setCursor(Qt::ClosedHandCursor);
}

void PaintArea::beginLayerMove(const QPoint &pos) {
    saveHistoryState(); movingLayer = true; moveStartPos = pos;
    moveLayerIdx = stack.currentIndex(); moveLayerBackup = stack.currentImage().copy();
}

bool PaintArea::moveObjectManipulation(const ToolCtx &ctx) {
    const QPointF canvasPos(ctx.pos.x(), ctx.pos.y());
    const int activeIdx = selMgr.activeObjectIndex();
    if (activeIdx < 0 || activeIdx >= selMgr.objectCount()) return false;
    if (selMgr.isObjectDragging() && selMgr.objectActiveHandle() == ObjectHandle::Move) {
        selMgr.moveActiveObject(canvasPos); setCursor(Qt::ClosedHandCursor); update(); return true;
    }
    if (selMgr.isObjectRotating() && selMgr.objectActiveHandle() == ObjectHandle::Rotate) {
        selMgr.rotateActiveObject(canvasPos, ctx.shift);
        setCursor(Qt::ClosedHandCursor); update(); return true;
    }
    if (selMgr.isObjectScaling()) {
        selMgr.scaleActiveObject(canvasPos, ctx.shift);
        setCursor(cursorForHandle(selMgr.objectActiveHandle())); update(); return true;
    }
    return false;
}

bool PaintArea::moveMovingLayer(const ToolCtx &ctx) {
    if (!movingLayer || moveLayerIdx < 0 || moveLayerIdx >= stack.count()) return false;
    const QPoint delta = ctx.pos - moveStartPos;
    stack.layerAt(moveLayerIdx).image.fill(Qt::transparent);
    QPainter p(&stack.layerAt(moveLayerIdx).image);
    p.drawImage(delta.x(), delta.y(), moveLayerBackup);
    p.end();
    recomponerImagen(); emit layersChanged(); update();
    return true;
}

bool PaintArea::moveToolHover(const ToolCtx &ctx) {
    const QPointF canvasPos(ctx.pos.x(), ctx.pos.y());
    const int activeIdx = selMgr.activeObjectIndex();
    if (activeIdx >= 0 && activeIdx < selMgr.objectCount() && selMgr.objectAt(activeIdx).selected) {
        ObjectHandle h = selMgr.findObjectGizmoHandleAt(activeIdx, canvasPos, zoomFactor);
        setCursor(cursorForHandle(h));
    } else {
        int hitObj = selMgr.findObjectAt(canvasPos);
        setCursor(hitObj >= 0 ? Qt::SizeAllCursor : Qt::ArrowCursor);
    }
    return true;
}

void PaintArea::moveCursorUpdate(const QPoint &pos) {
    Q_UNUSED(pos);
    applyToolCursor(); updateSilhouetteOnHover(); updateBezierPreviewOnHover(pos);
}

void PaintArea::applyToolCursor() { setCursor(ToolManager::info(currentTool).cursor); }

void PaintArea::updateSilhouetteOnHover() {
    const bool herramientaConSilueta =
        currentTool == ToolType::Blur || currentTool == ToolType::Heal ||
        currentTool == ToolType::ShadowBurn || currentTool == ToolType::Deform ||
        usaStampDePincel();
    if (!herramientaConSilueta) return;
    update(rectSiluetaWidget(hoverPos).adjusted(-2, -2, 2, 2));
}

void PaintArea::updateBezierPreviewOnHover(const QPoint &pos) {
    if (currentTool != ToolType::PenBezier) return;
    bezierTool.move(pos, zoomFactor); update();
}

void PaintArea::releaseFinishSelection(const QPoint &finalPoint) {
    drawing = false;
    if (currentTool == ToolType::SelectFree) {
        selMgr.closeFreePath();
        const QRect selBounds = selMgr.path().boundingRect().toRect().intersected(stack.canvasRect());
        if (selBounds.width() > 4 && selBounds.height() > 4) {
            saveHistoryState(); selMgr.finalizeFree(stack.currentImage());
            emit statusBarMessage(tr("Selección libre"));
            preserveSelectionOnToolSwitch = true; emit selectionFinalizedByElement();
            refreshAndNotify(); setTool(ToolType::Move);
        }
    } else {
        selMgr.updateRect(startPoint, finalPoint, stack.canvasSize());
        if (selMgr.rect().width() > 4 && selMgr.rect().height() > 4) {
            saveHistoryState(); selMgr.finalizeRect(stack.currentImage());
            emit statusBarMessage(tr("Selección"));
            preserveSelectionOnToolSwitch = true; emit selectionFinalizedByElement();
            refreshAndNotify(); setTool(ToolType::Move);
        }
    }
}

bool PaintArea::onPressMaskBezier(const ToolCtx &ctx) {
    if (!stack.isEditingMask() || currentTool != ToolType::PenBezier) return false;
    bool consumed = m_maskEdit.beginBezierClick(ctx.pos, zoomFactor, activeMouseButton == Qt::RightButton);
    if (consumed) update();
    return consumed;
}

bool PaintArea::onPressMaskPaint(const ToolCtx &ctx) {
    if (!stack.isEditingMask() || !herramientaDePintura()) return false;
    if (!m_maskEdit.beginStroke(ctx.pos)) return false;
    lastPoint = ctx.pos; drawing = true; emit layersChanged();
    return true;
}

bool PaintArea::onPressTextActive(const ToolCtx &ctx) {
    if (!textEdit.active || activeMouseButton != Qt::LeftButton) return false;
    TextEngine::Handle h = textEdit.hitHandleAt(ctx.pos);
    if (h != TextEngine::Handle::None && h != TextEngine::Handle::Body) { textEdit.startResize(h); return true; }
    if (h == TextEngine::Handle::Body) { textEdit.startDrag(ctx.pos); return true; }
    bakeTextFrame();
    return false;
}

bool PaintArea::onPressCanvasHandles(const ToolCtx &ctx) {
    if (activeMouseButton != Qt::LeftButton || selMgr.isActive() || textEdit.active) return false;
    auto enableResize = [&](int mode) { resizingCanvas = true; resizeMode = mode; previewCanvasSize = QPoint(stack.width(), stack.height()); };
    if (getBottomRightHandle().contains(ctx.rawPos)) { enableResize(3); return true; }
    if (getRightHandle().contains(ctx.rawPos))       { enableResize(1); return true; }
    if (getBottomHandle().contains(ctx.rawPos))      { enableResize(2); return true; }
    return false;
}

bool PaintArea::onPressSelectionGizmo(const ToolCtx &ctx) {
    if (!selMgr.isActive() || activeMouseButton != Qt::LeftButton || !selMgr.hasBuffer()) return false;
    const QPointF cp(ctx.pos.x(), ctx.pos.y());
    const double threshold = 12.0 / zoomFactor;
    const QPointF convPos = selMgr.convertHandlePos(zoomFactor);
    if (QLineF(cp, convPos).length() <= threshold) { convertSelectionToObject(); return true; }
    ObjectHandle hSel = selMgr.hitTestGizmoAt(cp, zoomFactor);
    if (hSel == ObjectHandle::Rotate) { selMgr.setRotating(true); setCursor(Qt::CrossCursor); return true; }
    if (isScaleHandle(hSel)) {
        selMgr.setResizing(true); selMgr.setResizeHandle(hSel);
        selMgr.setResizeStart(QRectF(selMgr.rect()), cp);
        setCursor(cursorForHandle(hSel)); return true;
    }
    if (hSel == ObjectHandle::Move) {
        selMgr.setDragging(true);
        selMgr.setDragOffset(ctx.pos - selMgr.rect().topLeft());
        setCursor(Qt::ClosedHandCursor); return true;
    }
    bakeSelection();
    return false;
}

bool PaintArea::onMoveMaskBezier(const ToolCtx &ctx) {
    if (!stack.isEditingMask() || currentTool != ToolType::PenBezier || !m_maskEdit.isDraggingNode()) return false;
    if (m_maskEdit.moveBezierNode(ctx.pos)) update();
    return true;
}

bool PaintArea::onMoveMaskPaint(const ToolCtx &ctx) {
    if (!drawing || !stack.isEditingMask() || !herramientaDePintura()) return false;
    m_maskEdit.continueStroke(lastPoint, ctx.pos); lastPoint = ctx.pos;
    return true;
}

bool PaintArea::onMoveSelectFreeVector(const ToolCtx &ctx) {
    if (currentTool != ToolType::SelectFree || selMgr.getFreeSubMode() != SelectionManager::FreeVector || !selMgr.isFreeVectorMode()) return false;
    QPointF cp(ctx.pos.x(), ctx.pos.y());
    int dragIdx = selMgr.freeVectorDragIndexValue();
    if (dragIdx >= 0) { selMgr.moveFreeVectorPoint(dragIdx, cp); update(); return true; }
    int hit = selMgr.findFreeVectorPointAt(cp, zoomFactor);
    selMgr.setFreeVectorHover(hit);
    if (selMgr.isNearFirstFreeVectorPoint(cp, zoomFactor) && selMgr.freeVectorNodes().size() >= 3) setCursor(Qt::PointingHandCursor);
    else if (hit >= 0) setCursor(Qt::OpenHandCursor);
    else setCursor(Qt::CrossCursor);
    update();
    return true;
}

bool PaintArea::onMoveSelectFreeElement(const ToolCtx &ctx) {
    if (currentTool != ToolType::SelectFree || selMgr.getFreeSubMode() != SelectionManager::FreeElement || !puedeEditarCapaActual()) return false;
    if ((ctx.pos - elementLastHover).manhattanLength() < 15) return true;
    elementLastHover = ctx.pos;
    QImage &layerImg = stack.currentImage();
    if (layerImg.isNull()) return true;
    ElementSelectOptions opts;
    opts.hoverDownsample = 6; opts.feather = 0; opts.closeRadius = 0; opts.fillHoles = false; opts.minArea = 10;
    selMgr.updateElementHover(layerImg, ctx.pos, opts);
    if (selMgr.isElementHoverActive()) setCursor(Qt::PointingHandCursor);
    else setCursor(Qt::CrossCursor);
    update();
    return true;
}

bool PaintArea::onMoveTextActive(const ToolCtx &ctx) {
    if (!textEdit.active) return false;
    if (textEdit.dragging) { textEdit.dragTo(ctx.pos); return true; }
    if (textEdit.resizing) { textEdit.resizeTo(ctx.pos, 20, 20); return true; }
    return false;
}

bool PaintArea::onMoveTextHover(const ToolCtx &ctx) {
    if (!textEdit.active) return false;
    TextEngine::Handle h = textEdit.hitHandleAt(ctx.pos);
    setCursor(TextEngine::cursorForHandle(h)); update();
    return true;
}

bool PaintArea::onMoveCanvasResize(const ToolCtx &ctx) {
    if (!resizingCanvas) return false;
    if (resizeMode == 1 || resizeMode == 3) previewCanvasSize.setX(qMax(50, int(ctx.rawPos.x() / zoomFactor)));
    if (resizeMode == 2 || resizeMode == 3) previewCanvasSize.setY(qMax(50, int(ctx.rawPos.y() / zoomFactor)));
    update();
    return true;
}

bool PaintArea::onMoveSelectionRotate(const ToolCtx &ctx) {
    if (!selMgr.isRotating()) return false;
    selMgr.updateRotation(ctx.pos);
    if (ctx.shift) selMgr.snapRotation();
    update();
    return true;
}

bool PaintArea::onMoveSelectionResize(const ToolCtx &ctx) {
    if (!selMgr.isResizing()) return false;
    selMgr.applyResize(selMgr.activeResizeHandle(), QPointF(ctx.pos.x(), ctx.pos.y()), ctx.shift);
    setCursor(cursorForHandle(selMgr.activeResizeHandle())); update();
    return true;
}

bool PaintArea::onMoveSelectionDrag(const ToolCtx &ctx) {
    if (!selMgr.isDragging()) return false;
    selMgr.moveTo(ctx.pos); update();
    return true;
}

bool PaintArea::onReleaseMaskPaint(const ToolCtx &ctx) {
    Q_UNUSED(ctx);
    if (!drawing || !stack.isEditingMask() || !herramientaDePintura()) return false;
    m_maskEdit.endStroke(); drawing = false; activeMouseButton = Qt::NoButton;
    emit layersChanged(); update();
    return true;
}

bool PaintArea::onReleaseSelectFreeVector(const ToolCtx &ctx) {
    Q_UNUSED(ctx);
    if (currentTool != ToolType::SelectFree || selMgr.getFreeSubMode() != SelectionManager::FreeVector) return false;
    if (selMgr.freeVectorDragIndexValue() >= 0) {
        selMgr.setFreeVectorDragIndex(-1); setCursor(Qt::CrossCursor); update(); return true;
    }
    return false;
}

bool PaintArea::onReleaseTextDragResize(const ToolCtx &ctx) {
    Q_UNUSED(ctx);
    if (textEdit.dragging) { textEdit.endDrag(); return true; }
    if (textEdit.resizing) { textEdit.endResize(); return true; }
    return false;
}

bool PaintArea::onReleaseCanvasResize(const ToolCtx &ctx) {
    Q_UNUSED(ctx);
    if (!resizingCanvas) return false;
    resizingCanvas = false;
    cambiarDimensionesLienzo(previewCanvasSize.x(), previewCanvasSize.y());
    resizeMode = 0;
    return true;
}

bool PaintArea::onReleaseSelectionGizmo(const ToolCtx &ctx) {
    Q_UNUSED(ctx);
    if (selMgr.isResizing()) { selMgr.setResizing(false); selMgr.setResizeHandle(ObjectHandle::None); return true; }
    if (selMgr.isDragging()) { selMgr.setDragging(false); return true; }
    if (selMgr.isRotating()) { selMgr.setRotating(false); return true; }
    return false;
}

bool PaintArea::onReleaseMove(const ToolCtx &ctx) {
    Q_UNUSED(ctx);
    if (selMgr.isObjectDragging() || selMgr.isObjectRotating() || selMgr.isObjectScaling()) {
        selMgr.stopObjectManipulation(); setCursor(Qt::ArrowCursor); update(); return true;
    }
    if (movingLayer) { movingLayer = false; moveLayerBackup = QImage(); moveLayerIdx = -1; update(); return true; }
    return false;
}

bool PaintArea::onKeySelectFree(const ToolCtx &ctx) {
    if (currentTool != ToolType::SelectFree || !ctx.key) return false;
    auto mode = selMgr.getFreeSubMode();
    if (mode == SelectionManager::FreeVector && selMgr.isFreeVectorMode()) {
        if (ctx.key->key() == Qt::Key_Return || ctx.key->key() == Qt::Key_Enter) {
            if (selMgr.closeFreeVectorPath(stack.canvasSize())) {
                saveHistoryState(); selMgr.finalizeFree(stack.currentImage());
                emit statusBarMessage(tr("Selección vectorial cerrada"));
                preserveSelectionOnToolSwitch = true; emit selectionFinalizedByElement();
                refreshAndNotify(); setTool(ToolType::Move);
            } else emit statusBarMessage(tr("Necesitas al menos 3 puntos"));
            return true;
        }
        if (ctx.key->key() == Qt::Key_Escape) { cancelSelectFreeVector(); emit statusBarMessage(tr("Selección vectorial cancelada")); return true; }
        if (ctx.key->key() == Qt::Key_Delete || ctx.key->key() == Qt::Key_Backspace) {
            int hover = selMgr.freeVectorHoverIndexValue();
            if (hover >= 0) { selMgr.removeFreeVectorPoint(hover); selMgr.setFreeVectorHover(-1); update(); return true; }
        }
    }
    if (mode == SelectionManager::FreeElement) {
        if (ctx.key->key() == Qt::Key_Escape) { cancelSelectFreeElement(); return true; }
    }
    return false;
}

bool PaintArea::onKeyMaskBezier(const ToolCtx &ctx) {
    if (!stack.isEditingMask() || currentTool != ToolType::PenBezier || !ctx.key) return false;
    if (!m_maskEdit.hasBezierNodes() || textEdit.active) return false;
    if (ctx.key->key() == Qt::Key_Return || ctx.key->key() == Qt::Key_Enter) { m_maskEdit.rasterizeBezier(zoomFactor); return true; }
    if (ctx.key->key() == Qt::Key_Escape) { m_maskEdit.cancelBezier(); update(); return true; }
    if ((ctx.key->key() == Qt::Key_Delete || ctx.key->key() == Qt::Key_Backspace) && m_maskEdit.selectedNode() >= 0) {
        m_maskEdit.removeSelectedNode(); update(); return true;
    }
    return false;
}

bool PaintArea::onKeyClipboard(const ToolCtx &ctx) {
    if (!ctx.ctrl || !ctx.key || textEdit.active) return false;
    if (ctx.key->key() == Qt::Key_C) { copiarSeleccion(); return true; }
    if (ctx.key->key() == Qt::Key_X) { cortarSeleccion(); return true; }
    if (ctx.key->key() == Qt::Key_V) { pegarClipboard(); return true; }
    return false;
}

bool PaintArea::onKeyDelete(const ToolCtx &ctx) {
    if (textEdit.active || !ctx.key) return false;
    if (ctx.key->key() != Qt::Key_Delete && ctx.key->key() != Qt::Key_Backspace) return false;
    borrarSeleccion(); return true;
}

bool PaintArea::onKeyObjectShortcuts(const ToolCtx &ctx) {
    if (textEdit.active || !ctx.key) return false;
    if (ctx.key->key() == Qt::Key_O) { convertSelectionToObject(); return true; }
    if (ctx.key->key() == Qt::Key_I) { integrateSelectedObjects(); return true; }
    return false;
}

bool PaintArea::onKeyTextEditing(const ToolCtx &ctx) {
    if (!textEdit.active || !ctx.key) return false;
    switch (ctx.key->key()) {
    case Qt::Key_Escape: cancelTextFrame(); emit textFrameCancelled(); return true;
    case Qt::Key_Return: case Qt::Key_Enter: textEdit.insertNewline(); return true;
    case Qt::Key_Backspace: textEdit.backspace(); return true;
    case Qt::Key_Delete: textEdit.deleteChar(); return true;
    case Qt::Key_Left: textEdit.moveLeft(); return true;
    case Qt::Key_Right: textEdit.moveRight(); return true;
    case Qt::Key_Home: textEdit.moveHome(); return true;
    case Qt::Key_End: textEdit.moveEnd(); return true;
    default: break;
    }
    const QString txt = ctx.key->text();
    if (!txt.isEmpty() && txt.at(0).isPrint()) { textEdit.insert(txt); return true; }
    return false;
}

bool PaintArea::onKeyToolSpecific(const ToolCtx &ctx) {
    if (!ctx.key) return false;
    if (currentTool == ToolType::Move && ctx.key->key() == Qt::Key_Escape) {
        selMgr.deselectAllObjects(); movingLayer = false; update(); return true;
    }
    if (currentTool == ToolType::Gradient && ctx.key->key() == Qt::Key_G && ctx.ctrl) {
        openGradientSettings(); return true;
    }
    return false;
}

bool PaintArea::onKeyBezierPen(const ToolCtx &ctx) {
    if (currentTool != ToolType::PenBezier || bezierTool.isEmpty() || !ctx.key) return false;
    if (ctx.key->key() == Qt::Key_Return || ctx.key->key() == Qt::Key_Enter) { if (bezierTool.finalize()) bakeActivePath(); return true; }
    if (ctx.key->key() == Qt::Key_Escape) { bezierTool.cancel(); update(); return true; }
    return false;
}
