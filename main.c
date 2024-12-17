#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "util.h"
#ifndef PIXEL
	#include "pixel.h"
#endif
#ifndef IMAGE
	#include "image.h"
#endif

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc < 2)
    {
        printf("No file provided.\n");
        return 0;
    }

    char dataBuf[1000];

    BmpFileHeader *bmpHeader;
    DibHeader *dibHeader;

    FILE *file = fopen(argv[1], "r");

    if (file == NULL)
    {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        return -1;
    }

    size_t bytesRead = fread(dataBuf, 1, sizeof(BmpFileHeader) + sizeof(DibHeader), file);

    bmpHeader = (BmpFileHeader *)dataBuf;
    dibHeader = (DibHeader *)&(dataBuf[sizeof(BmpFileHeader)]);

    printf("Header data for %s (bytes read = %lu):\n", argv[1], bytesRead);
    printf("\tSignature: %c%c (%s)\n", bmpHeader->signature[0], bmpHeader->signature[1], isSigTypeSupported(bmpHeader->sigCode) ? "Supported" : "Unsupported");
    printf("\tFile Size: (%u B) %0.2f MB\n", bmpHeader->fileSize, bmpHeader->fileSize / 1024.0 / 1024.0);
    printf("\tReserved 1 (unsigned): %u\n", bmpHeader->r1);
    printf("\tReserved 2 (unsigned): %u\n", bmpHeader->r2);
    printf("\tImage Data Offset: %u (0x%X)\n", bmpHeader->dataOffset, bmpHeader->dataOffset);
    printf("DIB Header Size: %u\n", dibHeader->headerSize);
    printf("\tWidth: %dpx\n", dibHeader->width);
    printf("\tHeight: %dpx\n", dibHeader->height);
    printf("\tColors Planes: %u\n", dibHeader->numPlanes);
    printf("\tBits Per Pixel: %u\n", dibHeader->bitsPerPixel);
    printf("\tCompression Method: %u\n", dibHeader->compressionMethod);
    printf("\tRaw Image Size: %0.2f MB\n", dibHeader->rawImageSize / 1024.0 / 1024.0);
    printf("\tHor. Resolution: %d\n", dibHeader->horRes);
    printf("\tVer. Resolution: %d\n", dibHeader->verRes);
    printf("\tNumber of colors: %u\n", dibHeader->numColors);
    printf("\tNumber of important colors: %u\n", dibHeader->numImportantColors);

    bmp_Pixel24_t bitmap;
    bitmap.bmpHeader = bmpHeader;
    bitmap.dibHeader = dibHeader;

    if (readBitmapPixelData(&bitmap, file) < 0)
    {
        return -1;
    }

    printf("Additional Info:\n");
    printf("\tPost-Header Data Size: %u\n", bitmap.postHeaderDataSize);
    printf("\tPost-Image Data Size: %u\n", bitmap.postDataSize);

    uint32_t calculatedSize = sizeof(BmpFileHeader) + sizeof(DibHeader) + bitmap.postHeaderDataSize +
                              bitmap.dibHeader->rawImageSize + bitmap.postDataSize;

    printf("\tTotal Calculated Size (%u B): %0.2f MB\n", calculatedSize, calculatedSize / 1024.0 / 1024.0);

    fclose(file);

    if ((dibHeader->width * dibHeader->height * 3) != dibHeader->rawImageSize)
    {
        unsigned difference = dibHeader->rawImageSize - (dibHeader->width * dibHeader->height * 3);

        if (difference < 0)
        {
            fprintf(stderr, "Raw image size is smaller than calculated size. This is impossible.  Difference=%u\n", difference);
            return -1;
        }

        printf("Raw image data contains %u more bytes than required space. This is fine if the 'Padding per row' is %u.\n", difference, difference / dibHeader->height);
    }

    int numPixels = bitmap.dibHeader->height * bitmap.dibHeader->width;
    ssize_t pixelsModified = randomChannelPixel24_t(&bitmap, CHAN_RED | CHAN_BLUE | CHAN_GREEN);

    if (numPixels != pixelsModified)
    {
        fprintf(stderr, "Expected to modify %d pixels, but got %ld.\n", numPixels, pixelsModified);
    }

    if (argc >= 3)
    {
        FILE *outFile = fopen(argv[2], "w+");

        if (outFile == NULL)
        {
            fprintf(stderr, "Failed to open or create %s for output\n", argv[2]);
            return -1;
        }

        printf("Writing image data to %s\n", argv[2]);

        writeToFilePixel24_t(&bitmap, outFile);

        fclose(outFile);
    }
    else
    {
        printf("No output file provided.\n");
    }
    return 0;
}
