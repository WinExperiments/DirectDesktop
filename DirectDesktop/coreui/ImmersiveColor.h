#pragma once
#include <Windows.h>

HMODULE hUxtheme = LoadLibraryW(L"uxtheme.dll");

typedef enum IMMERSIVE_COLOR_TYPE
{
    // Defining only used ones
    IMCLR_SystemAccentLight2 = 2,
    IMCLR_SystemAccent = 4,
    IMCLR_SystemAccentDark1 = 5,
    IMCLR_StartDesktopTilesText = 11,
} IMMERSIVE_COLOR_TYPE;

typedef struct IMMERSIVE_COLOR_PREFERENCE
{
    DWORD crStartColor;
    DWORD crAccentColor;
} IMMERSIVE_COLOR_PREFERENCE;

typedef enum IMMERSIVE_HC_CACHE_MODE
{
    IHCM_USE_CACHED_VALUE = 0,
    IHCM_REFRESH = 1
} IMMERSIVE_HC_CACHE_MODE;

typedef bool (*RefreshImmersiveColorPolicyState_t)(); // 104
RefreshImmersiveColorPolicyState_t RefreshImmersiveColorPolicyState = (RefreshImmersiveColorPolicyState_t)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));
typedef bool (*GetIsImmersiveColorUsingHighContrast_t)(IMMERSIVE_HC_CACHE_MODE); // 106
GetIsImmersiveColorUsingHighContrast_t GetIsImmersiveColorUsingHighContrast = (GetIsImmersiveColorUsingHighContrast_t)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106));;
typedef HRESULT (*GetUserColorPreference_t)(IMMERSIVE_COLOR_PREFERENCE*, bool); // 120
GetUserColorPreference_t GetUserColorPreference = (GetUserColorPreference_t)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(120));
typedef DWORD (*GetColorFromPreference_t)(const IMMERSIVE_COLOR_PREFERENCE*, IMMERSIVE_COLOR_TYPE, bool, IMMERSIVE_HC_CACHE_MODE); // 121
GetColorFromPreference_t GetColorFromPreference = (GetColorFromPreference_t)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(121));

class CImmersiveColor
{
public:
    static DWORD GetColor(IMMERSIVE_COLOR_TYPE colorType)
    {
        IMMERSIVE_COLOR_PREFERENCE icp;
        icp.crStartColor = 0;
        icp.crAccentColor = 0;
        GetUserColorPreference(&icp, false/*, true*/);
        return GetColorFromPreference(&icp, colorType, false, IHCM_REFRESH);
    }

    static bool IsColorSchemeChangeMessage(UINT uMsg, LPARAM lParam)
    {
        bool bRet = false;
        if (uMsg == WM_SETTINGCHANGE && lParam && CompareStringOrdinal((WCHAR*)lParam, -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
        {
            RefreshImmersiveColorPolicyState();
            bRet = true;
        }
        GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
        return bRet;
    }
};

class CImmersiveColorImpl
{
public:
    static HRESULT GetColorPreferenceImpl(IMMERSIVE_COLOR_PREFERENCE* pcpPreference, bool fForceReload, bool fUpdateCached)
    {
        return GetUserColorPreference(pcpPreference, fForceReload);
    }
};
