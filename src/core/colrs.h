#ifndef COLRS_H
#define COLRS_H

#include <QDialog>
#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QConicalGradient>
#include <QColor>
#include <QColorDialog>
#include <QCursor>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QVector>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QFrame>
#include <QSize>
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QSettings>
#include <QVariantList>
#include <QListWidget>
#include <QListView>
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QTextStream>
#include <QFileInfo>
#include <QToolTip>
#include <QTimer>
#include <QScrollArea>

namespace Colrs {

class TriangleColorPicker : public QWidget {
    Q_OBJECT
public:
    explicit TriangleColorPicker(QWidget *parent = nullptr);
    QColor color() const { return m_color; }
    void setColor(const QColor &c);

signals:
    void colorChanged(const QColor &color);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    QColor m_color = Qt::red;
    double m_hue = 0.0;
    double m_sat = 1.0;
    double m_val = 1.0;
    bool   m_dragging = false;
    int    m_dragMode = 0;
    QPointF m_center;
    double  m_ringOuter = 0.0;
    double  m_ringInner = 0.0;
    double  m_triRadius = 0.0;
    QPointF m_vWhite, m_vBlack, m_vHue;
    QImage m_cache;
    double m_cachedHue = -1.0;

    void recalcGeometry();
    void invalidateCache();
    static bool barycentric(const QPointF &p, const QPointF &a, const QPointF &b,
                            const QPointF &c, double &wa, double &wb, double &wc);
    void applyClick(const QPointF &pos);
    void emitColor();
    void regenerateCache();
    void paintHueRing(QPainter &p);
    void paintTriangle(QPainter &p);
    void paintMarkers(QPainter &p);
};

inline TriangleColorPicker::TriangleColorPicker(QWidget *parent) : QWidget(parent) {
    setMinimumSize(200, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
    recalcGeometry();
}

inline void TriangleColorPicker::setColor(const QColor &c) {
    if (!c.isValid()) return;
    float h = 0.0f, s = 0.0f, v = 0.0f, a = 0.0f;
    c.getHsvF(&h, &s, &v, &a);
    if (h >= 0.0f) m_hue = static_cast<double>(h);
    m_sat = qBound(0.0, static_cast<double>(s), 1.0);
    m_val = qBound(0.0, static_cast<double>(v), 1.0);
    m_color = QColor::fromHsvF(m_hue, m_sat, m_val, 1.0);
    invalidateCache();
    update();
}

inline void TriangleColorPicker::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    recalcGeometry();
    invalidateCache();
}

inline void TriangleColorPicker::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) return;
    const QPointF pos = e->position();
    const double dx = pos.x() - m_center.x();
    const double dy = pos.y() - m_center.y();
    const double dist = std::sqrt(dx * dx + dy * dy);
    if (dist >= m_ringInner && dist <= m_ringOuter) m_dragMode = 1;
    else m_dragMode = 2;
    m_dragging = true;
    applyClick(pos);
}

inline void TriangleColorPicker::mouseMoveEvent(QMouseEvent *e) {
    if (!m_dragging) return;
    applyClick(e->position());
}

inline void TriangleColorPicker::mouseReleaseEvent(QMouseEvent *) {
    m_dragging = false;
    m_dragMode = 0;
}

inline void TriangleColorPicker::recalcGeometry() {
    const double side = qMin<double>(width(), height());
    m_center = QPointF(width() / 2.0, height() / 2.0);
    m_ringOuter = side * 0.5 - 4.0;
    m_ringInner = m_ringOuter * 0.82;
    m_triRadius = m_ringInner - 6.0;
    auto vertexAt = [&](double deg) -> QPointF {
        const double rad = deg * M_PI / 180.0;
        return QPointF(m_center.x() + m_triRadius * std::cos(rad),
                       m_center.y() - m_triRadius * std::sin(rad));
    };
    m_vWhite = vertexAt(90.0);
    m_vBlack = vertexAt(210.0);
    m_vHue   = vertexAt(330.0);
}

inline void TriangleColorPicker::invalidateCache() { m_cachedHue = -1.0; }

inline bool TriangleColorPicker::barycentric(const QPointF &p, const QPointF &a,
                                             const QPointF &b, const QPointF &c,
                                             double &wa, double &wb, double &wc) {
    const double denom = (b.y() - c.y()) * (a.x() - c.x())
                       + (c.x() - b.x()) * (a.y() - c.y());
    if (std::abs(denom) < 1e-9) return false;
    wa = ((b.y() - c.y()) * (p.x() - c.x()) + (c.x() - b.x()) * (p.y() - c.y())) / denom;
    wb = ((c.y() - a.y()) * (p.x() - c.x()) + (a.x() - c.x()) * (p.y() - c.y())) / denom;
    wc = 1.0 - wa - wb;
    return wa >= -0.001 && wb >= -0.001 && wc >= -0.001;
}

inline void TriangleColorPicker::applyClick(const QPointF &pos) {
    const double dx = pos.x() - m_center.x();
    const double dy = pos.y() - m_center.y();
    const double dist = std::sqrt(dx * dx + dy * dy);
    if (m_dragMode == 1 || (m_dragMode == 0 && dist >= m_ringInner && dist <= m_ringOuter)) {
        double raw = std::atan2(-dy, dx) * 180.0 / M_PI;
        double hueDeg = std::fmod(90.0 - raw + 720.0, 360.0);
        m_hue = hueDeg / 360.0;
        invalidateCache();
        update();
        emitColor();
        return;
    }
    if (m_dragMode == 2 || m_dragMode == 0) {
        double w, b, s;
        if (barycentric(pos, m_vWhite, m_vBlack, m_vHue, w, b, s)) {
            m_sat = qBound(0.0, s, 1.0);
            m_val = qBound(0.0, w + s, 1.0);
            update();
            emitColor();
        }
    }
}

inline void TriangleColorPicker::emitColor() {
    QColor c = QColor::fromHsvF(m_hue, m_sat, m_val, 1.0);
    if (c != m_color) {
        m_color = c;
        emit colorChanged(m_color);
    }
}

inline void TriangleColorPicker::regenerateCache() {
    const int size = int(m_triRadius * 2.0) + 4;
    if (size <= 8) return;
    m_cache = QImage(size, size, QImage::Format_ARGB32);
    m_cache.fill(Qt::transparent);
    const QPointF lc(size / 2.0, size / 2.0);
    const QPointF lw = m_vWhite - m_center + lc;
    const QPointF lb = m_vBlack - m_center + lc;
    const QPointF lh = m_vHue   - m_center + lc;
    const double hue = m_hue;
    for (int y = 0; y < size; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(m_cache.scanLine(y));
        for (int x = 0; x < size; ++x) {
            const QPointF pt(x + 0.5, y + 0.5);
            double w, b, s;
            if (!barycentric(pt, lw, lb, lh, w, b, s)) continue;
            const double V = w + s;
            const double S = (V > 0.0001) ? (s / V) : 0.0;
            line[x] = QColor::fromHsvF(hue, qBound(0.0, S, 1.0),
                                       qBound(0.0, V, 1.0), 1.0).rgba();
        }
    }
    m_cachedHue = hue;
}

inline void TriangleColorPicker::paintHueRing(QPainter &p) {
    const QRectF ringRect(m_center.x() - m_ringOuter, m_center.y() - m_ringOuter,
                          m_ringOuter * 2, m_ringOuter * 2);
    QConicalGradient grad(m_center, 90.0);
    for (int i = 0; i <= 72; ++i) {
        const double t = i / 72.0;
        grad.setColorAt(t, QColor::fromHsvF(1.0 - t, 1.0, 1.0));
    }
    QPainterPath outer; outer.addEllipse(ringRect);
    QPainterPath inner; inner.addEllipse(m_center, m_ringInner, m_ringInner);
    QPainterPath ring = outer.subtracted(inner);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawPath(ring);
    p.restore();
}

inline void TriangleColorPicker::paintTriangle(QPainter &p) {
    if (m_cache.isNull() || m_cachedHue != m_hue) regenerateCache();
    if (m_cache.isNull()) return;
    p.drawImage(m_center.x() - m_cache.width() / 2.0,
                m_center.y() - m_cache.height() / 2.0, m_cache);
}

inline void TriangleColorPicker::paintMarkers(QPainter &p) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    const double qtAngle = (90.0 - m_hue * 360.0) * M_PI / 180.0;
    const double midR = (m_ringOuter + m_ringInner) / 2.0;
    const QPointF huePos(m_center.x() + midR * std::cos(qtAngle),
                          m_center.y() - midR * std::sin(qtAngle));
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Qt::white, 2.5)); p.drawEllipse(huePos, 7.0, 7.0);
    p.setPen(QPen(Qt::black, 1.0)); p.drawEllipse(huePos, 7.0, 7.0);
    const double w = m_val * (1.0 - m_sat);
    const double b = 1.0 - m_val;
    const double s = m_val * m_sat;
    const QPointF svPos = w * m_vWhite + b * m_vBlack + s * m_vHue;
    p.setPen(QPen(Qt::white, 2.5)); p.drawEllipse(svPos, 6.5, 6.5);
    p.setPen(QPen(Qt::black, 1.0)); p.drawEllipse(svPos, 6.5, 6.5);
    p.restore();
}

inline void TriangleColorPicker::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    paintHueRing(p);
    paintTriangle(p);
    paintMarkers(p);
}

class ColorSlotButton : public QPushButton {
    Q_OBJECT
public:
    explicit ColorSlotButton(int slotIndex, QWidget *parent = nullptr);
    void setColor(const QColor &c) { m_color = c; update(); }
    QColor color() const { return m_color; }
    int slotIndex() const { return m_slotIndex; }
    void setActive(bool a) { m_active = a; update(); }

signals:
    void slotClicked(int slotIndex);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    int m_slotIndex;
    QColor m_color = Qt::black;
    bool m_active = false;
};

inline ColorSlotButton::ColorSlotButton(int slotIndex, QWidget *parent)
    : QPushButton(parent), m_slotIndex(slotIndex) {
    setFixedSize(34, 56);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
}

inline void ColorSlotButton::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRect r = rect().adjusted(2, 2, -2, -2);
    QPixmap checker(8, 8);
    checker.fill(Qt::white);
    QPainter cp(&checker);
    cp.fillRect(0, 0, 4, 4, QColor(200, 200, 200));
    cp.fillRect(4, 4, 4, 4, QColor(200, 200, 200));
    cp.end();
    p.save();
    QPainterPath clip;
    clip.addRoundedRect(r, 4, 4);
    p.setClipPath(clip);
    p.drawTiledPixmap(r, checker);
    p.fillRect(r, m_color);
    p.restore();
    if (m_active) {
        p.setPen(QPen(QColor("#3b82f6"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(-1, -1, 1, 1), 5, 5);
    } else {
        p.setPen(QPen(QColor(120, 120, 120), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 4, 4);
    }
}

inline void ColorSlotButton::mousePressEvent(QMouseEvent *e) {
    emit slotClicked(m_slotIndex);
    QPushButton::mousePressEvent(e);
}

class PaletteStore {
public:
    struct Palette {
        QString name;
        QVector<QColor> colors;
    };

    static QString dir() {
        QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (base.isEmpty()) base = QDir::homePath() + "/.paintux";
        QDir d(base);
        if (!d.exists("palettes")) d.mkpath("palettes");
        return d.filePath("palettes");
    }

    static QString sanitize(const QString &n) {
        QString s = n.trimmed();
        static const QRegularExpression re("[^A-Za-z0-9_\\- ]");
        s.replace(re, "_");
        if (s.isEmpty()) s = "palette";
        return s;
    }

    static bool save(const Palette &p) {
        if (p.name.trimmed().isEmpty()) return false;
        QJsonObject obj;
        obj["name"] = p.name;
        QJsonArray arr;
        for (const QColor &c : p.colors) arr.append(c.name(QColor::HexArgb));
        obj["colors"] = arr;
        QFile f(QDir(dir()).filePath(sanitize(p.name) + ".json"));
        if (!f.open(QIODevice::WriteOnly)) return false;
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        return true;
    }

    static Palette load(const QString &filePath) {
        Palette p;
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) return p;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) return p;
        QJsonObject obj = doc.object();
        p.name = obj.value("name").toString();
        for (const QJsonValue &v : obj.value("colors").toArray()) {
            QColor c(v.toString());
            if (c.isValid()) p.colors.append(c);
        }
        return p;
    }

    static QVector<Palette> loadAll() {
        QVector<Palette> out;
        QDir d(dir());
        const QStringList files = d.entryList({"*.json"}, QDir::Files, QDir::Name);
        for (const QString &fn : files) {
            Palette p = load(d.filePath(fn));
            if (!p.name.isEmpty()) out.append(p);
        }
        return out;
    }

    static bool remove(const QString &name) {
        QFile f(QDir(dir()).filePath(sanitize(name) + ".json"));
        return f.exists() && f.remove();
    }

    static bool exists(const QString &name) {
        return QFile::exists(QDir(dir()).filePath(sanitize(name) + ".json"));
    }

    static Palette importGpl(const QString &filePath) {
        Palette p;
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return p;
        QTextStream in(&f);
        bool headerSkipped = false;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            if (!headerSkipped) {
                if (line.startsWith("GIMP Palette")) { headerSkipped = true; continue; }
            }
            if (line.startsWith("Name:"))     { p.name = line.mid(5).trimmed(); continue; }
            if (line.startsWith("Columns:"))  continue;
            const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                bool okR, okG, okB;
                int r = parts[0].toInt(&okR);
                int g = parts[1].toInt(&okG);
                int b = parts[2].toInt(&okB);
                if (okR && okG && okB)
                    p.colors.append(QColor(qBound(0,r,255), qBound(0,g,255), qBound(0,b,255)));
            }
        }
        if (p.name.isEmpty()) p.name = QFileInfo(filePath).baseName();
        return p;
    }

    static bool exportGpl(const Palette &p, const QString &filePath) {
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QTextStream out(&f);
        out << "GIMP Palette\n";
        out << "Name: " << p.name << "\n";
        out << "Columns: 10\n";
        out << "#\n";
        for (const QColor &c : p.colors)
            out << QString("%1 %2 %3\t%4\n")
                   .arg(c.red(), 3).arg(c.green(), 3).arg(c.blue(), 3)
                   .arg(c.name(QColor::HexRgb));
        return true;
    }

    static QVector<QColor> defaultMSPaint() {
        return {
            Qt::black, QColor(127,127,127), QColor(136,0,21), Qt::red,
            QColor(255,127,39), Qt::yellow, Qt::green, QColor(0,162,232),
            QColor(63,72,204), QColor(163,73,164), Qt::white, QColor(195,195,195),
            QColor(185,122,87), QColor(255,174,201), QColor(255,201,14),
            QColor(239,228,176), QColor(181,230,29), QColor(153,217,234),
            QColor(112,146,190), QColor(200,191,231)
        };
    }

    static QVector<QColor> defaultClassicArt() {
        return {
            QColor(255,255,255), QColor(240,230,210), QColor(220,190,150),
            QColor(180,120,70),  QColor(120,70,40),   QColor(60,30,15),
            QColor(0,0,0),       QColor(80,80,80),    QColor(160,160,160),
            QColor(220,40,40),   QColor(240,120,120), QColor(255,200,200),
            QColor(255,180,40),  QColor(255,220,120), QColor(240,240,180),
            QColor(40,120,60),   QColor(90,180,90),   QColor(180,220,140),
            QColor(30,90,160),   QColor(80,150,220)
        };
    }

    static QVector<QColor> defaultRetroNeon() {
        return {
            QColor(10,5,20),     QColor(40,10,60),    QColor(90,20,110),
            QColor(180,20,140),  QColor(255,40,180),  QColor(255,120,200),
            QColor(0,220,220),   QColor(0,180,255),   QColor(0,120,220),
            QColor(60,80,220),   QColor(140,60,220),  QColor(200,60,240),
            QColor(255,180,0),   QColor(255,100,20),  QColor(255,60,60),
            QColor(180,255,0),   QColor(0,255,150),   QColor(100,255,220),
            QColor(240,240,255), QColor(120,120,160)
        };
    }
};

class PaletteEditorDialog : public QDialog {
    Q_OBJECT
public:
    PaletteEditorDialog(const QString &name, const QVector<QColor> &colors,
                        bool darkMode, QWidget *parent = nullptr)
        : QDialog(parent), m_dark(darkMode)
    {
        setWindowTitle(tr("Editar paleta"));
        setMinimumSize(460, 480);
        applyTheme();

        QVBoxLayout *root = new QVBoxLayout(this);
        root->setContentsMargins(14, 14, 14, 14);
        root->setSpacing(10);

        QHBoxLayout *nameRow = new QHBoxLayout();
        QLabel *lblName = new QLabel(tr("Nombre:"));
        m_nameEdit = new QLineEdit(name);
        m_nameEdit->setPlaceholderText(tr("Mi paleta"));
        nameRow->addWidget(lblName);
        nameRow->addWidget(m_nameEdit, 1);
        root->addLayout(nameRow);

        QLabel *lblColors = new QLabel(tr("Colores (clic reemplaza - clic derecho quita):"));
        root->addWidget(lblColors);

        m_grid = new QListWidget();
        m_grid->setViewMode(QListView::IconMode);
        m_grid->setIconSize(QSize(36, 36));
        m_grid->setGridSize(QSize(46, 46));
        m_grid->setResizeMode(QListView::Adjust);
        m_grid->setMovement(QListView::Static);
        m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
        m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
        m_grid->setSpacing(2);
        root->addWidget(m_grid, 1);

        QHBoxLayout *editRow = new QHBoxLayout();
        QPushButton *btnAdd    = new QPushButton(tr("+ Anadir"));
        QPushButton *btnRemove = new QPushButton(tr("- Quitar"));
        QPushButton *btnEdit   = new QPushButton(tr("Reemplazar"));
        editRow->addWidget(btnAdd);
        editRow->addWidget(btnRemove);
        editRow->addWidget(btnEdit);
        editRow->addStretch();
        root->addLayout(editRow);

        QDialogButtonBox *btns = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        btns->button(QDialogButtonBox::Ok)->setText(tr("Guardar"));
        btns->button(QDialogButtonBox::Ok)->setStyleSheet(
            "QPushButton{background:#2563eb;color:#fff;border:none;border-radius:6px;"
            "padding:6px 18px;font-weight:600;}"
            "QPushButton:hover{background:#1d4ed8;}");
        root->addWidget(btns);
        connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

        for (const QColor &c : colors) addColorItem(c);

        connect(btnAdd,    &QPushButton::clicked, this, &PaletteEditorDialog::onAdd);
        connect(btnRemove, &QPushButton::clicked, this, &PaletteEditorDialog::onRemove);
        connect(btnEdit,   &QPushButton::clicked, this, &PaletteEditorDialog::onReplace);
        connect(m_grid, &QListWidget::itemClicked, this, [this](QListWidgetItem *it) {
            if (!it) return;
            onReplaceItem(it);
        });
        connect(m_grid, &QListWidget::customContextMenuRequested,
                this, &PaletteEditorDialog::onContextMenu);
    }

    QString paletteName() const { return m_nameEdit->text().trimmed(); }

    QVector<QColor> paletteColors() const {
        QVector<QColor> out;
        for (int i = 0; i < m_grid->count(); ++i)
            out.append(m_grid->item(i)->data(Qt::UserRole).value<QColor>());
        return out;
    }

private:
    bool m_dark;
    QLineEdit   *m_nameEdit = nullptr;
    QListWidget *m_grid = nullptr;

    void applyTheme() {
        QString bg     = m_dark ? "#1a1a1a" : "#fafafa";
        QString panel  = m_dark ? "#242424" : "#ffffff";
        QString input  = m_dark ? "#2a2a2a" : "#ffffff";
        QString text   = m_dark ? "#e5e5e5" : "#111827";
        QString border = m_dark ? "#3a3a3a" : "#d1d5db";
        QString accent = "#3b82f6";
        setStyleSheet(QString(
            "QDialog { background-color: %1; }"
            "QLabel { color: %2; font-size: 12px; background: transparent; }"
            "QLineEdit { background-color: %3; color: %2; border: 1px solid %4;"
            " border-radius: 5px; padding: 5px 8px; font-size: 12px; }"
            "QLineEdit:focus { border: 1px solid %5; }"
            "QListWidget { background-color: %6; border: 1px solid %4; border-radius: 6px;"
            " padding: 4px; outline: none; }"
            "QListWidget::item { border-radius: 4px; }"
            "QListWidget::item:selected { background: %5; }"
            "QPushButton { background-color: %3; color: %2; border: 1px solid %4;"
            " border-radius: 5px; padding: 5px 12px; font-size: 12px; }"
            "QPushButton:hover { border: 1px solid %5; }")
            .arg(bg, text, input, border, accent, panel));
    }

    static QPixmap makeSwatchPix(const QColor &c) {
        QPixmap pm(36, 36);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        for (int y = 0; y < 36; y += 6)
            for (int x = 0; x < 36; x += 6)
                p.fillRect(x, y, 6, 6,
                    ((x/6 + y/6) % 2) ? QColor(200,200,200) : QColor(240,240,240));
        p.setBrush(c);
        p.setPen(QPen(QColor(0,0,0,90), 1));
        p.drawRoundedRect(QRectF(0.5, 0.5, 35, 35), 4, 4);
        p.end();
        return pm;
    }

    void addColorItem(const QColor &c) {
        QListWidgetItem *it = new QListWidgetItem();
        it->setData(Qt::UserRole, c);
        it->setIcon(QIcon(makeSwatchPix(c)));
        it->setToolTip(c.name(QColor::HexArgb));
        m_grid->addItem(it);
    }

    void replaceColorItem(QListWidgetItem *it, const QColor &c) {
        it->setData(Qt::UserRole, c);
        it->setIcon(QIcon(makeSwatchPix(c)));
        it->setToolTip(c.name(QColor::HexArgb));
    }

    void onAdd() {
        QColor c = QColorDialog::getColor(Qt::black, this, tr("Color"));
        if (c.isValid()) addColorItem(c);
    }

    void onRemove() {
        QListWidgetItem *it = m_grid->currentItem();
        if (it) delete m_grid->takeItem(m_grid->row(it));
    }

    void onReplace() {
        QListWidgetItem *it = m_grid->currentItem();
        if (it) onReplaceItem(it);
    }

    void onReplaceItem(QListWidgetItem *it) {
        QColor cur = it->data(Qt::UserRole).value<QColor>();
        QColor c = QColorDialog::getColor(cur, this, tr("Color"));
        if (c.isValid()) replaceColorItem(it, c);
    }

    void onContextMenu(const QPoint &pos) {
        QListWidgetItem *it = m_grid->itemAt(pos);
        if (!it) return;
        QMenu menu(this);
        QAction *aEdit = menu.addAction(tr("Reemplazar color..."));
        QAction *aDup  = menu.addAction(tr("Duplicar"));
        QAction *aDel  = menu.addAction(tr("Eliminar"));
        QAction *chosen = menu.exec(m_grid->mapToGlobal(pos));
        if (chosen == aEdit) onReplaceItem(it);
        else if (chosen == aDup) addColorItem(it->data(Qt::UserRole).value<QColor>());
        else if (chosen == aDel) delete m_grid->takeItem(m_grid->row(it));
    }
};

class PaletteStripWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaletteStripWidget(QWidget *parent = nullptr);

    void setColumns(int c) { m_cols = qMax(1, c); updateSize(); update(); }
    int  columns() const { return m_cols; }
    void setCellSize(int px) { m_cell = qBound(8, px, 60); updateSize(); update(); }
    void setGap(int g) { m_gap = qMax(0, g); updateSize(); update(); }
    void setDarkMode(bool dark) { m_dark = dark; update(); }

    void setMaxVisibleColors(int n) { m_maxVisible = qMax(0, n); updateSize(); update(); }
    int  maxVisibleColors() const { return m_maxVisible; }

    void setPalette(const QVector<QColor> &colors, const QString &name = QString()) {
        m_colors = colors;
        if (!name.isEmpty()) m_paletteName = name;
        m_selectedIdx = -1;
        updateSize(); update();
    }
    const QVector<QColor> &palette() const { return m_colors; }
    QString paletteName() const { return m_paletteName; }
    void setPaletteName(const QString &name) { m_paletteName = name; }

    void setShowRecent(bool s) { m_showRecent = s; updateSize(); update(); }
    bool showRecent() const { return m_showRecent; }

    void setShowManageMenu(bool s) { m_showManageMenu = s; }
    void setEditable(bool e) { m_editable = e; }
    bool isEditable() const { return m_editable; }

    QColor selectedColor() const {
        if (m_selectedIdx >= 0 && m_selectedIdx < m_colors.size())
            return m_colors[m_selectedIdx];
        return QColor();
    }
    void setSelectedIndex(int i) { m_selectedIdx = i; update(); }
    void clearSelection() { m_selectedIdx = -1; update(); }

    void addRecentColor(const QColor &c);
    void reloadRecentFromSettings();

    QSize sizeHint() const override;

    void showPaletteMenuAt(const QPoint &globalPos);

signals:
    void colorClicked(const QColor &color, bool isRecent);
    void paletteChanged(const QString &name, const QVector<QColor> &colors);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    QVector<QColor> m_colors;
    QVector<QColor> m_recent;
    QString m_paletteName = "MS Paint";
    int m_cols = 10;
    int m_cell = 22;
    int m_gap  = 3;
    int m_hover = -1;
    int m_hoverRecent = -1;
    int m_selectedIdx = -1;
    bool m_showRecent = true;
    bool m_showManageMenu = true;
    bool m_editable = false;
    int m_recentSlots = 10;
    int m_maxVisible = 0;
    bool m_dark = true;

    int visibleCount() const {
        return (m_maxVisible <= 0) ? m_colors.size()
                                   : qMin(m_colors.size(), m_maxVisible);
    }

    void updateSize() { setFixedSize(sizeHint()); }
    QRect cellRect(int idx) const;
    QRect recentRect(int idx) const;
    int paletteIndexAt(const QPoint &p) const;
    int recentIndexAt(const QPoint &p) const;
    void drawSwatch(QPainter &p, const QRect &r, const QColor &c, bool selected, bool hover);

    void createNewPalette();
    void editCurrentPalette();
    void duplicateCurrentPalette();
    void loadPaletteFromDisk();
    void importGplFromDisk();
    void exportCurrentAsGpl();
    void deleteCurrentPalette();
};

inline PaletteStripWidget::PaletteStripWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    m_colors = PaletteStore::defaultMSPaint();
    reloadRecentFromSettings();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    updateSize();
}

inline void PaletteStripWidget::reloadRecentFromSettings() {
    m_recent.clear();
    QSettings s("Paintux", "PaintuxStudio");
    const QVariantList recents = s.value("recentColors").toList();
    for (const QVariant &v : recents) {
        QColor c(v.toString());
        if (c.isValid() && m_recent.size() < m_recentSlots) m_recent.append(c);
    }
    while (m_recent.size() < m_recentSlots) m_recent.append(QColor(0,0,0,0));
}

inline void PaletteStripWidget::addRecentColor(const QColor &c) {
    if (!c.isValid()) return;
    for (int i = m_recent.size() - 1; i >= 0; --i)
        if (m_recent[i] == c) m_recent.removeAt(i);
    m_recent.prepend(c);
    while (m_recent.size() > m_recentSlots) m_recent.removeLast();

    QVariantList vs;
    for (const QColor &rc : m_recent) if (rc.isValid()) vs.append(rc.name(QColor::HexArgb));
    QSettings s("Paintux", "PaintuxStudio");
    s.setValue("recentColors", vs);
    update();
}

inline QSize PaletteStripWidget::sizeHint() const {
    const int vis = visibleCount();
    if (vis <= 0 && !m_showRecent) return QSize(0, 0);
    const int rows = (vis + m_cols - 1) / m_cols;
    const int recentRows = m_showRecent ? (m_recentSlots + m_cols - 1) / m_cols : 0;
    int w = m_cols * (m_cell + m_gap) + m_gap;
    int h = rows * (m_cell + m_gap) + m_gap;
    if (m_showRecent && recentRows > 0)
        h += m_gap + recentRows * (m_cell + m_gap);
    return QSize(w, h);
}

inline QRect PaletteStripWidget::cellRect(int idx) const {
    if (idx < 0 || idx >= visibleCount()) return QRect();
    const int row = idx / m_cols;
    const int col = idx % m_cols;
    return QRect(m_gap + col * (m_cell + m_gap),
                 m_gap + row * (m_cell + m_gap),
                 m_cell, m_cell);
}

inline QRect PaletteStripWidget::recentRect(int idx) const {
    const int vis = visibleCount();
    const int rows = (vis + m_cols - 1) / m_cols;
    const int baseY = m_gap + rows * (m_cell + m_gap) + m_gap;
    const int row = idx / m_cols;
    const int col = idx % m_cols;
    return QRect(m_gap + col * (m_cell + m_gap),
                 baseY + row * (m_cell + m_gap),
                 m_cell, m_cell);
}

inline int PaletteStripWidget::paletteIndexAt(const QPoint &p) const {
    for (int i = 0; i < visibleCount(); ++i)
        if (cellRect(i).contains(p)) return i;
    return -1;
}

inline int PaletteStripWidget::recentIndexAt(const QPoint &p) const {
    if (!m_showRecent) return -1;
    for (int i = 0; i < m_recent.size(); ++i)
        if (recentRect(i).contains(p)) return i;
    return -1;
}

inline void PaletteStripWidget::drawSwatch(QPainter &p, const QRect &r, const QColor &c,
                                           bool selected, bool hover) {
    if (c.alpha() < 255) {
        for (int y = 0; y < r.height(); y += 5)
            for (int x = 0; x < r.width(); x += 5)
                p.fillRect(r.x()+x, r.y()+y, 5, 5,
                    ((x/5 + y/5) % 2) ? QColor(190,190,190) : QColor(230,230,230));
    }
    p.fillRect(r, c);
    if (selected) {
        p.setPen(QPen(QColor("#3b82f6"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0,0,-1,-1));
        p.setPen(QPen(Qt::white, 1));
        p.drawRect(r.adjusted(1,1,-2,-2));
    } else if (hover) {
        p.setPen(QPen(QColor("#60a5fa"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0,0,-1,-1));
    } else {
        p.setPen(QPen(QColor(0,0,0,60), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0,0,-1,-1));
    }
}

inline void PaletteStripWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    for (int i = 0; i < visibleCount(); ++i) {
        const QRect r = cellRect(i);
        drawSwatch(p, r, m_colors[i], i == m_selectedIdx, i == m_hover);
    }
    if (m_showRecent) {
        for (int i = 0; i < m_recent.size(); ++i) {
            const QRect r = recentRect(i);
            const QColor c = m_recent[i];
            const bool hover = (i == m_hoverRecent);
            if (!c.isValid()) {
                p.setPen(QPen(m_dark ? QColor(90,90,90) : QColor(180,180,180),
                              1, Qt::DashLine));
                p.setBrush(m_dark ? QColor(35,35,35) : QColor(245,245,245));
                p.drawRoundedRect(r.adjusted(0,0,-1,-1), 3, 3);
                if (hover) {
                    p.setPen(QPen(QColor(100,150,255), 1));
                    p.drawRoundedRect(r.adjusted(0,0,-1,-1), 3, 3);
                }
            } else {
                drawSwatch(p, r, c, false, hover);
            }
        }
    }
}

inline void PaletteStripWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) { QWidget::mousePressEvent(e); return; }
    int idx = paletteIndexAt(e->pos());
    if (idx >= 0) {
        m_selectedIdx = idx;
        update();
        if (m_editable && (e->modifiers() & Qt::ShiftModifier)) {
            QColor cur = m_colors[idx];
            QColor c = QColorDialog::getColor(cur, this, tr("Color"));
            if (c.isValid()) {
                m_colors[idx] = c;
                PaletteStore::Palette p{ m_paletteName, m_colors };
                PaletteStore::save(p);
                update();
                emit paletteChanged(m_paletteName, m_colors);
            }
            return;
        }
        emit colorClicked(m_colors[idx], false);
        return;
    }
    int ridx = recentIndexAt(e->pos());
    if (ridx >= 0 && m_recent[ridx].isValid())
        emit colorClicked(m_recent[ridx], true);
}

inline void PaletteStripWidget::mouseMoveEvent(QMouseEvent *e) {
    int idx = paletteIndexAt(e->pos());
    int ridx = recentIndexAt(e->pos());
    if (idx != m_hover || ridx != m_hoverRecent) {
        m_hover = idx;
        m_hoverRecent = ridx;
        update();
    }
}

inline void PaletteStripWidget::leaveEvent(QEvent *) {
    if (m_hover != -1 || m_hoverRecent != -1) {
        m_hover = -1; m_hoverRecent = -1; update();
    }
}

inline void PaletteStripWidget::contextMenuEvent(QContextMenuEvent *e) {
    if (!m_showManageMenu && !m_editable) return;
    showPaletteMenuAt(e->globalPos());
}

inline void PaletteStripWidget::showPaletteMenuAt(const QPoint &globalPos) {
    QMenu menu(this);
    QAction *aNew      = menu.addAction(tr("Nueva paleta..."));
    QAction *aEdit     = menu.addAction(tr("Editar paleta actual..."));
    QAction *aDuplicate= menu.addAction(tr("Duplicar paleta actual"));
    menu.addSeparator();
    QAction *aLoad     = menu.addAction(tr("Cargar paleta..."));
    QAction *aImport   = menu.addAction(tr("Importar .gpl..."));
    QAction *aExport   = menu.addAction(tr("Exportar actual como .gpl..."));
    menu.addSeparator();
    QAction *aDelete   = menu.addAction(tr("Eliminar paleta actual"));
    aDelete->setEnabled(PaletteStore::exists(m_paletteName));

    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;
    if (chosen == aNew) createNewPalette();
    else if (chosen == aEdit) editCurrentPalette();
    else if (chosen == aDuplicate) duplicateCurrentPalette();
    else if (chosen == aLoad) loadPaletteFromDisk();
    else if (chosen == aImport) importGplFromDisk();
    else if (chosen == aExport) exportCurrentAsGpl();
    else if (chosen == aDelete) deleteCurrentPalette();
}

inline void PaletteStripWidget::createNewPalette() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nueva paleta"),
                                         tr("Nombre de la paleta:"),
                                         QLineEdit::Normal, tr("Mi paleta"), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    if (PaletteStore::exists(name)) {
        if (QMessageBox::question(this, tr("Ya existe"),
                tr("Ya existe '%1'. Sobrescribir?").arg(name)) != QMessageBox::Yes) return;
    }
    QVector<QColor> base = m_colors;
    while (base.size() < 20) base.append(Qt::white);
    base.resize(20);
    PaletteEditorDialog dlg(name, base, m_dark, this);
    if (dlg.exec() != QDialog::Accepted) return;
    PaletteStore::Palette p{ dlg.paletteName(), dlg.paletteColors() };
    if (PaletteStore::save(p)) {
        m_paletteName = p.name;
        m_colors = p.colors;
        m_selectedIdx = -1;
        updateSize(); update();
        emit paletteChanged(m_paletteName, m_colors);
    }
}

inline void PaletteStripWidget::editCurrentPalette() {
    PaletteEditorDialog dlg(m_paletteName, m_colors, m_dark, this);
    if (dlg.exec() != QDialog::Accepted) return;
    PaletteStore::Palette p{ dlg.paletteName(), dlg.paletteColors() };
    if (p.name.isEmpty()) return;
    if (PaletteStore::save(p)) {
        m_paletteName = p.name;
        m_colors = p.colors;
        m_selectedIdx = -1;
        updateSize(); update();
        emit paletteChanged(m_paletteName, m_colors);
    }
}

inline void PaletteStripWidget::duplicateCurrentPalette() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Duplicar paleta"),
                                         tr("Nombre de la copia:"),
                                         QLineEdit::Normal,
                                         m_paletteName + " (copia)", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    PaletteStore::Palette p{ name, m_colors };
    if (PaletteStore::save(p)) {
        m_paletteName = p.name;
        updateSize(); update();
        emit paletteChanged(m_paletteName, m_colors);
    }
}

inline void PaletteStripWidget::loadPaletteFromDisk() {
    const QVector<PaletteStore::Palette> all = PaletteStore::loadAll();
    if (all.isEmpty()) {
        QMessageBox::information(this, tr("Sin paletas"),
            tr("Todavia no tenes paletas guardadas."));
        return;
    }
    QStringList names;
    for (const auto &p : all) names << p.name;
    bool ok = false;
    QString chosen = QInputDialog::getItem(this, tr("Cargar paleta"),
                                           tr("Elegi una paleta:"),
                                           names, 0, false, &ok);
    if (!ok) return;
    for (const auto &p : all) {
        if (p.name == chosen) {
            m_paletteName = p.name;
            m_colors = p.colors;
            m_selectedIdx = -1;
            updateSize(); update();
            emit paletteChanged(m_paletteName, m_colors);
            break;
        }
    }
}

inline void PaletteStripWidget::importGplFromDisk() {
    QString path = QFileDialog::getOpenFileName(this, tr("Importar .gpl"),
                    QDir::homePath(), tr("Paletas GIMP (*.gpl);;Todos (*)"));
    if (path.isEmpty()) return;
    PaletteStore::Palette p = PaletteStore::importGpl(path);
    if (p.colors.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("No se pudo leer la paleta."));
        return;
    }
    if (PaletteStore::exists(p.name)) {
        bool ok = false;
        QString nn = QInputDialog::getText(this, tr("Nombre en uso"),
            tr("Ya existe '%1'. Nuevo nombre:").arg(p.name),
            QLineEdit::Normal, p.name + " (import)", &ok);
        if (!ok || nn.isEmpty()) return;
        p.name = nn;
    }
    PaletteStore::save(p);
    m_paletteName = p.name;
    m_colors = p.colors;
    m_selectedIdx = -1;
    updateSize(); update();
    emit paletteChanged(m_paletteName, m_colors);
}

inline void PaletteStripWidget::exportCurrentAsGpl() {
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar paleta"),
                    QDir::homePath() + "/" + m_paletteName + ".gpl",
                    tr("Paleta GIMP (*.gpl)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(".gpl", Qt::CaseInsensitive)) path += ".gpl";
    PaletteStore::Palette p{ m_paletteName, m_colors };
    if (!PaletteStore::exportGpl(p, path))
        QMessageBox::warning(this, tr("Error"), tr("No se pudo exportar."));
}

inline void PaletteStripWidget::deleteCurrentPalette() {
    if (!PaletteStore::exists(m_paletteName)) return;
    if (QMessageBox::question(this, tr("Eliminar paleta"),
            tr("Eliminar la paleta '%1'?").arg(m_paletteName))
        != QMessageBox::Yes) return;
    PaletteStore::remove(m_paletteName);
    m_paletteName = "MS Paint";
    m_colors = PaletteStore::defaultMSPaint();
    m_selectedIdx = -1;
    updateSize(); update();
    emit paletteChanged(m_paletteName, m_colors);
}

class ColorPickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ColorPickerDialog(const QColor &initial, QWidget *parent = nullptr);
    QColor primaryColor() const   { return m_slots[0]; }
    QColor secondaryColor() const { return m_slots[1]; }
    QColor selectedColor() const  { return m_slots[m_activeSlot]; }

private:
    TriangleColorPicker *m_triangle = nullptr;
    PaletteStripWidget  *m_stripMSPaint = nullptr;
    PaletteStripWidget  *m_stripClassic = nullptr;
    PaletteStripWidget  *m_stripRetro = nullptr;
    PaletteStripWidget  *m_stripCustom = nullptr;
    QLabel *m_customLabel = nullptr;
    QPushButton *m_btnMorePalettes = nullptr;
    ColorSlotButton *m_slotBtn[2] = { nullptr, nullptr };
    QLineEdit *m_hexEdit = nullptr;
    QLabel *m_preview = nullptr;

    QColor m_slots[2];
    int m_activeSlot = 0;
    bool m_syncing = false;

    void buildUi();
    void onTriangleChanged(const QColor &c);
    void onPaletteColorClicked(const QColor &c);
    void onSlotClicked(int idx);
    void onHexEdited();
    void onMorePalettesClicked();
    void updateActiveSlotVisuals();
    void syncTriangleFromActiveSlot();
    void syncHexFromColor();
    void updatePreview();
    void updateCustomPaletteSection();
};

inline ColorPickerDialog::ColorPickerDialog(const QColor &initial, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Selector de Color"));
    setMinimumSize(560, 580);
    m_slots[0] = initial.isValid() ? initial : QColor(Qt::black);
    m_slots[1] = Qt::white;
    buildUi();
    updateActiveSlotVisuals();
    syncTriangleFromActiveSlot();
    syncHexFromColor();
    updatePreview();
}

inline void ColorPickerDialog::buildUi() {
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(6);

    QHBoxLayout *mainContent = new QHBoxLayout();
    mainContent->setSpacing(10);

    m_triangle = new TriangleColorPicker(this);
    m_triangle->setFixedSize(280, 280);
    mainContent->addWidget(m_triangle, 0, Qt::AlignTop);

    QVBoxLayout *rightCol = new QVBoxLayout();
    rightCol->setSpacing(2);
    rightCol->setContentsMargins(0, 0, 0, 0);

    auto makePaletteRow = [this, rightCol](const QString &labelText,
                                           PaletteStripWidget **outStrip,
                                           const QVector<QColor> &colors,
                                           const QString &name,
                                           bool manage,
                                           QLabel **outLabel) {
        QLabel *lbl = new QLabel(labelText, this);
        lbl->setStyleSheet("color: #888; font-size: 10px; font-weight: 600;"
                           " padding: 0; margin: 0;");
        rightCol->addWidget(lbl);
        if (outLabel) *outLabel = lbl;

        PaletteStripWidget *strip = new PaletteStripWidget(this);
        strip->setColumns(10);
        strip->setCellSize(22);
        strip->setGap(3);
        strip->setMaxVisibleColors(20);
        strip->setShowRecent(false);
        strip->setShowManageMenu(manage);
        strip->setDarkMode(true);
        strip->setPalette(colors, name);
        rightCol->addWidget(strip, 0, Qt::AlignLeft);
        *outStrip = strip;
    };

    makePaletteRow(tr("MS Paint"), &m_stripMSPaint,
                   PaletteStore::defaultMSPaint(), "MS Paint", false, nullptr);

    makePaletteRow(tr("Clasica artista"), &m_stripClassic,
                   PaletteStore::defaultClassicArt(), "Clasica artista", false, nullptr);

    makePaletteRow(tr("Retro neón"), &m_stripRetro,
                   PaletteStore::defaultRetroNeon(), "Retro neón", false, nullptr);

    makePaletteRow(tr("Personalizada"), &m_stripCustom,
                   QVector<QColor>(), "Personalizada", true, &m_customLabel);

    m_customLabel->setVisible(false);
    m_stripCustom->setVisible(false);

    m_btnMorePalettes = new QPushButton(tr("+ Mas paletas..."), this);
    m_btnMorePalettes->setCursor(Qt::PointingHandCursor);
    m_btnMorePalettes->setFixedHeight(24);
    m_btnMorePalettes->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #e0e0e0;"
        " border: 1px dashed #555; border-radius: 5px;"
        " padding: 0 10px; font-size: 11px; }"
        "QPushButton:hover { background-color: #333; border: 1px solid #3b82f6; }");
    rightCol->addWidget(m_btnMorePalettes, 0, Qt::AlignLeft);

    rightCol->addStretch();
    mainContent->addLayout(rightCol, 1);
    root->addLayout(mainContent, 1);

    QHBoxLayout *previewRow = new QHBoxLayout();
    previewRow->setSpacing(8);
    previewRow->setContentsMargins(0, 0, 0, 0);
    QLabel *lblPreview = new QLabel(tr("Preview:"), this);
    previewRow->addWidget(lblPreview);

    m_preview = new QLabel(this);
    m_preview->setFixedSize(48, 34);
    m_preview->setStyleSheet("border: 1px solid #555; border-radius: 4px;");
    previewRow->addWidget(m_preview);

    QLabel *lblHex = new QLabel(tr("Hex:"), this);
    previewRow->addWidget(lblHex);

    m_hexEdit = new QLineEdit(this);
    m_hexEdit->setMaxLength(7);
    m_hexEdit->setFixedWidth(100);
    m_hexEdit->setPlaceholderText("#RRGGBB");
    previewRow->addWidget(m_hexEdit);
    previewRow->addStretch();
    root->addLayout(previewRow);

    QHBoxLayout *slotsRow = new QHBoxLayout();
    slotsRow->setSpacing(8);
    slotsRow->setContentsMargins(0, 0, 0, 0);
    QLabel *lblSlots = new QLabel(tr("Slots:"), this);
    slotsRow->addWidget(lblSlots);

    for (int i = 0; i < 2; ++i) {
        m_slotBtn[i] = new ColorSlotButton(i, this);
        m_slotBtn[i]->setColor(m_slots[i]);
        slotsRow->addWidget(m_slotBtn[i]);
        connect(m_slotBtn[i], &ColorSlotButton::slotClicked,
                this, &ColorPickerDialog::onSlotClicked);
    }
    slotsRow->addStretch();

    QPushButton *btnCancel = new QPushButton(tr("Cancelar"), this);
    QPushButton *btnOk = new QPushButton(tr("Aceptar"), this);
    btnOk->setDefault(true);
    btnOk->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: white; border: none;"
        " border-radius: 6px; padding: 6px 20px; font-weight: 600; }"
        "QPushButton:hover { background-color: #1d4ed8; }");
    btnCancel->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #555;"
        " border-radius: 6px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: rgba(255,255,255,0.05); }");

    slotsRow->addWidget(btnCancel);
    slotsRow->addWidget(btnOk);
    root->addLayout(slotsRow);

    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnMorePalettes, &QPushButton::clicked,
            this, &ColorPickerDialog::onMorePalettesClicked);

    connect(m_triangle, &TriangleColorPicker::colorChanged,
            this, &ColorPickerDialog::onTriangleChanged);
    connect(m_stripMSPaint, &PaletteStripWidget::colorClicked,
            this, [this](const QColor &c, bool){ onPaletteColorClicked(c); });
    connect(m_stripClassic, &PaletteStripWidget::colorClicked,
            this, [this](const QColor &c, bool){ onPaletteColorClicked(c); });
    connect(m_stripRetro, &PaletteStripWidget::colorClicked,
            this, [this](const QColor &c, bool){ onPaletteColorClicked(c); });
    connect(m_stripCustom, &PaletteStripWidget::colorClicked,
            this, [this](const QColor &c, bool){ onPaletteColorClicked(c); });
    connect(m_stripCustom, &PaletteStripWidget::paletteChanged,
            this, [this](const QString &n, const QVector<QColor> &cols) {
                m_stripCustom->setPalette(cols, n);
                updateCustomPaletteSection();
            });
    connect(m_hexEdit, &QLineEdit::editingFinished,
            this, &ColorPickerDialog::onHexEdited);
}

inline void ColorPickerDialog::updateCustomPaletteSection() {
    bool has = !m_stripCustom->palette().isEmpty();
    m_customLabel->setVisible(has);
    m_stripCustom->setVisible(has);
}

inline void ColorPickerDialog::onTriangleChanged(const QColor &c) {
    if (m_syncing) return;
    m_slots[m_activeSlot] = c;
    m_slotBtn[m_activeSlot]->setColor(c);
    syncHexFromColor();
    updatePreview();
}

inline void ColorPickerDialog::onPaletteColorClicked(const QColor &c) {
    m_syncing = true;
    m_slots[m_activeSlot] = c;
    m_slotBtn[m_activeSlot]->setColor(c);
    m_triangle->setColor(c);
    syncHexFromColor();
    updatePreview();
    m_syncing = false;
}

inline void ColorPickerDialog::onSlotClicked(int idx) {
    if (idx < 0 || idx > 1) return;
    m_activeSlot = idx;
    updateActiveSlotVisuals();
    syncTriangleFromActiveSlot();
    syncHexFromColor();
    updatePreview();
}

inline void ColorPickerDialog::onHexEdited() {
    QString text = m_hexEdit->text().trimmed();
    if (!text.startsWith('#')) text.prepend('#');
    QColor c(text);
    if (!c.isValid()) { syncHexFromColor(); return; }
    m_syncing = true;
    m_slots[m_activeSlot] = c;
    m_slotBtn[m_activeSlot]->setColor(c);
    m_triangle->setColor(c);
    updatePreview();
    m_syncing = false;
}

inline void ColorPickerDialog::onMorePalettesClicked() {
    if (!m_stripCustom) return;
    QPoint pos = m_btnMorePalettes->mapToGlobal(QPoint(0, m_btnMorePalettes->height()));
    m_stripCustom->showPaletteMenuAt(pos);
    updateCustomPaletteSection();
    updatePreview();
}

inline void ColorPickerDialog::updateActiveSlotVisuals() {
    m_slotBtn[0]->setActive(m_activeSlot == 0);
    m_slotBtn[1]->setActive(m_activeSlot == 1);
}

inline void ColorPickerDialog::syncTriangleFromActiveSlot() {
    m_syncing = true;
    m_triangle->setColor(m_slots[m_activeSlot]);
    m_syncing = false;
}

inline void ColorPickerDialog::syncHexFromColor() {
    const QColor &c = m_slots[m_activeSlot];
    QString hex = c.name(QColor::HexRgb).toUpper();
    m_hexEdit->blockSignals(true);
    m_hexEdit->setText(hex);
    m_hexEdit->blockSignals(false);
}

inline void ColorPickerDialog::updatePreview() {
    const QColor &c = m_slots[m_activeSlot];
    m_preview->setStyleSheet(QString(
        "background-color: %1; border: 1px solid #555; border-radius: 4px;")
        .arg(c.name(QColor::HexRgb)));
}

} // namespace Colrs

#endif // COLRS_H