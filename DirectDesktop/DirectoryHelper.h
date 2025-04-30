#pragma once
#include <vector>
#include <string>
#include "Logger.h"
#include "DDControls.h"
#include "DirectUI/DirectUI.h"

using namespace std;
extern Logger MainLogger;

extern int logging;
extern vector<LVItem*> pm, subpm;
extern vector<DirectUI::Element*> shortpm;
extern vector<DDScalableElement*> iconpm;
extern vector<DirectUI::Element*> shadowpm;
extern vector<DirectUI::RichText*> filepm;
extern vector<DirectUI::RichText*> fileshadowpm;
extern vector<DirectUI::Element*> cbpm;
extern void InitLayout(bool bUnused1, bool bUnused2);
extern wstring LoadStrFromRes(UINT id);
extern wstring LoadStrFromRes(UINT id, LPCWSTR dllName);

class ThumbIcons {
private:
	wstring _filename{};
	bool _isHidden = false;
	bool _colorLock = false;
public:
    wstring GetFilename();
    void SetFilename(const wstring& wsFilename);
    bool GetHiddenState();
    bool GetColorLock();
    void SetHiddenState(bool hiddenState);
    void SetColorLock(bool colorLockState);
};

int GetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
void SetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToSet, DWORD dwValue, bool find, bool* isNewValue);
wchar_t* GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
BYTE* GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind);
void EnumerateFolder(LPWSTR path, vector<LVItem*>* pm, bool bReset, bool bCountItems, unsigned short* countedItems = nullptr, int* count2 = nullptr, unsigned short limit = 65535);
void EnumerateFolderForThumbnails(LPWSTR path, vector<ThumbIcons>* strs, unsigned short limit);
void GetPos(bool getSpotlightIcon = false, int* setSpotlightIcon = nullptr);
HWND GetWorkerW();
HWND GetWorkerW2(int* x, int* y);
bool PlaceDesktopInPos(int* WindowsBuild, HWND* hWndProgman, HWND* hWorkerW, HWND* hSHELLDLL_DefView, bool findSHELLDLL_DefView);