#pragma once
#include "framework.h"
#include "DirectUI/DirectUI.h"

using namespace DirectUI;

extern bool showcheckboxes;
extern bool treatdirasgroup;
extern bool tripleclickandhide;
extern bool lockiconpos;
extern bool isColorized;
extern bool atleastonesetting;
extern int localeType;

extern Element* UIContainer;
extern NativeHWNDHost* wnd;
extern void RearrangeIcons(bool animation, bool reloadicons);
extern void InitLayout(bool bUnused1, bool bUnused2);

void ToggleSetting(Element* elem, Event* iev);
POINT GetTopLeftMonitor();
int GetRightMonitor();