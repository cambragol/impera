#include "actors.h"
#include "vars.h"
#include "contents.h"
#include "macros.h"
#include <string.h>
static unsigned encounter;
TypeDef ACTORS_Def(unsigned type) {
    TypeDef d;
    if (type >= 512 && TYPES_Get(type, &d) && d.kind)
        return d;
    if (type < 48 && TYPES_Defined(320 + type * 4) && TYPES_Get(320 + type * 4, &d) && d.kind)
        return d;
    unsigned i = type < 48 ? type : 0;
    TYPES_Vanilla(1, 64 + i * 4, &d);
    memcpy(d.stats, &D_13bc_enemyStats[i], 8);
    d.actorFlags = D_153c_enemyFlags[i];
    d.range = D_159c[i];
    d.projectileAppearance = D_15cc[i];
    d.companion = D_16d4[i];
    return d;
}
EnemyStats ACTORS_Stats(unsigned type) {
    TypeDef d = ACTORS_Def(type);
    EnemyStats result;
    memcpy(&result, d.stats, sizeof(result));
    return result;
}
unsigned ACTORS_Flags(unsigned type) { return ACTORS_Def(type).actorFlags; }
unsigned ACTORS_Range(unsigned type) { return ACTORS_Def(type).range; }
unsigned ACTORS_Projectile(unsigned type) { return ACTORS_Def(type).projectileAppearance; }
unsigned ACTORS_Profile(unsigned type) {
    return (ACTORS_Def(type).base - 64) / 4;
}
char *ACTORS_Name(unsigned type, int plural) {
    if (type < 48 && !TYPES_Defined(320 + type * 4))
        return plural ? D_18b6[type] : D_1856[type];
    static char names[4][128];
    static unsigned next;
    TypeDef d = ACTORS_Def(type);
    char *out = names[next++ % 4];
    snprintf(out, 128, "%s", plural && d.description[0] ? d.description : d.name);
    return out;
}
unsigned ACTORS_Target(unsigned type, unsigned action, unsigned fallback) {
    TypeDef d = ACTORS_Def(type);
    unsigned id = action < 4 ? d.result[action] : 0;
    return id ? (id < 512 ? ((id - 320) / 4) : id) : fallback;
}
unsigned ACTORS_Instance(unsigned actor) {
    if (actor >= 32 || !D_5c5a[actor]._0_tile)
        return 0;
    unsigned type = 0;
    if (D_5893_map_id >= 128) {
        for (unsigned i = 0; i < 32; i++)
            if ((D_ba14[i].flags & COMBAT_FLAGS_MONSTER) && D_ba14[i].actorIdx == actor) {
                type = D_ba14[i].entityIdx >= 512 ? D_ba14[i].entityIdx : 0;
                break;
            }
    } else if (!D_5893_map_id) {
        TypeCell key = {4, 0, (int8_t)D_5895_map_level, actor, 0, 0, 0};
        type = TYPES_At(key, 0);
    } else if (D_5893_map_id <= 32) {
        for (unsigned slot = 1; slot < 32; slot++)
            if (D_5f5e[slot]._0 && D_5f5e[slot]._c == actor) {
                TypeCell key = {3, D_5893_map_id, 0, slot, 0, 0, 0};
                type = TYPES_At(key, 0);
                break;
            }
    }
    if (!type) {
        unsigned original = 256 + (D_5c5a[actor]._0_tile & 252);
        if (TYPES_Defined(original))
            type = original;
    }
    TypeDef d;
    /* Corpse, invisibility and temporary native effects retain their native artwork. */
    if (!type || !TYPES_Get(type, &d) || !d.kind || (D_5c5a[actor]._0_tile & 252) != d.base)
        return 0;
    return type;
}
unsigned ACTORS_Sprite(unsigned actor, unsigned fallback) {
    unsigned type = ACTORS_Instance(actor);
    return type && D_5c5a[actor]._1_animTile ? TYPES_ActorArt(type, fallback) : fallback;
}
unsigned ACTORS_AtCell(int x, int y, unsigned fallback) {
    for (unsigned actor = 0; actor < 32; actor++) {
        ActorFmt *a = &D_5c5a[actor];
        if (a->_0_tile && a->_1_animTile && a->_2_x == x && a->_3_y == y &&
            a->_4_z == D_5895_map_level)
            return ACTORS_Sprite(actor, fallback);
    }
    return fallback;
}
unsigned ACTORS_MoveShape(unsigned actor, unsigned fallback) {
    unsigned type = ACTORS_Instance(actor);
    if (!type)
        return fallback;
    TypeDef d;
    TYPES_Get(type, &d);
    TypeDef native;
    TYPES_Vanilla(1, fallback, &native);
    if (native.moveProfile == d.moveProfile)
        return fallback;
    for (unsigned shape = 0; shape < 256; shape += 4) {
        TYPES_Vanilla(1, shape, &native);
        if (native.moveProfile == d.moveProfile)
            return shape | (fallback & 3);
    }
    return fallback;
}
void ACTORS_SetEncounter(unsigned actor) {
    encounter = ACTORS_Instance(actor);
    if (encounter < 512)
        encounter = 0;
}
unsigned ACTORS_Encounter(void) { return encounter; }
unsigned ACTORS_PlacedEncounter(unsigned slot, unsigned fallback) {
    TypeCell key = {CONTENTS_EncounterKind() + 4, CONTENTS_EncounterMap(), 0, slot, 0, 0, 0};
    return TYPES_At(key, fallback);
}

int ACTORS_Transform(unsigned entity) {
    if (entity >= 32 || !(D_ba14[entity].flags & COMBAT_FLAGS_MONSTER))
        return 0;
    CombatEntity *e = &D_ba14[entity];
    TypeDef old = ACTORS_Def(e->entityIdx);
    if (!old.result[0])
        return 0;
    unsigned target = ACTORS_Target(e->entityIdx, 0, e->entityIdx);
    TypeDef next = ACTORS_Def(target);
    e->entityIdx = target;
    e->hp = next.stats[5];
    e->dex = next.stats[1];
    e->turnTimer = 36 - e->dex;
    D_5c5a[e->actorIdx]._0_tile = D_5c5a[e->actorIdx]._1_animTile = next.base;
    return 1;
}
