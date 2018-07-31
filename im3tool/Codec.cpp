#include "stdafx.h"
#include <cmath>
#include "BitmapUtility.h"
#include "Codec.h"
#include "IM3File.h"

// Quantization matrix
const std::array<std::array<INT8, 8>, 8> Codec::Q{ {
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
const std::array<std::pair<INT8, INT8>, 63> Codec::Z{ {
	{ 0,1 },{ 1,0 },
	{ 2,0 },{ 1,1 },{ 0,2 },
	{ 0,3 },{ 1,2 },{ 2,1 },{ 3,0 },
	{ 4,0 },{ 3,1 },{ 2,2 },{ 1,3 },{ 0,4 },
	{ 0,5 },{ 1,4 },{ 2,3 },{ 3,2 },{ 4,1 },{ 5,0 },
	{ 6,0 },{ 5,1 },{ 4,2 },{ 3,3 },{ 2,4 },{ 1,5 },{ 0,6 },
	{ 0,7 },{ 1,6 },{ 2,5 },{ 3,4 },{ 4,3 },{ 5,2 },{ 6,1 },{ 7,0 },
	{ 7,1 },{ 6,2 },{ 5,3 },{ 4,4 },{ 3,5 },{ 2,6 },{ 1,7 },
	{ 2,7 },{ 3,6 },{ 4,5 },{ 5,4 },{ 6,3 },{ 7,2 },
	{ 7,3 },{ 6,4 },{ 5,5 },{ 4,6 },{ 3,7 },
	{ 4,7 },{ 5,6 },{ 6,5 },{ 7,4 },
	{ 7,5 },{ 6,6 },{ 5,7 },
	{ 6,7 },{ 7,6 },
	{ 7,7 }
	} };

DOUBLE Codec::C(const UINT8 x) {
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

Codec::YUVPlanes<INT16> Codec::dct(const YUVPlanes<INT8>& yuv)
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

Codec::YUVPlanes<INT8> Codec::quantize(const YUVPlanes<INT16>& dct)
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

std::pair<Codec::CodedDC, Codec::CodedAC> Codec::runLengthDifferenceCoder(const Codec::YUVPlanes<INT8>& quantized)
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

std::array<std::pair<Codec::EntropiedDC, Codec::EntropiedAC>, 3>
Codec::entropyCoder(const Codec::CodedDC& codedDC, const Codec::CodedAC& codedAC)
{
	std::array<std::pair<EntropiedDC, EntropiedAC>, 3> output;
	for (UINT8 i = 0; i < 3; i++) {
		std::vector<INT8> dcComponents = codedDC[i];
		std::vector<std::pair<UINT8, INT8>> runLengthACComponents = codedAC[i];
		std::vector<UINT8> zeroesACComponents;
		std::vector<INT8> valuesACComponents;
		for (auto it = runLengthACComponents.begin();
			it != runLengthACComponents.end();
			it++) {
			zeroesACComponents.push_back(it->first);
			valuesACComponents.push_back(it->second);
		}
		EntropiedDC entropiedDC = huffmanEncode<INT8>(dcComponents);
		EntropiedACFirst entropiedACFirst = huffmanEncode<UINT8>(zeroesACComponents);
		EntropiedACSecond entropiedACSecond = huffmanEncode<INT8>(valuesACComponents);
		EntropiedAC entropiedAC = EntropiedAC(entropiedACFirst, entropiedACSecond);
		std::pair<EntropiedDC, EntropiedAC> entropiedPlane(entropiedDC, entropiedAC);
		output[i] = entropiedPlane;
	}
	return output;
}

IM3File* Codec::compress(BitmapFile * bitmapFile)
{
	Codec::YUVPlanes<INT8> yuv = bitmapToYUV(bitmapFile);
	Codec::YUVPlanes<INT16> dctCoefficients = dct(yuv);
	Codec::YUVPlanes<INT8> quantized = quantize(dctCoefficients);
	std::pair<CodedDC, CodedAC> runLengthDifferenceCoded = runLengthDifferenceCoder(quantized);
	std::array<std::pair<EntropiedDC, EntropiedAC>, 3> entropyCoded = entropyCoder(
		runLengthDifferenceCoded.first, runLengthDifferenceCoded.second);
	UINT8 blocksWide = yuv.getWidth() / 8;
	UINT8 blocksHigh = yuv.getHeight() / 8;
	IM3File* file = new IM3File(blocksWide, blocksHigh, entropyCoded);
	return file;
}

BitmapFile * Codec::decompress(IM3File* im3File)
{
	// TODO: Implement
	return nullptr;
}

Codec::Codec()
{
}
