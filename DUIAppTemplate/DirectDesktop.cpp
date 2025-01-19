// DUIAppTemplate.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "DirectUI/DirectUI.h"
#include "DUIAppTemplate.h"
#include <string>
#include "resource.h"
#include <propkey.h>
#include "strsafe.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <WinUser.h>
#include "StyleModifier.h"
#include "BitmapHelper.h"


using namespace DirectUI;

NativeHWNDHost* wnd;
HWNDElement* parent;
DUIXmlParser* parser;
Element* pMain;
unsigned long key = 0;

Button* testButton;
Button* testButton2;
Button* testButton3;
Button* testButton4;
Element* sampleText;
Element* mainContainer;
Element* outerElem, *outerElem2, *outerElem3, *outerElem4, *outerElem5, *outerElem6;
Element* iconElem, *iconElem2, *iconElem3, *iconElem4, *iconElem5, *iconElem6;
RichText* textElem, *textElem2, *textElem3, *textElem4, *textElem5, *textElem6;
Element* fullscreenpopup, *fullscreeninner;
HRESULT err;

int i, j, k, l, n;

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

int x, y;

const wchar_t* CharToWChar(const char* input)
{
    size_t charCount = strlen(input) + 1;
    wchar_t* result = new wchar_t[charCount];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, result, charCount, input, _TRUNCATE);
    return result;
}

double px[80];
double py[80];

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

HANDLE animThreadHandle;

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
        outerElem->SetWidth(76 * (0.7 + 0.3 * py[i - 1]));
        outerElem->SetHeight(96 * (0.7 + 0.3 * py[i - 1]));
        outerElem->SetX(mainContainer->GetWidth() * 0.15 * (1 - py[i - 1]));
        outerElem->SetY(mainContainer->GetHeight() * 0.15 * (1 - py[i - 1]));
        outerElem2->SetWidth(76 * (0.7 + 0.3 * py[i - 1]));
        outerElem2->SetHeight(96 * (0.7 + 0.3 * py[i - 1]));
        outerElem2->SetX((mainContainer->GetWidth() - 152) * 0.15 * (1 - py[i - 1]) + 76);
        outerElem2->SetY(mainContainer->GetHeight() * 0.15 * (1 - py[i - 1]));
        outerElem5->SetWidth(76 * (0.7 + 0.3 * py[i - 1]));
        outerElem5->SetHeight(96 * (0.7 + 0.3 * py[i - 1]));
        outerElem5->SetX((mainContainer->GetWidth() - 1520) * 0.15 * (1 - py[i - 1]) + 760);
        outerElem5->SetY((mainContainer->GetHeight()) * 0.15 * (1 - py[i - 1]));
        iconElem->SetWidth(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem->SetHeight(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem->SetX(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem->SetY(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem2->SetWidth(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem2->SetHeight(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem2->SetX(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem2->SetY(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem5->SetWidth(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem5->SetHeight(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem5->SetX(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        iconElem5->SetY(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        float f = ((textElem->GetWidth() / 76.0) * 100);
        char buffer[32];
        sprintf_s(buffer, "iconfont;%d", (int)f);
        strcat_s(buffer, "%");
        textElem->SetFont((UCString)CharToWChar(buffer));
        textElem->SetWidth(outerElem->GetWidth());
        textElem->SetHeight(outerElem->GetHeight() * 0.35);
        textElem2->SetFont((UCString)CharToWChar(buffer));
        textElem2->SetWidth(outerElem2->GetWidth());
        textElem2->SetHeight(outerElem2->GetHeight() * 0.35);
        textElem4->SetFont((UCString)CharToWChar(buffer));
        textElem4->SetWidth(outerElem4->GetWidth());
        textElem4->SetHeight(outerElem4->GetHeight() * 0.35);
        textElem5->SetFont((UCString)CharToWChar(buffer));
        textElem5->SetWidth(outerElem5->GetWidth());
        textElem5->SetHeight(outerElem5->GetHeight() * 0.35);
        textElem6->SetFont((UCString)CharToWChar(buffer));
        textElem6->SetWidth(outerElem6->GetWidth());
        textElem6->SetHeight(outerElem6->GetHeight() * 0.35);

        break;
    }
    case WM_USER + 2: {
        outerElem3->SetWidth(76 * (0.7 + 0.3 * py[j - 1]));
        outerElem3->SetHeight(96 * (0.7 + 0.3 * py[j - 1]));
        outerElem3->SetX((mainContainer->GetWidth() - 152) * 0.15 * (1 - py[j - 1]) + 76);
        outerElem3->SetY((mainContainer->GetHeight() - 192) * 0.15 * (1 - py[j - 1]) + 96);
        iconElem3->SetWidth(48 * (0.7 + 0.3 * py[j - 2]) + 1);
        iconElem3->SetHeight(48 * (0.7 + 0.3 * py[j - 2]) + 1);
        iconElem3->SetX(14 * (0.7 + 0.3 * py[j - 2]) + 1);
        iconElem3->SetY(14 * (0.7 + 0.3 * py[j - 2]) + 1);
        float f = ((textElem->GetWidth() / 76.0) * 100);
        char buffer[32];
        sprintf_s(buffer, "iconfont;%d", (int)f);
        strcat_s(buffer, "%");
        textElem3->SetFont((UCString)CharToWChar(buffer));
        textElem3->SetWidth(outerElem3->GetWidth());
        textElem3->SetHeight(outerElem3->GetHeight() * 0.35);
        break;
    }
    case WM_USER + 3: {
        outerElem4->SetWidth(76 * (0.7 + 0.3 * py[k - 1]));
        outerElem4->SetHeight(96 * (0.7 + 0.3 * py[k - 1]));
        outerElem4->SetX((mainContainer->GetWidth() - 304) * 0.15 * (1 - py[k - 1]) + 152);
        outerElem4->SetY((mainContainer->GetHeight() - 384) * 0.15 * (1 - py[k - 1]) + 192);
        outerElem6->SetWidth(76 * (0.7 + 0.3 * py[k - 1]));
        outerElem6->SetHeight(96 * (0.7 + 0.3 * py[k - 1]));
        outerElem6->SetX((mainContainer->GetWidth() - 1368) * 0.15 * (1 - py[k - 1]) + 684);
        outerElem6->SetY((mainContainer->GetHeight() - 384) * 0.15 * (1 - py[k - 1]) + 192);
        iconElem4->SetWidth(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        iconElem4->SetHeight(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        iconElem4->SetX(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        iconElem4->SetY(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        iconElem6->SetWidth(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        iconElem6->SetHeight(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        iconElem6->SetX(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        iconElem6->SetY(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        float f = ((textElem->GetWidth() / 76.0) * 100);
        char buffer[32];
        sprintf_s(buffer, "iconfont;%d", (int)f);
        strcat_s(buffer, "%");
        break;
    }
    case WM_USER + 4: {
        outerElem3->SetAlpha(255);
        break;
    }
    case WM_USER + 5: {
        outerElem4->SetAlpha(255);
        outerElem6->SetAlpha(255);
        break;
    }
    case WM_USER + 6: {
        outerElem->SetX(800 * py[l - 1]);
        outerElem->SetY(500 * py[l - 1]);
        outerElem2->SetX(76 + 724 * py[l - 1]);
        outerElem2->SetY(500 * py[l - 1]);
        outerElem3->SetX(76 + 724 * py[l - 1]);
        outerElem3->SetY(96 + 404 * py[l - 1]);
        outerElem4->SetX(152 + 648 * py[l - 1]);
        outerElem4->SetY(192 + 308 * py[l - 1]);
        outerElem5->SetX(760 + 40 * py[l - 1]);
        outerElem5->SetY(500 * py[l - 1]);
        outerElem6->SetX(684 + 116 * py[l - 1]);
        outerElem6->SetY(192 + 304 * py[l - 1]);
        break;
    }
    case WM_USER + 7: {
        fullscreeninner->SetY(100 * (1 - py[n - 1]) + 1);
    }
    }
    return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
}

unsigned long animate(LPVOID lpParam) {
    CubicBezier(40, px, py, 0.1, 0.9, 0.2, 1.0);
    for (int m = 1; m <= 40; m++) {
        i = m;
        SendMessage(wnd->GetHWND(), WM_USER + 1, NULL, NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[i] - px[i-1]) * 800)));
    }
    return 0;
}

unsigned long animate2(LPVOID lpParam) {
    std::this_thread::sleep_for(std::chrono::milliseconds(75));
    SendMessage(wnd->GetHWND(), WM_USER + 4, NULL, NULL);
    for (int m = 1; m <= 40; m++) {
        j = m;
        SendMessage(wnd->GetHWND(), WM_USER + 2, NULL, NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[j] - px[j - 1]) * 800)));
    }
    return 0;
}

unsigned long animate3(LPVOID lpParam) {
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    SendMessage(wnd->GetHWND(), WM_USER + 5, NULL, NULL);
    for (int m = 1; m <= 40; m++) {
        k = m;
        SendMessage(wnd->GetHWND(), WM_USER + 3, NULL, NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[k] - px[k - 1]) * 800)));
    }
    return 0;
}

unsigned long animate4(LPVOID lpParam) {
    CubicBezier(60, px, py, 0.75, 0.45, 0.0, 1.0);
    for (int m = 1; m <= 60; m++) {
        l = m;
        SendMessage(wnd->GetHWND(), WM_USER + 6, NULL, NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[l] - px[l - 1]) * 50)));
    }
    return 0;
}

unsigned long animate5(LPVOID lpParam) {
    CubicBezier(20, px, py, 0.05, 0.9, 0.3, 1.0);
    for (int m = 1; m <= 20; m++) {
        n = m;
        SendMessage(wnd->GetHWND(), WM_USER + 7, NULL, NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[n] - px[n - 1]) * 300)));
    }
    return 0;
}

void startAnimation() {
    DWORD animThread;
    animThreadHandle = CreateThread(0, 0, animate, NULL, 0, &animThread);
    animThreadHandle = CreateThread(0, 0, animate2, NULL, 0, &animThread);
    animThreadHandle = CreateThread(0, 0, animate3, NULL, 0, &animThread);
}

void dragdropAnimation() {
    DWORD animThread;
    animThreadHandle = CreateThread(0, 0, animate4, NULL, 0, &animThread);
}

void fullscreenAnimation() {
    DWORD animThread;
    animThreadHandle = CreateThread(0, 0, animate5, NULL, 0, &animThread);
}

void ModifyStyle(Element* elem, Event* iev)
{
    HINSTANCE testInst = LoadLibraryW(L"imageres.dll");
    if (iev->type == Button::Click) {
        testButton4->SetForegroundColor(ImmersiveColor);
        //UpdateCache *h{}, *h2{};
        //Value* test = iconElem->GetValue(Element::ContentProp, 1, h);
        //Value* test2 = iconElem2->GetValue(Element::ContentProp, 1, h2);
        HICON ico = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(4), IMAGE_ICON, 48, 48, LR_SHARED);
        HICON ico2 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(18), IMAGE_ICON, 48, 48, LR_SHARED);
        HICON ico3 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(148), IMAGE_ICON, 48, 48, LR_SHARED);
        HICON ico4 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(1306), IMAGE_ICON, 48, 48, LR_SHARED);
        HICON ico5 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(101), IMAGE_ICON, 48, 48, LR_SHARED);
        HICON ico6 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(82), IMAGE_ICON, 48, 48, LR_SHARED);
        HBITMAP bmp = IconToBitmap(ico);
        HBITMAP bmp2 = IconToBitmap(ico2);
        HBITMAP bmp3 = IconToBitmap(ico3);
        HBITMAP bmp4 = IconToBitmap(ico4);
        HBITMAP bmp5 = IconToBitmap(ico5);
        HBITMAP bmp6 = IconToBitmap(ico6);
        IterateBitmap(bmp, StandardBitmapPixelHandler);
        IterateBitmap(bmp2, StandardBitmapPixelHandler);
        IterateBitmap(bmp3, StandardBitmapPixelHandler);
        IterateBitmap(bmp4, StandardBitmapPixelHandler);
        IterateBitmap(bmp5, StandardBitmapPixelHandler);
        IterateBitmap(bmp6, StandardBitmapPixelHandler);
        Value* bitmap = DirectUI::Value::CreateGraphic(bmp, 2, 0xffffffff, false, false, false);
        Value* bitmap2 = DirectUI::Value::CreateGraphic(bmp2, 2, 0xffffffff, false, false, false);
        Value* bitmap3 = DirectUI::Value::CreateGraphic(bmp3, 2, 0xffffffff, false, false, false);
        Value* bitmap4 = DirectUI::Value::CreateGraphic(bmp4, 2, 0xffffffff, false, false, false);
        Value* bitmap5 = DirectUI::Value::CreateGraphic(bmp5, 2, 0xffffffff, false, false, false);
        Value* bitmap6 = DirectUI::Value::CreateGraphic(bmp6, 2, 0xffffffff, false, false, false);
        iconElem->SetValue(Element::ContentProp, 1, bitmap);
        iconElem2->SetValue(Element::ContentProp, 1, bitmap2);
        iconElem3->SetValue(Element::ContentProp, 1, bitmap3);
        iconElem4->SetValue(Element::ContentProp, 1, bitmap4);
        iconElem5->SetValue(Element::ContentProp, 1, bitmap5);
        iconElem6->SetValue(Element::ContentProp, 1, bitmap6);
        // 
        //if (samplecolor != NULL) {

        //    samplecolor->SetValue(Element::BackgroundProp, 1, bitmap);
        //    samplecolor->SetForegroundColor(btnforeground);
        //}
        //if (tabPill != NULL) {
        //    tabPill->SetValue(Element::ContentProp, 1, bitmap2);
        //}
        //test->Release();
        //test2->Release();
        //bitmap->Release();
        //bitmap2->Release();
    }
}

void testEventListener(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        switch (outerElem->GetAlpha()) {
        case 0:
            outerElem->SetAlpha(255);
            outerElem2->SetAlpha(255);
            outerElem5->SetAlpha(255);
            startAnimation();
            break;
        case 255:
            outerElem->SetAlpha(0);
            outerElem2->SetAlpha(0);
            outerElem3->SetAlpha(0);
            outerElem4->SetAlpha(0);
            outerElem5->SetAlpha(0);
            outerElem6->SetAlpha(0);
            break;
        }
    }
}

bool dragdrop = 0;

void testEventListener2(Element* elem, Event* iev) {
    if (iev->type == Button::Click) {
        dragdropAnimation();
    }
}

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
    mainContainer = regElem(L"mainContainer");
    outerElem = regElem(L"outerElem");
    iconElem = regElem(L"iconElem");
    textElem = regRichText(L"textElem");
    outerElem2 = regElem(L"outerElem2");
    iconElem2 = regElem(L"iconElem2");
    textElem2 = regRichText(L"textElem2");
    outerElem3 = regElem(L"outerElem3");
    iconElem3 = regElem(L"iconElem3");
    textElem3 = regRichText(L"textElem3");
    outerElem4 = regElem(L"outerElem4");
    iconElem4 = regElem(L"iconElem4");
    textElem4 = regRichText(L"textElem4");
    outerElem5 = regElem(L"outerElem5");
    iconElem5 = regElem(L"iconElem5");
    textElem5 = regRichText(L"textElem5");
    outerElem6 = regElem(L"outerElem6");
    iconElem6 = regElem(L"iconElem6");
    textElem6 = regRichText(L"textElem6");
    fullscreenpopup = regElem(L"fullscreenpopup");
    fullscreeninner = regElem(L"fullscreeninner");

    assignFn(testButton,testEventListener);
    assignFn(testButton2, testEventListener2);
    assignFn(testButton3, testEventListener3);
    assignFn(testButton4, ModifyStyle);

    wnd->Host(pMain);

    wnd->ShowWindow(SW_SHOW);
    StartMessagePump();
    UnInitProcessPriv(0);

    return 0;
}

