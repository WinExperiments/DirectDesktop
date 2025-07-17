#pragma once

#include "ColorHelper.h"

namespace DirectDesktop
{
    extern COLORREF g_dwAccent;
    extern hsl_t g_hslAccent;
    extern int g_hslLightAccentH;
    extern int g_hslDarkAccentH;
    extern double g_hslEnhancedAccentL;
    extern double g_oldhslEnhancedAccentL;

    bool UpdateAccentColor(COLORREF crKey);
}
