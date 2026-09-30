#define WIN32_LEAN_AND_MEAN 

#include <Windows.h>
#include <commctrl.h>
#include <pathcch.h>
#include <string>
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

UINT g_umTaskbar = RegisterWindowMessageW(L"TaskbarCreated");
HANDLE g_semaphore = CreateSemaphoreW(nullptr, 1, 1, nullptr);

LRESULT CALLBACK LauncherProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_umTaskbar)
    {
        ReleaseSemaphore(g_semaphore, 1, nullptr);
        DestroyWindow(hWnd);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HANDLE GetShellProcessHandle()
{
    HWND hWndShell = GetShellWindow();
    if (!hWndShell) return nullptr;
    DWORD dwShellPID = 0;
    GetWindowThreadProcessId(hWndShell, &dwShellPID);
    if (!dwShellPID) return nullptr;
    return OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwShellPID);
}

DWORD WINAPI StartExplorerLaunchListener(LPVOID lpParam)
{
    WNDCLASS wc = { 0 };
    wc.hInstance = (HINSTANCE)lpParam;
    wc.lpfnWndProc = LauncherProc;
    wc.lpszClassName = L"DDLauncherListener";
    RegisterClassW(&wc);

    HWND hWndListener = CreateWindowExW(NULL, wc.lpszClassName, nullptr, WS_POPUP, 0, 0, 0, 0, NULL, NULL, (HINSTANCE)lpParam, nullptr);

    if (!hWndListener)
    {
        ReleaseSemaphore(g_semaphore, 1, nullptr);
        return 1;
    }

    ChangeWindowMessageFilterEx(hWndListener, g_umTaskbar, MSGFLT_ALLOW, nullptr);
    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    UnregisterClassW(wc.lpszClassName, (HINSTANCE)lpParam);
    return 0;
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
        WaitForSingleObject(g_semaphore, INFINITE);
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

        HANDLE handles[2];
        handles[0] = pi.hProcess;
        handles[1] = GetShellProcessHandle();

        DWORD dwWait = WaitForMultipleObjects(2, handles, false, INFINITE);
        switch (dwWait)
        {
        case WAIT_OBJECT_0 + 0:
            GetExitCodeProcess(pi.hProcess, &exitCode);
            break;
        case WAIT_OBJECT_0 + 1:
        {
            Sleep(250);
            HWND hWndDD = FindWindowW(L"DD_SubviewHost", L"DirectDesktop Subview");
            if (hWndDD)
            {
                SendMessageTimeoutW(hWndDD, WM_CLOSE, NULL, 69, SMTO_ABORTIFHUNG, 3000, nullptr);
                HANDLE hThread = CreateThread(nullptr, 0, StartExplorerLaunchListener, (LPVOID)hInstance, NULL, nullptr);
                if (hThread) CloseHandle(hThread);
                WaitForSingleObject(g_semaphore, INFINITE);
                exitCode = 3;
            }
            break;
        }
        }
        delete[] pszParentPath;
        delete[] pszExePath;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (ullTick > GetTickCount64() - 5000 && dwWait != WAIT_OBJECT_0 + 1)
            break;
        if (exitCode < 2)
            break;
        Sleep(1000); 
        ReleaseSemaphore(g_semaphore, 1, nullptr);
    }
    return 0;
}