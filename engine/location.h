#ifndef LOCATION_H
#define LOCATION_H

typedef enum {
    VOID_LOCATION,
    FIELD_LOCATION,
    VEHICLE_LOCATION,
} location_type;

typedef struct location {
    location_type type;
    union {
        struct field_state* field;
        struct vehicle_state* vehicle;
    };
} location;

location field_location_from_coords(int,int);
location field_location_from_field(struct field_state*);
location vehicle_location(struct vehicle_state*);

struct field_state* location_field(location);
void location_coords(location, int*, int*);
int location_equal(location, location);
int location_is(location, int, int);

#endif