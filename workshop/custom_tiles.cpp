#include "custom_tiles.h"
#include "map_editor.h"
#include "map_entities.h"
#include "mod/package.h"
#include <QImageReader>
#include <QUuid>
#include <algorithm>
#include <cstring>
using U5::require;
namespace CustomTiles {
QMap<unsigned, QByteArray> records(const Project &p) {
    QMap<unsigned, QByteArray> result;
    auto data = p.resources.value("TYPES.OVR").edited;
    if (data.isEmpty())
        return result;
    static thread_local QByteArray cachedBytes;
    static thread_local QMap<unsigned, QByteArray> cachedRecords;
    if (data == cachedBytes)
        return cachedRecords;
    require(TYPES_Validate(data.constData(), data.size()), "Invalid tile library");
    int off = 32;
    for (unsigned i = 0; i < TYPES_U32(data.constData() + 24); i++) {
        auto len = TYPES_U32(data.constData() + off);
        result[TYPES_U32(data.constData() + off + 4)] = data.mid(off, len);
        off += len;
    }
    cachedBytes = data;
    cachedRecords = result;
    return result;
}
TypeDef definition(const Project &p, unsigned id) {
    auto r = records(p).value(id);
    TypeDef d;
    if (r.isEmpty()) {
        TYPES_Vanilla(id >= 256, id & 255, &d);
        if (id < 256) {
            auto label = MapDocument::tileName(id).toUtf8();
            std::memset(d.name, 0, 64);
            std::memcpy(d.name, label.constData(), qMin(63, int(label.size())));
        }
        if (id < 512 && p.resources.contains("LOOK2.DAT")) {
            try {
                auto look = Workshop::description(p.data("LOOK2.DAT"), id);
                std::memcpy(d.description, look.constData(), qMin(127, int(look.size())));
            } catch (const std::exception &) {
            }
        }
    } else
        TYPES_Decode(reinterpret_cast<const unsigned char *>(r.constData()), &d);
    return d;
}
QVector<Frame> frames(const QByteArray &r) {
    QVector<Frame> result;
    if (r.isEmpty())
        return result;
    const auto *p = reinterpret_cast<const unsigned char *>(r.constData());
    int off = 256;
    for (unsigned i = 0; i < U5::word(r, 248); i++) {
        int side = p[off + 1];
        QImage img(p + off + 4, side, side, side * 4, QImage::Format_RGBA8888);
        result.append({img.copy(), U5::word(r, off + 2), p[off]});
        off += 4 + side * side * 4;
    }
    return result;
}
QByteArray record(TypeDef d, const QVector<Frame> &f) {
    d.frames = f.size();
    QByteArray b(256, 0);
    TYPES_Encode(reinterpret_cast<unsigned char *>(b.data()), &d);
    for (const auto &frame : f) {
        auto img = frame.image.convertToFormat(QImage::Format_RGBA8888);
        require((img.width() == 16 || img.width() == 32) && img.width() == img.height(),
                "Frames must be 16 × 16 or 32 × 32 pixels");
        b.append(char(frame.variant));
        b.append(char(img.width()));
        b.append(char(frame.duration));
        b.append(char(frame.duration >> 8));
        for (int y = 0; y < img.height(); y++)
            b.append(reinterpret_cast<const char *>(img.constScanLine(y)), img.width() * 4);
    }
    TYPES_Put32(b.data(), b.size());
    return b;
}
QByteArray library(const Project &p, const QMap<unsigned, QByteArray> &r) {
    QByteArray b(32, 0);
    memcpy(b.data(), "ITYPES01", 8);
    auto old = p.resources.value("TYPES.OVR").edited;
    auto uuid = old.isEmpty() ? QUuid::createUuid().toRfc4122() : old.mid(8, 16);
    memcpy(b.data() + 8, uuid.constData(), 16);
    TYPES_Put32(b.data() + 24, r.size());
    for (auto it = r.begin(); it != r.end(); ++it)
        b += it.value();
    require(b.size() <= MOD_MAX_RESOURCE, "Tile artwork exceeds the 8 MiB library limit");
    require(TYPES_Validate(b.constData(), b.size()),
            "Invalid tile settings, references or mismatched animation variants");
    return b;
}
QImage image(const Project &p, unsigned id, unsigned variant, quint64 ms) {
    auto r = records(p).value(id);
    if (!r.isEmpty()) {
        unsigned side = 0;
        auto rgba =
            TYPES_Frame(reinterpret_cast<const unsigned char *>(r.constData()), variant, ms, &side);
        if (rgba)
            return QImage(rgba, side, side, side * 4, QImage::Format_RGBA8888).copy();
    }
    auto tiles = U5::readGraphics("TILES.16", p.data("TILES.16")).images;
    auto d = definition(p, id);
    auto art = tiles.value(d.base).convertToFormat(QImage::Format_RGBA8888);
    if (d.render & TYPES_SPRITE)
        for (int y = 0; y < art.height(); y++)
            for (int x = 0; x < art.width(); x++)
                if ((art.pixel(x, y) & 0xffffff) == 0)
                    art.setPixelColor(x, y, Qt::transparent);
    return art;
}
TypeCell cell(const QString &resource, const U5::MapPage &mp, int page, int x, int y) {
    TypeCell c = {};
    c.x = x;
    c.y = y;
    c.floor = mp.floor;
    if (resource.endsWith(".CBT")) {
        c.kind = resource == "BRIT.CBT" ? 1 : 2;
        c.map = page;
        c.floor = 0;
    } else if (resource == "BRIT.DAT" || resource == "UNDER.DAT") {
        c.map = 0;
        c.floor = resource == "UNDER.DAT" ? -1 : 0;
    } else {
        int bank = resource == "TOWNE.DAT"      ? 0
                   : resource == "DWELLING.DAT" ? 8
                   : resource == "CASTLE.DAT"   ? 16
                                                : 24;
        c.map = bank + mp.settlement + 1;
    }
    return c;
}
QMap<quint64, TypeCell> placements(const Project &p) {
    QMap<quint64, TypeCell> result;
    auto b = p.resources.value("TYPEMAP.OVR").edited;
    if (b.isEmpty())
        return result;
    auto defs = p.resources.value("TYPES.OVR").edited;
    require(TYPES_ValidateMap(b.constData(), b.size(), defs.constData(), defs.size()),
            "Invalid custom tile placements");
    for (unsigned i = 0; i < TYPES_U32(b.constData() + 24); i++) {
        TypeCell c;
        TYPES_CellDecode(reinterpret_cast<const unsigned char *>(b.constData() + 32 + i * 16), &c);
        result[TYPES_CellKey(&c)] = c;
    }
    return result;
}
QByteArray mapBytes(const Project &p, const QMap<quint64, TypeCell> &cells) {
    auto defs = p.resources.value("TYPES.OVR").edited;
    require(!defs.isEmpty(), "Tile library is missing");
    QByteArray b(32, 0);
    memcpy(b.data(), "ITYMAP01", 8);
    memcpy(b.data() + 8, defs.constData() + 8, 16);
    TYPES_Put32(b.data() + 24, cells.size());
    for (const auto &c : cells) {
        QByteArray r(16, 0);
        TYPES_CellEncode(reinterpret_cast<unsigned char *>(r.data()), &c);
        b += r;
    }
    require(TYPES_ValidateMap(b.constData(), b.size(), defs.constData(), defs.size()),
            "Invalid map tile reference");
    return b;
}
QByteArray expanded(const Project &p, const QString &resource, const U5::MapPage &mp, int page,
                    const QByteArray &base) {
    QByteArray b(base.size() * 4, 0);
    auto placed = placements(p);
    for (int i = 0; i < base.size(); i++) {
        auto c = cell(resource, mp, page, i % mp.side, i / mp.side);
        TYPES_Put32(b.data() + i * 4, placed.value(TYPES_CellKey(&c), c).type);
    }
    return b;
}
QMap<QString, QByteArray> paint(const Project &p, const QString &resource, const U5::MapPage &mp,
                                int page, const QByteArray &base, const QByteArray &expanded) {
    auto cells = placements(p);
    require(expanded.size() == base.size() * 4, "Invalid custom terrain grid");
    for (int i = 0; i < base.size(); i++) {
        auto c = cell(resource, mp, page, i % mp.side, i / mp.side);
        auto k = TYPES_CellKey(&c);
        unsigned id = TYPES_U32(expanded.constData() + i * 4);
        if (id >= 512) {
            c.type = id;
            cells[k] = c;
        } else
            cells.remove(k);
    }
    if (cells.isEmpty() && p.resources.value("TYPEMAP.OVR").edited.isEmpty())
        return {};
    return {{"TYPEMAP.OVR", mapBytes(p, cells)}};
}
void validate(const Project &p) {
    auto defs = p.resources.value("TYPES.OVR").edited,
         map = p.resources.value("TYPEMAP.OVR").edited;
    if (defs.isEmpty()) {
        require(map.isEmpty(), "Custom placements need their tile library");
        return;
    }
    require(TYPES_Validate(defs.constData(), defs.size()),
            "Invalid tile definitions or animation artwork");
    if (!map.isEmpty())
        require(TYPES_ValidateMap(map.constData(), map.size(), defs.constData(), defs.size()),
                "A placed custom tile is missing or invalid");
    for (const auto &c : placements(p)) {
        QString source;
        int page = -1;
        if (c.kind) {
            source = c.kind == 1 ? "BRIT.CBT" : "DUNGEON.CBT";
            page = c.map;
        } else if (!c.map) {
            source = c.floor < 0 ? "UNDER.DAT" : "BRIT.DAT";
            page = 0;
        } else
            source =
                QStringList{"TOWNE.DAT", "DWELLING.DAT", "CASTLE.DAT", "KEEP.DAT"}[(c.map - 1) / 8];
        require(p.resources.contains(source), "Custom tile references a missing map");
        auto pages = U5::mapPages(source, p.data(source));
        if (page < 0)
            for (int i = 0; i < pages.size(); i++)
                if (pages[i].settlement == (c.map - 1) % 8 && pages[i].floor == c.floor)
                    page = i;
        require(page >= 0 && page < pages.size(),
                "Custom tile references a missing floor or encounter");
    }
}
QMap<QString, QByteArray> moveScenery(const Project &p, const QString &resource, int page,
                                      QPoint from, QPoint to) {
    MapDocument doc(&p, resource);
    const auto mp = doc.pages.at(page);
    auto base = doc.terrain(page);
    auto ids = expanded(p, resource, mp, page, base);
    require(QRect(0, 0, mp.side, mp.side).contains(from) &&
                QRect(0, 0, mp.side, mp.side).contains(to),
            "Scenery move is outside the map");
    int source = from.y() * mp.side + from.x(), dest = to.y() * mp.side + to.x();
    unsigned type = TYPES_U32(ids.constData() + source * 4);
    if (type < 512 || from == to)
        return {};
    auto d = definition(p, type);
    unsigned destType = TYPES_U32(ids.constData() + dest * 4);
    unsigned destBase = U5::byte(base, dest);
    require((destType >= 512 && !(definition(p, destType).render & TYPES_SPRITE)) ||
                (!destType &&
                 (destBase == 5 || destBase == 0x40 || destBase == 0x44 || destBase == 0x45)),
            "Scenery destination is occupied");
    unsigned ground = d.ground == TYPES_INFER_GROUND ? (destType ? destType : destBase) : d.ground;
    base[source] = char(ground >= 512 ? definition(p, ground).base : ground);
    base[dest] = char(d.base);
    TYPES_Put32(ids.data() + source * 4, ground >= 512 ? ground : 0);
    TYPES_Put32(ids.data() + dest * 4, type);
    auto changes = doc.changes(page, base, ids), ext = paint(p, resource, mp, page, base, ids);
    for (auto it = ext.begin(); it != ext.end(); it++)
        changes[it.key()] = it.value();
    return changes;
}
class Canvas : public QWidget {
  public:
    QImage pixels, before;
    QRgb color = qRgb(255, 255, 255);
    int tool = 0;
    std::function<void()> changed;
    Canvas() {
        setMinimumSize(384, 384);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setObjectName("customTileCanvas");
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        for (int y = 0; y < height(); y += 12)
            for (int x = 0; x < width(); x += 12)
                p.fillRect(x, y, 12, 12,
                           (x / 12 + y / 12) % 2 ? QColor("#333e4e") : QColor("#151c27"));
        if (!pixels.isNull())
            p.drawImage(rect(), pixels);
    }
    void point(QMouseEvent *e) {
        if (pixels.isNull())
            return;
        QPoint q(e->position().x() * pixels.width() / width(),
                 e->position().y() * pixels.height() / height());
        if (!pixels.rect().contains(q))
            return;
        if (e->buttons() & Qt::RightButton || tool == 3) {
            color = pixels.pixel(q);
            return;
        }
        if (!(e->buttons() & Qt::LeftButton))
            return;
        if (tool == 2) {
            QRgb source = pixels.pixel(q);
            if (source == color)
                return;
            QVector<QPoint> todo{q};
            pixels.setPixel(q, color);
            while (!todo.isEmpty()) {
                auto t = todo.takeLast();
                for (auto n :
                     {t + QPoint(1, 0), t + QPoint(-1, 0), t + QPoint(0, 1), t + QPoint(0, -1)})
                    if (pixels.rect().contains(n) && pixels.pixel(n) == source) {
                        pixels.setPixel(n, color);
                        todo.append(n);
                    }
            }
        } else
            pixels.setPixel(q, tool == 1 ? 0 : color);
        update();
    }
    void mousePressEvent(QMouseEvent *e) override {
        before = pixels;
        point(e);
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (tool != 2)
            point(e);
    }
    void mouseReleaseEvent(QMouseEvent *) override {
        if (before != pixels && changed)
            changed();
    }
};
QWidget *editor(Project *p, Workshop::TextCommit commit, QMap<QString, int> *state) {
    auto w = new QWidget;
    w->setObjectName("customTileEditor");
    auto layout = new QHBoxLayout(w);
    auto left = new QVBoxLayout;
    auto search = new QLineEdit;
    search->setPlaceholderText("Find a tile…");
    left->addWidget(search);
    auto filter = new QComboBox;
    filter->setObjectName("customTileFilter");
    filter->addItems({"All tiles", "World", "Locations"});
    left->addWidget(filter);
    auto list = new QListWidget;
    list->setObjectName("customTileList");
    list->setIconSize({32, 32});
    left->addWidget(list, 1);
    auto add = new QPushButton("New tile…");
    add->setObjectName("newCustomTile");
    left->addWidget(add);
    auto copy = new QPushButton("Copy tile…");
    left->addWidget(copy);
    auto remove = new QPushButton("Delete / Reset");
    left->addWidget(remove);
    add->setToolTip("Create an independent tile without replacing an Ultima 5 tile.");
    copy->setToolTip("Create a new tile using the selected tile’s artwork and settings.");
    remove->setToolTip("Delete a new tile or restore a modified original tile. Used tiles must be "
                       "replaced first.");
    layout->addLayout(left);
    auto middle = new QVBoxLayout;
    auto variants = new QComboBox;
    variants->setObjectName("customTileVariant");
    variants->addItems({"Default / DOS", "Amiga", "Apple II", "Grayscale", "Sharp", "Custom"});
    variants->setCurrentIndex(qBound(0, state->value("customTiles/variant", 0), 5));
    middle->addWidget(variants);
    auto canvas = new Canvas;
    middle->addWidget(canvas, 0, Qt::AlignTop);
    auto tools = new QComboBox;
    tools->addItems({"Pencil", "Eraser", "Fill", "Eyedropper"});
    tools->setCurrentIndex(qBound(0, state->value("customTiles/tool", 0), 3));
    canvas->tool = tools->currentIndex();
    canvas->color = QRgb(state->value("customTiles/color", int(qRgb(255, 255, 255))));
    middle->addWidget(tools);
    auto color = new QPushButton("Brush color…");
    middle->addWidget(color);
    auto frameList = new QListWidget;
    frameList->setObjectName("customTileFrames");
    frameList->setFlow(QListView::LeftToRight);
    frameList->setIconSize({32, 32});
    frameList->setMaximumHeight(72);
    middle->addWidget(frameList);
    auto row = new QHBoxLayout;
    auto frameAdd = new QPushButton("+ Frame"), frameDelete = new QPushButton("− Frame"),
         frameLeft = new QPushButton("←"), frameRight = new QPushButton("→"),
         play = new QPushButton("Play");
    for (auto b : {frameAdd, frameDelete, frameLeft, frameRight, play})
        row->addWidget(b);
    frameAdd->setToolTip("Add a frame to every supplied artwork variant.");
    frameDelete->setToolTip("Delete the selected frame from every artwork variant.");
    frameLeft->setToolTip("Move this frame earlier.");
    frameRight->setToolTip("Move this frame later.");
    play->setToolTip("Preview this tile’s animation.");
    middle->addLayout(row);
    auto ms = new QSpinBox;
    ms->setRange(10, 60000);
    ms->setSuffix(" ms");
    middle->addWidget(ms);
    auto import = new QPushButton("Import PNG…");
    import->setObjectName("customTileImport");
    middle->addWidget(import);
    middle->addStretch();
    layout->addLayout(middle, 1);
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto props = new QWidget;
    auto form = new QFormLayout(props);
    auto name = new QLineEdit, description = new QLineEdit;
    name->setObjectName("customTileName");
    name->setToolTip("Name shown in the tile picker and map inspector.");
    description->setToolTip("Text shown by Impera’s Look command.");
    description->setObjectName("customTileDescription");
    form->addRow("Name", name);
    form->addRow("Look description", description);
    auto world = new QCheckBox("World"), location = new QCheckBox("Locations");
    world->setToolTip("Include this tile in the map picker’s World filter.");
    location->setToolTip(
        "Include this tile in the Locations filter. Both filters can include the same tile.");
    form->addRow(world);
    form->addRow(location);
    auto sprite = new QCheckBox("Transparent sprite"), masonry = new QCheckBox("Masonry");
    sprite->setToolTip(
        "Composite PNG alpha over the ground underneath. Opaque black remains visible.");
    masonry->setToolTip("Keep structural edges crisp when darkness fades around walls.");
    form->addRow(sprite);
    form->addRow(masonry);
    auto ground = new QComboBox;
    form->addRow("Ground underneath", ground);
    auto sight = new QComboBox;
    sight->addItems({"Clear", "Blocks sight", "Window"});
    form->addRow("Sight", sight);
    auto projectile = new QCheckBox("Projectiles pass through");
    form->addRow(projectile);
    auto speed = new QComboBox;
    speed->addItems({"Normal", "Slow", "Very slow"});
    form->addRow("Travel speed", speed);
    QVector<QCheckBox *> movement;
    for (auto label :
         {"Walking", "Sea creatures", "Flying", "Horse", "Phasing", "Skiff", "Frigate",
          "Swamp creatures", "Grass creatures", "Deep-water creatures", "Sand creatures"}) {
        auto box = new QCheckBox(label);
        movement.append(box);
        form->addRow(box);
    }
    QVector<QCheckBox *> skiffDirections;
    auto boatGroup = new QGroupBox("Skiff headings");
    auto boatLayout = new QHBoxLayout(boatGroup);
    for (auto label : {"North", "East", "South", "West"}) {
        auto box = new QCheckBox(label);
        boatLayout->addWidget(box);
        skiffDirections.append(box);
    }
    boatGroup->setToolTip("Allowed skiff headings when skiffs can use this terrain.");
    QObject::connect(movement[5], &QCheckBox::toggled, w, [=](bool enabled) {
        if (enabled && std::none_of(skiffDirections.begin(), skiffDirections.end(),
                                    [](QCheckBox *b) { return b->isChecked(); }))
            for (auto b : skiffDirections)
                b->setChecked(true);
    });
    form->addRow(boatGroup);
    auto humanoidSeat = new QCheckBox("Humanoid seating only");
    form->addRow(humanoidSeat);
    auto combatMap = new QComboBox;
    combatMap->addItem("From terrain", 0);
    for (int i = 0; i < 16; i++)
        combatMap->addItem(QString("Combat map %1").arg(i + 1), i + 1);
    form->addRow("Combat terrain", combatMap);
    auto poison = new QCheckBox("Poisonous"), burn = new QCheckBox("Damaging");
    form->addRow(poison);
    form->addRow(burn);
    QVector<QCheckBox *> effectModes;
    auto effectGroup = new QGroupBox("Affected movement modes");
    auto effectLayout = new QVBoxLayout(effectGroup);
    for (auto label :
         {"Walking", "Sea creatures", "Flying", "Horse", "Phasing", "Skiff", "Frigate",
          "Swamp creatures", "Grass creatures", "Deep-water creatures", "Sand creatures"}) {
        auto box = new QCheckBox(label);
        effectModes.append(box);
        effectLayout->addWidget(box);
    }
    form->addRow(effectGroup);
    auto damageMin = new QSpinBox, damageMax = new QSpinBox;
    damageMin->setRange(0, 255);
    damageMax->setRange(0, 255);
    form->addRow("Minimum damage", damageMin);
    form->addRow("Maximum damage", damageMax);
    auto interaction = new QComboBox;
    interaction->setObjectName("customTileInteraction");
    for (auto pair :
         QList<QPair<QString, int>>{{"Ground", 5},           {"Decoration", 0x43},
                                    {"Wall", 0x4f},          {"Water", 2},
                                    {"Door — closed", 0xb8}, {"Door — magically locked", 0x97},
                                    {"Door — locked", 0xb9}, {"Container", 0xa6},
                                    {"Sign", 0xf8},          {"Stairs", 0xc4},
                                    {"Ladder up", 0xc8},     {"Ladder down", 0xc9},
                                    {"Mirror", 0x9d},        {"Well", 0xa1},
                                    {"Fountain", 0xd8},      {"Clock", 0xfa},
                                    {"Cannon", 0xb4},        {"Tree", 0x2e},
                                    {"Crops", 0x2d},         {"Moongate", 0xdc}})
        interaction->addItem(pair.first, pair.second);
    interaction->setToolTip(
        "Choose the game interaction independently of the tile’s artwork and movement settings.");
    form->addRow("Interaction", interaction);
    QVector<QComboBox *> results;
    for (auto label : {"When opened", "When closed", "When locked", "When broken"}) {
        auto box = new QComboBox;
        results.append(box);
        form->addRow(label, box);
    }
    auto apply = new QPushButton("Apply settings");
    apply->setObjectName("applyCustomTile");
    form->addRow(apply);
    scroll->setWidget(props);
    layout->addWidget(scroll);
    struct Draft {
        unsigned id = 512;
        TypeDef def = {};
        QVector<Frame> frames;
        bool loading = false;
    };
    QSet<unsigned> worldTiles, locationTiles;
    for (const auto &resource : p->resources.keys())
        if (MapDocument::supported(resource)) {
            try {
                MapDocument doc(p, resource);
                for (int page = 0; page < doc.pages.size(); page++)
                    for (unsigned char id : doc.terrain(page))
                        (resource == "BRIT.DAT" || resource == "UNDER.DAT" ? worldTiles
                                                                           : locationTiles)
                            .insert(id);
            } catch (const std::exception &) {
            }
        }
    auto filterList = [=] {
        for (int i = 0; i < list->count(); i++) {
            auto item = list->item(i);
            unsigned id = item->data(Qt::UserRole).toUInt();
            bool group = filter->currentIndex() == 0;
            if (id < 256)
                group |= (filter->currentIndex() == 1 ? worldTiles : locationTiles).contains(id);
            else
                group |= definition(*p, id).filters & (filter->currentIndex() == 1 ? 1 : 2);
            item->setHidden(!group || !item->text().contains(search->text(), Qt::CaseInsensitive));
        }
    };
    QObject::connect(filter, &QComboBox::currentIndexChanged, w, [=] { filterList(); });
    auto draft = std::make_shared<Draft>();
    auto timer = new QTimer(w);
    timer->setInterval(40);
    auto clock = std::make_shared<QElapsedTimer>();
    clock->start();
    auto report = [=](std::function<void()> f) {
        try {
            f();
        } catch (const std::exception &e) {
            QMessageBox::warning(w, "Tile edit rejected", QString::fromUtf8(e.what()));
        }
    };
    auto save = [=](const QString &title) {
        auto r = records(*p);
        if (draft->frames.isEmpty())
            draft->frames.append({image(*p, draft->id), 150, 0});
        r[draft->id] = record(draft->def, draft->frames);
        return commit({{"TYPES.OVR", library(*p, r)}}, title);
    };
    auto fillRefs = [=] {
        ground->clear();
        ground->addItem("Nearby ground", qulonglong(TYPES_INFER_GROUND));
        for (auto box : results) {
            box->clear();
            box->addItem("None / original result", 0);
        }
        auto r = records(*p);
        for (unsigned id = 0; id < 256; id++) {
            QString label = MapDocument::tileName(id);
            ground->addItem(label, id);
            for (auto box : results)
                box->addItem(label, id);
        }
        for (auto it = r.begin(); it != r.end(); ++it) {
            auto d = definition(*p, it.key());
            if (d.kind)
                continue;
            QString label = QString::fromUtf8(d.name);
            if (!(d.render & TYPES_SPRITE))
                ground->addItem(label, d.id);
            for (auto box : results)
                box->addItem(label, d.id);
        }
    };
    auto reloadFrames = [=] {
        QSignalBlocker block(frameList);
        frameList->clear();
        for (int i = 0; i < draft->frames.size(); i++)
            if (draft->frames[i].variant == unsigned(variants->currentIndex())) {
                auto item = new QListWidgetItem(QIcon(QPixmap::fromImage(draft->frames[i].image)),
                                                QString::number(frameList->count() + 1), frameList);
                item->setData(Qt::UserRole, i);
            }
        if (frameList->count())
            frameList->setCurrentRow(
                qBound(0, state->value(QString("customTiles/%1/frame").arg(draft->id), 0),
                       frameList->count() - 1));
        if (frameList->currentItem()) {
            auto f = draft->frames[frameList->currentItem()->data(Qt::UserRole).toInt()];
            canvas->pixels = f.image;
            ms->setValue(f.duration);
        } else
            canvas->pixels = image(*p, draft->id);
        canvas->update();
    };
    auto load = [=](unsigned id) {
        draft->loading = true;
        draft->id = id;
        (*state)["customTiles/id"] = id;
        draft->def = definition(*p, id);
        auto d = draft->def;
        draft->frames = frames(records(*p).value(id));
        fillRefs();
        name->setText(QString::fromUtf8(d.name));
        description->setText(QString::fromUtf8(d.description));
        world->setEnabled(id >= 512);
        location->setEnabled(id >= 512);
        world->setChecked(id >= 512 ? (d.filters & 1) : worldTiles.contains(id));
        location->setChecked(id >= 512 ? (d.filters & 2) : locationTiles.contains(id));
        sprite->setChecked(d.render & TYPES_SPRITE);
        masonry->setChecked(d.render & TYPES_MASONRY);
        sight->setCurrentIndex(d.sight);
        projectile->setChecked(d.projectile);
        speed->setCurrentIndex(d.travel);
        for (int i = 0; i < 11; i++)
            movement[i]->setChecked(d.movement & (1 << i));
        for (int i = 0; i < 11; i++)
            effectModes[i]->setChecked(d.effectModes & (1u << i));
        for (int i = 0; i < 4; i++)
            skiffDirections[i]->setChecked(d.skiffDirections & (8u >> i));
        humanoidSeat->setChecked(d.humanoidSeat);
        combatMap->setCurrentIndex(d.combatMap);
        poison->setChecked(d.effects & TYPES_POISON);
        burn->setChecked(d.effects & TYPES_BURN);
        damageMin->setValue(d.damageMin);
        damageMax->setValue(d.damageMax);
        if (interaction->findData(d.base) < 0)
            interaction->addItem("Same interaction as " + MapDocument::tileName(d.base), d.base);
        interaction->setCurrentIndex(interaction->findData(d.base));
        ground->setCurrentIndex(d.ground == TYPES_INFER_GROUND ? 0 : ground->findData(d.ground));
        for (int i = 0; i < 4; i++)
            results[i]->setCurrentIndex(results[i]->findData(d.result[i]));
        reloadFrames();
        draft->loading = false;
    };
    auto refresh = [=] {
        QSignalBlocker block(list);
        list->clear();
        for (unsigned id = 0; id < 256; id++) {
            auto item = new QListWidgetItem(MapDocument::tileName(id), list);
            item->setData(Qt::UserRole, id);
        }
        auto r = records(*p);
        for (auto it = r.begin(); it != r.end(); ++it)
            if (it.key() >= 512) {
                auto d = definition(*p, it.key());
                if (!d.kind) {
                    auto item = new QListWidgetItem(QIcon(QPixmap::fromImage(image(*p, d.id))),
                                                    QString::fromUtf8(d.name), list);
                    item->setData(Qt::UserRole, d.id);
                }
            }
        for (int i = 0; i < list->count(); i++)
            if (list->item(i)->data(Qt::UserRole).toUInt() == draft->id)
                list->setCurrentRow(i);
        filterList();
    };
    QObject::connect(search, &QLineEdit::textChanged, w, [=] { filterList(); });
    QObject::connect(list, &QListWidget::currentItemChanged, w, [=](QListWidgetItem *item) {
        if (item)
            load(item->data(Qt::UserRole).toUInt());
    });
    auto create = [=](bool copying) {
        report([&] {
            bool ok = false;
            auto title = QInputDialog::getText(w, "New tile", "Name", QLineEdit::Normal,
                                               copying ? name->text() + " copy" : "New tile", &ok);
            if (!ok || title.trimmed().isEmpty())
                return;
            auto r = records(*p);
            unsigned id = 512;
            while (r.contains(id))
                id++;
            require(id <= TYPES_LAST_CUSTOM, "Tile library is full");
            TypeDef d;
            if (copying)
                d = draft->def;
            else
                TYPES_Vanilla(0, 5, &d);
            if (copying && d.id < 256)
                d.filters =
                    (worldTiles.contains(d.id) ? 1 : 0) | (locationTiles.contains(d.id) ? 2 : 0);
            if (!copying)
                d.filters = 3;
            d.id = id;
            d.kind = 0;
            auto utf = title.trimmed().toUtf8();
            require(utf.size() < 64, "Tile name is too long");
            memset(d.name, 0, 64);
            memcpy(d.name, utf.constData(), utf.size());
            QVector<Frame> f = copying ? draft->frames : QVector<Frame>();
            if (f.isEmpty()) {
                QImage img =
                    copying ? image(*p, draft->id) : QImage(16, 16, QImage::Format_RGBA8888);
                if (!copying)
                    img.fill(Qt::black);
                f.append({img, 150, 0});
            }
            r[id] = record(d, f);
            if (commit({{"TYPES.OVR", library(*p, r)}}, "Add tile")) {
                draft->id = id;
                refresh();
                load(id);
            }
        });
    };
    QObject::connect(add, &QPushButton::clicked, w, [=] { create(false); });
    QObject::connect(copy, &QPushButton::clicked, w, [=] { create(true); });
    QObject::connect(remove, &QPushButton::clicked, w, [=] {
        report([&] {
            auto r = records(*p);
            if (!r.contains(draft->id))
                return;
            for (const auto &c : placements(*p))
                require(c.type != draft->id,
                        "This tile is placed on a map. Replace its placements before deleting it.");
            r.remove(draft->id);
            if (commit({{"TYPES.OVR", library(*p, r)}},
                       draft->id < 256 ? "Reset tile" : "Delete tile")) {
                draft->id = 5;
                refresh();
                load(5);
            }
        });
    });
    QObject::connect(apply, &QPushButton::clicked, w, [=] {
        report([&] {
            auto &d = draft->def;
            auto n = name->text().trimmed().toUtf8(), desc = description->text().toUtf8();
            require(!n.isEmpty() && n.size() < 64 && desc.size() < 128,
                    "Name or description exceeds its limit");
            memset(d.name, 0, 64);
            memcpy(d.name, n.constData(), n.size());
            memset(d.description, 0, 128);
            memcpy(d.description, desc.constData(), desc.size());
            if (d.id >= 512)
                d.filters = (world->isChecked() ? 1 : 0) | (location->isChecked() ? 2 : 0);
            d.render = (sprite->isChecked() ? 1 : 0) | (masonry->isChecked() ? 2 : 0);
            d.sight = sight->currentIndex();
            d.projectile = projectile->isChecked();
            d.travel = speed->currentIndex();
            d.movement = 0;
            for (int i = 0; i < 11; i++)
                if (movement[i]->isChecked())
                    d.movement |= 1 << i;
            d.effects = (poison->isChecked() ? 1 : 0) | (burn->isChecked() ? 2 : 0);
            d.effectModes = 0;
            for (int i = 0; i < 11; i++)
                if (effectModes[i]->isChecked())
                    d.effectModes |= 1u << i;
            d.skiffDirections = 0;
            for (int i = 0; i < 4; i++)
                if (skiffDirections[i]->isChecked())
                    d.skiffDirections |= 8u >> i;
            d.humanoidSeat = humanoidSeat->isChecked();
            d.combatMap = combatMap->currentData().toUInt();
            d.damageMin = damageMin->value();
            d.damageMax = damageMax->value();
            d.base =
                interaction->currentData().isValid() ? interaction->currentData().toUInt() : d.base;
            d.ground = ground->currentData().toUInt();
            for (int i = 0; i < 4; i++)
                d.result[i] = results[i]->currentData().toUInt();
            if (save("Edit tile settings"))
                refresh();
        });
    });
    QObject::connect(tools, &QComboBox::currentIndexChanged, w, [=](int i) {
        canvas->tool = i;
        (*state)["customTiles/tool"] = i;
    });
    QObject::connect(color, &QPushButton::clicked, w, [=] {
        auto c = QColorDialog::getColor(QColor(canvas->color), w, "Brush color",
                                        QColorDialog::ShowAlphaChannel);
        if (c.isValid())
            canvas->color = c.rgba();
    });
    QObject::connect(frameList, &QListWidget::currentItemChanged, w, [=](QListWidgetItem *item) {
        if (!item)
            return;
        (*state)[QString("customTiles/%1/frame").arg(draft->id)] = frameList->row(item);
        const auto &f = draft->frames[item->data(Qt::UserRole).toInt()];
        canvas->pixels = f.image;
        QSignalBlocker block(ms);
        ms->setValue(f.duration);
        canvas->update();
    });
    canvas->changed = [=] {
        report([&] {
            if (!frameList->currentItem()) {
                unsigned variant = variants->currentIndex();
                auto defaults = draft->frames;
                if (variant) {
                    for (auto f : defaults)
                        if (!f.variant) {
                            f.variant = variant;
                            draft->frames.append(f);
                        }
                    for (auto &f : draft->frames)
                        if (f.variant == variant) {
                            f.image = canvas->pixels;
                            break;
                        }
                } else
                    draft->frames.append({canvas->pixels, 150, 0});
                std::stable_sort(
                    draft->frames.begin(), draft->frames.end(),
                    [](const Frame &a, const Frame &b) { return a.variant < b.variant; });
            } else
                draft->frames[frameList->currentItem()->data(Qt::UserRole).toInt()].image =
                    canvas->pixels;
            save("Paint tile");
        });
    };
    QObject::connect(variants, &QComboBox::currentIndexChanged, w, [=](int variant) {
        (*state)["customTiles/variant"] = variant;
        reloadFrames();
    });
    QObject::connect(ms, &QSpinBox::editingFinished, w, [=] {
        if (!draft->loading && frameList->currentItem())
            report([&] {
                draft->frames[frameList->currentItem()->data(Qt::UserRole).toInt()].duration =
                    ms->value();
                save("Change frame duration");
            });
    });
    QObject::connect(frameAdd, &QPushButton::clicked, w, [=] {
        report([&] {
            require(variants->currentIndex() == 0,
                    "Add frames to the default animation before supplying alternative artwork");
            for (int v = 0; v < 7; v++) {
                auto f = std::find_if(draft->frames.begin(), draft->frames.end(),
                                      [=](const Frame &f) { return f.variant == unsigned(v); });
                if (f != draft->frames.end()) {
                    auto frame = *f;
                    auto end = std::find_if(f, draft->frames.end(), [=](const Frame &g) {
                        return g.variant > unsigned(v);
                    });
                    draft->frames.insert(end, frame);
                }
            }
            save("Add animation frame");
            reloadFrames();
        });
    });
    QObject::connect(frameDelete, &QPushButton::clicked, w, [=] {
        report([&] {
            require(variants->currentIndex() == 0 && frameList->count() > 1,
                    "The default animation needs at least one frame");
            int index = frameList->currentRow();
            for (int v = 6; v >= 0; v--) {
                int n = 0;
                for (int i = 0; i < draft->frames.size(); i++)
                    if (draft->frames[i].variant == unsigned(v) && n++ == index) {
                        draft->frames.removeAt(i);
                        break;
                    }
            }
            save("Delete animation frame");
            reloadFrames();
        });
    });
    for (auto pair : {qMakePair(frameLeft, -1), qMakePair(frameRight, 1)})
        QObject::connect(pair.first, &QPushButton::clicked, w, [=] {
            report([&] {
                int row = frameList->currentRow(), next = row + pair.second;
                if (row < 0 || next < 0 || next >= frameList->count())
                    return;
                int a = frameList->item(row)->data(Qt::UserRole).toInt(),
                    b = frameList->item(next)->data(Qt::UserRole).toInt();
                draft->frames.swapItemsAt(a, b);
                save("Reorder animation frames");
                reloadFrames();
                frameList->setCurrentRow(next);
            });
        });
    QObject::connect(import, &QPushButton::clicked, w, [=] {
        report([&] {
            auto path =
                QFileDialog::getOpenFileName(w, "Import tile frames", {}, "PNG images (*.png)");
            if (path.isEmpty())
                return;
            QImageReader reader(path);
            require(reader.size().width() <= 4096 && reader.size().height() <= 4096,
                    "PNG dimensions are too large");
            auto img = reader.read().convertToFormat(QImage::Format_RGBA8888);
            require(!img.isNull(), "Cannot decode PNG");
            int side = qMin(img.width(), img.height()) == 16   ? 16
                       : qMin(img.width(), img.height()) == 32 ? 32
                                                               : 0;
            if (!side) {
                bool ok = false;
                QString selected = QInputDialog::getItem(w, "Slice tile sheet", "Frame size",
                                                         {"16 × 16", "32 × 32"}, 0, false, &ok);
                if (!ok)
                    return;
                side = selected.startsWith("16") ? 16 : 32;
            }
            require(img.width() % side == 0 && img.height() % side == 0,
                    "The sheet dimensions must be multiples of the frame size");
            int columns = img.width() / side, rows = img.height() / side, count = columns * rows;
            require(count >= 1 && count <= 64,
                    "An animation supports 1–64 frames per artwork variant");
            QMessageBox preview(
                QMessageBox::Question, "Import frames",
                QString("Import %1 frame(s) of %2 × %2 pixels?").arg(count).arg(side),
                QMessageBox::Yes | QMessageBox::Cancel, w);
            preview.setIconPixmap(QPixmap::fromImage(img).scaled(512, 256, Qt::KeepAspectRatio,
                                                                 Qt::FastTransformation));
            if (preview.exec() != QMessageBox::Yes)
                return;
            unsigned variant = variants->currentIndex();
            QVector<Frame> next;
            for (const auto &f : draft->frames)
                if (f.variant != variant)
                    next.append(f);
            for (int i = 0; i < count; i++)
                next.append({img.copy((i % columns) * side, (i / columns) * side, side, side), 150,
                             variant});
            std::stable_sort(next.begin(), next.end(),
                             [](const Frame &a, const Frame &b) { return a.variant < b.variant; });
            bool alpha = false;
            for (int y = 0; y < img.height() && !alpha; y++)
                for (int x = 0; x < img.width(); x++)
                    if (img.pixelColor(x, y).alpha() < 255) {
                        alpha = true;
                        break;
                    }
            if (alpha) {
                draft->def.render |= TYPES_SPRITE;
                if (draft->def.ground == draft->id) {
                    draft->def.ground = TYPES_INFER_GROUND;
                    ground->setCurrentIndex(0);
                }
                sprite->setChecked(true);
            }
            auto old = draft->frames;
            draft->frames = next;
            try {
                save("Import tile animation");
            } catch (...) {
                draft->frames = old;
                throw;
            }
            reloadFrames();
        });
    });
    QObject::connect(play, &QPushButton::clicked, w, [=] {
        if (timer->isActive()) {
            timer->stop();
            play->setText("Play");
            reloadFrames();
        } else {
            timer->start();
            play->setText("Stop");
        }
    });
    QObject::connect(timer, &QTimer::timeout, w, [=] {
        auto r = record(draft->def, draft->frames);
        unsigned side = 0;
        auto rgba = TYPES_Frame(reinterpret_cast<const unsigned char *>(r.constData()),
                                variants->currentIndex(), clock->elapsed(), &side);
        if (rgba) {
            canvas->pixels = QImage(rgba, side, side, side * 4, QImage::Format_RGBA8888).copy();
            canvas->update();
        }
    });
    draft->id = state->value("customTiles/id", 5);
    refresh();
    load(draft->id);
    return w;
}
} // namespace CustomTiles
