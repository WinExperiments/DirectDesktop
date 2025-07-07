#include "SettingsHelper.h"
#include "DirectoryHelper.h"
#include <shlobj.h>
#include <vector>
#include <algorithm>

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    bool g_showcheckboxes;
    bool g_treatdirasgroup;
    bool g_tripleclickandhide;
    bool g_lockiconpos;
    bool g_isColorized;
    bool g_isDarkIconsEnabled;
    bool g_automaticDark;
    bool g_isGlass;
    BYTE iconColorID;
    COLORREF IconColorizationColor;
    bool g_atleastonesetting{};
    Element* UIContainer;

    BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
        vector<RECT>* monitors = reinterpret_cast<std::vector<RECT>*>(dwData);
        MONITORINFOEX monitorInfo{};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        if (GetMonitorInfoW(hMonitor, &monitorInfo)) {
            RECT rcMonitor;
            rcMonitor = monitorInfo.rcMonitor;
            monitors->push_back(rcMonitor);
        }
        return TRUE;
    }

    POINT GetTopLeftMonitor() {
        vector<RECT> monitors{};
        POINT ptFinal{};
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
        sort(monitors.begin(), monitors.end(), [](const RECT& a, const RECT& b) {
            if (localeType != 1) return a.left < b.left;
            else return a.left > b.left;
            });
        ptFinal.x = monitors[0].left;
        sort(monitors.begin(), monitors.end(), [](const RECT& a, const RECT& b) {
            return a.top < b.top;
            });
        ptFinal.y = monitors[0].top;
        return ptFinal;
    }
    int GetRightMonitor() {
        vector<RECT> monitors{};
        int iFinal{};
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
        sort(monitors.begin(), monitors.end(), [](const RECT& a, const RECT& b) {
            return a.right > b.right;
            });
        iFinal = monitors[0].right;
        return iFinal;
    }

    DWORD WINAPI TempDisableToggle(LPVOID lpParam) {
        SendMessageW(wnd->GetHWND(), WM_USER + 11, (WPARAM)lpParam, NULL);
        Sleep(700);
        SendMessageW(wnd->GetHWND(), WM_USER + 11, (WPARAM)lpParam, 1);
        return 0;
    }

    void ToggleSetting(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            DDToggleButton* ddtb = (DDToggleButton*)elem;
            ddtb->SetCheckedState(!ddtb->GetCheckedState());
            bool* associatedBool = ddtb->GetAssociatedBool();
            if (associatedBool != nullptr) *associatedBool = !(*associatedBool);
            RegKeyValue rkv = ddtb->GetRegKeyValue();
            BYTE regSetter = ddtb->GetCheckedState();
            if (rkv._valueToFind == L"Hidden") regSetter = (!ddtb->GetCheckedState() + 1);
            if (rkv._valueToFind == L"Logging") regSetter = (!ddtb->GetCheckedState() + 6);
            if (rkv._hKeyName != nullptr) SetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind, regSetter, false, nullptr);
            SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, NULL, NULL);
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 300, NULL);
            if (rkv._valueToFind == L"Hidden") {
                DWORD dwDisableToggle;
                HANDLE DisableToggleHandle = CreateThread(0, 0, TempDisableToggle, (LPVOID)elem, 0, &dwDisableToggle);
                return;
            }
            if (ddtb->GetAssociatedFn() != nullptr)
                ddtb->ExecAssociatedFn(ddtb->GetAssociatedFn(), false, true, true);
            g_atleastonesetting = true;
        }
    }
}