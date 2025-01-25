#pragma once
#include "framework.h"

typedef void (*BitmapPixelHandler)(int& r, int& g, int& b, int& a);

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type);
HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int padding);