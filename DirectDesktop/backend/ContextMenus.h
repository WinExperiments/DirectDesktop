#pragma once

#include <string>
#include "..\framework.h"

#include "..\Include\dui70\DirectUI\DirectUI.h"

#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 30000

using namespace DirectUI;

namespace DirectDesktop
{
    extern void InitLayout(bool bUnused1, bool bUnused2, bool bAlreadyOpen);
    extern void RearrangeIcons(bool animation, bool reloadicons, bool bAlreadyOpen);
    extern bool g_hiddenIcons;
    extern bool g_touchmode;
    extern int localeType;
    extern NativeHWNDHost* wnd;


    void SetView(int iconsz, int shiconsz, int gpiconsz, bool touch);
    void DesktopRightClick(Element* elem, Event* iev);
    void ItemRightClick(Element* elem, Event* iev);
}
