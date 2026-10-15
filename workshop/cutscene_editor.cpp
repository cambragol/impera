#include "cutscene_editor.h"
#include "cutscene_composition.h"
#include "mod/cutscene_layout.h"
#include "mod/cutscenes.h"
#include <QtWidgets>
#include <memory>
using U5::require;
namespace Workshop {
static QVector<int> placementFields(const U5CutscenePlacement &p) {
    return {p.custom,   p.imageX, p.imageY, p.left[0], p.right[0], p.left[1],
            p.right[1], p.top,    p.bottom, p.x,       p.y,        p.secondY};
}
bool operator==(const CutscenePage &a, const CutscenePage &b) {
    return a.layout == b.layout && a.text == b.text && a.placement.custom == b.placement.custom &&
           (!a.placement.custom || placementFields(a.placement) == placementFields(b.placement));
}
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
        U5CutscenePlacement placement = {};
        U5_CutsceneReadPlacement(bytes.constData(), bytes.size(), scene, i, &layout, &text, nullptr,
                                 &placement);
        pages.append({int(layout), QString::fromLatin1(text), placement});
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
    bool custom = false;
    for (const auto &sequence : sequences)
        for (const auto &page : sequence)
            custom |= page.placement.custom != 0;
    QByteArray out(custom ? "ICUT0002" : "ICUT0001", 8);
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
            require(U5_ValidCutscenePlacement(&page.placement),
                    "Keep artwork and text coordinates within the page, with left margins before "
                    "right margins");
            word(page.layout);
            word(text.size() + 1);
            if (custom)
                for (int field : placementFields(page.placement))
                    word(unsigned(field));
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
    int shiftX = page.placement.custom ? page.placement.imageX - layout.imageX : 0;
    int shiftY = page.placement.custom ? page.placement.imageY - layout.imageY : 0;
    auto blit = [&](const QString &name, int frame, int x, int y) {
        auto images = U5::readGraphics(name, p.data(name)).images;
        require(frame >= 0 && frame < images.size(), "Artwork frame is missing: " + name);
        QPainter painter(&image);
        painter.drawImage(x + shiftX, y + shiftY, images[frame]);
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
    U5_DrawPlacedCutsceneText(scene, page.layout, &page.placement, text.constData(), glyph,
                              &context);
    // The introduction's second scene reveals its foreground after the page transition.
    if (!scene && layout.effect == 2)
        blit(sheet, 2, 40, 86);
    return image;
}
QWidget *cutsceneEditor(Project *p,const QString &resource,TextCommit commit,QMap<QString,int> *state,std::function<bool()> *flush) {
    return compositionEditor(p,resource,commit,state,flush);
}
} // namespace Workshop
