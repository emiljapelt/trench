#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>

// https://rtouti.github.io/graphics/perlin-noise-algorithm

int view_width;
int view_height;
const float zoom_speed = 0.01;
const float move_speed = 1;

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

#define permutation_size 512
typedef struct layer {
    unsigned char data[permutation_size];
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
        for(i = 0; i < permutation_size/2; i++)
            l.data[i] = i;

        shuffle(l.data, permutation_size/2);

        for(i = 0; i < permutation_size/2; i++)
            l.data[permutation_size/2 + i] = l.data[i];
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

    unsigned int xu = (unsigned int)fx & 255;
    unsigned int yu = (unsigned int)fy & 255;

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




#pragma region CONSOLIDATE WITH ENGINE

void clear_screen(void) {
    printf("\033[H\033[J");
}
void reset_cursor(void) {
    printf("\033[0;0H");
}

void terminal_echo_off() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag &= ~(ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

void terminal_echo_on() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag |= ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

void terminal_canonical_off() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag &= ~(ICANON);

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

void terminal_canonical_on() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag |= ICANON;

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

#pragma endregion

layer height_layer;
layer biome_layer;
layer feature_layer;

enum layers {
    HEIGHT =    0b000001,
    BIOME =     0b000010,
    FEAUTRE =   0b000100,

    ALL =       0b000111,
};

float ocean_limit = 0.5;
float land_limit = 0.84;

enum limits {
    OCEAN_LIMIT = 0b000001,

};

unsigned int active_layers = ALL;

char* out = NULL;

#define EMPTY '.';
#define MOUNTAIN 'M';
#define OCEAN '~';
#define TREE 'T';
#define CLAY 'C';

void render() {
    clear_screen();
    reset_cursor();

    for(int y = 0; y < view_height; y++) {
        for(int x = 0; x < view_width; x++) {
            float n = fractal_brownian_layer_value(height_layer, x, y, 8);
            char c = '?';

            if (n < 0.5) {
                c = OCEAN;
            }
            else if (n < 0.84) {
                if (fractal_brownian_layer_value(biome_layer, x, y, 4) < 0.4 && layer_value(feature_layer, x, y) < 0.5) {
                    c = TREE
                }
                else if (fractal_brownian_layer_value(biome_layer, x, y, 4) < 0.6 && layer_value(feature_layer, x, y) < 0.22) {
                    c = CLAY
                }
                else {
                    c = EMPTY
                }
            }
            else {    
                c = MOUNTAIN;
            }

            printf("%c", c);
        }
        putchar('\n');
    }

    printf("%i,%i,%i\n", 
        active_layers & HEIGHT ? 1 : 0, 
        active_layers & BIOME ? 1 : 0, 
        active_layers & FEAUTRE ? 1 : 0);
}

void move(layer* l, int x, int y) {
    l->x += x * move_speed * l->z;
    l->y += y * move_speed * l->z;
}

void zoom(layer* l, float a) {
    l->z += a;
    if (l->z < 0) l->z = 0;
}

void write_file() {
    if (out == NULL) return;
}

void run() {
    char buf[1];
    while (1) {
        render();

        int read_status = read(STDIN_FILENO, &buf, 1);

        if (read_status == -1 || read_status == 0) 
            return;

        switch (buf[0]) {
            case '+':
                if (active_layers & HEIGHT) zoom(&height_layer, -zoom_speed);
                if (active_layers & BIOME) zoom(&biome_layer, -zoom_speed);
                if (active_layers & FEAUTRE) zoom(&feature_layer, -zoom_speed);
                break;
            case '-': 
                if (active_layers & HEIGHT) zoom(&height_layer, zoom_speed);
                if (active_layers & BIOME) zoom(&biome_layer, zoom_speed);
                if (active_layers & FEAUTRE) zoom(&feature_layer, zoom_speed);
                break;
            case '1':
                active_layers ^= HEIGHT;
                break;
            case '2':
                active_layers ^= BIOME;
                break;
            case '3':
                active_layers ^= FEAUTRE;
                break;
            case 'a':
                if (active_layers == ALL)
                    active_layers = 0;
                else
                    active_layers = ALL;
                break;
            case '\033': {
                char special_buf[2];
                read(STDIN_FILENO, &special_buf, 2);
                switch (special_buf[1]) {
                    case 'A': // UP
                        if (active_layers & HEIGHT) move(&height_layer, 0, 1);
                        if (active_layers & BIOME) move(&biome_layer, 0, 1);
                        if (active_layers & FEAUTRE) move(&feature_layer, 0, 1);
                        break;
                    case 'B': // DOWN
                        if (active_layers & HEIGHT) move(&height_layer, 0, -1);
                        if (active_layers & BIOME) move(&biome_layer, 0, -1);
                        if (active_layers & FEAUTRE) move(&feature_layer, 0, -1);
                        break;
                    case 'C': // RIGHT
                        if (active_layers & HEIGHT) move(&height_layer, -1, 0);
                        if (active_layers & BIOME) move(&biome_layer, -1, 0);
                        if (active_layers & FEAUTRE) move(&feature_layer, -1, 0);
                        break;
                    case 'D': // LEFT
                        if (active_layers & HEIGHT) move(&height_layer, 1, 0);
                        if (active_layers & BIOME) move(&biome_layer, 1, 0);
                        if (active_layers & FEAUTRE) move(&feature_layer, 1, 0);
                        break;
                }
                break;
            }
            case 'q': 
                terminal_echo_on();
                terminal_canonical_on();
                write_file();
                exit(0);
                break;
        }
    }
}


// fine _z: ~0.2;

int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Missing arguments");
        exit(0);
    }

    terminal_echo_off();
    terminal_canonical_off();

    view_width = atoi(argv[1]);
    view_height = atoi(argv[2]);
    int _s = argc > 3 ? atoi(argv[3]) : (unsigned) time(NULL);
    //float _z = argc > 4 ? atof(argv[4]) : 0.2;

    srand(_s);

    height_layer = make_layer(0.12, 1);
    biome_layer = make_layer(0.12, 1);
    feature_layer = make_layer(0.7, 1);

    run();
}
