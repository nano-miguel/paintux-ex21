#ifndef TEXT_ENGINE_H
#define TEXT_ENGINE_H

#include <QtGlobal>
#include <QRect>
#include <QRectF>
#include <QPoint>
#include <QPointF>
#include <QFont>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QColor>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QCursor>
#include <QTimer>
#include <QObject>
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QColorDialog>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QToolTip>
#include <QWindow>
#include <algorithm>
#include <cmath>

namespace TextEngine {

enum class Handle {
    None = 0,
    TopLeft, TopRight, BottomLeft, BottomRight,
    Top, Bottom, Left, Right,
    Body
};

struct Style {
    QColor accent;
    QColor accentLight;
    QColor fill;
    QColor textColor;
    QColor cursorColor;
    QColor handleBorder;
    QColor handleFill;
    int handleSize = 8;
    bool darkMode = false;

    static Style forDarkMode() {
        Style s;
        s.darkMode = true;
        s.accent = QColor("#60a5fa");
        s.accentLight = QColor("#93c5fd");
        s.fill = QColor(255, 255, 255, 18);
        s.textColor = QColor("#e5e5e5");
        s.cursorColor = QColor("#e5e5e5");
        s.handleBorder = QColor("#60a5fa");
        s.handleFill = QColor("#93c5fd");
        return s;
    }

    static Style forLightMode() {
        Style s;
        s.darkMode = false;
        s.accent = QColor("#2563eb");
        s.accentLight = QColor("#3b82f6");
        s.fill = QColor(0, 0, 0, 12);
        s.textColor = QColor("#111827");
        s.cursorColor = QColor("#111827");
        s.handleBorder = QColor("#2563eb");
        s.handleFill = QColor("#3b82f6");
        return s;
    }
};

inline QRect handleRect(const QRect &r, Handle h, int size)
{
    if (size <= 0) return QRect();
    const int half = size / 2;
    switch (h) {
    case Handle::TopLeft:     return QRect(r.left() - half,  r.top() - half,    size, size);
    case Handle::TopRight:    return QRect(r.right() - half, r.top() - half,    size, size);
    case Handle::BottomLeft:  return QRect(r.left() - half,  r.bottom() - half, size, size);
    case Handle::BottomRight: return QRect(r.right() - half, r.bottom() - half, size, size);
    case Handle::Top:         return QRect(r.left() + r.width()/2 - half, r.top() - half,    size, size);
    case Handle::Bottom:      return QRect(r.left() + r.width()/2 - half, r.bottom() - half, size, size);
    case Handle::Left:        return QRect(r.left() - half, r.top() + r.height()/2 - half, size, size);
    case Handle::Right:       return QRect(r.right() - half, r.top() + r.height()/2 - half, size, size);
    default: return QRect();
    }
}

inline Handle hitHandle(const QRect &r, const QPoint &pos, int size)
{
    static const Handle handles[] = {
        Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight,
        Handle::Top, Handle::Bottom, Handle::Left, Handle::Right
    };
    for (Handle h : handles) {
        if (handleRect(r, h, size).contains(pos)) return h;
    }
    if (r.contains(pos)) return Handle::Body;
    return Handle::None;
}

inline Qt::CursorShape cursorForHandle(Handle h)
{
    switch (h) {
    case Handle::TopLeft:
    case Handle::BottomRight: return Qt::SizeFDiagCursor;
    case Handle::TopRight:
    case Handle::BottomLeft:  return Qt::SizeBDiagCursor;
    case Handle::Top:
    case Handle::Bottom:      return Qt::SizeVerCursor;
    case Handle::Left:
    case Handle::Right:       return Qt::SizeHorCursor;
    case Handle::Body:        return Qt::IBeamCursor;
    default:                  return Qt::CrossCursor;
    }
}

struct TextData {
    QRect bounds;
    QString text;
    QFont font;
    QColor color;
    double lineSpacing = 1.0;
    double letterSpacing = 0.0;
};

struct VisualLine {
    QString text;
    int startPos = 0;
    int length = 0;
};

inline QFont fontWithLetterSpacing(const QFont &base, double letterSpacing)
{
    QFont f = base;
    if (qAbs(letterSpacing) > 0.001) {
        f.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
    }
    return f;
}

inline int measureLineWidth(const QString &text, const QFont &font, double letterSpacing)
{
    QFontMetrics fm(fontWithLetterSpacing(font, letterSpacing));
    return fm.horizontalAdvance(text);
}

inline QVector<VisualLine> wrapTextToVisualLines(const QString &text,
                                                 const QFont &font,
                                                 int maxWidth,
                                                 double letterSpacing)
{
    QVector<VisualLine> out;
    QFont measureFont = fontWithLetterSpacing(font, letterSpacing);
    QFontMetrics fm(measureFont);
    if (maxWidth < 4) maxWidth = 4;

    int absPos = 0;
    const QStringList paragraphs = text.split(QLatin1Char('\n'));
    for (int pi = 0; pi < paragraphs.size(); ++pi) {
        const QString &para = paragraphs[pi];
        if (para.isEmpty()) {
            VisualLine vl;
            vl.text = QString();
            vl.startPos = absPos;
            vl.length = 0;
            out.append(vl);
            absPos += 1;
            continue;
        }
        const int len = para.length();
        int start = 0;
        while (start < len) {
            int lo = start + 1;
            int hi = len;
            int best = start + 1;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                int w = fm.horizontalAdvance(para.mid(start, mid - start));
                if (w <= maxWidth) {
                    best = mid;
                    lo = mid + 1;
                } else {
                    hi = mid - 1;
                }
            }
            VisualLine vl;
            vl.text = para.mid(start, best - start);
            vl.startPos = absPos + start;
            vl.length = vl.text.length();
            out.append(vl);
            start = best;
        }
        absPos += para.length() + 1;
    }
    return out;
}

inline qreal computeTextBlockHeight(const QString &text, const QFont &font,
                                    int maxWidth, double lineSpacing,
                                    double letterSpacing)
{
    QFontMetrics fm(font);
    const qreal lineHeight = fm.height();
    const qreal spacing = qMax<qreal>(0.1, lineSpacing);
    QVector<VisualLine> lines = wrapTextToVisualLines(text, font, maxWidth, letterSpacing);
    int n = lines.size();
    if (n <= 0) n = 1;
    qreal total = 2.0;
    for (int i = 0; i < n; ++i) {
        total += lineHeight;
        if (i < n - 1) total += lineHeight * (spacing - 1.0);
    }
    total += 2.0;
    return total;
}

class EditSession : public QObject
{
    Q_OBJECT

public:
    explicit EditSession(QObject *parent = nullptr) : QObject(parent) {
        cursorTimer = new QTimer(this);
        cursorTimer->setInterval(500);
        connect(cursorTimer, &QTimer::timeout, this, [this]() {
            if (active) { cursorVisible = !cursorVisible; emit needsRepaint(); }
        });
    }

    bool active = false;
    QRect rect;
    QString text;
    int cursor = 0;
    QFont font;
    QColor color;
    double lineSpacing = 1.0;
    double letterSpacing = 0.0;
    bool cursorVisible = true;
    int handleSize = 8;

    bool dragging = false;
    bool resizing = false;
    Handle resizeHandle = Handle::None;
    QPoint dragOffset;

    void beginNew(const QRect &r, const QFont &f, const QColor &c) {
        active = true;
        rect = r;
        text.clear();
        cursor = 0;
        font = f;
        color = c;
        cursorVisible = true;
        dragging = false;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = QPoint(0, 0);
        autoResizeHeight();
        cursorTimer->start();
    }

    void loadExisting(const QRect &r, const QString &t, const QFont &f, const QColor &c) {
        active = true;
        rect = r;
        text = t;
        cursor = t.length();
        font = f;
        color = c;
        cursorVisible = true;
        dragging = false;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = QPoint(0, 0);
        autoResizeHeight();
        cursorTimer->start();
    }

    void end() {
        active = false;
        text.clear();
        cursor = 0;
        cursorVisible = true;
        dragging = false;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = QPoint(0, 0);
        cursorTimer->stop();
    }

    void cancel() { end(); }
    bool isEmpty() const { return text.isEmpty(); }

    TextData data() const {
        TextData d;
        d.bounds = rect;
        d.text = text;
        d.font = font;
        d.color = color;
        d.lineSpacing = lineSpacing;
        d.letterSpacing = letterSpacing;
        return d;
    }

    void applyFormat(const QFont &f, const QColor &c) {
        font = f;
        color = c;
        autoResizeHeight();
        emit needsRepaint();
    }

    void setLineSpacing(double s) {
        lineSpacing = qBound(0.1, s, 30.0);
        autoResizeHeight();
        emit needsRepaint();
    }

    void setLetterSpacing(double s) {
        letterSpacing = qBound(-20.0, s, 100.0);
        autoResizeHeight();
        emit needsRepaint();
    }

    void insert(const QString &s) {
        if (!active || s.isEmpty()) return;
        text.insert(cursor, s);
        cursor += s.length();
        cursorVisible = true;
        autoResizeHeight();
        emit needsRepaint();
    }
    void insertNewline() { insert(QStringLiteral("\n")); }
    void backspace() {
        if (!active || cursor <= 0) return;
        --cursor;
        text.remove(cursor, 1);
        cursorVisible = true;
        autoResizeHeight();
        emit needsRepaint();
    }
    void deleteChar() {
        if (!active || cursor >= text.length()) return;
        text.remove(cursor, 1);
        cursorVisible = true;
        autoResizeHeight();
        emit needsRepaint();
    }
    void moveLeft()    { if (active && cursor > 0) { --cursor; cursorVisible = true; emit needsRepaint(); } }
    void moveRight()   { if (active && cursor < text.length()) { ++cursor; cursorVisible = true; emit needsRepaint(); } }
    void moveHome()    { if (active) { cursor = 0; cursorVisible = true; emit needsRepaint(); } }
    void moveEnd()     { if (active) { cursor = text.length(); cursorVisible = true; emit needsRepaint(); } }
    void toggleCursor(){ cursorVisible = !cursorVisible; }

    Handle hitHandleAt(const QPoint &canvasPos) const {
        if (!active) return Handle::None;
        return hitHandle(rect, canvasPos, handleSize);
    }
    bool contains(const QPoint &canvasPos) const { return active && rect.contains(canvasPos); }

    void startDrag(const QPoint &canvasPos) {
        if (!active) return;
        dragging = true;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = canvasPos - rect.topLeft();
    }
    void dragTo(const QPoint &canvasPos) {
        if (!active || !dragging) return;
        rect.moveTo(canvasPos - dragOffset);
        emit needsRepaint();
    }
    void endDrag() { dragging = false; }

    void startResize(Handle h) {
        if (!active || h == Handle::None || h == Handle::Body) return;
        resizing = true;
        resizeHandle = h;
        dragging = false;
    }
    void resizeTo(const QPoint &canvasPos, int minW = 20, int minH = 20) {
        if (!active || !resizing) return;
        QRect r = rect;
        switch (resizeHandle) {
        case Handle::TopLeft:     r.setTopLeft(canvasPos); break;
        case Handle::TopRight:    r.setTopRight(canvasPos); break;
        case Handle::BottomLeft:  r.setBottomLeft(canvasPos); break;
        case Handle::BottomRight: r.setBottomRight(canvasPos); break;
        case Handle::Top:         r.setTop(canvasPos.y()); break;
        case Handle::Bottom:      r.setBottom(canvasPos.y()); break;
        case Handle::Left:        r.setLeft(canvasPos.x()); break;
        case Handle::Right:       r.setRight(canvasPos.x()); break;
        default: return;
        }
        if (r.width() >= minW && r.height() >= minH) {
            rect = r.normalized();
            autoResizeHeight();
            emit needsRepaint();
        }
    }
    void endResize() { resizing = false; resizeHandle = Handle::None; }

    void autoResizeHeight() {
        if (!active) return;
        const int maxW = qMax(10, rect.width() - 4);
        const qreal needed = computeTextBlockHeight(text, font, maxW, lineSpacing, letterSpacing);
        const int newH = qMax(24, (int)std::ceil(needed));
        if (newH > rect.height()) {
            rect.setHeight(newH);
        }
    }

    void paint(QPainter &painter, double zoomFactor, const Style &style) const {
        if (!active) return;

        const double z = qMax(0.0001, zoomFactor);
        painter.save();
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        painter.fillRect(rect, style.fill);

        painter.setPen(QPen(style.accent, 1.5 / z, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect);

        const QFont measureFont = fontWithLetterSpacing(font, letterSpacing);
        QFontMetrics fm(measureFont);
        const int maxW = qMax(10, rect.width() - 4);
        const QVector<VisualLine> lines = wrapTextToVisualLines(text, font, maxW, letterSpacing);
        const qreal lineHeight = fm.height();
        const qreal spacing = qMax<qreal>(0.1, lineSpacing);

        QVector<qreal> ys;
        qreal y = 2.0;
        for (int i = 0; i < lines.size(); ++i) {
            ys.append(y);
            if (i < lines.size() - 1) {
                y += lineHeight * spacing;
            } else {
                y += lineHeight;
            }
        }

        painter.setFont(measureFont);
        painter.setPen(color);
        for (int i = 0; i < lines.size(); ++i) {
            const qreal baseY = rect.top() + ys[i] + fm.ascent();
            painter.drawText(QPointF(rect.left() + 2.0, baseY), lines[i].text);
        }

        if (cursorVisible && !text.isEmpty()) {
            int visLineIdx = -1;
            for (int i = 0; i < lines.size(); ++i) {
                const int start = lines[i].startPos;
                const int end = start + lines[i].length;
                if (cursor >= start && cursor <= end) {
                    visLineIdx = i;
                    break;
                }
            }
            if (visLineIdx < 0 && !lines.isEmpty()) visLineIdx = lines.size() - 1;

            if (visLineIdx >= 0 && visLineIdx < lines.size()) {
                const int cursorInLine = qBound(0, cursor - lines[visLineIdx].startPos,
                                                lines[visLineIdx].length);
                const QString before = lines[visLineIdx].text.left(cursorInLine);
                const qreal xOff = fm.horizontalAdvance(before);
                const qreal cx = rect.left() + 2.0 + xOff;
                const qreal cyTop = rect.top() + ys[visLineIdx];
                painter.setPen(QPen(color, 1.5 / z));
                painter.drawLine(QPointF(cx, cyTop), QPointF(cx, cyTop + lineHeight));
            }
        } else if (cursorVisible && text.isEmpty()) {
            QPointF p(rect.left() + 2, rect.top() + 2);
            painter.setPen(QPen(color, 1.5 / z));
            painter.drawLine(p, QPointF(p.x(), p.y() + fm.height()));
        }

        painter.setPen(QPen(style.handleBorder, 1.0 / z));
        painter.setBrush(style.handleFill);
        static const Handle handles[] = {
            Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight,
            Handle::Top, Handle::Bottom, Handle::Left, Handle::Right
        };
        for (Handle h : handles)
            painter.drawRect(handleRect(rect, h, handleSize));

        painter.restore();
    }

signals:
    void needsRepaint();

private:
    QTimer *cursorTimer = nullptr;
};

// ═══════════════════════════════════════════════════════════════════════
//  FormatBar
//  • Barra flotante para editar formato de texto.
//  • Ahora se puede ARRASTRAR desde cualquier zona libre (incluyendo
//    labels) gracias a WA_TransparentForMouseEvents en los QLabel.
//  • Botón "+" al lado del combo de tipografías para CARGAR TTF/OTF.
//  • Las fuentes cargadas se guardan en QSettings y se re-cargan al
//    iniciar (persistencia entre sesiones).
// ═══════════════════════════════════════════════════════════════════════

class FormatBar : public QWidget
{
    Q_OBJECT

public:
    explicit FormatBar(QWidget *parent = nullptr)
        : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint
                          | Qt::WindowStaysOnTopHint)
    {
        setWindowTitle(QObject::tr("Formato de Texto"));
        setAttribute(Qt::WA_TranslucentBackground, true);
        setFixedSize(480, 130);
        currentColor = Qt::black;

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 10, 12, 10);
        mainLayout->setSpacing(6);

        // ─────────────────────────────────────────────
        //  Fila 1: Fuente + botón "+"
        // ─────────────────────────────────────────────
        QHBoxLayout *fontLayout = new QHBoxLayout();
        fontLayout->setSpacing(4);

        QLabel *lblFuente = new QLabel(QObject::tr("Fuente:"));
        lblFuente->setStyleSheet("color: #e5e5e5; background: transparent;");
        lblFuente->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        fontLayout->addWidget(lblFuente);

        fontCombo = new QComboBox();
        QStringList families = QFontDatabase::families();
        families.removeDuplicates();
        families.sort();

        m_fontModel = new QStandardItemModel(this);
        for (const QString &f : families) {
            QStandardItem *item = new QStandardItem(f);
            QFont itemFont(f);
            itemFont.setPointSize(12);
            item->setFont(itemFont);
            m_fontModel->appendRow(item);
        }
        fontCombo->setModel(m_fontModel);

        int defaultIndex = families.indexOf("Adwaita Sans");
        if (defaultIndex == -1) defaultIndex = families.indexOf("Arial");
        if (defaultIndex == -1) defaultIndex = families.indexOf("Sans Serif");
        if (defaultIndex == -1) defaultIndex = 0;
        fontCombo->setCurrentIndex(defaultIndex);

        fontCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        fontCombo->setMinimumHeight(26);
        fontCombo->view()->setMinimumWidth(280);
        fontCombo->setStyleSheet(
            "QComboBox { background-color: rgba(255,255,255,30); color: #e5e5e5;"
            " border: 1px solid rgba(255,255,255,50); border-radius: 6px;"
            " padding: 4px 8px; font-size: 12px; }"
            "QComboBox:hover { border-color: #3b82f6; }"
            "QComboBox::drop-down { border: none; width: 16px; }"
            "QComboBox QAbstractItemView { background-color: #1e1e1e; color: #e5e5e5;"
            " border: 1px solid rgba(255,255,255,50); selection-background-color: #3b82f6; }");
        fontLayout->addWidget(fontCombo);

        // ═══════════════════════════════════════════════════════
        //  Botón "+" para cargar tipografía personalizada
        // ═══════════════════════════════════════════════════════
        btnLoadFont = new QPushButton("+");
        btnLoadFont->setFixedSize(26, 26);
        btnLoadFont->setCursor(Qt::PointingHandCursor);
        btnLoadFont->setToolTip(QObject::tr("Cargar tipografía (.ttf / .otf)"));
        btnLoadFont->setStyleSheet(
            "QPushButton { background-color: rgba(59,130,246,80); color: white;"
            " border: 1px solid rgba(59,130,246,120); border-radius: 5px;"
            " font-size: 16px; font-weight: bold; padding: 0; }"
            "QPushButton:hover { background-color: #3b82f6;"
            " border: 1px solid #60a5fa; }"
            "QPushButton:pressed { background-color: #2563eb; }");
        connect(btnLoadFont, &QPushButton::clicked, this, &FormatBar::onLoadFontClicked);
        fontLayout->addWidget(btnLoadFont);

        mainLayout->addLayout(fontLayout);

        // ─────────────────────────────────────────────
        //  Fila 2: Tamaño + Interlineado + Letra + B/I
        // ─────────────────────────────────────────────
        QHBoxLayout *styleLayout = new QHBoxLayout();
        styleLayout->setSpacing(6);

        QLabel *lblTam = new QLabel(QObject::tr("Tam:"));
        lblTam->setStyleSheet("color: #e5e5e5; background: transparent;");
        lblTam->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        styleLayout->addWidget(lblTam);

        sizeSpin = new QSpinBox();
        sizeSpin->setRange(6, 400);
        sizeSpin->setValue(24);
        sizeSpin->setFixedWidth(58);
        sizeSpin->setStyleSheet(
            "QSpinBox { background-color: rgba(255,255,255,30); color: #e5e5e5;"
            " border: 1px solid rgba(255,255,255,50); border-radius: 6px;"
            " padding: 4px 6px; font-size: 12px; }"
            "QSpinBox:hover { border-color: #3b82f6; }");
        styleLayout->addWidget(sizeSpin);

        QLabel *lblInter = new QLabel(QObject::tr("Inter:"));
        lblInter->setStyleSheet("color: #e5e5e5; background: transparent;");
        lblInter->setToolTip(QObject::tr("Interlineado (0.1 apretado · 1.0 normal · 30.0 muy separado)"));
        lblInter->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        styleLayout->addWidget(lblInter);

        lineSpacingSpin = new QDoubleSpinBox();
        lineSpacingSpin->setRange(0.1, 30.0);
        lineSpacingSpin->setSingleStep(0.1);
        lineSpacingSpin->setDecimals(1);
        lineSpacingSpin->setValue(1.0);
        lineSpacingSpin->setFixedWidth(62);
        lineSpacingSpin->setToolTip(QObject::tr("Interlineado (0.1 apretado · 1.0 normal · 30.0 muy separado)"));
        lineSpacingSpin->setStyleSheet(
            "QDoubleSpinBox { background-color: rgba(255,255,255,30); color: #e5e5e5;"
            " border: 1px solid rgba(255,255,255,50); border-radius: 6px;"
            " padding: 4px 6px; font-size: 12px; }"
            "QDoubleSpinBox:hover { border-color: #3b82f6; }");
        styleLayout->addWidget(lineSpacingSpin);

        QLabel *lblLet = new QLabel(QObject::tr("Let:"));
        lblLet->setStyleSheet("color: #e5e5e5; background: transparent;");
        lblLet->setToolTip(QObject::tr("Espaciado entre letras en px (-20 apretado · 0 normal · 100 separado)"));
        lblLet->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        styleLayout->addWidget(lblLet);

        letterSpacingSpin = new QDoubleSpinBox();
        letterSpacingSpin->setRange(-20.0, 100.0);
        letterSpacingSpin->setSingleStep(0.5);
        letterSpacingSpin->setDecimals(1);
        letterSpacingSpin->setValue(0.0);
        letterSpacingSpin->setFixedWidth(62);
        letterSpacingSpin->setToolTip(QObject::tr("Espaciado entre letras en px (-20 apretado · 0 normal · 100 separado)"));
        letterSpacingSpin->setStyleSheet(
            "QDoubleSpinBox { background-color: rgba(255,255,255,30); color: #e5e5e5;"
            " border: 1px solid rgba(255,255,255,50); border-radius: 6px;"
            " padding: 4px 6px; font-size: 12px; }"
            "QDoubleSpinBox:hover { border-color: #3b82f6; }");
        styleLayout->addWidget(letterSpacingSpin);

        boldCheck = new QCheckBox(QObject::tr("B"));
        boldCheck->setFont(QFont(fontCombo->currentText(), 10, QFont::Bold));
        boldCheck->setFixedWidth(26);
        boldCheck->setStyleSheet(
            "QCheckBox { color: #e5e5e5; padding: 2px 4px; border-radius: 4px; }"
            "QCheckBox:hover { background-color: rgba(255,255,255,20); }"
            "QCheckBox:checked { background-color: #3b82f6; color: white; }"
            "QCheckBox::indicator { width: 0px; height: 0px; }");
        styleLayout->addWidget(boldCheck);

        italicCheck = new QCheckBox(QObject::tr("I"));
        QFont italicFont = italicCheck->font();
        italicFont.setItalic(true);
        italicCheck->setFont(italicFont);
        italicCheck->setFixedWidth(26);
        italicCheck->setStyleSheet(boldCheck->styleSheet());
        styleLayout->addWidget(italicCheck);
        styleLayout->addStretch();
        mainLayout->addLayout(styleLayout);

        // ─────────────────────────────────────────────
        //  Fila 3: Color + Aplicar/Cancelar
        // ─────────────────────────────────────────────
        QHBoxLayout *colorLayout = new QHBoxLayout();
        QLabel *lblColor = new QLabel(QObject::tr("Color:"));
        lblColor->setStyleSheet("color: #e5e5e5; background: transparent;");
        lblColor->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        colorLayout->addWidget(lblColor);

        btnColor = new QPushButton();
        btnColor->setFixedSize(60, 26);
        btnColor->setCursor(Qt::PointingHandCursor);
        btnColor->setStyleSheet(
            "background-color: black; border: 1px solid rgba(255,255,255,50); border-radius: 4px;");
        colorLayout->addWidget(btnColor);
        colorLayout->addStretch();

        btnApply = new QPushButton(QObject::tr("Aplicar"));
        btnApply->setCursor(Qt::PointingHandCursor);
        btnApply->setStyleSheet(
            "QPushButton { background-color: #2563eb; color: white; font-weight: bold;"
            " padding: 5px 12px; border-radius: 5px; border: none; font-size: 12px; }"
            "QPushButton:hover { background-color: #1d4ed8; }");
        colorLayout->addWidget(btnApply);

        btnCancel = new QPushButton(QObject::tr("Cancelar"));
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setStyleSheet(
            "QPushButton { background-color: transparent; color: #e5e5e5;"
            " padding: 5px 10px; border: 1px solid rgba(255,255,255,50);"
            " border-radius: 5px; font-size: 12px; }"
            "QPushButton:hover { border-color: #3b82f6; }");
        colorLayout->addWidget(btnCancel);
        mainLayout->addLayout(colorLayout);

        // ─────────────────────────────────────────────
        //  Conexiones
        // ─────────────────────────────────────────────
        connect(btnColor, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(currentColor, this, QObject::tr("Color del texto"));
            if (c.isValid()) {
                currentColor = c;
                btnColor->setStyleSheet(QString(
                    "background-color: %1; border: 1px solid rgba(255,255,255,50); border-radius: 4px;")
                    .arg(c.name()));
                emit formatChanged();
            }
        });
        connect(fontCombo, &QComboBox::currentTextChanged, this, [this]() {
            boldCheck->setFont(QFont(fontCombo->currentText(), 10, QFont::Bold));
            QFont ifont = italicCheck->font();
            ifont.setFamily(fontCombo->currentText());
            italicCheck->setFont(ifont);
            emit formatChanged();
        });
        connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { emit formatChanged(); });
        connect(lineSpacingSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() { emit formatChanged(); });
        connect(letterSpacingSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() { emit formatChanged(); });
        connect(boldCheck, &QCheckBox::toggled, this, [this]() { emit formatChanged(); });
        connect(italicCheck, &QCheckBox::toggled, this, [this]() { emit formatChanged(); });
        connect(btnApply, &QPushButton::clicked, this, [this]() { emit applyClicked(); });
        connect(btnCancel, &QPushButton::clicked, this, [this]() { emit cancelClicked(); });

        // ═══════════════════════════════════════════════════════
        //  Cargar fuentes personalizadas guardadas (persistencia)
        // ═══════════════════════════════════════════════════════
        QSettings settings("Paintux", "PaintuxStudio");
        const QStringList savedFonts = settings.value("customFonts").toStringList();
        for (const QString &path : savedFonts) {
            if (QFile::exists(path)) {
                loadFontFromPath(path, /*silent=*/true);
            }
        }
    }

    QColor getColor() const { return currentColor; }

    void setColor(const QColor &c) {
        currentColor = c;
        btnColor->setStyleSheet(QString(
            "background-color: %1; border: 1px solid rgba(255,255,255,50); border-radius: 4px;")
            .arg(c.name()));
    }

    void setFont(const QFont &f) {
        fontCombo->setCurrentText(f.family());
        sizeSpin->setValue(f.pointSize() > 0 ? f.pointSize() : 24);
        boldCheck->setChecked(f.bold());
        italicCheck->setChecked(f.italic());
    }

    QFont getCurrentFont() const {
        QFont f(fontCombo->currentText());
        f.setPointSize(sizeSpin->value());
        f.setBold(boldCheck->isChecked());
        f.setItalic(italicCheck->isChecked());
        return f;
    }

    double getLineSpacing() const { return lineSpacingSpin->value(); }
    void setLineSpacing(double s) { lineSpacingSpin->setValue(s); }

    double getLetterSpacing() const { return letterSpacingSpin->value(); }
    void setLetterSpacing(double s) { letterSpacingSpin->setValue(s); }

signals:
    void applyClicked();
    void cancelClicked();
    void formatChanged();

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QColor bg(26, 26, 30, 191);
        const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(QPen(QColor(255, 255, 255, 45), 1));
        p.setBrush(bg);
        p.drawRoundedRect(r, 10, 10);
    }

    // ══════════════════════════════════════════════════════════════
    //  DRAG: mover la barra
    //  Primero intentamos startSystemMove (Wayland), si no cae al
    //  fallback manual que ya existía (X11).
    //  Las labels tienen WA_TransparentForMouseEvents para que el
    //  click pase al padre y se pueda arrastrar desde ahí.
    // ══════════════════════════════════════════════════════════════
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(e);
            return;
        }

        // Wayland / compositores modernos: delegar al sistema
        if (QWindow *wh = windowHandle()) {
            if (wh->startSystemMove()) {
                e->accept();
                return;
            }
        }

        // X11 o fallback manual
        m_dragging = true;
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
        setCursor(Qt::SizeAllCursor);
        e->accept();
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (!m_dragging) {
            QWidget::mouseMoveEvent(e);
            return;
        }
        if (!(e->buttons() & Qt::LeftButton)) {
            m_dragging = false;
            unsetCursor();
            return;
        }
        move(e->globalPosition().toPoint() - m_dragOffset);
        e->accept();
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        m_dragging = false;
        unsetCursor();
        QWidget::mouseReleaseEvent(e);
    }

private slots:
    // ══════════════════════════════════════════════════════════════
    //  Cargar tipografía desde archivo (.ttf / .otf)
    //  La ruta queda guardada en QSettings para que persista entre
    //  sesiones.
    // ══════════════════════════════════════════════════════════════
    void onLoadFontClicked() {
        const QString path = QFileDialog::getOpenFileName(
            this,
            QObject::tr("Cargar tipografía"),
            QDir::homePath(),
            QObject::tr("Tipografías (*.ttf *.otf *.TTF *.OTF);;Todos los archivos (*)"));

        if (path.isEmpty()) return;

        if (!loadFontFromPath(path, /*silent=*/false)) return;

        // Guardar ruta para re-cargarla en próximas sesiones
        QSettings settings("Paintux", "PaintuxStudio");
        QStringList savedFonts = settings.value("customFonts").toStringList();
        if (!savedFonts.contains(path)) {
            savedFonts.append(path);
            settings.setValue("customFonts", savedFonts);
        }
    }

private:
    // ══════════════════════════════════════════════════════════════
    //  Carga una fuente desde disco y la agrega al combo.
    //  Devuelve true si se cargó correctamente.
    //  silent = true → no muestra diálogos (para carga automática).
    // ══════════════════════════════════════════════════════════════
    bool loadFontFromPath(const QString &path, bool silent) {
        if (!QFile::exists(path)) {
            if (!silent) {
                QMessageBox::warning(this, QObject::tr("Tipografía no encontrada"),
                    QObject::tr("El archivo no existe:\n%1").arg(path));
            }
            return false;
        }

        const int fontId = QFontDatabase::addApplicationFont(path);
        if (fontId < 0) {
            if (!silent) {
                QMessageBox::warning(this, QObject::tr("Error al cargar tipografía"),
                    QObject::tr("No se pudo cargar la tipografía:\n%1").arg(path));
            }
            return false;
        }

        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (families.isEmpty()) {
            if (!silent) {
                QMessageBox::warning(this, QObject::tr("Error"),
                    QObject::tr("La tipografía no contiene familias válidas."));
            }
            return false;
        }

        // Agregar cada familia nueva al modelo
        bool addedAny = false;
        for (const QString &family : families) {
            if (family.isEmpty()) continue;

            // Evitar duplicados
            bool exists = false;
            for (int i = 0; i < m_fontModel->rowCount(); ++i) {
                QStandardItem *existing = m_fontModel->item(i);
                if (existing && existing->text() == family) {
                    exists = true;
                    break;
                }
            }
            if (exists) continue;

            QStandardItem *item = new QStandardItem(family);
            QFont itemFont(family);
            itemFont.setPointSize(12);
            item->setFont(itemFont);
            m_fontModel->appendRow(item);
            addedAny = true;
        }

        if (!silent && addedAny) {
            // Seleccionar la primera familia nueva
            fontCombo->setCurrentText(families.first());

            // Notificar al usuario con un tooltip rápido
            QToolTip::showText(QCursor::pos(),
                QObject::tr("Tipografía cargada: %1").arg(families.first()),
                this, QRect(), 2000);

            emit formatChanged();
        }
        return true;
    }

    // ── Widgets ──
    QComboBox *fontCombo = nullptr;
    QSpinBox *sizeSpin = nullptr;
    QDoubleSpinBox *lineSpacingSpin = nullptr;
    QDoubleSpinBox *letterSpacingSpin = nullptr;
    QCheckBox *boldCheck = nullptr;
    QCheckBox *italicCheck = nullptr;
    QPushButton *btnColor = nullptr;
    QPushButton *btnApply = nullptr;
    QPushButton *btnCancel = nullptr;
    QPushButton *btnLoadFont = nullptr;
    QStandardItemModel *m_fontModel = nullptr;

    QColor currentColor = Qt::black;

    // ── Estado del drag ──
    QPoint m_dragOffset;
    bool m_dragging = false;
};

} // namespace TextEngine

#endif