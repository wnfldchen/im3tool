#include "stdafx.h"
#include <algorithm>
#include "BitmapFile.h"
#include "BitmapPixelOperation.h"


// Base class BitmapPixelOperation definitions

BitmapFile::Pixel BitmapPixelOperation::OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y) {
  // Default no-op on the pixel
  // Unreferenced parameter macro silences compiler warnings
  UNREFERENCED_PARAMETER(x);
  UNREFERENCED_PARAMETER(y);
  return pixel;
}

// Derived class Brighten definitions

Brighten::Brighten(DOUBLE factor) : Factor(factor) {}

BitmapFile::Pixel Brighten::OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y) {
  // Unreferenced parameter macro silences compiler warnings
  UNREFERENCED_PARAMETER(x);
  UNREFERENCED_PARAMETER(y);
  // Go from RGB to HSV then HSV V-channel * factor then back to RGB
  NormalizedRGB rgb = PixelToNormalizedRGB(pixel);
  HSV hsv = NormalizedRGBtoHSV(rgb);
  // Brighten the pixel by a factor
  hsv.V *= Factor; // Multiply the HSV Value (Brightness)
  hsv.V = ClampToRange(hsv.V, 0.0, 1.0); // Restore it to valid range [0,1]
  rgb = HSVtoNormalizedRGB(hsv);
  return NormalizedRGBtoPixel(rgb);
}

// Derived class Grayscale definitions

BitmapFile::Pixel Grayscale::OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y) {
  // Unreferenced parameter macro silences compiler warnings
  UNREFERENCED_PARAMETER(x);
  UNREFERENCED_PARAMETER(y);
  // Change pixel to grayscale
  NormalizedRGB rgb = PixelToNormalizedRGB(pixel);
  // Go from RGB to YUV then YUV UV-channels to zero then back to RGB
  YUV yuv = NormalizedRGBtoYUV(rgb);
  yuv.U = 0.0; // Set YUV chrominances to zero to keep only the luma
  yuv.V = 0.0;
  rgb = YUVtoNormalizedRGB(yuv);
  return NormalizedRGBtoPixel(rgb);
}

// Derived class OrderedDither definitions

BitmapFile::Pixel OrderedDither::OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y) {
  // Since it is grayscale (R == G == B) use R channel for the value
  INT32 input = pixel.Red;
  // i,j are indices into the dither matrix
  INT32 i = x % M_SIZE;
  INT32 j = y % M_SIZE;
  // Normalize input to [0,M_SIZE*M_SIZE]
  input = static_cast<INT32>(input * (M_SIZE*M_SIZE + 1) / 256.0);
  // If the value is darker than the matrix print a dot
  if (input < DITHER_MATRIX[j][i]) {
    return{ 0, 0, 0 };
  }
  // Else dont print a dot
  return{ 255, 255, 255 };
}

