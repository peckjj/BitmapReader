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

	dest->pixelArray = malloc(dest->dibHeader->rawImageSize);

	if (fread(dest->pixelArray, 1, dest->dibHeader->rawImageSize, bitmap) != dest->dibHeader->rawImageSize)
	{
		fprintf(stderr, "readBitmapPixelData(): Failed to read image data");
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

size_t removeRedPixel24_t(bmp_Pixel24_t *bitmap)
{
	int64_t numPixels = bitmap->dibHeader->width * bitmap->dibHeader->height;

	for (int64_t i = 0; i < numPixels; i++)
	{
		bitmap->pixelArray[i].r = 0;
	}

	return numPixels;
}

size_t writeToFilePixel24_t(bmp_Pixel24_t *bitmap, FILE* outFile)
{
	size_t bytesWritten = 0;
	bytesWritten += fwrite(bitmap->bmpHeader, 1, sizeof(BmpFileHeader), outFile);
	bytesWritten += fwrite(bitmap->dibHeader, 1, sizeof(DibHeader), outFile);
	bytesWritten += fwrite(bitmap->postHeaderData, 1, bitmap->postHeaderDataSize, outFile);
	bytesWritten += fwrite(bitmap->pixelArray, 1, bitmap->dibHeader->rawImageSize, outFile);
	bytesWritten += fwrite(bitmap->postData, 1, bitmap->postDataSize, outFile);
	return bytesWritten;
}

int freeImageDataPixel24_t(bmp_Pixel24_t *bitmap)
{
	free(bitmap->pixelArray);
}
