#include "stdafx.h"
#include "BitmapFile.h"

BitmapFile::CreateResult BitmapFile::TestFile() {
  // Record any difference between expected and actual values
  DWORD errorAccumulator = 0;
  // The first two bytes of the file must be "BM"
  errorAccumulator |= strncmp(
    (CHAR*)&File.Header.Type,
    "BM",
    2) != 0;
  // The two reserved fields must be 0
  errorAccumulator |= File.Header.Reserved1 != 0;
  errorAccumulator |= File.Header.Reserved2 != 0;
  // If otherwise then the file is not a bitmap
  if (errorAccumulator) {
    return ERROR_NOT_BMP;
  }
  // The bitmap must be uncompressed
  errorAccumulator |= File.Header.Compression != 0;
  if (errorAccumulator) {
    return ERROR_NOT_UNCOMPRESSED;
  }
  // The bitmap must be 24-bit
  errorAccumulator |= File.Header.Planes != 1;
  errorAccumulator |= File.Header.BitCount != 24;
  errorAccumulator |= File.Header.ColorsUsed != 0;
  errorAccumulator |= File.Header.ColorsImportant != 0;
  if (errorAccumulator) {
    return ERROR_NOT_24BIT;
  }
  return OK;
}

BitmapFile::CreateResult BitmapFile::ReadBitmapFile(HANDLE fileHandle) {
  // The basic header information is the first 54 bytes
  static const int HEADERSIZE = 54;
  // Record any differences between expected and actual bytes read
  DWORD errorAccumulator = 0;
  DWORD bytesRead = 0;
  DWORD bytesToRead = 0;
  // Read the basic header information
  bytesToRead = HEADERSIZE;
  errorAccumulator |= ReadFile(fileHandle, &File.Header, bytesToRead, &bytesRead, NULL) != TRUE;
  errorAccumulator |= bytesRead != bytesToRead;
  // Skip over any other header information
  SetFilePointer(
    fileHandle,
    File.Header.Offset - HEADERSIZE,
    NULL,
    FILE_CURRENT);
  // Allocate space to store image pixel data
  File.Pixels = new Pixel[File.Header.Width * absHeight()];
  // Read and store the image pixels one scan line at a time
  for (INT32 i = 0; i < absHeight(); i++) {
    // Get the number of bytes in the pixel line
    bytesToRead = pixelLineBytes();
    // The location in allocated memory to write the pixel line
    Pixel* bufPos = NULL;
    if (File.Header.Height >= 0) // Pixel lines ordered bottom first
    {
      bufPos = File.Pixels + (File.Header.Width * (absHeight() - i - 1));
    } else // Pixel lines ordered top first
    {
      bufPos = File.Pixels + (i * File.Header.Width);
    }
    // Read the pixel line
    errorAccumulator |= ReadFile(
      fileHandle,
      bufPos,
      bytesToRead,
      &bytesRead,
      NULL) != TRUE;
    errorAccumulator |= bytesRead != bytesToRead;
    // Skip the zero padding left in the scan line
    SetFilePointer(
      fileHandle,
      scanLineBytes() - bytesToRead,
      NULL,
      FILE_CURRENT);
  }
  // Close the file
  CloseHandle(fileHandle);
  // If any mismatch between expected and actual bytes read
  if (errorAccumulator) // There was a read error
  {
    return ERROR_READ_FAILED;
  }
  // Test header fields to determine if supported format
  return TestFile();
}

INT32 BitmapFile::absHeight() {
  // How many scan lines are present
  return abs(File.Header.Height);
}

INT32 BitmapFile::pixelLineBytes() {
  // How many bytes per scan line are pixels
  return File.Header.Width * 3;
}

INT32 BitmapFile::scanLineBytes() {
  // Each scan line is zero-padded to a multiple of 4
  INT32 bytes = pixelLineBytes();
  INT32 remainder = bytes % 4;
  if (remainder == 0) {
    return bytes;
  }
  // Result is the bytes per scan line in total
  return bytes - remainder + 4;
}

BitmapFile::BitmapFile(HANDLE fileHandle, CreateResult* result) {
  // Read a bitmap from the file
  *result = ReadBitmapFile(fileHandle);
}

BitmapFile::BitmapFile(const BitmapFile & bitmapFile) {
  File = bitmapFile.File;
  // Find how many bytes are in the image pixel data
  INT32 length = File.Header.Width * absHeight() * sizeof(Pixel);
  // Perform a deep copy of the image pixel data
  if (length != 0) {
    File.Pixels = new Pixel[length];
    memcpy(File.Pixels, bitmapFile.File.Pixels, length);
  }
}

BitmapFile::Pixel BitmapFile::getPixel(UINT32 x, UINT32 y) {
  // Get the pixel at the location
  return File.Pixels[y * File.Header.Width + x];
}

INT32 BitmapFile::getWidth() {
  // Width in pixels of the bitmap
  return File.Header.Width;
}

INT32 BitmapFile::getHeight() {
  // Height in pixels (or scan lines) of the bitmap
  return absHeight();
}

void BitmapFile::setPixel(UINT32 x, UINT32 y, Pixel pixel) {
  // Set the pixel at the location
  File.Pixels[y * File.Header.Width + x] = pixel;
}

BitmapFile::File::File() : Pixels(NULL) {
  // Initialize pointer to image data as null before loading the image
}

BitmapFile::File::~File() {
  // When the bitmap is no longer needed
  // If memory was allocated for image data
  if (Pixels) {
    // Deallocate it and set to null
    delete[] Pixels;
    Pixels = NULL;
  }
}
