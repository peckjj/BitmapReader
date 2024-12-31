#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#ifndef IMAGE
#include "image.h"
#endif

int32_t calcIdx(bmp_Pixel24_t *bmp, uint16_t x, uint16_t y)
{
	return bmp->dibHeader->width * y + x;
}

Pixel24_t* getPixel(bmp_Pixel24_t *bmp, uint16_t x, uint16_t y)
{
	int32_t idx = calcIdx(bmp, x, y);

	if (idx < 0 || idx >= bmp->dibHeader->width * bmp->dibHeader->height)
	{
		fprintf(stderr, "getPixel(): Index %d is out of bounds.\n", idx);
		return NULL;
	}

	return &(bmp->pixelArray[idx]);
}

int32_t setPixel(bmp_Pixel24_t *bitmap, uint16_t x, uint16_t y, Pixel24_t *value, ColorChannel channel)
{
	int32_t idx = calcIdx(bitmap, x, y);

//	printf("Modifying pixel w/ index: %d\n", idx);

	if (idx < 0 || idx >= bitmap->dibHeader->width * bitmap->dibHeader->height)
	{
		fprintf(stderr, "setPixel(): Index %d is out of bounds.\n", idx);
		return -1;
	}

	Pixel24_t *target = &(bitmap->pixelArray[idx]);

	if (channel & CHAN_RED) target->r = value->r;
	if (channel & CHAN_GREEN) target->g = value->g;
	if (channel & CHAN_BLUE) target->b = value->b;

	return idx;
}

ssize_t readBitmapPixelData(bmp_Pixel24_t *dest, FILE *bitmap)
{
	// Read Post- Header Data
	if (fseek(bitmap, sizeof(BmpFileHeader) + sizeof(DibHeader), SEEK_SET) != 0)
	{
		fprintf(stderr, "readBitmapPixelData(): fseek() to start of post-header data has failed.\n");
		return -1;
	}

	long fPosition = ftell(bitmap);
	if (fPosition != sizeof(BmpFileHeader) + sizeof(DibHeader))
	{
		fprintf(stderr, "readBitmapPixelData(): File stream not at right offset, offset is %ld, post-header data offset is %lu\n",
				fPosition,
				sizeof(BmpFileHeader) + sizeof(DibHeader));
		return -1;
	}

	dest->postHeaderDataSize = (long)(dest->bmpHeader->dataOffset) - (sizeof(BmpFileHeader) + sizeof(DibHeader));

	printf("Allocating %lu bytes for postHeaderDataSize\n", dest->postHeaderDataSize);

	dest->postHeaderData = malloc(dest->postHeaderDataSize);

	if (dest->postHeaderDataSize && fread(dest->postHeaderData, 1, dest->postHeaderDataSize, bitmap) != dest->postHeaderDataSize)
	{
		fprintf(stderr, "readBitmapPixelData(): Failed to read post-header data");
		return -1;
	}

	// Read Image Data
	if (fseek(bitmap, dest->bmpHeader->dataOffset, SEEK_SET) != 0)
	{
		fprintf(stderr, "readBitmapPixelData(): fseek() to start of image data has failed.\n");
		return -1;
	}

	fPosition = ftell(bitmap);
	if (fPosition != dest->bmpHeader->dataOffset)
	{
		fprintf(stderr, "readBitmapPixelData(): File stream not at right offset, offset is %ld, image data offset is %d\n", fPosition, dest->bmpHeader->dataOffset);
		return -1;
	}

	size_t arrayByteCount = dest->dibHeader->width * dest->dibHeader->height * 3;


	printf("Allocating %lu bytes for pixel data\n", arrayByteCount);

	dest->pixelArray = malloc(arrayByteCount);

	printf("Reading pixel data, (arrayByteCount = %lu)\n", arrayByteCount);

	ssize_t bytesRead = readAllRows_Pixel24_t(dest, bitmap);

	if ( bytesRead != (arrayByteCount + (dest->padding * dest->dibHeader->height)) )
	{
		fprintf(stderr, "Read %lu bytes, but expected %lu bytes\n", bytesRead, (arrayByteCount + (dest->padding * dest->dibHeader->height)));
//		return -1;
	}

	printf("Bytes Read: %lu | Pixel Bytes: %lu | Expected: %lu\n", bytesRead, arrayByteCount, (arrayByteCount + (dest->padding * dest->dibHeader->height)));

	// Read Post-Image Data
	dest->postDataSize = dest->bmpHeader->fileSize - (sizeof(BmpFileHeader) + sizeof(DibHeader) + dest->postHeaderDataSize + bytesRead);

	printf("Post data size = %lu\n", dest->postDataSize);

	dest->postData = malloc(dest->postDataSize);

	size_t postDataBytesRead = fread(dest->postData, 1, dest->postDataSize, bitmap);

	if (postDataBytesRead != dest->postDataSize)
	{
		fprintf(stderr, "readBitmapPixelData(): Failed to read post-image data. Bytes read=%lu, Bytes expected=%lu\n", postDataBytesRead, dest->postDataSize);
		return -1;
	}

	printf("DONE\n");

	return 0;
}

void testAlloc(char *s)
{
	char *myData = malloc(100);
	strncpy(myData, s, 100);
	printf("probe: %s\n", myData);

	if (myData == NULL)
	{
		printf("myData is NULL\n");
	}

	free(myData);
}

ssize_t readAllRows_Pixel24_t(bmp_Pixel24_t *dest, FILE *bitmap)
{
	size_t bytesWritten = 0; // Will track which Array Index of dest->pixelArray we are on. The size of this array should be Width * Height * 3 bytes per pixel

	// Calculate "bytes per row". BMP files store rows in multiples of 4 bytes (double words). To round off the end, padding is added, and this should be skipped"
	unsigned bytesPerRow = dest->dibHeader->width * 3;
	uint8_t paddingBytes = bytesPerRow % 4 == 0 ? 0 : 4 - (bytesPerRow % 4); // Skip this amount of bytes after reading each row.

	dest->padding = paddingBytes;
	//	printf("Will read %u bytes\n", bytesPerRow * dest->dibHeader->height);

	// Read Image Data
	if (fseek(bitmap, dest->bmpHeader->dataOffset, SEEK_SET) != 0)
	{
		fprintf(stderr, "readAllRows_Pixel24_t(): fseek() to start of image data has failed.\n");
		return -1;
	}

	size_t fPosition = ftell(bitmap);
	if (fPosition != dest->bmpHeader->dataOffset)
	{
		fprintf(stderr, "readAllRows_Pixel24_t(): File stream not at right offset, offset is %lu, image data offset is %d\n", fPosition, dest->bmpHeader->dataOffset);
		return -1;
	}

	size_t bytesRead = 0;

	for (int i = 0; i < dest->dibHeader->height; i++)
	{
		//		printf("Reading row #%d / %d (Bytes per row = %u, width=%d)\n", i + 1, dest->dibHeader->height, bytesPerRow, dest->dibHeader->width);

		bytesRead = fread(&(dest->pixelArray[i * dest->dibHeader->width]), 3, dest->dibHeader->width, bitmap) * 3;

		if (bytesRead != bytesPerRow)
		{
			fprintf(stderr, "readAllRows_Pixel24_t(): Did not read correct number of bytes for current row. Bytes read=%lu, expected=%u\n", bytesRead, bytesPerRow);
			return -1;
		}

		// Skip padding
		if (fseek(bitmap, paddingBytes, SEEK_CUR) != 0)
		{
			fprintf(stderr, "readAllRows_Pixel24_t(): Could not skip padding bytes. Padding per row=%u\n", paddingBytes);
			return -1;
		}

		bytesWritten += bytesRead + paddingBytes;
		//		printf("Bytes written: %u, (%u pixels), offset=%u\n", bytesWritten, bytesRead / 3, paddingBytes);
	}

	printf("Casting bytesWritten %ld\n", (ssize_t)bytesWritten);

	return (ssize_t)bytesWritten;
}

ssize_t removeRedPixel24_t(bmp_Pixel24_t *bitmap)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].r = 0;
	}

	return numPixels;
}

ssize_t removeBluePixel24_t(bmp_Pixel24_t *bitmap)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].b = 0;
	}

	return numPixels;
}

ssize_t removeGreenPixel24_t(bmp_Pixel24_t *bitmap)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].g = 0;
	}

	return numPixels;
}

ssize_t randomChannelPixel24_t(bmp_Pixel24_t *bitmap, ColorChannel channel)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].r += 50;
		bitmap->pixelArray[i].g -= 50;
		// bitmap->pixelArray[i].b = (char)0xFF; //(uint8_t)rand();
	}

	return numPixels;
}

ssize_t writeToFilePixel24_t(bmp_Pixel24_t *bitmap, FILE *outFile)
{
	char PADDING_BYTE[1] = {0xFF};

	size_t bytesWritten = 0;
	bytesWritten += fwrite(bitmap->bmpHeader, 1, sizeof(BmpFileHeader), outFile);
	bytesWritten += fwrite(bitmap->dibHeader, 1, sizeof(DibHeader), outFile);
	bytesWritten += fwrite(bitmap->postHeaderData, 1, bitmap->postHeaderDataSize, outFile);
	// bytesWritten += fwrite(bitmap->pixelArray, 1, bitmap->dibHeader->width * bitmap->dibHeader->height * 3, outFile);

	int32_t widthInBytes = bitmap->dibHeader->width * 3;
	uint8_t paddingBytes = widthInBytes % 4 == 0 ? 0 : (4 - (widthInBytes % 4));

	bitmap->padding = paddingBytes;

	printf("Padding each row with %d bytes.\n", paddingBytes);

	for (int i = 0; i < bitmap->dibHeader->height; i++)
	{
		bytesWritten += (fwrite(&((bitmap->pixelArray[i * bitmap->dibHeader->width])), 3, bitmap->dibHeader->width, outFile)) * 3;
		for (int ii = 0; ii < paddingBytes; ii++)
		{
			bytesWritten += fwrite(PADDING_BYTE, 1, 1, outFile);
		}
	}

	bytesWritten += fwrite(bitmap->postData, 1, bitmap->postDataSize, outFile);

	// rewrite file size
	fseek(outFile, 2, SEEK_SET);

	uint32_t fileSize = (uint32_t)bytesWritten;

	printf("Updating file size to %u, previously %u (bytes written = %lu)\n", fileSize, bitmap->bmpHeader->fileSize, bytesWritten);

	fwrite(&fileSize, sizeof(uint32_t), 1, outFile);

	return bytesWritten;
}

int freeImageDataPixel24_t(bmp_Pixel24_t *bitmap)
{
	free(bitmap->pixelArray);
	bitmap->pixelArray = NULL;
	return 0;
}
