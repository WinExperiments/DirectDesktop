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

wstring hideExt(const wstring& filename, bool isEnabled, vector<LVItem*>* shortpm, int* index) {
    if (isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(true);
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(true);
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find_last_of(L".");
        else {
            if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(true);
            return filename.substr(0, lastdot);
        }
        if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(false);
        if (lastdot == wstring::npos) return filename;
        return filename.substr(0, lastdot);
    }
    if (!isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(true);
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(true);
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) {
            if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(false);
            return filename;
        }
        else {
            if (index != nullptr) (*shortpm)[(*index)++]->SetShortcutState(true);
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
        if (dwValue != nullptr) result = *dwValue;
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

static int checkSpotlight{};
void FindShellIcon(vector<LVItem*>* pm, LPCWSTR clsid, LPCWSTR displayName, int* dirIndex, int* hiddenIndex, int* shortIndex, int* fileCount) {
    if (clsid == L"{645FF040-5081-101B-9F08-00AA002F954E}") {
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", clsid) == -1) {
            WCHAR clsidEx[64];
            StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
            (*pm)[(*hiddenIndex)]->SetSimpleFilename(displayName);
            (*pm)[(*hiddenIndex)]->SetFilename(clsidEx);
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
    if (clsid == L"{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}") {
        GetPos(true, &checkSpotlight);
        if (checkSpotlight == 1) {
            WCHAR clsidEx[64];
            StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
            (*pm)[(*hiddenIndex)]->SetSimpleFilename(displayName);
            (*pm)[(*hiddenIndex)]->SetFilename(clsidEx);
            (*dirIndex)++;
            (*hiddenIndex)++;
            (*shortIndex)++;
        }
        return;
    }
    if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", clsid) == 0) {
        WCHAR clsidEx[64];
        StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
        (*pm)[(*hiddenIndex)]->SetSimpleFilename(displayName);
        (*pm)[(*hiddenIndex)]->SetFilename(clsidEx);
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

void EnumerateFolder(LPWSTR path, vector<LVItem*>* pm, bool bReset, bool bCountItems, unsigned short* countedItems, unsigned short limit) {
    if (!PathFileExistsW(path) && path != L"InternalCodeForNamespace") return;
    static int dirIndex{}, hiddenIndex{}, shortIndex{}, fileCount{};
    int runs = 0;
    if (bReset) dirIndex = 0, hiddenIndex = 0, shortIndex = 0, fileCount = 0;
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    if (path == L"InternalCodeForNamespace") {
        HINSTANCE WinStorageDLL = LoadLibraryW(L"windows.storage.dll");
        HINSTANCE Shell32DLL = LoadLibraryW(L"shell32.dll");
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
        wchar_t* LearnAbout = new wchar_t[260];
        LoadStringW(Shell32DLL, 51761, LearnAbout, 260);
        FindShellIcon(pm, L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", ThisPC, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, L"{645FF040-5081-101B-9F08-00AA002F954E}", RecycleBin, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, L"{59031A47-3F72-44A7-89C5-5595FE6B30EE}", UserFiless.c_str(), &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, L"{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}", ControlPanel, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, L"{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}", Network, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        FindShellIcon(pm, L"{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}", LearnAbout, &dirIndex, &hiddenIndex, &shortIndex, &fileCount);
        delete[] ThisPC;
        delete[] RecycleBin;
        delete[] UserFiles;
        delete[] ControlPanel;
        delete[] Network;
        delete[] LearnAbout;
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
            WIN32_FIND_DATAW fd;
            hr = SHGetDataFromIDListW(psfFolder, pidl, SHGDFIL_FINDDATA, &fd, sizeof(WIN32_FIND_DATAW));
            if (!bCountItems) {
                if (fd.dwFileAttributes & 16) (*pm)[dirIndex++]->SetDirState(true);
                else (*pm)[dirIndex++]->SetDirState(false);
                if (fd.dwFileAttributes & 2) (*pm)[hiddenIndex++]->SetHiddenState(true);
                else (*pm)[hiddenIndex++]->SetHiddenState(false);
                /*if ((*pm)[dirIndex - 1].isDirectory == true) {
                    files->push_back((wstring)fd.cFileName);
                    (*pm)[shortIndex++]->SetShortcutState(false);
                }
                else*/ (*pm)[hiddenIndex - 1]->SetSimpleFilename(hideExt((wstring)fd.cFileName, isFileExtHidden, pm, &shortIndex));
                (*pm)[hiddenIndex - 1]->SetFilename(path + (wstring)L"\\" + wstring(fd.cFileName));
            }
            if (isFileHiddenEnabled == 2 && fd.dwFileAttributes & 2) {
                if (!bCountItems) pm->pop_back();
                dirIndex--;
                hiddenIndex--;
                shortIndex--;
                pMalloc->Free(pidl);
                continue;
            }
            if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
                if (fd.dwFileAttributes & 4) {
                    if (!bCountItems) pm->pop_back();
                    dirIndex--;
                    hiddenIndex--;
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
    if (bCountItems) *countedItems = runs;
}

void EnumerateFolderForThumbnails(LPWSTR path, vector<wstring>* strs, unsigned short limit) {
    if (!PathFileExistsW(path)) return;
    int runs = 0;
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
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
            WIN32_FIND_DATAW fd;
            hr = SHGetDataFromIDListW(psfFolder, pidl, SHGDFIL_FINDDATA, &fd, sizeof(WIN32_FIND_DATAW));
            strs->push_back(path + (wstring)L"\\" + wstring(fd.cFileName));
            if (isFileHiddenEnabled == 2 && fd.dwFileAttributes & 2) {
                strs->pop_back();
                pMalloc->Free(pidl);
                continue;
            }
            if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
                if (fd.dwFileAttributes & 4) {
                    strs->pop_back();
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

// A portion of this function (till parsing position tables) was sourced from
// https://stackoverflow.com/questions/70039190/how-to-read-the-values-of-iconlayouts-reg-binary-registry-file
// and "translated" to C++ using AI.

void GetPos(bool getSpotlightIcon, int* setSpotlightIcon) {
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    HINSTANCE WinStorageDLL = LoadLibraryW(L"windows.storage.dll");
    HINSTANCE Shell32DLL = LoadLibraryW(L"shell32.dll");
    BYTE* value = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconLayouts");

    // Parse header at offset 0x10
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Preparing to parse icon layout registry at offset 0x10.");
    size_t offset = 0x10;
    vector<uint16_t> head;
    for (int i = 0; i < 4; ++i) {  // First 4 WORDs
        head.push_back(*reinterpret_cast<uint16_t*>(&value[offset + i * 2]));
    }
    head.push_back(*reinterpret_cast<uint32_t*>(&value[offset + 8]));  // DWORD at offset + 8

    uint32_t number_of_items = head[4];
    offset += 12;

    if (logging == IDYES) {
        WCHAR regReport[96];
        StringCchPrintfW(regReport, 96, L"\nInformation: Registry reports you have %d items.", number_of_items);
        MainLogger.WriteLine(regReport);
    }

    // Start parsing desktop items
    vector<DesktopItem> desktop_items;
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Preparing to parse desktop item names.");
    for (uint32_t x = 0; x < number_of_items; x++) {
        if (logging == IDYES) {
            WCHAR pReport[96];
            StringCchPrintfW(pReport, 96, L"\nInformation: Preparing to parse item %d of %d...", x + 1, number_of_items);
            MainLogger.WriteLine(pReport);
        }
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
        if (item.name == L"::{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}") {
            LoadStringW(Shell32DLL, 51761, nameBuffer, 260);
            item.name = nameBuffer;
            if (getSpotlightIcon) {
                if (logging == IDYES) MainLogger.WriteLine(L"Information: Found Windows Spotlight icon.");
                (*setSpotlightIcon) = 1;
                break;
            }
        }
        delete[] nameBuffer;
        offset += name_len + 4;

        if (!getSpotlightIcon) {
            desktop_items.push_back(item);
            if (logging == IDYES) {
                WCHAR details[320];
                StringCchPrintfW(details, 320, L"New item found, Item name: %s\nItem name to be shown on desktop: %s", pm[x]->GetFilename().c_str(), pm[x]->GetSimpleFilename().c_str());
                MainLogger.WriteLine(details);
            }
        }
    }
    if (getSpotlightIcon) return;

    // Parse head2 (64 bytes)
    vector<uint16_t> head2;
    size_t offs = offset;
    for (int x = 0; x < 32; x++) {
        head2.push_back(*reinterpret_cast<uint16_t*>(&value[offs + x * 2]));
    }
    offs += 64;
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Parsed head2.");

    // Parse position tables
    for (uint32_t x = 0; x < number_of_items; ++x) {
        uint16_t column = *reinterpret_cast<uint16_t*>(&value[offs + 2]);
        uint16_t row = *reinterpret_cast<uint16_t*>(&value[offs + 6]);
        uint16_t index = *reinterpret_cast<uint16_t*>(&value[offs + 8]);
        desktop_items[index].column = column;
        desktop_items[index].row = row;
        offs += 10;
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Parsed position table.");

    // This is such a bad way to sort...
    vector<LVItem*> pmBuf;
    vector<Element*> pmShortcutBuf;
    vector<Element*> pmIconBuf;
    vector<Element*> pmIconShadowBuf;
    vector<RichText*> pmFileBuf;
    vector<RichText*> pmFileShadowBuf;
    vector<Element*> pmCBBuf;
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Created temporary arrays to arrange icons.");
    pmBuf.resize(pm.size());
    pmShortcutBuf.resize(pm.size());
    pmIconBuf.resize(pm.size());
    pmIconShadowBuf.resize(pm.size());
    pmFileBuf.resize(pm.size());
    pmFileShadowBuf.resize(pm.size());
    pmCBBuf.resize(pm.size());
    vector<DesktopItem> new_desktop_items;
    for (int index = 0; index < number_of_items; index++) {
        for (int index2 = 0; index2 < pm.size(); index2++) {
            if (desktop_items[index].name == pm[index2]->GetSimpleFilename()) {
                new_desktop_items.push_back(desktop_items[index]);
                break;
            }
        }
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Resized the temporary arrays.");
    int fileCount{};
    for (int index = 0; index < pm.size(); index++) {
        for (int index2 = 0; index2 < new_desktop_items.size(); index2++) {
            if (new_desktop_items[index2].name == pm[index]->GetSimpleFilename()) {
                new_desktop_items[index2].name = L"?";
                pmBuf[index2] = pm[index];
                pmShortcutBuf[index2] = shortpm[index];
                pmIconBuf[index2] = iconpm[index];
                pmIconShadowBuf[index2] = shadowpm[index];
                pmFileBuf[index2] = filepm[index];
                pmFileShadowBuf[index2] = fileshadowpm[index];
                pmCBBuf[index2] = cbpm[index];
                fileCount++;
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
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Filled the temporary arrays.");
    ////////////////////////////////////

    for (int index = 0; index < fileCount; index++) {
        int r = new_desktop_items[index].row, c = new_desktop_items[index].column;
        short tempXPos{}, tempYPos{}, bitsX = 128, bitsY = 128, bitsXAccumulator{}, bitsYAccumulator{};
        if (logging == IDYES) {
            WCHAR details[320];
            StringCchPrintfW(details, 320, L"\nItem prepared for arrangement (%d of %d)\nItem name: %s\nX (encoded): %d, Y (encoded): %d", index + 1, fileCount, pm[index]->GetFilename().c_str(), desktop_items[index].column, desktop_items[index].row);
            MainLogger.WriteLine(details);
        }
        while (true) {
            if (c == 0) {
                pm[index]->SetInternalXPos(tempXPos);
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
                pm[index]->SetInternalYPos(tempYPos);
                if (logging == IDYES) {
                    WCHAR details[320];
                    StringCchPrintfW(details, 320, L"\nItem arranged (%d of %d)\nItem name: %s\nX: %d, Y: %d", index + 1, fileCount, pm[index]->GetFilename().c_str(), pm[index]->GetInternalXPos(), pm[index]->GetInternalYPos());
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
    validItems = fileCount;
    pmBuf.clear();
    pmShortcutBuf.clear();
    pmIconBuf.clear();
    pmIconShadowBuf.clear();
    pmFileBuf.clear();
    pmFileShadowBuf.clear();
    pmCBBuf.clear();
}