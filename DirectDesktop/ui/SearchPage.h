#pragma once
#include "..\DirectDesktop.h"

namespace DirectDesktop
{
    struct SearchParams
    {
        DWORD flags;
        LPWSTR path;
    };

    extern HWND g_hWndTaskbar;
    extern bool g_editmode;
    extern bool g_searchopen;
    extern bool g_peek;
    extern BYTE* shellstate;
    extern DirectUI::DUIXmlParser* parser;
    extern DirectUI::NativeHWNDHost* wnd;
    extern DirectUI::NativeHWNDHost* searchwnd;
    extern void ApplyIcons(vector<DDUI::LVItem*>* pmLVItem, DesktopIcon* di, bool subdirectory, int id, float scale, COLORREF crSubdir);

    void CreateSearchPage(SearchParams* psp);
    void DestroySearchPage();
}
