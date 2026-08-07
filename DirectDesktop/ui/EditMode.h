#pragma once

#include <vector>

using namespace DirectUI;
using namespace DDUI;

namespace DirectDesktop
{
    extern bool g_editmode;
    extern bool g_editavailable;
    extern bool g_invokedpagechange;
    extern bool g_hiddenIcons;
    extern bool g_pageviewer;
    extern std::vector<DDUI::LVItem*> pm;
    extern struct DesktopIcon;
    extern DUIXmlParser* parser;
    extern Element* pEdit;
    extern NativeHWNDHost* wnd;
    extern NativeHWNDHost* editwnd;
    extern NativeHWNDHost* editbgwnd;
    extern HWND g_hWndTaskbar, g_hWorkerW, g_hSHELLDLL_DefView;
    extern Element* mainContainer;
    extern Element* PageViewer;
    extern int g_currentPageID, g_maxPageID;
    extern void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
    extern void ApplyIcons(std::vector<DDUI::LVItem*>* pmLVItem, DesktopIcon* di, bool subdirectory, int id, float scale, COLORREF crSubdir, bool fShadow);
    extern TouchButton *nextpageMain, *prevpageMain;
    extern DDScalableTouchButton *nextpage, *prevpage;
    extern DDScalableRichText* pageinfo;
    extern DDScalableTouchButton* fullscreeninnerE;
    extern Element* centeredE;

    extern void testEventListener3(Element* elem, Event* iev);
    extern void ShowSettings(Element* elem, Event* iev);
    extern void GoToPrevPage(Element* elem, Event* iev);
    extern void GoToNextPage(Element* elem, Event* iev);
    extern DWORD WINAPI AnimateWindowWrapper2(LPVOID lpParam);

    void ShowSimpleView(bool animate, DWORD animFlags);
    void RefreshSimpleView(DWORD animFlags);
    void HideSimpleView(bool fullanimate);
    void TriggerEMToPV(bool fReverse);
    void TriggerNoMorePagesOnEdit();
}
