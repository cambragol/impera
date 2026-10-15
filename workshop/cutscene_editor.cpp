#include "cutscene_editor.h"
#include "mod/cutscene_layout.h"
#include "mod/cutscenes.h"
#include <QtWidgets>
#include <memory>
using U5::require;
namespace Workshop {
static QByteArray companion(const Project &p) { return p.resources.value("CUTSCENE.OVR").edited; }
static QVector<CutscenePage> overridden(const Project &p, int scene) {
    auto bytes = companion(p);
    if (bytes.isEmpty())
        return {};
    unsigned count = 0;
    require(U5_CutsceneRead(bytes.constData(), bytes.size(), scene, 0, nullptr, nullptr, &count),
            "Invalid cutscene pages");
    QVector<CutscenePage> pages;
    for (unsigned i = 0; i < count; i++) {
        unsigned layout = 0;
        const char *text = nullptr;
        U5_CutsceneRead(bytes.constData(), bytes.size(), scene, i, &layout, &text, nullptr);
        pages.append({int(layout), QString::fromLatin1(text)});
    }
    return pages;
}
QVector<CutscenePage> cutscenePages(const Project &p, int scene) {
    require(scene == 0 || scene == 1, "Invalid cutscene");
    auto pages = overridden(p, scene);
    if (!pages.isEmpty())
        return pages;
    auto resource = scene ? QString("END.DAT") : QString("STORY.DAT");
    auto bytes = p.data(resource);
    for (int i = 0; i < (scene ? 6 : 21); i++) {
        QString text;
        if (!scene && i == 6)
            text = "Instantly, a shimmering blue door springs up!\nWith heart beating rapidly, you "
                   "step into it.";
        else {
            unsigned offset = U5CutsceneLayouts[scene][i].offset;
            if (scene)
                text = QString::fromLatin1(narrativeText(p, *U5_NarrativeEntry("END.DAT", offset)));
            else {
                int end = bytes.indexOf('\0', offset);
                require(end >= int(offset), "Unterminated introduction page");
                text = QString::fromLatin1(bytes.mid(offset, end - int(offset)));
            }
        }
        text.replace("\r\n", "\n");
        pages.append({i, text});
    }
    return pages;
}
QByteArray changeCutscene(const Project &p, int scene, const QVector<CutscenePage> &pages) {
    require(scene == 0 || scene == 1, "Invalid cutscene");
    require(!pages.isEmpty() && pages.size() <= U5_CUTSCENE_MAX_PAGES,
            "Keep between 1 and 128 pages");
    QVector<CutscenePage> sequences[2];
    sequences[scene] = pages;
    sequences[1 - scene] =
        overridden(p, 1 - scene); // The other sequence continues inheriting vanilla text.
    QByteArray out("ICUT0001", 8);
    auto word = [&](unsigned n) {
        for (int i = 0; i < 4; i++)
            out.append(char(n >> (8 * i)));
    };
    word(sequences[0].size());
    word(sequences[1].size());
    for (int s = 0; s < 2; ++s)
        for (const auto &page : sequences[s]) {
            require(page.layout >= 0 && page.layout < (s ? 6 : 21), "Invalid artwork layout");
            auto text = page.text.toLatin1();
            require(QString::fromLatin1(text) == page.text, "Cutscene text requires ASCII");
            text.replace("\r\n", "\n");
            word(page.layout);
            word(text.size() + 1);
            out += text;
            out += char(0);
        }
    require(U5_ValidateCutscenes(out.constData(), out.size()),
            "Use at most 1999 ASCII characters per page (through z, plus paragraph indents)");
    return out;
}
struct PreviewGlyphs {
    QImage *image;
    QByteArray font;
};
static void glyph(void *context, int index, int x, int y) {
    auto &c = *static_cast<PreviewGlyphs *>(context);
    int count = U5::word(c.font, 0);
    require(count > 0 && count <= 256 && c.font.size() >= 2 + count * 2,
            "Invalid proportional font directory");
    require(index >= 0 && index < count, "Missing proportional glyph");
    int offset = U5::word(c.font, 2 + index * 2);
    require(offset >= 2 + count * 2, "Proportional glyph overlaps its directory");
    int w = U5::word(c.font, offset), h = U5::word(c.font, offset + 2), stride = (w + 7) / 8;
    require(w <= 64 && h <= 64 && offset + 4 + stride * h <= c.font.size(),
            "Invalid proportional glyph");
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++)
            if (U5::byte(c.font, offset + 4 + row * stride + col / 8) & (128 >> (col % 8))) {
                if (x + col >= 0 && x + col < 320 && y + row >= 0 && y + row < 200)
                    c.image->setPixel(x + col, y + row, qRgb(255, 255, 255));
            }
}
QImage previewCutscene(const Project &p, int scene, const CutscenePage &page) {
    require(scene >= 0 && scene < 2 && page.layout >= 0 && page.layout < (scene ? 6 : 21),
            "Invalid cutscene layout");
    // Validate before invoking the same bounded-character layout routine as Impera.
    changeCutscene(p, scene, {page});
    QImage image(320, 200, QImage::Format_RGB32);
    image.fill(Qt::black);
    auto layout = U5CutsceneLayouts[scene][page.layout];
    auto blit = [&](const QString &name, int frame, int x, int y) {
        auto images = U5::readGraphics(name, p.data(name)).images;
        require(frame >= 0 && frame < images.size(), "Artwork frame is missing: " + name);
        QPainter painter(&image);
        painter.drawImage(x, y, images[frame]);
    };
    if (layout.effect == 1) {
        if (!scene && page.layout == 0) {
            blit("TEXT.16", 0, 224, 30);
            blit("TEXT.16", 1, 168, 58);
        }
        if (!scene && page.layout == 7) {
            blit("TEXT.16", 0, 232, 26);
            blit("TEXT.16", 2, 200, 54);
        }
        if (!scene && page.layout == 14) {
            blit("TEXT.16", 0, 184, 0);
            blit("TEXT.16", 3, 248, 0);
        }
        if (scene && page.layout == 0) {
            blit("TEXT.16", 0, 216, 0);
            blit("TEXT.16", 4, 152, 28);
        }
        if (scene && page.layout == 3) {
            blit("TEXT.16", 5, 224, 0);
            blit("TEXT.16", 0, 176, 0);
        }
    }
    QString sheet = QString(scene ? "END%1.16" : "STORY%1.16").arg(layout.sheet + 1);
    blit(sheet, layout.frame, layout.imageX, layout.imageY);
    if (!scene && layout.effect >= 4)
        blit(sheet, layout.effect * 2 - 5, layout.imageX, layout.imageY + 55);
    if (!scene && layout.effect == 3)
        blit(sheet, 3, 96, 39);
    QDir dir(p.sourceDirectory);
    QString fontPath;
    for (auto name : dir.entryList(QDir::Files))
        if (name.compare("PROPORT.PCS", Qt::CaseInsensitive) == 0)
            fontPath = dir.filePath(name);
    QFile file(fontPath);
    require(file.open(QIODevice::ReadOnly) && file.size() <= 1024 * 1024,
            "Preview needs PROPORT.PCS in the game folder");
    PreviewGlyphs context{&image, U5::decompress(file.readAll())};
    auto text = page.text.toLatin1();
    text.replace("\r\n", "\n");
    U5_DrawCutscenePageText(scene, page.layout, text.constData(), glyph, &context);
    // The introduction's second scene reveals its foreground after the page transition.
    if (!scene && layout.effect == 2)
        blit(sheet, 2, 40, 86);
    return image;
}
QWidget *cutsceneEditor(Project *project, const QString &resource, TextCommit commit,
                        QMap<QString, int> *state, std::function<bool()> *flush) {
    int scene = resource == "END.DAT" ? 1 : 0;
    auto root = new QWidget;
    root->setObjectName("cutsceneEditor");
    auto outer = new QHBoxLayout(root);
    auto side = new QVBoxLayout;
    auto list = new QListWidget;
    list->setObjectName("cutscenePages");
    list->setMaximumWidth(260);
    side->addWidget(list, 1);
    auto controls = new QGridLayout;
    QStringList names = {"Add page", "Duplicate", "Remove", "Move up", "Move down"};
    QVector<QPushButton *> buttons;
    for (int i = 0; i < names.size(); i++) {
        auto b = new QPushButton(names[i]);
        buttons << b;
        controls->addWidget(b, i / 2, i % 2);
    }
    buttons[0]->setObjectName("addCutscenePage");
    buttons[1]->setObjectName("duplicateCutscenePage");
    buttons[2]->setObjectName("removeCutscenePage");
    buttons[0]->setToolTip(
        "Insert a blank page after the selected page, using the same artwork layout.");
    buttons[1]->setToolTip("Copy the selected page and its artwork layout.");
    buttons[2]->setToolTip("Remove the selected page. At least one page must remain.");
    buttons[3]->setToolTip("Show this page earlier in the cutscene.");
    buttons[4]->setToolTip("Show this page later in the cutscene.");
    side->addLayout(controls);
    outer->addLayout(side);
    auto detail = new QVBoxLayout;
    auto row = new QHBoxLayout;
    row->addWidget(new QLabel("Artwork layout:"));
    auto artwork = new QComboBox;
    artwork->setObjectName("cutsceneArtwork");
    for (int i = 0; i < (scene ? 6 : 21); i++)
        artwork->addItem(QString("Original page %1").arg(i + 1), i);
    artwork->setToolTip("Choose an original page's artwork arrangement and text placement.");
    row->addWidget(artwork);
    row->addStretch();
    detail->addLayout(row);
    auto text = new QPlainTextEdit;
    text->setObjectName("cutsceneText");
    text->setMinimumHeight(100);
    detail->addWidget(text, 1);
    auto preview = new QLabel;
    preview->setObjectName("cutscenePreview");
    preview->setAlignment(Qt::AlignCenter);
    preview->setWordWrap(true);
    preview->setMinimumSize(640, 400);
    detail->addWidget(preview, 2);
    auto status = new QLabel;
    status->setWordWrap(true);
    detail->addWidget(status);
    auto apply = new QPushButton("Apply page");
    apply->setObjectName("applyCutscenePage");
    auto bottom = new QHBoxLayout;
    bottom->addWidget(apply);
    bottom->addStretch();
    detail->addLayout(bottom);
    outer->addLayout(detail, 1);
    auto pages = std::make_shared<QVector<CutscenePage>>(cutscenePages(*project, scene));
    auto selected =
        std::make_shared<int>(qBound(0, state->value(resource + "/page"), int(pages->size()) - 1));
    auto busy = std::make_shared<bool>(false);
    auto update = [=] {
        if (*busy)
            return;
        try {
            CutscenePage page{artwork->currentIndex(), text->toPlainText()};
            changeCutscene(*project, scene, {page});
            apply->setEnabled(true);
            status->setText(QString("%1 / 1999 characters").arg(page.text.size()));
            try {
                auto image = previewCutscene(*project, scene, page);
                preview->setPixmap(QPixmap::fromImage(
                    image.scaled(640, 400, Qt::IgnoreAspectRatio, Qt::FastTransformation)));
            } catch (const std::exception &e) {
                preview->clear();
                preview->setText(QString::fromUtf8(e.what()));
            }
        } catch (const std::exception &e) {
            status->setText(QString::fromUtf8(e.what()));
            apply->setEnabled(false);
        }
    };
    auto load = [=] {
        *busy = true;
        list->clear();
        for (int i = 0; i < pages->size(); i++)
            list->addItem(QString("Page %1").arg(i + 1));
        list->setCurrentRow(*selected);
        text->setPlainText((*pages)[*selected].text);
        artwork->setCurrentIndex((*pages)[*selected].layout);
        buttons[0]->setEnabled(pages->size() < 128);
        buttons[1]->setEnabled(pages->size() < 128);
        buttons[2]->setEnabled(pages->size() > 1);
        buttons[3]->setEnabled(*selected > 0);
        buttons[4]->setEnabled(*selected + 1 < pages->size());
        (*state)[resource + "/page"] = *selected;
        *busy = false;
        update();
    };
    auto save = [=]() -> bool {
        auto page = CutscenePage{artwork->currentIndex(), text->toPlainText()};
        if (page.text == (*pages)[*selected].text && page.layout == (*pages)[*selected].layout)
            return true;
        try {
            auto next = *pages;
            next[*selected] = page;
            if (!commit({{"CUTSCENE.OVR", changeCutscene(*project, scene, next)}},
                        "Edit cutscene page"))
                return false;
            *pages = next;
            return true;
        } catch (const std::exception &e) {
            status->setText(QString::fromUtf8(e.what()));
            return false;
        }
    };
    *flush = save;
    QObject::connect(text, &QPlainTextEdit::textChanged, root, update);
    QObject::connect(artwork, &QComboBox::currentIndexChanged, root, [=] { update(); });
    QObject::connect(apply, &QPushButton::clicked, root, [=] { save(); });
    QObject::connect(list, &QListWidget::currentRowChanged, root, [=](int next) {
        if (*busy || next < 0 || next == *selected)
            return;
        if (!save()) {
            QSignalBlocker block(list);
            list->setCurrentRow(*selected);
            return;
        }
        *selected = next;
        load();
    });
    for (int i = 0; i < buttons.size(); i++)
        QObject::connect(buttons[i], &QPushButton::clicked, root, [=] {
            if (!save())
                return;
            auto next = *pages;
            int index = *selected;
            if (i == 0 || i == 1) {
                auto page = next[index];
                if (i == 0)
                    page.text.clear();
                next.insert(++index, page);
            }
            if (i == 2) {
                next.removeAt(index);
                index = qMin(index, int(next.size()) - 1);
            }
            if (i == 3 || i == 4) {
                int target = index + (i == 3 ? -1 : 1);
                next.swapItemsAt(index, target);
                index = target;
            }
            try {
                if (commit({{"CUTSCENE.OVR", changeCutscene(*project, scene, next)}}, names[i])) {
                    *pages = next;
                    *selected = index;
                    load();
                }
            } catch (const std::exception &e) {
                status->setText(QString::fromUtf8(e.what()));
            }
        });
    load();
    return root;
}
} // namespace Workshop
