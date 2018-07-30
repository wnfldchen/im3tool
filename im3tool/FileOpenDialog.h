#pragma once
#include "BitmapFile.h"

// FileOpenDialog class declaration
class FileOpenDialog {
private:
  static const int szFileN = 260; // Recommended max filename size
  OPENFILENAME ofn; // Structure for the open file dialog
  WCHAR szFile[szFileN]; // Buffer for filename
  // Show the open file dialog
  HANDLE Show();
  // Configure the open file dialog
  void InitOpenDialog(HWND hWnd);
  // Constructor to create an open file dialog
  FileOpenDialog(HWND hWnd, HANDLE* fileHandle);
public:
  // Open a bitmap file using the open file dialog
  static BitmapFile* OpenBitmapFile(HWND hWnd);
};

