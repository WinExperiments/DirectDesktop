#include "EditMode.h"
#include "DirectDesktop.h"
#include "BitmapHelper.h"
#include "StyleModifier.h"
#include "ShutdownDialog.h"
#include "SettingsHelper.h"
#include "SearchPage.h"
#include <strsafe.h>
#include <uxtheme.h>
#include <dwmapi.h>

using namespace DirectUI; 

NativeHWNDHost* editwnd, *editbgwnd;
HWNDElement* editparent, *editbgparent;
DUIXmlParser* parser5;
Element* pEdit, *pEditBG;
unsigned long key5 = 0, key6 = 0;
WNDPROC WndProc6, WndProc7;
DDScalableElement* fullscreeninnerE;
Element* popupcontainerE;
TouchButton* fullscreenpopupbaseE;
Button* centeredE;
DDScalableElement* simpleviewoverlay;
Element* deskpreview, *deskpreviewmask;
TouchButton* SimpleViewTop, *SimpleViewBottom;
Button* SimpleViewPower, *SimpleViewSearch, *SimpleViewSettings, *SimpleViewClose;
TouchButton* nextpage, *prevpage;
RichText* pageinfo;

LRESULT CALLBACK EditModeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        HideSimpleView(true);
        return 0;
        break;
    case WM_DESTROY:
        return 0;
        break;
    }
    return CallWindowProc(WndProc6, hWnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK EditModeBGWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_WINDOWPOSCHANGING: {
        //((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
        return 0L;
        break;
    }
    case WM_CLOSE:
        return 0;
        break;
    case WM_DESTROY:
        return 0;
        break;
    }
    return CallWindowProc(WndProc7, hWnd, uMsg, wParam, lParam);
}

unsigned long animate7(LPVOID lpParam) {
    SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
    Sleep(350);
    editwnd->DestroyWindow();
    //editbgwnd->DestroyWindow();
    return 0;
}

static int g_savedanim, g_savedanim2, g_savedanim3, g_savedanim4;
void fullscreenAnimation3(int width, int height, float animstartscale, bool animate) {
    parser5->CreateElement(L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninnerE);
    centeredE->Add((Element**)&fullscreeninnerE, 1);
    static const int savedanim = centeredE->GetAnimation();
    static const int savedanim2 = fullscreeninnerE->GetAnimation();
    g_savedanim = savedanim;
    g_savedanim2 = savedanim2;
    PlaySimpleViewAnimation(centeredE, width, height, animate ? savedanim : NULL, animstartscale);
    PlaySimpleViewAnimation(fullscreeninnerE, width, height, animate ? savedanim2 : NULL, animstartscale);
    centeredE->SetBackgroundColor(0);
    fullscreenpopupbaseE->SetVisible(true);
    fullscreeninnerE->SetVisible(true);
    SimpleViewTop->SetAlpha(255);
    SimpleViewBottom->SetAlpha(255);
    prevpage->SetAlpha(255);
    nextpage->SetAlpha(255);
}

void fullscreenAnimation4() {
    SimpleViewTop->SetAlpha(0);
    SimpleViewBottom->SetAlpha(0);
    prevpage->SetAlpha(255);
    nextpage->SetAlpha(255);
    DWORD animThread;
    HANDLE animThreadHandle = CreateThread(0, 0, animate7, NULL, 0, &animThread);
}

void HideSimpleView(bool animate) {
    if (animate) {
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        PlaySimpleViewAnimation(centeredE, dimensions.right, dimensions.bottom, animate ? g_savedanim : NULL, 0.7);
        PlaySimpleViewAnimation(fullscreeninnerE, dimensions.right, dimensions.bottom, animate ? g_savedanim2 : NULL, 0.7);
        PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right, dimensions.bottom, animate ? g_savedanim3 : NULL, 0.7);
        PlaySimpleViewAnimation(deskpreview, dimensions.right, dimensions.bottom, animate ? g_savedanim4 : NULL, 0.7);
        mainContainer->SetVisible(true);
        mainContainer->SetAlpha(255);
        editmode = false;
        fullscreenAnimation4();
    }
    else {
        mainContainer->SetVisible(true);
        mainContainer->SetAlpha(255);
        editwnd->DestroyWindow();
        //editbgwnd->DestroyWindow();
    }
}

void TriggerHSV(Element* elem, Event* iev) {
    static int validation;
    if (iev->uidType == TouchButton::Click && prevpage->GetMouseFocused() == false && nextpage->GetMouseFocused() == false) {
        HideSimpleView(true);
        return;
    }
    if (iev->uidType == Button::Click) {
        validation++;
        if (validation % 3 == 1) {
            HideSimpleView(true);
            return;
        }
    }
}

void ShowShutdownDialog(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        DisplayShutdownDialog();
    }
}
void ShowSearchUI(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        CreateSearchPage();
    }
}
void ExitWindow(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
        SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, NULL);
    }
}

void ShowSimpleView(bool animate) {
    editmode = true;
    if (!invokedpagechange) SendMessageW(hWndTaskbar, WM_COMMAND, 419, 0);
    RECT dimensions;
    POINT topLeftMon = GetTopLeftMonitor();
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    if (localeType == 1) {
        int rightMon = GetRightMonitor();
        topLeftMon.x = dimensions.right + dimensions.left - rightMon;
    }
    NativeHWNDHost::Create(L"DD_EditModeHost", L"DirectDesktop Edit Mode", NULL, NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y,
        dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_NOINHERITLAYOUT, WS_POPUP, NULL, 0, &editwnd);
    HWNDElement::Create(editwnd->GetHWND(), true, NULL, NULL, &key5, (Element**)&editparent);
    DUIXmlParser::Create(&parser5, NULL, NULL, NULL, NULL);
    parser5->SetXMLFromResource(IDR_UIFILE6, HINST_THISCOMPONENT, HINST_THISCOMPONENT);

    parser5->CreateElement(L"editmode", editparent, NULL, NULL, &pEdit);
    //parser5->CreateElement(L"editmodeblur", editbgparent, NULL, NULL, &pEditBG);

    SetWindowLongPtrW(editwnd->GetHWND(), GWL_STYLE, 0x56003A40L);
    //SetWindowLongPtrW(editbgwnd->GetHWND(), GWL_EXSTYLE, 0xC0080800L);
    //SetParent(editbgwnd->GetHWND(), NULL);

    pEdit->SetVisible(true);
    AddLayeredRef(pEdit->GetDisplayNode());
    SetGadgetFlags(pEdit->GetDisplayNode(), 0x1, 0x1);
    pEdit->EndDefer(key5);
    //pEditBG->SetVisible(true);
    //AddLayeredRef(pEditBG->GetDisplayNode());
    //SetGadgetFlags(pEditBG->GetDisplayNode(), 0x1, 0x1);
    //pEditBG->EndDefer(key6);

    fullscreenpopupbaseE = regElem<TouchButton*>(L"fullscreenpopupbase", pEdit);
    popupcontainerE = regElem<Button*>(L"popupcontainer", pEdit);
    centeredE = regElem<Button*>(L"centered", pEdit);
    SimpleViewTop = regElem<TouchButton*>(L"SimpleViewTop", pEdit);
    SimpleViewBottom = regElem<TouchButton*>(L"SimpleViewBottom", pEdit);
    SimpleViewPower = regElem<Button*>(L"SimpleViewPower", pEdit);
    SimpleViewSearch = regElem<Button*>(L"SimpleViewSearch", pEdit);
    SimpleViewSettings = regElem<Button*>(L"SimpleViewSettings", pEdit);
    SimpleViewClose = regElem<Button*>(L"SimpleViewClose", pEdit);
    prevpage = regElem<TouchButton*>(L"prevpage", pEdit);
    nextpage = regElem<TouchButton*>(L"nextpage", pEdit);
    pageinfo = regElem<RichText*>(L"pageinfo", pEdit);

    assignFn(prevpage, GoToPrevPage);
    assignFn(nextpage, GoToNextPage);
    assignFn(fullscreenpopupbaseE, TriggerHSV);
    assignFn(SimpleViewPower, ShowShutdownDialog);
    assignFn(SimpleViewPower, TriggerHSV);
    assignFn(SimpleViewSearch, ShowSearchUI);
    assignFn(SimpleViewSettings, ShowSettings);
    assignFn(SimpleViewSettings, TriggerHSV);
    assignFn(SimpleViewClose, ExitWindow);

    WndProc6 = (WNDPROC)SetWindowLongPtrW(editwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeWindowProc);
    //WndProc7 = (WNDPROC)SetWindowLongPtrW(editbgwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeBGWindowProc);

    LPWSTR sheetName = theme ? (LPWSTR)L"edit" : (LPWSTR)L"editdark";
    StyleSheet* sheet = pEdit->GetSheet();
    Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
    parser5->GetSheet(sheetName, &sheetStorage);
    pEdit->SetValue(Element::SheetProp, 1, sheetStorage);

    editwnd->Host(pEdit);
    editwnd->ShowWindow(SW_SHOW);
    //editbgwnd->Host(pEditBG);
    //editbgwnd->ShowWindow(SW_SHOW);
    SetParent(editwnd->GetHWND(), hSHELLDLL_DefView);
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(editwnd->GetHWND(), &m);
    //DwmExtendFrameIntoClientArea(editbgwnd->GetHWND(), &m);
    Value* bitmap{};
    HBITMAP hbmCapture{};
    HDC hdcWindow = GetDC(wnd->GetHWND());
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    hbmCapture = CreateCompatibleBitmap(hdcWindow, dimensions.right, dimensions.bottom);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
    BitBlt(hdcMem, 0, 0, dimensions.right, dimensions.bottom, hdcWindow, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    ReleaseDC(wnd->GetHWND(), hdcWindow);
    wnd->ShowWindow(SW_HIDE);
    IterateBitmap(hbmCapture, UndoPremultiplication, 1, 0, 1, NULL);
    bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 7, 0xffffffff, false, false, false);
    fullscreenAnimation3(dimensions.right * 0.7, dimensions.bottom * 0.7, 1.4285, animate);
    parser5->CreateElement(L"deskpreview", NULL, NULL, NULL, (Element**)&deskpreview);
    //parser5->CreateElement(L"deskpreviewmask", NULL, NULL, NULL, (Element**)&deskpreviewmask);
    centeredE->Add((Element**)&deskpreview, 1);
    //pEditBG->Add((Element**)&deskpreviewmask, 1);
    if (bitmap != nullptr) deskpreview->SetValue(Element::BackgroundProp, 1, bitmap);
    static const int savedanim4 = deskpreview->GetAnimation();
    g_savedanim4 = savedanim4;
    //static const int savedanim5 = deskpreviewmask->GetAnimation();
    //deskpreviewmask->SetX(dimensions.right * 0.15);
    //deskpreviewmask->SetY(dimensions.bottom * 0.15);
    //BlurBackground(editbgwnd->GetHWND(), true, true);
    //SetLayeredWindowAttributes(editbgwnd->GetHWND(), RGB(0, 255, 255), 0, LWA_COLORKEY);
    PlaySimpleViewAnimation(deskpreview, dimensions.right * 0.7, dimensions.bottom * 0.7, animate ? savedanim4 : NULL, 1.4285);
    //PlaySimpleViewAnimation(deskpreviewmask, dimensions.right * 0.7, dimensions.bottom * 0.7, savedanim5, 1.4285);
    if (bitmap != nullptr) bitmap->Release();
    if (hbmCapture != nullptr) DeleteObject(hbmCapture);
    if (maxPageID != 1) {
        WCHAR currentPage[64];
        StringCchPrintfW(currentPage, 64, LoadStrFromRes(4026).c_str(), currentPageID, maxPageID);
        pageinfo->SetContentString(currentPage);
    }
    else pageinfo->SetContentString(L" ");
    if (currentPageID != maxPageID) {
        float xLoc = (localeType == 1) ? 0 : 0.9;
        TogglePage(nextpage, xLoc, 0.2, 0.1, 0.6);
    }
    if (currentPageID != 1) {
        float xLoc = (localeType == 1) ? 0.9 : 0;
        TogglePage(prevpage, xLoc, 0.2, 0.1, 0.6);
    }
    if (!invokedpagechange) {
        mainContainer->SetAlpha(0);
    }
    mainContainer->SetVisible(false);
    SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
    parser5->CreateElement(L"simpleviewoverlay", NULL, NULL, NULL, (Element**)&simpleviewoverlay);
    centeredE->Add((Element**)&simpleviewoverlay, 1);
    static const int savedanim3 = simpleviewoverlay->GetAnimation();
    g_savedanim3 = savedanim3;
    PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right * 0.7, dimensions.bottom * 0.7, animate ? savedanim3 : NULL, 1.4285);
    wnd->ShowWindow(SW_SHOW);

    //NativeHWNDHost::Create(L"DD_EditModeBlur", L"DirectDesktop Edit Mode Blur Helper", NULL, NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, WS_POPUP | WS_VISIBLE, NULL, 0, &editbgwnd);
    //HWNDElement::Create(editbgwnd->GetHWND(), true, NULL, NULL, &key6, (Element**)&editbgparent);

    //editbgwnd->ShowWindow(SW_SHOW);

    invokedpagechange = false;
}