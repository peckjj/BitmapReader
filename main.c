#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "util.h"
#include "pixel.h"
#include "image.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("No file provided.\n");
        return 0;
    }

    char dataBuf[1000];

    BmpFileHeader *bmpHeader;
    DibHeader *dibHeader;

    FILE *file = fopen(argv[1], "r");
    size_t bytesRead = fread(dataBuf, 1, sizeof(BmpFileHeader) + sizeof(DibHeader), file);

    bmpHeader = (BmpFileHeader*)dataBuf;
    dibHeader = (DibHeader*)&(dataBuf[sizeof(BmpFileHeader)]);

    printf("Header data for %s (bytes read = %u):\n", argv[1], bytesRead);
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

    printf("\tTotal Calculated Size (%u B): %0.2f M\n", calculatedSize, calculatedSize / 1024.0 / 1024.0);

    fclose(file);

    FILE* outFile = fopen(argv[2], "w+");

    removeRedPixel24_t(&bitmap);
    writeToFilePixel24_t(&bitmap, outFile);

    fclose(outFile);
    return 0;
}
