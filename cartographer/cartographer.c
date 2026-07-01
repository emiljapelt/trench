#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

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
typedef struct noise_map {
    unsigned char data[permutation_size];
    unsigned int width;
    unsigned int height;
    float z;
} noise_map;



// maybe unsigned char is good enough?
void shuffle(unsigned char* array, int len) {
    for(int i = len - 1; i > 0; i--) {
        int index = rand() % (len + 1);
        unsigned char temp = array[i];

        array[i] = array[index];
        array[index] = temp;
    }
}

noise_map make_noise_map(int w, int h, float z, char p) {

    noise_map map;

    map.width = w;
    map.height = h;
    map.z = z;

    if (p) {
        int i;
        for(i = 0; i < permutation_size/2; i++)
            map.data[i] = i;

        shuffle(map.data, permutation_size/2);

        for(i = 0; i < permutation_size/2; i++)
            map.data[permutation_size/2 + i] = map.data[i];
    }

    return map;
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

float map_value(noise_map map, float x, float y) {
    x *= map.z;
    y *= map.z;

    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);

    vector_2 TR = vec2(xf - 1.0, yf - 1.0);
    vector_2 TL = vec2(xf, yf - 1.0);
    vector_2 BR = vec2(xf - 1.0, yf);
    vector_2 BL = vec2(xf, yf);

    int VTR = map.data[(map.data[(X+1) & 255 ]+Y+1) & 255];
    int VTL = map.data[(map.data[X]+Y+1) & 255];
    int VBR = map.data[(map.data[(X+1) & 255]+Y) & 255];
    int VBL = map.data[(map.data[X]+Y) & 255];

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
float fractal_brownian_map_value(noise_map map, int x, int y, int numOctaves) {
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


// fine _z: ~0.2;

#define EMPTY '.';
#define MOUNTAIN 'M';
#define OCEAN '~';
#define TREE 'T';
#define CLAY 'C';

int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Missing arguments");
        exit(0);
    }

    int _w = atoi(argv[1]);
    int _h = atoi(argv[2]);
    int _s = argc > 3 ? atoi(argv[3]) : (unsigned) time(NULL);
    float _z = argc > 4 ? atof(argv[4]) : 0.2;

    srand(_s);

    noise_map height_map = make_noise_map(_w, _h, 0.12, 1);
    noise_map biome_map = make_noise_map(_w, _h, 0.12, 1);
    noise_map feature_map = make_noise_map(_w, _h, 0.7, 1);

    for(int y = 0; y < _h; y++) {
        for(int x = 0; x < _w; x++) {
            float n = map_value(height_map, x, y);
            //float n = fractal_brownian_map_value(height_map, x, y, 3);
            char c = '?';

            if (n < 0.5) {
                c = OCEAN;
            }
            else if (n < 0.84) {
                if (map_value(biome_map, x, y) < 0.4 && map_value(feature_map, x, y) < 0.5) {
                    c = TREE
                }
                else if (map_value(biome_map, x, y) < 0.6 && map_value(feature_map, x, y) < 0.22) {
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
}
