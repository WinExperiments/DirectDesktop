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

    wstring RenameHelper(LVItem* lvi)
    {
        wstring result;
        result.reserve(lvi->GetFilename().length());

        size_t pos = 0;
        size_t found = wstring::npos;
        if (lvi->GetShortcutState()) found = lvi->GetFilename().rfind(L".");
        size_t found2 = lvi->GetFilename().rfind(lvi->GetSimpleFilename(), found);

        while (found2 != wstring::npos)
        {
            result.append(lvi->GetFilename(), pos, found2 - pos);
            result += L"%s";
            pos = found2 + lvi->GetSimpleFilename().length();
            found2 = lvi->GetFilename().find(lvi->GetSimpleFilename(), pos);
            break;
        }

        result.append(lvi->GetFilename(), pos, lvi->GetFilename().length() - pos);
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
        wstring newFilenameBuffer = RenameHelper(selectedElement);
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
        DUIElem->GetRenderPadding(&rcPadding);
        GetWindowRect(hRichEdit, &rc);
        int currentWidth = rc.right - rc.left;
        int newHeight = GetContentHeight(hRichEdit);
        SetWindowPos(hRichEdit, nullptr, 0, 0, currentWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER);
        DUIElem->SetHeight(newHeight + rcPadding.top + rcPadding.bottom + 1);
    }

    LRESULT CALLBACK RichEditWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        CSafeElementPtr<Element> DUIElem{}, innerElement{};
        CSafeElementPtr<RichText> textElement{};
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
                        textElement.Assign(((LVItem*)dwRefData)->GetText());
                        innerElement.Assign(regElem<RichText*>(L"innerElem", (Element*)dwRefData));
                        textElement->SetVisible(true);
                        if (!g_touchmode)
                            innerElement->SetVisible(true);
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
                        SetTimer(hWnd, 2, 50, nullptr);
                        return 0;
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
                            MessageBeep(MB_OK);
                            CSafeElementPtr<DDNotificationBanner> ddnb;
                            ddnb.Assign(new DDNotificationBanner);
                            ddnb->CreateBanner(DDNT_WARNING, nullptr, LoadStrFromRes(4109, L"shell32.dll").c_str(), 5);
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
                    textElement.Assign(((LVItem*)dwRefData)->GetText());
                    innerElement.Assign(regElem<RichText*>(L"innerElem", (Element*)dwRefData));
                    textElement->SetVisible(true);
                    if (!g_touchmode)
                        innerElement->SetVisible(true);
                    else if (((LVItem*)dwRefData)->GetTileSize() == LVITS_ICONONLY)
                    {
                        textElement->SetVisible(false);
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
        int itemID{};
        for (int items = 0; items < pm.size(); items++)
        {
            if (pm[items]->GetSelected() == true)
            {
                found++;
                if (found > 1)
                {
                    MessageBeep(MB_OK);
                    CSafeElementPtr<DDNotificationBanner> ddnb;
                    ddnb.Assign(new DDNotificationBanner);
                    ddnb->CreateBanner(DDNT_ERROR, nullptr, LoadStrFromRes(4041).c_str(), 3);
                    break;
                }
                itemID = items;
            }
        }
        if (found == 1)
        {
            static RichText* textElement{};
            static Element* innerElement{};
            static Element* RenameBoxTexture{};
            if (pm[itemID]->GetPage() == g_currentPageID)
            {
                RECT dimensions{};
                GetClientRect(wnd->GetHWND(), &dimensions);
                int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
                g_renameactive = true;
                textElement = pm[itemID]->GetText();
                innerElement = regElem<RichText*>(L"innerElem", pm[itemID]);
                textElement->SetVisible(false);
                if (!g_touchmode)
                    innerElement->SetVisible(false);
                LoadLibraryW(L"Msftedit.dll");
                itemWidth = pm[itemID]->GetWidth();
                itemHeight = pm[itemID]->GetHeight();
                itemX = (localeType == 1) ? dimensions.right - pm[itemID]->GetMemXPos() - itemWidth : pm[itemID]->GetMemXPos();
                itemY = pm[itemID]->GetMemYPos();
                // 0.4.9: width, x, and y are hardcoded due to changing layoutpos of text from absolute to top in touchmode
                textWidth = g_touchmode ? textElement->GetWidth() - 6 * g_flScaleFactor : textElement->GetWidth();
                // 0.5.2: Text element now contains the shadow, so the height increased. As a result, 4 is now 2
                textHeight = textElement->GetHeight() + 2 * g_flScaleFactor;
                textX = g_touchmode ? 11 * g_flScaleFactor : 0;
                textY = g_touchmode ? 10 * g_flScaleFactor : 0;
                unsigned long keyR{};
                CValuePtr v;
                parser->CreateElement(L"RenameBoxTexture", nullptr, nullptr, nullptr, &RenameBoxTexture);
                UIContainer->Add(&RenameBoxTexture, 1);
                if (DWMActive)
                {
                    AddLayeredRef(RenameBoxTexture->GetDisplayNode());
                    SetGadgetFlags(RenameBoxTexture->GetDisplayNode(), NULL, NULL);
                }
                RECT ebsz{}, rcPadding{};
                ebsz.left = itemX, ebsz.top = itemY + itemHeight - textHeight, ebsz.right = itemWidth, ebsz.bottom = textHeight;
                RenameBoxTexture->GetRenderPadding(&rcPadding);
                if (g_touchmode)
                    ebsz.left = itemX + textX - rcPadding.left, ebsz.top = itemY + textY - rcPadding.top, ebsz.right = textWidth + rcPadding.left, ebsz.bottom = textHeight;
                int xRender = (localeType == 1) ? dimensions.right - ebsz.left - ebsz.right : ebsz.left;
                RenameBoxTexture->SetX(xRender), RenameBoxTexture->SetY(ebsz.top);
                RenameBoxTexture->SetWidth(ebsz.right), RenameBoxTexture->SetHeight(ebsz.bottom);
                LPWSTR sheetName = g_theme ? (LPWSTR)L"renamestyle" : (LPWSTR)L"renamestyledark";
                StyleSheet* sheet = pMain->GetSheet();
                CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                parser->GetSheet(sheetName, &sheetStorage);
                RenameBoxTexture->SetValue(Element::SheetProp, 1, sheetStorage);
                DWORD alignment = g_touchmode ? (localeType == 1) ? ES_RIGHT : ES_LEFT : ES_CENTER;
                HWND hRichEdit = CreateWindowExW(NULL, MSFTEDIT_CLASS, pm[itemID]->GetSimpleFilename().c_str(), WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL | alignment,
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
                int textLen = pm[itemID]->GetSimpleFilename().find_last_of(L".");
                if (textLen == wstring::npos) textLen = pm[itemID]->GetSimpleFilename().length();
                SendMessageW(hRichEdit, EM_SETSEL, 0, textLen);
                SetWindowSubclass(hRichEdit, RichEditWindowProc, 0, (DWORD_PTR)pm[itemID]);
            }
        }
    }
}
