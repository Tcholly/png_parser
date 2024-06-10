#include "image.h"
#include "png.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INVALID_IMAGE (image_t) { .valid = false }

image_t create_image_from_png(const png_t* png)
{
	if (!png->valid)
		return INVALID_IMAGE;

	if (png->chunks.size < 2)
		return INVALID_IMAGE;

	if (png->chunks.items[0].type != CHUNK_TYPE_IHDR)
		return INVALID_IMAGE;

	IHDR_chunk_t ihdr = png->chunks.items[0].data.ihdr;


	uint8_t* raw_data = get_png_raw_data(png);
	if (!raw_data)
	{
		fprintf(stderr, "ERROR: Failed to get png data\n");
		return INVALID_IMAGE;
	}

	image_t result = { .valid = true };
	result.width = ihdr.width;
	result.height = ihdr.height;
	result.data = (color_t*)malloc(ihdr.width * ihdr.height * sizeof(color_t));

	for (size_t y = 0; y < result.height; y++)
	{
		for (size_t x = 0; x < result.width; x++)
		{
			// TODO: Assuming RGB triple
			size_t index = x + y * result.width;
			color_t color = {.a = 255};
			color.r = raw_data[index * 3];
			color.g = raw_data[index * 3 + 1];
			color.b = raw_data[index * 3 + 2];
			result.data[index] = color;
		}
	}

	free(raw_data);

	return result;
}

void free_image(image_t* image)
{
	if (image->valid)
		free(image->data);

	image->valid = false;
	image->width = 0;
	image->height = 0;
	image->data = NULL;
}
