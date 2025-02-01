#pragma once
#include "framework.h"

typedef void (*BitmapPixelHandler)(int& r, int& g, int& b, int& a);

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type, int blurradius = 4, float alpha = 0.33);
HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int padding);
HBITMAP CreateTextBitmap(LPCWSTR text, int width, int height, DWORD ellipsisType);