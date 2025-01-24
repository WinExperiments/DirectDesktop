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

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, bool type) // type: true = color, false = blur
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
    }


    return true;
}