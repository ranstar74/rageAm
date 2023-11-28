#define STB_IMAGE_IMPLEMENTATION

// TODO: We need ImageReAlloc

//#include <cstdint>
//
//namespace rageam::graphics
//{
//	extern void* ImageAlloc(uint32_t size);
//	extern void* ImageAllocTemp(uint32_t size);
//	extern void ImageFree(void* block);
//	extern void ImageFreeTemp(void* block);
//}
//
//#define STBI_MALLOC(size,user_data) rageam::graphics::ImageAllocTemp(size)
//#define STBI_FREE(ptr,user_data) rageam::graphics::ImageFreeTemp(ptr)

#include "stb_image.h"
