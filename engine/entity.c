#include "entity.h"

#include <stdlib.h>
#include "player.h"
#include "vehicles.h"
#include "location.h"

entity_t* entity_of_player(struct player_state* player) {
    entity_t* entity = malloc(sizeof(entity_t));
    entity->type = ENTITY_PLAYER;
    entity->player = player;
    return entity;
}

entity_t* entity_of_vehicle(struct vehicle_state* vehicle) {
    entity_t* entity = malloc(sizeof(entity_t));
    entity->type = ENTITY_VEHICLE;
    entity->vehicle = vehicle;
    return entity;
}

int id_of_entity(struct entity_t* entity) {
    switch (entity->type) {
        case ENTITY_PLAYER:
            return entity->player->id;
        case ENTITY_VEHICLE:
            return entity->vehicle->id;
    }
}

location location_of_entity(struct entity_t* entity) {
    switch (entity->type) {
        case ENTITY_PLAYER:
            return entity->player->location;
        case ENTITY_VEHICLE:
            return entity->vehicle->location;
    }
}

void set_location_of_entity(struct entity_t* entity, location l) {
    switch (entity->type) {
        case ENTITY_PLAYER:
            entity->player->location = l;
        case ENTITY_VEHICLE:
            entity->vehicle->location = l;
    }
}

const entity_namespace entity = {
    .of_player = &entity_of_player,
    .of_vehicle = &entity_of_vehicle,
    .get_id = &id_of_entity,
    .get_location = &location_of_entity,
    .set_location = &set_location_of_entity,
};