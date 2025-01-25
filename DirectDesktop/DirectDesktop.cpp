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
Element* iconElemShadow;
RichText* textElem;
Element* fullscreenpopup, *fullscreenpopupbase, *fullscreeninner;
Element* itemcountstatus;
Button* emptyspace;
HRESULT err;

int* frame, popupframe;// j, k, l;

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

WNDPROC WndProc; // Window procedure data type (from Windows API)

vector<wstring> list_directory() {
    static int isFileHiddenEnabled;
    static int isFileSuperHiddenEnabled;
    LPCWSTR path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    HKEY hKey;
    DWORD lResult = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    DWORD lResult2 = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwSize = NULL;
        DWORD dwSize2 = NULL;
        lResult = RegGetValue(hKey, NULL, L"Hidden", RRF_RT_DWORD, NULL, NULL, &dwSize);
        lResult2 = RegGetValue(hKey, NULL, L"ShowSuperHidden", RRF_RT_DWORD, NULL, NULL, &dwSize2);
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
        if (runs > 0) dir_list.push_back(wstring(findData.cFileName));
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            dir_list.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                dir_list.pop_back();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path2, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) dir_list.push_back(wstring(findData.cFileName));
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            dir_list.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                dir_list.pop_back();
                continue;
            }
        }
    }

    runs = 0;
    hFind = FindFirstFileW(full_path3, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) dir_list.push_back(wstring(findData.cFileName));
        runs++;
        if (isFileHiddenEnabled == 2 && findData.dwFileAttributes & 2) {
            dir_list.pop_back();
            continue;
        }
        if (isFileSuperHiddenEnabled == 2 || isFileSuperHiddenEnabled == 0) {
            if (findData.dwFileAttributes & 4) {
                dir_list.pop_back();
                continue;
            }
        }
    }

    FindClose(hFind);
    return dir_list;
}

vector<parameters> pm;
vector<parameters> iconpm;
vector<parameters> shadowpm;
vector<parameters> filepm;
int index = 0, smIndex = 0, smIndex2 = 0;

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case 0x001A: {
        UpdateModeInfo();
        break;
    }
    case WM_COMMAND: {
        break;
    }
    case WM_USER + 1: {
        pm[index].elem->SetAlpha(255);
        pm[index].elem->SetVisible(true);
        index++;
        break;
    }
    case WM_USER + 2: {
        pm[index].elem->SetAlpha(0);
        shadowpm[index].elem->SetAlpha(0);
        index++;
        break;
    }
    case WM_USER + 3: {
        for (int fIndex = 0; fIndex < smIndex; fIndex++) {
            double bezierProgress = py[frame[fIndex] - 1];
            pm[fIndex].elem->SetWidth(76 * (0.7 + 0.3 * bezierProgress));
            pm[fIndex].elem->SetHeight(96 * (0.7 + 0.3 * bezierProgress));
            pm[fIndex].elem->SetX(round((mainContainer->GetWidth() - 2 * pm[fIndex].x) * 0.15 * (1 - bezierProgress) + pm[fIndex].x));
            pm[fIndex].elem->SetY(round((mainContainer->GetHeight() - 2 * pm[fIndex].y) * 0.15 * (1 - bezierProgress) + pm[fIndex].y));
            //float f = ((pm[fIndex].elem->GetWidth() / 76.0) * 100);
            //wchar_t buffer[32];
            //swprintf_s(buffer, L"iconfont;%d", (int)f);
            //wcscat_s(buffer, L"%");
            //filepm[fIndex].elem->SetWidth(pm[fIndex].elem->GetWidth());
            filepm[fIndex].elem->SetHeight(pm[fIndex].elem->GetHeight() * 0.35);
            iconpm[fIndex].elem->SetWidth(round(48 * (0.7 + 0.3 * bezierProgress)));
            iconpm[fIndex].elem->SetHeight(round(48 * (0.7 + 0.3 * bezierProgress)));
            iconpm[fIndex].elem->SetX(round(14 * (0.7 + 0.3 * bezierProgress)));
            iconpm[fIndex].elem->SetY(round(10 * (0.7 + 0.3 * bezierProgress)));
            //filepm[fIndex].elem->SetFont((UCString)buffer);
        }
        break;
    }
    case WM_USER + 4: {
        for (int fIndex = 0; fIndex < smIndex; fIndex++) {
            shadowpm[fIndex].elem->SetAlpha(255);
            shadowpm[fIndex].elem->SetWidth(64); // * (0.7 + 0.3 * bezierProgress));
            shadowpm[fIndex].elem->SetHeight(64); // * (0.7 + 0.3 * bezierProgress));
            shadowpm[fIndex].elem->SetX(6); // * (0.7 + 0.3 * bezierProgress));
            shadowpm[fIndex].elem->SetY(4); // * (0.7 + 0.3 * bezierProgress));
        }
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
    static int test;
    this_thread::sleep_for(chrono::milliseconds(static_cast<int>(pm[test++].y * 0.5)));
    SendMessageW(wnd->GetHWND(), WM_USER + 1, NULL, NULL);
    if (test == pm.size()) test = 0;
    return 0;
}

unsigned long fastin(LPVOID lpParam) {
    yValue* yV2 = (yValue*)lpParam;
    yValue* yV = (yValue*)lpParam;
    this_thread::sleep_for(chrono::milliseconds(static_cast<int>(pm[yV2->y].y * 0.5)));
    SendMessage(wnd->GetHWND(), WM_USER + 4, NULL, NULL);
    for (int m = 1; m <= 24; m++) {
        frame[yV->y] = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 3, NULL, NULL);
        this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 500)));
    }
    return 0;
}

unsigned long rem(LPVOID lpParam) {
    SendMessageW(wnd->GetHWND(), WM_USER + 2, NULL, NULL);
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
        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[m] - px[m - 1]) * 300)));
    }
    return 0;
}

void remove(Element* elem) {
    DWORD animThread;
    pm[index].elem = elem;
    index++;
    HANDLE animThreadHandle = CreateThread(0, 0, rem, NULL, 0, &animThread);
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
            HBITMAP bmp = IconToBitmap(ico);
            HBITMAP bmpShadow = AddPaddingToBitmap(bmp, 8);
            IterateBitmap(bmp, type, 1);
            IterateBitmap(bmpShadow, SimpleBitmapPixelHandler, 0);
            Value* bitmap = DirectUI::Value::CreateGraphic(bmp, 2, 0xffffffff, false, false, false);
            Value* bitmapShadow = DirectUI::Value::CreateGraphic(bmpShadow, 2, 0xffffffff, false, false, false);
            iconpm[icon].elem->SetValue(Element::ContentProp, 1, bitmap);
            shadowpm[icon].elem->SetValue(Element::ContentProp, 1, bitmapShadow);
            bitmap->Release();
            bitmapShadow->Release();
        }
    }
}

void testEventListener(Element* elem, Event* iev) {
    static bool openclose = 0;
    if (iev->type == Button::Click) {
        unsigned int count = list_directory().size();
        switch (openclose) {
        case 0: {
            int x = 0, y = 0;
            index = 0;
            smIndex = 0;
            CubicBezier(24, px, py, 0.1, 0.9, 0.2, 1.0);
            DWORD* animThread = new DWORD[count];
            DWORD* animThread2 = new DWORD[count];
            HANDLE* animThreadHandle = new HANDLE[count];
            HANDLE* animThreadHandle2 = new HANDLE[count];
            for (int i = 0; i < count; i++) {
                pm[index].x = x, pm[index].y = y;
                yValue* yV = new yValue{ i };
                index++;
                smIndex++;
                x += 80;
                if (x >= mainContainer->GetWidth() - 80) {
                    x = 0;
                    y += 100;
                }
                animThreadHandle[i] = CreateThread(0, 0, animate, NULL, 0, &(animThread[i]));
                animThreadHandle2[i] = CreateThread(0, 0, fastin, (LPVOID)yV, 0, &(animThread2[i]));
            }
            index = 0;
            openclose = 1;
            delete[] animThread;
            delete[] animThread2;
            delete[] animThreadHandle;
            delete[] animThreadHandle2;
            break;
        }
        case 1: {
            index = 0;
            for (int i = 0; i < count; i++) {
                remove(pm[index].elem);
            }
            index = 0;
            openclose = 0;
            break;
        }
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
    if (iev->type == Button::Click) {
        vector<wstring> files = list_directory();
        unsigned int count = files.size();
        wchar_t icount[32];
        swprintf_s(icount, L"        Found %d items!", count);
        itemcountstatus->SetContentString((UCString)icount);
        itemcountstatus->SetVisible(true); itemcountstatus->SetAlpha(0);
        int x = 0, y = 0;
        index = 0;
        smIndex = 0;
        CubicBezier(24, px, py, 0.1, 0.9, 0.2, 1.0);
        pm.resize(count);
        iconpm.resize(count);
        shadowpm.resize(count);
        filepm.resize(count);
        frame = new int[count]{};
        for (int i = 0; i < count; i++) {
            Button* outerElem;
            parser->CreateElement((UCString)L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
            UIContainer->Add((Element**)&outerElem, 1);
            iconElem = (Element*)outerElem->FindDescendent(StrToID((UCString)L"iconElem"));
            iconElemShadow = (Element*)outerElem->FindDescendent(StrToID((UCString)L"iconElemShadow"));
            textElem = (RichText*)outerElem->FindDescendent(StrToID((UCString)L"textElem"));
            textElem->SetContentString((UCString)files[i].c_str());
            pm[index].elem = outerElem, pm[index].x = x, pm[index].y = y;
            iconpm[index].elem = iconElem;
            shadowpm[index].elem = iconElemShadow;
            filepm[index].elem = textElem;
            assignFn(outerElem, SelectItem);
            yValue* yV = new yValue{ i };
            index++;
            smIndex++;
            x += 80;
            if (x >= mainContainer->GetWidth() - 80) {
                x = 0;
                y += 100;
            }
        }
        index = 0;
        files.clear();
        testButton->SetEnabled(true);
        testButton5->SetEnabled(false);
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
    //assignFn(testButton2, testEventListener2);
    assignFn(testButton3, testEventListener3);
    assignFn(testButton4, ApplyIcons);
    assignFn(testButton5, InitLayout);
    assignFn(testButton5, testEventListener);
    assignFn(testButton5, ApplyIcons);
    assignFn(emptyspace, SelectItem);

    wnd->Host(pMain);

    wnd->ShowWindow(SW_SHOW);
    StartMessagePump();
    UnInitProcessPriv(0);

    return 0;
}