#include "custom_tiles.h"
#include "color_palette.h"
#include "window.h"
#include "map_entities.h"
#include <QApplication>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QSettings>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x);                                        \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
int main(int argc, char **argv) {
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    QApplication app(argc, argv);
    try {
        Project p;
        p.openGame(TEST_GAME_DIR);
        QImage tile(16, 16, QImage::Format_RGBA8888);
        tile.fill(Qt::transparent);
        tile.setPixelColor(0, 0, Qt::black);
        tile.setPixelColor(1, 0, QColor(20, 80, 220, 128));
        TypeDef d;
        TYPES_Vanilla(0, 5, &d);
        d.id = 40000;
        d.filters = 3;
        strcpy(d.name, "Crystal floor");
        d.render = TYPES_MASONRY;
        auto frames =
            QVector<CustomTiles::Frame>{{tile, 100, 0}, {tile.mirrored(true, false), 200, 0}};
        QMap<unsigned, QByteArray> records;
        records[d.id] = CustomTiles::record(d, frames);
        p.resources["TYPES.OVR"].edited = CustomTiles::library(p, records);
        p.validate();
        CHECK(CustomTiles::image(p, 40000).pixelColor(0, 0) == Qt::black);
        CHECK(CustomTiles::image(p, 40000).pixelColor(1, 0).alpha() == 128);
        CHECK(CustomTiles::image(p, 40000, 4, 100).pixelColor(15, 0) == Qt::black);
        MapDocument doc(&p, "DWELLING.DAT");
        int page = -1;
        for (int i = 0; i < doc.pages.size(); i++)
            if (doc.pages[i].name.startsWith("Iolo"))
                page = i;
        CHECK(page >= 0);
        auto base = doc.terrain(page);
        auto extended = CustomTiles::expanded(p, "DWELLING.DAT", doc.pages[page], page, base);
        TYPES_Put32(extended.data() + 4 * (4 * 32 + 3), 40000);
        base[4 * 32 + 3] = 5;
        base[4 * 32 + 4] = 5;
        auto changes = doc.changes(page, base, extended);
        auto extra = CustomTiles::paint(p, "DWELLING.DAT", doc.pages[page], page, base, extended);
        for (auto it = extra.begin(); it != extra.end(); it++)
            changes[it.key()] = it.value();
        for (auto it = changes.begin(); it != changes.end(); it++)
            p.resources[it.key()].edited = it.value();
        p.validate();
        auto cells = CustomTiles::placements(p);
        CHECK(cells.size() == 1);
        CHECK(cells.first().map == 13);
        CHECK(cells.first().type == 40000);
        auto moved = CustomTiles::moveScenery(p, "DWELLING.DAT", page, {3, 4}, {4, 4});
        CHECK(moved.contains("TYPEMAP.OVR"));
        Project movedProject = p;
        for (auto it = moved.begin(); it != moved.end(); it++)
            movedProject.resources[it.key()].edited = it.value();
        movedProject.validate();
        CHECK(CustomTiles::placements(movedProject).first().x == 4);
        CHECK(CustomTiles::placements(p).first().x == 3);
        auto idImage =
            doc.terrainImage(page, U5::readGraphics("TILES.16", p.data("TILES.16")).images, true);
        CHECK(idImage.format() == QImage::Format_Grayscale16);
        CHECK(reinterpret_cast<const quint16 *>(idImage.constScanLine(4))[3] == 40000);
        MapDocument world(&p, "BRIT.DAT");
        auto water = world.terrain(0);
        auto worldIds = CustomTiles::expanded(p, "BRIT.DAT", world.pages[0], 0, water);
        TYPES_Put32(worldIds.data(), 40000);
        water[0] = 5;
        auto worldChanges = world.changes(0, water, worldIds);
        CHECK(worldChanges["BRIT.DAT"] == p.data("BRIT.DAT"));
        CHECK(worldChanges["DATA.OVL"] == p.data("DATA.OVL"));
        auto package = p.package();
        CHECK(package.startsWith("IMOD0009"));
        Project imported;
        imported.openGame(TEST_GAME_DIR);
        imported.importPackage(package);
        CHECK(CustomTiles::placements(imported).first().type == 40000);
        QTemporaryDir temp;
        CHECK(temp.isValid());
        p.save(temp.filePath("tile.imperaproject"));
        Project restored;
        restored.load(temp.filePath("tile.imperaproject"));
        CHECK(restored.data("TYPES.OVR") == p.data("TYPES.OVR"));
        CHECK(restored.data("TYPEMAP.OVR") == p.data("TYPEMAP.OVR"));
        MapCanvas canvas;
        canvas.side = 2;
        canvas.ids = QByteArray(4, 5);
        canvas.extended = QByteArray(16, 0);
        canvas.libraryUuid = p.data("TYPES.OVR").mid(8, 16);
        canvas.legacyTiles[40000] = 5;
        canvas.setTileId(0, 40000);
        CHECK(canvas.tileId(0) == 40000);
        canvas.selection = QRect(0, 0, 1, 1);
        CHECK(canvas.copySelection());
        canvas.setTileId(0, 5);
        CHECK(canvas.beginPaste());
        canvas.commit = [&](const QByteArray &) { return true; };
        canvas.resizeMap();
        canvas.show();
        QPointF pos(16 * canvas.zoom + 2, 16 * canvas.zoom + 2);
        QMouseEvent press(QEvent::MouseButtonPress, pos, pos, Qt::LeftButton, Qt::LeftButton,
                          Qt::NoModifier);
        QApplication::sendEvent(&canvas, &press);
        QMouseEvent release(QEvent::MouseButtonRelease, pos, pos, Qt::LeftButton, Qt::NoButton,
                            Qt::NoModifier);
        QApplication::sendEvent(&canvas, &release);
        CHECK(canvas.tileId(3) == 40000);
        canvas.libraryUuid = QByteArray(16, 'x');
        CHECK(!canvas.beginPaste());
        QMap<QString, int> state;
        state["customTiles/id"] = 40000;
        int edits = 0;
        auto edit = [&](const QMap<QString, QByteArray> &c, const QString &) {
            Project candidate = p;
            for (auto it = c.begin(); it != c.end(); it++)
                candidate.resources[it.key()].edited = it.value();
            candidate.validate();
            p = candidate;
            edits++;
            return true;
        };
        std::unique_ptr<QWidget> ui(CustomTiles::editor(&p, edit, &state));
        CHECK(ui->findChild<QListWidget *>("customTileList"));
        auto name = ui->findChild<QLineEdit *>("customTileName");
        CHECK(name && name->text() == "Crystal floor");
        name->setText("Polished crystal");
        QMetaObject::invokeMethod(name, "editingFinished", Qt::DirectConnection);
        CHECK(edits == 1);
        CHECK(QString::fromUtf8(CustomTiles::definition(p, 40000).name) == "Polished crystal");
        CHECK(!ui->findChild<QPushButton *>("applyCustomTile"));
        for (const auto &section : {"Interaction", "Movement", "Appearance", "Hazards", "Advanced"})
            CHECK(ui->findChild<QWidget *>(QString("tileSection%1").arg(section)));
        auto groundPicker = ui->findChild<QComboBox *>("customTileGround");
        CHECK(groundPicker && groundPicker->isHidden());
        CHECK(groundPicker->isEditable() && !groundPicker->itemIcon(1).isNull());
        auto behavior = ui->findChild<QComboBox *>("customTileInteraction");
        auto openAppearance = ui->findChild<QComboBox *>("tileStateAppearance0");
        CHECK(openAppearance->isHidden());
        behavior->setCurrentIndex(behavior->findData(0xb8));
        CHECK(!openAppearance->isHidden());
        CHECK(CustomTiles::definition(p, 40000).base == 0xb8);
        behavior->setCurrentIndex(behavior->findData(5));
        CHECK(openAppearance->isHidden());
        openAppearance->setCurrentIndex(openAppearance->findData(40000));
        CHECK(!openAppearance->isHidden());
        CHECK(CustomTiles::definition(p, 40000).result[0] == 40000);
        openAppearance->setCurrentIndex(0);
        CHECK(openAppearance->isHidden());
        auto error = ui->findChild<QLabel *>("customTileSettingsError");
        int validEdits = edits;
        name->clear();
        QMetaObject::invokeMethod(name, "editingFinished", Qt::DirectConnection);
        CHECK(edits == validEdits && !error->isHidden());
        CHECK(QString::fromUtf8(CustomTiles::definition(p, 40000).name) == "Polished crystal");
        name->setText("Polished crystal");
        QMetaObject::invokeMethod(name, "editingFinished", Qt::DirectConnection);
        CHECK(error->isHidden());
        auto variants = ui->findChild<QComboBox *>("customTileVariant");
        auto colors = ui->findChild<QWidget *>("customTilePalette");
        CHECK(variants && colors);
        for (int i = 0; i < 4; i++) {
            auto tool = ui->findChild<QToolButton *>(QString("customTileTool%1").arg(i));
            CHECK(tool && !tool->icon().isNull() && !tool->toolTip().isEmpty());
            tool->click();
            CHECK(state["customTiles/tool"] == i && tool->isChecked());
        }
        ui->findChild<QToolButton *>("customTileTool0")->click();
        const QStringList sheets{"TILES.16", "TILE_AMI.PNG", "TILE_APP.PNG", "TILE_GRY.PNG",
                                 "TILE_SHW.PNG"};
        for (int i = 0; i < sheets.size(); i++) {
            variants->setCurrentIndex(i);
            auto canvasSize = ui->findChild<QWidget *>("customTileCanvas")->size();
            auto nativeSize = CustomTiles::artworkSize(i);
            CHECK(canvasSize.width() * nativeSize.height() ==
                  canvasSize.height() * nativeSize.width());
            auto expected = Workshop::artworkColors(U5::readGraphics(sheets[i], p.data(sheets[i])));
            QSet<QRgb> actual;
            for (auto b : colors->findChildren<QToolButton *>())
                if (b->property("rgba").isValid())
                    actual.insert(b->property("rgba").toUInt());
            CHECK(actual.size() == expected.size() + 1);
            CHECK(colors->findChildren<QToolButton *>().size() == actual.size());
            if (i == 4) {
                CHECK(expected.size() == 8);
                for (int rgb = 0; rgb < 8; rgb++)
                    CHECK(actual.contains(
                        qRgb(rgb & 1 ? 255 : 0, rgb & 2 ? 255 : 0, rgb & 4 ? 255 : 0)));
            }
            for (auto rgba : expected)
                CHECK(actual.contains(rgba));
            auto strip = ui->findChild<QListWidget *>("customTileFrames");
            CHECK(strip->count() == 2); // Empty slots expose the DOS animation.
        }
        // Editing an empty Sharp slot copies DOS frames without changing DOS artwork.
        auto dosBefore = CustomTiles::image(p, 40000);
        auto paintCanvas = ui->findChild<QWidget *>("customTileCanvas");
        QMouseEvent pixelPress(QEvent::MouseButtonPress, QPointF(50, 50), QPointF(50, 50),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent pixelRelease(QEvent::MouseButtonRelease, QPointF(50, 50), QPointF(50, 50),
                                 Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(paintCanvas, &pixelPress);
        QApplication::sendEvent(paintCanvas, &pixelRelease);
        CHECK(CustomTiles::image(p, 40000) == dosBefore);
        CHECK(CustomTiles::image(p, 40000, 4) != dosBefore);
        auto sharp = CustomTiles::image(p, 40000, 4);
        auto sharpColors =
            Workshop::artworkColors(U5::readGraphics("TILE_SHW.PNG", p.data("TILE_SHW.PNG")));
        for (int y = 0; y < sharp.height(); y++)
            for (int x = 0; x < sharp.width(); x++) {
                auto rgba = sharp.pixel(x, y);
                CHECK(!qAlpha(rgba) ||
                      sharpColors.contains(qRgb(qRed(rgba), qGreen(rgba), qBlue(rgba))));
            }
        CHECK(!ui->findChild<QPushButton *>("customTileUseDos"));
        auto useDos = ui->findChild<QAction *>("customTileUseDos");
        CHECK(useDos && useDos->isEnabled());
        useDos->trigger();
        CHECK(CustomTiles::image(p, 40000, 4) == dosBefore);
        CHECK(!useDos->isEnabled());
        variants->setCurrentIndex(2);
        QApplication::sendEvent(paintCanvas, &pixelPress);
        QApplication::sendEvent(paintCanvas, &pixelRelease);
        CHECK(CustomTiles::image(p, 40000, 2).size() == QSize(14, 16));
        auto appleFrames = CustomTiles::frames(CustomTiles::records(p).value(40000));
        CHECK(appleFrames.size() == 4);
        CHECK(appleFrames[2].image.size() == QSize(14, 16));
        CHECK(appleFrames[3].image.size() == QSize(14, 16));
        CHECK(CustomTiles::image(p, 40000) == dosBefore);
        useDos->trigger();

        // Independent NPC/monster definitions share art but not identity or stats.
        auto actorRecords = CustomTiles::records(p);
        for (unsigned kind = 1; kind <= 2; kind++) {
            TypeDef actor;
            TYPES_Vanilla(1, kind == 1 ? 80 : 144, &actor);
            actor.kind = kind;
            actor.id = 40009 + kind;
            actor.render = TYPES_SPRITE;
            actor.stats[5] = kind == 1 ? 30 : 60;
            actor.stats[6] = 1;
            strcpy(actor.name, kind == 1 ? "Scholar" : "Crystal beast");
            strcpy(actor.description, kind == 1 ? "Scholars" : "Crystal beasts");
            actorRecords[actor.id] = CustomTiles::record(actor, {{tile, 150, 0}});
        }
        p.resources["TYPES.OVR"].edited = CustomTiles::library(p, actorRecords);
        auto applyChanges = [&](const QMap<QString, QByteArray> &changes) {
            for (auto it = changes.begin(); it != changes.end(); ++it)
                p.resources[it.key()].edited = it.value();
        };
        auto placedNpc = Workshop::addMapItem(p, "DWELLING.DAT", page, {8, 8},
                                              Workshop::MapItemKind::Npc, 40010, -1, "Mira");
        applyChanges(placedNpc.changes);
        CHECK(CustomTiles::actorType(p, "DWELLING.DAT", doc.pages[page], page, placedNpc.slot, 0) ==
              40010);
        MapNpcDocument npcDocument(&p, "DWELLING.NPC");
        CHECK(npcDocument.sprite(doc.pages[page].settlement, placedNpc.slot) == 40010);
        CHECK(npcDocument.dialogue(doc.pages[page].settlement, placedNpc.slot) > 0);
        auto worldMonster =
            Workshop::addMapItem(p, "BRIT.DAT", 0, {20, 20}, Workshop::MapItemKind::Monster, 40011);
        applyChanges(worldMonster.changes);
        auto worldPage = U5::mapPages("BRIT.DAT", p.data("BRIT.DAT"))[0];
        CHECK(CustomTiles::actorType(p, "BRIT.DAT", worldPage, 0, worldMonster.slot, 0) == 40011);
        auto roomPage = U5::mapPages("DUNGEON.CBT", p.data("DUNGEON.CBT"))[0];
        applyChanges(CustomTiles::assignActor(p, "DUNGEON.CBT", roomPage, 0, 0, 40011));
        CHECK(CustomTiles::actorType(p, "DUNGEON.CBT", roomPage, 0, 0, 0) == 40011);
        p.validate();
        CHECK(p.package().startsWith("IMOD0010"));
        Project actorImported;
        actorImported.openGame(TEST_GAME_DIR);
        actorImported.importPackage(p.package());
        CHECK(CustomTiles::actorType(actorImported, "DWELLING.DAT", doc.pages[page], page,
                                     placedNpc.slot, 0) == 40010);
        QMap<QString, int> actorState{{"customTiles/kind", 2}, {"customTiles/2/id", 40011}};
        std::unique_ptr<QWidget> actorUi(CustomTiles::editor(&p, edit, &actorState));
        auto hitPoints = actorUi->findChild<QSpinBox *>("actorStat5");
        CHECK(hitPoints && hitPoints->value() == 60);
        hitPoints->setValue(75);
        QMetaObject::invokeMethod(hitPoints, "editingFinished", Qt::DirectConnection);
        CHECK(CustomTiles::definition(p, 40011).stats[5] == 75);
        CHECK(CustomTiles::definition(p, 40010).stats[5] == 30);
        auto removedNpc = Workshop::deleteMapItem(p, "DWELLING.DAT", page, "NPCs / Schedules",
                                                  placedNpc.slot, {8, 8});
        applyChanges(removedNpc);
        CHECK(CustomTiles::actorType(p, "DWELLING.DAT", doc.pages[page], page, placedNpc.slot, 0) ==
              0);
        p.validate();
        QMap<QString, int> npcTypeState{{"customTiles/kind", 1}, {"customTiles/1/id", 40010}};
        std::unique_ptr<QWidget> npcTypesUi(CustomTiles::editor(&p, edit, &npcTypeState));
        for (auto button : npcTypesUi->findChildren<QPushButton *>())
            if (button->text() == "Delete / Reset")
                button->click();
        CHECK(!CustomTiles::records(p).contains(40010));
        CHECK(npcTypeState["customTiles/1/id"] == 336);
        CHECK(!npcTypesUi->findChild<QLineEdit *>("customTileName")->text().isEmpty());
        // Original actor frames belong to the native sheets, not a timed type override.
        for (int kind : {1, 2}) {
            const unsigned nativeId = kind == 1 ? 336 : 400;
            QMap<QString, int> nativeState{{"customTiles/kind", kind},
                                           {QString("customTiles/%1/id").arg(kind), int(nativeId)}};
            std::unique_ptr<QWidget> nativeUi(CustomTiles::editor(&p, edit, &nativeState));
            auto nativeFrames = nativeUi->findChild<QListWidget *>("customTileFrames");
            auto nativeVariants = nativeUi->findChild<QComboBox *>("customTileVariant");
            CHECK(nativeFrames && nativeFrames->count() == 4);
            CHECK(nativeUi->findChild<QSpinBox *>("customTileFrameDuration")->isHidden());
            const QStringList nativeSheets{"TILES.16",     "TILE_AMI.PNG", "TILE_APP.PNG",
                                           "TILE_GRY.PNG", "TILE_SHN.PNG", "TILES.16"};
            for (int variant = 0; variant < nativeSheets.size(); ++variant) {
                nativeVariants->setCurrentIndex(variant);
                CHECK(nativeFrames->count() == 4);
                nativeFrames->setCurrentRow(2);
                auto before =
                    U5::readGraphics(nativeSheets[variant], p.data(nativeSheets[variant]));
                int index = int(nativeId) - before.tileOffset;
                auto color = before.images[index + 2].pixelColor(0, 0) == QColor(Qt::white)
                                 ? qRgb(0, 0, 0)
                                 : qRgb(255, 255, 255);
                auto nativePalette = nativeUi->findChild<QWidget *>("customTilePalette");
                bool picked = false;
                for (auto swatch : nativePalette->findChildren<QToolButton *>())
                    if (swatch->property("rgba").isValid() &&
                        swatch->property("rgba").toUInt() == color) {
                        swatch->click();
                        picked = true;
                    }
                CHECK(picked);
                nativeUi->findChild<QToolButton *>("customTileTool0")->click();
                auto nativeCanvas = nativeUi->findChild<QWidget *>("customTileCanvas");
                QMouseEvent down(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1),
                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QMouseEvent up(QEvent::MouseButtonRelease, QPointF(1, 1), QPointF(1, 1),
                               Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(nativeCanvas, &down);
                QApplication::sendEvent(nativeCanvas, &up);
                auto after = U5::readGraphics(nativeSheets[variant], p.data(nativeSheets[variant]));
                CHECK(after.images[index + 2].pixelColor(0, 0).rgb() == color);
                for (int i = 0; i < before.images.size(); ++i)
                    if (i != index + 2)
                        for (int y = 0; y < before.images[i].height(); ++y)
                            for (int x = 0; x < before.images[i].width(); ++x)
                                CHECK(before.images[i].pixel(x, y) == after.images[i].pixel(x, y));
                CHECK(CustomTiles::frames(CustomTiles::records(p).value(nativeId)).isEmpty());
                CHECK(nativeFrames->count() == 4 && nativeFrames->currentRow() == 2);
            }
            auto nativeHealth = nativeUi->findChild<QSpinBox *>("actorStat5");
            nativeHealth->setValue(nativeHealth->value() + 1);
            QMetaObject::invokeMethod(nativeHealth, "editingFinished", Qt::DirectConnection);
            CHECK(CustomTiles::frames(CustomTiles::records(p).value(nativeId)).isEmpty());
            CHECK(nativeFrames->count() == 4);
            for (auto button : nativeUi->findChildren<QPushButton *>())
                if (button->text() == "Delete / Reset")
                    button->click();
            CHECK(!CustomTiles::records(p).contains(nativeId));
            for (const auto &resource : nativeSheets) {
                auto restored = U5::readGraphics(resource, p.data(resource));
                auto original = U5::readGraphics(resource, p.resources.value(resource).original);
                int index = int(nativeId) - restored.tileOffset;
                for (int frame = 0; frame < 4; ++frame)
                    for (int y = 0; y < restored.images[index + frame].height(); ++y)
                        for (int x = 0; x < restored.images[index + frame].width(); ++x)
                            CHECK(restored.images[index + frame].pixel(x, y) ==
                                  original.images[index + frame].pixel(x, y));
            }
            p.validate();
        }
        WorkshopWindow window;
        CHECK(window.openGame(TEST_GAME_DIR));
        window.projectForTests() = p;
        for (const auto &sheet : sheets) {
            window.selectResource(sheet);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            auto palette = window.findChild<QWidget *>("artworkPalette");
            CHECK(palette);
            auto expected = Workshop::artworkColors(U5::readGraphics(sheet, p.data(sheet)));
            QSet<QRgb> actual;
            for (auto b : palette->findChildren<QToolButton *>())
                if (b->property("rgba").isValid())
                    actual.insert(b->property("rgba").toUInt());
            CHECK(actual.size() == expected.size());
            for (auto rgba : expected) {
                if (!actual.contains(rgba))
                    fprintf(stderr, "Palette mismatch %s color %08x\n", qPrintable(sheet), rgba);
                CHECK(actual.contains(rgba));
            }
        }

        window.selectResource("DWELLING.DAT");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        auto palette = window.findChild<QListWidget *>("mapPalette");
        CHECK(palette);
        bool found = false;
        for (int i = 0; i < palette->count(); i++)
            if (palette->item(i)->data(Qt::UserRole).toUInt() == 40000) {
                palette->setCurrentRow(i);
                found = true;
            }
        CHECK(found);
        auto mapCanvas = dynamic_cast<MapCanvas *>(window.findChild<QWidget *>("mapCanvas"));
        CHECK(mapCanvas && mapCanvas->brush == 40000);
        window.selectResource("TYPES.OVR");
        CHECK(window.findChild<QWidget *>("customTileEditor"));
        puts("PASS custom tile artwork, map painting, stable IDs, package/project persistence, "
             "clipboard and native UI");
        return 0;
    } catch (const std::exception &e) {
        fprintf(stderr, "FAIL exception: %s\n", e.what());
        return 1;
    }
}
