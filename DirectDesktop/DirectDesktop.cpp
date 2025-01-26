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
#include "StyleModifier.h"
#include "BitmapHelper.h"


using namespace DirectUI;
using namespace std;

NativeHWNDHost* wnd;
HWNDElement* parent;
DUIXmlParser* parser;
Element* pMain;
unsigned long key = 0;

Button* testButton;
Button* testButton2;
Button* testButton3;
Button* testButton4;
Button* testButton5;
Element* sampleText;
Element* mainContainer;
Element* UIContainer;
Element* iconElem;
Element* shortcutElem;
Element* iconElemShadow;
RichText* textElem;
Element* fullscreenpopup, *fullscreenpopupbase, *fullscreeninner;
Element* itemcountstatus;
Button* emptyspace;
HRESULT err;

int popupframe;
vector<int> frame;

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
Edit* regEdit(const wchar_t* editName) {
    Edit* result = (Edit*)pMain->FindDescendent(StrToID((UCString)editName));
    return result;
}
void assignFn(Element* btnName, void(*fnName)(Element* elem, Event* iev)) {
    btnName->AddListener(new EventListener(fnName));
}

struct parameters {
    Element* elem{};
    int x{};
    int y{};
};

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

void CubicBezier(const int frames, double px[], double py[], double x0, double y0, double x1, double y1) {
    for (int c = 0; c < frames; c++) {
        double t = (1.0 / frames) * c;
        px[c] = (3 * t * pow(1 - t, 2) * x0) + (3 * pow(t, 2) * (1 - t) * x1) + pow(t, 3);
        py[c] = (3 * t * pow(1 - t, 2) * y0) + (3 * pow(t, 2) * (1 - t) * y1) + pow(t, 3);
    }
    px[frames - 1] = 1;
    py[frames - 1] = 1;
}

WNDPROC WndProc;
vector<parameters> pm;
vector<parameters> iconpm;
vector<parameters> shortpm;
vector<parameters> shadowpm;
vector<parameters> filepm;
int smIndex = 0, shortIndex = 0;

wstring hideExt(const wstring& filename, bool isEnabled) {
    if (isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) lastdot = filename.find_last_of(L".");
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        shortIndex++;
        if (lastdot == wstring::npos) return filename;
        return filename.substr(0, lastdot);
    }
    if (!isEnabled) {
        size_t lastdot = filename.find(L".lnk");
        if (lastdot == wstring::npos) lastdot = filename.find(L".pif");
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
        if (lastdot == wstring::npos) {
            shortIndex++;
            return filename;
        }
        else {
            shortpm[shortIndex++].x = 1;
            return filename.substr(0, lastdot);
        }
    }
}

vector<wstring> list_directory() {
    static int isFileHiddenEnabled;
    static int isFileSuperHiddenEnabled;
    static int isFileExtHidden;
    LPCWSTR path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    HKEY hKey;
    DWORD lResult = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    DWORD lResult2 = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    DWORD lResult3 = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwSize = NULL;
        DWORD dwSize2 = NULL;
        DWORD dwSize3 = NULL;
        lResult = RegGetValue(hKey, NULL, L"Hidden", RRF_RT_DWORD, NULL, NULL, &dwSize);
        lResult2 = RegGetValue(hKey, NULL, L"ShowSuperHidden", RRF_RT_DWORD, NULL, NULL, &dwSize2);
        lResult3 = RegGetValue(hKey, NULL, L"HideFileExt", RRF_RT_DWORD, NULL, NULL, &dwSize3);
        if (lResult == ERROR_SUCCESS && dwSize != NULL)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValue(hKey, NULL, L"Hidden", RRF_RT_DWORD, NULL, dwValue, &dwSize);
            isFileHiddenEnabled = *dwValue;
            free(dwValue);
        }
        if (lResult == ERROR_SUCCESS && dwSize != NULL)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValue(hKey, NULL, L"ShowSuperHidden", RRF_RT_DWORD, NULL, dwValue, &dwSize);
            isFileSuperHiddenEnabled = *dwValue;
            free(dwValue);
        }
        if (lResult == ERROR_SUCCESS && dwSize != NULL)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValue(hKey, NULL, L"HideFileExt", RRF_RT_DWORD, NULL, dwValue, &dwSize);
            isFileExtHidden = *dwValue;
            free(dwValue);
        }
        RegCloseKey(hKey);
    }
    WIN32_FIND_DATAW findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    wchar_t full_path[260];
    wchar_t full_path2[260];
    wchar_t full_path3[260];
    DWORD d = GetEnvironmentVariableW(L"userprofile", 0, 0);
    DWORD d2 = GetEnvironmentVariableW(L"PUBLIC", 0, 0);
    vector<wchar_t> envName(d);
    vector<wchar_t> envName2(d2);
    GetEnvironmentVariableW(L"userprofile", envName.data(), 260);
    GetEnvironmentVariableW(L"PUBLIC", envName2.data(), 260);
    StringCchPrintfW(full_path, 260, L"%s\\Desktop\\*", envName.data());
    StringCchPrintfW(full_path2, 260, L"%s\\Desktop\\*", envName2.data());
    StringCchPrintfW(full_path3, 260, L"%s\\OneDrive\\Desktop\\*", envName.data());
    vector<wstring> dir_list;

    int runs = 0;
    hFind = FindFirstFileW(full_path, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            shortpm.push_back({NULL, NULL, NULL});
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            shortIndex--;
            shortpm.pop_back();
            dir_list.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                shortIndex--;
                shortpm.pop_back();
                dir_list.pop_back();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path2, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            shortpm.push_back({ NULL, NULL, NULL });
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            shortIndex--;
            shortpm.pop_back();
            dir_list.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                shortIndex--;
                shortpm.pop_back();
                dir_list.pop_back();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path3, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            shortpm.push_back({ NULL, NULL, NULL });
            dir_list.push_back(hideExt(wstring(findData.cFileName), isFileExtHidden));
        }
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            shortIndex--;
            shortpm.pop_back();
            dir_list.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                shortIndex--;
                shortpm.pop_back();
                dir_list.pop_back();
                continue;
            }
        }
    }

    FindClose(hFind);
    return dir_list;
}

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT dimensions;
    GetClientRect(wnd->GetHWND(), &dimensions);
    int innerSizeX = GetSystemMetrics(SM_CXICONSPACING);
    int innerSizeY = GetSystemMetrics(SM_CYICONSPACING) + 20;
    int iconPadding = (innerSizeX - 48) / 2;
    Event evt;
    evt.type == Button::Click;
    switch (uMsg) {
    case WM_DWMCOLORIZATIONCOLORCHANGED: {
        UpdateModeInfo();
        break;
    }
    case WM_COMMAND: {
        break;
    }
    case WM_USER + 1: {
            pm[wParam].elem->SetAlpha(255);
            pm[wParam].elem->SetVisible(true);
        break;
    }
    case WM_USER + 3: {
            double bezierProgress = py[frame[wParam] - 1];
            pm[wParam].elem->SetWidth(innerSizeX * (0.7 + 0.3 * bezierProgress));
            pm[wParam].elem->SetHeight(innerSizeY * (0.7 + 0.3 * bezierProgress));
            pm[wParam].elem->SetX(round((dimensions.right - 2 * pm[wParam].x) * 0.15 * (1 - bezierProgress) + pm[wParam].x));
            pm[wParam].elem->SetY(round((dimensions.bottom - 2 * pm[wParam].y) * 0.15 * (1 - bezierProgress) + pm[wParam].y));
            float f = ((pm[wParam].elem->GetWidth() / 76.0) * 100);
            wchar_t buffer[32];
            swprintf_s(buffer, L"iconfont;%d", (int)f);
            wcscat_s(buffer, L"%");
            filepm[wParam].elem->SetHeight(pm[wParam].elem->GetHeight() * 0.35);
            iconpm[wParam].elem->SetWidth(round(48 * (0.7 + 0.3 * bezierProgress)));
            iconpm[wParam].elem->SetHeight(round(48 * (0.7 + 0.3 * bezierProgress)));
            iconpm[wParam].elem->SetX(round(iconPadding * (0.7 + 0.3 * bezierProgress)));
            iconpm[wParam].elem->SetY(round((iconPadding * 0.72) * (0.7 + 0.3 * bezierProgress)));
            filepm[wParam].elem->SetFont((UCString)buffer);
        break;
    }
    case WM_USER + 4: {
            shadowpm[wParam].elem->SetAlpha(255);
            shadowpm[wParam].elem->SetWidth(64);
            shadowpm[wParam].elem->SetHeight(64);
            shadowpm[wParam].elem->SetX(iconPadding - 8);
            shadowpm[wParam].elem->SetY((iconPadding * 0.72) - 6);
            shortpm[wParam].elem->SetAlpha(255);
            shortpm[wParam].elem->SetWidth(32);
            shortpm[wParam].elem->SetHeight(32);
            shortpm[wParam].elem->SetX(iconPadding);
            shortpm[wParam].elem->SetY((iconPadding * 0.72) + 16);
        break;
    }
    case WM_USER + 7: {
        fullscreenpopupbase->SetVisible(true);
        fullscreeninner->SetVisible(true);
        fullscreeninner->SetY(100 * (1 - py[popupframe - 1]) + 1);
        break;
    }
    }
    return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
}

unsigned long animate(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
    this_thread::sleep_for(chrono::milliseconds(static_cast<int>(pm[yV->y].y * 0.5)));
    SendMessageW(wnd->GetHWND(), WM_USER + 1, yV->y, NULL);
    return 0;
}

unsigned long fastin(LPVOID lpParam) {
    yValue* yV = (yValue*)lpParam;
    this_thread::sleep_for(chrono::milliseconds(static_cast<int>(pm[yV->y].y * 0.5)));
    SendMessageW(wnd->GetHWND(), WM_USER + 4, yV->y, NULL);
    //for (int m = 1; m <= 24; m++) {
        frame[yV->y] = 24;
        SendMessageW(wnd->GetHWND(), WM_USER + 3, yV->y, NULL);
        //this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 400)));
    //}
    return 0;
}

//unsigned long animate4(LPVOID lpParam) {
//    CubicBezier(60, px, py, 0.75, 0.45, 0.0, 1.0);
//    for (int m = 1; m <= 60; m++) {
//        l = m;
//        SendMessage(wnd->GetHWND(), WM_USER + 6, NULL, NULL);
//        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[l] - px[l - 1]) * 50)));
//    }
//    return 0;
//}

unsigned long animate5(LPVOID lpParam) {
    CubicBezier(20, px, py, 0.05, 0.9, 0.3, 1.0);
    for (int m = 1; m <= 20; m++) {
        popupframe = m;
        SendMessage(wnd->GetHWND(), WM_USER + 7, NULL, NULL);
        this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 300)));
    }
    return 0;
}

void SelectItem(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        if (GetAsyncKeyState(VK_CONTROL) == 0) {
            for (int items = 0; items < pm.size(); items++) {
                pm[items].elem->SetSelected(false);
            }
        }
        if (elem != emptyspace) elem->SetSelected(true);
    }
}

//void dragdropAnimation() {
//    DWORD animThread;
//    animThreadHandle = CreateThread(0, 0, animate4, NULL, 0, &animThread);
//}

void fullscreenAnimation() {
    DWORD animThread;
    HANDLE animThreadHandle = CreateThread(0, 0, animate5, NULL, 0, &animThread);
}

void ApplyIcons(Element* elem = NULL, Event* iev = NULL)
{
    HINSTANCE testInst = LoadLibraryW(L"imageres.dll");
    static bool isColorized = 0;
    if (iev->type == Button::Click) {
        if (elem == testButton4) {
            isColorized = !isColorized;
        }
        BitmapPixelHandler type = isColorized ? StandardBitmapPixelHandler : AlphaBitmapPixelHandler;
        for (int icon = 0; icon < iconpm.size(); icon++) {
            HICON ico = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(4), IMAGE_ICON, 48, 48, LR_SHARED);
            HICON icoShortcut = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(163), IMAGE_ICON, 32, 32, LR_SHARED);
            HBITMAP bmp = IconToBitmap(ico);
            HBITMAP bmpShadow = AddPaddingToBitmap(bmp, 8);
            HBITMAP bmpShortcut = IconToBitmap(icoShortcut);
            IterateBitmap(bmp, type, 1);
            IterateBitmap(bmpShadow, SimpleBitmapPixelHandler, 0);
            IterateBitmap(bmpShortcut, type, 1);
            Value* bitmap = DirectUI::Value::CreateGraphic(bmp, 2, 0xffffffff, false, false, false);
            Value* bitmapShadow = DirectUI::Value::CreateGraphic(bmpShadow, 2, 0xffffffff, false, false, false);
            Value* bitmapShortcut = DirectUI::Value::CreateGraphic(bmpShortcut, 2, 0xffffffff, false, false, false);
            iconpm[icon].elem->SetValue(Element::ContentProp, 1, bitmap);
            shadowpm[icon].elem->SetValue(Element::ContentProp, 1, bitmapShadow);
            if (shortpm[icon].x == 1) shortpm[icon].elem->SetValue(Element::ContentProp, 1, bitmapShortcut);
            bitmap->Release();
            bitmapShadow->Release();
            bitmapShortcut->Release();
        }
    }
}

bool dragdrop = 0;

//void testEventListener2(Element* elem, Event* iev) {
//    if (iev->type == Button::Click) {
//        dragdropAnimation();
//    }
//}

void testEventListener3(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        switch (fullscreenpopup->GetAlpha()) {
        case 0:
            fullscreenpopup->SetAlpha(255);
            fullscreenAnimation();
            break;
        case 255:
            fullscreenpopup->SetAlpha(0);
            break;
        }
    }
}

void InitLayout(Element* elem, Event* iev) {
    static bool openclose = 0;
    Event* testEvent = iev;
    if (iev->type == Button::Click) {
        vector<wstring> files = list_directory();
        unsigned int count = files.size();
        wchar_t icount[32];
        testButton->SetEnabled(true);
        testButton5->SetEnabled(false);
        switch (openclose) {
        case 0: {
            swprintf_s(icount, L"        Found %d items!", count);
            itemcountstatus->SetContentString((UCString)icount);
            itemcountstatus->SetVisible(true); itemcountstatus->SetAlpha(0);
            int x = 0, y = 0;
            smIndex = 0;
            CubicBezier(24, px, py, 0.1, 0.9, 0.2, 1.0);
            frame.resize(count);
            pm.resize(count);
            iconpm.resize(count);
            shortpm.resize(count);
            shadowpm.resize(count);
            filepm.resize(count);
            RECT dimensions;
            GetClientRect(wnd->GetHWND(), &dimensions);
            DWORD* animThread = new DWORD[count];
            DWORD* animThread2 = new DWORD[count];
            HANDLE* animThreadHandle = new HANDLE[count];
            HANDLE* animThreadHandle2 = new HANDLE[count];
            int outerSizeX = GetSystemMetrics(SM_CXICONSPACING) + 4;
            int outerSizeY = GetSystemMetrics(SM_CYICONSPACING) + 24;
            for (int i = 0; i < count; i++) {
                Button* outerElem;
                parser->CreateElement((UCString)L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
                UIContainer->Add((Element**)&outerElem, 1);
                iconElem = (Element*)outerElem->FindDescendent(StrToID((UCString)L"iconElem"));
                shortcutElem = (Element*)outerElem->FindDescendent(StrToID((UCString)L"shortcutElem"));
                iconElemShadow = (Element*)outerElem->FindDescendent(StrToID((UCString)L"iconElemShadow"));
                textElem = (RichText*)outerElem->FindDescendent(StrToID((UCString)L"textElem"));
                textElem->SetContentString((UCString)files[i].c_str());
                pm[i].elem = outerElem, pm[i].x = x, pm[i].y = y;
                iconpm[i].elem = iconElem;
                shortpm[i].elem = shortcutElem;
                shadowpm[i].elem = iconElemShadow;
                filepm[i].elem = textElem;
                assignFn(outerElem, SelectItem);
                yValue* yV = new yValue{ i };
                yValue* yV2 = new yValue{ i };
                smIndex++;
                y += outerSizeY;
                if (y > dimensions.bottom - (outerSizeY + 74)) { // 74 is 64px of non-desktop area + 8px padding + 2px borders. 
                    y = 0;
                    x += outerSizeX;
                }
                animThreadHandle[i] = CreateThread(0, 0, animate, (LPVOID)yV, 0, &(animThread[i]));
                animThreadHandle2[i] = CreateThread(0, 0, fastin, (LPVOID)yV2, 0, &(animThread2[i]));
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
            frame.clear();
            pm.clear();
            iconpm.clear();
            shortpm.clear();
            shadowpm.clear();
            filepm.clear();
            shortIndex = 0;
            openclose = 0;
            itemcountstatus->SetVisible(false); itemcountstatus->SetAlpha(255);
            break;
        }
        }
    }
}

void testEventListener(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        InitLayout(testButton5, iev);
        InitLayout(testButton5, iev);
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
    NativeHWNDHost::Create((UCString)L"DirectDesktop", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 750, NULL, WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU, 0, &wnd);
    DUIXmlParser::Create(&parser, NULL, NULL, NULL, NULL);
    parser->SetXMLFromResource(IDR_UIFILE2, hInstance, hInstance);
    HWNDElement::Create(wnd->GetHWND(), true, NULL, NULL, &key, (Element**)&parent);
    WndProc = (WNDPROC)SetWindowLongPtr(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);

    parser->CreateElement((UCString)L"main", parent, NULL, NULL, &pMain);
    pMain->SetVisible(true);
    pMain->EndDefer(key);

    UpdateModeInfo();

    sampleText = regElem(L"sampleText");
    testButton = regBtn(L"testButton");
    testButton2 = regBtn(L"testButton2");
    testButton3 = regBtn(L"testButton3");
    testButton4 = regBtn(L"testButton4");
    testButton5 = regBtn(L"testButton5");
    mainContainer = regElem(L"mainContainer");
    UIContainer = regElem(L"UIContainer");
    fullscreenpopup = regElem(L"fullscreenpopup");
    fullscreenpopupbase = regElem(L"fullscreenpopupbase");
    fullscreeninner = regElem(L"fullscreeninner");
    itemcountstatus = regElem(L"itemcountstatus");
    emptyspace = regBtn(L"emptyspace");

    assignFn(testButton, testEventListener);
    assignFn(testButton, ApplyIcons);
    //assignFn(testButton2, testEventListener2);
    assignFn(testButton3, testEventListener3);
    assignFn(testButton4, ApplyIcons);
    assignFn(testButton5, InitLayout);
    assignFn(testButton5, ApplyIcons);
    assignFn(emptyspace, SelectItem);

    wnd->Host(pMain);

    wnd->ShowWindow(SW_SHOW);
    StartMessagePump();
    UnInitProcessPriv(0);

    return 0;
}