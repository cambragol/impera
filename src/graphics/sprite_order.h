#ifndef IMPERA_SPRITE_ORDER_H
#define IMPERA_SPRITE_ORDER_H
#include "../tiles.h"
#include "../5000.h"
/* Painter order shared by completed combat frames and interpolation.
 * Ground objects precede standing actors; depth is the footprint's south edge.
 * The final key keeps native lower-slot priority when footprints coincide. */
typedef struct SpriteOrder {
    int layer;
    float depth, x;
    int tie;
} SpriteOrder;
/* Preserve native furniture and transient poses without using the flattened
 * actor map, which cannot represent multiple occupants of the same cell. */
static inline int SPRITES_NativeActorTile(const ActorFmt *actor, int terrain, int north,
                                          int south) {
    int tile = actor->_1_animTile;
    if ((actor->_0_tile & 252) == TILE_ACTOR_E8 || actor->_0_tile == TILE_ACTOR_SLEEP ||
        actor->_0_tile == TILE_ACTOR_DEAD || tile == TILE_ACTOR_1D || tile == TILE_ACTOR_SLEEP)
        return tile;
    if (actor->_0_tile == TILE_ACTOR_BARD) {
        if (terrain == TILE_MAP_CHAIR_92)
            return tile;
        tile -= 8;
    }
    bool reflection;
    return ULTIMA_ResolveActorTile(tile, terrain, north, south, &reflection);
}
static inline int SPRITES_Layer(unsigned tile) {
    if (tile < 256 || tile >= 512)
        return 1;
    unsigned native = tile - 256;
    if (native > 0 && native <= TILE_ACTOR_FOOD)
        return 0;
    switch (native) {
    case TILE_ACTOR_MOONSTONE:
    case TILE_ACTOR_SLEEPING_IN_BED:
    case TILE_ACTOR_CARPET:
    case TILE_ACTOR_SLEEP:
    case TILE_ACTOR_DEAD:
    case TILE_ACTOR_SHARD:
    case TILE_ACTOR_CROWN:
    case TILE_ACTOR_SCEPTRE:
    case TILE_ACTOR_AMULET_LB:
        return 0;
    case TILE_ACTOR_1D:
        return 2;
    default:
        return 1;
    }
}
static inline int SPRITES_Compare(const void *left, const void *right) {
    const SpriteOrder *a = (const SpriteOrder *)left, *b = (const SpriteOrder *)right;
    if (a->layer != b->layer)
        return a->layer < b->layer ? -1 : 1;
    if (a->depth != b->depth)
        return a->depth < b->depth ? -1 : 1;
    if (a->x != b->x)
        return a->x < b->x ? -1 : 1;
    return (a->tie > b->tie) - (a->tie < b->tie);
}
#endif
