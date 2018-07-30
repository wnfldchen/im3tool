#pragma once

class Codec : public BitmapUtility
{
private:
	// Image pixels in YUV
	struct YUVTable {
		YUV** yuv;
	};

	// Template types

	// Before DCT: T == INT8
	// Before Quantization: T == INT16
	// Before Entropy Coding: T == INT8

	// 8-by-8 block
	template <typename T>
	struct Block {
		T values[8][8];
	};

	// Image plane
	template <typename T>
	struct Plane {
		Block** values;
	};

	// Y, U, and V planes
	template <typename T>
	struct YUVPlanes {
		Plane y;
		Plane u;
		Plane v;
	};
};

