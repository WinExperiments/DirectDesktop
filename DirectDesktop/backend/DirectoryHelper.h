#pragma once

#include "Logger.h"

#include "..\ui\DDControls.h"

using namespace std;

namespace DirectDesktop
{
    extern Logger MainLogger;

    extern bool g_touchmode;
    extern int logging;
    extern int localeType;
    extern int g_iconsz;
    extern vector<LVItem*> pm;
    extern void InitLayout(bool animation, bool fResetUIState, bool bAlreadyOpen);
    extern void InitNewLVItem(const wstring& filepath, const wstring& filename, POINTL* ppt, const UINT page);
    extern void RemoveLVItem(const wstring& filepath, const wstring& filename);
    extern void UpdateLVItem(const wstring& filepath, const wstring& filename, BYTE type); // types: 1: old, 2: new

    class ThumbIcons
    {
    public:
        wstring GetFilename();
        void SetFilename(const wstring& wsFilename);
        bool GetHiddenState();
        bool GetColorLock();
        bool GetHasAdvancedIcon();
        void SetHiddenState(bool hiddenState);
        void SetColorLock(bool colorLockState);
        void SetHasAdvancedIcon(bool hai);

    private:
        wstring _filename{};
        bool _isHidden = false;
        bool _colorLock = false;
        bool _hai = false;
    };

    extern vector<const wchar_t*> imageExts;
    extern vector<const wchar_t*> advancedIconExts;
    wstring hideExt(const wstring& filename, bool isEnabled, bool dir, LVItem* shortpm);
    void isSpecialProp(const wstring& filename, bool bReset, bool* result, vector<const wchar_t*>* exts);
    wstring GetExplorerTooltipText(const wstring& filePath);
    void StartMonitorFileChanges(const wstring& path);
    unsigned short EnumerateFolder_Helper(LPWSTR path);
    void EnumerateFolder(LPWSTR path, vector<LVItem*>* pm, int* count2 = nullptr, unsigned short limit = 65535);
    void EnumerateFolderForThumbnails(LPWSTR path, vector<ThumbIcons>* strs, unsigned short limit);
    void GetPos(bool getSpotlightIcon, int* setSpotlightIcon);
    void GetPos2(bool full);
    void SetPos(bool full);
    bool PlaceDesktopInPos(int* WindowsBuild, HWND* hWndProgman, HWND* hWorkerW, HWND* hSHELLDLL_DefView, bool findSHELLDLL_DefView);
}
