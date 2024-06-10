#pragma once

#include "png.h"

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} color_t;


typedef struct
{
	bool valid;
	uint32_t width;
	uint32_t height;
	color_t* data;
} image_t;

image_t create_image_from_png(const png_t* png);
void free_image(image_t* image);
