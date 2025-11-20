#include "pch.h"

#include "SettingsHelper.h"
#include "..\DirectDesktop.h"

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    bool g_debugmode;
    bool g_AnimShiftKey;
    bool g_debuginfo;
    bool g_enableexit;
    bool g_showcheckboxes;
    BYTE g_showHidden;
    BYTE g_showSuperHidden;
    BYTE g_hideFileExt;
    BYTE g_iconunderline;
    bool g_treatdirasgroup;
    bool g_tripleclickandhide;
    bool g_lockiconpos;
    bool g_isColorized;
    bool g_isColorizedOld;
    bool g_isDarkIconsEnabled;
    bool g_automaticDark;
    bool g_isGlass;
    bool g_isThumbnailHidden;
    BYTE iconColorID;
    COLORREF IconColorizationColor;
    bool g_atleastonesetting{};
    Element* UIContainer;

    HKEY RegKeyValue::GetHKeyName() const noexcept
    {
        return _hKeyName;
    }

    const WCHAR* RegKeyValue::GetPath() const noexcept
    {
        return _path;
    }

    const WCHAR* RegKeyValue::GetValueToFind() const noexcept
    {
        return _valueToFind;
    }

    DWORD RegKeyValue::GetDwValue() const noexcept
    {
        return _dwValue;
    }

    void RegKeyValue::SetHKeyName(HKEY hKeyName) noexcept
    {
        _hKeyName = hKeyName;
    }

    void RegKeyValue::SetPath(const WCHAR* path) noexcept
    {
        _path = path;
    }

    void RegKeyValue::SetValueToFind(const WCHAR* valueToFind) noexcept
    {
        _valueToFind = valueToFind;
    }

    void RegKeyValue::SetValue(DWORD dwValue) noexcept
    {
        _dwValue = dwValue;
    }

    BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
    {
        vector<RECT>* monitors = reinterpret_cast<vector<RECT>*>(dwData);
        MONITORINFOEX monitorInfo{};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        if (GetMonitorInfoW(hMonitor, &monitorInfo))
        {
            RECT rcMonitor;
            rcMonitor = monitorInfo.rcMonitor;
            monitors->push_back(rcMonitor);
        }
        return TRUE;
    }

    POINT GetTopLeftMonitor()
    {
        vector<RECT> monitors{};
        POINT ptFinal{};
        EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
        sort(monitors.begin(), monitors.end(), [](const RECT& a, const RECT& b)
        {
            if (localeType != 1) return a.left < b.left;
            else return a.left > b.left;
        });
        ptFinal.x = monitors[0].left;
        sort(monitors.begin(), monitors.end(), [](const RECT& a, const RECT& b)
        {
            return a.top < b.top;
        });
        ptFinal.y = monitors[0].top;
        return ptFinal;
    }

    int GetRightMonitor()
    {
        vector<RECT> monitors{};
        int iFinal{};
        EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
        sort(monitors.begin(), monitors.end(), [](const RECT& a, const RECT& b)
        {
            return a.right > b.right;
        });
        iFinal = monitors[0].right;
        return iFinal;
    }

    DWORD WINAPI TempDisableToggle(LPVOID lpParam)
    {
        SendMessageW(subviewwnd->GetHWND(), WM_USER + 5, (WPARAM)lpParam, NULL);
        Sleep(700);
        SendMessageW(subviewwnd->GetHWND(), WM_USER + 5, (WPARAM)lpParam, 1);
        return 0;
    }

    void ToggleSetting(Element* elem, Event* iev)
    {
        if (elem->GetClassInfoW() == DDCheckBox::GetClassInfoPtr() || elem->GetClassInfoW() == DDToggleButton::GetClassInfoPtr())
        {
            if (iev->uidType == TouchButton::Click)
            {
                DDToggleButton* ddtb = (DDToggleButton*)elem;
                ddtb->SetCheckedState(!ddtb->GetCheckedState());
                void* associatedSetting = ddtb->GetAssociatedSetting();
                RegKeyValue rkv = ddtb->GetRegKeyValue();
                BYTE regSetter = ddtb->GetCheckedState();
                if (rkv.GetValueToFind() == L"Hidden") regSetter = (!ddtb->GetCheckedState() + 1);
                if (rkv.GetValueToFind() == L"Logging") regSetter = (!ddtb->GetCheckedState() + 6);
                if (rkv.GetValueToFind() == L"IconUnderline") regSetter = (!ddtb->GetCheckedState() + 2);
                if (rkv.GetHKeyName() != nullptr) SetRegistryValues(rkv.GetHKeyName(), rkv.GetPath(), rkv.GetValueToFind(), regSetter, false, nullptr);
                if (ddtb->GetAssociatedFn() != nullptr)
                    ddtb->ExecAssociatedFn(ddtb->GetAssociatedFn());
                if (ddtb->GetShellInteraction())
                {
                    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, nullptr, nullptr);
                    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 200, nullptr);
                }
                else if (associatedSetting) *(BYTE*)associatedSetting = regSetter;
                if (rkv.GetValueToFind() == L"Hidden")
                {
                    DWORD dwDisableToggle;
                    HANDLE DisableToggleHandle = CreateThread(nullptr, 0, TempDisableToggle, (LPVOID)elem, 0, &dwDisableToggle);
                    if (DisableToggleHandle) CloseHandle(DisableToggleHandle);
                    return;
                }
                g_atleastonesetting = true;
            }
        }
        if (elem->GetClassInfoW() == DDCombobox::GetClassInfoPtr())
        {
            if (iev->uidType == DDCombobox::SelectionChange)
            {
                DDCombobox* ddcmb = (DDCombobox*)elem;
                void* associatedSetting = ddcmb->GetAssociatedSetting();
                RegKeyValue rkv = ddcmb->GetRegKeyValue();
                BYTE regSetter = ddcmb->GetSelection();
                if (rkv.GetHKeyName() != nullptr) SetRegistryValues(rkv.GetHKeyName(), rkv.GetPath(), rkv.GetValueToFind(), regSetter, false, nullptr);
                if (ddcmb->GetAssociatedFn() != nullptr)
                    ddcmb->ExecAssociatedFn(ddcmb->GetAssociatedFn());
                if (ddcmb->GetShellInteraction())
                {
                    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, nullptr, nullptr);
                    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 200, nullptr);
                }
                else if (associatedSetting) *(BYTE*)associatedSetting = regSetter;
                g_atleastonesetting = true;
            }
        }
    }
}
