#include "RenameCore.h"
#include "StyleModifier.h"
#include "DirectDesktop.h"
#include <strsafe.h>
#include <Richedit.h>
#include <textserv.h>
#include <shlobj.h>

using namespace std;
using namespace DirectUI;

wstring RemoveEndingSpaces(const wstring& str) {
    wstring newStr;
    size_t pos = str.find_first_not_of(L' ');
    size_t pos2 = str.find_last_not_of(L' ');
    if (pos != 0 || pos2 != wstring::npos) newStr = str.substr(pos, pos2 - pos + 1);
    else return L"";
    return newStr;
}
wstring RenameHelper(const wstring& fullname, const wstring& simplename) {
    wstring result;
    result.reserve(fullname.length());

    size_t pos = 0;
    size_t found = fullname.find(simplename);

    while (found != wstring::npos) {
        result.append(fullname, pos, found - pos);
        result += L"%s";
        pos = found + simplename.length();
        found = fullname.find(simplename, pos);
    }

    result.append(fullname, pos, fullname.length() - pos);
    return result;
}
void RenameCore(Element* elem) {
    HRESULT hr{};
    LVItem* selectedElement = (LVItem*)(elem->GetParent()->GetParent());
    HWND hRichEdit = FindWindowExW(((HWNDElement*)elem)->GetHWND(), NULL, MSFTEDIT_CLASS, NULL);
    WCHAR* buffer = new WCHAR[256];
    wstring newText{};
    GetWindowTextW(hRichEdit, buffer, 256);
    if (!buffer) hr = E_FAIL;
    else newText = RemoveEndingSpaces(buffer);
    wstring newFilenameBuffer = RenameHelper(selectedElement->GetFilename(), selectedElement->GetSimpleFilename());
    WCHAR* newFilename = new WCHAR[256];
    StringCchPrintfW(newFilename, 256, newFilenameBuffer.c_str(), newText.c_str());
    if (hr != E_FAIL && MoveFileW(RemoveQuotes(selectedElement->GetFilename()).c_str(), RemoveQuotes(newFilename).c_str())) {
        SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, NULL, NULL);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_NORMAL, 300, NULL);
    }
    delete[] newFilename;
    delete[] buffer;
    renameactive = false;
}
int GetContentHeight(HWND hRichEdit) {
    int lineCount = (int)SendMessageW(hRichEdit, EM_GETLINECOUNT, 0, 0);
    HDC hdc = GetDC(hRichEdit);
    TEXTMETRICW tm{};
    LOGFONTW lf{};
    RECT rc = { 0, 0, 100, 100 };
    HDC hdcBuffer = CreateCompatibleDC(NULL);
    SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, dpi);
    HFONT hFont = CreateFontIndirectW(&lf);
    HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
    DrawTextW(hdcBuffer, L" ", -1, &rc, touchmode ? DT_LEFT : DT_CENTER);
    GetTextMetricsW(hdcBuffer, &tm);
    SelectObject(hdcBuffer, hOldFont);
    DeleteObject(hFont);
    DeleteDC(hdcBuffer);
    int lineHeight = (tm.tmHeight + tm.tmExternalLeading);
    ReleaseDC(hRichEdit, hdc);
    if (touchmode) lineHeight *= 1.25;
    return (lineCount * lineHeight);
}
void ResizeToContent(HWND hRichEdit, HWNDElement* hDUIParent) {
    Value* v{};
    Element* DUIElem = ((HWNDElement*)hDUIParent)->GetParent();
    RECT rc, rcPadding;
    rcPadding = *(DUIElem->GetPadding(&v));
    GetWindowRect(hRichEdit, &rc);
    int currentWidth = rc.right - rc.left;
    int newHeight = GetContentHeight(hRichEdit);
    SetWindowPos(hRichEdit, NULL, 0, 0, currentWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowPos(((HWNDElement*)hDUIParent)->GetHWND(), NULL, 0, 0, currentWidth + rcPadding.left + rcPadding.right, newHeight + rcPadding.top + rcPadding.bottom, SWP_NOMOVE | SWP_NOZORDER);
}
LRESULT CALLBACK RichEditWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    Element* DUIElem{}, *ParentElement{}, *innerElement{};
    RichText* textElement{}, *textElementShadow{};
    static bool dontkill{};
    if (renameactive) {
        switch (uMsg) {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                RenameCore((Element*)dwRefData);
                DestroyWindow(hWnd);
                DUIElem = ((HWNDElement*)dwRefData)->GetParent();
                DestroyWindow(((HWNDElement*)dwRefData)->GetHWND());
                ParentElement = DUIElem->GetParent();
                textElement = regElem<RichText*>(L"textElem", ParentElement);
                textElementShadow = regElem<RichText*>(L"textElemShadow", ParentElement);
                innerElement = regElem<RichText*>(L"innerElem", ParentElement);
                textElement->SetVisible(true);
                if (!touchmode) {
                    textElementShadow->SetVisible(true);
                    innerElement->SetVisible(true);
                }
                DUIElem->DestroyAll(true);
                DUIElem->Destroy(true);
                return 0;
            }
        case WM_PASTE:
        case WM_CHAR:
            if (wcschr(L"\\/:*?\"<>|", (WCHAR)wParam) != NULL) {
                SetTimer(hWnd, 2, 50, NULL);
                return 0;
                break;
            }
            SetTimer(hWnd, 1, 150, NULL);
            break;
        case WM_TIMER:
            KillTimer(hWnd, wParam);
            switch (wParam) {
            case 1:
                ResizeToContent(hWnd, (HWNDElement*)dwRefData);
                break;
            case 2:
                MessageBeep(MB_OK);
                DDNotificationBanner* ddnb{};
                DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_WARNING, L"DDNB", NULL, LoadStrFromRes(4109, L"shell32.dll").c_str(), 5, false);
                break;
            }
            break;
        case WM_KILLFOCUS:
            WCHAR className[256];
            GetClassNameW((HWND)wParam, className, 256);
            if (wcscmp(className, L"DD_NotificationHost") != 0) {
                RenameCore((Element*)dwRefData);
                DestroyWindow(hWnd);
                DUIElem = ((HWNDElement*)dwRefData)->GetParent();
                DestroyWindow(((HWNDElement*)dwRefData)->GetHWND());
                ParentElement = DUIElem->GetParent();
                textElement = regElem<RichText*>(L"textElem", ParentElement);
                textElementShadow = regElem<RichText*>(L"textElemShadow", ParentElement);
                innerElement = regElem<RichText*>(L"innerElem", ParentElement);
                textElement->SetVisible(true);
                if (!touchmode) {
                    textElementShadow->SetVisible(true);
                    innerElement->SetVisible(true);
                }
                DUIElem->DestroyAll(true);
                DUIElem->Destroy(true);
            }
            else SetFocus(hWnd);
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, RichEditWindowProc, uIdSubclass);
            break;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
void ShowRename() {
    int itemWidth{}, itemHeight{}, itemX{}, itemY{}, textWidth{}, textHeight{}, textX{}, textY{};
    int found{};
    for (int items = 0; items < pm.size(); items++) {
        if (pm[items]->GetSelected() == true) {
            static RichText* textElement{};
            static RichText* textElementShadow{};
            static Element* innerElement{};
            static HWNDElement* RenameBox{};
            static Element* RenameBoxElement{};
            found++;
            if (found > 1) {
                Sleep(250);
                DestroyWindow(FindWindowW(MSFTEDIT_CLASS, NULL));
                if (RenameBox) {
                    DestroyWindow(RenameBox->GetHWND());
                    textElement->SetVisible(true);
                    if (!touchmode) {
                        textElementShadow->SetVisible(true);
                        innerElement->SetVisible(true);
                    }
                    RenameBoxElement->DestroyAll(true);
                    RenameBoxElement->Destroy(true);
                }
                renameactive = false;
                MessageBeep(MB_OK);
                DDNotificationBanner* ddnb{};
                DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", NULL, LoadStrFromRes(4041).c_str(), 3, false);
                break;
            }
            if (pm[items]->GetPage() == currentPageID) {
                RECT dimensions{};
                GetClientRect(wnd->GetHWND(), &dimensions);
                int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
                renameactive = true;
                textElement = regElem<RichText*>(L"textElem", pm[items]);
                textElementShadow = regElem<RichText*>(L"textElemShadow", pm[items]);
                innerElement = regElem<RichText*>(L"innerElem", pm[items]);
                textElement->SetVisible(false);
                if (!touchmode) {
                    textElementShadow->SetVisible(false);
                    innerElement->SetVisible(false);
                }
                LoadLibraryW(L"Msftedit.dll");
                itemWidth = pm[items]->GetWidth();
                itemHeight = pm[items]->GetHeight();
                itemX = (localeType == 1) ? dimensions.right - pm[items]->GetX() - itemWidth : pm[items]->GetX();
                itemY = pm[items]->GetY();
                // 0.4.9: width, x, and y are hardcoded due to changing layoutpos of text from absolute to top in touchmode
                textWidth = touchmode ? textElement->GetWidth() - 6 * flScaleFactor : textElement->GetWidth();
                textHeight = textElement->GetHeight() + 4 * flScaleFactor;
                textX = touchmode ? 7 * flScaleFactor : 0;
                textY = touchmode ? 8 * flScaleFactor : 0;
                unsigned long keyR{};
                Value* v{};
                parser->CreateElement(L"RenameBoxElement", NULL, NULL, NULL, &RenameBoxElement);
                RenameBoxElement->SetLayoutPos(textElement->GetLayoutPos());
                pm[items]->Add((Element**)&RenameBoxElement, 1);
                RenameBoxElement->SetHeight(textHeight);
                parser->CreateElement(L"RenameBox", RenameBoxElement, NULL, NULL, (Element**)&RenameBox);
                HWNDElement::Create(wnd->GetHWND(), true, NULL, NULL, &keyR, (Element**)&RenameBox);
                RenameBoxElement->Add((Element**)&RenameBox, 1);
                RenameBox->SetVisible(true);
                SetWindowPos(RenameBox->GetHWND(), HWND_TOP, itemX, itemY + itemHeight - textHeight, itemWidth, textHeight, SWP_SHOWWINDOW);
                ShowWindow(RenameBox->GetHWND(), SW_SHOW);
                RECT ebsz{}, rcPadding{};
                GetClientRect(RenameBox->GetHWND(), &ebsz);
                rcPadding = *(RenameBoxElement->GetPadding(&v));
                if (touchmode) {
                    SetWindowPos(RenameBox->GetHWND(), HWND_TOP, itemX + textX - rcPadding.left, itemY + textY - rcPadding.top, textWidth + rcPadding.left + rcPadding.right, textHeight, SWP_SHOWWINDOW);
                    GetClientRect(RenameBox->GetHWND(), &ebsz);
                }
                LPWSTR sheetName = theme ? (LPWSTR)L"renamestyle" : (LPWSTR)L"renamestyledark";
                StyleSheet* sheet = pMain->GetSheet();
                Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                parser->GetSheet(sheetName, &sheetStorage);
                RenameBox->SetValue(Element::SheetProp, 1, sheetStorage);
                sheetStorage->Release();
                DWORD alignment = touchmode ? (localeType == 1) ? ES_RIGHT : ES_LEFT : ES_CENTER;
                HWND hRichEdit = CreateWindowExW(NULL, MSFTEDIT_CLASS, pm[items]->GetSimpleFilename().c_str(), WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL | alignment,
                    ebsz.left + rcPadding.left, ebsz.top + rcPadding.top, ebsz.right - rcPadding.left - rcPadding.right, ebsz.bottom - rcPadding.top - rcPadding.bottom, RenameBox->GetHWND(), (HMENU)2050, HINST_THISCOMPONENT, NULL);
                LOGFONTW lf{};
                int dpiAdjusted = (dpiLaunch * 96.0) * (dpiLaunch / 96.0) / dpi;
                SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL);
                CHARFORMAT2W cf{};
                cf.cbSize = sizeof(CHARFORMAT2W);
                cf.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR | CFM_BOLD | CFM_ITALIC;
                cf.yHeight = (lf.lfHeight * -15 * 96.0) / dpiAdjusted;
                if (touchmode) cf.yHeight *= 1.25;
                cf.crTextColor = theme ? GetSysColor(COLOR_WINDOWTEXT) : RGB(255, 255, 255);
                wcscpy_s(cf.szFaceName, lf.lfFaceName);
                if (lf.lfWeight == FW_BOLD) cf.dwEffects |= CFE_BOLD;
                if (lf.lfItalic) cf.dwEffects |= CFE_ITALIC;

                COLORREF editbg = theme ? GetSysColor(COLOR_WINDOW) : RGB(32, 32, 32);
                SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
                SendMessageW(hRichEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)editbg);
                SetWindowLongPtrW(hRichEdit, GWL_EXSTYLE, 0xC0000A40L | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP);
                SetLayeredWindowAttributes(hRichEdit, 0, 255, LWA_ALPHA);
                SetWindowPos(hRichEdit, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                SetFocus(hRichEdit);
                int textLen = pm[items]->GetSimpleFilename().find_last_of(L".");
                if (textLen == wstring::npos) textLen = pm[items]->GetSimpleFilename().length();
                SendMessageW(hRichEdit, EM_SETSEL, 0, textLen);
                SetWindowSubclass(hRichEdit, RichEditWindowProc, 0, (DWORD_PTR)RenameBox);
                RenameBox->EndDefer(keyR);
            }
        }
    }
}