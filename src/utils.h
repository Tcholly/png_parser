#pragma once

#include <stdlib.h>

#define DA_INIT_SIZE 32
#define da_append(da, value)                                                         \
	do                                                                               \
	{                                                                                \
		if ((da).size + 1 > (da).capacity)                                           \
		{                                                                            \
			if ((da).capacity == 0)                                                  \
				(da).capacity = DA_INIT_SIZE;                                        \
			else                                                                     \
				(da).capacity *= 2;                                                  \
			(da).items = realloc((da).items, (da).capacity * sizeof((da).items[0])); \
		}                                                                            \
		(da).items[(da).size++] = value;                                             \
	} while (0)

#define da_remove_at(da, index)                              \
	do                                                       \
	{                                                        \
		if ((index) >= 0 && (index) < (da).size)             \
		{                                                    \
			(da).items[(index)] = (da).items[(da).size - 1]; \
			(da).size--;                                     \
		}                                                    \
	} while (0)

#define da_remove_at_keep_order(da, index)                                                                                    \
	do                                                                                                                        \
	{                                                                                                                         \
		if ((index) >= 0 && (index) < (da).size)                                                                              \
		{                                                                                                                     \
			memmove((da).items + index, (da).items + index + 1, ((da).size - index - 1) * sizeof((da).items[0])); \
			(da).size--;                                                                                                      \
		}                                                                                                                     \
	} while (0)
