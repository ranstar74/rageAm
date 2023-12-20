#define STB_IMAGE_IMPLEMENTATION

#include <cstdint>

namespace rageam::graphics
{
	extern void* ImageAlloc(uint32_t size);
	extern void* ImageAllocTemp(uint32_t size);
	extern void* ImageReAlloc(void* block, uint32_t newSize);
	extern void* ImageReAllocTemp(void* block, uint32_t newSize);
	extern void ImageFree(void* block);
	extern void ImageFreeTemp(void* block);
}

#define STBI_MALLOC(size,user_data)			rageam::graphics::ImageAlloc(size)
#define STBI_REALLOC(ptr, size,user_data)	rageam::graphics::ImageReAllocTemp(ptr, size)
#define STBI_FREE(ptr,user_data)			rageam::graphics::ImageFree(ptr)

#include "stb_image.h"
