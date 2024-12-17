#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#ifndef IMAGE
	#include "image.h"
#endif

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

	dest->postHeaderDataSize = (long)(dest->bmpHeader->dataOffset) - fPosition;
	dest->postHeaderData = malloc(dest->postHeaderDataSize);

	if (fread(dest->postHeaderData, 1, dest->postHeaderDataSize, bitmap) != dest->postHeaderDataSize)
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

	dest->pixelArray = malloc(arrayByteCount);

	printf("Reading pixel data...\n");

	unsigned bytesRead = readAllRows_Pixel24_t(dest, bitmap);

	if (bytesRead != arrayByteCount)
	{
		fprintf(stderr, "Read %u bytes (%u pixels), but expected %lu bytes (%lu pixels)\n", bytesRead, bytesRead / 3, arrayByteCount, arrayByteCount / 3);
		return -1;
	}

	// Read Post-Image Data
	dest->postDataSize = dest->bmpHeader->fileSize - (sizeof(BmpFileHeader) +
													  sizeof(DibHeader) + dest->postHeaderDataSize + dest->dibHeader->rawImageSize);
	dest->postData = malloc(dest->postDataSize);

	if (fread(dest->postData, 1, dest->postDataSize, bitmap) != dest->postDataSize)
	{
		fprintf(stderr, "readBitmapPixelData(): Failed to read post-image data");
		return -1;
	}

	return 0;
}

ssize_t readAllRows_Pixel24_t(bmp_Pixel24_t *dest, FILE *bitmap)
{
	size_t bytesWritten = 0; // Will track which Array Index of dest->pixelArray we are on. The size of this array should be Width * Height * 3 bytes per pixel

	// Calculate "bytes per row". BMP files store rows in multiples of 4 bytes (double words). To round off the end, padding is added, and this should be skipped"
	unsigned bytesPerRow = dest->dibHeader->width * 3;
	unsigned paddingBytes = bytesPerRow % 4 == 0 ? 0 : 4 - (bytesPerRow % 4); // Skip this amount of bytes after reading each row.

	printf("Padding per row=%u\n", paddingBytes);

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

		bytesRead = fread((void *)&(((uint8_t *)(dest->pixelArray))[bytesWritten]), 1, bytesPerRow, bitmap);
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

		bytesWritten += bytesRead;
		//		printf("Bytes written: %u, (%u pixels), offset=%u\n", bytesWritten, bytesRead / 3, paddingBytes);
	}

	return bytesWritten;
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
	int paddingBytes = widthInBytes % 4 == 0 ? 0 : (4 - (widthInBytes % 4));

	printf("Padding each row with %d bytes.\n", paddingBytes);

	for (int i = 0; i < bitmap->dibHeader->height; i++)
	{
		bytesWritten += fwrite(&((bitmap->pixelArray[i * bitmap->dibHeader->width])), 3, bitmap->dibHeader->width, outFile);
		for (int ii = 0; ii < paddingBytes; ii++)
		{
			bytesWritten += fwrite(PADDING_BYTE, 1, 1, outFile);
		}
	}

	bytesWritten += fwrite(bitmap->postData, 1, bitmap->postDataSize, outFile);
	return bytesWritten;
}

int freeImageDataPixel24_t(bmp_Pixel24_t *bitmap)
{
	free(bitmap->pixelArray);
	bitmap->pixelArray = NULL;
	return 0;
}
