#pragma once

namespace DirectDesktop
{
    extern HWND g_hWndTaskbar;
    extern bool g_editmode;
    extern bool g_searchopen;
    extern bool g_peek;
    extern BYTE* shellstate;
    extern DirectUI::DUIXmlParser* parser;

    extern DirectUI::NativeHWNDHost* wnd;
    extern DirectUI::NativeHWNDHost* searchwnd;
    void CreateSearchPage(bool WinAltQ);
    void DestroySearchPage();
}
