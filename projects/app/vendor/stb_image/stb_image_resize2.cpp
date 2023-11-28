#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_AVX2

#include <cstdint>

namespace rageam::graphics
{
	extern void* ImageAlloc(uint32_t size);
	extern void* ImageAllocTemp(uint32_t size);
	extern void ImageFree(void* block);
	extern void ImageFreeTemp(void* block);
}

#define STBIR_MALLOC(size,user_data) rageam::graphics::ImageAllocTemp(size)
#define STBIR_FREE(ptr,user_data) rageam::graphics::ImageFreeTemp(ptr)

#include "stb_image_resize2.h"
