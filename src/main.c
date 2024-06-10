#include <iso646.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <raylib.h>

#include "image.h"
#include "png.h"

void anal_png(const png_t* png)
{
	if (!png->valid)
	{
		printf("PNG is invalid\n");
		return;
	}

	for (size_t i = 0; i < png->chunks.size; i++)
	{
		chunk_t* chunk = &png->chunks.items[i];
		printf("Chunk %zu:\n", i);
		if (!chunk->valid)
		{
			printf("    Valid: false\n");
			continue;
		}

		printf("    Data length: %u\n", chunk->data_length);

		switch (chunk->type)
		{
			case CHUNK_TYPE_IHDR:
				printf("    Chunk type: IHDR\n");
				break;

			case CHUNK_TYPE_IEND:
				printf("    Chunk type: IEND\n");
				break;

			case CHUNK_TYPE_IDAT:
				printf("    Chunk type: IDAT\n");
				break;

			case CHUNK_TYPE_tEXt:
				printf("    Chunk type: tEXt\n");
				break;

			default:
				printf("    Chunk type: UNKNOWN\n");
				break;
		}

		printf("    CRC: %u\n", chunk->CRC);
		
		printf("    Data:\n");
		switch (chunk->type)
		{
			case CHUNK_TYPE_IHDR:
				printf("        Width: %u\n", chunk->data.ihdr.width);
				printf("        Height: %u\n", chunk->data.ihdr.height);
				printf("        Bit depth: %u\n", chunk->data.ihdr.bit_depth);
				printf("        Color type: %u\n", chunk->data.ihdr.color_type);
				printf("        Compression method: %u\n", chunk->data.ihdr.compression_method);
				printf("        Filter method: %u\n", chunk->data.ihdr.filter_method);
				printf("        Interlace method: %u\n", chunk->data.ihdr.interlace_method);
				break;

			case CHUNK_TYPE_IDAT:
				printf("        ...\n");
				break;

			case CHUNK_TYPE_tEXt:
				printf("        Keyword: \"%s\"\n", chunk->data.tEXt.keyword);
				printf("        Text:  \"%s\"\n", chunk->data.tEXt.text);
				break;

			case CHUNK_TYPE_IEND:
			default:
				printf("        No data\n");
				break;
		}
	}		
}


char* eat_args(int* argc, char*** argv)
{
	if (*argc < 1)
		return NULL;

	*argc -= 1;
	return *(*argv)++;
}

void usage(const char* program)
{
	printf("USAGE: %s <file.png>\n", program);
}


int main(int argc, char** argv)
{
	char* program = eat_args(&argc, &argv);
	char* filename = eat_args(&argc, &argv);

	if (filename == NULL)
	{
		fprintf(stderr, "ERROR: Missing arguments\n"); 
		usage(program);
		return 1;
	}

	FILE* file = fopen(filename, "r");
	if (!file)
	{
		fprintf(stderr, "ERROR: Failed to open file %s: %s\n", filename, strerror(errno)); 
		usage(program);
		return 1;
	}

	png_t png = parse_png(file);
	fclose(file);
	if (!png.valid)
	{
		fprintf(stderr, "ERROR: Invalid png\n");
		free_png(&png);
		return 1;
	}

	anal_png(&png);

	image_t image = create_image_from_png(&png);
	free_png(&png);

	if (!image.valid)
	{
		fprintf(stderr, "ERROR: Invalid image\n");
		free_image(&image);
		return 1;
	}

	InitWindow(1500, 900, "Image");
	SetTargetFPS(60);

	Image raylib_image = GenImageColor(image.width, image.height, BLUE);

	for (size_t y = 0; y < image.height; y++)
	{
		for (size_t x = 0; x < image.width; x++)
		{
			color_t orig = image.data[x + y * image.width];
			Color color =
			{
				.r = orig.r,
				.g = orig.g,
				.b = orig.b,
				.a = orig.a,
			};
			ImageDrawPixel(&raylib_image, x, y, color);
		}
	}

	Texture2D texture = LoadTextureFromImage(raylib_image);
	UnloadImage(raylib_image);

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(WHITE);

		DrawRectangle(0, 0, texture.width + 4, texture.height + 4, RED);
		DrawTexture(texture, 2, 2, WHITE);

		EndDrawing();
	}
	UnloadTexture(texture);

	CloseWindow();

	free_image(&image);

	return 0;
}
