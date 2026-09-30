#pragma once

#include "resource.h"
#include <wrl.h>
#include "..\DDUI\DDUI.h"

namespace DDInstaller
{
	extern DDUI::DDUICtx* g_pctx;
	extern DDUI::DDUIColors* g_pColors;

    enum InstallerFlags
    {
        DDIF_INSTALL = 0,
        DDIF_UPDATE = 1,
        DDIF_UNINSTALL = 2,
        DDIF_MIXED = 3
    };

    enum AdvancedInstallOptions : DWORD
    {
        AIO_SHORTCUT = 0x1,
        AIO_DEBUG = 0x2,
        AIO_RESET = 0x4
    };

    class CInstallerFileOperationProgressSink : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IFileOperationProgressSink>
    {
    public:
        CInstallerFileOperationProgressSink() : _lRefCount(0) {}
        ~CInstallerFileOperationProgressSink() {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT STDMETHODCALLTYPE StartOperations();
        HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT hrResult);
        HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName,
            HRESULT hrRename, IShellItem* psiNewlyCreated) {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName,
            HRESULT hrMove, IShellItem* psiNewlyCreated) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName,
            HRESULT hrCopy, IShellItem* psiNewlyCreated) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD dwFlags, IShellItem* psiItem) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD dwFlags, IShellItem* psiItem,
            HRESULT hrDelete, IShellItem* psiNewlyCreated) {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE PreNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName) { return S_OK; }
        HRESULT STDMETHODCALLTYPE PostNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName,
            LPCWSTR pszTemplateName, DWORD dwFileAttributes, HRESULT hrNew, IShellItem* psiNewItem) {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar);
        HRESULT STDMETHODCALLTYPE ResetTimer() { return S_OK; }
        HRESULT STDMETHODCALLTYPE PauseTimer() { return S_OK; }
        HRESULT STDMETHODCALLTYPE ResumeTimer() { return S_OK; }

        void SetDestinationDirectory(LPCWSTR pszDest);
        void InitDimensions(RECT* prcDimensions, POINTL* ppt, UINT* pPage);
        void PrepDimensions();
        void SetTargetLVItem(DDUI::LVItem* lviTargetDir);

    private:
        LONG _lRefCount;
    };
}