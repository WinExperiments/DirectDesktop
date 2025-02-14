#include "DirectoryHelper.h"
#include "strsafe.h"
#include <shlobj.h>
#include <shlwapi.h>
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "shlwapi.lib")

using namespace std;

wstring hideExt(const wstring& filename, bool isEnabled, vector<parameters>* shortpm, int* index) {
    if (isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find_last_of(L".");
        else {
            (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        (*shortpm)[(*index)++].isShortcut = false;
        if (lastdot == wstring::npos) return filename;
        return filename.substr(0, lastdot);
    }
    if (!isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) {
            (*shortpm)[(*index)++].isShortcut = false;
            return filename;
        }
        else {
            (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
    }
}

int GetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind) {
    int result{};
    DWORD dwSize{};
    LONG lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_ANY, NULL, NULL, &dwSize);
    if (lResult == ERROR_SUCCESS) {
        DWORD* dwValue = (DWORD*)malloc(dwSize);
        lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_ANY, NULL, dwValue, &dwSize);
        result = *dwValue;
    }
    return result;
}
void SetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToSet, DWORD dwValue) {
    int result{};
    HKEY hKey;
    LONG lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_SET_VALUE, &hKey);
    if (lResult == ERROR_SUCCESS) {
        lResult = RegSetValueExW(hKey, valueToSet, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
    }
    RegCloseKey(hKey);
}

wchar_t* GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind) {
    wchar_t* result{};
    DWORD dwSize{};
    LONG lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_REG_SZ, NULL, NULL, &dwSize);
    if (lResult == ERROR_SUCCESS) {
        result = (wchar_t*)malloc(dwSize);
        lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_REG_SZ, NULL, result, &dwSize);
    }
    return result;
}

BYTE* GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind) {
    BYTE* result{};
    DWORD dwSize{};
    LONG lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_REG_BINARY, NULL, NULL, &dwSize);
    if (lResult == ERROR_SUCCESS) {
        result = (BYTE*)malloc(dwSize);
        lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_REG_BINARY, NULL, result, &dwSize);
    }
    return result;
}

void EnumerateFolder(LPWSTR path, vector<parameters>* pm, vector<wstring>* files, vector<wstring>* filepaths, bool bReset, unsigned short limit) {
    if (!PathFileExistsW(path)) return;
    static int dirIndex{}, hiddenIndex{}, shortIndex{};
    int runs = 0;
    if (bReset) dirIndex = 0, hiddenIndex = 0, shortIndex = 0;
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    HRESULT hr;

    LPMALLOC pMalloc = NULL;
    hr = SHGetMalloc(&pMalloc);

    LPSHELLFOLDER psfDesktop = NULL;
    hr = SHGetDesktopFolder(&psfDesktop);

    LPITEMIDLIST pidl = NULL;
    hr = psfDesktop->ParseDisplayName(NULL, NULL, path, NULL, &pidl, NULL);

    LPSHELLFOLDER psfFolder = NULL;
    hr = psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (void**)&psfFolder);
    psfDesktop->Release();
    pMalloc->Free(pidl);

    LPENUMIDLIST pEnumIDL = NULL;
    hr = psfFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN, &pEnumIDL);
    while (runs < limit) {
        if (pEnumIDL == nullptr) break;
        hr = pEnumIDL->Next(1, &pidl, NULL);
        if (hr == NOERROR) {
            WIN32_FIND_DATA fd;
            hr = SHGetDataFromIDListW(psfFolder, pidl, SHGDFIL_FINDDATA, &fd, sizeof(WIN32_FIND_DATA));
            pm->push_back({ NULL });
            files->push_back(hideExt((wstring)fd.cFileName, isFileExtHidden, pm, &shortIndex));
            filepaths->push_back(path + (wstring)L"\\" + wstring(fd.cFileName));
            if (fd.dwFileAttributes & 16) (*pm)[dirIndex++].isDirectory = true;
            else (*pm)[dirIndex++].isDirectory = false;
            if (fd.dwFileAttributes & 2) (*pm)[hiddenIndex++].isHidden = true;
            else (*pm)[hiddenIndex++].isHidden = false;
            if (isFileHiddenEnabled == 2 && fd.dwFileAttributes & 2) {
                files->pop_back();
                filepaths->pop_back();
                pm->pop_back();
                dirIndex--;
                shortIndex--;
                pMalloc->Free(pidl);
                continue;
            }
            if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
                if (fd.dwFileAttributes & 4) {
                    files->pop_back();
                    filepaths->pop_back();
                    pm->pop_back();
                    dirIndex--;
                    shortIndex--;
                    pMalloc->Free(pidl);
                    continue;
                }
            }
            pMalloc->Free(pidl);
        }
        else break;
        runs++;
    }

    if (pEnumIDL != nullptr) pEnumIDL->Release();
    psfFolder->Release();
    pMalloc->Release();
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    WCHAR className[64];
    GetClassNameW(hwnd, className, sizeof(className) / 2);
    if (wcscmp(className, L"WorkerW") == 0) {
        HWND hSHELLDLL_DefView = FindWindowExW(hwnd, NULL, L"SHELLDLL_DefView", NULL);
        if (hSHELLDLL_DefView) {
            *(HWND*)lParam = hwnd;
            return 0;
        }
    }
    return 1;
}
BOOL CALLBACK EnumWindowsProc2(HWND hwnd, LPARAM lParam) {
    WCHAR className[64];
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    SendMessageTimeoutW(hWndProgman, 0x052C, 0, 0, SMTO_NORMAL, 250, NULL);
    GetClassNameW(hwnd, className, sizeof(className) / 2);
    if (wcscmp(className, L"WorkerW") == 0) {
        DWORD pid = 0, pid2 = 0;
        DWORD threadId = GetWindowThreadProcessId(hWndProgman, &pid);
        DWORD threadId2 = GetWindowThreadProcessId(hwnd, &pid2);
        if (threadId == threadId2) {
            RECT dimensions;
            GetWindowRect(hwnd, &dimensions);
            int right = GetSystemMetrics(SM_CXSCREEN);
            int bottom = GetSystemMetrics(SM_CYSCREEN);
            if (dimensions.right == right && dimensions.bottom == bottom) {
                *(HWND*)lParam = hwnd;
                return 0;
            }
        }
    }
    return 1;
}

HWND GetWorkerW() {
    HWND hWorkerW = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&hWorkerW);
    return hWorkerW;
}
HWND GetWorkerW2() {
    HWND hWorkerW = NULL;
    EnumWindows(EnumWindowsProc2, (LPARAM)&hWorkerW);
    return hWorkerW;
}

bool ToggleDesktopIcons(bool visibility, bool wholeHost) {
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    HWND hWndDesktop = NULL;
    if (hWndProgman) {
        hWndDesktop = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
        if (hWndDesktop && !wholeHost) {
            hWndDesktop = FindWindowExW(hWndDesktop, NULL, L"SysListView32", L"FolderView");
            //if (hWndDesktop) TaskDialog(hWndDesktop, GetModuleHandleW(NULL), L"Information", NULL, L"Found SysListView32 inside Program Manager", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        }
    }
    if (!hWndDesktop) {
        HWND hWorkerW = GetWorkerW();
        if (hWorkerW) {
            hWndDesktop = FindWindowExW(hWorkerW, NULL, L"SHELLDLL_DefView", NULL);
            if (hWndDesktop && !wholeHost) {
                hWndDesktop = FindWindowExW(hWndDesktop, NULL, L"SysListView32", L"FolderView");
                //if (hWndDesktop) TaskDialog(hWndDesktop, GetModuleHandleW(NULL), L"Information", NULL, L"Found SysListView32 inside WorkerW", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
                //else TaskDialog(hWndDesktop, GetModuleHandleW(NULL), L"Error", NULL, L"No SysListView32 found", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
            }
        }
    }
    if (hWndDesktop) {
        ShowWindow(hWndDesktop, visibility ? SW_SHOW : SW_HIDE);
        return true;
    }
    return false;
}