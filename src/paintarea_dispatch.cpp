#include "paintarea.h"

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