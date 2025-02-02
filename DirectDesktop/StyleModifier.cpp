#include "StyleModifier.h"
#include "BitmapHelper.h"
#include "ColorHelper.h"
#include "AccentColorHelper.h"
#include "ImmersiveColor.h"
#include "DirectoryHelper.h"

COLORREF ImmersiveColor;
bool theme;
const wchar_t* sheetName;

HBITMAP IconToBitmap(HICON hIcon, int x, int y) {
    HDC hDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, x, y);
    HBITMAP hResultBmp = NULL;
    HGDIOBJ hOrgBMP = SelectObject(hMemDC, hMemBmp);

    DrawIconEx(hMemDC, 0, 0, hIcon, x, y, 0, NULL, DI_NORMAL);

    hResultBmp = hMemBmp;
    hMemBmp = NULL;

    SelectObject(hMemDC, hOrgBMP);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);
    DestroyIcon(hIcon);
    return hResultBmp;
}

void UpdateModeInfo() {
    theme = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme");
    ImmersiveColor = CImmersiveColor::GetColor(IMCLR_SystemAccent);
    sheetName = theme ? L"default" : L"defaultdark";
}

rgb_t ImmersiveToRGB(COLORREF immersivecolor) {
    rgb_t result;
    result.r = (int)(immersivecolor % 16777216);
    result.g = (int)((immersivecolor / 256) % 65536);
    result.b = (int)((immersivecolor / 65536) % 256);
    return result;
}


void StandardBitmapPixelHandler(int& r, int& g, int& b, int& a)
{
    UpdateAccentColor();
    rgb_t rgbVal = { r, g, b };

    hsl_t hslVal = rgb2hsl(rgbVal);

    double g_hslEnhancedAccentHL = 107 - g_hslAccent.l;

    hslVal.l += (g_hslAccent.l * hslVal.s);

    if (hslVal.l > g_hslEnhancedAccentHL) {
        hslVal.h = g_hslAccent.h + 0.5 * ((g_hslLightAccentH - g_hslAccent.h) * ((hslVal.l - g_hslEnhancedAccentHL) / (255.0 - g_hslEnhancedAccentHL)));
    }
    else if (hslVal.l <= g_hslEnhancedAccentHL) {
        hslVal.h = g_hslAccent.h + 0.5 * ((g_hslDarkAccentH - g_hslAccent.h) * ((g_hslEnhancedAccentHL - hslVal.l) / g_hslEnhancedAccentHL));
    }

    hslVal.s = (double)hslVal.s * (double)g_hslAccent.s;
    
    rgbVal = hsl2rgb(hslVal);
    
    r = rgbVal.r;
    g = rgbVal.g;
    b = rgbVal.b;
}

void SimpleBitmapPixelHandler(int& r, int& g, int& b, int& a)
{
    r = 0;
    g = 0;
    b = 0;
}

void UndoPremultiplication(int& r, int& g, int& b, int& a)
{
    rgb_t rgbVal = { r, g, b };
    r = rgbVal.r / (a / 255.0);
    g = rgbVal.g / (a / 255.0);
    b = rgbVal.b / (a / 255.0);
}

void DesaturateWhiten(int& r, int& g, int& b, int& a)
{
    rgb_t rgbVal = { r, g, b };

    hsl_t hslVal = rgb2hsl(rgbVal);

    a = hslVal.l;
    r = 255.0;
    g = 255.0;
    b = 255.0;
}