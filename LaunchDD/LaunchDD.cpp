#define WIN32_LEAN_AND_MEAN 

#include <Windows.h>
#include <commctrl.h>
#include <pathcch.h>
#include <shobjidl_core.h>
#include <strsafe.h>

void GetRelativeExePath(LPWSTR& pszDir, LPWSTR& pszExe, LPCWSTR pszExeName)
{
    pszDir = new WCHAR[260];
    pszExe = new WCHAR[260];
    GetModuleFileNameW(NULL, pszDir, 260);
    PathCchRemoveFileSpec(pszDir, 260);
    StringCchPrintfW(pszExe, 260, L"\"%s\\%s\"", pszDir, pszExeName);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    HWND hwnd = GetConsoleWindow();
    if (hwnd)
        ShowWindow(hwnd, SW_HIDE);

    HMODULE hModDDUI = LoadLibraryW(L"DDUI.dll");
    if (!hModDDUI)
    {
        TaskDialog(NULL, NULL, L"DirectDesktop Launcher", L"Failed to launch DirectDesktop",
            L"The following modules are missing:\n\n\tDDUI.dll", TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
        return 1;
    }

    DWORD exitCode = 0;
    ULONGLONG ullTick = 0;

    while (true)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        LPWSTR pszParentPath{}, pszExePath{};
        GetRelativeExePath(pszParentPath, pszExePath, L"DirectDesktop.exe");

        WCHAR command[320];
        StringCchPrintfW(command, 320, L"%s -c %lu", pszExePath, exitCode);

        ullTick = GetTickCount64();
        if (!CreateProcessW(nullptr, command, nullptr, nullptr, FALSE, NULL, nullptr, pszParentPath, &si, &pi)) 
        {
            TaskDialog(NULL, NULL, L"DirectDesktop Launcher", L"Failed to launch DirectDesktop",
                L"Cannot find executable.", TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
            return 1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);
        delete[] pszParentPath;
        delete[] pszExePath;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (ullTick > GetTickCount64() - 5000)
            break;
        if (exitCode < 2)
            break;
        Sleep(1000); 
    }
    return 0;
}