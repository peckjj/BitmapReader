#include <stdint.h>

#ifndef PIXEL
	#define PIXEL 1
#endif

typedef struct __attribute__((__packed__)) {
	uint8_t b;
	uint8_t g;
	uint8_t r;
} Pixel24_t;
