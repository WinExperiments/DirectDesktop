#include "DirectoryHelper.h"
#include "strsafe.h"

using namespace std;

int shortIndex, subshortIndex, dirIndex;
vector<parameters> pm, subpm;
vector<parameters> shortpm, subshortpm;
vector<wstring> listDirBuffer, sublistDirBuffer;

wstring hideExt(const wstring& filename, bool isEnabled, bool type) {
    if (isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            type ? shortpm[shortIndex++].x = 1 : subshortpm[subshortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            type ? shortpm[shortIndex++].x = 1 : subshortpm[subshortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find_last_of(L".");
        else {
            type ? shortpm[shortIndex++].x = 1 : subshortpm[subshortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        type ? shortIndex++ : subshortIndex++;
        if (lastdot == wstring::npos) return filename;
        return filename.substr(0, lastdot);
    }
    if (!isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            type ? shortpm[shortIndex++].x = 1 : subshortpm[subshortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find(L".url");
        else {
            type ? shortpm[shortIndex++].x = 1 : subshortpm[subshortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) {
            type ? shortIndex++ : subshortIndex++;
            return filename;
        }
        else {
            type ? shortpm[shortIndex++].x = 1 : subshortpm[subshortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
    }
}

int GetRegistryValues(HKEY hKeyName, LPCWSTR path, const wchar_t* valueToFind) {
    int result{};
    DWORD dwSize{};
    LONG lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_ANY, NULL, NULL, &dwSize);
    if (lResult == ERROR_SUCCESS)
    {
        DWORD* dwValue = (DWORD*)malloc(dwSize);
        lResult = RegGetValueW(hKeyName, path, valueToFind, RRF_RT_ANY, NULL, dwValue, &dwSize);
        result = *dwValue;
    }
    return result;
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

void AddFile(wstring path, WIN32_FIND_DATAW data) {
    pm.push_back({ NULL, NULL, NULL });
    shortpm.push_back({ NULL, NULL, NULL });
    listDirBuffer.push_back(path + wstring(data.cFileName));
    if (data.dwFileAttributes & 16) {
        pm[dirIndex++].isDirectory = true;
    }
    else pm[dirIndex++].isDirectory = false;
}
void RemoveFile() {
    dirIndex--;
    shortIndex--;
    pm.pop_back();
    shortpm.pop_back();
    listDirBuffer.pop_back();
}

vector<wstring> list_directory() {
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    WIN32_FIND_DATAW findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    wchar_t* full_path = new wchar_t[260];
    wchar_t* full_path2 = new wchar_t[260];
    wchar_t* full_path3 = new wchar_t[260];
    DWORD d = GetEnvironmentVariableW(L"userprofile", 0, 0);
    DWORD d2 = GetEnvironmentVariableW(L"PUBLIC", 0, 0);
    vector<wchar_t> envName(d);
    vector<wchar_t> envName2(d2);
    GetEnvironmentVariableW(L"userprofile", envName.data(), 260);
    GetEnvironmentVariableW(L"PUBLIC", envName2.data(), 260);
    StringCchPrintfW(full_path, 260, L"%s\\Desktop\\*", envName.data());
    StringCchPrintfW(full_path2, 260, L"%s\\Desktop\\*", envName2.data());
    StringCchPrintfW(full_path3, 260, L"%s\\OneDrive\\Desktop\\*", envName.data());
    vector<wstring> dir_list;

    int runs = 0;
    wchar_t buffer[260];
    size_t asterisk = ((wstring)full_path).find(L"*");
    size_t asterisk2 = ((wstring)full_path2).find(L"*");
    size_t asterisk3 = ((wstring)full_path3).find(L"*");
    wstring full_path_truncated = ((wstring)full_path).substr(0, asterisk);
    wstring full_path2_truncated = ((wstring)full_path2).substr(0, asterisk2);
    wstring full_path3_truncated = ((wstring)full_path3).substr(0, asterisk3);
    hFind = FindFirstFileW(full_path, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            AddFile(full_path_truncated, findData);
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden, 1));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            dir_list.pop_back();
            RemoveFile();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                dir_list.pop_back();
                RemoveFile();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path2, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            AddFile(full_path2_truncated, findData);
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden, 1));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            dir_list.pop_back();
            RemoveFile();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                dir_list.pop_back();
                RemoveFile();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path3, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            AddFile(full_path3_truncated, findData);
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden, 1));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            dir_list.pop_back();
            RemoveFile();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                dir_list.pop_back();
                RemoveFile();
                continue;
            }
        }
    }

    delete[] full_path;
    delete[] full_path2;
    delete[] full_path3;
    envName.clear();
    envName2.clear();

    FindClose(hFind);
    return dir_list;
}

vector<wstring> list_subdirectory(wstring path) {
    int isFileHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden");
    int isFileSuperHiddenEnabled = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ShowSuperHidden");
    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
    WIN32_FIND_DATAW findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    vector<wstring> subdir_list;

    int runs = 0;
    wchar_t buffer[260];
    size_t asterisk = path.find(L"*");
    wstring path_truncated = path.substr(0, asterisk);
    hFind = FindFirstFileW(path.c_str(), &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            subshortpm.push_back({ NULL, NULL, NULL });
            subdir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden, 0));
            sublistDirBuffer.push_back(path_truncated + wstring(findData.cFileName));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            subshortIndex--;
            subshortpm.pop_back();
            subdir_list.pop_back();
            sublistDirBuffer.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                subshortIndex--;
                subshortpm.pop_back();
                subdir_list.pop_back();
                sublistDirBuffer.pop_back();
                continue;
            }
        }
    }

    FindClose(hFind);
    return subdir_list;
}