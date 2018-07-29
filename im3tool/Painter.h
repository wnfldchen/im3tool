#pragma once
// Painter class declaration
class Painter {
private:
  HWND HWnd; // The window to paint
  BitmapFile* P; // The bitmap to draw
  void ResizeWindowToImage(); // Resize the window to fit image
  void Draw(); // Draw the bitmap
public:
  // Create a new painter to paint bitmap on window
  Painter(HWND hWnd, BitmapFile* p);
};

