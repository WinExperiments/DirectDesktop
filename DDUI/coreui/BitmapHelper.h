#pragma once

#ifdef DDUI_EXPORTS
#define DDUIAPI __declspec(dllexport)
#else
#define DDUIAPI __declspec(dllimport)
#endif

#define DT_LVICON (DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_NOPREFIX | DT_EDITCONTROL)

namespace DDUI
{
    typedef void (*BitmapPixelHandler)(int& r, int& g, int& b, int& a, COLORREF& crOpt);

    DDUIAPI bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type, unsigned int blurradius, float alpha, COLORREF crOpt);
    DDUIAPI bool AddPaddingToBitmap(HBITMAP hOriginalBitmap, HBITMAP& hNewBitmap, int pL, int pT, int pR, int pB);
    DDUIAPI bool CaptureWallpaperFromProgman(HBITMAP& hBitmap, RECT rc);
    DDUIAPI bool CreateTextBitmap(HBITMAP& hBitmap, LPCWSTR text, int width, int height, DWORD ellipsisType, bool touch, DWORD dwFontStyle);
    DDUIAPI bool LoadPNGAsBitmap(HMODULE hModule, HBITMAP& hBitmap, int imageID);
    DDUIAPI bool CompositeBitmaps(HBITMAP hbmBg, HBITMAP hbmFg, bool hardLight, float hlCoef);
    DDUIAPI void BlurBackground(HWND hwnd, bool blur, bool fullscreen, BYTE alpha, DirectUI::Element* peOptional);
    DDUIAPI void BlurBackground2(HWND hwnd, bool blur, bool fullscreen, DirectUI::Element* peOptional);
}
