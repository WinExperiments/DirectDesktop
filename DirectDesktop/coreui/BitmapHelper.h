#pragma once
#include "..\framework.h"
#include "..\Include\dui70\DirectUI\DirectUI.h"

#define DT_LVICON (DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_NOPREFIX | DT_EDITCONTROL)

namespace DirectDesktop
{
    typedef void (*BitmapPixelHandler)(int& r, int& g, int& b, int& a, COLORREF& crOpt);

    bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type, unsigned int blurradius, float alpha, COLORREF crOpt);
    bool AddPaddingToBitmap(HBITMAP hOriginalBitmap, HBITMAP& hNewBitmap, int pL, int pT, int pR, int pB);
    bool CaptureWallpaperFromProgman(HBITMAP& hBitmap, RECT rc);
    bool CreateTextBitmap(HBITMAP& hBitmap, LPCWSTR text, int width, int height, DWORD ellipsisType, bool touch);
    bool LoadPNGAsBitmap(HBITMAP& hBitmap, int imageID);
    bool CompositeBitmaps(HBITMAP hbmBg, HBITMAP hbmFg, bool hardLight, float hlCoef);
    void BlurBackground(HWND hwnd, bool blur, bool fullscreen, DirectUI::Element* peOptional);
    void BlurBackground2(HWND hwnd, bool blur, bool fullscreen, DirectUI::Element* peOptional);

    extern TEXTMETRICW textm;
    extern int g_dpi;
}
