#include "pch.h"

#include "RenameCore.h"
#include "..\coreui\StyleModifier.h"
#include "..\DirectDesktop.h"

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
        wstring fileNameNoQuote = RemoveQuotes(lvi->GetFilename());
        wstring result;
        result.reserve(fileNameNoQuote.length());

        size_t pos = 0;
        size_t found = wstring::npos;
        if (lvi->GetFlags() & LVIF_SHORTCUT) found = fileNameNoQuote.rfind(L".");
        size_t found2 = fileNameNoQuote.rfind(lvi->GetSimpleFilename(), found);

        if (found2 != wstring::npos)
        {
            result.append(fileNameNoQuote, pos, found2 - pos);
            result += L"%s";
            pos = found2 + lvi->GetSimpleFilename().length();
            found2 = fileNameNoQuote.find(lvi->GetSimpleFilename(), pos);
        }

        result.append(fileNameNoQuote, pos, fileNameNoQuote.length() - pos);
        size_t found3 = result.find_last_of(L"\\") + 1;
        if (found3 != wstring::npos)
            result = result.substr(found3, wstring::npos);
        return result;
    }

    void RenameCore(LVItem* selectedElement)
    {
        HRESULT hr{};
        HWND hEdit = FindWindowExW(wnd->GetHWND(), nullptr, L"Edit", nullptr);
        WCHAR* buffer = new WCHAR[MAX_PATH];
        wstring newText{};
        GetWindowTextW(hEdit, buffer, MAX_PATH);
        if (wcscmp(buffer, selectedElement->GetSimpleFilename().c_str()) == 0 || wcslen(buffer) == 0)
        {
            delete[] buffer;
            return;
        }
        if (!buffer) hr = E_FAIL;
        else newText = RemoveEndingSpaces(buffer);
        wstring newFilenameBuffer = RenameHelper(selectedElement);
        WCHAR* newFilename = new WCHAR[MAX_PATH];
        StringCchPrintfW(newFilename, MAX_PATH, newFilenameBuffer.c_str(), newText.c_str());
        if (SUCCEEDED(hr))
        {
            IShellItem2* pShellItem{};
            hr = SHCreateItemFromParsingName(RemoveQuotes(selectedElement->GetFilename()).c_str(), nullptr, IID_PPV_ARGS(&pShellItem));
            if (SUCCEEDED(hr))
            {
                IFileOperation* pfo;
                hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, IID_IFileOperation, (LPVOID*)&pfo);
                if (SUCCEEDED(hr))
                {
                    pfo->SetOperationFlags(FOF_SILENT | FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR);
                    pfo->RenameItem(pShellItem, newFilename, nullptr);
                    pfo->PerformOperations();
                    pfo->Release();
                }
                pShellItem->Release();
            }
        }
        delete[] newFilename;
        delete[] buffer;
    }

    void GetContentRect(HWND hEdit, RECT* prcEdit, UINT uBound, UINT* puLines)
    {
        HDC hdcEdit = GetDC(hEdit);
        HFONT hFont = (HFONT)SendMessageW(hEdit, WM_GETFONT, NULL, NULL);
        HFONT hOldFont = (HFONT)SelectObject(hdcEdit, hFont);
        int len = GetWindowTextLengthW(hEdit);
        wchar_t* buffer = new wchar_t[len + 1];
        GetWindowTextW(hEdit, buffer, len + 1);
        DWORD dwFlags = DT_CALCRECT | DT_WORDBREAK;
        int lines = SendMessageW(hEdit, EM_GETLINECOUNT, NULL, NULL);
        if (!lines) lines = 1;
        if (puLines) *puLines = lines;
        if (prcEdit->right < uBound && lines < 2) dwFlags |= DT_SINGLELINE;
        else
        {
            prcEdit->right = uBound;
            dwFlags |= DT_EDITCONTROL;
        }
        DrawTextW(hdcEdit, buffer, -1, prcEdit, dwFlags);

        TEXTMETRICW tm;
        GetTextMetricsW(hdcEdit, &tm);
        int lineHeight = tm.tmHeight + tm.tmExternalLeading;
        prcEdit->bottom = prcEdit->top + lines * lineHeight;

        if (prcEdit->right < uBound)
        {
            prcEdit->left -= round(3 * g_flScaleFactor);
            prcEdit->right += round(3 * g_flScaleFactor);
        }
        delete[] buffer;
        SelectObject(hdcEdit, hOldFont);
        ReleaseDC(hEdit, hdcEdit);
    }

    void ResizeToContent(HWND hEdit, LVItem* lvi)
    {
        CValuePtr v;
        CSafeElementPtr<Element> DUIElem; DUIElem.Assign(regElem<Element*>(L"RenameBoxTexture", UIContainer));
        RECT rc{}, rcWindow, rcPadding, rcGadget;
        GetWindowRect(hEdit, &rcWindow);
        GetGadgetRect(lvi->GetDisplayNode(), &rcGadget, 0xC);
        DUIElem->GetRenderPadding(&rcPadding);
        if (lvi->GetGroupSize() == LVIGS_NORMAL)
        {
            int currentWidth = rcWindow.right - rcWindow.left;
            rc.right = rcWindow.right - rcWindow.left;
            UINT lines;
            GetContentRect(hEdit, &rc, g_touchmode ? 0 : rcGadget.right - rcGadget.left - rcPadding.left - rcPadding.right, &lines);
            if (g_touchmode)
                SetWindowPos(hEdit, nullptr, NULL, NULL, currentWidth, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
            else
            {
                SetWindowPos(hEdit, nullptr, rcGadget.left + (rcGadget.right - rcGadget.left - max(rc.right - rc.left, 16)) / 2,
                    rcWindow.top, max(rc.right - rc.left, 16), rc.bottom - rc.top, SWP_NOZORDER);
                DUIElem->SetX(rcGadget.left + (rcGadget.right - rcGadget.left - max(rc.right - rc.left, 16) - (rcPadding.left + rcPadding.right + 1)) / 2);
                DUIElem->SetWidth(max(rc.right - rc.left, 16) + rcPadding.left + rcPadding.right + 1);
            }
            DUIElem->SetHeight(rc.bottom - rc.top + rcPadding.top + rcPadding.bottom + 1);
            GetClientRect(hEdit, &rcWindow);
            rcWindow.left += round(g_flScaleFactor);
            rcWindow.right -= round(g_flScaleFactor);
            SendMessageW(hEdit, EM_SETRECT, NULL, (LPARAM)&rcWindow);
            int linesEdit = SendMessageW(hEdit, EM_GETLINECOUNT, NULL, NULL);
            if (lines != linesEdit && linesEdit != 0)
                ResizeToContent(hEdit, lvi);
        }
        else
        {
            int currentHeight = rcWindow.bottom - rcWindow.top;
            rc.right = 16;
            GetContentRect(hEdit, &rc, -1, nullptr);
            SetWindowPos(hEdit, nullptr, 0, 0, max(rc.right - rc.left, 16), currentHeight, SWP_NOMOVE | SWP_NOZORDER);
            DUIElem->SetWidth(max(rc.right - rc.left, 16) + rcPadding.left + rcPadding.right + 1);
        }
    }

    LRESULT CALLBACK RichEditWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        CSafeElementPtr<Element> DUIElem{}, innerElement{};
        CSafeElementPtr<RichText> textElement{};
        CSafeElementPtr<DDScalableRichText> dirname{};
        if (g_renameactive)
        {
            switch (uMsg)
            {
                case WM_KEYDOWN:
                    if (wParam == VK_RETURN)
                    {
                        SendMessageW(hWnd, WM_USER + 1, NULL, NULL);
                        SendMessageW(hWnd, WM_KILLFOCUS, (WPARAM)GetShellWindow(), NULL);
                        return 0;
                    }
                    if (wParam == VK_ESCAPE)
                    {
                        g_renameactive = false;
                        DUIElem.Assign(regElem<Element*>(L"RenameBoxTexture", UIContainer));
                        DestroyWindow(hWnd);
                        textElement.Assign(((LVItem*)dwRefData)->GetText());
                        innerElement.Assign(((LVItem*)dwRefData)->GetInnerElement());
                        dirname.Assign(regElem<DDScalableRichText*>(L"dirname", (LVItem*)dwRefData));
                        textElement->SetVisible(true);
                        if (!g_touchmode)
                            innerElement->SetVisible(true);
                        if (dirname)
                            dirname->SetVisible(true);
                        if (DUIElem)
                        {
                            DUIElem->DestroyAll(true);
                            DUIElem->Destroy(true);
                        }
                        return 0;
                    }
                case WM_CHAR:
                    if (wcschr(L"\\/:*?\"<>|", (WCHAR)wParam) != nullptr)
                    {
                        SetTimer(hWnd, 2, 50, nullptr);
                        return 0;
                    }
                    SetTimer(hWnd, 1, 10, nullptr);
                    break;
                case WM_PASTE:
                    if (OpenClipboard(hWnd))
                    {
                        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                        if (hData)
                        {
                            WCHAR* pszText = (WCHAR*)GlobalLock(hData);
                            if (pszText)
                            {
                                int len = wcslen(pszText);
                                for (int i = 0; i < len; i++)
                                {
                                    if (wcschr(L"\\/:*?\"<>|", *pszText) != nullptr)
                                    {
                                        SetTimer(hWnd, 2, 50, nullptr);
                                        GlobalUnlock(hData);
                                        CloseClipboard();
                                        return 0;
                                    }
                                    pszText++;
                                }
                                GlobalUnlock(hData);
                            }
                        }
                        CloseClipboard();
                    }
                    SetTimer(hWnd, 1, 10, nullptr);
                    break;
                case WM_TIMER:
                    KillTimer(hWnd, wParam);
                    switch (wParam)
                    {
                        case 1:
                            ResizeToContent(hWnd, (LVItem*)dwRefData);
                            break;
                        case 2:
                        {
                            MessageBeep(MB_OK);
                            DDNotificationBanner* ddnb = new DDNotificationBanner();
                            ddnb->CreateBanner(DDNT_WARNING, nullptr, LoadStrFromRes(4109, L"shell32.dll").c_str(), 5);
                            break;
                        }
                        case 3:
                            DUIElem.Assign(regElem<Element*>(L"RenameBoxTexture", UIContainer));
                            DUIElem->SetVisible(true);
                            SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
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
                    innerElement.Assign(((LVItem*)dwRefData)->GetInnerElement());
                    dirname.Assign(regElem<DDScalableRichText*>(L"dirname", (LVItem*)dwRefData));
                    textElement->SetVisible(true);
                    if (!g_touchmode)
                        innerElement->SetVisible(true);
                    if (dirname)
                        dirname->SetVisible(true);
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

    void ShowRename(LVItem* lviOpt)
    {
        int found{};
        int itemID{};
        if (lviOpt)
            found++;
        for (int items = 0; items < pm.size(); items++)
        {
            if (pm[items]->GetSelected() == true && pm[items] != lviOpt)
            {
                found++;
                if (found > 1)
                {
                    MessageBeep(MB_OK);
                    DDNotificationBanner* ddnb = new DDNotificationBanner();
                    ddnb->CreateBanner(DDNT_ERROR, nullptr, LoadStrFromRes(4041).c_str(), 3);
                    break;
                }
                itemID = items;
            }
        }
        if (!lviOpt)
            lviOpt = pm[itemID];
        if (found == 1)
        {
            static RichText* textElement{};
            static Element* innerElement{};
            static Element* RenameBoxTexture{};
            static DDScalableRichText* dirname{};
            if (lviOpt->GetPage() == g_currentPageID)
            {
                RECT dimensions{};
                GetClientRect(wnd->GetHWND(), &dimensions);
                g_renameactive = true;
                textElement = lviOpt->GetText();
                innerElement = lviOpt->GetInnerElement();
                dirname = regElem<DDScalableRichText*>(L"dirname", lviOpt);
                textElement->SetVisible(false);
                if (!g_touchmode)
                    innerElement->SetVisible(false);
                if (dirname)
                    dirname->SetVisible(false);
                LoadLibraryW(L"Msftedit.dll");
                unsigned long keyR{};
                CValuePtr v;
                parser->CreateElement(L"RenameBoxTexture", nullptr, nullptr, nullptr, &RenameBoxTexture);
                RenameBoxTexture->SetVisible(false);
                UIContainer->Add(&RenameBoxTexture, 1);
                if (DWMActive)
                {
                    AddLayeredRef(RenameBoxTexture->GetDisplayNode());
                    SetGadgetFlags(RenameBoxTexture->GetDisplayNode(), NULL, NULL);
                }
                RECT ebsz{}, rcPadding{};
                RenameBoxTexture->GetRenderPadding(&rcPadding);
                if (lviOpt->GetGroupSize() == LVIGS_NORMAL)
                    GetGadgetRect(textElement->GetDisplayNode(), &ebsz, 0xC);
                else if (dirname)
                    GetGadgetRect(dirname->GetDisplayNode(), &ebsz, 0xC);
                ebsz.top -= round(2 * g_flScaleFactor);
                ebsz.bottom += round(g_flScaleFactor) - 1;
                if (g_touchmode)
                {
                    ebsz.left -= round(2 * g_flScaleFactor);
                    ebsz.top -= round(1 * g_flScaleFactor);
                    ebsz.right += round(3 * g_flScaleFactor);
                }
                RenameBoxTexture->SetX(ebsz.left), RenameBoxTexture->SetY(ebsz.top);
                RenameBoxTexture->SetWidth(ebsz.right - ebsz.left), RenameBoxTexture->SetHeight(ebsz.bottom - ebsz.top);
                LPWSTR sheetName = g_theme ? (LPWSTR)L"renamestyle" : (LPWSTR)L"renamestyledark";
                StyleSheet* sheet = pMain->GetSheet();
                CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                parser->GetSheet(sheetName, &sheetStorage);
                RenameBoxTexture->SetValue(Element::SheetProp, 1, sheetStorage);
                DWORD alignment = (g_touchmode || lviOpt->GetGroupSize() != LVIGS_NORMAL) ? (localeType == 1) ? ES_RIGHT : ES_LEFT : ES_CENTER;
                if (lviOpt->GetGroupSize() == LVIGS_NORMAL) alignment |= ES_MULTILINE | ES_AUTOVSCROLL;
                else alignment |= ES_AUTOHSCROLL;
                HWND hEdit = CreateWindowExW(NULL, L"Edit", lviOpt->GetSimpleFilename().c_str(), WS_CHILD | WS_VISIBLE | ES_WANTRETURN | ES_NOHIDESEL | alignment,
                    ebsz.left + rcPadding.left, ebsz.top + rcPadding.top, ebsz.right - ebsz.left - rcPadding.left - rcPadding.right, ebsz.bottom - ebsz.top - rcPadding.top - rcPadding.bottom,
                    wnd->GetHWND(), nullptr, HINST_THISCOMPONENT, nullptr);
                LOGFONTW lf{};
                int dpiAdjusted = (g_dpiLaunch * 96.0) * (g_dpiLaunch / 96.0) / g_dpi;
                if (lviOpt->GetGroupSize() == LVIGS_NORMAL)
                {
                    SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, g_dpi);
                    if (g_touchmode) lf.lfHeight *= 1.25;
                }
                else
                {
                    lf.lfHeight = dirname->GetFontSize();
                    lf.lfWeight = dirname->GetFontWeight();
                    lf.lfItalic = dirname->GetFontStyle() & 1;
                    lf.lfUnderline = dirname->GetFontStyle() & 2;
                    lf.lfStrikeOut = dirname->GetFontStyle() & 4;
                    lf.lfCharSet = DEFAULT_CHARSET;
                    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
                    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
                    lf.lfQuality = DEFAULT_QUALITY;
                    lf.lfPitchAndFamily = FF_DONTCARE;
                    wcscpy_s(lf.lfFaceName, dirname->GetFontFace(&v));
                }
                if (lf.lfHeight < 0) lf.lfHeight = round(lf.lfHeight * -1.33);
                HFONT hFont = CreateFontIndirectW(&lf);

                COLORREF editbg = g_theme ? GetSysColor(COLOR_WINDOW) : RGB(32, 32, 32);
                SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
                SetWindowLongPtrW(hEdit, GWL_EXSTYLE, 0xC0000A40L | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP);
                SetLayeredWindowAttributes(hEdit, 0, 1, LWA_ALPHA);
                SetWindowPos(hEdit, HWND_TOP, ebsz.left + rcPadding.left, ebsz.top + rcPadding.top, NULL, NULL, SWP_NOSIZE | SWP_SHOWWINDOW);
                SetFocus(hEdit);
                int textLen = lviOpt->GetSimpleFilename().find_last_of(L".");
                if (textLen == wstring::npos) textLen = lviOpt->GetSimpleFilename().length();
                SendMessageW(hEdit, EM_SETSEL, 0, textLen);
                SetWindowSubclass(hEdit, RichEditWindowProc, 0, (DWORD_PTR)lviOpt);
                SetTimer(hEdit, 1, 25, nullptr);
                SetTimer(hEdit, 3, 50, nullptr);
            }
        }
    }
}
