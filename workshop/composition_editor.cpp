#include "content_navigation.h"
#include "cutscene_composition.h"
#include "mod/package.h"
#include <QtWidgets>
#include <memory>
using U5::require;
namespace Workshop {
static void resizeTextBox(CutsceneTextBox &box, const QSize &size) {
    if (box.flow) {
        const int delta = size.width() - box.bounds.width();
        for (int i = 0; i < 2; ++i)
            box.layout.right[i] =
                qBound(qMin(320, box.layout.left[i] + 1), box.layout.right[i] + delta, 320);
    }
    box.bounds.setSize(size);
}
// Values shown in the layout panel are offsets from the text box, except the
// first-line indent (relative to that line's left margin) and right insets.
static U5CutsceneLayout textLayout(const CutsceneTextBox &box) {
    U5CompositionText t = {};
    t.width = box.bounds.width();
    t.flow = box.flow;
    t.layout = box.layout;
    return U5_CompositionTextLayout(&t);
}
static int firstMargin(const U5CutsceneLayout &l) { return l.y > l.top && l.y < l.bottom ? 1 : 0; }
static int layoutValue(const CutsceneTextBox &box, int field) {
    auto l = textLayout(box);
    switch (field) {
    case 0:
        return l.x - l.left[firstMargin(l)];
    case 1:
        return l.left[0];
    case 2:
        return box.bounds.width() - l.right[0];
    case 3:
        return l.y;
    case 4:
        return l.top;
    case 5:
        return l.bottom;
    case 6:
        return l.left[1];
    default:
        return box.bounds.width() - l.right[1];
    }
}
static void setLayoutValue(CutsceneTextBox &box, int field, int value) {
    auto l = textLayout(box);
    const int indent = l.x - l.left[firstMargin(l)];
    switch (field) {
    case 0:
        l.x = qBound(-320, l.left[firstMargin(l)] + value, 320);
        break;
    case 1:
        l.left[0] = qBound(-320, value, l.right[0] - 1);
        break;
    case 2:
        l.right[0] = qBound(l.left[0] + 1, box.bounds.width() - value, 320);
        break;
    case 3:
        l.y = qBound(-320, value, 320);
        break;
    case 4:
        l.top = qBound(-320, value, qMax(-320, l.bottom - 1));
        break;
    case 5:
        l.bottom = qBound(qMin(320, l.top + 1), value, 320);
        break;
    case 6:
        l.left[1] = qBound(-320, value, l.right[1] - 1);
        break;
    case 7:
        l.right[1] = qBound(l.left[1] + 1, box.bounds.width() - value, 320);
        break;
    }
    if (field != 0)
        l.x = qBound(-320, l.left[firstMargin(l)] + indent, 320);
    box.layout = l;
    box.flow = true;
}
class CompositionCanvas : public QWidget {
    QPoint press;
    QRect original;
    bool dragging = false, moved = false;
    int corner = -1, guide = -1;
    CutsceneComposition snapshot;
    QRectF display() const {
        return QRectF((width() - 640) / 2.0, (height() - 400) / 2.0, 640, 400);
    }
    QPoint point(const QPointF &p) const {
        return {int((p.x() - display().x()) / 2), int((p.y() - display().y()) / 2)};
    }

    QVector<QPoint> corners() const {
        auto r = bounds();
        return {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
    }
    QVector<QPoint> guides() const {
        if (!isText())
            return {};
        const auto &box = page->texts[selected - page->images.size()];
        auto l = textLayout(box);
        // Indent, ordinary margins, first-line height, then wrap-band boundaries.
        int mid = qBound(0, (l.top + l.bottom) / 2, box.bounds.height() - 1);
        QVector<QPoint> points = {{l.x, l.y - 4},
                                  {l.left[0], 8},
                                  {l.right[0], 8},
                                  {box.bounds.width() / 2, l.y},
                                  {box.bounds.width() / 2, l.top},
                                  {box.bounds.width() / 2, l.bottom},
                                  {l.left[1], mid},
                                  {l.right[1], mid}};
        for (auto &point : points) {
            point += box.bounds.topLeft();
            // Keep handles reachable when an inherited margin lies beyond the
            // box or the first line starts at the very edge of the page.
            point.setX(qBound(3, point.x(), 316));
            point.setY(qBound(3, point.y(), 196));
        }
        points[0].setY(qMax(5, points[0].y()));
        return points;
    }

    int cornerAt(const QPoint &at) const {
        if (!isText())
            return -1;
        const auto grips = corners();
        for (int i = 0; i < grips.size(); ++i)
            if (QRect(grips[i] - QPoint(3, 3), QSize(7, 7)).contains(at))
                return i;
        return -1;
    }
    int guideAt(const QPoint &at) const {
        if (!isText())
            return -1;
        const auto handles = guides();
        const auto l = textLayout(page->texts[selected - page->images.size()]);
        for (int i = 0; i < handles.size(); ++i) {
            if (i >= 4 && l.top == l.bottom)
                continue;
            if (QRect(handles[i] - QPoint(2, 2), QSize(5, 5)).contains(at))
                return i;
        }
        return -1;
    }

  public:
    CutsceneComposition *page = nullptr;
    int selected = -1;
    QImage image;
    QSet<int> overflow;
    std::function<void()> selection, begin, cancel, remove;
    std::function<void(bool)> changed;
    CompositionCanvas() {
        setMinimumSize(640, 400);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setObjectName("compositionCanvas");
    }
    bool isText() const {
        return page && selected >= page->images.size() &&
               selected < page->images.size() + page->texts.size();
    }
    QRect bounds() const {
        if (!page || selected < 0)
            return {};
        if (isText())
            return page->texts[selected - page->images.size()].bounds;
        if (selected < page->images.size())
            return {page->images[selected].position, page->images[selected].image.size()};
        return {};
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(15, 18, 24));
        p.translate(display().topLeft());
        p.scale(2, 2);
        p.drawImage(0, 0, image);
        p.setClipRect(0, 0, 320, 200);
        if (page)
            for (int i : overflow) {
                p.setPen(QPen(QColor(255, 85, 85), 0));
                p.drawRect(page->texts[i].bounds.adjusted(0, 0, -1, -1));
            }
        if (selected >= 0) {
            p.setPen(QPen(QColor(90, 210, 255), 0));
            p.drawRect(bounds().adjusted(0, 0, -1, -1));
            if (isText()) {
                const auto &box = page->texts[selected - page->images.size()];
                auto l = textLayout(box);
                auto handles = guides();
                p.save();
                p.setClipRect(box.bounds);
                p.setPen(QPen(QColor(110, 225, 160), 0, Qt::DashLine));
                for (int x : {l.left[0], l.right[0]})
                    p.drawLine(box.bounds.x() + x, box.bounds.y(), box.bounds.x() + x,
                               box.bounds.bottom());
                if (l.top < l.bottom) {
                    p.setPen(QPen(QColor(255, 190, 85), 0, Qt::DashLine));
                    QRect band(box.bounds.x() + l.left[1], box.bounds.y() + l.top,
                               l.right[1] - l.left[1], l.bottom - l.top);
                    p.drawRect(band);
                }
                p.restore();
                for (int i = 0; i < handles.size(); ++i) {
                    if (i >= 4 && l.top == l.bottom)
                        continue;
                    p.setPen(QPen(i >= 4 ? QColor(255, 190, 85) : QColor(110, 225, 160), 0));
                    p.setBrush(i >= 4 ? QColor(255, 190, 85) : QColor(110, 225, 160));
                    const auto h = handles[i];
                    if (i == 0)
                        p.drawPolygon(
                            QPolygon({h + QPoint(-3, -2), h + QPoint(3, -2), h + QPoint(0, 2)}));
                    else
                        p.drawEllipse(h, 2, 2);
                }
                for (const auto &h : corners())
                    p.fillRect(QRect(h - QPoint(2, 2), QSize(5, 5)), QColor(90, 210, 255));
            }
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (!page || e->button() != Qt::LeftButton || !display().contains(e->position()))
            return;
        QPoint at = point(e->position());
        setFocus();
        corner = cornerAt(at);
        guide = corner < 0 ? guideAt(at) : -1;
        if (corner < 0 && guide < 0) {
            selected = -1;
            for (int i = page->texts.size() - 1; i >= 0; i--)
                if (page->texts[i].bounds.contains(at)) {
                    selected = page->images.size() + i;
                    break;
                }
            if (selected < 0)
                for (int i = page->images.size() - 1; i >= 0; i--)
                    if (QRect(page->images[i].position, page->images[i].image.size())
                            .contains(at)) {
                        selected = i;
                        break;
                    }
        }
        if (selection)
            selection();
        update();
        if (selected < 0)
            return;
        original = bounds();
        press = at;
        snapshot = *page;
        dragging = true;
        moved = false;
        if (begin)
            begin();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (!page)
            return;
        if (!dragging) {
            const auto at = point(e->position());
            int grip = cornerAt(at), handle = guideAt(at);
            setCursor(
                grip >= 0     ? (grip == 0 || grip == 3 ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor)
                : handle >= 0 ? (handle >= 3 && handle <= 5 ? Qt::SizeVerCursor : Qt::SizeHorCursor)
                              : Qt::ArrowCursor);
            return;
        }
        QPoint delta = point(e->position()) - press;
        if (guide >= 0) {
            if (delta.isNull() && !moved)
                return;
            auto &box = page->texts[selected - page->images.size()];
            box = snapshot.texts[selected - page->images.size()];
            const bool vertical = guide == 3 || guide == 4 || guide == 5;
            int amount = vertical ? delta.y() : delta.x();
            if (guide == 2 || guide == 7)
                amount = -amount;
            setLayoutValue(box, guide, layoutValue(box, guide) + amount);
        } else if (corner >= 0) {
            auto &box = page->texts[selected - page->images.size()];
            QRect next = original;
            if (corner % 2 == 0)
                next.setLeft(qBound(0, original.left() + delta.x(), original.right()));
            else
                next.setRight(qBound(original.left(), original.right() + delta.x(), 319));
            if (corner < 2)
                next.setTop(qBound(0, original.top() + delta.y(), original.bottom()));
            else
                next.setBottom(qBound(original.top(), original.bottom() + delta.y(), 199));
            if (next == box.bounds)
                return;
            box = snapshot.texts[selected - page->images.size()];
            resizeTextBox(box, next.size());
            box.bounds.moveTopLeft(next.topLeft());
        } else {
            QPoint to = original.topLeft() + delta;
            to.setX(qBound(0, to.x(), isText() ? 320 - original.width() : 319));
            to.setY(qBound(0, to.y(), isText() ? 200 - original.height() : 199));
            if (to == bounds().topLeft())
                return;
            if (isText())
                page->texts[selected - page->images.size()].bounds.moveTopLeft(to);
            else
                page->images[selected].position = to;
        }
        moved = true;
        if (changed)
            changed(false);
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (!dragging || e->button() != Qt::LeftButton)
            return;
        mouseMoveEvent(e);
        dragging = false;
        if (moved && changed)
            changed(true);
    }
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape && dragging) {
            *page = snapshot;
            dragging = false;
            if (cancel)
                cancel();
            return;
        }
        if ((e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) && selected >= 0) {
            if (remove)
                remove();
            return;
        }
        QWidget::keyPressEvent(e);
    }
};
CutsceneImage importCutscenePng(const QString &path) {
    require(QFileInfo(path).size() <= MOD_MAX_RESOURCE, "PNG file exceeds 8 MiB");
    QImageReader reader(path, "png");
    require(reader.canRead() && reader.size().width() > 0 && reader.size().height() > 0 &&
                reader.size().width() <= 1024 && reader.size().height() <= 1024,
            "Choose a PNG no larger than 1024 × 1024 pixels");
    auto image = reader.read();
    require(!image.isNull(), "Cannot read PNG: " + reader.errorString());
    return {QFileInfo(path).fileName(), image, QPoint(8, 8)};
}
static bool chooseGameImage(QWidget *parent, const Project &project, CutsceneImage *chosen) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Add game artwork");
    dialog.resize(620, 480);
    auto layout = new QVBoxLayout(&dialog);
    auto row = new QHBoxLayout;
    auto bank = new QComboBox;
    bank->setObjectName("gameArtworkBank");
    for (auto it = project.resources.begin(); it != project.resources.end(); ++it)
        if (it.key().endsWith(".16") && it.key() != "TILES.16")
            bank->addItem(artworkTitle(it.key()), it.key());
    auto version = new QComboBox;
    version->addItems({"Current mod artwork", "Original game artwork"});
    version->setObjectName("gameArtworkVersion");
    row->addWidget(bank, 1);
    row->addWidget(version);
    layout->addLayout(row);
    auto list = new QListWidget;
    list->setObjectName("gameArtworkImages");
    list->setViewMode(QListView::IconMode);
    list->setIconSize({96, 96});
    list->setGridSize({120, 125});
    list->setResizeMode(QListView::Adjust);
    layout->addWidget(list, 1);
    auto message = new QLabel;
    message->setWordWrap(true);
    layout->addWidget(message);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    auto images = std::make_shared<QVector<QImage>>();
    auto load = [=] {
        list->clear();
        images->clear();
        message->clear();
        try {
            auto resource = bank->currentData().toString();
            *images = U5::readGraphics(resource, version->currentIndex()
                                                     ? project.resources.value(resource).original
                                                     : project.data(resource))
                          .images;
            for (int i = 0; i < images->size(); i++)
                if (!(*images)[i].isNull()) {
                    auto item = new QListWidgetItem(QIcon(QPixmap::fromImage((*images)[i])),
                                                    QString("Image %1").arg(i + 1), list);
                    item->setData(Qt::UserRole, i);
                    item->setToolTip(QString("%1 × %2 pixels")
                                         .arg((*images)[i].width())
                                         .arg((*images)[i].height()));
                }
        } catch (const std::exception &e) {
            message->setText(QString::fromUtf8(e.what()));
        }
        if (list->count())
            list->setCurrentRow(0);
        buttons->button(QDialogButtonBox::Ok)->setEnabled(list->count());
    };
    QObject::connect(bank, &QComboBox::currentIndexChanged, &dialog, [=] { load(); });
    QObject::connect(version, &QComboBox::currentIndexChanged, &dialog, [=] { load(); });
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(list, &QListWidget::itemDoubleClicked, &dialog, [&] { dialog.accept(); });
    load();
    if (dialog.exec() != QDialog::Accepted || !list->currentItem())
        return false;
    int index = list->currentItem()->data(Qt::UserRole).toInt();
    *chosen = {QString("%1 · Image %2").arg(bank->currentText()).arg(index + 1), (*images)[index],
               QPoint(8, 8)};
    return true;
}
QWidget *compositionEditor(Project *project, const QString &resource, TextCommit commit,
                           QMap<QString, int> *state, std::function<bool()> *flush) {
    int scene = resource == "END.DAT";
    auto root = new QWidget;
    root->setObjectName("cutsceneEditor");
    auto outer = new QHBoxLayout(root);
    auto left = new QVBoxLayout;
    auto list = new QListWidget;
    list->setObjectName("cutscenePages");
    list->setMaximumWidth(180);
    left->addWidget(list, 1);
    QStringList names = {"Add page", "Duplicate page", "Remove page", "Move up", "Move down"};
    QVector<QPushButton *> pageButtons;
    auto pageActions = new QGridLayout;
    for (int i = 0; i < names.size(); i++) {
        auto b = new QPushButton(names[i]);
        pageButtons << b;
        pageActions->addWidget(b, i / 2, i % 2);
    }
    pageButtons[0]->setObjectName("addCutscenePage");
    pageButtons[1]->setObjectName("duplicateCutscenePage");
    pageButtons[2]->setObjectName("removeCutscenePage");
    left->addLayout(pageActions);
    outer->addLayout(left);
    auto center = new QVBoxLayout;
    auto tools = new QHBoxLayout;
    auto addImage = new QToolButton;
    addImage->setText("Add image…");
    addImage->setPopupMode(QToolButton::InstantPopup);
    auto menu = new QMenu(addImage);
    auto png = menu->addAction("Import PNG…");
    png->setObjectName("importCutscenePng");
    auto game = menu->addAction("Game artwork…");
    game->setObjectName("importGameCutsceneArt");
    addImage->setMenu(menu);
    tools->addWidget(addImage);
    auto addText = new QPushButton("Add text box");
    addText->setObjectName("addCutsceneText");
    tools->addWidget(addText);
    auto forward = new QPushButton("Bring forward");
    forward->setObjectName("bringCutsceneImageForward");
    auto backward = new QPushButton("Send backward");
    backward->setObjectName("sendCutsceneImageBackward");
    tools->addWidget(forward);
    tools->addWidget(backward);
    auto remove = new QPushButton("Delete");
    remove->setObjectName("deleteCutsceneElement");
    tools->addWidget(remove);
    tools->addStretch();
    center->addLayout(tools);
    auto canvas = new CompositionCanvas;
    center->addWidget(canvas, 1);
    auto status = new QLabel;
    status->setWordWrap(true);
    center->addWidget(status);
    outer->addLayout(center, 1);
    auto properties = new QWidget;
    properties->setFixedWidth(
        qMax(240, properties->fontMetrics().horizontalAdvance(QString(28, '0')) + 32));
    auto form = new QVBoxLayout(properties);
    auto title = new QLabel;
    title->setWordWrap(true);
    form->addWidget(title);
    auto choose = new QComboBox;
    choose->setObjectName("cutsceneElement");
    form->addWidget(choose);
    auto geometry = new QFormLayout;
    QVector<QSpinBox *> fields;
    QStringList fieldNames = {"X", "Y", "Width", "Height"};
    for (int i = 0; i < 4; i++) {
        auto spin = new QSpinBox;
        spin->setObjectName("composition" + fieldNames[i]);
        spin->setRange(i < 2 ? 0 : 1, i % 2 ? 200 : 320);
        fields << spin;
        geometry->addRow(fieldNames[i] + ":", spin);
    }
    form->addLayout(geometry);
    auto text = new QPlainTextEdit;
    text->setObjectName("compositionText");
    text->setMinimumHeight(100);
    form->addWidget(text, 1);
    auto layoutGroup = new QGroupBox("Text layout");
    layoutGroup->setObjectName("compositionTextLayout");
    auto layoutForm = new QFormLayout(layoutGroup);
    QVector<QSpinBox *> layoutFields;
    const QStringList layoutLabels = {"First-line indent", "Left margin",      "Right margin",
                                      "Text top",          "Band top",         "Band bottom",
                                      "Band left margin",  "Band right margin"};
    const QStringList layoutNames = {"Indent",  "LeftMargin", "RightMargin", "TextTop",
                                     "BandTop", "BandBottom", "BandLeft",    "BandRight"};
    auto wrap = new QCheckBox("Wrap around artwork");
    wrap->setObjectName("compositionWrap");
    for (int i = 0; i < 8; ++i) {
        if (i == 4)
            layoutForm->addRow(wrap);
        auto spin = new QSpinBox;
        spin->setObjectName("composition" + layoutNames[i]);
        spin->setRange(-640, 640);
        spin->setSuffix(" px");
        spin->setToolTip(i == 0 ? "Indent the first line from its left margin."
                         : i == 2 || i == 7
                             ? "Distance inward from the right edge of the text box."
                             : "Distance from the text box's left or top edge, in game pixels.");
        layoutFields << spin;
        layoutForm->addRow(layoutLabels[i] + ":", spin);
    }
    wrap->setToolTip("Use different left and right margins between the band's top and bottom.");
    auto resetLayout = new QPushButton("Reset layout");
    resetLayout->setObjectName("resetCompositionLayout");
    resetLayout->setToolTip("Remove indentation and artwork wrapping, keeping the box and text.");
    layoutForm->addRow(resetLayout);
    form->addWidget(layoutGroup);
    form->addStretch();
    auto propertiesScroll = new QScrollArea;
    propertiesScroll->setWidgetResizable(true);
    propertiesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    propertiesScroll->setFrameShape(QFrame::NoFrame);
    propertiesScroll->setWidget(properties);
    propertiesScroll->setFixedWidth(properties->width() +
                                    propertiesScroll->verticalScrollBar()->sizeHint().width());
    outer->addWidget(propertiesScroll);
    addImage->setToolTip("Add a PNG or an image from the original or modded game artwork.");
    addText->setToolTip("Add a resizable Ultima 5 text box. Text always appears above images.");
    remove->setToolTip("Delete the selected image or text box. Undo restores it.");
    forward->setToolTip("Move the selected image in front of another image.");
    backward->setToolTip("Move the selected image behind another image.");
    for (auto field : fields)
        field->setToolTip("Game pixels on the 320 × 200 page.");
    auto pages = std::make_shared<QVector<CutsceneComposition>>(compositionPages(*project, scene));
    auto selected =
        std::make_shared<int>(qBound(0, state->value(resource + "/page"), int(pages->size()) - 1));
    auto dirty = std::make_shared<bool>(false), busy = std::make_shared<bool>(false),
         beforeDrag = std::make_shared<bool>(false);
    auto timer = new QTimer(root);
    timer->setSingleShot(true);
    auto redraw = [=] {
        try {
            QStringList issues;
            canvas->image = previewComposition(*project, (*pages)[*selected], &issues);
            canvas->overflow.clear();
            for (int i = 0; i < (*pages)[*selected].texts.size(); i++)
                if (issues.contains(QString("Text box %1: text does not fit").arg(i + 1)))
                    canvas->overflow.insert(i);
            status->setText(issues.join("\n"));
        } catch (const std::exception &e) {
            status->setText(QString::fromUtf8(e.what()));
        }
        canvas->update();
    };
    auto save = [=]() -> bool {
        timer->stop();
        if (!*dirty)
            return true;
        try {
            auto bytes = changeComposition(*project, scene, *pages);
            if (!commit({{"CUTSCENE.OVR", bytes}}, "Edit cutscene page"))
                return false;
            *dirty = false;
            return true;
        } catch (const std::exception &e) {
            status->setText(QString::fromUtf8(e.what()));
            return false;
        }
    };
    auto refresh = [=] {
        *busy = true;
        canvas->page = &(*pages)[*selected];
        auto &page = *canvas->page;
        choose->clear();
        for (auto &image : page.images)
            choose->addItem("Image: " + image.name);
        for (int i = 0; i < page.texts.size(); i++)
            choose->addItem(QString("Text box %1").arg(i + 1));
        if (canvas->selected >= choose->count())
            canvas->selected = -1;
        choose->setCurrentIndex(canvas->selected);
        bool valid = canvas->selected >= 0;
        bool box = valid && canvas->isText();
        QRect rect = canvas->bounds();
        title->setText(!valid ? QString() : box ? "Text box" : page.images[canvas->selected].name);
        for (auto field : fields)
            field->setEnabled(valid);
        fields[0]->setMaximum(box ? 320 - rect.width() : 319);
        fields[1]->setMaximum(box ? 200 - rect.height() : 199);
        fields[2]->setMaximum(box ? 320 - rect.x() : 1024);
        fields[3]->setMaximum(box ? 200 - rect.y() : 1024);
        fields[0]->setValue(rect.x());
        fields[1]->setValue(rect.y());
        fields[2]->setValue(rect.width());
        fields[3]->setValue(rect.height());
        fields[2]->setEnabled(box);
        fields[3]->setEnabled(box);
        text->setVisible(box);
        if (box && text->toPlainText() != page.texts[canvas->selected - page.images.size()].text)
            text->setPlainText(page.texts[canvas->selected - page.images.size()].text);
        layoutGroup->setVisible(box);
        if (box) {
            const auto &t = page.texts[canvas->selected - page.images.size()];
            auto l = textLayout(t);
            wrap->setChecked(l.top < l.bottom);
            for (int i = 0; i < layoutFields.size(); ++i) {
                layoutFields[i]->setValue(layoutValue(t, i));
                layoutFields[i]->setEnabled(i < 4 || wrap->isChecked());
            }
        }
        remove->setEnabled(valid);
        forward->setEnabled(valid && !box && canvas->selected + 1 < page.images.size());
        backward->setEnabled(valid && !box && canvas->selected > 0);
        (*state)[resource + "/object"] = canvas->selected;
        *busy = false;
        redraw();
    };
    auto load = [=] {
        *busy = true;
        list->clear();
        for (int i = 0; i < pages->size(); i++)
            list->addItem(QString("Page %1").arg(i + 1));
        list->setCurrentRow(*selected);
        (*state)[resource + "/page"] = *selected;
        pageButtons[0]->setEnabled(pages->size() < 128);
        pageButtons[1]->setEnabled(pages->size() < 128);
        pageButtons[2]->setEnabled(pages->size() > 1);
        pageButtons[3]->setEnabled(*selected > 0);
        pageButtons[4]->setEnabled(*selected + 1 < pages->size());
        *busy = false;
        refresh();
    };
    *flush = save;
    canvas->selection = [=] {
        save();
        refresh();
    };
    canvas->begin = [=] { *beforeDrag = *dirty; };
    canvas->cancel = [=] {
        *dirty = *beforeDrag;
        refresh();
    };
    canvas->changed = [=](bool finished) {
        *dirty = true;
        refresh();
        if (finished)
            save();
    };
    auto deleteElement = [=] {
        if (canvas->selected < 0)
            return;
        auto &page = (*pages)[*selected];
        if (canvas->isText())
            page.texts.removeAt(canvas->selected - page.images.size());
        else
            page.images.removeAt(canvas->selected);
        canvas->selected = -1;
        *dirty = true;
        refresh();
        save();
    };
    canvas->remove = deleteElement;
    QObject::connect(remove, &QPushButton::clicked, root, deleteElement);
    QObject::connect(choose, &QComboBox::currentIndexChanged, root, [=](int index) {
        if (*busy)
            return;
        save();
        canvas->selected = index;
        refresh();
    });
    QObject::connect(addText, &QPushButton::clicked, root, [=] {
        auto &page = (*pages)[*selected];
        if (page.texts.size() >= 32)
            return;
        page.texts.append({QRect(16, 16, 288, 72), "", false, {}});
        canvas->selected = page.images.size() + page.texts.size() - 1;
        *dirty = true;
        refresh();
        save();
        text->setFocus();
    });
    auto insertImage = [=](CutsceneImage image) {
        auto &page = (*pages)[*selected];
        require(page.images.size() < 64, "A page supports up to 64 images");
        page.images.append(image);
        canvas->selected = page.images.size() - 1;
        *dirty = true;
        refresh();
        save();
    };
    QObject::connect(png, &QAction::triggered, root, [=] {
        QString path =
            QFileDialog::getOpenFileName(root, "Import PNG", {}, "PNG images (*.png *.PNG)");
        if (path.isEmpty())
            return;
        try {
            insertImage(importCutscenePng(path));
        } catch (const std::exception &e) {
            QMessageBox::warning(root, "Cannot import image", QString::fromUtf8(e.what()));
        }
    });
    QObject::connect(game, &QAction::triggered, root, [=] {
        try {
            CutsceneImage image;
            if (chooseGameImage(root, *project, &image))
                insertImage(image);
        } catch (const std::exception &e) {
            QMessageBox::warning(root, "Cannot import image", QString::fromUtf8(e.what()));
        }
    });
    for (auto pair : {qMakePair(forward, 1), qMakePair(backward, -1)})
        QObject::connect(pair.first, &QPushButton::clicked, root, [=] {
            auto &page = (*pages)[*selected];
            if (canvas->isText() || canvas->selected < 0)
                return;
            int next = canvas->selected + pair.second;
            if (next < 0 || next >= page.images.size())
                return;
            page.images.swapItemsAt(next, canvas->selected);
            canvas->selected = next;
            *dirty = true;
            refresh();
            save();
        });
    for (int i = 0; i < fields.size(); i++) {
        QObject::connect(fields[i], &QSpinBox::valueChanged, root, [=](int value) {
            if (*busy || canvas->selected < 0)
                return;
            auto &page = (*pages)[*selected];
            if (canvas->isText()) {
                auto &box = page.texts[canvas->selected - page.images.size()];
                if (i == 0)
                    box.bounds.moveLeft(value);
                if (i == 1)
                    box.bounds.moveTop(value);
                if (i == 2) {
                    resizeTextBox(box, QSize(value, box.bounds.height()));
                }
                if (i == 3) {
                    resizeTextBox(box, QSize(box.bounds.width(), value));
                }
            } else {
                if (i == 0)
                    page.images[canvas->selected].position.setX(value);
                if (i == 1)
                    page.images[canvas->selected].position.setY(value);
            }
            *dirty = true;
            redraw();
        });
        QObject::connect(fields[i], &QSpinBox::editingFinished, root, [=] {
            save();
            refresh();
        });
    }
    for (int i = 0; i < layoutFields.size(); ++i) {
        QObject::connect(layoutFields[i], &QSpinBox::valueChanged, root, [=](int value) {
            if (*busy || !canvas->isText())
                return;
            auto &box = canvas->page->texts[canvas->selected - canvas->page->images.size()];
            setLayoutValue(box, i, value);
            *dirty = true;
            refresh();
            timer->start(500);
        });
        QObject::connect(layoutFields[i], &QSpinBox::editingFinished, root, [=] { save(); });
    }
    QObject::connect(wrap, &QCheckBox::toggled, root, [=](bool on) {
        if (*busy || !canvas->isText())
            return;
        auto &box = canvas->page->texts[canvas->selected - canvas->page->images.size()];
        auto l = textLayout(box);
        const int indent = l.x - l.left[firstMargin(l)];
        l.top = on ? box.bounds.height() / 3 : 320;
        l.bottom = on ? qMax(l.top + 1, box.bounds.height() * 2 / 3) : 320;
        l.x = qBound(-320, l.left[firstMargin(l)] + indent, 320);
        box.layout = l;
        box.flow = true;
        *dirty = true;
        refresh();
        save();
    });
    QObject::connect(resetLayout, &QPushButton::clicked, root, [=] {
        if (!canvas->isText())
            return;
        auto &box = canvas->page->texts[canvas->selected - canvas->page->images.size()];
        box.flow = false;
        box.layout = {};
        *dirty = true;
        refresh();
        save();
    });
    QObject::connect(text, &QPlainTextEdit::textChanged, root, [=] {
        if (*busy || !canvas->isText())
            return;
        (*pages)[*selected].texts[canvas->selected - canvas->page->images.size()].text =
            text->toPlainText();
        *dirty = true;
        redraw();
        timer->start(500);
    });
    QObject::connect(timer, &QTimer::timeout, root, [=] { save(); });
    QObject::connect(list, &QListWidget::currentRowChanged, root, [=](int index) {
        if (*busy || index < 0 || index == *selected)
            return;
        if (!save()) {
            QSignalBlocker b(list);
            list->setCurrentRow(*selected);
            return;
        }
        *selected = index;
        canvas->selected = -1;
        load();
    });
    for (int i = 0; i < pageButtons.size(); i++)
        QObject::connect(pageButtons[i], &QPushButton::clicked, root, [=] {
            if (!save())
                return;
            auto before = *pages;
            int old = *selected;
            if (i == 0)
                pages->insert(++*selected, CutsceneComposition{});
            if (i == 1) {
                auto copy = (*pages)[*selected];
                pages->insert(++*selected, copy);
            }
            if (i == 2) {
                pages->removeAt(*selected);
                *selected = qMin(*selected, int(pages->size()) - 1);
            }
            if (i == 3 || i == 4) {
                int next = *selected + (i == 3 ? -1 : 1);
                pages->swapItemsAt(*selected, next);
                *selected = next;
            }
            *dirty = true;
            canvas->selected = -1;
            if (!save()) {
                *pages = before;
                *selected = old;
                *dirty = false;
            }
            load();
        });
    canvas->selected = state->value(resource + "/object", -1);
    load();
    return root;
}
} // namespace Workshop
