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
    bool isDirectory;
    bool isHidden;
};

extern int shortIndex, subshortIndex, dirIndex, hiddenIndex, subhiddenIndex;
extern vector<parameters> pm, subpm;
extern vector<parameters> shortpm, subshortpm;
extern vector<wstring> listDirBuffer, sublistDirBuffer;

int GetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
wchar_t* GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
BYTE* GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
vector<wstring> list_directory();
vector<wstring> list_subdirectory(wstring path);
