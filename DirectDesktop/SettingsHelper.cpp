#include "SettingsHelper.h"
#include "DirectoryHelper.h"

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
    }
}
void ToggleShowHidden(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden", (!elem->GetSelected() + 1));
    }
}
void ToggleFilenameExts(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt", elem->GetSelected());
    }
}
void ToggleGroupMode(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        treatdirasgroup = !treatdirasgroup;
        elem->SetSelected(!elem->GetSelected());
    }
}
void ToggleAccentIcons(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        isColorized = !isColorized;
        elem->SetSelected(!elem->GetSelected());
    }
}