#pragma once

using namespace DirectUI;

namespace DirectDesktop
{
    extern bool g_debugmode;
    extern bool g_showcheckboxes;
    extern bool g_treatdirasgroup;
    extern bool g_tripleclickandhide;
    extern bool g_lockiconpos;
    extern bool g_isColorized;
    extern bool g_isColorizedOld;
    extern bool g_isDarkIconsEnabled;
    extern bool g_automaticDark;
    extern bool g_isGlass;
    extern BYTE iconColorID;
    extern COLORREF IconColorizationColor;
    extern bool g_atleastonesetting;
    extern int localeType;

    extern Element* UIContainer;
    extern NativeHWNDHost* wnd;
    extern void RearrangeIcons(bool animation, bool reloadicons, bool bAlreadyOpen);
    extern void InitLayout(bool bUnused1, bool bUnused2, bool bAlreadyOpen);

    void ToggleSetting(Element* elem, Event* iev);
    POINT GetTopLeftMonitor();
    int GetRightMonitor();
}
