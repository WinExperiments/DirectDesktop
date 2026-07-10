#pragma once

using namespace DirectUI;

namespace DDUI
{
    struct DDUICtx
    {
        UINT AnimShiftKey;
        UINT debugmode : 1;
        UINT showcheckboxes : 1;
        UINT labelshadow : 1;
        UINT selectionrect : 1;
        UINT DWMActive : 1;
        UINT iconunderline : 2;
        UINT fontsmoothing : 2;
        UINT localeType : 2;
        UINT dpi : 10;
        UINT dpiOld : 10;
        UINT dpiLaunch : 10;
        UINT theme : 1;
        UINT themeOld : 1;
        UINT atleastonesetting : 1;
        BOOL windowAnim : 1;
        BOOL clientAnim : 1;
        BOOL comboAnim : 1;
        BOOL menuAnim : 1;
        BOOL tooltipAnim : 1;
        DWORD animCoef;
        float flScaleFactor;
    };

    DDUIAPI void ToggleSetting(Element* elem, Event* iev);
    DDUIAPI POINT GetTopLeftMonitor();
    DDUIAPI int GetRightMonitor();

    class DDUIAPI RegKeyValue
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
