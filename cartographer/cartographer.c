#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>

// https://rtouti.github.io/graphics/perlin-noise-algorithm

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
    unsigned int width;
    unsigned int height;
    float z;
    int x;
    int y;
} layer;

void shuffle(unsigned char* array, int len) {
    for(int i = len - 1; i > 0; i--) {
        int index = rand() % (len + 1);
        unsigned char temp = array[i];

        array[i] = array[index];
        array[index] = temp;
    }
}

layer make_layer(int w, int h, float z, char p) {
    layer l;

    l.width = w;
    l.height = h;
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


float fade(float t) {
    return ((6*t - 15)*t + 10)*t*t*t;
}

float lerp(float t, float a, float b) {
    return a + t*(b-a);
}

float layer_value(layer l, float x, float y) {
    x += l.x;
    y += l.y;

    x *= l.z;
    y *= l.z;

    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);

    vector_2 TR = vec2(xf - 1.0, yf - 1.0);
    vector_2 TL = vec2(xf, yf - 1.0);
    vector_2 BR = vec2(xf - 1.0, yf);
    vector_2 BL = vec2(xf, yf);

    int VTR = l.data[(l.data[(X+1) & 255 ]+Y+1) & 255];
    int VTL = l.data[(l.data[X]+Y+1) & 255];
    int VBR = l.data[(l.data[(X+1) & 255]+Y) & 255];
    int VBL = l.data[(l.data[X]+Y) & 255];

    float dotTR = dot(TR, constant_vector(VTR));
    float dotTL = dot(TL, constant_vector(VTL));
    float dotBR = dot(BR, constant_vector(VBR));
    float dotBL = dot(BL, constant_vector(VBL));

    float u = fade(xf);
    float v = fade(yf);

    float n = lerp(u,
        lerp(v, dotBL, dotTL),
        lerp(v, dotBR, dotTR)
    );

    return (n + 1.0) / 2.0; // Scale to 0 .. 1.0
}


// I don't know if the difference can be seen at this resolution...
// Or if the settings work for the resolution
/*float fractal_brownian_map_value(layer map, int x, int y, int numOctaves) {
	float result = 0.0;
	float amplitude = 1.0;
	float frequency = 0.05;

	while(numOctaves--) {
		float n = amplitude * map_value(map, x * frequency, y * frequency);
		result += n;
		
		amplitude *= 0.5;
		frequency *= 2.0;
	}

	return result;
}
*/

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

unsigned int active_layers = ALL;

char* out;

int view_width;
int view_height;

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
            float n = layer_value(height_layer, x, y);
            //float n = fractal_brownian_map_value(height_map, x, y, 3);
            char c = '?';

            if (n < 0.5) {
                c = OCEAN;
            }
            else if (n < 0.84) {
                if (layer_value(biome_layer, x, y) < 0.4 && layer_value(feature_layer, x, y) < 0.5) {
                    c = TREE
                }
                else if (layer_value(biome_layer, x, y) < 0.6 && layer_value(feature_layer, x, y) < 0.22) {
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
        printf("\n");
    }

    printf("%i,%i,%i\n", 
        active_layers & HEIGHT ? 1 : 0, 
        active_layers & BIOME ? 1 : 0, 
        active_layers & FEAUTRE ? 1 : 0);
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
                if (active_layers & HEIGHT) height_layer.z -= 0.01;
                if (active_layers & BIOME) biome_layer.z -= 0.01;
                if (active_layers & FEAUTRE) feature_layer.z -= 0.01;
                break;
            case '-': 
                if (active_layers & HEIGHT) height_layer.z += 0.01;
                if (active_layers & BIOME) biome_layer.z += 0.01;
                if (active_layers & FEAUTRE) feature_layer.z += 0.01;
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
                        if (active_layers & HEIGHT) height_layer.y += 1;
                        if (active_layers & BIOME) biome_layer.y += 1;
                        if (active_layers & FEAUTRE) feature_layer.y += 1;
                        break;
                    case 'B': // DOWN
                        if (active_layers & HEIGHT) height_layer.y -= 1;
                        if (active_layers & BIOME) biome_layer.y -= 1;
                        if (active_layers & FEAUTRE) feature_layer.y -= 1;
                        break;
                    case 'C': // RIGHT
                        if (active_layers & HEIGHT) height_layer.x -= 1;
                        if (active_layers & BIOME) biome_layer.x -= 1;
                        if (active_layers & FEAUTRE) feature_layer.x -= 1;
                        break;
                    case 'D': // LEFT
                        if (active_layers & HEIGHT) height_layer.x += 1;
                        if (active_layers & BIOME) biome_layer.x += 1;
                        if (active_layers & FEAUTRE) feature_layer.x += 1;
                        break;
                }
                break;
            }
            case 'q': 
                // Write to file
                terminal_echo_on();
                terminal_canonical_on();
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

    view_height = atoi(argv[1]);
    view_width = atoi(argv[2]);
    int _s = argc > 3 ? atoi(argv[3]) : (unsigned) time(NULL);
    //float _z = argc > 4 ? atof(argv[4]) : 0.2;

    srand(_s);

    height_layer = make_layer(view_width, view_height, 0.12, 1);
    biome_layer = make_layer(view_width, view_height, 0.12, 1);
    feature_layer = make_layer(view_width, view_height, 0.7, 1);

    run();
}
