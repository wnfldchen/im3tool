#include "stdafx.h"
#include <cstring>
#include "IM3File.h"

void IM3File::Save(HANDLE fileHandle)
{
	static const DWORD fileHeaderWithTablesSize = sizeof(FileHeaderWithTables);
	DWORD bytesWritten;
	WriteFile(
		fileHandle,
		&FileHeaderWithTables,
		fileHeaderWithTablesSize,
		&bytesWritten,
		NULL);
	for (UINT8 i = 0; i < 3; i++) {
		Plane* plane = NULL;
		switch (i)
		{
		case 0:
			plane = &(Planes.Y);
			break;
		case 1:
			plane = &(Planes.U);
			break;
		case 2:
			plane = &(Planes.V);
			break;
		}
		for (UINT8 j = 0; j < 3; j++) {
			std::vector<bool>* data = NULL;
			switch (j) {
			case 0:
				data = &(plane->DC);
				break;
			case 1:
				data = &(plane->AC0);
				break;
			case 2:
				data = &(plane->AC1);
				break;
			}
			size_t numBits = data->size();
			DWORD bufSize = static_cast<DWORD>(ceil(numBits / 8.0));
			BYTE* outputBuffer = new BYTE[bufSize];
			BYTE b;
			for (size_t bit = 0; bit < numBits; bit++) {
				BYTE x = !!(data->at(bit));
				b ^= (-x ^ b) & (1 << (bit % 8));
				if ((bit + 1) % 8 == 0) {
					outputBuffer[bit / 8] = b;
				}
			}
			outputBuffer[(numBits - 1) / 8] = b;
			WriteFile(
				fileHandle,
				outputBuffer,
				bufSize,
				&bytesWritten,
				NULL);
			delete[] outputBuffer;
		}
	}
	CloseHandle(fileHandle);
}

IM3File::IM3File(HANDLE fileHandle)
{
}

IM3File::IM3File(
	UINT8 blocksWide,
	UINT8 blocksHigh,
	std::array<
	std::pair<Codec::EntropiedDC, Codec::EntropiedAC>, 3
	> entropyCoded)
{
	FileHeaderWithTables.FileHeader.BlocksWide = blocksWide;
	FileHeaderWithTables.FileHeader.BlocksHigh = blocksHigh;
	for (UINT8 i = 0; i < 3; i++) {
		Codec::EntropiedDC entropiedDC = entropyCoded[i].first;
		Codec::EntropiedACFirst entropiedACFirst = entropyCoded[i].second.first;
		Codec::EntropiedACSecond entropiedACSecond = entropyCoded[i].second.second;

		Codec::LengthTable<INT8> dcDifferences = entropiedDC.first;
		Codec::LengthTable<UINT8> acZeroes = entropiedACFirst.first;
		Codec::LengthTable<INT8> acValues = entropiedACSecond.first;

		PlaneHeader* dest = NULL;
		Plane* plane = NULL;
		switch (i)
		{
		case 0:
			dest = &(FileHeaderWithTables.YPlaneHeader);
			plane = &(Planes.Y);
			break;
		case 1:
			dest = &(FileHeaderWithTables.UPlaneHeader);
			plane = &(Planes.U);
			break;
		case 2:
			dest = &(FileHeaderWithTables.VPlaneHeader);
			plane = &(Planes.V);
			break;
		}

		PlaneHeader temp;
		for (UINT16 i = 0; i < 256; i++) {
			temp.DCCounts[i] = dcDifferences[i];
			temp.ACZeroesCounts[i] = acZeroes[i];
			temp.ACValuesCounts[i] = acValues[i];
		}
		std::memcpy(dest, &temp, sizeof(PlaneHeader));

		plane->DC = entropiedDC.second;
		plane->AC0 = entropiedACFirst.second;
		plane->AC1 = entropiedACSecond.second;
	}
}

IM3File::~IM3File()
{
	// TODO: Implement
}
