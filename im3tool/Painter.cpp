#include "stdafx.h"
#include "BitmapFile.h"
#include "Painter.h"

Painter::Painter(HWND hWnd, BitmapFile * p)
  : HWnd(hWnd), P(p) {
  // Store window and bitmap in class
  // Resize window to fit bitmap
  ResizeWindowToImage();
  // Draw the bitmap
  Draw();
}

void Painter::ResizeWindowToImage() {
  // Set the target client area size as the bitmap dimensions
  RECT sizeRect = { 0, 0, P->getWidth(), P->getHeight() };
  // Adjust the client area size to a window size
  AdjustWindowRect(&sizeRect, WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX, TRUE);
  // Get the current window position
  RECT windowRect;
  GetWindowRect(HWnd, &windowRect);
  // Resize the window to fit the image without moving it
  MoveWindow(
    HWnd,
    windowRect.left,
    windowRect.top,
    sizeRect.right - sizeRect.left,
    sizeRect.bottom - sizeRect.top,
    FALSE);
}

void Painter::Draw() {
  // Acquire the graphics resources to paint the window
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(HWnd, &ps);

  // Get the dimensions to paint
  INT32 width = P->getWidth();
  INT32 height = P->getHeight();

  // Acquire a backbuffer to draw on
  HDC backBufferHdc = CreateCompatibleDC(hdc);
  HBITMAP backBufferBmp = CreateCompatibleBitmap(hdc, width, height);
  SelectObject(backBufferHdc, backBufferBmp);

  // For each pixel
  for (INT32 y = 0; y < height; y++) {
    for (INT32 x = 0; x < width; x++) {
      BitmapFile::Pixel pixel = P->getPixel(x, y);
      // Draw the pixel onto the backbuffer
      SetPixel(
        backBufferHdc,
        x,
        y,
        RGB(pixel.Red, pixel.Green, pixel.Blue));
    }
  }

  // Blit the backbuffer all at once to the screen
  BitBlt(hdc, 0, 0, width, height, backBufferHdc, 0, 0, SRCCOPY);

  // Release the acquired backbuffer
  DeleteObject(backBufferBmp);
  DeleteDC(backBufferHdc);

  // Release the graphics resources
  EndPaint(HWnd, &ps);
}
