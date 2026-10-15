#include "cutscene_composition.h"
#include "content_navigation.h"
#include "mod/cutscenes.h"
#include "mod/package.h"
#include <QCryptographicHash>
#include <QtWidgets>
using U5::require;
namespace Workshop {
static QByteArray fontBytes(const Project &p) {
    QDir dir(p.sourceDirectory);
    for (auto name : dir.entryList(QDir::Files))
        if (name.compare("PROPORT.PCS", Qt::CaseInsensitive) == 0) {
            QFile file(dir.filePath(name));
            require(file.open(QIODevice::ReadOnly) && file.size() < 1024 * 1024,
                    "Cannot read proportional font");
            return U5::decompress(file.readAll());
        }
    return {};
}
struct BoxRaster {
    QImage *image = nullptr;
    QByteArray font;
    QRect bounds;
    QRect ink;
    bool overflow = false;
};
static void boxGlyph(void *context, int glyph, int x, int y) {
    auto &c = *static_cast<BoxRaster *>(context);
    int width = 5, height = 8, stride = 1, offset = 0;
    if (!c.font.isEmpty()) {
        int count = U5::word(c.font, 0);
        require(count > 0 && count <= 256 && glyph >= 0 && glyph < count,
                "Invalid proportional font");
        offset = U5::word(c.font, 2 + glyph * 2);
        require(offset >= 2 + count * 2, "Invalid proportional glyph offset");
        width = U5::word(c.font, offset);
        height = U5::word(c.font, offset + 2);
        stride = (width + 7) / 8;
        require(width <= 64 && height <= 64 && offset + 4 + stride * height <= c.font.size(),
                "Invalid proportional glyph");
    }
    for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
            if (c.font.isEmpty() ||
                (U5::byte(c.font, offset + 4 + row * stride + col / 8) & (128 >> (col % 8)))) {
                QPoint point(x + col, y + row);
                c.ink = c.ink.united(QRect(point, QSize(1, 1)));
                if (!QRect(QPoint(), c.bounds.size()).contains(point)) {
                    c.overflow = true;
                    continue;
                }
                point += c.bounds.topLeft();
                if (c.image && c.image->rect().contains(point))
                    c.image->setPixel(point, qRgb(255, 255, 255));
            }
}
static U5CompositionText textView(const CutsceneTextBox &box, const QByteArray &text) {
    U5CompositionText view = {};
    view.x = box.bounds.x();
    view.y = box.bounds.y();
    view.width = box.bounds.width();
    view.height = box.bounds.height();
    view.flow = box.flow;
    view.layout = box.layout;
    view.text = text.constData();
    return view;
}
static QVector<int> flowFields(const U5CutsceneLayout &l) {
    return {l.left[0], l.right[0], l.left[1], l.right[1], l.top, l.bottom, l.x, l.y};
}
static void addOriginalText(CutsceneComposition &page, const QString &text, U5CutsceneLayout layout,
                            const QByteArray &font) {
    if (text.isEmpty())
        return;
    BoxRaster raster;
    raster.font = font;
    raster.bounds = QRect(0, 0, 320, 200);
    auto bytes = text.toLatin1();
    U5_DrawCutsceneText(&layout, bytes.constData(), 5, boxGlyph, &raster);
    QRect bounds = raster.ink.intersected(QRect(0, 0, 320, 200));
    if (bounds.isEmpty())
        bounds = QRect(8, 8, 304, 184);
    for (int i = 0; i < 2; i++) {
        layout.left[i] -= bounds.x();
        layout.right[i] -= bounds.x();
    }
    layout.x -= bounds.x();
    layout.y -= bounds.y();
    layout.top -= bounds.y();
    layout.bottom -= bounds.y();
    page.texts.append({bounds, text, true, layout});
}
static CutsceneComposition originalComposition(const Project &p, int scene,
                                               const CutscenePage &source, const QByteArray &font) {
    CutsceneComposition page;
    auto layout = U5CutsceneLayouts[scene][source.layout];
    int dx = 0, dy = 0;
    if (source.placement.custom) {
        const auto &v = source.placement;
        dx = v.imageX - layout.imageX;
        dy = v.imageY - layout.imageY;
        for (int i = 0; i < 2; i++) {
            layout.left[i] = v.left[i];
            layout.right[i] = v.right[i];
        }
        layout.top = v.top;
        layout.bottom = v.bottom;
        layout.x = v.x;
        layout.y = v.y;
    }
    auto add = [&](QString resource, int frame, int x, int y) {
        if (!p.resources.contains(resource))
            return;
        auto images = U5::readGraphics(resource, p.data(resource)).images;
        require(frame >= 0 && frame < images.size(), "Missing cutscene image");
        QImage image = images[frame];
        QPoint origin(x + dx, y + dy);
        // Original grouped layouts can move a decorative image beyond the page.
        QRect visible = QRect(origin, image.size()).intersected(QRect(0, 0, 320, 200));
        if (visible.isEmpty())
            return;
        if (origin.x() < 0 || origin.y() < 0) {
            image = image.copy(QRect(visible.topLeft() - origin, visible.size()));
            origin = visible.topLeft();
        }
        page.images.append(
            {QString("%1 · Image %2").arg(artworkTitle(resource)).arg(frame + 1), image, origin});
    };
    if (layout.effect == 1) {
        if (!scene && source.layout == 0) {
            add("TEXT.16", 0, 224, 30);
            add("TEXT.16", 1, 168, 58);
        }
        if (!scene && source.layout == 7) {
            add("TEXT.16", 0, 232, 26);
            add("TEXT.16", 2, 200, 54);
        }
        if (!scene && source.layout == 14) {
            add("TEXT.16", 0, 184, 0);
            add("TEXT.16", 3, 248, 0);
        }
        if (scene && source.layout == 0) {
            add("TEXT.16", 0, 216, 0);
            add("TEXT.16", 4, 152, 28);
        }
        if (scene && source.layout == 3) {
            add("TEXT.16", 5, 224, 0);
            add("TEXT.16", 0, 176, 0);
        }
    }
    QString sheet = QString(scene ? "END%1.16" : "STORY%1.16").arg(layout.sheet + 1);
    add(sheet, layout.frame, layout.imageX, layout.imageY);
    if (!scene && layout.effect >= 4)
        add(sheet, layout.effect * 2 - 5, layout.imageX, layout.imageY + 55);
    if (!scene && layout.effect == 3)
        add(sheet, 3, 96, 39);
    if (!scene && layout.effect == 2)
        add(sheet, 2, 40, 86);
    if (!scene && source.layout == 6 && source.text.contains('\n')) {
        int split = source.text.indexOf('\n');
        addOriginalText(page, source.text.left(split), layout, font);
        layout.x = source.placement.custom ? source.placement.x : U5CutsceneLayouts[0][6].x;
        layout.y = source.placement.custom ? source.placement.secondY : 180;
        addOriginalText(page, source.text.mid(split + 1), layout, font);
    } else
        addOriginalText(page, source.text, layout, font);
    return page;
}
QVector<CutsceneComposition> compositionPages(const Project &p, int scene, bool inherit) {
    require(scene == 0 || scene == 1, "Invalid cutscene");
    auto bytes = p.resources.value("CUTSCENE.OVR").edited;
    if (bytes.startsWith("ICUT0003")) {
        unsigned count = 0;
        require(U5_CompositionRead(bytes.constData(), bytes.size(), scene, 0, &count, nullptr),
                "Invalid cutscene composition");
        if (count) {
            QVector<CutsceneComposition> pages;
            for (unsigned i = 0; i < count; i++) {
                CutsceneComposition page;
                U5CompositionPainter painter = {
                    &page,
                    [](void *c, const U5CompositionAsset *a, int x, int y) {
                        auto &p = *static_cast<CutsceneComposition *>(c);
                        QImage image(a->rgba, int(a->width), int(a->height), int(a->width * 4),
                                     QImage::Format_RGBA8888);
                        p.images.append({QString::fromUtf8(reinterpret_cast<const char *>(a->label),
                                                           int(a->labelSize)),
                                         image.copy(),
                                         {x, y}});
                    },
                    [](void *c, const U5CompositionText *t) {
                        static_cast<CutsceneComposition *>(c)->texts.append(
                            {QRect(t->x, t->y, t->width, t->height), QString::fromLatin1(t->text),
                             bool(t->flow), t->layout});
                    }};
                U5_CompositionRead(bytes.constData(), bytes.size(), scene, i, nullptr, &painter);
                pages.append(page);
            }
            return pages;
        }
    } else if (!inherit) {
        if (bytes.isEmpty())
            return {};
        unsigned count = 0;
        require(
            U5_CutsceneRead(bytes.constData(), bytes.size(), scene, 0, nullptr, nullptr, &count),
            "Invalid cutscene pages");
        if (!count)
            return {};
    }
    if (!inherit && bytes.startsWith("ICUT0003"))
        return {};
    Project legacy = p;
    if (bytes.startsWith("ICUT0003"))
        legacy.resources["CUTSCENE.OVR"].edited.clear();
    QVector<CutsceneComposition> pages;
    auto font = fontBytes(p);
    for (const auto &page : cutscenePages(legacy, scene))
        pages.append(originalComposition(p, scene, page, font));
    return pages;
}
QByteArray changeComposition(const Project &p, int scene,
                             const QVector<CutsceneComposition> &pages) {
    require(scene == 0 || scene == 1, "Invalid cutscene");
    require(!pages.isEmpty() && pages.size() <= 128, "Keep between 1 and 128 pages");
    QVector<CutsceneComposition> sequences[2];
    sequences[scene] = pages;
    sequences[1 - scene] = compositionPages(p, 1 - scene, false);
    struct Asset {
        QString name;
        QImage image;
        QByteArray raw;
    };
    QVector<Asset> assets;
    QHash<QByteArray, int> ids;
    auto assetId = [&](const CutsceneImage &element) {
        require(!element.image.isNull() && element.image.width() <= 1024 &&
                    element.image.height() <= 1024,
                "Images must be between 1 and 1024 pixels on each side");
        auto image = element.image.convertToFormat(QImage::Format_RGBA8888);
        QByteArray raw(reinterpret_cast<const char *>(image.constBits()), int(image.sizeInBytes()));
        auto hash = QCryptographicHash::hash(raw + QByteArray::number(image.width()) + ":" +
                                                 QByteArray::number(image.height()),
                                             QCryptographicHash::Sha256);
        if (ids.contains(hash))
            return ids[hash];
        require(assets.size() < 256, "A cutscene package can contain at most 256 different images");
        int id = assets.size();
        assets.append({element.name, image, raw});
        ids.insert(hash, id);
        return id;
    };
    for (const auto &sequence : sequences)
        for (const auto &page : sequence)
            for (const auto &image : page.images)
                assetId(image);
    QByteArray out("ICUT0003", 8);
    auto word = [&](unsigned n) {
        for (int i = 0; i < 4; i++)
            out.append(char(n >> (8 * i)));
    };
    word(sequences[0].size());
    word(sequences[1].size());
    word(assets.size());
    for (const auto &asset : assets) {
        auto name = asset.name.toUtf8();
        require(!name.isEmpty() && name.size() <= 256 && !name.contains('\0'),
                "Image name is too long");
        word(asset.image.width());
        word(asset.image.height());
        word(name.size());
        out += name;
        out += asset.raw;
        require(out.size() <= MOD_MAX_RESOURCE,
                "Cutscene images exceed the 8 MiB package resource limit");
    }
    for (const auto &sequence : sequences)
        for (const auto &page : sequence) {
            require(page.images.size() <= 64 && page.texts.size() <= 32,
                    "Each page supports up to 64 images and 32 text boxes");
            word(page.images.size());
            word(page.texts.size());
            for (const auto &image : page.images) {
                word(assetId(image));
                word(image.position.x());
                word(image.position.y());
            }
            for (const auto &box : page.texts) {
                auto text = box.text.toLatin1();
                require(QString::fromLatin1(text) == box.text, "Use Ultima 5 ASCII text");
                text.replace("\r\n", "\n");
                word(box.bounds.x());
                word(box.bounds.y());
                word(box.bounds.width());
                word(box.bounds.height());
                word(box.flow);
                for (int value : flowFields(box.flow ? box.layout : U5CutsceneLayout{}))
                    word(unsigned(value));
                word(text.size() + 1);
                out += text;
                out += char(0);
            }
        }
    require(U5_CompositionRead(out.constData(), out.size(), -1, 0, nullptr, nullptr),
            "Invalid page: keep images on the page, text boxes within its edges, and text under "
            "2000 ASCII characters");
    return out;
}
QImage previewComposition(const Project &p, const CutsceneComposition &page, QStringList *issues) {
    QImage image(320, 200, QImage::Format_RGB32);
    image.fill(Qt::black);
    for (const auto &element : page.images) {
        auto source = element.image.convertToFormat(QImage::Format_RGBA8888);
        for (int y = 0; y < source.height(); y++)
            for (int x = 0; x < source.width(); x++) {
                QPoint point = element.position + QPoint(x, y);
                if (!image.rect().contains(point))
                    continue;
                const uchar *pixel = source.constScanLine(y) + x * 4;
                unsigned alpha = pixel[3];
                if (!alpha)
                    continue;
                QRgb back = image.pixel(point);
                unsigned color = 0xff000000;
                for (int channel = 0; channel < 3; channel++) {
                    int shift = 16 - channel * 8;
                    color |=
                        ((pixel[channel] * alpha + ((back >> shift) & 255) * (255 - alpha) + 127) /
                         255)
                        << shift;
                }
                image.setPixel(point, color);
            }
    }
    auto font = fontBytes(p);
    int i = 0;
    for (const auto &box : page.texts) {
        auto text = box.text.toLatin1();
        require(text.size() < 2000 && QString::fromLatin1(text) == box.text,
                "Use at most 1999 Ultima 5 ASCII characters per text box");
        for (unsigned char ch : text)
            require(ch == 10 || ch == 13 || (ch >= 32 && ch <= 123),
                    "This character is not supported by the Ultima 5 font");
        auto view = textView(box, text);

        BoxRaster raster{&image, font, box.bounds, {}, false};
        U5_DrawCompositionText(&view, boxGlyph, &raster);
        if (raster.overflow && issues)
            issues->append(QString("Text box %1: text does not fit").arg(i + 1));
        ++i;
    }
    return image;
}
} // namespace Workshop
