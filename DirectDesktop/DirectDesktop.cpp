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

NativeHWNDHost* wnd;
HWNDElement* parent;
DUIXmlParser* parser;
Element* pMain;
unsigned long key = 0;

Element* sampleText;
Element* mainContainer;
Element* iconElem;
Element* shortcutElem;
Element* iconElemShadow;
RichText* textElem;
RichText* textElemShadow;
Button* checkboxElem;
Element* fullscreenpopup, *fullscreeninner;
Button* fullscreenpopupbase, *centered;
Button* emptyspace;
Element* selector, *selector2;
Element* dirnameanimator;
Element* tasksanimator;
Button* SimpleViewTop, *SimpleViewBottom;
TouchButton* SimpleViewSettings, *SimpleViewClose;
TouchButton* PageTab1, *PageTab2;
RichText* SubUIContainer;
HRESULT err;
HWND hWorkerW = NULL;

int popupframe, dframe, tframe;
vector<int> frame;

int dpi, dpiOld = 1;
float flScaleFactor;
bool isDpiPreviouslyChanged;
void InitialUpdateScale() {
    HDC screen = GetDC(0);
    dpi = static_cast<int>(GetDeviceCaps(screen, LOGPIXELSX));
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

Element* regElem(const wchar_t* elemName) {
    Element* result = (Element*)pMain->FindDescendent(StrToID(elemName));
    return result;
}
RichText* regRichText(const wchar_t* elemName) {
    RichText* result = (RichText*)pMain->FindDescendent(StrToID(elemName));
    return result;
}
Button* regBtn(const wchar_t* btnName) {
    Button* result = (Button*)pMain->FindDescendent(StrToID(btnName));
    return result;
}
TouchButton* regTouchBtn(const wchar_t* btnName) {
    TouchButton* result = (TouchButton*)pMain->FindDescendent(StrToID(btnName));
    return result;
}
Edit* regEdit(const wchar_t* editName) {
    Edit* result = (Edit*)pMain->FindDescendent(StrToID(editName));
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

const wchar_t* CharToWChar(const char* input)
{
    size_t charCount = strlen(input) + 1;
    wchar_t* result = new wchar_t[charCount];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, result, charCount, input, _TRUNCATE);
    return result;
}

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
    sheetStorage->Release();
}

WNDPROC WndProc;
HANDLE hMutex;
constexpr LPCWSTR szWindowClass = L"DIRECTDESKTOP";
vector<parameters> pm, subpm;
vector<Element*> shortpm, subshortpm;
vector<Element*> iconpm, subiconpm;
vector<Element*> shadowpm, subshadowpm;
vector<RichText*> filepm, subfilepm;
vector<RichText*> fileshadowpm;
vector<Element*> cbpm;
bool checkifelemexists = 0;
bool issubviewopen = 0;
bool hiddenIcons;
void fullscreenAnimation(int width, int height);

HBITMAP GetShellItemImage(LPCWSTR filePath, int width, int height) {

    IShellItem* pShellItem{};
    HRESULT hr = SHCreateItemFromParsingName(filePath, NULL, IID_PPV_ARGS(&pShellItem));

    IShellItemImageFactory* pImageFactory{};
    hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
    pShellItem->Release();

    SIZE size = { width * flScaleFactor, height * flScaleFactor };
    HBITMAP hBitmap{};
    hr = pImageFactory->GetImage(size, SIIGBF_RESIZETOFIT, &hBitmap);
    pImageFactory->Release();

    if (SUCCEEDED(hr)) {
        return hBitmap;
    }

}

void GetFontHeight() {
    LOGFONTW lf{};
    RECT rc = { 0, 0, 100, 100 };
    HDC hdcBuffer = CreateCompatibleDC(NULL);
    SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, dpi);
    DrawTextW(hdcBuffer, L" ", -1, &rc, DT_CENTER);
    GetTextMetricsW(hdcBuffer, &tm);
    DeleteDC(hdcBuffer);
}
float CalcTextLines(const wchar_t* str, int width) {
    HDC hdcBuffer = CreateCompatibleDC(NULL);
    LOGFONTW lf{};
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL);
    HFONT hFont = CreateFontIndirectW(&lf);
    HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
    RECT rc = { 0, 0, width - 4, tm.tmHeight };
    wchar_t filenameBuffer[260]{};
    wcscpy_s(filenameBuffer, str);
    DrawTextExW(hdcBuffer, filenameBuffer, -1, &rc, DT_MODIFYSTRING | DT_END_ELLIPSIS | DT_CENTER | DT_LVICON, NULL);
    int lines_b1 = wcscmp(str, filenameBuffer);
    RECT rc2 = { 0, 0, width - 4, tm.tmHeight * 2 };
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
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    if (!hiddenIcons) {
        Value* bitmap;
        HBITMAP hbmCapture{};
        HDC hdcWindow = GetDC(wnd->GetHWND());
        HDC hdcMem = CreateCompatibleDC(hdcWindow);
        hbmCapture = CreateCompatibleBitmap(hdcWindow, dimensions.right, dimensions.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
        BitBlt(hdcMem, 0, 0, dimensions.right, dimensions.bottom, hdcWindow, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
        ReleaseDC(wnd->GetHWND(), hdcWindow);
        bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 7, 0xffffffff, false, false, false);
        fullscreenAnimation(dimensions.right * 0.7, dimensions.bottom * 0.7);
        centered->SetBackgroundStdColor(7);
        fullscreeninner->SetValue(Element::BackgroundProp, 1, bitmap);
        bitmap->Release();
        DeleteObject(hbmCapture);
    }
    else {
        fullscreenAnimation(dimensions.right * 0.7, dimensions.bottom * 0.7);
        fullscreeninner->SetBackgroundStdColor(7);
    }
    SimpleViewTop->SetLayoutPos(1);
    SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
    SimpleViewBottom->SetLayoutPos(3);
    fullscreenpopup->SetBackgroundStdColor(7);
    BlurBackground(GetWorkerW(), false);
}

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
    int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor - tm.tmHeight;
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
            RearrangeIcons(true, false);
        }
        break;
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
        int logging = IDNO;
        ToggleDesktopIcons(!hiddenIcons, false, &logging);
        break;
    }
    case WM_COMMAND: {
        break;
    }
    case WM_USER + 1: {
        //pm[wParam].elem->SetAlpha(255);
        pm[wParam].elem->SetVisible(!hiddenIcons);
        break;
    }
    case WM_USER + 2: {
        //subpm[wParam].elem->SetAlpha(255);
        subpm[wParam].elem->SetVisible(true);
        break;
    }
    case WM_USER + 3: {
        double bezierProgress = py[frame[wParam] - 1];
        int lines_basedOnEllipsis{};
        lines_basedOnEllipsis = floor(CalcTextLines(pm[wParam].simplefilename.c_str(), innerSizeX)) * tm.tmHeight;
        pm[wParam].elem->SetWidth(innerSizeX * (0.7 + 0.3 * bezierProgress));
        pm[wParam].elem->SetHeight((innerSizeY + lines_basedOnEllipsis + 7 * flScaleFactor) * (0.7 + 0.3 * bezierProgress));
        pm[wParam].elem->SetX(round((dimensions.right - 2 * pm[wParam].x) * 0.15 * (1 - bezierProgress) + pm[wParam].x));
        pm[wParam].elem->SetY(round((dimensions.bottom - 2 * pm[wParam].y) * 0.15 * (1 - bezierProgress) + pm[wParam].y));
        filepm[wParam]->SetHeight(lines_basedOnEllipsis + 4 * flScaleFactor);
        fileshadowpm[wParam]->SetHeight(lines_basedOnEllipsis + 5 * flScaleFactor);
        iconpm[wParam]->SetWidth(round((globaliconsz * (0.7 + 0.3 * bezierProgress))) * flScaleFactor);
        iconpm[wParam]->SetHeight(round((globaliconsz * (0.7 + 0.3 * bezierProgress))) * flScaleFactor);
        iconpm[wParam]->SetX(round(iconPaddingX * (0.7 + 0.3 * bezierProgress)));
        iconpm[wParam]->SetY(round((iconPaddingY * 0.575) * (0.7 + 0.3 * bezierProgress)));
        HBITMAP capturedBitmap;
        capturedBitmap = CreateTextBitmap(pm[wParam].simplefilename.c_str(), innerSizeX - 4 * flScaleFactor, lines_basedOnEllipsis, DT_END_ELLIPSIS);
        HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor);
        IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
        IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), 1.33);
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
        fullscreenpopup->SetLayoutPos(-3);
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
        double bezierProgress = 1; //py[frame[wParam] - 1];
        subpm[wParam].elem->SetWidth(innerSizeX * (0.7 + 0.3 * bezierProgress));
        subpm[wParam].elem->SetHeight((innerSizeY + tm.tmHeight + 23 * flScaleFactor) * (0.7 + 0.3 * bezierProgress));
        subpm[wParam].elem->SetX(subpm[wParam].x); // round((720 - 2 * subpm[wParam].x) * 0.15 * (1 - bezierProgress) + subpm[wParam].x));
        subpm[wParam].elem->SetY(subpm[wParam].y); // round((360 - 2 * subpm[wParam].y) * 0.15 * (1 - bezierProgress) + subpm[wParam].y));
        int textlines = 1;
        if (tm.tmHeight <= 18 * flScaleFactor) textlines = 2;
        subfilepm[wParam]->SetHeight(tm.tmHeight * textlines + 4 * flScaleFactor);
        if (subfilepm[wParam]->GetHeight() > (iconPaddingY * 0.575 + 48)) subfilepm[wParam]->SetHeight(iconPaddingY * 0.575 + 48);
        subiconpm[wParam]->SetWidth(round((globaliconsz * (0.7 + 0.3 * bezierProgress))) * flScaleFactor);
        subiconpm[wParam]->SetHeight(round((globaliconsz * (0.7 + 0.3 * bezierProgress))) * flScaleFactor);
        subiconpm[wParam]->SetX(round(iconPaddingX * (0.7 + 0.3 * bezierProgress)));
        subiconpm[wParam]->SetY(round((iconPaddingY * 0.575) * (0.7 + 0.3 * bezierProgress)));
        HBITMAP capturedBitmap = CreateTextBitmap(subpm[wParam].simplefilename.c_str(), innerSizeX, tm.tmHeight * textlines, DT_END_ELLIPSIS);
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
        if (globaliconsz > 48) {
            padding = 12;
            paddingInner = 8;
        }
        else if (globaliconsz > 32) {
            padding = 6;
            paddingInner = 4;
        }
        for (int icon = 0; icon < pm.size(); icon++) {
            if (pm[icon].isDirectory == true && treatdirasgroup == true) {
                int x = padding * flScaleFactor, y = padding * flScaleFactor;
                iconpm[icon]->DestroyAll(true);
                iconpm[icon]->SetClass(L"groupthumbnail");
                vector<parameters> dummypm;
                vector<wstring> dummy, filenames;
                EnumerateFolder((LPWSTR)pm[icon].filename.c_str(), &dummypm, &dummy, &filenames, 1, 4);
                for (int thumbs = 0; thumbs < filenames.size(); thumbs++) {
                    HBITMAP thumbIcon = GetShellItemImage(filenames[thumbs].c_str(), globalgpiconsz * flScaleFactor, globalgpiconsz * flScaleFactor);
                    if (isColorized) {
                        IterateBitmap(thumbIcon, StandardBitmapPixelHandler, 1, 0, 1);
                    }
                    Value* vThumbIcon = DirectUI::Value::CreateGraphic(thumbIcon, 2, 0xffffffff, false, false, false);
                    Element* GroupedIcon;
                    parser->CreateElement(L"GroupedIcon", NULL, NULL, NULL, (Element**)&GroupedIcon);
                    iconpm[icon]->Add((Element**)&GroupedIcon, 1);
                    GroupedIcon->SetWidth(globalgpiconsz * flScaleFactor), GroupedIcon->SetHeight(globalgpiconsz* flScaleFactor);
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
        frame[yV->y] = 24;
        SendMessageW(wnd->GetHWND(), WM_USER + 3, yV->y, NULL);
        //this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 400)));
    //}
    free(yV);
    return 0;
}

unsigned long subfastin(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
    SendMessageW(wnd->GetHWND(), WM_USER + 10, yV->y, NULL);
    frame[yV->y] = 24;
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
    HDC hdcWindow = GetDC(wnd->GetHWND());
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hbmCapture = CreateCompatibleBitmap(hdcWindow, dimensions.right * 0.08, dimensions.bottom * 0.08);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
    SetStretchBltMode(hdcMem, HALFTONE);
    StretchBlt(hdcMem, 0, 0, dimensions.right * 0.08, dimensions.bottom * 0.08, hdcWindow, 0, 0, dimensions.right, dimensions.bottom, SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    ReleaseDC(wnd->GetHWND(), hdcWindow);
    IterateBitmap(hbmCapture, StandardBitmapPixelHandler, 2, 2 * flScaleFactor, 1);
    Value* bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 4, 0xffffffff, false, false, false);
    fullscreenpopup->SetValue(Element::BackgroundProp, 1, bitmap);
    bitmap->Release();
    //this_thread::sleep_for(chrono::milliseconds(80));
    fullscreenpopup->SetLayoutPos(4);
    fullscreenpopup->SetAlpha(255);
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
    fullscreenpopup->SetLayoutPos(4);
    fullscreenpopup->SetAlpha(255);
    fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor);
}
void HidePopupCore() {
    fullscreenpopup->SetAlpha(0);
    fullscreenAnimation2();
    frame.clear();
    subpm.clear();
    subiconpm.clear();
    subshortpm.clear();
    subshadowpm.clear();
    subfilepm.clear();
    SimpleViewTop->SetLayoutPos(-3);
    SimpleViewBottom->SetLayoutPos(-3);
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

vector<HBITMAP> GetDesktopIcons() {
    vector<HBITMAP> bmResult;
    for (int i = 0; i < pm.size(); i++) {
        if (pm[i].isDirectory == true && treatdirasgroup == true) bmResult.push_back(NULL);
        else bmResult.push_back(GetShellItemImage((pm[i].filename).c_str(), globaliconsz, globaliconsz));
    }
    return bmResult;
}
vector<HBITMAP> GetSubdirectoryIcons(int limit = subpm.size(), int width = globaliconsz, int height = globaliconsz) {
    vector<HBITMAP> bmResult;
    for (int i = 0; i < limit; i++) {
        bmResult.push_back(GetShellItemImage((subpm[i].filename).c_str(), width, height));
    }
    return bmResult;
}

unsigned long ApplyThumbnailIcons(LPVOID lpParam) {
    SendMessageW(wnd->GetHWND(), WM_USER + 14, NULL, NULL);
    return 0;
}

void ApplyIcons(vector<parameters> pmLVItem, vector<Element*> pmIcon, vector<Element*> pmIconShadow, vector<Element*> pmShortcut, vector<HBITMAP> iconstofetch) {
    HINSTANCE testInst = LoadLibraryW(L"imageres.dll");
    vector<HBITMAP> icons = iconstofetch;
    for (int icon = 0; icon < pmIcon.size(); icon++) {
        HICON icoShortcut = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(163), IMAGE_ICON, globalshiconsz * flScaleFactor, globalshiconsz * flScaleFactor, LR_SHARED);
        // The use of the 3 lines below is because we can't use a fully transparent bitmap
        HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"shell32.dll"), MAKEINTRESOURCE(24), IMAGE_ICON, 16, 16, LR_SHARED);
        HBITMAP dummyii = IconToBitmap(dummyi);
        IterateBitmap(dummyii, SimpleBitmapPixelHandler, 0, 0, 0.005);
        HBITMAP bmp{};
        if (pm[icon].isDirectory == false || treatdirasgroup == false || pmIcon != iconpm) bmp = icons[icon];
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
        if (pmLVItem[icon].isShortcut == true) pmShortcut[icon]->SetValue(Element::ContentProp, 1, bitmapShortcut);
        DeleteObject(icoShortcut);
        DeleteObject(bmp);
        DeleteObject(bmpShortcut);
        bitmap->Release();
        bitmapShortcut->Release();
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
            if (subpm[items].elem == elem) {
                execInfo.lpFile = (subpm[items].filename).c_str();
                ShellExecuteExW(&execInfo);
            }
        }
    }
}

void ShowDirAsGroup(LPCWSTR filename, wstring simplefilename) {
    fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor);
    Element* groupdirectory{};
    parser->CreateElement(L"groupdirectory", NULL, NULL, NULL, (Element**)&groupdirectory);
    fullscreeninner->Add((Element**)&groupdirectory, 1);
    ScrollViewer* groupdirlist = (ScrollViewer*)groupdirectory->FindDescendent(StrToID(L"groupdirlist"));
    SubUIContainer = (RichText*)groupdirlist->FindDescendent(StrToID(L"SubUIContainer"));
    vector<wstring> subfiles, ssss;
    EnumerateFolder((LPWSTR)filename, &subpm, &subfiles, &ssss, true);
    unsigned int count = subfiles.size();
    CubicBezier(48, px, py, 0.1, 0.9, 0.2, 1.0);
    if (count <= 128 && count > 0) {
        int x = 0, y = 0;
        frame.resize(count);
        subpm.resize(count);
        subiconpm.resize(count);
        subshortpm.resize(count);
        subshadowpm.resize(count);
        subfilepm.resize(count);
        Value* v;
        RECT dimensions;
        dimensions = *(groupdirectory->GetPadding(&v));
        int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
        int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 21) * flScaleFactor;
        DWORD* animThread = new DWORD[count];
        DWORD* animThread2 = new DWORD[count];
        HANDLE* animThreadHandle = new HANDLE[count];
        HANDLE* animThreadHandle2 = new HANDLE[count];
        for (int i = 0; i < count; i++) {
            Button* outerElemGrouped;
            parser->CreateElement(L"outerElemGrouped", NULL, NULL, NULL, (Element**)&outerElemGrouped);
            SubUIContainer->Add((Element**)&outerElemGrouped, 1);
            iconElem = (Element*)outerElemGrouped->FindDescendent(StrToID(L"iconElem"));
            shortcutElem = (Element*)outerElemGrouped->FindDescendent(StrToID(L"shortcutElem"));
            iconElemShadow = (Element*)outerElemGrouped->FindDescendent(StrToID(L"iconElemShadow"));
            textElem = (RichText*)outerElemGrouped->FindDescendent(StrToID(L"textElem"));
            subpm[i].elem = outerElemGrouped, subpm[i].filename = ssss[i], subpm[i].simplefilename = subfiles[i];
            subiconpm[i] = iconElem;
            subshortpm[i] = shortcutElem;
            subshadowpm[i] = iconElemShadow;
            subfilepm[i] = textElem;
            if (subpm[i].isHidden == true) {
                iconElem->SetAlpha(128);
                iconElemShadow->SetVisible(false);
                textElem->SetAlpha(128);
            }
            assignFn(outerElemGrouped, SelectSubItem);
            outerElemGrouped->SetClass(L"singleclicked");
        }
        ApplyIcons(subpm, subiconpm, subshadowpm, subshortpm, GetSubdirectoryIcons());
        for (int j = 0; j < count; j++) {
            subpm[j].x = x, subpm[j].y = y;
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
        SubUIContainer->SetHeight(y);
        subfiles.clear();
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
    dirname->SetContentString(simplefilename.c_str());
    dirname->SetAlpha(255);
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
        chooseColor = (Edit*)SettingsPage2->FindDescendent(StrToID(L"chooseColor"));
        Button* ApplyColor = (Button*)SettingsPage2->FindDescendent(StrToID(L"ApplyColor"));
        EnableAccent->SetSelected(isColorized);
        assignFn(EnableAccent, ToggleAccentIcons);
        assignFn(ApplyColor, ApplySelectedColor);
    }
}
void ShowSettings(Element* elem, Event* iev) {
    if (iev->uidType == TouchButton::Click) {
        fullscreenpopup->SetLayoutPos(-3);
        centered->DestroyAll(true);
        ShowPopupCore();
        SimpleViewTop->SetLayoutPos(-3);
        SimpleViewBottom->SetLayoutPos(-3);
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
            for (int items = 0; items < pm.size(); items++) {
                pm[items].elem->SetSelected(false);
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
        for (int items = 0; items < pm.size(); items++) {
            if (pm[items].mem_isSelected != pm[items].elem->GetSelected()) {
                float spacingInternal = CalcTextLines(pm[items].simplefilename.c_str(), pm[items].elem->GetWidth());
                int extraBottomSpacing = (pm[items].elem->GetSelected() == true) ? ceil(spacingInternal) * tm.tmHeight : floor(spacingInternal) *tm.tmHeight;
                textElem = (RichText*)pm[items].elem->FindDescendent(StrToID(L"textElem"));
                textElemShadow = (RichText*)pm[items].elem->FindDescendent(StrToID(L"textElemShadow"));
                if (spacingInternal == 1.5) {
                    if (pm[items].elem->GetSelected() == true) pm[items].elem->SetHeight(pm[items].elem->GetHeight() + extraBottomSpacing * 0.5);
                    else pm[items].elem->SetHeight(pm[items].elem->GetHeight() - extraBottomSpacing);
                }
                textElem->SetHeight(extraBottomSpacing + 4 * flScaleFactor);
                textElemShadow->SetHeight(extraBottomSpacing + 5 * flScaleFactor);
                HBITMAP capturedBitmap = CreateTextBitmap(pm[items].simplefilename.c_str(), pm[items].elem->GetWidth() - 4 * flScaleFactor, extraBottomSpacing, DT_END_ELLIPSIS);
                HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor);
                IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
                IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), 1.33);
                Value* bitmap = DirectUI::Value::CreateGraphic(capturedBitmap, 2, 0xffffffff, false, false, false);
                Value* bitmapSh = DirectUI::Value::CreateGraphic(shadowBitmap, 2, 0xffffffff, false, false, false);
                textElem->SetValue(Element::ContentProp, 1, bitmap);
                textElemShadow->SetValue(Element::ContentProp, 1, bitmapSh);
                bitmap->Release();
                bitmapSh->Release();
                DeleteObject(capturedBitmap);
                DeleteObject(shadowBitmap);
            }
            pm[items].mem_isSelected = pm[items].elem->GetSelected();
        }
        if (clicks % 2 == 1 && checkbox->GetMouseFocused() == false) {
            for (int items = 0; items < pm.size(); items++) {
                if (pm[items].elem == elem) {
                    execInfo.lpFile = (pm[items].filename).c_str();
                    if (pm[items].isDirectory == true && treatdirasgroup == true) {
                        ShowDirAsGroup(execInfo.lpFile, pm[items].simplefilename);
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
        for (int items = 0; items < cbpm.size(); items++) {
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
    HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"SystemSettingsAdminFlows.exe"), MAKEINTRESOURCE(10), IMAGE_ICON, 16, 16, LR_SHARED);
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
        switch (fullscreenpopup->GetAlpha()) {
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

void RearrangeIcons(bool animation, bool reloadgroups) {
    unsigned int count = pm.size();
    static const int savedanim = pm[0].elem->GetAnimation();
    ApplyIcons(pm, iconpm, shadowpm, shortpm, GetDesktopIcons());
    if (reloadgroups) {
        DWORD dd;
        HANDLE thumbnailThread = CreateThread(0, 0, ApplyThumbnailIcons, NULL, 0, &dd);
    }
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    int x = 4 * flScaleFactor + dimensions.left, y = 4 * flScaleFactor + dimensions.top;
    DWORD* animThread = new DWORD[count];
    DWORD* animThread2 = new DWORD[count];
    HANDLE* animThreadHandle = new HANDLE[count];
    HANDLE* animThreadHandle2 = new HANDLE[count];
    int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
    int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 21) * flScaleFactor;
    for (int j = 0; j < count; j++) {
        if (!animation) pm[j].elem->SetAnimation(NULL); else pm[j].elem->SetAnimation(savedanim);
        pm[j].x = x, pm[j].y = y;
        yValue* yV = new yValue{ j };
        yValue* yV2 = new yValue{ j };
        y += outerSizeY;
        if (y > dimensions.bottom - outerSizeY) {
            y = 4 * flScaleFactor;
            x += outerSizeX;
        }
        animThreadHandle[j] = CreateThread(0, 0, animate, (LPVOID)yV, 0, &(animThread[j]));
        animThreadHandle2[j] = CreateThread(0, 0, fastin, (LPVOID)yV2, 0, &(animThread2[j]));
    }
    delete[] animThread;
    delete[] animThread2;
    delete[] animThreadHandle;
    delete[] animThreadHandle2;
}

void InitLayout() {
    UIContainer->DestroyAll(true);
    frame.clear();
    pm.clear();
    iconpm.clear();
    shortpm.clear();
    shadowpm.clear();
    filepm.clear();
    cbpm.clear();
    GetFontHeight();
    vector<wstring> files, filePaths;
    LPWSTR path = GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop");
    wchar_t* secondaryPath = new wchar_t[260];
    wchar_t* cBuffer = new wchar_t[260];
    EnumerateFolder(path, &pm, &files, &filePaths, true);
    DWORD d = GetEnvironmentVariableW(L"PUBLIC", cBuffer, 260);
    StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
    EnumerateFolder(secondaryPath, &pm, &files, &filePaths, false);
    d = GetEnvironmentVariableW(L"OneDrive", cBuffer, 260);
    StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
    EnumerateFolder(secondaryPath, &pm, &files, &filePaths, false);
    unsigned int count{};
    count = files.size();
    Button* emptyspace;
    parser->CreateElement(L"emptyspace", NULL, NULL, NULL, (Element**)&emptyspace);
    UIContainer->Add((Element**)&emptyspace, 1);
    assignFn(emptyspace, SelectItem);
    assignExtendedFn(emptyspace, ShowCheckboxIfNeeded);
    assignExtendedFn(emptyspace, MarqueeSelector);
    assignFn(emptyspace, DesktopRightClick);
    CubicBezier(24, px, py, 0.1, 0.9, 0.2, 1.0);
    frame.resize(count);
    pm.resize(count);
    iconpm.resize(count);
    shortpm.resize(count);
    shadowpm.resize(count);
    filepm.resize(count);
    fileshadowpm.resize(count);
    cbpm.resize(count);
    for (int i = 0; i < count; i++) {
        Button* outerElem;
        parser->CreateElement(L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
        UIContainer->Add((Element**)&outerElem, 1);
        iconElem = (Element*)outerElem->FindDescendent(StrToID(L"iconElem"));
        shortcutElem = (Element*)outerElem->FindDescendent(StrToID(L"shortcutElem"));
        iconElemShadow = (Element*)outerElem->FindDescendent(StrToID(L"iconElemShadow"));
        textElem = (RichText*)outerElem->FindDescendent(StrToID(L"textElem"));
        textElemShadow = (RichText*)outerElem->FindDescendent(StrToID(L"textElemShadow"));
        checkboxElem = (Button*)outerElem->FindDescendent(StrToID(L"checkboxElem"));
        pm[i].elem = outerElem, pm[i].filename = filePaths[i], pm[i].simplefilename = files[i];
        iconpm[i] = iconElem;
        shortpm[i] = shortcutElem;
        shadowpm[i] = iconElemShadow;
        filepm[i] = textElem;
        fileshadowpm[i] = textElemShadow;
        cbpm[i] = checkboxElem;
        if (pm[i].isHidden == true) {
            iconElem->SetAlpha(128);
            iconElemShadow->SetVisible(false);
            textElem->SetAlpha(192);
            textElemShadow->SetAlpha(128);
        }
        assignFn(outerElem, SelectItem);
        assignFn(outerElem, ItemRightClick);
        assignExtendedFn(outerElem, ShowCheckboxIfNeeded);
        assignExtendedFn(checkboxElem, CheckboxHandler);
        if (shellstate[4] >= 48 && shellstate[4] <= 63) {
            outerElem->SetClass(L"doubleclicked");
            //if (pm[i].isDirectory == true && treatdirasgroup == true) outerElem->SetClass(L"singleclicked");
        }
        else outerElem->SetClass(L"singleclicked");
    }
    RearrangeIcons(false, true);
    files.clear();
    delete[] cBuffer;
    delete[] secondaryPath;
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
    InitProcessPriv(14, NULL, true, true, true);
    InitThread(TSM_IMMERSIVE);
    RegisterAllControls();
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    RECT dimensions;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 2) * 2;
    int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CYEDGE, dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
    int logging;
    TaskDialog(NULL, GetModuleHandleW(NULL), L"DirectDesktop", NULL,
        L"Enable logging?", TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &logging);
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    HWND hSHELLDLL_DefView = NULL;
    int WindowsBuild = _wtoi(GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber"));
    if (hWndProgman) {
        hSHELLDLL_DefView = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
        if (WindowsBuild > 26016 && logging == IDYES) TaskDialog(NULL, GetModuleHandleW(NULL), L"Information", NULL,
            L"Version is 24H2, skipping WorkerW creation!!!", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        SendMessageTimeoutW(hWndProgman, 0x052C, 0, 0, SMTO_NORMAL, 250, NULL);
        this_thread::sleep_for(chrono::milliseconds(250));
        if (hSHELLDLL_DefView) {
            bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, false, &logging);
            if (logging == IDYES) {
                if (pos) TaskDialog(NULL, GetModuleHandleW(NULL), L"Information", NULL, L"Successfully manipulated windows.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
                else TaskDialog(NULL, GetModuleHandleW(NULL), L"Error", NULL, L"Failed to manipulate windows.", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
            }
        }
    }
    if (!hSHELLDLL_DefView) {
        if (logging == IDYES) TaskDialog(NULL, GetModuleHandleW(NULL), L"Information", NULL,
            L"SHELLDLL_DefView was not inside Program Manager, retrying...", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, true, &logging);
        if (logging == IDYES) {
            if (pos) TaskDialog(NULL, GetModuleHandleW(NULL), L"Information", NULL, L"Successfully manipulated windows.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
            else TaskDialog(NULL, GetModuleHandleW(NULL), L"Error", NULL, L"Failed to manipulate windows.", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        }
    }
    NativeHWNDHost::Create(L"DirectDesktop", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, NULL, NULL, 0, &wnd);
    DUIXmlParser::Create(&parser, NULL, NULL, NULL, NULL);
    parser->SetXMLFromResource(IDR_UIFILE2, hInstance, hInstance);
    HWNDElement::Create(wnd->GetHWND(), true, NULL, NULL, &key, (Element**)&parent);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_STYLE, 0x56003A40L);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_EXSTYLE, 0xC0000800L);
    WndProc = (WNDPROC)SetWindowLongPtr(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);
    if (WindowsBuild > 26016) {
        // This does not work properly...
        SetWindowLongPtrW(hWorkerW, GWL_STYLE, 0x96000000L);
        SetWindowLongPtrW(hWorkerW, GWL_EXSTYLE, 0x20000880L);
        if (logging == IDYES) TaskDialog(NULL, GetModuleHandleW(NULL), L"Information", NULL,
            L"Applied styles to the new 24H2 WorkerW.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        SetWindowLongPtrW(hSHELLDLL_DefView, GWL_EXSTYLE, 0xC0080000L);
        if (logging == IDYES) TaskDialog(NULL, GetModuleHandleW(NULL), L"Information", NULL,
            L"Applied styles to the new 24H2 SHELLDLL_DefView.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
    }
    HWND dummyHWnd;
    dummyHWnd = SetParent(wnd->GetHWND(), hSHELLDLL_DefView);
    HBRUSH hbr = CreateSolidBrush(RGB(0, 0, 0));
    SetClassLongPtrW(hWorkerW, GCLP_HBRBACKGROUND, (LONG_PTR)hbr);
    SetClassLongPtrW(hSHELLDLL_DefView, GCLP_HBRBACKGROUND, (LONG_PTR)hbr);
    SetClassLongPtrW(wnd->GetHWND(), GCLP_HBRBACKGROUND, (LONG_PTR)hbr);
    if (logging == IDYES) {
        if (dummyHWnd) TaskDialog(wnd->GetHWND(), GetModuleHandleW(NULL), L"Information", NULL,
            L"DirectDesktop is now a part of Explorer.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        else TaskDialog(wnd->GetHWND(), GetModuleHandleW(NULL), L"Error", NULL,
            L"DirectDesktop is still hosted in its own window.", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
    }

    parser->CreateElement(L"main", parent, NULL, NULL, &pMain);
    pMain->SetVisible(true);
    pMain->EndDefer(key);

    InitialUpdateScale();
    UpdateModeInfo();

    sampleText = regElem(L"sampleText");
    mainContainer = regElem(L"mainContainer");
    UIContainer = regElem(L"UIContainer");
    fullscreenpopup = regElem(L"fullscreenpopup");
    fullscreenpopupbase = regBtn(L"fullscreenpopupbase");
    centered = regBtn(L"centered");
    selector = regElem(L"selector");
    selector2 = regElem(L"selector2");
    SimpleViewTop = regBtn(L"SimpleViewTop");
    SimpleViewBottom = regBtn(L"SimpleViewBottom");
    SimpleViewSettings = regTouchBtn(L"SimpleViewSettings");
    SimpleViewClose = regTouchBtn(L"SimpleViewClose");

    assignFn(fullscreenpopupbase, testEventListener3);
    assignFn(SimpleViewTop, testEventListener3);
    assignFn(SimpleViewBottom, testEventListener3);
    assignFn(SimpleViewSettings, ShowSettings);
    assignFn(SimpleViewClose, ExitWindow);

    showcheckboxes = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect");
    hiddenIcons = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons");
    globaliconsz = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize");
    shellstate = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState");
    if (globaliconsz > 48) globalshiconsz = 48; else globalshiconsz = 32;
    globalgpiconsz = 12;
    if (globaliconsz > 48) globalgpiconsz = 32;
    else if (globaliconsz > 32) globalgpiconsz = 16;
    InitLayout();
    SetTheme();

    wnd->Host(pMain);
    bool testB = ToggleDesktopIcons(false, false, &logging);
    if (logging == IDYES) {
        if (testB) TaskDialog(wnd->GetHWND(), GetModuleHandleW(NULL), L"Information", NULL, L"SysListView32 has been hidden.", TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
        else TaskDialog(wnd->GetHWND(), GetModuleHandleW(NULL), L"Error", NULL, L"Could not hide SysListView32.", TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
    }
    wnd->ShowWindow(SW_SHOW);
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(wnd->GetHWND(), &m);
    StartMessagePump();
    UnInitProcessPriv(0);
    CoUninitialize();

    return 0;
}