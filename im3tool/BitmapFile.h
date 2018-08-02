#pragma once
// Forward declarations for class dependencies
class BitmapPixelOperation;
// BitmapFile class declaration
class BitmapFile {
public:
  // Structure for RGB pixel data
  struct Pixel {
    BYTE Blue;
    BYTE Green;
    BYTE Red;
  };
  // Result codes for reading from file
  enum CreateResult {
    OK,
    ERROR_NOT_BMP,
    ERROR_NOT_UNCOMPRESSED,
    ERROR_NOT_24BIT,
    ERROR_READ_FAILED
  };
private:
  // Structure for storing data from file
  struct File {
    // Structure for bitmap file header data
#pragma pack(push, 1) // Pack struct members together for contiguous file reading
    struct Header {
      // BITMAPFILEHEADER (see wingdi.h)
      UINT16	Type; // == "BM" (magic/identifying bytes)
      UINT32	fSize;
      UINT16	Reserved1; // == 0 (bitmap spec)
      UINT16	Reserved2; // == 0 (bitmap spec)
      UINT32	Offset; // Offset into file for pixel data start
      // BITMAPINFOHEADER (see wingdi.h)
      UINT32	iSize;
      INT32	Width;
      INT32	Height;
      UINT16	Planes; // == 1 (for supported format)
      UINT16	BitCount; // == 24 (for supported format)
      UINT32	Compression; // == 0 (for supported format)
      UINT32	SizeImage;
      INT32	XPixelsPerMeter;
      INT32	YPixelsPerMeter;
      UINT32	ColorsUsed; // == 0 (for supported format)
      UINT32	ColorsImportant; // == 0 (for supported format)
    } Header;
#pragma pack(pop)
    Pixel* Pixels; // Pointer to allocated memory for pixel data
    File(); // Construct file data to null pixels pointer
    ~File(); // Destruct by deallocating pixels memory
  } File;
  // Utility functions used by other class functions
  INT32 absHeight(); // Image height
  INT32 pixelLineBytes(); // Bytes per pixel line
  INT32 scanLineBytes(); // Bytes per scan line
  CreateResult TestFile(); // Run tests to check file validity
  CreateResult ReadBitmapFile(HANDLE fileHandle); // Read a file
public:
  // Public functions used by other classes and window code
  BitmapFile(HANDLE fileHandle, CreateResult* result); // Constructor from file
  BitmapFile(INT32 width, INT32 height);
  BitmapFile(const BitmapFile& bitmapFile); // Deep copy constructor from other instance
  Pixel getPixel(UINT32 x, UINT32 y); // Get a pixel from the location
  INT32 getWidth(); // Get image width in pixels
  INT32 getHeight(); // Get image height in pixels
  void setPixel(UINT32 x, UINT32 y, Pixel pixel); // Set a pixel at location
  void doPixelOperation(BitmapPixelOperation & operation); // Execute a per-pixel operation
};