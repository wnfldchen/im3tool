#include "stdafx.h"
#include <algorithm>
#include "BitmapUtility.h"


// Base class BitmapUtility definitions

BOOL BitmapUtility::FloatingPointEquals(DOUBLE x, DOUBLE y) {
	// Compare floating points using an acceptably small tolerance
	static const DOUBLE EPSILON = 1e-4;
	return abs(x - y) <= EPSILON;
}

BitmapUtility::NormalizedRGB BitmapUtility::PixelToNormalizedRGB(BitmapFile::Pixel pixel) {
	// Scale RGB from range [0,255] to [0,1]
	NormalizedRGB rgb = {
		pixel.Red / (DOUBLE)255,
		pixel.Green / (DOUBLE)255,
		pixel.Blue / (DOUBLE)255
	};
	return rgb;
}

BitmapFile::Pixel BitmapUtility::NormalizedRGBtoPixel(NormalizedRGB rgb) {
	// Scale RGB from range [0,1] to [0,255]
	BitmapFile::Pixel pixel;
	pixel.Red = static_cast<BYTE>(rgb.R * 255);
	pixel.Green = static_cast<BYTE>(rgb.G * 255);
	pixel.Blue = static_cast<BYTE>(rgb.B * 255);
	return pixel;
}

BitmapUtility::NormalizedRGB BitmapUtility::YUVtoNormalizedRGB(YUV yuv) {
	// [RGB vector] = [Conversion Matrix]*[YUV vector]
	DOUBLE		Y = yuv.Y, U = yuv.U, V = yuv.V;
	DOUBLE R = 1.000*Y + 0.000*U + 1.000*V;
	DOUBLE G = 1.000*Y - 0.194*U - 0.509*V;
	DOUBLE B = 1.000*Y + 1.000*U + 0.000*V;
	NormalizedRGB rgb = { R, G, B };
	return rgb;
}

BitmapUtility::YUV BitmapUtility::NormalizedRGBtoYUV(NormalizedRGB rgb) {
	// [YUV vector] = [Conversion Matrix]*[RGB vector]
	DOUBLE		R = rgb.R, G = rgb.G, B = rgb.B;
	DOUBLE Y = 0.299*R + 0.587*G + 0.114*B;
	DOUBLE U = -0.299*R - 0.587*G + 0.886*B;
	DOUBLE V = 0.701*R - 0.587*G - 0.114*B;
	YUV yuv = { Y, U, V };
	return yuv;
}

BitmapUtility::NormalizedRGB BitmapUtility::HSVtoNormalizedRGB(HSV hsv) {
	DOUBLE H = hsv.H; // Hue between 0 and 360 degrees
	DOUBLE S = hsv.S; // Saturation normalized between 0 and 1
	DOUBLE V = hsv.V; // Value normalized between 0 and 1

	DOUBLE PrimaryChroma = V * S; // Chroma of the most significant colour
	H /= 60.0; // Locate correct subface of the RGB cube (60 degrees per subface)
			   // Chroma of the second most significant colour
	DOUBLE SecondaryChroma = PrimaryChroma * (1 - abs(fmod(H, 2.0) - 1));

	NormalizedRGB rgb; // RGB output
	switch ((LONG)H) // Color significance order depends on subface of the RGB cube
	{
	case 0:
		rgb = { PrimaryChroma, SecondaryChroma, 0.0 }; // Red, Green are significant
		break;
	case 1:
		rgb = { SecondaryChroma, PrimaryChroma, 0.0 }; // Green, Red are significant
		break;
	case 2:
		rgb = { 0.0, PrimaryChroma, SecondaryChroma }; // Green, Blue are significant
		break;
	case 3:
		rgb = { 0.0, SecondaryChroma, PrimaryChroma }; // Blue, Green are significant
		break;
	case 4:
		rgb = { SecondaryChroma, 0.0, PrimaryChroma }; // Blue, Red are significant
		break;
	case 5:
	case 6:
	default:
		rgb = { PrimaryChroma, 0.0, SecondaryChroma }; // Red, Blue are significant
		break;
	}

	DOUBLE brightnessAdjust = V - PrimaryChroma; // Brightness is value-chroma difference
	rgb.R += brightnessAdjust;
	rgb.G += brightnessAdjust;
	rgb.B += brightnessAdjust;

	return rgb;
}

BitmapUtility::HSV BitmapUtility::NormalizedRGBtoHSV(NormalizedRGB rgb) {
	// RGB inputs normalized between 0 and 1
	DOUBLE R = rgb.R;
	DOUBLE G = rgb.G;
	DOUBLE B = rgb.B;

	// Chroma is the range of RGB values
	DOUBLE min = std::min(std::min(R, G), B);
	DOUBLE max = std::max(std::max(R, G), B);
	DOUBLE chroma = max - min;

	// Hue calculation
	DOUBLE H;
	if (FloatingPointEquals(chroma, 0.0)) // Zero chroma is a graytone
	{
		H = 0; // Zero hue will represent undefined for convenience
	}
	else if (FloatingPointEquals(R, max)) // Red component was the largest
	{
		H = (G - B) / chroma + 6.0; // Hue in the Red section of the HSV chroma plane
	}
	else if (FloatingPointEquals(G, max)) // Green component was the largest
	{
		H = (B - R) / chroma + 2.0; // Hue in the Green section of the HSV chroma plane
	}
	else
		// Case where FloatingPointEquals(B, max) // Blue component was the largest
	{
		H = (R - G) / chroma + 4.0; // Hue in the Blue section of the HSV chroma plane
	}
	H = fmod(H, 6.0);
	H *= 60.0; // Map the 6 sections of the HSV chroma plane to degrees

			   // Value is the max of RGB in the HSV model
	DOUBLE V = max;

	// Saturation is chroma over value with zero for black
	DOUBLE S = FloatingPointEquals(V, 0.0) ? 0 : chroma / V;

	return{ H, S, V };
}
