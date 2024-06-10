#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum : uint32_t
{
	CHUNK_TYPE_IHDR = 0x49484452,
	CHUNK_TYPE_IEND = 0x49454E44,
	CHUNK_TYPE_IDAT = 0x49444154,
	CHUNK_TYPE_tEXt = 0x74455874
} chunk_type_e;

typedef struct
{
	uint32_t width;
	uint32_t height;
	uint8_t bit_depth;
	uint8_t color_type;
	uint8_t compression_method;
	uint8_t filter_method;
	uint8_t interlace_method;
} IHDR_chunk_t;

typedef struct
{
	uint8_t* data;
} IDAT_chunk_t;

typedef struct
{
} IEND_chunk_t;

typedef struct
{
	char* keyword;
	char* text;
} tEXt_chunk_t;


typedef struct
{
	bool valid;
	uint32_t data_length;
	chunk_type_e type;
	
	union
	{
		IHDR_chunk_t ihdr;
		IEND_chunk_t iend;
		IDAT_chunk_t idat;
		tEXt_chunk_t tEXt;
	} data;
	
	uint32_t CRC;
} chunk_t;

typedef struct
{
	chunk_t* items;
	size_t size;
	size_t capacity;
} da_chunk_t;

typedef struct
{
	bool valid;
	da_chunk_t chunks;
} png_t;

png_t parse_png(FILE* file);
void free_png(png_t* png);

// Returns uncompressed and unfiltered data from the png
uint8_t* get_png_raw_data(const png_t* png);
