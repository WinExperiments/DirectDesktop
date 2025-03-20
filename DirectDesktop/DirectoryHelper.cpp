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
wstring ThumbIcons::GetFilename() {
    return _filename;
}
void ThumbIcons::SetFilename(const wstring& wsFilename) {
    _filename = wsFilename;
}
bool ThumbIcons::GetHiddenState() {
    return _isHidden;
}
bool ThumbIcons::GetColorLock() {
    return _colorLock;
}
void ThumbIcons::SetHiddenState(bool hiddenState) {
    _isHidden = hiddenState;
}
void ThumbIcons::SetColorLock(bool colorLockState) {
    _colorLock = colorLockState;
}

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
vector<const wchar_t*> imageExts = { L".3gp", L".3gpp", L".ai", L".avi", L".avif", L".bmp", L".flv" L".gif", L".heic", L".heif", L".ico",
    L".jfif", L".jpe", L".jpeg", L".jpg", L".mp4", L".png", L".psd", L".svg", L".theme", L".tif", L".tiff", L".webm" L".webp", L".wma", L".wmv", L".xcf"};

int extIterator;
void isImage(const wstring& filename, bool bReset, const wchar_t* ext, bool* result) {
    if (bReset) extIterator = 0;
    size_t lastdot = filename.find(ext);
    if (extIterator == imageExts.size() - 1) {
        *result = false;
        return;
    }
    if (lastdot == wstring::npos) isImage(filename, false, imageExts[++extIterator], result);
    else {
        *result = true;
        return;
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
void SetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToSet, DWORD dwValue, bool find, bool* isNewValue) {
    int result{};
    DWORD dwSize{};
    HKEY hKey;
    LONG lResult = RegGetValueW(hKeyName, path, valueToSet, RRF_RT_ANY, NULL, NULL, &dwSize);
    lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_SET_VALUE, &hKey);
    if (lResult == ERROR_FILE_NOT_FOUND) {
        lResult = RegCreateKeyExW(hKeyName, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        if (lResult == ERROR_SUCCESS) {
            lResult = RegSetValueExW(hKey, valueToSet, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
            if (isNewValue != nullptr) *isNewValue = true;
        }
    }
    else if (lResult == ERROR_SUCCESS) {
        DWORD* dwValueInternal = (DWORD*)malloc(dwSize);
        lResult = RegGetValueW(hKeyName, path, valueToSet, RRF_RT_ANY, NULL, dwValueInternal, &dwSize);
        if (lResult == ERROR_SUCCESS && find == false) {
            lResult = RegSetValueExW(hKey, valueToSet, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
            if (isNewValue != nullptr) *isNewValue = false;
        }
        else if (lResult != ERROR_SUCCESS) {
            lResult = RegSetValueExW(hKey, valueToSet, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
            if (isNewValue != nullptr) *isNewValue = true;
        }
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
void FindShellIcon(vector<LVItem*>* pm, LPCWSTR clsid, LPCWSTR displayName, int* dirIndex, int* hiddenIndex, int* shortIndex, int* count2) {
    if (clsid == L"{645FF040-5081-101B-9F08-00AA002F954E}") {
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", clsid) == -1) {
            WCHAR clsidEx[64];
            StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
            (*pm)[(*count2)]->SetSimpleFilename(displayName);
            (*pm)[(*count2)]->SetFilename(clsidEx);
            (*dirIndex)++;
            (*hiddenIndex)++;
            (*shortIndex)++;
            (*count2)++;
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
            (*count2)++;
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
        (*count2)++;
        return;
    }
}

void EnumerateFolder(LPWSTR path, vector<LVItem*>* pm, bool bReset, bool bCountItems, unsigned short* countedItems, int* count2, unsigned short limit) {
    if (!PathFileExistsW(path) && path != L"InternalCodeForNamespace") {
        WCHAR details[320];
        StringCchPrintfW(details, 320, L"Error: Can't find %s.", path);
        MainLogger.WriteLine(details);
        return;
    }
    static int dirIndex{}, hiddenIndex{}, shortIndex{};
    int runs = 0;
    if (bReset) dirIndex = 0, hiddenIndex = 0, shortIndex = 0;
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    int isThumbnailHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"IconsOnly");
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
        FindShellIcon(pm, L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", ThisPC, &dirIndex, &hiddenIndex, &shortIndex, count2);
        FindShellIcon(pm, L"{645FF040-5081-101B-9F08-00AA002F954E}", RecycleBin, &dirIndex, &hiddenIndex, &shortIndex, count2);
        FindShellIcon(pm, L"{59031A47-3F72-44A7-89C5-5595FE6B30EE}", UserFiless.c_str(), &dirIndex, &hiddenIndex, &shortIndex, count2);
        FindShellIcon(pm, L"{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}", ControlPanel, &dirIndex, &hiddenIndex, &shortIndex, count2);
        FindShellIcon(pm, L"{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}", Network, &dirIndex, &hiddenIndex, &shortIndex, count2);
        FindShellIcon(pm, L"{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}", LearnAbout, &dirIndex, &hiddenIndex, &shortIndex, count2);
        delete[] ThisPC;
        delete[] RecycleBin;
        delete[] UserFiles;
        delete[] ControlPanel;
        delete[] Network;
        delete[] LearnAbout;
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Finished finding shell icons.");
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
    if(psfDesktop != nullptr) psfDesktop->Release();
    pMalloc->Free(pidl);

    LPENUMIDLIST pEnumIDL = NULL;
    hr = psfFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN, &pEnumIDL);
    while (runs < limit) {
        if (pEnumIDL == nullptr) break;
        hr = pEnumIDL->Next(1, &pidl, NULL);
        wstring foundsimplefilename{};
        wstring foundfilename{};
        if (hr == NOERROR) {
            WIN32_FIND_DATAW fd;
            hr = SHGetDataFromIDListW(psfFolder, pidl, SHGDFIL_FINDDATA, &fd, sizeof(WIN32_FIND_DATAW));
            if (!bCountItems) {
                if (count2 != nullptr) {
                    if (fd.dwFileAttributes & 16) (*pm)[*(count2)]->SetDirState(true);
                    else (*pm)[*(count2)]->SetDirState(false);
                    if (fd.dwFileAttributes & 2) (*pm)[*(count2)]->SetHiddenState(true);
                    else (*pm)[*(count2)]->SetHiddenState(false);
                    /*if ((*pm)[dirIndex - 1].isDirectory == true) {
                        files->push_back((wstring)fd.cFileName);
                        (*pm)[shortIndex++]->SetShortcutState(false);
                    }
                    else*/
                    foundsimplefilename = hideExt((wstring)fd.cFileName, isFileExtHidden, pm, &shortIndex);
                    foundfilename = (wstring)L"\"" + path + (wstring)L"\\" + wstring(fd.cFileName) + (wstring)L"\"";
                    if (isThumbnailHidden == 0) {
                        bool image;
                        isImage(foundfilename, true, imageExts[0], &image);
                        (*pm)[*(count2)]->SetColorLock(image);
                    }
                    (*pm)[*(count2)]->SetSimpleFilename(foundsimplefilename);
                    (*pm)[*(count2)]->SetFilename(foundfilename);
                }
                dirIndex++;
                hiddenIndex++;
            }
            if (isFileHiddenEnabled == 2 && fd.dwFileAttributes & 2) {
                dirIndex--;
                hiddenIndex--;
                shortIndex--;
                pMalloc->Free(pidl);
                continue;
            }
            if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
                if (fd.dwFileAttributes & 4) {
                    dirIndex--;
                    hiddenIndex--;
                    shortIndex--;
                    pMalloc->Free(pidl);
                    continue;
                }
            }
            pMalloc->Free(pidl);
        }
        else break;
        runs++;
        if (count2 != nullptr) {
            (*count2)++;
            if (logging == IDYES) {
                WCHAR details[320];
                StringCchPrintfW(details, 320, L"\nNew item added, Item name: %s\nItem name to be shown on desktop: %s", foundfilename.c_str(), foundsimplefilename.c_str());
                MainLogger.WriteLine(details);
            }
        }
    }

    if (pEnumIDL != nullptr) pEnumIDL->Release();
    if (psfFolder != nullptr) psfFolder->Release();
    if (pMalloc != nullptr) pMalloc->Release();
    //if (logging == IDYES) {
    //    WCHAR details[320];
    //    StringCchPrintfW(details, 320, L"Information: Finished searching in %s.", path);
    //    MainLogger.WriteLine(details);
    //}
    if (bCountItems) *countedItems = runs;
}

void EnumerateFolderForThumbnails(LPWSTR path, vector<ThumbIcons>* strs, unsigned short limit) {
    if (!PathFileExistsW(path)) return;
    int runs = 0;
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
    int isThumbnailHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"IconsOnly");
    HRESULT hr;

    LPMALLOC pMalloc = NULL;
    hr = SHGetMalloc(&pMalloc);

    LPSHELLFOLDER psfDesktop = NULL;
    hr = SHGetDesktopFolder(&psfDesktop);

    LPITEMIDLIST pidl = NULL;
    hr = psfDesktop->ParseDisplayName(NULL, NULL, path, NULL, &pidl, NULL);

    LPSHELLFOLDER psfFolder = NULL;
    hr = psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (void**)&psfFolder);
    if (psfDesktop != nullptr) psfDesktop->Release();
    pMalloc->Free(pidl);

    LPENUMIDLIST pEnumIDL = NULL;
    hr = psfFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN, &pEnumIDL);
    while (runs < limit) {
        if (pEnumIDL == nullptr) break;
        hr = pEnumIDL->Next(1, &pidl, NULL);
        if (hr == NOERROR) {
            WIN32_FIND_DATAW fd;
            hr = SHGetDataFromIDListW(psfFolder, pidl, SHGDFIL_FINDDATA, &fd, sizeof(WIN32_FIND_DATAW));
            strs->resize(runs + 1);
            wstring foundfilename = path + (wstring)L"\\" + wstring(fd.cFileName);
            (*strs)[runs].SetFilename(foundfilename);
            if (fd.dwFileAttributes & 2) (*strs)[runs].SetHiddenState(true);
            else (*strs)[runs].SetHiddenState(false);
            if (isThumbnailHidden == 0) {
                bool image;
                isImage(foundfilename, true, imageExts[0], &image);
                (*strs)[runs].SetColorLock(image);
            }
            if (isFileHiddenEnabled == 2 && fd.dwFileAttributes & 2) {
                pMalloc->Free(pidl);
                continue;
            }
            if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
                if (fd.dwFileAttributes & 4) {
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
    if (psfFolder != nullptr) psfFolder->Release();
    if (pMalloc != nullptr) pMalloc->Release();
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
            if (dimensions.right >= right && dimensions.bottom >= bottom) {
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
    HWND hWndResult{};
    if (*WindowsBuild < 26002) *hWorkerW = GetWorkerW2(&x, &y); else *hWorkerW = FindWindowExW(*hWndProgman, NULL, L"WorkerW", NULL);
    if (hWorkerW) {
        if (findSHELLDLL_DefView) *hSHELLDLL_DefView = FindWindowExW(*hWorkerW, NULL, L"SHELLDLL_DefView", NULL);
        if (logging == IDYES) {
            if (*hSHELLDLL_DefView != nullptr) MainLogger.WriteLine(L"Information: Found WorkerW.");
            else MainLogger.WriteLine(L"Error: No WorkerW found.");
        }
        if (*WindowsBuild >= 26002) {
            if (*hSHELLDLL_DefView != nullptr) hWndResult = SetParent(*hSHELLDLL_DefView, *hWorkerW);
            //SetParent(*hWorkerW, NULL);
            if (logging == IDYES) {
                if (hWndResult != nullptr) MainLogger.WriteLine(L"Information: Added DirectDesktop inside the new 24H2 WorkerW.");
                else MainLogger.WriteLine(L"Error: Could not add DirectDesktop inside the new 24H2 WorkerW.");
            }
        }
    }
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
    vector<DDScalableElement*> pmIconBuf;
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
        wstring itemToFind = pm[index]->GetSimpleFilename();
        for (int index2 = 0; index2 < new_desktop_items.size(); index2++) {
            if (new_desktop_items[index2].name == itemToFind) {
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