#include "pch.h"

#include "DDControls.h"
#include "common.h"
#include "SettingsHelper.h"
#include "coreui\AnimationHelper.h"
#include "coreui\BitmapHelper.h"
#include "coreui\StyleModifier.h"
#include <wrl.h>
#include <sstream>

using namespace std;
using namespace DirectUI;

namespace DDUI
{
    IClassInfo* DDScalableElement::s_pClassInfo;
    IClassInfo* DDScalableButton::s_pClassInfo;
    IClassInfo* DDScalableRichText::s_pClassInfo;
    IClassInfo* DDScalableTouchButton::s_pClassInfo;
    IClassInfo* DDScalableTouchEdit::s_pClassInfo;
    IClassInfo* LVCommon::s_pClassInfo;
    IClassInfo* LVGrid::s_pClassInfo;
    IClassInfo* LVTiles::s_pClassInfo;
    IClassInfo* LVItem::s_pClassInfo;
    IClassInfo* DDLVActionButton::s_pClassInfo;
    IClassInfo* DDIconButton::s_pClassInfo;
    IClassInfo* DDToggleButton::s_pClassInfo;
    IClassInfo* DDCheckBox::s_pClassInfo;
    IClassInfo* DDCheckBoxGlyph::s_pClassInfo;
    IClassInfo* DDNumberedButton::s_pClassInfo;
    IClassInfo* DDCombobox::s_pClassInfo;
    IClassInfo* DDSlider::s_pClassInfo;
    IClassInfo* DDColorPicker::s_pClassInfo;
    IClassInfo* DDColorPickerButton::s_pClassInfo;
    IClassInfo* DDTabbedPages::s_pClassInfo;
    IClassInfo* DDMenuButton::s_pClassInfo;

    struct MenuData
    {
        LONG x;
        LONG y;
        UINT uID;
        bool fInstant;
    };

    typedef HWND(WINAPI* pfnSHCreateWorkerWindowW)(WNDPROC, HWND, DWORD, DWORD, LPVOID);

    DDMenu* g_menu;
    vector<DDNotificationBanner*> g_nwnds{};

    WNDPROC WndProcNotification;
    WNDPROC g_oldMainProc;

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

    vector<wstring> SplitLineBreaks(const wstring& originalstr, WCHAR chBreak)
    {
        vector<wstring> strs;
        size_t start = 0;
        size_t end = originalstr.find(chBreak);
        while (end != wstring::npos)
        {
            strs.push_back(originalstr.substr(start, end - start));
            start = end + 1;
            end = originalstr.find(chBreak, start);
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
        vector<wstring> divided = SplitLineBreaks(textStr, L'\n');
        int saved{};
        for (int i = 0; i < divided.size(); i++)
        {
            GetTextExtentPoint32W(hdc, divided[i].c_str(), lstrlenW(divided[i].c_str()), szText);
            if (szText->cx > saved) saved = szText->cx;
        }
        szText->cx = saved;
    }

    void GetTextDimensions(Element* pe, const wstring& str, SIZE* psz, int* cy)
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
        if (cy) *cy += (lf.lfHeight * CalcLines(str));
        SelectObject(hdcMem, oldFont);
        DeleteObject(hFont);
        DeleteDC(hdcMem);
    }

    template <typename T>
    void RedrawImageCore(T* pe)
    {
        HMODULE hCaller = HINST_THISCOMPONENT;
        // 0.6 M1: Will be replaced with proper library loading later
        bool repeat = true;
    TEMP_ALTLOAD1:
        int scaleInterval = GetCurrentScaleInterval();
        int scaleIntervalImage = pe->GetScaledImageIntervals();
        if (scaleInterval > scaleIntervalImage - 1)
            scaleInterval = scaleIntervalImage - 1;
        int imageID = pe->GetFirstScaledImage() + scaleInterval;

        HBITMAP newImage = (HBITMAP)LoadImageW(hCaller, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (!newImage)
        {
            if (!repeat) hCaller = nullptr;
            LoadPNGAsBitmap(hCaller, newImage, imageID);
            IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1, NULL);
        }

        if (!newImage && repeat)
        {
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, (LPCWSTR)_ReturnAddress(), &hCaller);
            repeat = false;
            goto TEMP_ALTLOAD1;
        }

        if (newImage)
        {
            BITMAP bm{};
            GetObject(newImage, sizeof(BITMAP), &bm);
            int bmHeightNew = bm.bmHeight / pe->GetImageCount();

            HDC hdc = GetDC(nullptr);
            HDC hdcSrc = CreateCompatibleDC(hdc);
            HDC hdcDst = CreateCompatibleDC(hdc);
            SelectObject(hdcSrc, newImage);

            HBITMAP hbmIndexed = CreateCompatibleBitmap(hdc, bm.bmWidth, bmHeightNew);
            SelectObject(hdcDst, hbmIndexed);
            BitBlt(hdcDst, 0, 0, bm.bmWidth, bmHeightNew, hdcSrc, 0, (pe->GetImageIndex() - 1) * bmHeightNew, SRCCOPY);

            if (hbmIndexed)
            {
                COLORREF crAssoc;
                Value* vAssoc = pe->GetValue(DDScalableElement::AssociatedColorProp(), 1, nullptr);
                crAssoc = pe->GetAssociatedColor();
                double alpha = (pe->GetAssociatedColor() >> 24) / 255.0;
                if ((pe->GetAssociatedColor() >> 24) == 0)
                    alpha = 1.0;
                if ((crAssoc != 0 && crAssoc != 0xFFFFFFFF) || vAssoc->GetType() == (int)ValueType::Int)
                    IterateBitmap(hbmIndexed, StandardBitmapPixelHandler, 3, 0, (pe->GetDDCPIntensity() / 255.0) * alpha, pe->GetAssociatedColor());
                else if (pe->GetEnableAccent())
                    IterateBitmap(hbmIndexed, StandardBitmapPixelHandler, 1, 0, pe->GetDDCPIntensity() / 255.0, g_colors.ImmersiveColor);
                //else if (pe->GetDDCPIntensity() != 255)
                //    pe->SetAlpha(pe->GetDDCPIntensity()); // 0.5.8.2: Temporarily disabled
                vAssoc->Release();

                switch (pe->GetDrawType())
                {
                case 1:
                {
                    CValuePtr spvImage = Value::CreateGraphic(hbmIndexed, 7, 0xFFFFFFFF, true, false, false);
                    if (spvImage)
                        pe->SetValue(Element::BackgroundProp, 1, spvImage);
                    else
                        pe->SetBackgroundColor(0);
                    break;
                }
                case 2:
                {
                    CValuePtr spvImage = Value::CreateGraphic(hbmIndexed, 2, 0xFFFFFFFF, true, false, false);
                    if (spvImage)
                        pe->SetValue(Element::ContentProp, 1, spvImage);
                    else
                        pe->SetBackgroundColor(0);
                    break;
                }
                }
                DeleteObject(hbmIndexed);
            }
            DeleteObject(newImage);
            DeleteDC(hdcSrc);
            DeleteDC(hdcDst);
            ReleaseDC(nullptr, hdc);
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
            v->Release();
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
                int newFontSize = _wtoi(fontIntermediate2.c_str()) * g_ctx.dpi / static_cast<float>(g_ctx.dpiLaunch);
                wstring fontNew = fontIntermediate + to_wstring(newFontSize) + fontIntermediate3;
                pe->SetFont(fontNew.c_str());
                if (result) *result = false;
            }
            else if (pe->GetFontSize() > 0)
            {
                pe->SetFontSize(pe->GetFontSize() * g_ctx.flScaleFactor);
            }
        }
    }

    // Original author: Amrsatrio, modified by WinExperiments
    template <typename T>
    void RedrawBorderCore(T* pe)
    {
        CValuePtr v;
        RECT rcElement, rcRadius = *(pe->GetBorderRadius(&v));

        AddLayeredRef(pe->GetDisplayNode());

        Microsoft::WRL::ComPtr<IDCompositionVisual> spVisual;
        Microsoft::WRL::ComPtr<IDCompositionVisual> spVisualContent;
        Microsoft::WRL::ComPtr<IDCompositionDevice> spDUserDevice;

        HRESULT hr = ResultFromWin32Bool(GetGadgetVisual(0, pe->GetDisplayNode(), &spVisual, &spVisualContent, &spDUserDevice));

        if (!spDUserDevice.Get()) return;

        Microsoft::WRL::ComPtr<IDCompositionRectangleClip> spClip;
        spDUserDevice->CreateRectangleClip(&spClip);

        GetGadgetRect(pe->GetDisplayNode(), &rcElement, 0x8);

        spClip->SetLeft((float)rcElement.left);
        spClip->SetTop((float)rcElement.top);
        spClip->SetRight((float)rcElement.right);
        spClip->SetBottom((float)rcElement.bottom);

        
        float circularRadius = min((float)(rcElement.right - rcElement.left), (float)(rcElement.bottom - rcElement.top)) / 2.0f;
        if (rcRadius.left == -1) rcRadius.left = circularRadius;
        if (rcRadius.top == -1) rcRadius.top = circularRadius;
        if (rcRadius.right == -1) rcRadius.right = circularRadius;
        if (rcRadius.bottom == -1) rcRadius.bottom = circularRadius;

        spClip->SetBottomLeftRadiusX(rcRadius.bottom);
        spClip->SetBottomLeftRadiusY(rcRadius.bottom);
        spClip->SetBottomRightRadiusX(rcRadius.right);
        spClip->SetBottomRightRadiusY(rcRadius.right);
        spClip->SetTopLeftRadiusX(rcRadius.left);
        spClip->SetTopLeftRadiusY(rcRadius.left);
        spClip->SetTopRightRadiusX(rcRadius.top);
        spClip->SetTopRightRadiusY(rcRadius.top);

        spVisualContent->SetClip(spClip.Get());
        spDUserDevice->Commit();
    }

    DWORD WINAPI RedrawBorderCoreDelayed(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        if (*(dea->ppe))
        {
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 5);
        }
        else delete dea;
        return 0;
    }

    DWORD WINAPI SetVisibleBySelectionDelayed(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        if (dea->pe)
        {
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 6);
        }
        else delete dea;
        return 0;
    }

    static const int vvimpFirstScaledImageProp[] = { 1, 5, -1 };
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
    static const int vvimpImageCountProp[] = { 1, -1 };
    static PropertyInfoData dataimpImageCountProp;
    static const PropertyInfo impImageCountProp =
    {
        L"ImageCount",
        0x2 | 0x4,
        0x1,
        vvimpImageCountProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpImageCountProp
    };
    static const int vvimpImageIndexProp[] = { 1, -1 };
    static PropertyInfoData dataimpImageIndexProp;
    static const PropertyInfo impImageIndexProp =
    {
        L"ImageIndex",
        0x2 | 0x4,
        0x1,
        vvimpImageIndexProp,
        nullptr,
        Value::GetIntMinusOne,
        &dataimpImageIndexProp
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
    static const int vvimpAssociatedColorProp[] = { 1, 9, -1 };
    static PropertyInfoData dataimpAssociatedColorProp;
    static const PropertyInfo impAssociatedColorProp =
    {
        L"AssociatedColor",
        0x2 | 0x4,
        0x1,
        vvimpAssociatedColorProp,
        nullptr,
        Value::GetColorTrans,
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
    static const int vvimpDefaultColorProp[] = { 9, -1 };
    static PropertyInfoData dataimpDefaultColorProp;
    static const PropertyInfo impDefaultColorProp =
    {
        L"DefaultColor",
        0x2 | 0x4,
        0x1,
        vvimpDefaultColorProp,
        nullptr,
        Value::GetColorTrans,
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
    static const int vvimpIconFontProp[] = { 5, -1 };
    static PropertyInfoData dataimpIconFontProp;
    static const PropertyInfo impIconFontProp =
    {
        L"IconFont",
        0x2 | 0x4,
        0x1,
        vvimpIconFontProp,
        nullptr,
        Value::GetStringNull,
        &dataimpIconFontProp
    };
    static const int vvimpIconContentProp[] = { 11, 5, -1 };
    static PropertyInfoData dataimpIconContentProp;
    static const PropertyInfo impIconContentProp =
    {
        L"IconContent",
        0x2 | 0x4,
        0x1,
        vvimpIconContentProp,
        nullptr,
        Value::GetStringNull,
        &dataimpIconContentProp
    };
    static const int vvimpBorderRadiusProp[] = { 8, -1 };
    static PropertyInfoData dataimpBorderRadiusProp;
    static const PropertyInfo impBorderRadiusProp =
    {
        L"BorderRadius",
        0x2 | 0x4,
        0x1,
        vvimpBorderRadiusProp,
        nullptr,
        Value::GetRectZero,
        &dataimpBorderRadiusProp
    };
    static const int vvimpItemMinWidthProp[] = { 1, -1 };
    static PropertyInfoData dataimpItemMinWidthProp;
    static const PropertyInfo impItemMinWidthProp =
    {
        L"ItemMinWidth",
        0x2 | 0x4,
        0x1,
        vvimpItemMinWidthProp,
        nullptr,
        Value::GetIntZero,
        &dataimpItemMinWidthProp
    };
    static const int vvimpItemHeightProp[] = { 1, -1 };
    static PropertyInfoData dataimpItemHeightProp;
    static const PropertyInfo impItemHeightProp =
    {
        L"ItemHeight",
        0x2 | 0x4,
        0x1,
        vvimpItemHeightProp,
        nullptr,
        Value::GetIntZero,
        &dataimpItemHeightProp
    };

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
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageCountProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageIndexProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DDCPIntensityProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableElement>(this);
        }
        //static RECT rcOld, rcCurrent;
        //GetGadgetRect(this->GetDisplayNode(), &rcCurrent, 0x8);
        //if ((rcOld.right - rcOld.left != rcCurrent.right - rcCurrent.left || rcOld.bottom - rcOld.top != rcCurrent.bottom - rcCurrent.top) &&
        //    this->IsHosted() && DWMActive)
        //{
        //    CValuePtr v;
        //    RECT rcRadius = *(this->GetBorderRadius(&v));
        //    if (rcRadius.left != 0 || rcRadius.top != 0 || rcRadius.right != 0 || rcRadius.bottom != 0)
        //        RedrawBorderCore<DDScalableElement>(this);
        //}
        //GetGadgetRect(this->GetDisplayNode(), &rcOld, 0x8);
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::BorderRadiusProp) && g_ctx.DWMActive)
        {
            if (this->IsHosted())
                RedrawBorderCore<DDScalableElement>(this);
            else
            {
                DDScalableElement* ptr = this;
                DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(25), nullptr, (Element**)&ptr };
                HANDLE hRedraw = CreateThread(nullptr, 0, RedrawBorderCoreDelayed, dea, NULL, nullptr);
                if (hRedraw) CloseHandle(hRedraw);
            }
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
            &impImageCountProp,
            &impImageIndexProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impAssociatedColorProp,
            &impDDCPIntensityProp,
            &impBorderRadiusProp
        };
        return ClassInfo<DDScalableElement, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableElement", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableElement::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
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

    const PropertyInfo* WINAPI DDScalableElement::ImageCountProp()
    {
        return &impImageCountProp;
    }

    int DDScalableElement::GetImageCount()
    {
        int v = this->GetPropCommon(ImageCountProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableElement::SetImageCount(int iImageCount)
    {
        this->SetPropCommon(ImageCountProp, iImageCount, true);
    }

    const PropertyInfo* WINAPI DDScalableElement::ImageIndexProp()
    {
        return &impImageIndexProp;
    }

    int DDScalableElement::GetImageIndex()
    {
        int v = this->GetPropCommon(ImageIndexProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableElement::SetImageIndex(int iImageIndex)
    {
        this->SetPropCommon(ImageIndexProp, iImageIndex, true);
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

    COLORREF DDScalableElement::GetAssociatedColor()
    {
        if (this->IsDestroyed()) return 0;
        COLORREF crAssoc{};
        Value* pv = GetValue(AssociatedColorProp, 2, nullptr);
        if (pv->GetType() == (int)ValueType::Int || pv->GetType() == (int)ValueType::Unset)
        {
            int color = pv->GetInt();
            crAssoc = GetDUIImmersiveColor(color);
        }
        else if (pv->GetType() == (int)ValueType::Color)
        {
            const Fill* pf = pv->GetFill();
            crAssoc = pf->ref.cr;
        }
        pv->Release();
        return crAssoc;
    }

    void DDScalableElement::SetAssociatedColor(COLORREF crAssociatedColor)
    {
        Value* pv = Value::CreateColor(crAssociatedColor);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(AssociatedColorProp, 1, pv);
            pv->Release();
        }
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

    const PropertyInfo* WINAPI DDScalableElement::BorderRadiusProp()
    {
        return &impBorderRadiusProp;
    }

    const RECT* DDScalableElement::GetBorderRadius(Value** ppv)
    {
        Value* pv;
        if (this->IsDestroyed())
        {
            pv = impBorderRadiusProp.pData->_pvDefault;
            *ppv = pv;
        }
        else
            pv = GetValue(BorderRadiusProp, 2, nullptr);
        return pv->GetRect();
    }

    void DDScalableElement::SetBorderRadius(int l, int t, int r, int b)
    {
        Value* pv = Value::CreateRect(l, t, r, b, DSV_None);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(BorderRadiusProp, 1, pv);
            pv->Release();
        }
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
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageCountProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageIndexProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DDCPIntensityProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableButton>(this);
        }
        //static RECT rcOld, rcCurrent;
        //GetGadgetRect(this->GetDisplayNode(), &rcCurrent, 0x8);
        //if ((rcOld.right - rcOld.left != rcCurrent.right - rcCurrent.left || rcOld.bottom - rcOld.top != rcCurrent.bottom - rcCurrent.top) &&
        //    this->IsHosted() && DWMActive)
        //{
        //    CValuePtr v;
        //    RECT rcRadius = *(this->GetBorderRadius(&v));
        //    if (rcRadius.left != 0 || rcRadius.top != 0 || rcRadius.right != 0 || rcRadius.bottom != 0)
        //        RedrawBorderCore<DDScalableButton>(this);
        //}
        //GetGadgetRect(this->GetDisplayNode(), &rcOld, 0x8);
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::BorderRadiusProp) && g_ctx.DWMActive)
        {
            if (this->IsHosted())
                RedrawBorderCore<DDScalableButton>(this);
            else
            {
                DDScalableButton* ptr = this;
                DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(25), nullptr, (Element**)&ptr };
                HANDLE hRedraw = CreateThread(nullptr, 0, RedrawBorderCoreDelayed, dea, NULL, nullptr);
                if (hRedraw) CloseHandle(hRedraw);
            }
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
            RedrawFontCore<DDScalableButton>(this, nullptr, this->GetNeedsFontResize());
        Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDScalableButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDScalableButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDScalableButton::Register()
    {
        static const PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impImageCountProp,
            &impImageIndexProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impAssociatedColorProp,
            &impDDCPIntensityProp,
            &impBorderRadiusProp
        };
        return ClassInfo<DDScalableButton, Button>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableButton::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
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

    const PropertyInfo* WINAPI DDScalableButton::ImageCountProp()
    {
        return &impImageCountProp;
    }

    int DDScalableButton::GetImageCount()
    {
        int v = this->GetPropCommon(ImageCountProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableButton::SetImageCount(int iImageCount)
    {
        this->SetPropCommon(ImageCountProp, iImageCount, true);
    }

    const PropertyInfo* WINAPI DDScalableButton::ImageIndexProp()
    {
        return &impImageIndexProp;
    }

    int DDScalableButton::GetImageIndex()
    {
        int v = this->GetPropCommon(ImageIndexProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableButton::SetImageIndex(int iImageIndex)
    {
        this->SetPropCommon(ImageIndexProp, iImageIndex, true);
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

    COLORREF DDScalableButton::GetAssociatedColor()
    {
        if (this->IsDestroyed()) return 0;
        COLORREF crAssoc{};
        Value* pv = GetValue(AssociatedColorProp, 2, nullptr);
        if (pv->GetType() == (int)ValueType::Int || pv->GetType() == (int)ValueType::Unset)
        {
            int color = pv->GetInt();
            crAssoc = GetDUIImmersiveColor(color);
        }
        else if (pv->GetType() == (int)ValueType::Color)
        {
            const Fill* pf = pv->GetFill();
            crAssoc = pf->ref.cr;
        }
        pv->Release();
        return crAssoc;
    }

    void DDScalableButton::SetAssociatedColor(COLORREF crAssociatedColor)
    {
        Value* pv = Value::CreateColor(crAssociatedColor);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(AssociatedColorProp, 1, pv);
            pv->Release();
        }
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

    const PropertyInfo* WINAPI DDScalableButton::BorderRadiusProp()
    {
        return &impBorderRadiusProp;
    }

    const RECT* DDScalableButton::GetBorderRadius(Value** ppv)
    {
        Value* pv;
        if (this->IsDestroyed())
        {
            pv = impBorderRadiusProp.pData->_pvDefault;
            *ppv = pv;
        }
        else
            pv = GetValue(BorderRadiusProp, 2, nullptr);
        return pv->GetRect();
    }

    void DDScalableButton::SetBorderRadius(int l, int t, int r, int b)
    {
        Value* pv = Value::CreateRect(l, t, r, b, DSV_None);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(BorderRadiusProp, 1, pv);
            pv->Release();
        }
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
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageCountProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageIndexProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DDCPIntensityProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                RichText::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableRichText>(this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FontFaceProp))
        {
            CValuePtr v;
            const WCHAR* fontFace = this->GetFontFace(&v);
            this->SetAliasedRendering((!g_ctx.fontsmoothing || this->GetFontQuality() == 3) &&
                wcscmp(fontFace, L"Segoe UI Symbol") != 0 &&
                wcscmp(fontFace, L"Segoe MDL2 Assets") != 0 &&
                wcscmp(fontFace, L"Segoe Fluent Icons") != 0);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::BorderRadiusProp) && g_ctx.DWMActive)
        {
            if (this->IsHosted())
                RedrawBorderCore<DDScalableRichText>(this);
            else
            {
                DDScalableRichText* ptr = this;
                DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(25), nullptr, (Element**)&ptr };
                HANDLE hRedraw = CreateThread(nullptr, 0, RedrawBorderCoreDelayed, dea, NULL, nullptr);
                if (hRedraw) CloseHandle(hRedraw);
            }
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
            RedrawFontCore<DDScalableRichText>(this, nullptr, this->GetNeedsFontResize());
        RichText::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
        //if ((PropNotify::IsEqual(ppi, iIndex, Element::WidthProp) ||
        //    PropNotify::IsEqual(ppi, iIndex, Element::HeightProp) ||
        //    PropNotify::IsEqual(ppi, iIndex, Element::ContentProp) ||
        //    PropNotify::IsEqual(ppi, iIndex, Element::LayoutPosProp)) &&
        //    this->IsHosted() && DWMActive)
        //{
        //    RedrawBorderCore<DDScalableRichText>(this);
        //}
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
            &impImageCountProp,
            &impImageIndexProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impAssociatedColorProp,
            &impDDCPIntensityProp,
            &impBorderRadiusProp
        };
        return ClassInfo<DDScalableRichText, RichText>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableRichText", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableRichText::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
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

    const PropertyInfo* WINAPI DDScalableRichText::ImageCountProp()
    {
        return &impImageCountProp;
    }

    int DDScalableRichText::GetImageCount()
    {
        int v = this->GetPropCommon(ImageCountProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableRichText::SetImageCount(int iImageCount)
    {
        this->SetPropCommon(ImageCountProp, iImageCount, true);
    }

    const PropertyInfo* WINAPI DDScalableRichText::ImageIndexProp()
    {
        return &impImageIndexProp;
    }

    int DDScalableRichText::GetImageIndex()
    {
        int v = this->GetPropCommon(ImageIndexProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableRichText::SetImageIndex(int iImageIndex)
    {
        this->SetPropCommon(ImageIndexProp, iImageIndex, true);
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

    COLORREF DDScalableRichText::GetAssociatedColor()
    {
        if (this->IsDestroyed()) return 0;
        COLORREF crAssoc{};
        Value* pv = GetValue(AssociatedColorProp, 2, nullptr);
        if (pv->GetType() == (int)ValueType::Int || pv->GetType() == (int)ValueType::Unset)
        {
            int color = pv->GetInt();
            crAssoc = GetDUIImmersiveColor(color);
        }
        else if (pv->GetType() == (int)ValueType::Color)
        {
            const Fill* pf = pv->GetFill();
            crAssoc = pf->ref.cr;
        }
        pv->Release();
        return crAssoc;
    }

    void DDScalableRichText::SetAssociatedColor(COLORREF crAssociatedColor)
    {
        Value* pv = Value::CreateColor(crAssociatedColor);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(AssociatedColorProp, 1, pv);
            pv->Release();
        }
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

    const PropertyInfo* WINAPI DDScalableRichText::BorderRadiusProp()
    {
        return &impBorderRadiusProp;
    }

    const RECT* DDScalableRichText::GetBorderRadius(Value** ppv)
    {
        Value* pv;
        if (this->IsDestroyed())
        {
            pv = impBorderRadiusProp.pData->_pvDefault;
            *ppv = pv;
        }
        else
            pv = GetValue(BorderRadiusProp, 2, nullptr);
        return pv->GetRect();
    }

    void DDScalableRichText::SetBorderRadius(int l, int t, int r, int b)
    {
        Value* pv = Value::CreateRect(l, t, r, b, DSV_None);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(BorderRadiusProp, 1, pv);
            pv->Release();
        }
    }

    unsigned short DDScalableRichText::GetGroupColor()
    {
        return _gc;
    }

    void DDScalableRichText::SetGroupColor(unsigned short sGC)
    {
        _gc = sGC;
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
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageCountProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageIndexProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp) || 
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DDCPIntensityProp))
        {
            if (this->GetFirstScaledImage() == -1)
            {
                this->SetBackgroundColor(0);
                TouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
                return;
            }
            RedrawImageCore<DDScalableTouchButton>(this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FontFaceProp))
        {
            CValuePtr v;
            const WCHAR* fontFace = this->GetFontFace(&v);
            this->SetAliasedRendering((!g_ctx.fontsmoothing || this->GetFontQuality() == 3) &&
                wcscmp(fontFace, L"Segoe UI Symbol") != 0 &&
                wcscmp(fontFace, L"Segoe MDL2 Assets") != 0 &&
                wcscmp(fontFace, L"Segoe Fluent Icons") != 0);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::BorderRadiusProp) && g_ctx.DWMActive)
        {
            if (this->IsHosted())
                RedrawBorderCore<DDScalableTouchButton>(this);
            else
            {
                DDScalableTouchButton* ptr = this;
                DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(25), nullptr, (Element**)&ptr };
                HANDLE hRedraw = CreateThread(nullptr, 0, RedrawBorderCoreDelayed, dea, NULL, nullptr);
                if (hRedraw) CloseHandle(hRedraw);
            }
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
            RedrawFontCore<DDScalableTouchButton>(this, nullptr, this->GetNeedsFontResize());
        TouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
        //if ((PropNotify::IsEqual(ppi, iIndex, Element::WidthProp) ||
        //    PropNotify::IsEqual(ppi, iIndex, Element::HeightProp) ||
        //    PropNotify::IsEqual(ppi, iIndex, Element::ContentProp) ||
        //    PropNotify::IsEqual(ppi, iIndex, Element::LayoutPosProp)) &&
        //    this->IsHosted() && DWMActive)
        //{
        //    RedrawBorderCore<DDScalableTouchButton>(this);
        //}
    }

    HRESULT DDScalableTouchButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDScalableTouchButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDScalableTouchButton::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impFirstScaledImageProp,
            &impScaledImageIntervalsProp,
            &impImageCountProp,
            &impImageIndexProp,
            &impDrawTypeProp,
            &impEnableAccentProp,
            &impNeedsFontResizeProp,
            &impAssociatedColorProp,
            &impDDCPIntensityProp,
            &impBorderRadiusProp
        };
        return ClassInfo<DDScalableTouchButton, TouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableTouchButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    auto DDScalableTouchButton::GetPropCommon(const PropertyProcT pPropertyProc, bool useInt)
    {
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

    const PropertyInfo* WINAPI DDScalableTouchButton::ImageCountProp()
    {
        return &impImageCountProp;
    }

    int DDScalableTouchButton::GetImageCount()
    {
        int v = this->GetPropCommon(ImageCountProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableTouchButton::SetImageCount(int iImageCount)
    {
        this->SetPropCommon(ImageCountProp, iImageCount, true);
    }

    const PropertyInfo* WINAPI DDScalableTouchButton::ImageIndexProp()
    {
        return &impImageIndexProp;
    }

    int DDScalableTouchButton::GetImageIndex()
    {
        int v = this->GetPropCommon(ImageIndexProp, true);
        if (v < 1) v = 1;
        return v;
    }

    void DDScalableTouchButton::SetImageIndex(int iImageIndex)
    {
        this->SetPropCommon(ImageIndexProp, iImageIndex, true);
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

    COLORREF DDScalableTouchButton::GetAssociatedColor()
    {
        if (this->IsDestroyed()) return 0;
        COLORREF crAssoc{};
        Value* pv = GetValue(AssociatedColorProp, 2, nullptr);
        if (pv->GetType() == (int)ValueType::Int || pv->GetType() == (int)ValueType::Unset)
        {
            int color = pv->GetInt();
            crAssoc = GetDUIImmersiveColor(color);
        }
        else if (pv->GetType() == (int)ValueType::Color)
        {
            const Fill* pf = pv->GetFill();
            crAssoc = pf->ref.cr;
        }
        pv->Release();
        return crAssoc;
    }

    void DDScalableTouchButton::SetAssociatedColor(COLORREF crAssociatedColor)
    {
        Value* pv = Value::CreateColor(crAssociatedColor);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(AssociatedColorProp, 1, pv);
            pv->Release();
        }
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

    const PropertyInfo* WINAPI DDScalableTouchButton::BorderRadiusProp()
    {
        return &impBorderRadiusProp;
    }

    const RECT* DDScalableTouchButton::GetBorderRadius(Value** ppv)
    {
        Value* pv;
        if (this->IsDestroyed())
        {
            pv = impBorderRadiusProp.pData->_pvDefault;
            *ppv = pv;
        }
        else
            pv = GetValue(BorderRadiusProp, 2, nullptr);
        return pv->GetRect();
    }

    void DDScalableTouchButton::SetBorderRadius(int l, int t, int r, int b)
    {
        Value* pv = Value::CreateRect(l, t, r, b, DSV_None);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(BorderRadiusProp, 1, pv);
            pv->Release();
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

    void DDScalableTouchEdit::OnInput(InputEvent* pInput)
    {
        Element::OnInput(pInput);
    }

    bool DDScalableTouchEdit::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = Element::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::FontProp))
        {
            _pePreview->SetValue(Element::FontProp, 1, pvNew);
            _peEdit->SetValue(Element::FontProp, 1, pvNew);
            RedrawFontCore<DDScalableElement>(_pePreview, &result, this->GetNeedsFontResize());
            RedrawFontCore<TouchEdit2>(_peEdit, &result, this->GetNeedsFontResize());
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::ContentProp) || PropNotify::IsEqual(ppi, iIndex, TouchEdit2::PromptTextProp))
        {
            result = false;
            this->_SetValue(Element::AccNameProp, 1, pvNew, false);
            ElementSetValue(_peEdit, ppi, pvNew, this);
            CValuePtr v;
            _pePreview->SetVisible(!_peEdit->GetKeyWithin());
            if (!_peEdit->GetContentString(&v))
            {
                if (_peEdit->GetPromptText(&v))
                {
                    _pePreview->SetClass(L"prompttext");
                    _pePreview->SetContentString(_peEdit->GetPromptText(&v));
                }
                else _pePreview->SetContentString(L"");
            }
        }
        return result;
    }

    void DDScalableTouchEdit::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp))
        {
            ElementSetValue(_peBackground, ppi, pvNew, this);
            ElementSetValue(_peEdit, ppi, pvNew, this);
            ElementSetValue(_pePreview, ppi, pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::KeyWithinProp))
        {
            CValuePtr v;
            _pePreview->SetClass(L"");
            _pePreview->SetVisible(!_peEdit->GetKeyWithin());
            _pePreview->SetContentString(_peEdit->GetContentString(&v));
            this->SetSelected(_peEdit->GetKeyWithin());
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
        if (PropNotify::IsEqual(ppi, iIndex, Element::BorderThicknessProp))
        {
            const RECT* rcBorder = pvNew->GetRect();
            _peBackground->SetBorderThickness(rcBorder->left, rcBorder->top, rcBorder->right, rcBorder->bottom);
            this->SetPadding(-rcBorder->left, -rcBorder->top, -rcBorder->right, -rcBorder->bottom);
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::EnabledProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::FirstScaledImageProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ScaledImageIntervalsProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageCountProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::ImageIndexProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DrawTypeProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::EnableAccentProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::AssociatedColorProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDScalableElement::DDCPIntensityProp))
        {
            ElementSetValue(_peBackground, ppi, pvNew, this);
        }
        //static RECT rcOld, rcCurrent;
        //GetGadgetRect(this->GetDisplayNode(), &rcCurrent, 0x8);
        //if ((rcOld.right - rcOld.left != rcCurrent.right - rcCurrent.left || rcOld.bottom - rcOld.top != rcCurrent.bottom - rcCurrent.top) &&
        //    this->IsHosted() && DWMActive)
        //{
        //    CValuePtr v;
        //    RECT rcRadius = *(this->GetBorderRadius(&v));
        //    if (rcRadius.left != 0 || rcRadius.top != 0 || rcRadius.right != 0 || rcRadius.bottom != 0)
        //        RedrawBorderCore<DDScalableElement>(this);
        //}
        //GetGadgetRect(this->GetDisplayNode(), &rcOld, 0x8);
        if (PropNotify::IsEqual(ppi, iIndex, DDScalableElement::BorderRadiusProp) && g_ctx.DWMActive)
        {
            if (this->IsHosted())
                RedrawBorderCore<DDScalableElement>(this);
            else
            {
                DDScalableElement* ptr = this;
                DelayedElementActions* dea = new DelayedElementActions{ static_cast<DWORD>(25), nullptr, (Element**)&ptr };
                HANDLE hRedraw = CreateThread(nullptr, 0, RedrawBorderCoreDelayed, dea, NULL, nullptr);
                if (hRedraw) CloseHandle(hRedraw);
            }
        }
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
        return ClassInfo<DDScalableTouchEdit, DDScalableElement>::RegisterGlobal(HINST_THISCOMPONENT, L"DDScalableTouchEdit", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    const PropertyInfo* WINAPI DDScalableTouchEdit::PromptTextProp()
    {
        return TouchEdit2::PromptTextProp();
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

    void DDScalableTouchEdit::SetKeyFocus()
    {
        _peEdit->SetKeyFocus();
        Element::SetKeyFocus();
    }

    void DDScalableTouchEdit::SetContextMenu(bool fEnabled)
    {
        if (_fContextMenu != fEnabled)
        {
            _fContextMenu = fEnabled;
            if (fEnabled)
            {
                Microsoft::WRL::ComPtr<IDuiBehavior> spBehavior;
                if (SUCCEEDED(_behaviorHelper.CreateBehavior(L"Windows.UI.Popups", L"TouchEditContextMenu", nullptr, &spBehavior)))
                    _peEdit->AddBehavior(spBehavior.Get());
            }
        }
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

    LVCommonFlags operator&(LVCommonFlags lhs, LVCommonFlags rhs)
    {
        return static_cast<LVCommonFlags>(static_cast<DWORD>(lhs) & static_cast<DWORD>(rhs));
    }

    LVCommonFlags operator|(LVCommonFlags lhs, LVCommonFlags rhs)
    {
        return static_cast<LVCommonFlags>(static_cast<DWORD>(lhs) | static_cast<DWORD>(rhs));
    }

    LVItemFlags operator&(LVItemFlags lhs, LVItemFlags rhs)
    {
        return static_cast<LVItemFlags>(static_cast<DWORD>(lhs) & static_cast<DWORD>(rhs));
    }

    LVItemFlags operator|(LVItemFlags lhs, LVItemFlags rhs)
    {
        return static_cast<LVItemFlags>(static_cast<DWORD>(lhs) | static_cast<DWORD>(rhs));
    }

    IClassInfo* LVCommon::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void LVCommon::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* LVCommon::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void LVCommon::OnInput(InputEvent* pInput)
    {
        if (pInput->nCode == GMOUSE_MOVE && pInput->nDevice == GINPUT_KEYBOARD)
        {
            short sCtrl, sAKey;
            sCtrl = GetAsyncKeyState(VK_CONTROL);
            sAKey = GetAsyncKeyState('A');
            if ((sCtrl & 0x8000) && (sAKey & 0x8000) && !(_flags & LVCF_CTRLA))
            {
                this->AddFlags(LVCF_CTRLA);
                CValuePtr v;
                DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
                if (rgList)
                {
                    for (int items = 0; items < rgList->GetSize(); items++)
                        if (rgList->GetItem(items)->GetVisible())
                            rgList->GetItem(items)->SetSelected(true);
                }
            }
        }
        if (pInput->nCode == GMOUSE_DOWN && pInput->nDevice == GINPUT_KEYBOARD)
            this->RemoveFlags(LVCF_CTRLA);
        Element::OnInput(pInput);
    }

    void LVCommon::OnKeyFocusMoved(Element* peFrom, Element* peTo)
    {
        CValuePtr v;
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        BYTE ctrlKey = GetKeyState(VK_CONTROL);
        BYTE shiftKey = GetKeyState(VK_SHIFT);
        bool fKeyboardOrShift = (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000 || GetAsyncKeyState(VK_RBUTTON) & 0x8000) || shiftKey & 0x80);
        if (peTo == _peWhitespace && rgList && peFrom && peFrom->GetParent() != _peWhitespace)
        {
            if (!_peFocused)
                _peFocused = rgList->GetItem(0);
            if (!_peSelected)
            {
                _peSelected = rgList->GetItem(0);
                _idSelectedPivot = 0;
            }
            _peFocused->SetKeyFocus();
            if (fKeyboardOrShift)
                _peSelected->SetSelected(true);
        }
        if (peTo && peTo->GetClassInfoW() == LVItem::GetClassInfoPtr() && peTo->GetParent() == _peWhitespace)
        {
            if (peFrom && !(ctrlKey & 0x80))
            {
                if (rgList)
                {
                    UINT idCurrentItem{}, idFirstItem{}, idLastItem{};
                    for (int items = 0; items < rgList->GetSize(); items++)
                    {
                        if (rgList->GetItem(items) == peTo)
                        {
                            idCurrentItem = items;
                            if (!(shiftKey & 0x80)) _idSelectedPivot = idCurrentItem;
                        }
                        idFirstItem = _idSelectedPivot;
                        idLastItem = idCurrentItem;
                        if (idCurrentItem < _idSelectedPivot)
                        {
                            idFirstItem = idCurrentItem;
                            idLastItem = _idSelectedPivot;
                        }
                    }
                    if (fKeyboardOrShift)
                    {
                        for (int items = 0; items < rgList->GetSize(); items++)
                        {
                            bool withinShiftRange = ((shiftKey & 0x80) && items >= idFirstItem && items <= idLastItem);
                            if (rgList->GetItem(items) != peTo && !withinShiftRange && !(ctrlKey & 0x80))
                                rgList->GetItem(items)->SetSelected(false);
                            if (withinShiftRange)
                                rgList->GetItem(items)->SetSelected(true);
                        }
                    }
                }
                if (fKeyboardOrShift)
                {
                    _peSelected = peTo;
                    _peSelected->SetSelected(true);
                }
            }
            _peFocused = peTo;
        }
        Element::OnKeyFocusMoved(peFrom, peTo);
    }

    bool LVCommon::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = Element::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::HeightProp))
            result = false; 
        if (PropNotify::IsEqual(ppi, iIndex, Element::PaddingProp))
        {
            result = false;
            ElementSetValue(_peWhitespace, ppi, pvNew, this);
        }
        return result;
    }

    HRESULT LVCommon::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<LVCommon, int>(0, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT LVCommon::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((Element*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateLVVisual();
        HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
        if (hShlwapi)
        {
            pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
            _hWorker = SHCreateWorkerWindowW(s_ListViewProc, HWND_MESSAGE, 0, 0, nullptr);
        }
        return hr;
    }

    HRESULT LVCommon::Register()
    {
        return ClassInfo<LVCommon, Element>::RegisterGlobal(HINST_THISCOMPONENT, L"LVCommon", nullptr, 0);
    }

    Element* LVCommon::GetSelectionElement()
    {
        return _peSelector;
    }

    TouchButton* LVCommon::GetWhitespaceElement()
    {
        return _peWhitespace;
    }

    LVCommonFlags LVCommon::GetFlags()
    {
        return _flags;
    }

    void LVCommon::AddFlags(LVCommonFlags lvcf)
    {
        _flags = _flags | lvcf;
    }

    void LVCommon::RemoveFlags(LVCommonFlags lvcf)
    {
        _flags = _flags & static_cast<LVCommonFlags>(0xFFFFFFFF - lvcf);
    }

    void LVCommon::SetFlags(LVCommonFlags lvcf)
    {
        _flags = lvcf;
    }

    POINT LVCommon::GetDragOriginPoint()
    {
        return _ptOrigin;
    }

    SIZE LVCommon::GetDragSize()
    {
        return _szDrag;
    }

    void LVCommon::SelectItemBase(Element* elem, Event* iev)
    {
        static int validation = 0;
        if (iev->uidType == LVItem::Click())
        {
            short ctrlKey = GetAsyncKeyState(VK_CONTROL);
            short shiftKey = GetAsyncKeyState(VK_SHIFT);
            validation++;
            TouchButton* checkbox = ((LVItem*)elem)->GetCheckbox();
            bool fInCheckbox = (checkbox && checkbox->GetMouseWithin());
            if (!(ctrlKey & 0x8000) && !fInCheckbox)
            {
                CValuePtr v;
                DynamicArray<Element*>* rgList = elem->GetParent()->GetChildren(&v);
                if (rgList)
                {
                    for (int items = 0; items < rgList->GetSize(); items++)
                    {
                        if (rgList->GetItem(items) != elem && !(shiftKey & 0x8000))
                            rgList->GetItem(items)->SetSelected(false);
                    }
                }
            }
            bool selectedOld = elem->GetSelected();
            if (!fInCheckbox) elem->SetSelected(ctrlKey ? !elem->GetSelected() : true);
            else if (validation & 1) elem->SetSelected(!elem->GetSelected());
            if (elem->GetSelected() == selectedOld == true)
                ((LVItem*)elem)->AddFlags(LVIF_MEMSELECT);
        }
    }

    void LVCommon::RefineSelections(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseWithinProp() || pProp == Element::SelectedProp())
        {
            if (pProp == Element::SelectedProp())
            {
                if (!pV2->GetBool())
                    ((LVItem*)elem)->RemoveFlags(LVIF_MEMSELECT);
            }
            TouchButton* checkboxElem = ((LVItem*)elem)->GetCheckbox();
            bool fKeyboard = (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000 ||
                GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000 ||
                (GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState('A') & 0x8000) ||
                GetAsyncKeyState(VK_SHIFT) & 0x8000);
            if (elem->GetParent())
            {
                DWORD flags = ((LVCommon*)elem->GetParent()->GetParent())->GetFlags();
                if (!(flags & LVCF_TOUCH) && g_ctx.labelshadow && ((!elem->GetMouseWithin() && pProp == Element::SelectedProp()) ||
                    (!elem->GetSelected() && pProp == Element::MouseWithinProp())))
                {
                    CSafeElementPtr<Element> innerElem;
                    innerElem.Assign(((LVItem*)elem)->GetInnerElement());
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    if (innerElem)
                    {
                        float initialFade = elem->GetSelected() ? 1.0f : elem->GetMouseWithin() ? 0.0f : 1.0f;
                        float finalFade = elem->GetMouseWithin() || elem->GetSelected() ? 1.0f : 0.0f;
                        innerElem->SetVisible(!fKeyboard || finalFade == 1.0f);
                        if (!fKeyboard)
                        {
                            TriggerFade(innerElem, transDesc, 0, 0.0f, 0.125f, 0.1f, 0.25f, 0.75f, 0.9f, initialFade, finalFade, false, false, false);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                            DWORD animCoef = g_ctx.animCoef;
                            if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                            DelayedElementActions* dea = new DelayedElementActions{ animCoef, innerElem , nullptr };
                            HANDLE hFadeHelper = CreateThread(nullptr, 0, SetVisibleBySelectionDelayed, dea, NULL, nullptr);
                            if (hFadeHelper) CloseHandle(hFadeHelper);
                        }
                    }
                    // Apparently this needs to be done otherwise there will be ugly overlays...
                    ////////////////////////////////////
                    if (innerElem)
                    {
                        CValuePtr v;
                        DynamicArray<Element*>* pel = elem->GetChildren(&v);
                        if (pel && !fKeyboard)
                        {
                            for (int id = 0; id < pel->GetSize(); id++)
                            {
                                Element* child = pel->GetItem(id);
                                if (child == innerElem) continue;
                                TriggerFade(child, transDesc, 0, 0.0f, 0.125f, 0.0f, 0.0f, 1.0f, 1.0f, 0.99f, 1.0f, false, false, false);
                                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                            }
                        }
                    }
                    ////////////////////////////////////
                }
            }
            if (g_ctx.showcheckboxes == 1 && checkboxElem)
                checkboxElem->SetVisible(elem->GetMouseWithin() || elem->GetSelected());
        }
        if (pProp == LVItem::CapturedProp())
        {
            if (((LVItem*)elem)->GetCaptured() && elem->GetMouseWithin() && !elem->GetSelected() &&
                !(GetAsyncKeyState(VK_CONTROL) & 0x8000) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            {
                CValuePtr v;
                DynamicArray<Element*>* rgList = elem->GetParent()->GetChildren(&v);
                if (rgList)
                {
                    for (int items = 0; items < rgList->GetSize(); items++)
                        rgList->GetItem(items)->SetSelected(false);
                }
                elem->SetSelected(true);
            }
        }
    }

    void LVCommon::CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseFocusedProp())
        {
            UpdateCache* uc{};
            LVItem* parent = (LVItem*)elem->GetParent()->GetParent();
            CValuePtr v = elem->GetValue(Element::MouseFocusedProp, 1, uc);
            if (parent->GetInnerElement())
                parent->GetInnerElement()->SetValue(Element::MouseFocusedProp(), 1, v);
            if (uc) free(uc);
        }
    }

    HRESULT LVCommon::Add(Element* pe)
    {
        return this->Add(&pe, 1);
    }

    HRESULT LVCommon::Add(Element** ppe, UINT cCount)
    {
        HRESULT hr = _peWhitespace->Add(ppe, cCount);
        if (SUCCEEDED(hr))
        {
            RECT* prcAdd = new RECT[cCount];
            RECT rcNext{};
            for (int i = 0; i < cCount; i++)
            {
                GetGadgetRect(ppe[i]->GetDisplayNode(), &prcAdd[i], 0xC);
            }
            this->_OnAddOrInsert(ppe, prcAdd, &rcNext, cCount);
            delete[] prcAdd;
        }
        return hr;
    }

    HRESULT LVCommon::Add(Element* pe, CompareCallback lpfnCompare)
    {
        HRESULT hr = _peWhitespace->Add(pe, lpfnCompare);
        if (SUCCEEDED(hr))
        {
            RECT* prcAdd = new RECT[1];
            RECT rcNext{};
            GetGadgetRect(pe->GetDisplayNode(), &prcAdd[0], 0xC);
            if (pe->GetClassInfoW() == LVItem::GetClassInfoPtr())
                this->_OnAddOrInsert(&pe, prcAdd, &rcNext, 1);
            delete[] prcAdd;
        }
        return hr;
    }

    HRESULT LVCommon::Insert(Element* pe, UINT iInsertIdx)
    {
        return this->Insert(&pe, 1, iInsertIdx);
    }

    HRESULT LVCommon::Insert(Element** ppe, UINT cCount, UINT iInsertIdx)
    {
        HRESULT hr = _peWhitespace->Insert(ppe, cCount, iInsertIdx);
        if (SUCCEEDED(hr))
        {
            RECT* prcInsert = new RECT[cCount];
            RECT rcNext{};
            for (int i = 0; i < cCount; i++)
            {
                GetGadgetRect(ppe[i]->GetDisplayNode(), &prcInsert[i], 0xC);
            }
            if (!(_flags & LVCF_NOANIMATE))
            {
                if (this->GetClassInfoW() != LVGrid::GetClassInfoPtr())
                {
                    CValuePtr v;
                    DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
                    if (rgList && rgList->GetSize() - cCount > iInsertIdx)
                        GetGadgetRect(rgList->GetItem(iInsertIdx + cCount)->GetDisplayNode(), &rcNext, 0xC);
                }
            }
            this->_OnAddOrInsert(ppe, prcInsert, &rcNext, cCount);
            delete[] prcInsert;
        }
        return hr;
    }

    HRESULT LVCommon::Remove(Element* pe)
    {
        return this->Remove(&pe, 1);
    }

    HRESULT LVCommon::Remove(Element** ppe, UINT cCount)
    {
        RECT* prcRemove = new RECT[cCount];
        RECT* prcNext = new RECT[cCount];
        RECT rcParent{};
        Element** ppeClone = new Element*[cCount];
        HRESULT hr = S_OK;
        for (int i = 0; i < cCount; i++)
        {
            GetGadgetRect(ppe[i]->GetDisplayNode(), &prcRemove[i], 0xC);
            if (ppe[i] == _peSelected)
                _peSelected = nullptr;
            if (ppe[i] == _peFocused)
                _peFocused = nullptr;
        }
        if (!(_flags & LVCF_NOANIMATE))
        {
            hr = this->_OnRemoving(ppe, ppeClone, &rcParent, prcRemove, prcNext, cCount);
        }
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(_peWhitespace->Remove(ppe, cCount)))
                this->_OnRemove(ppe, ppeClone, prcRemove, prcNext, cCount);
        }
        delete[] prcRemove;
        delete[] prcNext;
        delete[] ppeClone;
        return hr;
    }

    HRESULT LVCommon::RemoveAll()
    {
        _peSelected = nullptr;
        _peFocused = nullptr;
        this->_OnRemoveAll();
        return _peWhitespace->RemoveAll();
    }

    HRESULT LVCommon::RemoveAndDestroy(Element* pe)
    {
        return this->RemoveAndDestroy(&pe, 1);
    }

    HRESULT LVCommon::RemoveAndDestroy(Element** ppe, UINT cCount)
    {
        RECT* prcRemove = new RECT[cCount];
        RECT* prcNext = new RECT[cCount];
        RECT rcParent{};
        Element** ppeClone = new Element*[cCount];
        HRESULT hr = S_OK;
        for (int i = 0; i < cCount; i++)
        {
            GetGadgetRect(ppe[i]->GetDisplayNode(), &prcRemove[i], 0xC);
            if (ppe[i] == _peSelected)
                _peSelected = nullptr;
            if (ppe[i] == _peFocused)
                _peFocused = nullptr;
        }
        if (!(_flags & LVCF_NOANIMATE))
        {
            hr = this->_OnRemoving(ppe, ppeClone, &rcParent, prcRemove, prcNext, cCount);
        }
        for (int i = 0; i < cCount && SUCCEEDED(hr); i++)
        {
            hr = ppe[i]->DestroyAll(true);
            if (SUCCEEDED(hr))
                hr = ppe[i]->Destroy(true);
        }
        if (SUCCEEDED(hr))
        {
            this->_OnRemove(ppe, ppeClone, prcRemove, prcNext, cCount);
        }
        delete[] prcRemove;
        delete[] prcNext;
        delete[] ppeClone;
        return hr;
    }

    HRESULT LVCommon::Destroy(bool fDelayed)
    {
        DestroyWindow(_hWorker);
        return Element::Destroy(fDelayed);
    }

    HRESULT LVCommon::DestroyAll(bool fDelayed)
    {
        this->_OnRemoveAll();
        return _peWhitespace->DestroyAll(fDelayed);
    }

    HRESULT LVCommon::_CreateLVVisual()
    {
        HRESULT hr = S_OK;

        CValuePtr spvLayout;
        FillLayout::Create(0, nullptr, &spvLayout);
        hr = this->_SetValue(Element::LayoutProp, 1, spvLayout, true);
        if (SUCCEEDED(hr))
        {
            hr = TouchButton::Create(this, nullptr, (Element**)&_peWhitespace);
            if (SUCCEEDED(hr))
            {
                Element::Insert((Element**)&_peWhitespace, 1, 0);
                _peWhitespace->SetID(L"whitespace");
                assignExtendedFn(_peWhitespace, _MarqueeSelector, true);
                if (SUCCEEDED(hr))
                {
                    if (this->GetClassInfoW() == LVCommon::GetClassInfoPtr())
                        CreateAndSetLayout(_peWhitespace, BorderLayout::Create, 0, nullptr);
                    else if (this->GetClassInfoW() == LVGrid::GetClassInfoPtr())
                        CreateAndSetLayout(_peWhitespace, FillLayout::Create, 0, nullptr);
                    _peWhitespace->SetLayoutPos(-1);
                    hr = Element::Create(0, this, nullptr, &_peSelector);
                    if (SUCCEEDED(hr))
                    {
                        Element::Insert(&_peSelector, 1, 1);
                        _peSelector->SetID(L"selector");
                        _peSelector->SetLayoutPos(-2);
                        _peSelector->SetVisible(false);
                        _peSelector->SetClass(g_ctx.selectionrect ? L"selectoralpha" : L"selectornoalpha");
                        WCHAR* cxDragStr{}, * cyDragStr{};
                        GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragWidth", &cxDragStr);
                        GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragHeight", &cyDragStr);
                        _szDrag.cx = _wtoi(cxDragStr);
                        _szDrag.cy = _wtoi(cyDragStr);
                        GetGadgetRect(this->GetDisplayNode(), &_rcGadget, 0xC);
                    }
                }
            }
        }
        return hr;
    }

    HRESULT LVCommon::_CreateAnimatingClone(Element** ppeOrig, RECT* prcOrig, Element** ppeClone, UINT cCount)
    {
        for (int i = 0; i < cCount; i++)
        {
            if (!ppeClone)
                return E_POINTER;
            if (ppeOrig[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
            {
                ppeClone[i] = nullptr;
                continue;
            }
            Element* peClone{};
            Element* peOrig = ppeOrig[i];
            RECT rcOrig = prcOrig[i];
            Element::Create(0, peOrig->GetParent()->GetParent(), nullptr, &peClone);
            peOrig->GetParent()->GetParent()->Add(&peClone, 1);
            AddLayeredRef(peClone->GetDisplayNode());
            SetGadgetFlags(peClone->GetDisplayNode(), NULL, NULL);
            peOrig->SetLayoutPos(-2);
            peOrig->SetX(-1); // This force refreshes the absolute placement, allowing for the gadget bitmap's capture.
            peOrig->SetX(0);
            peOrig->SetY(0);
            peOrig->SetWidth(rcOrig.right - rcOrig.left);
            peOrig->SetHeight(rcOrig.bottom - rcOrig.top);
            RECT rcParent{};
            GetGadgetRect(peOrig->GetParent()->GetDisplayNode(), &rcParent, 0xC);
            peClone->SetLayoutPos(-2);
            peClone->SetAccItemStatus(L"Cloned");
            peClone->SetX(rcOrig.left - rcParent.left);
            peClone->SetY(rcOrig.top - rcParent.top - 1);
            peClone->SetWidth(rcOrig.right - rcOrig.left);
            peClone->SetHeight(rcOrig.bottom - rcOrig.top);
            HBITMAP hbmOld;
            RECT rcDummy{};
            GetGadgetBitmap(peOrig->GetDisplayNode(), &hbmOld, &rcDummy);
            IterateBitmap(hbmOld, UndoPremultiplication, 1, 0, 1, NULL);
            CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(hbmOld, 7, 0xffffffff, false, false, false);
            if (spvBitmap)
                peClone->SetValue(Element::BackgroundProp, 1, spvBitmap);
            DeleteObject(hbmOld);
            if (ppeClone)
                ppeClone[i] = peClone;
        }
        return S_OK;
    }

    void LVCommon::_RemoveStuckClones(DynamicArray<Element*>* rgList)
    {
        if (_ullRemoveTick && GetTickCount64() - _ullRemoveTick > _dwSafeRemove)
        {
            for (int i = 0; i < rgList->GetSize(); i++)
            {
                CValuePtr v2;
                Element* child = rgList->GetItem(i);
                const WCHAR* accstatus = child->GetAccItemStatus(&v2);
                if (accstatus && wcscmp(accstatus, L"Cloned") == 0)
                    child->Destroy(true);
            }
        }
    }

    void LVCommon::_MarqueeSelector(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        DWORD marqueeThread;
        HANDLE marqueeThreadHandle;
        LVCommon* listview = (LVCommon*)elem->GetParent();
        if (pProp == TouchButton::PressedProp())
        {
            if (!(listview->_flags & LVCF_PRESSED))
            {
                GetGadgetRect(listview->GetDisplayNode(), &listview->_rcGadget, 0xC);
                GetCursorPos(&listview->_ptOrigin);
                ScreenToClient(((HWNDElement*)listview->GetRoot())->GetHWND(), &listview->_ptOrigin);
                GTRANS_DESC transDesc[1];
                TransitionStoryboardInfo tsbInfo = {};
                Element* selector = listview->_peSelector;
                TriggerFade(selector, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                DUI_SetGadgetZOrder(selector, 1);
                selector->SetWidth(0);
                selector->SetHeight(0);
                selector->SetX(listview->_ptOrigin.x);
                selector->SetY(listview->_ptOrigin.y);
                selector->SetVisible(true);
                marqueeThreadHandle = CreateThread(nullptr, 0, _UpdateMarqueeSelectorPosition, (LPVOID)listview, 0, &marqueeThread);
                if (marqueeThreadHandle) CloseHandle(marqueeThreadHandle);
            }
            listview->AddFlags(LVCF_PRESSED);
        }
        else if ((!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) || !elem->GetMouseWithin()) && listview && listview->_flags & LVCF_PRESSED)
        {
            Element* selector = listview->_peSelector;
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerFade(selector, transDesc, 0, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
            DUI_SetGadgetZOrder(selector, -1);
            listview->RemoveFlags(LVCF_PRESSED);
        }
    }

    DWORD WINAPI LVCommon::_UpdateMarqueeSelectorPosition(LPVOID lpParam)
    {
        LVCommon* listview = (LVCommon*)lpParam;
        bool ctrlKey = GetKeyState(VK_CONTROL) < 0;
        SetWindowLongPtrW(listview->_hWorker, GWLP_USERDATA, (LONG_PTR)listview);
        while (true)
        {
            if (!(listview->_flags & LVCF_PRESSED)) break;
            Sleep(10);
            SendMessageW(listview->_hWorker, WM_USER + 1, NULL, ctrlKey);
        }
        return 0;
    }

    LRESULT CALLBACK LVCommon::s_ListViewProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LVCommon* lvc = (LVCommon*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        switch (uMsg)
        {
        case WM_SETTINGCHANGE:
        {
            bool selectionrect = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ListviewAlphaSelect");
            lvc->_peSelector->SetClass(selectionrect ? L"selectoralpha" : L"selectornoalpha");
            break;
        }
        case WM_USER + 1:
        {
            POINT ppt;
            GetCursorPos(&ppt);
            ScreenToClient(((HWNDElement*)lvc->GetRoot())->GetHWND(), &ppt);
            MARGINS borders = {
                (ppt.x < lvc->_ptOrigin.x) ? ppt.x : lvc->_ptOrigin.x, abs(ppt.x - lvc->_ptOrigin.x),
                (ppt.y < lvc->_ptOrigin.y) ? ppt.y : lvc->_ptOrigin.y, abs(ppt.y - lvc->_ptOrigin.y)
            };
            if (borders.cxRightWidth == 0) borders.cxRightWidth = 1;
            if (borders.cyBottomHeight == 0) borders.cyBottomHeight = 1;
            borders.cxLeftWidth -= lvc->_rcGadget.left;
            borders.cyTopHeight -= lvc->_rcGadget.top;
            Element* selector = lvc->_peSelector;
            selector->SetWidth(borders.cxRightWidth);
            selector->SetX(borders.cxLeftWidth);
            selector->SetHeight(borders.cyBottomHeight);
            selector->SetY(borders.cyTopHeight);
            CValuePtr v;
            DynamicArray<Element*>* rgList = lvc->_peWhitespace->GetChildren(&v);
            if (rgList && rgList->GetSize() > 0)
            {
                for (int items = 0; items < rgList->GetSize(); items++)
                {
                    LVItem* child = (LVItem*)rgList->GetItem(items);
                    bool isLVItem = (child->GetClassInfoW() == LVItem::GetClassInfoPtr());
                    if (isLVItem)
                    {
                        RECT rcBorders{};
                        GetGadgetRect(child->GetDisplayNode(), &rcBorders, 0xC);
                        bool selectstate = (borders.cxRightWidth + borders.cxLeftWidth + lvc->_rcGadget.left > rcBorders.left &&
                            rcBorders.right > borders.cxLeftWidth + lvc->_rcGadget.left &&
                            borders.cyBottomHeight + borders.cyTopHeight + lvc->_rcGadget.top > rcBorders.top &&
                            rcBorders.bottom > borders.cyTopHeight + lvc->_rcGadget.top &&
                            child->GetVisible());
                        if (lParam)
                        {
                            if (selectstate)
                            {
                                if (!(child->GetFlags() & LVIF_NOSELTRIGGER))
                                {
                                    rgList->GetItem(items)->SetSelected(!(rgList->GetItem(items)->GetSelected()));
                                    child->AddFlags(LVIF_NOSELTRIGGER);
                                }
                            }
                            else
                                child->RemoveFlags(LVIF_NOSELTRIGGER);
                        }
                        else
                            child->SetSelected(selectstate);
                    }
                }
            }
            return 0;
        }
        case WM_USER + 2: // 0.5.8: Could be used for drag and drop in the future
        {
            return 0;
        }
        default:
            break;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    void LVCommon::_OnAddOrInsert(Element** ppe, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        UINT iMaxIdx = 0;
        for (int i = 0; i < cCount; i++)
        {
            if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                continue;
            ((LVItem*)ppe[i])->SetShowKeyFocus(false);
            if (!(_flags & LVCF_NOASSIGNFUNC))
            {
                assignFn(ppe[i], SelectItemBase);
                assignExtendedFn(ppe[i], RefineSelections);
                if (((LVItem*)ppe[i])->GetCheckbox())
                    assignExtendedFn(((LVItem*)ppe[i])->GetCheckbox(), CheckboxHandler);
            }
            iMaxIdx = i;
        }
        if (!(_flags & LVCF_NOANIMATE))
        {
            RECT rcLVHost;
            Element* peParent = this;
            while (peParent->GetClassInfoW() != TouchScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != StyledScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != ScrollViewer::GetClassInfoPtr())
            {
                if (!peParent->GetParent())
                    break;
                peParent = peParent->GetParent();
            }
            GetGadgetRect(peParent->GetDisplayNode(), &rcLVHost, 0xC);
            GTRANS_DESC transDesc[2];
            TransitionStoryboardInfo tsbInfo = {};
            for (int i = 0; i < cCount; i++)
            {
                if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                    continue;
                if (prcGadget[i].right > rcLVHost.left && prcGadget[i].bottom > rcLVHost.top &&
                    prcGadget[i].left < rcLVHost.right && prcGadget[i].top < rcLVHost.bottom)
                {
                    TriggerFade(ppe[i], transDesc, 0, 0.06f, 0.143f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                    TriggerScaleIn(ppe[i], transDesc, 1, 0.06f, 0.31f, 0.0f, 0.0f, 0.0f, 1.0f, 0.7f, 0.7f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                }
            }
            if (!(_flags & LVCF_ANIMATEPARTIAL))
            {
                CValuePtr v;
                DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
                if (rgList && rgList->GetSize() > 0 && ppe[0]->GetClassInfoW() == LVItem::GetClassInfoPtr())
                    _RemoveStuckClones(rgList);
                if (rgList && rgList->GetSize() > cCount && ppe[0]->GetClassInfoW() == LVItem::GetClassInfoPtr())
                {
                    *prcNext = prcGadget[0];
                    for (int i = 0; i < rgList->GetSize(); i++)
                    {
                        Element* child = rgList->GetItem(i);
                        if (child->GetLayoutPos() != -2 && child->GetClassInfoW() == LVItem::GetClassInfoPtr())
                        {
                            RECT rcItem;
                            GetGadgetRect(child->GetDisplayNode(), &rcItem, 0xC);
                            if (rcItem.top > prcGadget[iMaxIdx].top &&
                                prcNext->right > rcLVHost.left && prcNext->bottom > rcLVHost.top &&
                                prcNext->left < rcLVHost.right && prcNext->top < rcLVHost.bottom)
                            {
                                TriggerTranslate(child, transDesc, 0, 0.0f, 0.367f, 0.75f, 0.0f, 0.0f, 1.0f,
                                    prcGadget[0].left - prcNext[0].left, prcGadget[0].top - prcNext[0].top, 0, 0, false, false, true);
                                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, nullptr, &tsbInfo);
                            }
                        }
                    }
                }
            }
        }
    }

    HRESULT LVCommon::_OnRemoving(Element** ppe, Element** ppeClone, RECT* prcParent, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        GetGadgetRect(_peWhitespace->GetDisplayNode(), prcParent, 0xC);
        CValuePtr v;
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        if (rgList && rgList->GetSize() > 0)
        {
            _RemoveStuckClones(rgList);
            for (int i = 0; i < rgList->GetSize(); i++)
            {
                if (i < rgList->GetSize() - 1 && ppe[cCount - 1] == rgList->GetItem(i))
                    GetGadgetRect(rgList->GetItem(i + 1)->GetDisplayNode(), prcNext, 0xC);
            }
        }
        HRESULT hr = _CreateAnimatingClone(ppe, prcGadget, ppeClone, cCount);
        RECT rcParentOld{};
        GetGadgetRect(_peWhitespace->GetDisplayNode(), &rcParentOld, 0xC);
        for (int i = 0; i < cCount; i++)
        {
            prcGadget[i].top += (rcParentOld.top - prcParent->top);
            prcGadget[i].bottom += (rcParentOld.top - prcParent->top);
            prcNext[i].top += (rcParentOld.top - prcParent->top);
            prcNext[i].bottom += (rcParentOld.top - prcParent->top);
        }
        _ullRemoveTick = GetTickCount64();
        return hr;
    }

    void LVCommon::_OnRemove(Element** ppe, Element** ppeClone, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        if (!(_flags & LVCF_NOANIMATE))
        {
            UINT iMaxIdx = 0;
            RECT rcLVHost;
            Element* peParent = this;
            while (peParent->GetClassInfoW() != TouchScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != StyledScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != ScrollViewer::GetClassInfoPtr())
            {
                if (!peParent->GetParent())
                    break;
                peParent = peParent->GetParent();
            }
            GetGadgetRect(peParent->GetDisplayNode(), &rcLVHost, 0xC);
            GTRANS_DESC transDesc[3];
            TransitionStoryboardInfo tsbInfo = {};
            for (int i = 0; i < cCount; i++)
            {
                if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                    continue;
                if (prcGadget[i].right > rcLVHost.left && prcGadget[i].bottom > rcLVHost.top &&
                    prcGadget[i].left < rcLVHost.right && prcGadget[i].top < rcLVHost.bottom)
                {
                    TriggerFade(ppeClone[i], transDesc, 0, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                    TriggerScaleOut(ppeClone[i], transDesc, 1, 0.0f, 0.175f, 1.0f, 1.0f, 0.0f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, true);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, nullptr, &tsbInfo);
                }
                iMaxIdx = i;
            }
            DWORD animCoef = g_ctx.animCoef;
            if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            _dwSafeRemove = animCoef * 2;
            if (!(_flags & LVCF_ANIMATEPARTIAL))
            {
                CValuePtr v;
                DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
                if (rgList && rgList->GetSize() > 0)
                {
                    for (int i = rgList->GetSize() - 1; i >= 0; i--)
                    {
                        Element* child = rgList->GetItem(i);
                        if (child->GetLayoutPos() != -2 && child->GetClassInfoW() == LVItem::GetClassInfoPtr())
                        {
                            RECT rcItem;
                            GetGadgetRect(child->GetDisplayNode(), &rcItem, 0xC);
                            if (rcItem.top >= prcGadget[0].top &&
                                rcItem.right > rcLVHost.left && rcItem.bottom > rcLVHost.top &&
                                rcItem.left < rcLVHost.right && rcItem.top < rcLVHost.bottom)
                            {
                                TriggerTranslate(child, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                                    0, 0, 0, 0, false, false, true);
                                TriggerTranslate(child, transDesc, 1, 0.0f, 0.06f, 0.0f, 0.0f, 1.0f, 1.0f,
                                    prcNext->left - prcGadget[0].left, prcNext->top - prcGadget[0].top,
                                    prcNext->left - prcGadget[0].left, prcNext->top - prcGadget[0].top, false, false, true);
                                TriggerTranslate(child, transDesc, 2, 0.06f, 0.427f, 0.85f, 0.0f, 0.0f, 1.0f,
                                    prcNext->left - prcGadget[0].left, prcNext->top - prcGadget[0].top, 0, 0, false, false, true);
                                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                            }
                        }
                    }
                }
            }
        }
    }


    IClassInfo* LVGrid::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void LVGrid::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* LVGrid::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void LVGrid::OnInput(InputEvent* pInput)
    {
        BYTE ctrlKey, shiftKey;
        CValuePtr v;
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        if ((pInput->nCode == GMOUSE_MOVE || pInput->nCode == GMOUSE_DOWN) && pInput->nDevice == GINPUT_KEYBOARD && _keyState & 0x20)
        {
            short sLeft, sUp, sRight, sDown;
            sLeft = GetAsyncKeyState(VK_LEFT);
            sUp = GetAsyncKeyState(VK_UP);
            sRight = GetAsyncKeyState(VK_RIGHT);
            sDown = GetAsyncKeyState(VK_DOWN);
            ctrlKey = GetKeyState(VK_CONTROL);
            shiftKey = GetKeyState(VK_SHIFT);
            _keyState |= ((BYTE)((sLeft & 0x8000) && (sLeft & 1))) + ((BYTE)((sUp & 0x8000) && (sUp & 1)) << 1) +
                ((BYTE)((sRight & 0x8000) && (sRight & 1)) << 2) + ((BYTE)((sDown & 0x8000) && (sDown & 1)) << 3);
            if (pInput->nCode == GMOUSE_DOWN)
                _keyState &= 0xEF;

            if (!(_keyState & 0x10) || _keyState != _keyStateOld)
            {
                if (_keyState & 0xF)
                {
                    if (!_peFocused || !_peFocused->GetVisible())
                    {
                        int yBase = _rgYPos[0];
                        int indexY = 0;
                        LVItem* toSelect = _rgYItems[0];
                        while (_rgYPos[indexY] == yBase)
                        {
                            if (((g_ctx.localeType != 1 && _rgYItems[indexY]->GetX() < toSelect->GetX()) ||
                                (g_ctx.localeType == 1 && _rgYItems[indexY]->GetX() > toSelect->GetX())) && _rgYItems[indexY]->GetVisible())
                                toSelect = _rgYItems[indexY];
                            indexY++;
                        }
                        toSelect->SetSelected(true);
                        toSelect->SetKeyFocus();
                        _peSelected = toSelect;
                        _peFocused = toSelect;
                        if (!(shiftKey & 0x80) || !_pePivot)
                            _pePivot = _peSelected;
                        LVCommon::OnInput(pInput);
                        return;
                    }
                    RECT rcFocused{}, rcToBeFocused{};
                    GetGadgetRect(_peFocused->GetDisplayNode(), &rcFocused, 0xC);
                    int indexX = _SearchArrayExact(&_rgXPos, rcFocused.left);
                    int indexY = _SearchArrayExact(&_rgYPos, rcFocused.top);
                    bool foundItem = true;
                    bool animatebound = (_peFocused != _peFocusedOld || _keyState != _keyStateOld);
                    if (_keyState & 0x5)
                    {
                        while (_rgXPos[indexX] == rcFocused.left)
                        {
                            if (_keyState & 0x1)
                                indexX--;
                            else if (_keyState & 0x4)
                                indexX++;
                            if (indexX < 0 || indexX >= _rgXItems.size())
                            {
                                foundItem = false;
                                break;
                            }
                        }
                        if (foundItem)
                        {
                            GetGadgetRect(_rgXItems[indexX]->GetDisplayNode(), &rcToBeFocused, 0xC);
                            RECT* rcLong = (rcFocused.bottom - rcFocused.top > rcToBeFocused.bottom - rcToBeFocused.top) ? &rcFocused : &rcToBeFocused;
                            RECT* rcShort = (rcLong == &rcToBeFocused) ? &rcFocused : &rcToBeFocused;
                            int halfHeight = (rcShort->bottom - rcShort->top) / 2;
                            while (rcShort->top < rcLong->top - halfHeight || rcShort->bottom > rcLong->bottom + halfHeight ||
                                !_rgXItems[indexX]->GetVisible() || _rgXItems[indexX] == _peFocused)
                            {
                                if (_keyState & 0x1)
                                    indexX--;
                                else if (_keyState & 0x4)
                                    indexX++;
                                if (indexX < 0 || indexX >= _rgXItems.size())
                                {
                                    foundItem = false;
                                    break;
                                }
                                if (!_rgXItems[indexX]->GetVisible())
                                    continue;
                                GetGadgetRect(_rgXItems[indexX]->GetDisplayNode(), &rcToBeFocused, 0xC);
                                rcLong = (rcFocused.bottom - rcFocused.top > rcToBeFocused.bottom - rcToBeFocused.top) ? &rcFocused : &rcToBeFocused;
                                rcShort = (rcLong == &rcToBeFocused) ? &rcFocused : &rcToBeFocused;
                                halfHeight = (rcShort->bottom - rcShort->top) / 2;
                            }
                            if (foundItem && _rgXItems[indexX]->GetVisible())
                            {
                                _peFocusedOld = _peFocused;
                                if (!(ctrlKey & 0x80))
                                {
                                    _peSelected = _rgXItems[indexX];
                                    _peSelected->SetSelected(true);
                                }
                                _peFocused = _rgXItems[indexX];
                                _peFocused->SetKeyFocus();
                            }
                            else if (animatebound)
                                goto ELEMNOTFOUND;
                        }
                        else if (animatebound)
                            goto ELEMNOTFOUND;
                    }
                    else if (_keyState & 0xA)
                    {
                        while (_rgYPos[indexY] == rcFocused.top)
                        {
                            if (_keyState & 0x2)
                                indexY--;
                            else if (_keyState & 0x8)
                                indexY++;
                            if (indexY < 0 || indexY >= _rgYItems.size())
                            {
                                foundItem = false;
                                break;
                            }
                        }
                        if (foundItem)
                        {
                            GetGadgetRect(_rgYItems[indexY]->GetDisplayNode(), &rcToBeFocused, 0xC);
                            RECT* rcWide = (rcFocused.right - rcFocused.left > rcToBeFocused.right - rcToBeFocused.left) ? &rcFocused : &rcToBeFocused;
                            RECT* rcNarrow = (rcWide == &rcToBeFocused) ? &rcFocused : &rcToBeFocused;
                            int halfWidth = (rcNarrow->right - rcNarrow->left) / 2;
                            while (rcNarrow->left < rcWide->left - halfWidth || rcNarrow->right > rcWide->right + halfWidth ||
                                !_rgYItems[indexY]->GetVisible() || _rgYItems[indexY] == _peFocused)
                            {
                                if (_keyState & 0x2)
                                    indexY--;
                                else if (_keyState & 0x8)
                                    indexY++;
                                if (indexY < 0 || indexY >= _rgYItems.size())
                                {
                                    foundItem = false;
                                    break;
                                }
                                if (!_rgYItems[indexY]->GetVisible())
                                    continue;
                                GetGadgetRect(_rgYItems[indexY]->GetDisplayNode(), &rcToBeFocused, 0xC);
                                rcWide = (rcFocused.right - rcFocused.left > rcToBeFocused.right - rcToBeFocused.left) ? &rcFocused : &rcToBeFocused;
                                rcNarrow = (rcWide == &rcToBeFocused) ? &rcFocused : &rcToBeFocused;
                                halfWidth = (rcNarrow->right - rcNarrow->left) / 2;
                            }
                            if (foundItem && _rgYItems[indexY]->GetVisible())
                            {
                                _peFocusedOld = _peFocused;
                                if (!(ctrlKey & 0x80))
                                {
                                    _peSelected = _rgYItems[indexY];
                                    _peSelected->SetSelected(true);
                                }
                                _peFocused = _rgYItems[indexY];
                                _peFocused->SetKeyFocus();
                            }
                            else if (animatebound)
                                goto ELEMNOTFOUND;
                        }
                        else if (animatebound)
                            goto ELEMNOTFOUND;
                    }
                    if (rgList && !((ctrlKey & 0x80) || (shiftKey & 0x80)))
                    {
                        for (int items = 0; items < rgList->GetSize(); items++)
                        {
                            if (rgList->GetItem(items)->GetSelected())
                                rgList->GetItem(items)->SetSelected(false);
                        }
                    }
                    if (shiftKey & 0x80 && _pePivot)
                    {
                        _ShiftSelectionHelper(&rcFocused, &rcToBeFocused);
                    }
                    else if (!(ctrlKey & 0x80)) _pePivot = _peSelected;
                    _keyStateOld = _keyState;
                }
                else if (_peAnimating && _peSelected != _peAnimating)
                {
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    TriggerScaleOut(_peAnimating, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, _peAnimating->GetDisplayNode(), &tsbInfo);
                    if (pInput->nCode == GMOUSE_DOWN && !(_keyState & 0xF) && _keyStateOld != 0 && _peFocused == _peFocusedOld)
                        goto ELEMNOTFOUND;
                }
                else if (pInput->nCode == GMOUSE_DOWN && !(_keyState & 0xF) && _keyStateOld != 0 && _peFocused == _peFocusedOld)
                    goto ELEMNOTFOUND;
            }
        }
        else if (false)
        {
        ELEMNOTFOUND:
            if (!(ctrlKey & 0x80) && _peSelected)
            {
                if (_peAnimating && _peSelected != _peAnimating)
                {
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    TriggerScaleOut(_peAnimating, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, _peAnimating->GetDisplayNode(), &tsbInfo);
                }
                float flOriginX, flOriginY, flScaleX, flScaleY;
                if (_keyState & 0x5)
                {
                    flScaleX = 1.071f;
                    flScaleY = 0.933f;
                    if (_keyState & 0x1) flOriginX = 2.0f;
                    else if (_keyState & 0x5) flOriginX = -1.0f;
                    flOriginY = 0.5f;
                }
                if (_keyState & 0xA)
                {
                    flScaleX = 0.933f;
                    flScaleY = 1.071f;
                    flOriginX = 0.5f;
                    if (_keyState & 0x2) flOriginY = 2.0f;
                    else if (_keyState & 0x8) flOriginY = -1.0f;
                }
                if (_peFocused != _peSelected)
                {
                    _peSelected->SetSelected(false);
                    _peSelected = _peFocused;
                    if (!(shiftKey & 0x80) || !_pePivot)
                        _pePivot = _peSelected;
                }
                _peSelected->SetSelected(true);
                if (pInput->nCode == GMOUSE_MOVE)
                {
                    KillTimer(_hWorker, 1);
                    _peAnimating = _peSelected;
                    _keyState |= 0x10;
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    _ullOverflowTick = GetTickCount64();
                    TriggerScaleIn(_peSelected, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f,
                        flOriginX, flOriginY, flScaleX, flScaleY, flOriginX, flOriginY, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, _peSelected->GetDisplayNode(), &tsbInfo);
                    DUI_SetGadgetZOrder(_peSelected, -1);
                    _keyStateOld2 = _keyState;
                }
                else if (pInput->nCode == GMOUSE_DOWN)
                {
                    DWORD animCoef = g_ctx.animCoef;
                    if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                    DWORD dwTickDelay = 3 * animCoef - (GetTickCount64() - _ullOverflowTick);
                    if (dwTickDelay > 0x7FFFFFFF) dwTickDelay = 0;
                    SetWindowLongPtrW(_hWorker, GWLP_USERDATA, (LONG_PTR)this);
                    KillTimer(_hWorker, 1);
                    SetTimer(_hWorker, 1, dwTickDelay, nullptr);
                }
                _peFocusedOld = _peFocused;
                _keyStateOld = _keyState;
            }
        }
        _keyState &= 0xF0;
        LVCommon::OnInput(pInput);
    }

    void LVGrid::OnKeyFocusMoved(Element* peFrom, Element* peTo)
    {
        CValuePtr v;
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        BYTE ctrlKey = GetKeyState(VK_CONTROL);
        BYTE shiftKey = GetKeyState(VK_SHIFT);
        if (peTo)
        {
            if (peTo->GetClassInfoW() == LVItem::GetClassInfoPtr() || peTo == _peWhitespace)
                _keyState |= 0x20;
            else
                _keyState &= 0xDF;
            bool fKeyboard = (!(GetKeyState(VK_LBUTTON) & 0x80 || GetKeyState(VK_RBUTTON) & 0x80));
            // Handling checkboxes in LVItems (requires setting the LVItem's checkbox first)
            if (peTo->GetClassInfoW() != LVItem::GetClassInfoPtr())
            {
                Element* peParent = peTo;
                while (peParent->GetParent())
                {
                    peParent = peParent->GetParent();
                    if (peParent->GetClassInfoW() == LVItem::GetClassInfoPtr())
                        break;
                }
                if (peParent->GetClassInfoW() == LVItem::GetClassInfoPtr() && peTo == ((LVItem*)peParent)->GetCheckbox())
                {
                    _keyState |= 0x20;
                    peTo = peParent;
                }
            }
            ////////////////////////////////////////////////////////////////////////////////
            if (peTo->GetClassInfoW() == LVItem::GetClassInfoPtr() && peTo->GetParent() == _peWhitespace)
            {
                if (peFrom && !(ctrlKey & 0x80))
                {
                    _peSelected = peTo;
                    if (!(shiftKey & 0x80) || !_pePivot)
                        _pePivot = peTo;
                    else if (!fKeyboard) // 0.6 M4: incomplete
                    {
                        if (rgList)
                        {
                            for (int items = 0; items < rgList->GetSize(); items++)
                            {
                                if (rgList->GetItem(items) != _pePivot && rgList->GetItem(items)->GetSelected())
                                    rgList->GetItem(items)->SetSelected(false);
                            }
                        }
                        RECT rcPivot{}, rcTo{};
                        GetGadgetRect(_pePivot->GetDisplayNode(), &rcPivot, 0xC);
                        GetGadgetRect(peTo->GetDisplayNode(), &rcTo, 0xC);
                        _ShiftSelectionHelper(&rcPivot, &rcTo);
                    }
                    if (fKeyboard)
                        _peSelected->SetSelected(true);
                }
                _peFocused = peTo;
            }
        }

        // 0.5.8: TODO: Remove this part once a better EventListener is implemented
        // because we do not want this desktop-specific behavior to apply to EVERY LVGrid
        WCHAR elementinfo[160], className[64];
        HWND hwndForeground = GetForegroundWindow();
        DWORD threadId = GetWindowThreadProcessId(hwndForeground, NULL);
        GUITHREADINFO gui;
        ZeroMemory(&gui, sizeof(gui));
        gui.cbSize = sizeof(GUITHREADINFO);
        if (GetGUIThreadInfo(threadId, &gui))
        {
            if (gui.hwndFocus)
                hwndForeground = gui.hwndFocus;
        }
        HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
        HWND hTrayNotify = FindWindowExW(hTaskbar, nullptr, L"TrayNotifyWnd", nullptr);
        HWND hShowDesktop = FindWindowExW(hTrayNotify, nullptr, L"TrayShowDesktopButtonWClass", nullptr);
        RECT rcShow{};
        GetWindowRect(hShowDesktop, &rcShow);
        POINT pt;
        GetCursorPos(&pt);

        //if (!peTo && (hwndForeground == g_hSHELLDLL_DefView ||
        //    pt.x >= rcShow.left && pt.x <= rcShow.right && pt.y >= rcShow.top && pt.y <= rcShow.bottom))
        //{
        //    SetFocus(wnd->GetHWND());
        //    peTo = _peWhitespace;
        //}
        
        //GetClassNameW(hwndForeground, className, 64);
        //StringCchPrintfW(elementinfo, 160, L"peFrom: %x\npeTo: %x\npeSelected: %x\nActive window: %s",
        //    peFrom, peTo, _peSelected, className);
        //DDNotificationBanner* ddnb = new DDNotificationBanner();
        //ddnb->CreateBanner(DDNT_INFO, nullptr, elementinfo, 8);
        ///////////////////////////////////////////////////////////////////////////

        if (!(shiftKey & 0x80) && _pePivot != _peSelected)
            _pePivot = _peSelected;
        Element::OnKeyFocusMoved(peFrom, peTo);
    }

    bool LVGrid::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = LVCommon::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::HeightProp))
            result = true;
        return result;
    }

    HRESULT LVGrid::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<LVGrid, int>(0, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT LVGrid::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((Element*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateLVVisual();
        HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
        if (hShlwapi)
        {
            pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
            _hWorker = SHCreateWorkerWindowW(s_LVGridProc, HWND_MESSAGE, 0, 0, nullptr);
        }
        return hr;
    }

    HRESULT LVGrid::Register()
    {
        return ClassInfo<LVGrid, LVCommon>::RegisterGlobal(HINST_THISCOMPONENT, L"LVGrid", nullptr, 0);
    }

    void LVGrid::NotifyGridChanges(LVItem** rgLVItems, POINT* rgPosOld, POINT* rgPosNew, int cSize)
    {
        if (cSize == -1)
        {
            CValuePtr v;
            DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
            if (rgList && rgList->GetSize() > 0)
            {
                _rgXPos.clear();
                _rgYPos.clear();
                _rgXItems.clear();
                _rgYItems.clear();
                RECT rcGadget;
                POINT ptIndex;
                for (int i = 0; i < rgList->GetSize(); i++)
                {
                    if (rgList->GetItem(i)->GetClassInfoW() != LVItem::GetClassInfoPtr())
                        continue;
                    GetGadgetRect(rgList->GetItem(i)->GetDisplayNode(), &rcGadget, 0xC);
                    ptIndex.x = _SearchArray(&_rgXPos, rcGadget.left);
                    ptIndex.y = _SearchArray(&_rgYPos, rcGadget.top);
                    _rgXPos.insert(_rgXPos.begin() + ptIndex.x, rcGadget.left);
                    _rgYPos.insert(_rgYPos.begin() + ptIndex.y, rcGadget.top);
                    _rgXItems.insert(_rgXItems.begin() + ptIndex.x, (LVItem*)rgList->GetItem(i));
                    _rgYItems.insert(_rgYItems.begin() + ptIndex.y, (LVItem*)rgList->GetItem(i));
                }
            }
        }
        else if (cSize > 0)
        {
            POINT ptIndex, ptPivot;
            for (int i = 0; i < cSize; i++)
            {
                if (rgLVItems[i])
                {
                    ptIndex.x = _SearchArrayExact(&_rgXPos, rgPosOld[i].x);
                    ptIndex.y = _SearchArrayExact(&_rgYPos, rgPosOld[i].y);
                    ptPivot = ptIndex;
                    bool reverseX = false, reverseY = false;
                    RECT rcSearch{};
                    wstring filename = rgLVItems[i]->GetFilename();
                    while (_rgXItems[ptIndex.x]->GetFilename() != filename)
                    {
                        if (reverseX) ptIndex.x--;
                        else ptIndex.x++;
                        if (ptIndex.x >= 0 && ptIndex.x < _rgXPos.size())
                            GetGadgetRect(_rgXItems[ptIndex.x]->GetDisplayNode(), &rcSearch, 0xC);
                        if (rcSearch.left > rgPosOld[i].x || ptIndex.x >= _rgXPos.size())
                        {
                            if (!reverseX)
                                ptIndex.x = ptPivot.x - 1;
                            reverseX = true;
                        }
                        if (rcSearch.left < rgPosOld[i].x || ptIndex.x < 0)
                            break;
                    }
                    while (_rgYItems[ptIndex.y]->GetFilename() != filename)
                    {
                        if (reverseY) ptIndex.y--;
                        else ptIndex.y++;
                        if (ptIndex.y >= 0 && ptIndex.y < _rgYPos.size())
                            GetGadgetRect(_rgYItems[ptIndex.y]->GetDisplayNode(), &rcSearch, 0xC);
                        if (rcSearch.top > rgPosOld[i].y || ptIndex.y >= _rgYPos.size())
                        {
                            if (!reverseY)
                                ptIndex.y = ptPivot.y - 1;
                            reverseY = true;
                        }
                        if (rcSearch.top < rgPosOld[i].y || ptIndex.y < 0)
                            break;
                    }
                    _rgXPos.erase(_rgXPos.begin() + ptIndex.x);
                    _rgYPos.erase(_rgYPos.begin() + ptIndex.y);
                    _rgXItems.erase(_rgXItems.begin() + ptIndex.x);
                    _rgYItems.erase(_rgYItems.begin() + ptIndex.y);
                }
            }
            for (int i = 0; i < cSize; i++)
            {
                ptIndex.x = _SearchArray(&_rgXPos, rgPosNew[i].x);
                ptIndex.y = _SearchArray(&_rgYPos, rgPosNew[i].y);
                _rgXPos.insert(_rgXPos.begin() + ptIndex.x, rgPosNew[i].x);
                _rgYPos.insert(_rgYPos.begin() + ptIndex.y, rgPosNew[i].y);
                _rgXItems.insert(_rgXItems.begin() + ptIndex.x, rgLVItems[i]);
                _rgYItems.insert(_rgYItems.begin() + ptIndex.y, rgLVItems[i]);
            }
        }
    }

    int LVGrid::_SearchArray(vector<int>* rgPos, int iCloseValue)
    {
        int index = 0;
        int size = rgPos->size();
        int step = size / 2;
        if (step == 0) step = 1;
        while (index < size && (*rgPos)[index] != iCloseValue)
        {
            while (index < size && (*rgPos)[index] < iCloseValue)
            {
                index += step;
                if (step > 1) step /= 2;
            }
            if (index >= size)
            {
                index = size;
                break;
            }
            if (index > 0 && (*rgPos)[index - 1] <= iCloseValue)
                break;
            while (index > 0 && (*rgPos)[index] > iCloseValue)
            {
                index -= step;
                if (step > 1) step /= 2;
            }
            if (index <= 0)
            {
                index = 0;
                break;
            }
            if (index < size - 1 && (*rgPos)[index + 1] >= iCloseValue)
                break;
        }
        if (index < size && (*rgPos)[index] < iCloseValue)
            index++;
        return index;
    }

    int LVGrid::_SearchArrayExact(vector<int>* rgPos, int iTargetValue)
    {
        int index = 0;
        int size = rgPos->size();
        int step = size / 2;
        if (step == 0) step = 1;
        unsigned short runs = 0; // Preventive measure for launch/refresh animation, which causes no exact match and an infinite loop
        int prev = 0;
        int prevprev = 0;
        while (index < size && (*rgPos)[index] != iTargetValue && runs < 0xFFF0)
        {
            bool nomatch = false;
            while (index < size && (*rgPos)[index] < iTargetValue && runs < 0xFFF0)
            {
                runs++;
                index += step;
                if (step > 1) step /= 2;
                else
                {
                    if (index == prevprev)
                    {
                        nomatch = true;
                        break;
                    }
                    prevprev = prev;
                    prev = index;
                }
            }
            while (index > 0 && (*rgPos)[index] > iTargetValue && runs < 0xFFF0)
            {
                runs++;
                index -= step;
                if (step > 1) step /= 2;
                else
                {
                    if (index == prevprev)
                    {
                        nomatch = true;
                        break;
                    }
                    prevprev = prev;
                    prev = index;
                }
            }
            if (nomatch || (index == 0 && (*rgPos)[0] > iTargetValue)) break;
            runs++;
        }
        if (index < size && (*rgPos)[index] == iTargetValue) return index;
        return 0;
    }

    void LVGrid::_ShiftSelectionHelper(RECT* prcFrom, RECT* prcTo)
    {
        int indexY;
        RECT rcPivot{};
        GetGadgetRect(_pePivot->GetDisplayNode(), &rcPivot, 0xC);
        RECT* prcPivot = &rcPivot;
        RECT* rcLong = (prcPivot->bottom - prcPivot->top > prcTo->bottom - prcTo->top) ? prcPivot : prcTo;
        RECT* rcShort = (rcLong == prcTo) ? prcPivot : prcTo;
        int halfHeight = (rcShort->bottom - rcShort->top) / 2;
        int start, end;
        int leftOneRow = min(prcFrom->left, prcTo->left);
        int rightOneRow = max(prcFrom->right, prcTo->right);
        bool selectmode{};
        DWORD flags{};
        if (prcFrom->top - halfHeight <= prcTo->top && prcFrom->bottom + halfHeight >= prcTo->bottom)
        {
            bool fRelative = (g_ctx.localeType != 1 && prcFrom->left <= prcTo->left) || (g_ctx.localeType == 1 && prcFrom->right >= prcTo->right);
            bool fPivot = !(prcPivot->top - halfHeight <= prcTo->top && (!(prcPivot->bottom + halfHeight >= prcTo->bottom) || 
                (g_ctx.localeType != 1 && prcPivot->left <= prcFrom->left) || (g_ctx.localeType == 1 && prcPivot->right >= prcFrom->right)));
            selectmode = fRelative ^ fPivot;
            flags |= 0x10;
            start = (g_ctx.localeType == 1) ? rightOneRow : leftOneRow;
            end = (g_ctx.localeType == 1) ? leftOneRow : rightOneRow;
        }
        else if (prcPivot->top - halfHeight <= prcTo->top)
        {
            flags |= 0x1;
            if (prcFrom->top - halfHeight <= prcTo->top)
            {
                selectmode = true;
                flags |= 0x2;
            }
            if (prcPivot->bottom + halfHeight >= prcTo->bottom)
            {
                selectmode = false;
                flags |= 0x4;
            }
            start = (g_ctx.localeType == 1) ? prcFrom->right : prcFrom->left;
            end = (g_ctx.localeType == 1) ? prcTo->left : prcTo->right;
        }
        else
        {
            if (prcPivot->top - halfHeight <= prcTo->top && prcPivot->bottom + halfHeight >= prcTo->bottom)
                ;
            else if (prcFrom->top - halfHeight >= prcTo->top)
            {
                selectmode = true;  
                flags |= 0x8;
            }
            start = (g_ctx.localeType == 1) ? prcTo->right : prcTo->left;
            end = (g_ctx.localeType == 1) ? prcFrom->left : prcFrom->right;
        }
        int idxStart = min(prcFrom->top, prcTo->top);
        indexY = _SearchArrayExact(&_rgYPos, idxStart);
        int top = min(prcFrom->top, prcTo->top);
        int bottom = max(prcFrom->top, prcTo->top);
        while (indexY >= 0 && _rgYPos[indexY] >= top - halfHeight)
            indexY--;
        indexY++;
        RECT rcCurrent{};
        while (indexY < _rgYPos.size() && _rgYPos[indexY] <= bottom + halfHeight)
        {
            if (_rgYItems[indexY] != _pePivot)
            {
                bool cond{};
                if (_rgYPos[indexY] <= top + halfHeight)
                {
                    GetGadgetRect(_rgYItems[indexY]->GetDisplayNode(), &rcCurrent, 0xC);
                    if (flags & 0xE && prcPivot->top - halfHeight <= rcCurrent.top && prcPivot->bottom + halfHeight >= rcCurrent.bottom)
                        cond = (g_ctx.localeType != 1 && rcCurrent.right < rcPivot.right) || (g_ctx.localeType == 1 && rcCurrent.left > rcPivot.left);
                    if (_rgYPos[indexY] >= bottom - halfHeight)
                    {
                        if (prcFrom->left == prcPivot->left && prcFrom->top == prcPivot->top &&
                            prcFrom->right == prcPivot->right && prcFrom->bottom == prcPivot->bottom)
                            cond = (g_ctx.localeType != 1 && rcCurrent.right < rcPivot.right) || (g_ctx.localeType == 1 && rcCurrent.left > rcPivot.left);
                        if ((g_ctx.localeType != 1 && (rcCurrent.left >= start && rcCurrent.right <= end)) ||
                            (g_ctx.localeType == 1 && (rcCurrent.right <= start && rcCurrent.left >= end)))
                            _rgYItems[indexY]->SetSelected(selectmode ^ cond);
                    }
                    else if ((g_ctx.localeType != 1 && rcCurrent.left >= start) || (g_ctx.localeType == 1 && rcCurrent.right <= start))
                        _rgYItems[indexY]->SetSelected(selectmode^ cond);
                }
                else if (_rgYPos[indexY] >= bottom - halfHeight)
                {
                    GetGadgetRect(_rgYItems[indexY]->GetDisplayNode(), &rcCurrent, 0xC);
                    if (flags & 0xE && prcPivot->top - halfHeight <= rcCurrent.top && prcPivot->bottom + halfHeight >= rcCurrent.bottom)
                        cond = (g_ctx.localeType != 1 && rcCurrent.right > rcPivot.right) || (g_ctx.localeType == 1 && rcCurrent.left < rcPivot.left);
                    if ((g_ctx.localeType != 1 && rcCurrent.right <= end) || (g_ctx.localeType == 1 && rcCurrent.left >= end))
                        _rgYItems[indexY]->SetSelected(selectmode ^ cond);
                }
                else
                    _rgYItems[indexY]->SetSelected(selectmode);
            }
            indexY++;
        }
    }

    LRESULT CALLBACK LVGrid::s_LVGridProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LVGrid* lvg = (LVGrid*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        switch (uMsg)
        {
        case WM_TIMER:
        {
        case 1:
            if (lvg->_peAnimating)
            {
                float flOriginX{}, flOriginY{}, flScaleX = 1.0f, flScaleY = 1.0f;
                if (lvg->_keyStateOld2 & 0x5)
                {
                    flScaleX = 1.071f;
                    flScaleY = 0.933f;
                    if (lvg->_keyStateOld2 & 0x1) flOriginX = 2.0f;
                    else if (lvg->_keyStateOld2 & 0x5) flOriginX = -1.0f;
                    flOriginY = 0.5f;
                }
                if (lvg->_keyStateOld2 & 0xA)
                {
                    flScaleX = 0.933f;
                    flScaleY = 1.071f;
                    flOriginX = 0.5f;
                    if (lvg->_keyStateOld2 & 0x2) flOriginY = 2.0f;
                    else if (lvg->_keyStateOld2 & 0x8) flOriginY = -1.0f;
                }
                GTRANS_DESC transDesc[1];
                TransitionStoryboardInfo tsbInfo = {};
                TriggerScaleIn(lvg->_peAnimating, transDesc, 0, 0.0f, 0.2f, 0.11f, 0.6f, 0.23f, 0.97f,
                    flScaleX, flScaleY, flOriginX, flOriginY, 1.0f, 1.0f, flOriginX, flOriginY, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, lvg->_peAnimating->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(lvg->_peAnimating, -1);
            }
            lvg->_keyStateOld2 = 0;
            lvg->_peAnimating = nullptr;
            KillTimer(hWnd, wParam);
            break;
        }
        default:
            break;
        }
        return s_ListViewProc(hWnd, uMsg, wParam, lParam);
    }

    void LVGrid::_OnAddOrInsert(Element** ppe, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        GTRANS_DESC transDesc[2];
        TransitionStoryboardInfo tsbInfo = {};
        RECT rcLVHost;
        Element* peParent = this;
        while (peParent->GetClassInfoW() != TouchScrollViewer::GetClassInfoPtr() &&
            peParent->GetClassInfoW() != StyledScrollViewer::GetClassInfoPtr() &&
            peParent->GetClassInfoW() != ScrollViewer::GetClassInfoPtr())
        {
            if (!peParent->GetParent())
                break;
            peParent = peParent->GetParent();
        }
        GetGadgetRect(peParent->GetDisplayNode(), &rcLVHost, 0xC);
        for (int i = 0; i < cCount; i++)
        {
            if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                continue;
            ((LVItem*)ppe[i])->SetShowKeyFocus(false);
            POINT ptIndex;
            ptIndex.x = _SearchArray(&_rgXPos, prcGadget[i].left);
            ptIndex.y = _SearchArray(&_rgYPos, prcGadget[i].top);
            _rgXPos.insert(_rgXPos.begin() + ptIndex.x, prcGadget[i].left);
            _rgYPos.insert(_rgYPos.begin() + ptIndex.y, prcGadget[i].top);
            _rgXItems.insert(_rgXItems.begin() + ptIndex.x, (LVItem*)ppe[i]);
            _rgYItems.insert(_rgYItems.begin() + ptIndex.y, (LVItem*)ppe[i]);
            if (!(_flags & LVCF_NOASSIGNFUNC))
            {
                assignFn(ppe[i], SelectItemBase);
                assignExtendedFn(ppe[i], RefineSelections);
                if (((LVItem*)ppe[i])->GetCheckbox())
                    assignExtendedFn(((LVItem*)ppe[i])->GetCheckbox(), CheckboxHandler);
            }
            if (!(_flags & LVCF_NOANIMATE))
            {
                if (prcGadget[i].right > rcLVHost.left && prcGadget[i].bottom > rcLVHost.top &&
                    prcGadget[i].left < rcLVHost.right && prcGadget[i].top < rcLVHost.bottom)
                {
                    TriggerFade(ppe[i], transDesc, 0, 0.06f, 0.143f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                    TriggerScaleIn(ppe[i], transDesc, 1, 0.06f, 0.31f, 0.0f, 0.0f, 0.0f, 1.0f, 0.7f, 0.7f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                }
            }
        }
        CValuePtr v;
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        if (rgList && rgList->GetSize() > 0 && ppe[0]->GetClassInfoW() == LVItem::GetClassInfoPtr())
            _RemoveStuckClones(rgList);
    }

    HRESULT LVGrid::_OnRemoving(Element** ppe, Element** ppeClone, RECT* prcParent, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        CValuePtr v;
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        _RemoveStuckClones(rgList);
        _ullRemoveTick = GetTickCount64();
        return _CreateAnimatingClone(ppe, prcGadget, ppeClone, cCount);
    }

    void LVGrid::_OnRemove(Element** ppe, Element** ppeClone, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        GTRANS_DESC transDesc[2];
        TransitionStoryboardInfo tsbInfo = {};
        RECT rcLVHost;
        Element* peParent = this;
        while (peParent->GetClassInfoW() != TouchScrollViewer::GetClassInfoPtr() &&
            peParent->GetClassInfoW() != StyledScrollViewer::GetClassInfoPtr() &&
            peParent->GetClassInfoW() != ScrollViewer::GetClassInfoPtr())
        {
            if (!peParent->GetParent())
                break;
            peParent = peParent->GetParent();
        }
        GetGadgetRect(peParent->GetDisplayNode(), &rcLVHost, 0xC);
        for (int i = 0; i < cCount; i++)
        {
            if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                continue;
            POINT ptIndex, ptPivot;
            ptIndex.x = _SearchArrayExact(&_rgXPos, prcGadget[i].left);
            ptIndex.y = _SearchArrayExact(&_rgYPos, prcGadget[i].top);
            ptPivot = ptIndex;
            bool reverseX = false, reverseY = false;
            RECT rcSearch{};
            if (ptIndex.x > _rgXItems.size() - 1)
                ptIndex.x = _rgXItems.size() - 1;
            if (ptIndex.y > _rgYItems.size() - 1)
                ptIndex.y = _rgYItems.size() - 1;
            while (_rgXItems[ptIndex.x]->GetFilename() != ((LVItem*)ppe[i])->GetFilename())
            {
                if (reverseX) ptIndex.x--;
                else ptIndex.x++;
                if (ptIndex.x < 0 || ptIndex.x >= _rgXItems.size())
                    break;
                GetGadgetRect(_rgXItems[ptIndex.x]->GetDisplayNode(), &rcSearch, 0xC);
                if (rcSearch.left > prcGadget[i].left)
                {
                    reverseX = true;
                    ptIndex.x = ptPivot.x - 1;
                    if (ptIndex.x < 0)
                        break;
                }
                if (rcSearch.left < prcGadget[i].left)
                    break;
            }
            while (_rgYItems[ptIndex.y]->GetFilename() != ((LVItem*)ppe[i])->GetFilename())
            {
                if (reverseY) ptIndex.y--;
                else ptIndex.y++;
                if (ptIndex.y < 0 || ptIndex.y >= _rgYItems.size())
                    break;
                GetGadgetRect(_rgYItems[ptIndex.y]->GetDisplayNode(), &rcSearch, 0xC);
                if (rcSearch.top > prcGadget[i].top)
                {
                    reverseY = true;
                    ptIndex.y = ptPivot.y - 1;
                    if (ptIndex.y < 0)
                        break;
                }
                if (rcSearch.top < prcGadget[i].top)
                    break;
            }
            if (ppe[i] == _pePivot)
                _pePivot = nullptr;
            if (ptIndex.x > _rgXItems.size() - 1)
                ptIndex.x = _rgXItems.size() - 1;
            if (ptIndex.y > _rgYItems.size() - 1)
                ptIndex.y = _rgYItems.size() - 1;
            _rgXPos.erase(_rgXPos.begin() + ptIndex.x);
            _rgYPos.erase(_rgYPos.begin() + ptIndex.y);
            _rgXItems.erase(_rgXItems.begin() + ptIndex.x);
            _rgYItems.erase(_rgYItems.begin() + ptIndex.y);
            if (!(_flags & LVCF_NOANIMATE))
            {
                if (prcGadget[i].right > rcLVHost.left && prcGadget[i].bottom > rcLVHost.top &&
                    prcGadget[i].left < rcLVHost.right && prcGadget[i].top < rcLVHost.bottom)
                {
                    TriggerFade(ppeClone[i], transDesc, 0, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                    TriggerScaleOut(ppeClone[i], transDesc, 1, 0.0f, 0.175f, 1.0f, 1.0f, 0.0f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, true);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                }
            }
        }
        DWORD animCoef = g_ctx.animCoef;
        if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        _dwSafeRemove = animCoef * 2;
    }

    void LVGrid::_OnRemoveAll()
    {
        _peFocusedOld = nullptr;
        _pePivot = nullptr;
        _peAnimating = nullptr;
        _rgXPos.clear();
        _rgYPos.clear();
        _rgXItems.clear();
        _rgYItems.clear();
    }

    IClassInfo* LVTiles::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void LVTiles::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* LVTiles::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void LVTiles::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, LVTiles::ItemMinWidthProp) || PropNotify::IsEqual(ppi, iIndex, LVTiles::ItemHeightProp))
            this->_UpdateGridLayoutParams();
        LVCommon::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT LVTiles::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<LVTiles, int>(0, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT LVTiles::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((Element*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateLVVisual();
        HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
        if (hShlwapi)
        {
            pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
            _hWorker = SHCreateWorkerWindowW(s_ListViewProc, HWND_MESSAGE, 0, 0, nullptr);
        }
        return hr;
    }

    HRESULT LVTiles::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impItemMinWidthProp,
            &impItemHeightProp
        };
        return ClassInfo<LVTiles, LVCommon>::RegisterGlobal(HINST_THISCOMPONENT, L"LVTiles", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    const PropertyInfo* WINAPI LVTiles::ItemMinWidthProp()
    {
        return &impItemMinWidthProp;
    }

    int LVTiles::GetItemMinWidth()
    {
        int v = this->GetPropCommon(ItemMinWidthProp);
        if (v < 1) v = 1;
        return v;
    }

    void LVTiles::SetItemMinWidth(int iFirstImage)
    {
        this->SetPropCommon(ItemMinWidthProp, iFirstImage);
    }

    const PropertyInfo* WINAPI LVTiles::ItemHeightProp()
    {
        return &impItemHeightProp;
    }

    int LVTiles::GetItemHeight()
    {
        return this->GetPropCommon(ItemHeightProp);
    }

    void LVTiles::SetItemHeight(int iScaleIntervals)
    {
        this->SetPropCommon(ItemHeightProp, iScaleIntervals);
    }

    void LVTiles::_UpdateGridLayoutParams()
    {
        CValuePtr v;
        RECT rcGadget{}, rcPadding{};
        _peWhitespace->GetRenderPadding(&rcPadding);
        GetGadgetRect(_peWhitespace->GetDisplayNode(), &rcGadget, 0x4);
        int width = rcGadget.right - rcPadding.left - rcPadding.right;
        _szGridLayout.cx = floor(width / this->GetItemMinWidth());
        int iNonAbsChildren{};
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        if (rgList)
        {
            for (int i = 0; i < rgList->GetSize(); i++)
            {
                if (rgList->GetItem(i)->GetLayoutPos() != -2)
                    iNonAbsChildren++;
            }
            RECT rcMargin = *(rgList->GetItem(0)->GetMargin(&v));
            _szGridLayout.cx = floor(width / (this->GetItemMinWidth() + max(rcMargin.left, rcMargin.right)));
            _szGridLayout.cy = ceil(iNonAbsChildren / static_cast<float>(_szGridLayout.cx));
        }
        int iHeight = this->GetItemHeight() * _szGridLayout.cy + rcPadding.top + rcPadding.bottom;
        if (rgList && rgList->GetSize() > _szGridLayout.cx)
        {
            int i = _szGridLayout.cx;
            while (i < rgList->GetSize())
            {
                if (rgList->GetItem(i)->GetLayoutPos() == -2)
                {
                    i++;
                    continue;
                }
                RECT rcMargin = *(rgList->GetItem(i)->GetMargin(&v));
                iHeight += max(rcMargin.top, rcMargin.bottom);
                i += _szGridLayout.cx;
            }
        }
        _peWhitespace->SetHeight(iHeight);
        int gridlayoutParams[2] = { _szGridLayout.cy, _szGridLayout.cx };
        CreateAndSetLayout(_peWhitespace, GridLayout::Create, ARRAYSIZE(gridlayoutParams), gridlayoutParams);
    }

    int LVTiles::GetPropCommon(const PropertyProcT pPropertyProc)
    {
        if (this->IsDestroyed()) return -1;
        Value* pv = GetValue(pPropertyProc, 2, nullptr);
        int v = pv->GetInt();
        pv->Release();
        return v;
    }

    void LVTiles::SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt)
    {
        Value* pv = Value::CreateInt(iCreateInt);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(pPropertyProc, 1, pv);
            pv->Release();
        }
    }

    void LVTiles::_OnAddOrInsert(Element** ppe, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        _UpdateGridLayoutParams();
        UINT iMaxIdx = 0;
        for (int i = 0; i < cCount; i++)
        {
            if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                continue;
            ((LVItem*)ppe[i])->SetShowKeyFocus(false);
            GetGadgetRect(ppe[i]->GetDisplayNode(), &prcGadget[i], 0xC);
            if (!(_flags & LVCF_NOASSIGNFUNC))
            {
                assignFn(ppe[i], SelectItemBase);
                assignExtendedFn(ppe[i], RefineSelections);
                if (((LVItem*)ppe[i])->GetCheckbox())
                    assignExtendedFn(((LVItem*)ppe[i])->GetCheckbox(), CheckboxHandler);
            }
            iMaxIdx = i;
        }
        if (!(_flags & LVCF_NOANIMATE))
        {
            RECT rcLVHost;
            Element* peParent = this;
            while (peParent->GetClassInfoW() != TouchScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != StyledScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != ScrollViewer::GetClassInfoPtr())
            {
                if (!peParent->GetParent())
                    break;
                peParent = peParent->GetParent();
            }
            GetGadgetRect(peParent->GetDisplayNode(), &rcLVHost, 0xC);
            GTRANS_DESC transDesc[2];
            TransitionStoryboardInfo tsbInfo = {};
            for (int i = 0; i < cCount; i++)
            {
                if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                    continue;
                if (prcGadget[i].right > rcLVHost.left && prcGadget[i].bottom > rcLVHost.top &&
                    prcGadget[i].left < rcLVHost.right && prcGadget[i].top < rcLVHost.bottom)
                {
                    TriggerFade(ppe[i], transDesc, 0, 0.06f, 0.143f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                    TriggerScaleIn(ppe[i], transDesc, 1, 0.06f, 0.31f, 0.0f, 0.0f, 0.0f, 1.0f, 0.7f, 0.7f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                }
            }
            if (!(_flags & LVCF_ANIMATEPARTIAL))
            {
                CValuePtr v;
                DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
                if (rgList && rgList->GetSize() > 0 && ppe[0]->GetClassInfoW() == LVItem::GetClassInfoPtr())
                    _RemoveStuckClones(rgList);
                if (rgList && rgList->GetSize() > cCount && ppe[0]->GetClassInfoW() == LVItem::GetClassInfoPtr())
                {
                    *prcNext = prcGadget[0];
                    for (int i = 0; i < rgList->GetSize(); i++)
                    {
                        Element* child = rgList->GetItem(i);
                        if (child->GetLayoutPos() != -2 && child->GetClassInfoW() == LVItem::GetClassInfoPtr())
                        {
                            RECT rcItem;
                            GetGadgetRect(child->GetDisplayNode(), &rcItem, 0xC);
                            if ((rcItem.top > prcGadget[iMaxIdx].top || (rcItem.top >= prcGadget[iMaxIdx].top &&
                                ((g_ctx.localeType != 1 && rcItem.left > prcGadget[iMaxIdx].left) || (g_ctx.localeType == 1 && rcItem.right < prcGadget[iMaxIdx].right)))) &&
                                prcNext->right > rcLVHost.left && prcNext->bottom > rcLVHost.top && prcNext->left < rcLVHost.right && prcNext->top < rcLVHost.bottom)
                            {
                                if (i > cCount)
                                    GetGadgetRect(rgList->GetItem(i - cCount)->GetDisplayNode(), prcNext, 0xC);
                                if (prcNext->top - rcItem.top == 0 ||
                                    (g_ctx.localeType != 1 && prcNext->left - rcItem.left <= 0) || (g_ctx.localeType == 1 && prcNext->right - rcItem.right >= 0))
                                {
                                    TriggerTranslate(child, transDesc, 0, 0.0f, 0.367f, 0.75f, 0.0f, 0.0f, 1.0f,
                                        prcNext->left - rcItem.left, prcNext->top - rcItem.top, 0, 0, false, false, true);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, nullptr, &tsbInfo);
                                }
                                else
                                {
                                    GTRANS_DESC transDesc2[6];
                                    TriggerTranslate(child, transDesc2, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                                        0, 0, prcNext->left - rcItem.left, prcNext->top - rcItem.top, false, false, true);
                                    TriggerFade(child, transDesc2, 1, 0.0f, 0.147f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                                    TriggerScaleIn(child, transDesc2, 2, 0.0f, 0.147f, 1.0f, 0.0f, 1.0f, 1.0f,
                                        1.0f, 1.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, false, false);
                                    TriggerTranslate(child, transDesc2, 3, 0.147f, 0.147f, 0.0f, 0.0f, 1.0f, 1.0f,
                                        prcNext->left - rcItem.left, prcNext->top - rcItem.top, 0, 0, false, false, true);
                                    TriggerFade(child, transDesc2, 4, 0.147f, 0.367f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                                    TriggerScaleIn(child, transDesc2, 5, 0.147f, 0.367f, 0.0f, 0.0f, 0.0f, 1.0f,
                                        0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, nullptr, &tsbInfo);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    HRESULT LVTiles::_OnRemoving(Element** ppe, Element** ppeClone, RECT* prcParent, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        GetGadgetRect(_peWhitespace->GetDisplayNode(), prcParent, 0xC);
        CValuePtr v;
        DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
        if (rgList && rgList->GetSize() > 0)
        {
            _RemoveStuckClones(rgList);
            for (int i = 1; i <= cCount; i++)
                GetGadgetRect(rgList->GetItem(rgList->GetSize() - i)->GetDisplayNode(), &prcNext[cCount - i], 0xC);
        }
        _ullRemoveTick = GetTickCount64();
        HRESULT hr = _CreateAnimatingClone(ppe, prcGadget, ppeClone, cCount);
        RECT rcParentOld{};
        GetGadgetRect(_peWhitespace->GetDisplayNode(), &rcParentOld, 0xC);
        for (int i = 0; i < cCount; i++)
        {
            prcGadget[i].top += (rcParentOld.top - prcParent->top);
            prcGadget[i].bottom += (rcParentOld.top - prcParent->top);
            prcNext[i].top += (rcParentOld.top - prcParent->top);
            prcNext[i].bottom += (rcParentOld.top - prcParent->top);
        }
        return hr;
    }

    void LVTiles::_OnRemove(Element** ppe, Element** ppeClone, RECT* prcGadget, RECT* prcNext, UINT cCount)
    {
        RECT rcParent{};
        GetGadgetRect(_peWhitespace->GetDisplayNode(), &rcParent, 0xC);
        _UpdateGridLayoutParams();
        if (!(_flags & LVCF_NOANIMATE))
        {
            UINT iMaxIdx = 0;
            RECT rcLVHost;
            Element* peParent = this;
            while (peParent->GetClassInfoW() != TouchScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != StyledScrollViewer::GetClassInfoPtr() &&
                peParent->GetClassInfoW() != ScrollViewer::GetClassInfoPtr())
            {
                if (!peParent->GetParent())
                    break;
                peParent = peParent->GetParent();
            }
            GetGadgetRect(peParent->GetDisplayNode(), &rcLVHost, 0xC);
            RECT rcParentOld;
            GetGadgetRect(_peWhitespace->GetDisplayNode(), &rcParentOld, 0xC);
            GTRANS_DESC transDesc[3];
            TransitionStoryboardInfo tsbInfo = {};
            for (int i = 0; i < cCount; i++)
            {
                if (ppe[i]->GetClassInfoW() != LVItem::GetClassInfoPtr())
                    continue;
                prcGadget[i].top += (rcParentOld.top - rcParent.top);
                prcGadget[i].bottom += (rcParentOld.top - rcParent.top);
                prcNext[i].top += (rcParentOld.top - rcParent.top);
                prcNext[i].bottom += (rcParentOld.top - rcParent.top);
                if (prcGadget[i].right > rcLVHost.left && prcGadget[i].bottom > rcLVHost.top &&
                    prcGadget[i].left < rcLVHost.right && prcGadget[i].top < rcLVHost.bottom)
                {
                    TriggerFade(ppeClone[i], transDesc, 0, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                    TriggerScaleOut(ppeClone[i], transDesc, 1, 0.0f, 0.175f, 1.0f, 1.0f, 0.0f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, true);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, nullptr, &tsbInfo);
                }
                iMaxIdx = i;
            }
            DWORD animCoef = g_ctx.animCoef;
            if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            _dwSafeRemove = animCoef * 2;
            if (!(_flags & LVCF_ANIMATEPARTIAL))
            {
                CValuePtr v;
                DynamicArray<Element*>* rgList = _peWhitespace->GetChildren(&v);
                int cSize = rgList->GetSize();
                if (rgList && cSize > cCount)
                {
                    for (int i = cSize - 1; i >= 0; i--)
                    {
                        Element* child = rgList->GetItem(i);
                        if (child->GetLayoutPos() != -2 && child->GetClassInfoW() == LVItem::GetClassInfoPtr())
                        {
                            RECT rcItem;
                            GetGadgetRect(child->GetDisplayNode(), &rcItem, 0xC);
                            if ((rcItem.top > prcGadget[0].top || (rcItem.top >= prcGadget[0].top &&
                                ((g_ctx.localeType != 1 && rcItem.left >= prcGadget[0].left) || (g_ctx.localeType == 1 && rcItem.right <= prcGadget[0].right)))) &&
                                rcItem.right > rcLVHost.left && rcItem.bottom > rcLVHost.top && rcItem.left < rcLVHost.right && rcItem.top < rcLVHost.bottom)
                            {
                                int idx = cCount * 2 + i - cSize; // Why is cCount doubled? To compensate for the added animating clones.
                                if (idx < 0) idx = 0;
                                if (i < cSize - cCount && rgList->GetItem(i + cCount)->GetLayoutPos() != -2)
                                    GetGadgetRect(rgList->GetItem(i + cCount)->GetDisplayNode(), prcNext, 0xC);
                                if (prcNext[idx].top - rcItem.top == 0 ||
                                    (g_ctx.localeType != 1 && prcNext[idx].left - rcItem.left >= 0) || (g_ctx.localeType == 1 && prcNext[idx].right - rcItem.right <= 0))
                                {   
                                    TriggerTranslate(child, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                                        0, 0, prcNext[idx].left - rcItem.left, prcNext[idx].top - rcItem.top, false, false, true);
                                    TriggerTranslate(child, transDesc, 1, 0.06f, 0.06f, 0.0f, 0.0f, 1.0f, 1.0f,
                                        prcNext[idx].left - rcItem.left, prcNext[idx].top - rcItem.top,
                                        prcNext[idx].left - rcItem.left, prcNext[idx].top - rcItem.top, false, false, true);
                                    TriggerTranslate(child, transDesc, 2, 0.06f, 0.427f, 0.85f, 0.0f, 0.0f, 1.0f,
                                        prcNext[idx].left - rcItem.left, prcNext[idx].top - rcItem.top, 0, 0, false, false, true);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                                }
                                else
                                {
                                    GTRANS_DESC transDesc2[7];
                                    TriggerTranslate(child, transDesc2, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                                        0, 0, prcNext[idx].left - rcItem.left, prcNext[idx].top - rcItem.top, false, false, true);
                                    TriggerFade(child, transDesc2, 1, 0.0f, 0.06f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, false, false, true);
                                    TriggerFade(child, transDesc2, 2, 0.06f, 0.225f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                                    TriggerScaleIn(child, transDesc2, 3, 0.06f, 0.225f, 1.0f, 0.0f, 1.0f, 1.0f,
                                        1.0f, 1.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, false, false);
                                    TriggerTranslate(child, transDesc2, 4, 0.225f, 0.225f, 0.0f, 0.0f, 1.0f, 1.0f,
                                        prcNext[idx].left - rcItem.left, prcNext[idx].top - rcItem.top, 0, 0, false, false, true);
                                    TriggerFade(child, transDesc2, 5, 0.225f, 0.427f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                                    TriggerScaleIn(child, transDesc2, 6, 0.225f, 0.427f, 0.0f, 0.0f, 0.0f, 1.0f,
                                        0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, nullptr, &tsbInfo);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    LVItem::~LVItem()
    {
        if (_childItemss != nullptr)
        {
            _childItemss->clear();
            _childItemss = nullptr;
        }
        if (_touchGrid) if (_touchGrid->GetItemCount() == 0) delete _touchGrid;
        this->StopListening();
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
        return CreateAndInit<LVItem, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
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

    wstring LVItem::GetExt()
    {
        return _ext;
    }

    void LVItem::SetFilename(const wstring& wsFilename)
    {
        _filename = wsFilename;
    }

    void LVItem::SetSimpleFilename(const wstring& wsSimpleFilename)
    {
        _simplefilename = wsSimpleFilename;
    }

    void LVItem::SetExt(const wstring& wsExt)
    {
        _ext = wsExt;
    }

    LVItemFlags LVItem::GetFlags()
    {
        return _flags;
    }

    void LVItem::AddFlags(LVItemFlags lvif)
    {
        _flags = _flags | lvif;
    }

    void LVItem::RemoveFlags(LVItemFlags lvif)
    {
        _flags = _flags & static_cast<LVItemFlags>(0xFFFFFFFF - lvif);
        if (lvif & LVIF_DIR)
            this->StopListening();
    }

    void LVItem::SetFlags(LVItemFlags lvif)
    {
        _flags = lvif;
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

    DDScalableElement* LVItem::GetInnerElement()
    {
        return _peInner;
    }

    DDScalableElement* LVItem::GetIcon()
    {
        return _peIcon;
    }

    DDScalableElement** LVItem::GetIconRef()
    {
        return &_peIcon;
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

    DDScalableRichText* LVItem::GetItemCountElement()
    {
        return _peItemCount;
    }

    void LVItem::SetInnerElement(DDScalableElement* peInner)
    {
        _peInner = peInner;
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

    void LVItem::SetItemCountElement(DDScalableRichText* peItemCount)
    {
        _peItemCount = peItemCount;
    }

    void LVItem::DisconnectElements()
    {
        _peInner = nullptr;
        _peIcon = nullptr;
        _peShortcutArrow = nullptr;
        _peText = nullptr;
        _peCheckbox = nullptr;
        _peItemCount = nullptr;
    }

    vector<LVItem*>* LVItem::GetChildItems()
    {
        return _childItemss;
    }

    void LVItem::SetChildItems(vector<LVItem*>* vpm)
    {
        _childItemss = vpm;
    }

    vector<IElementListener*>* LVItem::GetListeners()
    {
        return &_pels;
    }

    void LVItem::SetListeners(vector<IElementListener*> pels)
    {
        _pels = pels;
    }

    void LVItem::ClearAllListeners()
    {
        for (auto pel : _pels)
        {
            // More elements can be added but these are the ones with listeners so far
            if (_peIcon) _peIcon->RemoveListener(pel);
            if (_peCheckbox) _peCheckbox->RemoveListener(pel);
            this->RemoveListener(pel);
        }
    }

    HANDLE LVItem::GetDirEvent()
    {
        return _hDirEvent;
    }

    void LVItem::SetDirEvent(HANDLE hDirEvent)
    {
        _hDirEvent = hDirEvent;
    }

    void LVItem::StopListening()
    {
        if (_hDirEvent)
        {
            SetEvent(_hDirEvent);
            CloseHandle(_hDirEvent);
            _hDirEvent = nullptr;
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
        short localeDirection = (g_ctx.localeType == 1) ? -1 : 1;
        for (int i = index; i < _itemCount; i++)
        {
            int finaldestX = _xFirstTile + ((i & 1) * (_cxTile + _cxPadding) / 2 * localeDirection);
            int finaldestY = _yFirstTile + (i / 2) * (_cyTile + _cyPadding) / 2; 
            _items[i]->SetMemXPos(finaldestX);
            _items[i]->SetX(finaldestX);
            _items[i]->SetMemYPos(finaldestY);
            _items[i]->SetY(finaldestY);
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
        return CreateAndInit<DDLVActionButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
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

    DDIconButton::~DDIconButton()
    {
        this->DestroyAll(true);
    }

    IClassInfo* DDIconButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDIconButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDIconButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    bool DDIconButton::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = DDScalableTouchButton::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::ContentProp))
        {
            result = false;
            this->_SetValue(Element::AccNameProp, 1, pvNew, false);
            ElementSetValue(_peContent, ppi, pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDIconButton::IconFontProp))
        {
            result = false;
            ElementSetValue(_peIcon, Element::FontProp(), pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, DDIconButton::IconContentProp))
        {
            result = false;
            ElementSetValue(_peIcon, Element::ContentProp(), pvNew, this);
        }
        return result;
    }

    void DDIconButton::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp) || PropNotify::IsEqual(ppi, iIndex, Element::ShortcutProp))
        {
            if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp)) ElementSetValue(_peIcon, ppi, pvNew, this);
            ElementSetValue(_peContent, ppi, pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::FontProp))
        {
            ElementSetValue(_peContent, Element::FontProp(), pvNew, this);
            RedrawFontCore<DDScalableRichText>(_peContent, nullptr, this->GetNeedsFontResize());
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::ForegroundProp) || PropNotify::IsEqual(ppi, iIndex, DDScalableElement::NeedsFontResizeProp))
        {
            ElementSetValue(_peIcon, ppi, pvNew, this);
            ElementSetValue(_peContent, ppi, pvNew, this);
        }
        DDScalableTouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDIconButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDIconButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDIconButton::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((DDScalableTouchButton*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateIBVisual();
        return hr;
    }

    HRESULT DDIconButton::Register()
    {
        static const DirectUI::PropertyInfo* const rgRegisterProps[] =
        {
            &impIconFontProp,
            &impIconContentProp
        };
        return ClassInfo<DDIconButton, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDIconButton", rgRegisterProps, ARRAYSIZE(rgRegisterProps));
    }

    const PropertyInfo* WINAPI DDIconButton::IconFontProp()
    {
        return &impIconFontProp;
    }

    const PropertyInfo* WINAPI DDIconButton::IconContentProp()
    {
        return &impIconContentProp;
    }

    const WCHAR* DDIconButton::GetIconFont(Value** ppv)
    {
        if (_peIcon) return _peIcon->GetFont(ppv);
        else return nullptr;
    }

    const WCHAR* DDIconButton::GetIconContent(Value** ppv)
    {
        if (_peIcon) return _peIcon->GetContentString(ppv);
        else return nullptr;
    }

    HRESULT DDIconButton::_CreateIBVisual()
    {
        HRESULT hr = S_OK;
        CValuePtr v;

        BorderLayout::Create(0, nullptr, &v);
        this->SetValue(Element::LayoutProp, 1, v);

        hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peIcon);
        if (SUCCEEDED(hr))
        {
            this->Add((Element**)&_peIcon, 1);
            _peIcon->SetID(L"DDIB_Icon");
            hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peContent);
            if (SUCCEEDED(hr))
            {
                this->Add((Element**)&_peContent, 1);
                _peContent->SetID(L"DDIB_Text");
            }
        }

        return hr;
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
        return CreateAndInit<DDToggleButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
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
        result = DDScalableTouchButton::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
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
        if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp))
        {
            ElementSetValue(_peGlyph, ppi, pvNew, this);
            ElementSetValue(_peText, ppi, pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::ShortcutProp))
            ElementSetValue(_peText, ppi, pvNew, this);
        if (PropNotify::IsEqual(ppi, iIndex, TouchButton::PressedProp))
            ElementSetValue(_peGlyph, Element::SelectedProp(), pvNew, this);
        if (PropNotify::IsEqual(ppi, iIndex, DDCheckBox::CheckedStateProp))
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
        DDScalableTouchButton::OnEvent(pEvent);
    }

    void DDNumberedButton::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        CValuePtr v;
        if (PropNotify::IsEqual(ppi, iIndex, TouchButton::PressedProp))
        {
            if (this->GetSelected())
            {
                LPCWSTR className = this->GetClass(&v);
                if (wcscmp(className, L"cmbsel") == 0)
                {
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    float scaleRelease = (this->GetPressed()) ? 0.75f : 1.0f;
                    CSafeElementPtr<DDScalableElement> indicator;
                    indicator.Assign((DDScalableElement*)regElem(L"DDCMB_SelectionIndicator", this));
                    TriggerScaleOut(indicator, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, scaleRelease, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                }
            }
        }
        if (PropNotify::IsEqual(ppi, iIndex, TouchButton::SelectedProp))
        {
            LPCWSTR className = this->GetClass(&v);
            if (wcscmp(className, L"cmbsel") == 0)
            {
                CSafeElementPtr<DDScalableElement> indicator;
                indicator.Assign((DDScalableElement*)regElem(L"DDCMB_SelectionIndicator", this));
                indicator->SetVisible(this->GetSelected());
            }
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::ShortcutProp))
        {
            LPCWSTR className = this->GetClass(&v);
            if (wcscmp(className, L"cmbsel") == 0)
            {
                CSafeElementPtr<DDScalableRichText> text;
                text.Assign((DDScalableRichText*)regElem(L"DDCMB_SelectionText", this));
                ElementSetValue(text, ppi, pvNew, this);
            }
        }
        DDScalableTouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDNumberedButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDNumberedButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDNumberedButton::Register()
    {
        return ClassInfo<DDNumberedButton, DDScalableTouchButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDNumberedButton", nullptr, 0);
    }

    void DDNumberedButton::SetNumberID(short id)
    {
        _id = id;
    }

    void DDNumberedButton::SetLinkedElement(void* peLinked)
    {
        _peLinked = peLinked;
    }

    LRESULT CALLBACK DDCombobox::s_TimerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DDCombobox* cmb = (DDCombobox*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        switch (uMsg)
        {
        case WM_TIMER:
            switch (wParam)
            {
            case 1:
            case 3:
                KillTimer(hWnd, wParam);
                KillTimer(hWnd, wParam + 1);
                cmb->_tick = GetTickCount64();
                SetTimer(hWnd, wParam + 1, 10, nullptr);
                break;
            case 2:
            case 4:
                LONGLONG dwTickDiff = GetTickCount64() - cmb->_tick;
                LONGLONG dwAlphaDiff{}, dwAlphaThreshold;
                if (wParam == 2)
                {
                    dwAlphaDiff = dwTickDiff * 2.5f;
                    dwAlphaThreshold = 255;
                    if (!g_ctx.comboAnim)
                        dwAlphaDiff = 256;
                }
                else
                {
                    dwAlphaDiff = 255 - dwTickDiff * 3.3f;
                    dwAlphaThreshold = 0;
                    if (!g_ctx.comboAnim)
                        dwAlphaDiff = -1;
                }
                if (dwAlphaDiff <= dwAlphaThreshold && wParam == 2)
                    SetLayeredWindowAttributes(cmb->_wndSelectionMenu->GetHWND(), 0, dwAlphaDiff, LWA_ALPHA);
                else if (dwAlphaDiff >= dwAlphaThreshold && wParam != 2)
                    SetLayeredWindowAttributes(cmb->_wndSelectionMenu->GetHWND(), 0, dwAlphaDiff, LWA_ALPHA);
                else
                {
                    dwAlphaDiff = dwAlphaThreshold;
                    SetLayeredWindowAttributes(cmb->_wndSelectionMenu->GetHWND(), 0, dwAlphaDiff, LWA_ALPHA);
                    KillTimer(hWnd, wParam - 1);
                    KillTimer(hWnd, wParam);
                    if (wParam == 2 && cmb->_peSelections[cmb->_selID])
                        cmb->_peSelections[cmb->_selID]->SetKeyFocus();
                    else if (wParam == 4)
                        cmb->_wndSelectionMenu->ShowWindow(SW_HIDE);
                }
                break;
            }
            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK DDCombobox::s_ComboboxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch (uMsg)
        {
        case WM_CLOSE:
            return 0;
        case WM_DESTROY:
            DestroyWindow(((DDCombobox*)dwRefData)->_hTimer);
            return 0;
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) ((DDCombobox*)dwRefData)->ToggleSelectionList(true);
            break;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) ((DDCombobox*)dwRefData)->ToggleSelectionList(true);
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
        DDScalableTouchButton::OnEvent(pEvent);
    }

    UID WINAPI DDCombobox::SelectionChange()
    {
        return Combobox::SelectionChange();
    }

    HRESULT DDCombobox::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDCombobox, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
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
        CValuePtr spvLayout;
        BorderLayout::Create(0, nullptr, &spvLayout);
        DDNumberedButton::Create(_peHostInner, nullptr, (Element**)&(_peSelections[index]));
        _peHostInner->Insert((Element**)&_peSelections[index], 1, index);
        _peSelections[index]->SetValue(Element::LayoutProp(), 1, spvLayout);
        DDScalableElement* peIndicator;
        DDScalableElement::Create(_peSelections[index], nullptr, (Element**)&peIndicator);
        _peSelections[index]->Add((Element**)&peIndicator, 1);
        peIndicator->SetVisible(false);
        peIndicator->SetID(L"DDCMB_SelectionIndicator");
        DDScalableRichText* peText;
        DDScalableRichText::Create(_peSelections[index], nullptr, (Element**)&peText);
        _peSelections[index]->Add((Element**)&peText, 1);
        peText->SetID(L"DDCMB_SelectionText");
        peText->SetBackgroundColor(0);
        peText->SetLayoutPos(0);
        peText->SetContentString(pszSelectionStr);
        _peSelections[index]->SetNumberID(index);
        _peSelections[index]->SetLinkedElement(this);
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
        CSafeElementPtr<DDScalableRichText> text;
        text.Assign((DDScalableRichText*)regElem(L"DDCMB_SelectionText", _peSelections[index]));
        this->SetContentString(text->GetContentString(&v));
        if (index != _selID)
        {
            Event ev;
            ev.uidType._address = nullptr;
            ev.uidType = DDCombobox::SelectionChange();
            this->FireEvent(&ev, true, false);
        }
        _selID = index;
    }

    void DDCombobox::ToggleSelectionList(bool fForceHide)
    {
        if (IsWindowVisible(_wndSelectionMenu->GetHWND()) || fForceHide)
        {
            if (g_ctx.DWMActive)
            {
                if (!_fDone)
                {
                    SetWindowLongPtrW(_hTimer, GWLP_USERDATA, (LONG_PTR)this);
                    SetTimer(_hTimer, 3, 0, nullptr);
                }
            }
            else
                _wndSelectionMenu->ShowWindow(SW_HIDE);
            _fDone = true;
        }   
        else
        {
            POINT ptRoot{}, ptDest;
            RECT rcRoot, rcPreExpand{}, rcElement{}, rcList{}, rcSelected{}, dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            GetWindowRect(((HWNDElement*)this->GetRoot())->GetHWND(), &rcRoot);
            this->GetRoot()->MapElementPoint(this, &ptRoot, &ptDest);
            GetGadgetRect(this->GetDisplayNode(), &rcElement, 0xC);
            GetGadgetRect(_peHostInner->GetDisplayNode(), &rcList, 0xC);
            rcList.top -= 1, rcList.bottom += 1; // Window borders, left and right are unused
            Element* peSelected = _peSelections[_selID] ? _peSelections[_selID] : _peHostInner;
            GetGadgetRect(peSelected->GetDisplayNode(), &rcSelected, 0xC);
            LONG halfHeight = (this->GetListMaxHeight() - rcElement.bottom + rcElement.top) / 2;
            _rcDest.left = rcRoot.left + ptDest.x;
            _rcDest.top = rcRoot.top + ptDest.y - halfHeight;
            _rcDest.right = rcElement.right - rcElement.left;
            _rcDest.bottom = min(rcList.bottom - rcList.top, this->GetListMaxHeight());
            if (_rcDest.bottom < this->GetListMaxHeight())
            {
                _rcDest.top += halfHeight - rcSelected.top - 1;
            }
            else
            {
                _tsvSelectionMenu->SetYOffset(rcSelected.top - rcList.top - halfHeight);
                if (halfHeight > rcSelected.top - rcList.top)
                {
                    _tsvSelectionMenu->SetYOffset(0);
                    _rcDest.top += halfHeight - rcSelected.top + rcList.top;
                }
                if (halfHeight > rcList.bottom - rcSelected.bottom)
                {
                    _tsvSelectionMenu->SetYOffset(rcList.bottom - rcList.top - _rcDest.bottom);
                    _rcDest.top += halfHeight - rcSelected.top + rcList.top + _tsvSelectionMenu->GetYOffset();
                }
                if (_rcDest.top < 0) _rcDest.top = 0;
                if (_rcDest.bottom > dimensions.bottom) _rcDest.bottom = dimensions.bottom;
            }
            SetWindowPos(_wndSelectionMenu->GetHWND(), HWND_TOPMOST, _rcDest.left, _rcDest.top, _rcDest.right, _rcDest.bottom, NULL);
            _wndSelectionMenu->ShowWindow(SW_SHOW);
            if (g_ctx.DWMActive)
            {
                SetLayeredWindowAttributes(_wndSelectionMenu->GetHWND(), NULL, 0, LWA_ALPHA);
                SetWindowLongPtrW(_hTimer, GWLP_USERDATA, (LONG_PTR)this);
                SetTimer(_hTimer, 1, 0, nullptr);
            }
            _fDone = false;

            MSG msg;
            while (!_fDone)
            {
                BOOL gm = GetMessageW(&msg, nullptr, 0, 0);
                if (gm <= 0)
                {
                    if (gm == 0) PostQuitMessage(static_cast<int>(msg.wParam));
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
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
            DWORD dwExStyle = WS_EX_TOOLWINDOW, dwCreateFlags = 0x10;
            if (g_ctx.DWMActive)
            {
                dwExStyle |= WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP;
                dwCreateFlags |= 0x28;
            }
            hr = NativeHWNDHost::Create(L"DDCMBMenuWindow", nullptr, nullptr, nullptr, 0, 0, 0, 0, dwExStyle, WS_POPUP | WS_BORDER | CBS_DROPDOWNLIST, HINST_THISCOMPONENT, 0x43, &_wndSelectionMenu);
            if (SUCCEEDED(hr))
            {
                HWNDElement::Create(_wndSelectionMenu->GetHWND(), true, dwCreateFlags, nullptr, &keyC, (Element**)&_peSelectionMenu);
                HWND hwndInner = FindWindowExW(_wndSelectionMenu->GetHWND(), nullptr, L"DirectUIHWND", nullptr);
                SetWindowSubclass(_wndSelectionMenu->GetHWND(), s_ComboboxProc, 1, (DWORD_PTR)this);
                SetWindowSubclass(hwndInner, s_ComboboxProc, 1, (DWORD_PTR)this);
                _peSelectionMenu->SetVisible(true);
                _peSelectionMenu->EndDefer(keyC);
                _wndSelectionMenu->Host(_peSelectionMenu);

                if (g_ctx.DWMActive)
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
                LPWSTR sheetName = g_ctx.theme ? (LPWSTR)L"DDBase" : (LPWSTR)L"DDBaseDark";
                StyleSheet* sheet = _peSelectionMenu->GetSheet();
                CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                g_parser->GetSheet(sheetName, &sheetStorage);
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
                            WCHAR* WindowsBuildStr;
                            GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
                            int WindowsBuild = _wtoi(WindowsBuildStr);
                            free(WindowsBuildStr);
                            if (g_ctx.DWMActive && WindowsBuild >= 16299)
                            {
                                BlurBackground(_wndSelectionMenu->GetHWND(), true, false, -1, nullptr);
                                AddLayeredRef(_tsvSelectionMenu->GetDisplayNode());
                                SetGadgetFlags(_tsvSelectionMenu->GetDisplayNode(), NULL, NULL);
                                MARGINS margins = { -1, -1, -1, -1 };
                                DwmExtendFrameIntoClientArea(_wndSelectionMenu->GetHWND(), &margins);
                            }
                            HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
                            if (hShlwapi)
                            {
                                pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                                    (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
                                _hTimer = SHCreateWorkerWindowW(s_TimerProc, HWND_MESSAGE, 0, 0, nullptr);
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
        if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp))
        {
            ElementSetValue(_peText, ppi, pvNew, this);
            ElementSetValue(_peTrackHolder, ppi, pvNew, this);
            ElementSetValue(_peSliderInner, ppi, pvNew, this);
            ElementSetValue(_peTrackBase, ppi, pvNew, this);
            ElementSetValue(_peFillBase, ppi, pvNew, this);
            ElementSetValue(_peTrack, ppi, pvNew, this);
            ElementSetValue(_peFill, ppi, pvNew, this);
            ElementSetValue(_peThumb, ppi, pvNew, this);
            ElementSetValue(_peThumbInner, ppi, pvNew, this);
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
        if (pInput->nCode == GMOUSE_DOWN || pInput->nCode == GMOUSE_DRAG || (pInput->nCode == GMOUSE_MOVE && pInput->nDevice == GINPUT_KEYBOARD))
        {
            POINT ppt;
            GetCursorPos(&ppt);
            ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &ppt);
            bool vertical = this->GetIsVertical();
            bool canMove{};
            float percentage{}, assocVal{};
            BYTE sLeft, sUp, sRight, sDown;
            int width{}, height{};
            if (vertical)
            {
                int sliderSize = this->GetHeight() - this->GetTextHeight();
                if (pInput->nDevice != GINPUT_KEYBOARD) canMove = true;
                sUp = GetKeyState(VK_UP);
                sDown = GetKeyState(VK_DOWN);
                _keyState |= ((BYTE)(static_cast<bool>(sUp & 0x80)) << 1) + ((BYTE)(static_cast<bool>(sDown & 0x80)) << 3);
                float flDelta = (_keyState != _keyStateOld) ? (_maxValue - _minValue) / _tickValue : min((_maxValue - _minValue) / _tickValue, 50.0f);
                if (sUp & 0x80)
                {
                    height = _peFillBase->GetHeight() + round((sliderSize - _peThumb->GetHeight() / 2) / flDelta);
                    canMove = true;
                }
                else if (sDown & 0x80)
                {
                    height = _peFillBase->GetHeight() - round((sliderSize - _peThumb->GetHeight() / 2) / flDelta);
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
                short localeDirection = (g_ctx.localeType == 1) ? -1 : 1;
                int sliderSize = this->GetWidth() - this->GetTextWidth();
                if (pInput->nDevice != GINPUT_KEYBOARD) canMove = true;
                sLeft = GetKeyState(VK_LEFT);
                sRight = GetKeyState(VK_RIGHT);
                _keyState |= ((BYTE)(static_cast<bool>(sLeft & 0x80))) + ((BYTE)(static_cast<bool>(sRight & 0x80)) << 2);
                float flDelta = (_keyState != _keyStateOld) ? (_maxValue - _minValue) / _tickValue : min((_maxValue - _minValue) / _tickValue, 50.0f);
                if (sLeft & 0x80)
                {
                    if (g_ctx.localeType == 1) width = _peTrackBase->GetWidth() - round((sliderSize - _peThumb->GetWidth() / 2) / flDelta);
                    else width = _peFillBase->GetWidth() - round((sliderSize - _peThumb->GetWidth() / 2) / flDelta);
                    canMove = true;
                }
                else if (sRight & 0x80)
                {
                    if (g_ctx.localeType == 1) width = _peTrackBase->GetWidth() + round((sliderSize - _peThumb->GetWidth() / 2) / flDelta);
                    else width = _peFillBase->GetWidth() + round((sliderSize - _peThumb->GetWidth() / 2) / flDelta);
                    canMove = true;
                }
                else width = ppt.x - _ptBeforeClick.x + _ptOnClick.x;
                if (width < _peThumb->GetWidth() / 2) width = _peThumb->GetWidth() / 2;
                if (width > sliderSize - _peThumb->GetWidth() / 2) width = sliderSize - _peThumb->GetWidth() / 2;
                int fillwidth = sliderSize - width;
                if (canMove)
                {
                    _peTrackBase->SetWidth((g_ctx.localeType == 1) ? width : fillwidth);
                    _peFillBase->SetWidth((g_ctx.localeType == 1) ? fillwidth : width);
                    _peThumb->SetX(width - _peThumb->GetWidth() / 2);
                    percentage = static_cast<float>(((g_ctx.localeType == 1) ? fillwidth : width) - _peThumb->GetWidth() / 2) / (sliderSize - _peThumb->GetWidth());
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
                _keyStateOld = _keyState;
                _keyState &= 0xF0;
                if (pInput->nCode == GMOUSE_DOWN)
                    _keyStateOld |= 0x10;
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
        return CreateAndInit<DDSlider, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
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
        if (fExternal)
        {
            if (currValue < _minValue) currValue = _minValue;
            if (currValue > _maxValue) currValue = _maxValue;
        }
        _currValue = currValue;
        if (fExternal)
        {
            bool vertical = this->GetIsVertical();
            CSafeElementPtr<TouchButton> peFill;
            peFill.Assign((TouchButton*)regElem(L"DDS_FillBase", this));
            CSafeElementPtr<TouchButton> peThumb;
            peThumb.Assign((TouchButton*)regElem(L"DDS_Thumb", this));
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
            if (rkv.GetValueToFind())
                SetRegistryValues(rkv.GetHKeyName(), rkv.GetPath(), rkv.GetValueToFind(), assocVal * _coef, false, nullptr);
            g_ctx.atleastonesetting = true;
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

        float boundCurrValue = _currValue;
        if (boundCurrValue < _minValue) boundCurrValue = _minValue;
        if (boundCurrValue > _maxValue) boundCurrValue = _maxValue;
        float relMaxValue = _maxValue - _minValue;
        float relCurrValue = boundCurrValue - _minValue;
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
            _peThumb->SetX(((g_ctx.localeType == 1) ? round(_peTrackBase->GetWidth()) : round(_peFillBase->GetWidth())) - _peThumb->GetWidth() / 2.0f);
            _peThumb->SetY(floor((this->GetHeight() - _peThumb->GetHeight()) / 2.0f));
            float padding = (this->GetHeight() - _peTrack->GetHeight()) / 2.0f;
            _peTrackBase->SetPadding(0, floor(padding), 0, ceil(padding));
            _peFillBase->SetPadding(0, floor(padding), 0, ceil(padding));
        }
    }

    void DDSlider::s_AnimateThumb(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        CSafeElementPtr<DDScalableTouchButton> peThumbInner;
        peThumbInner.Assign((DDScalableTouchButton*)regElem(L"DDS_ThumbInner", elem));
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
            DDCPHC.Assign((DDScalableElement*)regElem(L"DDColorPicker_HoverCircle", this->GetParent()));
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
        return CreateAndInit<DDColorPickerButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
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
        if (pInput->nCode == GMOUSE_DOWN || pInput->nCode == GMOUSE_DRAG || (pInput->nCode == GMOUSE_MOVE && pInput->nDevice == GINPUT_KEYBOARD))
        {
            short localeDirection = (g_ctx.localeType == 1) ? -1 : 1;
            POINT ppt;
            GetCursorPos(&ppt);
            ScreenToClient(((HWNDElement*)this->GetRoot())->GetHWND(), &ppt);
            bool canMove{};
            BYTE sLeft, sRight;
            int width{};

            short spacedWidth = this->GetWidth() / 8;
            width = ppt.x - _ptBeforeClick.x + _ptOnClick.x + (spacedWidth - _btnWidth) / 2 * localeDirection;
            sLeft = GetKeyState(VK_LEFT);
            sRight = GetKeyState(VK_RIGHT);
            _keyState |= ((BYTE)(static_cast<bool>(sLeft & 0x80))) + ((BYTE)(static_cast<bool>(sRight & 0x80)) << 2);
            short timeCoef = 600;
            if (_keyState != _keyStateOld)
                timeCoef = 50;
            if (sLeft & 0x80 && (_ullTick < GetTickCount64() - timeCoef))
            {
                _currentColorID -= localeDirection;
                _ullTick = GetTickCount64();
                canMove = true;
            }
            else if (sRight & 0x80 && (_ullTick < GetTickCount64() - timeCoef))
            {
                _currentColorID += localeDirection;
                _ullTick = GetTickCount64();
                canMove = true;
            }
            else if (pInput->nDevice != GINPUT_KEYBOARD) _currentColorID = (g_ctx.localeType == 1) ? (this->GetWidth() - width) / spacedWidth : width / spacedWidth;
            if (_currentColorID < 0) _currentColorID = 0;
            if (_currentColorID > 7) _currentColorID = 7;
            if (pInput->nDevice != GINPUT_KEYBOARD) canMove = true;
            if (_currentColorID != _oldColorID && canMove)
            {
                _peOverlayCheck->SetX((g_ctx.localeType == 1) ? this->GetWidth() - _btnWidth - _currentColorID * spacedWidth : _currentColorID * spacedWidth);
                _peOverlayHover->SetX(-9999);
                if (_rkv.GetHKeyName() != nullptr)
                {
                    _rkv.SetValue(_rgpeColorButtons[_currentColorID]->GetOrder());
                    SetRegistryValues(_rkv.GetHKeyName(), _rkv.GetPath(), _rkv.GetValueToFind(), _rkv.GetDwValue(), false, nullptr);
                }
                _ColorizeAssociatedItems<DDScalableElement>(_targetElems);
                _ColorizeAssociatedItems<DDScalableButton>(_targetBtns);
                _ColorizeAssociatedItems<DDScalableTouchButton>(_targetTouchBtns);
            }
            _oldColorID = _currentColorID;
            _keyStateOld = _keyState;
            _keyState &= 0xF0;
            if (pInput->nDevice == GINPUT_KEYBOARD && pInput->nCode == GMOUSE_DOWN)
                _keyStateOld |= 0x10;
        }
        Element::OnInput(pInput);
    }

    void DDColorPicker::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::WidthProp) || PropNotify::IsEqual(ppi, iIndex, Element::HeightProp) ||
            PropNotify::IsEqual(ppi, iIndex, DDColorPicker::FirstScaledImageProp))
        {
            HMODULE hCaller = HINST_THISCOMPONENT;
            // 0.6 M1: Will be replaced with proper library loading later
            bool repeat = true;
        TEMP_ALTLOAD2:
            int scaleInterval = GetCurrentScaleInterval();
            int scaleIntervalImage = this->GetScaledImageIntervals();
            if (scaleInterval > scaleIntervalImage - 1)
                scaleInterval = scaleIntervalImage - 1;
            int imageID = this->GetFirstScaledImage() + scaleInterval;

            HBITMAP newImage = (HBITMAP)LoadImageW(hCaller, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            if (!newImage)
            {
                if (!repeat) hCaller = nullptr;
                LoadPNGAsBitmap(hCaller, newImage, imageID);
                IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1, NULL);
            }

            if (!newImage && repeat)
            {
                GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, (LPCWSTR)_ReturnAddress(), &hCaller);
                repeat = false;
                goto TEMP_ALTLOAD2;
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
                int xPos = (g_ctx.localeType == 1) ? this->GetWidth() - i * _btnX - _btnWidth : i * _btnX;
                HBITMAP hbmPickerBtn = CreateCompatibleBitmap(hdc, _btnWidth, btnHeight);
                SelectObject(hdcDst, hbmPickerBtn);
                BitBlt(hdcDst, 0, 0, _btnWidth, btnHeight, hdcSrc, i * _btnWidth, 0, SRCCOPY);
                if (i == 1)
                    IterateBitmap(hbmPickerBtn, StandardBitmapPixelHandler, 1, 0, 1.0f, g_colors.ImmersiveColor);
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
        return CreateAndInit<DDColorPicker, int>(0, pParent, pdwDeferCookie, ppElement);
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

    COLORREF DDColorPicker::GetDefaultColor()
    {
        if (this->IsDestroyed()) return 0;
        Value* pv = GetValue(DefaultColorProp, 2, nullptr);
        const Fill* pf = pv->GetFill();
        pv->Release();
        return pf->ref.cr;
    }

    void DDColorPicker::SetDefaultColor(COLORREF crDefaultColor)
    {
        Fill pf;
        pf.ref.cr = crDefaultColor;
        Value* pv = Value::CreateFill(pf);
        HRESULT hr = pv ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = SetValue(DefaultColorProp, 1, pv);
            pv->Release();
        }
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
        int order = (_rkv.GetHKeyName()) ? GetRegistryValues(_rkv.GetHKeyName(), _rkv.GetPath(), _rkv.GetValueToFind()) * _btnX : _rkv.GetDwValue() * _btnX;
        int checkedBtnX = (g_ctx.localeType == 1) ? this->GetWidth() - order - _btnWidth : order;
        _currentColorID = order / (this->GetWidth() / 8);
        _oldColorID = _currentColorID;
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
        COLORREF* pImmersiveColor = this->GetThemeAwareness() ? g_ctx.theme ? &(g_colors.ImmersiveColorL) : &(g_colors.ImmersiveColorD) : &(g_colors.ImmersiveColor);
        COLORREF colorPickerPalette[8] =
        {
            this->GetDefaultColor(),
            *pImmersiveColor,
            _themeAwareness ? g_ctx.theme ? RGB(76, 194, 255) : RGB(0, 103, 192) : RGB(0, 120, 215),
            _themeAwareness ? g_ctx.theme ? RGB(216, 141, 225) : RGB(158, 58, 176) : RGB(177, 70, 194),
            _themeAwareness ? g_ctx.theme ? RGB(244, 103, 98) : RGB(210, 14, 30) : RGB(232, 17, 35),
            _themeAwareness ? g_ctx.theme ? RGB(251, 154, 68) : RGB(224, 83, 7) : RGB(247, 99, 12),
            _themeAwareness ? g_ctx.theme ? RGB(255, 213, 42) : RGB(225, 157, 0) : RGB(255, 185, 0),
            _themeAwareness ? g_ctx.theme ? RGB(38, 255, 142) : RGB(0, 178, 90) : RGB(0, 204, 106)
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
            if (vElem[i] && !vElem[i]->IsDestroyed())
            {
                if (_rgpeColorButtons[_currentColorID]->GetAssociatedColor() == vElem[i]->GetAssociatedColor())
                    continue;
                TriggerCrossfade(vElem[i], 0.0f, 0.133f, nullptr);
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

    // 0.5.8: Should be rewritten for better keyboard input like in LVGrid
    void DDTabbedPages::OnInput(InputEvent* pInput)
    {
        if (pInput->nCode == GMOUSE_MOVE && pInput->nDevice == GINPUT_KEYBOARD)
        {
            CValuePtr v;
            if (pInput->peTarget->GetClass(&v))
            {
                if (wcscmp(_peTabs[_pageID]->GetClass(&v), pInput->peTarget->GetClass(&v)) == 0)
                {
                    GTRANS_DESC transDesc[2];
                    TransitionStoryboardInfo tsbInfo = {};
                    short localeDirection = (g_ctx.localeType == 1) ? -1 : 1;
                    BYTE sLeft, sRight;
                    sLeft = GetKeyState(VK_LEFT);
                    sRight = GetKeyState(VK_RIGHT);
                    _keyState |= ((BYTE)(static_cast<bool>(sLeft & 0x80))) + ((BYTE)(static_cast<bool>(sRight & 0x80)) << 2);
                    short timeCoef = 600;
                    if (_keyState != _keyStateOld)
                        timeCoef = 50;
                    if (_keyState & 0x1 && (_ullTick < GetTickCount64() - timeCoef))
                    {
                        if ((_pageID > 0 && localeDirection == 1) || (_pageID < _pageSize - 1 && localeDirection == -1))
                            this->TraversePage(_pageID - localeDirection);
                        else
                        {
                            CSafeElementPtr<Element> peEdge;
                            LPCWSTR peResID = (g_ctx.localeType == 1) ? _pszPageIDs[_pageSize - 1] : _pszPageIDs[0];
                            peEdge.Assign(regElem(peResID, _peSubUIContainer));
                            TriggerTranslate(peEdge, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 0.0f, 0.0f, 40.0f * g_ctx.flScaleFactor, 0.0f, false, false, false);
                            TriggerTranslate(peEdge, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 40.0f * g_ctx.flScaleFactor, 0.0f, 0.0f, 0.0f, false, false, false);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peEdge->GetDisplayNode(), &tsbInfo);
                        }
                        _ullTick = GetTickCount64();
                    }
                    if (_keyState & 0x4 && (_ullTick < GetTickCount64() - timeCoef))
                    {
                        if ((_pageID < _pageSize - 1 && localeDirection == 1) || (_pageID > 0 && localeDirection == -1))
                            this->TraversePage(_pageID + localeDirection);
                        else
                        {
                            CSafeElementPtr<Element> peEdge;
                            LPCWSTR peResID = (g_ctx.localeType == 1) ? _pszPageIDs[0] : _pszPageIDs[_pageSize - 1];
                            peEdge.Assign(regElem(peResID, _peSubUIContainer));
                            TriggerTranslate(peEdge, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 0.0f, 0.0f, -40.0f * g_ctx.flScaleFactor, 0.0f, false, false, false);
                            TriggerTranslate(peEdge, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, -40.0f * g_ctx.flScaleFactor, 0.0f, 0.0f, 0.0f, false, false, false);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, peEdge->GetDisplayNode(), &tsbInfo);
                        }
                        _ullTick = GetTickCount64();
                    }
                    _keyStateOld = _keyState;
                    _keyState &= 0xF0;
                }
            }
        }
        if (pInput->nCode == GMOUSE_DOWN && pInput->nDevice == GINPUT_KEYBOARD)
            _keyStateOld |= 0x10;
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
            TriggerTranslate(_peSubUIContainer, transDesc2, 0, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_ctx.flScaleFactor, 0.0f, 0.0f, false, false, true);
            TriggerFade(_peSubUIContainer, transDesc2, 1, 0.2f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerClip(_peSubUIContainer, transDesc2, 2, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 0.0f, 1.0f, (rcList.bottom - rcList.top - 100 * g_ctx.flScaleFactor) / (rcList.bottom - rcList.top), 0.0f, 0.0f, 1.0f, 1.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc2, _peSubUIContainer->GetDisplayNode(), &tsbInfo);
        }
        else if (index != _pageID)
        {
            for (int id = 0; id < pel->GetSize(); id++)
            {
                Element* child = pel->GetItem(id);
                bool fAnimate = true;
                for (int id2 = 0; id2 < _vecAnimating.size(); id2++)
                {
                    if (child == _vecAnimating[id2])
                    {
                        fAnimate = false;
                        break;
                    }
                }
                if (fAnimate)
                {
                    if ((g_ctx.localeType != 1 && index < _pageID) || (g_ctx.localeType == 1 && index > _pageID))
                    {
                        TriggerTranslate(child, transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, rcList.right - rcList.left, 0.0f, false, false, true);
                        TriggerClip(child, transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, false, true);
                    }
                    else if ((g_ctx.localeType != 1 && index > _pageID) || (g_ctx.localeType == 1 && index < _pageID))
                    {
                        TriggerTranslate(child, transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, (rcList.right - rcList.left) * -1, 0.0f, false, false, true);
                        TriggerClip(child, transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, false, true);
                    }
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                    _vecAnimating.push_back(child);
                    DWORD animCoef = g_ctx.animCoef;
                    if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                    yValuePtrs* yV = new yValuePtrs{ &_vecAnimating, child, static_cast<DWORD>(3.3f * animCoef) };
                    HANDLE hRemoveFromVec = CreateThread(nullptr, 0, s_RemoveFromVec, yV, NULL, nullptr);
                    if (hRemoveFromVec) CloseHandle(hRemoveFromVec);
                }
            }
            _tsvPage->SetYOffset(0);
            _pParser->CreateElement(_pszPageIDs[index], nullptr, nullptr, nullptr, &peSettingsPage);
            _peSubUIContainer->Add(&peSettingsPage, 1);
            if (peSettingsPage)
            {
                if ((g_ctx.localeType != 1 && index < _pageID) || (g_ctx.localeType == 1 && index > _pageID))
                {
                    TriggerTranslate(peSettingsPage, transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, (rcList.right - rcList.left) * -1, 0.0f, 0.0f, 0.0f, false, false, true);
                    TriggerClip(peSettingsPage, transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, false, false);
                }
                else if ((g_ctx.localeType != 1 && index > _pageID) || (g_ctx.localeType == 1 && index < _pageID))
                {
                    TriggerTranslate(peSettingsPage, transDesc, 0, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, rcList.right - rcList.left, 0.0f, 0.0f, 0.0f, false, false, true);
                    TriggerClip(peSettingsPage, transDesc, 1, 0.0f, 0.33f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, false, false);
                }
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
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

    IClassInfo* DDMenuButton::GetClassInfoPtr()
    {
        return s_pClassInfo;
    }

    void DDMenuButton::SetClassInfoPtr(IClassInfo* pClass)
    {
        s_pClassInfo = pClass;
    }

    IClassInfo* DDMenuButton::GetClassInfoW()
    {
        return s_pClassInfo;
    }

    void DDMenuButton::OnEvent(Event* pEvent)
    {
        if (pEvent->uidType == TouchButton::Click && _peLinked)
        {
            ((DDMenu*)_peLinked)->_OnButtonClick(this, true);
        }
        if (pEvent->uidType == TouchButton::RightClick && _peLinked)
        {
            if (((DDMenu*)_peLinked)->_uTrackFlags & TPM_RIGHTBUTTON)
                ((DDMenu*)_peLinked)->_OnButtonClick(this, true);
        }
        DDScalableTouchButton::OnEvent(pEvent);
    }

    void DDMenuButton::OnInput(InputEvent* pInput)
    {
        if (pInput->nCode == GMOUSE_DOWN && pInput->nDevice == GINPUT_KEYBOARD)
        {
            short sLeft, sRight;
            sLeft = GetAsyncKeyState(VK_LEFT);
            sRight = GetAsyncKeyState(VK_RIGHT);
            bool rtl = ((DDMenu*)_peLinked)->_uTrackFlags & TPM_LAYOUTRTL;
            if ((!rtl && (sLeft & 1 || sLeft & 0x8000)) || (rtl && (sRight & 1 || sRight & 0x8000)))
            {
                if (((DDMenu*)this->_peLinked))
                    ((DDMenu*)this->_peLinked)->_HideMenu();
                _fKeyFocusInit = false;
            }
            else if ((!rtl && (sRight & 1 || sRight & 0x8000)) || (rtl && (sLeft & 1 || sLeft & 0x8000)))
            {
                if (this->_submenu)
                    ((DDMenu*)_peLinked)->_OnButtonClick(this, true);
            }
        }
        DDNumberedButton::OnInput(pInput);
    }

    void DDMenuButton::OnKeyFocusMoved(Element* peFrom, Element* peTo)
    {
        if (this == peTo)
        {
            WORD itemID = (WORD)GetMenuItemID(((DDMenu*)this->_peLinked)->_hMenu, _uOrder);
            if (itemID == 0xFFFF)
                itemID = 0;
            WPARAM wParam = MAKEWPARAM(itemID, MF_MOUSESELECT | MF_HILITE);
            PostMessageW(((DDMenu*)this->_peLinked)->_hWndTrack, WM_MENUSELECT, wParam, (LPARAM)((DDMenu*)this->_peLinked)->_hMenu);
            if (peFrom && _submenu)
            {
                _fKeyFocusInit = true;
                SetWindowLongPtrW(((DDMenu*)_peLinked)->_hTimer, GWLP_USERDATA, (LONG_PTR)this);
                SetTimer(((DDMenu*)_peLinked)->_hTimer, 8, 800, nullptr);
            }
        }
        if (peTo && this == peFrom && _submenu)
        {
            _submenu->_HideMenu();
            _fKeyFocusInit = false;
        }
        DDNumberedButton::OnKeyFocusMoved(peFrom, peTo);
    }

    bool DDMenuButton::OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        bool result{};
        result = DDScalableTouchButton::OnPropertyChanging(ppi, iIndex, pvOld, pvNew);
        if (PropNotify::IsEqual(ppi, iIndex, Element::ContentProp))
        {
            result = false;
            this->_SetValue(Element::AccNameProp, 1, pvNew, false);
            vector<wstring> divided = SplitLineBreaks(pvNew->GetString(), L'\t');
            _peMainText->SetContentString(divided[0].c_str());
            if (divided.size() > 1)
            {
                _peHelpText->SetLayoutPos(2);
                _peHelpText->SetContentString(divided[1].c_str());
            }
        }
        return result;
    }

    void DDMenuButton::OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
    {
        if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp) || PropNotify::IsEqual(ppi, iIndex, Element::ShortcutProp))
        {
            if (PropNotify::IsEqual(ppi, iIndex, Element::ClassProp)) ElementSetValue(_peIcon, ppi, pvNew, this);
            ElementSetValue(_peMainText, ppi, pvNew, this);
        }
        if (PropNotify::IsEqual(ppi, iIndex, Element::MouseFocusedProp))
        {
            if (this->GetMouseFocused())
            {
                WORD itemID = (WORD)GetMenuItemID(((DDMenu*)this->_peLinked)->_hMenu, _uOrder);
                if (itemID == 0xFFFF)
                    itemID = 0;
                WPARAM wParam = MAKEWPARAM(itemID, MF_MOUSESELECT | MF_HILITE);
                PostMessageW(((DDMenu*)this->_peLinked)->_hWndTrack, WM_MENUSELECT, wParam, (LPARAM)((DDMenu*)this->_peLinked)->_hMenu);
                for (int i = 0; i < ((DDMenu*)this->_peLinked)->_count; i++)
                {
                    ((DDMenu*)this->_peLinked)->_peSelections[i]->_fKeyFocusInit = false;
                    DDMenu* submenu = ((DDMenu*)this->_peLinked)->_peSelections[i]->_submenu;
                    if (submenu && submenu != this->_submenu)
                        submenu->_HideMenu();
                }
                if (this->_submenu)
                    ((DDMenu*)_peLinked)->_OnButtonClick(this, false);
            }
        }
        DDScalableTouchButton::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
    }

    HRESULT DDMenuButton::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
    {
        return CreateAndInit<DDMenuButton, int>(0x1 | 0x2 | 0x8, pParent, pdwDeferCookie, ppElement);
    }

    HRESULT DDMenuButton::Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie)
    {
        HRESULT hr = ((DDScalableTouchButton*)this)->Initialize(nCreate, pParent, pdwDeferCookie);
        if (SUCCEEDED(hr))
            hr = this->_CreateMBVisual();
        return hr;
    }

    HRESULT DDMenuButton::Register()
    {
        return ClassInfo<DDMenuButton, DDNumberedButton>::RegisterGlobal(HINST_THISCOMPONENT, L"DDMenuButton", nullptr, 0);
    }

    HRESULT DDMenuButton::_CreateMBVisual()
    {
        HRESULT hr = S_OK;
        CValuePtr v;

        BorderLayout::Create(0, nullptr, &v);
        this->SetValue(Element::LayoutProp, 1, v);

        hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peIcon);
        if (SUCCEEDED(hr))
        {
            this->Add((Element**)&_peIcon, 1);
            _peIcon->SetID(L"DDMB_Icon");
            _peIcon->SetLayoutPos(0);
            hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peMainText);
            if (SUCCEEDED(hr))
            {
                this->Add((Element**)&_peMainText, 1);
                _peMainText->SetLayoutPos(0);
                _peMainText->SetBackgroundColor(0);
                hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peSubmenuArrow);
                if (SUCCEEDED(hr))
                {
                    this->Add((Element**)&_peSubmenuArrow, 1);
                    _peSubmenuArrow->SetID(L"DDMB_SubmenuArrow");
                    _peSubmenuArrow->SetLayoutPos(-3);
                    hr = DDScalableRichText::Create(this, nullptr, (Element**)&_peHelpText);
                    if (SUCCEEDED(hr))
                    {
                        this->Add((Element**)&_peHelpText, 1);
                        _peHelpText->SetID(L"DDMB_HelpText");
                        _peHelpText->SetLayoutPos(-3);
                    }
                }
            }
        }
        return hr;
    }

    LRESULT CALLBACK DDMenu::s_TimerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DDMenu* menu = (DDMenu*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        DDMenuButton* menubutton = (DDMenuButton*)menu; // only for wParam 8
        switch (uMsg)
        {
        case WM_TIMER:
            switch (wParam)
            {
            case 7:
                KillTimer(hWnd, wParam);
                menu->_wndSelectionMenu->DestroyWindow();
                DestroyWindow(hWnd);
                if (menu->_subLevel == 0)
                    if (menu->_pICv1) menu->_pICv1->Release();
                delete menu;
                break;
            case 8:
                KillTimer(hWnd, wParam);
                if (menubutton->_fKeyFocusInit && !IsWindowVisible(menubutton->_submenu->_wndSelectionMenu->GetHWND()))
                    ((DDMenu*)menubutton->_peLinked)->_OnButtonClick(menubutton, true);
                break;
            case 1:
            case 3:
            case 5:
                KillTimer(hWnd, wParam);
                KillTimer(hWnd, wParam + 1);
                if (menu->_subLevel != 0)
                {
                    if (wParam == 1)
                    {
                        if (menu->_parent->_peSelections[menu->_uID]->GetMouseFocused() || menu->_parent->_peSelections[menu->_uID]->_fKeyFocusInit)
                            goto MENUANIM;
                    }
                    else goto MENUANIM;
                }
                else
                {
                MENUANIM:
                    if (wParam == 1) menu->_wndSelectionMenu->ShowWindow(SW_SHOW);
                    menu->_tick = GetTickCount64();
                    SetTimer(hWnd, wParam + 1, 10, nullptr);
                }
                menu->_fAnimating = false;
                break;
            case 2:
            case 4:
            case 6:
                LONGLONG dwTickDiff = GetTickCount64() - menu->_tick;
                LONGLONG dwAlphaDiff{}, dwAlphaThreshold;
                RECT rcAnim = { menu->_rcMenu.left, menu->_rcMenu.top };
                short localeDirection = (menu->_uTrackFlags & TPM_LAYOUTRTL) ? -1 : 1;
                if (wParam == 2)
                {
                    dwAlphaDiff = dwTickDiff * 2.25f;
                    dwAlphaThreshold = 255;
                    if (menu->_uTrackFlags & 0x3C00) dwAlphaDiff /= 3.0f;
                    if (!g_ctx.menuAnim)
                        dwAlphaDiff = 256;
                }
                else
                {
                    dwAlphaDiff = dwTickDiff * 3.0f;
                    dwAlphaThreshold = 0;
                    if (menu->_uTrackFlags & 0x3C00 && wParam != 6) dwAlphaDiff /= 1.25f;
                    dwAlphaDiff = 255 - dwAlphaDiff;
                    if (!g_ctx.menuAnim)
                        dwAlphaDiff = -1;
                }
                if (((dwAlphaDiff <= dwAlphaThreshold && wParam == 2) || (dwAlphaDiff >= dwAlphaThreshold && wParam != 2))
                    && IsWindowVisible(menu->_wndSelectionMenu->GetHWND()))
                {
                    if (wParam != 2)
                        menu->_fAnimating = true;
                    DWORD dwAlphaDiff2 = (wParam == 2 && menu->_uTrackFlags & 0x3C00) ? dwAlphaDiff * 4 : dwAlphaDiff;
                    if (dwAlphaDiff2 > dwAlphaThreshold && wParam == 2) dwAlphaDiff2 = dwAlphaThreshold;
                    SetLayeredWindowAttributes(menu->_wndSelectionMenu->GetHWND(), 0, dwAlphaDiff2, LWA_ALPHA);
                    if (wParam != 6 || menu->_subLevel == 0)
                    {
                        float progression;
                        if (wParam == 2)
                            progression = menu->_scbi->GetProgression(dwAlphaDiff / 255.0) - 1;
                        else
                            progression = -menu->_scbi->GetProgression((255 - dwAlphaDiff) / 255.0);
                        if (menu->_uTrackFlags & TPM_HORPOSANIMATION)
                            rcAnim.left += ceil(progression * g_ctx.flScaleFactor * 32) * localeDirection;
                        else if (menu->_uTrackFlags & TPM_HORNEGANIMATION)
                            rcAnim.left -= progression * g_ctx.flScaleFactor * 32 * localeDirection;
                        if (menu->_uTrackFlags & TPM_VERPOSANIMATION)
                            rcAnim.top += ceil(progression * g_ctx.flScaleFactor * 32);
                        else if (menu->_uTrackFlags & TPM_VERNEGANIMATION)
                            rcAnim.top -= progression * g_ctx.flScaleFactor * 32;
                        if (menu->_uTrackFlags & 0x3C00)
                            SetWindowPos(menu->_wndSelectionMenu->GetHWND(), NULL, rcAnim.left, rcAnim.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
                    }
                }
                else
                {
                    dwAlphaDiff = dwAlphaThreshold;
                    DWORD dwFlags = SWP_NOSIZE | SWP_NOZORDER;
                    if (dwAlphaDiff == 0)
                        dwFlags |= SWP_NOACTIVATE;
                    SetLayeredWindowAttributes(menu->_wndSelectionMenu->GetHWND(), 0, dwAlphaDiff, LWA_ALPHA);
                    SetWindowPos(menu->_wndSelectionMenu->GetHWND(), NULL, menu->_rcMenu.left, menu->_rcMenu.top, NULL, NULL, dwFlags);
                    KillTimer(hWnd, wParam - 1);
                    KillTimer(hWnd, wParam);
                    if (wParam == 4)
                    {
                        menu->_wndSelectionMenu->ShowWindow(SW_HIDE);
                        PostMessageW(menu->_hWndTrack, WM_UNINITMENUPOPUP, (WPARAM)menu->_hMenu, NULL);
                    }
                    else if (wParam == 6)
                    {
                        delete menu->_scbi;
                        if (menu->_subLevel == 0)
                        {
                            SetTimer(hWnd, 7, 2000, nullptr);
                            menu->_wndSelectionMenu->ShowWindow(SW_HIDE);
                        }
                        else SetTimer(hWnd, 7, 200, nullptr);
                    }
                }
                break;
            }
            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK DDMenu::s_HookMenuProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (g_menu)
        {
            bool legacy = g_menu->_fUsingLegacy;
            switch (uMsg)
            {
            case WM_MENUCHAR:
                if (g_menu->_interfaceLevel == 3)
                {
                    LRESULT lResult = 0;
                    g_menu->HandleMenuMsg(uMsg, wParam, lParam, &lResult);
                    return lResult;
                }
                break;

            case WM_DRAWITEM:
            case WM_MEASUREITEM:
                if (wParam)
                    break;

            case WM_INITMENUPOPUP:
            {
                LRESULT lResult = 0;
                if (legacy)
                {
                    g_menu->HandleMenuMsg(uMsg, wParam, lParam, &lResult);
                    return lResult;
                }
                else
                {
                    ((DDMenu*)wParam)->_fDynamicInit = true;
                    MenuData* pmd = (MenuData*)lParam;
                    if (((DDMenu*)wParam)->_interfaceLevel > 1)
                    {
                        for (DDMenuButton* pmb : ((DDMenu*)wParam)->_peSelections)
                        {
                            if (pmb)
                            {
                                if (pmb->_submenu)
                                    pmb->_submenu->_DestroyUI(false);
                                pmb->DestroyAll(true);
                                pmb->Destroy(true);
                                pmb = nullptr;
                            }
                        }
                        ((DDMenu*)wParam)->_peHostInner->DestroyAll(true);
                        ((DDMenu*)wParam)->_count = 0;
                        g_menu->HandleMenuMsg(uMsg, (WPARAM)((DDMenu*)wParam)->_hMenu, pmd->uID, &lResult);
                        int count = GetMenuItemCount(((DDMenu*)wParam)->_hMenu);
                        ((DDMenu*)wParam)->_PopulateFromQuery(count, false);
                        ((DDMenu*)wParam)->_SetVisible(pmd->x, pmd->y, ((DDMenu*)wParam)->_parent, pmd->fInstant);
                        delete pmd;
                        return lResult;
                    }
                    delete pmd;
                }
                break;
            }
            }
            if (!legacy)
            {
                switch (uMsg)
                {
                case WM_INITMENU:
                case WM_ENTERMENULOOP:
                case WM_MENUSELECT:
                case WM_EXITMENULOOP:
                case WM_UNINITMENUPOPUP:
                    if (g_menu->_interfaceLevel > 1)
                    {
                        LRESULT lResult = 0;
                        g_menu->HandleMenuMsg(uMsg, wParam, lParam, &lResult);
                        return lResult;
                    }
                    return 0;
                }
            }
        }
        return CallWindowProc(g_oldMainProc, hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK DDMenu::s_MenuProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch (uMsg)
        {
        case WM_CLOSE:
            return 0;
        case WM_ACTIVATE:
            WCHAR className[64];
            GetClassNameW((HWND)lParam, className, 64);
            if (LOWORD(wParam) == WA_INACTIVE && wcscmp(className, L"DDMenu") != 0)
                ((DDMenu*)dwRefData)->_DestroyUI(true);
            break;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) ((DDMenu*)dwRefData)->_DestroyUI(true);
            break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    HRESULT DDMenu::InitializeDesktopEntries(IShellFolder* psf, IShellView* psv)
    {
        HRESULT hr = psv->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu, (void**)&_pICv1);
        if (SUCCEEDED(hr))
        {
            void** ppv = (void**)malloc(8);
            *ppv = nullptr;
            hr = _pICv1->QueryInterface(IID_IContextMenu3, ppv);
            _interfaceLevel = 3;
            if (FAILED(hr))
            {
                hr = _pICv1->QueryInterface(IID_IContextMenu2, ppv);
                _interfaceLevel = 2;
            }

            if (SUCCEEDED(hr))
            {
                _pICv1->Release();
                _pICv1 = (IContextMenu*)*ppv;
            }
            else _interfaceLevel = 1;
        }
        return hr;
    }

    HRESULT DDMenu::InitializeItemEntries(vector<LVItem**> vItems, IShellFolder* psf, LPCITEMIDLIST* ppidl, UINT cidl)
    {
        bool fVirtual = false;
        HRESULT hr;
        wstring baseExt = L"None";
        DEFCONTEXTMENU dcm = { nullptr, nullptr, NULL, psf, cidl, ppidl, nullptr, ARRAYSIZE(_hKeys), _hKeys };
        if (cidl > 0)
        {
            baseExt = (*vItems[0])->GetExt();
            if (baseExt == L"Virtual_NotImpl")
            {
                fVirtual = true;
                goto VIRTUALITEMMENU;
            }
            for (int i = 1; i < cidl; i++)
            {
                if ((*vItems[i])->GetExt() != baseExt)
                {
                    if ((*vItems[i])->GetExt() == L"Virtual_NotImpl")
                        return E_NOTIMPL;
                    baseExt = L"None";
                    break;
                }
            }
        }

        RegOpenKeyExW(HKEY_CLASSES_ROOT, L"*", 0, KEY_READ, &_hKeys[0]);
        RegOpenKeyExW(HKEY_CLASSES_ROOT, L"AllFilesystemObjects", 0, KEY_READ, &_hKeys[1]);
        RegOpenKeyExW(HKEY_CLASSES_ROOT, baseExt.c_str(), 0, KEY_READ, &_hKeys[2]);
        if (baseExt == L"Folder")
            RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Directory", 0, KEY_READ, &_hKeys[3]);
        else if (baseExt != L"None")
        {
            baseExt = baseExt.substr(1, wstring::npos) + L"file";
            RegOpenKeyExW(HKEY_CLASSES_ROOT, baseExt.c_str(), 0, KEY_READ, &_hKeys[3]);
        }

        hr = SHCreateDefaultContextMenu(&dcm, IID_IContextMenu, (void**)&_pICv1);

    VIRTUALITEMMENU:
        if (fVirtual)
        {
            hr = psf->GetUIObjectOf(nullptr, 1, ppidl, IID_IContextMenu, nullptr, (void**)&_pICv1);
            for (int i = 1; i < cidl; i++)
                (*vItems[i])->SetSelected(false);
        }
        
        if (SUCCEEDED(hr))
        {
            void** ppv = (void**)malloc(8);
            *ppv = nullptr;
            hr = _pICv1->QueryInterface(IID_IContextMenu3, ppv);
            _interfaceLevel = 3;
            if (FAILED(hr))
            {
                hr = _pICv1->QueryInterface(IID_IContextMenu2, ppv);
                _interfaceLevel = 2;
            }

            if (SUCCEEDED(hr))
            {
                _pICv1->Release();
                _pICv1 = (IContextMenu*)*ppv;
            }
            else _interfaceLevel = 1;
            hr = S_OK;
        }
        return hr;
    }

    HRESULT DDMenu::CreatePopupMenu(bool fLegacy)
    {
        _fUsingLegacy = fLegacy;
        _hMenu = ::CreatePopupMenu();
        HRESULT hr = S_OK;
        if (!fLegacy)
        {
            _scbi = new SimpleCubicBezierInterpolator();
            DWORD keyM{};
            CValuePtr spvLayout;
            DWORD dwExStyle = WS_EX_TOOLWINDOW, dwCreateFlags = 0x10;
            if (g_ctx.DWMActive)
            {
                dwExStyle |= WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP;
                dwCreateFlags |= 0x28;
            }
            hr = NativeHWNDHost::Create(L"DDMenu", nullptr, nullptr, nullptr, 0, 0, 0, 0, dwExStyle, WS_POPUP | WS_BORDER, HINST_THISCOMPONENT, 0x43, &_wndSelectionMenu);
            if (SUCCEEDED(hr))
            {
                HWNDElement::Create(_wndSelectionMenu->GetHWND(), true, dwCreateFlags, nullptr, &keyM, (Element**)&_peSelectionMenu);
                HWND hwndInner = FindWindowExW(_wndSelectionMenu->GetHWND(), nullptr, L"DirectUIHWND", nullptr);
                SetWindowSubclass(_wndSelectionMenu->GetHWND(), s_MenuProc, 1, (DWORD_PTR)this);
                SetWindowSubclass(hwndInner, s_MenuProc, 1, (DWORD_PTR)this);
                _peSelectionMenu->SetVisible(true);
                _peSelectionMenu->EndDefer(keyM);

                if (g_ctx.DWMActive)
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
                LPWSTR sheetName = g_ctx.theme ? (LPWSTR)L"DDBase" : (LPWSTR)L"DDBaseDark";
                StyleSheet* sheet = _peSelectionMenu->GetSheet();
                CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                g_parser->GetSheet(sheetName, &sheetStorage);
                _peSelectionMenu->SetValue(Element::SheetProp, 1, sheetStorage);
                free(sheet);
                _peSelectionMenu->SetID(L"DDM_SelectionList");
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
                        _peHostInner->SetID(L"DDM_SelectionListInner");
                        _wndSelectionMenu->ShowWindow(SW_HIDE);
                        if (_peHostInner)
                        {
                            _tsvSelectionMenu->Add(&_peHostInner, 1);
                            _peSelectionMenu->Add((Element**)&_tsvSelectionMenu, 1);
                            WCHAR* WindowsBuildStr;
                            GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
                            int WindowsBuild = _wtoi(WindowsBuildStr);
                            free(WindowsBuildStr);
                            if (g_ctx.DWMActive && WindowsBuild >= 16299)
                            {
                                BlurBackground(_wndSelectionMenu->GetHWND(), true, false, -1, nullptr);
                                AddLayeredRef(_tsvSelectionMenu->GetDisplayNode());
                                SetGadgetFlags(_tsvSelectionMenu->GetDisplayNode(), NULL, NULL);
                                MARGINS margins = { -1, -1, -1, -1 };
                                DwmExtendFrameIntoClientArea(_wndSelectionMenu->GetHWND(), &margins);
                            }
                            HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
                            if (hShlwapi)
                            {
                                pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                                    (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
                                _hTimer = SHCreateWorkerWindowW(s_TimerProc, HWND_MESSAGE, 0, 0, nullptr);
                            }
                        }
                    }
                }
            }
        }
        return hr;
    }

    void DDMenu::DestroyPopupMenu()
    {
        if (_subLevel == 0)
        {
            if (!_fUsingLegacy)
                this->_DestroyUI(true);
            if (_hMenu) DestroyMenu(_hMenu);
            for (HMENU hm : _hmChildren)
                if (hm) DestroyMenu(hm);
            if (_pItemArray) _pItemArray->Release();
            if (_pAssoc) _pAssoc->Release();
            for (int i = 0; i < ARRAYSIZE(_hKeys); i++)
                RegCloseKey(_hKeys[i]);
        }
        else return;
    }

    bool DDMenu::GetMenuItemInfoW(UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii)
    {
        return ::GetMenuItemInfoW(_hMenu, item, fByPosition, lpmii);
    }

    bool DDMenu::SetMenuItemInfoW(UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii)
    {
        bool result = ::SetMenuItemInfoW(_hMenu, item, fByPosition, lpmii);
        if (!_fUsingLegacy)
        {
            if (fByPosition)
            {
                _peSelections[item]->_lpmii = lpmii;
                _ApplyMII(_peSelections[item], false, item);
            }
            else
            {
                int count = GetMenuItemCount(_hMenu);
                for (int i = 0; i < count; i++)
                {
                    if (_peSelections[i]->_id == item)
                    {
                        _peSelections[i]->_lpmii = lpmii;
                        if (lpmii->fMask & MIIM_FTYPE) _peSelections[i]->_fRadio = lpmii->fType & MFT_RADIOCHECK;
                        _ApplyMII(_peSelections[i], false, i);
                        break;
                    }
                }
            }
        }
        return result;
    }

    void DDMenu::SetMenuItemGlyph(UINT item, BOOL fByPosition, LPCWSTR pszGlyph)
    {
        if (!_fUsingLegacy)
        {
            if (fByPosition)
            {
                _peSelections[item]->_peIcon->SetContentString(pszGlyph);
                _rgMenuImg[item] = new MenuImage{ NULL, pszGlyph, 2 };
            }
            else
            {
                int count = GetMenuItemCount(_hMenu);
                for (int i = 0; i < count; i++)
                {
                    if (_peSelections[i]->_id == item)
                    {
                        _peSelections[i]->_peIcon->SetContentString(pszGlyph);
                        _rgMenuImg[i] = new MenuImage{ NULL, pszGlyph, 2 };
                        break;
                    }
                }
            }
        }
    }

    void DDMenu::AppendMenuW(UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
    {
        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(MENUITEMINFOW);
        mii.fMask = 0x1EF;
        UINT_PTR uIDNewItemPopup = (uFlags & MF_POPUP) ? (UINT_PTR)((DDMenu*)uIDNewItem)->_hMenu : uIDNewItem;
        ::AppendMenuW(_hMenu, uFlags, uIDNewItemPopup, lpNewItem);
        ::GetMenuItemInfoW(_hMenu, _count, TRUE, &mii);
        if (uFlags & MFT_RADIOCHECK) mii.fType |= MFT_RADIOCHECK;
        this->_AppendItem(&mii, lpNewItem, false);
    }

    void DDMenu::EnableMenuItem(UINT uIDEnableItem, UINT uEnable)
    {
        int count = GetMenuItemCount(_hMenu);
        int index{};
        if (uEnable & MF_BYPOSITION) index = uIDEnableItem;
        else if (!_fUsingLegacy)
        {
            while ((uIDEnableItem > _peSelections[index]->_id || _peSelections[index]->_id <= 0) && index < count)
                index++;
        }
        if (count >= MAX_ITEMS || index < 0 || index > count)
            return;
        if (!_fUsingLegacy)
            _peSelections[index]->SetEnabled(!(uEnable & (MF_GRAYED | MF_DISABLED)));
        ::EnableMenuItem(_hMenu, uIDEnableItem, uEnable);
    }

    void DDMenu::InsertMenuW(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
    {
        int index{};
        if (uFlags & MF_BYPOSITION) index = uPosition;
        else if (!_fUsingLegacy)
        {
            while ((uPosition > _peSelections[index]->_id || _peSelections[index]->_id <= 0) && index < _count)
                index++;
        }
        if (_count >= MAX_ITEMS || index < 0 || index > _count)
            return;

        UINT_PTR uIDNewItemPopup = (uFlags & MF_POPUP) ? (UINT_PTR)((DDMenu*)uIDNewItem)->_hMenu : uIDNewItem;
        ::InsertMenuW(_hMenu, uPosition, uFlags, uIDNewItemPopup, lpNewItem);
        if (!_fUsingLegacy)
        {
            for (int i = _count - 1; i >= index; i--)
            {
                _rgMenuImg[i + 1] = _rgMenuImg[i];
                _peSelections[i + 1] = _peSelections[i];
                _peSelections[i + 1]->_uOrder = i + 1;
                if (_peSelections[i + 1]->_submenu)
                    _peSelections[i + 1]->_submenu->_uID = i + 1;
            }
            DDMenuButton::Create(_peHostInner, nullptr, (Element**)&(_peSelections[index]));
            _peHostInner->Insert((Element**)&_peSelections[index], 1, index);
            _peSelections[index]->_id = uIDNewItem;
            _peSelections[index]->_peLinked = this;
            _peSelections[index]->SetContentString(lpNewItem);
            if (uFlags & (MF_GRAYED | MF_DISABLED)) _peSelections[_count]->SetEnabled(false);
            if (uFlags & MF_POPUP)
            {
                ((DDMenu*)uIDNewItem)->_RegisterAsSubmenu(_peSelections[index], this);
                _peSelections[index]->_id = -1;
            }
            MENUITEMINFOW mii{};
            mii.cbSize = sizeof(MENUITEMINFOW);
            mii.fMask = 0x1EF;
            ::GetMenuItemInfoW(_hMenu, uPosition, (bool)(uFlags & MF_BYPOSITION), &mii);
            _peSelections[index]->_lpmii = &mii;
            _peSelections[index]->_fRadio = mii.fType & MFT_RADIOCHECK;
            _ApplyMII(_peSelections[index], false, index);
        }
        _count++;
    }

    void DDMenu::RemoveMenu(UINT uPosition, UINT uFlags)
    {
        int index{};
        if (uFlags & MF_BYPOSITION) index = uPosition;
        else if (!_fUsingLegacy)
        {
            while ((uPosition > _peSelections[index]->_id || _peSelections[index]->_id <= 0) && index < _count)
                index++;
        }
        if (_count >= MAX_ITEMS || index < 0)
            return;
        if (!_fUsingLegacy)
        {
            if (_peSelections[index]->_submenu)
                _peSelections[index]->_submenu->_DestroyUI(false);
            CValuePtr v;
            DynamicArray<Element*>* pel = _peSelections[index]->GetChildren(&v);
            if (pel) _peSelections[index]->DestroyAll(true);
            _peSelections[index]->Destroy(true);
            _peSelections[index] = nullptr;
            if (_rgMenuImg[index])
            {
                if (_rgMenuImg[index]->hbmp)
                    DeleteObject(_rgMenuImg[index]->hbmp);
                delete _rgMenuImg[index];
            }
            for (int i = index; i < _count - 1; i++)
            {
                _rgMenuImg[i] = _rgMenuImg[i + 1];
                _rgMenuImg[i + 1] = nullptr;
                _peSelections[i] = _peSelections[i + 1];
                _peSelections[i]->_uOrder = i;
                if (_peSelections[i]->_submenu)
                    _peSelections[i]->_submenu->_uID = i;
            }
        }
        ::RemoveMenu(_hMenu, uPosition, uFlags);
        _count--;
    }

    void DDMenu::QueryContextMenu(UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
    {
        LRESULT res{};
        HRESULT hr = _pICv1->QueryContextMenu(_hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
        _idCmdFirst = idCmdFirst;
        _idCmdLast = idCmdLast;
        if (SUCCEEDED(hr) && !_fUsingLegacy)
        {
            int count = GetMenuItemCount(_hMenu);
            if (count <= 0) return;

            this->_PopulateFromQuery(count, true);
        }
    }

    int DDMenu::TrackPopupMenuEx(UINT uFlags, int x, int y, HWND hwnd, LPTPMPARAMS lptpm)
    {
        _hWndTrack = hwnd;
        g_menu = this;
        g_oldMainProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)s_HookMenuProc);
        if (_fUsingLegacy)
        {
            uFlags &= 0x1FFFF;
            int result = ::TrackPopupMenuEx(_hMenu, uFlags, x, y, hwnd, lptpm);
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)g_oldMainProc);
            g_menu = nullptr;
            return result;
        }
        else
        {
            this->_uTrackFlags = uFlags;
            PostMessageW(hwnd, WM_ENTERMENULOOP, 1, NULL);
            PostMessageW(hwnd, WM_INITMENU, (WPARAM)this->_hMenu, NULL);
            if (_interfaceLevel > 1)
            {
                MenuData* pmd = new MenuData{ x, y, NULL, false };
                PostMessageW(hwnd, WM_INITMENUPOPUP, (WPARAM)this, (LPARAM)pmd);
            }
            else
                this->_SetVisible(x, y, nullptr, false);

            _selectedCommand = 0;
            _fDone = false;

            MSG msg;
            while (!_fDone)
            {
                BOOL gm = GetMessageW(&msg, nullptr, 0, 0);
                if (gm <= 0)
                {
                    if (gm == 0) PostQuitMessage(static_cast<int>(msg.wParam));
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            PostMessageW(hwnd, WM_UNINITMENUPOPUP, (WPARAM)this->_hMenu, NULL);
            PostMessageW(hwnd, WM_EXITMENULOOP, 1, NULL);

            int result = 0;
            if (uFlags & TPM_RETURNCMD) result = _selectedCommand;
            else if (!(uFlags & TPM_NONOTIFY))
            {
                if (_selectedCommand != 0 && hwnd)
                {
                    PostMessageW(hwnd, WM_COMMAND, static_cast<WPARAM>(_selectedCommand), 0);
                    result = TRUE;
                }
                else
                    result = FALSE;
            }
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)g_oldMainProc);
            g_menu = nullptr;
            return result;
        }
    }

    HRESULT DDMenu::InvokeCommand(CMINVOKECOMMANDINFO* pici)
    {
        return _pICv1->InvokeCommand(pici);
    }

    HRESULT DDMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
    {
        HRESULT hr = E_FAIL;
        switch (_interfaceLevel)
        {
        case 2:
            hr = ((IContextMenu2*)_pICv1)->HandleMenuMsg(uMsg, wParam, lParam);
            *plResult = hr;
            break;
        case 3:
            hr = ((IContextMenu3*)_pICv1)->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
            break;
        }
        return hr;
    }

    HRESULT DDMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved, CHAR* pszName, UINT cchMax)
    {
        return _pICv1->GetCommandString(idCmd, uType, pReserved, pszName, cchMax);
    }

    int DDMenu::GetItemCount()
    {
        return GetMenuItemCount(_hMenu);
    }

    HRESULT DDMenu::GetMenuRect(LPRECT lprc)
    {
        if (_fUsingLegacy)
            return E_NOTIMPL;
        else
        {
            GetGadgetRect(_peSelectionMenu->GetDisplayNode(), lprc, 0xC);
            return S_OK;
        }
    }

    void DDMenu::ForceEnablePaste()
    {
        for (int i = 0; i < MAX_ITEMS; i++)
        {
            if (!_peSelections[i] || _peSelections[i]->_id == -1 || _peSelections[i]->_id < _idCmdFirst)
                continue;
            if (_peSelections[i]->_lpmii->fType & MF_SEPARATOR)
                continue;
            CHAR commandW[MAX_PATH];
            _pICv1->GetCommandString(_peSelections[i]->_id - _idCmdFirst, GCS_VERBW, nullptr, commandW, MAX_PATH);
            if (wcscmp((LPWSTR)commandW, L"paste") == 0)
            {
                UINT fPaste = MF_DISABLED;
                OpenClipboard(nullptr);
                if (IsClipboardFormatAvailable(CF_HDROP))
                    fPaste = MF_ENABLED;
                CloseClipboard();
                EnableMenuItem(_peSelections[i]->_uOrder, MF_BYPOSITION | fPaste);
            }
        }
    }

    void DDMenu::_AppendItem(LPCMENUITEMINFOW lpmii, LPCWSTR lpNewItem, bool fInternal)
    {
        if (_count >= MAX_ITEMS)
            return;
        if (!_fUsingLegacy)
        {
            DDMenuButton::Create(_peHostInner, nullptr, (Element**)&(_peSelections[_count]));
            _peHostInner->Add((Element**)&_peSelections[_count], 1);
            _peSelections[_count]->_id = lpmii->wID;
            _peSelections[_count]->_uOrder = _count;
            _peSelections[_count]->_peLinked = this;
            _peSelections[_count]->SetContentString(lpNewItem);
            _peSelections[_count]->_lpmii = (LPMENUITEMINFOW)lpmii;
            if (lpmii->fMask & MIIM_FTYPE) _peSelections[_count]->_fRadio = lpmii->fType & MFT_RADIOCHECK;
            _ApplyMII(_peSelections[_count], fInternal, _count);
        }
        _count++;
    }

    void DDMenu::_ApplyMII(DDMenuButton* pmb, bool fInternal, unsigned short count)
    {
        pmb->SetEnabled(!(pmb->_lpmii->fState & (MF_GRAYED | MF_DISABLED)));
        if ((pmb->_lpmii->fType & MF_POPUP || pmb->_lpmii->hSubMenu) && !(pmb->_submenu))
        {
            if (fInternal)
            {
                int count = GetMenuItemCount(pmb->_lpmii->hSubMenu);
                if (count <= 0) return;
                DDMenu* psm = new DDMenu();
                psm->CreatePopupMenu(_fUsingLegacy);
                DestroyMenu(psm->_hMenu);
                psm->_hMenu = pmb->_lpmii->hSubMenu;
                DDMenu* toAdd = this;
                while (toAdd->_parent)
                    toAdd = toAdd->_parent;
                toAdd->_hmChildren.push_back(psm->_hMenu);
                psm->_RegisterAsSubmenu(_peSelections[_count], this);
            }
            else if (pmb->_lpmii->fType & MF_POPUP)
            {
                if (pmb->_lpmii->wID)
                    ((DDMenu*)pmb->_lpmii->wID)->_RegisterAsSubmenu(_peSelections[_count], this);
            }
        }
        if (pmb->_lpmii->fType & MF_SEPARATOR) pmb->SetClass(L"menuseparator");
        else pmb->SetClass(L"menusel");
        if (pmb->_lpmii->fState & MF_CHECKED)
        {
            if (pmb->_fRadio) pmb->_peIcon->SetClass(L"radio");
            else pmb->_peIcon->SetClass(L"check");
        }
        else pmb->_peIcon->SetClass(L"");
        if (_rgMenuImg[count])
        {
            Value* v;
            HBITMAP hbmpSaved{};
            switch (_rgMenuImg[count]->type)
            {
            case 1:
                AddPaddingToBitmap(_rgMenuImg[count]->hbmp, hbmpSaved, 0, 0, 0, 0);
                v = DirectUI::Value::CreateGraphic(_rgMenuImg[count]->hbmp, 2, 0xffffffff, false, false, false);
                if (v)
                {
                    pmb->_peIcon->SetValue(Element::ContentProp, 1, v);
                    v->Release();
                }
                DeleteObject(_rgMenuImg[count]->hbmp);
                delete _rgMenuImg[count];
                _rgMenuImg[count] = new MenuImage{ hbmpSaved, NULL, 1 };
                break;
            case 2:
                pmb->_peIcon->SetContentString(_rgMenuImg[count]->glyph);
                break;
            }
        }
        else if (pmb->_lpmii->hbmpItem)
        {
            if (_fDynamicInit)
            {
                HBITMAP hbmpSaved{};
                IterateBitmap(pmb->_lpmii->hbmpItem, UndoPremultiplication, 1, 0, 1, NULL);
                AddPaddingToBitmap(pmb->_lpmii->hbmpItem, hbmpSaved, 0, 0, 0, 0);
                CValuePtr v = DirectUI::Value::CreateGraphic(pmb->_lpmii->hbmpItem, 2, 0xffffffff, false, false, false);
                if (v) pmb->_peIcon->SetValue(Element::ContentProp, 1, v);
                _rgMenuImg[count] = new MenuImage{ hbmpSaved, NULL, 1 };
            }
        }
    }

    void DDMenu::_DestroyUI(bool fSource)
    {
        if (!_fDone)
        {
            if (fSource)
            {
                DDMenu* toDestroy = this;
                while (toDestroy->_parent)
                {
                    toDestroy->_parent->_selectedCommand = toDestroy->_selectedCommand;
                    toDestroy = toDestroy->_parent;
                }
                toDestroy->_DestroyUI(false);
            }
            else
            {
                if (_subLevel == 0)
                    _scbi->SetCurve(1.0, 0.0, 1.0, 1.0);
                _fDone = true;
                for (int i = 0; i < _count; i++)
                {
                    if (_peSelections[i]->_submenu)
                    {
                        _peSelections[i]->_submenu->_DestroyUI(false);
                        _peSelections[i]->_submenu = nullptr;
                    }
                }
                if (!_fUsingLegacy)
                {
                    SetWindowLongPtrW(_hTimer, GWLP_USERDATA, (LONG_PTR)this);
                    KillTimer(_hTimer, 1);
                    KillTimer(_hTimer, 2);
                    SetTimer(_hTimer, 5, 0, nullptr);
                    for (int i = 0; i < DDMenu::MAX_ITEMS; i++)
                    {
                        if (_rgMenuImg[i] != nullptr)
                        {
                            if (_rgMenuImg[i]->hbmp)
                                DeleteObject(_rgMenuImg[i]->hbmp);
                            delete _rgMenuImg[i];
                            _rgMenuImg[i] = nullptr;
                        }
                    }
                }
            }
        }
    }

    void DDMenu::_HideMenu()
    {
        if (_subLevel > 0 && _wndSelectionMenu->GetHWND())
        {
            if (!_fAnimating)
            {
                _scbi->SetCurve(1.0, 0.0, 1.0, 1.0);
                SetWindowLongPtrW(_hTimer, GWLP_USERDATA, (LONG_PTR)this);
                KillTimer(_hTimer, 1);
                KillTimer(_hTimer, 2);
                SetTimer(_hTimer, 3, 0, nullptr);
            }
        }
    }

    void DDMenu::_OnButtonClick(DDMenuButton* button, bool fInstant)
    {
        if (!button) return;
        if (button->_submenu)
        {
            button->_fKeyFocusInit = true;
            RECT rcParentMenu{};
            POINT ptZero{}, ptSelection{}, ptVisible{};
            GetWindowRect(_wndSelectionMenu->GetHWND(), &rcParentMenu);
            _peSelections[0]->MapElementPoint(_peHostInner, &ptZero, &ptVisible);
            _peSelectionMenu->MapElementPoint(button, &ptVisible, &ptSelection);
            int x = (_uTrackFlags & TPM_LAYOUTRTL) ? rcParentMenu.left + round(g_ctx.flScaleFactor) : rcParentMenu.right - round(g_ctx.flScaleFactor);
            if (_uTrackFlags & TPM_RIGHTBUTTON)
                button->_submenu->_uTrackFlags |= TPM_RIGHTBUTTON;
            if (_uTrackFlags & TPM_LAYOUTRTL)
                button->_submenu->_uTrackFlags |= TPM_LAYOUTRTL;
            if (_uTrackFlags & DDM_ANIMATESUBMENUS)
                button->_submenu->_uTrackFlags |= TPM_HORPOSANIMATION | DDM_ANIMATESUBMENUS;
            if (_interfaceLevel > 1)
            {
                MenuData* pmd = new MenuData{ x, rcParentMenu.top + ptSelection.y, _uID, fInstant };
                PostMessageW(_hWndTrack, WM_INITMENUPOPUP, (WPARAM)(button->_submenu), (LPARAM)pmd);
            }
            else
                button->_submenu->_SetVisible(x, rcParentMenu.top + ptSelection.y, this, fInstant);
        }
        else
        {
            _selectedCommand = static_cast<int>(button->_id);
            this->_DestroyUI(true);
        }
    }

    void DDMenu::_PopulateFromQuery(UINT uCount, bool fCheckID)
    {
        for (int i = 0; i < uCount; i++)
        {
            MENUITEMINFOW mii{};
            mii.cbSize = sizeof(MENUITEMINFOW);
            mii.fMask = 0x1EF;
            ::GetMenuItemInfoW(_hMenu, i, TRUE, &mii);
            if (fCheckID && (mii.wID < _idCmdFirst || mii.wID > _idCmdLast) && mii.wID != -1)
                continue;
            WCHAR text[256]{};
            if (!(mii.fType & MF_SEPARATOR))
            {
                mii.dwTypeData = text;
                mii.cch = _countof(text);
            }
            ::GetMenuItemInfoW(_hMenu, i, TRUE, &mii);
            this->_AppendItem(&mii, text, true);
        }
    }

    void DDMenu::_RegisterAsSubmenu(DDMenuButton* pmb, DDMenu* parent)
    {
        pmb->_submenu = this;
        pmb->_peSubmenuArrow->SetLayoutPos(2);
        this->_uID = pmb->_uOrder;
        this->_parent = parent;
        this->_fUsingLegacy = ((DDMenu*)pmb->_peLinked)->_fUsingLegacy;
        this->_subLevel = ((DDMenu*)pmb->_peLinked)->_subLevel + 1;
        this->_pICv1 = ((DDMenu*)pmb->_peLinked)->_pICv1;
        this->_interfaceLevel = ((DDMenu*)pmb->_peLinked)->_interfaceLevel;

    }

    void DDMenu::_SetVisible(int x, int y, DDMenu* menu, bool fInstant)
    {
        int width{}, height{};
        RECT rcHost{}, rcParent{}, dimensions{};
        GetGadgetRect(_peSelectionMenu->GetDisplayNode(), &rcHost, 0xC);
        width = rcHost.right + 2;
        height = rcHost.bottom + 2;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        short localeDirection = (_uTrackFlags & TPM_LAYOUTRTL) ? -1 : 1;
        if (_uTrackFlags & TPM_LAYOUTRTL) x -= width;
        if (_uTrackFlags & TPM_RIGHTALIGN)
            x -= width * localeDirection;
        else if (_uTrackFlags & TPM_CENTERALIGN)
            x -= width / 2 * localeDirection;
        if (_uTrackFlags & TPM_BOTTOMALIGN)
            y -= height;
        else if (_uTrackFlags & TPM_VCENTERALIGN)
            y -= height / 2;
        _peSelectionMenu->SetDirection((_uTrackFlags & TPM_LAYOUTRTL) ? 1 : 0);
        if (_uTrackFlags & TPM_LAYOUTRTL)
        {
            if (x < dimensions.left)
            {
                if (menu)
                {
                    menu->GetMenuRect(&rcParent);
                    x += rcParent.right + width;
                    if (menu->_uTrackFlags & DDM_ANIMATESUBMENUS)
                    {
                        this->_uTrackFlags &= (0x3FFFF - TPM_HORPOSANIMATION);
                        this->_uTrackFlags |= TPM_HORNEGANIMATION;
                    }
                }
                else x = dimensions.left;
            }
            if (x + width > dimensions.right)
            {
                width += dimensions.right - dimensions.left;
                x = dimensions.left;
            }
        }
        else
        {
            if (x + width > dimensions.right)
            {
                if (menu)
                {
                    menu->GetMenuRect(&rcParent);
                    x -= rcParent.right + rcHost.right + 4 - 2 * round(g_ctx.flScaleFactor);
                    if (menu->_uTrackFlags & DDM_ANIMATESUBMENUS)
                    {
                        this->_uTrackFlags &= (0x3FFFF - TPM_HORPOSANIMATION);
                        this->_uTrackFlags |= TPM_HORNEGANIMATION;
                    }
                }
                else x = dimensions.right - width;
            }
            if (x < dimensions.left)
            {
                width += x;
                x = dimensions.left;
            }
        }
        if (y + height > dimensions.bottom)
        {
            if (menu)
            {
                GetWindowRect(menu->_wndSelectionMenu->GetHWND(), &rcParent);
                y = rcParent.bottom - height;
            }
            else y -= height;
        }
        if (y < dimensions.top)
        {
            if (height > dimensions.bottom) height = dimensions.bottom - dimensions.top;
            y = dimensions.top;
        }
        SetWindowPos(_wndSelectionMenu->GetHWND(), HWND_TOPMOST, x, y, width, height, NULL);
        _wndSelectionMenu->Host(_peSelectionMenu);
        Element* YScrollbar;
        _tsvSelectionMenu->GetVScrollbar(&YScrollbar);
        RECT rcScroll;
        GetGadgetRect(YScrollbar->GetDisplayNode(), &rcScroll, 0x4);
        width += rcScroll.right;
        SetWindowPos(_wndSelectionMenu->GetHWND(), HWND_TOPMOST, x, y, width, height, NULL);
        GetWindowRect(_wndSelectionMenu->GetHWND(), &_rcMenu);

        if (_uTrackFlags & 0x3C00 && _fAnimating)
        {
            _scbi->SetCurve(0.1, 0.9, 0.2, 1.0);
            if (_uTrackFlags & TPM_HORPOSANIMATION)
                x -= 32 * g_ctx.flScaleFactor * localeDirection;
            else if (_uTrackFlags & TPM_HORNEGANIMATION)
                x += 32 * g_ctx.flScaleFactor * localeDirection;
            if (_uTrackFlags & TPM_VERPOSANIMATION)
                y -= 32 * g_ctx.flScaleFactor;
            else if (_uTrackFlags & TPM_VERNEGANIMATION)
                y += 32 * g_ctx.flScaleFactor;
            SetWindowPos(_wndSelectionMenu->GetHWND(), HWND_TOPMOST, x, y, width, height, NULL);
        }

        if (!(IsWindowVisible(_wndSelectionMenu->GetHWND())))
        {
            if (_peSelections[0])
                _peSelections[0]->SetKeyFocus();
            if (g_ctx.DWMActive)
                SetLayeredWindowAttributes(_wndSelectionMenu->GetHWND(), NULL, 0, LWA_ALPHA);
            SetWindowLongPtrW(_hTimer, GWLP_USERDATA, (LONG_PTR)this);
            SetTimer(_hTimer, 1, menu && !fInstant ? 300 : 0, nullptr);
        }
    }

    LRESULT CALLBACK DDNotificationBanner::s_TimerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DDNotificationBanner* nb = (DDNotificationBanner*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        switch (uMsg)
        {
        case WM_TIMER:
            static DWORD animCoef;
            switch (wParam)
            {
            case 7:
                KillTimer(hWnd, wParam);
                delete nb;
                nb = nullptr;
                break;
            case 1:
                nb->_fStartedAnim = true;
            case 3:
            case 5:
            {
                KillTimer(hWnd, wParam);
                KillTimer(hWnd, wParam + 1);
                animCoef = g_ctx.animCoef;
                if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                nb->_tick = GetTickCount64();
                if (wParam == 1)
                    nb->_wnd->ShowWindow(SW_SHOWNOACTIVATE);
                SetTimer(hWnd, wParam + 1, 10, nullptr);
                break;
            }
            case 2:
            case 4:
            {
                LONGLONG dwTickDiff = GetTickCount64() - nb->_tick;
                LONGLONG dwDistDiff{}, dwDistThreshold;
                double time;
                double progression;
                int cy = nb->_iDeltaY;
                if (wParam == 2)
                {
                    time = dwTickDiff / (4.0 * animCoef);
                    progression = nb->_scbi->GetProgression(time);
                    dwDistThreshold = 0;
                    dwDistDiff = cy - cy * progression;
                }
                else
                {
                    time = dwTickDiff / (2.0 * animCoef);
                    progression = nb->_scbi->GetProgression(time);
                    dwDistThreshold = cy;
                    dwDistDiff = cy * progression;
                }
                if (g_ctx.windowAnim && time <= 1)
                {
                    SetWindowPos(nb->_wnd->GetHWND(), NULL, nb->_rcWindow.left, nb->_rcWindow.top - dwDistDiff, NULL, NULL,
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
                }
                else
                {
                    dwDistDiff = dwDistThreshold;
                    SetWindowPos(nb->_wnd->GetHWND(), NULL, nb->_rcWindow.left, nb->_rcWindow.top - dwDistDiff, NULL, NULL,
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
                    KillTimer(hWnd, wParam - 1);
                    KillTimer(hWnd, wParam);
                    if (wParam == 4)
                    {
                        nb->_wnd->ShowWindow(SW_HIDE);
                        SetTimer(hWnd, 7, 500, nullptr);
                    }
                }
                break;
            }
            case 6:
            {
                LONGLONG dwTickDiff = GetTickCount64() - nb->_tick;
                LONGLONG dwAlphaDiff{}, dwAlphaThreshold;
                dwAlphaDiff = 255 - dwTickDiff * 2.5f;
                dwAlphaThreshold = 0;
                if (!g_ctx.windowAnim)
                    dwAlphaDiff = -1;
                if (dwAlphaDiff >= dwAlphaThreshold)
                    SetLayeredWindowAttributes(nb->_wnd->GetHWND(), 0, dwAlphaDiff, LWA_ALPHA);
                else
                {
                    dwAlphaDiff = dwAlphaThreshold;
                    SetLayeredWindowAttributes(nb->_wnd->GetHWND(), 0, dwAlphaDiff, LWA_ALPHA);
                    KillTimer(hWnd, wParam - 1);
                    KillTimer(hWnd, wParam);
                    nb->_wnd->ShowWindow(SW_HIDE);
                    SetTimer(hWnd, 7, 500, nullptr);
                }
                break;
            }
            }
            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK DDNotificationBanner::s_NotificationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        DDNotificationBanner* nb = (DDNotificationBanner*)dwRefData;
        switch (uMsg)
        {
            case WM_CLOSE:
                return 0;
            case WM_DESTROY:
                return 0;
            case WM_TIMER:
                KillTimer(hWnd, wParam);
                switch (wParam)
                {
                default:
                    if (nb)
                    {
                        if (nb->_stackCount == 0)
                            nb->DestroyBanner();
                        else
                        {
                            WCHAR stackStr[64];
                            StringCchPrintfW(stackStr, 64, L"+%d more", --nb->_stackCount);
                            nb->_stackIndicator->SetContentString(stackStr);
                            if (nb->_stackCount == 0)
                            {
                                nb->_stackIndicator->SetLayoutPos(-3);
                                int cy{};
                                SIZE szText{};
                                RECT windowRect{};
                                GetWindowRect(nb->_wnd->GetHWND(), &windowRect);
                                GetTextDimensions(nb->_stackIndicator, stackStr, &szText, &cy);
                                cy = windowRect.bottom - windowRect.top - cy;
                                SetWindowPos(nb->_wnd->GetHWND(), NULL, NULL, NULL, windowRect.right - windowRect.left, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
                                DDNotificationBanner::s_RepositionBanners(true, -cy - windowRect.top + windowRect.bottom, windowRect.bottom);
                            }
                        }
                    }
                    break;
                }
                break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    
    DDNotificationBanner::~DDNotificationBanner()
    {
        DestroyWindow(_hTimer);
        _hTimer = nullptr;
        _wnd->GetElement()->DestroyAll(true);
        _wnd->GetElement()->Destroy(true);
        _wnd->DestroyWindow();
        _wnd = nullptr;
    }

    void DDNotificationBanner::CreateBanner(DDNotificationType type, LPCWSTR title, LPCWSTR content, short timeout, DDNotificationParams* pnp)
    {
        if (g_nwnds.size())
        {
            Element* peTitle = g_nwnds.back()->_title;
            Element* peContent = g_nwnds.back()->_content;
            CValuePtr v;
            if ((!title || (peTitle && wcscmp(peTitle->GetContentString(&v), title) == 0)) &&
                (!content || (peContent && wcscmp(peContent->GetContentString(&v), content) == 0))
                && type == g_nwnds.back()->_notificationType && g_nwnds.back()->_stackCount < 255)
            {
                WCHAR stackStr[64];
                StringCchPrintfW(stackStr, 64, L"+%d more", ++g_nwnds.back()->_stackCount);
                g_nwnds.back()->_stackIndicator->SetContentString(stackStr);
                if (g_nwnds.back()->_stackCount == 1)
                {
                    g_nwnds.back()->_stackIndicator->SetLayoutPos(3);
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    TriggerFade(g_nwnds.back()->_stackIndicator, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f,
                        0.0f, g_nwnds.back()->_stackIndicator->GetAlpha() / 255.0f, false, false, true);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, g_nwnds.back()->_stackIndicator->GetDisplayNode(), &tsbInfo);
                    int cy{};
                    SIZE szText{};
                    RECT windowRect{};
                    GetWindowRect(g_nwnds.back()->_wnd->GetHWND(), &windowRect);
                    GetTextDimensions(g_nwnds.back()->_stackIndicator, stackStr, &szText, &cy);
                    cy += windowRect.bottom - windowRect.top;
                    SetWindowPos(g_nwnds.back()->_wnd->GetHWND(), NULL, NULL, NULL, windowRect.right - windowRect.left, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
                    DDNotificationBanner::s_RepositionBanners(false, cy - windowRect.bottom + windowRect.top, windowRect.bottom);
                }
                if (timeout > 0)
                    SetTimer(g_nwnds.back()->_wnd->GetHWND(), (UINT_PTR)this, timeout * 1000, nullptr);
                return;
            }
        }
        unsigned long keyN{};
        Element* pHostElement;
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, dwCreateFlags = 0x10;
        if (g_ctx.DWMActive)
        {
            dwExStyle |= WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP;
            dwCreateFlags |= 0x28;
        }
        NativeHWNDHost::Create(L"DD_NotificationHost", L"DirectDesktop In-App Notification", nullptr, nullptr, 0, 0, 0, 0, dwExStyle, WS_POPUP | WS_BORDER, HINST_THISCOMPONENT, 0, &_wnd);
        HWNDElement::Create(_wnd->GetHWND(), true, dwCreateFlags, nullptr, &keyN, (Element**)&_pDDNB);
        Element::Create(0, _pDDNB, nullptr, &pHostElement);
        _pDDNB->Add(&pHostElement, 1);

        LPWSTR sheetName = g_ctx.theme ? (LPWSTR)L"DDBase" : (LPWSTR)L"DDBaseDark";
        StyleSheet* sheet = pHostElement->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        g_parser->GetSheet(sheetName, &sheetStorage);
        pHostElement->SetValue(Element::SheetProp, 1, sheetStorage);
        free(sheet);

        pHostElement->SetID(L"DDNB_Host");
        SetWindowSubclass(_wnd->GetHWND(), s_NotificationProc, 1, (DWORD_PTR)this);
        _pDDNB->SetVisible(true);
        _pDDNB->EndDefer(keyN);
        _wnd->Host(_pDDNB);
        WCHAR* WindowsBuildStr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
        int WindowsBuild = _wtoi(WindowsBuildStr);
        free(WindowsBuildStr);
        int WindowsRev = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\BuildLayers\\ShellCommon", L"BuildQfe");
        if (g_ctx.DWMActive)
        {
            AddLayeredRef(_pDDNB->GetDisplayNode());
            SetGadgetFlags(_pDDNB->GetDisplayNode(), NULL, NULL);
            MARGINS margins = { -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(_wnd->GetHWND(), &margins);
            BOOL bNoCloak = TRUE;
            HRESULT hr = DwmSetWindowAttribute(_wnd->GetHWND(), DWMWA_EXCLUDED_FROM_PEEK, &bNoCloak, sizeof(bNoCloak));
            if (WindowsBuild > 22000 || WindowsBuild == 22000 && WindowsRev >= 51)
            {
                DWORD cornerPreference = DWMWCP_ROUND;
                DwmSetWindowAttribute(_wnd->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
            }
        }
        BlurBackground(_wnd->GetHWND(), true, false, -1, nullptr);
        _pDDNB->SetBackgroundColor(0);
        CValuePtr v;
        CreateAndSetLayout(_pDDNB, BorderLayout::Create, 0, nullptr);
        CreateAndSetLayout(pHostElement, BorderLayout::Create, 0, nullptr);

        DDScalableElement::Create(pHostElement, nullptr, (Element**)&_icon);
        _icon->SetID(L"DDNB_Icon");
        pHostElement->Add((Element**)&_icon, 1);
        _notificationType = type;
        switch (type)
        {
            case DDNT_SUCCESS:
                if (!title) LoadStrFromRes(_titleStr, 64, 217);
                _icon->SetClass(L"DDNB_Icon_Success");
                break;
            case DDNT_INFO:
                if (!title) LoadStrFromRes(_titleStr, 64, 218);
                _icon->SetClass(L"DDNB_Icon_Info");
                break;
            case DDNT_WARNING:
                if (!title) LoadStrFromRes(_titleStr, 64, 219);
                _icon->SetClass(L"DDNB_Icon_Warning");
                break;
            case DDNT_ERROR:
                if (!title) LoadStrFromRes(_titleStr, 64, 220);
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

        DDScalableElement::Create(pHostElement, nullptr, (Element**)&_stackIndicator);
        _stackIndicator->SetID(L"DDNB_StackIndicator");
        pHostElement->Add((Element**)&_stackIndicator, 1);

        int cx{}, cy{};
        RECT hostpadding = *(pHostElement->GetPadding(&v));
        RECT titlepadding = *(_title->GetPadding(&v));
        cx += (hostpadding.left + hostpadding.right + _icon->GetWidth() + 2 * g_ctx.flScaleFactor);
        cy += (hostpadding.top + hostpadding.bottom);

        SIZE szText{}, szText2{};
        GetTextDimensions(_title, _titleStr, &szText, &cy);
        if (content)
            GetTextDimensions(_content, content, &szText2, &cy);

        cx += (max(szText.cx, szText2.cx));
        cy += (titlepadding.top + titlepadding.bottom);
        if (content)
        {
            RECT contentpadding = *(_content->GetPadding(&v));
            cy += (contentpadding.top + contentpadding.bottom);
        }

        // Window borders
        cx += 2;

        if (_wnd)
        {
            HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
            if (hShlwapi)
            {
                pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                    (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
                _hTimer = SHCreateWorkerWindowW(s_TimerProc, HWND_MESSAGE, 0, 0, nullptr);
            }
            SetWindowPos(_wnd->GetHWND(), HWND_TOPMOST, (dimensions.left + dimensions.right - cx) / 2, dimensions.top + 40 * g_ctx.flScaleFactor, cx, cy, SWP_FRAMECHANGED | SWP_NOACTIVATE);
            SetLayeredWindowAttributes(_wnd->GetHWND(), 0, 255, LWA_ALPHA);
            g_nwnds.push_back(this);
            DDNotificationBanner::s_RepositionBanners(false, 16 * g_ctx.flScaleFactor + cy, 0);
            _scbi = new SimpleCubicBezierInterpolator();
            _scbi->SetCurve(0.1, 1.5, 1.0, 1.0);
            SetWindowLongPtrW(_hTimer, GWLP_USERDATA, (LONG_PTR)this);
            if (timeout > 0)
                SetTimer(_wnd->GetHWND(), (UINT_PTR)this, timeout * 1000, nullptr);
            SetTimer(_hTimer, 1, 50, nullptr);
        }
    }

    void DDNotificationBanner::s_RepositionBanners(bool fReverse, int iDeltaY, int iBoundY)
    {
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        int offset = dimensions.top + 40 * g_ctx.flScaleFactor;
        for (int i = g_nwnds.size() - 1; i >= 0; i--)
        {
            RECT windowRect{};
            GetClientRect(g_nwnds[i]->_wnd->GetHWND(), &windowRect);
            g_nwnds[i]->_iDeltaY = fReverse ? iDeltaY * -1 : iDeltaY;
            if (g_ctx.windowAnim && iDeltaY && g_nwnds[i]->_rcWindow.top >= iBoundY)
            {
                GetWindowRect(g_nwnds[i]->_wnd->GetHWND(), &g_nwnds[i]->_rcWindow);
                g_nwnds[i]->_rcWindow.left = (dimensions.left + dimensions.right - windowRect.right) / 2;
                g_nwnds[i]->_rcWindow.bottom += offset - g_nwnds[i]->_rcWindow.top;
                g_nwnds[i]->_rcWindow.top = offset;
                KillTimer(g_nwnds[i]->_hTimer, 1);
                KillTimer(g_nwnds[i]->_hTimer, 2);
                SetTimer(g_nwnds[i]->_hTimer, 1, fReverse ? 100 : 0, nullptr);
            }
            else
            {
                SetWindowPos(g_nwnds[i]->_wnd->GetHWND(), HWND_TOPMOST, (dimensions.left + dimensions.right - windowRect.right) / 2, offset,
                    NULL, NULL, SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
                GetWindowRect(g_nwnds[i]->_wnd->GetHWND(), &g_nwnds[i]->_rcWindow);
            }
            offset += windowRect.bottom + 16 * g_ctx.flScaleFactor;
        }
    }

    void DDNotificationBanner::DestroyBanner()
    {
        if (_wnd != nullptr)
        {
            SetWindowLongW(_wnd->GetHWND(), GWLP_USERDATA, NULL);
            bool topmost = (this == g_nwnds.back());
            auto toRemove = find(g_nwnds.begin(), g_nwnds.end(), this);
            g_nwnds.erase(toRemove);
            DDNotificationBanner::s_RepositionBanners(true, this->_rcWindow.bottom - this->_rcWindow.top, this->_rcWindow.bottom);
            DWORD animCoef = g_ctx.animCoef;
            if (g_ctx.AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            _scbi->SetCurve(1.0, 0.0, 1.0, 1.0);
            this->_iDeltaY = this->_rcWindow.bottom - this->_rcWindow.top;
            SetWindowLongPtrW(_hTimer, GWLP_USERDATA, (LONG_PTR)this);
            KillTimer(_hTimer, 1);
            KillTimer(_hTimer, 2);
            if (topmost)
                SetTimer(_hTimer, 3, 0, nullptr);
            else SetTimer(_hTimer, 5, 0, nullptr);
        }
    }

    void DDNotificationBanner::s_DestroyBannerByButton(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            Element* pDestroyElement = elem->GetParent()->GetParent();
            DDNotificationBanner* pDestroy{};
            int i;
            for (i = 0; i < g_nwnds.size(); i++)
            {
                if (g_nwnds[i]->_pDDNB == pDestroyElement)
                {
                    pDestroy = g_nwnds[i];
                    break;
                }
            }
            if (pDestroy)
                SetTimer(pDestroy->_wnd->GetHWND(), (UINT_PTR)pDestroy, 0, nullptr);
        }
    }

    void DDNotificationBanner::AppendButton(LPCWSTR szButtonText, void(*pListener)(Element* elem, Event* iev), bool fClose)
    {
        if (_pDDNB)
        {
            if (_btnCount == 0)
            {
                Element::Create(0, _pDDNB, nullptr, &_peButtonSection);
                _pDDNB->Add(&_peButtonSection, 1);

                LPWSTR sheetName = g_ctx.theme ? (LPWSTR)L"DDBase" : (LPWSTR)L"DDBaseDark";
                StyleSheet* sheet = _peButtonSection->GetSheet();
                CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                g_parser->GetSheet(sheetName, &sheetStorage);
                _peButtonSection->SetValue(Element::SheetProp, 1, sheetStorage);
                free(sheet);

                _peButtonSection->SetID(L"DDNB_Buttons");
                int cy{};
                RECT windowRect{};
                GetWindowRect(_wnd->GetHWND(), &windowRect);
                cy = (_peButtonSection->GetHeight() + windowRect.bottom - windowRect.top);
                SetWindowPos(_wnd->GetHWND(), NULL, NULL, NULL, windowRect.right - windowRect.left, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
                DDNotificationBanner::s_RepositionBanners(false, _fStartedAnim ? cy - windowRect.bottom + windowRect.top : cy, windowRect.bottom);
            }
            _btnCount++;
            int flowlayoutParams[2] = { 1, _btnCount };
            CreateAndSetLayout(_peButtonSection, GridLayout::Create, ARRAYSIZE(flowlayoutParams), flowlayoutParams);
            DDScalableButton* pBtn{};
            DDScalableButton::Create(_peButtonSection, nullptr, (Element**)&pBtn);
            pBtn->SetNeedsFontResize(false);
            pBtn->SetClass(L"pushbuttonsecondary");
            pBtn->SetHeight(32 * g_ctx.flScaleFactor);
            pBtn->SetMargin(8 * g_ctx.flScaleFactor, 0, 0, 0);
            pBtn->SetContentString(szButtonText);
            _peButtonSection->Add((Element**)&pBtn, 1);
            if (pListener) assignFn(pBtn, pListener);
            if (fClose) assignFn(pBtn, DDNotificationBanner::s_DestroyBannerByButton);
        }
    }
}