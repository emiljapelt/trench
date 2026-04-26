#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct {
    unsigned char b;
    unsigned char g;
    unsigned char r;
} rgb;

int color_eq(rgb c1, rgb c2) {
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

typedef struct {
    char symbol;
    rgb color;
} translation;

const int translation_count = 7;
const translation translations[] = {
    {'.', {.r = 0, .g = 0, .b = 0}},
    {'+', {.r = 255, .g = 255, .b = 255}},
    {'w', {.r = 40, .g = 40, .b = 40}},
    {'~', {.r = 0, .g = 0, .b = 255}},
    {'T', {.r = 0, .g = 255, .b = 0}},
    {'C', {.r = 195, .g = 140, .b = 95}},
    {'M', {.r = 140, .g = 140, .b = 140}},
};

void failure(char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    exit(1);
}

rgb to_color(char c) {
    for(int i = 0; i < translation_count; i++) {
        if (translations[i].symbol == c) return translations[i].color;
    }
    failure("Could not translate: %c", c);
}

char to_symbol(rgb c) {
    for(int i = 0; i < translation_count; i++) {
        if (color_eq(translations[i].color, c)) return translations[i].symbol;
    }
    failure("Could not translate");
}


void to_bmp(char* in_path, char* out_path) {
    FILE* in_fp = fopen(in_path, "rb");

    if (in_fp == NULL) {
        failure("Could not open file: %s\n", in_path);
    }

    fseek(in_fp, 0L, SEEK_END);
    int content_len = ftell(in_fp);
    fseek(in_fp, 0L, SEEK_SET);

    char* content = malloc(content_len);
    fread(content, 1, content_len, in_fp);
    fclose(in_fp);

    int height = 0;
    int width = 0;

    {
        int current_width = 0;
        for(int i = 0; i < content_len; i++) {
            if (content[i] == '\n') {
                if (height == 0) 
                    width = current_width;
                else if (width != current_width)
                    failure("Bad input file\n");
                height++;
                current_width = 0;
            }
            else 
                current_width++;
        }
    }

    rgb* pixels = malloc(sizeof(rgb) * width * height);

    content_len = width * height + height;
    int pos = 0;
    for(int i = 0; i < content_len; i++) {
        char symbol = content[i];
        if (symbol == '\n')
            continue;

        pixels[pos++] = to_color(symbol);
    }

    int padding = width % 4;
    
    char tag[] = { 'B', 'M' };
    int header[] = { 
        0x00, // file_size, 
        0x00, // unused
        0x36, // offset
        0x28, // dib_header_size
        width,
        height,
        0x180001, // pixel info
        0, // no compression
        0, // pixel data size?
        0x002e23, 0x002e23, // print resolution
        0, 0, // no color palette
    };

    header[0] = sizeof(tag) + sizeof(header) + (height * (width + padding) * sizeof(rgb));
    
    FILE* out_fp = fopen(out_path, "wb");

    fwrite(tag, sizeof(tag), 1, out_fp);
    fwrite(header, sizeof(header), 1, out_fp);

    for(int y = height - 1; y >= 0; y--) {
        for(int x = 0; x < width; x++) {
            rgb color = pixels[x + (y * width)];
            fwrite(&color, sizeof(rgb), 1, out_fp);
        }
        for(int p = 0; p < padding; p++) 
            fwrite("\0", 1, 1, out_fp);
    }

    fclose(out_fp);
}


void to_trm(char* in_path, char* out_path) {

}


// Translate between .trm and .bmp
int main(int argc, char** argv) {

    if (argc < 3) failure("Too few arguments given, needs: <in> <out>\n");

    char* path = argv[1];
    char* out = argv[2];
    int len = strlen(path);

    if (strcmp(path + (len - 4), ".bmp") == 0) {
        failure("Not implemented\n");
    }
    else if (strcmp(path + (len - 4), ".trm") == 0) {
        to_bmp(path, out);
    }
    else {
        failure("Unsupported file type\n");
    }

    return 0;
}