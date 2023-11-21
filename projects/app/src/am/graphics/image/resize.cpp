#include "resize.h"

#include "avir.h"

void rageam::graphics::ImageResizer::Resample(
	pVoid src, pVoid dst, ImagePixelFormat fmt, int xFrom, int yFrom, int xTo, int yTo)
{
	using fpclass_dith = 
		avir::fpclass_def<float, float, avir::CImageResizerDithererErrdINL<float>>;

	// TODO: We should add this as an option
	static avir::CImageResizerParamsUltra roptions;
	static avir::CImageResizer<fpclass_dith> avirResizer(8, 0, roptions);

	int elementCount = static_cast<int>(ImagePixelFormatToSize[fmt]);
	avirResizer.resizeImage((u8*)src, xFrom, yFrom, 0, (u8*)dst, xTo, yTo, elementCount, 0);
}
