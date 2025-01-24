#include "framework.h"
#include "BlurHandler.h"
#include "DirectUI\DirectUI.h"
#include <d2d1.h>

using namespace DirectUI;

/*HBITMAP CaptureSurfaceToBitmap(HDC sourceDC, RECT rect) {
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Create a compatible bitmap and DC
    //HDC memDC = CreateCompatibleDC(sourceDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(sourceDC, width, height);
    //HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    // Copy content from the source DC to the memory DC
    //BitBlt(memDC, 0, 0, width, height, sourceDC, rect.left, rect.top, SRCCOPY);

    // Restore and cleanup
    //SelectObject(memDC, oldBitmap);
    //DeleteDC(memDC);

    return hBitmap;
}*/

HBITMAP CopyScreenToBitmap(HDC hSourceDC, int x, int y, int width, int height) {
    // Create a compatible DC and bitmap
    HDC memDC = CreateCompatibleDC(hSourceDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hSourceDC, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    // Copy the content from the source DC
    BitBlt(memDC, 0, 0, width, height, hSourceDC, x, y, SRCCOPY);

    // Restore and clean up
    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);

    return hBitmap;
}

HBITMAP CaptureTextAsBitmap(HWND hWnd, int x, int y, int width, int height) {
    // Get the device context for the entire screen
    HDC screenDC = GetDC(hWnd);

    // Capture the text area into an HBITMAP
    HBITMAP hBitmap = CopyScreenToBitmap(screenDC, x, y, width, height);

    // Release the screen DC
    ReleaseDC(hWnd, screenDC);

    // The HBITMAP now contains the captured content, including the text "Hello"
    // You can now use the HBITMAP as needed (e.g., convert to ID2D1Bitmap for Direct2D processing)
    return hBitmap;
}

ID2D1Bitmap* HBITMAPToID2D1Bitmap(ID2D1RenderTarget* pRenderTarget, HBITMAP hBitmap) {
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    // Create a compatible bitmap source
    D2D1_BITMAP_PROPERTIES bitmapProperties = {};
    bitmapProperties.pixelFormat = pRenderTarget->GetPixelFormat();

    D2D1_SIZE_U size = D2D1::SizeU(bmp.bmWidth, bmp.bmHeight);

    // Create the D2D1Bitmap from the raw data
    ID2D1Bitmap* pBitmap = nullptr;
    pRenderTarget->CreateBitmap(
        size,
        bmp.bmBits,
        bmp.bmWidthBytes,
        bitmapProperties,
        &pBitmap
    );

    return pBitmap;
}
