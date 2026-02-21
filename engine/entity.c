#include "entity.h"

#include <stdlib.h>
#include "player.h"
#include "vehicles.h"
#include "location.h"
#include "game_state.h"

entity_t* entity_of_player(struct player_state* player) {
    entity_t* entity = malloc(sizeof(entity_t));
    entity->id = _gs->id_counter++;
    entity->type = ENTITY_PLAYER;
    entity->player = player;
    entity->location = (location) { .type = VOID_LOCATION };
    player->entity = entity;
    return entity;
}

entity_t* entity_of_vehicle(struct vehicle_state* vehicle) {
    entity_t* entity = malloc(sizeof(entity_t));
    entity->id = _gs->id_counter++;
    entity->type = ENTITY_VEHICLE;
    entity->vehicle = vehicle;
    entity->location = (location) { .type = VOID_LOCATION };
    vehicle->entity = entity;
    return entity;
}
