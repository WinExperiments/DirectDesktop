#include "BitmapHelper.h"
#include "BlurCore.h"

HBITMAP CreateTextBitmap(LPCWSTR text, int width, int height, DWORD ellipsisType) {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBitmapData = NULL;
    HBITMAP hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBitmapData, NULL, 0);
    if (!hBitmap) return NULL;
    HDC hdcMem = CreateCompatibleDC(NULL);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    memset(pBitmapData, 0, width * height * 4);
    LOGFONTW lf{};
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL);
    HFONT hFont = CreateFontIndirectW(&lf);
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(255, 255, 255));
    RECT rc = {2, 1, width - 2, height - 1};
    DrawTextW(hdcMem, text, -1, &rc, DT_CENTER | ellipsisType | DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_NOPREFIX | DT_EDITCONTROL);
    DWORD* pixels = (DWORD*)pBitmapData;
    for (int i = 0; i < width * height; i++) {
        BYTE* pPixel = (BYTE*)&pixels[i];
        if (pPixel[0] | pPixel[1] | pPixel[2]) {
            pPixel[3] = 255;
        }
    }
    SelectObject(hdcMem, hOldFont);
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hFont);
    DeleteDC(hdcMem);

    return hBitmap;
}

HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int padding)
{
    BITMAP bmp;
    GetObject(hOriginalBitmap, sizeof(BITMAP), &bmp);

    int originalWidth = bmp.bmWidth;
    int originalHeight = bmp.bmHeight;
    int newWidth = originalWidth + 2 * padding;
    int newHeight = originalHeight + 2 * padding;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hNewBitmap = CreateCompatibleBitmap(hdcScreen, newWidth, newHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hNewBitmap);
    HBRUSH hBrush = CreateSolidBrush(0);

    RECT rect = { 0, 0, newWidth, newHeight };
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);
    HDC hdcOriginal = CreateCompatibleDC(hdcScreen);
    HBITMAP hOldOriginalBitmap = (HBITMAP)SelectObject(hdcOriginal, hOriginalBitmap);

    BitBlt(hdcMem, padding, padding, originalWidth, originalHeight, hdcOriginal, 0, 0, SRCCOPY);

    SelectObject(hdcOriginal, hOldOriginalBitmap);
    DeleteDC(hdcOriginal);
    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return hNewBitmap;
}

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type, int blurradius, float alpha) // type: 0 = original, 1 = color, 2 = blur
{
    BITMAP bm;
    GetObject(hbm, sizeof(bm), &bm);

    if (!hbm || bm.bmBitsPixel != 32)
    {
        return false;
    }

    int bmBits = (bm.bmWidth) * (bm.bmHeight) * 4;

    switch (type) {
    case 1: {
        BYTE* pBits = new BYTE[bmBits];
        GetBitmapBits(hbm, bmBits, pBits);

        BYTE* pPixel;
        int x, y;
        int r, g, b, a;

        for (y = 0; y < bm.bmHeight; y++)
        {
            pPixel = pBits + bm.bmWidth * 4 * y;

            for (x = 0; x < bm.bmWidth; x++)
            {
                r = pPixel[2] & 0xFFFFFF;
                g = pPixel[1] & 0xFFFFFF;
                b = pPixel[0] & 0xFFFFFF;
                a = pPixel[3] & 0xFFFFFF;

                handler(r, g, b, a);

                pPixel[2] = r;
                pPixel[1] = g;
                pPixel[0] = b;
                pPixel[3] = a;

                pPixel += 4;
            }
        }

        SetBitmapBits(hbm, bmBits, pBits);
        delete[] pBits;
        break;
    }
    case 0: {
        BYTE* pBits = new BYTE[bmBits];
        GetBitmapBits(hbm, bmBits, pBits);

        BYTE* pPixel;
        int x, y;
        int r, g, b, a;

        for (y = 0; y < bm.bmHeight; y++)
        {
            pPixel = pBits + (bm.bmWidth) * 4 * y;

            for (x = 0; x < bm.bmWidth; x++)
            {
                a = (pPixel[3] & 0xFFFFFF) * alpha;

                handler(r, g, b, a);

                pPixel[2] = r;
                pPixel[1] = g;
                pPixel[0] = b;
                pPixel[3] = a;

                pPixel += 4;
            }
        }

        vector<BYTE> vBits;
        vBits.assign(pBits, pBits + bmBits / 4);
        int channel = 0;
        for (int alpha = 3; alpha < bmBits; alpha += 4) {
            vBits[channel++] = pBits[alpha];
        }
        vector<BYTE> vResultBits = Blur(vBits, (int)bm.bmWidth, (int)bm.bmHeight, blurradius);
        channel = 0;
        for (int alpha = 3; alpha < bmBits; alpha += 4) {
            pBits[alpha] = vResultBits[channel++];
        }

        SetBitmapBits(hbm, bmBits, pBits);
        delete[] pBits;
        vBits.clear();
        vResultBits.clear();
        break;
    }
    case 2: {
        BYTE* pBits = new BYTE[bmBits];
        GetBitmapBits(hbm, bmBits, pBits);

        int x, y;
        int r, g, b, a;

        vector<BYTE> vBitsR, vBitsG, vBitsB, vBitsA;
        vBitsR.assign(pBits, pBits + bmBits / 4);
        vBitsG.assign(pBits, pBits + bmBits / 4);
        vBitsB.assign(pBits, pBits + bmBits / 4);
        vBitsA.assign(pBits, pBits + bmBits / 4);
        int channel = 0;
        for (int alpha = 0; alpha < bmBits; alpha += 4) {
            vBitsB[channel++] = pBits[alpha];
        }
        channel = 0;
        for (int alpha = 1; alpha < bmBits; alpha += 4) {
            vBitsG[channel++] = pBits[alpha];
        }
        channel = 0;
        for (int alpha = 2; alpha < bmBits; alpha += 4) {
            vBitsR[channel++] = pBits[alpha];
        }
        channel = 0;
        for (int alpha = 3; alpha < bmBits; alpha += 4) {
            vBitsA[channel++] = pBits[alpha];
        }
        vector<BYTE> vResultBitsR = Blur(vBitsR, (int)bm.bmWidth, (int)bm.bmHeight, blurradius);
        vector<BYTE> vResultBitsG = Blur(vBitsG, (int)bm.bmWidth, (int)bm.bmHeight, blurradius);
        vector<BYTE> vResultBitsB = Blur(vBitsB, (int)bm.bmWidth, (int)bm.bmHeight, blurradius);
        vector<BYTE> vResultBitsA = Blur(vBitsA, (int)bm.bmWidth, (int)bm.bmHeight, blurradius);
        channel = 0;
        for (int alpha = 0; alpha < bmBits; alpha += 4) {
            pBits[alpha] = vResultBitsB[channel++];
        }
        channel = 0;
        for (int alpha = 1; alpha < bmBits; alpha += 4) {
            pBits[alpha] = vResultBitsG[channel++];
        }
        channel = 0;
        for (int alpha = 2; alpha < bmBits; alpha += 4) {
            pBits[alpha] = vResultBitsR[channel++];
        }
        channel = 0;
        for (int alpha = 3; alpha < bmBits; alpha += 4) {
            pBits[alpha] = vResultBitsA[channel++];
        }

        SetBitmapBits(hbm, bmBits, pBits);
        delete[] pBits;
        vBitsR.clear();
        vBitsG.clear();
        vBitsB.clear();
        vBitsA.clear();
        vResultBitsR.clear();
        vResultBitsG.clear();
        vResultBitsB.clear();
        vResultBitsA.clear();
        break;
    }
    }


    return true;
}