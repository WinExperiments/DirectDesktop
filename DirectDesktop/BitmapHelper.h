#pragma once
#include "framework.h"
#define DT_LVICON (DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_NOPREFIX | DT_EDITCONTROL)

typedef void (*BitmapPixelHandler)(int& r, int& g, int& b, int& a, COLORREF& crOpt);

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type, unsigned int blurradius, float alpha, COLORREF crOpt);
HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int pL, int pT, int pR, int pB);
HBITMAP CaptureWallpaperFromProgman(RECT rc);
HBITMAP CreateTextBitmap(LPCWSTR text, int width, int height, DWORD ellipsisType, bool touch);
HBITMAP LoadPNGAsBitmap(int imageID);
bool CompositeBitmaps(HBITMAP hbmBg, HBITMAP hbmFg, bool hardLight, float hlCoef);
void BlurBackground(HWND hwnd, bool blur, bool fullscreen);
void BlurBackground2(HWND hwnd, bool blur, bool fullscreen);

extern TEXTMETRICW textm;
extern int dpi;