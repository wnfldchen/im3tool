#pragma once
#define _USE_MATH_DEFINES
#include <array>
#include <vector>
#include <utility>
#include <math.h>
#include "IM3File.h"
#include "BitmapFile.h"

class Codec : public BitmapUtility
{
private:
	// Quantization matrix
	std::array<std::array<INT8, 8>, 8> Q = { {
		{ 16, 11, 10, 16, 24, 40, 51, 61 },
		{ 12, 12, 14, 19, 26, 58, 60, 55 },
		{ 14, 13, 16, 24, 40, 57, 69, 56 },
		{ 14, 17, 22, 29, 51, 87, 80, 62 },
		{ 18, 22, 37, 56, 68, 109, 103, 77 },
		{ 24, 35, 55, 64, 81, 104, 113, 92 },
		{ 49, 64, 78, 87, 103, 121, 120, 101 },
		{ 72, 92, 95, 98, 112, 100, 103, 99 }
	} };

	// Zig-zag scan pattern
	std::array<std::pair<INT8, INT8>, 63> Z = { {
		{0,1},{1,0},
		{2,0},{1,1},{0,2},
		{0,3},{1,2},{2,1},{3,0},
		{4,0},{3,1},{2,2},{1,3},{0,4},
		{0,5},{1,4},{2,3},{3,2},{4,1},{5,0},
		{6,0},{5,1},{4,2},{3,3},{2,4},{1,5},{0,6},
		{0,7},{1,6},{2,5},{3,4},{4,3},{5,2},{6,1},{7,0},
		{7,1},{6,2},{5,3},{4,4},{3,5},{2,6},{1,7},
		{2,7},{3,6},{4,5},{5,4},{6,3},{7,2},
		{7,3},{6,4},{5,5},{4,6},{3,7},
		{4,7},{5,6},{6,5},{7,4},
		{7,5},{6,6},{5,7},
		{6,7},{7,6},
		{7,7}
	} };

	// Template types

	// Before DCT: T == INT8
	// Before Quantization: T == INT16
	// Before Entropy Coding: T == INT8

	// Alias templates
	template <typename T>
	using Block = std::array<std::array<T, 8>, 8>;

	template <typename T>
	using Plane = std::vector<std::vector<Block>>;

	// Keys for each plane
	enum PlaneKeys {
		Y = 0,
		U = 1,
		V = 2
	};

	// Y, U, and V image planes consisting of 8-by-8 blocks
	template <typename T>
	struct YUVPlanes {
		std::array<Plane, 3> planes;
		INT32 getWidth();
		INT32 getHeight();
		YUVPlanes(INT32 width, INT32 height);
	};

	// Utility functions

	// Function C in DCT
	DOUBLE C(UINT8 x);

	// Huffman coding of a bitstream with N-bit symbols
	template <size_t N>
	std::vector<bool> huffmanEncode(std::vector<bool> input);

	// DCT on a 8-by-8 block
	template <typename T, typename U>
	Block<U> dctOnBlock(Block<T> block);

	// Quantization on a 8-by-8 block
	template <typename T, typename U>
	Block<U> quantizeOnBlock(Block<T> block);

	// Compression functions

	// Transform bitmap to YUV planes
	YUVPlanes<INT8> bitmapToYUV(BitmapFile* bitmapFile);

	// Discrete Cosine Transform (DCT) on YUV planes
	YUVPlanes<INT16> dct(YUVPlanes<INT8> yuv);

	// Quantization on DCT'd YUV planes
	YUVPlanes<INT8> quantize(YUVPlanes<INT16> dct);

	// Difference code the DC components and run-length code the AC components
	typedef std::array<std::vector<INT8>, 3> CodedDC;
	typedef std::array<std::vector<std::pair<UINT8, INT8>>, 3> CodedAC;
	std::pair<CodedDC, CodedAC> runLengthDifferenceCoder(Codec::YUVPlanes<INT8> quantized);

	// Entropy coding on run-length difference-encoded AC and DC components
	typedef std::vector<bool> bitsDC;
	typedef std::vector<bool> bitsAC;
	std::array<std::pair<bitsDC, bitsAC>, 3> entropyCoder(Codec::CodedDC codedDC, Codec::CodedAC codedAC);

	// Decompression functions

	// TODO: Implement
	// Entropy decoding
	// Dequantization
	// Inverse DCT
	// YUV to bitmap
public:
	// Compress a bitmap
	IM3File compress(BitmapFile* bitmapFile);
	// Decompress an IM3
	BitmapFile* decompress(IM3File im3File);
};

template<size_t N>
inline std::vector<bool> Codec::huffmanEncode(std::vector<bool> input)
{
	// TODO: Implement
	return std::vector<bool>();
}

template<typename T, typename U>
inline Block<U> Codec::dctOnBlock(Block<T> block) {
	Block<U> output;
	for (UINT8 v = 0; v < 8; v++) {
		for (UINT8 u = 0; u < 8; u++) {
			DOUBLE sum = 0.0;
			for (UINT8 j = 0; j < 8; j++) {
				for (UINT8 i = 0; i < 8; i++) {
					sum += cos((2 * i + 1) * u * M_PI / 16)
						* cos((2 * j + 1) * v * M_PI / 16)
						* block[j][i];
				}
			}
			U F = static_cast<U>(C(u) * C(v) * sum / 4);
			output[v][u] = F;
		}
	}
	return output;
}

template<typename T, typename U>
inline Block<U> Codec::quantizeOnBlock(Block<T> block)
{
	Block<U> output;
	for (UINT8 i = 0; i < 8; i++) {
		for (UINT8 j = 0; j < 8; j++) {
			output[i][j] = block[i][j] / Q[i][j];
		}
	}
	return output;
}

template<typename T>
inline INT32 Codec::YUVPlanes<T>::getWidth()
{
	return planes[0][0].size() * 8;
}

template<typename T>
inline INT32 Codec::YUVPlanes<T>::getHeight()
{
	return planes[0].size() * 8;
}

template<typename T>
inline Codec::YUVPlanes<T>::YUVPlanes(INT32 width, INT32 height)
{
	for (UINT8 i = 0; i < 3; i++) {
		planes[i].resize(height / 8);
		for (INT32 j = 0; j < height; j++) {
			planes[i][j].resize(width / 8);
		}
	}
}
