#include "map_entities.h"
#include "actor_names.h"
#include "contents_editor.h"
#include "dialogue.h"
#include "location_text.h"
#include "world_objects.h"
#include <QRandomGenerator>
#include <algorithm>
#include <cstring>
using U5::require;
namespace Workshop {
QString actorName(int shape) {
    // Combat group names do not always describe the NPC appearance artwork.
    if (shape == 96)
        return "person in stocks";
    if (shape >= 64 && shape <= 252) {
        QString name = QString::fromLatin1(engineActorNames[(shape - 64) / 4]).toLower();
        // This table names combat groups; the picker places one creature at a time.
        if (name == "headlesses")
            return "headless";
        if (name.endsWith('s'))
            name.chop(1);
        return name;
    }
    return QString("Appearance %1").arg(shape);
}
QList<int> monsterShapes() {
    QList<int> out;
    for (int shape = 128; shape <= 248; shape += 4)
        if (shape != 180 && shape != 232 && shape != 236 && actorName(shape) != "x")
            out.append(shape);
    return out;
}
QString mapItemName(MapItemKind k) {
    switch (k) {
    case MapItemKind::Npc:
        return "NPC";
    case MapItemKind::Monster:
        return "Monster";
    case MapItemKind::Sign:
        return "Sign";
    case MapItemKind::Container:
        return "Chest";
    case MapItemKind::HiddenContents:
        return "Container";
    case MapItemKind::Horse:
        return "Horse";
    case MapItemKind::Skiff:
        return "Skiff";
    case MapItemKind::Frigate:
        return "Frigate";
    case MapItemKind::Carpet:
        return "Magic carpet";
    case MapItemKind::Trigger:
        return "Trigger";
    }
    return {};
}
QList<MapItemKind> availableMapItems(const Project &p, const QString &r) {
    QList<MapItemKind> out;
    if (!MapDocument::supported(r))
        return out;
    const auto npc = r.left(r.size() - 4) + ".NPC";
    if (p.resources.contains(npc))
        out << MapItemKind::Npc;
    if (r == "BRIT.DAT" || r == "UNDER.DAT" || r.endsWith(".CBT") || p.resources.contains(npc))
        out << MapItemKind::Monster;
    if (p.resources.contains("SIGNS.DAT") &&
        QStringList{"BRIT.DAT", "TOWNE.DAT", "DWELLING.DAT", "CASTLE.DAT", "KEEP.DAT"}.contains(r))
        out << MapItemKind::Sign;
    out << MapItemKind::Container << MapItemKind::HiddenContents;
    if ((r == "BRIT.DAT" || r == "UNDER.DAT") && p.resources.contains("INIT.OOL") &&
        p.resources.contains("BRIT.OOL") && p.resources.contains("UNDER.OOL"))
        out << MapItemKind::Horse << MapItemKind::Skiff << MapItemKind::Frigate
            << MapItemKind::Carpet;
    if (r == "DUNGEON.CBT")
        out << MapItemKind::Trigger;
    return out;
}
QByteArray addContentsAt(const Project &p, ContentsTarget target, QPoint cell, bool container) {
    int side = target.kind ? 11 : target.map >= 33 ? 8 : target.map ? 32 : 256;
    require(cell.x() >= 0 && cell.y() >= 0 && cell.x() < side && cell.y() < side,
            "Placement outside map");
    require(!contentsHit(p, target, cell),
            "This tile already has contents; select it to edit them");
    auto defs = readContents(p.resources.value("CONTENTS.OVR").edited);
    ContentsDef d{};
    d.kind = target.kind;
    d.map = target.map;
    d.floor = target.floor;
    d.x = cell.x();
    d.y = cell.y();
    d.slot = 255;
    d.original = 65535;
    do {
        d.id = QRandomGenerator::global()->generate() | 0x80000000u;
    } while (std::any_of(defs.begin(), defs.end(), [&](auto o) { return o.id == d.id; }));
    d.action = container;
    defs.append(d);
    return writeContents(defs, p.data("CONTENTS.OVR"));
}
QMap<QString, QByteArray> moveSignTerrain(const Project &p, const QString &r, int page, QPoint from,
                                          QPoint to) {
    MapDocument doc(&p, r);
    require(page >= 0 && page < doc.pages.size(), "Invalid sign floor");
    auto terrain = doc.terrain(page);
    int side = doc.pages[page].side;
    require(from.x() >= 0 && from.y() >= 0 && from.x() < side && from.y() < side,
            "Sign source outside map");
    require(to.x() >= 0 && to.y() >= 0 && to.x() < side && to.y() < side,
            "Sign destination outside map");
    int tile = U5::byte(terrain, from.y() * side + from.x());
    if (tile != 0x89 && tile != 0x8a && tile != 0xf8)
        return {};
    int ground = 5;
    for (QPoint delta : {QPoint(0, 1), QPoint(1, 0), QPoint(-1, 0), QPoint(0, -1)}) {
        auto n = from + delta;
        if (n.x() >= 0 && n.y() >= 0 && n.x() < side && n.y() < side) {
            int t = U5::byte(terrain, n.y() * side + n.x());
            if (t == 5 || t == 0x40 || t == 0x44 || t == 0x45) {
                ground = t;
                break;
            }
        }
    }
    terrain[from.y() * side + from.x()] = char(ground);
    if (to != from)
        terrain[to.y() * side + to.x()] = char(tile);
    return doc.changes(page, terrain);
}
static int freeWorldSlot(const Project &p, int world) {
    auto b = WorldObjects::effective(p, world);
    // OUTSUBS_0566 uses Underworld slots 28–31 for the amulet and shards.
    for (int slot = 1; slot < (world ? 28 : 32); ++slot)
        if (!U5::byte(b, slot * 8))
            return slot;
    require(false, "No free world object slots remain");
    return -1;
}
MapItemEdit addMapItem(const Project &p, const QString &r, int index, QPoint cell, MapItemKind kind,
                       int appearance, int conversation, const QString &name) {
    require(availableMapItems(p, r).contains(kind), "This item is not supported on this map");
    MapDocument doc(&p, r);
    require(index >= 0 && index < doc.pages.size(), "Invalid map floor");
    const auto page = doc.pages[index];
    require(cell.x() >= 0 && cell.y() >= 0 && cell.x() < page.side && cell.y() < page.side,
            "Placement outside map");
    MapItemEdit out;
    auto target = contentsTarget(r, page, index);
    if (kind == MapItemKind::Npc ||
        (kind == MapItemKind::Monster && p.resources.contains(r.left(r.size() - 4) + ".NPC"))) {
        const auto npc = r.left(r.size() - 4) + ".NPC", talk = r.left(r.size() - 4) + ".TLK";
        auto b = p.data(npc);
        require(b.size() == 4608 && page.settlement >= 0, "No valid NPC records on this map");
        int start = page.settlement * 576;
        for (int i = 1; i < 32; i++)
            if (!U5::byte(b, start + 512 + i)) {
                out.slot = i;
                break;
            }
        require(out.slot > 0, "This location has no free NPC slots");
        require(appearance == -1 ||
                    (kind == MapItemKind::Monster
                         ? monsterShapes().contains(appearance)
                         : (appearance >= 64 && appearance < 128 && appearance % 4 == 0)),
                "Choose a supported appearance");
        if (kind == MapItemKind::Monster)
            conversation = 0;
        else if (conversation < 0) {
            require(p.resources.contains(talk), "Conversation resource unavailable");
            auto entries = U5::readDialogue(p.data(talk));
            QSet<int> used;
            for (auto e : entries)
                used.insert(e.id);
            for (int town = 0; town < 8; town++)
                for (int i = 0; i < 32; i++)
                    used.insert(U5::byte(b, town * 576 + 544 + i));
            for (int id = 1; id < 128; id++)
                if (!used.contains(id)) {
                    conversation = id;
                    break;
                }
            require(conversation > 0, "No free conversation numbers remain");
            auto bytes = Dialogue::text(name);
            bytes +=
                U5::encodeText("<Entry>A traveler.<Entry>Greetings.<Entry>I live "
                               "here.<Entry>Farewell.<End Conversation><Entry><Any><Label 15>@");
            require(Dialogue::parse(bytes).structuralError.isEmpty(), "Invalid new conversation");
            entries.append({unsigned(conversation), bytes});
            out.changes[talk] = U5::writeDialogue(entries);
        } else {
            require(conversation > 0 && conversation < 128, "Choose an editable conversation");
            bool found = false;
            for (auto e : U5::readDialogue(p.data(talk)))
                found |= int(e.id) == conversation;
            require(found, "Conversation unavailable");
        }
        int base = start + out.slot * 16;
        b.replace(base, 16, QByteArray(16, 0));
        for (int loc = 0; loc < 3; loc++) {
            b[base + loc] = char(kind == MapItemKind::Monster ? 5 : 0);
            b[base + 3 + loc] = char(cell.x());
            b[base + 6 + loc] = char(cell.y());
            b[base + 9 + loc] = char(page.floor);
        }
        for (int h = 0; h < 4; h++)
            b[base + 12 + h] = char(h * 6);
        b[start + 512 + out.slot] =
            char(appearance < 0 ? (kind == MapItemKind::Monster ? 144 : 80) : appearance);
        b[start + 544 + out.slot] = char(conversation);
        out.changes[npc] = b;
        out.mode = "NPCs / Schedules";
        if (p.resources.contains("INIT.GAM")) {
            auto initial = p.data("INIT.GAM");
            int flag = 0x5b4 + (target.map - 1) * 4 + out.slot / 8;
            require(flag < initial.size(), "Initial NPC state is truncated");
            initial[flag] = char(U5::byte(initial, flag) & ~(1u << (out.slot % 8)));
            if (initial != p.data("INIT.GAM"))
                out.changes["INIT.GAM"] = initial;
        }
    } else if (kind == MapItemKind::Sign) {
        require(page.floor <= 3, "Signs support basement through upper floor 3");
        auto signs = readSigns(p.data("SIGNS.DAT"));
        for (auto s : signs)
            require(s.map != target.map || s.floor != target.floor || s.x != cell.x() ||
                        s.y != cell.y(),
                    "A sign already exists here");
        signs.append({target.map, target.floor, cell.x(), cell.y(), encodeSign("New sign")});
        out.changes["SIGNS.DAT"] = writeSigns(p.data("SIGNS.DAT"), signs);
        auto terrain = doc.terrain(index);
        terrain[cell.y() * page.side + cell.x()] = char(0xf8);
        auto changes = doc.changes(index, terrain);
        for (auto it = changes.begin(); it != changes.end(); ++it)
            out.changes[it.key()] = it.value();
        out.slot = signs.size() - 1;
        out.mode = "Signs";
    } else if (kind == MapItemKind::Container || kind == MapItemKind::HiddenContents) {
        out.changes["CONTENTS.OVR"] =
            addContentsAt(p, target, cell, kind == MapItemKind::Container);
        out.mode = "Contents";
    } else if (r.endsWith(".CBT")) {
        auto b = p.data(r);
        int base = page.offset;
        if (kind == MapItemKind::Trigger) {
            for (int i = 0; i < 8; i++)
                if (U5::byte(b, base + 8 * 32 + 11 + i) > 10 ||
                    U5::byte(b, base + 8 * 32 + 19 + i) > 10) {
                    out.slot = 22 + i;
                    break;
                }
            require(out.slot >= 22, "No free trigger slots remain");
            int i = out.slot - 22;
            b[base + 8 * 32 + 11 + i] = char(cell.x());
            b[base + 8 * 32 + 19 + i] = char(cell.y());
            b[base + 11 + i] = char(5);
            for (int row : {9, 10}) {
                b[base + row * 32 + 11 + i] = char(cell.x());
                b[base + row * 32 + 19 + i] = char(cell.y());
            }
        } else {
            for (int i = 0; i < 16; i++)
                if (r == "DUNGEON.CBT" ? !U5::byte(b, base + 5 * 32 + 11 + i)
                                       : (U5::byte(b, base + 6 * 32 + 11 + i) > 10 ||
                                          U5::byte(b, base + 7 * 32 + 11 + i) > 10)) {
                    out.slot = 6 + i;
                    break;
                }
            require(out.slot >= 6, "No free monster positions remain");
            int i = out.slot - 6;
            require(appearance < 0 || monsterShapes().contains(appearance),
                    "Choose a supported monster");
            if (r == "DUNGEON.CBT")
                b[base + 5 * 32 + 11 + i] = char(appearance < 0 ? 144 : appearance);
            b[base + 6 * 32 + 11 + i] = char(cell.x());
            b[base + 7 * 32 + 11 + i] = char(cell.y());
        }
        out.changes[r] = b;
        out.mode = "Encounter";
    } else {
        int world = r == "UNDER.DAT";
        out.slot = freeWorldSlot(p, world);
        QByteArray record(8, 0);
        int shape = kind == MapItemKind::Horse     ? 16
                    : kind == MapItemKind::Skiff   ? 40
                    : kind == MapItemKind::Frigate ? 32
                    : kind == MapItemKind::Carpet  ? 27
                                                   : (appearance < 0 ? 144 : appearance);
        require(kind != MapItemKind::Monster || monsterShapes().contains(shape),
                "Choose a supported monster");
        record[0] = record[1] = char(shape);
        record[2] = char(cell.x());
        record[3] = char(cell.y());
        record[4] = char(world ? 255 : 0);
        record[5] = char(kind == MapItemKind::Frigate ? 99 : 0);
        record[7] = char(255);
        out.changes[WorldObjects::resource(world)] =
            WorldObjects::change(p, world, out.slot, record);
        out.mode = "Starting Objects";
    }
    Project candidate = p;
    for (auto it = out.changes.begin(); it != out.changes.end(); ++it) {
        candidate.resources[it.key()].edited = it.value();
        if (it.key().endsWith(".OOL"))
            candidate.resources[it.key()].included = true;
    }
    candidate.validate();
    return out;
}
QMap<QString, QByteArray> deleteMapItem(const Project &p, const QString &r, int index,
                                        const QString &mode, int slot, QPoint cell) {
    MapDocument doc(&p, r);
    const auto page = doc.pages.at(index);
    QMap<QString, QByteArray> out;
    if (mode == "NPCs / Schedules") {
        require(slot > 0 && slot < 32, "Select an NPC");
        auto npc = r.left(r.size() - 4) + ".NPC";
        auto b = p.data(npc);
        int base = page.settlement * 576;
        b.replace(base + slot * 16, 16, QByteArray(16, 0));
        b[base + 512 + slot] = b[base + 544 + slot] = 0;
        out[npc] = b;
        // Conversations can be shared: deleting an NPC never deletes their
        // conversation.
    } else if (mode == "Encounter") {
        require(slot >= 6 && slot < 30, "Party entry points cannot be deleted");
        auto b = p.data(r);
        int base = page.offset;
        if (slot < 22) {
            int i = slot - 6;
            b[base + 5 * 32 + 11 + i] = 0;
            b[base + 6 * 32 + 11 + i] = b[base + 7 * 32 + 11 + i] = char(255);
        } else {
            int i = slot - 22;
            b[base + 8 * 32 + 11 + i] = b[base + 8 * 32 + 19 + i] = char(255);
        }
        out[r] = b;
    } else if (mode == "Signs") {
        auto signs = readSigns(p.data("SIGNS.DAT"));
        auto target = contentsTarget(r, page, index);
        bool removed = false;
        for (int i = signs.size() - 1; i >= 0; i--)
            if (signs[i].map == target.map && signs[i].floor == target.floor &&
                signs[i].x == cell.x() && signs[i].y == cell.y()) {
                signs.removeAt(i);
                removed = true;
            }
        require(removed, "Select an editable sign");
        out["SIGNS.DAT"] = writeSigns(p.data("SIGNS.DAT"), signs);
        auto changes = moveSignTerrain(p, r, index, cell, cell);
        for (auto it = changes.begin(); it != changes.end(); ++it)
            out[it.key()] = it.value();
    } else if (mode == "Starting Objects" || mode == "Contents") {
        auto target = contentsTarget(r, page, index);
        auto defs = readContents(p.data("CONTENTS.OVR"));
        if (mode == "Starting Objects") {
            int world = r == "UNDER.DAT";
            require(slot > 0 && slot < (world ? 28 : 32), "Select an editable world object");
            auto record = WorldObjects::effective(p, world).mid(slot * 8, 8);
            record[0] = record[1] = 0;
            out[WorldObjects::resource(world)] = WorldObjects::change(p, world, slot, record);
            for (int i = defs.size() - 1; i >= 0; i--)
                if (!defs[i].kind && !defs[i].map && defs[i].floor == target.floor &&
                    defs[i].slot == slot)
                    defs.removeAt(i);
        } else {
            ContentsDef selected{};
            for (auto d : mapContents(p, target))
                if (d.x == cell.x() && d.y == cell.y())
                    selected = d;
            require(selected.id, "Select contents to delete");
            for (int i = defs.size() - 1; i >= 0; i--)
                if (defs[i].id == selected.id)
                    defs.removeAt(i);
            if (selected.slot != 255) {
                int world = selected.floor < 0;
                auto record = WorldObjects::effective(p, world).mid(selected.slot * 8, 8);
                record[0] = record[1] = 0;
                out[WorldObjects::resource(world)] =
                    WorldObjects::change(p, world, selected.slot, record);
            } else if (selected.original != 65535) {
                selected.count = 0;
                selected.flags = 0;
                selected.refill = 0;
                memset(selected.items, 0, sizeof(selected.items));
                defs.append(selected); // Suppress inherited rewards instead of restoring them.
            }
        }
        out["CONTENTS.OVR"] = writeContents(defs, p.data("CONTENTS.OVR"));
    } else
        require(false, "Select a map item to delete");
    return out;
}
} // namespace Workshop
