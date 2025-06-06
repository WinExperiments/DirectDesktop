#include "BitmapHelper.h"
#include "BlurCore.h"
#include <wincodec.h>
#pragma comment (lib, "WindowsCodecs.lib")

// https://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
IStream* CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType) {
    IStream* ipStream = nullptr;

    HRSRC hrsrc = FindResourceW(NULL, lpName, lpType);
    if (hrsrc == nullptr) return ipStream;

    DWORD dwResourceSize = SizeofResource(NULL, hrsrc);
    HGLOBAL hglbImage = LoadResource(NULL, hrsrc);
    if (hglbImage == nullptr) return ipStream;

    LPVOID pvSourceResourceData = LockResource(hglbImage);
    if (pvSourceResourceData == nullptr) return ipStream;

    HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
    if (hgblResourceData == nullptr) return ipStream;

    LPVOID pvResourceData = GlobalLock(hgblResourceData);
    if (pvResourceData == nullptr) {
        GlobalFree(hgblResourceData);
        return ipStream;
    }

    CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);
    GlobalUnlock(hgblResourceData);

    if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream))) return ipStream;
}
IWICBitmapSource* LoadBitmapFromStream(IStream* ipImageStream) {
    IWICBitmapSource* ipBitmap = nullptr;

    IWICBitmapDecoder* ipDecoder = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER, __uuidof(ipDecoder), reinterpret_cast<void**>(&ipDecoder)))) return ipBitmap;

    if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad))) {
        ipDecoder->Release();
        return ipBitmap;
    }

    UINT nFrameCount = 0;
    if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1) {
        ipDecoder->Release();
        return ipBitmap;
    }

    IWICBitmapFrameDecode* ipFrame = nullptr;
    if (FAILED(ipDecoder->GetFrame(0, &ipFrame))) {
        ipDecoder->Release();
        return ipBitmap;
    }

    WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
    ipFrame->Release();
    ipDecoder->Release();
    return ipBitmap;
}
HBITMAP CreateHBITMAP(IWICBitmapSource* ipBitmap) {
    HBITMAP hbmp = nullptr;

    UINT width = 0;
    UINT height = 0;
    if (FAILED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0) return hbmp;

    BITMAPINFO bminfo;
    ZeroMemory(&bminfo, sizeof(bminfo));
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = width;
    bminfo.bmiHeader.biHeight = -((LONG)height);
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    void* pvImageBits = nullptr;
    HDC hdcScreen = GetDC(NULL);
    hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
    ReleaseDC(NULL, hdcScreen);
    if (hbmp == nullptr) return hbmp;

    const UINT cbStride = width * 4;
    const UINT cbImage = cbStride * height;
    if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE*>(pvImageBits)))) {
        DeleteObject(hbmp);
        hbmp = nullptr;
    }
    return hbmp;
}
HBITMAP LoadPNGAsBitmap(int imageID) {
    HBITMAP convertedPNG = nullptr;

    IStream* ipImageStream{};
    ipImageStream = CreateStreamOnResource(MAKEINTRESOURCE(imageID), _T("PNG"));
    if (ipImageStream == nullptr) return convertedPNG;

    IWICBitmapSource* ipBitmap = LoadBitmapFromStream(ipImageStream);
    if (ipBitmap == nullptr) {
        ipImageStream->Release();
        return convertedPNG;
    }

    convertedPNG = CreateHBITMAP(ipBitmap);
    ipBitmap->Release();
    ipImageStream->Release();
    return convertedPNG;
}

TEXTMETRICW textm;
HBITMAP CreateTextBitmap(LPCWSTR text, int width, int height, DWORD ellipsisType, bool touch) {
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
    SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, dpi);
    if (touch) lf.lfHeight *= 1.25;
    HFONT hFont = CreateFontIndirectW(&lf);
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(255, 255, 255));
    RECT rc = {2, 0, width - 2, height};
    DrawTextW(hdcMem, text, -1, &rc, ellipsisType | DT_LVICON);
    DWORD* pixels = (DWORD*)pBitmapData;
    for (int i = 0; i < width * height; i++) {
        BYTE* pPixel = (BYTE*)&pixels[i];
        if (pPixel[0] | pPixel[1] | pPixel[2]) {
            pPixel[3] = 255;
        }
    }
    GetTextMetricsW(hdcMem, &textm);
    SelectObject(hdcMem, hOldFont);
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hFont);
    DeleteDC(hdcMem);

    return hBitmap;
}

HBITMAP AddPaddingToBitmap(HBITMAP hOriginalBitmap, int pL, int pT, int pR, int pB)
{
    BITMAP bmp;
    GetObject(hOriginalBitmap, sizeof(BITMAP), &bmp);

    int originalWidth = bmp.bmWidth;
    int originalHeight = bmp.bmHeight;
    int newWidth = originalWidth + pL + pR;
    int newHeight = originalHeight + pT + pB;

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

    BitBlt(hdcMem, pL, pT, originalWidth, originalHeight, hdcOriginal, 0, 0, SRCCOPY);

    SelectObject(hdcOriginal, hOldOriginalBitmap);
    DeleteDC(hdcOriginal);
    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return hNewBitmap;
}

bool IterateBitmap(HBITMAP hbm, BitmapPixelHandler handler, int type, unsigned int blurradius, float alphaValue, COLORREF crOpt) // type: 0 = original, 1 = color, 2 = blur, 3 = solid color
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

                handler(r, g, b, a, crOpt);

                pPixel[2] = r;
                pPixel[1] = g;
                pPixel[0] = b;
                a *= alphaValue;
                if (a > 255) a = 255;
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
                a = (pPixel[3] & 0xFFFFFF);

                handler(r, g, b, a, crOpt);

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
        vector<BYTE> vResultBits;
        if (blurradius >= 1) vResultBits = Blur(vBits, (int)bm.bmWidth, (int)bm.bmHeight, blurradius);
        else vResultBits = vBits;
        channel = 0;
        for (int alpha = 3; alpha < bmBits; alpha += 4) {
            short tempAlpha = vResultBits[channel++] * alphaValue;
            if (tempAlpha > 255) tempAlpha = 255;
            pBits[alpha] = tempAlpha;
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
    case 3: {
        // handlers do not affect this, except a few.
        // SimpleBitmapPixelHandler: converts all alpha to the alphaValue arg
        // UndoPremultiplication: premultiplies the alpha by itself (lol)
        BYTE* pBits = new BYTE[bmBits];
        GetBitmapBits(hbm, bmBits, pBits);

        BYTE* pPixel;
        int x, y;

        for (y = 0; y < bm.bmHeight; y++)
        {
            pPixel = pBits + bm.bmWidth * 4 * y;

            for (x = 0; x < bm.bmWidth; x++)
            {
                pPixel[2] = (int)(crOpt % 16777216);
                pPixel[1] = (int)((crOpt / 256) % 65536);
                pPixel[0] = (int)((crOpt / 65536) % 256);
                pPixel[3] = (handler == SimpleBitmapPixelHandler) ? 255 * alphaValue : pPixel[3] * alphaValue;
                if (handler == UndoPremultiplication) pPixel[3] = pow(pPixel[3] / 255.0, 2) * 255.0;

                pPixel += 4;
            }
        }

        SetBitmapBits(hbm, bmBits, pBits);
        delete[] pBits;
        break;
    }
    }

    return true;
}

bool CompositeBitmaps(HBITMAP hbmBg, HBITMAP hbmFg, bool hardLight, float hlCoef) {
    BITMAP bmBg, bmFg;
    GetObject(hbmBg, sizeof(bmBg), &bmBg);
    GetObject(hbmFg, sizeof(bmFg), &bmFg);
    if (!hbmBg || !hbmFg || bmBg.bmWidth != bmFg.bmWidth || bmBg.bmHeight != bmFg.bmHeight || bmBg.bmBitsPixel != 32 || bmFg.bmBitsPixel != 32) {
        return false;
    }
    int bmBits = (bmBg.bmWidth) * (bmBg.bmHeight) * 4;
    BYTE* pBitsBg = new BYTE[bmBits];
    GetBitmapBits(hbmBg, bmBits, pBitsBg);
    BYTE* pBitsFg = new BYTE[bmBits];
    GetBitmapBits(hbmFg, bmBits, pBitsFg);

    BYTE* pPixel, *pPixelSec;
    int x, y;

    for (y = 0; y < bmBg.bmHeight; y++)
    {
        pPixel = pBitsBg + bmBg.bmWidth * 4 * y;
        pPixelSec = pBitsFg + bmFg.bmWidth * 4 * y;
        for (x = 0; x < bmBg.bmWidth; x++)
        {
            float aBg = pPixel[3] / 255.0;
            float aFg = pPixelSec[3] / 255.0;
            float resultAlpha = aFg + aBg * (1.0 - aFg);
            if (resultAlpha < 0.003) {
                pPixel[2] = 0, pPixel[1] = 0, pPixel[0] = 0, pPixel[3] = 0;
            }
            else {
                float r, g, b;
                r = (pPixelSec[2] * aFg + pPixel[2] * aBg * (1.0 - aFg)) / resultAlpha;
                g = (pPixelSec[1] * aFg + pPixel[1] * aBg * (1.0 - aFg)) / resultAlpha;
                b = (pPixelSec[0] * aFg + pPixel[0] * aBg * (1.0 - aFg)) / resultAlpha;
                if (hardLight) {
                    float r2 = (pPixelSec[2] > 128) ? (1 - 2 * (1 - pPixel[2] / 255.0) * (1 - pPixelSec[2] / 255.0)) * 255 : 2 * (pPixel[2] * pPixelSec[2]) / 255.0;
                    float g2 = (pPixelSec[1] > 128) ? (1 - 2 * (1 - pPixel[1] / 255.0) * (1 - pPixelSec[1] / 255.0)) * 255 : 2 * (pPixel[1] * pPixelSec[1]) / 255.0;
                    float b2 = (pPixelSec[0] > 128) ? (1 - 2 * (1 - pPixel[0] / 255.0) * (1 - pPixelSec[0] / 255.0)) * 255 : 2 * (pPixel[0] * pPixelSec[0]) / 255.0;
                    r = r2 * hlCoef + r * (1 - hlCoef), g = g2 * hlCoef + g * (1 - hlCoef), b = b2 * hlCoef + b * (1 - hlCoef);
                }
                pPixel[2] = r;
                pPixel[1] = g;
                pPixel[0] = b;
                pPixel[3] = resultAlpha * 255.0;
            }

            pPixel += 4;
            pPixelSec += 4;
        }
    }

    SetBitmapBits(hbmBg, bmBits, pBitsBg);
    delete[] pBitsBg;
    delete[] pBitsFg;
    return true;
}

void BlurBackground(HWND hwnd, bool blur, bool fullscreen) {
    ToggleAcrylicBlur(hwnd, blur, fullscreen);
}