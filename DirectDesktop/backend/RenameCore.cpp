#include "pch.h"

#include "RenameCore.h"
#include "..\coreui\StyleModifier.h"
#include "..\DirectDesktop.h"
#include <Richedit.h>
#include <textserv.h>

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    wstring RemoveEndingSpaces(const wstring& str)
    {
        wstring newStr;
        size_t pos = str.find_first_not_of(L' ');
        size_t pos2 = str.find_last_not_of(L' ');
        if (pos != 0 || pos2 != wstring::npos) newStr = str.substr(pos, pos2 - pos + 1);
        else return L"";
        return newStr;
    }

    wstring RenameHelper(const wstring& fullname, const wstring& simplename)
    {
        wstring result;
        result.reserve(fullname.length());

        size_t pos = 0;
        size_t found = fullname.find(simplename);

        while (found != wstring::npos)
        {
            result.append(fullname, pos, found - pos);
            result += L"%s";
            pos = found + simplename.length();
            found = fullname.find(simplename, pos);
        }

        result.append(fullname, pos, fullname.length() - pos);
        return result;
    }

    void RenameCore(LVItem* selectedElement)
    {
        HRESULT hr{};
        HWND hRichEdit = FindWindowExW(wnd->GetHWND(), nullptr, MSFTEDIT_CLASS, nullptr);
        WCHAR* buffer = new WCHAR[256];
        wstring newText{};
        GetWindowTextW(hRichEdit, buffer, 256);
        if (wcscmp(buffer, selectedElement->GetSimpleFilename().c_str()) == 0)
        {
            delete[] buffer;
            return;
        }
        if (!buffer) hr = E_FAIL;
        else newText = RemoveEndingSpaces(buffer);
        wstring newFilenameBuffer = RenameHelper(selectedElement->GetFilename(), selectedElement->GetSimpleFilename());
        WCHAR* newFilename = new WCHAR[256];
        StringCchPrintfW(newFilename, 256, newFilenameBuffer.c_str(), newText.c_str());
        if (hr != E_FAIL && MoveFileW(RemoveQuotes(selectedElement->GetFilename()).c_str(), RemoveQuotes(newFilename).c_str()))
        {
            SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, nullptr, nullptr);
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 300, nullptr);
        }
        delete[] newFilename;
        delete[] buffer;
    }

    int GetContentHeight(HWND hRichEdit)
    {
        int lineCount = (int)SendMessageW(hRichEdit, EM_GETLINECOUNT, 0, 0);
        HDC hdc = GetDC(hRichEdit);
        TEXTMETRICW tm{};
        LOGFONTW lf{};
        RECT rc = { 0, 0, 100, 100 };
        HDC hdcBuffer = CreateCompatibleDC(nullptr);
        SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, g_dpi);
        HFONT hFont = CreateFontIndirectW(&lf);
        HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
        DrawTextW(hdcBuffer, L" ", -1, &rc, g_touchmode ? DT_LEFT : DT_CENTER);
        GetTextMetricsW(hdcBuffer, &tm);
        SelectObject(hdcBuffer, hOldFont);
        DeleteObject(hFont);
        DeleteDC(hdcBuffer);
        int lineHeight = (tm.tmHeight + tm.tmExternalLeading);
        ReleaseDC(hRichEdit, hdc);
        if (g_touchmode) lineHeight *= 1.25;
        return (lineCount * lineHeight);
    }

    void ResizeToContent(HWND hRichEdit)
    {
        CValuePtr v;
        CSafeElementPtr<Element> DUIElem; DUIElem.Assign(regElem<Element*>(L"RenameBoxTexture", UIContainer));
        RECT rc, rcPadding;
        rcPadding = *(DUIElem->GetPadding(&v));
        GetWindowRect(hRichEdit, &rc);
        int currentWidth = rc.right - rc.left;
        int newHeight = GetContentHeight(hRichEdit);
        SetWindowPos(hRichEdit, nullptr, 0, 0, currentWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER);
        DUIElem->SetHeight(newHeight + rcPadding.top + rcPadding.bottom);
    }

    LRESULT CALLBACK RichEditWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        CSafeElementPtr<Element> DUIElem{}, innerElement{};
        CSafeElementPtr<RichText> textElement{}, textElementShadow{};
        static bool dontkill{};
        if (g_renameactive)
        {
            switch (uMsg)
            {
                case WM_KEYDOWN:
                    if (wParam == VK_RETURN)
                    {
                        RenameCore((LVItem*)dwRefData);
                        SendMessageW(hWnd, WM_USER + 1, NULL, NULL);
                        return 0;
                    }
                    if (wParam == VK_ESCAPE)
                    {
                        g_renameactive = false;
                        DUIElem.Assign(regElem<Element*>(L"RenameBoxTexture", UIContainer));
                        DestroyWindow(hWnd);
                        textElement.Assign(regElem<RichText*>(L"textElem", (Element*)dwRefData));
                        textElementShadow.Assign(regElem<RichText*>(L"textElemShadow", (Element*)dwRefData));
                        innerElement.Assign(regElem<RichText*>(L"innerElem", (Element*)dwRefData));
                        textElement->SetVisible(true);
                        if (!g_touchmode)
                        {
                            textElementShadow->SetVisible(true);
                            innerElement->SetVisible(true);
                        }
                        if (DUIElem)
                        {
                            DUIElem->DestroyAll(true);
                            DUIElem->Destroy(true);
                        }
                        return 0;
                    }
                case WM_PASTE:
                case WM_CHAR:
                    if (wcschr(L"\\/:*?\"<>|", (WCHAR)wParam) != nullptr)
                    {
                        SetTimer(hWnd, 3, 50, nullptr);
                        return 0;
                        break;
                    }
                    SetTimer(hWnd, 1, 150, nullptr);
                    break;
                case WM_TIMER:
                    KillTimer(hWnd, wParam);
                    switch (wParam)
                    {
                        case 1:
                            ResizeToContent(hWnd);
                            break;
                        case 2:
                            SendMessageW(hWnd, WM_USER + 1, NULL, NULL);
                            break;
                        case 3:
                            MessageBeep(MB_OK);
                            DDNotificationBanner* ddnb{};
                            DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_WARNING, L"DDNB", nullptr, LoadStrFromRes(4109, L"shell32.dll").c_str(), 5, false);
                            break;
                    }
                    break;
                case WM_KILLFOCUS:
                    WCHAR className[256];
                    GetClassNameW((HWND)wParam, className, 256);
                    if (wcscmp(className, L"DD_NotificationHost") != 0)
                    {
                        RenameCore((LVItem*)dwRefData);
                        SendMessageW(hWnd, WM_USER + 1, NULL, NULL);
                    }
                    else SetFocus(hWnd);
                    break;
                case WM_NCDESTROY:
                    RemoveWindowSubclass(hWnd, RichEditWindowProc, uIdSubclass);
                    break;
                case WM_USER + 1:
                    DUIElem.Assign(regElem<Element*>(L"RenameBoxTexture", UIContainer));
                    DestroyWindow(hWnd);
                    textElement.Assign(regElem<RichText*>(L"textElem", (Element*)dwRefData));
                    textElementShadow.Assign(regElem<RichText*>(L"textElemShadow", (Element*)dwRefData));
                    innerElement.Assign(regElem<RichText*>(L"innerElem", (Element*)dwRefData));
                    textElement->SetVisible(true);
                    if (!g_touchmode)
                    {
                        textElementShadow->SetVisible(true);
                        innerElement->SetVisible(true);
                    }
                    if (DUIElem)
                    {
                        DUIElem->DestroyAll(true);
                        DUIElem->Destroy(true);
                    }
                    g_renameactive = false;
                    break;
            }
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void ShowRename()
    {
        int itemWidth{}, itemHeight{}, itemX{}, itemY{}, textWidth{}, textHeight{}, textX{}, textY{};
        int found{};
        for (int items = 0; items < pm.size(); items++)
        {
            if (pm[items]->GetSelected() == true)
            {
                static RichText* textElement{};
                static RichText* textElementShadow{};
                static Element* innerElement{};
                static Element* RenameBoxTexture{};
                found++;
                if (found > 1)
                {
                    HWND hRichEdit = FindWindowExW(wnd->GetHWND(), nullptr, MSFTEDIT_CLASS, nullptr);
                    SetTimer(hRichEdit, 2, 250, nullptr);
                    MessageBeep(MB_OK);
                    DDNotificationBanner* ddnb{};
                    DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", nullptr, LoadStrFromRes(4041).c_str(), 3, false);
                    break;
                }
                if (pm[items]->GetPage() == g_currentPageID)
                {
                    RECT dimensions{};
                    GetClientRect(wnd->GetHWND(), &dimensions);
                    int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
                    g_renameactive = true;
                    textElement = regElem<RichText*>(L"textElem", pm[items]);
                    textElementShadow = regElem<RichText*>(L"textElemShadow", pm[items]);
                    innerElement = regElem<RichText*>(L"innerElem", pm[items]);
                    textElement->SetVisible(false);
                    if (!g_touchmode)
                    {
                        textElementShadow->SetVisible(false);
                        innerElement->SetVisible(false);
                    }
                    LoadLibraryW(L"Msftedit.dll");
                    itemWidth = pm[items]->GetWidth();
                    itemHeight = pm[items]->GetHeight();
                    itemX = (localeType == 1) ? dimensions.right - pm[items]->GetX() - itemWidth : pm[items]->GetX();
                    itemY = pm[items]->GetY();
                    // 0.4.9: width, x, and y are hardcoded due to changing layoutpos of text from absolute to top in touchmode
                    textWidth = g_touchmode ? textElement->GetWidth() - 6 * g_flScaleFactor : textElement->GetWidth();
                    textHeight = textElement->GetHeight() + 4 * g_flScaleFactor;
                    textX = g_touchmode ? 7 * g_flScaleFactor : 0;
                    textY = g_touchmode ? 8 * g_flScaleFactor : 0;
                    unsigned long keyR{};
                    CValuePtr v;
                    parser->CreateElement(L"RenameBoxTexture", nullptr, nullptr, nullptr, &RenameBoxTexture);
                    UIContainer->Add(&RenameBoxTexture, 1);
                    RECT ebsz{}, rcPadding{};
                    ebsz.left = itemX, ebsz.top = itemY + itemHeight - textHeight, ebsz.right = itemWidth, ebsz.bottom = textHeight;
                    rcPadding = *(RenameBoxTexture->GetPadding(&v));
                    if (g_touchmode)
                        ebsz.left = itemX + textX - rcPadding.left, ebsz.top = itemY + textY - rcPadding.top, ebsz.right = textWidth + rcPadding.left, ebsz.bottom = textHeight;
                    RenameBoxTexture->SetX(ebsz.left), RenameBoxTexture->SetY(ebsz.top);
                    RenameBoxTexture->SetWidth(ebsz.right), RenameBoxTexture->SetHeight(ebsz.bottom);
                    LPWSTR sheetName = g_theme ? (LPWSTR)L"renamestyle" : (LPWSTR)L"renamestyledark";
                    StyleSheet* sheet = pMain->GetSheet();
                    CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                    parser->GetSheet(sheetName, &sheetStorage);
                    RenameBoxTexture->SetValue(Element::SheetProp, 1, sheetStorage);
                    DWORD alignment = g_touchmode ? (localeType == 1) ? ES_RIGHT : ES_LEFT : ES_CENTER;
                    HWND hRichEdit = CreateWindowExW(NULL, MSFTEDIT_CLASS, pm[items]->GetSimpleFilename().c_str(), WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL | alignment,
                                                     ebsz.left + rcPadding.left, ebsz.top + rcPadding.top, ebsz.right - rcPadding.left - rcPadding.right, ebsz.bottom - rcPadding.top - rcPadding.bottom, wnd->GetHWND(), (HMENU)2050, HINST_THISCOMPONENT, nullptr);
                    LOGFONTW lf{};
                    int dpiAdjusted = (g_dpiLaunch * 96.0) * (g_dpiLaunch / 96.0) / g_dpi;
                    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL);
                    CHARFORMAT2W cf{};
                    cf.cbSize = sizeof(CHARFORMAT2W);
                    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR | CFM_BOLD | CFM_ITALIC;
                    cf.yHeight = (lf.lfHeight * -15 * 96.0) / dpiAdjusted;
                    if (g_touchmode) cf.yHeight *= 1.25;
                    cf.crTextColor = g_theme ? GetSysColor(COLOR_WINDOWTEXT) : RGB(255, 255, 255);
                    wcscpy_s(cf.szFaceName, lf.lfFaceName);
                    if (lf.lfWeight == FW_BOLD) cf.dwEffects |= CFE_BOLD;
                    if (lf.lfItalic) cf.dwEffects |= CFE_ITALIC;

                    COLORREF editbg = g_theme ? GetSysColor(COLOR_WINDOW) : RGB(32, 32, 32);
                    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
                    SendMessageW(hRichEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)editbg);
                    SetWindowLongPtrW(hRichEdit, GWL_EXSTYLE, 0xC0000A40L | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP);
                    SetLayeredWindowAttributes(hRichEdit, 0, 255, LWA_ALPHA);
                    if (g_touchmode)
                        SetWindowPos(hRichEdit, HWND_TOP, itemX + textX, itemY + textY, NULL, NULL, SWP_NOSIZE | SWP_SHOWWINDOW);
                    else 
                        SetWindowPos(hRichEdit, HWND_TOP, itemX + rcPadding.left, itemY + itemHeight - textHeight + rcPadding.top, NULL, NULL, SWP_NOSIZE | SWP_SHOWWINDOW);
                    SetFocus(hRichEdit);
                    int textLen = pm[items]->GetSimpleFilename().find_last_of(L".");
                    if (textLen == wstring::npos) textLen = pm[items]->GetSimpleFilename().length();
                    SendMessageW(hRichEdit, EM_SETSEL, 0, textLen);
                    SetWindowSubclass(hRichEdit, RichEditWindowProc, 0, (DWORD_PTR)pm[items]);
                }
            }
        }
    }
}
