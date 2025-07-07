#pragma once

#include "DDControls.h"

using namespace DirectUI;

namespace DirectDesktop
{
	extern bool g_editmode;
	extern bool g_invokedpagechange;
	extern bool g_hiddenIcons;
	extern std::vector<LVItem*> pm;
	extern std::vector<DDScalableElement*> iconpm;
	extern std::vector<Element*> shadowpm, shortpm;
	extern struct DesktopIcon;
	extern DUIXmlParser* parser;
	extern Element* pEdit;
	extern NativeHWNDHost* wnd;
	extern NativeHWNDHost* editwnd;
	extern NativeHWNDHost* editbgwnd;
	extern HWND g_hWndTaskbar, g_hSHELLDLL_DefView;
	extern Element* mainContainer;
	extern int localeType;
	extern int g_currentPageID, g_maxPageID;
	extern void PlaySimpleViewAnimation(Element* elem, int width, int height, int animation, float startscale);
	extern void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
	extern HBITMAP GetShellItemImage(LPCWSTR filePath, int width, int height);
	extern void ApplyIcons(vector<LVItem*> pmLVItem, vector<DDScalableElement*> pmIcon, DesktopIcon* di, bool subdirectory, int id, float scale);
	extern TouchButton* nextpage, *prevpage;
	extern DDScalableRichText* pageinfo;

	extern void testEventListener3(Element* elem, Event* iev);
	extern void ShowSettings(Element* elem, Event* iev);
	extern void GoToPrevPage(Element* elem, Event* iev);
	extern void GoToNextPage(Element* elem, Event* iev);
	extern DWORD WINAPI AnimateWindowWrapper2(LPVOID lpParam);

	void ShowSimpleView(bool animate);
	void HideSimpleView(bool animate);
}