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
Element* outerElem;// *outerElem2, * outerElem3, * outerElem4, * outerElem5, * outerElem6;
Element* iconElem;// , *iconElem2, *iconElem3, *iconElem4, *iconElem5, *iconElem6;
Element* iconElemShadow;
RichText* textElem;// , * textElem2, * textElem3, * textElem4, * textElem5, * textElem6;
Element* fullscreenpopup, *fullscreeninner;
Element* itemcountstatus;
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

    hFind = FindFirstFileW(full_path, &findData);
    int runs = 0;
    while (FindNextFileW(hFind, &findData) != 0)
    {
        if (runs > 0) {
            dir_list.push_back(wstring(findData.cFileName));
        }
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

    hFind = FindFirstFileW(full_path2, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        dir_list.push_back(wstring(findData.cFileName));
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

    hFind = FindFirstFileW(full_path3, &findData);
    while (FindNextFileW(hFind, &findData) != 0)
    {
        dir_list.push_back(wstring(findData.cFileName));
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
        index++;
        //outerElem->SetX(mainContainer->GetWidth() * 0.15 * (1 - py[i - 1]));
        //outerElem->SetY(mainContainer->GetHeight() * 0.15 * (1 - py[i - 1]));
        //outerElem2->SetWidth(76 * (0.7 + 0.3 * py[i - 1]));
        //outerElem2->SetHeight(96 * (0.7 + 0.3 * py[i - 1]));
        //outerElem2->SetX((mainContainer->GetWidth() - 152) * 0.15 * (1 - py[i - 1]) + 76);
        //outerElem2->SetY(mainContainer->GetHeight() * 0.15 * (1 - py[i - 1]));
        //outerElem5->SetWidth(76 * (0.7 + 0.3 * py[i - 1]));
        //outerElem5->SetHeight(96 * (0.7 + 0.3 * py[i - 1]));
        //outerElem5->SetX((mainContainer->GetWidth() - 1520) * 0.15 * (1 - py[i - 1]) + 760);
        //outerElem5->SetY((mainContainer->GetHeight()) * 0.15 * (1 - py[i - 1]));
        //iconElem->SetWidth(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem->SetHeight(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem->SetX(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem->SetY(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem2->SetWidth(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem2->SetHeight(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem2->SetX(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem2->SetY(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem5->SetWidth(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem5->SetHeight(48 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem5->SetX(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        //iconElem5->SetY(14 * (0.7 + 0.3 * py[i - 2]) + 1);
        //float f = ((textElem->GetWidth() / 76.0) * 100);
        //char buffer[32];
        //sprintf_s(buffer, "iconfont;%d", (int)f);
        //strcat_s(buffer, "%");
        //textElem->SetContentString((UCString)list_directory()[10].c_str());
        //textElem->SetFont((UCString)CharToWChar(buffer));
        //textElem->SetWidth(outerElem->GetWidth());
        //textElem->SetHeight(outerElem->GetHeight() * 0.35);
        //textElem2->SetFont((UCString)CharToWChar(buffer));
        //textElem2->SetWidth(outerElem2->GetWidth());
        //textElem2->SetHeight(outerElem2->GetHeight() * 0.35);
        //textElem4->SetFont((UCString)CharToWChar(buffer));
        //textElem4->SetWidth(outerElem4->GetWidth());
        //textElem4->SetHeight(outerElem4->GetHeight() * 0.35);
        //textElem5->SetFont((UCString)CharToWChar(buffer));
        //textElem5->SetWidth(outerElem5->GetWidth());
        //textElem5->SetHeight(outerElem5->GetHeight() * 0.35);
        //textElem6->SetFont((UCString)CharToWChar(buffer));
        //textElem6->SetWidth(outerElem6->GetWidth());
        //textElem6->SetHeight(outerElem6->GetHeight() * 0.35);
        break;
    }
    case WM_USER + 2: {
        pm[index].elem->SetAlpha(0);
        index++;
        break;
    }
    case WM_USER + 3: {
        for (int fIndex = 0; fIndex < smIndex; fIndex++) {
            double bezierProgress = py[frame[fIndex] - 1];
            pm[fIndex].elem->SetWidth(76 * (0.7 + 0.3 * bezierProgress));
            pm[fIndex].elem->SetHeight(96 * (0.7 + 0.3 * bezierProgress));
            pm[fIndex].elem->SetX((mainContainer->GetWidth() - 2 * pm[fIndex].x) * 0.15 * (1 - bezierProgress) + pm[fIndex].x);
            pm[fIndex].elem->SetY((mainContainer->GetHeight() - 2 * pm[fIndex].y) * 0.15 * (1 - bezierProgress) + pm[fIndex].y);
        }
        break;
    }
    case WM_USER + 22: { // used to be 2
        //outerElem3->SetWidth(76 * (0.7 + 0.3 * py[j - 1]));
        //outerElem3->SetHeight(96 * (0.7 + 0.3 * py[j - 1]));
        //outerElem3->SetX((mainContainer->GetWidth() - 152) * 0.15 * (1 - py[j - 1]) + 76);
        //outerElem3->SetY((mainContainer->GetHeight() - 192) * 0.15 * (1 - py[j - 1]) + 96);
        //iconElem3->SetWidth(48 * (0.7 + 0.3 * py[j - 2]) + 1);
        //iconElem3->SetHeight(48 * (0.7 + 0.3 * py[j - 2]) + 1);
        //iconElem3->SetX(14 * (0.7 + 0.3 * py[j - 2]) + 1);
        //iconElem3->SetY(14 * (0.7 + 0.3 * py[j - 2]) + 1);
        //float f = ((textElem->GetWidth() / 76.0) * 100);
        //char buffer[32];
        //sprintf_s(buffer, "iconfont;%d", (int)f);
        //strcat_s(buffer, "%");
        //textElem3->SetFont((UCString)CharToWChar(buffer));
        //textElem3->SetWidth(outerElem3->GetWidth());
        //textElem3->SetHeight(outerElem3->GetHeight() * 0.35);
        break;
    }
    case WM_USER + 333: { //used to be 3
        //outerElem4->SetWidth(76 * (0.7 + 0.3 * py[k - 1]));
        //outerElem4->SetHeight(96 * (0.7 + 0.3 * py[k - 1]));
        //outerElem4->SetX((mainContainer->GetWidth() - 304) * 0.15 * (1 - py[k - 1]) + 152);
        //outerElem4->SetY((mainContainer->GetHeight() - 384) * 0.15 * (1 - py[k - 1]) + 192);
        //outerElem6->SetWidth(76 * (0.7 + 0.3 * py[k - 1]));
        //outerElem6->SetHeight(96 * (0.7 + 0.3 * py[k - 1]));
        //outerElem6->SetX((mainContainer->GetWidth() - 1368) * 0.15 * (1 - py[k - 1]) + 684);
        //outerElem6->SetY((mainContainer->GetHeight() - 384) * 0.15 * (1 - py[k - 1]) + 192);
        //iconElem4->SetWidth(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        //iconElem4->SetHeight(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        //iconElem4->SetX(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        //iconElem4->SetY(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        //iconElem6->SetWidth(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        //iconElem6->SetHeight(48 * (0.7 + 0.3 * py[k - 2]) + 1);
        //iconElem6->SetX(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        //iconElem6->SetY(14 * (0.7 + 0.3 * py[k - 2]) + 1);
        //float f = ((textElem->GetWidth() / 76.0) * 100);
        //char buffer[32];
        //sprintf_s(buffer, "iconfont;%d", (int)f);
        //strcat_s(buffer, "%");
        break;
    }
    case WM_USER + 4444: {
        //outerElem3->SetAlpha(255);
        break;
    }
    case WM_USER + 55555: {
        //outerElem4->SetAlpha(255);
        //outerElem6->SetAlpha(255);
        break;
    }
    case WM_USER + 666666: {
        //outerElem->SetX(800 * py[l - 1]);
        //outerElem->SetY(500 * py[l - 1]);
        //outerElem2->SetX(76 + 724 * py[l - 1]);
        //outerElem2->SetY(500 * py[l - 1]);
        //outerElem3->SetX(76 + 724 * py[l - 1]);
        //outerElem3->SetY(96 + 404 * py[l - 1]);
        //outerElem4->SetX(152 + 648 * py[l - 1]);
        //outerElem4->SetY(192 + 308 * py[l - 1]);
        //outerElem5->SetX(760 + 40 * py[l - 1]);
        //outerElem5->SetY(500 * py[l - 1]);
        //outerElem6->SetX(684 + 116 * py[l - 1]);
        //outerElem6->SetY(192 + 304 * py[l - 1]);
        break;
    }
    case WM_USER + 7: {
        fullscreeninner->SetY(100 * (1 - py[popupframe - 1]) + 1);
        break;
    }
    case WM_USER + 8: {
        //iconElemShadow->SetAlpha(0);
        break;
    }
    case WM_USER + 9: {
        //iconElemShadow->SetX(iconElem->GetX() - 4);
        //iconElemShadow->SetY(iconElem->GetY() - 2);
        //iconElemShadow->SetAlpha(255);
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
    for (int m = 1; m <= 24; m++) {
        frame[yV->y] = m;
        SendMessageW(wnd->GetHWND(), WM_USER + 3, NULL, NULL);
        //if (m == 18) SendMessage(wnd->GetHWND(), WM_USER + 9, (WPARAM)lpParam, NULL);
        this_thread::sleep_for(chrono::milliseconds((int)((px[m] - px[m - 1]) * 500)));
    }
    return 0;
}

unsigned long rem(LPVOID lpParam) {
    SendMessageW(wnd->GetHWND(), WM_USER + 2, NULL, NULL);
    return 0;
}

//unsigned long animate2(LPVOID lpParam) {
//    std::this_thread::sleep_for(std::chrono::milliseconds(75));
//    SendMessage(wnd->GetHWND(), WM_USER + 4, NULL, NULL);
//    for (int m = 1; m <= 40; m++) {
//        j = m;
//        SendMessage(wnd->GetHWND(), WM_USER + 2, NULL, NULL);
//        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[j] - px[j - 1]) * 800)));
//    }
//    return 0;
//}

//unsigned long animate3(LPVOID lpParam) {
//    std::this_thread::sleep_for(std::chrono::milliseconds(150));
//    SendMessage(wnd->GetHWND(), WM_USER + 5, NULL, NULL);
//    for (int m = 1; m <= 40; m++) {
//        k = m;
//        SendMessage(wnd->GetHWND(), WM_USER + 3, NULL, NULL);
//        std::this_thread::sleep_for(std::chrono::milliseconds((int)((px[k] - px[k - 1]) * 800)));
//    }
//    return 0;
//}

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

//void dragdropAnimation() {
//    DWORD animThread;
//    animThreadHandle = CreateThread(0, 0, animate4, NULL, 0, &animThread);
//}

void fullscreenAnimation() {
    DWORD animThread;
    HANDLE animThreadHandle = CreateThread(0, 0, animate5, NULL, 0, &animThread);
}

void ModifyStyle(Element* elem, Event* iev)
{
    //HINSTANCE testInst = LoadLibraryW(L"imageres.dll");
    if (iev->type == Button::Click) {
        //testButton4->SetForegroundColor(ImmersiveColor);
        //UpdateCache *h{}, *h2{};
        //Value* test = iconElem->GetValue(Element::ContentProp, 1, h);
        //Value* test2 = iconElem2->GetValue(Element::ContentProp, 1, h2);
        //HICON ico = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(4), IMAGE_ICON, 48, 48, LR_SHARED);
        //HICON ico2 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(18), IMAGE_ICON, 48, 48, LR_SHARED);
        //HICON ico3 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(148), IMAGE_ICON, 48, 48, LR_SHARED);
        //HICON ico4 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(1306), IMAGE_ICON, 48, 48, LR_SHARED);
        //HICON ico5 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(101), IMAGE_ICON, 48, 48, LR_SHARED);
        //HICON ico6 = (HICON)LoadImageW(testInst, MAKEINTRESOURCE(82), IMAGE_ICON, 48, 48, LR_SHARED);
        //HBITMAP bmp = IconToBitmap(ico);
        //HBITMAP bmp2 = IconToBitmap(ico2);
        //HBITMAP bmp3 = IconToBitmap(ico3);
        //HBITMAP bmp4 = IconToBitmap(ico4);
        //HBITMAP bmp5 = IconToBitmap(ico5);
        //HBITMAP bmp6 = IconToBitmap(ico6);
        //HBITMAP bmpShadow = AddPaddingToBitmap(bmp, 4);
        //IterateBitmap(bmp, StandardBitmapPixelHandler, 1);
        //IterateBitmap(bmp2, StandardBitmapPixelHandler, 1);
        //IterateBitmap(bmp3, StandardBitmapPixelHandler, 1);
        //IterateBitmap(bmp4, StandardBitmapPixelHandler, 1);
        //IterateBitmap(bmp5, StandardBitmapPixelHandler, 1);
        //IterateBitmap(bmp6, StandardBitmapPixelHandler, 1);
        //IterateBitmap(bmpShadow, StandardBitmapPixelHandler, 0);
        //Value* bitmap = DirectUI::Value::CreateGraphic(bmp, 2, 0xffffffff, false, false, false);
        //Value* bitmap2 = DirectUI::Value::CreateGraphic(bmp2, 2, 0xffffffff, false, false, false);
        //Value* bitmap3 = DirectUI::Value::CreateGraphic(bmp3, 2, 0xffffffff, false, false, false);
        //Value* bitmap4 = DirectUI::Value::CreateGraphic(bmp4, 2, 0xffffffff, false, false, false);
        //Value* bitmap5 = DirectUI::Value::CreateGraphic(bmp5, 2, 0xffffffff, false, false, false);
        //Value* bitmap6 = DirectUI::Value::CreateGraphic(bmp6, 2, 0xffffffff, false, false, false);
        //Value* bitmapShadow = DirectUI::Value::CreateGraphic(bmpShadow, 2, 0xffffffff, false, false, false);
        //iconElem->SetValue(Element::ContentProp, 1, bitmap);
        //iconElem2->SetValue(Element::ContentProp, 1, bitmap2);
        //iconElem3->SetValue(Element::ContentProp, 1, bitmap3);
        //iconElem4->SetValue(Element::ContentProp, 1, bitmap4);
        //iconElem5->SetValue(Element::ContentProp, 1, bitmap5);
        //iconElem6->SetValue(Element::ContentProp, 1, bitmap6);
        //iconElemShadow->SetValue(Element::ContentProp, 1, bitmapShadow);
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
        //bitmap3->Release();
        //bitmap4->Release();
        //bitmap5->Release();
        //bitmap6->Release();
        //bitmapShadow->Release();
    }
}

void testEventListener(Element* elem, Event* iev) {
    static bool openclose = 1;
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
        //switch (outerElem->GetAlpha()) {
        //case 0:
        //    outerElem->SetAlpha(255);

        //    startAnimation();
        //    break;
        //case 255:
        //    outerElem->SetAlpha(0);

        //    break;
        //}
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
        unsigned int count = list_directory().size();
        wchar_t icount[32];
        swprintf_s(icount, L"        Found %d items!", count);
        itemcountstatus->SetContentString((UCString)icount);
        itemcountstatus->SetVisible(true); itemcountstatus->SetAlpha(0);
        int x = 0, y = 0;
        index = 0;
        smIndex = 0;
        CubicBezier(24, px, py, 0.1, 0.9, 0.2, 1.0);
        pm.resize(count);
        DWORD* animThread = new DWORD[count];
        DWORD* animThread2 = new DWORD[count];
        HANDLE* animThreadHandle = new HANDLE[count];
        HANDLE* animThreadHandle2 = new HANDLE[count];
        frame = new int[count]{};
        for (int i = 0; i < count; i++) {
            Element* elBuffer;
            parser->CreateElement((UCString)L"outerElem", NULL, NULL, NULL, (Element**)&elBuffer);
            mainContainer->Add((Element**)&elBuffer, 1);
            pm[index].elem = elBuffer, pm[index].x = x, pm[index].y = y;
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
        delete[] animThread;
        delete[] animThread2;
        delete[] animThreadHandle;
        delete[] animThreadHandle2;
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
    //outerElem = regElem(L"outerElem");
    //iconElem = (Element*)
    iconElemShadow = regElem(L"iconElemShadow");
    textElem = regRichText(L"textElem");
    //outerElem2 = regElem(L"outerElem2");
    //iconElem2 = regElem(L"iconElem2");
    //textElem2 = regRichText(L"textElem2");
    //outerElem3 = regElem(L"outerElem3");
    //iconElem3 = regElem(L"iconElem3");
    //textElem3 = regRichText(L"textElem3");
    //outerElem4 = regElem(L"outerElem4");
    //iconElem4 = regElem(L"iconElem4");
    //textElem4 = regRichText(L"textElem4");
    //outerElem5 = regElem(L"outerElem5");
    //iconElem5 = regElem(L"iconElem5");
    //textElem5 = regRichText(L"textElem5");
    //outerElem6 = regElem(L"outerElem6");
    //iconElem6 = regElem(L"iconElem6");
    //textElem6 = regRichText(L"textElem6");
    fullscreenpopup = regElem(L"fullscreenpopup");
    fullscreeninner = regElem(L"fullscreeninner");
    itemcountstatus = regElem(L"itemcountstatus");

    assignFn(testButton, testEventListener);
    //assignFn(testButton2, testEventListener2);
    assignFn(testButton3, testEventListener3);
    assignFn(testButton4, ModifyStyle);
    assignFn(testButton5, InitLayout);

    wnd->Host(pMain);

    wnd->ShowWindow(SW_SHOW);
    StartMessagePump();
    UnInitProcessPriv(0);

    return 0;
}