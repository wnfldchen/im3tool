#include "stdafx.h"
#include "BitmapFile.h"
#include "FileOpenDialog.h"

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
  ofn.lpstrFilter = L"BMP files\0*.BMP\0\0";
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  // File must exist
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

BitmapFile * FileOpenDialog::OpenBitmapFile(HWND hWnd) {
  BitmapFile* bitmapFile = NULL;
  BitmapFile::CreateResult result;
  HANDLE fileHandle;
  // Show the file open dialog
  FileOpenDialog fileOpenDialog(hWnd, &fileHandle);
  // If a file was opened
  if (fileHandle) {
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
  return bitmapFile;
}

FileOpenDialog::FileOpenDialog(HWND hWnd, HANDLE* fileHandle) {
  // Configure the file open dialog
  InitOpenDialog(hWnd);
  // Show the dialog and set the result
  *fileHandle = Show();
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
