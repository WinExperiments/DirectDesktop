#pragma once
#include "framework.h"
#include "DirectUI/DirectUI.h"

#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 30000

using namespace DirectUI;

extern void InitLayout();
extern void ShowSimpleView();
extern void RearrangeIcons(bool animation, bool reloadicons);
extern bool hiddenIcons;
extern int globaliconsz, globalshiconsz, globalgpiconsz;
extern int currentPageID;
extern NativeHWNDHost* wnd;


void DesktopRightClick(Element* elem, Event* iev);
void ItemRightClick(Element* elem, Event* iev);