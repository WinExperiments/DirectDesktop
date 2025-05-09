#pragma once
#include "framework.h"
#define DT_LVICON (DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_NOPREFIX | DT_EDITCONTROL)

typedef void (*BitmapPixelHandler)(int& r, int& g, int& b, int& a);

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type, unsigned int blurradius = 4, float alpha = 0.33);
HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int padding);
HBITMAP CreateTextBitmap(LPCWSTR text, int width, int height, DWORD ellipsisType, bool touch);
HBITMAP LoadPNGAsBitmap(int imageID);
void BlurBackground(HWND hwnd, bool blur, bool fullscreen);

extern TEXTMETRICW textm;
extern int dpi;