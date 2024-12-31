#include <stdint.h>
#include <sys/types.h>

#ifndef IMAGE
#define IMAGE 1
#endif

#ifndef PIXEL
#include "pixel.h"
#endif

void testAlloc(char *s);

typedef struct __attribute__((__packed__)) BmpFileHeader
{
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

typedef struct __attribute__((__packed__)) DibHeader
{
    uint32_t headerSize;
    int32_t width;
    int32_t height;
    uint16_t numPlanes;
    uint16_t bitsPerPixel;
    uint32_t compressionMethod;
    uint32_t rawImageSize;
    int32_t horRes;
    int32_t verRes;
    uint32_t numColors;
    uint32_t numImportantColors;
} DibHeader;

typedef struct bmp_Pixel24_t
{
    char *fileName;
    BmpFileHeader *bmpHeader;
    DibHeader *dibHeader;
    size_t postHeaderDataSize;
    char *postHeaderData;
    Pixel24_t *pixelArray;
    size_t postDataSize;
    char *postData;
    uint8_t padding;
} bmp_Pixel24_t;

// Pixel24_t type functions
ssize_t readBitmapPixelData(bmp_Pixel24_t *dest, FILE *bitmap);
ssize_t removeRedPixel24_t(bmp_Pixel24_t *bitmap);
ssize_t removeBluePixel24_t(bmp_Pixel24_t *bitmap);
ssize_t removeGreenPixel24_t(bmp_Pixel24_t *bitmap);
ssize_t randomChannelPixel24_t(bmp_Pixel24_t *bitmap, ColorChannel channel);

int32_t setPixel(bmp_Pixel24_t *bitmap, uint16_t x, uint16_t y, Pixel24_t *value, ColorChannel channel);
Pixel24_t* getPixel(bmp_Pixel24_t *bmp, uint16_t x, uint16_t y);

// Assumes pixelArray field has already been allocated
ssize_t readAllRows_Pixel24_t(bmp_Pixel24_t *dest, FILE *bitmap);

ssize_t writeToFilePixel24_t(bmp_Pixel24_t *bitmap, FILE *outFile);
int freeImageDataPixel24_t(bmp_Pixel24_t *bitmap);
