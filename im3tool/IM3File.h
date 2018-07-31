#pragma once
#include <vector>
#include "Codec.h"

class IM3File
{
private:
	// File structure types
	// Structure packing set to 1-byte to have continuous reading
#pragma pack(push, 1)
	// File Header
	struct FileHeader {
		UINT8 MagicByteI = 73; // 'I' == 73
		UINT8 MagicByteM = 77; // 'M' == 77
		UINT8 BlocksWide; // Width of image in blocks
		UINT8 BlocksHigh; // Height of image in blocks
	};
	// Plane Header
	struct PlaneHeader {
		UINT32 DCCounts[256]; // Difference-Coded DC Huffman Table
		UINT32 ACZeroesCounts[256]; // Run-Length Zeroes AC Huffman Table
		UINT32 ACValuesCounts[256]; // Run-Length Values AC Huffman Table
	};
	// File Header With Tables
	struct FileHeaderWithTables {
		FileHeader FileHeader;
		PlaneHeader YPlaneHeader;
		PlaneHeader UPlaneHeader;
		PlaneHeader VPlaneHeader;
	} FileHeaderWithTables;
#pragma pack(pop)
	struct Plane {
		std::vector<bool> DC;
		std::vector<bool> AC0;
		std::vector<bool> AC1;
	};
	struct Planes {
		Plane Y;
		Plane U;
		Plane V;
	} Planes;
public:
	void Save(HANDLE fileHandle);
	IM3File(HANDLE fileHandle);
	IM3File(
		UINT8 blocksWide,
		UINT8 blocksHigh,
		std::array<
		std::pair<Codec::EntropiedDC, Codec::EntropiedAC>, 3
		> entropyCoded);
	~IM3File();
};

