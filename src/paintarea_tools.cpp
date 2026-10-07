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
