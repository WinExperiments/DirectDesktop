#include "pch.h"

#include "StyleModifier.h"

#include <map>

#include "AccentColorHelper.h"
#include "cdpa.h"
#include "ImmersiveColor.h"
#include "..\backend\DirectoryHelper.h"

namespace DirectDesktop
{
    COLORREF ImmersiveColor, ImmersiveColorL, ImmersiveColorD;
    bool g_theme;
    const wchar_t* sheetName;
    rgb_t WhiteText;

    bool IconToBitmap(HICON hIcon, HBITMAP& hBitmap, int x, int y)
    {
        if (hBitmap) DeleteObject(hBitmap);
        HDC hDC = GetDC(nullptr);
        HDC hMemDC = CreateCompatibleDC(hDC);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = x;
        bmi.bmiHeader.biHeight = -y;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pvBits = nullptr;
        HBITMAP hResultBmp = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
        if (!hResultBmp)
        {
            DeleteDC(hMemDC);
            ReleaseDC(nullptr, hDC);
            return false;
        }

        HGDIOBJ hOrgBMP = SelectObject(hMemDC, hResultBmp);
        DrawIconEx(hMemDC, 0, 0, hIcon, x, y, 0, nullptr, DI_NORMAL);

        DWORD* pixels = (DWORD*)pvBits;
        for (int i = 0; i < x * y; ++i)
        {
            BYTE* px = (BYTE*)&pixels[i];
            if ((px[0] != 0 || px[1] != 0 || px[2] != 0) && px[3] == 0)
            {
                px[3] = 0xFF;
            }
        }

        SelectObject(hMemDC, hOrgBMP);
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hDC);
        DestroyIcon(hIcon);
        hBitmap = hResultBmp;
        return true;
    }

    void UpdateModeInfo()
    {
        g_theme = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme");
        ImmersiveColor = CImmersiveColor::GetColor(IMCLR_SystemAccent);
        ImmersiveColorL = CImmersiveColor::GetColor(IMCLR_SystemAccentLight2);
        ImmersiveColorD = CImmersiveColor::GetColor(IMCLR_SystemAccentDark1);
        if (iconColorID == 1) SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"IconColorizationColor", ImmersiveColor, false, nullptr);
    }

    void StandardBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        if (crOpt == NULL) crOpt = ImmersiveColor;
        UpdateAccentColor(crOpt);
        rgb_t rgbVal = { r, g, b };

        hsl_t hslVal = rgb2hsl(rgbVal);

        double g_hslEnhancedAccentHL = 107 - g_hslAccent.l;

        hslVal.l += (g_hslAccent.l * hslVal.s);

        if (hslVal.l > g_hslEnhancedAccentHL)
        {
            hslVal.h = g_hslAccent.h + 0.5 * ((g_hslLightAccentH - g_hslAccent.h) * ((hslVal.l - g_hslEnhancedAccentHL) / (255.0 - g_hslEnhancedAccentHL)));
        }
        else if (hslVal.l <= g_hslEnhancedAccentHL)
        {
            hslVal.h = g_hslAccent.h + 0.5 * ((g_hslDarkAccentH - g_hslAccent.h) * ((g_hslEnhancedAccentHL - hslVal.l) / g_hslEnhancedAccentHL));
        }

        hslVal.s = (double)hslVal.s * (double)g_hslAccent.s;

        rgbVal = hsl2rgb(hslVal);

        r = rgbVal.r;
        g = rgbVal.g;
        b = rgbVal.b;
    }

    void EnhancedBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        if (crOpt == NULL) crOpt = ImmersiveColor;
        UpdateAccentColor(crOpt);
        rgb_t rgbVal = { r, g, b };

        hsl_t hslVal = rgb2hsl(rgbVal);

        double g_hslEnhancedAccentHL = 107 - g_hslAccent.l;

        hslVal.l += (g_hslAccent.l * hslVal.s);
        if (hslVal.l < 64) hslVal.l = 64 - pow((64 - hslVal.l), 0.8);
        if (hslVal.l > 192) hslVal.l = 192 + pow((hslVal.l - 192), 0.8);

        hslVal.s = (hslVal.s * 0.8 - 0.2) * g_hslAccent.s;

        if (hslVal.l > g_hslEnhancedAccentHL)
        {
            hslVal.h = g_hslAccent.h + 0.5 * ((g_hslLightAccentH - g_hslAccent.h) * ((hslVal.l - g_hslEnhancedAccentHL) / (255.0 - g_hslEnhancedAccentHL)));
        }
        else if (hslVal.l <= g_hslEnhancedAccentHL)
        {
            hslVal.h = g_hslAccent.h + 0.5 * ((g_hslDarkAccentH - g_hslAccent.h) * ((g_hslEnhancedAccentHL - hslVal.l) / g_hslEnhancedAccentHL));
        }

        rgbVal = hsl2rgb(hslVal);

        r = rgbVal.r;
        g = rgbVal.g;
        b = rgbVal.b;
    }

    void SimpleBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        r = 0;
        g = 0;
        b = 0;
    }

    void UndoPremultiplication(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        rgb_t rgbVal = { r, g, b };
        r = rgbVal.r / (a / 255.0);
        g = rgbVal.g / (a / 255.0);
        b = rgbVal.b / (a / 255.0);
    }

    void DesaturateWhiten(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        rgb_t rgbVal = { r, g, b };
        hsl_t hslVal = rgb2hsl(rgbVal);

        a = hslVal.l;
        r = 255.0;
        g = 255.0;
        b = 255.0;
    }

    void DesaturateWhitenGlass(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        rgb_t rgbSrc = { GetRValue(crOpt), GetGValue(crOpt), GetBValue(crOpt) };
        hsl_t hslSrc = rgb2hsl(rgbSrc);
        rgb_t rgbVal = { r, g, b };
        hsl_t hslVal = rgb2hsl(rgbVal);

        hslVal.l += (255.0 - hslSrc.l);

        a = hslVal.l * (a / 255.0);
        r = 255.0;
        g = 255.0;
        b = 255.0;
    }

    void ColorToAlpha(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        // https://stackoverflow.com/a/40862635
        int r1 = GetRValue(crOpt);
        int r2 = GetGValue(crOpt);
        int r3 = GetBValue(crOpt);
        rgb_t rgbThreshold = { r1, r2, r3 };
        hsl_t hslThreshold = rgb2hsl(rgbThreshold);
        if (hslThreshold.l < 208)
        {
            a *= 0.8;
            return;
        }
        rgb_t rgbVal = { r, g, b };
        double aA{}, a1{}, a2{}, a3{}, maxA = 255.0, maxX = 255.0;

        if (r > r1) a1 = maxA * (r - r1) / (maxX - r1);
        else if (r < r1) a1 = maxA * (r1 - r) / r1;
        else a1 = 0.0;

        if (g > r2) a2 = maxA * (g - r2) / (maxX - r2);
        else if (g < r2) a2 = maxA * (r2 - g) / r2;
        else a2 = 0.0;

        if (b > r3) a3 = maxA * (b - r3) / (maxX - r3);
        else if (b < r3) a3 = maxA * (r3 - b) / r3;
        else a3 = 0.0;

        aA = a1;
        if (a2 > aA) aA = a2;
        if (a3 > aA) aA = a3;

        if (aA >= maxA / maxX)
        {
            a = aA * a / 255.0;
            rgbVal.r = maxA * (r - r1) / aA + r1;
            rgbVal.g = maxA * (g - r2) / aA + r2;
            rgbVal.b = maxA * (b - r3) / aA + r3;

            // Color inversion while maintaining hues
            hsl_t hslVal = rgb2hsl(rgbVal);
            if (hslVal.h >= 180.0) hslVal.h -= 180.0;
            else hslVal.h += 180.0;
            rgbVal = hsl2rgb(hslVal);
            r = 255.0 - rgbVal.r;
            g = 255.0 - rgbVal.g;
            b = 255.0 - rgbVal.b;
        }
        else
        {
            a = 0;
            r = 0;
            g = 0;
            b = 0;
        }
    }

    void InvertConstHue(int& r, int& g, int& b, int& a, COLORREF& crOpt)
    {
        rgb_t rgbVal = { r, g, b };
        hsl_t hslVal = rgb2hsl(rgbVal);
        if (hslVal.h >= 180.0) hslVal.h -= 180.0;
        else hslVal.h += 180.0;
        rgbVal = hsl2rgb(hslVal);
        r = 255.0 - rgbVal.r;
        g = 255.0 - rgbVal.g;
        b = 255.0 - rgbVal.b;
    }

    struct BUCKET
    {
        CDPA<RGBQUAD, CTContainer_PolicyUnOwned<RGBQUAD>> _dpa;

        BUCKET()
        {
            _dpa.Create(16);
        }
    };

    void IncreaseBrightness(COLORREF& cr)
    {
        rgb_t rgbVal = { GetRValue(cr), GetGValue(cr), GetBValue(cr) };
        hsl_t hslVal = rgb2hsl(rgbVal);
        hslVal.l = (sqrt(hslVal.l / 255.0)) * 255;
        hslVal.s *= 1.33;
        rgbVal = hsl2rgb(hslVal);
        cr = RGB(rgbVal.r, rgbVal.g, rgbVal.b);
    }

    COLORREF GetColorFromPixel(HDC hdc, POINT pt)
    {
        COLORREF cr = GetPixel(hdc, pt.x, pt.y);
        return cr;
    }

    COLORREF GetDominantColorFromIcon(HBITMAP hbm, int iconsize, int nonGreyishThreshold)
    {
        COLORREF outDominantColor = g_isColorized ? IconColorizationColor : g_isDarkIconsEnabled ? RGB(72, 76, 80) : RGB(128, 136, 144);

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
        if (hBufferedPaint)
        {
            BufferedPaintClear(hBufferedPaint, &rcIcon);
            BitBlt(hdcPaint, rcIcon.left, rcIcon.top, rcIcon.right, rcIcon.bottom, hMemDC2, 0, 0, SRCCOPY);

            RGBQUAD* pbBuffer;
            int cxRow;
            GetBufferedPaintBits(hBufferedPaint, &pbBuffer, &cxRow);

            constexpr int bucketCoef = 86;
            constexpr int hitRatioThreshold = 7;

            constexpr int frac = 0xFF / (bucketCoef - 1);
            BUCKET rgBucket[
                (frac * frac) * (0xFF / bucketCoef)
                + (frac) * (0xFF / bucketCoef)
                + (0xFF / bucketCoef)
                + 1
            ];
            for (int row = 0; row < iconsize; ++row, pbBuffer += cxRow - iconsize)
            {
                for (int column = 0; column < iconsize; ++column, ++pbBuffer)
                {
                    if (pbBuffer->rgbReserved)
                    {
                        BYTE maxValue = max(pbBuffer->rgbRed, max(pbBuffer->rgbGreen, pbBuffer->rgbBlue));
                        BYTE minValue = min(pbBuffer->rgbRed, min(pbBuffer->rgbGreen, pbBuffer->rgbBlue));
                        if (maxValue - minValue > nonGreyishThreshold)
                        {
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
            for (SIZE_T bucketIdx = 0; bucketIdx < ARRAYSIZE(rgBucket); ++bucketIdx)
            {
                int myHits = rgBucket[bucketIdx]._dpa.GetPtrCount();
                totalHits += myHits;
                if (bucketWithMostHits == -1 || myHits > rgBucket[bucketWithMostHits]._dpa.GetPtrCount())
                {
                    bucketWithMostHits = bucketIdx;
                }
            }

            const BUCKET* bestBucket = &rgBucket[bucketWithMostHits];
            int denominator = 0;
            int totalR = 0;
            int totalG = 0;
            int totalB = 0;
            int numBestBucketHits = bestBucket->_dpa.GetPtrCount();
            if (numBestBucketHits > 0)
            {
                denominator = numBestBucketHits;
                for (int colorIdx = 0; colorIdx < numBestBucketHits; ++colorIdx)
                {
                    const RGBQUAD* color = bestBucket->_dpa.FastGetPtr(colorIdx);
                    totalR += color->rgbRed;
                    totalG += color->rgbGreen;
                    totalB += color->rgbBlue;
                }
            }

            if (MulDiv(numBestBucketHits, 100, totalHits) >= hitRatioThreshold)
            {
                outDominantColor = RGB(totalR / denominator, totalG / denominator, totalB / denominator);
            }
            if (GetRValue(outDominantColor) * 0.299 + GetGValue(outDominantColor) * 0.587 + GetBValue(outDominantColor) * 0.114 > 156)
            {
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

    COLORREF GetMostFrequentLightnessFromIcon(HBITMAP hbm, int iconsize)
    {
        COLORREF outFrequentColor = RGB(136, 136, 136);

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
        if (hBufferedPaint)
        {
            BufferedPaintClear(hBufferedPaint, &rcIcon);
            BitBlt(hdcPaint, rcIcon.left, rcIcon.top, rcIcon.right, rcIcon.bottom, hMemDC2, 0, 0, SRCCOPY);

            RGBQUAD* pbBuffer;
            int cxRow;
            GetBufferedPaintBits(hBufferedPaint, &pbBuffer, &cxRow);
            vector<BYTE> lightValues;

            for (int row = 0; row < iconsize; ++row, pbBuffer += cxRow - iconsize)
            {
                for (int column = 0; column < iconsize; ++column, ++pbBuffer)
                {
                    if (pbBuffer->rgbReserved > 64)
                    {
                        rgb_t rgbVal = { pbBuffer->rgbRed, pbBuffer->rgbGreen, pbBuffer->rgbBlue };
                        hsl_t hslVal = rgb2hsl(rgbVal);
                        hslVal.l = round(hslVal.l / 32.0) * 32;
                        if (hslVal.l > 255) hslVal.l = 255;
                        lightValues.push_back(hslVal.l);
                    }
                }
            }

            map<int, int> lightMap = {};
            int lightCount = 0;
            int lightVal = 0;

            for (auto&& item : lightValues)
            {
                lightMap[item] = lightMap.emplace(item, 0).first->second + 1;
                if (lightMap[item] >= lightCount)
                {
                    tie(lightCount, lightVal) = tie(lightMap[item], item);
                }
            }

            outFrequentColor = RGB(lightVal, lightVal, lightVal);

            lightValues.clear();
            lightMap.clear();
            EndBufferedPaint(hBufferedPaint, FALSE);
            DeleteObject(hOldBitmap);
            DeleteDC(hMemDC);
            DeleteDC(hMemDC2);
        }

        return outFrequentColor;
    }

    COLORREF GetLightestPixel(HBITMAP hbm)
    {
        COLORREF outLightestColor = RGB(255, 255, 255);
        BITMAP bm;
        GetObject(hbm, sizeof(bm), &bm);

        if (!hbm || bm.bmBitsPixel != 32)
        {
            return outLightestColor;
        }

        int bmBits = (bm.bmWidth) * (bm.bmHeight) * 4;

        BYTE* pBits = new BYTE[bmBits];
        GetBitmapBits(hbm, bmBits, pBits);

        BYTE* pPixel;
        int x, y;
        vector<BYTE> lightValues;

        for (y = 0; y < bm.bmHeight; y++)
        {
            pPixel = pBits + bm.bmWidth * 4 * y;
            for (x = 0; x < bm.bmWidth; x++)
            {
                rgb_t rgbVal = { pPixel[2] & 0xFFFFFF, pPixel[1] & 0xFFFFFF, pPixel[0] & 0xFFFFFF };
                hsl_t hslVal = rgb2hsl(rgbVal);
                lightValues.push_back((BYTE)hslVal.l);
                pPixel += 4;
            }
        }
        BYTE lightestPixel{};
        for (auto b : lightValues)
        {
            if (b > lightestPixel)
            {
                lightestPixel = b;
            }
        }
        outLightestColor = (lightestPixel, lightestPixel, lightestPixel);

        return outLightestColor;
    }
}
