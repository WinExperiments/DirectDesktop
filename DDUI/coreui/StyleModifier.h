#pragma once

#ifdef DDUI_EXPORTS
#define DDUIAPI __declspec(dllexport)
#else
#define DDUIAPI __declspec(dllimport)
#endif

namespace DDUI
{
    struct DDUIColors
    {
        COLORREF ImmersiveColor;
        COLORREF ImmersiveColorL;
        COLORREF ImmersiveColorD;
        COLORREF crPalette[8];
    };

    DDUIAPI void UpdateModeInfo(bool fInit);
    DDUIAPI bool IconToBitmap(HICON hIcon, HBITMAP& hBitmap, int x, int y);

    DDUIAPI void StandardBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void EnhancedBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void SimpleBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void UndoPremultiplication(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void DesaturateWhiten(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void DesaturateWhitenGlass(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void ColorToAlpha(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void InvertConstHue(int& r, int& g, int& b, int& a, COLORREF& crOpt);
    DDUIAPI void IncreaseBrightness(COLORREF& cr);
    DDUIAPI COLORREF GetColorFromPixel(HDC hdc, POINT pt);
    DDUIAPI COLORREF GetDominantColorFromIcon(HBITMAP hbm, int iconsize, int nonGreyishThreshold, DWORD dwBits);
    DDUIAPI COLORREF GetMostFrequentLightnessFromIcon(HBITMAP hbm, int iconsize);
    DDUIAPI COLORREF GetLightestPixel(HBITMAP hbm);
    DDUIAPI COLORREF CreateGlowColor(COLORREF cr);

    DDUIAPI COLORREF GetDUIImmersiveColor(int iDuiColor);
}
