#pragma once
#include "framework.h"

extern COLORREF ImmersiveColor;
extern bool theme;
extern const wchar_t* sheetName;

void UpdateScale();
void UpdateModeInfo();
void UpdateFontSize();
void SetTheme();
extern HBITMAP IconToBitmap(HICON hIcon, int x = 48, int y = 48);

void StandardBitmapPixelHandler(int& r, int& g, int& b, int& a);
void SimpleBitmapPixelHandler(int& r, int& g, int& b, int& a);
void UndoPremultiplication(int& r, int& g, int& b, int& a);