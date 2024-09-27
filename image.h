#include <stdint.h>

#ifndef IMAGE
	#define IMAGE 1
#endif

#ifndef PIXEL
	#include "pixel.h"
#endif

typedef struct __attribute__((__packed__)) BmpFileHeader {
    union
    {
        char signature[2];
        uint16_t sigCode;
    };
    uint32_t fileSize;
    uint16_t r1;
    uint16_t r2;
    uint32_t dataOffset;
} BmpFileHeader;

typedef struct __attribute__((__packed__)) DibHeader {
    uint32_t headerSize;
    int32_t width;
    int32_t height;
    uint16_t numPlanes;
    uint16_t bitsPerPixel;
    uint32_t compressionMethod;
    uint32_t rawImageSize;
    int32_t  horRes;
    int32_t  verRes;
    uint32_t numColors;
    uint32_t numImportantColors;
} DibHeader;

typedef struct bmp_Pixel24_t {
	BmpFileHeader *bmpHeader;
	DibHeader *dibHeader;
	uint32_t postHeaderDataSize;
	char *postHeaderData;
	Pixel24_t *pixelArray;
	uint32_t postDataSize;
	char *postData;
} bmp_Pixel24_t;


// Pixel24_t type functions
size_t readBitmapPixelData(bmp_Pixel24_t *dest, FILE* bitmap);
size_t removeRedPixel24_t(bmp_Pixel24_t *bitmap);

size_t writeToFilePixel24_t(bmp_Pixel24_t *bitmap, FILE* outFile);
int freeImageDataPixel24_t(bmp_Pixel24_t *bitmap);
