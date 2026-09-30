#include "pch.h"
#include "InstallDD.h"
#include <stack>
#include <wrl.h>

#pragma comment(lib, "version.lib")

using namespace std;
using namespace DirectUI;
using namespace DDUI;

namespace DDInstaller
{
    AdvancedInstallOptions operator&(AdvancedInstallOptions lhs, AdvancedInstallOptions rhs)
    {
        return static_cast<AdvancedInstallOptions>(static_cast<DWORD>(lhs) & static_cast<DWORD>(rhs));
    }

    AdvancedInstallOptions operator|(AdvancedInstallOptions lhs, AdvancedInstallOptions rhs)
    {
        return static_cast<AdvancedInstallOptions>(static_cast<DWORD>(lhs) | static_cast<DWORD>(rhs));
    }

    NativeHWNDHost* wnd;
    HWNDElement* parent;
    DUIXmlParser* parser;
    Element* pMain;
    unsigned long key = 0;

    void SetTheme()
    {
        StyleSheet* sheet = pMain->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parser->GetSheet(g_pctx->theme ? L"default" : L"defaultdark", &sheetStorage);
        pMain->SetValue(Element::SheetProp, 1, sheetStorage);
    }

    HANDLE hMutex;
    constexpr LPCWSTR szWindowClass = L"DDINSTALLER";
    WNDPROC WndProc;

    DDUICtx* g_pctx = GetProcContext();
    DDUIColors* g_pColors = GetProcColors();
    InstallerFlags g_flag;
    AdvancedInstallOptions g_options;

    stack<LPWSTR> pages;
    Element* pageHost;
    DDScalableTouchButton* back, *next;
    Element* g_currentPage;
    WCHAR g_currentPageStr[64];
    WCHAR g_installStr[260];
    WCHAR g_installedStr[260] = L"C:\\Program Files\\DirectDesktop";
    WCHAR* g_ver;
    vector<LPWSTR> g_filenames;
    float g_flProgress;
    DDScalableRichText* progressCounter;
    void GoNext(Element* elem, Event* iev);
    void OnNewPage(Element* peOldPage, Element* peNewPage, LPCWSTR pszOld, LPCWSTR pszNew);

    void SetRegistryStrValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE* bValue, DWORD length, bool find, bool* isNewValue) // TODO: Merge with DDUI
    {
        int result{};
        DWORD dwSize{};
        HKEY hKey;
        LONG lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_SET_VALUE, &hKey);
        if (lResult == ERROR_FILE_NOT_FOUND)
        {
            lResult = RegCreateKeyExW(hKeyName, path, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
            if (lResult == ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_SZ, bValue, length);
                if (isNewValue != nullptr) *isNewValue = true;
            }
        }
        else if (lResult == ERROR_SUCCESS)
        {
            if (!find)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_SZ, bValue, length);
                if (isNewValue != nullptr) *isNewValue = false;
            }
            else if (!EnsureRegValueExists(hKeyName, path, valueName))
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_SZ, bValue, length);
                if (isNewValue != nullptr) *isNewValue = true;
            }
            else if (isNewValue != nullptr) *isNewValue = false;
        }
        RegCloseKey(hKey);
    }

    BOOL CALLBACK EnumWindowsProc2(HWND hwnd, LPARAM lParam)
    {
        WCHAR className[64];
        HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
        GetClassNameW(hwnd, className, sizeof(className) / 2);
        if (wcscmp(className, L"WorkerW") == 0)
        {
            DWORD pid = 0, pid2 = 0;
            DWORD threadId = GetWindowThreadProcessId(hWndProgman, &pid);
            DWORD threadId2 = GetWindowThreadProcessId(hwnd, &pid2);
            if (threadId == threadId2)
            {
                RECT dimensions;
                GetWindowRect(hwnd, &dimensions);
                int right = GetSystemMetrics(SM_CXSCREEN);
                int bottom = GetSystemMetrics(SM_CYSCREEN);
                if (dimensions.right >= right && dimensions.bottom >= bottom)
                {
                    *(HWND*)lParam = hwnd;
                    return 0;
                }
            }
        }
        return 1;
    }

    HWND GetWorkerW2()
    {
        HWND hWorkerW = nullptr;
        EnumWindows(EnumWindowsProc2, (LPARAM)&hWorkerW);
        return hWorkerW;
    }

    LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_CLOSE:
        {
            if ((wcscmp(g_currentPageStr, L"FinalizePage") == 0 || wcscmp(g_currentPageStr, L"FinishedPage") == 0) && wParam != 69)
                return 0;
            else if (wcscmp(g_currentPageStr, L"WelcomePage") == 0 || wParam == 69)
            {
            FULLEXIT:
                pMain->Destroy(true);
                StopMessagePump();
                break;
            }
            else
            {
                int response;
                TaskDialog(wnd->GetHWND(), HINST_THISCOMPONENT, L"Install DirectDesktop", L"Are you sure you want to exit?",
                    nullptr, TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, NULL, &response);
                if (response == IDYES)
                    goto FULLEXIT;
                return 0;
            }
        }
        case WM_USER + 1:
        {
            progressCounter->SetContentString(L"0%% complete");
            break;
        }
        case WM_USER + 2:
        {
            if (wParam != 0)
            {
                WCHAR pszProgress[48];
                float flProgressCoef = (g_flag == DDIF_MIXED) ? wParam * 100.0f : wParam * 200.0f;
                float flPercentage = lParam / flProgressCoef;
                if (g_flProgress >= 50.0f)
                    g_flProgress = 50.0f + flPercentage;
                else
                    g_flProgress = flPercentage;
                StringCchPrintfW(pszProgress, 48, L"%d%% complete", static_cast<int>(g_flProgress));
                progressCounter->SetContentString(pszProgress);
            }
            break;
        }
        case WM_USER + 3:
        {
            if (SUCCEEDED((HRESULT)wParam))
            {
                progressCounter->SetContentString(L"100%% complete");
                Event ev;
                ev.uidType = TouchButton::Click;
                GoNext(nullptr, &ev);
            }
            else
            {
                WCHAR pszProgress[48];
                WCHAR pszAction[32];
                if (g_flag == DDIF_INSTALL) StringCchCopyW(pszAction, 32, L"install");
                else if (g_flag == DDIF_UPDATE) StringCchCopyW(pszAction, 32, L"update");
                else if (g_flag == DDIF_UNINSTALL) StringCchCopyW(pszAction, 32, L"uninstall");
                StringCchPrintfW(pszProgress, 48, L"Failed to %s (error code 0x%X)", pszAction, static_cast<int>(wParam));
                progressCounter->SetContentString(pszProgress);
                back->SetEnabled(true);
            }
            break;
        }
        }
        return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
    }

    HRESULT STDMETHODCALLTYPE CInstallerFileOperationProgressSink::QueryInterface(REFIID riid, LPVOID* ppvObject)
    {
        if (riid == IID_IFileOperationProgressSink || riid == IID_IUnknown)
        {
            *ppvObject = static_cast<IFileOperationProgressSink*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE CInstallerFileOperationProgressSink::AddRef()
    {
        return InterlockedIncrement(&_lRefCount);
    }

    ULONG STDMETHODCALLTYPE CInstallerFileOperationProgressSink::Release()
    {
        LONG nCount;
        if ((nCount = InterlockedDecrement(&_lRefCount)) == 0)
            delete this;
        return nCount;
    }

    HRESULT STDMETHODCALLTYPE CInstallerFileOperationProgressSink::StartOperations()
    {
        SendMessageW(wnd->GetHWND(), WM_USER + 1, NULL, NULL);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CInstallerFileOperationProgressSink::FinishOperations(HRESULT hrResult)
    {
        SendMessageW(wnd->GetHWND(), WM_USER + 3, hrResult, NULL);
        return hrResult;
    }

    HRESULT STDMETHODCALLTYPE CInstallerFileOperationProgressSink::UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar)
    {
        SendMessageW(wnd->GetHWND(), WM_USER + 2, iWorkTotal, iWorkSoFar);
        return S_OK;
    }

    wstring GetExeVersion()
    {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);

        DWORD size = GetFileVersionInfoSizeW(path, nullptr);
        if (size == 0) return L"";

        vector<BYTE> buffer(size);
        if (!GetFileVersionInfoW(path, 0, size, buffer.data()))
            return L"";

        VS_FIXEDFILEINFO* pFileInfo = nullptr;
        UINT len = 0;
        if (VerQueryValueW(buffer.data(), L"\\", (LPVOID*)&pFileInfo, &len)) {
            int edition = HIWORD(pFileInfo->dwFileVersionMS);
            int major = LOWORD(pFileInfo->dwFileVersionMS);
            int minor = HIWORD(pFileInfo->dwFileVersionLS);
            int rev = LOWORD(pFileInfo->dwFileVersionLS);

            WCHAR ver[16];
            StringCchPrintfW(ver, 16, L"%d.%d.%d.%d", edition, major, minor, rev);
            return ver;
        }
        return L"";
    }

    wstring GetExeCopyright()
    {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);

        DWORD size = GetFileVersionInfoSizeW(path, nullptr);
        if (size == 0) return L"";

        vector<BYTE> buffer(size);
        if (!GetFileVersionInfoW(path, 0, size, buffer.data()))
            return L"";

        WCHAR* pszCopyright = nullptr;
        UINT len = 0;
        if (VerQueryValueW(buffer.data(), L"\\StringFileInfo\\040904b0\\LegalCopyright", (LPVOID*)&pszCopyright, &len) && len > 0) {
            return (wstring)pszCopyright;
        }
        return L"";
    }

    bool CheckAppInstalled(LPWSTR pszVer, LPWSTR pszPath)
    {
        HKEY hKey = NULL;
        LONG lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{4E01A297-F7C3-4AB1-935A-85D30DE0FA25}",
            0, KEY_READ | KEY_WOW64_64KEY, &hKey);
        if (lStatus != ERROR_SUCCESS)
            return false;

        bool result = true;
        WCHAR pszVerBuf[32]{};
        DWORD dwBufferSize = sizeof(pszVerBuf);
        if (RegQueryValueExW(hKey, L"DisplayVersion", NULL, NULL, reinterpret_cast<BYTE*>(pszVerBuf), &dwBufferSize) == ERROR_SUCCESS)
            StringCchCopyW(pszVer, 32, pszVerBuf);
        else result = false;

        WCHAR pszPathBuf[260]{};
        dwBufferSize = sizeof(pszPathBuf);
        if (RegQueryValueExW(hKey, L"InstallLocation", NULL, NULL, reinterpret_cast<BYTE*>(pszPathBuf), &dwBufferSize) == ERROR_SUCCESS)
            StringCchCopyW(pszPath, 260, pszPathBuf);
        else result = false;

        RegCloseKey(hKey);
        return result;
    }

    void PageAnimation(Element* peFrom, Element* peTo, short direction)
    {
        GTRANS_DESC desc[2];
        TransitionStoryboardInfo tsbInfo = {};

        TriggerFade(peFrom, desc, 0, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
        TriggerTranslate(peFrom, desc, 1, 0.0f, 0.25f, 0.11f, 0.5f, 0.24f, 0.96f, 0.0f, 0.0f, 100.0f * g_pctx->flScaleFactor * direction, 0.0f, false, true, true);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(desc), desc, peFrom->GetDisplayNode(), &tsbInfo);
        TriggerFade(peTo, desc, 0, 0.1f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, true);
        TriggerTranslate(peTo, desc, 1, 0.1f, 0.6f, 0.1f, 0.9f, 0.2f, 1.0f, -100.0f * g_pctx->flScaleFactor * direction, 0.0f, 0.0f, 0.0f, false, false, true);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(desc), desc, peTo->GetDisplayNode(), &tsbInfo);
    }

    HRESULT PerformCopyOrDeleteOp(LPCWSTR destDir, InstallerFlags flags)
    {
        if ((g_filenames.empty() && flags == DDIF_INSTALL) || !destDir || wcslen(destDir) == 0) return E_INVALIDARG;

        Microsoft::WRL::ComPtr<IShellItem> psiFolder; 
        Microsoft::WRL::ComPtr<CInstallerFileOperationProgressSink> pfops = Microsoft::WRL::Make<CInstallerFileOperationProgressSink>();
        HRESULT hr = SHCreateItemFromParsingName(destDir, nullptr, IID_PPV_ARGS(&psiFolder));
        if (SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<IFileOperation> pfo;
            hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfo));
            if (SUCCEEDED(hr))
            {
                pfo->SetOperationFlags(FOF_NO_UI);
                DWORD dwCookie = 0;
                pfo->Advise(pfops.Get(), &dwCookie);
                for (UINT i = 0; i < g_filenames.size(); i++)
                {
                    Microsoft::WRL::ComPtr<IShellItem> psiSrc;
                    hr = SHCreateItemFromParsingName(g_filenames[i], nullptr, IID_PPV_ARGS(&psiSrc));
                    if (SUCCEEDED(hr))
                    {
                        if (flags == DDIF_INSTALL)
                            hr = pfo->CopyItem(psiSrc.Get(), psiFolder.Get(), nullptr, nullptr);
                        else if (flags == DDIF_UNINSTALL)
                            hr = pfo->DeleteItem(psiSrc.Get(), nullptr);
                    }
                }
                pfo->PerformOperations();
                pfo->Unadvise(dwCookie);
            }
        }
        for (WCHAR* pszFilename : g_filenames)
            delete[] pszFilename;
        g_filenames.clear();
        return hr;
    }

    void CreateShortcutCore(LPCWSTR pszShortcutPath, LPCWSTR pszTargetPath)
    {
        Microsoft::WRL::ComPtr<IShellLinkW> psl;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
        if (SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<IPersistFile> ppf;
            psl->SetPath(pszTargetPath);
            hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hr))
                hr = ppf->Save(pszShortcutPath, TRUE);
        }
    }

    void RegisterStartMenuShortcut()
    {
        LPWSTR basePath = NULL;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_CommonPrograms, 0, NULL, &basePath);
        if (SUCCEEDED(hr))
        {
            WCHAR path[260];
            StringCchPrintfW(path, 260, L"%s\\DirectDesktop", basePath);
            CoTaskMemFree(basePath);
            CreateDirectoryW(path, NULL);
            WCHAR pszShortcutPath[260];
            WCHAR pszInstalled[260];
            StringCchPrintfW(pszShortcutPath, 260, L"%s\\%s", path, L"Launch DirectDesktop.lnk");
            StringCchPrintfW(pszInstalled, 260, L"%s\\%s", g_installStr, L"LaunchDD.exe");
            CreateShortcutCore(pszShortcutPath, pszInstalled);
            StringCchPrintfW(pszShortcutPath, 260, L"%s\\%s", path, L"Uninstall DirectDesktop.lnk");
            StringCchPrintfW(pszInstalled, 260, L"%s\\%s", g_installStr, L"DDUninstall.exe");
            CreateShortcutCore(pszShortcutPath, pszInstalled);
        }
    }

    void RemoveStartMenuShortcut()
    {
        LPWSTR basePath = NULL;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_CommonPrograms, 0, NULL, &basePath);
        if (SUCCEEDED(hr))
        {
            WCHAR path[260];
            StringCchPrintfW(path, 260, L"%s\\DirectDesktop", basePath);
            CoTaskMemFree(basePath);
            WCHAR pszShortcutPath[260];
            StringCchPrintfW(pszShortcutPath, 260, L"%s\\%s", path, L"Launch DirectDesktop.lnk");
            DeleteFileW(pszShortcutPath);
            StringCchPrintfW(pszShortcutPath, 260, L"%s\\%s", path, L"Uninstall DirectDesktop.lnk");
            DeleteFileW(pszShortcutPath);
            RemoveDirectoryW(path);
        }
    }

    void RemoveUserRegKeys()
    {
        HKEY hKeyUsers{};
        LSTATUS result = RegOpenKeyExW(HKEY_USERS, nullptr, 0, KEY_READ | KEY_ENUMERATE_SUB_KEYS, &hKeyUsers);
        if (result != ERROR_SUCCESS)
            return;

        DWORD dwSubKeys = 0;
        RegQueryInfoKeyW(hKeyUsers, nullptr, nullptr, nullptr, &dwSubKeys, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

        for (DWORD i = 0; i < dwSubKeys; i++)
        {
            wchar_t sidName[256];
            DWORD dwSize = 256;
            result = RegEnumKeyExW(hKeyUsers, i, sidName, &dwSize, nullptr, nullptr, nullptr, nullptr);
            if (result == ERROR_SUCCESS)
            {
                if (wcsstr(sidName, L"S-1-5-18") || wcsstr(sidName, L"S-1-5-19") || wcsstr(sidName, L"S-1-5-20"))
                    continue;

                HKEY hKeyUser{};
                result = RegOpenKeyExW(HKEY_USERS, sidName, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKeyUser);
                if (result == ERROR_SUCCESS)
                {
                    SHDeleteKeyW(hKeyUser, L"Software\\DirectDesktop");
                    RegCloseKey(hKeyUser);
                }
            }
        }

        RegCloseKey(hKeyUsers);
    }

    HRESULT GetInstalledSize(LPCWSTR pszDir, DWORD* pdwTotal)
    {
        WIN32_FIND_DATAW fd;
        HANDLE hFind{};

        WCHAR pszDir2[260];
        StringCchPrintfW(pszDir2, 260, L"%s\\*", pszDir);
        hFind = FindFirstFileW(pszDir2, &fd);

        if (hFind == INVALID_HANDLE_VALUE)
            return E_INVALIDARG;

        do
        {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
            {
                WCHAR pszDir3[260];
                StringCchPrintfW(pszDir3, 260, L"%s\\%s", pszDir, fd.cFileName);
                GetInstalledSize(pszDir3, pdwTotal);
            }
            else
                *pdwTotal += ((fd.nFileSizeHigh & 0x000002FF) >> 10) + (fd.nFileSizeLow >> 10);
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
        return S_OK;
    }

    DWORD WINAPI CopyItemsThread(LPVOID lpParam)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        const WCHAR* pszRegPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{4E01A297-F7C3-4AB1-935A-85D30DE0FA25}";
        WCHAR pszExeDir[260];
        GetModuleFileNameW(NULL, pszExeDir, 260);
        int len = wcslen(pszExeDir);
        for (int i = len - 1; len >= 0; i--)
        {
            if (pszExeDir[i] == '\\')
            {
                pszExeDir[i] = '\0';
                break;
            }
        }
        g_filenames.push_back(new WCHAR[260]);
        StringCchPrintfW(g_filenames[0], 260, L"%s\\%s", pszExeDir, L"DirectDesktop.exe");
        g_filenames.push_back(new WCHAR[260]);
        StringCchPrintfW(g_filenames[1], 260, L"%s\\%s", pszExeDir, L"DDUI.dll");
        g_filenames.push_back(new WCHAR[260]);
        StringCchPrintfW(g_filenames[2], 260, L"%s\\%s", pszExeDir, L"LaunchDD.exe");
        g_filenames.push_back(new WCHAR[260]);
        StringCchPrintfW(g_filenames[3], 260, L"%s\\%s", pszExeDir, L"Everything64.dll");
        g_filenames.push_back(new WCHAR[260]);
        StringCchPrintfW(g_filenames[4], 260, L"%s\\%s", pszExeDir, L"DDUninstall.exe");
        if (g_flag == DDIF_UNINSTALL || g_flag == DDIF_MIXED)
        {
            PerformCopyOrDeleteOp(g_installedStr, DDIF_UNINSTALL);
            SHDeleteKeyW(HKEY_LOCAL_MACHINE, pszRegPath);
            RemoveStartMenuShortcut();
            if (g_options & AIO_RESET)
                RemoveUserRegKeys();
        }
        if (g_flag == DDIF_INSTALL || g_flag == DDIF_UPDATE || g_flag == DDIF_MIXED)
        {
            CreateDirectoryW(g_installStr, NULL);
            hr = PerformCopyOrDeleteOp(g_installStr, DDIF_INSTALL);
            if (SUCCEEDED(hr))
            {
                if (g_flag == DDIF_INSTALL || g_flag == DDIF_MIXED)
                {
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"DebugMode", (g_options & AIO_DEBUG) ? 1 : 0, true, nullptr);
                    WCHAR pszInstalled[260];
                    StringCchPrintfW(pszInstalled, 260, L"\"%s\\%s\"", g_installStr, L"DirectDesktop.exe");
                    SetRegistryStrValues(HKEY_LOCAL_MACHINE, pszRegPath, L"DisplayIcon", (BYTE*)pszInstalled, wcslen(pszInstalled) * 2 + 2, true, nullptr);
                    SetRegistryStrValues(HKEY_LOCAL_MACHINE, pszRegPath, L"DisplayName", (BYTE*)L"DirectDesktop", 28, true, nullptr);
                    WCHAR pszVer[32];
                    StringCchCopyW(pszVer, 32, GetExeVersion().c_str());
                    SetRegistryStrValues(HKEY_LOCAL_MACHINE, pszRegPath, L"DisplayVersion", (BYTE*)pszVer, wcslen(pszVer) * 2 + 2, true, nullptr);
                    DWORD dwEstimated = 0;
                    GetInstalledSize(g_installStr, &dwEstimated);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"EstimatedSize", dwEstimated, true, nullptr);
                    SetRegistryStrValues(HKEY_LOCAL_MACHINE, pszRegPath, L"HelpLink", (BYTE*)L"https://rectify11.com", 44, true, nullptr);
                    SYSTEMTIME time;
                    GetSystemTime(&time);
                    WCHAR pszInstallDate[12];
                    StringCchPrintfW(pszInstallDate, 12, L"%d%02d%02d", time.wYear, time.wMonth, time.wDay);
                    SetRegistryStrValues(HKEY_LOCAL_MACHINE, pszRegPath, L"InstallDate", (BYTE*)pszInstallDate, wcslen(pszInstallDate) * 2 + 2, true, nullptr);
                    SetRegistryStrValues(HKEY_LOCAL_MACHINE, pszRegPath, L"InstallLocation", (BYTE*)g_installStr, wcslen(g_installStr) * 2 + 2, true, nullptr);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"NoModify", 1, true, nullptr);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"NoRepair", 1, true, nullptr);
                    UINT ver1 = 0, ver2 = 0, ver3 = 0, ver4 = 0;
                    swscanf_s(pszVer, L"%d.%d.%d.%d", &ver1, &ver2, &ver3, &ver4);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"VersionMajor", ver1, true, nullptr);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"VersionMinor", ver2, true, nullptr);
                    if (g_options & AIO_SHORTCUT)
                        RegisterStartMenuShortcut();
                }
                if (g_flag == DDIF_UPDATE)
                {
                    WCHAR pszVer[32];
                    StringCchCopyW(pszVer, 32, GetExeVersion().c_str());
                    SetRegistryStrValues(HKEY_LOCAL_MACHINE, pszRegPath, L"DisplayVersion", (BYTE*)pszVer, wcslen(pszVer) * 2 + 2, true, nullptr);
                    DWORD dwEstimated = 0;
                    GetInstalledSize(g_installStr, &dwEstimated);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"EstimatedSize", dwEstimated, true, nullptr);
                    UINT ver1 = 0, ver2 = 0, ver3 = 0, ver4 = 0;
                    swscanf_s(pszVer, L"%d.%d.%d.%d", &ver1, &ver2, &ver3, &ver4);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"VersionMajor", ver1, true, nullptr);
                    SetRegistryValues(HKEY_LOCAL_MACHINE, pszRegPath, L"VersionMinor", ver2, true, nullptr);
                }
            }
        }
        CoUninitialize();
        return 0;
    }

    void OpenBrowseDialog(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click())
        {
            IFileOpenDialog* pfd = nullptr;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfd));
            if (SUCCEEDED(hr))
            {
                FILEOPENDIALOGOPTIONS fos;
                hr = pfd->GetOptions(&fos);
                pfd->SetOptions(fos | FOS_PICKFOLDERS);
                if (SUCCEEDED(pfd->Show(wnd->GetHWND())))
                {
                    IShellItem* psi = nullptr;
                    if (SUCCEEDED(pfd->GetResult(&psi)))
                    {
                        LPWSTR pszAbs;
                        psi->GetDisplayName(SIGDN_FILESYSPATH, &pszAbs);
                        DDScalableTouchEdit* installpath = (DDScalableTouchEdit*)regElem(L"installpath", elem->GetRoot());
                        HRESULT hr = installpath->SetContentString(pszAbs);
                        CoTaskMemFree(pszAbs);
                    }
                }
            }
        }
    }

    void ToggleInstallOption(Element* elem, Event* iev)
    {
        if (iev->uidType == DDCheckBox::Click)
        {
            ((DDCheckBox*)elem)->SetCheckedState(!((DDCheckBox*)elem)->GetCheckedState());
            ATOM id = elem->GetID();
            AdvancedInstallOptions newOption{};
            if (id == StrToID(L"cbShortcut"))
                newOption = AIO_SHORTCUT;
            if (id == StrToID(L"cbDebug"))
                newOption = AIO_DEBUG;
            if (id == StrToID(L"cbReset"))
                newOption = AIO_RESET;
            if (((DDCheckBox*)elem)->GetCheckedState())
                g_options = g_options | newOption;
            else
                g_options = g_options & (static_cast<AdvancedInstallOptions>(0xFFFFFFFF - newOption));
        }
    }

    void OpenReinstallView(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click())
        {
            g_flag = DDIF_MIXED;
            next->SetEnabled(true);
            Element* newPage;
            WCHAR* targetPage = new WCHAR[64]{};
            StringCchPrintfW(targetPage, 64, L"InstallOptionsPage");
            parser->CreateElement(targetPage, nullptr, nullptr, nullptr, &newPage);
            pageHost->Add(&newPage, 1);
            PageAnimation(g_currentPage, newPage, -1);
            StringCchPrintfW(g_currentPageStr, 64, targetPage);
            OnNewPage(g_currentPage, newPage, pages.top(), g_currentPageStr);
            pages.push(targetPage);
            g_currentPage = newPage;
        }
    }

    void CloseAndLaunch(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click())
        {
            SendMessageW(wnd->GetHWND(), WM_CLOSE, 69, NULL);
            if (g_flag != DDIF_UNINSTALL)
            {
                WCHAR pszInstalled[260];
                StringCchPrintfW(pszInstalled, 260, L"%s\\%s", g_installStr, L"LaunchDD.exe");
                STARTUPINFOW si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                ZeroMemory(&pi, sizeof(pi));
                CreateProcessW(nullptr, pszInstalled, nullptr, nullptr, FALSE, NULL, nullptr, g_installStr, &si, &pi);
            }
        }
    }

    void OnNewPage(Element* peOldPage, Element* peNewPage, LPCWSTR pszOld, LPCWSTR pszNew)
    {
        if (wcscmp(pszOld, L"InstallOptionsPage") == 0)
        {
            CValuePtr v;
            DDScalableTouchEdit* installpath = (DDScalableTouchEdit*)regElem(L"installpath", peOldPage);
            StringCchPrintfW(g_installStr, 260, L"%s", installpath->GetContentString(&v));
        }
        if (wcscmp(pszNew, L"InstallOptionsPage") == 0)
        {
            DDScalableTouchEdit* installpath = (DDScalableTouchEdit*)regElem(L"installpath", peNewPage);
            if (!wcslen(g_installStr))
            {
                LPWSTR basePath = NULL;
                HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &basePath);
                if (SUCCEEDED(hr))
                {
                    WCHAR path[260];
                    StringCchPrintfW(path, 260, L"%s\\DirectDesktop", basePath);
                    installpath->SetContentString(path);
                    CoTaskMemFree(basePath);
                }
            }
            else
                installpath->SetContentString(g_installStr);
            DDScalableTouchButton* browse = (DDScalableTouchButton*)regElem(L"browse", peNewPage);
            assignFn(browse, OpenBrowseDialog);
            DDCheckBox* cbShortcut = (DDCheckBox*)regElem(L"cbShortcut", peNewPage);
            DDCheckBox* cbDebug = (DDCheckBox*)regElem(L"cbDebug", peNewPage);
            assignFn(cbShortcut, ToggleInstallOption);
            assignFn(cbDebug, ToggleInstallOption);
            if (g_flag == DDIF_MIXED)
            {
                DDCheckBox* cbReset = (DDCheckBox*)regElem(L"cbReset", peNewPage);
                cbReset->GetParent()->SetVisible(true);
                assignFn(cbReset, ToggleInstallOption);
            }
        }
        if (wcscmp(pszNew, L"UpdatePage") == 0)
        {
            g_flag = DDIF_UPDATE;
            DDScalableTouchButton* reinstall = (DDScalableTouchButton*)regElem(L"reinstall", peNewPage);
            assignFn(reinstall, OpenReinstallView);
            DDScalableElement* UpdateStatusIcon = (DDScalableElement*)regElem(L"UpdateStatusIcon", peNewPage);
            DDScalableRichText* UpdateStatusText = (DDScalableRichText*)regElem(L"UpdateStatusText", peNewPage);
            ULONGLONG verOld1 = 0, verOld2 = 0, verOld3 = 0, verOld4 = 0;
            ULONGLONG verNew1 = 0, verNew2 = 0, verNew3 = 0, verNew4 = 0;
            swscanf_s(g_ver, L"%I64u.%I64u.%I64u.%I64u", &verOld1, &verOld2, &verOld3, &verOld4);
            swscanf_s(GetExeVersion().c_str(), L"%I64u.%I64u.%I64u.%I64u", &verNew1, &verNew2, &verNew3, &verNew4);
            bool noupdate = false;
            bool older = false;
            WCHAR pszStatus[64];
            if (((verOld1 << 48) + (verOld2 << 32) + (verOld3 << 16) + verOld4) >=
                ((verNew1 << 48) + (verNew2 << 32) + (verNew3 << 16) + verNew4))
                older = ((verOld1 << 48) + (verOld2 << 32) + (verOld3 << 16) + verOld4) >
                        ((verNew1 << 48) + (verNew2 << 32) + (verNew3 << 16) + verNew4);
            else
            {
                StringCchPrintfW(pszStatus, 64, L"Update available (%s)", GetExeVersion().c_str());
                UpdateStatusIcon->SetClass(L"DDNB_Icon_Info");
                noupdate = true;
            }
            if (!noupdate || older)
            {
                next->SetEnabled(false);
                StringCchPrintfW(pszStatus, 64, L"Up to date (%s)", g_ver);
                UpdateStatusIcon->SetClass(L"DDNB_Icon_Success");
            }
            UpdateStatusText->SetContentString(pszStatus);
        }
        if (wcscmp(pszNew, L"FinalizePage") == 0)
        {
            if (g_flag != DDIF_INSTALL)
            {
                WCHAR* WindowsBuildStr;
                GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
                int WindowsBuild = _wtoi(WindowsBuildStr);
                free(WindowsBuildStr);
                HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
                HWND hWorkerW{};
                if (WindowsBuild < 26002) hWorkerW = GetWorkerW2();
                else hWorkerW = FindWindowExW(hWndProgman, nullptr, L"WorkerW", nullptr);
                HWND hSHELLDLL_DefView = FindWindowExW(hWorkerW, nullptr, L"SHELLDLL_DefView", nullptr);
                if (!hSHELLDLL_DefView)
                    hSHELLDLL_DefView = FindWindowExW(hWndProgman, nullptr, L"SHELLDLL_DefView", nullptr);
                HWND hWndDD = FindWindowExW(hWndProgman, nullptr, L"DD_DesktopHost", L"DirectDesktop");
                if (!hWndDD)
                    hWndDD = FindWindowExW(hSHELLDLL_DefView, nullptr, L"DD_DesktopHost", L"DirectDesktop");
                if (hWndDD)
                    SendMessageTimeoutW(hWndDD, WM_CLOSE, NULL, 69, SMTO_ABORTIFHUNG, 3000, nullptr);
            }
            DDScalableRichText* title = (DDScalableRichText*)regElem(L"title", peNewPage);
            DDScalableRichText* subtitle = (DDScalableRichText*)regElem(L"subtitle", peNewPage);
            WCHAR pszSubtitle[96];
            WCHAR pszAction[32];
            switch (g_flag)
            {
            case DDIF_INSTALL:
                title->SetContentString(L"Installing...");
                StringCchCopyW(pszAction, 32, L"installed");
                break;
            case DDIF_UPDATE:
                title->SetContentString(L"Updating...");
                StringCchCopyW(pszAction, 32, L"updated");
                break;
            case DDIF_UNINSTALL:
                title->SetContentString(L"Uninstalling...");
                StringCchCopyW(pszAction, 32, L"uninstalled");
                break;
            case DDIF_MIXED:
                title->SetContentString(L"Reinstalling...");
                StringCchCopyW(pszAction, 32, L"reinstalled");
                break;
            }
            StringCchPrintfW(pszSubtitle, 96, L"Please wait a few seconds while DirectDesktop is being %s.", pszAction);
            subtitle->SetContentString(pszSubtitle);
            back->SetEnabled(false);
            next->SetEnabled(false);
            progressCounter = (DDScalableRichText*)regElem(L"progressCounter", peNewPage);
            HANDLE hCopyThread = CreateThread(nullptr, 0, CopyItemsThread, nullptr, NULL, nullptr);
            if (hCopyThread) CloseHandle(hCopyThread);
        }
        if (wcscmp(pszNew, L"FinishedPage") == 0)
        {
            DDScalableRichText* title = (DDScalableRichText*)regElem(L"title", peNewPage);
            switch (g_flag)
            {
            case DDIF_INSTALL:
            case DDIF_MIXED:
                title->SetContentString(L"Setup Complete");
                break;
            case DDIF_UPDATE:
                title->SetContentString(L"Update Complete");
                break;
            case DDIF_UNINSTALL:
                title->SetContentString(L"Uninstall Complete");
                break;
            }
            DDScalableTouchButton* finish = (DDScalableTouchButton*)regElem(L"finish", peNewPage);
            assignFn(finish, CloseAndLaunch);
        }
    }

    void GoBack(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            next->SetEnabled(true);
            Element* newPage;
            delete[] pages.top();
            pages.pop();
            parser->CreateElement(pages.top(), nullptr, nullptr, nullptr, &newPage);
            pageHost->Add(&newPage, 1);
            PageAnimation(g_currentPage, newPage, 1);
            OnNewPage(g_currentPage, newPage, g_currentPageStr, pages.top());
            g_currentPage = newPage;
            StringCchPrintfW(g_currentPageStr, 64, pages.top());
            if (pages.size() == 1)
                back->SetVisible(false);
        }
    }

    void GoNext(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            if (wcscmp(g_currentPageStr, L"FinishedPage") == 0)
            {
                return;
            }
            back->SetVisible(true);
            Element* newPage;
            WCHAR* targetPage = new WCHAR[64]{};
            if (wcscmp(g_currentPageStr, L"WelcomePage") == 0)
            {
                if (CheckAppInstalled(g_ver, g_installedStr))
                    StringCchPrintfW(targetPage, 64, L"UpdatePage");
                else
                    StringCchPrintfW(targetPage, 64, L"InstallOptionsPage");
            }
            if (wcscmp(g_currentPageStr, L"InstallOptionsPage") == 0)
            {
                StringCchPrintfW(targetPage, 64, L"FinalizePage");
            }
            if (wcscmp(g_currentPageStr, L"UpdatePage") == 0)
            {
                StringCchPrintfW(targetPage, 64, L"FinalizePage");
            }
            if (wcscmp(g_currentPageStr, L"FinalizePage") == 0)
            {
                StringCchPrintfW(targetPage, 64, L"FinishedPage");
            }
            if (wcscmp(g_currentPageStr, L"UninstallPage") == 0)
            {
                StringCchPrintfW(targetPage, 64, L"FinalizePage");
            }
            parser->CreateElement(targetPage, nullptr, nullptr, nullptr, &newPage);
            pageHost->Add(&newPage, 1);
            PageAnimation(g_currentPage, newPage, -1);
            StringCchPrintfW(g_currentPageStr, 64, targetPage);
            OnNewPage(g_currentPage, newPage, pages.top(), g_currentPageStr);
            pages.push(targetPage);
            g_currentPage = newPage;
        }
    }

    extern "C" int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPWSTR lpCmdLine,
        _In_ int nCmdShow)
    {
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        WCHAR* WindowsBuildStr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
        int WindowsBuild = _wtoi(WindowsBuildStr);
        free(WindowsBuildStr);
        WCHAR title[64], content[128];
        if (WindowsBuild < 18362)
        {
            WCHAR currentBuild[128];
            StringCchPrintfW(currentBuild, 128, L"Windows 10 version 1903 (OS Build 18362) or higher is required.\nCurrent build: %d", WindowsBuild);
            TaskDialog(nullptr, HINST_THISCOMPONENT, L"DirectDesktop", L"Unsupported Windows version", currentBuild, TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, nullptr);
            return 1;
        }
        hMutex = CreateMutex(nullptr, TRUE, szWindowClass);
        if (!hMutex || ERROR_ALREADY_EXISTS == GetLastError())
        {
            TaskDialog(nullptr, HINST_THISCOMPONENT, L"Error", nullptr, L"DirectDesktop Installer is already running.", TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, nullptr);
            return 1;
        }

        InitializeDDUI(HINST_THISCOMPONENT);
        RegisterAllControls();
        HRESULT hr = OleInitialize(NULL);

        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, g_pctx->dpi) + GetSystemMetricsForDpi(SM_CXEDGE, g_pctx->dpi) * 2) * 2;
        int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, g_pctx->dpi) + GetSystemMetricsForDpi(SM_CYEDGE, g_pctx->dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, g_pctx->dpi);
        InitialUpdateScale();

        DWORD dwExStyle = NULL, dwCreateFlags = 0x10;
        if (g_pctx->DWMActive)
        {
            dwExStyle |= WS_EX_NOINHERITLAYOUT;
            dwCreateFlags |= 0x28;
        }
        DUIXmlParser::Create(&parser, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        parser->SetXMLFromResource(IDR_UIFILE1, hInstance, HINST_THISCOMPONENT);
        int sizeX = 640 * g_pctx->flScaleFactor + windowsThemeX;
        int sizeY = 480 * g_pctx->flScaleFactor + windowsThemeY;
        NativeHWNDHost::Create(L"DDI_Main", L"Install DirectDesktop", nullptr, LoadIconW(HINST_THISCOMPONENT, MAKEINTRESOURCEW(IDI_INSTALLDD)),
            (dimensions.right - dimensions.left - sizeX) / 2, (dimensions.bottom - dimensions.top - sizeY) / 2, sizeX, sizeY, dwExStyle, WS_MINIMIZEBOX | WS_SYSMENU, HINST_THISCOMPONENT, 0x43, &wnd);
        HWNDElement::Create(wnd->GetHWND(), true, dwCreateFlags, nullptr, &key, (Element**)&parent);
        EnableMouseInPointer(TRUE);
        WndProc = (WNDPROC)SetWindowLongPtrW(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);

        parser->CreateElement(L"main", parent, nullptr, nullptr, &pMain);
        pMain->SetVisible(true);
        pMain->EndDefer(key);

        if (g_pctx->DWMActive)
        {
            AddLayeredRef(pMain->GetDisplayNode());
            SetGadgetFlags(pMain->GetDisplayNode(), NULL, NULL);
        }

        DDScalableRichText* verText = (DDScalableRichText*)regElem(L"verText", pMain);
        DDScalableRichText* copyrightText = (DDScalableRichText*)regElem(L"copyrightText", pMain);
        WCHAR info[256];
        StringCchPrintfW(info, 256, L"Version %s", GetExeVersion().c_str());
        verText->SetContentString(info);
        copyrightText->SetContentString(GetExeCopyright().c_str());

        pageHost = regElem(L"pageHost", pMain);
        back = (DDScalableTouchButton*)regElem(L"back", pMain);
        next = (DDScalableTouchButton*)regElem(L"next", pMain);
        assignFn(back, GoBack);
        assignFn(next, GoNext);

        WCHAR pszFirstPage[64];
        StringCchCopyW(pszFirstPage, 64, L"WelcomePage");
        g_flag = DDIF_INSTALL;
        if (argv)
        {
            for (int i = 1; i < argc; i++)
            {
                if ((wcscmp(argv[i], L"-uninstall") == 0 || wcscmp(argv[i], L"/uninstall") == 0))
                {
                    StringCchCopyW(pszFirstPage, 64, L"UninstallPage");
                    g_flag = DDIF_UNINSTALL;
                    g_options = g_options | AIO_RESET;
                }
            }
        }
        Element* page;
        parser->CreateElement(pszFirstPage, nullptr, nullptr, nullptr, &page);
        pageHost->Add(&page, 1);
        StringCchPrintfW(g_currentPageStr, 64, pszFirstPage);
        pages.push((LPWSTR)pszFirstPage);
        g_currentPage = page;

        wnd->Host(pMain);
        wnd->ShowWindow(SW_SHOW);
        SetTheme();

        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{4E01A297-F7C3-4AB1-935A-85D30DE0FA25}",
            L"DisplayVersion", &g_ver);

        StartMessagePump();
        UnInitProcess();
        CoUninitialize();

        return 0;
    }
}