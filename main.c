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

void swapPixel(bmp_Pixel24_t *bmp, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	Pixel24_t *a, *b;
	Pixel24_t temp;

	a = getPixel(bmp, x1, y1);
	b = getPixel(bmp, x2, y2);

	if (a == NULL || b == NULL) return;

	temp = *a;

	setPixel(bmp, x1, y1, b, CHAN_RED | CHAN_GREEN | CHAN_BLUE);
	setPixel(bmp, x2, y2, &temp, CHAN_RED | CHAN_GREEN | CHAN_BLUE);
}

int main(int argc, char *argv[])
{
    int ERR = 0;
    char dataBuf[1000];
    bmp_Pixel24_t bitmap;
    BmpFileHeader *bmpHeader;
    DibHeader *dibHeader;
    FILE *file;

    srand(time(NULL));

    if (argc < 2)
    {
        printf("No file provided.\n");
        return 0;
    }

    file = fopen(argv[1], "r");

    if (file == NULL)
    {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        ERR = -1;
    }

    if (!ERR)
    {
        bitmap.fileName = argv[1];
        size_t bytesRead = fread(dataBuf, 1, sizeof(BmpFileHeader) + sizeof(DibHeader), file);

        if (bytesRead != sizeof(BmpFileHeader) + sizeof(DibHeader))
        {
            fprintf(stderr, "Failed to read header data, bytes read=%lu, expected %lu.\n", bytesRead, sizeof(BmpFileHeader) + sizeof(DibHeader));
            ERR = -1;
        }
    }

    if (!ERR)
    {
        bmpHeader = (BmpFileHeader *)dataBuf;
        dibHeader = (DibHeader *)&(dataBuf[sizeof(BmpFileHeader)]);

        if (dibHeader->bitsPerPixel != 24)
        {
            printf("The image does not use 24 bits (3 bytes) per pixel. This is whack and the image format is not supported.\n");
            ERR = -1;
        }
    }

    if (!ERR)
    {
        bitmap.bmpHeader = bmpHeader;
        bitmap.dibHeader = dibHeader;
        if (readBitmapPixelData(&bitmap, file) < 0)
        {
            ERR = -1;
        }
    }

    if (!ERR)
    {
        char *fileInfo = malloc(1000);
        writeInfo(fileInfo, 1000, &bitmap);
        printf("%s", fileInfo);
    }

    if (!ERR)
    {

        if ((dibHeader->width * dibHeader->height * 3) != dibHeader->rawImageSize)
        {
            unsigned difference = dibHeader->rawImageSize - (dibHeader->width * dibHeader->height * 3);

            if (difference < 0)
            {
                fprintf(stderr, "Raw image size is smaller than calculated size. This is impossible.  Difference=%u\n", difference);
                ERR = -1;
            }

            printf("Raw image data contains %u more bytes than required space. It could be that the \"raw image size\" in the file metadata is incorrect or unset. This is also fine if the 'Padding per row' is %u.\n", difference, difference / dibHeader->height);
        }
    }

    if (!ERR)
    {
	for (int row = 0; row < bitmap.dibHeader->height / 2; row++)
	{
		for (int col = 0; col < bitmap.dibHeader->width; col++)
		{
			printf("(%d, %d) -> (%d, %d)\n", col, row, col, bitmap.dibHeader->height - row - 1);
			swapPixel(&bitmap, col, row, bitmap.dibHeader->height - row - 1, col);
		}
	}
    }


    // Write image to file if filename arg was provided
    if (!ERR)
    {
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
    }

    return ERR;
}

ssize_t writeInfo(char *out, size_t outMaxize, bmp_Pixel24_t *bitmap)
{
    BmpFileHeader *bmpHeader = bitmap->bmpHeader;
    DibHeader *dibHeader = bitmap->dibHeader;
    uint32_t calculatedSize = sizeof(BmpFileHeader) + sizeof(DibHeader) + bitmap->postHeaderDataSize +
                              dibHeader->rawImageSize + bitmap->postDataSize;

    return snprintf(
        out,
        outMaxize,
        "Header data for %s (%lu bytes):\n"
        "\tSignature: %c%c (%s)\n"
        "\tFile Size: (%u B) %0.2f MB\n"
        "\tReserved 1 (unsigned): %u\n"
        "\tReserved 2 (unsigned): %u\n"
        "\tImage Data Offset: %u (0x%X)\n"
        "DIB Header Size: %u\n"
        "\tWidth: %dpx\n"
        "\tHeight: %dpx\n"
        "\tColors Planes: %u\n"
        "\tBits Per Pixel: %u\n"
        "\tCompression Method: %u\n"
        "\tRaw Image Size: %0.2f MB\n"
        "\tHor. Resolution: %d\n"
        "\tVer. Resolution: %d\n"
        "\tNumber of colors: %u\n"
        "\tNumber of important colors: %u\n"
        "Additional Info:\n"
        "\tPost-Header Data Size: %u\n"
        "\tPost-Image Data Size: %u\n"
        "\tTotal Calculated Size (%u B): %0.2f MB\n",
        bitmap->fileName, sizeof(BmpFileHeader) + sizeof(DibHeader),
        bmpHeader->signature[0], bmpHeader->signature[1], isSigTypeSupported(bmpHeader->sigCode) ? "Supported" : "Unsupported",
        bmpHeader->fileSize, bmpHeader->fileSize / 1024.0 / 1024.0,
        bmpHeader->r1,
        bmpHeader->r2,
        bmpHeader->dataOffset, bmpHeader->dataOffset,
        dibHeader->headerSize,
        dibHeader->width,
        dibHeader->height,
        dibHeader->numPlanes,
        dibHeader->bitsPerPixel,
        dibHeader->compressionMethod,
        dibHeader->rawImageSize / 1024.0 / 1024.0,
        dibHeader->horRes,
        dibHeader->verRes,
        dibHeader->numColors,
        dibHeader->numImportantColors,

        bitmap->postHeaderDataSize,
        bitmap->postDataSize,
        calculatedSize, calculatedSize / 1024.0 / 1024.0);
}
