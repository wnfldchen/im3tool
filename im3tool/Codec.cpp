#include "stdafx.h"
#include <cmath>
#include <limits>
#include "BitmapUtility.h"
#include "commontypes.h"
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
	static const DOUBLE a = M_SQRT1_2;
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
		for (INT32 j = 0; j < width; j += 8) {
			INT32 blockY = i / 8;
			INT32 blockX = j / 8;
			output.planes[Y][blockY][blockX] = quantizeOnBlock<INT16, INT8>(dct.planes[Y][blockY][blockX]);
			output.planes[U][blockY][blockX] = quantizeOnBlock<INT16, INT8>(dct.planes[U][blockY][blockX]);
			output.planes[V][blockY][blockX] = quantizeOnBlock<INT16, INT8>(dct.planes[V][blockY][blockX]);
		}
	}
	return output;
}

std::pair<CodedDC, CodedAC> Codec::runLengthDifferenceCoder(const Codec::YUVPlanes<INT8>& quantized)
{
	// Difference coding DC components
	std::array<std::vector<INT8>, 3> dcDifferences;
	// Run-length coding AC components
	std::array<std::vector<std::pair<UINT8, INT8>>, 3> runLengthCodes;
	// Get plane dimensions
	INT32 width = quantized.getWidth();
	INT32 height = quantized.getHeight();
	// For each channel
	for (UINT8 channel = 0; channel < 3; channel++) {
		// For each block
		INT8 lastDCValue = 0;
		for (INT32 i = 0; i < height; i += 8) {
			for (INT32 j = 0; j < width; j += 8) {
				INT32 blockY = i / 8;
				INT32 blockX = j / 8;
				Block<INT8> block = quantized.planes[channel][blockY][blockX];
				// Encode the DC difference from last block
				dcDifferences[channel].push_back(block[0][0] - lastDCValue);
				lastDCValue = block[0][0];
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
				runLengthCodes[channel].push_back(std::pair<UINT8, INT8>{ 0, 0 });
			}
		}
	}
	std::pair<CodedDC, CodedAC> result(dcDifferences, runLengthCodes);
	return result;
}

std::array<std::pair<EntropiedDC, EntropiedAC>, 3>
Codec::entropyCoder(const CodedDC& codedDC, const CodedAC& codedAC)
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

std::pair<CodedDC, CodedAC> Codec::entropyDecoder(const FileHeaderWithTables & fileHeaderWithTables, std::vector<bool>& bitsReadFromFile)
{
	UINT8 blocksWide = fileHeaderWithTables.FileHeader.BlocksWide;
	UINT8 blocksHigh = fileHeaderWithTables.FileHeader.BlocksHigh;
	INT32 numBlocks = blocksWide * blocksHigh;
	CodedDC codedDC;
	CodedAC codedAC;
	for (UINT8 i = 0; i < 3; i++) {
		const PlaneHeader* planeHeader = NULL;
		UINT16 acZeroesBytes;
		UINT16 acValuesBytes;
		switch (i)
		{
		case 0:
			planeHeader = &(fileHeaderWithTables.YPlaneHeader);
			acZeroesBytes = fileHeaderWithTables.FileHeader.YACZeroesBytes;
			acValuesBytes = fileHeaderWithTables.FileHeader.YACValuesBytes;
			break;
		case 1:
			planeHeader = &(fileHeaderWithTables.UPlaneHeader);
			acZeroesBytes = fileHeaderWithTables.FileHeader.UACZeroesBytes;
			acValuesBytes = fileHeaderWithTables.FileHeader.UACValuesBytes;
			break;
		case 2:
			planeHeader = &(fileHeaderWithTables.VPlaneHeader);
			acZeroesBytes = fileHeaderWithTables.FileHeader.VACZeroesBytes;
			acValuesBytes = fileHeaderWithTables.FileHeader.VACValuesBytes;
			break;
		}
		LengthTable<INT8> dcLengths;
		LengthTable<UINT8> acZeroesLengths;
		LengthTable<INT8> acValuesLengths;
		std::copy(
			std::begin(planeHeader->DCLengths),
			std::end(planeHeader->DCLengths),
			dcLengths.begin());
		std::copy(
			std::begin(planeHeader->ACZeroesLengths),
			std::end(planeHeader->ACZeroesLengths),
			acZeroesLengths.begin());
		std::copy(
			std::begin(planeHeader->ACValuesLengths),
			std::end(planeHeader->ACValuesLengths),
			acValuesLengths.begin());
		std::vector<INT8> dcDifferences =
			huffmanDecode<INT8>(
				dcLengths,
				bitsReadFromFile,
				numBlocks);
		std::vector<bool> entropiedACZeroes(
			bitsReadFromFile.begin(),
			bitsReadFromFile.begin() + (acZeroesBytes * 8));
		bitsReadFromFile.erase(
			bitsReadFromFile.begin(),
			bitsReadFromFile.begin() + (acZeroesBytes * 8));
		std::vector<UINT8> acZeroes =
			huffmanDecode<UINT8>(
				acZeroesLengths,
				entropiedACZeroes);
		std::vector<bool> entropiedACValues(
			bitsReadFromFile.begin(),
			bitsReadFromFile.begin() + (acValuesBytes * 8));
		bitsReadFromFile.erase(
			bitsReadFromFile.begin(),
			bitsReadFromFile.begin() + (acValuesBytes * 8));
		std::vector<INT8> acValues =
			huffmanDecode<INT8>(
				acValuesLengths,
				entropiedACValues);
		codedDC[i] = dcDifferences;
		std::vector<std::pair<UINT8, INT8>> runLengthCodedAC(acZeroes.size());
		INT32 blocksCoded = 0;
		for (INT32 j = 0; j < runLengthCodedAC.size(); j++) {
			runLengthCodedAC[j] = { acZeroes[j], acValues[j] };
			if (acValues[j] == 0) {
				blocksCoded += 1;
			}
			if (blocksCoded == numBlocks) {
				break;
			}
		}
		codedAC[i] = runLengthCodedAC;
	}
	return std::pair<CodedDC, CodedAC>(codedDC, codedAC);
}

Codec::YUVPlanes<INT8> Codec::runLengthDifferenceDecoder(
	const FileHeaderWithTables & fileHeaderWithTables,
	const std::pair<CodedDC, CodedAC> & runLengthDifferenceCoded)
{
	UINT8 blocksWide = fileHeaderWithTables.FileHeader.BlocksWide;
	UINT8 blocksHigh = fileHeaderWithTables.FileHeader.BlocksHigh;
	INT32 imageWidth = blocksWide * 8;
	INT32 imageHeight = blocksHigh * 8;
	// DC differences
	CodedDC dcDifferences = runLengthDifferenceCoded.first;
	// Run-length coded AC
	CodedAC runLengthCodes = runLengthDifferenceCoded.second;
	// Create planes
	YUVPlanes<INT8> quantized(imageWidth, imageHeight);
	// For each channel
	for (UINT8 channel = 0; channel < 3; channel++) {
		// Previous block's DC component
		INT8 dc = 0;
		// For each block
		for (INT32 i = 0; i < imageHeight; i += 8) {
			INT32 blockY = i / 8;
			for (INT32 j = 0; j < imageWidth; j += 8) {
				INT32 blockX = j / 8;
				// Create block
				Block<INT8> block;
				block.fill(std::array<INT8, 8>{});
				// Decode the DC differences
				dc += dcDifferences[channel][0];
				dcDifferences[channel].erase(dcDifferences[channel].begin());
				block[0][0] = dc;
				// Decode the zig-zag traversed run-length codes
				std::pair<UINT8, INT8> code;
				for (auto it = Z.begin(); it != Z.end(); it++) {
					code = runLengthCodes[channel][0];
					runLengthCodes[channel].erase(runLengthCodes[channel].begin());
					if (code.first == 0 && code.second == 0) {
						break;
					}
					while (code.first > 0) {
						it++;
						code.first -= 1;
					}
					block[it->second][it->first] = code.second;
				}
				if (!(code.first == 0 && code.second == 0)) {
					runLengthCodes[channel].erase(runLengthCodes[channel].begin());
				}
				quantized.planes[channel][blockY][blockX] = block;
			}
		}

	}
	return quantized;
}

Codec::YUVPlanes<INT16> Codec::dequantize(const YUVPlanes<INT8>& quantized)
{
	INT32 width = quantized.getWidth();
	INT32 height = quantized.getHeight();
	YUVPlanes<INT16> output(width, height);
	for (INT32 i = 0; i < height; i += 8) {
		INT32 blockY = i / 8;
		for (INT32 j = 0; j < width; j += 8) {
			INT32 blockX = j / 8;
			output.planes[Y][blockY][blockX] = dequantizeOnBlock<INT8, INT16>(quantized.planes[Y][blockY][blockX]);
			output.planes[U][blockY][blockX] = dequantizeOnBlock<INT8, INT16>(quantized.planes[U][blockY][blockX]);
			output.planes[V][blockY][blockX] = dequantizeOnBlock<INT8, INT16>(quantized.planes[V][blockY][blockX]);
		}
	}
	return output;
}

Codec::YUVPlanes<INT8> Codec::inverseDCT(const YUVPlanes<INT16>& dct)
{
	INT32 width = dct.getWidth();
	INT32 height = dct.getHeight();
	YUVPlanes<INT8> output(width, height);
	for (INT32 i = 0; i < height; i += 8) {
		INT32 blockY = i / 8;
		for (INT32 j = 0; j < width; j += 8) {
			INT32 blockX = j / 8;
			output.planes[Y][blockY][blockX] = inverseDCTOnBlock<INT16, INT8>(dct.planes[Y][blockY][blockX]);
			output.planes[U][blockY][blockX] = inverseDCTOnBlock<INT16, INT8>(dct.planes[U][blockY][blockX]);
			output.planes[V][blockY][blockX] = inverseDCTOnBlock<INT16, INT8>(dct.planes[V][blockY][blockX]);
		}
	}
	return output;
}

BitmapFile * Codec::YUVToBitmap(const YUVPlanes<INT8>& yuv)
{
	INT32 width = yuv.getWidth();
	INT32 height = yuv.getHeight();
	BitmapFile* bitmapFile = new BitmapFile(width, height);
	for (INT32 i = 0; i < height; i++) {
		INT32 blockY = i / 8;
		INT32 offsetY = i % 8;
		for (INT32 j = 0; j < width; j++) {
			INT32 blockX = j / 8;
			INT32 offsetX = j % 8;
			INT32 scaledY = yuv.planes[Y][blockY][blockX][offsetY][offsetX];
			INT32 scaledU = yuv.planes[U][blockY][blockX][offsetY][offsetX];
			INT32 scaledV = yuv.planes[V][blockY][blockX][offsetY][offsetX];
			YUV yuvOutput;
			yuvOutput.Y = static_cast<DOUBLE>(scaledY + 128) / 255.0;
			yuvOutput.U = static_cast<DOUBLE>(scaledU + 128) / 255.0;
			yuvOutput.V = static_cast<DOUBLE>(scaledV + 128) / 255.0;
			NormalizedRGB rgbOutput = YUVtoNormalizedRGB(yuvOutput);
			BitmapFile::Pixel pixelOutput = NormalizedRGBtoPixel(rgbOutput);
			bitmapFile->setPixel(j, i, pixelOutput);
		}
	}
	return bitmapFile;
}

IM3File* Codec::compress(BitmapFile * bitmapFile)
{
	Codec::YUVPlanes<INT8> yuv = bitmapToYUV(bitmapFile);
	Codec::YUVPlanes<INT16> dctCoefficients = dct(yuv);
	Codec::YUVPlanes<INT8> quantized = quantize(dctCoefficients);
	std::pair<CodedDC, CodedAC> runLengthDifferenceCoded =
		runLengthDifferenceCoder(quantized);
	std::array<std::pair<EntropiedDC, EntropiedAC>, 3> entropyCoded = entropyCoder(
		runLengthDifferenceCoded.first, runLengthDifferenceCoded.second);
	UINT8 blocksWide = yuv.getWidth() / 8;
	UINT8 blocksHigh = yuv.getHeight() / 8;
	IM3File* file = new IM3File(blocksWide, blocksHigh, entropyCoded);
	return file;
}

BitmapFile * Codec::decompress(IM3File* im3File)
{
	FileHeaderWithTables fileHeaderWithTables =
		im3File->getFileHeaderWithTables();
	std::vector<bool> bitsReadFromFile =
		im3File->getBitsReadFromFile();
	std::pair<CodedDC, CodedAC> runLengthDifferenceCoded =
		entropyDecoder(fileHeaderWithTables, bitsReadFromFile);
	YUVPlanes<INT8> quantized = runLengthDifferenceDecoder(
		fileHeaderWithTables,
		runLengthDifferenceCoded);
	YUVPlanes<INT16> dct = dequantize(quantized);
	YUVPlanes<INT8> yuv = inverseDCT(dct);
	return YUVToBitmap(yuv);
}

Codec::Codec()
{
}
