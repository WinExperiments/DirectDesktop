#pragma once
#include <vector>
#include <string>
#include "Logger.h"
#include "LVItem.h"
#include "DirectUI/DirectUI.h"

using namespace std;
extern Logger MainLogger;

extern int logging, validItems;
extern vector<LVItem*> pm, subpm;
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
void EnumerateFolder(LPWSTR path, vector<LVItem*>* pm, bool bReset, bool bCountItems, unsigned short* countedItems = nullptr, int* count2 = nullptr, unsigned short limit = 65535);
void EnumerateFolderForThumbnails(LPWSTR path, vector<wstring>* strs, unsigned short limit);
void GetPos(bool getSpotlightIcon = false, int* setSpotlightIcon = nullptr);
HWND GetWorkerW();
HWND GetWorkerW2(int* x, int* y);
bool PlaceDesktopInPos(int* WindowsBuild, HWND* hWndProgman, HWND* hWorkerW, HWND* hSHELLDLL_DefView, bool findSHELLDLL_DefView);