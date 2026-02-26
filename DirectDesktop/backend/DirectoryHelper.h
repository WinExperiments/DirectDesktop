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
    extern HRESULT UpdateLVItem(const wstring& filepath, const wstring& filename, BYTE type); // types: 1: old, 2: new

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

    class CFileOperationProgressSink : public IFileOperationProgressSink
    {
    public:
        CFileOperationProgressSink() : _lRefCount(0), _destDir(nullptr), _prcDimensions(nullptr), _ppt(nullptr), _pPage(nullptr), _pszPending{} {}
        ~CFileOperationProgressSink();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT STDMETHODCALLTYPE StartOperations();
        HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT hrResult);
        HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName,
            HRESULT hrRename, IShellItem* psiNewlyCreated) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName);
        HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName,
            HRESULT hrMove, IShellItem* psiNewlyCreated);
        HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName);
        HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName,
            HRESULT hrCopy, IShellItem* psiNewlyCreated);
        HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD dwFlags, IShellItem* psiItem) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD dwFlags, IShellItem* psiItem,
            HRESULT hrDelete, IShellItem* psiNewlyCreated) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PreNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName,
            LPCWSTR pszTemplateName, DWORD dwFileAttributes, HRESULT hrNew, IShellItem* psiNewItem) { return S_OK; }
        HRESULT STDMETHODCALLTYPE UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar) { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE ResetTimer() { return S_OK; }
        HRESULT STDMETHODCALLTYPE PauseTimer() { return S_OK; }
        HRESULT STDMETHODCALLTYPE ResumeTimer() { return S_OK; }

        void SetDestinationDirectory(LPCWSTR pszDest);
        void InitDimensions(RECT* prcDimensions, POINTL* ppt, UINT* pPage);
        void PrepDimensions();

    private:
        LONG _lRefCount;
        LPWSTR _destDir;
        RECT* _prcDimensions;
        POINTL* _ppt;
        UINT* _pPage;
        vector<LPCWSTR> _pszPending;
        void _PreProcessItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName);
        void _PostProcessItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName,
                HRESULT hr, IShellItem* psiNewlyCreated);
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
