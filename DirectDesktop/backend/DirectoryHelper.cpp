#include "pch.h"

#include "DirectoryHelper.h"
#include "SettingsHelper.h"
#include "..\DirectDesktop.h"
#include "..\coreui\StyleModifier.h"
#include <shlwapi.h>
#include <exdisp.h>
#include <ShlGuid.h>

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    struct DesktopItem
    {
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

    struct filestruct
    {
        wstring rawstr;
        DWORD attr;
    };

    vector<filestruct> filestructs;

    wstring ThumbIcons::GetFilename()
    {
        return _filename;
    }

    void ThumbIcons::SetFilename(const wstring& wsFilename)
    {
        _filename = wsFilename;
    }

    bool ThumbIcons::GetHiddenState()
    {
        return _isHidden;
    }

    bool ThumbIcons::GetColorLock()
    {
        return _colorLock;
    }

    bool ThumbIcons::GetHasAdvancedIcon()
    {
        return _hai;
    }

    void ThumbIcons::SetHiddenState(bool hiddenState)
    {
        _isHidden = hiddenState;
    }

    void ThumbIcons::SetColorLock(bool colorLockState)
    {
        _colorLock = colorLockState;
    }

    void ThumbIcons::SetHasAdvancedIcon(bool hai)
    {
        _hai = hai;
    }

    wstring hideExt(const wstring& filename, bool isEnabled, bool dir, LVItem* shortpm)
    {
        if (isEnabled)
        {
            if (dir)
            {
                return filename;
            }
            size_t lastdot = filename.rfind(L".lnk");
            if (lastdot == wstring::npos) lastdot = filename.rfind(L".pif");
            else
            {
                if (shortpm != nullptr) shortpm->AddFlags(LVIF_SHORTCUT);
                return filename.substr(0, lastdot);
            }
            if (lastdot == wstring::npos) lastdot = filename.rfind(L".url");
            else
            {
                if (shortpm != nullptr) shortpm->AddFlags(LVIF_SHORTCUT);
                return filename.substr(0, lastdot);
            }
            if (lastdot == wstring::npos) lastdot = filename.find_last_of(L".");
            else
            {
                if (shortpm != nullptr) shortpm->AddFlags(LVIF_SHORTCUT);
                return filename.substr(0, lastdot);
            }
            if (shortpm != nullptr) shortpm->RemoveFlags(LVIF_SHORTCUT);
            if (lastdot == wstring::npos) return filename;
            return filename.substr(0, lastdot);
        }
        if (!isEnabled)
        {
            size_t lastdot = filename.rfind(L".lnk");
            if (lastdot == wstring::npos) lastdot = filename.rfind(L".pif");
            else
            {
                if (shortpm != nullptr) shortpm->AddFlags(LVIF_SHORTCUT);
                return filename.substr(0, lastdot);
            }
            if (lastdot == wstring::npos) lastdot = filename.rfind(L".url");
            else
            {
                if (shortpm != nullptr) shortpm->AddFlags(LVIF_SHORTCUT);
                return filename.substr(0, lastdot);
            }
            if (lastdot == wstring::npos)
            {
                if (shortpm != nullptr) shortpm->RemoveFlags(LVIF_SHORTCUT);
                return filename;
            }
            else
            {
                if (shortpm != nullptr) shortpm->AddFlags(LVIF_SHORTCUT);
                return filename.substr(0, lastdot);
            }
        }
    }

    wstring hideExtFromGetPos(const wstring& filename, bool isEnabled)
    {
        for (int i = 0; i < filestructs.size(); i++)
        {
            if (filename == filestructs[i].rawstr)
            {
                return hideExt(filename, isEnabled, (filestructs[i].attr & 16), nullptr);
            }
        }
        return filename;
    }

    vector<const wchar_t*> imageExts = {
        L".3gp", L".3gpp", L".accountpicture-ms", L".ai", L".avi", L".avif", L".bmp", L".flv", L".gif", L".heic", L".heif", L".ico",
        L".jfif", L".jpe", L".jpeg", L".jpg", L".mov", L".mp4", L".pdn", L".png", L".psd", L".svg", L".theme", L".tif", L".tiff", L".webm", L".webp", L".wma", L".wmv", L".xcf"
    };

    vector<const wchar_t*> advancedIconExts = {
        L".msc", L".url"
    };

    void isSpecialProp(const wstring& filename, bool bReset, bool* result, vector<const wchar_t*>* exts)
    {
        wstring filename2 = filename;
        size_t lastdot = wstring::npos;
        if (filename2.length() > 1)
        {
            transform(filename2.begin(), filename2.end(), filename2.begin(), ::tolower);
            for (int i = 0; i < exts->size(); i++)
            {
                lastdot = filename2.find((*exts)[i]);
                if (lastdot != wstring::npos)
                {
                    *result = true;
                    return;
                }
            }
        }
        *result = false;
    }

    bool EnsureRegValueExists(HKEY hKeyName, LPCWSTR path, LPCWSTR valueToFind)
    {
        HKEY hKey = nullptr;
        LONG lResult;
        lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_READ, &hKey);
        if (lResult == ERROR_FILE_NOT_FOUND) return false;

        DWORD type;
        DWORD dataSize = 0;
        lResult = RegQueryValueExW(hKey, valueToFind, nullptr, &type, nullptr, &dataSize);
        RegCloseKey(hKey);

        if (lResult == ERROR_FILE_NOT_FOUND) return false;
        else if (lResult != ERROR_SUCCESS) return false;

        return true;
    }

    int GetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName)
    {
        int result = -1;
        DWORD dwSize{};
        LONG lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, nullptr, &dwSize);
        if (lResult == ERROR_SUCCESS)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, dwValue, &dwSize);
            if (dwValue != nullptr)
            {
                result = *dwValue;
                free(dwValue);
            }
        }
        return result;
    }

    void SetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, DWORD dwValue, bool find, bool* isNewValue)
    {
        int result{};
        DWORD dwSize{};
        HKEY hKey;
        LONG lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, nullptr, &dwSize);
        lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_SET_VALUE, &hKey);
        if (lResult == ERROR_FILE_NOT_FOUND)
        {
            lResult = RegCreateKeyExW(hKeyName, path, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
            if (lResult == ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
                if (isNewValue != nullptr) *isNewValue = true;
            }
        }
        else if (lResult == ERROR_SUCCESS)
        {
            DWORD* dwValueInternal = (DWORD*)malloc(dwSize);
            lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, dwValueInternal, &dwSize);
            if (lResult == ERROR_SUCCESS && find == false)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
                if (isNewValue != nullptr) *isNewValue = false;
            }
            else if (lResult != ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
                if (isNewValue != nullptr) *isNewValue = true;
            }
            free(dwValueInternal);
        }
        RegCloseKey(hKey);
    }

    bool GetRegistryStrValues(HKEY hKey, LPCWSTR path, LPCWSTR valueName, WCHAR** outStr)
    {
        if (!outStr) return false;

        DWORD dwSize{};
        LONG lResult = RegGetValueW(hKey, path, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            return false;
        }

        WCHAR* buffer = (WCHAR*)malloc(dwSize);
        if (!buffer)
        {
            return false;
        }

        lResult = RegGetValueW(hKey, path, valueName, RRF_RT_REG_SZ, nullptr, buffer, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            free(buffer);
            return false;
        }

        *outStr = buffer;
        return true;
    }

    bool GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE** outBytes)
    {
        if (!outBytes) return false;

        DWORD dwSize{};
        LONG lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_REG_BINARY, nullptr, nullptr, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            return false;
        }

        BYTE* buffer = (BYTE*)malloc(dwSize);
        if (!buffer)
        {
            return false;
        }

        lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_REG_BINARY, nullptr, buffer, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            free(buffer);
            return false;
        }

        *outBytes = buffer;
        return true;
    }

    void SetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueToSet, BYTE* bValue, DWORD length, bool find, bool* isNewValue)
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
                lResult = RegSetValueExW(hKey, valueToSet, 0, REG_BINARY, bValue, length);
                if (isNewValue != nullptr) *isNewValue = true;
            }
        }
        else if (lResult == ERROR_SUCCESS)
        {
            if (lResult == ERROR_SUCCESS && find == false)
            {
                lResult = RegSetValueExW(hKey, valueToSet, 0, REG_BINARY, bValue, length);
                if (isNewValue != nullptr) *isNewValue = false;
            }
            else if (lResult != ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueToSet, 0, REG_BINARY, bValue, length);
                if (isNewValue != nullptr) *isNewValue = true;
            }
        }
        RegCloseKey(hKey);
    }

    wstring GetExplorerTooltipText(const wstring& filePath)
    {
        wstring tooltipText;

        LPITEMIDLIST pidl = ILCreateFromPathW(filePath.c_str());
        if (!pidl) return L"";

        LPITEMIDLIST parentPidl = ILClone(pidl);
        if (!parentPidl)
        {
            ILFree(pidl);
            return L"";
        }

        if (!ILRemoveLastID(parentPidl))
        {
            ILFree(pidl);
            ILFree(parentPidl);
            return L"";
        }

        IShellFolder* pDesktopFolder = nullptr;
        if (SUCCEEDED(SHBindToObject(NULL, parentPidl, NULL, IID_IShellFolder, (void**)&pDesktopFolder)))
        {
            LPCITEMIDLIST relativePidl = ILFindLastID(pidl);
            IQueryInfo* pQueryInfo = nullptr;
            if (SUCCEEDED(pDesktopFolder->GetUIObjectOf(
                NULL, 1, &relativePidl, IID_IQueryInfo, NULL, (void**)&pQueryInfo)))
            {
                wchar_t* pTip = nullptr;
                if (SUCCEEDED(pQueryInfo->GetInfoTip(0, &pTip)) && pTip)
                {
                    tooltipText = pTip;
                    CoTaskMemFree(pTip);
                }
                pQueryInfo->Release();
            }
            pDesktopFolder->Release();
        }

        ILFree(pidl);
        ILFree(parentPidl);
        return tooltipText;
    }

    DWORD WINAPI MonitorFileChanges(LPVOID lpParam)
    {
        WCHAR* path = static_cast<WCHAR*>(lpParam);
        HANDLE hDir = CreateFileW(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (hDir == INVALID_HANDLE_VALUE)
        {
            TaskDialog(nullptr, nullptr, LoadStrFromRes(4025).c_str(), L"Failed to open directory handle", path, TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
            return 1;
        }

        BYTE buffer[1024];
        DWORD bytesReturned;
        while (true)
        {
            if (ReadDirectoryChangesW(hDir, &buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME
                                      | FILE_NOTIFY_CHANGE_ATTRIBUTES, &bytesReturned, nullptr, nullptr))
            {
                FILE_NOTIFY_INFORMATION* pNotify = nullptr;
                size_t offset = 0;
                do
                {
                    pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)buffer + offset);
                    wstring filename(pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
                    switch (pNotify->Action)
                    {
                        case FILE_ACTION_ADDED:
                            InitNewLVItem(path, filename);
                            break;
                        case FILE_ACTION_REMOVED:
                            RemoveLVItem(path, filename);
                            break;
                        case FILE_ACTION_RENAMED_OLD_NAME:
                            UpdateLVItem(path, filename, 1);
                            break;
                        case FILE_ACTION_RENAMED_NEW_NAME:
                            UpdateLVItem(path, filename, 2);
                            break;
                    }
                    offset += pNotify->NextEntryOffset;
                }
                while (pNotify->NextEntryOffset != 0);
            }
            else
            {
                TaskDialog(nullptr, nullptr, LoadStrFromRes(4025).c_str(), L"Failed to read directory changes", path, TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                break;
            }
        }

        CloseHandle(hDir);
        delete[] path;
        return 0;
    }

    void StartMonitorFileChanges(const wstring& path)
    {
        if (!PathFileExistsW(path.c_str())) return;
        HANDLE hThread = CreateThread(nullptr, 0, MonitorFileChanges, (LPVOID)path.c_str(), NULL, nullptr);
        if (hThread) CloseHandle(hThread);
    }

    static int checkSpotlight{};

    void FindShellIcon(vector<LVItem*>* pm, LPCWSTR clsid, LPCWSTR displayName, int* count2)
    {
        if (wcscmp(clsid, L"{645FF040-5081-101B-9F08-00AA002F954E}") == 0)
        {
            if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", clsid) == -1)
            {
                WCHAR clsidEx[64];
                StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
                WCHAR clsidEx2[64];
                StringCchPrintfW(clsidEx2, 64, L"CLSID\\%s", clsid);
                WCHAR* cRegValue{};
                GetRegistryStrValues(HKEY_CLASSES_ROOT, clsidEx2, L"InfoTip", &cRegValue);
                wstring regValue{};
                if (cRegValue) regValue = cRegValue;
                size_t modifier = regValue.find_last_of(L"\\") + 1;
                size_t modifier2 = regValue.find(L",");
                (*pm)[(*count2)]->SetSimpleFilename(displayName);
                (*pm)[(*count2)]->SetFilename(clsidEx);
                (*pm)[(*count2)]->SetAccDesc(LoadStrFromRes(_wtoi(regValue.substr(modifier2 + 2).c_str()), regValue.substr(modifier, modifier2 - modifier).c_str()).c_str());
                (*count2)++;
                free(cRegValue);
                return;
            }
        }
        if (wcscmp(clsid, L"{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}") == 0)
        {
            GetPos(true, &checkSpotlight);
            if (checkSpotlight == 1)
            {
                WCHAR clsidEx[64];
                StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
                WCHAR clsidEx2[64];
                StringCchPrintfW(clsidEx2, 64, L"CLSID\\%s", clsid);
                WCHAR* cRegValue{};
                GetRegistryStrValues(HKEY_CLASSES_ROOT, clsidEx2, L"InfoTip", &cRegValue);
                wstring regValue{};
                if (cRegValue) regValue = cRegValue;
                size_t modifier = regValue.find_last_of(L"\\") + 1;
                size_t modifier2 = regValue.find(L",");
                (*pm)[(*count2)]->SetSimpleFilename(displayName);
                (*pm)[(*count2)]->SetFilename(clsidEx);
                if (cRegValue) (*pm)[(*count2)]->SetAccDesc(LoadStrFromRes(_wtoi(regValue.substr(modifier2 + 2).c_str()), regValue.substr(modifier, modifier2 - modifier).c_str()).c_str());
                (*count2)++;
                free(cRegValue);
            }
            return;
        }
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel", clsid) == 0)
        {
            WCHAR clsidEx[64];
            StringCchPrintfW(clsidEx, 64, L"::%s", clsid);
            WCHAR clsidEx2[64];
            StringCchPrintfW(clsidEx2, 64, L"CLSID\\%s", clsid);
            WCHAR* cRegValue{};
            GetRegistryStrValues(HKEY_CLASSES_ROOT, clsidEx2, L"InfoTip", &cRegValue);
            wstring regValue{};
            if (cRegValue) regValue = cRegValue;
            size_t modifier = regValue.find_last_of(L"\\") + 1;
            size_t modifier2 = regValue.find(L",");
            (*pm)[(*count2)]->SetSimpleFilename(displayName);
            (*pm)[(*count2)]->SetFilename(clsidEx);
            if (cRegValue) (*pm)[(*count2)]->SetAccDesc(LoadStrFromRes(_wtoi(regValue.substr(modifier2 + 2).c_str()), regValue.substr(modifier, modifier2 - modifier).c_str()).c_str());
            (*count2)++;
            free(cRegValue);
            return;
        }
    }

    unsigned short EnumerateFolder_Helper(LPWSTR path)
    {
        WIN32_FIND_DATAW fd;
        wstring searchPath = wstring(path) + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) return 0;

        unsigned short count = 0;
        do
        {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            count++;
        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);
        return count;
    }

    void EnumerateFolder(LPWSTR path, vector<LVItem*>* pmLVItem, int* count2, unsigned short limit)
    {
        if (!PathFileExistsW(path) && wcscmp(path, L"InternalCodeForNamespace") != 0)
        {
            WCHAR details[320];
            StringCchPrintfW(details, 320, L"Error: Can't find %s.", path);
            MainLogger.WriteLine(details);
            return;
        }
        int runs = 0;
        if (wcscmp(path, L"InternalCodeForNamespace") == 0)
        {
            filestructs.clear();
            HINSTANCE WinStorageDLL = LoadLibraryW(L"windows.storage.dll");
            HINSTANCE Shell32DLL = LoadLibraryW(L"shell32.dll");
            wchar_t *ThisPC = new wchar_t[260], *ThisPCBuf{};
            GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}", nullptr, &ThisPCBuf);
            if (ThisPCBuf == nullptr)
            {
                LoadStringW(WinStorageDLL, 9216, ThisPC, 260);
                FindShellIcon(pmLVItem, L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", ThisPC, count2);
                delete[] ThisPC;
            }
            else FindShellIcon(pmLVItem, L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", ThisPCBuf, count2);
            wchar_t *RecycleBin = new wchar_t[260], *RecycleBinBuf{};
            GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}", nullptr, &RecycleBinBuf);
            if (RecycleBinBuf == nullptr)
            {
                LoadStringW(WinStorageDLL, 8964, RecycleBin, 260);
                FindShellIcon(pmLVItem, L"{645FF040-5081-101B-9F08-00AA002F954E}", RecycleBin, count2);
                delete[] RecycleBin;
            }
            else FindShellIcon(pmLVItem, L"{645FF040-5081-101B-9F08-00AA002F954E}", RecycleBinBuf, count2);
            wchar_t* UserFiles = new wchar_t[260];
            DWORD d = GetEnvironmentVariableW(L"userprofile", UserFiles, 260);
            wstring UserFiless = UserFiles;
            if (UserFiless.length() >= 9) UserFiless.erase(0, 9);
            wchar_t* ControlPanel = new wchar_t[260];
            LoadStringW(WinStorageDLL, 4161, ControlPanel, 260);
            wchar_t* Network = new wchar_t[260];
            LoadStringW(WinStorageDLL, 9217, Network, 260);
            //wchar_t* LearnAbout = new wchar_t[260];
            //LoadStringW(Shell32DLL, 51761, LearnAbout, 260);
            FindShellIcon(pmLVItem, L"{59031A47-3F72-44A7-89C5-5595FE6B30EE}", UserFiless.c_str(), count2);
            FindShellIcon(pmLVItem, L"{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}", ControlPanel, count2);
            FindShellIcon(pmLVItem, L"{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}", Network, count2);
            //FindShellIcon(pmLVItem, L"{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}", LearnAbout, count2); // Disabled until a better way to find CLSIDs is written
            free(ThisPCBuf);
            free(RecycleBinBuf);
            delete[] UserFiles;
            delete[] ControlPanel;
            delete[] Network;
            //delete[] LearnAbout;
            if (WinStorageDLL) FreeLibrary(WinStorageDLL);
            if (Shell32DLL) FreeLibrary(Shell32DLL);
            if (logging == IDYES) MainLogger.WriteLine(L"Information: Finished finding shell icons.");
            return;
        }
        HRESULT hr;

        LPMALLOC pMalloc = nullptr;
        hr = SHGetMalloc(&pMalloc);

        LPSHELLFOLDER psfDesktop = nullptr;
        hr = SHGetDesktopFolder(&psfDesktop);

        if (FAILED(hr) || pMalloc == nullptr || psfDesktop == nullptr)
        {
            if (pMalloc) pMalloc->Release();
            if (psfDesktop) psfDesktop->Release();
            return;
        }

        LPITEMIDLIST pidl = nullptr;
        hr = psfDesktop->ParseDisplayName(nullptr, nullptr, path, nullptr, &pidl, nullptr);

        LPSHELLFOLDER psfFolder = nullptr;
        hr = psfDesktop->BindToObject(pidl, nullptr, IID_IShellFolder, (void**)&psfFolder);
        if (psfDesktop != nullptr) psfDesktop->Release();
        pMalloc->Free(pidl);

        LPENUMIDLIST pEnumIDL = nullptr;
        if (psfFolder) hr = psfFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN, &pEnumIDL);
        while (runs < limit)
        {
            if (pEnumIDL == nullptr) break;
            hr = pEnumIDL->Next(1, &pidl, nullptr);
            wstring foundsimplefilename{};
            wstring foundfilename{};
            if (hr == NOERROR)
            {
                WIN32_FIND_DATAW fd;
                hr = SHGetDataFromIDListW(psfFolder, pidl, SHGDFIL_FINDDATA, &fd, sizeof(WIN32_FIND_DATAW));
                if (count2 != nullptr)
                {
                    DWORD lviFlags{};
                    foundfilename = (wstring)L"\"" + path + (wstring)L"\\" + wstring(fd.cFileName) + (wstring)L"\"";
                    if (fd.dwFileAttributes & 16)
                    {
                        lviFlags |= LVIF_DIR;
                        unsigned short itemsInside = EnumerateFolder_Helper((LPWSTR)RemoveQuotes(foundfilename).c_str());
                        if (pmLVItem == &pm && itemsInside <= 192) lviFlags |= LVIF_GROUP;
                        (*pmLVItem)[*(count2)]->SetItemCount(itemsInside);
                    }
                    if (fd.dwFileAttributes & 2) lviFlags |= LVIF_HIDDEN;
                    filestructs.push_back({ fd.cFileName, fd.dwFileAttributes });
                    if (g_isThumbnailHidden == 0)
                    {
                        bool image;
                        isSpecialProp(foundfilename, true, &image, &imageExts);
                        if (image) lviFlags |= LVIF_COLORLOCK;
                    }
                    bool advancedicon;
                    isSpecialProp(foundfilename, true, &advancedicon, &advancedIconExts);
                    if (advancedicon) lviFlags |= LVIF_ADVANCEDICON;
                    (*pmLVItem)[*(count2)]->SetFlags(static_cast<LVItemFlags>(lviFlags));
                    foundsimplefilename = hideExt((wstring)fd.cFileName, g_hideFileExt, (fd.dwFileAttributes & 16), (*pmLVItem)[*(count2)]);
                    (*pmLVItem)[*(count2)]->SetSimpleFilename(foundsimplefilename);
                    (*pmLVItem)[*(count2)]->SetFilename(foundfilename);
                    (*pmLVItem)[*(count2)]->SetAccDesc(GetExplorerTooltipText(RemoveQuotes(foundfilename)).c_str());
                }
                if (g_showHidden == 2 && fd.dwFileAttributes & 2)
                {
                    pMalloc->Free(pidl);
                    continue;
                }
                if (g_showSuperHidden == 2 || g_showSuperHidden == 0)
                {
                    if (fd.dwFileAttributes & 4)
                    {
                        pMalloc->Free(pidl);
                        continue;
                    }
                }
                pMalloc->Free(pidl);
            }
            else break;
            runs++;
            if (count2 != nullptr)
            {
                (*count2)++;
                if (logging == IDYES)
                {
                    WCHAR details[320];
                    StringCchPrintfW(details, 320, L"\nNew item added, Item name: %s\nItem name to be shown on desktop: %s\nItems counted: %d", foundfilename.c_str(), foundsimplefilename.c_str(), *count2);
                    MainLogger.WriteLine(details);
                }
            }
        }

        if (pEnumIDL != nullptr) pEnumIDL->Release();
        if (psfFolder != nullptr) psfFolder->Release();
        if (pMalloc != nullptr) pMalloc->Release();
        if (logging == IDYES)
        {
            WCHAR details[320];
            StringCchPrintfW(details, 320, L"Information: Finished searching in %s.", path);
            MainLogger.WriteLine(details);
        }
    }

    void EnumerateFolderForThumbnails(LPWSTR path, vector<ThumbIcons>* strs, unsigned short limit)
    {
        if (!PathFileExistsW(path)) return;
        int runs = 0;
        HRESULT hr;

        LPMALLOC pMalloc = nullptr;
        hr = SHGetMalloc(&pMalloc);

        LPSHELLFOLDER psfDesktop = nullptr;
        hr = SHGetDesktopFolder(&psfDesktop);

        LPITEMIDLIST pidl = nullptr;
        hr = psfDesktop->ParseDisplayName(nullptr, nullptr, path, nullptr, &pidl, nullptr);

        LPSHELLFOLDER psfFolder = nullptr;
        hr = psfDesktop->BindToObject(pidl, nullptr, IID_IShellFolder, (void**)&psfFolder);
        if (psfDesktop != nullptr) psfDesktop->Release();
        pMalloc->Free(pidl);

        LPENUMIDLIST pEnumIDL = nullptr;
        hr = psfFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN, &pEnumIDL);
        while (runs < limit)
        {
            if (pEnumIDL == nullptr) break;
            hr = pEnumIDL->Next(1, &pidl, nullptr);
            if (hr == NOERROR)
            {
                WIN32_FIND_DATAW fd;
                hr = SHGetDataFromIDListW(psfFolder, pidl, SHGDFIL_FINDDATA, &fd, sizeof(WIN32_FIND_DATAW));
                strs->resize(runs + 1);
                wstring foundfilename = path + (wstring)L"\\" + wstring(fd.cFileName);
                (*strs)[runs].SetFilename(foundfilename);
                if (fd.dwFileAttributes & 2) (*strs)[runs].SetHiddenState(true);
                else (*strs)[runs].SetHiddenState(false);
                if (g_isThumbnailHidden == 0)
                {
                    bool image;
                    isSpecialProp(foundfilename, true, &image, &imageExts);
                    (*strs)[runs].SetColorLock(image);
                }
                bool advancedicon;
                isSpecialProp(foundfilename, true, &advancedicon, &advancedIconExts);
                (*strs)[runs].SetHasAdvancedIcon(advancedicon);
                if (g_showHidden == 2 && fd.dwFileAttributes & 2)
                {
                    pMalloc->Free(pidl);
                    continue;
                }
                if (g_showSuperHidden == 2 || g_showSuperHidden == 0)
                {
                    if (fd.dwFileAttributes & 4)
                    {
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

    bool PlaceDesktopInPos(int* WindowsBuild, HWND* hWndProgman, HWND* hWorkerW, HWND* hSHELLDLL_DefView, bool findSHELLDLL_DefView)
    {
        HWND hWndResult{};
        if (*WindowsBuild < 26002) *hWorkerW = GetWorkerW2();
        else *hWorkerW = FindWindowExW(*hWndProgman, nullptr, L"WorkerW", nullptr);
        if (hWorkerW)
        {
            if (findSHELLDLL_DefView) *hSHELLDLL_DefView = FindWindowExW(*hWorkerW, nullptr, L"SHELLDLL_DefView", nullptr);
            if (logging == IDYES)
            {
                if (*hSHELLDLL_DefView != nullptr) MainLogger.WriteLine(L"Information: Found SHELLDLL_DefView.");
                else MainLogger.WriteLine(L"Error: No SHELLDLL_DefView found.");
            }
            if (*hSHELLDLL_DefView != nullptr)
            {
                if (*WindowsBuild >= 26002)
                {
                    hWndResult = SetParent(*hSHELLDLL_DefView, *hWndProgman);
                    if (logging == IDYES)
                    {
                        if (hWndResult != nullptr) MainLogger.WriteLine(L"Information: Added DirectDesktop inside Progman.");
                        else MainLogger.WriteLine(L"Error: Could not add DirectDesktop inside Progman.");
                    }
                }
                else hWndResult = SetParent(*hSHELLDLL_DefView, *hWorkerW);
            }
        }
        return 1;
    }

    // A portion of this function (parsing) was sourced from
    // https://stackoverflow.com/questions/70039190/how-to-read-the-values-of-iconlayouts-reg-binary-registry-file
    // and "translated" to C++ using AI.

    void GetPos(bool getSpotlightIcon, int* setSpotlightIcon)
    {
        SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, nullptr, nullptr);
        //SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 200, nullptr);

        HINSTANCE WinStorageDLL = LoadLibraryW(L"windows.storage.dll");
        HINSTANCE Shell32DLL = LoadLibraryW(L"shell32.dll");
        BYTE* value{};
        GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconLayouts", &value);

        // Parse header at offset 0x10
        if (logging == IDYES && !getSpotlightIcon) MainLogger.WriteLine(L"Information: Preparing to parse icon layout registry at offset 0x10.");
        size_t offset = 0x10;
        vector<uint16_t> head;
        for (int i = 0; i < 4; ++i)
        {
            // First 4 WORDs
            head.push_back(*reinterpret_cast<uint16_t*>(&value[offset + i * 2]));
        }
        head.push_back(*reinterpret_cast<uint32_t*>(&value[offset + 8])); // DWORD at offset + 8

        uint32_t number_of_items = head[4];
        offset += 12;

        if (logging == IDYES && !getSpotlightIcon)
        {
            WCHAR regReport[96];
            StringCchPrintfW(regReport, 96, L"\nInformation: Registry reports you have %d items.", number_of_items);
            MainLogger.WriteLine(regReport);
        }

        // Start parsing desktop items
        vector<DesktopItem> desktop_items;
        if (logging == IDYES && !getSpotlightIcon) MainLogger.WriteLine(L"Information: Preparing to parse desktop item names.");
        for (uint32_t x = 0; x < number_of_items; x++)
        {
            if (logging == IDYES && !getSpotlightIcon)
            {
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
            item.name = hideExtFromGetPos(wstring(
                                              reinterpret_cast<const wchar_t*>(&value[offset]),
                                              name_len / sizeof(wchar_t)
                                          ), g_hideFileExt);
            wchar_t *nameBuffer = new wchar_t[260], *nameBuffer2{};
            if (item.name == L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}")
            {
                GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}", nullptr, &nameBuffer2);
                if (nameBuffer2 == nullptr)
                {
                    delete[] nameBuffer;
                    nameBuffer = new wchar_t[260];
                    LoadStringW(WinStorageDLL, 9216, nameBuffer, 260);
                    item.name = nameBuffer;
                }
                else item.name = nameBuffer2;
            }
            if (nameBuffer2) free(nameBuffer2);
            if (item.name == L"::{645FF040-5081-101B-9F08-00AA002F954E}")
            {
                GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}", nullptr, &nameBuffer2);
                if (nameBuffer2 == nullptr)
                {
                    delete[] nameBuffer;
                    nameBuffer = new wchar_t[260];
                    LoadStringW(WinStorageDLL, 8964, nameBuffer, 260);
                    item.name = nameBuffer;
                }
                else item.name = nameBuffer2;
            }
            if (nameBuffer2) free(nameBuffer2);
            if (item.name == L"::{59031A47-3F72-44A7-89C5-5595FE6B30EE}")
            {
                DWORD d = GetEnvironmentVariableW(L"userprofile", nameBuffer, 260);
                wstring UserFiless = nameBuffer;
                if (UserFiless.length() >= 9) UserFiless.erase(0, 9);
                item.name = UserFiless;
            }
            if (item.name == L"::{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}")
            {
                LoadStringW(WinStorageDLL, 4161, nameBuffer, 260);
                item.name = nameBuffer;
            }
            if (item.name == L"::{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}")
            {
                LoadStringW(WinStorageDLL, 9217, nameBuffer, 260);
                item.name = nameBuffer;
            }
            if (item.name == L"::{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}")
            {
                LoadStringW(Shell32DLL, 51761, nameBuffer, 260);
                item.name = nameBuffer;
                if (getSpotlightIcon)
                {
                    if (logging == IDYES) MainLogger.WriteLine(L"Information: Found Windows Spotlight icon.");
                    (*setSpotlightIcon) = 1;
                    break;
                }
            }
            delete[] nameBuffer;
            offset += name_len + 4;

            if (!getSpotlightIcon)
            {
                desktop_items.push_back(item);
                if (logging == IDYES)
                {
                    WCHAR details[320];
                    StringCchPrintfW(details, 320, L"New item found, Item name: %s\nItem name to be shown on desktop: %s", pm[x]->GetFilename().c_str(), pm[x]->GetSimpleFilename().c_str());
                    MainLogger.WriteLine(details);
                }
            }
        }
        if (getSpotlightIcon)
        {
            free(value);
            return;
        }

        // Parse head2 (64 bytes)
        vector<uint16_t> head2;
        size_t offs = offset;
        for (int x = 0; x < 32; x++)
        {
            head2.push_back(*reinterpret_cast<uint16_t*>(&value[offs + x * 2]));
        }
        offs += 64;
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Parsed head2.");

        // Parse position tables
        for (uint32_t x = 0; x < number_of_items; ++x)
        {
            uint16_t column = *reinterpret_cast<uint16_t*>(&value[offs + 2]);
            uint16_t row = *reinterpret_cast<uint16_t*>(&value[offs + 6]);
            uint16_t index = *reinterpret_cast<uint16_t*>(&value[offs + 8]);
            desktop_items[index].column = column;
            desktop_items[index].row = row;
            offs += 10;
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Parsed position table.");

        // This is such a bad way to sort...
        vector<LVItem*> pmBuf, pmBuf2;
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Created temporary arrays to arrange icons.");
        pmBuf.resize(pm.size());
        pmBuf2.resize(pm.size());
        vector<DesktopItem> new_desktop_items;
        for (int index = 0; index < number_of_items; index++)
        {
            pmBuf2[index] = pm[index];
            for (int index2 = 0; index2 < pm.size(); index2++)
            {
                if (desktop_items[index].name == pm[index2]->GetSimpleFilename())
                {
                    new_desktop_items.push_back(desktop_items[index]);
                    break;
                }
            }
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Resized the temporary arrays.");
        int fileCount{};
        for (int index = 0; index < pm.size(); index++)
        {
            wstring itemToFind = pm[index]->GetSimpleFilename();
            for (int index2 = 0; index2 < new_desktop_items.size(); index2++)
            {
                if (new_desktop_items[index2].name == itemToFind)
                {
                    new_desktop_items[index2].name = L"?";
                    pmBuf[index2] = pm[index];
                    fileCount++;
                    break;
                }
            }
        }
        for (int index = 0; index < pm.size(); index++)
        {
            pm[index] = pmBuf[index];
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Filled the temporary arrays.");
        ////////////////////////////////////

        for (int index = 0; index < fileCount; index++)
        {
            int r = new_desktop_items[index].row, c = new_desktop_items[index].column;
            short tempXPos{}, tempYPos{}, bitsX = 128, bitsY = 128, bitsXAccumulator{}, bitsYAccumulator{};
            if (logging == IDYES)
            {
                WCHAR details[320];
                StringCchPrintfW(details, 320, L"\nItem prepared for arrangement (%d of %d)\nItem name: %s\nX (encoded): %d, Y (encoded): %d", index + 1, fileCount, pm[index]->GetFilename().c_str(), desktop_items[index].column, desktop_items[index].row);
                MainLogger.WriteLine(details);
            }
            while (true)
            {
                if (c == 0)
                {
                    pm[index]->SetInternalXPos(tempXPos);
                    break;
                }
                if (tempXPos == 0)
                {
                    c -= 16256;
                    tempXPos++;
                    continue;
                }
                c -= bitsX;
                bitsXAccumulator += bitsX;
                if (bitsXAccumulator == 128)
                {
                    bitsXAccumulator = 0;
                    bitsX /= 2;
                }
                tempXPos++;
                if (tempXPos > 200) break;
            }
            while (true)
            {
                if (r == 0)
                {
                    pm[index]->SetInternalYPos(tempYPos);
                    if (logging == IDYES)
                    {
                        WCHAR details[320];
                        StringCchPrintfW(details, 320, L"\nItem arranged (%d of %d)\nItem name: %s\nX: %d, Y: %d", index + 1, fileCount, pm[index]->GetFilename().c_str(), pm[index]->GetInternalXPos(), pm[index]->GetInternalYPos());
                        MainLogger.WriteLine(details);
                    }
                    break;
                }
                if (tempYPos == 0)
                {
                    r -= 16256;
                    tempYPos++;
                    continue;
                }
                r -= bitsY;
                bitsYAccumulator += bitsY;
                if (bitsYAccumulator == 128)
                {
                    bitsYAccumulator = 0;
                    bitsY /= 2;
                }
                tempYPos++;
                if (tempYPos > 200) break;
            }
            pm[index] = pmBuf2[index];
        }
        pmBuf.clear();
        pmBuf2.clear();
        free(value);
    }

    void GetPos2(bool full)
    {
        WCHAR DesktopLayoutWithSize[24];
        if (!g_touchmode) StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_%d", g_iconsz);
        else StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_Touch");
        COLORREF* pImmersiveColor = g_theme ? &ImmersiveColorL : &ImmersiveColorD;
        COLORREF colorPickerPalette[8] =
        {
            -1,
            *pImmersiveColor,
            g_theme ? RGB(76, 194, 255) : RGB(0, 103, 192),
            g_theme ? RGB(216, 141, 225) : RGB(158, 58, 176),
            g_theme ? RGB(244, 103, 98) : RGB(210, 14, 30),
            g_theme ? RGB(251, 154, 68) : RGB(224, 83, 7),
            g_theme ? RGB(255, 213, 42) : RGB(225, 157, 0),
            g_theme ? RGB(38, 255, 142) : RGB(0, 178, 90)
        };
        BYTE* valueSizeKey{}, *valueCommon{};
        GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize, &valueSizeKey);
        //GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"DesktopLayoutCommon", &valueCommon);
        size_t offsetSizeKey = 0, offsetCommon = 0;
        if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize))
        {
            g_maxPageID = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
            offsetSizeKey += 2;
            g_homePageID = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
            offsetSizeKey += 2;
        }
        if (full)
        {
            for (int i = 0; i < pm.size(); i++)
            {
                unsigned short namelen = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                offsetSizeKey += 2;
                wstring filename = wstring(reinterpret_cast<WCHAR*>(&valueSizeKey[offsetSizeKey]), namelen);
                offsetSizeKey += (namelen * 2);
                bool match = false;
                for (int j = 0; j < pm.size(); j++)
                {
                    if (pm[j]->GetFilename() == filename)
                    {
                        unsigned short xPos = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                        offsetSizeKey += 2;
                        pm[j]->SetInternalXPos(xPos);
                        unsigned short yPos = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                        offsetSizeKey += 6;
                        pm[j]->SetInternalYPos(yPos);
                        unsigned short page = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                        offsetSizeKey += 2;
                        pm[j]->SetPage(page);
                        if (page > g_maxPageID) g_maxPageID = page;
                        if (g_touchmode)
                        {
                            unsigned short tilesize = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                            offsetSizeKey += 2;
                            pm[j]->SetTileSize(static_cast<LVItemTileSize>(tilesize));
                            unsigned short smpos = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                            offsetSizeKey += 2;
                            pm[j]->SetSmallPos(smpos);
                        }
                        match = true;
                        break;
                    }
                }
                if (!match) offsetSizeKey += g_touchmode ? 14 : 10;
            }
        }
        if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"GroupSizeTable"))
        {
            free(valueSizeKey);
            valueSizeKey = nullptr;
            GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"GroupSizeTable", &valueSizeKey);
            offsetSizeKey = 0;
            for (int i = 0; i < pm.size(); i++)
            {
                if (pm[i]->GetFlags() & LVIF_DIR)
                {
                    unsigned short namelen = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                    offsetSizeKey += 2;
                    wstring filename = wstring(reinterpret_cast<WCHAR*>(&valueSizeKey[offsetSizeKey]), namelen);
                    offsetSizeKey += (namelen * 2);
                    bool match = false;
                    for (int j = 0; j < pm.size(); j++)
                    {
                        if (pm[j]->GetFilename() == filename)
                        {
                            unsigned short size = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                            offsetSizeKey += 2;
                            pm[j]->SetGroupSize(static_cast<LVItemGroupSize>(size));
                            match = true;
                            break;
                        }
                    }
                    if (!match) offsetSizeKey += 2;
                }
            }
        }
        if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"GroupColorTable"))
        {
            free(valueSizeKey);
            valueSizeKey = nullptr;
            GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"GroupColorTable", &valueSizeKey);
            offsetSizeKey = 0;
            for (int i = 0; i < pm.size(); i++)
            {
                DDScalableElement* peIcon = pm[i]->GetIcon();
                if (pm[i]->GetFlags() & LVIF_DIR)
                {
                    unsigned short namelen = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                    offsetSizeKey += 2;
                    wstring filename = wstring(reinterpret_cast<WCHAR*>(&valueSizeKey[offsetSizeKey]), namelen);
                    offsetSizeKey += (namelen * 2);
                    bool match = false;
                    for (int j = 0; j < pm.size(); j++)
                    {
                        if (pm[j]->GetFilename() == filename)
                        {
                            unsigned short colorID = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                            offsetSizeKey += 2;
                            peIcon->SetGroupColor(colorID);
                            if (pm[j]->GetGroupSize() == LVIGS_NORMAL)
                                pm[j]->GetIcon()->SetAssociatedColor(colorPickerPalette[colorID]);
                            else
                            {
                                pm[j]->SetAssociatedColor(colorPickerPalette[colorID]);
                                if (pm[j]->GetOpenDirState() == LVIODS_PINNED)
                                    pm[j]->GetIcon()->SetAssociatedColor(colorPickerPalette[colorID]);
                            }
                            unsigned short intensity = *reinterpret_cast<unsigned short*>(&valueSizeKey[offsetSizeKey]);
                            offsetSizeKey += 2;
                            peIcon->SetDDCPIntensity(intensity);
                            match = true;
                            break;
                        }
                    }
                    if (!match) offsetSizeKey += 4;
                }
            }
        }
        free(valueSizeKey);
    }

    void SetPos(bool full)
    {
        vector<BYTE> DesktopLayout;
        //vector<BYTE> DesktopLayoutCommon;
        if (full)
        {
            RECT dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            unsigned short page = g_maxPageID;
            const BYTE* minpageBinary = reinterpret_cast<const BYTE*>(&page);
            DesktopLayout.push_back(minpageBinary[0]);
            DesktopLayout.push_back(minpageBinary[1]);
            page = g_homePageID;
            const BYTE* homepageBinary = reinterpret_cast<const BYTE*>(&page);
            DesktopLayout.push_back(homepageBinary[0]);
            DesktopLayout.push_back(homepageBinary[1]);
            for (int i = 0; i < pm.size(); i++)
            {
                unsigned short xPos = (localeType == 1) ? dimensions.right - pm[i]->GetX() - pm[i]->GetWidth() : pm[i]->GetX();
                wstring filename = pm[i]->GetFilename();
                unsigned short temp = filename.length();
                const BYTE* namelen = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(namelen[0]);
                DesktopLayout.push_back(namelen[1]);
                const BYTE* bytes = reinterpret_cast<const BYTE*>(filename.c_str());
                size_t len = (filename.length()) * sizeof(WCHAR);
                DesktopLayout.insert(DesktopLayout.end(), bytes, bytes + len);
                temp = pm[i]->GetInternalXPos();
                const BYTE* xBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(xBinary[0]);
                DesktopLayout.push_back(xBinary[1]);
                temp = pm[i]->GetInternalYPos();
                const BYTE* yBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(yBinary[0]);
                DesktopLayout.push_back(yBinary[1]);
                temp = xPos;
                const BYTE* xPosBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(xPosBinary[0]);
                DesktopLayout.push_back(xPosBinary[1]);
                temp = (unsigned short)pm[i]->GetY();
                const BYTE* yPosBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(yPosBinary[0]);
                DesktopLayout.push_back(yPosBinary[1]);
                temp = pm[i]->GetPage();
                const BYTE* pageBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(pageBinary[0]);
                DesktopLayout.push_back(pageBinary[1]);
                if (g_touchmode)
                {
                    temp = static_cast<unsigned short>(pm[i]->GetTileSize());
                    const BYTE* tilesizeBinary = reinterpret_cast<const BYTE*>(&temp);
                    DesktopLayout.push_back(tilesizeBinary[0]);
                    DesktopLayout.push_back(tilesizeBinary[1]);
                    temp = static_cast<unsigned short>(pm[i]->GetSmallPos());
                    const BYTE* smposBinary = reinterpret_cast<const BYTE*>(&temp);
                    DesktopLayout.push_back(smposBinary[0]);
                    DesktopLayout.push_back(smposBinary[1]);
                }
            }
            WCHAR DesktopLayoutWithSize[24];
            if (!g_touchmode) StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_%d", g_iconsz);
            else StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_Touch");
            SetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize, DesktopLayout.data(), DesktopLayout.size(), false, nullptr);
            //SetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"DesktopLayoutCommon", DesktopLayoutCommon.data(), DesktopLayoutCommon.size(), false, nullptr);
            DesktopLayout.clear();
            //DesktopLayoutCommon.clear();
        }
        for (int i = 0; i < pm.size(); i++)
        {
            if (pm[i]->GetFlags() & LVIF_DIR)
            {
                wstring filename = pm[i]->GetFilename();
                unsigned short temp = filename.length();
                const BYTE* namelen = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(namelen[0]);
                DesktopLayout.push_back(namelen[1]);
                const BYTE* bytes = reinterpret_cast<const BYTE*>(filename.c_str());
                size_t len = (filename.length()) * sizeof(WCHAR);
                DesktopLayout.insert(DesktopLayout.end(), bytes, bytes + len);
                temp = static_cast<unsigned short>(pm[i]->GetGroupSize());
                const BYTE* sizeBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(sizeBinary[0]);
                DesktopLayout.push_back(sizeBinary[1]);
            }
        }
        SetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"GroupSizeTable", DesktopLayout.data(), DesktopLayout.size(), false, nullptr);
        DesktopLayout.clear();
        for (int i = 0; i < pm.size(); i++)
        {
            if (pm[i]->GetFlags() & LVIF_DIR)
            {
                DDScalableElement* peIcon = pm[i]->GetIcon();
                wstring filename = pm[i]->GetFilename();
                unsigned short temp = filename.length();
                const BYTE* namelen = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(namelen[0]);
                DesktopLayout.push_back(namelen[1]);
                const BYTE* bytes = reinterpret_cast<const BYTE*>(filename.c_str());
                size_t len = (filename.length()) * sizeof(WCHAR);
                DesktopLayout.insert(DesktopLayout.end(), bytes, bytes + len);
                temp = peIcon->GetGroupColor();
                const BYTE* colorBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(colorBinary[0]);
                DesktopLayout.push_back(colorBinary[1]);
                temp = static_cast<unsigned short>(peIcon->GetDDCPIntensity());
                const BYTE* intensityBinary = reinterpret_cast<const BYTE*>(&temp);
                DesktopLayout.push_back(intensityBinary[0]);
                DesktopLayout.push_back(intensityBinary[1]);
            }
        }
        SetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"GroupColorTable", DesktopLayout.data(), DesktopLayout.size(), false, nullptr);
        DesktopLayout.clear();
    }
}
