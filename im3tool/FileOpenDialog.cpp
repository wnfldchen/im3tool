#include "stdafx.h"
#include <string>
#include "BitmapFile.h"
#include "FileOpenDialog.h"
#include "Codec.h"

// Forward declaration of class dependencies

WCHAR FileOpenDialog::szFile[];

void FileOpenDialog::InitOpenDialog(HWND hWnd) {
  // Configure the file opening dialog
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  // Parent window is the current window
  ofn.hwndOwner = hWnd;
  // Buffer for storing the filename
  ofn.lpstrFile = szFile;
  // Set the buffer to an empty string
  ofn.lpstrFile[0] = L'\0';
  // Recommended max filename size is 260 characters
  ofn.nMaxFile = szFileN;
  // Filter only bitmap files
  ofn.lpstrFilter = L"BMP files\0*.BMP\0IM3 files\0*.IM3\0\0";
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  // File must exist
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

BitmapFile * FileOpenDialog::OpenBitmapFile(HWND hWnd) {
  static Codec codec;
  BitmapFile* bitmapFile = NULL;
  BitmapFile::CreateResult result;
  HANDLE fileHandle;
  // Show the file open dialog
  FileOpenDialog fileOpenDialog(hWnd, &fileHandle);
  // If a file was opened
  if (fileHandle) {
	// Check if it was an IM3 file
	std::wstring fileName = getFileName();
	// If it is an IM3 file, decompress it
	// Else it is a BMP file, read it in
	if (fileName.find(L".im3") != std::wstring::npos ||
		fileName.find(L".IM3") != std::wstring::npos) {
		IM3File im3File(fileHandle);
		bitmapFile = codec.decompress(&im3File);
	}
	else {
		// Read the file into memory
		bitmapFile = new BitmapFile(fileHandle, &result);
		// If there was an error while reading the file show a message
		switch (result) {
		case BitmapFile::ERROR_NOT_BMP:
			MessageBox(hWnd, L"Not a valid BMP file", NULL, MB_OK);
			break;
		case BitmapFile::ERROR_NOT_UNCOMPRESSED:
			MessageBox(hWnd, L"Not uncompressed", NULL, MB_OK);
			break;
		case BitmapFile::ERROR_NOT_24BIT:
			MessageBox(hWnd, L"Not a 24-bit image", NULL, MB_OK);
			break;
		case BitmapFile::ERROR_READ_FAILED:
			MessageBox(hWnd, L"Read failed", NULL, MB_OK);
			break;
		}
		// If there was an error while reading the file
		switch (result) {
		case BitmapFile::ERROR_NOT_BMP:
		case BitmapFile::ERROR_NOT_UNCOMPRESSED:
		case BitmapFile::ERROR_NOT_24BIT:
		case BitmapFile::ERROR_READ_FAILED:
			// Deallocate the memory and set the pointer to null
			delete bitmapFile;
			bitmapFile = NULL;
			break;
		}
	}
  }
  return bitmapFile;
}

HANDLE FileOpenDialog::OverwriteFileFromName(std::wstring fileNameToOpen)
{
	return CreateFile(
		fileNameToOpen.c_str(),
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
}

FileOpenDialog::FileOpenDialog(HWND hWnd, HANDLE* fileHandle) {
  // Configure the file open dialog
  InitOpenDialog(hWnd);
  // Show the dialog and set the result
  *fileHandle = Show();
}

std::wstring FileOpenDialog::getFileName()
{
	return std::wstring(szFile);
}

HANDLE FileOpenDialog::Show() {
  HANDLE fileHandle = NULL;
  // If a file was selected
  if (GetOpenFileName(&ofn) == TRUE) {
    // Open the file for reading
    fileHandle = CreateFile(
      szFile,
      GENERIC_READ,
      0,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL);
  }
  return fileHandle;
}
