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
};

extern vector<parameters> pm, subpm;

int GetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
void SetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToSet, DWORD dwValue);
wchar_t* GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
BYTE* GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
void EnumerateFolder(LPWSTR path, vector<parameters>* pm, vector<wstring>* files, vector<wstring>* filepaths, bool bReset, unsigned short limit = 65535);