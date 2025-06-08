#pragma once
#include "Include\dui70\DirectUI\DirectUI.h"
#include "DDControls.h"

using namespace DirectUI;

extern bool editmode;
extern bool invokedpagechange;
extern NativeHWNDHost* wnd;
extern NativeHWNDHost* editwnd;
extern NativeHWNDHost* editbgwnd;
extern HWND hWndTaskbar, hSHELLDLL_DefView;
extern Element* mainContainer;
extern int localeType;
extern int currentPageID, maxPageID;
extern void PlaySimpleViewAnimation(Element* elem, int width, int height, int animation, float startscale);
extern void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
extern TouchButton* nextpage, *prevpage;
extern RichText* pageinfo;

extern void testEventListener3(Element* elem, Event* iev);
extern void ShowSettings(Element* elem, Event* iev);
extern void GoToPrevPage(Element* elem, Event* iev);
extern void GoToNextPage(Element* elem, Event* iev);
extern unsigned long AnimateWindowWrapper2(LPVOID lpParam);

void ShowSimpleView(bool animate);
void HideSimpleView(bool animate);