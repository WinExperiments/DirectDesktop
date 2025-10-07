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
    IClassInfo* DDScalableTouchButton::s_pClassInfo;
    IClassInfo* DDScalableTouchEdit::s_pClassInfo;
    IClassInfo* LVItem::s_pClassInfo;
    IClassInfo* DDLVActionButton::s_pClassInfo;
    IClassInfo* DDToggleButton::s_pClassInfo;
    IClassInfo* DDCheckBox::s_pClassInfo;
    IClassInfo* DDCheckBoxGlyph::s_pClassInfo;
    IClassInfo* DDSlider::s_pClassInfo;
    IClassInfo* DDColorPicker::s_pClassInfo;
    IClassInfo* DDColorPickerButton::s_pClassInfo;
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
    static const int vvimpStopListeningProp[] = { 2, -1 };
    static PropertyInfoData dataimpStopListeningProp;
    static const PropertyInfo impStopListeningProp =
    {
        L"StopListening",
        0x2 | 0x4,
        0x1,
        vvimpStopListeningProp,
        nullptr,
        Value::GetBoolFalse,
        &dataimpStopListeningProp
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

    void RedrawImageCore(DDScalableElement* pe)
    {
        if (!pe) return;
        if (pe->IsDestroyed()) return;
        if (pe->GetFirstScaledImage() == -1)
        {
            pe->SetBackgroundColor(0);
            return;
        }

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

    void RedrawFontCore(DDScalableElement* pe)
    {
        if (!pe) return;
        if (pe->IsDestroyed()) return;
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
            if (elem)
            {
                if (!((DDScalableElement*)elem)->GetStopListening()) ((DDScalableElement*)elem)->InitDrawImage();
            }
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

    DWORD WINAPI DragThumb(LPVOID lpParam)
    {
        yValuePtrs* yV = (yValuePtrs*)lpParam;
        CSafeElementPtr<DDScalableButton> peThumb;
        peThumb.Assign(regElem<DDScalableButton*>(L"DDS_Thumb", (Element*)yV->ptr1));
        while (true)
        {
            if (!peThumb->GetCaptured()) break;
            SendMessageW(g_msgwnd, WM_USER + 7, (WPARAM)yV->ptr1, NULL);
            Sleep(10);
        }
        return 0;
    }
    void SetThumbPosOnClick(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDSlider> slider;
            slider.Assign((DDSlider*)elem->GetParent()->GetParent()->GetParent());
            bool vertical = slider->GetIsVertical();
            CSafeElementPtr<Button> peTrack;
            peTrack.Assign(regElem<Button*>(L"DDS_TrackBase", slider));
            CSafeElementPtr<Button> peFill;
            peFill.Assign(regElem<Button*>(L"DDS_FillBase", slider));
            CSafeElementPtr<DDScalableButton> peThumb;
            peThumb.Assign(regElem<DDScalableButton*>(L"DDS_Thumb", slider));
            CSafeElementPtr<DDScalableRichText> peText;
            peText.Assign(regElem<DDScalableRichText*>(L"DDS_Text", slider));
            if (vertical)
            {
                int y = peFill->GetHeight();
                if (elem == peFill)
                {
                    y -= elem->GetParent()->GetHeight() / 5;
                    if (y < (peThumb->GetHeight() / 2)) y = peThumb->GetHeight() / 2;
                }
                else if (elem == peTrack)
                {
                    y += elem->GetParent()->GetHeight() / 5;
                    if (y > elem->GetParent()->GetHeight() - (peThumb->GetHeight() / 2)) y = elem->GetParent()->GetHeight() - (peThumb->GetHeight() / 2);
                }
                peTrack->SetHeight(elem->GetParent()->GetHeight() - y);
                peFill->SetHeight(y);
                peThumb->SetY(elem->GetParent()->GetHeight() - y - peThumb->GetHeight() / 2);
            }
            else
            {
                int x = peFill->GetWidth();
                if (elem == peFill)
                {
                    x -= elem->GetParent()->GetWidth() / 5;
                    if (x < (peThumb->GetWidth() / 2)) x = peThumb->GetWidth() / 2;
                }
                else if (elem == peTrack)
                {
                    x += elem->GetParent()->GetWidth() / 5;
                    if (x > elem->GetParent()->GetWidth() - (peThumb->GetWidth() / 2)) x = elem->GetParent()->GetWidth() - (peThumb->GetWidth() / 2);
                }
                peTrack->SetWidth(elem->GetParent()->GetWidth() - x);
                peFill->SetWidth(x);
                peThumb->SetX((localeType == 1) ? elem->GetParent()->GetWidth() - x - peThumb->GetWidth() / 2 : x - peThumb->GetWidth() / 2);
            }
            slider->SetCurrentValue(NULL, true);
            WCHAR formattedNum[8];
            StringCchPrintfW(formattedNum, 8, slider->GetFormattedString(), slider->GetCurrentValue());
            peText->SetContentString(formattedNum);
        }
    }
    void SetThumbPosOnDrag(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        bool vertical = ((DDSlider*)elem->GetParent()->GetParent())->GetIsVertical();
        CSafeElementPtr<Button> peTrack;
        peTrack.Assign(regElem<Button*>(L"DDS_TrackBase", elem->GetParent()));
        CSafeElementPtr<Button> peFill;
        peFill.Assign(regElem<Button*>(L"DDS_FillBase", elem->GetParent()));
        CSafeElementPtr<DDScalableButton> peThumbInner;
        peThumbInner.Assign(regElem<DDScalableButton*>(L"DDS_ThumbInner", elem));
        if (pProp == Element::MouseWithinProp())
        {
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            float scaleRelease = (elem->GetMouseWithin()) ? 1.33f : 1.0f;
            TriggerScaleOut(peThumbInner, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, scaleRelease, scaleRelease, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peThumbInner->GetDisplayNode(), &tsbInfo);
        }
        if (pProp == Button::CapturedProp())
        {
            GTRANS_DESC transDesc[2];
            TransitionStoryboardInfo tsbInfo = {};
            float alpha1 = (((Button*)elem)->GetCaptured()) ? 1.0f : 0.8f;
            float alpha2 = (((Button*)elem)->GetCaptured()) ? 0.8f : 1.0f;
            float scaleRelease = (elem->GetMouseWithin()) ? 1.33f : 1.0f;
            if (((Button*)elem)->GetCaptured())
                TriggerScaleOut(peThumbInner, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, 0.83f, 0.83f, 0.5f, 0.5f, false, false);
            else
            {
                ((DDSlider*)elem->GetParent()->GetParent())->SetCurrentValue(NULL, true);
                TriggerScaleOut(peThumbInner, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, scaleRelease, scaleRelease, 0.5f, 0.5f, false, false);
            }
            TriggerFade(peThumbInner, transDesc, 1, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, alpha1, alpha2, false, false, ((Button*)elem)->GetCaptured());
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peThumbInner->GetDisplayNode(), &tsbInfo);
            POINT ppt;
            GetCursorPos(&ppt);
            if (vertical)
            {
                ((DDSlider*)elem->GetParent()->GetParent())->SetDragStart(ppt.y);
                ((DDSlider*)elem->GetParent()->GetParent())->SetFillOnDragStart(peTrack->GetHeight());
                ((DDSlider*)elem->GetParent()->GetParent())->SetPosOnDragStart(elem->GetY());
            }
            else
            {
                ((DDSlider*)elem->GetParent()->GetParent())->SetDragStart(ppt.x);
                ((DDSlider*)elem->GetParent()->GetParent())->SetFillOnDragStart((localeType == 1) ? peTrack->GetWidth() : peFill->GetWidth());
                ((DDSlider*)elem->GetParent()->GetParent())->SetPosOnDragStart(elem->GetX());
            }
            yValuePtrs* yV = new yValuePtrs{ (DDSlider*)elem->GetParent()->GetParent() };
            HANDLE hDragThumb = CreateThread(nullptr, 0, DragThumb, yV, NULL, nullptr);
            if (hDragThumb) CloseHandle(hDragThumb);
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
            vector<DDScalableButton*> tb = ((DDColorPickerButton*)elem)->GetTargetButtons();
            RegKeyValue rkv = ((DDColorPicker*)elem->GetParent())->GetRegKeyValue();
            if (rkv._hKeyName != nullptr)
            {
                rkv._dwValue = ((DDColorPickerButton*)elem)->GetOrder();
                SetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind, rkv._dwValue, false, nullptr);
            }
            bool awareness = ((DDColorPicker*)elem->GetParent())->GetThemeAwareness();
            BYTE order = ((DDColorPickerButton*)elem)->GetOrder();
            for (int i = 0; i < te.size(); i++)
            {
                if (te[i])
                {
                    te[i]->SetDDCPIntensity(((DDColorPicker*)elem->GetParent())->GetColorIntensity());
                    if (order == 0)
                        te[i]->SetDDCPIntensity(255);
                    if (awareness)
                        te[i]->SetGroupColor(order);
                    te[i]->SetAssociatedColor(((DDColorPickerButton*)elem)->GetAssociatedColor());
                }
            }
            te.clear();
            for (int i = 0; i < tb.size(); i++)
            {
                if (tb[i])
                {
                    tb[i]->SetDDCPIntensity(((DDColorPicker*)elem->GetParent())->GetColorIntensity());
                    if (order == 0)
                        tb[i]->SetDDCPIntensity(255);
                    if (awareness)
                        tb[i]->SetGroupColor(order);
                    tb[i]->SetAssociatedColor(((DDColorPickerButton*)elem)->GetAssociatedColor());
                }
            }
            tb.clear();
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
        PostMessageW(g_msgwnd, WM_USER + 3, (WPARAM)lpParam, NULL);
        assignExtendedFn((Element*)lpParam, UpdateGlyphOnPress);
        return 0;
    }

    DWORD WINAPI CreateSliderInnerElements(LPVOID lpParam)
    {
        PostMessageW(g_msgwnd, WM_USER + 6, (WPARAM)lpParam, NULL);
        return 0;
    }

    DWORD WINAPI ColorPickerLayout(LPVOID lpParam)
    {
        PostMessageW(g_msgwnd, WM_USER + 4, (WPARAM)lpParam, NULL);
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
        PostMessageW(g_msgwnd, WM_USER + 8, (WPARAM)lpParam, NULL);
        return 0;
    }

    vector<DDScalableElement*> DDScalableElement::_arrCreatedElements;

    DDScalableElement::DDScalableElement()
    {
        _arrCreatedElements.push_back(this);
    }

    DDScalableElement::~DDScalableElement()
    {
        if (_pelPropChange)
        {
            this->RemoveListener(_pelPropChange);
            free(_pelPropChange);
            _pelPropChange = nullptr;
        }
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
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impNeedsFontResize2Prop,
            &impAssociatedColorProp,
            &impDDCPIntensityProp,
            &impStopListeningProp
        };
        return ClassInfo<DDScalableElement, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableElement", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    void DDScalableElement::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
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

    const PropertyInfo* WINAPI DDScalableElement::StopListeningProp()
    {
        return &impStopListeningProp;
    }

    bool DDScalableElement::GetStopListening()
    {
        return this->GetPropCommon(StopListeningProp, false);
    }

    void DDScalableElement::StopListening()
    {
        this->SetPropCommon(StopListeningProp, true, false);
    }

    unsigned short DDScalableElement::GetGroupColor()
    {
        return _gc;
    }

    void DDScalableElement::SetGroupColor(unsigned short sGC)
    {
        _gc = sGC;
    }

    void DDScalableElement::InitDrawImage()
    {
        PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableElement::RedrawImages()
    {
        for (DDScalableElement* pe : _arrCreatedElements)
        {
            PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableElement::InitDrawFont()
    {
        PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableElement::RedrawFonts()
    {
        for (DDScalableElement* pe : _arrCreatedElements)
        {
            PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)pe, NULL);
        }
    }

    vector<DDScalableButton*> DDScalableButton::_arrCreatedButtons;

    DDScalableButton::DDScalableButton()
    {
        _arrCreatedButtons.push_back(this);
    }

    DDScalableButton::~DDScalableButton()
    {
        if (_pelPropChange)
        {
            this->RemoveListener(_pelPropChange);
            free(_pelPropChange);
            _pelPropChange = nullptr;
        }
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
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impNeedsFontResize2Prop,
            &impAssociatedColorProp,
            &impDDCPIntensityProp,
            &impStopListeningProp
        };
        return ClassInfo<DDScalableButton, Button>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    void DDScalableButton::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
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

    const PropertyInfo* WINAPI DDScalableButton::StopListeningProp()
    {
        return &impStopListeningProp;
    }

    bool DDScalableButton::GetStopListening()
    {
        return this->GetPropCommon(StopListeningProp, false);
    }

    void DDScalableButton::StopListening()
    {
        this->SetPropCommon(StopListeningProp, true, false);
    }

    void DDScalableButton::InitDrawImage()
    {
        PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableButton::RedrawImages()
    {
        for (DDScalableButton* pe : _arrCreatedButtons)
        {
            PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableButton::InitDrawFont()
    {
        PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableButton::RedrawFonts()
    {
        for (DDScalableButton* pe : _arrCreatedButtons)
        {
            PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)pe, NULL);
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

    void DDScalableButton::SetAssociatedBool(bool* pb)
    {
        _assocBool = pb;
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

    vector<DDScalableRichText*> DDScalableRichText::_arrCreatedTexts;

    DDScalableRichText::DDScalableRichText()
    {
        _arrCreatedTexts.push_back(this);
    }

    DDScalableRichText::~DDScalableRichText()
    {
        if (_pelPropChange)
        {
            this->RemoveListener(_pelPropChange);
            free(_pelPropChange);
            _pelPropChange = nullptr;
        }
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
            &impAssociatedColorProp,
            &impStopListeningProp
        };
        return ClassInfo<DDScalableRichText, RichText>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableRichText", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    void DDScalableRichText::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
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

    const PropertyInfo* WINAPI DDScalableRichText::StopListeningProp()
    {
        return &impStopListeningProp;
    }

    bool DDScalableRichText::GetStopListening()
    {
        return this->GetPropCommon(StopListeningProp, false);
    }

    void DDScalableRichText::StopListening()
    {
        this->SetPropCommon(StopListeningProp, true, false);
    }

    void DDScalableRichText::InitDrawImage()
    {
        PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableRichText::RedrawImages()
    {
        for (DDScalableRichText* pe : _arrCreatedTexts)
        {
            PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableRichText::InitDrawFont()
    {
        PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableRichText::RedrawFonts()
    {
        for (DDScalableRichText* pe : _arrCreatedTexts)
        {
            PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)pe, NULL);
        }
    }

    vector<DDScalableTouchButton*> DDScalableTouchButton::_arrCreatedTButtons;

    DDScalableTouchButton::DDScalableTouchButton()
    {
        _arrCreatedTButtons.push_back(this);
    }

    DDScalableTouchButton::~DDScalableTouchButton()
    {
        if (_pelPropChange)
        {
            this->RemoveListener(_pelPropChange);
            free(_pelPropChange);
            _pelPropChange = nullptr;
        }
        auto toRemove = find(_arrCreatedTButtons.begin(), _arrCreatedTButtons.end(), this);
        if (toRemove != _arrCreatedTButtons.end())
        {
            _arrCreatedTButtons.erase(toRemove);
        }
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

    HRESULT DDScalableTouchButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDScalableTouchButton, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            ((DDScalableTouchButton*)*ppElement)->SetPropChangeListener((IElementListener*)assignExtendedFn(*ppElement, UpdateImageOnPropChange, true));
            ((DDScalableTouchButton*)*ppElement)->InitDrawImage();
            ((DDScalableTouchButton*)*ppElement)->InitDrawFont();
        }
        return hr;
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
            &impNeedsFontResize2Prop,
            &impAssociatedColorProp,
            &impStopListeningProp
        };
        return ClassInfo<DDScalableTouchButton, TouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableTouchButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    void DDScalableTouchButton::SetPropChangeListener(IElementListener* pel)
    {
        _pelPropChange = pel;
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

    const PropertyInfo* WINAPI DDScalableTouchButton::NeedsFontResize2Prop()
    {
        return &impNeedsFontResize2Prop;
    }

    bool DDScalableTouchButton::GetNeedsFontResize2()
    {
        return this->GetPropCommon(NeedsFontResize2Prop, false);
    }

    void DDScalableTouchButton::SetNeedsFontResize2(bool bNeedsFontResize2)
    {
        this->SetPropCommon(NeedsFontResize2Prop, bNeedsFontResize2, false);
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

    const PropertyInfo* WINAPI DDScalableTouchButton::StopListeningProp()
    {
        return &impStopListeningProp;
    }

    bool DDScalableTouchButton::GetStopListening()
    {
        return this->GetPropCommon(StopListeningProp, false);
    }

    void DDScalableTouchButton::StopListening()
    {
        this->SetPropCommon(StopListeningProp, true, false);
    }

    void DDScalableTouchButton::InitDrawImage()
    {
        PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)this, NULL);
    }

    void DDScalableTouchButton::RedrawImages()
    {
        for (DDScalableTouchButton* pe : _arrCreatedTButtons)
        {
            PostMessageW(g_msgwnd, WM_USER + 1, (WPARAM)pe, NULL);
        }
    }

    void DDScalableTouchButton::InitDrawFont()
    {
        PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)this, NULL);
    }

    void DDScalableTouchButton::RedrawFonts()
    {
        for (DDScalableTouchButton* pe : _arrCreatedTButtons)
        {
            PostMessageW(g_msgwnd, WM_USER + 2, (WPARAM)pe, NULL);
        }
    }

    RegKeyValue DDScalableTouchButton::GetRegKeyValue()
    {
        return _rkv;
    }

    void (*DDScalableTouchButton::GetAssociatedFn())(bool, bool, bool)
    {
        return _assocFn;
    }

    bool* DDScalableTouchButton::GetAssociatedBool()
    {
        return _assocBool;
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

    void DDScalableTouchButton::SetAssociatedBool(bool* pb)
    {
        _assocBool = pb;
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
        HRESULT hr = CreateAndInit<DDScalableTouchEdit, int>(0, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            DWORD dw2;
            HANDLE drawingHandle2 = CreateThread(nullptr, 0, CreateTEVisual, (LPVOID)*ppElement, NULL, &dw2);
            if (drawingHandle2) CloseHandle(drawingHandle2);
        }
        return hr;
    }

    HRESULT DDScalableTouchEdit::Register()
    {
        return ClassInfo<DDScalableTouchEdit, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableTouchEdit", nullptr, 0);
    }

    const WCHAR* DDScalableTouchEdit::GetContentString(Value** ppv)
    {
        CSafeElementPtr<TouchEdit2> peEdit;
        peEdit.Assign(regElem<TouchEdit2*>(L"TE_EditBox", this));
        if (peEdit) return peEdit->GetContentString(ppv);
        else return nullptr;
    }

    HRESULT DDScalableTouchEdit::SetContentString(const WCHAR* v)
    {
        CSafeElementPtr<TouchEdit2> peEdit;
        peEdit.Assign(regElem<TouchEdit2*>(L"TE_EditBox", this));
        if (peEdit) return peEdit->SetContentString(v);
        else return E_FAIL;
    }

    void DDScalableTouchEdit::UpdateTEPreview(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::KeyWithinProp())
        {
            CSafeElementPtr<DDScalableElement> TE_Background;
            TE_Background.Assign(regElem<DDScalableElement*>(L"TE_Background", elem->GetParent()->GetParent()));
            CSafeElementPtr<Element> TE_Preview;
            TE_Preview.Assign(regElem<Element*>(L"TE_Preview", elem->GetParent()->GetParent()));
            CValuePtr v;
            TE_Preview->SetVisible(!elem->GetKeyWithin());
            TE_Preview->SetContentString(elem->GetContentString(&v));
            TE_Background->SetSelected(elem->GetKeyWithin());
        }
        if (pProp == Element::MouseWithinProp())
        {
            CSafeElementPtr<DDScalableElement> TE_Background;
            TE_Background.Assign(regElem<DDScalableElement*>(L"TE_Background", elem->GetParent()->GetParent()));
            TE_Background->SetOverhang(elem->GetMouseWithin());
        }
        if (pProp == Element::EnabledProp())
        {
            CSafeElementPtr<DDScalableElement> TE_Background;
            TE_Background.Assign(regElem<DDScalableElement*>(L"TE_Background", elem->GetParent()->GetParent()));
            TE_Background->SetEnabled(elem->GetEnabled());
        }
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
        return ClassInfo<LVItem, DDScalableButton>::RegisterGlobal(HINST_THISCOMPONENT, L"LVItem", nullptr, 0);
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

    BYTE LVItem::GetMasterSmallPos()
    {
        return _smallPosMaster;
    }

    void LVItem::SetSmallPos(BYTE smPos)
    {
        _smallPos = smPos;
    }

    void LVItem::SetMasterSmallPos(BYTE smPos)
    {
        _smallPosMaster = smPos;
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

    Element* LVItem::GetShadow()
    {
        return _peShadow;
    }

    Element* LVItem::GetShortcutArrow()
    {
        return _peShortcutArrow;
    }

    RichText* LVItem::GetText()
    {
        return _peText;
    }

    RichText* LVItem::GetTextShadow()
    {
        return _peTextShadow;
    }

    Button* LVItem::GetCheckbox()
    {
        return _peCheckbox;
    }

    void LVItem::SetIcon(DDScalableElement* peIcon)
    {
        _peIcon = peIcon;
    }

    void LVItem::SetShadow(Element* peShadow)
    {
        _peShadow = peShadow;
    }

    void LVItem::SetShortcutArrow(Element* peShortcutArrow)
    {
        _peShortcutArrow = peShortcutArrow;
    }

    void LVItem::SetText(RichText* peText)
    {
        _peText = peText;
    }

    void LVItem::SetTextShadow(RichText* peTextShadow)
    {
        _peTextShadow = peTextShadow;
    }

    void LVItem::SetCheckbox(Button* peCheckbox)
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
        if (_itemCount == _maxCount) return;
        _items[_itemCount] = lvi;
        _itemCount++;
        lvi->SetSmallPos(_itemCount);
    }

    void LVItemTouchGrid::Insert(LVItem* lvi, BYTE index)
    {
        if (_itemCount >= _maxCount || index >= _maxCount || index < 0) return;
        BYTE internalIndex = index;
        if (index > _itemCount) internalIndex = _itemCount;
        for (int i = _itemCount - 1; i >= index; i--)
            _items[i + 1] = _items[i];
        _items[internalIndex] = lvi;
        _itemCount++;
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
        if (_itemCount > 0) _RefreshLVItemPositions(index, -1);
        else delete this;
    }

    BYTE LVItemTouchGrid::GetItemCount()
    {
        return _itemCount;
    }

    void LVItemTouchGrid::_RefreshLVItemPositions(BYTE index, short direction)
    {
        short localeDirection = (localeType == 1) ? -1 : 1;
        for (int i = index; i < _itemCount; i++)
        {
            if (i & 1)
            {
                _items[i]->SetMemXPos(_items[i]->GetMemXPos() + (g_touchSizeX + DESKPADDING_TOUCH) / 2 * localeDirection);
                _items[i]->SetX(_items[i]->GetMemXPos());
                if (direction == -1)
                {
                    _items[i]->SetMemYPos(_items[i]->GetMemYPos() - (g_touchSizeY + DESKPADDING_TOUCH) / 2);
                    _items[i]->SetY(_items[i]->GetMemYPos());
                }
            }
            else
            {
                _items[i]->SetMemXPos(_items[i]->GetMemXPos() - (g_touchSizeX + DESKPADDING_TOUCH) / 2 * localeDirection);
                _items[i]->SetX(_items[i]->GetMemXPos());
                if (direction == 1)
                {
                    _items[i]->SetMemYPos(_items[i]->GetMemYPos() + (g_touchSizeY + DESKPADDING_TOUCH) / 2);
                    _items[i]->SetY(_items[i]->GetMemYPos());
                }
            }
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
        return ClassInfo<DDLVActionButton, DDScalableButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDLVActionButton", nullptr, 0);
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
        return ClassInfo<DDToggleButton, DDScalableButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDToggleButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
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
        return ClassInfo<DDCheckBox, DDScalableButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDCheckBox", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
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

    HRESULT DDSlider::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        HRESULT hr = CreateAndInit<DDSlider, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
        if (SUCCEEDED(hr))
        {
            DWORD dw;
            HANDLE drawingHandle = CreateThread(nullptr, 0, CreateSliderInnerElements, (LPVOID)*ppElement, NULL, &dw);
            if (drawingHandle)
            {
                CloseHandle(drawingHandle);
            }
        }
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
        return ClassInfo<DDSlider, Button>::RegisterGlobal(HINST_THISCOMPONENT, L"DDSlider", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
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

    int DDSlider::GetDragStart()
    {
        return _dragStart;
    }

    int DDSlider::GetFillOnDragStart()
    {
        return _fodragStart;
    }

    int DDSlider::GetPosOnDragStart()
    {
        return _podragStart;
    }

    void DDSlider::SetMinValue(float minValue)
    {
        _minValue = minValue;
    }

    void DDSlider::SetMaxValue(float maxValue)
    {
        _maxValue = maxValue;
    }

    void DDSlider::SetCurrentValue(float currValue, bool fExternal)
    {
        if (currValue < _minValue) currValue = _minValue;
        if (currValue > _maxValue) currValue = _maxValue;
        _currValue = currValue;
        if (fExternal)
        {
            bool vertical = this->GetIsVertical();
            CSafeElementPtr<Button> peFill;
            peFill.Assign(regElem<Button*>(L"DDS_FillBase", this));
            CSafeElementPtr<Button> peThumb;
            peThumb.Assign(regElem<Button*>(L"DDS_Thumb", this));
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

    void DDSlider::SetDragStart(int dragStart)
    {
        _dragStart = dragStart;
    }

    void DDSlider::SetFillOnDragStart(int fodragStart)
    {
        _fodragStart = fodragStart;
    }

    void DDSlider::SetPosOnDragStart(int podragStart)
    {
        _podragStart = podragStart;
    }

    LPCWSTR DDSlider::GetFormattedString()
    {
        return _szFormatted;
    }

    void DDSlider::SetFormattedString(LPCWSTR szFormatted)
    {
        _szFormatted = szFormatted;
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
            if (drawingHandle)
            {
                CloseHandle(drawingHandle);
            }
        }
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

    vector<DDScalableButton*> DDColorPicker::GetTargetButtons()
    {
        return _targetBtns;
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

    void DDColorPicker::SetTargetButtons(vector<DDScalableButton*> vtb)
    {
        _targetBtns = vtb;
    }

    void DDColorPicker::SetThemeAwareness(bool ta)
    {
        _themeAwareness = ta;
    }

    DDColorPickerButton::~DDColorPickerButton()
    {
        if (_pelPropChange)
        {
            this->RemoveListener(_pelPropChange);
            free(_pelPropChange);
            _pelPropChange = nullptr;
        }
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
        return ClassInfo<DDColorPickerButton, Button>::RegisterGlobal(HINST_THISCOMPONENT, L"DDColorPickerButton", nullptr, 0);
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

    vector<DDScalableButton*> DDColorPickerButton::GetTargetButtons()
    {
        return _targetBtns;
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

    void DDColorPickerButton::SetTargetButtons(vector<DDScalableButton*> vtb)
    {
        _targetBtns = vtb;
    }

    LRESULT CALLBACK NotificationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        //WNDPROC WndProcNotification = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_WNDPROC);
        NotificationData* nd = (NotificationData*)wParam;
        switch (uMsg)
        {
            case WM_CLOSE:
                //DDNotificationBanner::DestroyBanner(nullptr);
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
        Sleep(50);
        if (nd->nb->GetWindowHost())
        {
            DWORD animCoef = g_animCoef;
            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            AnimateWindow(nd->nb->GetWindowHost()->GetHWND(), 180 * (animCoef / 100.0f), AW_BLEND);
            nd->nb->GetWindowHost()->ShowWindow(SW_SHOW);
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
        HRESULT hr = CreateAndInit<DDNotificationBanner, HWND, bool, UINT>(hParent, fDblBuffer, nCreate, pParent, pdwDeferCookie, ppElement);
        return hr;
    }

    HRESULT DDNotificationBanner::Register()
    {
        return ClassInfo<DDNotificationBanner, HWNDElement, EmptyCreator<DDNotificationBanner>>::RegisterGlobal(HINST_THISCOMPONENT, L"DDNotificationBanner", nullptr, 0);
    }

    NativeHWNDHost* DDNotificationBanner::GetWindowHost()
    {
        return _wnd;
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

    void DDNotificationBanner::SetWindowHost(NativeHWNDHost* pHost)
    {
        _wnd = pHost;
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
        NativeHWNDHost::Create(L"DD_NotificationHost", L"DirectDesktop In-App Notification", nullptr, nullptr, 0, 0, 0, 0, NULL, WS_POPUP | WS_BORDER, HINST_THISCOMPONENT, 0, &_wnd);
        HWNDElement::Create(_wnd->GetHWND(), true, NULL, nullptr, &keyN, (Element**)ppDDNB);
        Microsoft::WRL::ComPtr<ITaskbarList> pTaskbarList;
        if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList, (void**)&pTaskbarList)))
        {
            if (SUCCEEDED(pTaskbarList->HrInit()))
            {
                pTaskbarList->DeleteTab(_wnd->GetHWND());
            }
        }
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
        //WndProcNotification = (WNDPROC)SetWindowLongPtrW(_wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)NotificationProc);
        SetWindowSubclass(_wnd->GetHWND(), NotificationProc, 1, (DWORD_PTR)this);
        _pDDNB->SetVisible(true);
        _pDDNB->EndDefer(keyN);
        _wnd->Host(_pDDNB);
        WCHAR* WindowsBuildStr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
        int WindowsBuild = _wtoi(WindowsBuildStr);
        free(WindowsBuildStr);
        int WindowsRev = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\BuildLayers\\ShellCommon", L"BuildQfe");
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(_wnd->GetHWND(), &margins);
        if (WindowsBuild > 22000 || WindowsBuild == 22000 && WindowsRev >= 51)
        {
            DWORD cornerPreference = DWMWCP_ROUND;
            DwmSetWindowAttribute(_wnd->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
        }
        BlurBackground(_wnd->GetHWND(), true, false, nullptr);
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
                if (!title) _titleStr = LoadStrFromRes(217);
                _icon->SetClass(L"DDNB_Icon_Success");
                break;
            case DDNT_INFO:
                if (!title) _titleStr = LoadStrFromRes(218);
                _icon->SetClass(L"DDNB_Icon_Info");
                break;
            case DDNT_WARNING:
                if (!title) _titleStr = LoadStrFromRes(219);
                _icon->SetClass(L"DDNB_Icon_Warning");
                break;
            case DDNT_ERROR:
                if (!title) _titleStr = LoadStrFromRes(220);
                _icon->SetClass(L"DDNB_Icon_Error");
                break;
        }
        if (title) _titleStr = title;

        DDScalableElement::Create(pHostElement, nullptr, (Element**)&_title);
        _title->SetID(L"DDNB_Title");
        pHostElement->Add((Element**)&_title, 1);
        _title->SetContentString(_titleStr.c_str());

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
            SetWindowPos(_wnd->GetHWND(), HWND_TOPMOST, (dimensions.left + dimensions.right - cx) / 2, dimensions.top + 40 * g_flScaleFactor, cx, cy, SWP_FRAMECHANGED);
            notificationopen = true;
            NotificationData* nd = new NotificationData{ this, nullptr, timeout };
            HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateWindowWrapper, nd, NULL, nullptr);
            if (AnimHandle) CloseHandle(AnimHandle);
            g_nwnds.push_back(_wnd->GetHWND());
            DDNotificationBanner::RepositionBanners();
            if (timeout > 0)
            {
                NotificationData* nd = new NotificationData{ this, nullptr, timeout };
                DWORD dwAutoClose;
                AutoCloseHandle = CreateThread(nullptr, 0, AutoCloseNotification, nd, NULL, &dwAutoClose);
            }
            _pDDNB->SetWindowHost((NativeHWNDHost*)this); // Unsafe hack
        }
    }

    void DDNotificationBanner::RepositionBanners()
    {
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        int offset{};
        offset += dimensions.top + 40 * g_flScaleFactor;
        for (int i = g_nwnds.size() - 1; i >= 0; i--) {
            RECT windowRect{};
            GetClientRect(g_nwnds[i], &windowRect);
            SetWindowPos(g_nwnds[i], HWND_TOPMOST, (dimensions.left + dimensions.right - windowRect.right - 2 * g_flScaleFactor) / 2, offset, NULL, NULL, SWP_NOSIZE | SWP_FRAMECHANGED);
            offset += windowRect.bottom + 18 * g_flScaleFactor;
        }
    }

    void DDNotificationBanner::DestroyBanner(bool* notificationopen, bool manual)
    {
        if (_wnd != nullptr)
        {
            CValuePtr v;
            if (_peButtonSection)
            {
                DynamicArray<Element*>* pelButtons = _peButtonSection->GetChildren(&v);
                for (int i = 0; i < pelButtons->GetSize(); i++)
                    ((DDScalableButton*)pelButtons->GetItem(i))->StopListening();
            }
            auto toRemove = find(g_nwnds.begin(), g_nwnds.end(), _wnd->GetHWND());
            g_nwnds.erase(toRemove);
            DDNotificationBanner::RepositionBanners();
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
    }

    void DDNotificationBanner::DestroyBannerByButton(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            DDNotificationBanner* pDDNB = (DDNotificationBanner*)elem->GetParent()->GetParent();
            DDNotificationBanner* pDestroy = (DDNotificationBanner*)pDDNB->GetWindowHost();
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
            SetWindowPos(_wnd->GetHWND(), NULL, NULL, NULL, windowRect.right, cy, SWP_NOMOVE | SWP_NOZORDER);
            DDNotificationBanner::RepositionBanners();
        }
        _btnCount++;
        int flowlayoutParams[2] = { 1, _btnCount };
        CreateAndSetLayout(_peButtonSection, GridLayout::Create, ARRAYSIZE(flowlayoutParams), flowlayoutParams);
        DDScalableButton* pBtn{};
        DDScalableButton::Create(_peButtonSection, nullptr, (Element**)&pBtn);
        pBtn->SetNeedsFontResize(false);
        pBtn->SetNeedsFontResize2(false);
        pBtn->SetClass(L"pushbuttonsecondary");
        pBtn->SetHeight(32 * g_flScaleFactor);
        pBtn->SetMargin(8 * g_flScaleFactor, 0, 0, 0);
        pBtn->SetContentString(szButtonText);
        _peButtonSection->Add((Element**)&pBtn, 1);
        if (pListener) assignFn(pBtn, pListener);
        if (fClose) assignFn(pBtn, DDNotificationBanner::DestroyBannerByButton);
    }
}
