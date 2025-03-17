#include "SettingsHelper.h"
#include "DirectoryHelper.h"
#include <shlobj.h>

using namespace DirectUI;

bool showcheckboxes;
bool treatdirasgroup;
bool isColorized;
Element* UIContainer;

void ToggleCheckbox(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        showcheckboxes = !showcheckboxes;
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect", showcheckboxes);
        SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, NULL, NULL);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 500, NULL);
    }
}
void ToggleShowHidden(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden", (!elem->GetSelected() + 1));
        SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, NULL, NULL);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 500, NULL);
        Sleep(500);
        InitLayout();
    }
}
void ToggleFilenameExts(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt", elem->GetSelected());
        SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, NULL, NULL);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 500, NULL);
        InitLayout();
    }
}
void ToggleGroupMode(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        treatdirasgroup = !treatdirasgroup;
        elem->SetSelected(!elem->GetSelected());
        RearrangeIcons(false, true);
    }
}
void ToggleAccentIcons(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        isColorized = !isColorized;
        elem->SetSelected(!elem->GetSelected());
        RearrangeIcons(false, true);
    }
}
void ToggleThumbnails(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"IconsOnly", elem->GetSelected());
        SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, NULL, NULL);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 500, NULL);
        RearrangeIcons(false, true);
    }
}