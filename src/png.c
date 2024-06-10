#include "png.h"
#include "utils.h"

#include <byteswap.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#define INVALID_PNG   (png_t)   { .valid = false };
#define INVALID_CHUNK (chunk_t) { .valid = false };

#define MAGIC_LENGTH 8
const uint8_t MAGIC[MAGIC_LENGTH] = {137, 80, 78, 71, 13, 10, 26, 10};

#define IHDR_CHUNK_LENGTH 13


static char* type_to_str(uint32_t type)
{
	static char result[5];
	result[4] = 0;

	for (int i = 0; i < 4; i++)
		result[i] = 0xFF & (type >> ((3 - i) * 8));

	return result;
}

static uint32_t read_u32(FILE* file)
{
	uint32_t result;
	fread(&result, 4, 1, file);
	return bswap_32(result);
}

static uint8_t read_u8(FILE* file)
{
	uint8_t result;
	fread(&result, 1, 1, file);
	return result;
}

static IHDR_chunk_t read_IHDR_chunk(FILE* file)
{
	IHDR_chunk_t result = {0};

	result.width = read_u32(file);
	result.height = read_u32(file);
	result.bit_depth = read_u8(file);
	result.color_type = read_u8(file);
	result.compression_method = read_u8(file);
	result.filter_method = read_u8(file);
	result.interlace_method = read_u8(file);

	return result;
}


static chunk_t read_chunk(FILE* file)
{
	chunk_t result = { .valid = true };
	result.data_length = read_u32(file);
	result.type = read_u32(file);

	switch (result.type)
	{
		case CHUNK_TYPE_IHDR:
			if (result.data_length != IHDR_CHUNK_LENGTH)
			{
				fprintf(stderr, "ERROR: Invalid chunk: IHDR length does not match\n");
				return INVALID_CHUNK;
			}
			result.data.ihdr = read_IHDR_chunk(file);
			break;

		case CHUNK_TYPE_IEND:
			if (result.data_length != 0)
			{
				fprintf(stderr, "ERROR: Invalid chunk: IEND length does not match\n");
				return INVALID_CHUNK;
			}
			break;

		case CHUNK_TYPE_IDAT:
			result.data.idat.data = (uint8_t*)malloc(result.data_length);
			fread(result.data.idat.data, 1, result.data_length, file);
			break;

		case CHUNK_TYPE_tEXt:
			{
				uint8_t* data = (uint8_t*)malloc(result.data_length);
				fread(data, 1, result.data_length, file);

				int keyword_len = 0;
				while (data[keyword_len] != 0)
					keyword_len++;

				int text_len = result.data_length - keyword_len - 1;

				result.data.tEXt.keyword = (char*)malloc(keyword_len + 1);
				result.data.tEXt.text = (char*)malloc(text_len + 1);
				
				memcpy(result.data.tEXt.keyword, data, keyword_len);
				result.data.tEXt.keyword[keyword_len] = '\0';

				memcpy(result.data.tEXt.text, data + keyword_len + 1, text_len);
				result.data.tEXt.text[text_len] = '\0';

				free(data);
			}
			break;

		default:
			fprintf(stderr, "ERROR: Invalid chunk type: %s\n", type_to_str(result.type));
			return INVALID_CHUNK;
			break;
	}

	result.CRC = read_u32(file); // TODO: compute and verify?


	return result;
}


// Specs: http://libpng.org/pub/png/spec/1.2/PNG-Contents.html
png_t parse_png(FILE* file)
{
	if (!file)
	{
		fprintf(stderr, "ERROR: Invalid argument: file is NULL\n"); 
		return INVALID_PNG;
	}

	// Read signature and check if it is correct
	uint8_t signature[MAGIC_LENGTH];
	size_t read_len = fread(signature, 1, MAGIC_LENGTH, file);
	if (read_len != MAGIC_LENGTH)
	{
		fprintf(stderr, "ERROR: failed to read signature: (TODO: distinguish error)\n"); 
		return INVALID_PNG;
	}

	bool valid_signature = true;
	for (size_t i = 0; i < MAGIC_LENGTH; i++)
	{
		if (signature[i] != MAGIC[i])
		{
			valid_signature = false;
			break;
		}
	}

	if (!valid_signature)
	{
		fprintf(stderr, "ERROR: Invalid signature\n"); 
		return INVALID_PNG;
	}

	printf("INFO: valid signature\n");

	png_t result = { .valid = true };

	// Read chunks
	chunk_type_e last_type;
	do
	{

		chunk_t chunk = read_chunk(file);
		if (!chunk.valid)
		{
			result.valid = false;
			break;
		}

		last_type = chunk.type;
		da_append(result.chunks, chunk);
	} while (last_type != CHUNK_TYPE_IEND && feof(file) == 0);

	printf("INFO: Read %zu chunks\n", result.chunks.size);

	return result;
}



void free_png(png_t* png)
{
	if (!png->valid)
	{
		if (png->chunks.capacity > 0)
		{
			free(png->chunks.items);
			png->chunks.capacity = 0;
			png->chunks.size = 0;
		}
		return;
	}

	for (size_t i = 0; i < png->chunks.size; i++)
	{
		chunk_t* chunk = &png->chunks.items[i];
		if (!chunk->valid)
			continue;

		switch (chunk->type)
		{
			case CHUNK_TYPE_IHDR:
				chunk->data.ihdr.width = 0;
				chunk->data.ihdr.height = 0;
				chunk->data.ihdr.bit_depth = 0;
				chunk->data.ihdr.color_type = 0;
				chunk->data.ihdr.compression_method = 0;
				chunk->data.ihdr.filter_method = 0;
				chunk->data.ihdr.interlace_method = 0;
				break;

			case CHUNK_TYPE_IEND:
				break;

			case CHUNK_TYPE_IDAT:
				free(chunk->data.idat.data);
				chunk->data.idat.data = NULL;
				break;

			case CHUNK_TYPE_tEXt:
				free(chunk->data.tEXt.keyword);
				free(chunk->data.tEXt.text);
				chunk->data.tEXt.keyword = NULL;
				chunk->data.tEXt.text = NULL;
				break;

			default:
				fprintf(stderr, "ERROR: Invalid chunk type: %s\n", type_to_str(chunk->type));
				break;
        }

		chunk->CRC = 0;
		chunk->data_length = 0;
		chunk->type = 0;
		chunk->valid = false;
	}

	free(png->chunks.items);
	png->chunks.capacity = 0;
	png->chunks.size = 0;

	png->valid = false;
}

static uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c)
{
	int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

	uint8_t pr = c;
	if (pa <= pb && pa <= pc)
		pr = a;
	else if (pb <= pc)
		pr = b;

    return pr;
}

uint8_t* get_png_raw_data(const png_t* png)
{
	if (!png->valid)
		return NULL;

	if (png->chunks.size < 2)
		return NULL;

	if (png->chunks.items[0].type != CHUNK_TYPE_IHDR)
		return NULL;

	IHDR_chunk_t ihdr = png->chunks.items[0].data.ihdr;

	size_t idat_total_size = 0;
	// TODO: find better way to loop to avoid unnecessary iterations
	for (size_t i = 1; i < png->chunks.size; i++)
	{
		chunk_t* chunk = &png->chunks.items[i];
		if (chunk->type != CHUNK_TYPE_IDAT)
			continue;

		idat_total_size += chunk->data_length;
	}

	uint8_t* data = (uint8_t*)malloc(idat_total_size);
	size_t cursor = 0;
	for (size_t i = 1; i < png->chunks.size; i++)
	{
		chunk_t* chunk = &png->chunks.items[i];
		if (chunk->type != CHUNK_TYPE_IDAT)
			continue;

		memcpy(data + cursor, chunk->data.idat.data, chunk->data_length);
		cursor += chunk->data_length;
	}

	// TODO: uncompress data
	if (ihdr.compression_method != 0)
	{
		fprintf(stderr, "ERROR: Unimplemented compression method: %u", ihdr.compression_method);
		free(data);
		return NULL;
	}

	// Result is RGB triple + 1 filter type byte for scanline
	uint32_t uncompressed_size = ihdr.width * ihdr.height * 3 + ihdr.height;
	uint8_t* uncompressed_data = (uint8_t*)malloc(uncompressed_size);

	// Setup zlib inflate
	z_stream zlib_stream = {0};
	zlib_stream.avail_in = idat_total_size;
	zlib_stream.next_in = data;
	zlib_stream.avail_out = uncompressed_size;
	zlib_stream.next_out = uncompressed_data;

	int res = inflateInit(&zlib_stream);
	if (res != Z_OK)
	{
		fprintf(stderr, "ERROR: inflateInit %d: %s\n", res, zlib_stream.msg); 
		free(data);
		free(uncompressed_data);
		return NULL;
	}
	res = inflate(&zlib_stream, Z_FINISH);
	if (res != Z_STREAM_END)
	{
		fprintf(stderr, "ERROR: inflate %d: %s\n", res, zlib_stream.msg); 
		free(data);
		free(uncompressed_data);
		return NULL;
	}
	res = inflateEnd(&zlib_stream);
	if (res != Z_OK)
	{
		fprintf(stderr, "ERROR: inflateEnd %d: %s\n", res, zlib_stream.msg); 
		free(data);
		free(uncompressed_data);
		return NULL;
	}

	free(data);
	data = NULL;

	if (zlib_stream.total_out != uncompressed_size)
	{
		fprintf(stderr, "ERROR: expected size after decompression does not match actual size: expected %u bytes, got %lu bytes", uncompressed_size, zlib_stream.total_out);
		free(uncompressed_data);
		return NULL;
	}


	// TODO: unfilter data
	if (ihdr.filter_method != 0)
	{
		fprintf(stderr, "ERROR: Unimplemented filter method: %u", ihdr.filter_method);
		free(uncompressed_data);
		return NULL;
	}

	uint8_t* result = (uint8_t*)malloc(ihdr.width * ihdr.height * 3);
	uint32_t bytes_per_pixel = 3; // TODO: Assuming RGB triple
	uint32_t scanline_size = ihdr.width * bytes_per_pixel + 1;
	uint32_t pixel_per_scanline = ihdr.width * bytes_per_pixel;

	for (size_t i = 0; i < ihdr.height; i++)
	{
		uint8_t* scanline = uncompressed_data + i * scanline_size;
		uint8_t filter_type = scanline[0];
		for (size_t j = 0; j < pixel_per_scanline; j++)
		{
			uint8_t current = scanline[j + 1];
			uint8_t a = 0;
			uint8_t b = 0;
			uint8_t c = 0;

			if (j >= bytes_per_pixel)
				a = result[j - bytes_per_pixel + i * pixel_per_scanline];

			if (i > 0)
				b = result[j + (i - 1) * pixel_per_scanline];

			if (j >= bytes_per_pixel && i > 0)
				c = result[j - bytes_per_pixel + (i - 1) * pixel_per_scanline];


			switch (filter_type)
			{
				case 0:
					result[j + i * pixel_per_scanline] = current;
				break;

				case 1:
					result[j + i * pixel_per_scanline] = current + a;
					break;

				case 2:
					result[j + i * pixel_per_scanline] = current + b;
					break;

				case 3:
					result[j + i * pixel_per_scanline] = current + floorf((a + b) / 2.0f);
					break;

				case 4:
					result[j + i * pixel_per_scanline] = current + paeth_predictor(a, b, c);
					break;

				default:
					fprintf(stderr, "ERROR: unknown filter type %u: scanline = %zu\n", filter_type, i + 1);
					free(uncompressed_data);
					free(result);
					return NULL;
			}
		}
	}

	free(uncompressed_data);

	return result;
}
