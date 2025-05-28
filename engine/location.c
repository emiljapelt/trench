#include "location.h"

#include <stdlib.h>
#include "game_state.h"

location field_location_from_coords(int x, int y) {
    return (location) {
        .type = FIELD_LOCATION,
        .field = get_field(x,y),
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

void location_coords(location loc, int* x, int* y) {
    switch (loc.type) {
        case FIELD_LOCATION: {
            size_t diff = (size_t)loc.field - (size_t)_gs->board;
            diff /= sizeof(field_state);
            *y = diff / _gs->board_x;
            *x = diff % _gs->board_x;
            break;
        }
        case VEHICLE_LOCATION: {
            location_coords(loc.vehicle->location, x, y);
            break;
        }
    }
}

struct field_state* location_field(location loc) {
    int x, y;
    location_coords(loc, &x, &y);
    return get_field(x,y);
}


int location_equal(location loc1, location loc2) {
    int x1, y1, x2, y2;
    location_coords(loc1, &x1, &y1);
    location_coords(loc2, &x2, &y2);
    return x1 == x2 && y1 == y2;
}

int location_is(location loc, int x, int y) {
    int _x, _y;
    location_coords(loc, &_x, &_y);
    return x == _x && y == _y;
}
