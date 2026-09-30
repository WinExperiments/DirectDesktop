#pragma once

#include <windows.h>

bool LoadPNGAsBitmap(HMODULE hModule, HBITMAP& hBitmap, int imageID);
void SetSplashImage(HWND hwndSplash, HBITMAP hbmpSplash);