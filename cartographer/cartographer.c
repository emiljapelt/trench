#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>

#include "../engine/util.h"
#include "../engine/symbols.h"
#include "../engine/color.h"

// https://rtouti.github.io/graphics/perlin-noise-algorithm

int view_width;
int view_height;
const float zoom_speed = 0.01;
const float move_speed = 1;
const float limit_move = 0.01;

typedef struct vector_2  {
    float x;
    float y;
} vector_2;

vector_2 vec2(float x, float y) {
    return (vector_2) { .x = x, .y = y };
}

float dot(vector_2 a, vector_2 b) {
    return a.x * b.x + a.y * b.y;
}

#define perm_size 256
typedef struct layer {
    unsigned char data[perm_size * 2];
    float z;
    float x;
    float y;
} layer;

void shuffle(unsigned char* array, int len) {
    for(int i = len - 1; i > 0; i--) {
        int index = rand() % (len + 1);
        unsigned char temp = array[i];

        array[i] = array[index];
        array[index] = temp;
    }
}

layer make_layer(float z, char p) {
    layer l;

    l.z = z;
    l.x = 0;
    l.y = 0;

    if (p) {
        int i;
        for(i = 0; i < perm_size; i++)
            l.data[i] = i;

        shuffle(l.data, perm_size);

        for(i = 0; i < perm_size; i++)
            l.data[perm_size + i] = l.data[i];
    }

    return l;
}

vector_2 constant_vector(int v) {
    switch (v & 3) {
        case 0: return vec2(1.0, 1.0);
        case 1: return vec2(-1.0, 1.0);
        case 2: return vec2(-1.0, -1.0);
        default: return vec2(1.0, -1.0);
    }
}

float gradient_vector_2d(unsigned int hash, float x, float y) {
  unsigned int h = hash & 3;
  if (h == 0) {
    return  x;
  }

  if (h == 1) {
    return -x;
  }

  if (h == 2) {
    return  y;
  }
  
  return -y;
}

float fade(float t) {
    return ((6*t - 15)*t + 10)*t*t*t;
}

float lerp(float t, float a, float b) {
    return a + t*(b-a);
}


float noise_value(layer l, float x, float y) {
    float fx = floorf(x);
    float fy = floorf(y);

    unsigned int xu = (unsigned int)fx % perm_size;
    unsigned int yu = (unsigned int)fy % perm_size;

    x -= fx;
    y -= fy;

    unsigned int a = l.data[xu] + yu;
    unsigned int b = l.data[xu + 1] + yu;

    float u = fade(x);
    float v = fade(y);
    
    float n = lerp(v, lerp(u, gradient_vector_2d(l.data[a    ], x    , y    ),
                                gradient_vector_2d(l.data[b    ], x - 1, y    )),
                        lerp(u, gradient_vector_2d(l.data[a + 1], x    , y - 1),
                                gradient_vector_2d(l.data[b + 1], x - 1, y - 1)));
}

float fractal_brownian_noise_value(layer map, int x, int y, int numOctaves) {
	float result = 0.0;
	float amplitude = 1.0;
	float frequency = 0.05;

	while(numOctaves--) {
		float n = amplitude * noise_value(map, x * frequency, y * frequency);
		result += n;
		
		amplitude *= 0.5;
		frequency *= 2.0;
	}

	return result;
}

float fractal_brownian_layer_value(layer l, float x, float y, int ocataves) {
    float n = fractal_brownian_noise_value(
        l,
        ((x - (view_width/2.0f)) * l.z) + l.x,
        ((y - (view_height/2.0f)) * l.z) + l.y,
        8
    );

    return (n + 1.0) / 2.0;
}

float layer_value(layer l, float x, float y) {
    float n = noise_value(
        l,
        ((x - (view_width/2.0f)) * l.z) + l.x,
        ((y - (view_height/2.0f)) * l.z) + l.y
    );

    return (n + 1.0) / 2.0;
}

layer height_layer;
layer biome_layer;
layer feature_layer;

enum layers {
    HEIGHT =    0b000001,
    BIOME =     0b000010,
    FEATURE =   0b000100,

    ALL_LAYERS =       0b000111,
};

float ocean_land_limit = 0.5;
float land_mountain_limit = 0.84;

enum limits {
    OCEAN_LAND_LIMIT =      0b000001,
    LAND_MOUNTAIN_LIMIT =   0b000010,

    ALL_LIMITS =            0b000011,
};

unsigned int active_limits = 0;
unsigned int active_layers = ALL_LAYERS;

char* map;

char* out = NULL;

void generate() {
    int i = 0;
    for(int y = 0; y < view_height; y++) {
        for(int x = 0; x < view_width; x++) {
            float n = fractal_brownian_layer_value(height_layer, x, y, 8);
            char c = '?';

            if (n < ocean_land_limit) {
                c = TRM_OCEAN;
            }
            else if (n < land_mountain_limit) {
                if (fractal_brownian_layer_value(biome_layer, x, y, 4) < 0.4 && layer_value(feature_layer, x, y) < 0.5) {
                    c = TRM_TREE;
                }
                else if (fractal_brownian_layer_value(biome_layer, x, y, 4) < 0.6 && layer_value(feature_layer, x, y) < 0.22) {
                    c = TRM_CLAY;
                }
                else {
                    c = TRM_EMPTY;
                }
            }
            else {    
                c = TRM_MOUNTAIN;
            }

            map[i++] = c;
        }
        map[i++] = '\n';
    }
}

void move(layer* l, int x, int y) {
    l->x += x * move_speed * l->z;
    l->y += y * move_speed * l->z;
}

void zoom(layer* l, float a) {
    l->z += a;
    if (l->z < 0) l->z = 0;
}

void move_limit(float* limit, float d) {
    *limit = clampf(*limit + d * limit_move, 1, 0);
}

void to_terminal() {
    int i = 0;
    char c = map[i];
    color fclr = color_lookup[DARK_GREY];
    color bclr = color_lookup[BLACK];
    const char* s = NULL;
    while(c != 0) {
        
        switch (c) {
            case '\n':
                printf(_TERM_PRINT_RESET); 
                putchar('\n'); 
                goto end;
            case 'T': 
                fclr = color_lookup[GREEN];
                bclr = color_lookup[BLACK];
                s = symbol_lookup[TREE_VISUAL];
                break;
            case '~': 
                fclr = color_lookup[ICE_BLUE];
                bclr = color_lookup[BLUE];
                s = "~";
                break;
            case '.': 
                fclr = color_lookup[DARK_GREY];
                bclr = color_lookup[BLACK];
                s = symbol_lookup[MIDDOT];
                break;
            case 'M':
                fclr = color_lookup[LIGHT_GREY];
                bclr = color_lookup[BLACK];
                s = symbol_lookup[MOUNTAIN_VISUAL];
                break;
            case 'C': 
                fclr = color_lookup[CLAY_BROWN];
                bclr = color_lookup[WOOD_BROWN];
                s = " ";
                break;
            default:
                printf("%s?\n" _TERM_PRINT_RESET);
                goto end;
        }

        printf(_TERM_PRINT_COLOR, FORE, fclr.r, fclr.g, fclr.b);
        printf(_TERM_PRINT_COLOR, BACK, bclr.r, bclr.g, bclr.b);
        printf("%s", s);

        end:
        c = map[++i];
    }
}

void to_file() {
    if (out == NULL) return;

    FILE* f = fopen(out, "w");

    if (f == NULL) return;

    fprintf(f, "%s", map);
    fclose(f);
}

void run() {
    char buf[1];
    while (1) {
        generate();
        clear_screen();
        reset_cursor();
        to_terminal();

        printf("  HBF   O L M\n");
        printf("l:%i%i%i i: %i %i\n", 
            active_layers & HEIGHT ? 1 : 0, 
            active_layers & BIOME ? 1 : 0, 
            active_layers & FEATURE ? 1 : 0,
            active_limits & OCEAN_LAND_LIMIT ? 1 : 0,
            active_limits & LAND_MOUNTAIN_LIMIT ? 1 : 0);

        int read_status = read(STDIN_FILENO, &buf, 1);

        if (read_status == -1 || read_status == 0) 
            return;

        switch (buf[0]) {
            case '+':
                if (active_layers & HEIGHT) zoom(&height_layer, -zoom_speed);
                if (active_layers & BIOME) zoom(&biome_layer, -zoom_speed);
                if (active_layers & FEATURE) zoom(&feature_layer, -zoom_speed);

                if (active_limits & OCEAN_LAND_LIMIT) move_limit(&ocean_land_limit, 1);
                if (active_limits & LAND_MOUNTAIN_LIMIT) move_limit(&land_mountain_limit, 1);
                break;

            case '-': 
                if (active_layers & HEIGHT) zoom(&height_layer, zoom_speed);
                if (active_layers & BIOME) zoom(&biome_layer, zoom_speed);
                if (active_layers & FEATURE) zoom(&feature_layer, zoom_speed);

                if (active_limits & OCEAN_LAND_LIMIT) move_limit(&ocean_land_limit, -1);
                if (active_limits & LAND_MOUNTAIN_LIMIT) move_limit(&land_mountain_limit, -1);
                break;

            case '1':
                active_layers ^= HEIGHT;
                break;

            case '2':
                active_layers ^= BIOME;
                break;

            case '3':
                active_layers ^= FEATURE;
                break;

            case '4':
                active_limits ^= OCEAN_LAND_LIMIT;    
                break;

            case '5':
                active_limits ^= LAND_MOUNTAIN_LIMIT;
                break;

            case 'a':
                if (active_layers == ALL_LAYERS && active_limits == ALL_LIMITS) {
                    active_layers = 0;
                    active_limits = 0;
                }
                else {
                    active_layers = ALL_LAYERS;
                    active_limits = ALL_LIMITS;
                }
                break;

            case 'l':
                if (active_layers == ALL_LAYERS)
                    active_layers = 0;
                else
                    active_layers = ALL_LAYERS;
                break;

            case 'i':
                if (active_limits == ALL_LIMITS)
                    active_limits = 0;
                else
                    active_limits = ALL_LIMITS;
                break;

            case '\033': {
                char special_buf[2];
                read(STDIN_FILENO, &special_buf, 2);
                switch (special_buf[1]) {
                    case 'A': // UP
                        if (active_layers & HEIGHT) move(&height_layer, 0, 1);
                        if (active_layers & BIOME) move(&biome_layer, 0, 1);
                        if (active_layers & FEATURE) move(&feature_layer, 0, 1);
                        break;

                    case 'B': // DOWN
                        if (active_layers & HEIGHT) move(&height_layer, 0, -1);
                        if (active_layers & BIOME) move(&biome_layer, 0, -1);
                        if (active_layers & FEATURE) move(&feature_layer, 0, -1);
                        break;

                    case 'C': // RIGHT
                        if (active_layers & HEIGHT) move(&height_layer, -1, 0);
                        if (active_layers & BIOME) move(&biome_layer, -1, 0);
                        if (active_layers & FEATURE) move(&feature_layer, -1, 0);
                        break;

                    case 'D': // LEFT
                        if (active_layers & HEIGHT) move(&height_layer, 1, 0);
                        if (active_layers & BIOME) move(&biome_layer, 1, 0);
                        if (active_layers & FEATURE) move(&feature_layer, 1, 0);
                        break;
                }
                break;
            }
            case 'q': 
                terminal_echo_on();
                terminal_canonical_on();
                to_file();
                free(map);
                exit(0);
                break;
        }
    }
}


int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Missing arguments");
        exit(0);
    }

    terminal_echo_off();
    terminal_canonical_off();

    view_width = atoi(argv[1]);
    view_height = atoi(argv[2]);
    out = argc > 3 ? argv[3] : NULL;
    int _s = argc > 4 ? atoi(argv[4]) : (unsigned) time(NULL);
    
    int map_size = sizeof(char) * (view_width + 1) * view_height;
    map = malloc(map_size + 1);
    map[map_size] = 0;

    srand(_s);

    height_layer = make_layer(0.2, 1);
    biome_layer = make_layer(0.2, 1);
    feature_layer = make_layer(0.7, 1);

    run();
}
