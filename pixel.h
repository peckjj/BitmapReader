#include <stdint.h>

#ifndef PIXEL
	#define PIXEL 1
#endif

// Color Channel constants
typedef uint8_t ColorChannel;

#define CHAN_RED (ColorChannel)0
#define CHAN_GREEN (ColorChannel)1
#define CHAN_BLUE (ColorChannel)2

typedef struct __attribute__((__packed__)) {
	uint8_t b;
	uint8_t g;
	uint8_t r;
} Pixel24_t;
