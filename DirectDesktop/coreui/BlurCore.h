#pragma once
#include "..\framework.h"
#include "StyleModifier.h"
#include "..\backend\DirectoryHelper.h"
#include <dwmapi.h>
#include <vector>
#include <cmath>

using namespace std;

namespace DirectDesktop
{
    // https://blog.ivank.net/fastest-gaussian-blur.html

    vector<int> boxesForGauss(float sigma, int n) // standard deviation, number of boxes
    {
        auto wIdeal = sqrt((12 * sigma * sigma / n) + 1); // Ideal averaging filter width 
        int wl = floor(wIdeal);
        if (wl % 2 == 0)
            wl--;
        int wu = wl + 2;

        auto mIdeal = (12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / (-4 * wl - 4);
        int m = round(mIdeal);

        vector<int> sizes(n);
        for (auto i = 0; i < n; i++)
            sizes[i] = i < m ? wl : wu;
        return sizes;
    }

    void boxBlurH_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r)
    {
        float iarr = 1.f / (r + r + 1);
        for (auto i = 0; i < h; i++)
        {
            auto ti = i * w, li = ti, ri = ti + r;
            auto fv = scl[ti], lv = scl[ti + w - 1];
            auto val = (r + 1) * fv;
            for (auto j = 0; j < r; j++) val += scl[ti + j];
            for (auto j = 0; j <= r; j++)
            {
                val += scl[ri++] - fv;
                tcl[ti++] = round(val * iarr);
            }
            for (auto j = r + 1; j < w - r; j++)
            {
                val += scl[ri++] - scl[li++];
                tcl[ti++] = round(val * iarr);
            }
            for (auto j = w - r; j < w; j++)
            {
                val += lv - scl[li++];
                tcl[ti++] = round(val * iarr);
            }
        }
    }

    void boxBlurT_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r)
    {
        float iarr = 1.f / (r + r + 1);
        for (auto i = 0; i < w; i++)
        {
            auto ti = i, li = ti, ri = ti + r * w;
            auto fv = scl[ti], lv = scl[ti + w * (h - 1)];
            auto val = (r + 1) * fv;
            for (auto j = 0; j < r; j++) val += scl[ti + j * w];
            for (auto j = 0; j <= r; j++)
            {
                val += scl[ri] - fv;
                tcl[ti] = round(val * iarr);
                ri += w;
                ti += w;
            }
            for (auto j = r + 1; j < h - r; j++)
            {
                val += scl[ri] - scl[li];
                tcl[ti] = round(val * iarr);
                li += w;
                ri += w;
                ti += w;
            }
            for (auto j = h - r; j < h; j++)
            {
                val += lv - scl[li];
                tcl[ti] = round(val * iarr);
                li += w;
                ti += w;
            }
        }
    }

    void boxBlur_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r)
    {
        for (auto i = 0; i < scl.size(); i++) tcl[i] = scl[i];
        boxBlurH_4(tcl, scl, w, h, r);
        boxBlurT_4(scl, tcl, w, h, r);
    }

    void gaussBlur_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r)
    {
        auto bxs = boxesForGauss(r, 3);
        boxBlur_4(scl, tcl, w, h, (bxs[0] - 1) / 2);
        boxBlur_4(tcl, scl, w, h, (bxs[1] - 1) / 2);
        boxBlur_4(scl, tcl, w, h, (bxs[2] - 1) / 2);
    }

    void Blur(vector<BYTE>& source, int w, int h, int radius)
    {
        vector<BYTE> lowpass = source; // copy constructor
        vector<BYTE> target(source.size(), 0);
        gaussBlur_4(lowpass, target, w, h, radius);

        source = target;
        lowpass.clear();
        target.clear();
    }

    // https://github.com/ALTaleX531/TranslucentFlyouts/blob/master/TFMain/EffectHelper.hpp

    enum class WINDOWCOMPOSITIONATTRIBUTE
    {
        WCA_UNDEFINED,
        WCA_NCRENDERING_ENABLED,
        WCA_NCRENDERING_POLICY,
        WCA_TRANSITIONS_FORCEDISABLED,
        WCA_ALLOW_NCPAINT,
        WCA_CAPTION_BUTTON_BOUNDS,
        WCA_NONCLIENT_RTL_LAYOUT,
        WCA_FORCE_ICONIC_REPRESENTATION,
        WCA_EXTENDED_FRAME_BOUNDS,
        WCA_HAS_ICONIC_BITMAP,
        WCA_THEME_ATTRIBUTES,
        WCA_NCRENDERING_EXILED,
        WCA_NCADORNMENTINFO,
        WCA_EXCLUDED_FROM_LIVEPREVIEW,
        WCA_VIDEO_OVERLAY_ACTIVE,
        WCA_FORCE_ACTIVEWINDOW_APPEARANCE,
        WCA_DISALLOW_PEEK,
        WCA_CLOAK,
        WCA_CLOAKED,
        WCA_ACCENT_POLICY,
        WCA_FREEZE_REPRESENTATION,
        WCA_EVER_UNCLOAKED,
        WCA_VISUAL_OWNER,
        WCA_HOLOGRAPHIC,
        WCA_EXCLUDED_FROM_DDA,
        WCA_PASSIVEUPDATEMODE,
        WCA_USEDARKMODECOLORS,
        WCA_CORNER_STYLE,
        WCA_PART_COLOR,
        WCA_DISABLE_MOVESIZE_FEEDBACK,
        WCA_LAST
    };

    struct WINDOWCOMPOSITIONATTRIBDATA
    {
        DWORD dwAttribute;
        PVOID pvData;
        SIZE_T cbData;
    };

    enum class ACCENT_STATE
    {
        ACCENT_DISABLED,
        ACCENT_ENABLE_GRADIENT,
        ACCENT_ENABLE_TRANSPARENTGRADIENT,
        ACCENT_ENABLE_BLURBEHIND, // Removed in Windows 11 22H2+
        ACCENT_ENABLE_ACRYLICBLURBEHIND,
        ACCENT_ENABLE_HOSTBACKDROP,
        ACCENT_INVALID_STATE
    };

    enum class ACCENT_FLAG
    {
        ACCENT_NONE,
        ACCENT_ENABLE_MODERN_ACRYLIC_RECIPE = 1 << 1, // Windows 11 22H2+ exclusive
        ACCENT_ENABLE_GRADIENT_COLOR = 1 << 1, // ACCENT_ENABLE_BLURBEHIND
        ACCENT_ENABLE_FULLSCREEN = 1 << 2,
        ACCENT_ENABLE_BORDER_LEFT = 1 << 5,
        ACCENT_ENABLE_BORDER_TOP = 1 << 6,
        ACCENT_ENABLE_BORDER_RIGHT = 1 << 7,
        ACCENT_ENABLE_BORDER_BOTTOM = 1 << 8,
        ACCENT_ENABLE_BLUR_RECT = 1 << 9, // DwmpUpdateAccentBlurRect, it conflicts with ACCENT_ENABLE_GRADIENT_COLOR when using ACCENT_ENABLE_BLURBEHIND
        ACCENT_ENABLE_BORDER = ACCENT_ENABLE_BORDER_LEFT | ACCENT_ENABLE_BORDER_TOP | ACCENT_ENABLE_BORDER_RIGHT | ACCENT_ENABLE_BORDER_BOTTOM
    };

    struct ACCENT_POLICY
    {
        DWORD AccentState;
        DWORD AccentFlags;
        DWORD dwGradientColor;
        DWORD dwAnimationId;
    };

    typedef BOOL (WINAPI*pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

    void ToggleAcrylicBlur(HWND hwnd, bool blur, bool fullscreen, Element* peOptional)
    {
        if (peOptional) peOptional->SetClass(L"TransparentDisabled");
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"EnableTransparency") == 1 || !fullscreen)
        {
            HMODULE hUser = GetModuleHandleW(L"user32.dll");
            if (hUser)
            {
                pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
                    (pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

                if (SetWindowCompositionAttribute)
                {
                    int WindowsBuild = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\ShellCommon", L"BuildNumber");
                    int blurcolor = fullscreen ? g_theme ? 0x33D3D3D3 : 0x33202020 : WindowsBuild < 22523 ? g_theme ? 0xDCE4E4E4 : 0xCA1F1F1F : g_theme ? 0x00F8F8F8 : 0x00303030;
                    ACCENT_POLICY policy = { static_cast<DWORD>(ACCENT_STATE::ACCENT_DISABLED), fullscreen ? static_cast<DWORD>(ACCENT_FLAG::ACCENT_NONE) : static_cast<DWORD>(ACCENT_FLAG::ACCENT_ENABLE_BORDER), blurcolor, 0 };
                    WINDOWCOMPOSITIONATTRIBDATA data = { static_cast<DWORD>(WINDOWCOMPOSITIONATTRIBUTE::WCA_ACCENT_POLICY), &policy, sizeof(ACCENT_POLICY) };
                    policy.AccentState = static_cast<DWORD>(blur ? ACCENT_STATE::ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_STATE::ACCENT_DISABLED);
                    if (!fullscreen && WindowsBuild >= 22523)
                        policy.AccentFlags |= static_cast<DWORD>(ACCENT_FLAG::ACCENT_ENABLE_MODERN_ACRYLIC_RECIPE);
                    SetWindowCompositionAttribute(hwnd, &data);
                    if (peOptional) peOptional->SetClass(L"TransparentEnabled");
                }
            }
        }
    }

    void ToggleAcrylicBlur2(HWND hwnd, bool blur, bool fullscreen, Element* peOptional)
    {
    }
}
