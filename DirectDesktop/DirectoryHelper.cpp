#include "DirectoryHelper.h"
#include "strsafe.h"
#include <shlobj.h>
#include <shlwapi.h>
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "shlwapi.lib")

using namespace std;
using namespace DirectUI;

struct DesktopItem {
    int index{};
    uint16_t size{};
    uint16_t flags{};
    uint32_t fileSize{};
    uint16_t fileAttr{};
    wstring name;
    int column = 0;
    int row = 0;
};

int logging;

wstring hideExt(const wstring& filename, bool isEnabled, vector<parameters>* shortpm, int* index) {
    if (isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            if (index != nullptr) (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            if (index != nullptr) (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find_last_of(L".");
        else {
            if (index != nullptr) (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (index != nullptr) (*shortpm)[(*index)++].isShortcut = false;
        if (lastdot == wstring::npos) return filename;
        return filename.substr(0, lastdot);
    }
    if (!isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            if (index != nullptr) (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            if (index != nullptr) (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) {
            if (index != nullptr) (*shortpm)[(*index)++].isShortcut = false;
            return filename;
        }
        else {
            if (index != nullptr) (*shortpm)[(*index)++].isShortcut = true;
            return filename.substr(0, lastdot);
        }
    }
}

int GetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind) {
    int result = -1;
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

void FindShellIcon(vector<parameters>* pm, vector<wstring>* files, vector<wstring>* filepaths, LPCWSTR clsid, LPCWSTR displayName, int* dirIndex, int* hiddenIndex, int* shortIndex, int* fileCount) {
    if (clsid == L"{645FF040-5081-101B-9F08-00AA002F954E}") {
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", clsid) == -1) {
            WCHAR clsidEx[64];
            StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
            pm->push_back({ NULL });
            files->push_back(displayName);
            filepaths->push_back(clsidEx);
            (*dirIndex)++;
            (*hiddenIndex)++;
            (*shortIndex)++;
            //if (logging == IDYES) {
            //    WCHAR totalItems[64];
            //    StringCchPrintfW(totalItems, 64, L"New item found (%d total)", ++(*fileCount));
            //    TaskDialog(NULL, NULL, L"Item Found", totalItems, clsidEx, TDCBF_OK_BUTTON, NULL, NULL);
            //}
            return;
        }
    }
    if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", clsid) == 0) {
        WCHAR clsidEx[64];
        StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
        pm->push_back({ NULL });
        files->push_back(displayName);
        filepaths->push_back(clsidEx);
        (*dirIndex)++;
        (*hiddenIndex)++;
        (*shortIndex)++;
        //if (logging == IDYES) {
        //    WCHAR totalItems[64];
        //    StringCchPrintfW(totalItems, 64, L"New item found (%d total)", ++(*fileCount));
        //    TaskDialog(NULL, NULL, L"Item Found", totalItems, clsidEx, TDCBF_OK_BUTTON, NULL, NULL);
        //}
    }
}

void EnumerateFolder(LPWSTR path, vector<parameters>* pm, vector<wstring>* files, vector<wstring>* filepaths, bool bReset, unsigned short limit) {
    if (!PathFileExistsW(path) && path != L"InternalCodeForNamespace") return;
    static int dirIndex{}, hiddenIndex{}, shortIndex{}, fileCount{};
    int runs = 0;
    if (bReset) dirIndex = 0, hiddenIndex = 0, shortIndex = 0, fileCount = 0;
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    if (path == L"InternalCodeForNamespace") {
        HINSTANCE WinStorageDLL = LoadLibraryW(L"windows.storage.dll");
        wchar_t* ThisPC = new wchar_t[260];
        ThisPC = GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}", NULL);
        if (ThisPC == NULL) {
            delete[] ThisPC;
            ThisPC = new wchar_t[260];
            LoadStringW(WinStorageDLL, 9216, ThisPC, 260);
        }
        wchar_t* RecycleBin = new wchar_t[260];
        RecycleBin = GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}", NULL);
        if (RecycleBin == NULL) {
            delete[] RecycleBin;
            RecycleBin = new wchar_t[260];
            LoadStringW(WinStorageDLL, 8964, RecycleBin, 260);
        }
        wchar_t* UserFiles = new wchar_t[260];
        DWORD d = GetEnvironmentVariableW(L"userprofile", UserFiles, 260);
        wstring UserFiless = UserFiles;
        UserFiless.erase(0, 9);
        wchar_t* ControlPanel = new wchar_t[260];
        LoadStringW(WinStorageDLL, 4161, ControlPanel, 260);
        wchar_t* Network = new wchar_t[260];
        LoadStringW(WinStorageDLL, 9217, Network, 260);
        FindShellIcon(pm, files, filepaths, L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", ThisPC, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, files, filepaths, L"{645FF040-5081-101B-9F08-00AA002F954E}", RecycleBin, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, files, filepaths, L"{59031A47-3F72-44A7-89C5-5595FE6B30EE}", UserFiless.c_str(), &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, files, filepaths, L"{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}", ControlPanel, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, files, filepaths, L"{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}", Network, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        delete[] ThisPC;
        delete[] RecycleBin;
        delete[] UserFiles;
        delete[] ControlPanel;
        delete[] Network;
        return;
    }
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
            //if (logging == IDYES) {
            //    WCHAR totalItems[64];
            //    WCHAR filePath[260];
            //    StringCchPrintfW(totalItems, 64, L"New item found (%d total)", ++fileCount);
            //    StringCchPrintfW(filePath, 260, L"%s\\%s", path, fd.cFileName);
            //    TaskDialog(NULL, NULL, L"Item Found", totalItems, filePath, TDCBF_OK_BUTTON, NULL, NULL);
            //}
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
int messageId = 1280;
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
HWND GetWorkerW2(int *x, int *y) {
    HWND hWorkerW = NULL;
    EnumWindows(EnumWindowsProc2, (LPARAM)&hWorkerW);
    return hWorkerW;
}

bool PlaceDesktopInPos(int* WindowsBuild, HWND* hWndProgman, HWND* hWorkerW, HWND* hSHELLDLL_DefView, bool findSHELLDLL_DefView) {
    int x = 0, y = 0;
    if (*WindowsBuild < 26016) *hWorkerW = GetWorkerW2(&x, &y); else *hWorkerW = FindWindowExW(*hWndProgman, NULL, L"WorkerW", NULL);
    if (hWorkerW) {
        if (findSHELLDLL_DefView) *hSHELLDLL_DefView = FindWindowExW(*hWorkerW, NULL, L"SHELLDLL_DefView", NULL);
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Found WorkerW.");
        if (*WindowsBuild > 26016) {
            SetParent(*hSHELLDLL_DefView, *hWorkerW);
            //SetParent(*hWorkerW, NULL);
            if (logging == IDYES) MainLogger.WriteLine(L"Information: Added DirectDesktop inside the new 24H2 WorkerW.");
        }
    }
    else if (logging == IDYES) TaskDialog(NULL, GetModuleHandleW(NULL), L"Error", NULL,
        L"No WorkerW found.", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
    return 1;
}

bool ToggleDesktopIcons(bool visibility, bool wholeHost) {
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    HWND hWndDesktop = NULL;
    if (hWndProgman) {
        hWndDesktop = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
        if (hWndDesktop && !wholeHost) {
            hWndDesktop = FindWindowExW(hWndDesktop, NULL, L"SysListView32", L"FolderView");
            if (hWndDesktop && logging == IDYES) TaskDialog(hWndDesktop, GetModuleHandleW(NULL), L"Information", NULL, L"Found SysListView32 inside Program Manager", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        }
    }
    if (!hWndDesktop) {
        HWND hWorkerW = GetWorkerW();
        if (hWorkerW) {
            hWndDesktop = FindWindowExW(hWorkerW, NULL, L"SHELLDLL_DefView", NULL);
            if (hWndDesktop && !wholeHost) {
                hWndDesktop = FindWindowExW(hWndDesktop, NULL, L"SysListView32", L"FolderView");
                if (hWndDesktop && logging == IDYES) TaskDialog(hWndDesktop, GetModuleHandleW(NULL), L"Information", NULL, L"Found SysListView32 inside WorkerW", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
            }
        }
    }
    if (!hWndDesktop) {
        HWND hWorkerW = FindWindowExW(hWndProgman, NULL, L"WorkerW", NULL);
        if (hWorkerW) hWndDesktop = FindWindowExW(hWorkerW, NULL, L"SHELLDLL_DefView", NULL);
        if (hWndDesktop && !wholeHost) {
            hWndDesktop = FindWindowExW(hWndDesktop, NULL, L"SysListView32", L"FolderView");
            if (hWndDesktop && logging == IDYES) TaskDialog(hWndDesktop, GetModuleHandleW(NULL), L"Information", NULL, L"Found SysListView32 inside WorkerW inside Program Manager", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        }
    }
    if (hWndDesktop) {
        ShowWindow(hWndDesktop, visibility ? SW_SHOW : SW_HIDE);
        return true;
    }
    else if (logging == IDYES) TaskDialog(hWndDesktop, GetModuleHandleW(NULL), L"Error", NULL, L"No SysListView32 found", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
    return false;
}

// A portion of this function (till parsing position tables) was sourced from
// https://stackoverflow.com/questions/70039190/how-to-read-the-values-of-iconlayouts-reg-binary-registry-file
// and "translated" to C++ using AI.

void GetPos() {
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    HINSTANCE WinStorageDLL = LoadLibraryW(L"windows.storage.dll");
    BYTE* value = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconLayouts");

    // Parse header at offset 0x10
    size_t offset = 0x10;
    std::vector<uint16_t> head;
    for (int i = 0; i < 4; ++i) {  // First 4 WORDs
        head.push_back(*reinterpret_cast<uint16_t*>(&value[offset + i * 2]));
    }
    head.push_back(*reinterpret_cast<uint32_t*>(&value[offset + 8]));  // DWORD at offset + 8

    offset += 12;

    // Start parsing desktop items
    vector<DesktopItem> desktop_items;
    for (uint32_t x = 0; x < pm.size(); ++x) {
        DesktopItem item;
        item.index = x;
        item.size = *reinterpret_cast<uint16_t*>(&value[offset]);
        item.flags = *reinterpret_cast<uint16_t*>(&value[offset + 2]);
        item.fileSize = *reinterpret_cast<uint32_t*>(&value[offset + 4]);
        item.fileAttr = *reinterpret_cast<uint16_t*>(&value[offset + 12]);

        offset += 12;

        // Parse names
        size_t name_len = (item.fileSize * 2) - 8;
        item.name = hideExt(wstring(
            reinterpret_cast<const wchar_t*>(&value[offset]),
            name_len / sizeof(wchar_t) 
        ), isFileExtHidden, nullptr, nullptr);
        wchar_t* nameBuffer = new wchar_t[260];
        if (item.name == L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}") {
            nameBuffer = GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}", NULL);
            if (nameBuffer == NULL) {
                delete[] nameBuffer;
                nameBuffer = new wchar_t[260];
                LoadStringW(WinStorageDLL, 9216, nameBuffer, 260);
            }
            item.name = nameBuffer;
        }
        if (item.name == L"::{645FF040-5081-101B-9F08-00AA002F954E}") {
            nameBuffer = GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}", NULL);
            if (nameBuffer == NULL) {
                delete[] nameBuffer;
                nameBuffer = new wchar_t[260];
                LoadStringW(WinStorageDLL, 8964, nameBuffer, 260);
            }
            item.name = nameBuffer;
        }
        if (item.name == L"::{59031A47-3F72-44A7-89C5-5595FE6B30EE}") {
            DWORD d = GetEnvironmentVariableW(L"userprofile", nameBuffer, 260);
            wstring UserFiless = nameBuffer;
            UserFiless.erase(0, 9);
            item.name = UserFiless;
        }
        if (item.name == L"::{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}") {
            LoadStringW(WinStorageDLL, 4161, nameBuffer, 260);
            item.name = nameBuffer;
        }
        if (item.name == L"::{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}") {
            LoadStringW(WinStorageDLL, 9217, nameBuffer, 260);
            item.name = nameBuffer;
        }
        delete[] nameBuffer;

        offset += name_len + 4;

        desktop_items.push_back(item);
    }

    // Parse head2 (64 bytes)
    vector<uint16_t> head2;
    size_t offs = offset;
    for (int x = 0; x < 32; ++x) {
        head2.push_back(*reinterpret_cast<uint16_t*>(&value[offs + x * 2]));
    }
    offs += 64;

    // Parse position tables
    for (uint32_t x = 0; x < pm.size(); ++x) {
        uint16_t column = *reinterpret_cast<uint16_t*>(&value[offs + 2]);
        uint16_t row = *reinterpret_cast<uint16_t*>(&value[offs + 6]);
        uint16_t index = *reinterpret_cast<uint16_t*>(&value[offs + 8]);
        desktop_items[index].column = column;
        desktop_items[index].row = row;
        offs += 10;
    }

    // This is such a bad way to sort...
    vector<parameters> pmBuf;
    vector<Element*> pmShortcutBuf;
    vector<Element*> pmIconBuf;
    vector<Element*> pmIconShadowBuf;
    vector<RichText*> pmFileBuf;
    vector<RichText*> pmFileShadowBuf;
    vector<Element*> pmCBBuf;
    pmBuf.resize(pm.size());
    pmShortcutBuf.resize(pm.size());
    pmIconBuf.resize(pm.size());
    pmIconShadowBuf.resize(pm.size());
    pmFileBuf.resize(pm.size());
    pmFileShadowBuf.resize(pm.size());
    pmCBBuf.resize(pm.size());
    int fileCount{};
    for (int index = 0; index < pm.size(); index++) {
        for (int index2 = 0; index2 < pm.size(); index2++) {
            if (desktop_items[index2].name == pm[index].simplefilename) {
                desktop_items[index2].name = L"?";
                pmBuf[index2] = pm[index];
                pmShortcutBuf[index2] = shortpm[index];
                pmIconBuf[index2] = iconpm[index];
                pmIconShadowBuf[index2] = shadowpm[index];
                pmFileBuf[index2] = filepm[index];
                pmFileShadowBuf[index2] = fileshadowpm[index];
                pmCBBuf[index2] = cbpm[index];
                if (logging == IDYES) {
                    WCHAR details[320];
                    StringCchPrintfW(details, 320, L"New item found, Item name: %s", pm[index].filename.c_str());
                    MainLogger.WriteLine(details);
                    fileCount++;
                }
                break;
            }
        }
    }
    for (int index = 0; index < pm.size(); index++) {
        pm[index] = pmBuf[index];
        shortpm[index] = pmShortcutBuf[index];
        iconpm[index] = pmIconBuf[index];
        shadowpm[index] = pmIconShadowBuf[index];
        filepm[index] = pmFileBuf[index];
        fileshadowpm[index] = pmFileShadowBuf[index];
        cbpm[index] = pmCBBuf[index]; 
    }
    ////////////////////////////////////

    for (int index = 0; index < pm.size(); index++) {
        int r = desktop_items[index].row, c = desktop_items[index].column;
        short tempXPos{}, tempYPos{}, bitsX = 128, bitsY = 128, bitsXAccumulator{}, bitsYAccumulator{};

        while (true) {
            if (c == 0) {
                pm[index].xPos = tempXPos;
                break;
            }
            if (tempXPos == 0) {
                c -= 16256;
                tempXPos++;
                continue;
            }
            c -= bitsX;
            bitsXAccumulator += bitsX;
            if (bitsXAccumulator == 128) {
                bitsXAccumulator = 0;
                bitsX /= 2;
            }
            tempXPos++;
            if (tempXPos > 200) break;
        }
        while (true) {
            if (r == 0) {
                pm[index].yPos = tempYPos;
                if (logging == IDYES) {
                    WCHAR details[320];
                    StringCchPrintfW(details, 320, L"Item arranged (%d of %d)\nItem name: %s\nX: %d\nY: %d\n", index + 1, fileCount, pm[index].filename.c_str(), pm[index].xPos, pm[index].yPos);
                    MainLogger.WriteLine(details);
                }
                break;
            }
            if (tempYPos == 0) {
                r -= 16256;
                tempYPos++;
                continue;
            }
            r -= bitsY;
            bitsYAccumulator += bitsY;
            if (bitsYAccumulator == 128) {
                bitsYAccumulator = 0;
                bitsY /= 2;
            }
            tempYPos++;
            if (tempYPos > 200) break;
        }
    }
    pmBuf.clear();
}