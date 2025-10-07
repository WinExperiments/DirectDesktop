#pragma once

#include "DDControls.h"

using namespace DirectUI;

namespace DirectDesktop
{
    extern bool g_editmode;
    extern bool g_invokedpagechange;
    extern bool g_hiddenIcons;
    extern bool g_pageviewer;
    extern std::vector<LVItem*> pm;
    extern struct DesktopIcon;
    extern DUIXmlParser* parser;
    extern Element* pEdit;
    extern NativeHWNDHost* wnd;
    extern NativeHWNDHost* editwnd;
    extern NativeHWNDHost* editbgwnd;
    extern HWND g_hWndTaskbar, g_hSHELLDLL_DefView;
    extern Element* mainContainer, *UIContainer;
    extern Button* PageViewer;
    extern int localeType;
    extern int g_currentPageID, g_maxPageID;
    extern void SetPopupSize(Element* elem, int width, int height);
    extern void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
    extern void ApplyIcons(vector<LVItem*> pmLVItem, DesktopIcon* di, bool subdirectory, int id, float scale, COLORREF crSubdir);
    extern TouchButton *nextpageMain, *prevpageMain;
    extern DDScalableTouchButton *nextpage, *prevpage;
    extern DDScalableRichText* pageinfo;
    extern DDScalableElement* fullscreeninnerE;

    extern void testEventListener3(Element* elem, Event* iev);
    extern void ShowSettings(Element* elem, Event* iev);
    extern void GoToPrevPage(Element* elem, Event* iev);
    extern void GoToNextPage(Element* elem, Event* iev);
    extern DWORD WINAPI AnimateWindowWrapper2(LPVOID lpParam);

    void ShowSimpleView(bool animate, DWORD animFlags);
    void RefreshSimpleView(DWORD animFlags);
    void HideSimpleView(bool animate);
    void TriggerEMToPV(bool fReverse);
    float CalcAnimOrigin(float flOriginFrom, float flOriginTo, float flScaleFrom, float flScaleTo);
}
