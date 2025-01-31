#include "BitmapHelper.h"
#include "BlurCore.h"

HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int padding)
{
    BITMAP bmp;
    GetObject(hOriginalBitmap, sizeof(BITMAP), &bmp);

    int originalWidth = bmp.bmWidth;
    int originalHeight = bmp.bmHeight;

    // New dimensions with padding
    int newWidth = originalWidth + 2 * padding;
    int newHeight = originalHeight + 2 * padding;

    // Create a new bitmap
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hNewBitmap = CreateCompatibleBitmap(hdcScreen, newWidth, newHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hNewBitmap);

    // Fill the new bitmap with the padding color
    HBRUSH hBrush = CreateSolidBrush(0);
    RECT rect = { 0, 0, newWidth, newHeight };
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);

    // Copy the original bitmap into the center of the new bitmap
    HDC hdcOriginal = CreateCompatibleDC(hdcScreen);
    HBITMAP hOldOriginalBitmap = (HBITMAP)SelectObject(hdcOriginal, hOriginalBitmap);

    BitBlt(hdcMem, padding, padding, originalWidth, originalHeight, hdcOriginal, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(hdcOriginal, hOldOriginalBitmap);
    DeleteDC(hdcOriginal);

    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return hNewBitmap;
}

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type) // type: 0 = original, 1 = color, 2 = blur
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
                a = (pPixel[3] & 0xFFFFFF) * 0.33;

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
        vector<BYTE> vResultBits = Blur(vBits, (int)bm.bmWidth, (int)bm.bmHeight, 4);
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
        vector<BYTE> vResultBitsR = Blur(vBitsR, (int)bm.bmWidth, (int)bm.bmHeight, 4);
        vector<BYTE> vResultBitsG = Blur(vBitsG, (int)bm.bmWidth, (int)bm.bmHeight, 4);
        vector<BYTE> vResultBitsB = Blur(vBitsB, (int)bm.bmWidth, (int)bm.bmHeight, 4);
        vector<BYTE> vResultBitsA = Blur(vBitsA, (int)bm.bmWidth, (int)bm.bmHeight, 4);
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