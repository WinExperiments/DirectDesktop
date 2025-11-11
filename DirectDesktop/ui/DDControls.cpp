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
    IClassInfo* DDScalableElement::s_pClassInfo;
    IClassInfo* DDScalableButton::s_pClassInfo;
    IClassInfo* DDScalableRichText::s_pClassInfo;
    IClassInfo* DDScalableTouchButton::s_pClassInfo;
    IClassInfo* DDScalableTouchEdit::s_pClassInfo;
    IClassInfo* LVItem::s_pClassInfo;
    IClassInfo* DDLVActionButton::s_pClassInfo;
    IClassInfo* DDToggleButton::s_pClassInfo;
    IClassInfo* DDCheckBox::s_pClassInfo;
    IClassInfo* DDCheckBoxGlyph::s_pClassInfo;
    IClassInfo* DDNumberedButton::s_pClassInfo;
    IClassInfo* DDCombobox::s_pClassInfo;
    IClassInfo* DDSlider::s_pClassInfo;
    IClassInfo* DDColorPicker::s_pClassInfo;
    IClassInfo* DDColorPickerButton::s_pClassInfo;
    IClassInfo* DDTabbedPages::s_pClassInfo;
    IClassInfo* DDNotificationBanner::s_pClassInfo;

    struct NotificationData
    {
        DDNotificationBanner* nb;
        Element* pe;
        int val;
    };

    vector<HWND> g_nwnds{};

    WNDPROC WndProcNotification;

    HRESULT WINAPI CreateAndSetLayout(Element* pe, HRESULT (*pfnCreate)(int, int*, Value**), int dNumParams, int* pParams)
    {
        CValuePtr spvLayout;
        HRESULT hr = pfnCreate(dNumParams, pParams, &spvLayout);
        if (SUCCEEDED(hr))
        {
            hr = pe->SetValue(Element::LayoutProp, 1, spvLayout);
        }

        return hr;
    }

    void ElementSetValue(Element* peTo, const PropertyInfo* ppi, Value* pvNew, Element* peFrom)
    {
        Value* v;
        if (peTo)
        {
            v = pvNew;
            if (pvNew)
                pvNew->AddRef();
            else
                v = peFrom->GetValue(ppi, 2, 0);
            peTo->SetValue(ppi, 1, v);
            v->Release();
        }
    }

    template <typename T>
    void RedrawImageCore(T* pe)
    {
        int scaleInterval = GetCurrentScaleInterval();
        int scaleIntervalImage = pe->GetScaledImageIntervals();
        if (scaleInterval > scaleIntervalImage - 1)
            scaleInterval = scaleIntervalImage - 1;
        int imageID = pe->GetFirstScaledImage() + scaleInterval;

        HBITMAP newImage = (HBITMAP)LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (newImage == nullptr)
        {
            LoadPNGAsBitmap(newImage, imageID);
            IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1, NULL);
        }

        if (newImage != nullptr)
        {
            if (pe->GetAssociatedColor() != -1)
                IterateBitmap(newImage, StandardBitmapPixelHandler, 3, 0, pe->GetDDCPIntensity() / 255.0, pe->GetAssociatedColor());
            else if (pe->GetEnableAccent())
                IterateBitmap(newImage, StandardBitmapPixelHandler, 1, 0, pe->GetDDCPIntensity() / 255.0, ImmersiveColor);
            else if (pe->GetDDCPIntensity() != 255)
                pe->SetAlpha(pe->GetDDCPIntensity());


            switch (pe->GetDrawType())
            {
            case 1:
            {
                CValuePtr spvImage = Value::CreateGraphic(newImage, 7, 0xFFFFFFFF, true, false, false);
                if (spvImage)
                    pe->SetValue(Element::BackgroundProp, 1, spvImage);
                break;
            }
            case 2:
            {
                CValuePtr spvImage = Value::CreateGraphic(newImage, 2, 0xFFFFFFFF, true, false, false);
                if (spvImage)
                    pe->SetValue(Element::ContentProp, 1, spvImage);
                break;
            }
            }
            DeleteObject(newImage);
        }
    }

    template <typename T>
    void RedrawFontCore(T* pe, bool* result, bool fResize)
    {
        CValuePtr v;
        if (fResize)
        {
            if (pe->GetFont(&v) == nullptr)
            {
                if (result) *result = true;
                return;
            }
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
                if (result) *result = false;
            }
            else if (pe->GetFontSize() > 0)
            {
                pe->SetFontSize(pe->GetFontSize() * g_flScaleFactor);
            }
        }
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
    static const int vvimpDDCPIntensityProp[] = { 1, -1 };
    static PropertyInfoData dataimpDDCPIntensityProp;
    static const PropertyInfo impDDCPIntensityProp =
    {
        L"DDCPIntensity",
        0x2 | 0x4,
        0x1,
        vvimpDDCPIntensityProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpDDCPIntensityProp
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
    static const int vvimpIsVerticalProp[] = { 2, -1 };
    static PropertyInfoData dataimpIsVerticalProp;
    static const PropertyInfo impIsVerticalProp =
    {
        L"IsVertical",
        0x2 | 0x4,
        0x1,
        vvimpIsVerticalProp,
        nullptr,
        Value::GetBoolFalse,
        &dataimpIsVerticalProp
    };
    static const int vvimpTextWidthProp[] = { 1, -1 };
    static PropertyInfoData dataimpTextWidthProp;
    static const PropertyInfo impTextWidthProp =
    {
        L"TextWidth",
        0x2 | 0x4,
        0x1,
        vvimpTextWidthProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpTextWidthProp
    };
    static const int vvimpTextHeightProp[] = { 1, -1 };
    static PropertyInfoData dataimpTextHeightProp;
    static const PropertyInfo impTextHeightProp =
    {
        L"TextHeight",
        0x2 | 0x4,
        0x1,
        vvimpTextHeightProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpTextHeightProp
    };
    static const int vvimpListMaxHeightProp[] = { 1, -1 };
    static PropertyInfoData dataimpListMaxHeightProp;
    static const PropertyInfo impListMaxHeightProp =
    {
        L"ListMaxHeight",
        0x2 | 0x4,
        0x1,
        vvimpListMaxHeightProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpListMaxHeightProp
    };

    void DDSlider::s_AnimateThumb(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        CSafeElementPtr<DDScalableTouchButton> peThumbInner;
        peThumbInner.Assign(regElem<DDScalableTouchButton*>(L"DDS_ThumbInner", elem));
        GTRANS_DESC transDesc[2];
        TransitionStoryboardInfo tsbInfo = {};
        float alpha1 = 1.0f;
        float alpha2 = 1.0f;
        float scaleRelease{};
        if (pProp == Element::MouseWithinProp())
        {
            scaleRelease = (elem->GetMouseWithin()) ? 1.33f : 1.0f;
            goto THUMBANIMATE;
        }
        if (pProp == TouchButton::CapturedProp())
        {
            alpha1 = (((TouchButton*)elem)->GetCaptured() || type == 5) ? 1.0f : 0.8f;
            alpha2 = (((TouchButton*)elem)->GetCaptured() || type == 5) ? 0.8f : 1.0f;
            scaleRelease = (elem->GetMouseWithin()) ? 1.33f : 1.0f;
        THUMBANIMATE:
            if (((TouchButton*)elem)->GetCaptured() || type == 5)
                TriggerScaleOut(peThumbInner, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, 0.83f, 0.83f, 0.5f, 0.5f, false, false);
            else
                TriggerScaleOut(peThumbInner, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, scaleRelease, scaleRelease, 0.5f, 0.5f, false, false);
            TriggerFade(peThumbInner, transDesc, 1, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, alpha1, alpha2, false, false, ((Button*)elem)->GetCaptured());
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peThumbInner->GetDisplayNode(), &tsbInfo);
        }
    }

    DDScalableElement::~DDScalableElement()
    {
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

    bool DDScalableElement::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = Element::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        //if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FontProp))
        //{
        //    RedrawFontCore<DDScalableElement>(this, &result);
        //}
        return result;
    }

    void DDScalableElement::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FirstScaledImageProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableElement>(this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
            RedrawFontCore<DDScalableElement>(this, nullptr, this->GetNeedsFontResize());
        Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDScalableElement::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDScalableElement, int>(0, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDScalableElement::Register()
    {
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impAssociatedColorProp,
            &impDDCPIntensityProp
        };
        return ClassInfo<DDScalableElement, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableElement", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableElement::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        if (!this) return -1;
        if (this->IsDestroyed()) return -1;
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

    const PropertyInfo* WINAPI DDScalableElement::DDCPIntensityProp()
    {
        return &impDDCPIntensityProp;
    }

    int DDScalableElement::GetDDCPIntensity()
    {
        int v = this->GetPropCommon(DDCPIntensityProp, true);
        if (v < 0) v += 256;
        return v;
    }

    void DDScalableElement::SetDDCPIntensity(int intensity)
    {
        this->SetPropCommon(DDCPIntensityProp, intensity, true);
    }

    unsigned short DDScalableElement::GetGroupColor()
    {
        return _gc;
    }

    void DDScalableElement::SetGroupColor(unsigned short sGC)
    {
        _gc = sGC;
    }

    DDScalableButton::~DDScalableButton()
    {
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

    bool DDScalableButton::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = Button::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        //if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FontProp))
        //{
        //    RedrawFontCore<DDScalableButton>(this, &result);
        //}
        return result;
    }
    void DDScalableButton::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FirstScaledImageProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableButton>(this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
            RedrawFontCore<DDScalableButton>(this, nullptr, this->GetNeedsFontResize());
        Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDScalableButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDScalableButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDScalableButton::Register()
    {
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impAssociatedColorProp,
            &impDDCPIntensityProp
        };
        return ClassInfo<DDScalableButton, Button>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableButton::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        if (!this) return -1;
        if (this->IsDestroyed()) return -1;
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

    const PropertyInfo* WINAPI DDScalableButton::DDCPIntensityProp()
    {
        return &impDDCPIntensityProp;
    }

    int DDScalableButton::GetDDCPIntensity()
    {
        int v = this->GetPropCommon(DDCPIntensityProp, true);
        if (v < 0) v += 256;
        return v;
    }

    void DDScalableButton::SetDDCPIntensity(int intensity)
    {
        this->SetPropCommon(DDCPIntensityProp, intensity, true);
    }

    RegKeyValue DDScalableButton::GetRegKeyValue()
    {
        return _rkv;
    }

    void (*DDScalableButton::GetAssociatedFn())(bool, bool, bool)
    {
        return _assocFn;
    }

    void* DDScalableButton::GetAssociatedSetting()
    {
        return _assocSetting;
    }

    unsigned short DDScalableButton::GetGroupColor()
    {
        return _gc;
    }

    bool DDScalableButton::GetShellInteraction()
    {
        return _shellinteraction;
    }

    void DDScalableButton::SetRegKeyValue(RegKeyValue rkvNew)
    {
        _rkv = rkvNew;
    }

    void DDScalableButton::SetAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3)
    {
        _assocFn = pfn;
        _fnb1 = fnb1;
        _fnb2 = fnb2;
        _fnb3 = fnb3;
    }

    void DDScalableButton::SetAssociatedSetting(void* pb)
    {
        _assocSetting = pb;
    }

    void DDScalableButton::SetGroupColor(unsigned short sGC)
    {
        _gc = sGC;
    }

    void DDScalableButton::SetShellInteraction(bool bShellInteraction)
    {
        _shellinteraction = bShellInteraction;
    }

    void DDScalableButton::ExecAssociatedFn(void (*pfn)(bool, bool, bool))
    {
        pfn(_fnb1, _fnb2, _fnb3);
    }

    DDScalableRichText::~DDScalableRichText()
    {
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

    bool DDScalableRichText::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = RichText::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        //if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FontProp))
        //{
        //    RedrawFontCore<DDScalableRichText>(this, &result);
        //}
        return result;
    }

    void DDScalableRichText::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FirstScaledImageProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                RichText::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableRichText>(this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
            RedrawFontCore<DDScalableRichText>(this, nullptr, this->GetNeedsFontResize());
        RichText::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDScalableRichText::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDScalableRichText>(pParent, pdwDeferCookie, ppElement);
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
            &impAssociatedColorProp
        };
        return ClassInfo<DDScalableRichText, RichText>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableRichText", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableRichText::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        if (!this) return -1;
        if (this->IsDestroyed()) return -1;
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

    const PropertyInfo* WINAPI DDScalableRichText::DDCPIntensityProp()
    {
        return &impDDCPIntensityProp;
    }

    int DDScalableRichText::GetDDCPIntensity()
    {
        int v = this->GetPropCommon(DDCPIntensityProp, true);
        if (v < 0) v += 256;
        return v;
    }

    void DDScalableRichText::SetDDCPIntensity(int intensity)
    {
        this->SetPropCommon(DDCPIntensityProp, intensity, true);
    }

    DDScalableTouchButton::~DDScalableTouchButton()
    {
        this->DestroyAll(true);
    }

    IClassInfo* DDScalableTouchButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDScalableTouchButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDScalableTouchButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    bool DDScalableTouchButton::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = TouchButton::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        //if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FontProp))
        //{
        //    RedrawFontCore<DDScalableTouchButton>(this, &result);
        //}
        return result;
    }
    void DDScalableTouchButton::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FirstScaledImageProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                TouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableTouchButton>(this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
            RedrawFontCore<DDScalableTouchButton>(this, nullptr, this->GetNeedsFontResize());
        TouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDScalableTouchButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDScalableTouchButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        DirectUI::ActiveState;
    }

    HRESULT DDScalableTouchButton::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impAssociatedColorProp
        };
        return ClassInfo<DDScalableTouchButton, TouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableTouchButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableTouchButton::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        if (!this) return -1;
        if (this->IsDestroyed()) return -1;
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        auto v = useInt ? pv->GetInt() : pv->GetBool();
        pv->Release();
        return v;
    }

    void DDScalableTouchButton::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt)
    {
        Value* pv = useInt ? Value::CreateInt(iCreateInt) : Value::CreateBool(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::FirstScaledImageProp()
    {
        return &impFirstScaledImageProp;
    }

    int DDScalableTouchButton::GetFirstScaledImage()
    {
        return this->GetPropCommon(FirstScaledImageProp, true);
    }

    void DDScalableTouchButton::SetFirstScaledImage(int iFirstImage)
    {
        this->SetPropCommon(FirstScaledImageProp, iFirstImage, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::ScaledImageIntervalsProp()
    {
        return &impScaledImageIntervalsProp;
    }

    int DDScalableTouchButton::GetScaledImageIntervals()
    {
        int v = this->GetPropCommon(ScaledImageIntervalsProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableTouchButton::SetScaledImageIntervals(int iScaleIntervals)
    {
        this->SetPropCommon(ScaledImageIntervalsProp, iScaleIntervals, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::DrawTypeProp()
    {
        return &impDrawTypeProp;
    }

    int DDScalableTouchButton::GetDrawType()
    {
        return this->GetPropCommon(DrawTypeProp, true);
    }

    void DDScalableTouchButton::SetDrawType(int iDrawType)
    {
        this->SetPropCommon(DrawTypeProp, iDrawType, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::EnableAccentProp()
    {
        return &impEnableAccentProp;
    }

    bool DDScalableTouchButton::GetEnableAccent()
    {
        return this->GetPropCommon(EnableAccentProp, false);
    }

    void DDScalableTouchButton::SetEnableAccent(bool bEnableAccent)
    {
        this->SetPropCommon(EnableAccentProp, bEnableAccent, false);
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::NeedsFontResizeProp()
    {
        return &impNeedsFontResizeProp;
    }

    bool DDScalableTouchButton::GetNeedsFontResize()
    {
        return this->GetPropCommon(NeedsFontResizeProp, false);
    }

    void DDScalableTouchButton::SetNeedsFontResize(bool bNeedsFontResize)
    {
        this->SetPropCommon(NeedsFontResizeProp, bNeedsFontResize, false);
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::AssociatedColorProp()
    {
        return &impAssociatedColorProp;
    }

    int DDScalableTouchButton::GetAssociatedColor()
    {
        return this->GetPropCommon(AssociatedColorProp, true);
    }

    void DDScalableTouchButton::SetAssociatedColor(int iAssociatedColor)
    {
        this->SetPropCommon(AssociatedColorProp, iAssociatedColor, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::DDCPIntensityProp()
    {
        return &impDDCPIntensityProp;
    }

    int DDScalableTouchButton::GetDDCPIntensity()
    {
        int v = this->GetPropCommon(DDCPIntensityProp, true);
        if (v < 0) v += 256;
        return v;
    }

    void DDScalableTouchButton::SetDDCPIntensity(int intensity)
    {
        this->SetPropCommon(DDCPIntensityProp, intensity, true);
    }

    RegKeyValue DDScalableTouchButton::GetRegKeyValue()
    {
        return _rkv;
    }

    void (*DDScalableTouchButton::GetAssociatedFn())(bool, bool, bool)
    {
        return _assocFn;
    }

    void* DDScalableTouchButton::GetAssociatedSetting()
    {
        return _assocSetting;
    }

    unsigned short DDScalableTouchButton::GetGroupColor()
    {
        return _gc;
    }

    bool DDScalableTouchButton::GetShellInteraction()
    {
        return _shellinteraction;
    }

    void DDScalableTouchButton::SetRegKeyValue(RegKeyValue rkvNew)
    {
        _rkv = rkvNew;
    }

    void DDScalableTouchButton::SetAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3)
    {
        _assocFn = pfn;
        _fnb1 = fnb1;
        _fnb2 = fnb2;
        _fnb3 = fnb3;
    }

    void DDScalableTouchButton::SetAssociatedSetting(void* pb)
    {
        _assocSetting = pb;
    }

    void DDScalableTouchButton::SetGroupColor(unsigned short sGC)
    {
        _gc = sGC;
    }

    void DDScalableTouchButton::SetShellInteraction(bool bShellInteraction)
    {
        _shellinteraction = bShellInteraction;
    }

    void DDScalableTouchButton::ExecAssociatedFn(void (*pfn)(bool, bool, bool))
    {
        pfn(_fnb1, _fnb2, _fnb3);
    }

    DDScalableTouchEdit::~DDScalableTouchEdit()
    {
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

    bool DDScalableTouchEdit::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = Element::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::ContentProp) || PropNotify::IsEqual(ppi, iIndex, TouchEdit2::PromptTextProp))
        {
            result = false;
            this->_SetValue(Element::AccNameProp, 1, pvNew, false);
            ElementSetValue(_peEdit, ppi, pvNew, this);
            ElementSetValue(_pePreview, Element::ContentProp(), pvNew, this);
        }
        return result;
    }

    void DDScalableTouchEdit::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::KeyWithinProp))
        {
            CValuePtr v;
            _pePreview->SetClass(L"");
            _pePreview->SetVisible(!_peEdit->GetKeyWithin());
            _pePreview->SetContentString(_peEdit->GetContentString(&v));
            if (!_peEdit->GetContentString(&v))
            {
                if (_peEdit->GetPromptText(&v))
                {
                    _pePreview->SetClass(L"prompttext");
                    _pePreview->SetContentString(_peEdit->GetPromptText(&v));
                }
                else _pePreview->SetContentString(L"");
            }
            ElementSetValue(_peBackground, Element::SelectedProp(), pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::MouseWithinProp))
            ElementSetValue(_peBackground, Element::OverhangProp(), pvNew, this);
        if (PropNotify::IsEqual(ppi, iIndex, Element::EnabledProp))
            ElementSetValue(_peBackground, ppi, pvNew, this);
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
        {
            RedrawFontCore<DDScalableElement>(_pePreview, nullptr, this->GetNeedsFontResize());
            RedrawFontCore<TouchEdit2>(_peEdit, nullptr, this->GetNeedsFontResize());
        }
        Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDScalableTouchEdit::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDScalableTouchEdit, int>(0, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDScalableTouchEdit::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((Element*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateTEVisual();
        return hr;
    }

    HRESULT DDScalableTouchEdit::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            TouchEdit2::PromptTextProp(),
            &impNeedsFontResizeProp
        };
        return ClassInfo<DDScalableTouchEdit, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableTouchEdit", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableTouchEdit::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
        if (!this) return -1;
        if (this->IsDestroyed()) return -1;
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

    const PropertyInfo* WINAPI DDScalableTouchEdit::PromptTextProp()
    {
        return TouchEdit2::PromptTextProp();
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::NeedsFontResizeProp()
    {
        return &impNeedsFontResizeProp;
    }

    const WCHAR* DDScalableTouchEdit::GetPromptText(Value** ppv)
    {
        if (_peEdit) return _peEdit->GetPromptText(ppv);
        else return nullptr;
    }

    const WCHAR* DDScalableTouchEdit::GetContentString(Value** ppv)
    {
        if (_peEdit) return _peEdit->GetContentString(ppv);
        else return nullptr;
    }

    bool DDScalableTouchEdit::GetNeedsFontResize()
    {
        return this->GetPropCommon(NeedsFontResizeProp, false);
    }

    void DDScalableTouchEdit::SetNeedsFontResize(bool bNeedsFontResize)
    {
        this->SetPropCommon(NeedsFontResizeProp, bNeedsFontResize, false);
    }

    void DDScalableTouchEdit::SetKeyFocus()
    {
        _peEdit->SetKeyFocus();
        Element::SetKeyFocus();
    }

    HRESULT DDScalableTouchEdit::_CreateTEVisual()
    {
        HRESULT hr = S_OK;
        CValuePtr v;

        FillLayout::Create(0, nullptr, &v);
        this->SetValue(Element::LayoutProp, 1, v);

        hr = DDScalableElement::Create(this, nullptr, (Element**)&_peBackground);
        if (SUCCEEDED(hr))
        {
            this->Add((Element**)&_peBackground, 1);
            _peBackground->SetID(L"TE_Background");
            _peBackground->SetEnabled(this->GetEnabled());;
            hr = TouchEdit2::Create(this, nullptr, (Element**)&_peEdit);
            if (SUCCEEDED(hr))
            {
                this->Add((Element**)&_peEdit, 1);
                _peEdit->SetID(L"TE_EditBox");
                hr = DDScalableElement::Create(this, nullptr, (Element**)&_pePreview);
                if (SUCCEEDED(hr))
                {
                    this->Add((Element**)&_pePreview, 1);
                    _pePreview->SetID(L"TE_Preview");
                }
            }
        }
        return hr;
    }

    LVItem::~LVItem()
    {
        if (_childItemss != nullptr)
        {
            _childItemss->clear();
            _childItemss = nullptr;
        }
        this->ClearAllListeners();
        for (auto pel : _pels)
        {
            delete pel;
        }
        _pels.clear();
        if (_touchGrid) if (_touchGrid->GetItemCount() == 0) delete _touchGrid;
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
        return CreateAndInit<LVItem, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT LVItem::Register()
    {
        return ClassInfo<LVItem, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"LVItem", nullptr, 0);
    }

    unsigned short LVItem::GetInternalXPos()
    {
        return _xPos;
    }

    unsigned short LVItem::GetInternalYPos()
    {
        return _yPos;
    }

    unsigned short LVItem::GetMemXPos()
    {
        return _mem_xPos;
    }

    unsigned short LVItem::GetMemYPos()
    {
        return _mem_yPos;
    }

    void LVItem::SetInternalXPos(unsigned short iXPos)
    {
        _xPos = iXPos;
    }

    void LVItem::SetInternalYPos(unsigned short iYPos)
    {
        _yPos = iYPos;
    }

    void LVItem::SetMemXPos(unsigned short iXPos)
    {
        _mem_xPos = iXPos;
    }

    void LVItem::SetMemYPos(unsigned short iYPos)
    {
        _mem_yPos = iYPos;
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

    bool LVItem::GetFlying()
    {
        return _flying;
    }

    bool LVItem::GetMoving()
    {
        return _moving;
    }

    bool LVItem::GetHasAdvancedIcon()
    {
        return _hai;
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

    void LVItem::SetFlying(bool flying)
    {
        _flying = flying;
    }

    void LVItem::SetMoving(bool moving)
    {
        _moving = moving;
    }

    void LVItem::SetHasAdvancedIcon(bool hai)
    {
        _hai = hai;
    }

    unsigned short LVItem::GetPage()
    {
        return _page;
    }

    unsigned short LVItem::GetMemPage()
    {
        return _mem_page;
    }

    unsigned short LVItem::GetPreRefreshMemPage()
    {
        return _prmem_page;
    }

    unsigned short LVItem::GetMemIconSize()
    {
        return _mem_iconsize;
    }

    unsigned short LVItem::GetItemCount()
    {
        return _itemCount;
    }

    unsigned short LVItem::GetItemIndex()
    {
        return _itemIndex;
    }

    void LVItem::SetPage(unsigned short pageID)
    {
        _page = pageID;
    }

    void LVItem::SetMemPage(unsigned short pageID)
    {
        _mem_page = pageID;
    }

    void LVItem::SetPreRefreshMemPage(unsigned short pageID)
    {
        _prmem_page = pageID;
    }

    void LVItem::SetMemIconSize(unsigned short iconsz)
    {
        _mem_iconsize = iconsz;
    }

    void LVItem::SetItemCount(unsigned short itemCount)
    {
        _itemCount = itemCount;
    }

    void LVItem::SetItemIndex(unsigned short itemIndex)
    {
        _itemIndex = itemIndex;
    }

    LVItemGroupSize LVItem::GetGroupSize()
    {
        return _groupsize;
    }

    void LVItem::SetGroupSize(LVItemGroupSize lvigs)
    {
        _groupsize = lvigs;
    }

    LVItemTileSize LVItem::GetTileSize()
    {
        return _tilesize;
    }

    void LVItem::SetTileSize(LVItemTileSize lvits)
    {
        _tilesize = lvits;
    }

    LVItemOpenDirState LVItem::GetOpenDirState()
    {
        return _opendirstate;
    }

    void LVItem::SetOpenDirState(LVItemOpenDirState lviods)
    {
        _opendirstate = lviods;
    }

    BYTE LVItem::GetSmallPos()
    {
        return _smallPos;
    }

    void LVItem::SetSmallPos(BYTE smPos)
    {
        _smallPos = smPos;
    }

    LVItemTouchGrid* LVItem::GetTouchGrid()
    {
        return _touchGrid;
    }

    void LVItem::SetTouchGrid(LVItemTouchGrid* lvitg)
    {
        if (_touchGrid) _touchGrid->Erase(_smallPos - 1);
        _touchGrid = lvitg;
        if (_touchGrid) _touchGrid->Insert(this);
    }

    void LVItem::SetTouchGrid(LVItemTouchGrid* lvitg, BYTE index)
    {
        if (_touchGrid) _touchGrid->Erase(_smallPos - 1);
        _touchGrid = lvitg;
        if (_touchGrid) _touchGrid->Insert(this, index);
    }

    DDScalableElement* LVItem::GetIcon()
    {
        return _peIcon;
    }

    Element* LVItem::GetShortcutArrow()
    {
        return _peShortcutArrow;
    }

    RichText* LVItem::GetText()
    {
        return _peText;
    }

    TouchButton* LVItem::GetCheckbox()
    {
        return _peCheckbox;
    }

    void LVItem::SetIcon(DDScalableElement* peIcon)
    {
        _peIcon = peIcon;
    }

    void LVItem::SetShortcutArrow(Element* peShortcutArrow)
    {
        _peShortcutArrow = peShortcutArrow;
    }

    void LVItem::SetText(RichText* peText)
    {
        _peText = peText;
    }

    void LVItem::SetCheckbox(TouchButton* peCheckbox)
    {
        _peCheckbox = peCheckbox;
    }

    vector<LVItem*>* LVItem::GetChildItems()
    {
        return _childItemss;
    }

    void LVItem::SetChildItems(vector<LVItem*>* vpm)
    {
        _childItemss = vpm;
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

    void LVItemTouchGrid::Insert(LVItem* lvi)
    {
        if (_itemCount >= _maxCount) return;
        _items[_itemCount] = lvi;
        if (_itemCount == 0)
        {
            _xFirstTile = lvi->GetMemXPos();
            _yFirstTile = lvi->GetMemYPos();
        }
        _itemCount++;
        lvi->SetSmallPos(_itemCount);
        _RefreshLVItemPositions(_itemCount - 1);
    }

    void LVItemTouchGrid::Insert(LVItem* lvi, BYTE index)
    {
        if (_itemCount >= _maxCount || index >= _maxCount || index < 0) return;
        BYTE internalIndex = index;
        if (index > _itemCount) internalIndex = _itemCount;
        for (int i = _itemCount - 1; i >= index; i--)
            _items[i + 1] = _items[i];
        _items[internalIndex] = lvi;
        if (_itemCount == 0)
        {
            _xFirstTile = lvi->GetMemXPos();
            _yFirstTile = lvi->GetMemYPos();
        }
        _itemCount++;
        _RefreshLVItemPositions(internalIndex);
    }

    void LVItemTouchGrid::Erase(BYTE index)
    {
        if (index < 0 || index >= _itemCount) return;
        for (int i = index; i < _itemCount - 1; i++)
        {
            _items[i] = _items[i + 1];
            _items[i]->SetSmallPos(i + 1);
        }
        _itemCount--;
        _items[_itemCount] = nullptr;
        if (_itemCount > 0) _RefreshLVItemPositions(index);
        else delete this;
    }

    BYTE LVItemTouchGrid::GetItemCount()
    {
        return _itemCount;
    }

    void LVItemTouchGrid::_RefreshLVItemPositions(BYTE index)
    {
        short localeDirection = (localeType == 1) ? -1 : 1;
        for (int i = index; i < _itemCount; i++)
        {
            _items[i]->SetMemXPos(_xFirstTile + ((i & 1) * (g_touchSizeX + DESKPADDING_TOUCH) / 2 * localeDirection));
            _items[i]->SetX(_items[i]->GetMemXPos());
            _items[i]->SetMemYPos(_yFirstTile + (i / 2) * (g_touchSizeY + DESKPADDING_TOUCH) / 2);
            _items[i]->SetY(_items[i]->GetMemYPos());
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
        return CreateAndInit<DDLVActionButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDLVActionButton::Register()
    {
        return ClassInfo<DDLVActionButton, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDLVActionButton", nullptr, 0);
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
        return CreateAndInit<DDToggleButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDToggleButton::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impCheckedStateProp
        };
        return ClassInfo<DDToggleButton, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDToggleButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
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
        return CreateAndInit<DDCheckBoxGlyph, int>(0, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDCheckBoxGlyph::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impCheckedStateProp
        };
        return ClassInfo<DDCheckBoxGlyph, DDScalableElement>::RegisterGlobal(HINST_THISCOMPONENT, L"DDCheckBoxGlyph", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
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

    bool DDCheckBox::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = Element::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::ContentProp))
        {
            result = false;
            this->_SetValue(Element::AccNameProp, 1, pvNew, false);
            ElementSetValue(_peText, ppi, pvNew, this);
        }
        return result;
    }

    void DDCheckBox::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp()))
        {
            ElementSetValue(_peGlyph, ppi, pvNew, this);
            ElementSetValue(_peText, ppi, pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::ShortcutProp()))
            ElementSetValue(_peText, ppi, pvNew, this);
        if (PropNotify::IsEqual(ppi, iIndex, TouchButton::PressedProp()))
            ElementSetValue(_peGlyph, Element::SelectedProp(), pvNew, this);
        if (PropNotify::IsEqual(ppi, iIndex, DDCheckBox::CheckedStateProp()))
            ElementSetValue(_peGlyph, ppi, pvNew, this);
        DDScalableTouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDCheckBox::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDCheckBox, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDCheckBox::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((DDScalableTouchButton*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateCBVisual();
        return hr;
    }

    HRESULT DDCheckBox::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impCheckedStateProp
        };
        return ClassInfo<DDCheckBox, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDCheckBox", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
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

    HRESULT DDCheckBox::_CreateCBVisual()
    {
        HRESULT hr = S_OK;

        int layoutParams[4] = { 0, 2, 0, 2 };
        CValuePtr spvLayout;
        FlowLayout::Create(ARRAYSIZE(layoutParams), layoutParams, &spvLayout);
        hr = this->_SetValue(Element::LayoutProp, 1, spvLayout, true);
        if (SUCCEEDED(hr))
        {
            hr = DDCheckBoxGlyph::Create(this, nullptr, (Element**)&_peGlyph);
            if (SUCCEEDED(hr))
            {
                this->Add((Element**)&_peGlyph, 1);
                _peGlyph->SetCheckedState(this->GetCheckedState());
                _peGlyph->SetID(L"DDCB_Glyph");
                hr = DDScalableElement::Create(this, nullptr, (Element**)&_peText);
                if (SUCCEEDED(hr))
                {
                    this->Add((Element**)&_peText, 1);
                    _peText->SetID(L"DDCB_Text");
                }
            }
        }
        return hr;
    }


    IClassInfo* DDNumberedButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDNumberedButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDNumberedButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void DDNumberedButton::OnEvent(Event* pEvent)
    {
        if (pEvent->uidType == TouchButton::Click)
        {
            CValuePtr v;
            LPCWSTR className = this->GetClass(&v);
            if (wcscmp(className, L"tab") == 0)
            {
                ((DDTabbedPages*)this->_peLinked)->TraversePage(this->_id);
            }
            if (wcscmp(className, L"cmbsel") == 0)
            {
                ((DDCombobox*)this->_peLinked)->SetSelection(this->_id);
                ((DDCombobox*)this->_peLinked)->ToggleSelectionList(true);
            }
        }
        TouchButton::OnEvent(pEvent);
    }

    HRESULT DDNumberedButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDNumberedButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDNumberedButton::Register()
    {
        return ClassInfo<DDNumberedButton, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDNumberedButton", nullptr, 0);
    }

    void DDNumberedButton::SetNumberID(BYTE id)
    {
        _id = id;
    }

    void DDNumberedButton::SetLinkedElement(void* peLinked)
    {
        _peLinked = peLinked;
    }

    LRESULT CALLBACK ComboboxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch (uMsg)
        {
        case WM_CLOSE:
            return 0;
        case WM_DESTROY:
            return 0;
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) ShowWindow(hWnd, SW_HIDE);
            break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    DDCombobox::~DDCombobox()
    {
        this->DestroyAll(true);
        _wndSelectionMenu->DestroyWindow();
    }

    IClassInfo* DDCombobox::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDCombobox::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDCombobox::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void DDCombobox::OnEvent(Event* pEvent)
    {
        if (pEvent->uidType == TouchButton::Click)
        {
            this->ToggleSelectionList(false);
        }
        TouchButton::OnEvent(pEvent);
    }

    UID WINAPI DDCombobox::SelectionChange()
    {
        return Combobox::SelectionChange();
    }

    HRESULT DDCombobox::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDCombobox, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDCombobox::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((DDScalableTouchButton*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateCMBVisual();
        return hr;
    }

    HRESULT DDCombobox::Register()
    {
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impListMaxHeightProp
        };
        return ClassInfo<DDCombobox, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDCombobox", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    const PropertyInfo* WINAPI DDCombobox::ListMaxHeightProp()
    {
        return &impListMaxHeightProp;
    }

    int DDCombobox::GetListMaxHeight()
    {
        return this->GetPropCommon(ListMaxHeightProp, true);
    }

    void DDCombobox::SetListMaxHeight(int iListMaxHeight)
    {
        this->SetPropCommon(ListMaxHeightProp, iListMaxHeight, true);
    }

    void DDCombobox::InsertSelection(BYTE index, LPCWSTR pszSelectionStr)
    {
        if (index == MAX_SELECTIONS) index = _selSize;
        if (_selSize >= MAX_SELECTIONS || index < 0 || index > _selSize)
            return;
        for (int i = _selSize - 1; i >= index; i--)
        {
            _peSelections[i + 1] = _peSelections[i];
            _peSelections[i + 1]->SetNumberID(i + 1);
        }
        DDNumberedButton::Create(_peHostInner, nullptr, (Element**)&(_peSelections[index]));
        _peHostInner->Insert((Element**)&_peSelections[index], 1, index);
        _peSelections[index]->SetNumberID(index);
        _peSelections[index]->SetLinkedElement(this);
        _peSelections[index]->SetContentString(pszSelectionStr);
        _peSelections[index]->SetClass(L"cmbsel");
        _selSize++;
    }

    void DDCombobox::EraseSelection(BYTE index)
    {
        if (index < 0 || index >= _selSize)
            return;
        _peSelections[index]->Destroy(true);
        _peSelections[index] = nullptr;
        for (int i = index; i < _selSize - 1; i++)
        {
            _peSelections[i] = _peSelections[i + 1];
            _peSelections[i]->SetNumberID(i);
        }
        _selSize--;
    }

    BYTE DDCombobox::GetSelection()
    {
        return _selID;
    }

    void DDCombobox::SetSelection(BYTE index)
    {
        if (index < 0 || index >= _selSize) return;
        _peSelections[_selID]->SetSelected(false);
        _peSelections[index]->SetSelected(true);
        CValuePtr v;
        _selID = index; 
        this->SetContentString(_peSelections[index]->GetContentString(&v));
        Event ev;
        ev.uidType._address = nullptr;
        ev.uidType = DDCombobox::SelectionChange();
        this->FireEvent(&ev, true, false);
    }

    void DDCombobox::ToggleSelectionList(bool fForceHide)
    {
        if (IsWindowVisible(_wndSelectionMenu->GetHWND()) || fForceHide)
        {
            _wndSelectionMenu->ShowWindow(SW_HIDE);
        }   
        else
        {
            POINT ptRoot{}, ptDest;
            RECT rcRoot, rcDest, rcElement{}, rcList{}, rcSelected{}, dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            GetWindowRect(((HWNDElement*)this->GetRoot())->GetHWND(), &rcRoot);
            this->GetRoot()->MapElementPoint(this, &ptRoot, &ptDest);
            GetGadgetRect(this->GetDisplayNode(), &rcElement, 0xC);
            GetGadgetRect(_peHostInner->GetDisplayNode(), &rcList, 0xC);
            rcList.top -= round(g_flScaleFactor), rcList.bottom += round(g_flScaleFactor); // Window borders, left and right are unused
            Element* peSelected = _peSelections[_selID] ? _peSelections[_selID] : _peHostInner;
            GetGadgetRect(peSelected->GetDisplayNode(), &rcSelected, 0xC);
            LONG halfHeight = (this->GetListMaxHeight() - rcElement.bottom + rcElement.top) / 2;
            rcDest.left = rcRoot.left + ptDest.x;
            rcDest.top = rcRoot.top + ptDest.y - halfHeight;
            rcDest.right = rcElement.right - rcElement.left;
            rcDest.bottom = min(rcList.bottom - rcList.top, this->GetListMaxHeight());
            if (rcDest.bottom < this->GetListMaxHeight())
            {
                rcDest.top += halfHeight - rcSelected.top - round(g_flScaleFactor);
            }
            else
            {
                _tsvSelectionMenu->SetYOffset(rcSelected.top - rcList.top - halfHeight);
                if (halfHeight > rcSelected.top - rcList.top)
                {
                    _tsvSelectionMenu->SetYOffset(0);
                    rcDest.top += halfHeight - rcSelected.top + rcList.top + _tsvSelectionMenu->GetYOffset();
                }
                if (halfHeight > rcList.bottom - rcSelected.bottom)
                {
                    _tsvSelectionMenu->SetYOffset(rcList.bottom - rcList.top - rcDest.bottom);
                    rcDest.top += halfHeight - rcSelected.top + rcList.top + _tsvSelectionMenu->GetYOffset();
                }
                if (rcDest.top < 0) rcDest.top = 0;
                if (rcDest.bottom > dimensions.bottom) rcDest.bottom = dimensions.bottom;
            }
            SetWindowPos(_wndSelectionMenu->GetHWND(), HWND_TOPMOST, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, NULL);
            _wndSelectionMenu->ShowWindow(SW_SHOW);
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerFade(_peHostInner, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, _peHostInner->GetDisplayNode(), &tsbInfo);
        }
    }

    HRESULT DDCombobox::_CreateCMBVisual()
    {
        HRESULT hr = S_OK;
        DWORD keyC{};
        CValuePtr spvLayout;
        BorderLayout::Create(0, nullptr, &spvLayout);
        this->SetValue(Element::LayoutProp, 1, spvLayout);
        hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peDropDownGlyph);
        if (SUCCEEDED(hr))
        {
            this->Add((Element**)&_peDropDownGlyph, 1);
            _peDropDownGlyph->SetID(L"DDCMB_DropDownGlyph");
            hr = NativeHWNDHost::Create(L"DDCMBMenuWindow", nullptr, nullptr, nullptr, 0, 0, 0, 0, WS_EX_TOOLWINDOW, WS_POPUP | WS_BORDER | CBS_DROPDOWNLIST, HINST_THISCOMPONENT, 0x43, &_wndSelectionMenu);
            if (SUCCEEDED(hr))
            {
                HWNDElement::Create(_wndSelectionMenu->GetHWND(), true, 0x38, nullptr, &keyC, (Element**)&_peSelectionMenu);
                SetWindowSubclass(_wndSelectionMenu->GetHWND(), ComboboxProc, 1, (DWORD_PTR)this);
                _peSelectionMenu->SetVisible(true);
                _peSelectionMenu->EndDefer(keyC);
                _wndSelectionMenu->Host(_peSelectionMenu);

                if (DWMActive)
                {
                    WCHAR* WindowsBuildStr;
                    GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
                    int WindowsBuild = _wtoi(WindowsBuildStr);
                    free(WindowsBuildStr);
                    int WindowsRev = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\BuildLayers\\ShellCommon", L"BuildQfe");
                    if (WindowsBuild > 22000 || WindowsBuild == 22000 && WindowsRev >= 51)
                    {
                        DWORD cornerPreference = DWMWCP_ROUND;
                        DwmSetWindowAttribute(_wndSelectionMenu->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
                    }
                }

                FillLayout::Create(0, nullptr, &spvLayout);
                _peSelectionMenu->SetValue(Element::LayoutProp, 1, spvLayout);
                LPWSTR sheetName = g_theme ? (LPWSTR)L"default" : (LPWSTR)L"defaultdark";
                StyleSheet* sheet = _peSelectionMenu->GetSheet();
                CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                parser->GetSheet(sheetName, &sheetStorage);
                _peSelectionMenu->SetValue(Element::SheetProp, 1, sheetStorage);
                free(sheet);
                _peSelectionMenu->SetID(L"DDCMB_SelectionList");
                hr = TouchScrollViewer::Create(_peSelectionMenu, nullptr, (Element**)&_tsvSelectionMenu);
                if (SUCCEEDED(hr))
                {
                    _tsvSelectionMenu->SetLayoutPos(-1);
                    _tsvSelectionMenu->SetActive(0xB);
                    _tsvSelectionMenu->SetXBarVisibility(0);
                    _tsvSelectionMenu->SetYBarVisibility(0);
                    _tsvSelectionMenu->SetXScrollable(false);
                    _tsvSelectionMenu->SetYScrollable(true);
                    _tsvSelectionMenu->SetInteractionMode(18);
                    hr = Element::Create(0, _tsvSelectionMenu, nullptr, &_peHostInner);
                    if (SUCCEEDED(hr))
                    {
                        BorderLayout::Create(0, nullptr, &spvLayout);
                        _peHostInner->SetValue(Element::LayoutProp, 1, spvLayout);
                        _peHostInner->SetLayoutPos(-1);
                        _peHostInner->SetID(L"DDCMB_SelectionListInner");
                        _wndSelectionMenu->ShowWindow(SW_HIDE);
                        if (_peHostInner)
                        {
                            _tsvSelectionMenu->Add(&_peHostInner, 1);
                            _peSelectionMenu->Add((Element**)&_tsvSelectionMenu, 1);
                            BlurBackground(_wndSelectionMenu->GetHWND(), true, false, -1, nullptr);
                            if (DWMActive)
                            {                                
                                AddLayeredRef(_tsvSelectionMenu->GetDisplayNode());
                                SetGadgetFlags(_tsvSelectionMenu->GetDisplayNode(), NULL, NULL);
                                MARGINS margins = { -1, -1, -1, -1 };
                                DwmExtendFrameIntoClientArea(_wndSelectionMenu->GetHWND(), &margins);
                            }
                        }
                    }
                }
            }
        }
        return hr;
    }

    DDSlider::~DDSlider()
    {
        this->DestroyAll(true);
    }

    IClassInfo* DDSlider::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDSlider::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDSlider::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void DDSlider::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp()))
        {
            ElementSetValue(_peTrack, ppi, pvNew, this);
            ElementSetValue(_peFill, ppi, pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDSlider::IsVerticalProp) ||
            PropNotify::IsEqual(ppi, iIndex, Element::WidthProp) || PropNotify::IsEqual(ppi, iIndex, Element::HeightProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDSlider::TextWidthProp) || PropNotify::IsEqual(ppi, iIndex, DDSlider::TextHeightProp))
        {
            _RedrawSlider();
        }
        TouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    void DDSlider::OnInput(InputEvent* pInput)
    {
        if (pInput->nCode == GMOUSE_MOVE)
        {
            GetCursorPos(&_ptBeforeClick);
            ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &_ptBeforeClick);
        }
        if (pInput->nCode == GMOUSE_DOWN && pInput->nDevice != GINPUT_KEYBOARD)
        {
            POINT ptRoot;
            GetCursorPos(&ptRoot);
            ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &ptRoot);
            _peSliderInner->MapElementPoint(this->GetRoot(), &ptRoot, &_ptOnClick);
            s_AnimateThumb(_peThumb, TouchButton::CapturedProp(), 5, nullptr, nullptr);
        }
        if (pInput->nCode == GMOUSE_DOWN || pInput->nCode == GMOUSE_DRAG)
        {
            if ((pInput->uModifiers == 0 && pInput->nStage == GMF_BUBBLED) || pInput->nDevice != GINPUT_KEYBOARD)
            {
                static POINT ppt;
                GetCursorPos(&ppt);
                ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &ppt);
                static bool vertical = this->GetIsVertical();
                static bool canMove{};
                static float percentage{}, assocVal{};
                static short sLeft, sUp, sRight, sDown;
                static int width{}, height{};
                if (vertical)
                {
                    int sliderSize = this->GetHeight() - this->GetTextHeight();
                    if (pInput->nDevice != GINPUT_KEYBOARD) canMove = true;
                    sUp = GetAsyncKeyState(VK_UP);
                    sDown = GetAsyncKeyState(VK_DOWN);
                    if (sUp & 1 || sUp & 0x8000)
                    {
                        height = _peFillBase->GetHeight() + round((sliderSize - _peThumb->GetHeight() / 2) / 10);
                        canMove = true;
                    }
                    else if (sDown & 1 || sUp & 0x8000)
                    {
                        height = _peFillBase->GetHeight() - round((sliderSize - _peThumb->GetHeight() / 2) / 10);
                        canMove = true;
                    }
                    else height = ppt.y - _ptBeforeClick.y + _ptOnClick.y;
                    if (height < _peThumb->GetHeight() / 2) height = _peThumb->GetHeight() / 2;
                    if (height > sliderSize - _peThumb->GetHeight() / 2) height = sliderSize - _peThumb->GetHeight() / 2;
                    int fillheight = sliderSize - height;
                    if (canMove)
                    {
                        _peTrackBase->SetHeight(height);
                        _peFillBase->SetHeight(fillheight);
                        _peThumb->SetY(height - _peThumb->GetHeight() / 2);
                        percentage = static_cast<float>(fillheight - _peThumb->GetHeight() / 2) / (sliderSize - _peThumb->GetHeight());
                    }
                }
                else
                {
                    static short localeDirection = (localeType == 1) ? -1 : 1;
                    int sliderSize = this->GetWidth() - this->GetTextWidth();
                    if (pInput->nDevice != GINPUT_KEYBOARD) canMove = true;
                    sLeft = GetAsyncKeyState(VK_LEFT);
                    sRight = GetAsyncKeyState(VK_RIGHT);
                    if (sLeft & 1 || sLeft & 0x8000)
                    {
                        if (localeType == 1) width = _peTrackBase->GetWidth() - round((sliderSize - _peThumb->GetWidth() / 2) / 10);
                        else width = _peFillBase->GetWidth() - round((sliderSize - _peThumb->GetWidth() / 2) / 10);
                        canMove = true;
                    }
                    else if (sRight & 1 || sRight & 0x8000)
                    {
                        if (localeType == 1) width = _peTrackBase->GetWidth() + round((sliderSize - _peThumb->GetWidth() / 2) / 10);
                        else width = _peFillBase->GetWidth() + round((sliderSize - _peThumb->GetWidth() / 2) / 10);
                        canMove = true;
                    }
                    else width = ppt.x - _ptBeforeClick.x + _ptOnClick.x;
                    if (width < _peThumb->GetWidth() / 2) width = _peThumb->GetWidth() / 2;
                    if (width > sliderSize - _peThumb->GetWidth() / 2) width = sliderSize - _peThumb->GetWidth() / 2;
                    int fillwidth = sliderSize - width;
                    if (canMove)
                    {
                        _peTrackBase->SetWidth((localeType == 1) ? width : fillwidth);
                        _peFillBase->SetWidth((localeType == 1) ? fillwidth : width);
                        _peThumb->SetX(width - _peThumb->GetWidth() / 2);
                        percentage = static_cast<float>(((localeType == 1) ? fillwidth : width) - _peThumb->GetWidth() / 2) / (sliderSize - _peThumb->GetWidth());
                    }
                }
                if (canMove)
                {
                    if (percentage < 0) percentage = 0;
                    if (percentage > 1) percentage = 1;
                    assocVal = _minValue + (_maxValue - _minValue) * percentage;
                    if (_tickValue > 0) assocVal = round(assocVal / _tickValue) * _tickValue;
                    WCHAR formattedNum[8];
                    StringCchPrintfW(formattedNum, 8, _szFormatted, assocVal);
                    _peText->SetContentString(formattedNum);
                }
                if (pInput->nDevice == GINPUT_KEYBOARD)
                {
                    this->SetCurrentValue(NULL, true);
                    _peThumb->SetKeyFocus();
                }
                sLeft = 0, sUp = 0, sRight = 0, sDown = 0;
                canMove = false;
            }
        }
        if (pInput->nCode == GMOUSE_UP)
        {
            this->SetCurrentValue(NULL, true);
        }
        TouchButton::OnInput(pInput);
    }

    HRESULT DDSlider::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDSlider, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDSlider::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((TouchButton*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateDDSVisual();
        return hr;
    }

    HRESULT DDSlider::Register()
    {
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impIsVerticalProp,
            &impTextWidthProp,
            &impTextHeightProp
        };
        return ClassInfo<DDSlider, TouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDSlider", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    int DDSlider::GetPropCommon(const PropertyProcT pPropertyProc)
    {
        if (!this) return -1;
        if (this->IsDestroyed()) return -1;
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        int v = pv->GetInt();
        pv->Release();
        return v;
    }

    void DDSlider::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt)
    {
        Value* pv = Value::CreateInt(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    const PropertyInfo* WINAPI DDSlider::IsVerticalProp()
    {
        return &impIsVerticalProp;
    }

    bool DDSlider::GetIsVertical()
    {
        return this->GetPropCommon(IsVerticalProp);
    }

    void DDSlider::SetIsVertical(bool bIsVertical)
    {
        this->SetPropCommon(IsVerticalProp, bIsVertical);
    }

    const PropertyInfo* WINAPI DDSlider::TextWidthProp()
    {
        return &impTextWidthProp;
    }

    int DDSlider::GetTextWidth()
    {
        return this->GetPropCommon(TextWidthProp);
    }

    void DDSlider::SetTextWidth(int iTextWidth)
    {
        this->SetPropCommon(TextWidthProp, iTextWidth);
    }

    const PropertyInfo* WINAPI DDSlider::TextHeightProp()
    {
        return &impTextHeightProp;
    }

    int DDSlider::GetTextHeight()
    {
        return this->GetPropCommon(TextHeightProp);
    }

    void DDSlider::SetTextHeight(int iTextHeight)
    {
        this->SetPropCommon(TextHeightProp, iTextHeight);
    }

    RegKeyValue DDSlider::GetRegKeyValue()
    {
        return _rkv;
    }

    void DDSlider::SetRegKeyValue(RegKeyValue rkvNew)
    {
        _rkv = rkvNew;
    }

    float DDSlider::GetMinValue()
    {
        return _minValue;
    }

    float DDSlider::GetMaxValue()
    {
        return _maxValue;
    }

    float DDSlider::GetCurrentValue()
    {
        return _currValue;
    }

    float DDSlider::GetTickValue()
    {
        return _tickValue;
    }

    int* DDSlider::GetAssociatedValue()
    {
        return _assocVal;
    }

    void DDSlider::SetMinValue(float minValue)
    {
        _minValue = minValue;
        _RedrawSlider();
    }

    void DDSlider::SetMaxValue(float maxValue)
    {
        _maxValue = maxValue;
        _RedrawSlider();
    }

    void DDSlider::SetCurrentValue(float currValue, bool fExternal)
    {
        if (currValue < _minValue) currValue = _minValue;
        if (currValue > _maxValue) currValue = _maxValue;
        _currValue = currValue;
        if (fExternal)
        {
            bool vertical = this->GetIsVertical();
            CSafeElementPtr<TouchButton> peFill;
            peFill.Assign(regElem<TouchButton*>(L"DDS_FillBase", this));
            CSafeElementPtr<TouchButton> peThumb;
            peThumb.Assign(regElem<TouchButton*>(L"DDS_Thumb", this));
            int thumbOffset = vertical ? peThumb->GetHeight() : peThumb->GetWidth();
            int sliderSize = vertical ? this->GetHeight() - this->GetTextHeight() : this->GetWidth() - this->GetTextWidth();
            float percentage{};
            if (vertical) percentage = static_cast<float>(peFill->GetHeight() - thumbOffset / 2.0f) / (sliderSize - thumbOffset);
            else percentage = static_cast<float>(peFill->GetWidth() - thumbOffset / 2.0f) / (sliderSize - thumbOffset);
            if (percentage < 0) percentage = 0;
            if (percentage > 1) percentage = 1;
            float assocVal = _minValue + (_maxValue - _minValue) * percentage;
            if (_tickValue > 0) assocVal = round(assocVal / _tickValue) * _tickValue;
            if (_assocVal)
                (*_assocVal) = assocVal * _coef;
            RegKeyValue rkv = this->GetRegKeyValue();
            if (rkv._valueToFind)
                SetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind, assocVal * _coef, false, nullptr);
            g_atleastonesetting = true;
            _currValue = assocVal;
        }
        else _RedrawSlider();
    }

    void DDSlider::SetTickValue(float tickValue)
    {
        _tickValue = tickValue;
    }

    void DDSlider::SetAssociatedValue(int* assocVal, int extValueMultiplier)
    {
        _assocVal = assocVal;
        _coef = extValueMultiplier;
    }

    LPCWSTR DDSlider::GetFormattedString()
    {
        return _szFormatted;
    }

    void DDSlider::SetFormattedString(LPCWSTR szFormatted)
    {
        _szFormatted = szFormatted;
        WCHAR formattedNum[8];
        StringCchPrintfW(formattedNum, 8, _szFormatted, _currValue);
        _peText->SetContentString(formattedNum);
    }

    HRESULT DDSlider::_CreateDDSVisual()
    {
        HRESULT hr = S_OK;
        CValuePtr spvLayout;
        BorderLayout::Create(0, nullptr, &spvLayout);
        this->SetValue(Element::LayoutProp, 1, spvLayout);
        hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peText);
        if (SUCCEEDED(hr))
        {
            this->Add((Element**)&_peText, 1);
            _peText->SetID(L"DDS_Text");
            FillLayout::Create(0, nullptr, &spvLayout);
            hr = Element::Create(0, this, nullptr, &_peSliderInner);
            if (SUCCEEDED(hr))
            {
                this->Add(&_peSliderInner, 1);
                _peSliderInner->SetValue(Element::LayoutProp, 1, spvLayout);
                _peSliderInner->SetLayoutPos(-1);
                BorderLayout::Create(0, nullptr, &spvLayout);
                hr = Element::Create(0, _peSliderInner, nullptr, &_peTrackHolder);
                if (SUCCEEDED(hr))
                {
                    _peSliderInner->Add(&_peTrackHolder, 1);
                    _peTrackHolder->SetValue(Element::LayoutProp, 1, spvLayout);
                    FillLayout::Create(0, nullptr, &spvLayout);
                    hr = TouchButton::Create(_peTrackHolder, nullptr, (Element**)&_peTrackBase);
                    if (SUCCEEDED(hr))
                    {
                        _peTrackHolder->Add((Element**)&_peTrackBase, 1);
                        _peTrackBase->SetID(L"DDS_TrackBase");
                        _peTrackBase->SetValue(Element::LayoutProp, 1, spvLayout);
                        hr = TouchButton::Create(_peTrackHolder, nullptr, (Element**)&_peFillBase);
                        if (SUCCEEDED(hr))
                        {
                            _peTrackHolder->Add((Element**)&_peFillBase, 1);
                            _peFillBase->SetID(L"DDS_FillBase");
                            _peFillBase->SetValue(Element::LayoutProp, 1, spvLayout);
                            hr = DDScalableElement::Create(_peTrackBase, nullptr, (Element**)&_peTrack);
                            if (SUCCEEDED(hr))
                            {
                                _peTrackBase->Add((Element**)&_peTrack, 1);
                                _peTrack->SetID(L"DDS_Track");
                                hr = DDScalableElement::Create(_peFillBase, nullptr, (Element**)&_peFill);
                                if (SUCCEEDED(hr))
                                {
                                    _peFillBase->Add((Element**)&_peFill, 1);
                                    _peFill->SetID(L"DDS_Fill");
                                    hr = DDScalableTouchButton::Create(_peSliderInner, nullptr, (Element**)&_peThumb);
                                    if (SUCCEEDED(hr))
                                    {
                                        _peSliderInner->Add((Element**)&_peThumb, 1);
                                        _peThumb->SetID(L"DDS_Thumb");
                                        _peThumb->SetValue(Element::LayoutProp, 1, spvLayout);
                                        assignExtendedFn(_peThumb, s_AnimateThumb);
                                        hr = DDScalableElement::Create(_peThumb, nullptr, (Element**)&_peThumbInner);
                                        if (SUCCEEDED(hr))
                                        {
                                            _peThumb->Add((Element**)&_peThumbInner, 1);
                                            _peThumbInner->SetID(L"DDS_ThumbInner");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return hr;
    }

    void DDSlider::_RedrawSlider()
    {
        bool vertical = this->GetIsVertical();
        const WCHAR* szClassName = vertical ? L"DDS_Vert" : L"DDS_Horiz";
        this->SetClass(szClassName);

        BYTE DDSFillLayoutPos = vertical ? 3 : 0;
        if (vertical) _peText->SetHeight(this->GetTextHeight());
        else _peText->SetWidth(this->GetTextWidth());
        _peText->SetLayoutPos(DDSFillLayoutPos);

        DDSFillLayoutPos = vertical ? 1 : 2;
        _peTrackHolder->SetLayoutPos(DDSFillLayoutPos);
        _peTrackBase->SetLayoutPos(DDSFillLayoutPos);

        DDSFillLayoutPos = vertical ? 3 : 0;
        _peFillBase->SetLayoutPos(DDSFillLayoutPos);

        DDSFillLayoutPos = vertical ? 0 : 3;
        _peTrack->SetLayoutPos(DDSFillLayoutPos);
        _peFill->SetLayoutPos(DDSFillLayoutPos);

        float relMaxValue = _maxValue - _minValue;
        float relCurrValue = _currValue - _minValue;
        if (vertical)
        {
            int height = this->GetHeight() - this->GetTextHeight();
            _peTrackHolder->SetHeight(height);
            _peTrackBase->SetHeight(round(height * (1 - (relCurrValue / relMaxValue)) - (0.5f - (relCurrValue / relMaxValue)) * _peThumb->GetHeight()));
            _peFillBase->SetHeight(round(height * (relCurrValue / relMaxValue) + (0.5f - (relCurrValue / relMaxValue)) * _peThumb->GetHeight()));
            _peThumb->SetY(round(_peTrackBase->GetHeight() - _peThumb->GetHeight() / 2.0f));
            _peThumb->SetX(floor((this->GetWidth() - _peThumb->GetWidth()) / 2.0f));
            float padding = (this->GetWidth() - _peTrack->GetWidth()) / 2.0f;
            _peTrackBase->SetPadding(floor(padding), 0, ceil(padding), 0);
            _peFillBase->SetPadding(floor(padding), 0, ceil(padding), 0);
        }
        else
        {
            int width = this->GetWidth() - this->GetTextWidth();
            _peTrackHolder->SetWidth(width);
            _peTrackBase->SetWidth(round(width * (1 - (relCurrValue / relMaxValue)) - (0.5f - (relCurrValue / relMaxValue)) * _peThumb->GetWidth()));
            _peFillBase->SetWidth(round(width * (relCurrValue / relMaxValue) + (0.5f - (relCurrValue / relMaxValue)) * _peThumb->GetWidth()));
            _peThumb->SetX(((localeType == 1) ? round(_peTrackBase->GetWidth()) : round(_peFillBase->GetWidth())) - _peThumb->GetWidth() / 2.0f);
            _peThumb->SetY(floor((this->GetHeight() - _peThumb->GetHeight()) / 2.0f));
            float padding = (this->GetHeight() - _peTrack->GetHeight()) / 2.0f;
            _peTrackBase->SetPadding(0, floor(padding), 0, ceil(padding));
            _peFillBase->SetPadding(0, floor(padding), 0, ceil(padding));
        }
    }

    DDColorPickerButton::~DDColorPickerButton()
    {
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

    void DDColorPickerButton::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::MouseFocusedProp))
        {
            CSafeElementPtr<DDScalableElement> DDCPHC;
            DDCPHC.Assign(regElem<DDScalableElement*>(L"DDColorPicker_HoverCircle", this->GetParent()));
            if (DDCPHC)
            {
                ElementSetValue(DDCPHC, Element::VisibleProp(), pvNew, this);
                DDCPHC->SetX(this->GetX());
            }
        }
        TouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDColorPickerButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDColorPickerButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDColorPickerButton::Register()
    {
        return ClassInfo<DDColorPickerButton, Button>::RegisterGlobal(HINST_THISCOMPONENT, L"DDColorPickerButton", nullptr, 0);
    }

    COLORREF DDColorPickerButton::GetAssociatedColor()
    {
        return _assocCR;
    }

    BYTE DDColorPickerButton::GetOrder()
    {
        return _order;
    }

    void DDColorPickerButton::SetAssociatedColor(COLORREF cr)
    {
        _assocCR = cr;
    }

    void DDColorPickerButton::SetOrder(BYTE bOrder)
    {
        _order = bOrder;
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

    void DDColorPicker::OnInput(InputEvent* pInput)
    {
        if (pInput->nCode == GMOUSE_MOVE)
        {
            GetCursorPos(&_ptBeforeClick);
            ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &_ptBeforeClick);
        }
        if (pInput->nCode == GMOUSE_DOWN && pInput->nDevice != GINPUT_KEYBOARD)
        {
            POINT ptRoot;
            GetCursorPos(&ptRoot);
            ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &ptRoot);
            this->MapElementPoint(this->GetRoot(), &ptRoot, &_ptOnClick);
        }
        if (pInput->nCode == GMOUSE_DOWN || pInput->nCode == GMOUSE_DRAG)
        {
            if ((pInput->uModifiers == 0 && pInput->nStage == GMF_BUBBLED) || pInput->nDevice != GINPUT_KEYBOARD)
            {
                static short localeDirection = (localeType == 1) ? -1 : 1;
                static POINT ppt;
                GetCursorPos(&ppt);
                ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &ppt);
                static bool canMove{};
                static short sLeft, sRight;
                static int width{};
                static short oldColorID{};

                short spacedWidth = this->GetWidth() / 8;
                width = ppt.x - _ptBeforeClick.x + _ptOnClick.x + (spacedWidth - _btnWidth) / 2 * localeDirection;
                sLeft = GetAsyncKeyState(VK_LEFT);
                sRight = GetAsyncKeyState(VK_RIGHT);
                if (sLeft & 1 || sLeft & 0x8000)
                {
                    _currentColorID -= localeDirection;
                    canMove = true;
                }
                else if (sRight & 1 || sRight & 0x8000)
                {
                    _currentColorID += localeDirection;
                    canMove = true;
                }
                else _currentColorID = (localeType == 1) ? (this->GetWidth() - width) / spacedWidth : width / spacedWidth;
                if (_currentColorID < 0) _currentColorID = 0;
                if (_currentColorID > 7) _currentColorID = 7;
                if (pInput->nDevice != GINPUT_KEYBOARD) canMove = true;
                if (_currentColorID != oldColorID && canMove)
                {
                    _peOverlayCheck->SetX((localeType == 1) ? this->GetWidth() - _btnWidth - _currentColorID * spacedWidth : _currentColorID * spacedWidth);
                    _peOverlayHover->SetX(-9999);
                    if (_rkv._hKeyName != nullptr)
                    {
                        _rkv._dwValue = _rgpeColorButtons[_currentColorID]->GetOrder();
                        SetRegistryValues(_rkv._hKeyName, _rkv._path, _rkv._valueToFind, _rkv._dwValue, false, nullptr);
                    }
                    _ColorizeAssociatedItems<DDScalableElement>(_targetElems);
                    _ColorizeAssociatedItems<DDScalableButton>(_targetBtns);
                    _ColorizeAssociatedItems<DDScalableTouchButton>(_targetTouchBtns);
                }
                oldColorID = _currentColorID;
                sLeft = 0, sRight = 0;
                canMove = false;
            }
        }
        Element::OnInput(pInput);
    }

    void DDColorPicker::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::WidthProp()) || PropNotify::IsEqual(ppi, iIndex, Element::HeightProp()) ||
            PropNotify::IsEqual(ppi, iIndex, DDColorPicker::FirstScaledImageProp()))
        {
            int scaleInterval = GetCurrentScaleInterval();
            int scaleIntervalImage = this->GetScaledImageIntervals();
            if (scaleInterval > scaleIntervalImage - 1) scaleInterval = scaleIntervalImage - 1;
            int imageID = this->GetFirstScaledImage() + scaleInterval;
            HBITMAP newImage = (HBITMAP)LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            if (newImage == nullptr)
            {
                LoadPNGAsBitmap(newImage, imageID);
                IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1, NULL);
            }
            BITMAP bm{};
            GetObject(newImage, sizeof(BITMAP), &bm);
            _btnWidth = bm.bmWidth / 8;
            int btnHeight = bm.bmHeight;
            _btnX = this->GetWidth() / 8;
            int btnY = (this->GetHeight() - bm.bmHeight) / 2;

            HDC hdc = GetDC(nullptr);
            HDC hdcSrc = CreateCompatibleDC(hdc);
            HDC hdcDst = CreateCompatibleDC(hdc);
            SelectObject(hdcSrc, newImage);
            for (int i = 0; i < ARRAYSIZE(_rgpeColorButtons); i++)
            {
                int xPos = (localeType == 1) ? this->GetWidth() - i * _btnX - _btnWidth : i * _btnX;
                HBITMAP hbmPickerBtn = CreateCompatibleBitmap(hdc, _btnWidth, btnHeight);
                SelectObject(hdcDst, hbmPickerBtn);
                BitBlt(hdcDst, 0, 0, _btnWidth, btnHeight, hdcSrc, i * _btnWidth, 0, SRCCOPY);
                _rgpeColorButtons[i]->SetX(xPos);
                _rgpeColorButtons[i]->SetY(btnY);
                _rgpeColorButtons[i]->SetWidth(bm.bmWidth / 8);
                _rgpeColorButtons[i]->SetHeight(btnHeight);
                CValuePtr spvPickerBtn = Value::CreateGraphic(hbmPickerBtn, 2, 0xffffffff, true, false, false);
                if (spvPickerBtn) _rgpeColorButtons[i]->SetValue(Element::ContentProp, 1, spvPickerBtn);
                DeleteObject(hbmPickerBtn);
            }
            if (newImage) DeleteObject(newImage);
            DeleteDC(hdcSrc);
            DeleteDC(hdcDst);
            ReleaseDC(nullptr, hdc);

            _peOverlayHover->SetY(btnY);
            _peOverlayHover->SetWidth(bm.bmWidth / 8);
            _peOverlayHover->SetHeight(btnHeight);
            _peOverlayCheck->SetY(btnY);
            _peOverlayCheck->SetWidth(bm.bmWidth / 8);
            _peOverlayCheck->SetHeight(btnHeight);
        }
        Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDColorPicker::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDColorPicker, int>(0x2, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDColorPicker::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((Element*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateCLRVisual();
        return hr;
    }

    HRESULT DDColorPicker::Register()
    {
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impColorIntensityProp,
            &impDefaultColorProp
        };
        return ClassInfo<DDColorPicker, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"DDColorPicker", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    int DDColorPicker::GetPropCommon(const PropertyProcT pPropertyProc)
    {
        if (!this) return -1;
        if (this->IsDestroyed()) return -1;
        Value* pv = GetValue(pPropertyProc, 3, nullptr);
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

    vector<DDScalableButton*> DDColorPicker::GetTargetButtons()
    {
        return _targetBtns;
    }

    vector<DDScalableTouchButton*> DDColorPicker::GetTargetTouchButtons()
    {
        return _targetTouchBtns;
    }

    bool DDColorPicker::GetThemeAwareness()
    {
        return _themeAwareness;
    }

    void DDColorPicker::SetRegKeyValue(RegKeyValue rkvNew)
    {
        _rkv = rkvNew;
        int order = (_rkv._hKeyName) ? GetRegistryValues(_rkv._hKeyName, _rkv._path, _rkv._valueToFind) * _btnX : _rkv._dwValue * _btnX;
        int checkedBtnX = (localeType == 1) ? this->GetWidth() - order - _btnWidth : order;
        _currentColorID = order / (this->GetWidth() / 8);
        _peOverlayCheck->SetX(checkedBtnX);
    }

    void DDColorPicker::SetTargetElements(vector<DDScalableElement*> vte)
    {
        _targetElems = vte;
    }

    void DDColorPicker::SetTargetButtons(vector<DDScalableButton*> vtb)
    {
        _targetBtns = vtb;
    }

    void DDColorPicker::SetTargetTouchButtons(vector<DDScalableTouchButton*> vttb)
    {
        _targetTouchBtns = vttb;
    }

    void DDColorPicker::SetThemeAwareness(bool ta)
    {
        _themeAwareness = ta;
        COLORREF* pImmersiveColor = this->GetThemeAwareness() ? g_theme ? &ImmersiveColorL : &ImmersiveColorD : &ImmersiveColor;
        COLORREF colorPickerPalette[8] =
        {
            this->GetDefaultColor(),
            *pImmersiveColor,
            _themeAwareness ? g_theme ? RGB(76, 194, 255) : RGB(0, 103, 192) : RGB(0, 120, 215),
            _themeAwareness ? g_theme ? RGB(216, 141, 225) : RGB(158, 58, 176) : RGB(177, 70, 194),
            _themeAwareness ? g_theme ? RGB(244, 103, 98) : RGB(210, 14, 30) : RGB(232, 17, 35),
            _themeAwareness ? g_theme ? RGB(251, 154, 68) : RGB(224, 83, 7) : RGB(247, 99, 12),
            _themeAwareness ? g_theme ? RGB(255, 213, 42) : RGB(225, 157, 0) : RGB(255, 185, 0),
            _themeAwareness ? g_theme ? RGB(38, 255, 142) : RGB(0, 178, 90) : RGB(0, 204, 106)
        };
        for (int i = 0; i < ARRAYSIZE(_rgpeColorButtons); i++)
            _rgpeColorButtons[i]->SetAssociatedColor(colorPickerPalette[i]);
    }

    HRESULT DDColorPicker::_CreateCLRVisual()
    {
        HRESULT hr = S_OK;
        for (int i = 0; i < ARRAYSIZE(_rgpeColorButtons); i++)
        {
            hr = DDColorPickerButton::Create(this, nullptr, (Element**)&_rgpeColorButtons[i]);
            if (SUCCEEDED(hr))
            {
                this->Add((Element**)&_rgpeColorButtons[i], 1);
                _rgpeColorButtons[i]->SetLayoutPos(-2);
                _rgpeColorButtons[i]->SetOrder(i);
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = DDScalableElement::Create(this, nullptr, (Element**)&_peOverlayHover);
            if (SUCCEEDED(hr))
            {
                this->Add((Element**)&_peOverlayHover, 1);
                _peOverlayHover->SetLayoutPos(-2);
                _peOverlayHover->SetX(-9999);
                _peOverlayHover->SetID(L"DDColorPicker_HoverCircle");
                hr = DDScalableElement::Create(this, nullptr, (Element**)&_peOverlayCheck);
                if (SUCCEEDED(hr))
                {
                    this->Add((Element**)&_peOverlayCheck, 1);
                    _peOverlayCheck->SetLayoutPos(-2);
                    _peOverlayCheck->SetID(L"DDColorPicker_CheckedCircle");
                }
            }
        }
        return hr;
    }

    template <typename T>
    void DDColorPicker::_ColorizeAssociatedItems(vector<T*> vElem)
    {
        for (int i = 0; i < vElem.size(); i++)
        {
            if (vElem[i])
            {
                vElem[i]->SetDDCPIntensity(this->GetColorIntensity());
                if (_currentColorID == 0)
                    vElem[i]->SetDDCPIntensity(255);
                if (_themeAwareness)
                    vElem[i]->SetGroupColor(_currentColorID);
                vElem[i]->SetAssociatedColor(_rgpeColorButtons[_currentColorID]->GetAssociatedColor());
            }
        }
    }

    DDTabbedPages::~DDTabbedPages()
    {
        this->DestroyAll(true);
    }

    IClassInfo* DDTabbedPages::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDTabbedPages::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDTabbedPages::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void DDTabbedPages::OnInput(InputEvent* pInput)
    {
        if (pInput->nCode == GMOUSE_DOWN && pInput->nDevice == GINPUT_KEYBOARD)
        {
            CValuePtr v;
            if (pInput->peTarget->GetClass(&v))
            {
                if (wcscmp(_peTabs[_pageID]->GetClass(&v), pInput->peTarget->GetClass(&v)) == 0)
                {
                    static GTRANS_DESC transDesc[2];
                    static TransitionStoryboardInfo tsbInfo = {};
                    static short sLeft, sRight;
                    static short localeDirection = (localeType == 1) ? -1 : 1;
                    sLeft = GetAsyncKeyState(VK_LEFT);
                    sRight = GetAsyncKeyState(VK_RIGHT);
                    if ((sLeft & 1 || sLeft & 0x8000))
                    {
                        if ((_pageID > 0 && localeDirection == 1) || (_pageID < _pageSize - 1 && localeDirection == -1))
                            this->TraversePage(_pageID - localeDirection);
                        else
                        {
                            CSafeElementPtr<Element> peEdge;
                            LPCWSTR peResID = (localeType == 1) ? _pszPageIDs[_pageSize - 1] : _pszPageIDs[0];
                            peEdge.Assign(regElem<Element*>(peResID, _peSubUIContainer));
                            TriggerTranslate(peEdge, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 0.0f, 0.0f, 40.0f * g_flScaleFactor, 0.0f, false, false);
                            TriggerTranslate(peEdge, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 40.0f * g_flScaleFactor, 0.0f, 0.0f, 0.0f, false, false);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peEdge->GetDisplayNode(), &tsbInfo);
                        }
                    }
                    if ((sRight & 1 || sRight & 0x8000))
                    {
                        if ((_pageID < _pageSize - 1 && localeDirection == 1) || (_pageID > 0 && localeDirection == -1))
                            this->TraversePage(_pageID + localeDirection);
                        else
                        {
                            CSafeElementPtr<Element> peEdge;
                            LPCWSTR peResID = (localeType == 1) ? _pszPageIDs[0] : _pszPageIDs[_pageSize - 1];
                            peEdge.Assign(regElem<Element*>(peResID, _peSubUIContainer));
                            TriggerTranslate(peEdge, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 0.0f, 0.0f, -40.0f * g_flScaleFactor, 0.0f, false, false);
                            TriggerTranslate(peEdge, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, -40.0f * g_flScaleFactor, 0.0f, 0.0f, 0.0f, false, false);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peEdge->GetDisplayNode(), &tsbInfo);
                        }
                    }
                    sLeft = 0, sRight = 0;
                }
            }
        }
        Element::OnInput(pInput);
    }

    bool DDTabbedPages::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = Element::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::MinSizeProp))
        {
            result = false;
            ElementSetValue(_peSubUIContainer, ppi, pvNew, this);
            SIZE size = *pvNew->GetSize();
            RECT rcList{};
            _tsvTabCtrl->GetVisibleRect(&rcList);
            _tsvTabCtrl->SetXScrollable(size.cx > rcList.right - rcList.left);
            _tsvPage->SetXScrollable(size.cx > rcList.right - rcList.left);
        }
        return result;
    }

    HRESULT DDTabbedPages::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDTabbedPages, int>(0, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDTabbedPages::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((Element*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateTPVisual();
        return hr;
    }

    HRESULT DDTabbedPages::Register()
    {
        return ClassInfo<DDTabbedPages, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"DDTabbedPages", nullptr, 0);
    }

    void DDTabbedPages::SetKeyFocus()
    {
        if (_peTabs[_pageID - 1]) _peTabs[_pageID - 1]->SetKeyFocus();
    }
    
    void DDTabbedPages::BindParser(DUIXmlParser* pParser)
    {
        _pParser = pParser;
    }

    void DDTabbedPages::InsertTab(BYTE index, LPCWSTR pszResIDPage, LPCWSTR pszTabLabel, GenericTabFunction ptfn)
    {
        if (index == MAX_TABPAGES) index = _pageSize;
        if (_pageSize >= MAX_TABPAGES || index < 0 || index > _pageSize)
            return;
        for (int i = _pageSize - 1; i >= index; i--)
        {
            _pszPageIDs[i + 1] = _pszPageIDs[i];
            _pfnTabs[i + 1] = _pfnTabs[i];
            _peTabs[i + 1] = _peTabs[i];
            _peTabs[i + 1]->SetNumberID(i + 1);
        }
        _pszPageIDs[index] = pszResIDPage;
        _pfnTabs[index] = ptfn;
        DDNumberedButton::Create(_peTabCtrl, nullptr, (Element**)&(_peTabs[index]));
        _peTabCtrl->Insert((Element**)&_peTabs[index], 1, index);
        _peTabs[index]->SetNumberID(index);
        _peTabs[index]->SetLinkedElement(this);
        _peTabs[index]->SetContentString(pszTabLabel);
        _peTabs[index]->SetClass(L"tab");
        _pageSize++;
    }

    void DDTabbedPages::EraseTab(BYTE index)
    {
        if (index < 0 || index >= _pageSize)
            return;
        _peTabs[index]->Destroy(true);
        _peTabs[index] = nullptr;
        for (int i = index; i < _pageSize - 1; i++)
        {
            _pszPageIDs[i] = _pszPageIDs[i + 1];
            _pfnTabs[i] = _pfnTabs[i + 1];
            _peTabs[i] = _peTabs[i + 1];
            _peTabs[i]->SetNumberID(i);
        }
        _pageSize--;
    }

    void DDTabbedPages::TraversePage(BYTE index)
    {
        if (index >= MAX_TABPAGES) return;
        GTRANS_DESC transDesc[2];
        TransitionStoryboardInfo tsbInfo = {};
        CValuePtr v;
        DynamicArray<Element*>* pel;
        pel = _peSubUIContainer->GetChildren(&v);
        RECT rcList;
        _tsvPage->GetVisibleRect(&rcList);
        Element* peSettingsPage;
        for (DDNumberedButton* ddt : _peTabs)
            if (ddt) ddt->SetSelected(false);
        _peTabs[index]->SetSelected(true);
        if (!pel)
        {
            _pParser->CreateElement(_pszPageIDs[index], nullptr, nullptr, nullptr, &peSettingsPage);
            _peSubUIContainer->Add(&peSettingsPage, 1);
            _pfnTabs[index](peSettingsPage);
            GTRANS_DESC transDesc2[3];
            TriggerTranslate(_peSubUIContainer, transDesc2, 0, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false);
            TriggerFade(_peSubUIContainer, transDesc2, 1, 0.2f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerClip(_peSubUIContainer, transDesc2, 2, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 0.0f, 1.0f, (rcList.bottom - rcList.top - 100 * g_flScaleFactor) / (rcList.bottom - rcList.top), 0.0f, 0.0f, 1.0f, 1.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc2, _peSubUIContainer->GetDisplayNode(), &tsbInfo);
        }
        else if (index != _pageID)
        {
            for (int id = 0; id < pel->GetSize(); id++)
            {
                bool fAnimate = true;
                for (int id2 = 0; id2 < _vecAnimating.size(); id2++)
                {
                    if (pel->GetItem(id) == _vecAnimating[id2])
                    {
                        fAnimate = false;
                        break;
                    }
                }
                if (fAnimate)
                {
                    if ((localeType == 0 && index < _pageID) || (localeType == 1 && index > _pageID))
                    {
                        TriggerTranslate(pel->GetItem(id), transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, rcList.right - rcList.left, 0.0f, false, false);
                        TriggerClip(pel->GetItem(id), transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, false, true);
                    }
                    else if ((localeType == 0 && index > _pageID) || (localeType == 1 && index < _pageID))
                    {
                        TriggerTranslate(pel->GetItem(id), transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, (rcList.right - rcList.left) * -1, 0.0f, false, false);
                        TriggerClip(pel->GetItem(id), transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, false, true);
                    }
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pel->GetItem(id)->GetDisplayNode(), &tsbInfo);
                    _vecAnimating.push_back(pel->GetItem(id));
                    DWORD animCoef = g_animCoef;
                    if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                    yValuePtrs* yV = new yValuePtrs{ &_vecAnimating, pel->GetItem(id), static_cast<DWORD>(3.3f * animCoef) };
                    HANDLE hRemoveFromVec = CreateThread(nullptr, 0, s_RemoveFromVec, yV, NULL, nullptr);
                    if (hRemoveFromVec) CloseHandle(hRemoveFromVec);
                }
            }
            _tsvPage->SetYOffset(0);
            _pParser->CreateElement(_pszPageIDs[index], nullptr, nullptr, nullptr, &peSettingsPage);
            _peSubUIContainer->Add(&peSettingsPage, 1);
            if (peSettingsPage)
            {
                if ((localeType == 0 && index < _pageID) || (localeType == 1 && index > _pageID))
                {
                    TriggerTranslate(peSettingsPage, transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, (rcList.right - rcList.left) * -1, 0.0f, 0.0f, 0.0f, false, false);
                    TriggerClip(peSettingsPage, transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peSettingsPage->GetDisplayNode(), &tsbInfo);
                }
                else if ((localeType == 0 && index > _pageID) || (localeType == 1 && index < _pageID))
                {
                    TriggerTranslate(peSettingsPage, transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, rcList.right - rcList.left, 0.0f, 0.0f, 0.0f, false, false);
                    TriggerClip(peSettingsPage, transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peSettingsPage->GetDisplayNode(), &tsbInfo);
                }
                _pfnTabs[index](peSettingsPage);
            }
        }
        _pageID = index;
    }

    HRESULT DDTabbedPages::_CreateTPVisual()
    {
        HRESULT hr = S_OK;
        CValuePtr spvLayout;
        BorderLayout::Create(0, nullptr, &spvLayout);
        this->SetValue(Element::LayoutProp, 1, spvLayout);
        hr = TouchScrollViewer::Create(this, nullptr, (Element**)&_tsvTabCtrl);
        if (SUCCEEDED(hr))
        {
            this->Add((Element**)&_tsvTabCtrl, 1);
            _tsvTabCtrl->SetLayoutPos(1);
            _tsvTabCtrl->SetActive(0xB);
            _tsvTabCtrl->SetXBarVisibility(0);
            _tsvTabCtrl->SetYBarVisibility(0);
            _tsvTabCtrl->SetXScrollable(false);
            _tsvTabCtrl->SetYScrollable(false);
            _tsvTabCtrl->SetInteractionMode(16);
            hr = Element::Create(0, _tsvTabCtrl, nullptr, &_peTabCtrl);
            if (SUCCEEDED(hr))
            {
                _tsvTabCtrl->Add(&_peTabCtrl, 1);
                int flowLayoutParams[4] = { 0, 0, 0, 0 };
                FlowLayout::Create(ARRAYSIZE(flowLayoutParams), flowLayoutParams, &spvLayout);
                _peTabCtrl->SetValue(Element::LayoutProp, 1, spvLayout);
                _peTabCtrl->SetID(L"DDTP_TabControl");
                hr = TouchScrollViewer::Create(this, nullptr, (Element**)&_tsvPage);
                if (SUCCEEDED(hr))
                {
                    this->Add((Element**)&_tsvPage, 1);
                    _tsvPage->SetLayoutPos(1);
                    _tsvPage->SetActive(0xB);
                    _tsvPage->SetXBarVisibility(0);
                    _tsvPage->SetYBarVisibility(0);
                    _tsvPage->SetXScrollable(false);
                    _tsvPage->SetInteractionMode(18);
                    hr = Element::Create(0, _tsvPage, nullptr, (Element**)&_peSubUIContainer);
                    if (SUCCEEDED(hr))
                    {
                        _tsvPage->Add((Element**)&_peSubUIContainer, 1);
                        FillLayout::Create(0, nullptr, &spvLayout);
                        _peSubUIContainer->SetValue(Element::LayoutProp, 1, spvLayout);
                        _peSubUIContainer->SetLayoutPos(-1);
                    }
                }
            }
        }
        return hr;
    }

    DWORD WINAPI DDTabbedPages::s_RemoveFromVec(LPVOID lpParam)
    {
        yValuePtrs* yV = (yValuePtrs*)lpParam;
        Sleep(yV->dwMillis);
        vector<void*>* vec = ((vector<void*>*)yV->ptr1);
        auto toRemove = find(vec->begin(), vec->end(), yV->ptr2);
        vec->erase(toRemove);
        return 0;
    }

    LRESULT CALLBACK NotificationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        NotificationData* nd = (NotificationData*)wParam;
        switch (uMsg)
        {
            case WM_CLOSE:
                return 0;
            case WM_DESTROY:
                return 0;
            case WM_USER + 3:
                if (nd->nb)
                    nd->nb->DestroyBanner(nullptr, false);
                delete nd;
                break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI AnimateWindowWrapper(LPVOID lpParam)
    {
        NotificationData* nd = (NotificationData*)lpParam;
        Sleep(10);
        if (nd->nb->GetWindowHost())
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            AnimateWindow(nd->nb->GetWindowHost()->GetHWND(), 150 * (animCoef / 100.0f), AW_BLEND);
            nd->nb->GetWindowHost()->ShowWindow(SW_SHOWNOACTIVATE);
        }
        delete nd;
        return 0;
    }

    DWORD WINAPI AutoCloseNotification(LPVOID lpParam)
    {
        NotificationData* ndTemp = (NotificationData*)lpParam;
        HWND hWnd = ndTemp->nb->GetWindowHost()->GetHWND();
        Sleep(ndTemp->val * 1000);
        if (ndTemp->nb)
        {
            if (!ndTemp->nb->IsDestroyed())
                SendMessageW(hWnd, WM_USER + 3, (WPARAM)ndTemp, NULL);
            else delete ndTemp;
        }
        else delete ndTemp;
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
        return count(textStr.begin(), textStr.end(), L'\n') + 1;
    }

    void GetLongestLine(HDC hdc, const wstring& textStr, SIZE* szText)
    {
        vector<wstring> divided = SplitLineBreaks(textStr);
        int saved{};
        for (int i = 0; i < divided.size(); i++)
        {
            GetTextExtentPoint32W(hdc, divided[i].c_str(), lstrlenW(divided[i].c_str()), szText);
            if (szText->cx > saved) saved = szText->cx;
        }
        szText->cx = saved;
    }

    void GetTextDimensions(Element* pe, const wstring& str, SIZE* psz, int& cy)
    {
        CValuePtr v;
        HDC hdcMem = CreateCompatibleDC(nullptr);
        int fontsize = pe->GetFontSize();
        if (fontsize < 0) fontsize *= -1.5;
        LOGFONTW lf = { fontsize, 0, 0, 0, pe->GetFontWeight(), pe->GetFontStyle() & 1, pe->GetFontStyle() & 2, pe->GetFontStyle() & 4,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, NULL };
        wcscpy_s(lf.lfFaceName, pe->GetFontFace(&v));
        HFONT hFont = CreateFontIndirectW(&lf);
        HGDIOBJ oldFont = SelectObject(hdcMem, hFont);
        GetLongestLine(hdcMem, str, psz);
        cy += (lf.lfHeight * CalcLines(str));
        SelectObject(hdcMem, oldFont);
        DeleteObject(hFont);
        DeleteDC(hdcMem);
    }

    DDNotificationBanner::~DDNotificationBanner()
    {
        DestroyAll(true);
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
        return CreateAndInit<DDNotificationBanner, HWND, bool, UINT>(hParent, fDblBuffer, nCreate, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDNotificationBanner::Register()
    {
        return ClassInfo<DDNotificationBanner, HWNDElement, EmptyCreator<DDNotificationBanner>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDNotificationBanner", nullptr, 0);
    }

    NativeHWNDHost* DDNotificationBanner::GetWindowHost()
    {
        return _wnd;
    }

    void DDNotificationBanner::CreateBanner(DDNotificationType type, LPCWSTR title, LPCWSTR content, short timeout)
    {
        static bool notificationopen{};
        static HANDLE AutoCloseHandle;
        //if (notificationopen) DestroyBanner(&notificationopen);
        unsigned long keyN{};
        Element* pHostElement;
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        DDNotificationBanner* pDDNB = this;
        DDNotificationBanner** ppDDNB = &pDDNB;
        NativeHWNDHost::Create(L"DD_NotificationHost", L"DirectDesktop In-App Notification", nullptr, nullptr, 0, 0, 0, 0, WS_EX_TOOLWINDOW, WS_POPUP | WS_BORDER, HINST_THISCOMPONENT, 0, &_wnd);
        HWNDElement::Create(_wnd->GetHWND(), true, NULL, nullptr, &keyN, (Element**)ppDDNB);
        _pDDNB = pDDNB;
        Element::Create(0, pDDNB, nullptr, &pHostElement);
        _pDDNB->Add(&pHostElement, 1);

        LPWSTR sheetName = g_theme ? (LPWSTR)L"default" : (LPWSTR)L"defaultdark";
        StyleSheet* sheet = pHostElement->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parser->GetSheet(sheetName, &sheetStorage);
        pHostElement->SetValue(Element::SheetProp, 1, sheetStorage);
        free(sheet);

        pHostElement->SetID(L"DDNB_Host");
        SetWindowSubclass(_wnd->GetHWND(), NotificationProc, 1, (DWORD_PTR)this);
        _pDDNB->SetVisible(true);
        _pDDNB->EndDefer(keyN);
        _wnd->Host(_pDDNB);
        WCHAR* WindowsBuildStr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
        int WindowsBuild = _wtoi(WindowsBuildStr);
        free(WindowsBuildStr);
        int WindowsRev = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\BuildLayers\\ShellCommon", L"BuildQfe");
        if (DWMActive)
        {
            MARGINS margins = { -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(_wnd->GetHWND(), &margins);
            if (WindowsBuild > 22000 || WindowsBuild == 22000 && WindowsRev >= 51)
            {
                DWORD cornerPreference = DWMWCP_ROUND;
                DwmSetWindowAttribute(_wnd->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
            }
        }
        BlurBackground(_wnd->GetHWND(), true, false, -1, nullptr);
        _pDDNB->SetBackgroundStdColor(7);
        CValuePtr v;
        CreateAndSetLayout(_pDDNB, BorderLayout::Create, 0, nullptr);
        CreateAndSetLayout(pHostElement, BorderLayout::Create, 0, nullptr);

        DDScalableElement::Create(pHostElement, nullptr, (Element**)&_icon);
        _icon->SetID(L"DDNB_Icon");
        pHostElement->Add((Element**)&_icon, 1);
        switch (type)
        {
            case DDNT_SUCCESS:
                if (!title) StringCchPrintfW(_titleStr, 64, L"%s", LoadStrFromRes(217).c_str());
                _icon->SetClass(L"DDNB_Icon_Success");
                break;
            case DDNT_INFO:
                if (!title) StringCchPrintfW(_titleStr, 64, L"%s", LoadStrFromRes(218).c_str());
                _icon->SetClass(L"DDNB_Icon_Info");
                break;
            case DDNT_WARNING:
                if (!title) StringCchPrintfW(_titleStr, 64, L"%s", LoadStrFromRes(219).c_str());
                _icon->SetClass(L"DDNB_Icon_Warning");
                break;
            case DDNT_ERROR:
                if (!title) StringCchPrintfW(_titleStr, 64, L"%s", LoadStrFromRes(220).c_str());
                _icon->SetClass(L"DDNB_Icon_Error");
                break;
        }
        if (title) StringCchPrintfW(_titleStr, 64, L"%s", title);

        DDScalableElement::Create(pHostElement, nullptr, (Element**)&_title);
        _title->SetID(L"DDNB_Title");
        pHostElement->Add((Element**)&_title, 1);
        _title->SetContentString(_titleStr);

        if (content)
        {
            DDScalableElement::Create(pHostElement, nullptr, (Element**)&_content);
            _content->SetID(L"DDNB_Content");
            pHostElement->Add((Element**)&_content, 1);
            _content->SetContentString(content);
        }

        int cx{}, cy{};
        RECT hostpadding = *(pHostElement->GetPadding(&v));
        RECT titlepadding = *(_title->GetPadding(&v));
        cx += (hostpadding.left + hostpadding.right + _icon->GetWidth() + 2 * g_flScaleFactor);
        cy += (hostpadding.top + hostpadding.bottom);

        SIZE szText{}, szText2{};
        GetTextDimensions(_title, _titleStr, &szText, cy);
        if (content)
            GetTextDimensions(_content, content, &szText2, cy);

        cx += (max(szText.cx, szText2.cx));
        cy += (titlepadding.top + titlepadding.bottom);
        if (content)
        {
            RECT contentpadding = *(_content->GetPadding(&v));
            cy += (contentpadding.top + contentpadding.bottom);
        }

        // Window borders
        cx += (round(g_flScaleFactor)) * 2;

        if (_wnd)
        {
            SetWindowPos(_wnd->GetHWND(), HWND_TOPMOST, (dimensions.left + dimensions.right - cx) / 2, dimensions.top + 40 * g_flScaleFactor, cx, cy, SWP_FRAMECHANGED | SWP_NOACTIVATE);
            notificationopen = true;
            NotificationData* nd = new NotificationData{ this, nullptr, timeout };
            HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateWindowWrapper, nd, NULL, nullptr);
            if (AnimHandle) CloseHandle(AnimHandle);
            g_nwnds.push_back(_wnd->GetHWND());
            DDNotificationBanner::s_RepositionBanners();
            if (timeout > 0)
            {
                NotificationData* nd = new NotificationData{ this, nullptr, timeout };
                DWORD dwAutoClose;
                AutoCloseHandle = CreateThread(nullptr, 0, AutoCloseNotification, nd, NULL, &dwAutoClose);
            }
            _pDDNB->_wnd = (NativeHWNDHost*)this; // Unsafe hack
        }
    }

    void DDNotificationBanner::s_RepositionBanners()
    {
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        int offset{};
        offset += dimensions.top + 40 * g_flScaleFactor;
        for (int i = g_nwnds.size() - 1; i >= 0; i--) {
            RECT windowRect{};
            GetClientRect(g_nwnds[i], &windowRect);
            SetWindowPos(g_nwnds[i], HWND_TOPMOST, (dimensions.left + dimensions.right - windowRect.right - 2 * g_flScaleFactor) / 2, offset, NULL, NULL, SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
            offset += windowRect.bottom + 18 * g_flScaleFactor;
        }
    }

    void DDNotificationBanner::DestroyBanner(bool* notificationopen, bool manual)
    {
        if (_wnd != nullptr)
        {
            auto toRemove = find(g_nwnds.begin(), g_nwnds.end(), _wnd->GetHWND());
            g_nwnds.erase(toRemove);
            DDNotificationBanner::s_RepositionBanners();
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            AnimateWindow(_wnd->GetHWND(), 120 * (animCoef / 100.0f), AW_BLEND | AW_HIDE);
            _wnd->GetElement()->DestroyAll(true);
            _wnd->GetElement()->Destroy(true);
            if (manual) SetWindowLongPtrW(_wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)nullptr);
            _wnd->DestroyWindow();
            _wnd = nullptr;
        }
        if (notificationopen != nullptr) *notificationopen = false;
        delete this;
    }

    void DDNotificationBanner::s_DestroyBannerByButton(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            DDNotificationBanner* pDestroy = (DDNotificationBanner*)((DDNotificationBanner*)elem->GetParent()->GetParent())->_wnd;
            pDestroy->DestroyBanner(nullptr, true);
        }
    }

    void DDNotificationBanner::AppendButton(LPCWSTR szButtonText, void(*pListener)(Element* elem, Event* iev), bool fClose)
    {
        if (_btnCount == 0)
        {
            Element::Create(0, _pDDNB, nullptr, &_peButtonSection);
            _pDDNB->Add(&_peButtonSection, 1);

            LPWSTR sheetName = g_theme ? (LPWSTR)L"default" : (LPWSTR)L"defaultdark";
            StyleSheet* sheet = _peButtonSection->GetSheet();
            CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
            parser->GetSheet(sheetName, &sheetStorage);
            _peButtonSection->SetValue(Element::SheetProp, 1, sheetStorage);
            free(sheet);

            _peButtonSection->SetID(L"DDNB_Buttons");
            int cy{};
            RECT windowRect{};
            GetClientRect(_wnd->GetHWND(), &windowRect);
            cy = (_peButtonSection->GetHeight() + windowRect.bottom);
            SetWindowPos(_wnd->GetHWND(), NULL, NULL, NULL, windowRect.right, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
            DDNotificationBanner::s_RepositionBanners();
        }
        _btnCount++;
        int flowlayoutParams[2] = { 1, _btnCount };
        CreateAndSetLayout(_peButtonSection, GridLayout::Create, ARRAYSIZE(flowlayoutParams), flowlayoutParams);
        DDScalableButton* pBtn{};
        DDScalableButton::Create(_peButtonSection, nullptr, (Element**)&pBtn);
        pBtn->SetNeedsFontResize(false);
        pBtn->SetClass(L"pushbuttonsecondary");
        pBtn->SetHeight(32 * g_flScaleFactor);
        pBtn->SetMargin(8 * g_flScaleFactor, 0, 0, 0);
        pBtn->SetContentString(szButtonText);
        _peButtonSection->Add((Element**)&pBtn, 1);
        if (pListener) assignFn(pBtn, pListener);
        if (fClose) assignFn(pBtn, DDNotificationBanner::s_DestroyBannerByButton);
    }
}
