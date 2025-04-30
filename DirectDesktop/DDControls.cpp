#include "DDControls.h"
#include "BitmapHelper.h"
#include "DirectoryHelper.h"
#include "StyleModifier.h"
#include <regex>
#include <algorithm>
#include <uxtheme.h>
#include <dwmapi.h>

using namespace std;
using namespace DirectUI;

IClassInfo* DDScalableElement::s_pClassInfo;
IClassInfo* DDScalableButton::s_pClassInfo;
IClassInfo* LVItem::s_pClassInfo;
IClassInfo* DDToggleButton::s_pClassInfo;
IClassInfo* DDNotificationBanner::s_pClassInfo;
struct IntegerWrapper {
    int val;
};
NativeHWNDHost* notificationwnd{};
WNDPROC WndProc4;

HRESULT WINAPI CreateAndSetLayout(Element* pe, HRESULT (*pfnCreate)(int, int*, Value**), int dNumParams, int* pParams) {
    HRESULT hr{};
    Value* pvLayout{};
    hr = pfnCreate(dNumParams, pParams, &pvLayout);
    if (SUCCEEDED(hr)) {
        hr = pe->SetValue(Element::LayoutProp, 1, pvLayout);
        pvLayout->Release();
    }
    return hr;
}

static const int vvimpFirstScaledImageProp[] = { 1, -1 };
static PropertyInfoData dataimpFirstScaledImageProp;
static const PropertyInfo impFirstScaledImageProp =
{
    L"FirstScaledImage",
    0x2 | 0x4,
    0x1,
    vvimpFirstScaledImageProp,
    nullptr,
    Value::GetIntMinusOne,
    &dataimpFirstScaledImageProp
};
static const int vvimpScaledImageIntervalsProp[] = { 1, -1 };
static PropertyInfoData dataimpScaledImageIntervalsProp;
static const PropertyInfo impScaledImageIntervalsProp =
{
    L"ScaledImageIntervals",
    0x2 | 0x4,
    0x1,
    vvimpScaledImageIntervalsProp,
    nullptr,
    Value::GetIntMinusOne,
    &dataimpScaledImageIntervalsProp
};
static const int vvimpDrawTypeProp[] = { 1, -1 };
static PropertyInfoData dataimpDrawTypeProp;
static const PropertyInfo impDrawTypeProp =
{
    L"DrawType",
    0x2 | 0x4,
    0x1,
    vvimpDrawTypeProp,
    nullptr,
    Value::GetIntMinusOne,
    &dataimpDrawTypeProp
};
static const int vvimpEnableAccentProp[] = { 1, -1 };
static PropertyInfoData dataimpEnableAccentProp;
static const PropertyInfo impEnableAccentProp =
{
    L"EnableAccent",
    0x2 | 0x4,
    0x1,
    vvimpEnableAccentProp,
    nullptr,
    Value::GetIntMinusOne,
    &dataimpEnableAccentProp
};
static const int vvimpNeedsFontResizeProp[] = { 1, -1 };
static PropertyInfoData dataimpNeedsFontResizeProp;
static const PropertyInfo impNeedsFontResizeProp =
{
    L"NeedsFontResize",
    0x2 | 0x4,
    0x1,
    vvimpNeedsFontResizeProp,
    nullptr,
    Value::GetIntMinusOne,
    &dataimpNeedsFontResizeProp
};

void UpdateImageOnPropChange(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
    if (pProp == DDScalableElement::FirstScaledImageProp()) {
        ((DDScalableElement*)elem)->InitDrawImage();
    }
}
unsigned long DelayedDraw(LPVOID lpParam) {
    Sleep(50);
    assignExtendedFn((DDScalableElement*)lpParam, UpdateImageOnPropChange);
    ((DDScalableElement*)lpParam)->InitDrawImage();
    ((DDScalableElement*)lpParam)->InitDrawFont();
    return 0;
}
vector<DDScalableElement*> DDScalableElement::_arrCreatedElements;
DDScalableElement::DDScalableElement() {
    _arrCreatedElements.push_back(this);
}
DDScalableElement::~DDScalableElement() {
    auto toRemove = find(_arrCreatedElements.begin(), _arrCreatedElements.end(), this);
    if (toRemove != _arrCreatedElements.end()) {
        _arrCreatedElements.erase(toRemove);
    }
}
IClassInfo* DDScalableElement::GetClassInfoPtr() {
    return s_pClassInfo;
}
void DDScalableElement::SetClassInfoPtr(IClassInfo* pClass) {
    s_pClassInfo = pClass;
}
IClassInfo* DDScalableElement::GetClassInfoW() {
    return s_pClassInfo;
}
HRESULT DDScalableElement::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement) {
    HRESULT hr = CreateAndInit<DDScalableElement, int>(0, pParent, pdwDeferCookie, ppElement);
    DWORD dw;
    HANDLE drawingHandle = CreateThread(0, 0, DelayedDraw, (LPVOID)*ppElement, NULL, &dw);
    return hr;
}
HRESULT DDScalableElement::Register() {
    static const DirectUI::PropertyInfo* const rgRegisterProps[] =
    {
        &impFirstScaledImageProp,
        &impScaledImageIntervalsProp,
        &impDrawTypeProp,
        &impEnableAccentProp,
        &impNeedsFontResizeProp
    };
    return ClassInfo<DDScalableElement, Element, StandardCreator<DDScalableElement>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableElement", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
}
int DDScalableElement::GetPropCommon(const PropertyProcT pPropertyProc) {
    Value* pv = GetValue(pPropertyProc, 2, nullptr);
    int v = pv->GetInt();
    pv->Release();
    return v;
}
void DDScalableElement::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt) {
    Value* pv = Value::CreateInt(iCreateInt);
    HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr)) {
        hr = SetValue(pPropertyProc, 1, pv);
        pv->Release();
    }
}
const PropertyInfo* WINAPI DDScalableElement::FirstScaledImageProp() {
    return &impFirstScaledImageProp;
}
int DDScalableElement::GetFirstScaledImage() {
    return this->GetPropCommon(FirstScaledImageProp);
}
void DDScalableElement::SetFirstScaledImage(int iFirstImage) {
    this->SetPropCommon(FirstScaledImageProp, iFirstImage);
}
const PropertyInfo* WINAPI DDScalableElement::ScaledImageIntervalsProp() {
    return &impScaledImageIntervalsProp;
}
int DDScalableElement::GetScaledImageIntervals() {
    int v = this->GetPropCommon(ScaledImageIntervalsProp);
    if (v < 1) v = 1;
    return v;
}
void DDScalableElement::SetScaledImageIntervals(int iScaleIntervals) {
    this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals);
}
const PropertyInfo* WINAPI DDScalableElement::DrawTypeProp() {
    return &impDrawTypeProp;
}
int DDScalableElement::GetDrawType() {
    return this->GetPropCommon(DrawTypeProp);
}
void DDScalableElement::SetDrawType(int iDrawType) {
    this->SetPropCommon(DrawTypeProp, iDrawType);
}
const PropertyInfo* WINAPI DDScalableElement::EnableAccentProp() {
    return &impEnableAccentProp;
}
int DDScalableElement::GetEnableAccent() {
    return this->GetPropCommon(EnableAccentProp);
}
void DDScalableElement::SetEnableAccent(int iEnableAccent) {
    this->SetPropCommon(EnableAccentProp, iEnableAccent);
}
const PropertyInfo* WINAPI DDScalableElement::NeedsFontResizeProp() {
    return &impNeedsFontResizeProp;
}
int DDScalableElement::GetNeedsFontResize() {
    return this->GetPropCommon(NeedsFontResizeProp);
}
void DDScalableElement::SetNeedsFontResize(int iNeedsFontResize) {
    this->SetPropCommon(NeedsFontResizeProp, iNeedsFontResize);
}
void DDScalableElement::InitDrawImage() {
    SendMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)this, NULL);
}
void DDScalableElement::RedrawImages() {
    for (DDScalableElement* pe : _arrCreatedElements) {
        if (pe->GetFirstScaledImage() == -1) break;
        int scaleInterval = GetCurrentScaleInterval();
        int scaleIntervalImage = pe->GetScaledImageIntervals();
        if (scaleInterval > scaleIntervalImage - 1) scaleInterval = scaleIntervalImage - 1;
        int imageID = pe->GetFirstScaledImage() + scaleInterval;
        HBITMAP newImage = (HBITMAP)LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (newImage == nullptr) {
            newImage = LoadPNGAsBitmap(imageID);
            IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1);
        }
        if (pe->GetEnableAccent() == 1) IterateBitmap(newImage, StandardBitmapPixelHandler, 1, 0, 1);
        switch (pe->GetDrawType()) {
        case 1: {
            Value* vImage = Value::CreateGraphic(newImage, 7, 0xffffffff, true, false, false);
            if (vImage) {
                pe->SetValue(Element::BackgroundProp, 1, vImage);
                vImage->Release();
            }
            break;
        }
        case 2: {
            Value* vImage = Value::CreateGraphic(newImage, 2, 0xffffffff, true, false, false);
            if (vImage) {
                pe->SetValue(Element::ContentProp, 1, vImage);
                vImage->Release();
            }
            break;
        }
        }
        if (newImage) DeleteObject(newImage);
    }
}
void DDScalableElement::InitDrawFont() {
    SendMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)this, NULL);
}
void DDScalableElement::RedrawFonts() {
    Value* v;
    for (DDScalableElement* pe : _arrCreatedElements) {
        if (pe->GetNeedsFontResize() == true) {
            if (pe->GetFont(&v) == nullptr) break;
            wstring fontOld = pe->GetFont(&v);
            wregex fontRegex(L".*font;.*\%.*");
            bool isSysmetricFont = regex_match(fontOld, fontRegex);
            if (isSysmetricFont) {
                size_t modifier = fontOld.find(L";");
                size_t modifier2 = fontOld.find(L"%");
                wstring fontIntermediate = fontOld.substr(0, modifier + 1);
                wstring fontIntermediate2 = fontOld.substr(modifier + 1, modifier2);
                wstring fontIntermediate3 = fontOld.substr(modifier2, wcslen(fontOld.c_str()));
                int newFontSize = _wtoi(fontIntermediate2.c_str()) * dpi / dpiLaunch;
                wstring fontNew = fontIntermediate + to_wstring(newFontSize) + fontIntermediate3;
                pe->SetFont(fontNew.c_str());
            }
        }
    }
}

vector<DDScalableButton*> DDScalableButton::_arrCreatedButtons;
DDScalableButton::DDScalableButton() {
    _arrCreatedButtons.push_back(this);
}
DDScalableButton::~DDScalableButton() {
    auto toRemove = find(_arrCreatedButtons.begin(), _arrCreatedButtons.end(), this);
    if (toRemove != _arrCreatedButtons.end()) {
        _arrCreatedButtons.erase(toRemove);
    }
}
IClassInfo* DDScalableButton::GetClassInfoPtr() {
    return s_pClassInfo;
}
void DDScalableButton::SetClassInfoPtr(IClassInfo* pClass) {
    s_pClassInfo = pClass;
}
IClassInfo* DDScalableButton::GetClassInfoW() {
    return s_pClassInfo;
}
HRESULT DDScalableButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement) {
    HRESULT hr = CreateAndInit<DDScalableButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    DWORD dw;
    HANDLE drawingHandle = CreateThread(0, 0, DelayedDraw, (LPVOID)*ppElement, NULL, &dw);
    return hr;
}
HRESULT DDScalableButton::Register() {
    static const DirectUI::PropertyInfo* const rgRegisterProps[] =
    {
        &impFirstScaledImageProp,
        &impScaledImageIntervalsProp,
        &impDrawTypeProp,
        &impEnableAccentProp,
        &impNeedsFontResizeProp
    };
    return ClassInfo<DDScalableButton, Button, StandardCreator<DDScalableButton>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
}
int DDScalableButton::GetPropCommon(const PropertyProcT pPropertyProc) {
    Value* pv = GetValue(pPropertyProc, 2, nullptr);
    int v = pv->GetInt();
    pv->Release();
    return v;
}
void DDScalableButton::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt) {
    Value* pv = Value::CreateInt(iCreateInt);
    HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr)) {
        hr = SetValue(pPropertyProc, 1, pv);
        pv->Release();
    }
}
const PropertyInfo* WINAPI DDScalableButton::FirstScaledImageProp() {
    return &impFirstScaledImageProp;
}
int DDScalableButton::GetFirstScaledImage() {
    return this->GetPropCommon(FirstScaledImageProp);
}
void DDScalableButton::SetFirstScaledImage(int iFirstImage) {
    this->SetPropCommon(FirstScaledImageProp, iFirstImage);
}
const PropertyInfo* WINAPI DDScalableButton::ScaledImageIntervalsProp() {
    return &impScaledImageIntervalsProp;
}
int DDScalableButton::GetScaledImageIntervals() {
    int v = this->GetPropCommon(ScaledImageIntervalsProp);
    if (v < 1) v = 1;
    return v;
}
void DDScalableButton::SetScaledImageIntervals(int iScaleIntervals) {
    this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals);
}
const PropertyInfo* WINAPI DDScalableButton::DrawTypeProp() {
    return &impDrawTypeProp;
}
int DDScalableButton::GetDrawType() {
    return this->GetPropCommon(DrawTypeProp);
}
void DDScalableButton::SetDrawType(int iDrawType) {
    this->SetPropCommon(DrawTypeProp, iDrawType);
}
const PropertyInfo* WINAPI DDScalableButton::EnableAccentProp() {
    return &impEnableAccentProp;
}
int DDScalableButton::GetEnableAccent() {
    return this->GetPropCommon(EnableAccentProp);
}
void DDScalableButton::SetEnableAccent(int iEnableAccent) {
    this->SetPropCommon(EnableAccentProp, iEnableAccent);
}
const PropertyInfo* WINAPI DDScalableButton::NeedsFontResizeProp() {
    return &impNeedsFontResizeProp;
}
int DDScalableButton::GetNeedsFontResize() {
    return this->GetPropCommon(NeedsFontResizeProp);
}
void DDScalableButton::SetNeedsFontResize(int iNeedsFontResize) {
    this->SetPropCommon(NeedsFontResizeProp, iNeedsFontResize);
}
void DDScalableButton::InitDrawImage() {
    SendMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)this, NULL);
}
void DDScalableButton::RedrawImages() {
    for (DDScalableButton* pe : _arrCreatedButtons) {
        if (pe->GetFirstScaledImage() == -1) break;
        int scaleInterval = GetCurrentScaleInterval();
        int scaleIntervalImage = pe->GetScaledImageIntervals();
        if (scaleInterval > scaleIntervalImage - 1) scaleInterval = scaleIntervalImage - 1;
        int imageID = pe->GetFirstScaledImage() + scaleInterval;
        HBITMAP newImage = (HBITMAP)LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (newImage == nullptr) {
            newImage = LoadPNGAsBitmap(imageID);
            IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1);
        }
        if (pe->GetEnableAccent() == 1) IterateBitmap(newImage, StandardBitmapPixelHandler, 1, 0, 1);
        switch (pe->GetDrawType()) {
        case 1: {
            Value* vImage = Value::CreateGraphic(newImage, 7, 0xffffffff, true, false, false);
            pe->SetValue(Element::BackgroundProp, 1, vImage);
            vImage->Release();
            break;
        }
        case 2: {
            Value* vImage = Value::CreateGraphic(newImage, 2, 0xffffffff, true, false, false);
            pe->SetValue(Element::ContentProp, 1, vImage);
            vImage->Release();
            break;
        }
        }
        if (newImage) DeleteObject(newImage);
    }
}
void DDScalableButton::InitDrawFont() {
    SendMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)this, NULL);
}
void DDScalableButton::RedrawFonts() {
    Value* v;
    for (DDScalableButton* pe : _arrCreatedButtons) {
        if (pe->GetNeedsFontResize() == true) {
            if (pe->GetFont(&v) == nullptr) break;
            wstring fontOld = pe->GetFont(&v);
            wregex fontRegex(L".*font;.*\%.*");
            bool isSysmetricFont = regex_match(fontOld, fontRegex);
            if (isSysmetricFont) {
                size_t modifier = fontOld.find(L";");
                size_t modifier2 = fontOld.find(L"%");
                wstring fontIntermediate = fontOld.substr(0, modifier + 1);
                wstring fontIntermediate2 = fontOld.substr(modifier + 1, modifier2);
                wstring fontIntermediate3 = fontOld.substr(modifier2, wcslen(fontOld.c_str()));
                int newFontSize = _wtoi(fontIntermediate2.c_str()) * dpi / dpiLaunch;
                wstring fontNew = fontIntermediate + to_wstring(newFontSize) + fontIntermediate3;
                pe->SetFont(fontNew.c_str());
            }
        }
    }
}
RegKeyValue DDScalableButton::GetRegKeyValue() {
    return _rkv;
}
void(*DDScalableButton::GetAssociatedFn())(bool, bool) {
    return _assocFn;
}
bool* DDScalableButton::GetAssociatedBool() {
    return _assocBool;
}
void DDScalableButton::SetRegKeyValue(RegKeyValue rkvNew) {
    _rkv = rkvNew;
}
void DDScalableButton::SetAssociatedFn(void(*pfn)(bool, bool)) {
    _assocFn = pfn;
}
void DDScalableButton::SetAssociatedBool(bool* pb) {
    _assocBool = pb;
}
void DDScalableButton::ExecAssociatedFn(void(*pfn)(bool, bool), bool fnb1, bool fnb2) {
    pfn(fnb1, fnb2);
}

IClassInfo* LVItem::GetClassInfoPtr() {
    return s_pClassInfo;
}
void LVItem::SetClassInfoPtr(IClassInfo* pClass) {
    s_pClassInfo = pClass;
}
IClassInfo* LVItem::GetClassInfoW() {
    return s_pClassInfo;
}
HRESULT LVItem::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement) {
    HRESULT hr = CreateAndInit<LVItem, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    DWORD dw;
    HANDLE drawingHandle = CreateThread(0, 0, DelayedDraw, (LPVOID)*ppElement, NULL, &dw);
    return hr;
}
HRESULT LVItem::Register() {
    return ClassInfo<LVItem, DDScalableButton, StandardCreator<LVItem>>::RegisterGlobal(HINST_THISCOMPONENT, L"LVItem", nullptr, 0);
}
unsigned short LVItem::GetInternalXPos() {
    return _xPos;
}
unsigned short LVItem::GetInternalYPos() {
    return _yPos;
}
void LVItem::SetInternalXPos(unsigned short iXPos) {
    _xPos = iXPos;
}
void LVItem::SetInternalYPos(unsigned short iYPos) {
    _yPos = iYPos;
}
wstring LVItem::GetFilename() {
    return _filename;
}
wstring LVItem::GetSimpleFilename() {
    return _simplefilename;
}
void LVItem::SetFilename(const wstring& wsFilename) {
    _filename = wsFilename;
}
void LVItem::SetSimpleFilename(const wstring& wsSimpleFilename) {
    _simplefilename = wsSimpleFilename;
}
bool LVItem::GetDirState() {
    return _isDirectory;
}
bool LVItem::GetHiddenState() {
    return _isHidden;
}
bool LVItem::GetMemorySelected() {
    return _mem_isSelected;
}
bool LVItem::GetShortcutState() {
    return _isShortcut;
}
bool LVItem::GetColorLock() {
    return _colorLock;
}
bool LVItem::GetDragState() {
    return _dragged;
}
void LVItem::SetDirState(bool dirState) {
    _isDirectory = dirState;
}
void LVItem::SetHiddenState(bool hiddenState) {
    _isHidden = hiddenState;
}
void LVItem::SetMemorySelected(bool mem_isSelectedState) {
    _mem_isSelected = mem_isSelectedState;
}
void LVItem::SetShortcutState(bool shortcutState) {
    _isShortcut = shortcutState;
}
void LVItem::SetColorLock(bool colorLockState) {
    _colorLock = colorLockState;
}
void LVItem::SetDragState(bool dragstate) {
    _dragged = dragstate;
}
unsigned short LVItem::GetPage() {
    return _page;
}
void LVItem::SetPage(unsigned short pageID) {
    _page = pageID;
}

IClassInfo* DDToggleButton::GetClassInfoPtr() {
    return s_pClassInfo;
}
void DDToggleButton::SetClassInfoPtr(IClassInfo* pClass) {
    s_pClassInfo = pClass;
}
IClassInfo* DDToggleButton::GetClassInfoW() {
    return s_pClassInfo;
}
HRESULT DDToggleButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement) {
    HRESULT hr = CreateAndInit<DDToggleButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    DWORD dw;
    HANDLE drawingHandle = CreateThread(0, 0, DelayedDraw, (LPVOID)*ppElement, NULL, &dw);
    return hr;
}
HRESULT DDToggleButton::Register() {
    return ClassInfo<DDToggleButton, DDScalableButton, StandardCreator<DDToggleButton>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDToggleButton", nullptr, 0);
}

LRESULT CALLBACK NotificationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Value* v{};
    Element* ppeTemp;
    wstring fontOld;
    wregex fontRegex(L".*font;.*\%.*");
    bool isSysmetricFont;
    switch (uMsg) {
    case WM_CLOSE:
        //DDNotificationBanner::DestroyBanner(nullptr);
        return 0;
        break;
    case WM_DESTROY:
        return 0;
        break;
    case WM_USER + 2:
        ppeTemp = (Element*)wParam;
        if (ppeTemp != nullptr) fontOld = ppeTemp->GetFont(&v);
        isSysmetricFont = regex_match(fontOld, fontRegex);
        if (isSysmetricFont) {
            size_t modifier = fontOld.find(L";");
            size_t modifier2 = fontOld.find(L"%");
            wstring fontIntermediate = fontOld.substr(0, modifier + 1);
            wstring fontIntermediate2 = fontOld.substr(modifier + 1, modifier2);
            wstring fontIntermediate3 = fontOld.substr(modifier2, wcslen(fontOld.c_str()));
            int newFontSize = _wtoi(fontIntermediate2.c_str()) * dpi / dpiLaunch;
            wstring fontNew = fontIntermediate + to_wstring(newFontSize) + fontIntermediate3;
            ppeTemp->SetFont(fontNew.c_str());
        }
        break;
    case WM_USER + 3:
        DDNotificationBanner::DestroyBanner(nullptr);
        break;
    }
    return CallWindowProc(WndProc4, hWnd, uMsg, wParam, lParam);
}
unsigned long AnimateWindowWrapper(LPVOID lpParam) {
    Sleep(50);
    AnimateWindow(notificationwnd->GetHWND(), 180, AW_BLEND);
    notificationwnd->ShowWindow(SW_SHOW);
    return 0;
}
unsigned long AutoCloseNotification(LPVOID lpParam) {
    IntegerWrapper* iwTemp = (IntegerWrapper*)lpParam;
    Sleep(iwTemp->val * 1000);
    SendMessageW(notificationwnd->GetHWND(), WM_USER + 3, NULL, NULL);
    return 0;
}
vector<wstring> SplitLineBreaks(const wstring& originalstr) {
    vector<wstring> strs;
    size_t start = 0;
    size_t end = originalstr.find(L'\n');
    while (end != wstring::npos) {
        strs.push_back(originalstr.substr(start, end - start));
        start = end + 1;
        end = originalstr.find(L'\n', start);
    }
    strs.push_back(originalstr.substr(start));
    return strs;
}
int CalcLines(const wstring& textStr) {
    int lines = count(textStr.begin(), textStr.end(), L'\n') + 1;
    return lines;
}
void GetLongestLine(HDC hdc, const wstring& textStr, RECT* rcText) {
    static int lines = CalcLines(textStr);
    vector<wstring> divided = SplitLineBreaks(textStr);
    int saved{};
    for (int i = 0; i < lines; i++) {
        DrawTextW(hdc, divided[i].c_str(), -1, rcText, DT_CALCRECT | DT_SINGLELINE);
        if (rcText->right > saved) saved = rcText->right;
    }
    rcText->right = saved;
}
unsigned long AutoSizeFont(LPVOID lpParam) {
    InitThread(TSM_DESKTOP_DYNAMIC);
    Sleep(50);
    Element* ppeTemp = (Element*)lpParam;
    if (!ppeTemp || notificationwnd == nullptr) return 1;
    SendMessageW(notificationwnd->GetHWND(), WM_USER + 2, (WPARAM)ppeTemp, NULL);
    return 0;
}
IClassInfo* DDNotificationBanner::GetClassInfoPtr() {
    return s_pClassInfo;
}
void DDNotificationBanner::SetClassInfoPtr(IClassInfo* pClass) {
    s_pClassInfo = pClass;
}
IClassInfo* DDNotificationBanner::GetClassInfoW() {
    return s_pClassInfo;
}
HRESULT DDNotificationBanner::Create(HWND hParent, bool fDblBuffer, UINT nCreate, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement) {
    HRESULT hr{};
    HANDLE ProcessHeap{};
    DDNotificationBanner* buffer{};
    DDNotificationBanner* pResult{};
    ProcessHeap = GetProcessHeap();
    buffer = (DDNotificationBanner*)HeapAlloc(ProcessHeap, 0x8, sizeof(DDNotificationBanner));
    pResult = buffer;
    if (buffer) {
        hr = pResult->Initialize(hParent, fDblBuffer, nCreate, pParent, pdwDeferCookie);
        if (FAILED(hr)) {
            pResult->Destroy(true);
            pResult = nullptr;
        }
    }
    else {
        hr = E_OUTOFMEMORY;
    }
    *ppElement = pResult;
    return hr;
}
HRESULT DDNotificationBanner::Register() {
    return ClassInfo<DDNotificationBanner, HWNDElement, EmptyCreator<DDNotificationBanner>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDNotificationBanner", nullptr, 0);
}
Element* DDNotificationBanner::GetIconElement() {
    return _icon;
}
DDScalableElement* DDNotificationBanner::GetTitleElement() {
    return _title;
}
DDScalableElement* DDNotificationBanner::GetContentElement() {
    return _content;
}
void DDNotificationBanner::CreateBanner(DDNotificationBanner* pDDNB, DUIXmlParser* pParser, DDNotificationType type, LPCWSTR pszResID, LPCWSTR title, LPCWSTR content, short timeout, bool fClose) {
    static bool notificationopen{};
    static HANDLE AutoCloseHandle;
    if (notificationopen) DestroyBanner(&notificationopen);
    unsigned long keyN{};
    Element* pHostElement;
    RECT dimensions;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    NativeHWNDHost::Create(L"DD_NotificationHost", L"DirectDesktop In-App Notification", NULL, NULL, 0, 0, 0, 0, NULL, WS_POPUP | WS_BORDER, HINST_THISCOMPONENT, 0, &notificationwnd);
    HWNDElement::Create(notificationwnd->GetHWND(), true, NULL, NULL, &keyN, (Element**)&pDDNB);
    ITaskbarList* pTaskbarList = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskbarList, (void**)&pTaskbarList))) {
        if (SUCCEEDED(pTaskbarList->HrInit())) {
            pTaskbarList->DeleteTab(notificationwnd->GetHWND());
        }
        pTaskbarList->Release();
    }
    pParser->CreateElement(pszResID, pDDNB, NULL, NULL, &pHostElement);
    WndProc4 = (WNDPROC)SetWindowLongPtrW(notificationwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)NotificationProc);
    pHostElement->SetVisible(true);
    pHostElement->EndDefer(keyN);
    notificationwnd->Host(pHostElement);
    int WindowsBuild = _wtoi(GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber"));
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(notificationwnd->GetHWND(), &margins);
    if (WindowsBuild >= 22000) {
        DWORD cornerPreference = DWMWCP_ROUND;
        DwmSetWindowAttribute(notificationwnd->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
    }
    BlurBackground(notificationwnd->GetHWND(), true, false);
    pHostElement->SetBackgroundStdColor(7);
    Value* v{};
    pDDNB->GetPadding(&v);
    pHostElement->SetValue(Element::PaddingProp, 1, v);
    SetForegroundWindow(notificationwnd->GetHWND());
    CreateAndSetLayout(pHostElement, BorderLayout::Create, 0, 0);

    int cx{}, cy{};
    RECT hostpadding = *(v->GetRect());
    cx += (hostpadding.left + hostpadding.right + 48 * flScaleFactor); // 48: 28 is the icon width, 20 is extra padding
    cy += (hostpadding.top + hostpadding.bottom);
    Element* peTemp = pDDNB->GetIconElement();
    CreateAndInit<Element, int>(0, pHostElement, 0, (Element**)&peTemp);
    peTemp->SetID(L"DDNB_Icon");
    pHostElement->Add(&peTemp, 1);
    wstring titleStr{};
    switch (type) {
    case DDNT_SUCCESS:
        if (!title) titleStr = LoadStrFromRes(217);
        peTemp->SetClass(L"DDNB_Icon_Success");
        break;
    case DDNT_INFO:
        if (!title) titleStr = LoadStrFromRes(218);
        peTemp->SetClass(L"DDNB_Icon_Info");
        break;
    case DDNT_WARNING:
        if (!title) titleStr = LoadStrFromRes(219);
        peTemp->SetClass(L"DDNB_Icon_Warning");
        break;
    case DDNT_ERROR:
        if (!title) titleStr = LoadStrFromRes(220);
        peTemp->SetClass(L"DDNB_Icon_Error");
        break;
    }
    if (title) titleStr = title;

    HDC hdcMem = CreateCompatibleDC(NULL);
    NONCLIENTMETRICSW ncm{};
    TEXTMETRICW tm{};
    SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, NULL, dpi);
    HFONT hFont = CreateFontIndirectW(&(ncm.lfMessageFont));
    SelectObject(hdcMem, hFont);
    RECT rcText{}, rcText2{};
    GetLongestLine(hdcMem, content, &rcText2);
    GetTextMetricsW(hdcMem, &tm);
    cy += (ceil(tm.tmHeight * 1.15) * CalcLines(content)) * flScaleFactor;
    ncm.lfMessageFont.lfWeight = 700;
    hFont = CreateFontIndirectW(&(ncm.lfMessageFont));
    SelectObject(hdcMem, hFont);
    DrawTextW(hdcMem, title, -1, &rcText, DT_CALCRECT | DT_SINGLELINE);
    cx += (ceil(max(rcText.right, rcText2.right) * 1.15) * flScaleFactor);
    GetTextMetricsW(hdcMem, &tm);
    cy += (ceil(tm.tmHeight * 1.15) + 6) * flScaleFactor;
    DeleteObject(hFont);
    DeleteDC(hdcMem);

    peTemp = pDDNB->GetTitleElement();
    CreateAndInit<Element, int>(0, pHostElement, 0, (Element**)&peTemp);
    peTemp->SetID(L"DDNB_Title");
    pHostElement->Add(&peTemp, 1);
    HANDLE setFontStr = CreateThread(0, 0, AutoSizeFont, peTemp, 0, NULL);
    peTemp->SetContentString(titleStr.c_str());

    peTemp = pDDNB->GetContentElement();
    CreateAndInit<Element, int>(0, pHostElement, 0, (Element**)&peTemp);
    peTemp->SetID(L"DDNB_Content");
    pHostElement->Add(&peTemp, 1);
    HANDLE setFontStr2 = CreateThread(0, 0, AutoSizeFont, peTemp, 0, NULL);
    peTemp->SetContentString(content);

    LPWSTR sheetName = theme ? (LPWSTR)L"default" : (LPWSTR)L"defaultdark";
    StyleSheet* sheet = pHostElement->GetSheet();
    Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
    pParser->GetSheet(sheetName, &sheetStorage);
    pHostElement->SetValue(Element::SheetProp, 1, sheetStorage);

    // Window borders
    cx += (round(flScaleFactor)) * 2;
    cy += (round(flScaleFactor)) * 2;

    SetWindowPos(notificationwnd->GetHWND(), HWND_TOPMOST, (dimensions.left + dimensions.right - cx) / 2, 40 * flScaleFactor, cx, cy, SWP_FRAMECHANGED);
    notificationopen = true;
    IntegerWrapper* iw = new IntegerWrapper{ timeout };
    HANDLE AnimHandle = CreateThread(0, 0, AnimateWindowWrapper, &notificationopen, NULL, NULL);
    if (timeout > 0) {
        TerminateThread(AutoCloseHandle, 1);
        DWORD dwAutoClose;
        AutoCloseHandle = CreateThread(0, 0, AutoCloseNotification, iw, NULL, &dwAutoClose);
    }
}
void DDNotificationBanner::DestroyBanner(bool* notificationopen) {
    if (notificationwnd != nullptr) {
        AnimateWindow(notificationwnd->GetHWND(), 120, AW_BLEND | AW_HIDE);
        notificationwnd->DestroyWindow();
        notificationwnd = nullptr;
    }
    if (notificationopen != nullptr) *notificationopen = false;
}