#pragma once

namespace DirectDesktop
{
    extern HWND g_hWndTaskbar;
    extern DirectUI::Element* UIContainer;
    extern bool g_theme;
    extern bool g_searchopen;
    extern DirectUI::DUIXmlParser* parser;

    extern DirectUI::NativeHWNDHost* wnd;
    extern DirectUI::NativeHWNDHost* searchwnd;
    void CreateSearchPage(bool WinAltQ);
    void DestroySearchPage();
}
