#pragma once
#include "stdafx.h"

#include <array>
#include <vector>
#include <limits>
#include <utility>

// Common alias templates
template <typename T>
using LengthTable = std::array<UINT8, std::numeric_limits<T>::max() - std::numeric_limits<T>::min() + 1>;

// Common typedefs
typedef std::array<std::vector<INT8>, 3> CodedDC;
typedef std::array<std::vector<std::pair<UINT8, INT8>>, 3> CodedAC;
typedef std::pair<LengthTable<INT8>, std::vector<bool>> EntropiedDC;
typedef std::pair<LengthTable<UINT8>, std::vector<bool>> EntropiedACFirst;
typedef std::pair<LengthTable<INT8>, std::vector<bool>> EntropiedACSecond;
typedef std::pair<EntropiedACFirst, EntropiedACSecond> EntropiedAC;

// Structures
// File structure types
// Structure packing set to 1-byte to have continuous reading
#pragma pack(push, 1)
// File Header
struct FileHeader {
	UINT8 MagicByteI = 73; // 'I' == 73
	UINT8 MagicByteM = 77; // 'M' == 77
	UINT8 BlocksWide; // Width of image in blocks
	UINT8 BlocksHigh; // Height of image in blocks
	UINT16 YACZeroesBytes; // Number of bytes of Y plane Run-Length Zeroes
	UINT16 YACValuesBytes; // Number of bytes of Y plane Run-Length Values
	UINT16 UACZeroesBytes; // Number of bytes of U plane Run-Length Zeroes
	UINT16 UACValuesBytes; // Number of bytes of U plane Run-Length Values
	UINT16 VACZeroesBytes; // Number of bytes of V plane Run-Length Zeroes
	UINT16 VACValuesBytes; // Number of bytes of V plane Run-Length Values
};
// Plane Header
struct PlaneHeader {
	UINT8 DCLengths[256]; // Difference-Coded DC Canonical Huffman Table
	UINT8 ACZeroesLengths[256]; // Run-Length Zeroes AC Canonical Huffman Table
	UINT8 ACValuesLengths[256]; // Run-Length Values AC Canonical Huffman Table
};
// File Header With Tables
struct FileHeaderWithTables {
	FileHeader FileHeader;
	PlaneHeader YPlaneHeader;
	PlaneHeader UPlaneHeader;
	PlaneHeader VPlaneHeader;
};
#pragma pack(pop)
