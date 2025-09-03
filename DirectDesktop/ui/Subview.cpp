#include "pch.h"

#include "Subview.h"
#include "EditMode.h"
#include "..\backend\ContextMenus.h"
#include "..\backend\DirectoryHelper.h"
#include "..\backend\SettingsHelper.h"
#include "..\coreui\BitmapHelper.h"
#include "..\coreui\ColorHelper.h"
#include "..\coreui\StyleModifier.h"
#include "..\DirectDesktop.h"

using namespace DirectUI;

namespace DirectDesktop
{
    NativeHWNDHost* subviewwnd;
    HWNDElement* subviewparent;
    DUIXmlParser* parserSubview;
    Element* pSubview;
    unsigned long key2 = 0;

    Button* fullscreenpopupbase;
    Element* fullscreenpopupbg;
    DDScalableButton* fullscreeninner;
    Button* centered;
    Element* popupcontainer;
    DDScalableButton* PageTab1, *PageTab2, *PageTab3;
    DDScalableButton* SubUIContainer;
    void* g_tempElem;

    int g_settingsPageID = 1;
    bool g_checkifelemexists = false;
    bool g_issubviewopen = false;
    bool g_issettingsopen = false;
    bool g_pendingaction = false;

    WNDPROC WndProcSubview;

    LRESULT CALLBACK TopLevelWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
        int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor - textm.tmHeight;
        int iconPaddingX = (GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) - 48 * g_flScaleFactor) / 2;
        int iconPaddingY = (GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) - 48 * g_flScaleFactor) / 2;
        switch (uMsg)
        {
        case WM_DPICHANGED:
        {
            g_lastDpiChangeTick = GetTickCount64();
            g_delayGroupsForDpi = true;
            UpdateScale();
            SetTimer(wnd->GetHWND(), 10, 1000, nullptr);
            break;
        }
        case WM_DISPLAYCHANGE:
        {
            g_ignoreWorkAreaChange = true;
            g_lastWidth = 0, g_lastHeight = 0;
            AdjustWindowSizes(false);
            g_lastDpiChangeTick = GetTickCount64();
            SetTimer(wnd->GetHWND(), 5, 150, nullptr);
            break;
        }
        case WM_CLOSE:
        {
            return 0L;
            break;
        }
        case WM_USER + 1:
        {
            yValueEx* yV = (yValueEx*)lParam;
            vector<LVItem*>* l_pm = yV->vpm;
            for (int num = 0; num < yV->num; num++)
            {
                (*l_pm)[num]->SetVisible(true);
            }
            break;
        }
        case WM_USER + 2:
        {
            g_checkifelemexists = false;
            subviewwnd->ShowWindow(SW_HIDE);
            if (g_pendingaction) Sleep(700);
            if (lParam == 1)
            {
                HideSimpleView(false);
            }
            centered->DestroyAll(true);
            break;
        }
        case WM_USER + 3:
        {
            yValueEx* yV = (yValueEx*)lParam;
            vector<LVItem*>* l_pm = yV->vpm;
            vector<DDScalableElement*>* l_iconpm = yV->vipm;
            vector<Element*>* l_shadowpm = yV->vispm;
            vector<Element*>* l_shortpm = yV->vspm;
            vector<RichText*>* l_filepm = yV->vfpm;
            vector<DesktopIcon*>* vdi = (vector<DesktopIcon*>*)wParam;
            for (int num = 0; num < yV->num; num++)
            {
                if (!g_touchmode)
                {
                    if (l_pm)
                    {
                        (*l_pm)[num]->SetWidth(innerSizeX);
                        (*l_pm)[num]->SetHeight(innerSizeY + textm.tmHeight + 23 * g_flScaleFactor);
                    }
                    int textlines = 1;
                    if (textm.tmHeight <= 18 * g_flScaleFactor) textlines = 2;
                    if (l_filepm)
                    {
                        (*l_filepm)[num]->SetHeight(textm.tmHeight * textlines + 4 * g_flScaleFactor);
                    }
                    if (l_iconpm)
                    {
                        (*l_iconpm)[num]->SetWidth(round(g_iconsz * g_flScaleFactor));
                        (*l_iconpm)[num]->SetHeight(round(g_iconsz * g_flScaleFactor));
                        (*l_iconpm)[num]->SetX(iconPaddingX);
                        (*l_iconpm)[num]->SetY(round(iconPaddingY * 0.575));
                    }
                }
                HBITMAP iconbmp = (*vdi)[num]->icon;
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconbmp);
                if (spvBitmap != nullptr && l_iconpm) (*l_iconpm)[num]->SetValue(Element::ContentProp, 1, spvBitmap);
                HBITMAP iconshadowbmp = (*vdi)[num]->iconshadow;
                CValuePtr spvBitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshadowbmp);
                if (spvBitmapShadow != nullptr && l_shadowpm) (*l_shadowpm)[num]->SetValue(Element::ContentProp, 1, spvBitmapShadow);
                HBITMAP iconshortcutbmp = (*vdi)[num]->iconshortcut;
                CValuePtr spvBitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshortcutbmp);
                if (spvBitmapShortcut != nullptr && l_shortpm && (*l_pm)[num]->GetShortcutState() == true) (*l_shortpm)[num]->SetValue(Element::ContentProp, 1, spvBitmapShortcut);
                HBITMAP textbmp = (*vdi)[num]->text;
                CValuePtr spvBitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(textbmp);
                if (spvBitmapText != nullptr && l_filepm) (*l_filepm)[num]->SetValue(Element::ContentProp, 1, spvBitmapText);
                if (g_touchmode)
                {
                    ((DDScalableElement*)(*l_pm)[num])->SetDDCPIntensity(((*l_pm)[num]->GetHiddenState() == true) ? 192 : 255);
                    ((DDScalableElement*)(*l_pm)[num])->SetAssociatedColor((*vdi)[num]->crDominantTile);
                }
            }
            if (yV->peOptionalTarget1)
            {
                CSafeElementPtr<TouchScrollViewer> groupdirlist;
                groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", yV->peOptionalTarget1->GetParent()->GetParent()));
                groupdirlist->SetVisible(true);

                GTRANS_DESC transDesc[3];
                TriggerTranslate(groupdirlist, transDesc, 0, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false);
                TriggerFade(groupdirlist, transDesc, 1, 0.2f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                TriggerClip(groupdirlist, transDesc, 2, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 0.0f, 1.0f, ((groupdirlist->GetHeight() - 100 * g_flScaleFactor) / groupdirlist->GetHeight()), 0.0f, 0.0f, 1.0f, 1.0f, false, false);
                TransitionStoryboardInfo tsbInfo = {};
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, groupdirlist->GetDisplayNode(), &tsbInfo);

                CSafeElementPtr<Element> dirtitle; dirtitle.Assign(regElem<Element*>(L"dirtitle", yV->peOptionalTarget1->GetParent()->GetParent()));
                dirtitle->SetVisible(true);
                TriggerFade(dirtitle, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc) - 2, transDesc, dirtitle->GetDisplayNode(), &tsbInfo);

                CSafeElementPtr<Element> emptyview;
                emptyview.Assign(regElem<Element*>(L"emptyview", yV->peOptionalTarget1->GetParent()->GetParent()));
                emptyview->DestroyAll(true);
                emptyview->Destroy(true);
                yV->peOptionalTarget1->DestroyAll(true);
                yV->peOptionalTarget1->Destroy(true);
            }
            for (int i = 0; i < vdi->size(); i++)
            {
                free((*vdi)[i]);
            }
            delete yV;
            break;
        }
        case WM_USER + 4:
        {
            yValueEx* yV = (yValueEx*)lParam;
            vector<LVItem*>* l_pm = yV->vpm;
            vector<Element*>* l_shadowpm = yV->vispm;
            vector<Element*>* l_shortpm = yV->vspm;
            for (int num = 0; num < yV->num; num++)
            {
                if ((*l_pm)[num])
                {
                    if (!g_touchmode && l_shadowpm && l_shortpm)
                    {
                        (*l_shadowpm)[num]->SetWidth((g_iconsz + 16) * g_flScaleFactor);
                        (*l_shadowpm)[num]->SetHeight((g_iconsz + 16) * g_flScaleFactor);
                        (*l_shadowpm)[num]->SetX(iconPaddingX - 8 * g_flScaleFactor);
                        (*l_shadowpm)[num]->SetY((iconPaddingY * 0.575) - 6 * g_flScaleFactor);
                        (*l_shortpm)[num]->SetWidth(g_shiconsz * g_flScaleFactor);
                        (*l_shortpm)[num]->SetHeight(g_shiconsz * g_flScaleFactor);
                        (*l_shortpm)[num]->SetX(iconPaddingX);
                        (*l_shortpm)[num]->SetY((iconPaddingY * 0.575) + (g_iconsz - g_shiconsz) * g_flScaleFactor);
                    }
                }
            }
            break;
        }
        case WM_USER + 5:
        {
            g_pendingaction = true;
            Element* peTemp = reinterpret_cast<Element*>(wParam);
            peTemp->SetEnabled(!peTemp->GetEnabled());
            if (lParam == 1 && ((DDScalableButton*)peTemp)->GetAssociatedFn() != nullptr)
            {
                ((DDScalableButton*)peTemp)->ExecAssociatedFn(((DDScalableButton*)peTemp)->GetAssociatedFn());
                g_pendingaction = false;
            }
            break;
        }
        }
        return CallWindowProc(WndProcSubview, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI subfastin(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        Sleep(25);
        yValueEx* yV = (yValueEx*)lpParam;
        vector<LVItem*>* l_pm = yV->vpm;
        vector<DesktopIcon*> vdi;
        COLORREF colorPickerPalette[8] =
        {
            (iconColorID == 1) ? ImmersiveColor : IconColorizationColor, ImmersiveColor,
            RGB(0, 120, 215), RGB(177, 70, 194), RGB(232, 17, 35),
            RGB(247, 99, 12), RGB(255, 185, 0), RGB(0, 204, 106)
        };
        for (int num = 0; num < yV->num; num++)
        {
            if ((*l_pm)[num]->GetHasAdvancedIcon())
            {
                HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                break;
            }
        }
        for (int num = 0; num < yV->num; num++)
        {
            int lines_basedOnEllipsis{};
            DWORD alignment{};
            RECT g_touchmoderect{};
            int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
            int textlines = 1;
            if (textm.tmHeight <= 18 * g_flScaleFactor) textlines = 2;
            yValue* yV2 = new yValue{ num, yV->fl1, yV->fl2 };
            if (g_touchmode) CalcDesktopIconInfo(yV2, &lines_basedOnEllipsis, &alignment, true, yV->vpm, yV->vfpm);
            HBITMAP capturedBitmap{};
            if (g_touchmode) CreateTextBitmap(capturedBitmap, (*l_pm)[num]->GetSimpleFilename().c_str(), yV2->fl1 - 4 * g_flScaleFactor, lines_basedOnEllipsis, alignment, g_touchmode);
            else CreateTextBitmap(capturedBitmap, (*l_pm)[num]->GetSimpleFilename().c_str(), innerSizeX, textm.tmHeight * textlines, DT_CENTER | DT_END_ELLIPSIS, g_touchmode);
            delete yV2;
            DesktopIcon* di = new DesktopIcon;
            CSafeElementPtr<DDScalableElement> IconElement; IconElement.Assign(regElem<DDScalableElement*>(L"iconElem", yV->peOptionalTarget2));
            ApplyIcons(*l_pm, di, true, num, 1, colorPickerPalette[IconElement->GetGroupColor()]);
            if (((LVItem*)yV->peOptionalTarget2)->GetMemorySelected() == false)
            {
                for (int num2 = 0; num2 < num; num2++)
                {
                    DeleteObject(vdi[num2]->icon);
                    DeleteObject(vdi[num2]->iconshadow);
                    DeleteObject(vdi[num2]->iconshortcut);
                    DeleteObject(vdi[num2]->text);
                    DeleteObject(vdi[num2]->textshadow);
                    free(vdi[num2]);
                    UnInitThread();
                }
                vdi.clear();
                delete yV;
                return 0;
            }
            if (g_touchmode)
            {
                di->crDominantTile = GetDominantColorFromIcon(di->icon, g_iconsz, 48);
                rgb_t saturatedColor = { GetRValue(di->crDominantTile), GetGValue(di->crDominantTile), GetBValue(di->crDominantTile) };
                hsl_t saturatedColor2 = rgb2hsl(saturatedColor);
                saturatedColor2.l /= 4;
                saturatedColor2.s *= 4;
                saturatedColor = hsl2rgb(saturatedColor2);
                IterateBitmap(di->iconshadow, StandardBitmapPixelHandler, 3, 0, 1, RGB(saturatedColor.r, saturatedColor.g, saturatedColor.b));
                if (GetRValue(di->crDominantTile) * 0.299 + GetGValue(di->crDominantTile) * 0.587 + GetBValue(di->crDominantTile) * 0.114 > 156)
                {
                    IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
                    IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 1, NULL);
                }
                else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
            }
            else if (g_theme && !g_touchmode)
            {
                IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
                IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 0.9, NULL);
            }
            else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
            if (capturedBitmap != nullptr) di->text = capturedBitmap;
            vdi.push_back(di);
        }
        for (int num = 0; num < yV->num; num++)
        {
            if ((*l_pm)[num]->GetHasAdvancedIcon())
            {
                CoUninitialize();
                break;
            }
        }
        SendMessageW(subviewwnd->GetHWND(), WM_USER + 1, NULL, (LPARAM)yV);
        SendMessageW(subviewwnd->GetHWND(), WM_USER + 4, NULL, (LPARAM)yV);
        SendMessageW(subviewwnd->GetHWND(), WM_USER + 3, (WPARAM)&vdi, (LPARAM)yV);
        UnInitThread();
        return 0;
    }

    DWORD WINAPI animate6(LPVOID lpParam)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        Sleep(175 * (animCoef / 100.0f));
        AnimateWindow(subviewwnd->GetHWND(), 120 * (animCoef / 100.0f), AW_BLEND | AW_HIDE);
        //CloakWindow(subviewwnd->GetHWND(), true);
        BlurBackground(subviewwnd->GetHWND(), false, true, fullscreenpopupbg);
        SendMessageW(subviewwnd->GetHWND(), WM_USER + 2, NULL, NULL);
        SetForegroundWindow(g_hWndTaskbar);
        return 0;
    }

    DWORD WINAPI AnimateWindowWrapper2(LPVOID lpParam)
    {
        subviewwnd->ShowWindow(SW_SHOW);
        //AnimateWindow(subviewwnd->GetHWND(), 180, AW_BLEND);
        return 0;
    }

    HRESULT CloakWindow(HWND hwnd, bool fCloak)
    {
        HRESULT hr = S_OK;

        if (hwnd)
        {
            BOOL fCloak2 = fCloak;
            hr = DwmSetWindowAttribute(hwnd, DWMWA_CLOAK, &fCloak2, sizeof(fCloak2));
            if (SUCCEEDED(hr))
            {
                if (IsCompositionActive())
                {
                    ShowWindow(hwnd, SW_SHOWNA);
                }
                else
                {
                    ShowWindow(hwnd, fCloak ? SW_HIDE : SW_SHOWNA);
                }
            }
        }

        return hr;
    }

    void fullscreenAnimation(int width, int height, float animstartscale, float desktopanimstartscale)
    {
        CValuePtr v;
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        RECT padding = *(popupcontainer->GetPadding(&v));
        int maxwidth = dimensions.right - dimensions.left - padding.left - padding.right;
        int maxheight = dimensions.bottom - dimensions.top - padding.top - padding.bottom;
        if (width > maxwidth) width = maxwidth;
        if (height > maxheight) height = maxheight;
        parserSubview->CreateElement(L"fullscreeninner", nullptr, nullptr, nullptr, (Element**)&fullscreeninner);
        centered->Add((Element**)&fullscreeninner, 1);
        SetPopupSize(centered, width, height);
        SetPopupSize(fullscreeninner, width, height);
        centered->SetBackgroundColor(0);
        fullscreenpopupbase->SetVisible(true);
        fullscreeninner->SetVisible(true);
        GTRANS_DESC transDesc[2];
        TriggerFade(fullscreeninner, transDesc, 0, 0.2f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, true);
        TriggerScaleIn(fullscreeninner, transDesc, 1, 0.2f, 0.45f, 0.0f, 0.0f, 0.0f, 1.0f, animstartscale, animstartscale, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
        TransitionStoryboardInfo tsbInfo = {};
        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, fullscreeninner->GetDisplayNode(), &tsbInfo);
        GTRANS_DESC transDesc2[1];
        TriggerScaleOut(UIContainer, transDesc2, 0, 0.0f, 0.67f, 0.1f, 0.9f, 0.2f, 1.0f, desktopanimstartscale, desktopanimstartscale, 0.5f, 0.5f, false, false);
        TransitionStoryboardInfo tsbInfo2 = {};
        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, UIContainer->GetDisplayNode(), &tsbInfo2);
        if (!g_editmode) BlurBackground(subviewwnd->GetHWND(), true, true, fullscreenpopupbg);
        //CloakWindow(subviewwnd->GetHWND(), false);
        HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateWindowWrapper2, nullptr, NULL, nullptr);
        if (AnimHandle) CloseHandle(AnimHandle);
        SetTimer(wnd->GetHWND(), 7, 100, nullptr);
        g_issubviewopen = true;
    }

    void fullscreenAnimation2()
    {
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(nullptr, 0, animate6, nullptr, 0, &animThread);
        if (animThreadHandle) CloseHandle(animThreadHandle);
    }

    void ShowPopupCore()
    {
        fullscreenAnimation(800 * g_flScaleFactor, 480 * g_flScaleFactor, 0.8, 0.88);
        HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateWindowWrapper2, nullptr, NULL, nullptr);
        if (AnimHandle) CloseHandle(AnimHandle);
    }

    void HidePopupCore(bool WinDInvoked)
    {
        if (!WinDInvoked) SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
        for (LVItem* lvi : pm)
        {
            if (!(g_treatdirasgroup && lvi->GetGroupSize() != LVIGS_NORMAL))
            {
                lvi->SetMemorySelected(false);
                lvi->SetOpenDirState(LVIODS_NONE);
            }
        }
        if (g_issubviewopen)
        {
            CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
            GTRANS_DESC transDesc[2];
            TriggerScaleOut(fullscreeninner, transDesc, 0, 0.0f, 0.175f, 1.0f, 1.0f, 0.0f, 1.0f, 0.95f, 0.95f, 0.5f, 0.5f, false, false);
            TriggerFade(fullscreeninner, transDesc, 1, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, fullscreeninner->GetDisplayNode(), &tsbInfo);
            GTRANS_DESC transDesc2[1];
            TriggerScaleOut(UIContainer, transDesc2, 0, 0.175f, 0.675f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            TransitionStoryboardInfo tsbInfo2 = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, UIContainer->GetDisplayNode(), &tsbInfo2);
        }
        if (g_issettingsopen && g_atleastonesetting)
        {
            DDNotificationBanner* ddnb{};
            DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_SUCCESS, L"DDNB", LoadStrFromRes(4042).c_str(), nullptr, 3, false);
        }
        DUI_SetGadgetZOrder(UIContainer, -1);
        g_issubviewopen = false;
        g_issettingsopen = false;
        g_atleastonesetting = false;
        SetWindowPos(subviewwnd->GetHWND(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        fullscreenAnimation2();
    }

    void SelectSubItem(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            SHELLEXECUTEINFOW execInfo = {};
            execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
            execInfo.lpVerb = L"open";
            execInfo.nShow = SW_SHOWNORMAL;
            wstring temp = RemoveQuotes(((LVItem*)elem)->GetFilename());
            execInfo.lpFile = temp.c_str();
            ShellExecuteExW(&execInfo);
        }
    }

    void SelectSubItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (g_touchmode && pProp == Button::PressedProp())
        {
            CSafeElementPtr<Element> innerElem;
            innerElem.Assign(regElem<Element*>(L"innerElem", elem));
            innerElem->SetEnabled(!((LVItem*)elem)->GetPressed());
        }
    }

    void ShowDirAsGroup(LVItem* lvi)
    {
        unsigned short lviCount = 0;
        int count2{};
        EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), nullptr, true, &lviCount);
        SendMessageW(g_hWndTaskbar, WM_COMMAND, 419, 0);
        fullscreenAnimation(800 * g_flScaleFactor, 480 * g_flScaleFactor, 0.8, 0.88);
        SetWindowPos(subviewwnd->GetHWND(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        lvi->SetMemorySelected(true);
        lvi->SetOpenDirState(LVIODS_FULLSCREEN);
        Element* groupdirectory{};
        parserSubview->CreateElement(L"groupdirectory", nullptr, nullptr, nullptr, (Element**)&groupdirectory);
        CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
        fullscreeninner->Add((Element**)&groupdirectory, 1);
        CSafeElementPtr<DDScalableElement> iconElement;
        iconElement.Assign(regElem<DDScalableElement*>(L"iconElem", lvi));
        fullscreeninner->SetDDCPIntensity(iconElement->GetDDCPIntensity());
        fullscreeninner->SetAssociatedColor(iconElement->GetAssociatedColor());
        CSafeElementPtr<TouchScrollViewer> groupdirlist;
        groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory));
        CSafeElementPtr<DDScalableButton> lvi_SubUIContainer;
        lvi_SubUIContainer.Assign(regElem<DDScalableButton*>(L"SubUIContainer", groupdirlist));
        COLORREF colorPickerPalette[7] =
        {
            g_theme ? ImmersiveColorD : ImmersiveColorL,
            g_theme ? RGB(0, 103, 192) : RGB(76, 194, 255), g_theme ? RGB(158, 58, 176) : RGB(216, 141, 225),
            g_theme ? RGB(210, 14, 30) : RGB(244, 103, 98), g_theme ? RGB(224, 83, 7) : RGB(251, 154, 68),
            g_theme ? RGB(225, 157, 0) : RGB(255, 213, 42), g_theme ? RGB(0, 178, 90) : RGB(38, 255, 142)
        };
        if (lviCount > 0)
        {
            vector<IElementListener*> v_pels;
            vector<LVItem*>* subpm = new vector<LVItem*>;
            vector<DDScalableElement*>* subiconpm = new vector<DDScalableElement*>;
            vector<Element*>* subshadowpm = new vector<Element*>;
            vector<Element*>* subshortpm = new vector<Element*>;
            vector<RichText*>* subfilepm = new vector<RichText*>;
            StyleSheet* sheet = pSubview->GetSheet();
            CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
            parser->GetSheet(g_theme ? L"default" : L"defaultdark", &sheetStorage);
            const WCHAR* elemname = g_touchmode ? L"outerElemTouch" : L"outerElemGrouped";
            for (int i = 0; i < lviCount; i++)
            {
                LVItem* outerElemGrouped;
                parser->CreateElement(elemname, nullptr, nullptr, nullptr, (Element**)&outerElemGrouped);
                outerElemGrouped->SetValue(Element::SheetProp, 1, sheetStorage);
                lvi_SubUIContainer->Add((Element**)&outerElemGrouped, 1);
                CSafeElementPtr<DDScalableElement> iconElem;
                iconElem.Assign(regElem<DDScalableElement*>(L"iconElem", outerElemGrouped));
                CSafeElementPtr<Element> shortcutElem;
                shortcutElem.Assign(regElem<Element*>(L"shortcutElem", outerElemGrouped));
                CSafeElementPtr<Element> iconElemShadow;
                iconElemShadow.Assign(regElem<Element*>(L"iconElemShadow", outerElemGrouped));
                CSafeElementPtr<RichText> textElem;
                textElem.Assign(regElem<RichText*>(L"textElem", outerElemGrouped));
                subpm->push_back(outerElemGrouped);
                subiconpm->push_back(iconElem);
                subshortpm->push_back(shortcutElem);
                subshadowpm->push_back(iconElemShadow);
                subfilepm->push_back(textElem);
            }
            CSafeElementPtr<LVItem> PendingContainer;
            PendingContainer.Assign(regElem<LVItem*>(L"PendingContainer", groupdirectory));
            PendingContainer->SetVisible(true);
            EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), subpm, false, nullptr, &count2, lviCount);
            int x = 0, y = 0;
            int maxX{}, xRuns{};
            CValuePtr v;
            RECT dimensions;
            dimensions = *(groupdirectory->GetPadding(&v));
            int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 44) * g_flScaleFactor;
            int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 21) * g_flScaleFactor;
            if (g_touchmode)
            {
                outerSizeX = g_touchSizeX + DESKPADDING_TOUCH * g_flScaleFactor;
                outerSizeY = g_touchSizeY + DESKPADDING_TOUCH * g_flScaleFactor;
            }
            for (int j = 0; j < lviCount; j++)
            {
                if ((*subpm)[j]->GetHiddenState() == true)
                {
                    (*subiconpm)[j]->SetAlpha(128);
                    (*subshadowpm)[j]->SetAlpha(0);
                    (*subfilepm)[j]->SetAlpha(128);
                }
                v_pels.push_back(assignFn((*subpm)[j], SelectSubItem, true));
                v_pels.push_back(assignFn((*subpm)[j], ItemRightClick, true));
                v_pels.push_back(assignExtendedFn((*subpm)[j], SelectSubItemListener, true));
                v_pels.push_back(assignExtendedFn((*subpm)[j], ShowCheckboxIfNeeded, true));
                (*subpm)[j]->SetListeners(v_pels);
                v_pels.clear();
                if (!g_touchmode) (*subpm)[j]->SetClass(L"singleclicked");
                int xRender = (localeType == 1) ? (centered->GetWidth() - (dimensions.left + dimensions.right + outerSizeX)) - x : x;
                (*subpm)[j]->SetX(xRender), (*subpm)[j]->SetY(y);
                x += outerSizeX;
                xRuns++;
                if (x > centered->GetWidth() - (dimensions.left + dimensions.right + outerSizeX))
                {
                    maxX = xRuns;
                    xRuns = 0;
                    x = 0;
                    y += outerSizeY;
                }
            }
            x -= outerSizeX;
            if (maxX != 0 && xRuns % maxX != 0) y += outerSizeY;
            lvi_SubUIContainer->SetHeight(y);
            CSafeElementPtr<Element> dirtitle;
            dirtitle.Assign(regElem<Element*>(L"dirtitle", groupdirectory));
            groupdirlist->SetHeight(480 * g_flScaleFactor - (dirtitle->GetHeight() + dimensions.top + dimensions.bottom));
            for (int j = 0; j < lviCount; j++)
            {
                if (localeType == 1 && y > groupdirlist->GetHeight())
                    (*subpm)[j]->SetX((*subpm)[j]->GetX() - GetSystemMetricsForDpi(SM_CXVSCROLL, g_dpi));
            }
            lvi->SetChildItems(subpm);
            lvi->SetChildIcons(subiconpm);
            lvi->SetChildShadows(subshadowpm);
            lvi->SetChildShortcutArrows(subshortpm);
            lvi->SetChildFilenames(subfilepm);
            DWORD animThread2;
            yValueEx* yV = new yValueEx{ lviCount, NULL, NULL, subpm, subiconpm, subshadowpm, subshortpm, subfilepm, PendingContainer, lvi };
            HANDLE animThreadHandle2 = CreateThread(nullptr, 0, subfastin, (LPVOID)yV, 0, &animThread2);
            if (animThreadHandle2) CloseHandle(animThreadHandle2);
        }
        else
        {
            CSafeElementPtr<Element> emptyview;
            emptyview.Assign(regElem<Element*>(L"emptyview", groupdirectory));
            emptyview->SetVisible(true);
            CSafeElementPtr<DDScalableElement> emptygraphic;
            emptygraphic.Assign(regElem<DDScalableElement*>(L"emptygraphic", groupdirectory));
            if (iconElement->GetGroupColor() >= 1)
                emptygraphic->SetAssociatedColor(colorPickerPalette[iconElement->GetGroupColor() - 1]);
            CSafeElementPtr<Element> dirtitle;
            dirtitle.Assign(regElem<Element*>(L"dirtitle", groupdirectory));
            dirtitle->SetVisible(true);
        }
        CSafeElementPtr<DDScalableElement> dirname;
        dirname.Assign(regElem<DDScalableElement*>(L"dirname", groupdirectory));
        dirname->SetContentString(lvi->GetSimpleFilename().c_str());
        CSafeElementPtr<DDScalableElement> dirdetails;
        dirdetails.Assign(regElem<DDScalableElement*>(L"dirdetails", groupdirectory));
        WCHAR itemCount[64];
        if (lviCount == 1) StringCchPrintfW(itemCount, 64, LoadStrFromRes(4031).c_str());
        else StringCchPrintfW(itemCount, 64, LoadStrFromRes(4032).c_str(), lviCount);
        dirdetails->SetContentString(itemCount);
        if (lviCount == 0) dirdetails->SetContentString(L"");
        CSafeElementPtr<Element> tasks;
        tasks.Assign(regElem<Element*>(L"tasks", groupdirectory));
        g_checkifelemexists = true;
        CSafeElementPtr<DDLVActionButton> Pin;
        Pin.Assign(regElem<DDLVActionButton*>(L"Pin", groupdirectory));
        CSafeElementPtr<DDLVActionButton> Customize;
        Customize.Assign(regElem<DDLVActionButton*>(L"Customize", groupdirectory));
        CSafeElementPtr<DDLVActionButton> OpenInExplorer;
        OpenInExplorer.Assign(regElem<DDLVActionButton*>(L"OpenInExplorer", groupdirectory));
        Pin->SetVisible(true), Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
        assignFn(OpenInExplorer, OpenGroupInExplorer);
        assignFn(Customize, OpenCustomizePage);
        assignFn(Pin, PinGroup);
        OpenInExplorer->SetAssociatedItem(lvi);
        Customize->SetAssociatedItem(lvi);
        Pin->SetAssociatedItem(lvi);
    }

    void DisableColorPicker(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDColorPicker> DDCP_Icons;
            DDCP_Icons.Assign(regElem<DDColorPicker*>(L"DDCP_Icons", (Element*)g_tempElem));
            DDCP_Icons->SetEnabled(((DDToggleButton*)elem)->GetCheckedState());
        }
    }

    void DisableDarkToggle(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDToggleButton> EnableDarkIcons;
            EnableDarkIcons.Assign(regElem<DDToggleButton*>(L"EnableDarkIcons", (Element*)g_tempElem));
            if (((DDCheckBox*)elem)->GetCheckedState() == true)
            {
                EnableDarkIcons->SetCheckedState(!g_theme);
                g_isDarkIconsEnabled = !g_theme;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop\\Personalize", L"DarkIcons", !g_theme, false, nullptr);
            }
            EnableDarkIcons->SetEnabled(!((DDCheckBox*)elem)->GetCheckedState());
        }
    }

    void SetViewHelper(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDSlider> IconSize;
            IconSize.Assign(regElem<DDSlider*>(L"IconSize", elem->GetRoot()));
            int iconsize = IconSize->GetCurrentValue();
            int shortcutsize{}, smallsize{};
            shortcutsize = 32;
            if (iconsize > 96) shortcutsize = 64;
            else if (iconsize > 48) shortcutsize = 48;
            smallsize = 12;
            if (iconsize > 120) smallsize = 48;
            else if (iconsize > 80) smallsize = 32;
            else if (iconsize > 40) smallsize = 16;
            SetView(iconsize, shortcutsize, smallsize, false);
        }
    }

    void OpenDeskCpl(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click) ShellExecuteW(nullptr, L"open", L"control.exe", L"desk.cpl,Web,0", nullptr, SW_SHOW);
    }

    void OpenLog(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            wchar_t* desktoplog = new wchar_t[260];
            wchar_t* cBuffer = new wchar_t[260];
            DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
            StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
            ShellExecuteW(nullptr, L"open", L"notepad.exe", desktoplog, nullptr, SW_SHOW);
            delete[] desktoplog;
            delete[] cBuffer;
        }
    }

    void SetDefaultRes(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            RECT dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            g_defWidth = dimensions.right / g_flScaleFactor;
            SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"DefaultWidth", g_defWidth, false, nullptr);
            g_defHeight = dimensions.bottom / g_flScaleFactor;
            SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"DefaultHeight", g_defHeight, false, nullptr);
            elem->SetEnabled(false);
            CSafeElementPtr<DDScalableElement> CustomResDesc;
            CustomResDesc.Assign(regElem<DDScalableElement*>(L"CustomResDesc", elem->GetRoot()));
            WCHAR desc[256];
            StringCchPrintfW(desc, 256, L"Current default desktop area is %d x %d, likely %d x %d with %d dpi", g_defWidth, g_defHeight, static_cast<int>(ceil(g_defWidth * g_flScaleFactor)), static_cast<int>(ceil(g_defHeight * g_flScaleFactor)), g_dpi);
            CustomResDesc->SetContentString(desc);
        }
    }

    void ResetDesktopSize(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            WCHAR DesktopLayoutWithSize[24];
            if (!g_touchmode) StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_%d", g_iconsz);
            else StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_Touch");
            if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize))
            {
                RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize);
                InitLayout(true, false, false);
            }
        }
    }

    void ShowPage1(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            PageTab1->SetSelected(true);
            PageTab2->SetSelected(false);
            PageTab3->SetSelected(false);

            Element* SettingsPage1{};
            TriggerTabbedPageTransition(1, SettingsPage1, L"SettingsPage1", SubUIContainer);

            if (SettingsPage1)
            {
                CSafeElementPtr<DDToggleButton> ItemCheckboxes;
                ItemCheckboxes.Assign(regElem<DDToggleButton*>(L"ItemCheckboxes", SettingsPage1));
                CSafeElementPtr<DDToggleButton> ShowHiddenFiles;
                ShowHiddenFiles.Assign(regElem<DDToggleButton*>(L"ShowHiddenFiles", SettingsPage1));
                CSafeElementPtr<DDToggleButton> FilenameExts;
                FilenameExts.Assign(regElem<DDToggleButton*>(L"FilenameExts", SettingsPage1));
                CSafeElementPtr<DDToggleButton> TreatDirAsGroup;
                TreatDirAsGroup.Assign(regElem<DDToggleButton*>(L"TreatDirAsGroup", SettingsPage1));
                CSafeElementPtr<DDToggleButton> TripleClickAndHide;
                TripleClickAndHide.Assign(regElem<DDToggleButton*>(L"TripleClickAndHide", SettingsPage1));
                CSafeElementPtr<DDToggleButton> LockIconPos;
                LockIconPos.Assign(regElem<DDToggleButton*>(L"LockIconPos", SettingsPage1));
                RegKeyValue rkvTemp{};
                rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
                rkvTemp._valueToFind = L"AutoCheckSelect";
                ItemCheckboxes->SetCheckedState(g_showcheckboxes);
                ItemCheckboxes->SetAssociatedBool(&g_showcheckboxes);
                ItemCheckboxes->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"Hidden";
                if (GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind) == 1) ShowHiddenFiles->SetCheckedState(true);
                else ShowHiddenFiles->SetCheckedState(false);
                ShowHiddenFiles->SetAssociatedFn(InitLayout, false, false, true);
                ShowHiddenFiles->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"HideFileExt";
                FilenameExts->SetCheckedState(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
                FilenameExts->SetAssociatedFn(InitLayout, false, false, true);
                FilenameExts->SetRegKeyValue(rkvTemp);
                rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"TreatDirAsGroup";
                TreatDirAsGroup->SetCheckedState(g_treatdirasgroup);
                TreatDirAsGroup->SetAssociatedBool(&g_treatdirasgroup);
                TreatDirAsGroup->SetAssociatedFn(InitLayout, false, false, true);
                TreatDirAsGroup->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"TripleClickAndHide";
                TripleClickAndHide->SetCheckedState(g_tripleclickandhide);
                TripleClickAndHide->SetAssociatedBool(&g_tripleclickandhide);
                TripleClickAndHide->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"LockIconPos";
                LockIconPos->SetCheckedState(g_lockiconpos);
                LockIconPos->SetAssociatedBool(&g_lockiconpos);
                LockIconPos->SetRegKeyValue(rkvTemp);
                assignFn(ItemCheckboxes, ToggleSetting);
                assignFn(ShowHiddenFiles, ToggleSetting);
                assignFn(FilenameExts, ToggleSetting);
                assignFn(TreatDirAsGroup, ToggleSetting);
                assignFn(TripleClickAndHide, ToggleSetting);
                assignFn(LockIconPos, ToggleSetting);
            }
        }
    }

    void ShowPage2(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            PageTab1->SetSelected(false);
            PageTab2->SetSelected(true);
            PageTab3->SetSelected(false);

            Element* SettingsPage2{};
            TriggerTabbedPageTransition(2, SettingsPage2, L"SettingsPage2", SubUIContainer);

            if (SettingsPage2)
            {
                CSafeElementPtr<DDToggleButton> EnableAccent;
                EnableAccent.Assign(regElem<DDToggleButton*>(L"EnableAccent", SettingsPage2));
                CSafeElementPtr<DDColorPicker> DDCP_Icons;
                DDCP_Icons.Assign(regElem<DDColorPicker*>(L"DDCP_Icons", SettingsPage2));
                CSafeElementPtr<DDToggleButton> EnableDarkIcons;
                EnableDarkIcons.Assign(regElem<DDToggleButton*>(L"EnableDarkIcons", SettingsPage2));
                CSafeElementPtr<DDCheckBox> AutoDarkIcons;
                AutoDarkIcons.Assign(regElem<DDCheckBox*>(L"AutoDarkIcons", SettingsPage2));
                CSafeElementPtr<DDSlider> IconSize;
                IconSize.Assign(regElem<DDSlider*>(L"IconSize", SettingsPage2));
                CSafeElementPtr<DDScalableButton> ApplyIconSize;
                ApplyIconSize.Assign(regElem<DDScalableButton*>(L"ApplyIconSize", SettingsPage2));
                CSafeElementPtr<DDToggleButton> IconThumbnails;
                IconThumbnails.Assign(regElem<DDToggleButton*>(L"IconThumbnails", SettingsPage2));
                CSafeElementPtr<DDScalableButton> DesktopIconSettings;
                DesktopIconSettings.Assign(regElem<DDScalableButton*>(L"DesktopIconSettings", SettingsPage2));
                RegKeyValue rkvTemp{};
                rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop\\Personalize", rkvTemp._valueToFind = L"AccentColorIcons";
                EnableAccent->SetCheckedState(g_isColorized);
                EnableAccent->SetAssociatedBool(&g_isColorized);
                EnableAccent->SetAssociatedFn(RearrangeIcons, false, true, true);
                EnableAccent->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"IconColorID";
                DDCP_Icons->SetThemeAwareness(false);
                DDCP_Icons->SetEnabled(g_isColorized);
                DDCP_Icons->SetRegKeyValue(rkvTemp);
                vector<DDScalableElement*> elemTargets{};
                elemTargets.push_back(RegistryListener);
                DDCP_Icons->SetTargetElements(elemTargets);
                elemTargets.clear();
                rkvTemp._valueToFind = L"DarkIcons";
                EnableDarkIcons->SetEnabled(!g_automaticDark);
                EnableDarkIcons->SetCheckedState(g_isDarkIconsEnabled);
                EnableDarkIcons->SetAssociatedBool(&g_isDarkIconsEnabled);
                EnableDarkIcons->SetAssociatedFn(RearrangeIcons, false, true, true);
                EnableDarkIcons->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"AutoDarkIcons";
                AutoDarkIcons->SetCheckedState(g_automaticDark);
                AutoDarkIcons->SetAssociatedBool(&g_automaticDark);
                AutoDarkIcons->SetAssociatedFn(RearrangeIcons, false, true, true);
                AutoDarkIcons->SetRegKeyValue(rkvTemp);
                IconSize->SetMinValue(32);
                IconSize->SetMaxValue(144);
                IconSize->SetCurrentValue(g_iconsz, false);
                IconSize->SetFormattedString(L"%.0f");
                IconSize->SetEnabled(!g_touchmode);
                ApplyIconSize->SetEnabled(!g_touchmode);
                rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", rkvTemp._valueToFind = L"IconsOnly";
                IconThumbnails->SetCheckedState(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
                IconThumbnails->SetAssociatedFn(RearrangeIcons, false, true, true);
                IconThumbnails->SetRegKeyValue(rkvTemp);
                assignFn(EnableAccent, ToggleSetting);
                assignFn(EnableAccent, DisableColorPicker);
                assignFn(EnableDarkIcons, ToggleSetting);
                assignFn(AutoDarkIcons, ToggleSetting);
                assignFn(AutoDarkIcons, DisableDarkToggle);
                assignFn(ApplyIconSize, SetViewHelper);
                assignFn(IconThumbnails, ToggleSetting);
                assignFn(DesktopIconSettings, OpenDeskCpl);
                g_tempElem = (void*)SettingsPage2;
            }
        }
    }

    void ShowPage3(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            PageTab1->SetSelected(false);
            PageTab2->SetSelected(false);
            PageTab3->SetSelected(true);

            Element* SettingsPage3{};
            TriggerTabbedPageTransition(3, SettingsPage3, L"SettingsPage3", SubUIContainer);

            if (SettingsPage3)
            {
                CSafeElementPtr<DDToggleButton> EnableLogging;
                EnableLogging.Assign(regElem<DDToggleButton*>(L"EnableLogging", SettingsPage3));
                CSafeElementPtr<DDScalableButton> ViewLastLog;
                ViewLastLog.Assign(regElem<DDScalableButton*>(L"ViewLastLog", SettingsPage3));
                CSafeElementPtr<DDSlider> AnimSpeed;
                AnimSpeed.Assign(regElem<DDSlider*>(L"AnimSpeed", SettingsPage3));
                CSafeElementPtr<DDCheckBox> AnimShiftKey;
                AnimShiftKey.Assign(regElem<DDCheckBox*>(L"AnimShiftKey", SettingsPage3));
                CSafeElementPtr<DDToggleButton> ShowDbgInfo;
                ShowDbgInfo.Assign(regElem<DDToggleButton*>(L"ShowDbgInfo", SettingsPage3));
                CSafeElementPtr<DDScalableElement> CustomResDesc;
                CustomResDesc.Assign(regElem<DDScalableElement*>(L"CustomResDesc", SettingsPage3));
                CSafeElementPtr<DDScalableButton> SetCurrent;
                SetCurrent.Assign(regElem<DDScalableButton*>(L"SetCurrent", SettingsPage3));
                CSafeElementPtr<DDScalableButton> ResetDesktop;
                ResetDesktop.Assign(regElem<DDScalableButton*>(L"ResetDesktop", SettingsPage3));
                RegKeyValue rkvTemp{};
                rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop\\Debug", rkvTemp._valueToFind = L"Logging";
                EnableLogging->SetCheckedState(7 - GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
                EnableLogging->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"AnimationSpeed";
                AnimSpeed->SetMinValue(0.5f);
                AnimSpeed->SetMaxValue(20.0f);
                AnimSpeed->SetCurrentValue(g_animCoef / 100.0f, false);
                AnimSpeed->SetAssociatedValue((int*)&g_animCoef, 100);
                AnimSpeed->SetFormattedString(L"%.2f");
                AnimSpeed->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"AnimationsShiftKey";
                AnimShiftKey->SetCheckedState(g_AnimShiftKey);
                AnimShiftKey->SetAssociatedBool(&g_AnimShiftKey);
                AnimShiftKey->SetRegKeyValue(rkvTemp);
                rkvTemp._valueToFind = L"ShowDebugInfo";
                ShowDbgInfo->SetCheckedState(g_debuginfo);
                ShowDbgInfo->SetAssociatedBool(&g_debuginfo);
                ShowDbgInfo->SetAssociatedFn(ShowDebugInfoOnDesktop, false, false, false);
                ShowDbgInfo->SetRegKeyValue(rkvTemp);
                WCHAR desc[256];
                StringCchPrintfW(desc, 256, L"Current default desktop area is %d x %d, likely %d x %d with %d dpi", g_defWidth, g_defHeight, static_cast<int>(ceil(g_defWidth * g_flScaleFactor)), static_cast<int>(ceil(g_defHeight * g_flScaleFactor)), g_dpi);
                CustomResDesc->SetContentString(desc);
                SetCurrent->SetEnabled(!isDefaultRes());
                assignFn(EnableLogging, ToggleSetting);
                assignFn(ViewLastLog, OpenLog);
                assignFn(AnimShiftKey, ToggleSetting);
                assignFn(ShowDbgInfo, ToggleSetting);
                assignFn(SetCurrent, SetDefaultRes);
                assignFn(ResetDesktop, ResetDesktopSize);
            }
        }
    }

    void ShowSettings(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            g_editmode = false;
            fullscreenAnimation(800 * g_flScaleFactor, 480 * g_flScaleFactor, 0.8, 0.88);
            SetWindowPos(subviewwnd->GetHWND(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            g_issubviewopen = true;
            g_issettingsopen = true;
            Element* settingsview{};
            parserSubview->CreateElement(L"settingsview", nullptr, nullptr, nullptr, (Element**)&settingsview);
            CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
            fullscreeninner->Add((Element**)&settingsview, 1);
            CSafeElementPtr<TouchScrollViewer> settingslist;
            settingslist.Assign(regElem<TouchScrollViewer*>(L"settingslist", settingsview));
            SubUIContainer = regElem<DDScalableButton*>(L"SubUIContainer", settingsview);
            PageTab1 = regElem<DDScalableButton*>(L"PageTab1", settingsview);
            PageTab2 = regElem<DDScalableButton*>(L"PageTab2", settingsview);
            PageTab3 = regElem<DDScalableButton*>(L"PageTab3", settingsview);
            assignFn(PageTab1, ShowPage1);
            assignFn(PageTab2, ShowPage2);
            assignFn(PageTab3, ShowPage3);
            PageTab3->SetVisible(g_debugmode);
            g_settingsPageID = 1;

            ShowPage1(elem, iev);

            CValuePtr v;
            RECT dimensions;
            dimensions = *(settingsview->GetPadding(&v));
            CSafeElementPtr<DDScalableRichText> title;
            title.Assign(regElem<DDScalableRichText*>(L"title", settingsview));
            settingslist->SetWidth((800 * g_flScaleFactor - (dimensions.left + dimensions.right)));
            settingslist->SetHeight((480 * g_flScaleFactor - (title->GetHeight() + dimensions.top + dimensions.bottom)));
            settingslist->SetVisible(true);

            CSafeElementPtr<DDScalableRichText> name;
            name.Assign(regElem<DDScalableRichText*>(L"name", settingsview));

            GTRANS_DESC transDesc[3];
            TriggerTranslate(settingslist, transDesc, 0, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false);
            TriggerFade(settingslist, transDesc, 1, 0.2f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerClip(settingslist, transDesc, 2, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 0.0f, 1.0f, ((settingslist->GetHeight() - 100 * g_flScaleFactor) / settingslist->GetHeight()), 0.0f, 0.0f, 1.0f, 1.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, settingslist->GetDisplayNode(), &tsbInfo);

            g_checkifelemexists = true;
        }
    }

    void InitSubview()
    {
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);

        NativeHWNDHost::Create(L"DD_SubviewHost", L"DirectDesktop Subview", nullptr, nullptr, dimensions.left, dimensions.top, 9999, 9999, WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, WS_POPUP, nullptr, 0x43, &subviewwnd);
        DUIXmlParser::Create(&parserSubview, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        parserSubview->SetXMLFromResource(IDR_UIFILE3, HINST_THISCOMPONENT, HINST_THISCOMPONENT);
        HWNDElement::Create(subviewwnd->GetHWND(), true, 0x38, nullptr, &key2, (Element**)&subviewparent);
        WndProcSubview = (WNDPROC)SetWindowLongPtrW(subviewwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)TopLevelWindowProc);

        parserSubview->CreateElement(L"fullscreenpopup", subviewparent, nullptr, nullptr, &pSubview);
        pSubview->SetVisible(true);
        pSubview->EndDefer(key2);

        fullscreenpopupbg = regElem<Element*>(L"fullscreenpopupbg", pSubview);
        fullscreenpopupbase = regElem<Button*>(L"fullscreenpopupbase", pSubview);
        popupcontainer = regElem<Button*>(L"popupcontainer", pSubview);
        centered = regElem<Button*>(L"centered", pSubview);

        assignFn(fullscreenpopupbase, testEventListener3);

        subviewwnd->Host(pSubview);
        MARGINS m = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(subviewwnd->GetHWND(), &m);
    }
}
