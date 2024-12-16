#include <stdio.h>
#include <stdlib.h>
#include "image.h"

size_t readBitmapPixelData(bmp_Pixel24_t *dest, FILE* bitmap)
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
		fprintf(stderr, "readBitmapPixelData(): File stream not at right offset, offset is %l, post-header data offset is %d\n",
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
		fprintf(stderr, "readBitmapPixelData(): File stream not at right offset, offset is %l, image data offset is %d\n", fPosition, dest->bmpHeader->dataOffset);
		return -1;
	}

	size_t arrayByteCount = dest->dibHeader->width * dest->dibHeader->height * 3;

	dest->pixelArray = malloc(arrayByteCount);

	//if (fread(dest->pixelArray, 1, dest->dibHeader->rawImageSize, bitmap) != dest->dibHeader->rawImageSize)
	//{
	//	fprintf(stderr, "readBitmapPixelData(): Failed to read image data");
	//	return -1;
	//}

	printf("Reading pixel data...\n");

	unsigned bytesRead = readAllRows_Pixel24_t(dest, bitmap);

	if (bytesRead != arrayByteCount)
	{
		fprintf(stderr, "Read %u bytes (%u pixels), but expected %u bytes (%u pixels)\n", bytesRead, bytesRead / 3, arrayByteCount, arrayByteCount / 3);
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

unsigned readAllRows_Pixel24_t(bmp_Pixel24_t *dest, FILE* bitmap)
{
	size_t bytesWritten = 0; // Will track which Array Index of dest->pixelArray we are on. The size of this array should be Width * Height * 3 bytes per pixel

	// Calculate "bytes per row". BMP files store rows in multiples of 4 bytes (double words). To round off the end, padding is added, and this should be skipped"
	unsigned bytesPerRow = dest->dibHeader->width * 3;
	unsigned paddingBytes = bytesPerRow % 4 == 0 ? 0: 4 - (bytesPerRow % 4); // Skip this amound of bytes after reading each row.

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
		fprintf(stderr, "readAllRows_Pixel24_t(): File stream not at right offset, offset is %l, image data offset is %d\n", fPosition, dest->bmpHeader->dataOffset);
		return -1;
	}

	size_t bytesRead = 0;

	for (int i = 0; i < dest->dibHeader->height; i++)
	{
//		printf("Reading row #%d / %d (Bytes per row = %u, width=%d)\n", i + 1, dest->dibHeader->height, bytesPerRow, dest->dibHeader->width);

		bytesRead = fread((void *)&(((uint8_t *)(dest->pixelArray))[bytesWritten]), 1, bytesPerRow, bitmap);
		if (bytesRead != bytesPerRow)
		{
			fprintf(stderr, "readAllRows_Pixel24_t(): Did not read correct number of bytes for current row. Bytes read=%u, expected=%u\n", bytesRead, bytesPerRow);
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

size_t removeRedPixel24_t(bmp_Pixel24_t *bitmap)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].r = 0;
	}

	return numPixels;
}

size_t removeBluePixel24_t(bmp_Pixel24_t *bitmap)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].b = 0;
	}

	return numPixels;
}

size_t removeGreenPixel24_t(bmp_Pixel24_t *bitmap)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].g = 0;
	}

	return numPixels;
}

size_t randomChannelPixel24_t(bmp_Pixel24_t *bitmap, ColorChannel channel)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	uint32_t mask = 0;
	uint32_t r = 0;

	uint32_t *pxAddr;

	// Byte order is BB GG RR
	switch (channel)
	{
		case CHAN_BLUE:
			mask = 0x00FFFFFF;
			break;
		case CHAN_GREEN:
			mask = 0xFF00FFFF;
			break;
		case CHAN_RED:
			mask = 0xFFFF00FF;
			break;
		default:
			break;
	}

	for (int64_t i = 0; i < numPixels; i++)
	{/*
		pxAddr = (uint32_t*)&(bitmap->pixelArray[i]);
//		printf("Before: %X, ", *pxAddr);

		r = ( (uint32_t)(rand() & 0xFF) << (8 * (channel + 1)) ); // Isn't this the cleverest thing you've ever seen?
		pxAddr = (uint32_t*)&(bitmap->pixelArray[i]);

		*pxAddr = (*pxAddr & mask) + r;

//		printf("After: %X\n", *pxAddr);

	*/

		switch (channel)
		{
			case CHAN_RED:
				bitmap->pixelArray[i].r = (uint8_t)rand();
				break;
			case CHAN_GREEN:
				bitmap->pixelArray[i].g = (uint8_t)rand();
				break;
			default:
				bitmap->pixelArray[i].b = (uint8_t)rand();
		}
	}

	return numPixels;
}

size_t writeToFilePixel24_t(bmp_Pixel24_t *bitmap, FILE* outFile)
{
	size_t bytesWritten = 0;
	bytesWritten += fwrite(bitmap->bmpHeader, 1, sizeof(BmpFileHeader), outFile);
	bytesWritten += fwrite(bitmap->dibHeader, 1, sizeof(DibHeader), outFile);
	bytesWritten += fwrite(bitmap->postHeaderData, 1, bitmap->postHeaderDataSize, outFile);
	bytesWritten += fwrite(bitmap->pixelArray, 1, bitmap->dibHeader->width * bitmap->dibHeader->height * 3, outFile);
	bytesWritten += fwrite(bitmap->postData, 1, bitmap->postDataSize, outFile);
	return bytesWritten;
}

int freeImageDataPixel24_t(bmp_Pixel24_t *bitmap)
{
	free(bitmap->pixelArray);
}
