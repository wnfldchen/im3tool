#include "stdafx.h"
#include <cstring>
#include "commontypes.h"
#include "IM3File.h"

void IM3File::Save(HANDLE fileHandle)
{
	static const DWORD fileHeaderWithTablesSize = sizeof(fileHeaderWithTables);
	DWORD bytesWritten;
	WriteFile(
		fileHandle,
		&fileHeaderWithTables,
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
			outputBuffer[bufSize - 1] = b;
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

FileHeaderWithTables IM3File::getFileHeaderWithTables()
{
	return fileHeaderWithTables;
}

std::vector<bool> IM3File::getBitsReadFromFile()
{
	return bitsReadFromFile;
}

IM3File::IM3File(HANDLE fileHandle)
{
	static const UINT64 fileHeaderWithTablesSize = sizeof(fileHeaderWithTables);
	LARGE_INTEGER fileSizeStruct;
	GetFileSizeEx(fileHandle, &fileSizeStruct);
	UINT64 fileSize = fileSizeStruct.QuadPart;
	BYTE* readBytes = new BYTE[fileSize];
	DWORD bytesRead;
	ReadFile(
		fileHandle,
		readBytes,
		static_cast<DWORD>(fileSize),
		&bytesRead,
		NULL);
	CloseHandle(fileHandle);
	std::memcpy(
		&fileHeaderWithTables,
		readBytes,
		fileHeaderWithTablesSize);
	for (UINT64 i = fileHeaderWithTablesSize; i < fileSize; i++) {
		BYTE readByte = readBytes[i];
		BYTE mask = 0x01;
		for (UINT8 bitIndex = 0; bitIndex < 8; bitIndex++) {
			bitsReadFromFile.push_back(!!(readByte & mask));
			mask <<= 1;
		}
	}
	delete[] readBytes;
}

IM3File::IM3File(
	UINT8 blocksWide,
	UINT8 blocksHigh,
	std::array<
	std::pair<EntropiedDC, EntropiedAC>, 3
	> entropyCoded)
{
	fileHeaderWithTables.FileHeader.BlocksWide = blocksWide;
	fileHeaderWithTables.FileHeader.BlocksHigh = blocksHigh;
	for (UINT8 i = 0; i < 3; i++) {
		EntropiedDC entropiedDC = entropyCoded[i].first;
		EntropiedACFirst entropiedACFirst = entropyCoded[i].second.first;
		EntropiedACSecond entropiedACSecond = entropyCoded[i].second.second;

		LengthTable<INT8> dcDifferences = entropiedDC.first;
		LengthTable<UINT8> acZeroes = entropiedACFirst.first;
		LengthTable<INT8> acValues = entropiedACSecond.first;

		PlaneHeader* dest = NULL;
		Plane* plane = NULL;
		UINT16* acZeroesBytes = NULL;
		UINT16* acValuesBytes = NULL;
		switch (i)
		{
		case 0:
			dest = &(fileHeaderWithTables.YPlaneHeader);
			plane = &(Planes.Y);
			acZeroesBytes = &(fileHeaderWithTables.FileHeader.YACZeroesBytes);
			acValuesBytes = &(fileHeaderWithTables.FileHeader.YACValuesBytes);
			break;
		case 1:
			dest = &(fileHeaderWithTables.UPlaneHeader);
			plane = &(Planes.U);
			acZeroesBytes = &(fileHeaderWithTables.FileHeader.UACZeroesBytes);
			acValuesBytes = &(fileHeaderWithTables.FileHeader.UACValuesBytes);
			break;
		case 2:
			dest = &(fileHeaderWithTables.VPlaneHeader);
			plane = &(Planes.V);
			acZeroesBytes = &(fileHeaderWithTables.FileHeader.VACZeroesBytes);
			acValuesBytes = &(fileHeaderWithTables.FileHeader.VACValuesBytes);
			break;
		}
		*acZeroesBytes =
			static_cast<UINT16>(ceil(entropiedACFirst.second.size() / 8.0));
		*acValuesBytes =
			static_cast<UINT16>(ceil(entropiedACSecond.second.size() / 8.0));

		PlaneHeader temp;
		for (UINT16 i = 0; i < 256; i++) {
			temp.DCLengths[i] = dcDifferences[i];
			temp.ACZeroesLengths[i] = acZeroes[i];
			temp.ACValuesLengths[i] = acValues[i];
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
