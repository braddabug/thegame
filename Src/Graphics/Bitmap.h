#ifndef GRAPHICS_BITMAP_H
#define GRAPHICS_BITMAP_H

#include "../Common.h"

namespace Graphics
{
	struct Bitmap
	{
		uint32 Width;
		uint32 Height;
		uint8* Pixels;
	};
}

#endif // GRAPHICS_BITMAP_H
