#pragma once
#include "framework.h"

namespace DirectDesktop
{
	extern COLORREF ImmersiveColor;
	extern COLORREF ImmersiveColorL;
	extern COLORREF ImmersiveColorD;
	extern COLORREF IconColorizationColor;
	extern bool theme;
	extern bool touchmode;
	extern bool isColorized;
	extern bool isDarkIconsEnabled;
	extern BYTE iconColorID;
	extern const wchar_t* sheetName;

	void UpdateScale();
	void UpdateModeInfo();
	void SetTheme();
	extern HBITMAP IconToBitmap(HICON hIcon, int x = 48, int y = 48);

	void StandardBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void EnhancedBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void SimpleBitmapPixelHandler(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void UndoPremultiplication(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void DesaturateWhiten(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void DesaturateWhitenGlass(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void ColorToAlpha(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void InvertConstHue(int& r, int& g, int& b, int& a, COLORREF& crOpt);
	void IncreaseBrightness(COLORREF& cr);
	COLORREF GetColorFromPixel(HDC hdc, POINT pt);
	COLORREF GetDominantColorFromIcon(HBITMAP hbm, int iconsize, int nonGreyishThreshold);
	COLORREF GetMostFrequentLightnessFromIcon(HBITMAP hbm, int iconsize);
	COLORREF GetLightestPixel(HBITMAP hbm);
}