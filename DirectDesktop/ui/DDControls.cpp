#include "pch.h"

#include "DDControls.h"
#include "..\DirectDesktop.h"
#include "..\coreui\BitmapHelper.h"
#include "..\backend\DirectoryHelper.h"
#include "..\coreui\StyleModifier.h"
#include <wrl.h>

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    extern DDScalableButton* fullscreeninner;

    IClassInfo* DDScalableElement::s_pClassInfo;
    IClassInfo* DDScalableButton::s_pClassInfo;
    IClassInfo* DDScalableRichText::s_pClassInfo;
    IClassInfo* DDScalableTouchEdit::s_pClassInfo;
    IClassInfo* LVItem::s_pClassInfo;
    IClassInfo* DDLVActionButton::s_pClassInfo;
    IClassInfo* DDToggleButton::s_pClassInfo;
    IClassInfo* DDCheckBox::s_pClassInfo;
    IClassInfo* DDCheckBoxGlyph::s_pClassInfo;
    IClassInfo* DDColorPicker::s_pClassInfo;
    IClassInfo* DDColorPickerButton::s_pClassInfo;
    IClassInfo* DDNotificationBanner::s_pClassInfo;

    struct IntegerWrapper
    {
        int val;
    };

    NativeHWNDHost* notificationwnd{};
    WNDPROC WndProcNotification;

    HRESULT WINAPI CreateAndSetLayout(Element* pe, HRESULT (*pfnCreate)(int, int*, Value**), int dNumParams, int* pParams)
    {
        HRESULT hr{};
        CValuePtr spvLayout{};
        hr = pfnCreate(dNumParams, pParams, &spvLayout);
        if (SUCCEEDED(hr)) hr = pe->SetValue(Element::LayoutProp, 1, spvLayout);
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
    static const int vvimpEnableAccentProp[] = { 2, -1 };
    static PropertyInfoData dataimpEnableAccentProp;
    static const PropertyInfo impEnableAccentProp =
    {
        L"EnableAccent",
        0x2 | 0x4,
        0x1,
        vvimpEnableAccentProp,
        nullptr,
        Value::GetBoolFalse,
        &dataimpEnableAccentProp
    };
    static const int vvimpNeedsFontResizeProp[] = { 2, -1 };
    static PropertyInfoData dataimpNeedsFontResizeProp;
    static const PropertyInfo impNeedsFontResizeProp =
    {
        L"NeedsFontResize",
        0x2 | 0x4,
        0x1,
        vvimpNeedsFontResizeProp,
        nullptr,
        Value::GetBoolFalse,
        &dataimpNeedsFontResizeProp
    };
    static const int vvimpNeedsFontResize2Prop[] = { 2, -1 };
    static PropertyInfoData dataimpNeedsFontResize2Prop;
    static const PropertyInfo impNeedsFontResize2Prop =
    {
        L"NeedsFontResize2",
        0x2 | 0x4,
        0x1,
        vvimpNeedsFontResize2Prop,
        nullptr,
        Value::GetBoolFalse,
        &dataimpNeedsFontResize2Prop
    };
    static const int vvimpCheckedStateProp[] = { 2, -1 };
    static PropertyInfoData dataimpCheckedStateProp;
    static const PropertyInfo impCheckedStateProp =
    {
        L"CheckedState",
        0x2 | 0x4,
        0x1,
        vvimpCheckedStateProp,
        nullptr,
        Value::GetBoolFalse,
        &dataimpCheckedStateProp
    };
    static const int vvimpAssociatedColorProp[] = { 1, -1 };
    static PropertyInfoData dataimpAssociatedColorProp;
    static const PropertyInfo impAssociatedColorProp =
    {
        L"AssociatedColor",
        0x2 | 0x4,
        0x1,
        vvimpAssociatedColorProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpAssociatedColorProp
    };
    static const int vvimpColorIntensityProp[] = { 1, -1 };
    static PropertyInfoData dataimpColorIntensityProp;
    static const PropertyInfo impColorIntensityProp =
    {
        L"ColorIntensity",
        0x2 | 0x4,
        0x1,
        vvimpColorIntensityProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpColorIntensityProp
    };
    static const int vvimpDefaultColorProp[] = { 1, -1 };
    static PropertyInfoData dataimpDefaultColorProp;
    static const PropertyInfo impDefaultColorProp =
    {
        L"DefaultColor",
        0x2 | 0x4,
        0x1,
        vvimpDefaultColorProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpDefaultColorProp
    };

    void RedrawImageCore(DDScalableElement* pe)
    {
        if (!pe || pe->GetFirstScaledImage() == -1) return;
        int scaleInterval = GetCurrentScaleInterval();
        int scaleIntervalImage = pe->GetScaledImageIntervals();
        if (scaleInterval > scaleIntervalImage - 1) scaleInterval = scaleIntervalImage - 1;
        int imageID = pe->GetFirstScaledImage() + scaleInterval;
        HBITMAP newImage = (HBITMAP)LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (newImage == nullptr)
        {
            LoadPNGAsBitmap(newImage, imageID);
            IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1, NULL);
        }
        if (pe->GetAssociatedColor() != -1) IterateBitmap(newImage, StandardBitmapPixelHandler, 3, 0, pe->GetDDCPIntensity() / 255.0, pe->GetAssociatedColor());
        else if (pe->GetEnableAccent()) IterateBitmap(newImage, StandardBitmapPixelHandler, 1, 0, 1, ImmersiveColor);
        switch (pe->GetDrawType())
        {
            case 1:
            {
                CValuePtr vImage = Value::CreateGraphic(newImage, 7, 0xffffffff, true, false, false);
                if (vImage) pe->SetValue(Element::BackgroundProp, 1, vImage);
                break;
            }
            case 2:
            {
                CValuePtr vImage = Value::CreateGraphic(newImage, 2, 0xffffffff, true, false, false);
                if (vImage) pe->SetValue(Element::ContentProp, 1, vImage);
                break;
            }
        }
        if (newImage) DeleteObject(newImage);
    }

    void RedrawFontCore(DDScalableElement* pe)
    {
        if (!pe) return;
        CValuePtr v;
        if (pe->GetNeedsFontResize())
        {
            if (pe->GetFont(&v) == nullptr) return;
            wstring fontOld = pe->GetFont(&v);
            wregex fontRegex(L".*font;.*\%.*");
            bool isSysmetricFont = regex_match(fontOld, fontRegex);
            if (isSysmetricFont)
            {
                size_t modifier = fontOld.find(L";");
                size_t modifier2 = fontOld.find(L"%");
                wstring fontIntermediate = fontOld.substr(0, modifier + 1);
                wstring fontIntermediate2 = fontOld.substr(modifier + 1, modifier2);
                wstring fontIntermediate3 = fontOld.substr(modifier2, wcslen(fontOld.c_str()));
                int newFontSize = _wtoi(fontIntermediate2.c_str()) * g_dpi / static_cast<float>(g_dpiLaunch);
                wstring fontNew = fontIntermediate + to_wstring(newFontSize) + fontIntermediate3;
                pe->SetFont(fontNew.c_str());
            }
            else if (pe->GetFontSize() > 0)
            {
                pe->SetFontSize(pe->GetFontSize() * g_flScaleFactor);
            }
        }
        else if (pe->GetNeedsFontResize2() && g_dpiLaunch != 96 && g_dpi == 96)
        {
            if (pe->GetFont(&v) == nullptr) return;
            wstring fontOld = pe->GetFont(&v);
            wregex fontRegex(L".*font;.*\%.*");
            bool isSysmetricFont = regex_match(fontOld, fontRegex);
            if (isSysmetricFont)
            {
                size_t modifier = fontOld.find(L";");
                size_t modifier2 = fontOld.find(L"%");
                wstring fontIntermediate = fontOld.substr(0, modifier + 1);
                wstring fontIntermediate2 = fontOld.substr(modifier + 1, modifier2);
                wstring fontIntermediate3 = fontOld.substr(modifier2, wcslen(fontOld.c_str()));
                int newFontSize = _wtoi(fontIntermediate2.c_str()) * 1;
                wstring fontNew = fontIntermediate + to_wstring(newFontSize) + fontIntermediate3;
                pe->SetFont(fontNew.c_str());
            }
        }
    }

    void UpdateImageOnPropChange(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == DDScalableElement::FirstScaledImageProp() || pProp == DDScalableElement::DrawTypeProp() || pProp == DDScalableElement::AssociatedColorProp())
        {
            if (elem) ((DDScalableElement*)elem)->InitDrawImage();
        }
    }

    void UpdateGlyphOnPress(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        CSafeElementPtr<DDCheckBoxGlyph> glyph;
        glyph.Assign((DDCheckBoxGlyph*)elem->GetParent()->FindDescendent(StrToID(L"DDCB_Glyph")));
        if ((pProp == Button::PressedProp() || pProp == Button::MouseWithinProp()) && glyph)
        {
            glyph->SetSelected(((Button*)elem)->GetPressed());
        }
        if (pProp == DDCheckBox::CheckedStateProp() && glyph)
        {
            ((DDCheckBoxGlyph*)glyph)->SetCheckedState(((DDCheckBox*)elem)->GetCheckedState());
        }
    }

    void UpdateUICtrlColor(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDScalableElement> DDCPCC;
            DDCPCC.Assign(regElem<DDScalableElement*>(L"DDColorPicker_CheckedCircle", elem->GetParent()));
            DDCPCC->SetX(elem->GetX());
            vector<DDScalableElement*> te = ((DDColorPickerButton*)elem)->GetTargetElements();
            RegKeyValue rkv = ((DDColorPicker*)elem->GetParent())->GetRegKeyValue();
            if (rkv._hKeyName != nullptr)
            {
                rkv._dwValue = ((DDColorPickerButton*)elem)->GetOrder();
                SetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind, rkv._dwValue, false, nullptr);
            }
            for (int i = 0; i < te.size(); i++)
            {
                if (te[i])
                {
                    (te[i])->SetDDCPIntensity(((DDColorPicker*)elem->GetParent())->GetColorIntensity());
                    (te[i])->SetAssociatedColor(((DDColorPickerButton*)elem)->GetAssociatedColor());
                    if (((DDColorPicker*)elem->GetParent())->GetThemeAwareness() == true) te[i]->SetGroupColor(((DDColorPickerButton*)elem)->GetOrder());
                }
            }
            te.clear();
        }
    }

    void ShowHoverCircle(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Button::MouseFocusedProp())
        {
            CSafeElementPtr<DDScalableElement> DDCPHC;
            DDCPHC.Assign(regElem<DDScalableElement*>(L"DDColorPicker_HoverCircle", elem->GetParent()));
            DDCPHC->SetVisible(elem->GetMouseFocused());
            DDCPHC->SetX(elem->GetX());
        }
    }

    DWORD WINAPI CreateCBInnerElements(LPVOID lpParam)
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 3, (WPARAM)lpParam, NULL);
        assignExtendedFn((Element*)lpParam, UpdateGlyphOnPress);
        return 0;
    }

    DWORD WINAPI ColorPickerLayout(LPVOID lpParam)
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 4, (WPARAM)lpParam, NULL);
        return 0;
    }

    DWORD WINAPI PickerBtnFn(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        assignFn((Element*)lpParam, UpdateUICtrlColor);
        UnInitThread();
        return 0;
    }

    DWORD WINAPI CreateTEVisual(LPVOID lpParam)
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 5, (WPARAM)lpParam, NULL);
        return 0;
    }

    vector<DDScalableElement*> DDScalableElement::_arrCreatedElements;

    DDScalableElement::DDScalableElement()
    {
        _arrCreatedElements.push_back(this);
    }

    DDScalableElement::~DDScalableElement()
    {
        this->RemoveListener(_pelPropChange);
        free(_pelPropChange);
        _pelPropChange = nullptr;
        auto toRemove = find(_arrCreatedElements.begin(), _arrCreatedElements.end(), this);
        if (toRemove != _arrCreatedElements.end())
        {
            _arrCreatedElements.erase(toRemove);
        }
        this->DestroyAll(true);
    }

    IClassInfo* DDScalableElement::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDScalableElement::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDScalableElement::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDScalableElement::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDScalableElement, int>(0, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableElement*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableElement*)*ppElement)->InitDrawImage();
            ((DDScalableElement*)*ppElement)->InitDrawFont();
        }
        return hr;
    }

    HRESULT DDScalableElement::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impNeedsFontResize2Prop,
            &impAssociatedColorProp
        };
        return ClassInfo<DDScalableElement, Element, StandardCreator<DDScalableElement>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableElement", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    void DDScalableElement::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
    }

    auto DDScalableElement::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        if (!this) return -1;
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        auto v = useInt ? pv->GetInt() : pv->GetBool();
        pv->Release();
        return v;
    }

    void DDScalableElement::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt)
    {
        Value* pv = useInt ? Value::CreateInt(iCreateInt) : Value::CreateBool(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    const PropertyInfo* WINAPI DDScalableElement::FirstScaledImageProp()
    {
        return &impFirstScaledImageProp;
    }

    int DDScalableElement::GetFirstScaledImage()
    {
        return this->GetPropCommon(FirstScaledImageProp, true);
    }

    void DDScalableElement::SetFirstScaledImage(int iFirstImage)
    {
        this->SetPropCommon(FirstScaledImageProp, iFirstImage, true);
    }

    const PropertyInfo* WINAPI DDScalableElement::ScaledImageIntervalsProp()
    {
        return &impScaledImageIntervalsProp;
    }

    int DDScalableElement::GetScaledImageIntervals()
    {
        int v = this->GetPropCommon(ScaledImageIntervalsProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableElement::SetScaledImageIntervals(int iScaleIntervals)
    {
        this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals, true);
    }

    const PropertyInfo* WINAPI DDScalableElement::DrawTypeProp()
    {
        return &impDrawTypeProp;
    }

    int DDScalableElement::GetDrawType()
    {
        return this->GetPropCommon(DrawTypeProp, true);
    }

    void DDScalableElement::SetDrawType(int iDrawType)
    {
        this->SetPropCommon(DrawTypeProp, iDrawType, true);
    }

    const PropertyInfo* WINAPI DDScalableElement::EnableAccentProp()
    {
        return &impEnableAccentProp;
    }

    bool DDScalableElement::GetEnableAccent()
    {
        return this->GetPropCommon(EnableAccentProp, false);
    }

    void DDScalableElement::SetEnableAccent(bool bEnableAccent)
    {
        this->SetPropCommon(EnableAccentProp, bEnableAccent, false);
    }

    const PropertyInfo* WINAPI DDScalableElement::NeedsFontResizeProp()
    {
        return &impNeedsFontResizeProp;
    }

    bool DDScalableElement::GetNeedsFontResize()
    {
        return this->GetPropCommon(NeedsFontResizeProp, false);
    }

    void DDScalableElement::SetNeedsFontResize(bool bNeedsFontResize)
    {
        this->SetPropCommon(NeedsFontResizeProp, bNeedsFontResize, false);
    }

    const PropertyInfo* WINAPI DDScalableElement::NeedsFontResize2Prop()
    {
        return &impNeedsFontResize2Prop;
    }

    bool DDScalableElement::GetNeedsFontResize2()
    {
        return this->GetPropCommon(NeedsFontResize2Prop, false);
    }

    void DDScalableElement::SetNeedsFontResize2(bool bNeedsFontResize2)
    {
        this->SetPropCommon(NeedsFontResize2Prop, bNeedsFontResize2, false);
    }

    const PropertyInfo* WINAPI DDScalableElement::AssociatedColorProp()
    {
        return &impAssociatedColorProp;
    }

    int DDScalableElement::GetAssociatedColor()
    {
        return this->GetPropCommon(AssociatedColorProp, true);
    }

    void DDScalableElement::SetAssociatedColor(int iAssociatedColor)
    {
        this->SetPropCommon(AssociatedColorProp, iAssociatedColor, true);
    }

    int DDScalableElement::GetDDCPIntensity()
    {
        return _intensity;
    }

    unsigned short DDScalableElement::GetGroupColor()
    {
        return _gc;
    }

    void DDScalableElement::SetDDCPIntensity(int intensity)
    {
        _intensity = intensity;
    }

    void DDScalableElement::SetGroupColor(unsigned short sGC)
    {
        _gc = sGC;
    }

    void DDScalableElement::InitDrawImage()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableElement::RedrawImages()
    {
        for (DDScalableElement* pe : _arrCreatedElements)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableElement::InitDrawFont()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableElement::RedrawFonts()
    {
        for (DDScalableElement* pe : _arrCreatedElements)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)pe, NULL);
        }
    }

    vector<DDScalableButton*> DDScalableButton::_arrCreatedButtons;

    DDScalableButton::DDScalableButton()
    {
        _arrCreatedButtons.push_back(this);
    }

    DDScalableButton::~DDScalableButton()
    {
        ///////////////////// Will crash if any instance of DDLVActionButton is destroyed (tested on pages UI and folder UI)
        //if (_pelPropChange) {
        //	this->RemoveListener(_pelPropChange);
        //	free(_pelPropChange);
        //	_pelPropChange = nullptr;
        //}
        /////////////////////
        auto toRemove = find(_arrCreatedButtons.begin(), _arrCreatedButtons.end(), this);
        if (toRemove != _arrCreatedButtons.end())
        {
            _arrCreatedButtons.erase(toRemove);
        }
        this->DestroyAll(true);
    }

    IClassInfo* DDScalableButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDScalableButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDScalableButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDScalableButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDScalableButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableButton*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableButton*)*ppElement)->InitDrawImage();
            ((DDScalableButton*)*ppElement)->InitDrawFont();
        }
        return hr;
    }

    HRESULT DDScalableButton::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impNeedsFontResize2Prop,
            &impAssociatedColorProp
        };
        return ClassInfo<DDScalableButton, Button, StandardCreator<DDScalableButton>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    void DDScalableButton::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
    }

    auto DDScalableButton::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        if (!this) return -1;
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        auto v = useInt ? pv->GetInt() : pv->GetBool();
        pv->Release();
        return v;
    }

    void DDScalableButton::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt)
    {
        Value* pv = useInt ? Value::CreateInt(iCreateInt) : Value::CreateBool(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    const PropertyInfo* WINAPI DDScalableButton::FirstScaledImageProp()
    {
        return &impFirstScaledImageProp;
    }

    int DDScalableButton::GetFirstScaledImage()
    {
        return this->GetPropCommon(FirstScaledImageProp, true);
    }

    void DDScalableButton::SetFirstScaledImage(int iFirstImage)
    {
        this->SetPropCommon(FirstScaledImageProp, iFirstImage, true);
    }

    const PropertyInfo* WINAPI DDScalableButton::ScaledImageIntervalsProp()
    {
        return &impScaledImageIntervalsProp;
    }

    int DDScalableButton::GetScaledImageIntervals()
    {
        int v = this->GetPropCommon(ScaledImageIntervalsProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableButton::SetScaledImageIntervals(int iScaleIntervals)
    {
        this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals, true);
    }

    const PropertyInfo* WINAPI DDScalableButton::DrawTypeProp()
    {
        return &impDrawTypeProp;
    }

    int DDScalableButton::GetDrawType()
    {
        return this->GetPropCommon(DrawTypeProp, true);
    }

    void DDScalableButton::SetDrawType(int iDrawType)
    {
        this->SetPropCommon(DrawTypeProp, iDrawType, true);
    }

    const PropertyInfo* WINAPI DDScalableButton::EnableAccentProp()
    {
        return &impEnableAccentProp;
    }

    bool DDScalableButton::GetEnableAccent()
    {
        return this->GetPropCommon(EnableAccentProp, false);
    }

    void DDScalableButton::SetEnableAccent(bool bEnableAccent)
    {
        this->SetPropCommon(EnableAccentProp, bEnableAccent, false);
    }

    const PropertyInfo* WINAPI DDScalableButton::NeedsFontResizeProp()
    {
        return &impNeedsFontResizeProp;
    }

    bool DDScalableButton::GetNeedsFontResize()
    {
        return this->GetPropCommon(NeedsFontResizeProp, false);
    }

    void DDScalableButton::SetNeedsFontResize(bool bNeedsFontResize)
    {
        this->SetPropCommon(NeedsFontResizeProp, bNeedsFontResize, false);
    }

    const PropertyInfo* WINAPI DDScalableButton::NeedsFontResize2Prop()
    {
        return &impNeedsFontResize2Prop;
    }

    bool DDScalableButton::GetNeedsFontResize2()
    {
        return this->GetPropCommon(NeedsFontResize2Prop, false);
    }

    void DDScalableButton::SetNeedsFontResize2(bool bNeedsFontResize2)
    {
        this->SetPropCommon(NeedsFontResize2Prop, bNeedsFontResize2, false);
    }

    const PropertyInfo* WINAPI DDScalableButton::AssociatedColorProp()
    {
        return &impAssociatedColorProp;
    }

    int DDScalableButton::GetAssociatedColor()
    {
        return this->GetPropCommon(AssociatedColorProp, true);
    }

    void DDScalableButton::SetAssociatedColor(int iAssociatedColor)
    {
        this->SetPropCommon(AssociatedColorProp, iAssociatedColor, true);
    }

    void DDScalableButton::InitDrawImage()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableButton::RedrawImages()
    {
        for (DDScalableButton* pe : _arrCreatedButtons)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableButton::InitDrawFont()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableButton::RedrawFonts()
    {
        for (DDScalableButton* pe : _arrCreatedButtons)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)pe, NULL);
        }
    }

    RegKeyValue DDScalableButton::GetRegKeyValue()
    {
        return _rkv;
    }

    void (* DDScalableButton::GetAssociatedFn())(bool, bool, bool)
    {
        return _assocFn;
    }

    bool* DDScalableButton::GetAssociatedBool()
    {
        return _assocBool;
    }

    int DDScalableButton::GetDDCPIntensity()
    {
        return _intensity;
    }

    void DDScalableButton::SetRegKeyValue(RegKeyValue rkvNew)
    {
        _rkv = rkvNew;
    }

    void DDScalableButton::SetAssociatedFn(void (*pfn)(bool, bool, bool))
    {
        _assocFn = pfn;
    }

    void DDScalableButton::SetAssociatedBool(bool* pb)
    {
        _assocBool = pb;
    }

    void DDScalableButton::SetDDCPIntensity(int intensity)
    {
        _intensity = intensity;
    }

    void DDScalableButton::ExecAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3)
    {
        pfn(fnb1, fnb2, fnb3);
    }

    vector<DDScalableRichText*> DDScalableRichText::_arrCreatedTexts;

    DDScalableRichText::DDScalableRichText()
    {
        _arrCreatedTexts.push_back(this);
    }

    DDScalableRichText::~DDScalableRichText()
    {
        this->RemoveListener(_pelPropChange);
        free(_pelPropChange);
        _pelPropChange = nullptr;
        auto toRemove = find(_arrCreatedTexts.begin(), _arrCreatedTexts.end(), this);
        if (toRemove != _arrCreatedTexts.end())
        {
            _arrCreatedTexts.erase(toRemove);
        }
        this->DestroyAll(true);
    }

    IClassInfo* DDScalableRichText::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDScalableRichText::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDScalableRichText::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDScalableRichText::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDScalableRichText>(pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableRichText*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableRichText*)*ppElement)->InitDrawImage();
            ((DDScalableRichText*)*ppElement)->InitDrawFont();
        }
        return hr;
    }

    HRESULT DDScalableRichText::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impNeedsFontResize2Prop,
            &impAssociatedColorProp
        };
        return ClassInfo<DDScalableRichText, RichText, StandardCreator<DDScalableRichText>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableRichText", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    void DDScalableRichText::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
    }

    auto DDScalableRichText::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        auto v = useInt ? pv->GetInt() : pv->GetBool();
        pv->Release();
        return v;
    }

    void DDScalableRichText::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt)
    {
        Value* pv = useInt ? Value::CreateInt(iCreateInt) : Value::CreateBool(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    const PropertyInfo* WINAPI DDScalableRichText::FirstScaledImageProp()
    {
        return &impFirstScaledImageProp;
    }

    int DDScalableRichText::GetFirstScaledImage()
    {
        return this->GetPropCommon(FirstScaledImageProp, true);
    }

    void DDScalableRichText::SetFirstScaledImage(int iFirstImage)
    {
        this->SetPropCommon(FirstScaledImageProp, iFirstImage, true);
    }

    const PropertyInfo* WINAPI DDScalableRichText::ScaledImageIntervalsProp()
    {
        return &impScaledImageIntervalsProp;
    }

    int DDScalableRichText::GetScaledImageIntervals()
    {
        int v = this->GetPropCommon(ScaledImageIntervalsProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableRichText::SetScaledImageIntervals(int iScaleIntervals)
    {
        this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals, true);
    }

    const PropertyInfo* WINAPI DDScalableRichText::DrawTypeProp()
    {
        return &impDrawTypeProp;
    }

    int DDScalableRichText::GetDrawType()
    {
        return this->GetPropCommon(DrawTypeProp, true);
    }

    void DDScalableRichText::SetDrawType(int iDrawType)
    {
        this->SetPropCommon(DrawTypeProp, iDrawType, true);
    }

    const PropertyInfo* WINAPI DDScalableRichText::EnableAccentProp()
    {
        return &impEnableAccentProp;
    }

    bool DDScalableRichText::GetEnableAccent()
    {
        return this->GetPropCommon(EnableAccentProp, false);
    }

    void DDScalableRichText::SetEnableAccent(bool bEnableAccent)
    {
        this->SetPropCommon(EnableAccentProp, bEnableAccent, false);
    }

    const PropertyInfo* WINAPI DDScalableRichText::NeedsFontResizeProp()
    {
        return &impNeedsFontResizeProp;
    }

    bool DDScalableRichText::GetNeedsFontResize()
    {
        return this->GetPropCommon(NeedsFontResizeProp, false);
    }

    void DDScalableRichText::SetNeedsFontResize(bool bNeedsFontResize)
    {
        this->SetPropCommon(NeedsFontResizeProp, bNeedsFontResize, false);
    }

    const PropertyInfo* WINAPI DDScalableRichText::NeedsFontResize2Prop()
    {
        return &impNeedsFontResize2Prop;
    }

    bool DDScalableRichText::GetNeedsFontResize2()
    {
        return this->GetPropCommon(NeedsFontResize2Prop, false);
    }

    void DDScalableRichText::SetNeedsFontResize2(bool bNeedsFontResize2)
    {
        this->SetPropCommon(NeedsFontResize2Prop, bNeedsFontResize2, false);
    }

    const PropertyInfo* WINAPI DDScalableRichText::AssociatedColorProp()
    {
        return &impAssociatedColorProp;
    }

    int DDScalableRichText::GetAssociatedColor()
    {
        return this->GetPropCommon(AssociatedColorProp, true);
    }

    void DDScalableRichText::SetAssociatedColor(int iAssociatedColor)
    {
        this->SetPropCommon(AssociatedColorProp, iAssociatedColor, true);
    }

    void DDScalableRichText::InitDrawImage()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableRichText::RedrawImages()
    {
        for (DDScalableRichText* pe : _arrCreatedTexts)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableRichText::InitDrawFont()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableRichText::RedrawFonts()
    {
        for (DDScalableRichText* pe : _arrCreatedTexts)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)pe, NULL);
        }
    }

    vector<DDScalableTouchEdit*> DDScalableTouchEdit::_arrCreatedBoxes;

    DDScalableTouchEdit::DDScalableTouchEdit()
    {
        _arrCreatedBoxes.push_back(this);
    }

    DDScalableTouchEdit::~DDScalableTouchEdit()
    {
        auto toRemove = find(_arrCreatedBoxes.begin(), _arrCreatedBoxes.end(), this);
        if (toRemove != _arrCreatedBoxes.end())
        {
            _arrCreatedBoxes.erase(toRemove);
        }
        this->DestroyAll(true);
    }

    IClassInfo* DDScalableTouchEdit::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDScalableTouchEdit::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDScalableTouchEdit::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDScalableTouchEdit::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDScalableTouchEdit>(pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            assignExtendedFn(*ppElement, UpdateImageOnPropChange);
            ((DDScalableTouchEdit*)*ppElement)->InitDrawImage();
            ((DDScalableTouchEdit*)*ppElement)->InitDrawFont();
            DWORD dw2;
            HANDLE drawingHandle2 = CreateThread(nullptr, 0, CreateTEVisual, (LPVOID)*ppElement, NULL, &dw2);
            if (drawingHandle2) CloseHandle(drawingHandle2);
        }
        return hr;
    }

    HRESULT DDScalableTouchEdit::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impNeedsFontResize2Prop
        };
        return ClassInfo<DDScalableTouchEdit, TouchEdit2, StandardCreator<DDScalableTouchEdit>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableTouchEdit", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableTouchEdit::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        auto v = useInt ? pv->GetInt() : pv->GetBool();
        pv->Release();
        return v;
    }

    void DDScalableTouchEdit::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt)
    {
        Value* pv = useInt ? Value::CreateInt(iCreateInt) : Value::CreateBool(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::FirstScaledImageProp()
    {
        return &impFirstScaledImageProp;
    }

    int DDScalableTouchEdit::GetFirstScaledImage()
    {
        return this->GetPropCommon(FirstScaledImageProp, true);
    }

    void DDScalableTouchEdit::SetFirstScaledImage(int iFirstImage)
    {
        this->SetPropCommon(FirstScaledImageProp, iFirstImage, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::ScaledImageIntervalsProp()
    {
        return &impScaledImageIntervalsProp;
    }

    int DDScalableTouchEdit::GetScaledImageIntervals()
    {
        int v = this->GetPropCommon(ScaledImageIntervalsProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableTouchEdit::SetScaledImageIntervals(int iScaleIntervals)
    {
        this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::DrawTypeProp()
    {
        return &impDrawTypeProp;
    }

    int DDScalableTouchEdit::GetDrawType()
    {
        return this->GetPropCommon(DrawTypeProp, true);
    }

    void DDScalableTouchEdit::SetDrawType(int iDrawType)
    {
        this->SetPropCommon(DrawTypeProp, iDrawType, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::EnableAccentProp()
    {
        return &impEnableAccentProp;
    }

    bool DDScalableTouchEdit::GetEnableAccent()
    {
        return this->GetPropCommon(EnableAccentProp, false);
    }

    void DDScalableTouchEdit::SetEnableAccent(bool bEnableAccent)
    {
        this->SetPropCommon(EnableAccentProp, bEnableAccent, false);
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::NeedsFontResizeProp()
    {
        return &impNeedsFontResizeProp;
    }

    bool DDScalableTouchEdit::GetNeedsFontResize()
    {
        return this->GetPropCommon(NeedsFontResizeProp, false);
    }

    void DDScalableTouchEdit::SetNeedsFontResize(bool bNeedsFontResize)
    {
        this->SetPropCommon(NeedsFontResizeProp, bNeedsFontResize, false);
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::NeedsFontResize2Prop()
    {
        return &impNeedsFontResize2Prop;
    }

    bool DDScalableTouchEdit::GetNeedsFontResize2()
    {
        return this->GetPropCommon(NeedsFontResize2Prop, false);
    }

    void DDScalableTouchEdit::SetNeedsFontResize2(bool bNeedsFontResize2)
    {
        this->SetPropCommon(NeedsFontResize2Prop, bNeedsFontResize2, false);
    }

    void DDScalableTouchEdit::InitDrawImage()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableTouchEdit::RedrawImages()
    {
        for (DDScalableTouchEdit* pe : _arrCreatedBoxes)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableTouchEdit::InitDrawFont()
    {
        PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableTouchEdit::RedrawFonts()
    {
        for (DDScalableTouchEdit* pe : _arrCreatedBoxes)
        {
            PostMessageW(subviewwnd->GetHWND(), WM_USER + 2, (WPARAM)pe, NULL);
        }
    }

    LVItem::~LVItem()
    {
        _childItemss.clear();
        _childIcons.clear();
        _childShadows.clear();
        _childShortcutArrows.clear();
        _childFilenames.clear();
        this->ClearAllListeners();
        for (auto pel : _pels)
        {
            delete pel;
        }
        _pels.clear();
        this->DestroyAll(true);
    }

    IClassInfo* LVItem::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void LVItem::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* LVItem::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT LVItem::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<LVItem, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableButton*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableButton*)*ppElement)->InitDrawImage();
            ((DDScalableButton*)*ppElement)->InitDrawFont();
        }
        return hr;
    }

    HRESULT LVItem::Register()
    {
        return ClassInfo<LVItem, DDScalableButton, StandardCreator<LVItem>>::RegisterGlobal(HINST_THISCOMPONENT, L"LVItem", nullptr, 0);
    }

    unsigned short LVItem::GetInternalXPos()
    {
        return _xPos;
    }

    unsigned short LVItem::GetInternalYPos()
    {
        return _yPos;
    }

    void LVItem::SetInternalXPos(unsigned short iXPos)
    {
        _xPos = iXPos;
    }

    void LVItem::SetInternalYPos(unsigned short iYPos)
    {
        _yPos = iYPos;
    }

    wstring LVItem::GetFilename()
    {
        return _filename;
    }

    wstring LVItem::GetSimpleFilename()
    {
        return _simplefilename;
    }

    void LVItem::SetFilename(const wstring& wsFilename)
    {
        _filename = wsFilename;
    }

    void LVItem::SetSimpleFilename(const wstring& wsSimpleFilename)
    {
        _simplefilename = wsSimpleFilename;
    }

    bool LVItem::GetDirState()
    {
        return _isDirectory;
    }

    bool LVItem::GetGroupedDirState()
    {
        return _isGrouped;
    }

    bool LVItem::GetHiddenState()
    {
        return _isHidden;
    }

    bool LVItem::GetMemorySelected()
    {
        return _mem_isSelected;
    }

    bool LVItem::GetShortcutState()
    {
        return _isShortcut;
    }

    bool LVItem::GetColorLock()
    {
        return _colorLock;
    }

    bool LVItem::GetDragState()
    {
        return _dragged;
    }

    bool LVItem::GetRefreshState()
    {
        return _refreshable;
    }

    bool LVItem::GetSizedFromGroup()
    {
        return _sfg;
    }

    void LVItem::SetDirState(bool dirState)
    {
        _isDirectory = dirState;
    }

    void LVItem::SetGroupedDirState(bool groupedDirState)
    {
        _isGrouped = groupedDirState;
    }

    void LVItem::SetHiddenState(bool hiddenState)
    {
        _isHidden = hiddenState;
    }

    void LVItem::SetMemorySelected(bool mem_isSelectedState)
    {
        _mem_isSelected = mem_isSelectedState;
    }

    void LVItem::SetShortcutState(bool shortcutState)
    {
        _isShortcut = shortcutState;
    }

    void LVItem::SetColorLock(bool colorLockState)
    {
        _colorLock = colorLockState;
    }

    void LVItem::SetDragState(bool dragstate)
    {
        _dragged = dragstate;
    }

    void LVItem::SetRefreshState(bool refreshstate)
    {
        _refreshable = refreshstate;
    }

    void LVItem::SetSizedFromGroup(bool sfg)
    {
        _sfg = sfg;
    }

    unsigned short LVItem::GetPage()
    {
        return _page;
    }

    void LVItem::SetPage(unsigned short pageID)
    {
        _page = pageID;
    }

    LVItemGroupSize LVItem::GetGroupSize()
    {
        return _groupsize;
    }

    void LVItem::SetGroupSize(LVItemGroupSize lvigs)
    {
        _groupsize = lvigs;
    }

    vector<LVItem*> LVItem::GetChildItems()
    {
        return _childItemss;
    }

    vector<DDScalableElement*> LVItem::GetChildIcons()
    {
        return _childIcons;
    }

    vector<Element*> LVItem::GetChildShadows()
    {
        return _childShadows;
    }

    vector<Element*> LVItem::GetChildShortcutArrows()
    {
        return _childShortcutArrows;
    }

    vector<RichText*> LVItem::GetChildFilenames()
    {
        return _childFilenames;
    }

    void LVItem::SetChildItems(vector<LVItem*> vpm)
    {
        _childItemss = vpm;
    }

    void LVItem::SetChildIcons(vector<DDScalableElement*> vipm)
    {
        _childIcons = vipm;
    }

    void LVItem::SetChildShadows(vector<Element*> vispm)
    {
        _childShadows = vispm;
    }

    void LVItem::SetChildShortcutArrows(vector<Element*> vspm)
    {
        _childShortcutArrows = vspm;
    }

    void LVItem::SetChildFilenames(vector<RichText*> vfpm)
    {
        _childFilenames = vfpm;
    }

    void LVItem::SetListeners(vector<IElementListener*> pels)
    {
        _pels = pels;
    }

    void LVItem::ClearAllListeners()
    {
        for (auto pel : _pels)
        {
            this->RemoveListener(pel);
        }
    }

    DDLVActionButton::~DDLVActionButton()
    {
        _assocItem = nullptr;
    }

    IClassInfo* DDLVActionButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDLVActionButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDLVActionButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDLVActionButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDLVActionButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableButton*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableButton*)*ppElement)->InitDrawImage();
            ((DDScalableButton*)*ppElement)->InitDrawFont();
        }
        return hr;
    }

    HRESULT DDLVActionButton::Register()
    {
        return ClassInfo<DDLVActionButton, DDScalableButton, StandardCreator<DDLVActionButton>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDLVActionButton", nullptr, 0);
    }

    LVItem* DDLVActionButton::GetAssociatedItem()
    {
        return _assocItem;
    }

    void DDLVActionButton::SetAssociatedItem(LVItem* lvi)
    {
        _assocItem = lvi;
    }

    IClassInfo* DDToggleButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDToggleButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDToggleButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDToggleButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDToggleButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableButton*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableButton*)*ppElement)->InitDrawImage();
            ((DDScalableButton*)*ppElement)->InitDrawFont();
        }
        return hr;
    }

    HRESULT DDToggleButton::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impCheckedStateProp
        };
        return ClassInfo<DDToggleButton, DDScalableButton, StandardCreator<DDToggleButton>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDToggleButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    const PropertyInfo* WINAPI DDToggleButton::CheckedStateProp()
    {
        return &impCheckedStateProp;
    }

    bool DDToggleButton::GetCheckedState()
    {
        return this->GetPropCommon(CheckedStateProp, false);
    }

    void DDToggleButton::SetCheckedState(bool bChecked)
    {
        this->SetPropCommon(CheckedStateProp, bChecked, false);
    }

    IClassInfo* DDCheckBox::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDCheckBox::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDCheckBox::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDCheckBox::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDCheckBox, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            DWORD dw;
            HANDLE drawingHandle = CreateThread(nullptr, 0, CreateCBInnerElements, (LPVOID)*ppElement, NULL, &dw);
            if (drawingHandle) CloseHandle(drawingHandle);
        }
        return hr;
    }

    HRESULT DDCheckBox::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impCheckedStateProp
        };
        return ClassInfo<DDCheckBox, DDScalableButton, StandardCreator<DDCheckBox>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDCheckBox", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    const PropertyInfo* WINAPI DDCheckBox::CheckedStateProp()
    {
        return &impCheckedStateProp;
    }

    bool DDCheckBox::GetCheckedState()
    {
        return this->GetPropCommon(CheckedStateProp, false);
    }

    void DDCheckBox::SetCheckedState(bool bChecked)
    {
        this->SetPropCommon(CheckedStateProp, bChecked, false);
    }

    IClassInfo* DDCheckBoxGlyph::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDCheckBoxGlyph::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDCheckBoxGlyph::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDCheckBoxGlyph::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDCheckBoxGlyph, int>(0, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableElement*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableElement*)*ppElement)->InitDrawImage();
            ((DDScalableElement*)*ppElement)->InitDrawFont();
        }
        return hr;
    }

    HRESULT DDCheckBoxGlyph::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impCheckedStateProp
        };
        return ClassInfo<DDCheckBoxGlyph, DDScalableElement, StandardCreator<DDCheckBoxGlyph>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDCheckBoxGlyph", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    const PropertyInfo* WINAPI DDCheckBoxGlyph::CheckedStateProp()
    {
        return &impCheckedStateProp;
    }

    bool DDCheckBoxGlyph::GetCheckedState()
    {
        return this->GetPropCommon(CheckedStateProp, false);
    }

    void DDCheckBoxGlyph::SetCheckedState(bool bChecked)
    {
        this->SetPropCommon(CheckedStateProp, bChecked, false);
    }

    DDColorPicker::~DDColorPicker()
    {
        this->DestroyAll(true);
    }

    IClassInfo* DDColorPicker::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDColorPicker::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDColorPicker::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDColorPicker::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDColorPicker, int>(0, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            DWORD dw;
            HANDLE drawingHandle = CreateThread(nullptr, 0, ColorPickerLayout, (LPVOID)*ppElement, NULL, &dw);
            if (drawingHandle) CloseHandle(drawingHandle);
        }
        return hr;
    }

    HRESULT DDColorPicker::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impColorIntensityProp,
            &impDefaultColorProp
        };
        return ClassInfo<DDColorPicker, Element, StandardCreator<DDColorPicker>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDColorPicker", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    int DDColorPicker::GetPropCommon(const PropertyProcT pPropertyProc)
    {
        if (!this) return -1;
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        int v = pv->GetInt();
        pv->Release();
        return v;
    }

    void DDColorPicker::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt)
    {
        Value* pv = Value::CreateInt(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    const PropertyInfo* WINAPI DDColorPicker::FirstScaledImageProp()
    {
        return &impFirstScaledImageProp;
    }

    int DDColorPicker::GetFirstScaledImage()
    {
        return this->GetPropCommon(FirstScaledImageProp);
    }

    void DDColorPicker::SetFirstScaledImage(int iFirstImage)
    {
        this->SetPropCommon(FirstScaledImageProp, iFirstImage);
    }

    const PropertyInfo* WINAPI DDColorPicker::ScaledImageIntervalsProp()
    {
        return &impScaledImageIntervalsProp;
    }

    int DDColorPicker::GetScaledImageIntervals()
    {
        int v = this->GetPropCommon(ScaledImageIntervalsProp);
        if (v < 1) v = 1;
        return v;
    }

    void DDColorPicker::SetScaledImageIntervals(int iScaleIntervals)
    {
        this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals);
    }

    const PropertyInfo* WINAPI DDColorPicker::ColorIntensityProp()
    {
        return &impColorIntensityProp;
    }

    int DDColorPicker::GetColorIntensity()
    {
        return this->GetPropCommon(ColorIntensityProp);
    }

    void DDColorPicker::SetColorIntensity(int iColorIntensity)
    {
        this->SetPropCommon(ColorIntensityProp, iColorIntensity);
    }

    const PropertyInfo* WINAPI DDColorPicker::DefaultColorProp()
    {
        return &impDefaultColorProp;
    }

    int DDColorPicker::GetDefaultColor()
    {
        return this->GetPropCommon(DefaultColorProp);
    }

    void DDColorPicker::SetDefaultColor(int iDefaultColor)
    {
        this->SetPropCommon(DefaultColorProp, iDefaultColor);
    }

    RegKeyValue DDColorPicker::GetRegKeyValue()
    {
        return _rkv;
    }

    vector<DDScalableElement*> DDColorPicker::GetTargetElements()
    {
        return _targetElems;
    }

    bool DDColorPicker::GetThemeAwareness()
    {
        return _themeAwareness;
    }

    void DDColorPicker::SetRegKeyValue(RegKeyValue rkvNew)
    {
        _rkv = rkvNew;
    }

    void DDColorPicker::SetTargetElements(vector<DDScalableElement*> vte)
    {
        _targetElems = vte;
    }

    void DDColorPicker::SetThemeAwareness(bool ta)
    {
        _themeAwareness = ta;
    }

    DDColorPickerButton::~DDColorPickerButton()
    {
        this->RemoveListener(_pelPropChange);
        free(_pelPropChange);
        _pelPropChange = nullptr;
        this->DestroyAll(true);
    }

    IClassInfo* DDColorPickerButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDColorPickerButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDColorPickerButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDColorPickerButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDColorPickerButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            DWORD dw;
            HANDLE drawingHandle = CreateThread(nullptr, 0, PickerBtnFn, (LPVOID)*ppElement, NULL, &dw);
            if (drawingHandle) CloseHandle(drawingHandle);
        }
        return hr;
    }

    HRESULT DDColorPickerButton::Register()
    {
        return ClassInfo<DDColorPickerButton, Button, StandardCreator<DDColorPickerButton>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDColorPickerButton", nullptr, 0);
    }

    void DDColorPickerButton::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
    }

    COLORREF DDColorPickerButton::GetAssociatedColor()
    {
        return _assocCR;
    }

    BYTE DDColorPickerButton::GetOrder()
    {
        return _order;
    }

    vector<DDScalableElement*> DDColorPickerButton::GetTargetElements()
    {
        return _targetElems;
    }

    void DDColorPickerButton::SetAssociatedColor(COLORREF cr)
    {
        _assocCR = cr;
    }

    void DDColorPickerButton::SetOrder(BYTE bOrder)
    {
        _order = bOrder;
    }

    void DDColorPickerButton::SetTargetElements(vector<DDScalableElement*> vte)
    {
        _targetElems = vte;
    }

    LRESULT CALLBACK NotificationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CValuePtr v;
        Element* ppeTemp;
        wstring fontOld;
        wregex fontRegex(L".*font;.*\%.*");
        bool isSysmetricFont;
        switch (uMsg)
        {
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
                if (isSysmetricFont)
                {
                    size_t modifier = fontOld.find(L";");
                    size_t modifier2 = fontOld.find(L"%");
                    wstring fontIntermediate = fontOld.substr(0, modifier + 1);
                    wstring fontIntermediate2 = fontOld.substr(modifier + 1, modifier2);
                    wstring fontIntermediate3 = fontOld.substr(modifier2, wcslen(fontOld.c_str()));
                    int newFontSize = _wtoi(fontIntermediate2.c_str()) * g_dpi / g_dpiLaunch;
                    wstring fontNew = fontIntermediate + to_wstring(newFontSize) + fontIntermediate3;
                    ppeTemp->SetFont(fontNew.c_str());
                }
                break;
            case WM_USER + 3:
                DDNotificationBanner::DestroyBanner(nullptr);
                break;
        }
        return CallWindowProc(WndProcNotification, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI AnimateWindowWrapper(LPVOID lpParam)
    {
        Sleep(50);
        if (notificationwnd)
        {
            AnimateWindow(notificationwnd->GetHWND(), 180, AW_BLEND);
            notificationwnd->ShowWindow(SW_SHOW);
        }
        return 0;
    }

    DWORD WINAPI AutoCloseNotification(LPVOID lpParam)
    {
        IntegerWrapper* iwTemp = (IntegerWrapper*)lpParam;
        Sleep(iwTemp->val * 1000);
        SendMessageW(notificationwnd->GetHWND(), WM_USER + 3, NULL, NULL);
        return 0;
    }

    vector<wstring> SplitLineBreaks(const wstring& originalstr)
    {
        vector<wstring> strs;
        size_t start = 0;
        size_t end = originalstr.find(L'\n');
        while (end != wstring::npos)
        {
            strs.push_back(originalstr.substr(start, end - start));
            start = end + 1;
            end = originalstr.find(L'\n', start);
        }
        strs.push_back(originalstr.substr(start));
        return strs;
    }

    int CalcLines(const wstring& textStr)
    {
        int lines = count(textStr.begin(), textStr.end(), L'\n') + 1;
        return lines;
    }

    void GetLongestLine(HDC hdc, const wstring& textStr, RECT* rcText)
    {
        vector<wstring> divided = SplitLineBreaks(textStr);
        int saved{};
        for (int i = 0; i < divided.size(); i++)
        {
            DrawTextW(hdc, divided[i].c_str(), -1, rcText, DT_CALCRECT | DT_SINGLELINE);
            if (rcText->right > saved) saved = rcText->right;
        }
        rcText->right = saved;
    }

    DWORD WINAPI AutoSizeFont(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        Sleep(20);
        Element* ppeTemp = (Element*)lpParam;
        if (!ppeTemp || notificationwnd == nullptr) return 1;
        SendMessageW(notificationwnd->GetHWND(), WM_USER + 2, (WPARAM)ppeTemp, NULL);
        UnInitThread();
        return 0;
    }

    DDNotificationBanner::~DDNotificationBanner()
    {
        this->DestroyAll(true);
    }

    IClassInfo* DDNotificationBanner::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDNotificationBanner::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDNotificationBanner::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    HRESULT DDNotificationBanner::Create(HWND hParent, bool fDblBuffer, UINT nCreate, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr{};
        HANDLE ProcessHeap{};
        DDNotificationBanner* buffer{};
        DDNotificationBanner* pResult{};
        ProcessHeap = GetProcessHeap();
        buffer = (DDNotificationBanner*)HeapAlloc(ProcessHeap, 0x8, sizeof(DDNotificationBanner));
        pResult = buffer;
        if (buffer)
        {
            hr = pResult->Initialize(hParent, fDblBuffer, nCreate, pParent, pdwDeferCookie);
            if (FAILED(hr))
            {
                pResult->Destroy(true);
                pResult = nullptr;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        *ppElement = pResult;
        return hr;
    }

    HRESULT DDNotificationBanner::Register()
    {
        return ClassInfo<DDNotificationBanner, HWNDElement, EmptyCreator<DDNotificationBanner>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDNotificationBanner", nullptr, 0);
    }

    Element* DDNotificationBanner::GetIconElement()
    {
        return _icon;
    }

    DDScalableElement* DDNotificationBanner::GetTitleElement()
    {
        return _title;
    }

    DDScalableElement* DDNotificationBanner::GetContentElement()
    {
        return _content;
    }

    void DDNotificationBanner::CreateBanner(DDNotificationBanner* pDDNB, DUIXmlParser* pParser, DDNotificationType type, LPCWSTR pszResID, LPCWSTR title, LPCWSTR content, short timeout, bool fClose)
    {
        static bool notificationopen{};
        static HANDLE AutoCloseHandle;
        if (notificationopen) DestroyBanner(&notificationopen);
        unsigned long keyN{};
        Element* pHostElement;
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        NativeHWNDHost::Create(L"DD_NotificationHost", L"DirectDesktop In-App Notification", nullptr, nullptr, 0, 0, 0, 0, NULL, WS_POPUP | WS_BORDER, HINST_THISCOMPONENT, 0, &notificationwnd);
        HWNDElement::Create(notificationwnd->GetHWND(), true, NULL, nullptr, &keyN, (Element**)&pDDNB);
        Microsoft::WRL::ComPtr<ITaskbarList> pTaskbarList;
        if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList, (void**)&pTaskbarList)))
        {
            if (SUCCEEDED(pTaskbarList->HrInit()))
            {
                pTaskbarList->DeleteTab(notificationwnd->GetHWND());
            }
        }
        pParser->CreateElement(pszResID, pDDNB, nullptr, nullptr, &pHostElement);
        WndProcNotification = (WNDPROC)SetWindowLongPtrW(notificationwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)NotificationProc);
        pHostElement->SetVisible(true);
        pHostElement->EndDefer(keyN);
        notificationwnd->Host(pHostElement);
        int WindowsBuild = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\ShellCommon", L"BuildNumber");
        int WindowsRev = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\BuildLayers\\ShellCommon", L"BuildQfe");
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(notificationwnd->GetHWND(), &margins);
        if (WindowsBuild > 22000 || WindowsBuild == 22000 && WindowsRev >= 51)
        {
            DWORD cornerPreference = DWMWCP_ROUND;
            DwmSetWindowAttribute(notificationwnd->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
        }
        BlurBackground(notificationwnd->GetHWND(), true, false, nullptr);
        pHostElement->SetBackgroundStdColor(7);
        CValuePtr v;
        pDDNB->GetPadding(&v);
        pHostElement->SetValue(Element::PaddingProp, 1, v);
        SetForegroundWindow(notificationwnd->GetHWND());
        CreateAndSetLayout(pHostElement, BorderLayout::Create, 0, nullptr);

        int cx{}, cy{};
        RECT hostpadding = *(v->GetRect());
        cx += (hostpadding.left + hostpadding.right + 48 * g_flScaleFactor); // 48: 28 is the icon width, 20 is extra padding
        cy += (hostpadding.top + hostpadding.bottom);
        Element* peTemp = pDDNB->GetIconElement();
        CreateAndInit<Element, int>(0, pHostElement, nullptr, (Element**)&peTemp);
        peTemp->SetID(L"DDNB_Icon");
        pHostElement->Add(&peTemp, 1);
        wstring titleStr{};
        switch (type)
        {
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

        HDC hdcMem = CreateCompatibleDC(nullptr);
        NONCLIENTMETRICSW ncm{};
        TEXTMETRICW tm{};
        SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, NULL, g_dpi);
        HFONT hFont = CreateFontIndirectW(&(ncm.lfMessageFont));
        SelectObject(hdcMem, hFont);
        RECT rcText{}, rcText2{};
        if (content)
        {
            GetLongestLine(hdcMem, content, &rcText2);
            GetTextMetricsW(hdcMem, &tm);
            cy += (ceil(tm.tmHeight * 1.15) * CalcLines(content)) * g_flScaleFactor;
        }
        ncm.lfMessageFont.lfWeight = 700;
        hFont = CreateFontIndirectW(&(ncm.lfMessageFont));
        SelectObject(hdcMem, hFont);
        DrawTextW(hdcMem, title, -1, &rcText, DT_CALCRECT | DT_SINGLELINE);
        cx += (ceil(max(rcText.right, rcText2.right) * 1.15) * g_flScaleFactor);
        GetTextMetricsW(hdcMem, &tm);
        cy += (ceil(tm.tmHeight * 1.15) + 6) * g_flScaleFactor;
        DeleteObject(hFont);
        DeleteDC(hdcMem);

        peTemp = pDDNB->GetTitleElement();
        CreateAndInit<Element, int>(0, pHostElement, nullptr, (Element**)&peTemp);
        peTemp->SetID(L"DDNB_Title");
        pHostElement->Add(&peTemp, 1);
        HANDLE setFontStr = CreateThread(nullptr, 0, AutoSizeFont, peTemp, 0, nullptr);
        if (setFontStr) CloseHandle(setFontStr);
        peTemp->SetContentString(titleStr.c_str());

        if (content)
        {
            peTemp = pDDNB->GetContentElement();
            CreateAndInit<Element, int>(0, pHostElement, nullptr, (Element**)&peTemp);
            peTemp->SetID(L"DDNB_Content");
            pHostElement->Add(&peTemp, 1);
            HANDLE setFontStr2 = CreateThread(nullptr, 0, AutoSizeFont, peTemp, 0, nullptr);
            if (setFontStr2) CloseHandle(setFontStr2);
            peTemp->SetContentString(content);
        }

        LPWSTR sheetName = g_theme ? (LPWSTR)L"default" : (LPWSTR)L"defaultdark";
        StyleSheet* sheet = pHostElement->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        pParser->GetSheet(sheetName, &sheetStorage);
        pHostElement->SetValue(Element::SheetProp, 1, sheetStorage);

        // Window borders
        cx += (round(g_flScaleFactor)) * 2;
        cy += (round(g_flScaleFactor)) * 2;

        if (notificationwnd)
        {
            SetWindowPos(notificationwnd->GetHWND(), HWND_TOPMOST, (dimensions.left + dimensions.right - cx) / 2, 40 * g_flScaleFactor, cx, cy, SWP_FRAMECHANGED);
            notificationopen = true;
            IntegerWrapper* iw = new IntegerWrapper{ timeout };
            HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateWindowWrapper, &notificationopen, NULL, nullptr);
            if (AnimHandle) CloseHandle(AnimHandle);
            if (timeout > 0)
            {
                TerminateThread(AutoCloseHandle, 1);
                DWORD dwAutoClose;
                AutoCloseHandle = CreateThread(nullptr, 0, AutoCloseNotification, iw, NULL, &dwAutoClose);
            }
        }
    }

    void DDNotificationBanner::DestroyBanner(bool* notificationopen)
    {
        if (notificationwnd != nullptr)
        {
            AnimateWindow(notificationwnd->GetHWND(), 120, AW_BLEND | AW_HIDE);
            notificationwnd->DestroyWindow();
            notificationwnd = nullptr;
        }
        if (notificationopen != nullptr) *notificationopen = false;
    }
}
