#include "StyleModifier.h"
#include "BitmapHelper.h"
#include "ColorHelper.h"
#include "AccentColorHelper.h"
#include "ImmersiveColor.h"

COLORREF ImmersiveColor;
int btnforeground;
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
    //int i = 0;
    //LPCWSTR path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    //HKEY hKey;
    //DWORD lResult = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    //if (lResult == ERROR_SUCCESS)
    //{
    //    DWORD dwSize = NULL;
    //    lResult = RegGetValue(hKey, NULL, L"AppsUseLightTheme", RRF_RT_DWORD, NULL, NULL, &dwSize);
    //    if (lResult == ERROR_SUCCESS && dwSize != NULL)
    //    {
    //        DWORD* dwValue = (DWORD*)malloc(dwSize);
    //        lResult = RegGetValue(hKey, NULL, L"AppsUseLightTheme", RRF_RT_DWORD, NULL, dwValue, &dwSize);
    //        i = *dwValue;
    //        free(dwValue);
    //    }
    //    RegCloseKey(hKey);
    //}
    //if (i == 1) theme = 1;
    //else theme = 0;
    ImmersiveColor = CImmersiveColor::GetColor(IMCLR_SystemAccent);
    //btnforeground = theme ? 16777215 : 0;
    //sheetName = theme ? L"w" : L"wd";
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
    rgb_t rgbVal = {r, g, b};
    rgbVal.r = rgbVal.r / (a / 255.0);
    rgbVal.g = rgbVal.g / (a / 255.0);
    rgbVal.b = rgbVal.b / (a / 255.0);

    hsl_t hslVal = rgb2hsl(rgbVal);

    double g_hslEnhancedAccentHL = 107 - g_hslAccent.l;

    hslVal.l +=  (g_hslAccent.l * hslVal.s);

    if (hslVal.l > g_hslEnhancedAccentHL) {
        hslVal.h = g_hslAccent.h + 0.5 * ((g_hslLightAccentH - g_hslAccent.h) * ((hslVal.l - g_hslEnhancedAccentHL) / (255.0 - g_hslEnhancedAccentHL)));
    }
    else if (hslVal.l <= g_hslEnhancedAccentHL) {
        hslVal.h = g_hslAccent.h + 0.5 * ((g_hslDarkAccentH - g_hslAccent.h) * ((g_hslEnhancedAccentHL - hslVal.l) / g_hslEnhancedAccentHL));
    }

    hslVal.s = (double)hslVal.s * (double)g_hslAccent.s;

    rgbVal = hsl2rgb(hslVal);

    rgbVal.r = rgbVal.r * (a / 255.0);
    rgbVal.g = rgbVal.g * (a / 255.0);
    rgbVal.b = rgbVal.b * (a / 255.0);

    r = rgbVal.r;
    g = rgbVal.g;
    b = rgbVal.b;
}