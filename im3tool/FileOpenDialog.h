#pragma once
#include <string>
#include "BitmapFile.h"

// FileOpenDialog class declaration
class FileOpenDialog {
private:
  static const int szFileN = 260; // Recommended max filename size
  static WCHAR szFile[szFileN]; // Buffer for filename
  OPENFILENAME ofn; // Structure for the open file dialog
  // Show the open file dialog
  HANDLE Show();
  // Configure the open file dialog
  void InitOpenDialog(HWND hWnd);
  // Constructor to create an open file dialog
  FileOpenDialog(HWND hWnd, HANDLE* fileHandle);
public:
  // Get the opened file name
  static std::wstring getFileName();
  // Open a bitmap file using the open file dialog
  static BitmapFile* OpenBitmapFile(HWND hWnd);
  // Open a file from a file name
  static HANDLE OverwriteFileFromName(std::wstring fileNameToOpen);
};

