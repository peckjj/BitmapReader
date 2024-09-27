#include <stdbool.h>
#include <stdint.h>
#include "util.h"

bool isSigTypeSupported(BitMapSigType sigType)
{
	bool ret = false;

	switch (sigType)
	{
		case SIGT_BM:
			ret = true;
			break;
		default:
			break;
	}

	return ret;
}
