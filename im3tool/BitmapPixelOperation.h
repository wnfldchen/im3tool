#pragma once

// Base class BitmapPixelOperation declarations

class BitmapPixelOperation {
protected:
  // Utility types for color spaces
  // Normalized RGB: [0,1]
  struct NormalizedRGB {
    DOUBLE R;
    DOUBLE G;
    DOUBLE B;
  };
  // Hue Saturation Value: H:[0,360] S:[0,1] V:[0,1]
  struct HSV {
    DOUBLE H;
    DOUBLE S;
    DOUBLE V;
  };
  // YUV: [0,1]
  struct YUV {
    DOUBLE Y;
    DOUBLE U;
    DOUBLE V;
  };

  // Utility functions
  // Clamp a value to a range
  template <typename T>
  T ClampToRange(T n, T min, T max) {
    return std::max(min, std::min(n, max));
  }
  // Floating point equality using an epsilon
  BOOL FloatingPointEquals(DOUBLE x, DOUBLE y);

  // Color space transformation functions

  // Normalized RGB <---> Pixel
  NormalizedRGB PixelToNormalizedRGB(BitmapFile::Pixel pixel);
  BitmapFile::Pixel NormalizedRGBtoPixel(NormalizedRGB rgb);

  // Normalized RGB <---> YUV
  NormalizedRGB YUVtoNormalizedRGB(YUV yuv);
  YUV NormalizedRGBtoYUV(NormalizedRGB rgb);

  // Normalized RGB <---> HSV
  NormalizedRGB HSVtoNormalizedRGB(HSV hsv);
  HSV NormalizedRGBtoHSV(NormalizedRGB rgb);
public:
  // OnPixel is overrided by child classes to implement a new pixel operation
  // as it is what is called by BitmapFile's doPixelOperation function
  virtual BitmapFile::Pixel OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y);
};

// Derived class Brighten declarations

class Brighten : public BitmapPixelOperation {
private:
  DOUBLE Factor; // Factor to brighten by
public:
  Brighten(DOUBLE factor); // Constructor to initialize brighten factor
  // Brighten the pixel
  virtual BitmapFile::Pixel OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y);
};

// Derived class Grayscale declarations

class Grayscale : public BitmapPixelOperation {
public:
  // Grayscale the pixel
  virtual BitmapFile::Pixel OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y);
};

// Derived class OrderedDither declarations

class OrderedDither : public BitmapPixelOperation {
private:
  // Dither matrix to use
  static const UINT8 M_SIZE = 4;
  const BYTE DITHER_MATRIX[M_SIZE][M_SIZE] = {
    {	0,	8,	2,	10	},
    {	12,	4,	14,	6	},
    {	3,	11,	1,	9	},
    {	15,	7,	13,	5	}
  };
public:
  // Set the pixel to its dithered value
  virtual BitmapFile::Pixel OnPixel(BitmapFile::Pixel pixel, INT32 x, INT32 y);
};