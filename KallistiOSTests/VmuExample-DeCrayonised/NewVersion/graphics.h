#ifndef MY_GRAPHICS_H
#define MY_GRAPHICS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> //For the uintX_t types

#include "crayon.h"

#ifdef _arch_dreamcast

#include <dc/pvr.h>

pvr_ptr_t font_tex;

void font_init();

void draw_char(float x1, float y1, float z1, uint8_t a, uint8_t r, uint8_t g, uint8_t b, int c, float xs, float ys);
#endif

//Draw a string
//x, y, "argb" params and str are self explanatory. xs and ys are kind like scaling/how large to make it
//Keep in mind this works best with xs/xy == 2, smaller spaces the lines too far apart and too much doesn't space them enough
void draw_string(float x, float y, float z, uint8_t a, uint8_t r, uint8_t g, uint8_t b, char *str, float xs, float ys);

#endif