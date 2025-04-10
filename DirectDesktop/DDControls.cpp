#include "DDControls.h"
#include "BitmapHelper.h"
#include "StyleModifier.h"
#include <regex>

using namespace std;
using namespace DirectUI;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)


IClassInfo* DDScalableElement::s_pClassInfo;
IClassInfo* DDScalableButton::s_pClassInfo;
IClassInfo* LVItem::s_pClassInfo;
IClassInfo* DDToggleButton::s_pClassInfo;

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
    Sleep(150);
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