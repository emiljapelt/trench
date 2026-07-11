#include "location.h"

#include <stdlib.h>
#include "game_state.h"

location field_location_from_coords(int x, int y) {
    return (location) {
        .type = FIELD_LOCATION,
        .field = fields.get(x,y),
    };
}

location field_location_from_field(struct field_state* field) {
    return (location) {
        .type = FIELD_LOCATION,
        .field = field,
    };
}

location vehicle_location(struct vehicle_state* vehicle) {
    return (location) {
        .type = VEHICLE_LOCATION,
        .vehicle = vehicle,
    };
}

int location_coords(location loc, int* x, int* y) {
    switch (loc.type) {
        case FIELD_LOCATION: {
            size_t diff = (size_t)loc.field - (size_t)_gs->map;
            diff /= sizeof(field_state);
            *y = diff / _gs->map_width;
            *x = diff % _gs->map_width;
            return 1;
        }
        case VEHICLE_LOCATION: {
            return location_coords(loc.vehicle->entity->location, x, y);
        }
        case VOID_LOCATION: {
            return 0;
        }
    }

    return 0;
}

struct field_state* location_field(location loc) {
    int x, y;
    if (location_coords(loc, &x, &y))
        return fields.get(x,y);
    else 
        return NULL;
}

// NOT USED, but is the void equal to itself
int location_equal(location loc1, location loc2) {
    int x1, y1, x2, y2;
    if (
        location_coords(loc1, &x1, &y1) &&
        location_coords(loc2, &x2, &y2)
    )
        return x1 == x2 && y1 == y2;
    else 
        return 0;
}