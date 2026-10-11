#include "world_objects.h"
#include "../src/tiles.h"
#include <QtWidgets>
#include <memory>
using U5::require;
namespace Workshop {
QString WorldObjects::resource(int world) {
    require(world == 0 || world == 1, "Invalid world");
    return world == 0 ? "BRIT.OOL" : "UNDER.OOL";
}
bool WorldObjects::overridden(const Project &p, int world) {
    auto it = p.resources.constFind(resource(world));
    return it != p.resources.cend() && (it->included || it->edited != it->original);
}
QByteArray WorldObjects::effective(const Project &p, int world) {
    QByteArray out = overridden(p, world) ? p.data(resource(world))
                     : world == 0         ? QByteArray(256, 0)
                                          : p.data("INIT.OOL");
    require(out.size() == 256, "World objects need 32 eight-byte records");
    return out;
}
QByteArray WorldObjects::change(const Project &p, int world, int slot, const QByteArray &record) {
    require(slot > 0 && slot < 32 && record.size() == 8,
            "Choose an available object slot (slot 0 belongs to the party)");
    auto bytes = effective(p, world);
    auto original = bytes;
    bytes.replace(slot * 8, 8, record);
    validate(resource(world), bytes, original);
    return bytes;
}
void WorldObjects::validate(const QString &name, const QByteArray &b, const QByteArray &original) {
    require(b.size() == 256, "World objects must contain 32 eight-byte records");
    for (int i = 0; i < 32; ++i) {
        auto r = b.mid(i * 8, 8);
        if (r == original.mid(i * 8, 8) || !uchar(r[0]))
            continue;
        require(i > 0, "World object slot 0 is reserved for the active party");
        if (!QList<int>{1,  2,  15, 16, 17, 25, 27, 30, 32, 33,
                        34, 35, 36, 37, 38, 39, 40, 41, 42, 43}
                 .contains(uchar(r[0])))
            continue; // Unknown object types are retained, with diagnostics rather than invented
                      // semantics.
        require(uchar(r[4]) == (name == "BRIT.OOL" ? 0 : 255), "Object floor must match its world");
        // Every byte-sized coordinate is valid on a 256×256 world.
        int tile = uchar(r[0]), anim = uchar(r[1]);
        require(anim == tile || ((tile & 0xfc) == (anim & 0xfc) && tile >= 0x20),
                "Animated shape must match the object's tile family");
        if ((tile & 0xf8) == TILE_ACTOR_FRIGATE_20)
            require(uchar(r[5]) <= 100, "Ship hull condition must be between 0 and 100");
    }
}
static QString objectName(int shape) {
    if (shape == TILE_ACTOR_CHEST)
        return "Chest";
    if (shape == TILE_ACTOR_GOLD)
        return "Gold";
    if (shape == TILE_ACTOR_FOOD)
        return "Food";
    if (shape == TILE_ACTOR_HORSE || shape == TILE_ACTOR_HORSE + 1)
        return "Horse";
    if (shape == TILE_ACTOR_MOONSTONE)
        return "Moonstone";
    if (shape == TILE_ACTOR_CARPET)
        return "Magic carpet";
    if (shape == TILE_ACTOR_SLEEP)
        return "Sleeping figure";
    if ((shape & 0xf8) == TILE_ACTOR_FRIGATE_20)
        return "Frigate";
    if (shape >= TILE_ACTOR_SKIFF && shape <= TILE_ACTOR_SKIFF + 3)
        return "Skiff";
    return QString("World object (shape %1)").arg(shape);
}
QWidget *worldObjectEditor(Project *project, int initialWorld, ObjectCommit commit,
                           QMap<QString, int> *state, std::function<bool()> *flush,
                           MapCanvas *sharedCanvas, std::function<void(QPoint)> locate) {
    auto owner = std::make_unique<QWidget>();
    auto root = owner.get();
    root->setObjectName("worldObjectEditor");
    auto layout = new QVBoxLayout(root);
    auto toolbar = new QHBoxLayout;
    auto world = new QComboBox;
    world->setObjectName("objectWorld");
    world->addItems({"Britannia", "Underworld"});
    world->setCurrentIndex(initialWorld);
    world->setToolTip(
        "Edit the effective objects for a fresh game in this world. Existing saves are unchanged.");
    toolbar->addWidget(world);
    if (sharedCanvas)
        world->hide();
    auto add = new QPushButton("Add object");
    add->setObjectName("addWorldObject");
    add->setToolTip("Use the first empty world slot. Slot 0 is reserved for the active party.");
    toolbar->addWidget(add);
    auto remove = new QPushButton("Remove object");
    remove->setObjectName("removeWorldObject");
    remove->setToolTip("Empty the selected object slot; unrelated records are preserved.");
    toolbar->addWidget(remove);
    auto revert = new QPushButton("Revert to inherited");
    revert->setObjectName("revertWorldObjects");
    revert->setToolTip("Undoably remove this world's explicit override: Britannia becomes empty; "
                       "Underworld uses INIT.OOL.");
    if (!sharedCanvas)
        toolbar->addWidget(revert);
    toolbar->addStretch();
    layout->addLayout(toolbar);
    if (sharedCanvas)
        layout->addWidget(revert);
    auto status = new QLabel;
    status->setObjectName("objectProvenance");
    status->setWordWrap(true);
    layout->addWidget(status);
    auto split = new QSplitter;
    auto list = new QListWidget;
    list->setObjectName("worldObjectList");
    list->setToolTip(
        "Starting world objects, not settlement NPCs. Select an object to inspect and place it.");
    split->addWidget(list);
    auto canvas = sharedCanvas ? sharedCanvas : new MapCanvas;
    QScrollArea *view = nullptr;
    if (!sharedCanvas) {
        canvas->setObjectName("worldObjectMap");
        canvas->setToolTip(
            "Drag a selected object to place it. Map terrain is read-only in this layer.");
        canvas->side = canvas->rows = 256;
        canvas->zoom = 0.5;
        canvas->tiles = U5::readGraphics("TILES.16", project->data("TILES.16")).images;
        view = new QScrollArea;
        view->setObjectName("canvasView");
        view->setWidget(canvas);
        split->addWidget(view);
        split->setStretchFactor(1, 1);
    } else {
        split->setOrientation(Qt::Vertical);
    }
    canvas->tool = MapCanvas::InspectNpc;
    canvas->terrainEditingEnabled = false;
    canvas->ghosts.clear();
    auto inspector = new QWidget;
    auto il = new QFormLayout(inspector);
    auto type = new QComboBox;
    type->setObjectName("worldObjectType");
    for (int tile : {1, 2, 15, 16, 17, 25, 27, 30, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43})
        type->addItem(objectName(tile), tile);
    type->setToolTip("Choose a verified world object shape. Other inherited shapes are retained, "
                     "with unsupported properties read-only.");
    il->addRow("Object", type);
    auto x = new QSpinBox;
    auto y = new QSpinBox;
    x->setRange(0, 255);
    y->setRange(0, 255);
    x->setObjectName("worldObjectX");
    y->setObjectName("worldObjectY");
    x->setToolTip("World map column, 0–255.");
    y->setToolTip("World map row, 0–255.");
    il->addRow("Column", x);
    il->addRow("Row", y);
    auto hull = new QSpinBox;
    hull->setObjectName("shipHull");
    hull->setRange(0, 100);
    hull->setToolTip("Frigate hull condition. 99 is the engine's usual initial condition; rough "
                     "seas and combat can damage it.");
    il->addRow("Ship condition", hull);
    auto details = new QLabel;
    details->setWordWrap(true);
    details->setToolTip("Animation frame and type-specific bytes are preserved. Unverified "
                        "properties are intentionally read-only.");
    il->addRow("Advanced details", details);
    split->addWidget(inspector);
    layout->addWidget(split, 1);
    auto loading = std::make_shared<bool>(false);
    auto load = std::make_shared<std::function<void()>>();
    auto selection = [=] {
        int slot = list->currentItem() ? list->currentItem()->data(Qt::UserRole).toInt() : -1;
        *loading = true;
        auto b = WorldObjects::effective(*project, world->currentIndex());
        bool selected = slot > 0;
        inspector->setEnabled(selected);
        remove->setEnabled(selected);
        canvas->selectedNpc = slot;
        if (selected) {
            (*state)["objects/slot"] = slot;
            auto r = b.mid(slot * 8, 8);
            int tile = uchar(r[0]);
            int index = type->findData(tile);
            if (index < 0) {
                type->addItem(QString("Unusual shape %1 (preserved)").arg(tile), tile);
                index = type->count() - 1;
            }
            type->setCurrentIndex(index);
            x->setValue(uchar(r[2]));
            y->setValue(uchar(r[3]));
            hull->setEnabled((tile & 0xf8) == 0x20);
            hull->setValue(qMin(100, int(uchar(r[5]))));
            details->setText(
                QString("Slot %1 · %2\nAnimation: %3\nProperty bytes: %4 %5 %6\nFloor: %7")
                    .arg(slot)
                    .arg(WorldObjects::overridden(*project, world->currentIndex())
                             ? "World override"
                         : world->currentIndex() == 0 ? "Normal start"
                                                      : "Initial underworld objects")
                    .arg(uchar(r[1]))
                    .arg(uchar(r[5]))
                    .arg(uchar(r[6]))
                    .arg(uchar(r[7]))
                    .arg(world->currentIndex() == 0 ? "Surface" : "Underworld"));
            if (view)
                view->ensureVisible(uchar(r[2]) * 8, uchar(r[3]) * 8, 80, 80);
        }
        canvas->update();
        *loading = false;
    };
    *load = [=] {
        *loading = true;
        int which = world->currentIndex(), selected = state->value("objects/slot", 1);
        auto b = WorldObjects::effective(*project, which);
        list->clear();
        canvas->actors.clear();
        int count = 0;
        for (int slot = 1; slot < 32; ++slot) {
            auto r = b.mid(slot * 8, 8);
            int tile = uchar(r[0]);
            if (!tile)
                continue;
            ++count;
            QString label = objectName(tile);
            auto item = new QListWidgetItem(
                QIcon(QPixmap::fromImage(canvas->tiles.value(256 + tile))),
                QString("%1 · (%2,%3)").arg(label).arg(uchar(r[2])).arg(uchar(r[3])), list);
            item->setData(Qt::UserRole, slot);
            item->setToolTip(QString("World slot %1 · %2").arg(slot).arg(label));
            canvas->actors.append({int(uchar(r[2])), int(uchar(r[3])), 256 + tile, slot, label});
            if (slot == selected)
                list->setCurrentItem(item);
        }
        if (!list->currentItem() && list->count())
            list->setCurrentRow(0);
        const QString map = which == 0 ? "BRIT.DAT" : "UNDER.DAT";
        if (project->resources.contains(map)) {
            auto overlay =
                project->resources.contains("DATA.OVL") ? project->data("DATA.OVL") : QByteArray();
            canvas->ids = U5::worldMap(map, project->data(map), overlay);
        } else
            canvas->ids = QByteArray(65536, 0);
        if (!sharedCanvas)
            canvas->original = canvas->ids;
        canvas->resizeMap();
        if (sharedCanvas) {
            status->setText(QString("New-game objects · %1 of 31 slots used\n%2")
                                .arg(count)
                                .arg(WorldObjects::overridden(*project, which)
                                         ? "Customized for this world"
                                     : which == 0 ? "Normal start: Britannia begins without objects"
                                                  : "Inherited initial Underworld objects"));
        } else
            status->setText(
                QString("Starting world objects · %1 of 31 object slots used (%2 "
                        "available) · 32 fixed records, including reserved party slot\n%3")
                    .arg(count)
                    .arg(31 - count)
                    .arg(WorldObjects::overridden(*project, which) ? "World override"
                         : which == 0 ? "Normal start — empty Britannia list"
                                      : "Initial underworld objects — inherited from INIT.OOL"));
        revert->setEnabled(WorldObjects::overridden(*project, which));
        add->setEnabled(count < 31);
        *loading = false;
        selection();
    };
    auto write = [=](int slot, const QByteArray &r) {
        try {
            int which = world->currentIndex();
            auto bytes = WorldObjects::change(*project, which, slot, r);
            if (!commit(WorldObjects::resource(which), bytes, true))
                return false;
            (*state)["objects/slot"] = slot;
            (*load)();
            return true;
        } catch (const std::exception &error) {
            QMessageBox::warning(root, "Cannot change starting object", error.what());
            (*load)();
            return false;
        }
    };
    auto change = [=] {
        if (*loading || !list->currentItem())
            return;
        int slot = list->currentItem()->data(Qt::UserRole).toInt();
        auto r = WorldObjects::effective(*project, world->currentIndex()).mid(slot * 8, 8);
        int tile = type->currentData().toInt();
        if (tile != uchar(r[0])) {
            r[0] = r[1] = char(tile);
            if ((tile & 0xf8) == 0x20)
                r[5] = 99;
        }
        r[2] = char(x->value());
        r[3] = char(y->value());
        r[4] = world->currentIndex() == 0 ? 0 : char(255);
        if ((tile & 0xf8) == 0x20 &&
            tile == uchar(WorldObjects::effective(*project, world->currentIndex())[slot * 8]))
            r[5] = char(hull->value());
        write(slot, r);
    };
    QObject::connect(list, &QListWidget::currentRowChanged, root, [=](int) {
        if (!*loading)
            selection();
    });
    QObject::connect(world, qOverload<int>(&QComboBox::currentIndexChanged), root, [=](int i) {
        (*state)["objects/world"] = i;
        (*load)();
    });
    QObject::connect(type, qOverload<int>(&QComboBox::currentIndexChanged), root,
                     [=](int) { change(); });
    for (auto spin : {x, y, hull})
        QObject::connect(spin, qOverload<int>(&QSpinBox::valueChanged), root,
                         [=](int) { change(); });
    QObject::connect(add, &QPushButton::clicked, root, [=] {
        auto b = WorldObjects::effective(*project, world->currentIndex());
        for (int slot = 1; slot < 32; ++slot)
            if (!uchar(b[slot * 8])) {
                QByteArray r(8, 0);
                r[0] = r[1] = 36;
                r[4] = world->currentIndex() == 0 ? 0 : char(255);
                r[5] = 99;
                if (sharedCanvas) {
                    r[2] = char(canvas->keyboardCell.x());
                    r[3] = char(canvas->keyboardCell.y());
                }
                write(slot, r);
                return;
            }
    });
    QObject::connect(remove, &QPushButton::clicked, root, [=] {
        if (list->currentItem()) {
            int slot = list->currentItem()->data(Qt::UserRole).toInt();
            auto r = WorldObjects::effective(*project, world->currentIndex()).mid(slot * 8, 8);
            r[0] = r[1] = 0;
            write(slot, r);
        }
    });
    QObject::connect(revert, &QPushButton::clicked, root, [=] {
        auto name = WorldObjects::resource(world->currentIndex());
        if (commit(name, project->resources[name].original, false))
            (*load)();
    });
    if (sharedCanvas && locate) {
        auto show = new QPushButton("Show selected object");
        show->setToolTip("Center the shared map on the selected starting object.");
        layout->addWidget(show);
        QObject::connect(show, &QPushButton::clicked, root, [=] {
            if (list->currentItem())
                locate({x->value(), y->value()});
        });
    }
    canvas->chooseNpc = [=](int slot) {
        for (int i = 0; i < list->count(); ++i)
            if (list->item(i)->data(Qt::UserRole).toInt() == slot) {
                list->setCurrentRow(i);
                break;
            }
    };
    canvas->moveNpc = [=](int slot, QPoint p) {
        auto r = WorldObjects::effective(*project, world->currentIndex()).mid(slot * 8, 8);
        r[2] = char(p.x());
        r[3] = char(p.y());
        return write(slot, r);
    };
    *flush = [=] {
        canvas->cancelStroke();
        return true;
    };
    (*load)();
    return owner.release();
}
} // namespace Workshop
