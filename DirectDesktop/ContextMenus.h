#pragma once
#include <string>
#include "framework.h"
#include "DirectUI/DirectUI.h"

#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 30000

using namespace DirectUI;

extern void InitLayout(bool bUnused1, bool bUnused2);
extern void ShowSimpleView();
extern void RearrangeIcons(bool animation, bool reloadicons);
extern std::wstring LoadStrFromRes(UINT id);
extern std::wstring LoadStrFromRes(UINT id, LPCWSTR dllName);
extern bool hiddenIcons;
extern int globaliconsz, globalshiconsz, globalgpiconsz;
extern bool touchmode;
extern int currentPageID;
extern int localeType;
extern NativeHWNDHost* wnd;


void DesktopRightClick(Element* elem, Event* iev);
void ItemRightClick(Element* elem, Event* iev);
void SubItemRightClick(Element* elem, Event* iev);