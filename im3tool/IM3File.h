#pragma once
#include <vector>
#include "commontypes.h"
#include "Codec.h"

// Forward declaration of class dependencies
class Codec;

class IM3File
{
private:
	FileHeaderWithTables fileHeaderWithTables;
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
	std::vector<bool> bitsReadFromFile;
public:
	void Save(HANDLE fileHandle);
	FileHeaderWithTables getFileHeaderWithTables();
	std::vector<bool> getBitsReadFromFile();
	IM3File(HANDLE fileHandle);
	IM3File(
		UINT8 blocksWide,
		UINT8 blocksHigh,
		std::array<
		std::pair<EntropiedDC, EntropiedAC>, 3
		> entropyCoded);
	~IM3File();
};

