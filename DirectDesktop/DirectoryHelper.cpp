#include "DirectoryHelper.h"
#include "strsafe.h"

using namespace std;

int shortIndex;
vector<parameters> shortpm;
vector<wstring> listDirBuffer;

wstring hideExt(const wstring& filename, bool isEnabled) {
    if (isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find_last_of(L".");
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        shortIndex++;
        if (lastdot == wstring::npos) return filename;
        return filename.substr(0, lastdot);
    }
    if (!isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) {
            shortIndex++;
            return filename;
        }
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
    }
}



vector<wstring> list_directory() {
    static int isFileHiddenEnabled;
    static int isFileSuperHiddenEnabled;
    static int isFileExtHidden;
    LPCWSTR path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    HKEY hKey;
    DWORD lResult = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    DWORD lResult2 = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    DWORD lResult3 = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwSize = NULL;
        DWORD dwSize2 = NULL;
        DWORD dwSize3 = NULL;
        lResult = RegGetValue(hKey, NULL, L"Hidden", RRF_RT_DWORD, NULL, NULL, &dwSize);
        lResult2 = RegGetValue(hKey, NULL, L"ShowSuperHidden", RRF_RT_DWORD, NULL, NULL, &dwSize2);
        lResult3 = RegGetValue(hKey, NULL, L"HideFileExt", RRF_RT_DWORD, NULL, NULL, &dwSize3);
        if (lResult == ERROR_SUCCESS && dwSize != NULL)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValue(hKey, NULL, L"Hidden", RRF_RT_DWORD, NULL, dwValue, &dwSize);
            isFileHiddenEnabled = *dwValue;
            free(dwValue);
        }
        if (lResult == ERROR_SUCCESS && dwSize != NULL)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValue(hKey, NULL, L"ShowSuperHidden", RRF_RT_DWORD, NULL, dwValue, &dwSize);
            isFileSuperHiddenEnabled = *dwValue;
            free(dwValue);
        }
        if (lResult == ERROR_SUCCESS && dwSize != NULL)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValue(hKey, NULL, L"HideFileExt", RRF_RT_DWORD, NULL, dwValue, &dwSize);
            isFileExtHidden = *dwValue;
            free(dwValue);
        }
        RegCloseKey(hKey);
    }
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
            shortpm.push_back({ NULL, NULL, NULL });
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden));
            listDirBuffer.push_back(full_path_truncated + wstring(findData.cFileName));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            shortIndex--;
            shortpm.pop_back();
            dir_list.pop_back();
            listDirBuffer.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                shortIndex--;
                shortpm.pop_back();
                dir_list.pop_back();
                listDirBuffer.pop_back();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path2, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            shortpm.push_back({ NULL, NULL, NULL });
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden));
            listDirBuffer.push_back(full_path2_truncated + wstring(findData.cFileName));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            shortIndex--;
            shortpm.pop_back();
            dir_list.pop_back();
            listDirBuffer.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                shortIndex--;
                shortpm.pop_back();
                dir_list.pop_back();
                listDirBuffer.pop_back();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path3, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            shortpm.push_back({ NULL, NULL, NULL });
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden));
            listDirBuffer.push_back(full_path3_truncated + wstring(findData.cFileName));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            shortIndex--;
            shortpm.pop_back();
            dir_list.pop_back();
            listDirBuffer.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                shortIndex--;
                shortpm.pop_back();
                dir_list.pop_back();
                listDirBuffer.pop_back();
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