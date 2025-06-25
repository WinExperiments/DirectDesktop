#pragma once
#include "framework.h"
#include "Include\dui70\DirectUI\DirectUI.h"

using namespace DirectUI;

extern bool showcheckboxes;
extern bool treatdirasgroup;
extern bool tripleclickandhide;
extern bool lockiconpos;
extern bool isColorized;
extern bool isDarkIconsEnabled;
extern bool automaticDark;
extern bool isGlass;
extern BYTE iconColorID;
extern COLORREF IconColorizationColor;
extern bool atleastonesetting;
extern int localeType;

extern Element* UIContainer;
extern NativeHWNDHost* wnd;
extern void RearrangeIcons(bool animation, bool reloadicons, bool bAlreadyOpen);
extern void InitLayout(bool bUnused1, bool bUnused2, bool bAlreadyOpen);

void ToggleSetting(Element* elem, Event* iev);
POINT GetTopLeftMonitor();
int GetRightMonitor();