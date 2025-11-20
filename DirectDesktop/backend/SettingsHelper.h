#pragma once

using namespace DirectUI;

namespace DirectDesktop
{
    extern bool g_debugmode;
    extern bool g_AnimShiftKey;
    extern bool g_debuginfo;
    extern bool g_enableexit;
    extern bool g_showcheckboxes;
    extern BYTE g_showHidden;
    extern BYTE g_showSuperHidden;
    extern BYTE g_hideFileExt;
    extern BYTE g_iconunderline;
    extern bool g_treatdirasgroup;
    extern bool g_tripleclickandhide;
    extern bool g_lockiconpos;
    extern bool g_isColorized;
    extern bool g_isColorizedOld;
    extern bool g_isDarkIconsEnabled;
    extern bool g_automaticDark;
    extern bool g_isGlass;
    extern bool g_isThumbnailHidden;
    extern BYTE iconColorID;
    extern COLORREF IconColorizationColor;
    extern bool g_atleastonesetting;
    extern int localeType;

    extern Element* UIContainer;
    extern NativeHWNDHost* wnd;
    extern NativeHWNDHost* subviewwnd;
    extern void RearrangeIcons(bool animation, bool reloadicons, bool bAlreadyOpen);
    extern void InitLayout(bool animation, bool fResetUIState, bool bAlreadyOpen);

    void ToggleSetting(Element* elem, Event* iev);
    POINT GetTopLeftMonitor();
    int GetRightMonitor();

    class RegKeyValue
    {
    public:
        RegKeyValue() noexcept
            : _hKeyName(nullptr)
            , _path()
            , _valueToFind()
            , _dwValue(0)
        {
        }

        RegKeyValue(HKEY hKey, const WCHAR* path, const WCHAR* valueToFind, DWORD dwValue) noexcept
            : _hKeyName(hKey)
            , _path(path ? path : L"")
            , _valueToFind(valueToFind ? valueToFind : L"")
            , _dwValue(dwValue)
        {
        }

        RegKeyValue(const RegKeyValue&) = default;
        RegKeyValue(RegKeyValue&&) = default;
        RegKeyValue& operator=(const RegKeyValue&) = default;
        RegKeyValue& operator=(RegKeyValue&&) = default;
        ~RegKeyValue() = default;

        HKEY GetHKeyName() const noexcept;
        const WCHAR* GetPath() const noexcept;
        const WCHAR* GetValueToFind() const noexcept;
        DWORD GetDwValue() const noexcept;
        void SetHKeyName(HKEY hKeyName) noexcept;
        void SetPath(const WCHAR* path) noexcept;
        void SetValueToFind(const WCHAR* valueToFind) noexcept;
        void SetValue(DWORD dwValue) noexcept;

    private:
        HKEY _hKeyName;
        const WCHAR* _path;
        const WCHAR* _valueToFind;
        DWORD _dwValue;
    };
}
