#include "font_editor.h"
#include "text_preview.h"
#include <QtWidgets>
using U5::require;
namespace Workshop {
int FontDocument::stride(const QString &name) {
    require(QStringList{"IBM.CH", "RUNES.CH", "IBM.HCS", "RUNES.HCS"}.contains(name),
            "Unknown bitmap font");
    return name.endsWith(".HCS") ? 24 : 8;
}
QSize FontDocument::dimensions(const QString &name) {
    return stride(name) == 24 ? QSize(16, 12) : QSize(8, 8);
}
void FontDocument::validate(const QString &name, const QByteArray &bytes) {
    require(bytes.size() == 128 * stride(name), "Font must preserve all 128 glyph slots");
}
QImage FontDocument::glyph(const QString &name, const QByteArray &bytes, int index) {
    validate(name, bytes);
    require(index >= 0 && index < 128, "Invalid glyph slot");
    const auto size = dimensions(name);
    QImage image(size, QImage::Format_RGB32);
    for (int y = 0; y < size.height(); ++y)
        for (int x = 0; x < size.width(); ++x)
            image.setPixel(x, y,
                           (uchar(bytes[index * stride(name) + y * (size.width() / 8) + x / 8]) &
                            (128 >> (x % 8)))
                               ? qRgb(255, 255, 255)
                               : qRgb(0, 0, 0));
    return image;
}
QByteArray FontDocument::replace(const QString &name, const QByteArray &bytes, int index,
                                 const QImage &image) {
    glyph(name, bytes, index);
    auto size = dimensions(name);
    require(image.size() == size, "Pasted glyph dimensions do not match this font");
    auto out = bytes;
    for (int y = 0; y < size.height(); ++y)
        for (int x = 0; x < size.width(); ++x) {
            int offset = index * stride(name) + y * (size.width() / 8) + x / 8;
            int mask = 128 >> (x % 8);
            out[offset] =
                char((uchar(out[offset]) & ~mask) | (qGray(image.pixel(x, y)) >= 128 ? mask : 0));
        }
    return out;
}
QString FontDocument::label(int i) {
    // INTRO_08be / sign display use these exact glyphs, not Unicode box drawing.
    if (i >= 123 && i <= 127)
        return QStringList{"Border top-left", "Border top-right", "Border bottom-left",
                           "Border bottom-right", "Border stroke"}[i - 123];
    if (i == 32)
        return "Space";
    if (i >= 33 && i <= 122)
        return QString(QChar(i));
    return "Decorative glyph";
}
QImage FontDocument::sample(const QString &name, const QByteArray &bytes, const QString &text) {
    validate(name, bytes);
    auto size = dimensions(name);
    auto lines = text.split('\n');
    int columns = 1;
    for (auto line : lines)
        columns = qMax(columns, int(line.size()));
    require(columns <= 128 && lines.size() <= 64, "Sample is too large");
    QImage image(columns * size.width(), lines.size() * size.height(), QImage::Format_RGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    for (int row = 0; row < lines.size(); ++row)
        for (int col = 0; col < lines[row].size(); ++col) {
            int slot = lines[row][col].unicode();
            if (slot < 128)
                painter.drawImage(col * size.width(), row * size.height(),
                                  glyph(name, bytes, slot));
        }
    return image;
}
class GlyphCanvas : public QWidget {
  public:
    QImage image, before;
    int zoom = 24;
    bool erase = false, stroke = false;
    std::function<void(const QImage &)> commit;
    GlyphCanvas() {
        setObjectName("glyphCanvas");
        setToolTip("Left-drag draws pixels; right-drag erases. Each stroke is undoable.");
    }
    void resizeImage() {
        setFixedSize(image.width() * zoom + 1, image.height() * zoom + 1);
        update();
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);
        p.drawImage(QRect(0, 0, image.width() * zoom, image.height() * zoom), image);
        p.setPen(QColor(80, 80, 80));
        for (int x = 0; x <= image.width(); ++x)
            p.drawLine(x * zoom, 0, x * zoom, height());
        for (int y = 0; y <= image.height(); ++y)
            p.drawLine(0, y * zoom, width(), y * zoom);
    }
    void point(QMouseEvent *e) {
        int x = int(e->position().x()) / zoom, y = int(e->position().y()) / zoom;
        if (e->position().x() >= 0 && e->position().y() >= 0 && image.rect().contains(x, y)) {
            image.setPixel(x, y, erase ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
            update();
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() != Qt::LeftButton && e->button() != Qt::RightButton)
            return;
        before = image;
        stroke = true;
        erase = e->button() == Qt::RightButton || property("erase").toBool();
        point(e);
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (stroke)
            point(e);
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (!stroke)
            return;
        point(e);
        stroke = false;
        if (image != before)
            commit(image);
    }
};
QWidget *fontEditor(Project *project, const QString &name, TextCommit commit,
                    QMap<QString, int> *state, std::function<bool()> *flush) {
    FontDocument::validate(name, project->data(name));
    auto panel = new QWidget;
    panel->setObjectName("fontEditor");
    auto layout = new QVBoxLayout(panel);
    auto status = new QLabel(
        name.endsWith(".HCS")
            ? "Legacy Hercules font — not used by the SDL color renderer · 128 glyphs, 16×12 pixels"
            : "Game font · 128 glyphs, 8×8 pixels · Blank slots are intentional and preserved");
    status->setWordWrap(true);
    layout->addWidget(status);
    auto split = new QSplitter;
    auto left = new QWidget;
    auto ll = new QVBoxLayout(left);
    auto search = new QLineEdit;
    search->setObjectName("glyphSearch");
    search->setPlaceholderText("Find a character or border…");
    search->setToolTip("Search character labels or glyph numbers; all blank and decorative slots "
                       "remain editable.");
    auto gallery = new QListWidget;
    gallery->setObjectName("glyphGallery");
    gallery->setViewMode(QListView::IconMode);
    gallery->setResizeMode(QListView::Adjust);
    gallery->setIconSize(QSize(48, 48));
    gallery->setGridSize(QSize(64, 72));
    gallery->setMinimumWidth(220);
    ll->addWidget(search);
    ll->addWidget(gallery);
    auto right = new QWidget;
    auto rl = new QVBoxLayout(right);
    auto actions = new QHBoxLayout;
    auto tool = new QComboBox;
    tool->addItems({"Pencil", "Erase"});
    tool->setToolTip("Choose pixel drawing or erasing. Right mouse button always erases.");
    actions->addWidget(tool);
    auto zoom = new QSpinBox;
    zoom->setRange(8, 40);
    zoom->setValue(state->value(name + "/zoom", 24));
    zoom->setPrefix("Zoom ");
    zoom->setToolTip("Display pixels larger without changing glyph dimensions.");
    actions->addWidget(zoom);
    auto copy = new QPushButton("Copy");
    copy->setObjectName("copyGlyph");
    copy->setToolTip("Copy this glyph bitmap. Paste accepts the same font dimensions.");
    actions->addWidget(copy);
    auto paste = new QPushButton("Paste");
    paste->setObjectName("pasteGlyph");
    paste->setToolTip(
        "Paste a copied glyph of matching dimensions; undo restores its previous pixels.");
    actions->addWidget(paste);
    auto clear = new QPushButton("Clear");
    clear->setToolTip("Erase this glyph, preserving its slot. Undo restores it.");
    actions->addWidget(clear);
    rl->addLayout(actions);
    auto canvas = new GlyphCanvas;
    auto scroll = new QScrollArea;
    scroll->setWidget(canvas);
    scroll->setMinimumHeight(250);
    rl->addWidget(scroll, 1);
    auto sample = new QLineEdit;
    sample->setObjectName("fontSample");
    sample->setMaxLength(100);
    sample->setText(project->dialogueNames.value("font/sample", "Welcome to Britannia!"));
    sample->setToolTip(
        "Sample text is a project-only annotation; it is never exported into game resources.");
    rl->addWidget(sample);
    auto mode = new QComboBox;
    mode->addItems({"This font", "Normal text", "Runic text", "Sign / game border"});
    mode->setToolTip("Preview project font resources and the actual corner/stroke glyphs used by "
                     "signs and title dialogs.");
    rl->addWidget(mode);
    auto preview = new QLabel;
    preview->setObjectName("fontPreview");
    preview->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto ps = new QScrollArea;
    ps->setWidget(preview);
    ps->setMinimumHeight(100);
    rl->addWidget(ps);
    auto updatePreview = [=] {
        QString font = name;
        if (mode->currentIndex() == 1)
            font = name.endsWith("HCS") ? "IBM.HCS" : "IBM.CH";
        if (mode->currentIndex() == 2 || mode->currentIndex() == 3)
            font = name.endsWith("HCS") ? "RUNES.HCS" : "RUNES.CH";
        if (!project->resources.contains(font)) {
            preview->setText("Companion font is absent from this project.");
            return;
        }
        QString text = sample->text();
        if (mode->currentIndex() == 3) {
            int n = qMax(1, int(text.size()));
            text = QChar(123) + QString(n, QChar(127)) + QChar(124) + "\n " + text + " \n" +
                   QChar(125) + QString(n, QChar(127)) + QChar(126);
        }
        auto image = FontDocument::sample(font, project->data(font), text);
        preview->setPixmap(QPixmap::fromImage(
            image.scaled(image.size() * 2, Qt::IgnoreAspectRatio, Qt::FastTransformation)));
        preview->adjustSize();
    };
    auto select = [=](int index) {
        if (index < 0)
            return;
        (*state)[name + "/glyph"] = index;
        canvas->image = FontDocument::glyph(name, project->data(name), index);
        canvas->resizeImage();
    };
    auto apply = [=](const QImage &image) {
        int index = gallery->currentRow();
        if (index < 0)
            return;
        auto bytes = FontDocument::replace(name, project->data(name), index, image);
        if (commit({{name, bytes}}, "Edit font glyph")) {
            select(index);
            gallery->item(index)->setIcon(QPixmap::fromImage(
                canvas->image.scaled(48, 48, Qt::KeepAspectRatio, Qt::FastTransformation)));
            updatePreview();
        } else
            select(index);
    };
    canvas->commit = apply;
    canvas->zoom = zoom->value();
    for (int i = 0; i < 128; ++i) {
        auto item = new QListWidgetItem(
            QIcon(QPixmap::fromImage(
                FontDocument::glyph(name, project->data(name), i)
                    .scaled(48, 48, Qt::KeepAspectRatio, Qt::FastTransformation))),
            FontDocument::label(i), gallery);
        item->setToolTip(QString("%1 · Glyph %2 (0x%3)")
                             .arg(FontDocument::label(i))
                             .arg(i)
                             .arg(i, 2, 16, QChar('0')));
    }
    QObject::connect(gallery, &QListWidget::currentRowChanged, panel, select);
    QObject::connect(search, &QLineEdit::textChanged, panel, [=](const QString &s) {
        for (int i = 0; i < 128; ++i)
            gallery->item(i)->setHidden(
                !gallery->item(i)->toolTip().contains(s, Qt::CaseInsensitive));
    });
    QObject::connect(tool, qOverload<int>(&QComboBox::currentIndexChanged), panel,
                     [=](int i) { canvas->setProperty("erase", i == 1); });
    QObject::connect(zoom, qOverload<int>(&QSpinBox::valueChanged), panel, [=](int i) {
        (*state)[name + "/zoom"] = i;
        canvas->zoom = i;
        canvas->resizeImage();
    });
    QObject::connect(copy, &QPushButton::clicked, panel,
                     [=] { QApplication::clipboard()->setImage(canvas->image); });
    QObject::connect(paste, &QPushButton::clicked, panel, [=] {
        auto image = QApplication::clipboard()->image();
        if (image.size() != canvas->image.size()) {
            QMessageBox::warning(panel, "Cannot paste glyph",
                                 "Copy a glyph with matching pixel dimensions.");
            return;
        }
        apply(image);
    });
    QObject::connect(clear, &QPushButton::clicked, panel, [=] {
        auto image = canvas->image;
        image.fill(Qt::black);
        apply(image);
    });
    QObject::connect(sample, &QLineEdit::textChanged, panel, [=] { updatePreview(); });
    QObject::connect(sample, &QLineEdit::editingFinished, panel,
                     [=] { project->dialogueNames["font/sample"] = sample->text(); });
    QObject::connect(mode, qOverload<int>(&QComboBox::currentIndexChanged), panel,
                     [=](int) { updatePreview(); });
    split->addWidget(left);
    split->addWidget(right);
    split->setStretchFactor(1, 1);
    layout->addWidget(split, 1);
    gallery->setCurrentRow(qBound(0, state->value(name + "/glyph", 65), 127));
    updatePreview();
    *flush = [=] {
        canvas->stroke = false;
        project->dialogueNames["font/sample"] = sample->text();
        return true;
    };
    return panel;
}
} // namespace Workshop
