#pragma once
#include <vector>
#include <string>
#include "DirectUI/DirectUI.h"

using namespace std;

struct parameters {
    DirectUI::Element* elem{};
    int x{};
    int y{};
    wstring filename;
    wstring simplefilename;
    bool isDirectory = false;
    bool isHidden = false;
    bool mem_isSelected = false;
    bool isShortcut = false;
    unsigned short xPos = 999;
    unsigned short yPos = 999;
};

extern int logging;
extern vector<parameters> pm;
extern vector<DirectUI::Element*> shortpm;
extern vector<DirectUI::Element*> iconpm;
extern vector<DirectUI::Element*> shadowpm;
extern vector<DirectUI::RichText*> filepm;
extern vector<DirectUI::RichText*> fileshadowpm;
extern vector<DirectUI::Element*> cbpm;

int GetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
void SetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToSet, DWORD dwValue);
wchar_t* GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
BYTE* GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
void EnumerateFolder(LPWSTR path, vector<parameters>* pm, vector<wstring>* files, vector<wstring>* filepaths, bool bReset, unsigned short limit = 65535);
void GetPos();
bool ToggleDesktopIcons(bool visibility, bool wholeHost);
HWND GetWorkerW();
HWND GetWorkerW2(int* x, int* y);
bool PlaceDesktopInPos(int* WindowsBuild, HWND* hWndProgman, HWND* hWorkerW, HWND* hSHELLDLL_DefView, bool findSHELLDLL_DefView);