#include "AccentColorHelper.h"
#include "StyleModifier.h"
#include "ColorHelper.h"

namespace DirectDesktop
{
	COLORREF g_dwAccent{};
	hsl_t g_hslAccent{};
	int g_hslLightAccentH{};
	int g_hslDarkAccentH{};
	double g_hslEnhancedAccentL{};

	bool UpdateAccentColor(COLORREF crKey)
	{
		g_hslAccent.h = rgb2hsl({
			(double)GetRValue(crKey) / 255,
			(double)GetGValue(crKey) / 255,
			(double)GetBValue(crKey) / 255 }).h;
		if (g_hslAccent.h >= 0 && g_hslAccent.h < 125) {
			g_hslLightAccentH = 60;
		}
		else if (g_hslAccent.h >= 125 && g_hslAccent.h < 150) {
			g_hslLightAccentH = 120;
		}
		else if (g_hslAccent.h >= 150 && g_hslAccent.h < 240) {
			g_hslLightAccentH = 180;
		}
		else if (g_hslAccent.h >= 240 && g_hslAccent.h < 345) {
			g_hslLightAccentH = 300;
		}
		else if (g_hslAccent.h >= 345 && g_hslAccent.h < 360) {
			g_hslLightAccentH = 420;
		}
		if (g_hslAccent.h >= 0 && g_hslAccent.h < 60) {
			g_hslDarkAccentH = 0;
		}
		else if (g_hslAccent.h >= 60 && g_hslAccent.h < 180) {
			g_hslDarkAccentH = 120;
		}
		else if (g_hslAccent.h >= 180 && g_hslAccent.h < 300) {
			g_hslDarkAccentH = 240;
		}
		else if (g_hslAccent.h >= 300 && g_hslAccent.h < 360) {
			g_hslDarkAccentH = 360;
		}

		g_hslAccent.s = double(rgb2hsl({
			(double)GetRValue(crKey) / 254.999999999,
			(double)GetGValue(crKey) / 254.999999999,
			(double)GetBValue(crKey) / 254.999999999 }).s);

		g_hslAccent.l = ((double)(rgb2hsl({
			(double)GetRValue(crKey) / 255,
			(double)GetGValue(crKey) / 255,
			(double)GetBValue(crKey) / 255 }).l) * -255.0) + 107.0;

		return true;
	}
}