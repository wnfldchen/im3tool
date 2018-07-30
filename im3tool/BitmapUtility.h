#pragma once
#include "BitmapFile.h"

class BitmapUtility
{
protected:
	// Utility types for color spaces
	// Normalized RGB: [0,1]
	struct NormalizedRGB {
		DOUBLE R;
		DOUBLE G;
		DOUBLE B;
	};
	// Hue Saturation Value: H:[0,360] S:[0,1] V:[0,1]
	struct HSV {
		DOUBLE H;
		DOUBLE S;
		DOUBLE V;
	};
	// YUV: [0,1]
	struct YUV {
		DOUBLE Y;
		DOUBLE U;
		DOUBLE V;
	};

	// Utility functions
	// Clamp a value to a range
	template <typename T>
	T ClampToRange(T n, T min, T max) {
		return std::max(min, std::min(n, max));
	}
	// Floating point equality using an epsilon
	BOOL FloatingPointEquals(DOUBLE x, DOUBLE y);

	// Color space transformation functions

	// Normalized RGB <---> Pixel
	NormalizedRGB PixelToNormalizedRGB(BitmapFile::Pixel pixel);
	BitmapFile::Pixel NormalizedRGBtoPixel(NormalizedRGB rgb);

	// Normalized RGB <---> YUV
	NormalizedRGB YUVtoNormalizedRGB(YUV yuv);
	YUV NormalizedRGBtoYUV(NormalizedRGB rgb);

	// Normalized RGB <---> HSV
	NormalizedRGB HSVtoNormalizedRGB(HSV hsv);
	HSV NormalizedRGBtoHSV(NormalizedRGB rgb);
};

