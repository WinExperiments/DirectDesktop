#include "framework.h"
#include "DirectUI/DirectUI.h"
#include "DUser/DUser.h"
#include "DirectDesktop.h"
#include <string>
#include "resource.h"
#include <propkey.h>
#include "strsafe.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <vector>
#include <list>
#include <WinUser.h>
#include <ShObjIdl.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include "StyleModifier.h"
#include "BitmapHelper.h"
#include "DirectoryHelper.h"
#include "SettingsHelper.h"
#include "ContextMenus.h"
#pragma comment (lib, "dwmapi.lib")
#pragma comment (lib, "dui70.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shcore.lib")
#pragma comment (lib, "DUser.lib")

using namespace DirectUI;
using namespace std;

NativeHWNDHost* wnd, *subviewwnd;
HWNDElement* parent, *subviewparent;
DUIXmlParser* parser;
Element* pMain, *pSubview;
unsigned long key = 0, key2 = 0;

Element* sampleText;
Element* mainContainer;
Element* iconElem;
Element* shortcutElem;
Element* iconElemShadow;
RichText* textElem;
RichText* textElemShadow;
Button* checkboxElem;
Element* fullscreeninner;
Button* fullscreenpopupbase, *centered;
Button* emptyspace;
Element* selector, *selector2;
Element* dirnameanimator;
Element* tasksanimator;
Button* SimpleViewTop, *SimpleViewBottom;
TouchButton* SimpleViewSettings, *SimpleViewClose;
TouchButton* PageTab1, *PageTab2;
RichText* SubUIContainer;
TouchButton* nextpage, *prevpage;
TouchButton* prevpageMain, * nextpageMain;
RichText* pageinfo;

HRESULT err;
HWND hWorkerW = NULL;
HWND hSHELLDLL_DefView = NULL;
HWND hWndTaskbar = FindWindow(L"Shell_TrayWnd", NULL);
Logger MainLogger;

int maxPageID = 1, currentPageID = 1;
int popupframe, dframe, tframe;
//vector<int> frame;

wstring RemoveQuotes(const wstring& input) {
    if (input.size() >= 2 && input.front() == L'\"' && input.back() == L'\"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

int dpi = 96, dpiOld = 1;
float flScaleFactor = 1.0;
bool isDpiPreviouslyChanged;
void InitialUpdateScale() {
    HDC screen = GetDC(0);
    dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(0, screen);
    flScaleFactor = dpi / 96.0;
}

void UpdateScale() {
    static HWND hWnd = wnd->GetHWND();
    dpiOld = dpi;
    dpi = GetDpiForWindow(hWnd);
    isDpiPreviouslyChanged = true;
    flScaleFactor = dpi / 96.0;
}

struct EventListener : public IElementListener {

    void (*f)(Element*, Event*);

    EventListener(void (*func)(Element*, Event*)) {
        f = func; 
    }

    void OnListenerAttach(Element* elem) override { }
    void OnListenerDetach(Element* elem) override { }
    bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override {
        return true;
    }
    void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2) override {

    }
    void OnListenedEvent(Element* elem, struct Event* iev) override {
        f(elem, iev);
    }
    void OnListenedInput(Element* elem, struct InputEvent* ev) override { 
    }
};
struct EventListener2 : public IElementListener {

    void (*f)(Element*, const PropertyInfo*, int, Value*, Value*);

    EventListener2(void (*func)(Element*, const PropertyInfo*, int, Value*, Value*)) {
        f = func;
    }

    void OnListenerAttach(Element* elem) override { }
    void OnListenerDetach(Element* elem) override { }
    bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override {
        return true;
    }
    void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2) override {
        f(elem, prop, type, v1, v2);
    }
    void OnListenedEvent(Element* elem, struct Event* iev) override {

    }
    void OnListenedInput(Element* elem, struct InputEvent* ev) override {
    }
};

Element* regElem(const wchar_t* elemName, Element* peParent) {
    Element* result = (Element*)peParent->FindDescendent(StrToID(elemName));
    return result;
}
RichText* regRichText(const wchar_t* elemName, Element* peParent) {
    RichText* result = (RichText*)peParent->FindDescendent(StrToID(elemName));
    return result;
}
Button* regBtn(const wchar_t* btnName, Element* peParent) {
    Button* result = (Button*)peParent->FindDescendent(StrToID(btnName));
    return result;
}
TouchButton* regTouchBtn(const wchar_t* btnName, Element* peParent) {
    TouchButton* result = (TouchButton*)peParent->FindDescendent(StrToID(btnName));
    return result;
}
Edit* regEdit(const wchar_t* editName, Element* peParent) {
    Edit* result = (Edit*)peParent->FindDescendent(StrToID(editName));
    return result;
}
void assignFn(Element* btnName, void(*fnName)(Element* elem, Event* iev)) {
    btnName->AddListener(new EventListener(fnName));
}
void assignExtendedFn(Element* elemName, void(*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)) {
    elemName->AddListener(new EventListener2(fnName));
}

struct yValue {
    int y{};
};

double px[80]{};
double py[80]{};
int origX{}, origY{}, globaliconsz, globalshiconsz, globalgpiconsz;

void CubicBezier(const int frames, double px[], double py[], double x0, double y0, double x1, double y1) {
    for (int c = 0; c < frames; c++) {
        double t = (1.0 / frames) * c;
        px[c] = (3 * t * pow(1 - t, 2) * x0) + (3 * pow(t, 2) * (1 - t) * x1) + pow(t, 3);
        py[c] = (3 * t * pow(1 - t, 2) * y0) + (3 * pow(t, 2) * (1 - t) * y1) + pow(t, 3);
    }
    px[frames - 1] = 1;
    py[frames - 1] = 1;
}

void SetTheme() {
    StyleSheet* sheet = pMain->GetSheet();
    Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
    parser->GetSheet(sheetName, &sheetStorage);
    pMain->SetValue(Element::SheetProp, 1, sheetStorage);
    pSubview->SetValue(Element::SheetProp, 1, sheetStorage);
    sheetStorage->Release();
}

WNDPROC WndProc;
HANDLE hMutex;
constexpr LPCWSTR szWindowClass = L"DIRECTDESKTOP";
vector<LVItem*> pm, subpm;
vector<Element*> shortpm, subshortpm;
vector<Element*> iconpm, subiconpm;
vector<Element*> shadowpm, subshadowpm;
vector<RichText*> filepm, subfilepm;
vector<RichText*> fileshadowpm;
vector<Element*> cbpm;
bool checkifelemexists = 0;
bool issubviewopen = 0;
bool hiddenIcons;
bool editmode = 0;
void fullscreenAnimation(int width, int height);
void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);

HBITMAP GetShellItemImage(LPCWSTR filePath, int width, int height) {

    IShellItem* pShellItem{};
    HRESULT hr = SHCreateItemFromParsingName(filePath, NULL, IID_PPV_ARGS(&pShellItem));

    HBITMAP hBitmap{};
    if (pShellItem != nullptr) {
        IShellItemImageFactory* pImageFactory{};
        hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
        pShellItem->Release();

        SIZE size = { width * flScaleFactor, height * flScaleFactor };
        hr = pImageFactory->GetImage(size, SIIGBF_RESIZETOFIT, &hBitmap);
        pImageFactory->Release();
    }
    else {
        HICON fallback = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, width * flScaleFactor, height * flScaleFactor, LR_SHARED);
        hBitmap = IconToBitmap(fallback);
    }

    return hBitmap;
}

void GetFontHeight() {
    LOGFONTW lf{};
    RECT rc = { 0, 0, 100, 100 };
    HDC hdcBuffer = CreateCompatibleDC(NULL);
    SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, dpi);
    DrawTextW(hdcBuffer, L" ", -1, &rc, DT_CENTER);
    GetTextMetricsW(hdcBuffer, &textm);
    DeleteDC(hdcBuffer);
}
float CalcTextLines(const wchar_t* str, int width) {
    HDC hdcBuffer = CreateCompatibleDC(NULL);
    LOGFONTW lf{};
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL);
    HFONT hFont = CreateFontIndirectW(&lf);
    HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
    RECT rc = { 0, 0, width - 4, textm.tmHeight };
    wchar_t filenameBuffer[260]{};
    wcscpy_s(filenameBuffer, str);
    DrawTextExW(hdcBuffer, filenameBuffer, -1, &rc, DT_MODIFYSTRING | DT_END_ELLIPSIS | DT_CENTER | DT_LVICON, NULL);
    int lines_b1 = wcscmp(str, filenameBuffer);
    RECT rc2 = { 0, 0, width - 4, textm.tmHeight * 2 };
    wchar_t filenameBuffer2[260]{};
    wcscpy_s(filenameBuffer2, str);
    DrawTextExW(hdcBuffer, filenameBuffer2, -1, &rc2, DT_MODIFYSTRING | DT_WORD_ELLIPSIS | DT_CENTER | DT_LVICON, NULL);
    int lines_b2 = wcscmp(str, filenameBuffer2);
    DeleteObject(hFont);
    DeleteObject(hOldFont);
    DeleteDC(hdcBuffer);
    if (lines_b1 == 1 && lines_b2 == 0) return 2.0; else if (lines_b1 == 1 && lines_b2 == 1) return 1.5; else return 1;
}

void ShowSimpleView() {
    editmode = true;
    SendMessageW(hWndTaskbar, WM_COMMAND, 419, 0);
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    //if (!hiddenIcons) {
    //    Value* bitmap;
    //    HBITMAP hbmCapture{};
    //    HDC hdcWindow = GetDC(wnd->GetHWND());
    //    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    //    hbmCapture = CreateCompatibleBitmap(hdcWindow, dimensions.right, dimensions.bottom);
    //    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
    //    BitBlt(hdcMem, 0, 0, dimensions.right, dimensions.bottom, hdcWindow, 0, 0, SRCCOPY);
    //    SelectObject(hdcMem, hbmOld);
    //    DeleteDC(hdcMem);
    //    ReleaseDC(wnd->GetHWND(), hdcWindow);
    //    bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 7, 0xffffffff, false, false, false);
    //    fullscreenAnimation(dimensions.right * 0.7, dimensions.bottom * 0.7);
    //    centered->SetBackgroundStdColor(7);
    //    fullscreeninner->SetValue(Element::BackgroundProp, 1, bitmap);
    //    bitmap->Release();
    //    DeleteObject(hbmCapture);
    //}
    //else {
        fullscreenAnimation(dimensions.right * 0.7, dimensions.bottom * 0.7);
        fullscreeninner->SetBackgroundStdColor(7);
    //}
    if (maxPageID != 1) {
        WCHAR currentPage[64];
        StringCchPrintfW(currentPage, 64, L"Page %d / %d", currentPageID, maxPageID);
        pageinfo->SetContentString(currentPage);
    }
    else pageinfo->SetContentString(L" ");
    if (currentPageID != maxPageID) {
        TogglePage(nextpage, 0.9, 0.2, 0.1, 0.6);
    }
    if (currentPageID != 1) {
        TogglePage(prevpage, 0, 0.2, 0.1, 0.6);
    }
    SimpleViewTop->SetLayoutPos(1);
    SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
    SimpleViewBottom->SetLayoutPos(3);
    pSubview->SetBackgroundStdColor(7);
    Element* simpleviewoverlay{};
    parser->CreateElement(L"simpleviewoverlay", NULL, NULL, NULL, (Element**)&simpleviewoverlay);
    centered->Add((Element**)&simpleviewoverlay, 1);
    BlurBackground(GetWorkerW(), false);
}

unsigned long EndExplorer(LPVOID lpParam) {
    this_thread::sleep_for(chrono::milliseconds(250));
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    DWORD pid{};
    GetWindowThreadProcessId(hWndProgman, &pid);
    HANDLE hExplorer;
    hExplorer = OpenProcess(PROCESS_TERMINATE, false, pid);
    TerminateProcess(hExplorer, 2);
    CloseHandle(hExplorer);
    return 0;
}

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
    int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor - textm.tmHeight;
    int iconPaddingX = (GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) - 48 * flScaleFactor) / 2;
    int iconPaddingY = (GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) - 48 * flScaleFactor) / 2;
    int i = 20002;
    switch (uMsg) {
    case WM_SETTINGCHANGE: {
        UpdateModeInfo();
        SetTheme();
        if (wParam == SPI_SETWORKAREA) {
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            SetWindowPos(wnd->GetHWND(), NULL, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
            SetWindowPos(hWorkerW, NULL, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
            SetWindowPos(hSHELLDLL_DefView, NULL, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
            UIContainer->SetWidth(dimensions.right - dimensions.left);
            UIContainer->SetHeight(dimensions.bottom - dimensions.top);
            RearrangeIcons(true, false);
        }
        break;
    }
    case WM_DISPLAYCHANGE: {
        SendMessageW(wnd->GetHWND(), WM_SETTINGCHANGE, SPI_SETWORKAREA, NULL);
    }
    case WM_DPICHANGED: {
        UpdateScale();
        InitLayout();
        break;
    }
    case WM_WINDOWPOSCHANGING: {
        ((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
        return 0L;
        break;
    }
    case WM_CLOSE: {
        if (lParam == 420) {
            wchar_t* desktoplog = new wchar_t[260];
            wchar_t* cBuffer = new wchar_t[260];
            DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
            StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
            ShellExecuteW(NULL, L"open", L"notepad.exe", desktoplog, NULL, SW_SHOW);
        }
        DWORD dwTermination{};
        HANDLE termThread = CreateThread(0, 0, EndExplorer, NULL, 0, &dwTermination);
        this_thread::sleep_for(chrono::milliseconds(500));
        break;
    }
    case WM_COMMAND: {
        break;
    }
    case WM_USER + 1: {
        //pm[wParam].elem->SetAlpha(255);
        if (pm[wParam]->GetPage() == currentPageID) pm[wParam]->SetVisible(!hiddenIcons);
        else pm[wParam]->SetVisible(false);
        break;
    }
    case WM_USER + 2: {
        //subpm[wParam].elem->SetAlpha(255);
        subpm[wParam]->SetVisible(true);
        break;
    }
    case WM_USER + 3: {
        int lines_basedOnEllipsis{};
        lines_basedOnEllipsis = floor(CalcTextLines(pm[wParam]->GetSimpleFilename().c_str(), innerSizeX)) * textm.tmHeight;
        pm[wParam]->SetWidth(innerSizeX);
        pm[wParam]->SetHeight(innerSizeY + lines_basedOnEllipsis + 7 * flScaleFactor);
        //pm[wParam]->SetX(pm[wParam]->GetInternalXPos());
        //pm[wParam]->SetY(pm[wParam]->GetInternalYPos());
        filepm[wParam]->SetHeight(lines_basedOnEllipsis + 4 * flScaleFactor);
        fileshadowpm[wParam]->SetHeight(lines_basedOnEllipsis + 5 * flScaleFactor);
        iconpm[wParam]->SetWidth(round(globaliconsz * flScaleFactor));
        iconpm[wParam]->SetHeight(round(globaliconsz * flScaleFactor));
        iconpm[wParam]->SetX(iconPaddingX);
        iconpm[wParam]->SetY(round(iconPaddingY * 0.575));
        if (pm[wParam]->GetSimpleFilename() == L"") pm[wParam]->SetSimpleFilename(L"Failed to find item");
        HBITMAP capturedBitmap;
        capturedBitmap = CreateTextBitmap(pm[wParam]->GetSimpleFilename().c_str(), innerSizeX - 4 * flScaleFactor, lines_basedOnEllipsis, DT_END_ELLIPSIS);
        IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
        HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor);
        IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), 2);
        Value* bitmap = DirectUI::Value::CreateGraphic(capturedBitmap, 2, 0xffffffff, false, false, false);
        Value* bitmapSh = DirectUI::Value::CreateGraphic(shadowBitmap, 2, 0xffffffff, false, false, false);
        filepm[wParam]->SetValue(Element::ContentProp, 1, bitmap);
        fileshadowpm[wParam]->SetValue(Element::ContentProp, 1, bitmapSh);
        bitmap->Release();
        bitmapSh->Release();
        DeleteObject(capturedBitmap);
        DeleteObject(shadowBitmap);
        break;
    }
    case WM_USER + 4: {
        shadowpm[wParam]->SetAlpha(255);
        shadowpm[wParam]->SetWidth((globaliconsz + 16) * flScaleFactor);
        shadowpm[wParam]->SetHeight((globaliconsz + 16) * flScaleFactor);
        shadowpm[wParam]->SetX(iconPaddingX - 8 * flScaleFactor);
        shadowpm[wParam]->SetY((iconPaddingY * 0.575) - 6 * flScaleFactor);
        shortpm[wParam]->SetAlpha(255);
        shortpm[wParam]->SetWidth(globalshiconsz * flScaleFactor);
        shortpm[wParam]->SetHeight(globalshiconsz * flScaleFactor);
        shortpm[wParam] ->SetX(iconPaddingX);
        shortpm[wParam]->SetY((iconPaddingY * 0.575) + (globaliconsz - globalshiconsz) * flScaleFactor);
        break;
    }
    case WM_USER + 5: {
        POINT ppt;
        GetCursorPos(&ppt);
        ScreenToClient(wnd->GetHWND(), &ppt);
        if (ppt.x >= origX) {
            selector->SetWidth(ppt.x - origX);
            selector->SetX(origX);
            selector2->SetWidth(ppt.x - origX);
            selector2->SetX(origX);
        }
        if (ppt.x < origX) {
            selector->SetWidth(origX - ppt.x);
            selector->SetX(ppt.x);
            selector2->SetWidth(origX - ppt.x);
            selector2->SetX(ppt.x);
        }
        if (ppt.y >= origY) {
            selector->SetHeight(ppt.y - origY);
            selector->SetY(origY);
            selector2->SetHeight(ppt.y - origY);
            selector2->SetY(origY);
        }
        if (ppt.y < origY) {
            selector->SetHeight(origY - ppt.y);
            selector->SetY(ppt.y);
            selector2->SetHeight(origY - ppt.y);
            selector2->SetY(ppt.y);
        }
        break;
    }
    case WM_USER + 6: {
        fullscreenpopupbase->SetVisible(true);
        fullscreeninner->SetVisible(true);
        fullscreeninner->SetY(dimensions.bottom * 0.1 * (1 - py[popupframe - 1]) + 1);
        break;
    }
    case WM_USER + 7: {
        checkifelemexists = false;
        subviewwnd->ShowWindow(SW_HIDE);
        centered->DestroyAll(true);
        //if (issubviewopen) {
        //    ShowSimpleView();
        //}
        //issubviewopen = false;
        break;
    }
    case WM_USER + 8: {
        parser->CreateElement(L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninner);
        centered->Add((Element**)&fullscreeninner, 1);
        centered->SetMinSize(800 * flScaleFactor, 480 * flScaleFactor);
        fullscreeninner->SetMinSize(800 * flScaleFactor, 480 * flScaleFactor);
    }
    case WM_USER + 9: {
        subpm[wParam]->SetWidth(innerSizeX);
        subpm[wParam]->SetHeight(innerSizeY + textm.tmHeight + 23 * flScaleFactor);
        //subpm[wParam]->SetX(subpm[wParam]->GetInternalXPos()); // round((720 - 2 * subpm[wParam].x) * 0.15 * (1 - bezierProgress) + subpm[wParam].x));
        //subpm[wParam]->SetY(subpm[wParam]->GetInternalYPos()); // round((360 - 2 * subpm[wParam].y) * 0.15 * (1 - bezierProgress) + subpm[wParam].y));
        int textlines = 1;
        if (textm.tmHeight <= 18 * flScaleFactor) textlines = 2;
        subfilepm[wParam]->SetHeight(textm.tmHeight * textlines + 4 * flScaleFactor);
        if (subfilepm[wParam]->GetHeight() > (iconPaddingY * 0.575 + 48)) subfilepm[wParam]->SetHeight(iconPaddingY * 0.575 + 48);
        subiconpm[wParam]->SetWidth(round(globaliconsz * flScaleFactor));
        subiconpm[wParam]->SetHeight(round(globaliconsz * flScaleFactor));
        subiconpm[wParam]->SetX(iconPaddingX);
        subiconpm[wParam]->SetY(round(iconPaddingY * 0.575));
        HBITMAP capturedBitmap = CreateTextBitmap(subpm[wParam]->GetSimpleFilename().c_str(), innerSizeX, textm.tmHeight * textlines, DT_END_ELLIPSIS);
        if (theme) {
            IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1);
            IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 0.9);
        }
        else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
        Value* bitmap = DirectUI::Value::CreateGraphic(capturedBitmap, 2, 0xffffffff, false, false, false);
        subfilepm[wParam]->SetValue(Element::ContentProp, 1, bitmap);
        bitmap->Release();
        DeleteObject(capturedBitmap);
        break;
    }
    case WM_USER + 10: {
        subshadowpm[wParam]->SetAlpha(255);
        subshadowpm[wParam]->SetWidth((globaliconsz + 16) * flScaleFactor);
        subshadowpm[wParam]->SetHeight((globaliconsz + 16) * flScaleFactor);
        subshadowpm[wParam]->SetX(iconPaddingX - 8 * flScaleFactor);
        subshadowpm[wParam]->SetY((iconPaddingY * 0.575) - 6 * flScaleFactor);
        subshortpm[wParam]->SetAlpha(255);
        subshortpm[wParam]->SetWidth(globalshiconsz * flScaleFactor);
        subshortpm[wParam]->SetHeight(globalshiconsz * flScaleFactor);
        subshortpm[wParam]->SetX(iconPaddingX);
        subshortpm[wParam]->SetY((iconPaddingY * 0.575) + (globaliconsz - globalshiconsz) * flScaleFactor);
        break;
    }
    case WM_USER + 11: {
        break;
    }
    case WM_USER + 12: {
        if (checkifelemexists == true) dirnameanimator->SetWidth((160 * (1 - py[dframe - 1])) * flScaleFactor);
        break;
    }
    case WM_USER + 13: {
        if (checkifelemexists == true) tasksanimator->SetWidth((80 * (1 - py[tframe - 1])) * flScaleFactor);
        break;
    }
    case WM_USER + 14: {
        int padding = 3, paddingInner = 2;
        if (globaliconsz > 96) {
            padding = 18;
            paddingInner = 12;
        }
        else if (globaliconsz > 48) {
            padding = 12;
            paddingInner = 8;
        }
        else if (globaliconsz > 32) {
            padding = 6;
            paddingInner = 4;
        }
        for (int icon = 0; icon < pm.size(); icon++) {
            if (pm[icon]->GetDirState() == true && treatdirasgroup == true) {
                iconpm[icon]->DestroyAll(true);
                iconpm[icon]->SetClass(L"groupthumbnail");
                shadowpm[icon]->SetAlpha(0);
            }
        }
        for (int icon = 0; icon < pm.size(); icon++) {
            if (pm[icon]->GetDirState() == true && treatdirasgroup == true) {
                int x = padding * flScaleFactor, y = padding * flScaleFactor;
                vector<wstring> strs;
                unsigned short count = 0;
                wstring folderPath = RemoveQuotes(pm[icon]->GetFilename());
                EnumerateFolder((LPWSTR)folderPath.c_str(), nullptr, false, true, &count, nullptr, 4);
                EnumerateFolderForThumbnails((LPWSTR)folderPath.c_str(), &strs, 4);
                for (int thumbs = 0; thumbs < count; thumbs++) {
                    HBITMAP thumbIcon = GetShellItemImage(strs[thumbs].c_str(), globalgpiconsz * flScaleFactor, globalgpiconsz * flScaleFactor);
                    if (isColorized) {
                        IterateBitmap(thumbIcon, StandardBitmapPixelHandler, 1, 0, 1);
                    }
                    Value* vThumbIcon = DirectUI::Value::CreateGraphic(thumbIcon, 2, 0xffffffff, false, false, false);
                    Element* GroupedIcon;
                    parser->CreateElement(L"GroupedIcon", NULL, NULL, NULL, (Element**)&GroupedIcon);
                    iconpm[icon]->Add((Element**)&GroupedIcon, 1);
                    GroupedIcon->SetWidth(globalgpiconsz * flScaleFactor), GroupedIcon->SetHeight(globalgpiconsz * flScaleFactor);
                    GroupedIcon->SetX(x), GroupedIcon->SetY(y);
                    x += ((globalgpiconsz + paddingInner) * flScaleFactor);
                    if (x > (globaliconsz - globalgpiconsz) * flScaleFactor) {
                        x = padding * flScaleFactor;
                        y += ((globalgpiconsz + paddingInner) * flScaleFactor);
                    }
                    GroupedIcon->SetValue(Element::ContentProp, 1, vThumbIcon);
                    DeleteObject(thumbIcon);
                    vThumbIcon->Release();
                }
            }
        }
        break;
    }
    case WM_USER + 15: {
        ShowSimpleView();
        break;
    }
    }
    return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
}

unsigned long animate(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
    //this_thread::sleep_for(chrono::milliseconds(static_cast<int>(pm[yV->y].y * 0.5)));
    SendMessageW(wnd->GetHWND(), WM_USER + 1, yV->y, NULL);
    free(yV);
    return 0;
}

unsigned long subanimate(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
    SendMessageW(wnd->GetHWND(), WM_USER + 2, yV->y, NULL);
    free(yV);
    return 0;
}

unsigned long fastin(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
    //this_thread::sleep_for(chrono::milliseconds(static_cast<int>(pm[yV->y].y * 0.5)));
    SendMessageW(wnd->GetHWND(), WM_USER + 4, yV->y, NULL);
    //for (int m = 1; m <= 24; m++) {
        //frame[yV->y] = 24;
        SendMessageW(wnd->GetHWND(), WM_USER + 3, yV->y, NULL);
        //this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 400)));
    //}
    free(yV);
    return 0;
}

unsigned long subfastin(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
    SendMessageW(wnd->GetHWND(), WM_USER + 10, yV->y, NULL);
    //frame[yV->y] = 24;
    SendMessageW(wnd->GetHWND(), WM_USER + 9, yV->y, NULL);
    free(yV);
    return 0;
}

unsigned long animate5(LPVOID lpParam) {
    CubicBezier(30, px, py, 0.05, 0.9, 0.3, 1.0);
    SendMessageW(wnd->GetHWND(), WM_USER + 8, NULL, NULL);
    for (int m = 1; m <= 30; m++) {
        popupframe = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 6, NULL, NULL);
        this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 200)));
    }
    return 0;
}

unsigned long animate6(LPVOID lpParam) {
    //this_thread::sleep_for(chrono::milliseconds(100));
    SendMessageW(wnd->GetHWND(), WM_USER + 7, NULL, NULL);
    return 0;
}

unsigned long grouptitlebaranimation(LPVOID lpParam) {
    this_thread::sleep_for(chrono::milliseconds(750));
    for (int m = 1; m <= 48; m++) {
        dframe = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 12, NULL, NULL);
        this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 600)));
    }
    return 0;
}
unsigned long grouptasksanimation(LPVOID lpParam) {
    for (int m = 1; m <= 48; m++) {
        tframe = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 13, NULL, NULL);
        this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 450)));
    }
    return 0;
}

void fullscreenAnimation(int width, int height) {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    //HDC hdcWindow = GetDC(wnd->GetHWND());
    //HDC hdcMem = CreateCompatibleDC(hdcWindow);
    //HBITMAP hbmCapture = CreateCompatibleBitmap(hdcWindow, dimensions.right * 0.08, dimensions.bottom * 0.08);
    //HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
    //SetStretchBltMode(hdcMem, HALFTONE);
    //StretchBlt(hdcMem, 0, 0, dimensions.right * 0.08, dimensions.bottom * 0.08, hdcWindow, 0, 0, dimensions.right, dimensions.bottom, SRCCOPY);
    //SelectObject(hdcMem, hbmOld);
    //DeleteDC(hdcMem);
    //ReleaseDC(wnd->GetHWND(), hdcWindow);
    //IterateBitmap(hbmCapture, StandardBitmapPixelHandler, 2, 2 * flScaleFactor, 1);
    //Value* bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 4, 0xffffffff, false, false, true);
    //fullscreenpopup->SetValue(Element::BackgroundProp, 1, bitmap);
    //bitmap->Release();
    //this_thread::sleep_for(chrono::milliseconds(80));
    pSubview->SetAlpha(255);
    subviewwnd->ShowWindow(SW_SHOW);
    parser->CreateElement(L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninner);
    centered->Add((Element**)&fullscreeninner, 1);
    centered->SetMinSize(width, height);
    centered->SetBackgroundColor(0);
    fullscreeninner->SetMinSize(width, height);
    fullscreenpopupbase->SetVisible(true);
    fullscreeninner->SetVisible(true);
    BlurBackground(GetWorkerW(), true);
}
void fullscreenAnimation2() {
    DWORD animThread;
    HANDLE animThreadHandle = CreateThread(0, 0, animate6, NULL, 0, &animThread);
}
void ShowPopupCore() {
    pSubview->SetAlpha(255);
    subviewwnd->ShowWindow(SW_SHOW);
    fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor);
}
void HidePopupCore() {
    editmode = false;
    SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
    pSubview->SetAlpha(0);
    subviewwnd->ShowWindow(SW_HIDE);
    fullscreenAnimation2();
    //frame.clear();
    subpm.clear();
    subiconpm.clear();
    subshortpm.clear();
    subshadowpm.clear();
    subfilepm.clear();
    SimpleViewTop->SetLayoutPos(-3);
    SimpleViewBottom->SetLayoutPos(-3);
    nextpage->SetWidth(0);
    prevpage->SetWidth(0);
    BlurBackground(GetWorkerW(), false);
}

wstring bufferOpenInExplorer;
void OpenGroupInExplorer(Element* elem, Event* iev) {
    if (iev->uidType == TouchButton::Click) {
        SHELLEXECUTEINFOW execInfo = {};
        execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
        execInfo.lpVerb = L"open";
        execInfo.nShow = SW_SHOWNORMAL;
        execInfo.lpFile = bufferOpenInExplorer.c_str();
        ShellExecuteExW(&execInfo);
    }
}

int clicks = 1;
BYTE* shellstate;
unsigned long DoubleClickHandler(LPVOID lpParam) {
    wchar_t* dcms = GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"DoubleClickSpeed");
    this_thread::sleep_for(chrono::milliseconds(_wtoi(dcms)));
    clicks = 1;
    return 0;
}

void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB) {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    pageElem->SetX(dimensions.right * offsetL);
    pageElem->SetY(dimensions.bottom * offsetT);
    pageElem->SetWidth(dimensions.right * offsetR);
    pageElem->SetHeight(dimensions.bottom * offsetB);
    WCHAR currentPage[64];
    StringCchPrintfW(currentPage, 64, L"Page %d / %d", currentPageID, maxPageID);
    pageinfo->SetContentString(currentPage);
}
unsigned long LoadOtherPageThumbnail(LPVOID lpParam) {
    HidePopupCore();
    this_thread::sleep_for(chrono::milliseconds(100));
    SendMessageW(wnd->GetHWND(), WM_USER + 15, NULL, NULL);
    return 0;
}
void GoToPrevPage(Element* elem, Event* iev) {
    static const int savedanim = UIContainer->GetAnimation();
    static RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    if (iev->uidType == TouchButton::Click) {
        currentPageID--;
        for (int items = 0; items < validItems; items++) {
            if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
            else pm[items]->SetVisible(false);
        }
        if (editmode) {
            fullscreeninner->SetBackgroundStdColor(7);
            TogglePage(nextpage, 0.9, 0.2, 0.1, 0.6);
            if (currentPageID == 1) TogglePage(prevpage, 0, 0.2, 0, 0.6);
            DWORD dd;
            HANDLE thumbnailThread = CreateThread(0, 0, LoadOtherPageThumbnail, NULL, 0, &dd);
        }
        else {
            UIContainer->SetAnimation(NULL);
            UIContainer->SetX(-(dimensions.right - dimensions.left));
            UIContainer->SetAnimation(savedanim);
            UIContainer->SetX(0);
        }
        nextpageMain->SetVisible(true);
        if (currentPageID == 1) prevpageMain->SetVisible(false);
    }
}
void GoToNextPage(Element* elem, Event* iev) {
    static const int savedanim = UIContainer->GetAnimation();
    static RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    if (iev->uidType == TouchButton::Click) {
        currentPageID++;
        for (int items = 0; items < validItems; items++) {
            if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
            else pm[items]->SetVisible(false);
        }
        if (editmode) {
            fullscreeninner->SetBackgroundStdColor(7);
            TogglePage(prevpage, 0, 0.2, 0.1, 0.6);
            if (currentPageID == maxPageID) TogglePage(nextpage, 0.9, 0.2, 0, 0.6);
            DWORD dd;
            HANDLE thumbnailThread = CreateThread(0, 0, LoadOtherPageThumbnail, NULL, 0, &dd);
        }
        else {
            UIContainer->SetAnimation(NULL);
            UIContainer->SetX(dimensions.right - dimensions.left);
            UIContainer->SetAnimation(savedanim);
            UIContainer->SetX(0);
        }
        prevpageMain->SetVisible(true);
        if (currentPageID == maxPageID) nextpageMain->SetVisible(false);
    }
}

vector<HBITMAP> GetDesktopIcons() {
    vector<HBITMAP> bmResult;
    for (int i = 0; i < pm.size(); i++) {
        if (pm[i]->GetDirState() == true && treatdirasgroup == true) bmResult.push_back(NULL);
        else bmResult.push_back(GetShellItemImage(RemoveQuotes(pm[i]->GetFilename()).c_str(), globaliconsz, globaliconsz));
    }
    return bmResult;
}
vector<HBITMAP> GetSubdirectoryIcons(int limit = subpm.size(), int width = globaliconsz, int height = globaliconsz) {
    vector<HBITMAP> bmResult;
    for (int i = 0; i < limit; i++) {
        bmResult.push_back(GetShellItemImage(RemoveQuotes(subpm[i]->GetFilename()).c_str(), width, height));
    }
    return bmResult;
}

unsigned long ApplyThumbnailIcons(LPVOID lpParam) {
    PostMessageW(wnd->GetHWND(), WM_USER + 14, NULL, NULL);
    return 0;
}

void ApplyIcons(vector<LVItem*> pmLVItem, vector<Element*> pmIcon, vector<Element*> pmIconShadow, vector<Element*> pmShortcut, vector<HBITMAP> iconstofetch, bool subdirectory) {
    HINSTANCE testInst = LoadLibraryW(L"imageres.dll");
    vector<HBITMAP> icons = iconstofetch;
    for (int icon = 0; icon < pmIcon.size(); icon++) {
        if (icon < validItems || subdirectory == true) {
            HICON icoShortcut = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(163), IMAGE_ICON, globalshiconsz * flScaleFactor, globalshiconsz * flScaleFactor, LR_SHARED);
            // The use of the 3 lines below is because we can't use a fully transparent bitmap
            HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"shell32.dll"), MAKEINTRESOURCE(24), IMAGE_ICON, 16, 16, LR_SHARED);
            HBITMAP dummyii = IconToBitmap(dummyi);
            IterateBitmap(dummyii, SimpleBitmapPixelHandler, 0, 0, 0.005);
            HBITMAP bmp{};
            if (icon < pm.size()) {
                if (pm[icon]->GetDirState() == false || treatdirasgroup == false || pmIcon != iconpm) bmp = icons[icon];
                else bmp = dummyii;
            }
            else if (treatdirasgroup == false || pmIcon != iconpm) bmp = icons[icon];
            else bmp = dummyii;
            if (bmp != dummyii) {
                HBITMAP bmpShadow = AddPaddingToBitmap(bmp, 8 * flScaleFactor);
                IterateBitmap(bmpShadow, SimpleBitmapPixelHandler, 0, (int)(4 * flScaleFactor), 0.33);
                Value* bitmapShadow = DirectUI::Value::CreateGraphic(bmpShadow, 2, 0xffffffff, false, false, false);
                pmIconShadow[icon]->SetValue(Element::ContentProp, 1, bitmapShadow);
                DeleteObject(bmpShadow);
                bitmapShadow->Release();
            }
            HBITMAP bmpShortcut = IconToBitmap(icoShortcut);
            if (isColorized) {
                IterateBitmap(bmp, StandardBitmapPixelHandler, 1, 0, 1);
                IterateBitmap(bmpShortcut, StandardBitmapPixelHandler, 1, 0, 1);
            }
            IterateBitmap(bmpShortcut, UndoPremultiplication, 1, 0, 1);
            Value* bitmap = DirectUI::Value::CreateGraphic(bmp, 2, 0xffffffff, false, false, false);
            Value* bitmapShortcut = DirectUI::Value::CreateGraphic(bmpShortcut, 2, 0xffffffff, false, false, false);
            pmIcon[icon]->SetValue(Element::ContentProp, 1, bitmap);
            if (pmLVItem[icon]->GetShortcutState() == true) pmShortcut[icon]->SetValue(Element::ContentProp, 1, bitmapShortcut);
            DeleteObject(icoShortcut);
            if (bmp != nullptr) DeleteObject(bmp);
            DeleteObject(bmpShortcut);
            bitmap->Release();
            bitmapShortcut->Release();
        }
        else if (logging == IDYES) MainLogger.WriteLine(L"Warning: Empty filename, icon application may fail...");
    }
    icons.clear();
}

void SelectSubItem(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        SHELLEXECUTEINFOW execInfo = {};
        execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
        execInfo.lpVerb = L"open";
        execInfo.nShow = SW_SHOWNORMAL;
        for (int items = 0; items < subpm.size(); items++) {
            if (subpm[items] == elem) {
                wstring temp = subpm[items]->GetFilename();
                execInfo.lpFile = temp.c_str();
                ShellExecuteExW(&execInfo);
            }
        }
    }
}

void ShowDirAsGroup(LPCWSTR filename, LPCWSTR simplefilename) {
    SendMessageW(hWndTaskbar, WM_COMMAND, 419, 0);
    fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor);
    Element* groupdirectory{};
    parser->CreateElement(L"groupdirectory", NULL, NULL, NULL, (Element**)&groupdirectory);
    fullscreeninner->Add((Element**)&groupdirectory, 1);
    ScrollViewer* groupdirlist = (ScrollViewer*)groupdirectory->FindDescendent(StrToID(L"groupdirlist"));
    SubUIContainer = (RichText*)groupdirlist->FindDescendent(StrToID(L"SubUIContainer"));
    unsigned short count = 0;
    int count2{};
    EnumerateFolder((LPWSTR)filename, nullptr, false, true, &count);
    CubicBezier(48, px, py, 0.1, 0.9, 0.2, 1.0);
    if (count <= 128 && count > 0) {
        for (int i = 0; i < count; i++) {
            LVItem* outerElemGrouped;
            parser->CreateElement(L"outerElemGrouped", NULL, NULL, NULL, (Element**)&outerElemGrouped);
            SubUIContainer->Add((Element**)&outerElemGrouped, 1);
            iconElem = (Element*)outerElemGrouped->FindDescendent(StrToID(L"iconElem"));
            shortcutElem = (Element*)outerElemGrouped->FindDescendent(StrToID(L"shortcutElem"));
            iconElemShadow = (Element*)outerElemGrouped->FindDescendent(StrToID(L"iconElemShadow"));
            textElem = (RichText*)outerElemGrouped->FindDescendent(StrToID(L"textElem"));
            subpm.push_back(outerElemGrouped);
            subiconpm.push_back(iconElem);
            subshortpm.push_back(shortcutElem);
            subshadowpm.push_back(iconElemShadow);
            subfilepm.push_back(textElem);
        }
        EnumerateFolder((LPWSTR)filename, &subpm, true, false, nullptr, &count2);
        int x = 0, y = 0;
        Value* v;
        RECT dimensions;
        dimensions = *(groupdirectory->GetPadding(&v));
        int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
        int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 21) * flScaleFactor;
        DWORD* animThread = new DWORD[count];
        DWORD* animThread2 = new DWORD[count];
        HANDLE* animThreadHandle = new HANDLE[count];
        HANDLE* animThreadHandle2 = new HANDLE[count];
        ApplyIcons(subpm, subiconpm, subshadowpm, subshortpm, GetSubdirectoryIcons(), true);
        for (int j = 0; j < count; j++) {
            if (subpm[j]->GetHiddenState() == true) {
                subiconpm[j]->SetAlpha(128);
                subshadowpm[j]->SetVisible(false);
                subfilepm[j]->SetAlpha(128);
            }
            assignFn(subpm[j], SelectSubItem);
            assignFn(subpm[j], SubItemRightClick);
            subpm[j]->SetClass(L"singleclicked");
            subpm[j]->SetX(x), subpm[j]->SetY(y);
            yValue* yV = new yValue{ j };
            yValue* yV2 = new yValue{ j };
            x += outerSizeX;
            if (x > 800 * flScaleFactor - (dimensions.left + dimensions.right + outerSizeX)) {
                x = 0;
                y += outerSizeY;
            }
            animThreadHandle[j] = CreateThread(0, 0, subanimate, (LPVOID)yV, 0, &(animThread[j]));
            animThreadHandle2[j] = CreateThread(0, 0, subfastin, (LPVOID)yV2, 0, &(animThread2[j]));
        }
        x += outerSizeX;
        if (x > 800 * flScaleFactor - (dimensions.left + dimensions.right + outerSizeX)) y += outerSizeY;
        SubUIContainer->SetHeight(y);
        delete[] animThread;
        delete[] animThread2;
        delete[] animThreadHandle;
        delete[] animThreadHandle2;
        v->Release();
    }
    else {
        if (count > 128) {
            SubUIContainer->SetContentString(L"This folder is too large.");
        }
        else SubUIContainer->SetContentString(L"This folder is empty.");
    }
    dirnameanimator = (Element*)groupdirectory->FindDescendent(StrToID(L"dirnameanimator"));
    tasksanimator = (Element*)groupdirectory->FindDescendent(StrToID(L"tasksanimator"));
    RichText* dirname = (RichText*)groupdirectory->FindDescendent(StrToID(L"dirname"));
    dirname->SetContentString(simplefilename);
    dirname->SetAlpha(255);
    RichText* dirdetails = (RichText*)groupdirectory->FindDescendent(StrToID(L"dirdetails"));
    WCHAR itemCount[64];
    if (count == 1) StringCchPrintfW(itemCount, 64, L"contains 1 item");
    else StringCchPrintfW(itemCount, 64, L"contains %d items", count);
    dirdetails->SetContentString(itemCount);
    dirdetails->SetAlpha(160);
    Element* tasks = (Element*)groupdirectory->FindDescendent(StrToID(L"tasks"));
    checkifelemexists = true;
    DWORD animThread3;
    DWORD animThread4;
    HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
    HANDLE animThreadHandle4 = CreateThread(0, 0, grouptasksanimation, NULL, 0, &animThread4);
    TouchButton* Customize = (TouchButton*)groupdirectory->FindDescendent(StrToID(L"Customize"));
    TouchButton* OpenInExplorer = (TouchButton*)groupdirectory->FindDescendent(StrToID(L"OpenInExplorer"));
    Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
    assignFn(OpenInExplorer, OpenGroupInExplorer);
    bufferOpenInExplorer = (wstring)filename;
}

void ShowPage1(Element* elem, Event* iev) {
    if (iev->uidType == TouchButton::Click) {
        PageTab2->SetSelected(false);
        PageTab1->SetSelected(true);
        SubUIContainer->DestroyAll(true);
        Element* SettingsPage1;
        parser->CreateElement(L"SettingsPage1", NULL, NULL, NULL, (Element**)&SettingsPage1);
        SubUIContainer->Add((Element**)&SettingsPage1, 1);
        Button* ItemCheckboxes = (Button*)SettingsPage1->FindDescendent(StrToID(L"ItemCheckboxes"));
        Button* ShowHiddenFiles = (Button*)SettingsPage1->FindDescendent(StrToID(L"ShowHiddenFiles"));
        Button* FilenameExts = (Button*)SettingsPage1->FindDescendent(StrToID(L"FilenameExts"));
        Button* TreatDirAsGroup = (Button*)SettingsPage1->FindDescendent(StrToID(L"TreatDirAsGroup"));
        ItemCheckboxes->SetSelected(showcheckboxes);
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden") == 1) ShowHiddenFiles->SetSelected(true);
        else ShowHiddenFiles->SetSelected(false);
        FilenameExts->SetSelected(GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt"));
        TreatDirAsGroup->SetSelected(treatdirasgroup);
        assignFn(ItemCheckboxes, ToggleCheckbox);
        assignFn(ShowHiddenFiles, ToggleShowHidden);
        assignFn(FilenameExts, ToggleFilenameExts);
        assignFn(TreatDirAsGroup, ToggleGroupMode);
    }
}
void ShowPage2(Element* elem, Event* iev) {
    if (iev->uidType == TouchButton::Click) {
        PageTab1->SetSelected(false);
        PageTab2->SetSelected(true);
        SubUIContainer->DestroyAll(true);
        Element* SettingsPage2;
        parser->CreateElement(L"SettingsPage2", NULL, NULL, NULL, (Element**)&SettingsPage2);
        SubUIContainer->Add((Element**)&SettingsPage2, 1);
        Button* EnableAccent = (Button*)SettingsPage2->FindDescendent(StrToID(L"EnableAccent"));
        EnableAccent->SetSelected(isColorized);
        assignFn(EnableAccent, ToggleAccentIcons);
    }
}
void ShowSettings(Element* elem, Event* iev) {
    if (iev->uidType == TouchButton::Click) {
        subviewwnd->ShowWindow(SW_HIDE);
        centered->DestroyAll(true);
        ShowPopupCore();
        SimpleViewTop->SetLayoutPos(-3);
        SimpleViewBottom->SetLayoutPos(-3);
        nextpage->SetWidth(0);
        prevpage->SetWidth(0);
        issubviewopen = 1;
        Element* settingsview{};
        parser->CreateElement(L"settingsview", NULL, NULL, NULL, (Element**)&settingsview);
        fullscreeninner->Add((Element**)&settingsview, 1);
        ScrollViewer* settingslist = (ScrollViewer*)settingsview->FindDescendent(StrToID(L"settingslist"));
        SubUIContainer = (RichText*)settingsview->FindDescendent(StrToID(L"SubUIContainer"));
        PageTab1 = (TouchButton*)settingsview->FindDescendent(StrToID(L"PageTab1"));
        PageTab2 = (TouchButton*)settingsview->FindDescendent(StrToID(L"PageTab2"));
        assignFn(PageTab1, ShowPage1);
        assignFn(PageTab2, ShowPage2);
        ShowPage1(elem, iev);
        CubicBezier(48, px, py, 0.1, 0.9, 0.2, 1.0);
        dirnameanimator = (Element*)settingsview->FindDescendent(StrToID(L"dirnameanimator"));
        RichText* name = (RichText*)settingsview->FindDescendent(StrToID(L"name"));
        name->SetAlpha(255);
        checkifelemexists = true;
        DWORD animThread3;
        HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
    }
}
void ExitWindow(Element* elem, Event* iev) {
    if (iev->uidType == TouchButton::Click) {
        SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
        SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, NULL);
    }
}

Element* elemStorage;
void SelectItem(Element* elem, Event* iev) {
    static int validation = 0;
    if (iev->uidType == Button::Click) {
        validation++;
        Button* checkbox = (Button*)elem->FindDescendent(StrToID(L"checkboxElem"));
        if (GetAsyncKeyState(VK_CONTROL) == 0 && checkbox->GetMouseFocused() == false) {
            for (int items = 0; items < validItems; items++) {
                pm[items]->SetSelected(false);
                if (cbpm[items]->GetSelected() == false && showcheckboxes == 1) cbpm[items]->SetVisible(false);
            }
        }
        if (elem != emptyspace && checkbox->GetMouseFocused() == false) elem->SetSelected(!elem->GetSelected());
        if (validation % 2 == 1) {
            if (elem != emptyspace && checkbox->GetMouseFocused() == true) elem->SetSelected(!elem->GetSelected());
        }
        if (showcheckboxes == 1) checkbox->SetVisible(true);
        if (shellstate[4] >= 48 && shellstate[4] <= 63) {
            if (elem == elemStorage) clicks++; else clicks = 0;
            DWORD doubleClickThread{};
            HANDLE doubleClickThreadHandle = CreateThread(0, 0, DoubleClickHandler, NULL, 0, &doubleClickThread);
            elemStorage = elem;
        }
        SHELLEXECUTEINFOW execInfo = {};
        execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
        execInfo.lpVerb = L"open";
        execInfo.nShow = SW_SHOWNORMAL;
        for (int items = 0; items < validItems; items++) {
            if (pm[items]->GetMemorySelected() != pm[items]->GetSelected()) {
                float spacingInternal = CalcTextLines(pm[items]->GetSimpleFilename().c_str(), pm[items]->GetWidth());
                int extraBottomSpacing = (pm[items]->GetSelected() == true) ? ceil(spacingInternal) * textm.tmHeight : floor(spacingInternal) * textm.tmHeight;
                textElem = (RichText*)pm[items]->FindDescendent(StrToID(L"textElem"));
                textElemShadow = (RichText*)pm[items]->FindDescendent(StrToID(L"textElemShadow"));
                if (spacingInternal == 1.5) {
                    if (pm[items]->GetSelected() == true) pm[items]->SetHeight(pm[items]->GetHeight() + extraBottomSpacing * 0.5);
                    else pm[items]->SetHeight(pm[items]->GetHeight() - extraBottomSpacing);
                }
                textElem->SetHeight(extraBottomSpacing + 4 * flScaleFactor);
                textElemShadow->SetHeight(extraBottomSpacing + 5 * flScaleFactor);
                HBITMAP capturedBitmap = CreateTextBitmap(pm[items]->GetSimpleFilename().c_str(), pm[items]->GetWidth() - 4 * flScaleFactor, extraBottomSpacing, DT_END_ELLIPSIS);
                IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
                HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor);
                IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), 2);
                Value* bitmap = DirectUI::Value::CreateGraphic(capturedBitmap, 2, 0xffffffff, false, false, false);
                Value* bitmapSh = DirectUI::Value::CreateGraphic(shadowBitmap, 2, 0xffffffff, false, false, false);
                textElem->SetValue(Element::ContentProp, 1, bitmap);
                textElemShadow->SetValue(Element::ContentProp, 1, bitmapSh);
                bitmap->Release();
                bitmapSh->Release();
                DeleteObject(capturedBitmap);
                DeleteObject(shadowBitmap);
            }
            pm[items]->SetMemorySelected(pm[items]->GetSelected());
        }
        if (clicks % 2 == 1 && checkbox->GetMouseFocused() == false) {
            for (int items = 0; items < pm.size(); items++) {
                if (pm[items] == elem) {
                    wstring temp = RemoveQuotes(pm[items]->GetFilename());
                    execInfo.lpFile = temp.c_str();
                    if (pm[items]->GetDirState() == true && treatdirasgroup == true) {
                        wstring temp2 = pm[items]->GetSimpleFilename();
                        ShowDirAsGroup(execInfo.lpFile, temp2.c_str());
                    }
                    else ShellExecuteExW(&execInfo);
                }
            }
        }
    }
}

void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
    checkboxElem = (Button*)elem->FindDescendent(StrToID(L"checkboxElem"));   
    if (pProp == Element::MouseFocusedProp() && showcheckboxes == 1) {
        for (int items = 0; items < validItems; items++) {
            if (cbpm[items]->GetSelected() == false) cbpm[items]->SetVisible(false);
        }
        checkboxElem->SetVisible(true);
    }
}
void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
    UpdateCache u;
    if (pProp == Element::MouseFocusedProp()) {
        Element* parent = elem->GetParent();
        Value* v = elem->GetValue(Element::MouseFocusedProp, 1, &u);
        Element* item = parent->FindDescendent(StrToID(L"innerElem"));
        //item->SetAlpha(255);
        item->SetValue(Element::MouseFocusedProp(), 1, v);
    }
}

bool isPressed = 0;
unsigned long UpdateMarqueeSelectorPosition(LPVOID lpParam) {
    while (true) {
        SendMessageW(wnd->GetHWND(), WM_USER + 5, NULL, NULL);
        this_thread::sleep_for(chrono::milliseconds(10));
        if (!isPressed) break;
    }
    return 0;
}

void MarqueeSelector(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pv2) {
    DWORD marqueeThread;
    HANDLE marqueeThreadHandle;
    HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, 16, 16, LR_SHARED);
    HBITMAP selectorBmp = IconToBitmap(dummyi);
    if (pProp == Button::CapturedProp()) {
        POINT ppt;
        GetCursorPos(&ppt);
        ScreenToClient(wnd->GetHWND(), &ppt);
        origX = ppt.x;
        origY = ppt.y;
        selector->SetX(origX);
        selector->SetY(origY);
        selector->SetVisible(true);
        selector->SetLayoutPos(-2);
        IterateBitmap(selectorBmp, SimpleBitmapPixelHandler, 3, 0, 0.33);
        Value* selectorBmpV = DirectUI::Value::CreateGraphic(selectorBmp, 7, 0xffffffff, false, false, false);
        selector2->SetValue(Element::BackgroundProp, 1, selectorBmpV);
        selector2->SetVisible(true);
        selector2->SetLayoutPos(-2);
        isPressed = 1;
        marqueeThreadHandle = CreateThread(0, 0, UpdateMarqueeSelectorPosition, NULL, 0, &marqueeThread);
        selectorBmpV->Release();
    }
    else if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        selector->SetVisible(false);
        selector->SetLayoutPos(-3);
        selector2->SetVisible(false);
        selector2->SetLayoutPos(-3);
        isPressed = 0;
    }
}

void testEventListener3(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        switch (pSubview->GetAlpha()) {
        case 0:
            if (elem != fullscreenpopupbase) {
                ShowPopupCore();
            }
            break;
        case 255:
            if (centered->GetMouseWithin() == false && elem->GetMouseFocused() == true) {
                HidePopupCore();
            }
            break;
        }
    }
}

void RearrangeIcons(bool animation, bool reloadicons) {
    maxPageID = 1;
    prevpageMain->SetVisible(false);
    nextpageMain->SetVisible(false);
    GetPos();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 1 of 5 complete: Imported your desktop icon positions.");
    unsigned int count = pm.size();
    static const int savedanim = (pm[0] != nullptr) ? pm[0]->GetAnimation() : NULL;
    if (reloadicons) {
        ApplyIcons(pm, iconpm, shadowpm, shortpm, GetDesktopIcons(), false);
        DWORD dd;
        HANDLE thumbnailThread = CreateThread(0, 0, ApplyThumbnailIcons, NULL, 0, &dd);
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 2 of 5 complete: Applied icons to the relevant desktop items.");
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    int x = 4 * flScaleFactor, y = 4 * flScaleFactor;
    DWORD* animThread = new DWORD[count];
    DWORD* animThread2 = new DWORD[count];
    HANDLE* animThreadHandle = new HANDLE[count];
    HANDLE* animThreadHandle2 = new HANDLE[count];
    int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
    int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 22) * flScaleFactor;
    int largestXPos = dimensions.right / outerSizeX;
    int largestYPos = dimensions.bottom / outerSizeY;
    vector<bool> positions{};
    positions.resize(largestXPos * largestYPos - 1);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 3 of 5 complete: Created an array of positions.");
    for (int j = 0; j < validItems; j++) {
        if (!animation) pm[j]->SetAnimation(NULL); else pm[j]->SetAnimation(savedanim);
        if (pm[j]->GetInternalXPos() < largestXPos && pm[j]->GetInternalYPos() < largestYPos && positions[pm[j]->GetInternalYPos() + pm[j]->GetInternalXPos() * largestYPos] == false) {
            pm[j]->SetX(pm[j]->GetInternalXPos() * outerSizeX + x);
            pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
            pm[j]->SetPage(maxPageID);
            positions[pm[j]->GetInternalYPos() + pm[j]->GetInternalXPos() * largestYPos] = true;
        }
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 4 of 5 complete: Assigned positions to items that are in your resolution's bounds.");
    for (int j = 0; j < validItems; j++) {
        if (pm[j]->GetInternalXPos() >= largestXPos || pm[j]->GetInternalYPos() >= largestYPos) {
            int y{};
            while (positions[y] == true) {
                y++;
                if (y > positions.size()) {
                    y = 0;
                    for (int p = 0; p <= positions.size(); p++) {
                        positions[p] = false;
                    }
                    maxPageID++;
                    nextpageMain->SetVisible(true);
                    break;
                }
            }
            pm[j]->SetInternalXPos(y / largestYPos);
            pm[j]->SetInternalYPos(y % largestYPos);
            pm[j]->SetPage(maxPageID);
            positions[y] = true;
        }
        pm[j]->SetX(pm[j]->GetInternalXPos() * outerSizeX + x);
        pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
    }
    if (currentPageID > maxPageID) currentPageID = maxPageID;
    if (currentPageID != 1) prevpageMain->SetVisible(true);
    for (int j = 0; j < validItems; j++) {
        yValue* yV = new yValue{ j };
        yValue* yV2 = new yValue{ j };
        animThreadHandle[j] = CreateThread(0, 0, animate, (LPVOID)yV, 0, &(animThread[j]));
        animThreadHandle2[j] = CreateThread(0, 0, fastin, (LPVOID)yV2, 0, &(animThread2[j]));
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 5 of 5 complete: Successfully arranged the desktop items.");
    delete[] animThread;
    delete[] animThread2;
    delete[] animThreadHandle;
    delete[] animThreadHandle2;
    positions.clear();
}

void InitLayout() {
    UIContainer->DestroyAll(true);
    //frame.clear();
    pm.clear();
    iconpm.clear();
    shortpm.clear();
    shadowpm.clear();
    filepm.clear();
    fileshadowpm.clear();
    cbpm.clear();
    GetFontHeight();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 1 of 6 complete: Prepared DirectDesktop to receive desktop data.");
    LPWSTR path = GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop");
    wchar_t* secondaryPath = new wchar_t[260];
    wchar_t* cBuffer = new wchar_t[260];

    BYTE* value = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconLayouts");
    size_t offset = 0x10;
    vector<uint16_t> head;
    for (int i = 0; i < 4; ++i) {
        head.push_back(*reinterpret_cast<uint16_t*>(&value[offset + i * 2]));
    }
    head.push_back(*reinterpret_cast<uint32_t*>(&value[offset + 8]));
    uint32_t count = head[4];
    int count2{};
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 2 of 6 complete: Obtained desktop item count.");

    Button* emptyspace;
    parser->CreateElement(L"emptyspace", NULL, NULL, NULL, (Element**)&emptyspace);
    UIContainer->Add((Element**)&emptyspace, 1);
    assignFn(emptyspace, SelectItem);
    assignExtendedFn(emptyspace, ShowCheckboxIfNeeded);
    assignExtendedFn(emptyspace, MarqueeSelector);
    assignFn(emptyspace, DesktopRightClick);
    for (int i = 0; i < count; i++) {
        LVItem* outerElem;
        parser->CreateElement(L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
        UIContainer->Add((Element**)&outerElem, 1);
        iconElem = (Element*)outerElem->FindDescendent(StrToID(L"iconElem"));
        shortcutElem = (Element*)outerElem->FindDescendent(StrToID(L"shortcutElem"));
        iconElemShadow = (Element*)outerElem->FindDescendent(StrToID(L"iconElemShadow"));
        textElem = (RichText*)outerElem->FindDescendent(StrToID(L"textElem"));
        textElemShadow = (RichText*)outerElem->FindDescendent(StrToID(L"textElemShadow"));
        checkboxElem = (Button*)outerElem->FindDescendent(StrToID(L"checkboxElem"));
        pm.push_back(outerElem);
        iconpm.push_back(iconElem);
        shortpm.push_back(shortcutElem);
        shadowpm.push_back(iconElemShadow);
        filepm.push_back(textElem);
        fileshadowpm.push_back(textElemShadow);
        cbpm.push_back(checkboxElem);
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 3 of 6 complete: Created elements, preparing to enumerate desktop folders.");
    if (count2 < count) EnumerateFolder((LPWSTR)L"InternalCodeForNamespace", &pm, true, false, nullptr, &count2);
    DWORD d = GetEnvironmentVariableW(L"PUBLIC", cBuffer, 260);
    StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
    if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
    if (count2 < count) EnumerateFolder(secondaryPath, &pm, false, false, nullptr, &count2);
    if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
    if (count2 < count) EnumerateFolder(path, &pm, false, false, nullptr, &count2);
    if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
    d = GetEnvironmentVariableW(L"OneDrive", cBuffer, 260);
    StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
    if (count2 < count) EnumerateFolder(secondaryPath, &pm, false, false, nullptr, &count2);
    if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
    //CubicBezier(24, px, py, 0.1, 0.9, 0.2, 1.0);
    //frame.resize(count);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 4 of 6 complete: Created arrays according to your desktop items.");
    for (int i = 0; i < count; i++) {
        if (pm[i]->GetHiddenState() == true) {
            iconpm[i]->SetAlpha(128);
            shadowpm[i]->SetVisible(false);
            filepm[i]->SetAlpha(192);
            fileshadowpm[i]->SetAlpha(128);
        }
        assignFn(pm[i], SelectItem);
        assignFn(pm[i], ItemRightClick);
        assignExtendedFn(pm[i], ShowCheckboxIfNeeded);
        assignExtendedFn(cbpm[i], CheckboxHandler);
        if (shellstate[4] >= 48 && shellstate[4] <= 63) {
            pm[i]->SetClass(L"doubleclicked");
            //if (pm[i].isDirectory == true && treatdirasgroup == true) outerElem->SetClass(L"singleclicked");
        }
        else pm[i]->SetClass(L"singleclicked");
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 5 of 6 complete: Filled the arrays with relevant desktop icon data.");
    RearrangeIcons(false, true);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 6 of 6 complete: Arranged the icons according to your icon placements.");
    delete[] cBuffer;
    delete[] secondaryPath;
}

unsigned long FinishedLogging(LPVOID lpParam) {
    TaskDialog(NULL, NULL, L"Information", L"Logging complete", L"To close DirectDesktop, press the close button.\nThis will automatically view the log.", TDCBF_CLOSE_BUTTON, TD_INFORMATION_ICON, NULL);
    SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, 420);
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    hMutex = CreateMutex(NULL, TRUE, szWindowClass);
    if (!hMutex || ERROR_ALREADY_EXISTS == GetLastError()) {
        TaskDialog(NULL, GetModuleHandleW(NULL), L"Error", NULL,
            L"DirectDesktop is already running", TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
        return 1;
    }
    if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"FFlags") & 0x4);
    else {
        TaskDialog(NULL, GetModuleHandleW(NULL), L"DirectDesktop", L"Align your icons!",
            L"Your icons are not aligned to grid.\nThis configuration is not supported at the moment.", TDCBF_CLOSE_BUTTON, TD_WARNING_ICON, NULL);
        exit(0);
    }
    InitProcessPriv(14, NULL, true, true, true);
    InitThread(TSM_IMMERSIVE);
    RegisterAllControls();
    LVItem::Register();
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    RECT dimensions;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 2) * 2;
    int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CYEDGE, dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
    TaskDialog(NULL, GetModuleHandleW(NULL), L"DirectDesktop", NULL,
        L"Enable logging?", TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &logging);
    if (logging == IDYES) {
        wchar_t* docsfolder = new wchar_t[260];
        wchar_t* cBuffer = new wchar_t[260];
        DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
        StringCchPrintfW(docsfolder, 260, L"%s\\Documents", cBuffer);
        MainLogger.StartLogger(((wstring)docsfolder + L"\\DirectDesktop.log").c_str());
    }
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    int WindowsBuild = _wtoi(GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber"));
    if (hWndProgman) {
        hSHELLDLL_DefView = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
        if (logging == IDYES && hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: Found a SHELLDLL_DefView window.");
        if (WindowsBuild >= 26002 && logging == IDYES) MainLogger.WriteLine(L"Information: Version is 24H2, skipping WorkerW creation!!!");
        SendMessageTimeoutW(hWndProgman, 0x052C, 0, 0, SMTO_NORMAL, 250, NULL);
        this_thread::sleep_for(chrono::milliseconds(250));
        if (hSHELLDLL_DefView) {
            bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, false);
        }
    }
    if (logging == IDYES && !hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: SHELLDLL_DefView was not inside Program Manager, retrying...");
    bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, true);
    if (logging == IDYES && hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: Found a SHELLDLL_DefView window.");
    HWND hSysListView32 = FindWindowExW(hSHELLDLL_DefView, NULL, L"SysListView32", L"FolderView");
    if (hSysListView32) {
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Found SysListView32 window to hide.");
        ShowWindow(hSysListView32, SW_HIDE);
    }
    NativeHWNDHost::Create(L"DirectDesktop", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, NULL, NULL, 0, &wnd);
    NativeHWNDHost::Create(L"DirectDesktop Subview", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, NULL, NULL, 0, &subviewwnd);
    DUIXmlParser::Create(&parser, NULL, NULL, NULL, NULL);
    parser->SetXMLFromResource(IDR_UIFILE2, hInstance, hInstance);
    HWNDElement::Create(wnd->GetHWND(), true, NULL, NULL, &key, (Element**)&parent);
    HWNDElement::Create(subviewwnd->GetHWND(), true, NULL, NULL, &key2, (Element**)&subviewparent);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_STYLE, 0x56003A40L);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_EXSTYLE, 0xC0000800L);
    WndProc = (WNDPROC)SetWindowLongPtrW(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);
    if (WindowsBuild >= 26002) {
        SetWindowLongPtrW(hWorkerW, GWL_STYLE, 0x96000000L);
        SetWindowLongPtrW(hWorkerW, GWL_EXSTYLE, 0x20000880L);
    }
    HWND dummyHWnd;
    dummyHWnd = SetParent(wnd->GetHWND(), hSHELLDLL_DefView);
    HBRUSH hbr = CreateSolidBrush(RGB(0, 0, 0));
    SetClassLongPtrW(wnd->GetHWND(), GCLP_HBRBACKGROUND, (LONG_PTR)hbr);
    if (logging == IDYES) {
        if (dummyHWnd != nullptr) MainLogger.WriteLine(L"Information: DirectDesktop is now a part of Explorer.");
        else MainLogger.WriteLine(L"Error: DirectDesktop is still hosted in its own window.");
    }

    parser->CreateElement(L"main", parent, NULL, NULL, &pMain);
    pMain->SetVisible(true);
    pMain->EndDefer(key);
    parser->CreateElement(L"fullscreenpopup", subviewparent, NULL, NULL, &pSubview);
    pSubview->SetVisible(true);
    pSubview->EndDefer(key2);

    InitialUpdateScale();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated scaling.");
    UpdateModeInfo();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated color mode information.");

    sampleText = regElem(L"sampleText", pMain);
    mainContainer = regElem(L"mainContainer", pMain);
    UIContainer = regElem(L"UIContainer", pMain);
    fullscreenpopupbase = regBtn(L"fullscreenpopupbase", pSubview);
    centered = regBtn(L"centered", pSubview);
    selector = regElem(L"selector", pMain);
    selector2 = regElem(L"selector2", pMain);
    SimpleViewTop = regBtn(L"SimpleViewTop", pSubview);
    SimpleViewBottom = regBtn(L"SimpleViewBottom", pSubview);
    SimpleViewSettings = regTouchBtn(L"SimpleViewSettings", pSubview);
    SimpleViewClose = regTouchBtn(L"SimpleViewClose", pSubview);
    prevpage = regTouchBtn(L"prevpage", pSubview);
    nextpage = regTouchBtn(L"nextpage", pSubview);
    prevpageMain = regTouchBtn(L"prevpageMain", pMain);
    nextpageMain = regTouchBtn(L"nextpageMain", pMain);
    pageinfo = regRichText(L"pageinfo", pSubview);

    assignFn(fullscreenpopupbase, testEventListener3);
    assignFn(SimpleViewTop, testEventListener3);
    assignFn(SimpleViewBottom, testEventListener3);
    assignFn(SimpleViewSettings, ShowSettings);
    assignFn(SimpleViewClose, ExitWindow);
    assignFn(prevpage, GoToPrevPage);
    assignFn(nextpage, GoToNextPage);
    assignFn(prevpageMain, GoToPrevPage);
    assignFn(nextpageMain, GoToNextPage);

    UIContainer->SetWidth(dimensions.right - dimensions.left);
    UIContainer->SetHeight(dimensions.bottom - dimensions.top);
    showcheckboxes = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect");
    hiddenIcons = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons");
    globaliconsz = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize");
    shellstate = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState");
    if (globaliconsz > 96) globalshiconsz = 64; else if (globaliconsz > 48) globalshiconsz = 48; else globalshiconsz = 32;
    globalgpiconsz = 12;
    if (globaliconsz > 96) globalgpiconsz = 48;
    else if (globaliconsz > 48) globalgpiconsz = 32;
    else if (globaliconsz > 32) globalgpiconsz = 16;
    InitLayout();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialized layout successfully.");
    SetTheme();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Set the theme successfully.");

    wnd->Host(pMain);
    subviewwnd->Host(pSubview);
    wnd->ShowWindow(SW_SHOW);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been created and shown.");
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(wnd->GetHWND(), &m);
    BlurBackground(hWndProgman, true);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been made transparent.\n\nLogging is now complete.");
    if (logging == IDYES) {
        DWORD dd;
        HANDLE loggingThread = CreateThread(0, 0, FinishedLogging, NULL, 0, &dd);
    }
    //if (logging == IDYES) TaskDialog(wnd->GetHWND(), GetModuleHandleW(NULL), L"Information", NULL,
        //L"DirectDesktop is now transparent.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
    StartMessagePump();
    UnInitProcessPriv(0);
    CoUninitialize();

    return 0;
}