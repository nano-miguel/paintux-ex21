#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include <QString>
#include <QColor>
#include <QList>
#include <Qt>
#include <QCoreApplication>

enum class ToolType {
    Pencil, Eraser, Bucket, Picker, Spray, Text,
    Select, SelectFree,
    Line, Rectangle, Ellipse, RoundRect, Triangle, RightTriangle,
    Diamond, Pentagon, Hexagon, ArrowRight, ArrowLeft, Star, Heart, Cube,
    Zoom, Brush, CustomBrush, Crayon, Marker,
    Watercolor, OilBrush, Calligraphy, Highlighter,
    MagicWand, LassoExtract, LassoDelete,
    PenBezier, NodeEdit,
    Blur, Heal, MirrorPen, PixelStroke, Lighten,
    Pencil3B, Pencil4B,
    Gradient, Clone, Move, ShadowBurn, Deform
};

namespace ToolManager {

using ToolId = quint32;
constexpr ToolId idOf(ToolType t) { return static_cast<ToolId>(t); }

enum Category : quint32 {
    None         = 0,
    Shape        = 1u << 0,
    Painting     = 1u << 1,
    Vectorizable = 1u << 2,
    BrushStamp   = 1u << 3,
    Artistic     = 1u << 4,
    PixelArt     = 1u << 5,
    Retouch      = 1u << 6,
    Selection    = 1u << 7,
    Transform    = 1u << 8,
};

enum SilhouetteKind : quint8 {
    SilNone    = 0,
    SilRetouch = 1,
    SilDeform  = 2,
    SilClone   = 3,
    SilBrush   = 4,
};

struct Info {
    ToolType        tool;
    const char     *icon;
    const char     *name;
    quint32         cats;
    Qt::CursorShape cursor;
    int             marginMultiplier;
    int             marginOffset;
    bool            needsEditableLayer;
    quint8          silhouette;
};

namespace detail {

inline const Info* tools() {
    static const Info arr[] = {
        { ToolType::Pencil,        "lapiz.svg",                 "Lápiz",                Painting|PixelArt,             Qt::CrossCursor,        2,  6, true,  SilNone    },
        { ToolType::Eraser,        "goma.svg",                  "Goma",                 Painting|PixelArt,             Qt::CrossCursor,        1,  4, true,  SilNone    },
        { ToolType::Bucket,        "cubeta-pintura.svg",        "Cubeta",               None,                          Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Picker,        "gotero.svg",                "Gotero",               None,                          Qt::PointingHandCursor, 1,  6, false, SilNone    },
        { ToolType::Spray,         "spray.svg",                 "Spray",                Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::Text,          nullptr,                     "Texto",                None,                          Qt::IBeamCursor,        1,  6, true,  SilNone    },
        { ToolType::Select,        nullptr,                     "Selección",            Selection,                     Qt::CrossCursor,        1,  6, false, SilNone    },
        { ToolType::SelectFree,    nullptr,                     "Selección libre",      Selection|Vectorizable,        Qt::CrossCursor,        1,  6, false, SilNone    },
        { ToolType::Line,          "line.svg",                  "Línea",                Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Rectangle,     "cuadrado.svg",              "Rectángulo",           Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Ellipse,       "circle.svg",                "Elipse",               Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::RoundRect,     "rectangulo_redondeado.svg", "Rect. redondeado",     Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Triangle,      "triangulo.svg",             "Triángulo",            Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::RightTriangle, "triangulo_rectangulo.svg",  "Triángulo rect.",      Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Diamond,       "rombo.svg",                 "Rombo",                Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Pentagon,      "pentagono.svg",             "Pentágono",            Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Hexagon,       "hexagono.svg",              "Hexágono",             Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::ArrowRight,    "flecha_derecha.svg",        "Flecha derecha",       Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::ArrowLeft,     "flecha_izquierda.svg",      "Flecha izquierda",     Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Star,          "estrella.svg",              "Estrella",             Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Heart,         "corazon.svg",               "Corazón",              Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Cube,          "cubo2.svg",                 "Cubo",                 Shape,                         Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Zoom,          "lupa.svg",                  "Zoom",                 None,                          Qt::PointingHandCursor, 1,  6, false, SilNone    },
        { ToolType::Brush,         "pincel.svg",                "Pincel",               Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::CustomBrush,   "custom_brush.svg",          "Pincel personalizado", Painting|BrushStamp,           Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::Crayon,        "crayon.svg",                "Crayón",               Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::Marker,        "marker.svg",                "Marcador",             Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::Watercolor,    "acuarela.svg",              "Acuarela",             Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::OilBrush,      "oleo.svg",                  "Óleo",                 Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::Calligraphy,   "caligrafia.svg",            "Caligrafía",           Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::Highlighter,   "resaltador.svg",            "Resaltador",           Painting|BrushStamp|Artistic,  Qt::BlankCursor,        1,  6, true,  SilBrush   },
        { ToolType::MirrorPen,     "mirror.svg",                "Espejo",               Painting|PixelArt,             Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::PixelStroke,   "pixel_stroke.svg",          "Trazo Pixel",          Shape|PixelArt,                Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Lighten,       "lighten.svg",               "Aclarar",              Painting|PixelArt,             Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::MagicWand,     "magic_wand.svg",            "Varita mágica",        Selection,                     Qt::CrossCursor,        1,  6, false, SilNone    },
        { ToolType::LassoExtract,  "lasso_extract.svg",         "Recortar dentro",      Selection|Vectorizable,        Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::LassoDelete,   "lasso_delete.svg",          "Recortar fuera",       Selection|Vectorizable,        Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::PenBezier,     "pen_bezier.svg",            "Pluma Bezier",         Vectorizable,                  Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Blur,          "blur.svg",                  "Desenfoque",           Retouch,                       Qt::BlankCursor,        2,  4, true,  SilRetouch },
        { ToolType::Heal,          "heal.svg",                  "Corrector",            Retouch,                       Qt::BlankCursor,        2,  4, true,  SilRetouch },
        { ToolType::ShadowBurn,    "burn.svg",                  "Subexponer",           Retouch,                       Qt::BlankCursor,        2,  4, true,  SilRetouch },
        { ToolType::Gradient,      "gradient.svg",              "Gradiente",            None,                          Qt::CrossCursor,        1,  6, true,  SilNone    },
        { ToolType::Clone,         "clone.svg",                 "Clonar",               None,                          Qt::CrossCursor,        2,  6, true,  SilClone   },
        { ToolType::Move,          "move.svg",                  "Mover",                Transform,                     Qt::ArrowCursor,        1,  6, false, SilNone    },
        { ToolType::Deform,        nullptr,                     "Deformar",             Retouch|Transform,             Qt::BlankCursor,       -1,  0, true,  SilDeform  },
    };
    return arr;
}

constexpr int kCount = 45;

} // namespace detail

inline const Info& info(ToolType t) {
    const Info *arr = detail::tools();
    for (int i = 0; i < detail::kCount; ++i)
        if (arr[i].tool == t) return arr[i];
    static const Info unknown{ ToolType::Pencil, nullptr, "?", None, Qt::ArrowCursor, 1, 6, true, SilNone };
    return unknown;
}

inline QList<ToolType> all() {
    QList<ToolType> out;
    const Info *arr = detail::tools();
    out.reserve(detail::kCount);
    for (int i = 0; i < detail::kCount; ++i) out.append(arr[i].tool);
    return out;
}

inline QList<ToolType> byCategory(Category cat) {
    QList<ToolType> out;
    const Info *arr = detail::tools();
    for (int i = 0; i < detail::kCount; ++i)
        if (arr[i].cats & cat) out.append(arr[i].tool);
    return out;
}

inline bool isShape(ToolType t)        { return info(t).cats & Shape; }
inline bool isPainting(ToolType t)     { return info(t).cats & Painting; }
inline bool isVectorizable(ToolType t) { return info(t).cats & Vectorizable; }
inline bool usesBrushStamp(ToolType t) { return info(t).cats & BrushStamp; }
inline bool isArtistic(ToolType t)     { return info(t).cats & Artistic; }
inline bool isPixelArt(ToolType t)     { return info(t).cats & PixelArt; }
inline bool isRetouch(ToolType t)      { return info(t).cats & Retouch; }
inline bool isSelection(ToolType t)    { return info(t).cats & Selection; }
inline bool isTransform(ToolType t)    { return info(t).cats & Transform; }

inline int  marginMultiplier(ToolType t)   { return info(t).marginMultiplier; }
inline int  marginOffset(ToolType t)       { return info(t).marginOffset; }
inline bool needsEditableLayer(ToolType t) { return info(t).needsEditableLayer; }
inline quint8 silhouetteKind(ToolType t)   { return info(t).silhouette; }

inline QColor selectionColorFor(ToolType t) {
    if (t == ToolType::LassoDelete)  return QColor(236, 72, 153);
    if (t == ToolType::LassoExtract) return QColor(59, 130, 246);
    return QColor(16, 185, 129);
}

inline QString displayName(ToolType t) {
    return QCoreApplication::translate("ToolManager", info(t).name);
}

inline int retouchCode(ToolType t) {
    switch (t) {
        case ToolType::Blur:       return 201;
        case ToolType::Heal:       return 202;
        case ToolType::ShadowBurn: return 101;
        default:                   return 0;
    }
}

} // namespace ToolManager

#endif // TOOLMANAGER_H