#include "SettingsHelper.h"
#include "DirectoryHelper.h"
#include <shlobj.h>
#include <vector>
#include <algorithm>

using namespace std;
using namespace DirectUI;

bool showcheckboxes;
bool treatdirasgroup;
bool tripleclickandhide;
bool isColorized;
bool atleastonesetting{};
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

unsigned long TempDisableToggle(LPVOID lpParam) {
    SendMessageW(wnd->GetHWND(), WM_USER + 11, (WPARAM)lpParam, NULL);
    Sleep(700);
    SendMessageW(wnd->GetHWND(), WM_USER + 11, (WPARAM)lpParam, 1);
    return 0;
}

void ToggleSetting(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        bool* associatedBool = ((DDScalableButton*)elem)->GetAssociatedBool();
        if (associatedBool != nullptr) *associatedBool = !(*associatedBool);
        RegKeyValue rkv = ((DDScalableButton*)elem)->GetRegKeyValue();
        BYTE regSetter = elem->GetSelected();
        if (rkv._valueToFind == L"Hidden") regSetter = (!elem->GetSelected() + 1);
        if (rkv._valueToFind == L"Logging") regSetter = (!elem->GetSelected() + 6);
        if (rkv._hKeyName != nullptr) SetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind, regSetter, false, nullptr);
        SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, NULL, NULL);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 300, NULL);
        if (rkv._valueToFind == L"Hidden") {
            DWORD dwDisableToggle;
            HANDLE DisableToggleHandle = CreateThread(0, 0, TempDisableToggle, (LPVOID)elem, 0, &dwDisableToggle);
            return;
        }
        if (((DDScalableButton*)elem)->GetAssociatedFn() != nullptr)
            ((DDScalableButton*)elem)->ExecAssociatedFn(((DDScalableButton*)elem)->GetAssociatedFn(), false, true);
        atleastonesetting = true;
    }
}