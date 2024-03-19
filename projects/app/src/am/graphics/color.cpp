#include "color.h"

static float ColorConvertHueToRGB(float p, float q, float t)
{
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
	if (t < 1.0f / 2.0f) return q;
	if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
	return p;
}

void rageam::graphics::ColorConvertRGBtoHSL(u8 r, u8 g, u8 b, float& h, float& s, float& l)
{
	float rf = static_cast<float>(r) / 255.0f;
	float gf = static_cast<float>(g) / 255.0f;
	float bf = static_cast<float>(b) / 255.0f;

	float max = rf > gf && rf > bf ? rf : gf > bf ? gf : bf;
	float min = rf < gf && rf < bf ? rf : gf < bf ? gf : bf;

	h = s = l = (max + min) / 2.0f;

	if (fabsf(max - min) < 0.001f)
		h = s = 0.0f;

	else
	{
		float d = max - min;
		s = (l > 0.5f) ? d / (2.0f - max - min) : d / (max + min);

		if (rf > gf && rf > bf)
			h = (gf - bf) / d + (gf < bf ? 6.0f : 0.0f);

		else if (gf > bf)
			h = (bf - rf) / d + 2.0f;

		else
			h = (rf - gf) / d + 4.0f;

		h /= 6.0f;
	}
}

void rageam::graphics::ColorConvertHSLtoRGB(float h, float s, float l, u8& r, u8& g, u8& b)
{
	float rf, gf, bf;
	if (s == 0.0f)
	{
		rf = gf = bf = l;
	}
	else
	{
		float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
		float p = 2.0f * l - q;
		rf = ColorConvertHueToRGB(p, q, h + 1.0f / 3.0f);
		gf = ColorConvertHueToRGB(p, q, h);
		bf = ColorConvertHueToRGB(p, q, h - 1.0f / 3.0f);
	}

	r = static_cast<u8>(rf * 255.0f);
	g = static_cast<u8>(gf * 255.0f);
	b = static_cast<u8>(bf * 255.0f);
}

rageam::graphics::ColorU32 rageam::graphics::ColorTransformLuminosity(ColorU32 color, float themeBackgroundLuminosity)
{
	// https://stackoverflow.com/questions/36778989/vs2015-icon-guide-color-inversion

	static constexpr float HALO_LUMINOSITY = ColorRGBToLuminosity(246, 246, 246);

	float cH, cS, cL;
	ColorConvertRGBtoHSL(color.R, color.G, color.B, cH, cS, cL);

	float haloLuminosity = HALO_LUMINOSITY;
	if (themeBackgroundLuminosity < 0.5f)
	{
		haloLuminosity = 1.0f - haloLuminosity;
		cL = 1.0f - cL;
	}

	if (cL < haloLuminosity)
	{
		cL = themeBackgroundLuminosity * cL / haloLuminosity;
	}
	else
	{
		cL = (1.0f - themeBackgroundLuminosity) * (cL - 1.0f) / (1.0f - haloLuminosity) + 1.0f;
	}

	ColorConvertHSLtoRGB(cH, cS, cL, color.R, color.G, color.B);
	return color;
}

float rageam::graphics::ColorGetLuminosity(ColorU32 color)
{
	float rf = static_cast<float>(color.R) / 255.0f;
	float gf = static_cast<float>(color.G) / 255.0f;
	float bf = static_cast<float>(color.B) / 255.0f;
	float max = rf > gf && rf > bf ? rf : gf > bf ? gf : bf;
	float min = rf < gf && rf < bf ? rf : gf < bf ? gf : bf;
	return (max + min) / 2.0f;
}
