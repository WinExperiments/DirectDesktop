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
    bool g_treatdirasgroup;
    bool g_tripleclickandhide;
    bool g_lockiconpos;
    bool g_isColorized;
    bool g_isColorizedOld;
    bool g_isDarkIconsEnabled;
    bool g_automaticDark;
    bool g_isGlass;
    BYTE iconColorID;
    COLORREF IconColorizationColor;
    bool g_atleastonesetting{};
    Element* UIContainer;

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
        if (iev->uidType == Button::Click)
        {
            DDToggleButton* ddtb = (DDToggleButton*)elem;
            ddtb->SetCheckedState(!ddtb->GetCheckedState());
            bool* associatedBool = ddtb->GetAssociatedBool();
            if (associatedBool != nullptr) *associatedBool = !(*associatedBool);
            RegKeyValue rkv = ddtb->GetRegKeyValue();
            BYTE regSetter = ddtb->GetCheckedState();
            if (rkv._valueToFind == L"Hidden") regSetter = (!ddtb->GetCheckedState() + 1);
            if (rkv._valueToFind == L"Logging") regSetter = (!ddtb->GetCheckedState() + 6);
            if (rkv._hKeyName != nullptr) SetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind, regSetter, false, nullptr);
            if (ddtb->GetShellInteraction())
            {
                SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, nullptr, nullptr);
                SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 200, nullptr);
            }
            if (rkv._valueToFind == L"Hidden")
            {
                DWORD dwDisableToggle;
                HANDLE DisableToggleHandle = CreateThread(nullptr, 0, TempDisableToggle, (LPVOID)elem, 0, &dwDisableToggle);
                if (DisableToggleHandle) CloseHandle(DisableToggleHandle);
                return;
            }
            if (ddtb->GetAssociatedFn() != nullptr)
                ddtb->ExecAssociatedFn(ddtb->GetAssociatedFn());
            g_atleastonesetting = true;
        }
    }
}
