#include "StyleModifier.h"
#include "BitmapHelper.h"
#include "ColorHelper.h"
#include "AccentColorHelper.h"
#include "ImmersiveColor.h"
#include "DirectoryHelper.h"
#include "cdpa.h"
#include <uxtheme.h>

COLORREF ImmersiveColor;
bool theme;
const wchar_t* sheetName;
rgb_t WhiteText;

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
    WhiteText.r = GetRValue(CImmersiveColor::GetColor(IMCLR_StartDesktopTilesText));
    WhiteText.g = GetGValue(CImmersiveColor::GetColor(IMCLR_StartDesktopTilesText));
    WhiteText.b = GetBValue(CImmersiveColor::GetColor(IMCLR_StartDesktopTilesText));
}

void StandardBitmapPixelHandler(int& r, int& g, int& b, int& a)
{
    UpdateAccentColor(ImmersiveColor);
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

void EnhancedBitmapPixelHandler(int& r, int& g, int& b, int& a)
{
    UpdateAccentColor(ImmersiveColor);
    rgb_t rgbVal = { r, g, b };

    hsl_t hslVal = rgb2hsl(rgbVal);

    double g_hslEnhancedAccentHL = 107 - g_hslAccent.l;

    hslVal.l += (g_hslAccent.l * hslVal.s);
    if (hslVal.l < 64) hslVal.l = 64 - pow((64 - hslVal.l), 0.8);
    if (hslVal.l > 192) hslVal.l = 192 + pow((hslVal.l - 192), 0.8);

    hslVal.s = (hslVal.s * 0.8 - 0.2) * g_hslAccent.s;

    if (hslVal.l > g_hslEnhancedAccentHL) {
        hslVal.h = g_hslAccent.h + 0.5 * ((g_hslLightAccentH - g_hslAccent.h) * ((hslVal.l - g_hslEnhancedAccentHL) / (255.0 - g_hslEnhancedAccentHL)));
    }
    else if (hslVal.l <= g_hslEnhancedAccentHL) {
        hslVal.h = g_hslAccent.h + 0.5 * ((g_hslDarkAccentH - g_hslAccent.h) * ((g_hslEnhancedAccentHL - hslVal.l) / g_hslEnhancedAccentHL));
    }

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

struct BUCKET {
    CDPA<RGBQUAD, CTContainer_PolicyUnOwned<RGBQUAD>> _dpa;
    BUCKET() {
        _dpa.Create(16);
    }
};

COLORREF GetDominantColorFromIcon(HBITMAP hbm, int iconsize) {
    COLORREF outDominantColor = RGB(128, 136, 144);

    HDC hMemDC = CreateCompatibleDC(nullptr);
    HDC hMemDC2 = CreateCompatibleDC(nullptr);

    RECT rcIcon;
    rcIcon.left = 0;
    rcIcon.top = 0;
    rcIcon.right = iconsize;
    rcIcon.bottom = iconsize;

    HDC hdcPaint;
    HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hMemDC, &rcIcon, BPBF_TOPDOWNDIB, nullptr, &hdcPaint);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC2, hbm);
    if (hBufferedPaint) {
        BufferedPaintClear(hBufferedPaint, &rcIcon);
        BitBlt(hdcPaint, rcIcon.left, rcIcon.top, rcIcon.right, rcIcon.bottom, hMemDC2, 0, 0, SRCCOPY);

        RGBQUAD* pbBuffer;
        int cxRow;
        GetBufferedPaintBits(hBufferedPaint, &pbBuffer, &cxRow);

        constexpr int bucketCoef = 86;
        constexpr int nonGreyishThreshold = 48;
        constexpr int hitRatioThreshold = 7;

        constexpr int frac = 0xFF / (bucketCoef - 1);
        BUCKET rgBucket[
            (frac * frac) * (0xFF / bucketCoef)
                + (frac) * (0xFF / bucketCoef)
                + (0xFF / bucketCoef)
                + 1
        ];
        for (int row = 0; row < 32; ++row, pbBuffer += cxRow - 32) {
            for (int column = 0; column < 32; ++column, ++pbBuffer) {
                if (pbBuffer->rgbReserved) {
                    pbBuffer->rgbRed = 0xFF * pbBuffer->rgbRed / pbBuffer->rgbReserved;
                    pbBuffer->rgbGreen = 0xFF * pbBuffer->rgbGreen / pbBuffer->rgbReserved;
                    pbBuffer->rgbBlue = 0xFF * pbBuffer->rgbBlue / pbBuffer->rgbReserved;

                    BYTE maxValue = max(pbBuffer->rgbRed, max(pbBuffer->rgbGreen, pbBuffer->rgbBlue));
                    BYTE minValue = min(pbBuffer->rgbRed, min(pbBuffer->rgbGreen, pbBuffer->rgbBlue));
                    if (maxValue - minValue > nonGreyishThreshold) {
                        BUCKET* bucket = &rgBucket[
                            (frac * frac) * (pbBuffer->rgbRed / bucketCoef)
                                + (frac) * (pbBuffer->rgbGreen / bucketCoef)
                                + (pbBuffer->rgbBlue / bucketCoef)
                        ];
                        bucket->_dpa.AppendPtr(pbBuffer);
                    }
                }
            }
        }

        SIZE_T bucketWithMostHits = -1;
        int totalHits = 0;
        for (SIZE_T bucketIdx = 0; bucketIdx < ARRAYSIZE(rgBucket); ++bucketIdx) {
            int myHits = rgBucket[bucketIdx]._dpa.GetPtrCount();
            totalHits += myHits;
            if (bucketWithMostHits == -1 || myHits > rgBucket[bucketWithMostHits]._dpa.GetPtrCount()) {
                bucketWithMostHits = bucketIdx;
            }
        }

        const BUCKET* bestBucket = &rgBucket[bucketWithMostHits];
        int denominator = 0;
        int totalR = 0;
        int totalG = 0;
        int totalB = 0;
        int numBestBucketHits = bestBucket->_dpa.GetPtrCount();
        if (numBestBucketHits > 0) {
            denominator = numBestBucketHits;
            for (int colorIdx = 0; colorIdx < numBestBucketHits; ++colorIdx) {
                const RGBQUAD* color = bestBucket->_dpa.FastGetPtr(colorIdx);
                totalR += color->rgbRed;
                totalG += color->rgbGreen;
                totalB += color->rgbBlue;
            }
        }

        if (MulDiv(numBestBucketHits, 100, totalHits) >= hitRatioThreshold) {
            outDominantColor = RGB(totalR / denominator, totalG / denominator, totalB / denominator);
        }
        if (GetRValue(outDominantColor) * 0.299 + GetGValue(outDominantColor) * 0.587 + GetBValue(outDominantColor) * 0.114 > 156) {
            rgb_t odcTemp1 = { GetRValue(outDominantColor), GetGValue(outDominantColor), GetBValue(outDominantColor) };
            hsl_t odcTemp2 = rgb2hsl(odcTemp1);
            double reduced = pow((odcTemp2.l), 0.972);
            odcTemp2.s *= (1 + (odcTemp2.l - reduced) / 96);
            odcTemp2.l = reduced;
            odcTemp1 = hsl2rgb(odcTemp2);
            outDominantColor = RGB(odcTemp1.r, odcTemp1.g, odcTemp1.b);
        }

        EndBufferedPaint(hBufferedPaint, FALSE);
        DeleteObject(hOldBitmap);
        DeleteDC(hMemDC);
        DeleteDC(hMemDC2);
    }

    return outDominantColor;
}