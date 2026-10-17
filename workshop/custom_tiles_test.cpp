#include "custom_tiles.h"
#include "window.h"
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
        ui->findChild<QPushButton *>("applyCustomTile")->click();
        CHECK(edits == 1);
        CHECK(QString::fromUtf8(CustomTiles::definition(p, 40000).name) == "Polished crystal");
        WorkshopWindow window;
        CHECK(window.openGame(TEST_GAME_DIR));
        window.projectForTests() = p;
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
