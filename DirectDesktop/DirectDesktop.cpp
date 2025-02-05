#include "framework.h"
#include "DirectUI/DirectUI.h"
#include "DirectDesktop.h"
#include <string>
#include "resource.h"
#include <propkey.h>
#include "strsafe.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <vector>
#include <WinUser.h>
#include <ShlObj_core.h>
#include <ShlGuid.h>
#include <ShObjIdl.h>
#include <shellapi.h>
#include "StyleModifier.h"
#include "BitmapHelper.h"
#include "DirectoryHelper.h"
#include "SettingsHelper.h";

using namespace DirectUI;
using namespace std;

NativeHWNDHost* wnd;
HWNDElement* parent;
DUIXmlParser* parser;
Element* pMain;
unsigned long key = 0;

Button* testButton2, *testButton3, *testButton4, *testButton5;
Element* sampleText;
Element* mainContainer;
Element* UIContainer;
Element* iconElem;
Element* shortcutElem;
Element* iconElemShadow;
RichText* textElem;
RichText* textElemShadow;
Button* checkboxElem;
Element* fullscreenpopup, *fullscreeninner;
Button* fullscreenpopupbase, *centered;
Element* itemcountstatus;
Button* emptyspace;
Element* selector;
Element* dirnameanimator;
Element* tasksanimator;
Element* tools;
Button* SimpleViewTop, *SimpleViewBottom;
TouchButton* SimpleViewSettings;
TouchButton* PageTab1, *PageTab2;
RichText* SubUIContainer;
HRESULT err;

int popupframe, dframe, tframe;
vector<int> frame;
void InitLayout();

struct EventListener : public IElementListener {

    void (*f)(Element*, Event*);

    EventListener(void (*func)(Element*, Event*)) {
        f = func; 
    }

    void OnListenerAttach(Element* elem) override { }
    void OnListenerDetach(Element* elem) override { }
    bool OnPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override {
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
    bool OnPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override {
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
    Element* result = (Element*)pMain->FindDescendent(StrToID((UCString)elemName));
    return result;
}
RichText* regRichText(const wchar_t* elemName) {
    RichText* result = (RichText*)pMain->FindDescendent(StrToID((UCString)elemName));
    return result;
}
Button* regBtn(const wchar_t* btnName) {
    Button* result = (Button*)pMain->FindDescendent(StrToID((UCString)btnName));
    return result;
}
TouchButton* regTouchBtn(const wchar_t* btnName) {
    TouchButton* result = (TouchButton*)pMain->FindDescendent(StrToID((UCString)btnName));
    return result;
}
Edit* regEdit(const wchar_t* editName) {
    Edit* result = (Edit*)pMain->FindDescendent(StrToID((UCString)editName));
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
int origX{}, origY{};

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
    parser->GetSheet((UCString)sheetName, &sheetStorage);
    pMain->SetValue(Element::SheetProp, 1, sheetStorage);
    sheetStorage->Release();
}

WNDPROC WndProc;
vector<Element*> iconpm, subiconpm;
vector<Element*> shadowpm, subshadowpm;
vector<RichText*> filepm, subfilepm;
vector<RichText*> fileshadowpm;
vector<Element*> cbpm;
int holddownseconds = 0;
bool checkifelemexists = 0;
bool issubviewopen = 0;
void fullscreenAnimation(int width, int height);

HBITMAP GetShellItemImage(LPCWSTR filePath, int width, int height) {

    IShellItem* pShellItem{};
    HRESULT hr = SHCreateItemFromParsingName(filePath, NULL, IID_PPV_ARGS(&pShellItem));

    IShellItemImageFactory* pImageFactory{};
    hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
    pShellItem->Release();

    SIZE size = { width, height };
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
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL);
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

Value* bitmapbuffer;
void ShowSimpleView() {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    Value* bitmap;
    HBITMAP hbmCapture{};
    if (!issubviewopen) {
        HDC hdcWindow = GetDC(wnd->GetHWND());
        HDC hdcMem = CreateCompatibleDC(hdcWindow);
        hbmCapture = CreateCompatibleBitmap(hdcWindow, dimensions.right, dimensions.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
        BitBlt(hdcMem, 0, 0, dimensions.right, dimensions.bottom, hdcWindow, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
        ReleaseDC(wnd->GetHWND(), hdcWindow);
    }
    bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 4, 0xffffffff, false, false, false);
    fullscreenAnimation(dimensions.right * 0.7, dimensions.bottom * 0.7);
    if (issubviewopen) {
        fullscreeninner->SetValue(Element::BackgroundProp, 1, bitmapbuffer);
    }
    else fullscreeninner->SetValue(Element::BackgroundProp, 1, bitmap);
    SimpleViewTop->SetLayoutPos(1);
    SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
    SimpleViewBottom->SetLayoutPos(3);
    fullscreenpopup->SetBackgroundStdColor(24);
    if (!issubviewopen) bitmapbuffer = DirectUI::Value::CreateGraphic(hbmCapture, 4, 0xffffffff, false, false, false);
}

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    int innerSizeX = GetSystemMetrics(SM_CXICONSPACING);
    int innerSizeY = GetSystemMetrics(SM_CYICONSPACING) - tm.tmHeight;
    int iconPaddingX = (innerSizeX - 48) / 2;
    int iconPaddingY = (innerSizeY - 48 + tm.tmHeight) / 2;
    switch (uMsg) {
    case WM_SETTINGCHANGE: {
        UpdateModeInfo();
        SetTheme();
        break;
    }
    case WM_COMMAND: {
        break;
    }
    case WM_USER + 1: {
        //pm[wParam].elem->SetAlpha(255);
        pm[wParam].elem->SetVisible(true);
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
        pm[wParam].elem->SetHeight((innerSizeY + lines_basedOnEllipsis + 7) * (0.7 + 0.3 * bezierProgress));
        pm[wParam].elem->SetX(round((dimensions.right - 2 * pm[wParam].x) * 0.15 * (1 - bezierProgress) + pm[wParam].x));
        pm[wParam].elem->SetY(round((dimensions.bottom - 2 * pm[wParam].y) * 0.15 * (1 - bezierProgress) + pm[wParam].y));
        filepm[wParam]->SetHeight(lines_basedOnEllipsis + 4);
        fileshadowpm[wParam]->SetHeight(lines_basedOnEllipsis + 4);
        iconpm[wParam]->SetWidth(round(48 * (0.7 + 0.3 * bezierProgress)));
        iconpm[wParam]->SetHeight(round(48 * (0.7 + 0.3 * bezierProgress)));
        iconpm[wParam]->SetX(round(iconPaddingX * (0.7 + 0.3 * bezierProgress)));
        iconpm[wParam]->SetY(round((iconPaddingY * 0.575) * (0.7 + 0.3 * bezierProgress)));
        HBITMAP capturedBitmap;
        capturedBitmap = CreateTextBitmap(pm[wParam].simplefilename.c_str(), innerSizeX - 2, lines_basedOnEllipsis, DT_END_ELLIPSIS);
        HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 1);
        IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
        IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, 2, 1);
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
        shadowpm[wParam]->SetWidth(64);
        shadowpm[wParam]->SetHeight(64);
        shadowpm[wParam]->SetX(iconPaddingX - 8);
        shadowpm[wParam]->SetY((iconPaddingY * 0.575) - 6);
        shortpm[wParam].elem->SetAlpha(255);
        shortpm[wParam].elem->SetWidth(32);
        shortpm[wParam].elem->SetHeight(32);
        shortpm[wParam].elem ->SetX(iconPaddingX);
        shortpm[wParam].elem->SetY((iconPaddingY * 0.575) + 16);
        break;
    }
    case WM_USER + 5: {
        POINT ppt;
        GetCursorPos(&ppt);
        ScreenToClient(wnd->GetHWND(), &ppt);
        if (ppt.x >= origX) selector->SetWidth(ppt.x - origX);
        if (ppt.x < origX) {
            selector->SetWidth(origX - ppt.x);
            selector->SetX(ppt.x);
            holddownseconds = 0;
        }
        if (ppt.y >= origY) selector->SetHeight(ppt.y - origY);
        if (ppt.y < origY) {
            selector->SetHeight(origY - ppt.y);
            selector->SetY(ppt.y);
            holddownseconds = 0;
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
        if (issubviewopen) {
            ShowSimpleView();
        }
        issubviewopen = false;
        break;
    }
    case WM_USER + 8: {
        parser->CreateElement((UCString)L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninner);
        centered->Add((Element**)&fullscreeninner, 1);
        centered->SetMinSize(800, 480);
        fullscreeninner->SetMinSize(800, 480);
    }
    case WM_USER + 9: {
        double bezierProgress = 1; //py[frame[wParam] - 1];
        subpm[wParam].elem->SetWidth(innerSizeX * (0.7 + 0.3 * bezierProgress));
        subpm[wParam].elem->SetHeight((innerSizeY + tm.tmHeight + 24) * (0.7 + 0.3 * bezierProgress));
        subpm[wParam].elem->SetX(subpm[wParam].x); // round((720 - 2 * subpm[wParam].x) * 0.15 * (1 - bezierProgress) + subpm[wParam].x));
        subpm[wParam].elem->SetY(subpm[wParam].y); // round((360 - 2 * subpm[wParam].y) * 0.15 * (1 - bezierProgress) + subpm[wParam].y));
        int textlines = 1;
        if (tm.tmHeight <= 18) textlines = 2;
        subfilepm[wParam]->SetHeight(tm.tmHeight * textlines + 4);
        if (subfilepm[wParam]->GetHeight() > (iconPaddingY * 0.575 + 48)) subfilepm[wParam]->SetHeight(iconPaddingY * 0.575 + 48);
        subiconpm[wParam]->SetWidth(round(48 * (0.7 + 0.3 * bezierProgress)));
        subiconpm[wParam]->SetHeight(round(48 * (0.7 + 0.3 * bezierProgress)));
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
        subshadowpm[wParam]->SetWidth(64);
        subshadowpm[wParam]->SetHeight(64);
        subshadowpm[wParam]->SetX(iconPaddingX - 8);
        subshadowpm[wParam]->SetY((iconPaddingY * 0.575) - 6);
        subshortpm[wParam].elem->SetAlpha(255);
        subshortpm[wParam].elem->SetWidth(32);
        subshortpm[wParam].elem->SetHeight(32);
        subshortpm[wParam].elem->SetX(iconPaddingX);
        subshortpm[wParam].elem->SetY((iconPaddingY * 0.575) + 16);
        break;
    }
    case WM_USER + 11: {
        holddownseconds++;
        if (holddownseconds == 2 && selector->GetWidth() < 2 && selector->GetHeight() < 2) {
            ShowSimpleView();
            break;
        }
    }
    case WM_USER + 12: {
        if (checkifelemexists == true) dirnameanimator->SetWidth(160 * (1 - py[dframe - 1]));
        break;
    }
    case WM_USER + 13: {
        if (checkifelemexists == true) tasksanimator->SetWidth(80 * (1 - py[tframe - 1]));
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
    this_thread::sleep_for(chrono::milliseconds(100));
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
    fullscreenpopup->SetLayoutPos(4);
    fullscreenpopup->SetAlpha(255);
    HDC hdcWindow = GetDC(wnd->GetHWND());
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hbmCapture = CreateCompatibleBitmap(hdcWindow, dimensions.right * 0.33, dimensions.bottom * 0.33);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
    StretchBlt(hdcMem, 0, 0, dimensions.right * 0.33, dimensions.bottom * 0.33, hdcWindow, 0, 0, dimensions.right, dimensions.bottom, SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    ReleaseDC(wnd->GetHWND(), hdcWindow);
    IterateBitmap(hbmCapture, StandardBitmapPixelHandler, 2, 4, 1);
    Value* bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 4, 0xffffffff, false, false, false);
    fullscreenpopup->SetValue(Element::BackgroundProp, 1, bitmap);
    bitmap->Release();
    parser->CreateElement((UCString)L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninner);
    centered->Add((Element**)&fullscreeninner, 1);
    centered->SetMinSize(width, height);
    fullscreeninner->SetMinSize(width, height);
    fullscreenpopupbase->SetVisible(true);
    fullscreeninner->SetVisible(true);
}
void fullscreenAnimation2() {
    DWORD animThread;
    HANDLE animThreadHandle = CreateThread(0, 0, animate6, NULL, 0, &animThread);
}
void ShowPopupCore() {
    fullscreenpopup->SetLayoutPos(4);
    fullscreenpopup->SetAlpha(255);
    fullscreenAnimation(800, 480);
}
void HidePopupCore() {
    fullscreenpopup->SetAlpha(0);
    fullscreenAnimation2();
    sublistDirBuffer.clear();
    frame.clear();
    subpm.clear();
    subiconpm.clear();
    subshortpm.clear();
    subshadowpm.clear();
    subfilepm.clear();
    subhiddenIndex = 0;
    subshortIndex = 0;
    SimpleViewTop->SetLayoutPos(-3);
    SimpleViewBottom->SetLayoutPos(-3);
}

wstring bufferOpenInExplorer;
void OpenGroupInExplorer(Element* elem, Event* iev) {
    if (iev->type == TouchButton::Click) {
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
        bmResult.push_back(GetShellItemImage((pm[i].filename).c_str(), 48, 48));
    }
    return bmResult;
}
vector<HBITMAP> GetSubdirectoryIcons() {
    vector<HBITMAP> bmResult;
    for (int i = 0; i < subpm.size(); i++) {
        bmResult.push_back(GetShellItemImage((subpm[i].filename).c_str(), 48, 48));
    }
    return bmResult;
}

void ApplyIcons(vector<Element*> pmIcon, vector<Element*> pmIconShadow, vector<parameters> pmShortcut, vector<HBITMAP> iconstofetch) {
    HINSTANCE testInst = LoadLibraryW(L"imageres.dll");
    vector<HBITMAP> icons = iconstofetch;
    for (int icon = 0; icon < pmIcon.size(); icon++) {
        HICON ico = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(2), IMAGE_ICON, 48, 48, LR_SHARED);
        HICON icoShortcut = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(163), IMAGE_ICON, 32, 32, LR_SHARED);
        HBITMAP bmp = IconToBitmap(ico);
        bmp = icons[icon];
        HBITMAP bmpShadow = AddPaddingToBitmap(bmp, 8);
        HBITMAP bmpShortcut = IconToBitmap(icoShortcut);
        if (isColorized) {
            IterateBitmap(bmp, StandardBitmapPixelHandler, 1, 0, 1);
            IterateBitmap(bmpShortcut, StandardBitmapPixelHandler, 1, 0, 1);
        }
        IterateBitmap(bmpShadow, SimpleBitmapPixelHandler, 0, 4, 0.33);
        IterateBitmap(bmpShortcut, UndoPremultiplication, 1, 0, 1);
        Value* bitmap = DirectUI::Value::CreateGraphic(bmp, 2, 0xffffffff, false, false, false);
        Value* bitmapShadow = DirectUI::Value::CreateGraphic(bmpShadow, 2, 0xffffffff, false, false, false);
        Value* bitmapShortcut = DirectUI::Value::CreateGraphic(bmpShortcut, 2, 0xffffffff, false, false, false);
        pmIcon[icon]->SetValue(Element::ContentProp, 1, bitmap);
        pmIconShadow[icon]->SetValue(Element::ContentProp, 1, bitmapShadow);
        if (pmShortcut[icon].x == 1) pmShortcut[icon].elem->SetValue(Element::ContentProp, 1, bitmapShortcut);
        DeleteObject(ico);
        DeleteObject(icoShortcut);
        DeleteObject(bmp);
        DeleteObject(bmpShadow);
        DeleteObject(bmpShortcut);
        bitmap->Release();
        bitmapShadow->Release();
        bitmapShortcut->Release();
    }
    icons.clear();
}

void SelectSubItem(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
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
    fullscreenAnimation(800, 480);
    Element* groupdirectory{};
    parser->CreateElement((UCString)L"groupdirectory", NULL, NULL, NULL, (Element**)&groupdirectory);
    fullscreeninner->Add((Element**)&groupdirectory, 1);
    ScrollViewer* groupdirlist = (ScrollViewer*)groupdirectory->FindDescendent(StrToID((UCString)L"groupdirlist"));
    SubUIContainer = (RichText*)groupdirlist->FindDescendent(StrToID((UCString)L"SubUIContainer"));
    vector<wstring> subfiles = list_subdirectory(filename + wstring(L"\\*"));
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
        int outerSizeX = GetSystemMetrics(SM_CXICONSPACING) + 4;
        int outerSizeY = GetSystemMetrics(SM_CYICONSPACING) + 27;
        DWORD* animThread = new DWORD[count];
        DWORD* animThread2 = new DWORD[count];
        HANDLE* animThreadHandle = new HANDLE[count];
        HANDLE* animThreadHandle2 = new HANDLE[count];
        for (int i = 0; i < count; i++) {
            Button* outerElemGrouped;
            parser->CreateElement((UCString)L"outerElemGrouped", NULL, NULL, NULL, (Element**)&outerElemGrouped);
            SubUIContainer->Add((Element**)&outerElemGrouped, 1);
            iconElem = (Element*)outerElemGrouped->FindDescendent(StrToID((UCString)L"iconElem"));
            shortcutElem = (Element*)outerElemGrouped->FindDescendent(StrToID((UCString)L"shortcutElem"));
            iconElemShadow = (Element*)outerElemGrouped->FindDescendent(StrToID((UCString)L"iconElemShadow"));
            textElem = (RichText*)outerElemGrouped->FindDescendent(StrToID((UCString)L"textElem"));
            subpm[i].elem = outerElemGrouped, subpm[i].filename = sublistDirBuffer[i], subpm[i].simplefilename = subfiles[i];
            subiconpm[i] = iconElem;
            subshortpm[i].elem = shortcutElem;
            subshadowpm[i] = iconElemShadow;
            subfilepm[i] = textElem;
            if (subpm[i].isHidden == true) {
                iconElem->SetAlpha(128);
                iconElemShadow->SetVisible(false);
                textElem->SetAlpha(128);
            }
            assignFn(outerElemGrouped, SelectSubItem);
            outerElemGrouped->SetClass((UCString)L"singleclicked");
        }
        ApplyIcons(subiconpm, subshadowpm, subshortpm, GetSubdirectoryIcons());
        for (int j = 0; j < count; j++) {
            subpm[j].x = x, subpm[j].y = y;
            yValue* yV = new yValue{ j };
            yValue* yV2 = new yValue{ j };
            x += outerSizeX;
            if (x > 800 - (dimensions.left + dimensions.right + outerSizeX)) {
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
            SubUIContainer->SetContentString((UCString)L"This folder is too large.");
        }
        else SubUIContainer->SetContentString((UCString)L"This folder is empty.");
    }
    dirnameanimator = (Element*)groupdirectory->FindDescendent(StrToID((UCString)L"dirnameanimator"));
    tasksanimator = (Element*)groupdirectory->FindDescendent(StrToID((UCString)L"tasksanimator"));
    RichText* dirname = (RichText*)groupdirectory->FindDescendent(StrToID((UCString)L"dirname"));
    dirname->SetContentString((UCString)simplefilename.c_str());
    dirname->SetAlpha(255);
    Element* tasks = (Element*)groupdirectory->FindDescendent(StrToID((UCString)L"tasks"));
    checkifelemexists = true;
    DWORD animThread3;
    DWORD animThread4;
    HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
    HANDLE animThreadHandle4 = CreateThread(0, 0, grouptasksanimation, NULL, 0, &animThread4);
    TouchButton* Customize = (TouchButton*)groupdirectory->FindDescendent(StrToID((UCString)L"Customize"));
    TouchButton* OpenInExplorer = (TouchButton*)groupdirectory->FindDescendent(StrToID((UCString)L"OpenInExplorer"));
    Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
    assignFn(OpenInExplorer, OpenGroupInExplorer);
    bufferOpenInExplorer = (wstring)filename;
}

void ShowPage1(Element* elem, Event* iev) {
    if (iev->type == TouchButton::Click) {
        PageTab2->SetSelected(false);
        PageTab1->SetSelected(true);
        SubUIContainer->DestroyAll(true);
        Element* SettingsPage1;
        parser->CreateElement((UCString)L"SettingsPage1", NULL, NULL, NULL, (Element**)&SettingsPage1);
        SubUIContainer->Add((Element**)&SettingsPage1, 1);
        Button* ItemCheckboxes = (Button*)SettingsPage1->FindDescendent(StrToID((UCString)L"ItemCheckboxes"));
        Button* ShowHiddenFiles = (Button*)SettingsPage1->FindDescendent(StrToID((UCString)L"ShowHiddenFiles"));
        Button* FilenameExts = (Button*)SettingsPage1->FindDescendent(StrToID((UCString)L"FilenameExts"));
        Button* TreatDirAsGroup = (Button*)SettingsPage1->FindDescendent(StrToID((UCString)L"TreatDirAsGroup"));
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
    if (iev->type == TouchButton::Click) {
        PageTab1->SetSelected(false);
        PageTab2->SetSelected(true);
        SubUIContainer->DestroyAll(true);
        Element* SettingsPage2;
        parser->CreateElement((UCString)L"SettingsPage2", NULL, NULL, NULL, (Element**)&SettingsPage2);
        SubUIContainer->Add((Element**)&SettingsPage2, 1);
        Button* EnableAccent = (Button*)SettingsPage2->FindDescendent(StrToID((UCString)L"EnableAccent"));
        EnableAccent->SetSelected(isColorized);
        assignFn(EnableAccent, ToggleAccentIcons);
    }
}
void ShowSettings(Element* elem, Event* iev) {
    if (iev->type == TouchButton::Click) {
        fullscreenpopup->SetLayoutPos(-3);
        centered->DestroyAll(true);
        ShowPopupCore();
        SimpleViewTop->SetLayoutPos(-3);
        SimpleViewBottom->SetLayoutPos(-3);
        issubviewopen = 1;
        Element* settingsview{};
        parser->CreateElement((UCString)L"settingsview", NULL, NULL, NULL, (Element**)&settingsview);
        fullscreeninner->Add((Element**)&settingsview, 1);
        ScrollViewer* settingslist = (ScrollViewer*)settingsview->FindDescendent(StrToID((UCString)L"settingslist"));
        SubUIContainer = (RichText*)settingsview->FindDescendent(StrToID((UCString)L"SubUIContainer"));
        PageTab1 = (TouchButton*)settingsview->FindDescendent(StrToID((UCString)L"PageTab1"));
        PageTab2 = (TouchButton*)settingsview->FindDescendent(StrToID((UCString)L"PageTab2"));
        assignFn(PageTab1, ShowPage1);
        assignFn(PageTab2, ShowPage2);
        ShowPage1(elem, iev);
        CubicBezier(48, px, py, 0.1, 0.9, 0.2, 1.0);
        dirnameanimator = (Element*)settingsview->FindDescendent(StrToID((UCString)L"dirnameanimator"));
        RichText* name = (RichText*)settingsview->FindDescendent(StrToID((UCString)L"name"));
        name->SetAlpha(255);
        checkifelemexists = true;
        DWORD animThread3;
        HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
    }
}
#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 30000

Element* elemStorage;
void SelectItem(Element* elem, Event* iev) {
    static int validation = 0;
    if (iev->type == Button::Click) {
        validation++;
        Button* checkbox = (Button*)elem->FindDescendent(StrToID((UCString)L"checkboxElem"));
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
                textElem = (RichText*)pm[items].elem->FindDescendent(StrToID((UCString)L"textElem"));
                textElemShadow = (RichText*)pm[items].elem->FindDescendent(StrToID((UCString)L"textElemShadow"));
                if (spacingInternal == 1.5) {
                    if (pm[items].elem->GetSelected() == true) pm[items].elem->SetHeight(pm[items].elem->GetHeight() + extraBottomSpacing * 0.5);
                    else pm[items].elem->SetHeight(pm[items].elem->GetHeight() - extraBottomSpacing);
                }
                textElem->SetHeight(extraBottomSpacing + 4);
                textElemShadow->SetHeight(extraBottomSpacing + 4);
                HBITMAP capturedBitmap = CreateTextBitmap(pm[items].simplefilename.c_str(), pm[items].elem->GetWidth() - 2, extraBottomSpacing, DT_END_ELLIPSIS);
                HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 1);
                IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
                IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, 2, 1);
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
    else if (iev->type == Button::Context)
    {
        for (int items = 0; items < pm.size(); items++) {
            if (pm[items].elem == elem) {
                LPCWSTR folderPath = pm[items].filename.c_str();
                LPITEMIDLIST pidl = NULL;
                SHParseDisplayName(folderPath, NULL, &pidl, 0, NULL);

                IShellFolder* ppFolder = NULL;
                LPITEMIDLIST pidlChild = NULL;
                HRESULT hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&ppFolder, (LPCITEMIDLIST*)&pidlChild);

                LPCONTEXTMENU pICv1 = NULL;
                ppFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlChild ,IID_IContextMenu, NULL, (void**)&pICv1);
                if (pICv1)
                {
                    HMENU hm = CreatePopupMenu();
                    pICv1->QueryContextMenu(hm, 0, MIN_SHELL_ID, MAX_SHELL_ID, CMF_EXPLORE);
                        
                    UINT uFlags = TPM_RIGHTBUTTON;
                    if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
                        uFlags |= TPM_RIGHTALIGN;
                    else
                        uFlags |= TPM_LEFTALIGN;

                    // Use TPM_RETURNCMD flag let TrackPopupMenuEx function return the menu item identifier of the user's selection in the return value.
                    uFlags |= TPM_RETURNCMD;

                    POINT pt;
                    GetCursorPos(&pt);
                    int menuItemId = TrackPopupMenuEx(hm, uFlags, pt.x, pt.y, wnd->GetHWND(), NULL);
                    CMINVOKECOMMANDINFO ici;
                    ZeroMemory(&ici , sizeof(ici));
                    ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
                    ici.lpVerb = MAKEINTRESOURCEA(menuItemId-1);
                    ici.nShow = SW_SHOWNORMAL;

                    pICv1->InvokeCommand(&ici);
                }
                CoTaskMemFree(pidl);
                ppFolder->Release();
            }
        }
    }
}

void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
    checkboxElem = (Button*)elem->FindDescendent(StrToID((UCString)L"checkboxElem"));   
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
        Element* item = parent->FindDescendent(StrToID((UCString)L"innerElem"));
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
    if (pProp == Button::PressedProp()) {
        POINT ppt;
        GetCursorPos(&ppt);
        ScreenToClient(wnd->GetHWND(), &ppt);
        origX = ppt.x;
        origY = ppt.y;
        selector->SetX(origX);
        selector->SetY(origY);
        selector->SetVisible(true);
        selector->SetLayoutPos(-2);
        isPressed = 1;
        marqueeThreadHandle = CreateThread(0, 0, UpdateMarqueeSelectorPosition, NULL, 0, &marqueeThread);
    }
    else if (GetAsyncKeyState(VK_LBUTTON) == 0) {
        selector->SetVisible(false);
        selector->SetLayoutPos(-3);
        isPressed = 0;
        holddownseconds = 0;
    }
}

unsigned long PrepareForSimpleView(LPVOID lpParam) {
    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
        if (isPressed) {
            holddownseconds++;
            SendMessageW(wnd->GetHWND(), WM_USER + 11, NULL, NULL);
        }
        else {
            holddownseconds = 0;
            break;
        }
    }
    return 0;
}

void OpenSimpleView(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pv2) {
    DWORD sviewThread;
    HANDLE sviewThreadHandle;
    if (pProp == Button::PressedProp()) {
        isPressed = 1;
        sviewThreadHandle = CreateThread(0, 0, PrepareForSimpleView, NULL, 0, &sviewThread);
    }
    else if (GetAsyncKeyState(VK_LBUTTON) == 0) {
        isPressed = 0;
        holddownseconds = 0;
    }
}

void testEventListener3(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
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

void DesktopRightClick(Element* elem, Event* iev) {
    if (iev->type == Button::Context) {

        IShellView* pShellView = NULL;
        IContextMenu* pContextMenu = NULL;
        IShellFolder* pShellFolder = NULL;

        HRESULT hr = SHGetDesktopFolder(&pShellFolder);
        pShellFolder->CreateViewObject(GetShellWindow(), IID_PPV_ARGS(&pShellView));

        LPCONTEXTMENU3 pICv1 = NULL;
        pShellView->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu3, (LPVOID*)&pICv1);
        if (pICv1)
        {
            HMENU hm = CreatePopupMenu();
            AppendMenuW(hm, MF_STRING, 0, L"View");
            pICv1->QueryContextMenu(hm, 1, MIN_SHELL_ID, MAX_SHELL_ID, CMF_EXPLORE);
            RemoveMenu(hm, 1, MF_BYPOSITION);
            RemoveMenu(hm, 1, MF_BYPOSITION);
            RemoveMenu(hm, 1, MF_BYPOSITION);
            InsertMenuW(hm, 2, MF_BYPOSITION | MF_STRING, 2, L"Open Edit Mode");

            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
                uFlags |= TPM_RIGHTALIGN;
            else
                uFlags |= TPM_LEFTALIGN;

            // Use TPM_RETURNCMD flag let TrackPopupMenuEx function return the menu item identifier of the user's selection in the return value.
            uFlags |= TPM_RETURNCMD;

            POINT pt;
            GetCursorPos(&pt);
            int menuItemId = TrackPopupMenuEx(hm, uFlags, pt.x, pt.y, wnd->GetHWND(), NULL);
            CMINVOKECOMMANDINFO ici;
            ZeroMemory(&ici, sizeof(ici));
            ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
            ici.lpVerb = MAKEINTRESOURCEA(menuItemId - 1);
            ici.nShow = SW_SHOWNORMAL;
            pICv1->InvokeCommand(&ici);
            switch (menuItemId) {
            case 2:
                ShowSimpleView();
                break;
            case 4:
                InitLayout();
                InitLayout();
                break;
            }
        }
        pShellFolder->Release();
    }
}

void InitLayout() {
    GetFontHeight();
    static bool openclose = 0;
    wchar_t icount[32];
    vector<wstring> files = list_directory();
    unsigned int count = files.size();
    testButton5->SetEnabled(false);
    shellstate = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState");
    switch (openclose) {
    case 0: {
        Button* emptyspace;
        parser->CreateElement((UCString)L"emptyspace", NULL, NULL, NULL, (Element**)&emptyspace);
        UIContainer->Add((Element**)&emptyspace, 1);
        assignFn(emptyspace, SelectItem);
        assignExtendedFn(emptyspace, ShowCheckboxIfNeeded);
        assignExtendedFn(emptyspace, MarqueeSelector);
        assignFn(emptyspace, DesktopRightClick);
        assignExtendedFn(emptyspace, OpenSimpleView);
        swprintf_s(icount, L"        Found %d items!", count);
        itemcountstatus->SetContentString((UCString)icount);
        itemcountstatus->SetVisible(true); itemcountstatus->SetAlpha(0);
        int x = 4, y = 4;
        CubicBezier(24, px, py, 0.1, 0.9, 0.2, 1.0);
        frame.resize(count);
        pm.resize(count);
        iconpm.resize(count);
        shortpm.resize(count);
        shadowpm.resize(count);
        filepm.resize(count);
        fileshadowpm.resize(count);
        cbpm.resize(count);
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        DWORD* animThread = new DWORD[count];
        DWORD* animThread2 = new DWORD[count];
        HANDLE* animThreadHandle = new HANDLE[count];
        HANDLE* animThreadHandle2 = new HANDLE[count];
        int outerSizeX = GetSystemMetrics(SM_CXICONSPACING) + 4;
        int outerSizeY = GetSystemMetrics(SM_CYICONSPACING) + 27;
        for (int i = 0; i < count; i++) {
            Button* outerElem;
            parser->CreateElement((UCString)L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
            UIContainer->Add((Element**)&outerElem, 1);
            iconElem = (Element*)outerElem->FindDescendent(StrToID((UCString)L"iconElem"));
            shortcutElem = (Element*)outerElem->FindDescendent(StrToID((UCString)L"shortcutElem"));
            iconElemShadow = (Element*)outerElem->FindDescendent(StrToID((UCString)L"iconElemShadow"));
            textElem = (RichText*)outerElem->FindDescendent(StrToID((UCString)L"textElem"));
            textElemShadow = (RichText*)outerElem->FindDescendent(StrToID((UCString)L"textElemShadow"));
            checkboxElem = (Button*)outerElem->FindDescendent(StrToID((UCString)L"checkboxElem"));
            pm[i].elem = outerElem, pm[i].filename = listDirBuffer[i], pm[i].simplefilename = files[i];
            iconpm[i] = iconElem;
            shortpm[i].elem = shortcutElem;
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
            assignExtendedFn(outerElem, ShowCheckboxIfNeeded);
            assignExtendedFn(checkboxElem, CheckboxHandler);
            if (shellstate[4] >= 48 && shellstate[4] <= 63) {
                outerElem->SetClass((UCString)L"doubleclicked");
                //if (pm[i].isDirectory == true && treatdirasgroup == true) outerElem->SetClass((UCString)L"singleclicked");
            }
            else outerElem->SetClass((UCString)L"singleclicked");
        }
        ApplyIcons(iconpm, shadowpm, shortpm, GetDesktopIcons());
        for (int j = 0; j < count; j++) {
            pm[j].x = x, pm[j].y = y;
            yValue* yV = new yValue{ j };
            yValue* yV2 = new yValue{ j };
            y += outerSizeY;
            if (y > dimensions.bottom - outerSizeY) {
                y = 4;
                x += outerSizeX;
            }
            animThreadHandle[j] = CreateThread(0, 0, animate, (LPVOID)yV, 0, &(animThread[j]));
            animThreadHandle2[j] = CreateThread(0, 0, fastin, (LPVOID)yV2, 0, &(animThread2[j]));
        }
        files.clear();
        openclose = 1;
        delete[] animThread;
        delete[] animThread2;
        delete[] animThreadHandle;
        delete[] animThreadHandle2;
        break;
    }
    case 1: {
        UIContainer->DestroyAll(true);
        listDirBuffer.clear();
        frame.clear();
        pm.clear();
        iconpm.clear();
        shortpm.clear();
        shadowpm.clear();
        filepm.clear();
        cbpm.clear();
        dirIndex = 0;
        hiddenIndex = 0;
        shortIndex = 0;
        openclose = 0;
        itemcountstatus->SetVisible(false); itemcountstatus->SetAlpha(255);
        break;
    }
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    InitProcessPriv(14, NULL, 0, true);
    InitThread(2);
    RegisterAllControls();
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    NativeHWNDHost::Create((UCString)L"DirectDesktop", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 768, NULL, WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU, 0, &wnd);
    DUIXmlParser::Create(&parser, NULL, NULL, NULL, NULL);
    parser->SetXMLFromResource(IDR_UIFILE2, hInstance, hInstance);
    HWNDElement::Create(wnd->GetHWND(), true, NULL, NULL, &key, (Element**)&parent);
    WndProc = (WNDPROC)SetWindowLongPtr(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);

    parser->CreateElement((UCString)L"main", parent, NULL, NULL, &pMain);
    pMain->SetVisible(true);
    pMain->EndDefer(key);

    UpdateModeInfo();

    sampleText = regElem(L"sampleText");
    testButton2 = regBtn(L"testButton2");
    testButton3 = regBtn(L"testButton3");
    testButton4 = regBtn(L"testButton4");
    testButton5 = regBtn(L"testButton5");
    mainContainer = regElem(L"mainContainer");
    UIContainer = regElem(L"UIContainer");
    fullscreenpopup = regElem(L"fullscreenpopup");
    fullscreenpopupbase = regBtn(L"fullscreenpopupbase");
    centered = regBtn(L"centered");
    itemcountstatus = regElem(L"itemcountstatus");
    selector = regElem(L"selector");
    tools = regElem(L"tools");
    SimpleViewTop = regBtn(L"SimpleViewTop");
    SimpleViewBottom = regBtn(L"SimpleViewBottom");
    SimpleViewSettings = regTouchBtn(L"SimpleViewSettings");

    assignFn(testButton3, testEventListener3);
    assignFn(fullscreenpopupbase, testEventListener3);
    assignFn(SimpleViewTop, testEventListener3);
    assignFn(SimpleViewBottom, testEventListener3);
    assignFn(SimpleViewSettings, ShowSettings);

    showcheckboxes = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect");
    InitLayout();
    SetTheme();

    wnd->Host(pMain);

    wnd->ShowWindow(SW_SHOW);
    StartMessagePump();
    UnInitProcessPriv(0);
    CoUninitialize();

    return 0;
}