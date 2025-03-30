#include "framework.h"
#include "DirectUI/DirectUI.h"
#include "DUser/DUser.h"
#include "DirectDesktop.h"
#include <string>
#include "resource.h"
#include <propkey.h>
#include "strsafe.h"
#include <cmath>
#include <vector>
#include <list>
#include <regex>
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
DDScalableElement* iconElem;
Element* shortcutElem;
Element* iconElemShadow;
RichText* textElem;
RichText* textElemShadow;
Button* checkboxElem;
DDScalableButton* fullscreeninner;
Button* fullscreenpopupbase, *centered;
DDScalableElement* simpleviewoverlay;
Element* deskpreview;
Button* emptyspace;
Element* selector, *selector2;
Element* dirnameanimator;
Element* tasksanimator;
Button* SimpleViewTop, *SimpleViewBottom;
Button* SimpleViewSettings, *SimpleViewClose;
DDScalableButton* PageTab1, *PageTab2, *PageTab3;
RichText* SubUIContainer;
TouchButton* nextpage, *prevpage;
TouchButton* prevpageMain, *nextpageMain;
RichText* pageinfo;
Element* dragpreview;

HRESULT err;
HWND hWorkerW = NULL;
HWND hSHELLDLL_DefView = NULL;
HWND hWndTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
Logger MainLogger;

int maxPageID = 1, currentPageID = 1;
int popupframe, dframe, tframe;
int localeType{};
int touchSizeX, touchSizeY;

wstring LoadStrFromRes(UINT id) {
    WCHAR* loadedStrBuffer = new WCHAR[512]{};
    LoadStringW((HINSTANCE)GetModuleHandleW(NULL), id, loadedStrBuffer, 512);
    wstring loadedStr = loadedStrBuffer;
    delete[] loadedStrBuffer;
    return loadedStr;
}

wstring RemoveQuotes(const wstring& input) {
    if (input.size() >= 2 && input.front() == L'\"' && input.back() == L'\"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

int dpi = 96, dpiOld = 1, dpiLaunch{};
int listviewAnimStorage{};
float flScaleFactor = 1.0;
bool isDpiPreviouslyChanged;
void InitialUpdateScale() {
    HDC screen = GetDC(0);
    dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(0, screen);
    flScaleFactor = dpi / 96.0;
    dpiLaunch = dpi;
    touchSizeX *= flScaleFactor;
    touchSizeY *= flScaleFactor;
}

void UpdateScale() {
    HWND hWnd = subviewwnd->GetHWND();
    dpiOld = dpi;
    dpi = GetDpiForWindow(hWnd);
    isDpiPreviouslyChanged = true;
    flScaleFactor = dpi / 96.0;
    DDScalableElement::RedrawImages();
    DDScalableButton::RedrawImages();
    DDScalableElement::RedrawFonts();
    DDScalableButton::RedrawFonts();
    touchSizeX *= static_cast<float>(dpi) / dpiOld;
    touchSizeY *= static_cast<float>(dpi) / dpiOld;
}

int GetCurrentScaleInterval() {
    if (dpi >= 384) return 6;
    if (dpi >= 288) return 5;
    if (dpi >= 240) return 4;
    if (dpi >= 192) return 3;
    if (dpi >= 144) return 2;
    if (dpi >= 120) return 1;
    return 0;
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
    int innerSizeX{};
    int innerSizeY{};
};

struct DesktopIcon {
    HBITMAP icon{};
    HBITMAP iconshadow{};
    HBITMAP iconshortcut{};
    HBITMAP text{};
    HBITMAP textshadow{};
};

struct ThumbnailIcon {
    int x{};
    int y{};
    ThumbIcons str;
    HBITMAP icon;
};

double px[80]{};
double py[80]{};
int origX{}, origY{}, globaliconsz, globalshiconsz, globalgpiconsz;
bool touchmode{};

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

WNDPROC WndProc, WndProc2;
HANDLE hMutex;
constexpr LPCWSTR szWindowClass = L"DIRECTDESKTOP";
vector<LVItem*> pm, subpm;
vector<Element*> shortpm, subshortpm;
vector<DDScalableElement*> iconpm, subiconpm;
vector<Element*> shadowpm, subshadowpm;
vector<RichText*> filepm, subfilepm;
vector<RichText*> fileshadowpm, subfileshadowpm;
vector<Element*> cbpm;
vector<LVItem*> selectedLVItems;
bool checkifelemexists = 0;
bool issubviewopen = 0;
bool hiddenIcons;
bool editmode = 0;
bool pendingaction = 0;
bool invokedpagechange = 0;
void fullscreenAnimation(int width, int height, float animstartscale);
void HidePopupCore(bool WinDInvoked);
void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
void ApplyIcons(vector<LVItem*> pmLVItem, vector<DDScalableElement*> pmIcon, DesktopIcon* di, bool subdirectory, int id);
unsigned long CreateIndividualThumbnail(LPVOID lpParam);

HBITMAP GetShellItemImage(LPCWSTR filePath, int width, int height) {

    IShellItem2* pShellItem{};
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
    SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, dpi);
    if (touchmode) lf.lfHeight *= 1.25;
    HFONT hFont = CreateFontIndirectW(&lf);
    HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
    RECT rc = { 0, 0, width - 4, textm.tmHeight };
    wchar_t filenameBuffer[260]{};
    int lines_b1 = 1;
    int tilelines = 1;
    while (lines_b1 != 0) {
        rc.bottom = textm.tmHeight * tilelines;
        tilelines++;
        wcscpy_s(filenameBuffer, str);
        DWORD direction = (localeType == 1) ? DT_RIGHT : DT_LEFT;
        DWORD alignment = touchmode ? direction | DT_WORD_ELLIPSIS : DT_CENTER;
        DrawTextExW(hdcBuffer, filenameBuffer, -1, &rc, alignment | DT_MODIFYSTRING | DT_END_ELLIPSIS | DT_LVICON, NULL);
        lines_b1 = wcscmp(str, filenameBuffer);
        if (!touchmode || tilelines > 5) break;
    }
    if (touchmode) {
        DeleteObject(hFont);
        DeleteObject(hOldFont);
        DeleteDC(hdcBuffer);
        return tilelines;
    }
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
void CalcDesktopIconInfo(yValue* yV, int* lines_basedOnEllipsis, DWORD* alignment, bool subdirectory) {
    vector<LVItem*>* pmLVItem = subdirectory ? &subpm : &pm;
    vector<RichText*>* pmFile = subdirectory ? &subfilepm : &filepm;
    vector<RichText*>* pmFileShadow = subdirectory ? &subfileshadowpm : &fileshadowpm;
    *alignment = DT_CENTER | DT_END_ELLIPSIS;
    if (!touchmode) {
        *lines_basedOnEllipsis = floor(CalcTextLines((*pmLVItem)[yV->y]->GetSimpleFilename().c_str(), yV->innerSizeX)) * textm.tmHeight;
    }
    if (touchmode) {
        DWORD direction = (localeType == 1) ? DT_RIGHT : DT_LEFT;
        *alignment = direction | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS;
        int maxlines_basedOnEllipsis = (*pmFile)[yV->y]->GetHeight();
        yV->innerSizeX = (*pmFileShadow)[yV->y]->GetWidth();
        *lines_basedOnEllipsis = CalcTextLines((*pmLVItem)[yV->y]->GetSimpleFilename().c_str(), yV->innerSizeX) * textm.tmHeight;
        if (*lines_basedOnEllipsis > maxlines_basedOnEllipsis) *lines_basedOnEllipsis = maxlines_basedOnEllipsis;
    }
}

void PlaySimpleViewAnimation(Element* elem, int width, int height, int animation, float startscale) {
    elem->SetAnimation(NULL);
    elem->SetWidth(width * startscale);
    elem->SetHeight(height * startscale);
    elem->SetAnimation(animation);
    elem->SetWidth(width);
    elem->SetHeight(height);
}

void ShowSimpleView() {
    editmode = true;
    if (!invokedpagechange) SendMessageW(hWndTaskbar, WM_COMMAND, 419, 0);
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
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
    fullscreenAnimation(dimensions.right * 0.7, dimensions.bottom * 0.7, 1.4);
    SetWindowPos(subviewwnd->GetHWND(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    parser->CreateElement(L"deskpreview", NULL, NULL, NULL, (Element**)&deskpreview);
    centered->Add((Element**)&deskpreview, 1);
    if (bitmap != nullptr) deskpreview->SetValue(Element::BackgroundProp, 1, bitmap);
    static const int savedanim4 = deskpreview->GetAnimation();
    PlaySimpleViewAnimation(deskpreview, dimensions.right * 0.7, dimensions.bottom * 0.7, savedanim4, 1.4);
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
    SimpleViewTop->SetLayoutPos(1);
    SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
    SimpleViewBottom->SetLayoutPos(3);
    fullscreeninner->SetFirstScaledImage(-1);
    fullscreeninner->SetBackgroundStdColor(7);
    parser->CreateElement(L"simpleviewoverlay", NULL, NULL, NULL, (Element**)&simpleviewoverlay);
    centered->Add((Element**)&simpleviewoverlay, 1);
    static const int savedanim3 = simpleviewoverlay->GetAnimation();
    PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right * 0.7, dimensions.bottom * 0.7, savedanim3, 1.4);
    BlurBackground(subviewwnd->GetHWND(), false);
    wnd->ShowWindow(SW_SHOW);
    invokedpagechange = false;
}

unsigned long ShowTimedSimpleView(LPVOID lpParam) {
    Sleep(250);
    SendMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 1);
    return 0;
}

unsigned long EndExplorer(LPVOID lpParam) {
    Sleep(250);
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    DWORD pid{};
    GetWindowThreadProcessId(hWndProgman, &pid);
    HANDLE hExplorer;
    hExplorer = OpenProcess(PROCESS_TERMINATE, false, pid);
    TerminateProcess(hExplorer, 2);
    CloseHandle(hExplorer);
    return 0;
}

void AdjustWindowSizes(bool firsttime) {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    POINT topLeftMon = GetTopLeftMonitor();
    UINT swpFlags = SWP_NOZORDER;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    if (firsttime) swpFlags |= SWP_NOMOVE | SWP_NOSIZE;
    if (localeType == 1) {
        int rightMon = GetRightMonitor();
        topLeftMon.x = dimensions.right + dimensions.left - rightMon;
    }
    SetWindowPos(wnd->GetHWND(), NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
    SetWindowPos(subviewwnd->GetHWND(), NULL, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
    SetWindowPos(hWorkerW, NULL, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
    SetWindowPos(hSHELLDLL_DefView, NULL, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
    UIContainer->SetWidth(dimensions.right - dimensions.left);
    UIContainer->SetHeight(dimensions.bottom - dimensions.top);
    SetWindowPos(hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

unsigned long WallpaperHelper24H2(LPVOID lpParam) {
    int WindowsBuild = _wtoi(GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber"));
    Sleep(200);
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    hSHELLDLL_DefView = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
    bool bTest = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, false);
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
        if (wParam == SPI_SETWORKAREA) {
            AdjustWindowSizes(false);
            RearrangeIcons(true, false);
        }
        if (wParam == SPI_SETDESKWALLPAPER) {
            static int messagemitigation{};
            messagemitigation++;
            if (messagemitigation & 1) {
                int WindowsBuild = _wtoi(GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber"));
                if (WindowsBuild >= 26002) {
                    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
                    SetParent(hSHELLDLL_DefView, hWndProgman);
                    DWORD dwWallpaper{};
                    HANDLE wallpaperThread = CreateThread(0, 0, WallpaperHelper24H2, NULL, 0, &dwWallpaper);
                }
            }
        }
        if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
            UpdateModeInfo();
            // This message is sent 4-5 times upon changing accent color so this mitigation is applied
            static int messagemitigation{};
            messagemitigation++;
            SetTheme();
            DDScalableElement::RedrawImages();
            DDScalableButton::RedrawImages();
            if (messagemitigation % 5 == 2) {
                if (isColorized) RearrangeIcons(false, true);
            }
        }
        break;
    }
    case WM_WINDOWPOSCHANGING: {
        ((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
        return 0L;
        break;
    }
    case WM_CLOSE: {
        SetWindowPos(subviewwnd->GetHWND(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        if (lParam == 420) {
            wchar_t* desktoplog = new wchar_t[260];
            wchar_t* cBuffer = new wchar_t[260];
            DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
            StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
            ShellExecuteW(NULL, L"open", L"notepad.exe", desktoplog, NULL, SW_SHOW);
            delete[] desktoplog;
            delete[] cBuffer;
        }
        DWORD dwTermination{};
        HANDLE termThread = CreateThread(0, 0, EndExplorer, NULL, 0, &dwTermination);
        Sleep(500);
        break;
    }
    case WM_COMMAND: {
        break;
    }
    case WM_HOTKEY: {
        switch (wParam) {
        case 1:
            DWORD dw;
            HANDLE handle = CreateThread(0, 0, ShowTimedSimpleView, NULL, 0, &dw);
            break;
        }
        break;
    }
    case WM_USER + 1: {
        //pm[lParam].elem->SetAlpha(255);
        if (pm[lParam]->GetPage() == currentPageID) pm[lParam]->SetVisible(!hiddenIcons);
        else pm[lParam]->SetVisible(false);
        break;
    }
    case WM_USER + 2: {
        //subpm[lParam].elem->SetAlpha(255);
        if (checkifelemexists == true) subpm[lParam]->SetVisible(true);
        break;
    }
    case WM_USER + 3: {
        int lines_basedOnEllipsis{};
        if (!touchmode) {
            lines_basedOnEllipsis = floor(CalcTextLines(pm[lParam]->GetSimpleFilename().c_str(), innerSizeX)) * textm.tmHeight;
            pm[lParam]->SetWidth(innerSizeX);
            pm[lParam]->SetHeight(innerSizeY + lines_basedOnEllipsis + 7 * flScaleFactor);
            filepm[lParam]->SetHeight(lines_basedOnEllipsis + 4 * flScaleFactor);
            fileshadowpm[lParam]->SetHeight(lines_basedOnEllipsis + 5 * flScaleFactor);
            iconpm[lParam]->SetWidth(round(globaliconsz * flScaleFactor));
            iconpm[lParam]->SetHeight(round(globaliconsz * flScaleFactor));
            iconpm[lParam]->SetX(iconPaddingX);
            iconpm[lParam]->SetY(round(iconPaddingY * 0.575));
        }
        HBITMAP iconbmp = ((DesktopIcon*)wParam)->icon;
        Value* bitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(iconbmp);
        if (bitmap != nullptr) {
            iconpm[lParam]->SetValue(Element::ContentProp, 1, bitmap);
            bitmap->Release();
        }
        HBITMAP iconshadowbmp = ((DesktopIcon*)wParam)->iconshadow;
        Value* bitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(iconshadowbmp);
        if (bitmapShadow != nullptr) {
            shadowpm[lParam]->SetValue(Element::ContentProp, 1, bitmapShadow);
            bitmapShadow->Release();
        }
        HBITMAP iconshortcutbmp = ((DesktopIcon*)wParam)->iconshortcut;
        Value* bitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(iconshortcutbmp);
        if (bitmapShortcut != nullptr) {
            if (pm[lParam]->GetShortcutState() == true) shortpm[lParam]->SetValue(Element::ContentProp, 1, bitmapShortcut);
            bitmapShortcut->Release();
        }
        HBITMAP textbmp = ((DesktopIcon*)wParam)->text;
        Value* bitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(textbmp);
        if (bitmapText != nullptr) {
            filepm[lParam]->SetValue(Element::ContentProp, 1, bitmapText);
            bitmapText->Release();
        }
        HBITMAP textshadowbmp = ((DesktopIcon*)wParam)->textshadow;
        Value* bitmapTextShadow = DirectUI::Value::CreateGraphic(textshadowbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(textshadowbmp);
        if (bitmapTextShadow != nullptr) {
            fileshadowpm[lParam]->SetValue(Element::ContentProp, 1, bitmapTextShadow);
            bitmapTextShadow->Release();
        }
        pm[lParam]->SetAnimation(listviewAnimStorage);
        break;
    }
    case WM_USER + 4: {
        if (pm[lParam]->GetHiddenState() == false) shadowpm[lParam]->SetAlpha(255);
        shortpm[lParam]->SetAlpha(255);
        if (!touchmode) {
            shadowpm[lParam]->SetWidth((globaliconsz + 16) * flScaleFactor);
            shadowpm[lParam]->SetHeight((globaliconsz + 16) * flScaleFactor);
            shadowpm[lParam]->SetX(iconPaddingX - 8 * flScaleFactor);
            shadowpm[lParam]->SetY((iconPaddingY * 0.575) - 6 * flScaleFactor);
            shortpm[lParam]->SetWidth(globalshiconsz * flScaleFactor);
            shortpm[lParam]->SetHeight(globalshiconsz * flScaleFactor);
            shortpm[lParam]->SetX(iconPaddingX);
            shortpm[lParam]->SetY((iconPaddingY * 0.575) + (globaliconsz - globalshiconsz) * flScaleFactor);
        }
        break;
    }
    case WM_USER + 5: {
        POINT ppt;
        GetCursorPos(&ppt);
        ScreenToClient(wnd->GetHWND(), &ppt);
        if (localeType == 1) ppt.x = dimensions.right - ppt.x;
        MARGINS borders = { (ppt.x < origX) ? ppt.x : origX, abs(ppt.x - origX),
            (ppt.y < origY) ? ppt.y : origY, abs(ppt.y - origY) };
        selector->SetWidth(borders.cxRightWidth);
        selector->SetX(borders.cxLeftWidth);
        selector->SetHeight(borders.cyBottomHeight);
        selector->SetY(borders.cyTopHeight);
        selector2->SetWidth(borders.cxRightWidth);
        selector2->SetX(borders.cxLeftWidth);
        selector2->SetHeight(borders.cyBottomHeight);
        selector2->SetY(borders.cyTopHeight);
        for (int items = 0; items < validItems; items++) {
            MARGINS iconborders = { pm[items]->GetX(), pm[items]->GetX() + pm[items]->GetWidth(), pm[items]->GetY(), pm[items]->GetY() + pm[items]->GetHeight() };
            bool selectstate = (borders.cxRightWidth + borders.cxLeftWidth > iconborders.cxLeftWidth &&
                iconborders.cxRightWidth > borders.cxLeftWidth &&
                borders.cyBottomHeight + borders.cyTopHeight > iconborders.cyTopHeight &&
                iconborders.cyBottomHeight > borders.cyTopHeight);
            pm[items]->SetSelected(selectstate);
            if (showcheckboxes == 1) cbpm[items]->SetVisible(selectstate);
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
        if (pendingaction) Sleep(700);
        static const int savedanim = mainContainer->GetAnimation();
        if (lParam == 1) {
            //mainContainer->SetAnimation(NULL);
            mainContainer->SetAlpha(255);
            mainContainer->SetVisible(true);
            //mainContainer->SetAnimation(savedanim);
        }
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
        break;
    }
    case WM_USER + 9: {
        if (!touchmode) {
            subpm[lParam]->SetWidth(innerSizeX);
            subpm[lParam]->SetHeight(innerSizeY + textm.tmHeight + 23 * flScaleFactor);
            int textlines = 1;
            if (textm.tmHeight <= 18 * flScaleFactor) textlines = 2;
            subfilepm[lParam]->SetHeight(textm.tmHeight * textlines + 4 * flScaleFactor);
            if (subfilepm[lParam]->GetHeight() > (iconPaddingY * 0.575 + 48)) subfilepm[lParam]->SetHeight(iconPaddingY * 0.575 + 48);
            subiconpm[lParam]->SetWidth(round(globaliconsz * flScaleFactor));
            subiconpm[lParam]->SetHeight(round(globaliconsz * flScaleFactor));
            subiconpm[lParam]->SetX(iconPaddingX);
            subiconpm[lParam]->SetY(round(iconPaddingY * 0.575));
        }
        HBITMAP iconbmp = ((DesktopIcon*)wParam)->icon;
        Value* bitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(iconbmp);
        if (bitmap != nullptr) {
            subiconpm[lParam]->SetValue(Element::ContentProp, 1, bitmap);
            bitmap->Release();
        }
        HBITMAP iconshadowbmp = ((DesktopIcon*)wParam)->iconshadow;
        Value* bitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(iconshadowbmp);
        if (bitmapShadow != nullptr) {
            subshadowpm[lParam]->SetValue(Element::ContentProp, 1, bitmapShadow);
            bitmapShadow->Release();
        }
        HBITMAP iconshortcutbmp = ((DesktopIcon*)wParam)->iconshortcut;
        Value* bitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(iconshortcutbmp);
        if (bitmapShortcut != nullptr) {
            if (subpm[lParam]->GetShortcutState() == true) subshortpm[lParam]->SetValue(Element::ContentProp, 1, bitmapShortcut);
            bitmapShortcut->Release();
        }
        HBITMAP textbmp = ((DesktopIcon*)wParam)->text;
        Value* bitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
        DeleteObject(textbmp);
        if (bitmapText != nullptr) {
            subfilepm[lParam]->SetValue(Element::ContentProp, 1, bitmapText);
            bitmapText->Release();
        }
        if (touchmode) {
            HBITMAP textshadowbmp = ((DesktopIcon*)wParam)->textshadow;
            Value* bitmapTextShadow = DirectUI::Value::CreateGraphic(textshadowbmp, 2, 0xffffffff, false, false, false);
            DeleteObject(textshadowbmp);
            if (bitmapTextShadow != nullptr) {
                subfileshadowpm[lParam]->SetValue(Element::ContentProp, 1, bitmapTextShadow);
                bitmapTextShadow->Release();
            }
        }
        break;
    }
    case WM_USER + 10: {
        if (subpm[lParam]->GetHiddenState() == false) subshadowpm[lParam]->SetAlpha(255);
        subshortpm[lParam]->SetAlpha(255);
        if (!touchmode) {
            subshadowpm[lParam]->SetWidth((globaliconsz + 16) * flScaleFactor);
            subshadowpm[lParam]->SetHeight((globaliconsz + 16) * flScaleFactor);
            subshadowpm[lParam]->SetX(iconPaddingX - 8 * flScaleFactor);
            subshadowpm[lParam]->SetY((iconPaddingY * 0.575) - 6 * flScaleFactor);
            subshortpm[lParam]->SetWidth(globalshiconsz * flScaleFactor);
            subshortpm[lParam]->SetHeight(globalshiconsz * flScaleFactor);
            subshortpm[lParam]->SetX(iconPaddingX);
            subshortpm[lParam]->SetY((iconPaddingY * 0.575) + (globaliconsz - globalshiconsz) * flScaleFactor);
        }
        break;
    }
    case WM_USER + 11: {
        pendingaction = true;
        Element* peTemp = reinterpret_cast<Element*>(wParam);
        peTemp->SetEnabled(!peTemp->GetEnabled());
        if (lParam == 1 && ((DDScalableButton*)peTemp)->GetAssociatedFn() != nullptr) {
            ((DDScalableButton*)peTemp)->ExecAssociatedFn(((DDScalableButton*)peTemp)->GetAssociatedFn(), false, true);
            pendingaction = false;
        }
        break;
    }
    case WM_USER + 12: {
        if (checkifelemexists == true) dirnameanimator->SetWidth((100 * (1 - py[dframe - 1])) * flScaleFactor);
        break;
    }
    case WM_USER + 13: {
        if (checkifelemexists == true) tasksanimator->SetWidth((60 * (1 - py[tframe - 1])) * flScaleFactor);
        break;
    }
    case WM_USER + 14: {
        vector<DWORD> smThumbnailThread;
        vector<HANDLE> smThumbnailThreadHandle;
        smThumbnailThread.resize(pm.size());
        smThumbnailThreadHandle.resize(pm.size());
        static Value* v{};
        UpdateCache* uc{};
        if (iconpm[0] != nullptr) v = iconpm[0]->GetValue(Element::BackgroundProp, 1, uc);
        for (int icon = 0; icon < pm.size(); icon++) {
            iconpm[icon]->DestroyAll(true);
            iconpm[icon]->SetClass(L"");
            iconpm[icon]->SetValue(Element::BackgroundProp, 1, v);
            shadowpm[icon]->SetVisible(true);
            static const int groupspace = 8 * flScaleFactor;
            if (touchmode) {
                iconpm[icon]->SetWidth(globaliconsz * flScaleFactor + 2 * groupspace);
                iconpm[icon]->SetHeight(globaliconsz * flScaleFactor + 2 * groupspace);
            }
            Element* iconcontainer = (Element*)pm[icon]->FindDescendent(StrToID(L"iconcontainer"));
            if (touchmode) iconcontainer->SetPadding(0, 0, 0, 0);
            if (pm[icon]->GetDirState() == true && treatdirasgroup == true) {
                iconpm[icon]->SetClass(L"groupthumbnail");
                shadowpm[icon]->SetVisible(false);
                if (touchmode) {
                    iconcontainer->SetPadding(groupspace, groupspace, groupspace, groupspace);
                    iconpm[icon]->SetWidth(globaliconsz * flScaleFactor);
                    iconpm[icon]->SetHeight(globaliconsz * flScaleFactor);
                }
            }
        }
        for (int icon2 = 0; icon2 < pm.size(); icon2++) {
            yValue* yV = new yValue{ icon2 };
            smThumbnailThreadHandle[icon2] = CreateThread(0, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, &(smThumbnailThread[icon2]));
        }
        smThumbnailThread.clear();
        smThumbnailThreadHandle.clear();
        break;
    }
    case WM_USER + 15: {
        if (!editmode || lParam == 0) ShowSimpleView();
        else HidePopupCore(false);
        break;
    }
    case WM_USER + 16: {
        ThumbnailIcon* ti = (ThumbnailIcon*)wParam;
        HBITMAP thumbIcon = ti->icon;
        Value* vThumbIcon = DirectUI::Value::CreateGraphic(thumbIcon, 2, 0xffffffff, false, false, false);
        Element* GroupedIcon{};
        parser->CreateElement(L"GroupedIcon", NULL, NULL, NULL, (Element**)&GroupedIcon);
        iconpm[lParam]->Add((Element**)&GroupedIcon, 1);
        GroupedIcon->SetWidth(globalgpiconsz * flScaleFactor), GroupedIcon->SetHeight(globalgpiconsz * flScaleFactor);
        GroupedIcon->SetX(ti->x), GroupedIcon->SetY(ti->y);
        if (ti->str.GetHiddenState()) GroupedIcon->SetAlpha(128);
        if (vThumbIcon != nullptr) GroupedIcon->SetValue(Element::ContentProp, 1, vThumbIcon);
        DeleteObject(thumbIcon);
        if (vThumbIcon != nullptr) vThumbIcon->Release();
        free(ti);
        break;
    }
    case WM_USER + 17: {
        POINT ppt;
        GetCursorPos(&ppt);
        ScreenToClient(wnd->GetHWND(), &ppt);
        if (localeType == 1) ppt.x = dimensions.right - ppt.x;
        static const int dragWidth = _wtoi(GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragWidth"));
        static const int dragHeight = _wtoi(GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragHeight"));
        if (abs(ppt.x - ((POINT*)lParam)->x) > dragWidth || abs(ppt.y - ((POINT*)lParam)->y) > dragHeight) {
            (*(vector<LVItem*>*)wParam)[0]->SetDragState(true);
            dragpreview->SetVisible(true);
        }
        dragpreview->SetX(ppt.x - origX);
        dragpreview->SetY(ppt.y - origY);
        break;
    }
    case WM_USER + 18: {
        switch (lParam) {
        case 0: {
            vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
            POINT ppt;
            GetCursorPos(&ppt);
            ScreenToClient(wnd->GetHWND(), &ppt);
            int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
            int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 22) * flScaleFactor;
            int largestXPos = dimensions.right / outerSizeX;
            int largestYPos = dimensions.bottom / outerSizeY;
            int desktoppadding = touchmode ? DESKPADDING_TOUCH : DESKPADDING_NORMAL;
            desktoppadding *= flScaleFactor;
            if (touchmode) {
                outerSizeX = touchSizeX + desktoppadding;
                outerSizeY = touchSizeY + desktoppadding;
            }
            int xRender = (localeType == 1) ? ppt.x + origX : ppt.x - origX;
            int paddingmitigation = (localeType == 1) ? desktoppadding : 0;
            int destX = desktoppadding + round(xRender / static_cast<float>(outerSizeX)) * outerSizeX - paddingmitigation;
            int destY = desktoppadding + round((ppt.y - origY) / static_cast<float>(outerSizeY)) * outerSizeY;
            if (localeType == 1) {
                destX = dimensions.right - destX;
            }
            const int mainElementX = internalselectedLVItems[0]->GetX();
            const int mainElementY = internalselectedLVItems[0]->GetY();
            int itemstodrag = internalselectedLVItems.size();
            if (itemstodrag == 0) itemstodrag = 1;
            for (int items = 0; items < itemstodrag; items++) {
                int finaldestX = destX - mainElementX + internalselectedLVItems[items]->GetX();
                int finaldestY = destY - mainElementY + internalselectedLVItems[items]->GetY();
                if (localeType == 1) {
                    if (finaldestX < 0) {
                        xRender = 0;
                        finaldestX = (desktoppadding + round(xRender / static_cast<float>(outerSizeX)) * outerSizeX - paddingmitigation);
                    }
                    if (finaldestX > dimensions.right - outerSizeX) finaldestX = dimensions.right - outerSizeX;
                }
                else {
                    if (finaldestX < 0) finaldestX = desktoppadding;
                    if (finaldestX > dimensions.right - outerSizeX + desktoppadding) finaldestX = round((dimensions.right - outerSizeX) / static_cast<float>(outerSizeX)) * outerSizeX + desktoppadding;
                }
                if (finaldestY < 0) finaldestY = desktoppadding;
                if (finaldestY > dimensions.bottom - outerSizeY + desktoppadding) finaldestY = round((dimensions.bottom - outerSizeY) / static_cast<float>(outerSizeY)) * outerSizeY + desktoppadding;
                for (int items2 = 0; items2 < pm.size(); items2++) {
                    if (pm[items2]->GetX() == finaldestX && pm[items2]->GetY() == finaldestY && pm[items2]->GetPage() == internalselectedLVItems[items]->GetPage()) break;
                    if (items2 == pm.size() - 1) {
                        internalselectedLVItems[items]->SetX(finaldestX);
                        internalselectedLVItems[items]->SetY(finaldestY);
                    }
                }
            }
            internalselectedLVItems.clear();
            dragpreview->SetVisible(false);
            break;
        }
        case 1: {
            LVItem* item = (*(vector<LVItem*>*)wParam)[0];
            item->SetDragState(false);
            break;
        }
        }
        break;
    }
    }
    return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK TopLevelWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DPICHANGED: {
        UpdateScale();
        InitLayout(false, false);
        break;
    }
    case WM_DISPLAYCHANGE: {
        AdjustWindowSizes(true);
        RearrangeIcons(true, false);
        break;
    }
    case WM_USER + 1: {
        if (wParam < 4096) break;
        if (((DDScalableElement*)wParam)->GetFirstScaledImage() == -1) break;
        int scaleInterval = GetCurrentScaleInterval();
        int scaleIntervalImage = ((DDScalableElement*)wParam)->GetScaledImageIntervals();
        if (scaleInterval > scaleIntervalImage - 1) scaleInterval = scaleIntervalImage - 1;
        int imageID = ((DDScalableElement*)wParam)->GetFirstScaledImage() + scaleInterval;
        HBITMAP newImage = (HBITMAP)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (newImage == nullptr) {
            newImage = LoadPNGAsBitmap(imageID);
            IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1);
        }
        if (((DDScalableElement*)wParam)->GetEnableAccent() == 1) IterateBitmap(newImage, StandardBitmapPixelHandler, 1, 0, 1);
        switch (((DDScalableElement*)wParam)->GetDrawType()) {
        case 1: {
            Value* vImage = Value::CreateGraphic(newImage, 7, 0xffffffff, true, false, false);
            if (vImage) {
                ((DDScalableElement*)wParam)->SetValue(Element::BackgroundProp, 1, vImage);
                vImage->Release();
            }
            break;
        }
        case 2: {
            Value* vImage = Value::CreateGraphic(newImage, 2, 0xffffffff, true, false, false);
            if (vImage) {
                ((DDScalableElement*)wParam)->SetValue(Element::ContentProp, 1, vImage);
                vImage->Release();
            }
            break;
        }
        }
        if (newImage) DeleteObject(newImage);
        break;
    }
    case WM_USER + 2: {
        Value* v;
        if (((DDScalableElement*)wParam)->GetNeedsFontResize() > 0) {
            if (((DDScalableElement*)wParam)->GetFont(&v) == nullptr) break;
            wstring fontOld = ((DDScalableElement*)wParam)->GetFont(&v);
            wregex fontRegex(L".*font;.*\%");
            bool isSysmetricFont = regex_match(fontOld, fontRegex);
            if (isSysmetricFont) {
                size_t modifier = fontOld.find(L";");
                wstring fontIntermediate = fontOld.substr(0, modifier + 1);
                wstring fontIntermediate2 = fontOld.substr(modifier + 1, wcslen(fontOld.c_str()) - 1);
                int newFontSize = _wtoi(fontIntermediate2.c_str()) * dpi / dpiLaunch;
                wstring fontNew = fontIntermediate + to_wstring(newFontSize) + L"\%";
                ((DDScalableElement*)wParam)->SetFont(fontNew.c_str());
            }
        }
    }
    }
    return CallWindowProc(WndProc2, hWnd, uMsg, wParam, lParam);
}

unsigned long animate(LPVOID lpParam) {
    Sleep(50);
    yValue* yV = (yValue*)lpParam;
    SendMessageW(wnd->GetHWND(), WM_USER + 1, NULL, yV->y);
    free(yV);
    return 0;
}

unsigned long subanimate(LPVOID lpParam) {
    Sleep(50);
    yValue* yV = (yValue*)lpParam;
    SendMessageW(wnd->GetHWND(), WM_USER + 2, NULL, yV->y);
    free(yV);
    return 0;
}

unsigned long fastin(LPVOID lpParam) {
    Sleep(50);
    yValue* yV = (yValue*)lpParam;
    int lines_basedOnEllipsis{};
    DWORD alignment{};
    RECT touchmoderect{};
    CalcDesktopIconInfo(yV, &lines_basedOnEllipsis, &alignment, false);
    HBITMAP capturedBitmap;
    capturedBitmap = CreateTextBitmap(pm[yV->y]->GetSimpleFilename().c_str(), yV->innerSizeX - 4 * flScaleFactor, lines_basedOnEllipsis, alignment, touchmode);
    float textshader = touchmode ? 0.75 : 2;
    IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
    HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor);
    IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), textshader);
    DesktopIcon di;
    ApplyIcons(pm, iconpm, &di, false, yV->y);
    di.text = capturedBitmap;
    di.textshadow = shadowBitmap;
    SendMessageW(wnd->GetHWND(), WM_USER + 4, NULL, yV->y);
    SendMessageW(wnd->GetHWND(), WM_USER + 3, (WPARAM)&di, yV->y);
    free(yV);
    return 0;
}

unsigned long subfastin(LPVOID lpParam) {
    Sleep(50);
    yValue* yV = (yValue*)lpParam;
    int lines_basedOnEllipsis{};
    DWORD alignment{};
    RECT touchmoderect{};
    int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
    int textlines = 1;
    if (textm.tmHeight <= 18 * flScaleFactor) textlines = 2;
    if (touchmode) CalcDesktopIconInfo(yV, &lines_basedOnEllipsis, &alignment, true);
    HBITMAP capturedBitmap;
    if (touchmode) capturedBitmap = CreateTextBitmap(subpm[yV->y]->GetSimpleFilename().c_str(), yV->innerSizeX - 4 * flScaleFactor, lines_basedOnEllipsis, alignment, touchmode);
    else capturedBitmap = CreateTextBitmap(subpm[yV->y]->GetSimpleFilename().c_str(), innerSizeX, textm.tmHeight * textlines, DT_CENTER | DT_END_ELLIPSIS, touchmode);
    float textshader = touchmode ? 0.75 : 2;
    if (theme && !touchmode) {
        IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1);
        IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 0.9);
    }
    else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33);
    HBITMAP shadowBitmap{};
    if (touchmode) {
        shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor);
        IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), textshader);
    }
    DesktopIcon di;
    ApplyIcons(subpm, subiconpm, &di, false, yV->y);
    di.text = capturedBitmap;
    di.textshadow = shadowBitmap;
    SendMessageW(wnd->GetHWND(), WM_USER + 10, NULL, yV->y);
    SendMessageW(wnd->GetHWND(), WM_USER + 9, (WPARAM)&di, yV->y);
    free(yV);
    return 0;
}

unsigned long animate5(LPVOID lpParam) {
    CubicBezier(30, px, py, 0.05, 0.9, 0.3, 1.0);
    SendMessageW(wnd->GetHWND(), WM_USER + 8, NULL, NULL);
    for (int m = 1; m <= 30; m++) {
        popupframe = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 6, NULL, NULL);
        Sleep((int)((px[m] - px[m - 1]) * 200));
    }
    return 0;
}

unsigned long animate6(LPVOID lpParam) {
    Sleep(200);
    pSubview->SetAccessible(false);
    subviewwnd->ShowWindow(SW_HIDE);
    BlurBackground(subviewwnd->GetHWND(), false);
    SendMessageW(wnd->GetHWND(), WM_USER + 7, NULL, NULL);
    return 0;
}

unsigned long grouptitlebaranimation(LPVOID lpParam) {
    Sleep(750);
    for (int m = 1; m <= 32; m++) {
        dframe = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 12, NULL, NULL);
        Sleep((int)((px[m] - px[m - 1]) * 600));
    }
    return 0;
}
unsigned long grouptasksanimation(LPVOID lpParam) {
    for (int m = 1; m <= 32; m++) {
        tframe = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 13, NULL, NULL);
        Sleep((int)((px[m] - px[m - 1]) * 450));
    }
    return 0;
}

void fullscreenAnimation(int width, int height, float animstartscale) {
    pSubview->SetAccessible(true);
    subviewwnd->ShowWindow(SW_SHOW);
    parser->CreateElement(L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninner);
    centered->Add((Element**)&fullscreeninner, 1);
    static const int savedanim = centered->GetAnimation();
    static const int savedanim2 = fullscreeninner->GetAnimation();
    PlaySimpleViewAnimation(centered, width, height, savedanim, animstartscale);
    PlaySimpleViewAnimation(fullscreeninner, width, height, savedanim2, animstartscale);
    centered->SetBackgroundColor(0);
    fullscreenpopupbase->SetAlpha(255);
    fullscreenpopupbase->SetVisible(true);
    fullscreeninner->SetVisible(true);
    BlurBackground(subviewwnd->GetHWND(), true);
    issubviewopen = 1;
}
void fullscreenAnimation2() {
    DWORD animThread;
    HANDLE animThreadHandle = CreateThread(0, 0, animate6, NULL, 0, &animThread);
}
void ShowPopupCore() {
    pSubview->SetAccessible(true);
    subviewwnd->ShowWindow(SW_SHOW);
    fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor, 0.9);
}
void HidePopupCore(bool WinDInvoked) {
    if (!WinDInvoked) SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
    fullscreenpopupbase->SetAlpha(0);
    if (issubviewopen) {
        centered->SetWidth(centered->GetWidth() * 0.85);
        centered->SetHeight(centered->GetHeight() * 0.85);
        fullscreeninner->SetWidth(fullscreeninner->GetWidth() * 0.85);
        fullscreeninner->SetHeight(fullscreeninner->GetHeight() * 0.85);
        if (editmode) {
            simpleviewoverlay->SetWidth(simpleviewoverlay->GetWidth() * 0.85);
            simpleviewoverlay->SetHeight(simpleviewoverlay->GetHeight() * 0.85);
            deskpreview->SetWidth(deskpreview->GetWidth() * 0.85);
            deskpreview->SetHeight(deskpreview->GetHeight() * 0.85);
        }
    }
    issubviewopen = 0;
    mainContainer->SetVisible(true);
    mainContainer->SetAlpha(255);
    if (!editmode) SetWindowPos(subviewwnd->GetHWND(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    editmode = false;
    fullscreenAnimation2();
    subpm.clear();
    subiconpm.clear();
    subshortpm.clear();
    subshadowpm.clear();
    subfilepm.clear();
    subfileshadowpm.clear();
    SimpleViewTop->SetLayoutPos(-3);
    SimpleViewBottom->SetLayoutPos(-3);
    nextpage->SetWidth(0);
    prevpage->SetWidth(0);
}

wstring bufferOpenInExplorer;
void OpenGroupInExplorer(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
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
    Sleep(_wtoi(dcms));
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
    StringCchPrintfW(currentPage, 64, LoadStrFromRes(4026).c_str(), currentPageID, maxPageID);
    pageinfo->SetContentString(currentPage);
}
unsigned long LoadOtherPageThumbnail(LPVOID lpParam) {
    SendMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
    Sleep(50);
    SendMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
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
            invokedpagechange = true;
            float xLoc = (localeType == 1) ? 0 : 0.9;
            float xLoc2 = (localeType == 1) ? 0.9 : 0;
            TogglePage(nextpage, xLoc, 0.2, 0.1, 0.6);
            if (currentPageID == 1) TogglePage(prevpage, xLoc2, 0.2, 0, 0.6);
            DWORD dd;
            HANDLE thumbnailThread = CreateThread(0, 0, LoadOtherPageThumbnail, NULL, 0, &dd);
        }
        else {
            UIContainer->SetAnimation(NULL);
            short animSrc = (localeType == 1) ? 1 : -1;
            UIContainer->SetX((dimensions.right - dimensions.left) * animSrc);
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
            invokedpagechange = true;
            float xLoc = (localeType == 1) ? 0 : 0.9;
            float xLoc2 = (localeType == 1) ? 0.9 : 0;
            TogglePage(prevpage, xLoc, 0.2, 0.1, 0.6);
            if (currentPageID == maxPageID) TogglePage(nextpage, xLoc2, 0.2, 0, 0.6);
            DWORD dd;
            HANDLE thumbnailThread = CreateThread(0, 0, LoadOtherPageThumbnail, NULL, 0, &dd);
        }
        else {
            UIContainer->SetAnimation(NULL);
            short animSrc = (localeType == 1) ? -1 : 1;
            UIContainer->SetX((dimensions.right - dimensions.left) * animSrc);
            UIContainer->SetAnimation(savedanim);
            UIContainer->SetX(0);
        }
        prevpageMain->SetVisible(true);
        if (currentPageID == maxPageID) nextpageMain->SetVisible(false);
    }
}

unsigned long ApplyThumbnailIcons(LPVOID lpParam) {
    Sleep(150);
    PostMessageW(wnd->GetHWND(), WM_USER + 14, NULL, NULL);
    return 0;
}

unsigned long CreateIndividualThumbnail(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
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
    if (pm[yV->y]->GetDirState() == true && treatdirasgroup == true) {
        int x = padding * flScaleFactor, y = padding * flScaleFactor;
        vector<ThumbIcons> strs;
        unsigned short count = 0;
        wstring folderPath = RemoveQuotes(pm[yV->y]->GetFilename());
        EnumerateFolder((LPWSTR)folderPath.c_str(), nullptr, false, true, &count, nullptr, 4);
        EnumerateFolderForThumbnails((LPWSTR)folderPath.c_str(), &strs, 4);
        for (int thumbs = 0; thumbs < count; thumbs++) {
            HBITMAP thumbIcon = GetShellItemImage((strs[thumbs].GetFilename()).c_str(), globalgpiconsz, globalgpiconsz);
            if (isColorized && strs[thumbs].GetColorLock() == false) {
                IterateBitmap(thumbIcon, StandardBitmapPixelHandler, 1, 0, 1);
            }
            int xRender = (localeType == 1) ? (globaliconsz - globalgpiconsz) * flScaleFactor - x : x;
            x += ((globalgpiconsz + paddingInner) * flScaleFactor);
            ThumbnailIcon* ti = new ThumbnailIcon{ xRender, y, strs[thumbs], thumbIcon };
            if (x > (globaliconsz - globalgpiconsz) * flScaleFactor) {
                x = padding * flScaleFactor;
                y += ((globalgpiconsz + paddingInner) * flScaleFactor);
            }
            PostMessageW(wnd->GetHWND(), WM_USER + 16, (WPARAM)ti, yV->y);
        }
    }
    free(yV);
    return 0;
}

void ApplyIcons(vector<LVItem*> pmLVItem, vector<DDScalableElement*> pmIcon, DesktopIcon* di, bool subdirectory, int id) {
    HICON icoShortcut = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(163), IMAGE_ICON, globalshiconsz * flScaleFactor, globalshiconsz * flScaleFactor, LR_SHARED);
    // The use of the 3 lines below is because we can't use a fully transparent bitmap
    static const HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"shell32.dll"), MAKEINTRESOURCE(24), IMAGE_ICON, 16, 16, LR_SHARED);
    HBITMAP dummyii = IconToBitmap(dummyi);
    IterateBitmap(dummyii, SimpleBitmapPixelHandler, 0, 0, 0.005);
    HBITMAP bmp{};
    if (id < pm.size()) {
        if (pm[id]->GetDirState() == false || treatdirasgroup == false || pmIcon != iconpm) bmp = GetShellItemImage(RemoveQuotes(pmLVItem[id]->GetFilename()).c_str(), globaliconsz, globaliconsz);
        else bmp = dummyii;
    }
    else if (treatdirasgroup == false || pmIcon != iconpm) bmp = GetShellItemImage(RemoveQuotes(pmLVItem[id]->GetFilename()).c_str(), globaliconsz, globaliconsz);
    else bmp = dummyii;
    if (bmp != dummyii) {
        HBITMAP bmpShadow = AddPaddingToBitmap(bmp, 8 * flScaleFactor);
        IterateBitmap(bmpShadow, SimpleBitmapPixelHandler, 0, (int)(4 * flScaleFactor), 0.33);
        di->iconshadow = bmpShadow;
    }
    HBITMAP bmpShortcut = IconToBitmap(icoShortcut);
    if (isColorized) {
        if (pmLVItem[id]->GetColorLock() == false) IterateBitmap(bmp, StandardBitmapPixelHandler, 1, 0, 1);
        IterateBitmap(bmpShortcut, StandardBitmapPixelHandler, 1, 0, 1);
    }
    IterateBitmap(bmpShortcut, UndoPremultiplication, 1, 0, 1);
    di->icon = bmp;
    di->iconshortcut = bmpShortcut;
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
    fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor, 0.9);
    SetWindowPos(subviewwnd->GetHWND(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    Element* groupdirectory{};
    parser->CreateElement(L"groupdirectory", NULL, NULL, NULL, (Element**)&groupdirectory);
    fullscreeninner->Add((Element**)&groupdirectory, 1);
    ScrollViewer* groupdirlist = (ScrollViewer*)groupdirectory->FindDescendent(StrToID(L"groupdirlist"));
    SubUIContainer = (RichText*)groupdirlist->FindDescendent(StrToID(L"SubUIContainer"));
    unsigned short count = 0;
    int count2{};
    EnumerateFolder((LPWSTR)filename, nullptr, false, true, &count);
    CubicBezier(32, px, py, 0.1, 0.9, 0.2, 1.0);
    if (count <= 128 && count > 0) {
        for (int i = 0; i < count; i++) {
            LVItem* outerElemGrouped;
            if (touchmode) {
                parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElemGrouped);
            }
            else parser->CreateElement(L"outerElemGrouped", NULL, NULL, NULL, (Element**)&outerElemGrouped);
            SubUIContainer->Add((Element**)&outerElemGrouped, 1);
            iconElem = (DDScalableElement*)outerElemGrouped->FindDescendent(StrToID(L"iconElem"));
            shortcutElem = (Element*)outerElemGrouped->FindDescendent(StrToID(L"shortcutElem"));
            iconElemShadow = (Element*)outerElemGrouped->FindDescendent(StrToID(L"iconElemShadow"));
            textElem = (RichText*)outerElemGrouped->FindDescendent(StrToID(L"textElem"));
            subpm.push_back(outerElemGrouped);
            subiconpm.push_back(iconElem);
            subshortpm.push_back(shortcutElem);
            subshadowpm.push_back(iconElemShadow);
            subfilepm.push_back(textElem);
            if (touchmode) {
                textElemShadow = (RichText*)outerElemGrouped->FindDescendent(StrToID(L"textElemShadow"));
                subfileshadowpm.push_back(textElemShadow);
            }
            outerElemGrouped->SetAnimation(NULL);
        }
        EnumerateFolder((LPWSTR)filename, &subpm, true, false, nullptr, &count2);
        int x = 0, y = 0;
        int maxX{}, xRuns{};
        Value* v;
        RECT dimensions;
        dimensions = *(groupdirectory->GetPadding(&v));
        int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
        int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 21) * flScaleFactor;
        if (touchmode) {
            outerSizeX = touchSizeX + 16 * flScaleFactor;
            outerSizeY = touchSizeY + 16 * flScaleFactor;
        }
        DWORD* animThread = new DWORD[count];
        DWORD* animThread2 = new DWORD[count];
        HANDLE* animThreadHandle = new HANDLE[count];
        HANDLE* animThreadHandle2 = new HANDLE[count];
        for (int j = 0; j < count; j++) {
            if (subpm[j]->GetHiddenState() == true) {
                subiconpm[j]->SetAlpha(128);
                subshadowpm[j]->SetAlpha(0);
                subfilepm[j]->SetAlpha(128);
            }
            assignFn(subpm[j], SelectSubItem);
            assignFn(subpm[j], SubItemRightClick);
            if (!touchmode) subpm[j]->SetClass(L"singleclicked");
            int xRender = (localeType == 1) ? (800 * flScaleFactor - (dimensions.left + dimensions.right + outerSizeX)) - x : x;
            subpm[j]->SetX(xRender), subpm[j]->SetY(y);
            yValue* yV = new yValue{ j };
            yValue* yV2 = new yValue{ j };
            x += outerSizeX;
            xRuns++;
            if (x > 800 * flScaleFactor - (dimensions.left + dimensions.right + outerSizeX)) {
                maxX = xRuns;
                xRuns = 0;
                x = 0;
                y += outerSizeY;
            }
            animThreadHandle[j] = CreateThread(0, 0, subanimate, (LPVOID)yV, 0, &(animThread[j]));
            animThreadHandle2[j] = CreateThread(0, 0, subfastin, (LPVOID)yV2, 0, &(animThread2[j]));
        }
        x -= outerSizeX;
        if (maxX != 0 && xRuns % maxX != 0) y += outerSizeY;
        SubUIContainer->SetHeight(y);
        Element* dirtitle = (Element*)groupdirectory->FindDescendent(StrToID(L"dirtitle"));
        for (int j = 0; j < count; j++) {
            if (localeType == 1 && y > (480 * flScaleFactor - (dirtitle->GetHeight() + dimensions.top + dimensions.bottom)))
                subpm[j]->SetX(subpm[j]->GetX() - GetSystemMetricsForDpi(SM_CXVSCROLL, dpi));
        }
        delete[] animThread;
        delete[] animThread2;
        delete[] animThreadHandle;
        delete[] animThreadHandle2;
        v->Release();
    }
    else {
        if (count > 128) {
            SubUIContainer->SetContentString(LoadStrFromRes(4030).c_str());
        }
        else SubUIContainer->SetContentString(LoadStrFromRes(4029).c_str());
    }
    dirnameanimator = (Element*)groupdirectory->FindDescendent(StrToID(L"dirnameanimator"));
    tasksanimator = (Element*)groupdirectory->FindDescendent(StrToID(L"tasksanimator"));
    DDScalableElement* dirname = (DDScalableElement*)groupdirectory->FindDescendent(StrToID(L"dirname"));
    dirname->SetContentString(simplefilename);
    dirname->SetAlpha(255);
    DDScalableElement* dirdetails = (DDScalableElement*)groupdirectory->FindDescendent(StrToID(L"dirdetails"));
    WCHAR itemCount[64];
    if (count == 1) StringCchPrintfW(itemCount, 64, LoadStrFromRes(4031).c_str());
    else StringCchPrintfW(itemCount, 64, LoadStrFromRes(4032).c_str(), count);
    dirdetails->SetContentString(itemCount);
    BYTE alphavalue = (count == 0) ? 1 : 160;
    dirdetails->SetAlpha(alphavalue);
    Element* tasks = (Element*)groupdirectory->FindDescendent(StrToID(L"tasks"));
    checkifelemexists = true;
    DWORD animThread3;
    DWORD animThread4;
    HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
    HANDLE animThreadHandle4 = CreateThread(0, 0, grouptasksanimation, NULL, 0, &animThread4);
    DDScalableButton* Customize = (DDScalableButton*)groupdirectory->FindDescendent(StrToID(L"Customize"));
    DDScalableButton* OpenInExplorer = (DDScalableButton*)groupdirectory->FindDescendent(StrToID(L"OpenInExplorer"));
    Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
    assignFn(OpenInExplorer, OpenGroupInExplorer);
    bufferOpenInExplorer = (wstring)filename;
}

void OpenDeskCpl(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) ShellExecuteW(NULL, L"open", L"control.exe", L"desk.cpl,Web,0", NULL, SW_SHOW);
}
void OpenLog(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        wchar_t* desktoplog = new wchar_t[260];
        wchar_t* cBuffer = new wchar_t[260];
        DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
        StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
        ShellExecuteW(NULL, L"open", L"notepad.exe", desktoplog, NULL, SW_SHOW);
        delete[] desktoplog;
        delete[] cBuffer;
    }
}
void ShowPage1(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        PageTab1->SetSelected(true);
        PageTab2->SetSelected(false);
        PageTab3->SetSelected(false);
        SubUIContainer->DestroyAll(true);
        Element* SettingsPage1;
        parser->CreateElement(L"SettingsPage1", NULL, NULL, NULL, (Element**)&SettingsPage1);
        SubUIContainer->Add((Element**)&SettingsPage1, 1);
        DDToggleButton* ItemCheckboxes = (DDToggleButton*)SettingsPage1->FindDescendent(StrToID(L"ItemCheckboxes"));
        DDToggleButton* ShowHiddenFiles = (DDToggleButton*)SettingsPage1->FindDescendent(StrToID(L"ShowHiddenFiles"));
        DDToggleButton* FilenameExts = (DDToggleButton*)SettingsPage1->FindDescendent(StrToID(L"FilenameExts"));
        DDToggleButton* TreatDirAsGroup = (DDToggleButton*)SettingsPage1->FindDescendent(StrToID(L"TreatDirAsGroup"));
        RegKeyValue rkvTemp{};
        rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
        rkvTemp._valueToFind = L"AutoCheckSelect";
        ItemCheckboxes->SetSelected(showcheckboxes);
        ItemCheckboxes->SetAssociatedBool(&showcheckboxes);
        ItemCheckboxes->SetRegKeyValue(rkvTemp);
        rkvTemp._valueToFind = L"Hidden";
        if (GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind) == 1) ShowHiddenFiles->SetSelected(true);
        else ShowHiddenFiles->SetSelected(false);
        ShowHiddenFiles->SetAssociatedFn(InitLayout);
        ShowHiddenFiles->SetRegKeyValue(rkvTemp);
        rkvTemp._valueToFind = L"HideFileExt";
        FilenameExts->SetSelected(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
        FilenameExts->SetAssociatedFn(InitLayout);
        FilenameExts->SetRegKeyValue(rkvTemp);
        rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"TreatDirAsGroup";
        TreatDirAsGroup->SetSelected(treatdirasgroup);
        TreatDirAsGroup->SetAssociatedBool(&treatdirasgroup);
        TreatDirAsGroup->SetAssociatedFn(RearrangeIcons);
        TreatDirAsGroup->SetRegKeyValue(rkvTemp);
        assignFn(ItemCheckboxes, ToggleSetting);
        assignFn(ShowHiddenFiles, ToggleSetting);
        assignFn(FilenameExts, ToggleSetting);
        assignFn(TreatDirAsGroup, ToggleSetting);
    }
}
void ShowPage2(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        PageTab1->SetSelected(false);
        PageTab2->SetSelected(true);
        PageTab3->SetSelected(false);
        SubUIContainer->DestroyAll(true);
        Element* SettingsPage2;
        parser->CreateElement(L"SettingsPage2", NULL, NULL, NULL, (Element**)&SettingsPage2);
        SubUIContainer->Add((Element**)&SettingsPage2, 1);
        DDToggleButton* EnableAccent = (DDToggleButton*)SettingsPage2->FindDescendent(StrToID(L"EnableAccent"));
        DDToggleButton* IconThumbnails = (DDToggleButton*)SettingsPage2->FindDescendent(StrToID(L"IconThumbnails"));
        DDScalableButton* DesktopIconSettings = (DDScalableButton*)SettingsPage2->FindDescendent(StrToID(L"DesktopIconSettings"));
        RegKeyValue rkvTemp{};
        rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"AccentColorIcons";
        EnableAccent->SetSelected(isColorized);
        EnableAccent->SetAssociatedBool(&isColorized);
        EnableAccent->SetAssociatedFn(RearrangeIcons);
        EnableAccent->SetRegKeyValue(rkvTemp);
        rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", rkvTemp._valueToFind = L"IconsOnly";
        IconThumbnails->SetSelected(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
        IconThumbnails->SetAssociatedFn(RearrangeIcons);
        IconThumbnails->SetRegKeyValue(rkvTemp);
        assignFn(EnableAccent, ToggleSetting);
        assignFn(IconThumbnails, ToggleSetting);
        assignFn(DesktopIconSettings, OpenDeskCpl);
    }
}
void ShowPage3(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        PageTab1->SetSelected(false);
        PageTab2->SetSelected(false);
        PageTab3->SetSelected(true);
        SubUIContainer->DestroyAll(true);
        Element* SettingsPage3;
        parser->CreateElement(L"SettingsPage3", NULL, NULL, NULL, (Element**)&SettingsPage3);
        SubUIContainer->Add((Element**)&SettingsPage3, 1);
        DDToggleButton* EnableLogging = (DDToggleButton*)SettingsPage3->FindDescendent(StrToID(L"EnableLogging"));
        DDScalableButton* ViewLastLog = (DDScalableButton*)SettingsPage3->FindDescendent(StrToID(L"ViewLastLog"));
        RegKeyValue rkvTemp{};
        rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"Logging";
        EnableLogging->SetSelected(7 - GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
        EnableLogging->SetRegKeyValue(rkvTemp);
        assignFn(EnableLogging, ToggleSetting);
        assignFn(ViewLastLog, OpenLog);
    }
}
void ShowSettings(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        subviewwnd->ShowWindow(SW_HIDE);
        mainContainer->SetVisible(true);
        centered->DestroyAll(true);
        ShowPopupCore();
        SimpleViewTop->SetLayoutPos(-3);
        SimpleViewBottom->SetLayoutPos(-3);
        nextpage->SetWidth(0);
        prevpage->SetWidth(0);
        editmode = false;
        issubviewopen = 1;
        Element* settingsview{};
        parser->CreateElement(L"settingsview", NULL, NULL, NULL, (Element**)&settingsview);
        fullscreeninner->Add((Element**)&settingsview, 1);
        ScrollViewer* settingslist = (ScrollViewer*)settingsview->FindDescendent(StrToID(L"settingslist"));
        SubUIContainer = (RichText*)settingsview->FindDescendent(StrToID(L"SubUIContainer"));
        PageTab1 = (DDScalableButton*)settingsview->FindDescendent(StrToID(L"PageTab1"));
        PageTab2 = (DDScalableButton*)settingsview->FindDescendent(StrToID(L"PageTab2"));
        PageTab3 = (DDScalableButton*)settingsview->FindDescendent(StrToID(L"PageTab3"));
        assignFn(PageTab1, ShowPage1);
        assignFn(PageTab2, ShowPage2);
        assignFn(PageTab3, ShowPage3);
        ShowPage1(elem, iev);
        CubicBezier(32, px, py, 0.1, 0.9, 0.2, 1.0);
        dirnameanimator = (Element*)settingsview->FindDescendent(StrToID(L"dirnameanimator"));
        DDScalableElement* name = (DDScalableElement*)settingsview->FindDescendent(StrToID(L"name"));
        name->SetAlpha(255);
        checkifelemexists = true;
        DWORD animThread3;
        HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
    }
}
void ExitWindow(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
        SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, NULL);
    }
}

Element* elemStorage;
bool fileopened{};
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
        if (elem != emptyspace && checkbox->GetMouseFocused() == false && GetAsyncKeyState(VK_CONTROL) == 0) elem->SetSelected(!elem->GetSelected());
        if (validation & 1) {
            if (elem != emptyspace && checkbox->GetMouseFocused() == true && GetAsyncKeyState(VK_CONTROL) == 0) elem->SetSelected(!elem->GetSelected());
        }
        if (showcheckboxes == 1) checkbox->SetVisible(true);
        if (shellstate[4] & 0x20 && !touchmode) {
            if (elem == elemStorage) clicks++; else clicks = 0;
            DWORD doubleClickThread{};
            HANDLE doubleClickThreadHandle = CreateThread(0, 0, DoubleClickHandler, NULL, 0, &doubleClickThread);
            elemStorage = elem;
        }
        for (int items = 0; items < validItems; items++) {
            pm[items]->SetMemorySelected(pm[items]->GetSelected());
        }
        if (clicks % 2 == 1 && checkbox->GetMouseFocused() == false && ((LVItem*)elem)->GetDragState() == false) {
            for (int items = 0; items < pm.size(); items++) {
                if (pm[items] == elem) {
                    wstring temp = RemoveQuotes(pm[items]->GetFilename());
                    SHELLEXECUTEINFOW execInfo = {};
                    execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
                    execInfo.lpVerb = L"open";
                    execInfo.nShow = SW_SHOWNORMAL;
                    execInfo.lpFile = temp.c_str();
                    fileopened = true;
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
void SelectItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
    if (!touchmode && pProp == Element::SelectedProp()) {
        float spacingInternal = CalcTextLines(((LVItem*)elem)->GetSimpleFilename().c_str(), ((LVItem*)elem)->GetWidth());
        int extraBottomSpacing = (((LVItem*)elem)->GetSelected() == true) ? ceil(spacingInternal) * textm.tmHeight : floor(spacingInternal) * textm.tmHeight;
        textElem = (RichText*)((LVItem*)elem)->FindDescendent(StrToID(L"textElem"));
        textElemShadow = (RichText*)((LVItem*)elem)->FindDescendent(StrToID(L"textElemShadow"));
        if (spacingInternal == 1.5) {
            if (((LVItem*)elem)->GetSelected() == true) ((LVItem*)elem)->SetHeight(((LVItem*)elem)->GetHeight() + extraBottomSpacing * 0.5);
            else ((LVItem*)elem)->SetHeight(((LVItem*)elem)->GetHeight() - extraBottomSpacing);
        }
        textElem->SetHeight(extraBottomSpacing + 4 * flScaleFactor);
        textElemShadow->SetHeight(extraBottomSpacing + 5 * flScaleFactor);
        HBITMAP capturedBitmap = CreateTextBitmap(((LVItem*)elem)->GetSimpleFilename().c_str(), ((LVItem*)elem)->GetWidth() - 4 * flScaleFactor, extraBottomSpacing, DT_CENTER | DT_END_ELLIPSIS, false);
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
}

void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
    Button* checkboxElem = (Button*)elem->FindDescendent(StrToID(L"checkboxElem")); 
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
        Element* grandparent = parent->GetParent();
        Value* v = elem->GetValue(Element::MouseFocusedProp, 1, &u);
        Element* item = grandparent->FindDescendent(StrToID(L"innerElem"));
        if (item != nullptr) item->SetValue(Element::MouseFocusedProp(), 1, v);
    }
}

bool isPressed = 0, isIconPressed = 0;
unsigned long UpdateMarqueeSelectorPosition(LPVOID lpParam) {
    while (true) {
        if (!isPressed) break;
        Sleep(10);
        SendMessageW(wnd->GetHWND(), WM_USER + 5, NULL, NULL);
    }
    return 0;
}
unsigned long UpdateIconPosition(LPVOID lpParam) {
    if (fileopened) return 0;
    POINT ppt;
    GetCursorPos(&ppt);
    while (true) {
        Sleep(10);
        SendMessageW(wnd->GetHWND(), WM_USER + 17, (WPARAM)((vector<LVItem*>*)lpParam), (LPARAM)&ppt);
        if (!isIconPressed) {
            SendMessageW(wnd->GetHWND(), WM_USER + 18, (WPARAM)((vector<LVItem*>*)lpParam), 0);
            Sleep(100);
            SendMessageW(wnd->GetHWND(), WM_USER + 18, (WPARAM)((vector<LVItem*>*)lpParam), 1);
            break;
        }
    }
    return 0;
}

void MarqueeSelector(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pv2) {
    DWORD marqueeThread;
    HANDLE marqueeThreadHandle;
    HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, 16, 16, LR_SHARED);
    HBITMAP selectorBmp = IconToBitmap(dummyi);
    if (pProp == Button::CapturedProp()) {
        if (!isPressed) {
            emptyspace->SetLayoutPos(-3);
            POINT ppt;
            GetCursorPos(&ppt);
            ScreenToClient(wnd->GetHWND(), &ppt);
            RECT dimensions{};
            GetClientRect(wnd->GetHWND(), &dimensions);
            if (localeType == 1) origX = dimensions.right - ppt.x;
            else origX = ppt.x;
            origY = ppt.y;
            selector->SetX(origX);
            selector->SetY(origY);
            selector->SetSelected(true);
            selector->SetVisible(true);
            selector->SetLayoutPos(-2);
            IterateBitmap(selectorBmp, SimpleBitmapPixelHandler, 3, 0, 0.33);
            Value* selectorBmpV = DirectUI::Value::CreateGraphic(selectorBmp, 7, 0xffffffff, false, false, false);
            selector2->SetValue(Element::BackgroundProp, 1, selectorBmpV);
            selector2->SetSelected(true);
            selector2->SetVisible(true);
            selector2->SetLayoutPos(-2);
            marqueeThreadHandle = CreateThread(0, 0, UpdateMarqueeSelectorPosition, NULL, 0, &marqueeThread);
            selectorBmpV->Release();
        }
        isPressed = 1;
    }
    else if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        if (isPressed) {
            selector->SetVisible(false);
            selector->SetLayoutPos(-3);
            selector2->SetVisible(false);
            selector2->SetLayoutPos(-3);
            isPressed = 0;
            emptyspace->SetLayoutPos(4);
        }
    }
}
void ItemDragListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
    DWORD dragThread;
    HANDLE dragThreadHandle;
    POINT ppt;
    if (pProp == Button::CapturedProp()) {
        if (!isIconPressed) {
            selectedLVItems.clear();
            selectedLVItems.push_back((LVItem*)elem);
            int selectedItems{};
            if (elem->GetSelected() == false && GetAsyncKeyState(VK_CONTROL) == 0) {
                for (int items = 0; items < validItems; items++) {
                    pm[items]->SetSelected(false);
                    if (cbpm[items]->GetSelected() == false && showcheckboxes == 1) cbpm[items]->SetVisible(false);
                }
            }
            elem->SetSelected(true);
            for (int items = 0; items < validItems; items++) {
                if (pm[items]->GetSelected() == true) {
                    selectedItems++;
                    if (pm[items] != elem) selectedLVItems.push_back(pm[items]);
                }
            }
            Element* multipleitems = regElem(L"multipleitems", pMain);
            multipleitems->SetVisible(false);
            if (selectedItems >= 2) {
                multipleitems->SetVisible(true);
                multipleitems->SetContentString(to_wstring(selectedItems).c_str());
            }
            if (showcheckboxes) {
                Button* checkbox = (Button*)elem->FindDescendent(StrToID(L"checkboxElem"));
                checkbox->SetVisible(true);
            }
            fileopened = false;
            GetCursorPos(&ppt);
            ScreenToClient(wnd->GetHWND(), &ppt);
            RECT dimensions{};
            GetClientRect(wnd->GetHWND(), &dimensions);
            if (localeType == 1) origX = dimensions.right - ppt.x - elem->GetX();
            else origX = ppt.x - elem->GetX();
            origY = ppt.y - elem->GetY();
            Value* bitmap{};
            HBITMAP hbmCapture{};
            HDC hdcWindow = GetDC(wnd->GetHWND());
            HDC hdcMem = CreateCompatibleDC(hdcWindow);
            hbmCapture = CreateCompatibleBitmap(hdcWindow, elem->GetWidth(), elem->GetHeight());
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
            int iconX = (localeType == 1) ? dimensions.right - elem->GetX() - elem->GetWidth() : elem->GetX();
            BitBlt(hdcMem, 0, 0, elem->GetWidth(), elem->GetHeight(), hdcWindow, iconX, elem->GetY(), SRCCOPY);
            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);
            ReleaseDC(wnd->GetHWND(), hdcWindow);
            IterateBitmap(hbmCapture, UndoPremultiplication, 1, 0, 1);
            bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 7, 0xffffffff, false, false, false);
            if (bitmap != nullptr) {
                dragpreview->SetValue(Element::BackgroundProp, 1, bitmap);
                bitmap->Release();
            }
            if (hbmCapture != nullptr) DeleteObject(hbmCapture);
            dragpreview->SetWidth(elem->GetWidth());
            dragpreview->SetHeight(elem->GetHeight());
            dragThreadHandle = CreateThread(0, 0, UpdateIconPosition, &selectedLVItems, 0, &dragThread);
        }
        isIconPressed = 1;
    }
    else if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        if (isIconPressed) isIconPressed = 0;
    }
}

void testEventListener3(Element* elem, Event* iev) {
    if (iev->uidType == Button::Click) {
        switch (pSubview->GetAccessible()) {
        case false:
            if (elem != fullscreenpopupbase) {
                ShowPopupCore();
            }
            break;
        case true:
            if (centered->GetMouseWithin() == false && elem->GetMouseFocused() == true) {
                HidePopupCore(false);
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
    listviewAnimStorage = savedanim;
    if (reloadicons) {
        DWORD dd;
        HANDLE thumbnailThread = CreateThread(0, 0, ApplyThumbnailIcons, NULL, 0, &dd);
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 2 of 5 complete: Applied icons to the relevant desktop items.");
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    int desktoppadding = touchmode ? DESKPADDING_TOUCH : DESKPADDING_NORMAL;
    desktoppadding *= flScaleFactor;
    int x = desktoppadding, y = desktoppadding;
    DWORD* animThread = new DWORD[count];
    DWORD* animThread2 = new DWORD[count];
    HANDLE* animThreadHandle = new HANDLE[count];
    HANDLE* animThreadHandle2 = new HANDLE[count];
    int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
    int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 22) * flScaleFactor;
    int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
    int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor - textm.tmHeight;
    if (touchmode) {
        outerSizeX = touchSizeX + desktoppadding;
        outerSizeY = touchSizeY + desktoppadding;
        innerSizeX = touchSizeX;
        innerSizeY = touchSizeY;
    }
    int largestXPos = dimensions.right / outerSizeX;
    int largestYPos = dimensions.bottom / outerSizeY;
    vector<bool> positions{};
    positions.resize(largestXPos * largestYPos - 1);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 3 of 5 complete: Created an array of positions.");
    for (int j = 0; j < validItems; j++) {
        if (!animation) pm[j]->SetAnimation(NULL); else pm[j]->SetAnimation(savedanim);
        if (pm[j]->GetInternalXPos() < largestXPos && pm[j]->GetInternalYPos() < largestYPos && positions[pm[j]->GetInternalYPos() + pm[j]->GetInternalXPos() * largestYPos] == false) {
            int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - outerSizeX : pm[j]->GetInternalXPos() * outerSizeX + x;
            pm[j]->SetX(xRender);
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
        int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - outerSizeX : pm[j]->GetInternalXPos() * outerSizeX + x;
        pm[j]->SetX(xRender);
        pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
    }
    if (currentPageID > maxPageID) currentPageID = maxPageID;
    if (currentPageID != 1) prevpageMain->SetVisible(true);
    for (int j = 0; j < validItems; j++) {
        yValue* yV = new yValue{ j, innerSizeX, innerSizeY };
        yValue* yV2 = new yValue{ j, innerSizeX, innerSizeY };
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

void InitLayout(bool cloaked, bool bUnused2) {
    UIContainer->DestroyAll(true);
    UIContainer->SetVisible(!cloaked);
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

    parser->CreateElement(L"emptyspace", NULL, NULL, NULL, (Element**)&emptyspace);
    UIContainer->Add((Element**)&emptyspace, 1);
    assignFn(emptyspace, SelectItem);
    assignExtendedFn(emptyspace, ShowCheckboxIfNeeded);
    assignExtendedFn(emptyspace, MarqueeSelector);
    assignFn(emptyspace, DesktopRightClick);
    for (int i = 0; i < count; i++) {
        LVItem* outerElem;
        if (touchmode) {
            parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElem);
        }
        else parser->CreateElement(L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
        UIContainer->Add((Element**)&outerElem, 1);
        iconElem = (DDScalableElement*)outerElem->FindDescendent(StrToID(L"iconElem"));
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
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 4 of 6 complete: Created arrays according to your desktop items.");
    for (int i = 0; i < count; i++) {
        if (pm[i]->GetHiddenState() == true) {
            iconpm[i]->SetAlpha(128);
            shadowpm[i]->SetAlpha(0);
            filepm[i]->SetAlpha(192);
            fileshadowpm[i]->SetAlpha(128);
        }
        assignFn(pm[i], SelectItem);
        assignFn(pm[i], ItemRightClick);
        assignExtendedFn(pm[i], SelectItemListener);
        assignExtendedFn(pm[i], ShowCheckboxIfNeeded);
        assignExtendedFn(pm[i], ItemDragListener);
        assignExtendedFn(cbpm[i], CheckboxHandler);
        if (!touchmode) {
            if (shellstate[4] & 0x20) {
                pm[i]->SetClass(L"doubleclicked");
                //if (pm[i].isDirectory == true && treatdirasgroup == true) outerElem->SetClass(L"singleclicked");
            }
            else pm[i]->SetClass(L"singleclicked");
        }
    }
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 5 of 6 complete: Filled the arrays with relevant desktop icon data.");
    RearrangeIcons(false, true);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 6 of 6 complete: Arranged the icons according to your icon placements.");
    delete[] cBuffer;
    delete[] secondaryPath;
}

unsigned long FinishedLogging(LPVOID lpParam) {
    int logresponse{};
    TaskDialog(NULL, NULL, LoadStrFromRes(4024).c_str(), LoadStrFromRes(4019).c_str(), LoadStrFromRes(4020).c_str(), TDCBF_OK_BUTTON | TDCBF_CLOSE_BUTTON, TD_INFORMATION_ICON, &logresponse);
    if (logresponse == IDCLOSE) SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, 420);
    return 0;
}

HHOOK KeyHook = nullptr;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    static bool keyHold[256]{};
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
        if ((pKeyInfo->vkCode == 'D' || pKeyInfo->vkCode == 'M') && GetAsyncKeyState(VK_LWIN) & 0x8000) {
            HidePopupCore(true);
            SetWindowPos(hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        if ((pKeyInfo->vkCode == VK_F4) && GetAsyncKeyState(VK_MENU) & 0x8000) {
            static int isdialogopen = IDCLOSE;
            if (isdialogopen == IDCLOSE) {
                isdialogopen = 0;
                TaskDialog(wnd->GetHWND(), GetModuleHandleW(L"imageres.dll"), L"Shut Down Windows", L"Shutdown dialog stub", NULL, TDCBF_CLOSE_BUTTON, MAKEINTRESOURCE(101), &isdialogopen);
            }
            return 1;
        }
        if (pKeyInfo->vkCode == VK_F5) {
            if (!keyHold[pKeyInfo->vkCode]) {
                InitLayout(false, false);
                keyHold[pKeyInfo->vkCode] = true;
            }
        }
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            keyHold[pKeyInfo->vkCode] = false;
        }
    }
    return CallNextHookEx(KeyHook, nCode, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    hMutex = CreateMutex(NULL, TRUE, szWindowClass);
    if (!hMutex || ERROR_ALREADY_EXISTS == GetLastError()) {
        TaskDialog(NULL, GetModuleHandleW(NULL), LoadStrFromRes(4025).c_str(), NULL,
            LoadStrFromRes(4021).c_str(), TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
        return 1;
    }
    if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"FFlags") & 0x4);
    else {
        TaskDialog(NULL, GetModuleHandleW(NULL), L"DirectDesktop", LoadStrFromRes(4022).c_str(),
            LoadStrFromRes(4023).c_str(), TDCBF_CLOSE_BUTTON, TD_WARNING_ICON, NULL);
        return 1;
    }
    InitProcessPriv(14, NULL, true, true, true);
    InitThread(TSM_IMMERSIVE);
    RegisterAllControls();
    LVItem::Register();
    DDScalableElement::Register();
    DDScalableButton::Register();
    DDToggleButton::Register();
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    WCHAR localeName[256]{};
    ULONG numLanguages{};
    ULONG bufferSize = sizeof(localeName) / sizeof(WCHAR);
    GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, localeName, &bufferSize);
    GetLocaleInfoEx(localeName, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER, (LPWSTR)&localeType, sizeof(localeType) / sizeof(WCHAR));
    RECT dimensions;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 2) * 2;
    int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CYEDGE, dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
    bool checklog{};
    SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging", 0, true, &checklog);
    if (checklog) {
        TaskDialog(NULL, GetModuleHandleW(NULL), L"DirectDesktop", LoadStrFromRes(4017).c_str(),
            LoadStrFromRes(4018).c_str(), TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &logging);
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging", logging, false, nullptr);
    }
    else logging = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging");
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
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Found the Program Manager window.");
        hSHELLDLL_DefView = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
        if (logging == IDYES && hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: Found a SHELLDLL_DefView window.");
        if (WindowsBuild >= 26002 && logging == IDYES) MainLogger.WriteLine(L"Information: Version is 24H2, skipping WorkerW creation!!!");
        SendMessageTimeoutW(hWndProgman, 0x052C, 0, 0, SMTO_NORMAL, 250, NULL);
        Sleep(250);
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
    KeyHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(NULL), 0);
    NativeHWNDHost::Create(L"DirectDesktop", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, NULL, NULL, 0, &wnd);
    NativeHWNDHost::Create(L"DirectDesktop Subview", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, WS_EX_TOOLWINDOW, WS_POPUP, 0, &subviewwnd);
    DUIXmlParser::Create(&parser, NULL, NULL, NULL, NULL);
    parser->SetXMLFromResource(IDR_UIFILE2, hInstance, hInstance);
    HWNDElement::Create(wnd->GetHWND(), true, NULL, NULL, &key, (Element**)&parent);
    HWNDElement::Create(subviewwnd->GetHWND(), true, NULL, NULL, &key2, (Element**)&subviewparent);
    RegisterHotKey(wnd->GetHWND(), 1, 0x0002 | MOD_ALT, 'E');
    SetWindowLongPtrW(wnd->GetHWND(), GWL_STYLE, 0x56003A40L);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_EXSTYLE, 0xC0000800L);
    WndProc = (WNDPROC)SetWindowLongPtrW(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);
    WndProc2 = (WNDPROC)SetWindowLongPtrW(subviewwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)TopLevelWindowProc);
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

    LVItem* outerElemTouch;
    parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElemTouch);
    touchSizeX = outerElemTouch->GetWidth();
    touchSizeY = outerElemTouch->GetHeight();

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
    SetTheme();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Set the theme successfully.");

    sampleText = regElem(L"sampleText", pMain);
    mainContainer = regElem(L"mainContainer", pMain);
    UIContainer = regElem(L"UIContainer", pMain);
    fullscreenpopupbase = regBtn(L"fullscreenpopupbase", pSubview);
    centered = regBtn(L"centered", pSubview);
    selector = regElem(L"selector", pMain);
    selector2 = regElem(L"selector2", pMain);
    SimpleViewTop = regBtn(L"SimpleViewTop", pSubview);
    SimpleViewBottom = regBtn(L"SimpleViewBottom", pSubview);
    SimpleViewSettings = regBtn(L"SimpleViewSettings", pSubview);
    SimpleViewClose = regBtn(L"SimpleViewClose", pSubview);
    prevpage = regTouchBtn(L"prevpage", pSubview);
    nextpage = regTouchBtn(L"nextpage", pSubview);
    prevpageMain = regTouchBtn(L"prevpageMain", pMain);
    nextpageMain = regTouchBtn(L"nextpageMain", pMain);
    pageinfo = regRichText(L"pageinfo", pSubview);
    dragpreview = regElem(L"dragpreview", pMain);

    assignFn(fullscreenpopupbase, testEventListener3);
    assignFn(SimpleViewTop, testEventListener3);
    assignFn(SimpleViewBottom, testEventListener3);
    assignFn(SimpleViewSettings, ShowSettings);
    assignFn(SimpleViewClose, ExitWindow);
    assignFn(prevpage, GoToPrevPage);
    assignFn(nextpage, GoToNextPage);
    assignFn(prevpageMain, GoToPrevPage);
    assignFn(nextpageMain, GoToNextPage);

    AdjustWindowSizes(true);
    showcheckboxes = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect");
    hiddenIcons = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons");
    globaliconsz = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize");
    shellstate = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState");
    SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"TreatDirAsGroup", 0, true, nullptr);
    SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"AccentColorIcons", 0, true, nullptr);
    SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"TouchView", 0, true, nullptr);
    treatdirasgroup = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"TreatDirAsGroup");
    isColorized = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"AccentColorIcons");
    touchmode = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"TouchView");
    if (touchmode) globaliconsz = 32;
    if (globaliconsz > 48) globalshiconsz = 48; else globalshiconsz = 32;
    globalgpiconsz = 12;
    if (globaliconsz > 96) globalgpiconsz = 48;
    else if (globaliconsz > 48) globalgpiconsz = 32;
    else if (globaliconsz > 32) globalgpiconsz = 16;
    InitLayout(false, false);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialized layout successfully.");
    
    wnd->Host(pMain);
    subviewwnd->Host(pSubview);
    wnd->ShowWindow(SW_SHOW);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been created and shown.");
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(wnd->GetHWND(), &m);
    DwmExtendFrameIntoClientArea(subviewwnd->GetHWND(), &m);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been made transparent.\n\nLogging is now complete.");
    if (logging == IDYES) {
        DWORD dd;
        HANDLE loggingThread = CreateThread(0, 0, FinishedLogging, NULL, 0, &dd);
    }
    logging = IDNO;
    StartMessagePump();
    UnInitProcessPriv(0);
    CoUninitialize();
    if (KeyHook) {
        UnhookWindowsHookEx(KeyHook);
        KeyHook = nullptr;
    }

    return 0;
}