#include "SplashScreen.h"
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")

// https://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
IStream* CreateStreamOnResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType)
{
    IStream* ipStream = nullptr;

    HRSRC hrsrc = FindResourceW(hModule, lpName, lpType);
    if (hrsrc == nullptr) return ipStream;

    DWORD dwResourceSize = SizeofResource(hModule, hrsrc);
    HGLOBAL hglbImage = LoadResource(hModule, hrsrc);
    if (hglbImage == nullptr) return ipStream;

    LPVOID pvSourceResourceData = LockResource(hglbImage);
    if (pvSourceResourceData == nullptr) return ipStream;

    HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
    if (hgblResourceData == nullptr) return ipStream;

    LPVOID pvResourceData = GlobalLock(hgblResourceData);
    if (pvResourceData == nullptr)
    {
        GlobalFree(hgblResourceData);
        return ipStream;
    }

    CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);
    GlobalUnlock(hgblResourceData);

    if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream))) return ipStream;
}

IWICBitmapSource* LoadBitmapFromStream(IStream* ipImageStream)
{
    IWICBitmapSource* ipBitmap = nullptr;

    IWICBitmapDecoder* ipDecoder = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER, __uuidof(ipDecoder), reinterpret_cast<void**>(&ipDecoder)))) return ipBitmap;

    if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad)))
    {
        ipDecoder->Release();
        return ipBitmap;
    }

    UINT nFrameCount = 0;
    if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
    {
        ipDecoder->Release();
        return ipBitmap;
    }

    IWICBitmapFrameDecode* ipFrame = nullptr;
    if (FAILED(ipDecoder->GetFrame(0, &ipFrame)))
    {
        ipDecoder->Release();
        return ipBitmap;
    }

    WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
    ipFrame->Release();
    ipDecoder->Release();
    return ipBitmap;
}

bool CreateHBITMAP(HBITMAP& hBitmap, IWICBitmapSource* ipBitmap)
{
    if (hBitmap) DeleteObject(hBitmap);
    UINT width = 0;
    UINT height = 0;
    if (FAILED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0) return false;

    BITMAPINFO bminfo;
    ZeroMemory(&bminfo, sizeof(bminfo));
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = width;
    bminfo.bmiHeader.biHeight = -((LONG)height);
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    void* pvImageBits = nullptr;
    HDC hdcScreen = GetDC(nullptr);
    HBITMAP hInternalBitmap = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);
    if (hInternalBitmap == nullptr) return false;

    const UINT cbStride = width * 4;
    const UINT cbImage = cbStride * height;
    if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE*>(pvImageBits))))
    {
        DeleteObject(hInternalBitmap);
        hInternalBitmap = nullptr;
    }
    hBitmap = hInternalBitmap;
    return false;
}

bool LoadPNGAsBitmap(HMODULE hModule, HBITMAP& hBitmap, int imageID)
{
    if (hBitmap) DeleteObject(hBitmap);
    IStream* ipImageStream{};
    ipImageStream = CreateStreamOnResource(hModule, MAKEINTRESOURCEW(imageID), L"PNG");
    if (ipImageStream == nullptr) return false;

    IWICBitmapSource* ipBitmap = LoadBitmapFromStream(ipImageStream);
    if (ipBitmap == nullptr)
    {
        ipImageStream->Release();
        return false;
    }

    HBITMAP hInternalBitmap{};
    CreateHBITMAP(hInternalBitmap, ipBitmap);
    ipBitmap->Release();
    ipImageStream->Release();
    hBitmap = hInternalBitmap;
    return true;
}

void SetSplashImage(HWND hwndSplash, HBITMAP hbmpSplash)
{
    BITMAP bm;
    GetObject(hbmpSplash, sizeof(bm), &bm);
    SIZE sizeSplash = { bm.bmWidth, bm.bmHeight };

    POINT ptZero = { 0 };
    HMONITOR hmonPrimary = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorinfo = { 0 };
    monitorinfo.cbSize = sizeof(monitorinfo);
    GetMonitorInfo(hmonPrimary, &monitorinfo);

    const RECT& rcWork = monitorinfo.rcWork;
    POINT ptOrigin;
    ptOrigin.x = rcWork.left + (rcWork.right - rcWork.left - sizeSplash.cx) / 2;
    ptOrigin.y = rcWork.top + (rcWork.bottom - rcWork.top - sizeSplash.cy) / 2;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmpSplash);

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(hwndSplash, hdcScreen, &ptOrigin, &sizeSplash, hdcMem, &ptZero, RGB(0, 0, 0), &blend, ULW_ALPHA);

    SelectObject(hdcMem, hbmpOld);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}