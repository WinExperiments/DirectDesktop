#pragma once
#include "framework.h"

typedef void (*BitmapPixelHandler)(int& r, int& g, int& b, int& a);

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, bool type);
HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int padding);