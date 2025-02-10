#include "SettingsHelper.h"
#include "DirectoryHelper.h"

using namespace DirectUI;

bool showcheckboxes;
bool treatdirasgroup;
bool isColorized;
Element* UIContainer;
Edit* chooseColor;

void ToggleCheckbox(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        showcheckboxes = !showcheckboxes;
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect", showcheckboxes);
    }
}
void ToggleShowHidden(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden", (!elem->GetSelected() + 1));
    }
}
void ToggleFilenameExts(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        elem->SetSelected(!elem->GetSelected());
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt", elem->GetSelected());
    }
}
void ToggleGroupMode(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        treatdirasgroup = !treatdirasgroup;
        elem->SetSelected(!elem->GetSelected());
    }
}
void ToggleAccentIcons(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        isColorized = !isColorized;
        elem->SetSelected(!elem->GetSelected());
    }
}
void ApplySelectedColor(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        Value* v;
        UCString colorID = chooseColor->GetContentString(&v);
        UIContainer->SetBackgroundStdColor(_wtoi((wchar_t*)colorID));
    }
}