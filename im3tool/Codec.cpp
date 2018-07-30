#include "stdafx.h"
#include <cmath>
#include "BitmapUtility.h"
#include "Codec.h"

DOUBLE Codec::C(UINT8 x) {
	static const DOUBLE a = M_SQRT2 / 2.0;
	return x == 0 ? a : 1.0;
}

Codec::YUVPlanes<INT8> Codec::bitmapToYUV(BitmapFile * bitmapFile)
{
	INT32 width = bitmapFile->getWidth();
	INT32 height = bitmapFile->getHeight();
	YUVPlanes<INT8> yuvPlanes(width, height);
	for (INT32 i = 0; i < height; i++) {
		for (INT32 j = 0; j < width; j++) {
			BitmapFile::Pixel pixel = bitmapFile->getPixel(j, i);
			YUV yuv = NormalizedRGBtoYUV(PixelToNormalizedRGB(pixel));
			INT8 scaledY = static_cast<INT8>((yuv.Y * 255) - 128);
			INT8 scaledU = static_cast<INT8>((yuv.U * 255) - 128);
			INT8 scaledV = static_cast<INT8>((yuv.V * 255) - 128);
			INT32 blockY = i / 8;
			INT32 blockX = j / 8;
			INT32 offsetY = i % 8;
			INT32 offsetX = j % 8;
			yuvPlanes.planes[Y][blockY][blockX][offsetY][offsetX] = scaledY;
			yuvPlanes.planes[U][blockY][blockX][offsetY][offsetX] = scaledU;
			yuvPlanes.planes[V][blockY][blockX][offsetY][offsetX] = scaledV;
		}
	}
	return yuvPlanes;
}

Codec::YUVPlanes<INT16> Codec::dct(YUVPlanes<INT8> yuv)
{
	INT32 width = yuv.getWidth();
	INT32 height = yuv.getHeight();
	YUVPlanes<INT16> output(width, height);
	for (INT32 i = 0; i < height; i += 8) {
		for (INT32 j = 0; j < width; j += 8) {
			INT32 blockY = i / 8;
			INT32 blockX = j / 8;
			output.planes[Y][blockY][blockX] = dctOnBlock<INT8, INT16>(yuv.planes[Y][blockY][blockX]);
			output.planes[U][blockY][blockX] = dctOnBlock<INT8, INT16>(yuv.planes[U][blockY][blockX]);
			output.planes[V][blockY][blockX] = dctOnBlock<INT8, INT16>(yuv.planes[V][blockY][blockX]);
		}
	}
	return output;
}

Codec::YUVPlanes<INT8> Codec::quantize(YUVPlanes<INT16> dct)
{
	INT32 width = dct.getWidth();
	INT32 height = dct.getHeight();
	YUVPlanes<INT8> output(width, height);
	for (INT32 i = 0; i < height; i += 8) {
		for (INT32 j = 0; j < height; j += 8) {
			INT32 blockY = i / 8;
			INT32 blockX = j / 8;
			output.planes[Y][blockY][blockX] = quantizeOnBlock<INT16, INT8>(dct.planes[Y][blockY][blockX]);
			output.planes[U][blockY][blockX] = quantizeOnBlock<INT16, INT8>(dct.planes[U][blockY][blockX]);
			output.planes[V][blockY][blockX] = quantizeOnBlock<INT16, INT8>(dct.planes[V][blockY][blockX]);
		}
	}
	return output;
}

std::pair<Codec::CodedDC, Codec::CodedAC> Codec::runLengthDifferenceCoder(Codec::YUVPlanes<INT8> quantized)
{
	// Difference coding DC components
	std::array<std::vector<INT8>, 3> dcDifferences;
	for (UINT8 i = 0; i < 3; i++) {
		dcDifferences[i].push_back(0);
	}
	// Run-length coding AC components
	std::array<std::vector<std::pair<UINT8, INT8>>, 3> runLengthCodes;
	// Get plane dimensions
	INT32 width = quantized.getWidth();
	INT32 height = quantized.getHeight();
	// For each channel
	for (UINT8 channel = 0; channel < 3; channel++) {
		// For each block
		for (INT32 i = 0; i < height; i += 8) {
			for (INT32 j = 0; j < width; j += 8) {
				INT32 blockY = i / 8;
				INT32 blockX = j / 8;
				Block<INT8> block = quantized.planes[channel][blockY][blockX];
				// Encode the DC difference from last block
				dcDifferences[channel].push_back(block[0][0] - dcDifferences[channel].back());
				// Encode the run-length codes from zig-zag traversal
				UINT8 numZeroes = 0;
				for (auto it = Z.begin(); it != Z.end(); it++) {
					INT8 offsetY = it->second;
					INT8 offsetX = it->first;
					INT8 value = block[offsetY][offsetX];
					if (value == 0) {
						numZeroes += 1;
					}
					else {
						std::pair<UINT8, INT8> code(numZeroes, value);
						runLengthCodes[channel].push_back(code);
						numZeroes = 0;
					}
				}
				// Encode the end-of-block code
				runLengthCodes[channel].push_back(std::pair<UINT8, UINT8>{ 0, 0 });
			}
		}
	}
	// Remove the leading zero in the DC difference codings
	for (UINT8 i = 0; i < 3; i++) {
		dcDifferences[i].erase(dcDifferences[i].begin());
	}
	std::pair<CodedDC, CodedAC> result(dcDifferences, runLengthCodes);
	return result;
}

std::array<std::pair<Codec::bitsDC, Codec::bitsAC>, 3> Codec::entropyCoder(Codec::CodedDC codedDC, Codec::CodedAC codedAC)
{
	// TODO: Implement
}

IM3File Codec::compress(BitmapFile * bitmapFile)
{
	Codec::YUVPlanes<INT8> yuv = bitmapToYUV(bitmapFile);
	Codec::YUVPlanes<INT16> dctCoefficients = dct(yuv);
	Codec::YUVPlanes<INT8> quantized = quantize(dctCoefficients);
	std::pair<CodedDC, CodedAC> runLengthDifferenceCoded = runLengthDifferenceCoder(quantized);
	std::array<std::pair<bitsDC, bitsAC>, 3> entropyCoded = entropyCoder(
		runLengthDifferenceCoded.first, runLengthDifferenceCoded.second);
	// TODO: Implement
	return IM3File();
}

BitmapFile * Codec::decompress(IM3File im3File)
{
	// TODO: Implement
	return nullptr;
}
