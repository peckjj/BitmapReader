#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "main.h"
#include "util.h"
#ifndef PIXEL
#include "pixel.h"
#endif
#ifndef IMAGE
#include "image.h"
#endif

#define W 53
#define H 8

int generateNewImage(char *filename)
{
	// Create bitmap structure on stack
	bmp_Pixel24_t bmp;
	DibHeader dibHeader;
	BmpFileHeader bmpHeader;
	Pixel24_t pixelData[W * H];

	bmp.fileName = filename;
	bmp.bmpHeader = &bmpHeader;
		// Create bmp header info
	bmpHeader.signature[0] = 'B'; bmpHeader.signature[1] = 'M';
	bmpHeader.fileSize = sizeof(BmpFileHeader) + sizeof(DibHeader) + (W * H * 3);
	bmpHeader.r1 = 0;
	bmpHeader.r2 = 0;
	bmpHeader.dataOffset = sizeof(BmpFileHeader) + sizeof(DibHeader);

	bmp.dibHeader = &dibHeader;
		// Create Dib header info
	dibHeader.headerSize = sizeof(DibHeader);
	dibHeader.width = W;
	dibHeader.height = H;
	dibHeader.numPlanes = 1;
	dibHeader.bitsPerPixel = 24;
	dibHeader.compressionMethod = 0;
	dibHeader.rawImageSize = (W * H * 3); // 4 pixels * 4 pixels * 3 bytes per pixel
	dibHeader.horRes = 0;
	dibHeader.verRes = 0;
	dibHeader.numColors = 0;
	dibHeader.numImportantColors = 0;

	bmp.postHeaderDataSize = 0;
	bmp.postHeaderData = NULL;
	bmp.pixelArray = pixelData;
	bmp.postDataSize = 0;
	bmp.postData = NULL;


	// create pixel data, with basic gradient for now.
	float step = 256.0 / (W * H);
	uint8_t val = 0;
	for (int i = 0; i < W * H; i++)
	{
		val = i * step;
		if (val >= 256)
		{
			val = 255; // cap to 255 max (oxFFFFFF)
		}

		pixelData[i].r = val;
		pixelData[i].g = val;
		pixelData[i].b = val;
	}

	FILE *output = fopen(filename, "wb");

	if (output == NULL)
	{
		fprintf(stderr, "Failed to open %s for writing\n", filename);
		return -1;
	}

	ssize_t bytesWritten = writeToFilePixel24_t(&bmp, output);

	if (bytesWritten < 0)
	{
		fprintf(stderr, "Failed to write to %s\n", filename);
		return -1;
	}

	fclose(output);

	return 0;
}

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
	bool genNewImage = false;

	srand(time(NULL));

	if (argc < 2)
	{
		printf("No file provided.\n");
		return 0;
	}

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "gen") == 0)
		{
			genNewImage = true;
			break;
		}
	}
	if (genNewImage)
	{
		printf("Generating new image...\n");
		generateNewImage(argv[1]);
		printf("Done.\n");
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

		printf("Read file info\n");
	}

	if (!ERR)
	{
		printf("Allocating fileInfo\n");

		char fileInfo[1000];

		printf("Allocated file info\n");

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
		/*
		for (int row = 0; row < bitmap.dibHeader->height / 2; row++)
		{
			for (int col = 0; col < bitmap.dibHeader->width; col++)
			{
				printf("(%d, %d) -> (%d, %d)\n", col, row, col, bitmap.dibHeader->height - row - 1);
				swapPixel(&bitmap, col, row, bitmap.dibHeader->height - row - 1, col);
			}
		}
		*/
	}


	// Write image to file if filename arg was provided
	if (!ERR && !genNewImage)
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
        "\tPost-Header Data Size: %lu\n"
        "\tPost-Image Data Size: %lu\n"
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
