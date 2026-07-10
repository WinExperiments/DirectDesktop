#pragma once
#pragma warning(disable:6258)
#pragma warning(disable:28159)

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    extern DWORD shutdownReason;
    extern NativeHWNDHost* wnd;
    extern NativeHWNDHost* shutdownwnd;
    extern bool g_dialogopen;
    extern bool delayedshutdownstatuses[6];

    void DisplayShutdownDialog();
    void DestroyShutdownDialog();
}
