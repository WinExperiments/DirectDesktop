#include "EditMode.h"
#include "DirectDesktop.h"
#include "BitmapHelper.h"
#include "StyleModifier.h"
#include "ShutdownDialog.h"
#include "SearchPage.h"
#include <strsafe.h>
#include <uxtheme.h>
#include <dwmapi.h>

using namespace DirectUI; 

NativeHWNDHost* editwnd;
HWNDElement* editparent;
DUIXmlParser* parser5;
Element* pEdit;
unsigned long key5 = 0;
WNDPROC WndProc6;
DDScalableElement* fullscreeninnerE;
Element* popupcontainerE;
TouchButton* fullscreenpopupbaseE;
Button* centeredE;
DDScalableElement* simpleviewoverlay;
Element* deskpreview;
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

unsigned long animate7(LPVOID lpParam) {
    SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
    Sleep(350);
    editwnd->DestroyWindow();
    return 0;
}

void fullscreenAnimation3(int width, int height, float animstartscale) {
    parser5->CreateElement(L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninnerE);
    centeredE->Add((Element**)&fullscreeninnerE, 1);
    static const int savedanim = centeredE->GetAnimation();
    static const int savedanim2 = fullscreeninnerE->GetAnimation();
    PlaySimpleViewAnimation(centeredE, width, height, savedanim, animstartscale);
    PlaySimpleViewAnimation(fullscreeninnerE, width, height, savedanim2, animstartscale);
    centeredE->SetBackgroundColor(0);
    fullscreenpopupbaseE->SetVisible(true);
    fullscreeninnerE->SetVisible(true);
}

void fullscreenAnimation4() {
    DWORD animThread;
    HANDLE animThreadHandle = CreateThread(0, 0, animate7, NULL, 0, &animThread);
}

void HideSimpleView(bool animate) {
    if (animate) {
        centeredE->SetWidth(centeredE->GetWidth() * 1.4285);
        centeredE->SetHeight(centeredE->GetHeight() * 1.4285);
        fullscreeninnerE->SetWidth(fullscreeninnerE->GetWidth() * 1.4285);
        fullscreeninnerE->SetHeight(fullscreeninnerE->GetHeight() * 1.4285);
        simpleviewoverlay->SetWidth(simpleviewoverlay->GetWidth() * 1.4285);
        simpleviewoverlay->SetHeight(simpleviewoverlay->GetHeight() * 1.4285);
        deskpreview->SetWidth(deskpreview->GetWidth() * 1.4285);
        deskpreview->SetHeight(deskpreview->GetHeight() * 1.4285);
        mainContainer->SetVisible(true);
        mainContainer->SetAlpha(255);
        editmode = false;
        fullscreenAnimation4();
    }
    else {
        editwnd->DestroyWindow();
    }
}

void TriggerHSV(Element* elem, Event* iev) {
    if ((iev->uidType == TouchButton::Click && prevpage->GetMouseFocused() == false && nextpage->GetMouseFocused() == false) || (iev->uidType == Button::Click && SimpleViewSettings->GetMouseFocused() == true)) {
        HideSimpleView(true);
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

void ShowSimpleView() {
    editmode = true;
    if (!invokedpagechange) SendMessageW(hWndTaskbar, WM_COMMAND, 419, 0);
    RECT dimensions;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    NativeHWNDHost::Create(L"DD_EditModeHost", L"DirectDesktop Edit Mode", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, WS_EX_TOOLWINDOW, WS_POPUP, NULL, 0, &editwnd);
    HWNDElement::Create(editwnd->GetHWND(), true, NULL, NULL, &key5, (Element**)&editparent);
    DUIXmlParser::Create(&parser5, NULL, NULL, NULL, NULL);
    parser5->SetXMLFromResource(IDR_UIFILE6, HINST_THISCOMPONENT, HINST_THISCOMPONENT);

    parser5->CreateElement(L"editmode", editparent, NULL, NULL, &pEdit);
    pEdit->SetVisible(true);
    AddLayeredRef(pEdit->GetDisplayNode());
    SetGadgetFlags(pEdit->GetDisplayNode(), 0x1, 0x1);
    pEdit->EndDefer(key5);

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
    assignFn(SimpleViewSearch, ShowSearchUI);
    assignFn(SimpleViewSettings, ShowSettings);
    assignFn(SimpleViewSettings, TriggerHSV);
    assignFn(SimpleViewClose, ExitWindow);

    WndProc6 = (WNDPROC)SetWindowLongPtrW(editwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeWindowProc);

    LPWSTR sheetName = theme ? (LPWSTR)L"edit" : (LPWSTR)L"editdark";
    StyleSheet* sheet = pEdit->GetSheet();
    Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
    parser5->GetSheet(sheetName, &sheetStorage);
    pEdit->SetValue(Element::SheetProp, 1, sheetStorage);

    editwnd->Host(pEdit);
    editwnd->ShowWindow(SW_SHOW);
    SetParent(editwnd->GetHWND(), hSHELLDLL_DefView);
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(editwnd->GetHWND(), &m);
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
    IterateBitmap(hbmCapture, UndoPremultiplication, 1, 0, 1);
    bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 7, 0xffffffff, false, false, false);
    fullscreenAnimation3(dimensions.right * 0.7, dimensions.bottom * 0.7, 1.4285);
    parser5->CreateElement(L"deskpreview", NULL, NULL, NULL, (Element**)&deskpreview);
    centeredE->Add((Element**)&deskpreview, 1);
    if (bitmap != nullptr) deskpreview->SetValue(Element::BackgroundProp, 1, bitmap);
    static const int savedanim4 = deskpreview->GetAnimation();
    PlaySimpleViewAnimation(deskpreview, dimensions.right * 0.7, dimensions.bottom * 0.7, savedanim4, 1.4285);
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
    PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right * 0.7, dimensions.bottom * 0.7, savedanim3, 1.4285);
    //BlurBackground(subviewwnd->GetHWND(), false, true);
    wnd->ShowWindow(SW_SHOW);

    invokedpagechange = false;
}