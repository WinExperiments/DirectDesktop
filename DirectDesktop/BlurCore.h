#pragma once
#include "framework.h"
#include "StyleModifier.h"
#include <vector>
#include <cmath>

using namespace std;

// https://blog.ivank.net/fastest-gaussian-blur.html

vector<int> boxesForGauss(float sigma, int n)  // standard deviation, number of boxes
{
    auto wIdeal = sqrt((12 * sigma * sigma / n) + 1);  // Ideal averaging filter width 
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

void boxBlurH_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r) {
    float iarr = 1.f / (r + r + 1);
    for (auto i = 0; i < h; i++) {
        auto ti = i * w, li = ti, ri = ti + r;
        auto fv = scl[ti], lv = scl[ti + w - 1];
        auto val = (r + 1) * fv;
        for (auto j = 0; j < r; j++) val += scl[ti + j];
        for (auto j = 0; j <= r; j++) { val += scl[ri++] - fv;   tcl[ti++] = round(val * iarr); }
        for (auto j = r + 1; j < w - r; j++) { val += scl[ri++] - scl[li++];   tcl[ti++] = round(val * iarr); }
        for (auto j = w - r; j < w; j++) { val += lv - scl[li++];   tcl[ti++] = round(val * iarr); }
    }
}

void boxBlurT_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r) {
    float iarr = 1.f / (r + r + 1);
    for (auto i = 0; i < w; i++) {
        auto ti = i, li = ti, ri = ti + r * w;
        auto fv = scl[ti], lv = scl[ti + w * (h - 1)];
        auto val = (r + 1) * fv;
        for (auto j = 0; j < r; j++) val += scl[ti + j * w];
        for (auto j = 0; j <= r; j++) { val += scl[ri] - fv;  tcl[ti] = round(val * iarr);  ri += w; ti += w; }
        for (auto j = r + 1; j < h - r; j++) { val += scl[ri] - scl[li];  tcl[ti] = round(val * iarr);  li += w; ri += w; ti += w; }
        for (auto j = h - r; j < h; j++) { val += lv - scl[li];  tcl[ti] = round(val * iarr);  li += w; ti += w; }
    }
}

void boxBlur_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r) {
    for (auto i = 0; i < scl.size(); i++) tcl[i] = scl[i];
    boxBlurH_4(tcl, scl, w, h, r);
    boxBlurT_4(scl, tcl, w, h, r);
}

void gaussBlur_4(vector<BYTE>& scl, vector<BYTE>& tcl, int w, int h, int r) {
    auto bxs = boxesForGauss(r, 3);
    boxBlur_4(scl, tcl, w, h, (bxs[0] - 1) / 2);
    boxBlur_4(tcl, scl, w, h, (bxs[1] - 1) / 2);
    boxBlur_4(scl, tcl, w, h, (bxs[2] - 1) / 2);
}

vector<BYTE> Blur(vector<BYTE>& source, int w, int h, int radius)
{
    vector<BYTE> lowpass = source; // copy constructor
    vector<BYTE> target(source.size(), 0);
    gaussBlur_4(lowpass, target, w, h, radius);

    return target;
}

enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4 // Acrylic blur effect
};

struct ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    DWORD dwAttrib;
    PVOID pvData;
    SIZE_T cbData;
};

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

void ToggleAcrylicBlur(HWND hwnd, bool blur) {
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
            (pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_STATE as = blur ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_DISABLED;
            int blurcolor = 0x00000000;
            ACCENT_POLICY policy = { as, 0, blurcolor, 0 };
            WINDOWCOMPOSITIONATTRIBDATA data = { 19, &policy, sizeof(ACCENT_POLICY) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
    }
}