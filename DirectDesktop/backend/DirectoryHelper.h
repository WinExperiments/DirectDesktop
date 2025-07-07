#pragma once
#include <vector>
#include <string>
#include "Logger.h"
#include "..\ui\DDControls.h"
#include "..\Include\dui70\DirectUI\DirectUI.h"

using namespace std;

namespace DirectDesktop
{
    extern Logger MainLogger;

    extern bool g_touchmode;
    extern int logging;
    extern int localeType;
    extern int g_iconsz;
    extern vector<LVItem*> pm;
    extern vector<DirectUI::Element*> shortpm;
    extern vector<DDScalableElement*> iconpm;
    extern vector<DirectUI::Element*> shadowpm;
    extern vector<DirectUI::RichText*> filepm;
    extern vector<DirectUI::RichText*> fileshadowpm;
    extern vector<DirectUI::Element*> cbpm;
    extern void InitLayout(bool bUnused1, bool bUnused2, bool bAlreadyOpen);
    extern void InitNewLVItem(const wstring& filepath, const wstring& filename);
    extern void RemoveLVItem(const wstring& filepath, const wstring& filename);
    extern void UpdateLVItem(const wstring& filepath, const wstring& filename, BYTE type); // types: 1: old, 2: new

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

    extern vector<const wchar_t*> imageExts;
    wstring hideExt(const wstring& filename, bool isEnabled, bool dir, LVItem* shortpm);
    void isImage(const wstring& filename, bool bReset, const wchar_t* ext, bool* result);
    bool EnsureRegValueExists(HKEY hKeyName, LPCWSTR path, LPCWSTR valueToFind);
    int GetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName);
    void SetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, DWORD dwValue, bool find, bool* isNewValue);
    bool GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, WCHAR** outStr);
    bool GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE** outBytes);
    void SetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE* bValue, DWORD length, bool find, bool* isNewValue);
    wstring GetExplorerTooltipText(const wstring& filePath);
    void StartMonitorFileChanges(const wstring& path);
    void EnumerateFolder(LPWSTR path, vector<LVItem*>* pm, bool bCountItems, unsigned short* countedItems = nullptr, int* count2 = nullptr, unsigned short limit = 65535);
    void EnumerateFolderForThumbnails(LPWSTR path, vector<ThumbIcons>* strs, unsigned short limit);
    void GetPos(bool getSpotlightIcon, int* setSpotlightIcon);
    void GetPos2(bool full);
    void SetPos(bool full);
    HWND GetWorkerW();
    HWND GetWorkerW2(int* x, int* y);
    bool PlaceDesktopInPos(int* WindowsBuild, HWND* hWndProgman, HWND* hWorkerW, HWND* hSHELLDLL_DefView, bool findSHELLDLL_DefView);
}