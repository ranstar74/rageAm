#pragma once

#include "imgui_internal.h"

namespace ImGui
{
	// https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion

	inline float ColorConvertHueToRGB(float p, float q, float t)
	{
		if (t < 0.0f) t += 1.0f;
		if (t > 1.0f) t -= 1.0f;
		if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
		if (t < 1.0f / 2.0f) return q;
		if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
		return p;
	}

	inline void ColorConvertRGBtoHSL(int r, int g, int b, float& h, float& s, float& l)
	{
		float rf = static_cast<float>(r) / 255.0f;
		float gf = static_cast<float>(g) / 255.0f;
		float bf = static_cast<float>(b) / 255.0f;

		float max = rf > gf && rf > bf ? rf : gf > bf ? gf : bf;
		float min = rf < gf && rf < bf ? rf : gf < bf ? gf : bf;

		h = s = l = (max + min) / 2.0f;

		if (ImAbs(max - min) < 0.001f)
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

	inline void ColorConvertHSLtoRGB(float h, float s, float l, int& r, int& g, int& b)
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

		r = static_cast<int>(rf * 255);
		g = static_cast<int>(gf * 255);
		b = static_cast<int>(bf * 255);
	}
}
