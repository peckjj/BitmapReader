#ifndef IMAGE
#include "image.h"
#endif

int	main(int argc, char *argv[]);

ssize_t	writeInfo(char *out, size_t outMaxSize, bmp_Pixel24_t *bitmap);
int	generateNewImage(char* filename);
