#include "pch.h"

#include "EditMode.h"
#include "..\DirectDesktop.h"
#include "..\coreui\BitmapHelper.h"
#include "..\coreui\StyleModifier.h"
#include "ShutdownDialog.h"
#include "..\backend\SettingsHelper.h"
#include "SearchPage.h"
#include "..\backend\DirectoryHelper.h"

using namespace DirectUI;

namespace DirectDesktop
{   
    NativeHWNDHost *editwnd, *editbgwnd;
    HWNDElement *editparent, *editbgparent;
    DUIXmlParser* parserEdit;
    Element *pEdit, *pEditBG;
    unsigned long key5 = 0, key6 = 0;
    WNDPROC WndProcEdit, WndProcEditBG;
    DDScalableTouchButton* fullscreeninnerE;
    Element* popupcontainerE;
    Element* fullscreenpopupbaseE;
    Element* centeredE;
    TouchButton* centeredEBG;
    DDScalableElement* simpleviewoverlay;
    DDScalableElement* deskpreviewmask;
    Element *SimpleViewTop, *SimpleViewBottom;
    Element *SimpleViewTopInner, *SimpleViewBottomInner;
    TouchButton* SimpleViewPower, *SimpleViewSearch;
    DDIconButton* SimpleViewSettings, *SimpleViewPages, *SimpleViewClose;
    DDScalableTouchButton *nextpage, *prevpage;
    DDScalableRichText* pageinfo;
    Element* PageViewer;
    DDScalableTouchEdit* PV_EnterPage;
    Element* EM_Dim;
    Element* bg_left_top, *bg_left_middle, *bg_left_bottom, *bg_right_top, *bg_right_middle, *bg_right_bottom;

    HANDLE g_editSemaphore = CreateSemaphoreW(nullptr, 16, 16, nullptr);
    LPVOID timerPtr;

    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void ClosePageViewer(Element* elem, Event* iev);
    void ShowPageViewer(Element* elem, Event* iev);
    void TriggerHSV(Element* elem, Event* iev);
    void RemoveSelectedPage(Element* elem, Event* iev);
    void SetSelectedPageHome(Element* elem, Event* iev);
    void CreatePagePreview();
    void PV_SetEnterPageDesc();
    void _UpdateSimpleViewContent(bool animate, DWORD animFlags);
    bool ValidateStrDigits(const WCHAR* str);
    bool g_animatePVEnter = true;
    bool g_editingpages = false;

    LRESULT CALLBACK EditModeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_SIZE:
                SetTimer(hWnd, 3, 150, nullptr);
                break;
            case WM_CLOSE:
                HideSimpleView(true);
                return 0;
            case WM_DESTROY:
                return 0;
            case WM_TIMER:
            {
                KillTimer(hWnd, wParam);
                switch (wParam)
                {
                    case 1:
                        if (timerPtr)
                        {
                            CValuePtr v;
                            int removedPage{};
                            if (((DDLVActionButton*)timerPtr)->GetAssociatedItem())
                            {
                                removedPage = ((DDLVActionButton*)timerPtr)->GetAssociatedItem()->GetPage();
                            }
                            else
                            {
                                if (PV_EnterPage)
                                {
                                    if (PV_EnterPage->GetContentString(&v) != nullptr)
                                        removedPage = _wtoi(PV_EnterPage->GetContentString(&v));
                                    if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || removedPage < 1 || removedPage > g_maxPageID)
                                    {
                                        MessageBeep(MB_OK);
                                        WCHAR* errorcontent = new WCHAR[256];
                                        StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), g_maxPageID);
                                        CSafeElementPtr<DDNotificationBanner> ddnb;
                                        ddnb.Assign(new DDNotificationBanner);
                                        ddnb->CreateBanner(DDNT_ERROR, LoadStrFromRes(4060).c_str(), errorcontent, 5);
                                        delete[] errorcontent;
                                        return 0;
                                    }
                                    for (int i = 0; i <= g_maxPageID; i++)
                                    {
                                        int items = 0;
                                        for (int j = 0; j < pm.size(); j++)
                                        {
                                            if (pm[j]->GetPage() != i) continue;
                                            items++;
                                        }
                                        if (items != 0 && i == removedPage)
                                        {
                                            MessageBeep(MB_OK);
                                            CSafeElementPtr<DDNotificationBanner> ddnb;
                                            ddnb.Assign(new DDNotificationBanner);
                                            ddnb->CreateBanner(DDNT_INFO, LoadStrFromRes(4062).c_str(), LoadStrFromRes(4063).c_str(), 5);
                                            return 0;
                                        }
                                    }
                                }
                                else
                                {
                                    MessageBeep(MB_OK);
                                    CSafeElementPtr<DDNotificationBanner> ddnb;
                                    ddnb.Assign(new DDNotificationBanner);
                                    ddnb->CreateBanner(DDNT_ERROR, nullptr, nullptr, 3);
                                    return 0;
                                }
                            }
                            if (removedPage == g_homePageID || (removedPage < g_maxPageID && removedPage < g_homePageID))
                                g_homePageID--;
                            if (g_homePageID < 1) g_homePageID = 1;
                            if (removedPage == g_currentPageID || (removedPage < g_maxPageID && removedPage < g_currentPageID))
                                g_currentPageID--;
                            if (g_currentPageID < 1) g_currentPageID = 1;
                            if (removedPage < g_maxPageID)
                            {
                                for (int i = removedPage + 1; i <= g_maxPageID; i++)
                                {
                                    for (int j = 0; j < pm.size(); j++)
                                    {
                                        if (pm[j]->GetPage() != i) continue;
                                        pm[j]->SetPage(i - 1);
                                    }
                                }
                            }
                            g_maxPageID--;
                            g_animatePVEnter = false;
                            SetPos(isDefaultRes());
                            if (g_maxPageID <= 6)
                            {
                                PageViewer->DestroyAll(true);
                                PageViewer->Destroy(true);
                                Event* iev = new Event{ PageViewer, TouchButton::Click };
                                ShowPageViewer(PageViewer, iev);
                            }
                        }
                        if (g_maxPageID >= 7)
                            PV_SetEnterPageDesc();
                        g_editingpages = false;
                        break;
                    case 2:
                        if (timerPtr)
                        {
                            CValuePtr v;
                            int page{};
                            if (((DDLVActionButton*)timerPtr)->GetAssociatedItem())
                            {
                                page = ((DDLVActionButton*)timerPtr)->GetAssociatedItem()->GetPage();
                            }
                            else
                            {
                                if (PV_EnterPage)
                                {
                                    if (PV_EnterPage->GetContentString(&v) != nullptr) page = _wtoi(PV_EnterPage->GetContentString(&v));
                                    if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || page < 1 || page > g_maxPageID)
                                    {
                                        MessageBeep(MB_OK);
                                        WCHAR* errorcontent = new WCHAR[256];
                                        StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), g_maxPageID);
                                        CSafeElementPtr<DDNotificationBanner> ddnb;
                                        ddnb.Assign(new DDNotificationBanner);
                                        ddnb->CreateBanner(DDNT_ERROR, LoadStrFromRes(4060).c_str(), errorcontent, 5);
                                        delete[] errorcontent;
                                        return 0;
                                    }
                                }
                                else
                                {
                                    MessageBeep(MB_OK);
                                    CSafeElementPtr<DDNotificationBanner> ddnb;
                                    ddnb.Assign(new DDNotificationBanner);
                                    ddnb->CreateBanner(DDNT_ERROR, nullptr, nullptr, 3);
                                    return 0;
                                }
                            }
                            g_homePageID = page;
                            g_animatePVEnter = false;
                            SetPos(isDefaultRes());
                            if (g_maxPageID <= 6)
                            {
                                SetPos(isDefaultRes());
                                PageViewer->DestroyAll(true);
                                PageViewer->Destroy(true);
                                Event* iev = new Event{ PageViewer, TouchButton::Click };
                                ShowPageViewer(PageViewer, iev);
                            }
                        }
                        g_editingpages = false;
                        break;
                    case 3: 
                        RefreshSimpleView(0x0);
                        break;
                    case 4:
                        g_animatePVEnter = false;
                        g_maxPageID++;
                        SetPos(isDefaultRes());
                        if (g_maxPageID <= 7)
                        {
                            PageViewer->DestroyAll(true);
                            PageViewer->Destroy(true);
                            Event* iev = new Event{ PageViewer, TouchButton::Click };
                            ShowPageViewer(PageViewer, iev);
                        }
                        if (g_maxPageID >= 7)
                            PV_SetEnterPageDesc();
                        g_editingpages = false;
                        break;
                    case 5:
                        CreatePagePreview();
                        break;
                }
                break;
            }
            case WM_USER + 1:
            {
                DesktopIcon* di = (DesktopIcon*)wParam;
                yValueEx* yV = (yValueEx*)lParam;
                DDScalableElement* PV_IconShadowPreview{};
                DDScalableElement* PV_IconPreview{};
                Element* PV_IconShortcutPreview;
                const WCHAR* iconshadow = g_touchmode ? L"PV_IconShadowTouchPreview" : L"PV_IconShadowPreview";
                const WCHAR* icon = g_touchmode ? L"PV_IconTouchPreview" : L"PV_IconPreview";
                const WCHAR* iconshortcut = g_touchmode ? L"PV_IconShortcutTouchPreview" : L"PV_IconShortcutPreview";
                parserEdit->CreateElement(iconshadow, nullptr, nullptr, nullptr, (Element**)&PV_IconShadowPreview);
                parserEdit->CreateElement(icon, nullptr, nullptr, nullptr, (Element**)&PV_IconPreview);
                parserEdit->CreateElement(iconshortcut, nullptr, nullptr, nullptr, &PV_IconShortcutPreview);
                CSafeElementPtr<Element> pePreviewContainer;
                pePreviewContainer.Assign(yV->peOptionalTarget1);
                pePreviewContainer->Insert((Element**)&PV_IconShadowPreview, 1, 0);
                pePreviewContainer->Insert((Element**)&PV_IconPreview, 1, 0);
                pePreviewContainer->Insert(&PV_IconShortcutPreview, 1, 0);
                DDScalableElement* peIcon = pm[yV->num]->GetIcon();
                Element* peShortcutArrow = pm[yV->num]->GetShortcutArrow();
                DWORD lviFlags = pm[yV->num]->GetFlags();
                if (lviFlags & LVIF_HIDDEN)
                {
                    PV_IconShadowPreview->SetAlpha(192);
                    PV_IconPreview->SetAlpha(128);
                }
                if (g_touchmode)
                {
                    PV_IconShadowPreview->SetX(pm[yV->num]->GetX() * yV->fl1);
                    PV_IconPreview->SetX(pm[yV->num]->GetX() * yV->fl1);

                    PV_IconShadowPreview->SetY(pm[yV->num]->GetY() * yV->fl1);
                    PV_IconPreview->SetY(pm[yV->num]->GetY() * yV->fl1);

                    PV_IconShadowPreview->SetWidth(pm[yV->num]->GetWidth() * yV->fl1);
                    PV_IconPreview->SetWidth(pm[yV->num]->GetWidth() * yV->fl1);
                    PV_IconShortcutPreview->SetWidth(g_iconsz * g_flScaleFactor * yV->fl1);
                    PV_IconShadowPreview->SetHeight(pm[yV->num]->GetHeight() * yV->fl1);
                    PV_IconPreview->SetHeight(pm[yV->num]->GetHeight() * yV->fl1);
                    PV_IconShortcutPreview->SetHeight(g_iconsz * g_flScaleFactor * yV->fl1);

                    PV_IconShadowPreview->SetDDCPIntensity(pm[yV->num]->GetDDCPIntensity());
                    PV_IconShadowPreview->SetAssociatedColor(pm[yV->num]->GetAssociatedColor());
                    PV_IconPreview->SetAssociatedColor(pm[yV->num]->GetInnerElement()->GetAssociatedColor());
                    PV_IconShortcutPreview->SetX(PV_IconPreview->GetX() + (PV_IconPreview->GetWidth() - PV_IconShortcutPreview->GetWidth()) / 2.0);
                    PV_IconShortcutPreview->SetY(PV_IconPreview->GetY() + (PV_IconPreview->GetHeight() - PV_IconShortcutPreview->GetHeight()) / 2.0);
                }
                else
                {
                    PV_IconPreview->SetX((pm[yV->num]->GetX() + peIcon->GetX()) * yV->fl1);
                    PV_IconShortcutPreview->SetX((pm[yV->num]->GetX() + peShortcutArrow->GetX()) * yV->fl1);
                    PV_IconPreview->SetY((pm[yV->num]->GetY() + peIcon->GetY()) * yV->fl1);
                    PV_IconShortcutPreview->SetY((pm[yV->num]->GetY() + peShortcutArrow->GetY()) * yV->fl1);
                    PV_IconPreview->SetWidth(peIcon->GetWidth() * yV->fl1);
                    PV_IconPreview->SetHeight(peIcon->GetHeight() * yV->fl1);
                }
                if (g_treatdirasgroup && lviFlags & LVIF_GROUP)
                {
                    if (!g_touchmode)
                    {
                        if (PV_IconPreview->GetWidth() < 16 && PV_IconPreview->GetHeight() < 16) PV_IconPreview->SetBorderThickness(0, 0, 0, 0);
                        PV_IconPreview->SetClass(L"groupthumbnail");
                        PV_IconPreview->SetDDCPIntensity(peIcon->GetDDCPIntensity());
                        PV_IconPreview->SetAssociatedColor(peIcon->GetAssociatedColor());
                    }
                    int foldericonsize = (pm[yV->num]->GetTileSize() == LVITS_ICONONLY) ? 32 : g_iconsz;
                    CSafeElementPtr<Element> PV_FolderGroup;
                    PV_FolderGroup.Assign(regElem<Element*>(L"PV_FolderGroup", PV_IconPreview));
                    if (peIcon->GetGroupColor() == 0)
                    {
                        if (g_isColorized)
                            PV_FolderGroup->SetForegroundColor(g_colorPickerPalette[iconColorID]);
                        else PV_FolderGroup->SetForegroundColor(g_colorPickerPalette[1]);
                    }
                    else PV_FolderGroup->SetForegroundColor(g_colorPickerPalette[peIcon->GetGroupColor()]);
                    PV_FolderGroup->SetVisible(true);
                    int glyphiconsize = min(PV_IconPreview->GetWidth(), PV_IconPreview->GetHeight());
                    float sizeCoef = (log(glyphiconsize / (yV->fl1 * g_iconsz * g_flScaleFactor)) / log(100)) + 1;
                    PV_FolderGroup->SetFontSize(g_touchmode ? static_cast<int>(foldericonsize * g_flScaleFactor * yV->fl1) : static_cast<int>(glyphiconsize / (2.0f * sizeCoef)));
                }
                HBITMAP iconbmp = di->icon;
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconbmp);
                if (spvBitmap != nullptr) PV_IconPreview->SetValue(Element::ContentProp, 1, spvBitmap);
                HBITMAP iconshortcutbmp = di->iconshortcut;
                CValuePtr spvBitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshortcutbmp);
                if (spvBitmapShortcut != nullptr && lviFlags & LVIF_SHORTCUT) PV_IconShortcutPreview->SetValue(Element::ContentProp, 1, spvBitmapShortcut);
                break;
            }
        }
        return CallWindowProc(WndProcEdit, hWnd, uMsg, wParam, lParam);
    }

    //LRESULT CALLBACK EditModeBGWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    //{
    //    switch (uMsg)
    //    {
    //        case WM_WINDOWPOSCHANGING:
    //        {
    //            //((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
    //            return 0;
    //        }
    //        case WM_CLOSE:
    //            return 0;
    //        case WM_DESTROY:
    //            return 0;
    //    }
    //    return CallWindowProc(WndProcEditBG, hWnd, uMsg, wParam, lParam);
    //}

    void SetTransElementPosition(Element* pe, int x, int y, int cx, int cy)
    {
        pe->SetX(x);
        pe->SetY(y);
        pe->SetWidth(cx);
        pe->SetHeight(cy);
        pe->SetVisible(true);
    }
    void EM_CreateDimRect(Element*& pe, Element*& peParent, int x, int y, int cx, int cy)
    {
        Element::Create(0, peParent, nullptr, &pe);
        peParent->Add(&pe, 1);
        pe->SetLayoutPos(-2);
        pe->SetClass(L"popupbg");
        SetTransElementPosition(pe, x, y, cx, cy);
    }
    void PV_CreateDimRect(Element* peParent, int cx, int cy)
    {
        Element* PV_PageRow_Dim2{};
        parserEdit->CreateElement(L"PV_PageRow_Dim", nullptr, nullptr, nullptr, &PV_PageRow_Dim2);
        PV_PageRow_Dim2->SetWidth(cx);
        PV_PageRow_Dim2->SetHeight(cy);
        peParent->Add(&PV_PageRow_Dim2, 1);
        peParent->SetWidth(peParent->GetWidth() + cx);
    }
    float EM_GetRectAniWScale(Element* pe)
    {
        return (pe->GetWidth() + centeredE->GetWidth()) / static_cast<float>(pe->GetWidth());
    }
    void PV_SetEnterPageDesc()
    {
        CSafeElementPtr<DDScalableRichText> PV_EnterPageDesc;
        PV_EnterPageDesc.Assign(regElem<DDScalableRichText*>(L"PV_EnterPageDesc", PageViewer));
        WCHAR* desccontent = new WCHAR[256];
        StringCchPrintfW(desccontent, 256, LoadStrFromRes(4061).c_str(), g_maxPageID);
        PV_EnterPageDesc->SetContentString(desccontent);
        delete[] desccontent;
    }

    DWORD WINAPI CreateDesktopPreview(LPVOID lpParam)
    {
        yValueEx* yV = (yValueEx*)lpParam;
        DesktopIcon di;
        if (!g_hiddenIcons && yV->num >= 0 && yV->peOptionalTarget1)
        {
            DWORD lviFlags = pm[yV->num]->GetFlags();
            if (lviFlags & LVIF_ADVANCEDICON)
                HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            ApplyIcons(&pm, &di, false, yV->num, yV->fl1, -1);
            SendMessageW(editwnd->GetHWND(), WM_USER + 1, (WPARAM)&di, (LPARAM)yV);
            if (lviFlags & LVIF_ADVANCEDICON)
                CoUninitialize();
        }
        Sleep(250);
        free(yV);
        return 0;
    }

    DWORD WINAPI CreateDesktopPreviewHelper(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        yValueEx* yV = static_cast<yValueEx*>(lpParam);
        CreateDesktopPreview(yV);
        ReleaseSemaphore(g_editSemaphore, 1, nullptr);
        UnInitThread();
        return 0;
    }

    bool ValidateStrDigits(const WCHAR* str)
    {
        if (!str || *str == L'\0') return false;
        while (*str)
        {
            if (!iswdigit(*str)) return false;
            ++str;
        }
        return true;
    }

    DWORD WINAPI animate7(LPVOID lpParam)
    {
        DWORD animCoef = g_animCoef;
        if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        Sleep(400 * (animCoef / 100.0f));
        //pEdit->DestroyAll(true);
        editwnd->DestroyWindow();
        //pEditBG->DestroyAll(true);
        //editbgwnd->DestroyWindow();
        return 0;
    }

    void fullscreenAnimation3(int width, int height)
    {
        parserEdit->CreateElement(L"fullscreeninner", nullptr, nullptr, nullptr, (Element**)&fullscreeninnerE);
        centeredE->Add((Element**)&fullscreeninnerE, 1);
        SetPopupSize(centeredE, width, height);
        //SetPopupSize(fullscreeninnerE, width, height);
        fullscreenpopupbaseE->SetVisible(true);
        fullscreeninnerE->SetVisible(true);
        static IElementListener* pel;
        free(pel);
        pel = assignFn(fullscreeninnerE, TriggerHSV, true);
    }

    void fullscreenAnimation4()
    {
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(nullptr, 0, animate7, nullptr, 0, &animThread);
        if (animThreadHandle) CloseHandle(animThreadHandle);
    }

    void HideSimpleView(bool fullanimate)
    {
        if (g_editmode)
        {
            g_editmode = false;
            g_invokedpagechange = false;
            if (g_touchmode) g_iconsz = 32;
            UIContainer->SetVisible(true);
            if (fullanimate) SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
            if (!fullscreenpopupbaseE->IsDestroyed())
            {
                GTRANS_DESC transDesc[8];
                TransitionStoryboardInfo tsbInfo = {};
                float scaleFinal = fullanimate ? 1.0f : 0.92f;
                float scaleFinal2 = fullanimate ? 1.4285f : 1.3143f;
                float timeCoef = fullanimate ? 1.0f : 1.5f;
                float delay = fullanimate ? 0.0f : 0.033f;
                TriggerFade(UIContainer, transDesc, 0, delay, delay + 0.167f * timeCoef, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                TriggerScaleOut(UIContainer, transDesc, 1, delay, delay + 0.33f * timeCoef, 0.1f, 0.9f, 0.2f, 1.0f, scaleFinal, scaleFinal, 0.5f, 0.5f, false, false);
                TriggerFade(fullscreenpopupbaseE, transDesc, 2, delay, delay + 0.167f * timeCoef, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                TriggerScaleOut(fullscreenpopupbaseE, transDesc, 3, delay, delay + 0.33f * timeCoef, 0.1f, 0.9f, 0.2f, 1.0f, scaleFinal2, scaleFinal2, 0.5f, 0.5f, false, false);
                TriggerFade(SimpleViewTop, transDesc, 4, delay, delay + 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
                TriggerScaleOut(SimpleViewTop, transDesc, 5, delay, delay + 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, scaleFinal2, scaleFinal2, 0.5f, 3.33f, false, false);
                TriggerFade(SimpleViewBottom, transDesc, 6, delay, delay + 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
                TriggerScaleOut(SimpleViewBottom, transDesc, 7, delay, delay + 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, scaleFinal2, scaleFinal2, 0.5f, -3.33f, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                fullscreenAnimation4();
            }
            DUI_SetGadgetZOrder(UIContainer, -1);
        }
    }

    void TriggerHSV(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
            HideSimpleView(true);
    }

    void ShowShutdownDialog(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            DDMenu* ddm = new DDMenu();
            ddm->CreatePopupMenu(false);
            ddm->AppendMenuW(MF_STRING, 2001, LoadStrFromRes(3052, L"shutdownux.dll").c_str());
            ddm->AppendMenuW(MF_STRING, 2002, LoadStrFromRes(3034, L"shutdownux.dll").c_str());
            ddm->AppendMenuW(MF_STRING, 2003, LoadStrFromRes(3019, L"shutdownux.dll").c_str());
            ddm->AppendMenuW(MF_STRING, 2004, LoadStrFromRes(3022, L"shutdownux.dll").c_str());
            ddm->AppendMenuW(MF_STRING, 2005, LoadStrFromRes(3013, L"shutdownux.dll").c_str());
            ddm->AppendMenuW(MF_STRING, 2006, LoadStrFromRes(3016, L"shutdownux.dll").c_str());
            ddm->AppendMenuW(MF_STRING, 2007, L"More...");
            for (int i = 1; i <= 6; i++)
                ddm->SetMenuItemGlyph(i + 2000, FALSE, LoadStrFromRes(i + 200).c_str());
            UINT uFlags = TPM_RIGHTBUTTON | TPM_CENTERALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_VERPOSANIMATION;
            if (localeType == 1) uFlags |= TPM_LAYOUTRTL;

            POINT ptZero{}, pt{};
            RECT rcMenu{};
            elem->GetRoot()->MapElementPoint(elem, &ptZero, &pt);
            ddm->GetMenuRect(&rcMenu);
            pt.x += elem->GetWidth() / 2;
            pt.y += elem->GetHeight();
            UINT uID = ddm->TrackPopupMenuEx(uFlags, pt.x, pt.y, editwnd->GetHWND(), nullptr);
            switch (uID)
            {
            case 2001:
            case 2002:
            case 2003:
            case 2004:
            case 2005:
            case 2006:
                delayedshutdownstatuses[uID - 2001] = true; // Used by shutdown dialog
                SendMessageW(wnd->GetHWND(), WM_USER + 19, NULL, uID - 2000);
                break;
            case 2007:
                DisplayShutdownDialog();
                break;
            }
            ddm->DestroyPopupMenu();
        }
    }

    void ShowSearchUI(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            HideSimpleView(false);
            CreateSearchPage(false);
        }
    }

    void TriggerEMToPV(bool fReverse)
    {
        g_pageviewer = !fReverse;
        RECT dimensions;
        GetClientRect(editwnd->GetHWND(), &dimensions);
        float left = CalcAnimOrigin(0.5f, 0.3625f, 0.7f, 0.25f);
        float veryleft = CalcAnimOrigin(0.5f, 0.225f, 0.7f, 0.25f);
        float top = CalcAnimOrigin(0.5f, 0.375f - (0.0125f * dimensions.right / dimensions.bottom), 0.7f, 0.25f);
        float right = CalcAnimOrigin(0.5f, 0.6375f, 0.7f, 0.25f);
        float veryright = CalcAnimOrigin(0.5f, 0.775f, 0.7f, 0.25f);
        float bottom = CalcAnimOrigin(0.5f, 0.625f + (0.0125f * dimensions.right / dimensions.bottom), 0.7f, 0.25f);
        if (localeType == 1)
        {
            left = 1 - left, veryleft = 1 - veryleft;
            right = 1 - right, veryright = 1 - veryright;
        }
        POINTFLOAT ptPage{};
        switch (g_currentPageID)
        {
        case 1:
            switch (g_maxPageID)
            {
            case 1:
                ptPage.x = 0.5f, ptPage.y = 0.5f;
                break;
            case 2:
                ptPage.x = left, ptPage.y = 0.5f;
                break;
            case 3: case 4:
                ptPage.x = left, ptPage.y = top;
                break;
            case 5: case 6:
                ptPage.x = veryleft, ptPage.y = top;
                break;
            }
            break;
        case 2:
            switch (g_maxPageID)
            {
            case 2:
                ptPage.x = right, ptPage.y = 0.5f;
                break;
            case 3: case 4:
                ptPage.x = right, ptPage.y = top;
                break;
            case 5: case 6:
                ptPage.x = 0.5f, ptPage.y = top;
                break;
            }
            break;
        case 3:
            switch (g_maxPageID)
            {
            case 3:
                ptPage.x = 0.5f, ptPage.y = bottom;
                break;
            case 4:
                ptPage.x = left, ptPage.y = bottom;
                break;
            case 5: case 6:
                ptPage.x = veryright, ptPage.y = top;
                break;
            }
            break;
        case 4:
            ptPage.y = bottom;
            switch (g_maxPageID)
            {
            case 4:
                ptPage.x = right;
                break;
            case 5:
                ptPage.x = left;
                break;
            case 6:
                ptPage.x = veryleft;
                break;
            }
            break;
        case 5:
            ptPage.y = bottom;
            switch (g_maxPageID)
            {
            case 5:
                ptPage.x = right;
                break;
            case 6:
                ptPage.x = 0.5f;
                break;
            }
            break;
        case 6:
            ptPage.x = veryright, ptPage.y = bottom;
            break;
        }
        CSafeElementPtr<Element> PageViewerTop;
        PageViewerTop.Assign(regElem<Element*>(L"PageViewerTop", PageViewer));
        CSafeElementPtr<Element> PV_Inner;
        PV_Inner.Assign(regElem<Element*>(L"PV_Inner", PageViewer));
        if (!fReverse)
        {
            g_invokedpagechange = true;
        }
        g_animatePVEnter = fReverse;
        GTRANS_DESC transDesc[1];
        TransitionStoryboardInfo tsbInfo = {};
        CSafeElementPtr<Element> pagesrow1; pagesrow1.Assign(regElem<Element*>(L"pagesrow1", PV_Inner));
        CSafeElementPtr<Element> pagesrow2; pagesrow2.Assign(regElem<Element*>(L"pagesrow2", PV_Inner));
        CValuePtr v;
        DynamicArray<Element*>* PV_Children = pagesrow1->GetChildren(&v);
        float flFade1 = fReverse ? 0.0f : 0.125f;
        float flFade2 = fReverse ? 0.15f : 0.275f;
        float flFade3 = fReverse ? 0.0f : 1.0f;
        float flFade4 = fReverse ? 1.0f : 0.0f;
        for (int num = 0; num < 2; num++)
        {
            if (PV_Children)
            {
                for (int i = 0; i < PV_Children->GetSize(); i++)
                {
                    if (PV_Children->GetItem(i)->GetID() == StrToID(L"PV_Page"))
                    {
                        DynamicArray<Element*>* PageChildren = PV_Children->GetItem(i)->GetChildren(&v);
                        if (((LVItem*)PV_Children->GetItem(i))->GetPage() == g_currentPageID)
                        {
                            for (int j = 0; j < PageChildren->GetSize(); j++)
                            {
                                if (PageChildren->GetItem(j)->GetID() == StrToID(L"pagetasks"))
                                {
                                    PageChildren->GetItem(j)->SetVisible(true);
                                    TriggerFade(PageChildren->GetItem(j), transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PageChildren->GetItem(j)->GetDisplayNode(), &tsbInfo);
                                }
                                if (PageChildren->GetItem(j)->GetID() == StrToID(L"PV_PageIcons"))
                                {
                                    DynamicArray<Element*>* PVPIChildren = PageChildren->GetItem(j)->GetChildren(&v);
                                    for (int k = 0; k < PVPIChildren->GetSize(); k++)
                                    {
                                        if (PVPIChildren->GetItem(k)->GetID() == StrToID(L"number"))
                                        {
                                            PVPIChildren->GetItem(k)->SetVisible(true);
                                            TriggerFade(PVPIChildren->GetItem(k), transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PVPIChildren->GetItem(k)->GetDisplayNode(), &tsbInfo);
                                            DUI_SetGadgetZOrder(PVPIChildren->GetItem(k), 4);
                                        }
                                    }
                                }
                            }
                            continue;
                        }
                        for (int j = 0; j < PageChildren->GetSize(); j++)
                        {
                            if (PageChildren->GetItem(j)->GetID() == StrToID(L"animateddimming"))
                            {
                                PageChildren->GetItem(j)->SetVisible(true);
                                TriggerFade(PageChildren->GetItem(j), transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade3, flFade4, !fReverse, false, false);
                            }
                            else
                            {
                                if (PageChildren->GetItem(j)->GetID() == StrToID(L"PV_PageIcons"))
                                {
                                    DynamicArray<Element*>* PVPIChildren = PageChildren->GetItem(j)->GetChildren(&v);
                                    for (int k = 0; k < PVPIChildren->GetSize(); k++)
                                    {
                                        if (PVPIChildren->GetItem(k)->GetID() == StrToID(L"number"))
                                        {
                                            PVPIChildren->GetItem(k)->SetVisible(true);
                                            TriggerFade(PVPIChildren->GetItem(k), transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PVPIChildren->GetItem(k)->GetDisplayNode(), &tsbInfo);
                                            DUI_SetGadgetZOrder(PVPIChildren->GetItem(k), 4);
                                        }
                                    }
                                }
                                TriggerFade(PageChildren->GetItem(j), transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                            }
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PageChildren->GetItem(j)->GetDisplayNode(), &tsbInfo);
                        }
                    }
                }
            }
            PV_Children = pagesrow2->GetChildren(&v);
        }
        if (fReverse)
        {
            SimpleViewTop->SetVisible(true);
            SimpleViewBottom->SetVisible(true);
            TriggerFade(SimpleViewTop, transDesc, 0, 0.433f, 0.566f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, SimpleViewTop->GetDisplayNode(), &tsbInfo);
            TriggerFade(SimpleViewBottom, transDesc, 0, 0.433f, 0.566f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, SimpleViewBottom->GetDisplayNode(), &tsbInfo);
            TriggerFade(PageViewerTop, transDesc, 0, 0.067f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PageViewerTop->GetDisplayNode(), &tsbInfo);
        }
        else
        {
            TriggerFade(SimpleViewTop, transDesc, 0, 0.067f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, SimpleViewTop->GetDisplayNode(), &tsbInfo);
            TriggerFade(SimpleViewBottom, transDesc, 0, 0.067f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, SimpleViewBottom->GetDisplayNode(), &tsbInfo);
            TriggerFade(PageViewerTop, transDesc, 0, 0.433f, 0.566f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PageViewerTop->GetDisplayNode(), &tsbInfo);
        }
        if (g_maxPageID <= 6)
        {
            if (fReverse) TriggerScaleOut(PV_Inner, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 2.8f, 2.8f, ptPage.x, ptPage.y, false, false);
            else TriggerScaleIn(PV_Inner, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 2.8f, 2.8f, ptPage.x, ptPage.y, 1.0f, 1.0f, ptPage.x, ptPage.y, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PV_Inner->GetDisplayNode(), &tsbInfo);
        }
        else
        {
            CSafeElementPtr<Element> overflow; overflow.Assign(regElem<Element*>(L"overflow", PV_Inner));
            if (fReverse) TriggerFade(overflow, transDesc, 0, 0.033f, 0.216f, 0.0f, 0.0f, 0.58f, 1.0f, 1.0f, 0.0f, false, false, true);
            else TriggerFade(overflow, transDesc, 0, 0.0f, 0.183f, 0.25f, 0.1f, 0.25f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, overflow->GetDisplayNode(), &tsbInfo);
            if (fReverse) TriggerScaleOut(PV_Inner, transDesc, 0, 0.033f, 0.3f, 1.0f, 1.0f, 0.0f, 1.0f, 1.15f, 1.15f, 0.5f, 0.5f, false, false);
            else TriggerScaleIn(PV_Inner, transDesc, 0, 0.0f, 0.267f, 0.0f, 0.0f, 0.0f, 1.0f, 1.15f, 1.15f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, PV_Inner->GetDisplayNode(), &tsbInfo);
        }
        DUI_SetGadgetZOrder(PV_Inner, -4);
        if (fReverse)
        {
            GTRANS_DESC transDesc2[2];
            TriggerFade(PageViewer, transDesc2, 0, 0.31f, 0.36f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, true, true);
            TriggerFade(fullscreenpopupbaseE, transDesc2, 1, 0.3f, 0.35f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, true, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, nullptr, &tsbInfo);
            DUI_SetGadgetZOrder(fullscreenpopupbaseE, -1);
            float scaleOrigin = (localeType == 1) ? 1.0f : 0.0f;
            short direction = (localeType == 1) ? -1 : 1;
            if (g_currentPageID > 1)
            {
                TriggerTranslate(prevpage, transDesc2, 0, 0.3f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f, prevpage->GetX() - dimensions.right * 0.1 * direction, prevpage->GetY(), prevpage->GetX(), prevpage->GetY(), false, false, false);
                TriggerScaleIn(bg_left_middle, transDesc2, 1, 0.3f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f, 3.0f, 1.0f, 1.0f - scaleOrigin, 0.5f, 1.0f, 1.0f, 1.0f - scaleOrigin, 0.5f, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, nullptr, &tsbInfo);
            }
            if (g_currentPageID < g_maxPageID)
            {
                TriggerTranslate(nextpage, transDesc2, 0, 0.3f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f, nextpage->GetX() + dimensions.right * 0.1 * direction, nextpage->GetY(), nextpage->GetX(), nextpage->GetY(), false, false, false);
                TriggerScaleIn(bg_right_middle, transDesc2, 1, 0.3f, 0.6f, 0.0f, 0.0f, 0.0f, 1.0f, 3.0f, 1.0f, scaleOrigin, 0.5f, 1.0f, 1.0f, scaleOrigin, 0.5f, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, nullptr, &tsbInfo);
            }
        }
        SimpleViewPages->SetActive(fReverse ? 0xB : 0);
        SimpleViewSettings->SetActive(fReverse ? 0xB : 0);
    }

    void TriggerNoMorePagesOnEdit()
    {
        float flOriginLeft = (localeType == 1) ? 1.0f : 0.0f;
        float flOriginRight = (localeType == 1) ? 0.0f : 1.0f;
        GTRANS_DESC transDesc[2], transDescL[2], transDescR[2];
        TransitionStoryboardInfo tsbInfo = {};
        TriggerScaleIn(SimpleViewTopInner, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 3.705f / 3, 0.5f, 0.0f, false, false);
        TriggerScaleIn(SimpleViewTopInner, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 3.705f / 3, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f, 0.0f, false, false);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, SimpleViewTopInner->GetDisplayNode(), &tsbInfo);
        TriggerScaleIn(SimpleViewBottomInner, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f, 3.7f / 3, 0.5f, 1.0f, false, false);
        TriggerScaleIn(SimpleViewBottomInner, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 3.7f / 3, 0.5f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, false, false);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, SimpleViewBottomInner->GetDisplayNode(), &tsbInfo);
        TriggerScaleIn(bg_left_middle, transDescL, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 0.5f, 3.7f / 3, 0.9f, flOriginLeft, 0.5f, false, false);
        TriggerScaleIn(bg_left_middle, transDescL, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.9f, flOriginLeft, 0.5f, 1.0f, 1.0f, flOriginLeft, 0.5f, false, false);
        TriggerScaleIn(bg_right_middle, transDescR, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 0.5f, 3.7f / 3, 0.9f, flOriginRight, 0.5f, false, false);
        TriggerScaleIn(bg_right_middle, transDescR, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.9f, flOriginRight, 0.5f, 1.0f, 1.0f, flOriginRight, 0.5f, false, false);
        if (prevpage->GetWidth() > 1)
        {
            TriggerScaleIn(bg_left_top, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 1.0f, 3.7f / 3, 0.65f, flOriginLeft, 1.0f, false, false);
            TriggerScaleIn(bg_left_top, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginLeft, 1.0f, 1.0f, 1.0f, flOriginLeft, 1.0f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, bg_left_top->GetDisplayNode(), &tsbInfo);
            TriggerScaleIn(bg_left_middle, transDescL, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 0.5f, 1.7f, 1.0f, flOriginLeft, 0.5f, false, false);
            TriggerScaleIn(bg_left_middle, transDescL, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.7f, 1.0f, flOriginLeft, 0.5f, 1.0f, 1.0f, flOriginLeft, 0.5f, false, false);
            TriggerScaleIn(bg_left_bottom, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 0.0f, 3.7f / 3, 0.65f, flOriginLeft, 0.0f, false, false);
            TriggerScaleIn(bg_left_bottom, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginLeft, 0.0f, 1.0f, 1.0f, flOriginLeft, 0.0f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, bg_left_bottom->GetDisplayNode(), &tsbInfo);
        }
        if (nextpage->GetWidth() > 1)
        {
            TriggerScaleIn(bg_right_top, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 0.0f, 3.7f / 3, 0.65f, flOriginRight, 1.0f, false, false);
            TriggerScaleIn(bg_right_top, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginRight, 0.0f, 1.0f, 1.0f, flOriginRight, 1.0f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, bg_right_top->GetDisplayNode(), &tsbInfo);
            TriggerScaleIn(bg_right_middle, transDescR, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 0.5f, 1.7f, 1.0f, flOriginRight, 0.5f, false, false);
            TriggerScaleIn(bg_right_middle, transDescR, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.7f, 1.0f, flOriginRight, 0.5f, 1.0f, 1.0f, flOriginRight, 0.5f, false, false);
            TriggerScaleIn(bg_right_bottom, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 1.0f, 3.7f / 3, 0.65f, flOriginRight, 0.0f, false, false);
            TriggerScaleIn(bg_right_bottom, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginRight, 1.0f, 1.0f, 1.0f, flOriginRight, 0.0f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, bg_right_bottom->GetDisplayNode(), &tsbInfo);
        }
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDescL), transDescL, bg_left_middle->GetDisplayNode(), &tsbInfo);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDescR), transDescR, bg_right_middle->GetDisplayNode(), &tsbInfo);
    }

    void EnterSelectedPage(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            CValuePtr v;
            int page{};
            if (PV_EnterPage->GetContentString(&v) != nullptr) page = _wtoi(PV_EnterPage->GetContentString(&v));
            if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || page < 1 || page > g_maxPageID)
            {
                MessageBeep(MB_OK);
                WCHAR* errorcontent = new WCHAR[256];
                StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), g_maxPageID);
                CSafeElementPtr<DDNotificationBanner> ddnb;
                ddnb.Assign(new DDNotificationBanner);
                ddnb->CreateBanner(DDNT_ERROR, LoadStrFromRes(4060).c_str(), errorcontent, 5);
                delete[] errorcontent;
                return;
            }
            ((LVItem*)elem)->SetPage(page);
            if (page == g_currentPageID) ClosePageViewer(elem, iev);
            else if (page < g_currentPageID) GoToPrevPage(elem, iev);
            else if (page > g_currentPageID) GoToNextPage(elem, iev);
        }
    }

    float CalcAnimOrigin(float flOriginFrom, float flOriginTo, float flScaleFrom, float flScaleTo)
    {
        float relScale = flScaleTo / flScaleFrom;
        return (flOriginTo - relScale * flOriginFrom) / (1 - relScale);
    }

    void AddNewPage(Element* elem, Event* iev);

    void ClosePageViewer(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            CSafeElementPtr<DDLVActionButton> PV_Home; PV_Home.Assign(regElem<DDLVActionButton*>(L"PV_Home", elem));
            CSafeElementPtr<DDLVActionButton> PV_Remove; PV_Remove.Assign(regElem<DDLVActionButton*>(L"PV_Remove", elem));
            if (PV_Home)
                if (PV_Home->GetMouseWithin()) return;
            if (PV_Remove)
                if (PV_Remove->GetMouseWithin()) return;
            TriggerEMToPV(true);
            RefreshSimpleView(0x0);
        }
    }

    void ShowPageViewer(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            fullscreenpopupbaseE->SetVisible(false);
            RECT dimensions;
            GetClientRect(editwnd->GetHWND(), &dimensions);
            parserEdit->CreateElement(L"PageViewer", nullptr, nullptr, nullptr, (Element**)&PageViewer);
            pEdit->Add((Element**)&PageViewer, 1);
            if (DWMActive)
            {
                AddLayeredRef(PageViewer->GetDisplayNode());
                SetGadgetFlags(PageViewer->GetDisplayNode(), NULL, NULL);
            }
            CSafeElementPtr<Element> PageViewerTop;
            PageViewerTop.Assign(regElem<Element*>(L"PageViewerTop", PageViewer));
            PageViewerTop->SetHeight(dimensions.bottom * 0.15);
            if (dimensions.bottom * 0.15 < 80 * g_flScaleFactor) PageViewerTop->SetHeight(80 * g_flScaleFactor);
            CSafeElementPtr<TouchButton> PV_Back;
            PV_Back.Assign(regElem<TouchButton*>(L"PV_Back", PageViewer));
            assignFn(PV_Back, ClosePageViewer);
            CSafeElementPtr<TouchButton> PV_Add;
            PV_Add.Assign(regElem<TouchButton*>(L"PV_Add", PageViewer));
            PV_Add->SetEnabled(isDefaultRes());
            assignFn(PV_Add, AddNewPage);
            CSafeElementPtr<Element> PV_Inner; PV_Inner.Assign(regElem<Element*>(L"PV_Inner", PageViewer));
            CSafeElementPtr<LVItem> peAnimateFrom;
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            if (g_maxPageID <= 6)
            {
                CSafeElementPtr<Element> pagesrow1;
                pagesrow1.Assign(regElem<Element*>(L"pagesrow1", PageViewer));
                CSafeElementPtr<Element> pagesrow2;
                pagesrow2.Assign(regElem<Element*>(L"pagesrow2", PageViewer));
                pagesrow1->SetHeight(ceil(dimensions.bottom * 0.25));
                int row1 = g_maxPageID;
                int row2 = 0;
                CSafeElementPtr<Element> bg_left; bg_left.Assign(regElem<Element*>(L"bg_left", PageViewer));
                CSafeElementPtr<Element> bg_top; bg_top.Assign(regElem<Element*>(L"bg_top", PageViewer));
                CSafeElementPtr<Element> bg_right; bg_right.Assign(regElem<Element*>(L"bg_right", PageViewer));
                CSafeElementPtr<Element> bg_bottom; bg_bottom.Assign(regElem<Element*>(L"bg_bottom", PageViewer));
                CSafeElementPtr<Element> rowpadding; rowpadding.Assign(regElem<Element*>(L"rowpadding", PageViewer));
                int topYHeight = floor(dimensions.bottom * 0.375);
                int bottomYPos = topYHeight + ceil(dimensions.bottom * 0.25);
                if (g_maxPageID >= 3)
                {
                    rowpadding->SetHeight(floor(dimensions.right * 0.025));
                    pagesrow2->SetHeight(ceil(dimensions.bottom * 0.25));
                    row1 = ceil(g_maxPageID / 2.0f);
                    row2 = floor(g_maxPageID / 2.0f);
                    topYHeight = floor((dimensions.bottom - 2 * ceil(dimensions.bottom * 0.25) - rowpadding->GetHeight()) / 2.0f);
                    bottomYPos = topYHeight + 2 * ceil(dimensions.bottom * 0.25) + rowpadding->GetHeight();
                }
                for (int i = 1; i <= g_maxPageID; i++)
                {
                    LVItem* PV_Page{};
                    parserEdit->CreateElement(L"PV_Page", nullptr, nullptr, nullptr, (Element**)&PV_Page);
                    PV_Page->SetWidth(ceil(dimensions.right * 0.25));
                    PV_Page->SetHeight(ceil(dimensions.bottom * 0.25));
                    PV_Page->SetPage(i);
                    Element* PV_PageRow_Dim{};
                    parserEdit->CreateElement(L"PV_PageRow_Dim", nullptr, nullptr, nullptr, &PV_PageRow_Dim);
                    PV_PageRow_Dim->SetWidth(ceil(dimensions.right * 0.025));
                    PV_PageRow_Dim->SetHeight(ceil(dimensions.bottom * 0.25));
                    if (i <= row1)
                    {
                        pagesrow1->Add((Element**)&PV_Page, 1);
                        pagesrow1->SetWidth(pagesrow1->GetWidth() + PV_Page->GetWidth());
                        if (i < row1)
                        {
                            pagesrow1->Add(&PV_PageRow_Dim, 1);
                            pagesrow1->SetWidth(pagesrow1->GetWidth() + PV_PageRow_Dim->GetWidth());
                        }
                    }
                    else
                    {
                        if (g_maxPageID & 1 && i == row1 + 1)
                            PV_CreateDimRect(pagesrow2, round(dimensions.right * 0.1375 + 0.25), ceil(dimensions.bottom * 0.25));
                        pagesrow2->Add((Element**)&PV_Page, 1);
                        pagesrow2->SetWidth(pagesrow2->GetWidth() + PV_Page->GetWidth());
                        if (i < g_maxPageID)
                        {
                            pagesrow2->Add(&PV_PageRow_Dim, 1);
                            pagesrow2->SetWidth(pagesrow2->GetWidth() + PV_PageRow_Dim->GetWidth());
                        }
                        if (g_maxPageID & 1 && i == g_maxPageID)
                            PV_CreateDimRect(pagesrow2, ceil(dimensions.right * 0.1375), ceil(dimensions.bottom * 0.25));
                    }
                    int remainingIcons = 1;
                    PV_Page->AddFlags(LVIF_HIDDEN);
                    CSafeElementPtr<Element> PV_PageIcons;
                    PV_PageIcons.Assign(regElem<Element*>(L"PV_PageIcons", PV_Page));
                    for (int j = 0; j < pm.size(); j++)
                    {
                        if (pm[j]->GetPage() != i) continue;
                        remainingIcons++;
                        yValueEx* yV = new yValueEx{ j, 0.25, NULL, nullptr, PV_PageIcons };
                        PV_Page->RemoveFlags(LVIF_HIDDEN);
                        QueueUserWorkItem(CreateDesktopPreviewHelper, yV, 0);
                    }
                    CSafeElementPtr<RichText> number;
                    number.Assign(regElem<RichText*>(L"number", PV_Page));
                    number->SetContentString(to_wstring(i).c_str());
                    CSafeElementPtr<Element> PV_HomeBadge;
                    PV_HomeBadge.Assign(regElem<Element*>(L"PV_HomeBadge", PV_Page));
                    DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", PV_Page);
                    if (PV_Page->GetPage() == g_homePageID)
                    {
                        PV_Home->SetSelected(true);
                        PV_HomeBadge->SetSelected(true);
                    }
                    if (remainingIcons == 1)
                    {
                        DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", PV_Page);
                        if (PV_Remove)
                        {
                            PV_Remove->SetEnabled(isDefaultRes());
                            PV_Remove->SetVisible(PV_Page->GetMouseWithin());
                            PV_Remove->SetAssociatedItem(PV_Page);
                            assignFn(PV_Remove, RemoveSelectedPage);
                        }
                    }
                    if (PV_Home)
                    {
                        PV_Home->SetEnabled(isDefaultRes());
                        PV_Home->SetVisible(PV_Page->GetMouseWithin());
                        PV_Home->SetAssociatedItem(PV_Page);
                        if (PV_Page->GetPage() != g_homePageID) assignFn(PV_Home, SetSelectedPageHome);
                    }
                    if (isDefaultRes()) assignExtendedFn(PV_Page, ShowPageOptionsOnHover);
                    if (i == g_currentPageID) assignFn(PV_Page, ClosePageViewer);
                    else if (i < g_currentPageID) assignFn(PV_Page, GoToPrevPage);
                    else if (i > g_currentPageID) assignFn(PV_Page, GoToNextPage);
                }
                if (g_maxPageID >= 3) rowpadding->SetWidth(pagesrow1->GetWidth());
                SetTransElementPosition(bg_top, 0, 0, dimensions.right, topYHeight);
                SetTransElementPosition(bg_bottom, 0, bottomYPos, dimensions.right, dimensions.bottom - bottomYPos);
                SetTransElementPosition(bg_left, 0, bg_top->GetHeight(), floor((dimensions.right - pagesrow1->GetWidth()) / 2.0f), dimensions.bottom - bg_top->GetHeight() - bg_bottom->GetHeight());
                SetTransElementPosition(bg_right, bg_left->GetWidth() + pagesrow1->GetWidth(), bg_top->GetHeight(),
                    dimensions.right - bg_left->GetWidth() + pagesrow1->GetWidth(), dimensions.bottom - bg_top->GetHeight() - bg_bottom->GetHeight());
            }
            else
            {
                CSafeElementPtr<Element> bg; bg.Assign(regElem<Element*>(L"bg", PageViewer));
                bg->SetVisible(true);
                Element* overflow;
                parserEdit->CreateElement(L"overflow", nullptr, nullptr, nullptr, &overflow);
                PV_Inner->Add(&overflow, 1);
                PV_EnterPage = regElem<DDScalableTouchEdit*>(L"PV_EnterPage", PageViewer);
                CSafeElementPtr<LVItem> PV_ConfirmEnterPage;
                PV_ConfirmEnterPage.Assign(regElem<LVItem*>(L"PV_ConfirmEnterPage", PageViewer));
                assignFn(PV_ConfirmEnterPage, EnterSelectedPage);
                PV_SetEnterPageDesc();
                DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", PageViewer);
                if (PV_Remove)
                {
                    PV_Remove->SetEnabled(isDefaultRes());
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
                DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", PageViewer);
                if (PV_Home)
                {
                    PV_Home->SetEnabled(isDefaultRes());
                    assignFn(PV_Home, SetSelectedPageHome);
                }
            }
            if (g_animatePVEnter)
            {
                TriggerEMToPV(false);
            }
        }
    }

    void AddNewPage(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click())
        {
            g_editingpages = true;
            SetTimer(editwnd->GetHWND(), 4, 80, nullptr);
        }
    }

    void RemoveSelectedPage(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click || iev->uidType == TouchButton::MultipleClick)
        {
            g_editingpages = true;
            timerPtr = elem;
            SetTimer(editwnd->GetHWND(), 1, 80, nullptr);
        }
    }

    void SetSelectedPageHome(Element* elem, Event* iev)
    {
        if ((iev->uidType == TouchButton::Click || iev->uidType == TouchButton::MultipleClick) && g_maxPageID > 1)
        {
            g_editingpages = true;
            timerPtr = elem;
            SetTimer(editwnd->GetHWND(), 2, 80, nullptr);
        }
    }

    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseWithinProp())
        {
            if (((LVItem*)elem)->GetFlags() & LVIF_HIDDEN)
            {
                DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", elem);
                if (PV_Remove)
                {
                    PV_Remove->SetEnabled(isDefaultRes());
                    PV_Remove->SetVisible(elem->GetMouseWithin());
                    PV_Remove->SetAssociatedItem((LVItem*)elem);
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
            }
            DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", elem);
            if (PV_Home)
            {
                PV_Home->SetEnabled(isDefaultRes());
                PV_Home->SetVisible(elem->GetMouseWithin());
                PV_Home->SetAssociatedItem((LVItem*)elem);
                if (((LVItem*)elem)->GetPage() != g_homePageID) assignFn(PV_Home, SetSelectedPageHome);
            }
        }
    }

    void ExitWindow(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            DDMenu* ddm = new DDMenu();
            ddm->CreatePopupMenu(false);
            ddm->AppendMenuW(MF_STRING, 1, L"Restart E&xplorer");
            ddm->AppendMenuW(MF_STRING, 69, L"Do not restart &Explorer");
            UINT uFlags = TPM_RIGHTBUTTON | TPM_CENTERALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD | TPM_VERNEGANIMATION;
            if (localeType == 1) uFlags |= TPM_LAYOUTRTL;

            POINT ptZero{}, pt{};
            RECT rcMenu{};
            elem->GetRoot()->MapElementPoint(elem, &ptZero, &pt);
            ddm->GetMenuRect(&rcMenu);
            pt.x += elem->GetWidth() / 2;
            LPARAM lParam = ddm->TrackPopupMenuEx(uFlags, pt.x, pt.y, editwnd->GetHWND(), nullptr);
            ddm->DestroyPopupMenu();
            if (lParam > 0)
            {
                SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
                SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, lParam);
            }
        }
    }

    void CreatePagePreview()
    {
        for (int j = 0; j < pm.size(); j++)
        {
            Element* peContainer{};
            float peContainerScale{};
            if (pm[j]->GetPage() == g_currentPageID)
            {
                peContainer = fullscreeninnerE;
                peContainerScale = 0.7;
            }
            else if (pm[j]->GetPage() == g_currentPageID - 1)
            {
                peContainer = prevpage;
                peContainerScale = 0.5;
            }
            else if (pm[j]->GetPage() == g_currentPageID + 1)
            {
                peContainer = nextpage;
                peContainerScale = 0.5;
            }
            else continue;
            yValueEx* yV = new yValueEx{ j, peContainerScale, NULL, nullptr, peContainer };
            QueueUserWorkItem(CreateDesktopPreviewHelper, yV, 0);
        }
    }

    void _UpdateSimpleViewContent(bool animate, DWORD animFlags)
    {
        RECT dimensions;
        GetClientRect(editwnd->GetHWND(), &dimensions);

        if (animFlags && !(animFlags & 0x10))
            CreatePagePreview();

        CSafeElementPtr<RichText> SimpleViewHomeBadge;
        SimpleViewHomeBadge.Assign(regElem<RichText*>(L"SimpleViewHomeBadge", pEdit));
        if (g_maxPageID != 1)
        {
            WCHAR currentPage[64];
            StringCchPrintfW(currentPage, 64, LoadStrFromRes(4026).c_str(), g_currentPageID, g_maxPageID);
            pageinfo->SetContentString(currentPage);
            if (g_currentPageID == g_homePageID) SimpleViewHomeBadge->SetLayoutPos(0);
            else SimpleViewHomeBadge->SetLayoutPos(-3);
        }
        else
        {
            pageinfo->SetContentString(L" ");
            SimpleViewHomeBadge->SetLayoutPos(-3);
        }
        prevpage->SetWidth(0);
        nextpage->SetWidth(0);
        if (g_currentPageID != g_maxPageID)
        {
            float xLoc = (localeType == 1) ? -0.4 : 0.9;
            TogglePage(nextpage, xLoc, 0.25, 0.5, 0.5);
        }
        if (g_currentPageID != 1)
        {
            float xLoc = (localeType == 1) ? 0.9 : -0.4;
            TogglePage(prevpage, xLoc, 0.25, 0.5, 0.5);
        }

        SetPopupSize(centeredE, ceil(dimensions.right * 0.7), ceil(dimensions.bottom * 0.7));
        SetPopupSize(fullscreeninnerE, ceil(dimensions.right * 0.7), ceil(dimensions.bottom * 0.7));
        SetPopupSize(simpleviewoverlay, ceil(dimensions.right * 0.7), ceil(dimensions.bottom * 0.7));
        SimpleViewTop->SetHeight(floor(dimensions.bottom * 0.15));
        SimpleViewTopInner->SetHeight(floor(dimensions.bottom * 0.15));
        SimpleViewBottom->SetHeight(dimensions.bottom - SimpleViewTop->GetHeight() - fullscreeninnerE->GetHeight());
        SimpleViewBottomInner->SetHeight(dimensions.bottom - SimpleViewTopInner->GetHeight() - fullscreeninnerE->GetHeight());
        if (dimensions.bottom * 0.15 < 80 * g_flScaleFactor) SimpleViewTop->SetHeight(80 * g_flScaleFactor);
        if (dimensions.bottom * 0.15 < 106 * g_flScaleFactor) SimpleViewBottom->SetHeight(106 * g_flScaleFactor);

        unsigned short leftX = (localeType == 1) ? round(dimensions.right * 0.15) + fullscreeninnerE->GetWidth() : 0;
        unsigned short leftWidth = (localeType == 1) ? dimensions.right - leftX : floor(dimensions.right * 0.15);
        unsigned short leftXSmall = (localeType == 1) ? leftX : prevpage->GetX() + prevpage->GetWidth();
        unsigned short leftWidthSmall = (localeType == 1) ? prevpage->GetX() - leftXSmall : leftWidth - (prevpage->GetX() + prevpage->GetWidth());
        unsigned short rightX = (localeType == 1) ? 0 : floor(dimensions.right * 0.15) + fullscreeninnerE->GetWidth();
        unsigned short rightWidth = (localeType == 1) ? round(dimensions.right * 0.15) : ceil(dimensions.right * 0.15);
        unsigned short rightXSmall = (localeType == 1) ? nextpage->GetX() + nextpage->GetWidth() : rightX;
        unsigned short rightWidthSmall = (localeType == 1) ? rightWidth - (nextpage->GetX() + nextpage->GetWidth()) : nextpage->GetX() - rightXSmall;
        if (prevpage->GetWidth() > 1)
        {
            SetTransElementPosition(bg_left_top, leftX, SimpleViewTopInner->GetHeight(),
                leftWidth, prevpage->GetY() - SimpleViewTopInner->GetHeight());

            SetTransElementPosition(bg_left_middle, leftXSmall, bg_left_top->GetY() + bg_left_top->GetHeight(),
                leftWidthSmall, prevpage->GetHeight());

            SetTransElementPosition(bg_left_bottom, leftX, bg_left_middle->GetY() + bg_left_middle->GetHeight(),
                leftWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_left_middle->GetY() - bg_left_middle->GetHeight());
        }
        else
        {
            SetTransElementPosition(bg_left_top, 0, 0, 0, 0);
            SetTransElementPosition(bg_left_middle, leftX, SimpleViewTopInner->GetHeight(),
                leftWidth, dimensions.bottom - SimpleViewTopInner->GetHeight() - SimpleViewBottomInner->GetHeight());
            SetTransElementPosition(bg_left_bottom, 0, 0, 0, 0);
            leftXSmall = (localeType == 1) ? leftX : dimensions.right * 0.1;
            leftWidthSmall = (localeType == 1) ? dimensions.right * 0.9 - leftXSmall : leftWidth - dimensions.right * 0.1;
        }
        if (nextpage->GetWidth() > 1)
        {
            SetTransElementPosition(bg_right_top, rightX, SimpleViewTopInner->GetHeight(),
                rightWidth, nextpage->GetY() - SimpleViewTopInner->GetHeight());

            SetTransElementPosition(bg_right_middle, rightXSmall, bg_right_top->GetY() + bg_right_top->GetHeight(),
                rightWidthSmall, nextpage->GetHeight());

            SetTransElementPosition(bg_right_bottom, rightX, bg_right_middle->GetY() + bg_right_middle->GetHeight(),
                rightWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_right_middle->GetY() - bg_right_middle->GetHeight());
        }
        else
        {
            SetTransElementPosition(bg_right_top, 0, 0, 0, 0);
            SetTransElementPosition(bg_right_middle, rightX, SimpleViewTopInner->GetHeight(),
                rightWidth, dimensions.bottom - SimpleViewTopInner->GetHeight() - SimpleViewBottomInner->GetHeight());
            SetTransElementPosition(bg_right_bottom, 0, 0, 0, 0);
            rightXSmall = (localeType == 1) ? dimensions.right * 0.1 : rightX;
            rightWidthSmall = (localeType == 1) ? rightWidth : dimensions.right * 0.9 - rightXSmall;
        }
        if (animate)
        {
            GTRANS_DESC transDesc[8];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerFade(UIContainer, transDesc, 0, 0.0f, 0.167f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            TriggerScaleOut(UIContainer, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 0.7f, 0.7f, 0.5f, 0.5f, false, false);
            TriggerFade(fullscreenpopupbaseE, transDesc, 2, 0.0f, 0.167f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleIn(fullscreenpopupbaseE, transDesc, 3, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            TriggerFade(SimpleViewTop, transDesc, 4, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleIn(SimpleViewTop, transDesc, 5, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, 3.33f, 1.0f, 1.0f, 0.5f, 3.33f, false, false);
            TriggerFade(SimpleViewBottom, transDesc, 6, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleIn(SimpleViewBottom, transDesc, 7, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, -3.33f, 1.0f, 1.0f, 0.5f, -3.33f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
            DUI_SetGadgetZOrder(fullscreenpopupbaseE, -1);
            DUI_SetGadgetZOrder(SimpleViewTop, 0);
            DUI_SetGadgetZOrder(SimpleViewBottom, 0);
        }
        else
        {
            // 0.5.6: bg_extras are never destroyed, which is problematic as long as the edit mode isn't closed
            Element* bg_main[7]{};
            Element* bg_extras[4]{};
            short localeDirection = (localeType == 1) ? -1 : 1;
            short animDirection = 1;
            float flOrig = 0.5f * (1 - localeDirection), flOrig2 = 0.5f * (1 + localeDirection);
            BYTE fullLeftCoef = (prevpage->GetWidth() > 0) ? 1 : 3, fullRightCoef = (nextpage->GetWidth() > 0) ? 1 : 3;
            float flMiddleAnim = -1 * localeDirection * centeredE->GetWidth() / static_cast<float>(bg_right_middle->GetWidth()) * fullRightCoef + (1 - localeDirection) / 2.0f;
            float flMiddleSmAnim = localeDirection * (centeredE->GetWidth() + bg_left_middle->GetWidth() / (7.0f * fullLeftCoef)) / (bg_left_middle->GetWidth() / 1.4f) * fullLeftCoef + (1 + localeDirection) / 2.0f;
            float flMiddleSmAnim2 = -1 * localeDirection * nextpage->GetWidth() / static_cast<float>(bg_right_middle->GetWidth()) + (1 - localeDirection) / 2.0f;
            bool invert{};
            UIContainer->SetVisible(false);
            GTRANS_DESC transDesc[1], transDesc2[1], transDesc3[1];
            TransitionStoryboardInfo tsbInfo = {};
            if (animFlags & 1)
            {
                EM_CreateDimRect(bg_extras[0], EM_Dim, nextpage->GetX() + nextpage->GetWidth() * localeDirection, nextpage->GetY(), nextpage->GetWidth(), nextpage->GetHeight());
                EM_CreateDimRect(bg_extras[1], EM_Dim, leftX, SimpleViewTopInner->GetHeight(), leftWidth, prevpage->GetY() - SimpleViewTopInner->GetHeight());
                EM_CreateDimRect(bg_extras[2], EM_Dim, leftXSmall + leftWidthSmall / 3.5f, bg_extras[1]->GetY() + bg_extras[1]->GetHeight(), leftWidthSmall / 1.4f, prevpage->GetHeight());
                EM_CreateDimRect(bg_extras[3], EM_Dim, leftX, bg_extras[2]->GetY() + bg_extras[2]->GetHeight(), leftWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_extras[2]->GetY() - bg_extras[2]->GetHeight());
                bg_main[0] = bg_left_middle, bg_main[1] = bg_right_top, bg_main[2] = bg_right_middle, bg_main[3] = bg_right_bottom;
                TriggerScaleIn(centeredE, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f - 3.18f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f - 3.18f * localeDirection, 0.5f, false, false);
                TriggerScaleIn(nextpage, transDesc3, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1.4f, 1.4f, 0.5f + 3.25f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f + 3.25f * localeDirection, 0.5f, false, false);
            }
            if (animFlags & 2)
            {
                bg_main[4] = bg_left_top, bg_main[5] = bg_left_middle, bg_main[6] = bg_left_bottom;
                TriggerScaleIn(prevpage, transDesc2, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f - 3.0f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f - 3.0f * localeDirection, 0.5f, false, false);
            }
            if (animFlags & 4)
            {
                invert = true;
                EM_CreateDimRect(bg_extras[0], EM_Dim, prevpage->GetX() - prevpage->GetWidth() * localeDirection, prevpage->GetY(), prevpage->GetWidth(), prevpage->GetHeight());
                EM_CreateDimRect(bg_extras[1], EM_Dim, rightX, SimpleViewTopInner->GetHeight(), rightWidth, nextpage->GetY() - SimpleViewTopInner->GetHeight());
                EM_CreateDimRect(bg_extras[2], EM_Dim, rightXSmall, bg_extras[1]->GetY() + bg_extras[1]->GetHeight(), rightWidthSmall / 1.4f, nextpage->GetHeight());
                EM_CreateDimRect(bg_extras[3], EM_Dim, rightX, bg_extras[2]->GetY() + bg_extras[2]->GetHeight(), rightWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_extras[2]->GetY() - bg_extras[2]->GetHeight());
                bg_main[0] = bg_right_middle, bg_main[1] = bg_left_top, bg_main[2] = bg_left_middle, bg_main[3] = bg_left_bottom;
                TriggerScaleIn(centeredE, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f + 3.18f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f + 3.18f * localeDirection, 0.5f, false, false);
                TriggerScaleIn(prevpage, transDesc2, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1.4f, 1.4f, 0.5f - 3.25f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f - 3.25f * localeDirection, 0.5f, false, false);
            }
            if (animFlags & 5)
            {
                if (invert)
                {
                    animDirection = -1;
                    flOrig = 0.5f * (1 + localeDirection);
                    flOrig2 = 0.5f * (1 - localeDirection);
                    flMiddleAnim = localeDirection * centeredE->GetWidth() / static_cast<float>(bg_left_middle->GetWidth()) * fullLeftCoef + (1 + localeDirection) / 2.0f;
                    flMiddleSmAnim = -1 * localeDirection * (centeredE->GetWidth() + bg_left_middle->GetWidth() / 7.0f) / (bg_right_middle->GetWidth() / 1.4f) * fullRightCoef + (1 - localeDirection) / 2.0f;
                    flMiddleSmAnim2 = localeDirection * prevpage->GetWidth() / static_cast<float>(bg_left_middle->GetWidth()) + (1 + localeDirection) / 2.0f;
                }
                GTRANS_DESC transDesc[8];
                TriggerScaleIn(bg_extras[0], transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1.4f, 1.4f, 0.5f + 2.25f * localeDirection * animDirection, 0.5f, 1.0f, 1.0f, 0.5f + 2.25f * localeDirection * animDirection, 0.5f, false, false);
                TriggerScaleOut(bg_extras[1], transDesc, 1, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, EM_GetRectAniWScale(bg_extras[1]), 0.0f, flOrig, 0.0f, false, false);
                TriggerScaleOut(bg_extras[2], transDesc, 2, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 0.0f, (centeredE->GetHeight() / static_cast<float>(bg_extras[2]->GetHeight())), flMiddleSmAnim, 0.5f, false, false);
                TriggerScaleOut(bg_extras[3], transDesc, 3, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, EM_GetRectAniWScale(bg_extras[3]), 0.0f, flOrig, 1.0f, false, false);
                TriggerScaleIn(bg_main[0], transDesc, 4, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                    1 / 1.4f, 1 / 1.4f, 0.5f - 12.0f * localeDirection * animDirection, 0.5f, 1.0f, 1.0f, 0.5f - 12.0f * localeDirection * animDirection, 0.5f, false, false);
                TriggerScaleIn(bg_main[1], transDesc, 5, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                    EM_GetRectAniWScale(bg_main[1]), 0.0f, flOrig2, 0.0f, 1.0f, 1.0f, flOrig2, 0.0f, false, false);
                TriggerScaleIn(bg_main[2], transDesc, 6, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                    0.0f, (centeredE->GetHeight() / static_cast<float>(bg_main[2]->GetHeight())),
                    flMiddleAnim, 0.5f, 1.0f, 1.0f, flMiddleAnim, 0.5f, false, false);
                TriggerScaleIn(bg_main[3], transDesc, 7, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                    EM_GetRectAniWScale(bg_main[3]), 0.0f, flOrig2, 1.0f, 1.0f, 1.0f, flOrig2, 1.0f, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
            }
            if (animFlags & 6) ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, prevpage->GetDisplayNode(), &tsbInfo);
            if (animFlags & 8)
            {
                bg_main[4] = bg_right_top, bg_main[5] = bg_right_middle, bg_main[6] = bg_right_bottom;
                TriggerScaleIn(nextpage, transDesc3, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f + 3.0f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f + 3.0f * localeDirection, 0.5f, false, false);
            }
            if (animFlags & 9) ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc3), transDesc3, nextpage->GetDisplayNode(), &tsbInfo);
            if (animFlags & 0xA)
            {
                GTRANS_DESC transDesc[3];
                TriggerScaleIn(bg_main[4], transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                    1 / 1.4f, 1 / 1.4f, 0.5f - 12.0f * localeDirection * animDirection, 3.5f, 1.0f, 1.0f, 0.5f - 12.0f * localeDirection * animDirection, 3.5f, false, false);
                TriggerScaleIn(bg_main[5], transDesc, 1, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                    0.0f, 1 / 1.4f, flMiddleSmAnim2, 0.5f, 1.0f, 1.0f, flMiddleSmAnim2, 0.5f, false, false);
                TriggerScaleIn(bg_main[6], transDesc, 2, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                    1 / 1.4f, 1 / 1.4f, 0.5f - 12.0f * localeDirection * animDirection, -2.5f, 1.0f, 1.0f, 0.5f - 12.0f * localeDirection * animDirection, -2.5f, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
            }
            if (animFlags & 0xF)
            {
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, centeredE->GetDisplayNode(), &tsbInfo);
            }
        }
        g_invokedpagechange = false;
        CSafeElementPtr<TouchScrollViewer> svBottomOptions;
        svBottomOptions.Assign(regElem<TouchScrollViewer*>(L"svBottomOptions", SimpleViewBottom));
        Element* XScrollbar;
        svBottomOptions->GetHScrollbar(&XScrollbar);
        svBottomOptions->SetXScrollable(XScrollbar->GetVisible());

        if (!animFlags || animFlags & 0x10)
            SetTimer(editwnd->GetHWND(), 5, 10, nullptr);
    }
    void RefreshSimpleView(DWORD animFlags)
    {
        fullscreeninnerE->DestroyAll(true);
        prevpage->DestroyAll(true);
        nextpage->DestroyAll(true);
        prevpageMain->SetVisible(true);
        nextpageMain->SetVisible(true);
        if (g_currentPageID == 1) prevpageMain->SetVisible(false);
        if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
        _UpdateSimpleViewContent(false, animFlags);
    }
    void ShowSimpleView(bool animate, DWORD animFlags)
    {
        if (g_touchmode) g_iconsz = 64;
        g_editmode = true;
        g_animatePVEnter = true;
        if (!g_invokedpagechange) SendMessageW(g_hWndTaskbar, WM_COMMAND, 419, 0);
        static IElementListener *pel_GoToPrevPage, *pel_GoToNextPage, *pel_ShowShutdownDialog,
                                *pel_ShowSearchUI, *pel_ShowSettings, *pel_ShowPageViewer, *pel_ExitWindow;
        RECT dimensions;
        POINT topLeftMon = GetTopLeftMonitor();
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        if (localeType == 1)
        {
            int rightMon = GetRightMonitor();
            topLeftMon.x = dimensions.right + dimensions.left - rightMon;
        }
        DWORD dwExStyle = WS_EX_TOOLWINDOW, dwCreateFlags = 0x10;
        if (DWMActive)
        {
            dwExStyle |= WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP;
            dwCreateFlags |= 0x28;
        }
        NativeHWNDHost::Create(L"DD_EditModeHost", L"DirectDesktop Edit Mode", nullptr, nullptr, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y,
                               9999, 9999, dwExStyle, WS_POPUP, nullptr, 0x43, &editwnd);
        HWNDElement::Create(editwnd->GetHWND(), true, dwCreateFlags, nullptr, &key5, (Element**)&editparent);
        //NativeHWNDHost::Create(L"DD_EditModeBlur", L"DirectDesktop Edit Mode Blur Helper", nullptr, NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y,
        //dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, NULL, WS_POPUP, nullptr, 0x43, &editbgwnd);
        //HWNDElement::Create(editbgwnd->GetHWND(), true, NULL, nullptr, &key6, (Element**)&editbgparent);
        DUIXmlParser::Create(&parserEdit, nullptr, nullptr, DUI_ParserErrorCB, nullptr);

        parserEdit->SetXMLFromResource(IDR_UIFILE6, HINST_THISCOMPONENT, HINST_THISCOMPONENT);

        parserEdit->CreateElement(L"editmode", editparent, nullptr, nullptr, &pEdit);
        //parserEdit->CreateElement(L"editmodeblur", editbgparent, nullptr, NULL, &pEditBG);

        //SetWindowLongPtrW(editwnd->GetHWND(), GWL_STYLE, 0x56003A40L);
        //SetWindowLongPtrW(editbgwnd->GetHWND(), GWL_STYLE, 0x56003A40L);

        SetWindowPos(editwnd->GetHWND(), nullptr, NULL, NULL, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOMOVE | SWP_NOZORDER);
        GetClientRect(editwnd->GetHWND(), &dimensions);

        pEdit->SetVisible(true);
        pEdit->EndDefer(key5);
        //pEditBG->SetVisible(true);
        //pEditBG->EndDefer(key6);

        fullscreenpopupbaseE = regElem<Element*>(L"fullscreenpopupbase", pEdit);
        popupcontainerE = regElem<Element*>(L"popupcontainer", pEdit);
        centeredE = regElem<Element*>(L"centered", pEdit);
        //centeredEBG = regElem<Button*>(L"centered", pEditBG);
        SimpleViewTop = regElem<Element*>(L"SimpleViewTop", pEdit);
        SimpleViewBottom = regElem<Element*>(L"SimpleViewBottom", pEdit);
        SimpleViewTopInner = regElem<Element*>(L"SimpleViewTopInner", pEdit);
        SimpleViewBottomInner = regElem<Element*>(L"SimpleViewBottomInner", pEdit);
        SimpleViewPower = regElem<TouchButton*>(L"SimpleViewPower", pEdit);
        SimpleViewSearch = regElem<TouchButton*>(L"SimpleViewSearch", pEdit);
        SimpleViewSettings = regElem<DDIconButton*>(L"SimpleViewSettings", pEdit);
        SimpleViewPages = regElem<DDIconButton*>(L"SimpleViewPages", pEdit);
        SimpleViewClose = regElem<DDIconButton*>(L"SimpleViewClose", pEdit);
        EM_Dim = regElem<Element*>(L"EM_Dim", pEdit);
        bg_left_top = regElem<Element*>(L"bg_left_top", pEdit);
        bg_left_middle = regElem<Element*>(L"bg_left_middle", pEdit);
        bg_left_bottom = regElem<Element*>(L"bg_left_bottom", pEdit);
        bg_right_top = regElem<Element*>(L"bg_right_top", pEdit);
        bg_right_middle = regElem<Element*>(L"bg_right_middle", pEdit);
        bg_right_bottom = regElem<Element*>(L"bg_right_bottom", pEdit);
        prevpage = regElem<DDScalableTouchButton*>(L"prevpage", pEdit);
        nextpage = regElem<DDScalableTouchButton*>(L"nextpage", pEdit);
        pageinfo = regElem<DDScalableRichText*>(L"pageinfo", pEdit);

        free(pel_GoToPrevPage), free(pel_GoToNextPage), free(pel_ShowShutdownDialog),
            free(pel_ShowSearchUI), free(pel_ShowSettings), free(pel_ShowPageViewer), free(pel_ExitWindow);
        pel_GoToPrevPage = (IElementListener*)assignFn(prevpage, GoToPrevPage, true);
        pel_GoToNextPage = (IElementListener*)assignFn(nextpage, GoToNextPage, true);
        pel_ShowShutdownDialog = (IElementListener*)assignFn(SimpleViewPower, ShowShutdownDialog, true);
        pel_ShowSearchUI = (IElementListener*)assignFn(SimpleViewSearch, ShowSearchUI, true);
        pel_ShowSettings = (IElementListener*)assignFn(SimpleViewSettings, ShowSettings, true);
        pel_ShowPageViewer = (IElementListener*)assignFn(SimpleViewPages, ShowPageViewer, true);
        pel_ExitWindow = (IElementListener*)assignFn(SimpleViewClose, ExitWindow, true);

        SimpleViewClose->SetLayoutPos(g_enableexit ? -1 : -3);

        WndProcEdit = (WNDPROC)SetWindowLongPtrW(editwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeWindowProc);
        //WndProcEditBG = (WNDPROC)SetWindowLongPtrW(editbgwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeBGWindowProc);

        LPWSTR sheetName = g_theme ? (LPWSTR)L"edit" : (LPWSTR)L"editdark";
        StyleSheet* sheet = pEdit->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parserEdit->GetSheet(sheetName, &sheetStorage);
        pEdit->SetValue(Element::SheetProp, 1, sheetStorage);
        //pEditBG->SetValue(Element::SheetProp, 1, sheetStorage);

        editwnd->Host(pEdit);
        editwnd->ShowWindow(SW_SHOW);
        //editbgwnd->Host(pEditBG);
        //editbgwnd->ShowWindow(SW_SHOW);
        //editbgwnd->ShowWindow(SW_HIDE);

        WCHAR* WindowsBuildStr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
        int WindowsBuild = _wtoi(WindowsBuildStr);
        free(WindowsBuildStr);
        if (WindowsBuild >= 26002)
        {
            HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
            SetParent(editwnd->GetHWND(), hWndProgman);
        }
        else SetParent(editwnd->GetHWND(), g_hSHELLDLL_DefView);
        MARGINS m = { -1, -1, -1, -1 };
        if (DWMActive)
        {
            AddLayeredRef(fullscreenpopupbaseE->GetDisplayNode());
            SetGadgetFlags(fullscreenpopupbaseE->GetDisplayNode(), NULL, NULL);
            AddLayeredRef(SimpleViewTop->GetDisplayNode());
            SetGadgetFlags(SimpleViewTop->GetDisplayNode(), NULL, NULL);
            AddLayeredRef(SimpleViewBottom->GetDisplayNode());
            SetGadgetFlags(SimpleViewBottom->GetDisplayNode(), NULL, NULL);
            DwmExtendFrameIntoClientArea(editwnd->GetHWND(), &m);
        }
        //DwmExtendFrameIntoClientArea(editbgwnd->GetHWND(), &m);

        fullscreenAnimation3(dimensions.right * 0.7, dimensions.bottom * 0.7);

        parserEdit->CreateElement(L"simpleviewoverlay", nullptr, nullptr, nullptr, (Element**)&simpleviewoverlay);
        centeredE->Add((Element**)&simpleviewoverlay, 1);
        //parserEdit->CreateElement(L"deskpreviewmask", nullptr, nullptr, nullptr, (Element**)&deskpreviewmask);
        //centeredEBG->Add((Element**)&deskpreviewmask, 1);
        //deskpreviewmask->SetX(dimensions.right * 0.15);
        //deskpreviewmask->SetY(dimensions.bottom * 0.15);

        _UpdateSimpleViewContent(animate, animFlags);

        SimpleViewSettings->SetKeyFocus();
    }
}
