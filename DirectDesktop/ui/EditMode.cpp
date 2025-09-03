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
    DDScalableElement* fullscreeninnerE;
    Element* popupcontainerE;
    TouchButton* fullscreenpopupbaseE;
    Button* centeredE;
    Button* centeredEBG;
    DDScalableElement* simpleviewoverlay;
    DDScalableElement* deskpreviewmask;
    TouchButton *SimpleViewTop, *SimpleViewBottom;
    Element *SimpleViewTopInner, *SimpleViewBottomInner;
    Button *SimpleViewPower, *SimpleViewSearch, *SimpleViewSettings, *SimpleViewPages, *SimpleViewClose;
    DDScalableTouchButton *nextpage, *prevpage;
    DDScalableRichText* pageinfo;
    Button* PageViewer;
    TouchEdit2* PV_EnterPage;
    Element* EM_Dim;
    Element* bg_left_top, *bg_left_middle, *bg_left_bottom, *bg_right_top, *bg_right_middle, *bg_right_bottom;

    HANDLE g_editSemaphore = CreateSemaphoreW(nullptr, 16, 16, nullptr);
    LPVOID timerPtr;

    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void ClosePageViewer(Element* elem, Event* iev);
    void ShowPageViewer(Element* elem, Event* iev);
    void RemoveSelectedPage(Element* elem, Event* iev);
    void SetSelectedPageHome(Element* elem, Event* iev);
    void _UpdateSimpleViewContent(bool animate, DWORD animFlags);
    bool ValidateStrDigits(const WCHAR* str);
    bool g_animatePVEnter = true;
    DWORD WINAPI ReloadPV(LPVOID lpParam);

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
                break;
            case WM_DESTROY:
                return 0;
                break;
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
                                if (PV_EnterPage->GetContentString(&v) != nullptr) removedPage = _wtoi(PV_EnterPage->GetContentString(&v));
                                if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || removedPage < 1 || removedPage > g_maxPageID)
                                {
                                    MessageBeep(MB_OK);
                                    WCHAR* errorcontent = new WCHAR[256];
                                    StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), g_maxPageID);
                                    DDNotificationBanner* ddnb{};
                                    DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", LoadStrFromRes(4060).c_str(), errorcontent, 5, false);
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
                                        DDNotificationBanner* ddnb{};
                                        DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_INFO, L"DDNB", LoadStrFromRes(4062).c_str(), LoadStrFromRes(4063).c_str(), 5, false);
                                        return 0;
                                    }
                                }
                            }
                            ((DDLVActionButton*)timerPtr)->StopListening();
                            if (removedPage == g_homePageID) g_homePageID = 1;
                            else if (removedPage < g_maxPageID && removedPage < g_homePageID) g_homePageID--;
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
                            if (g_maxPageID <= 6)
                            {
                                DWORD dwReload;
                                HANDLE hReload = CreateThread(nullptr, 0, ReloadPV, nullptr, 0, &dwReload);
                                if (hReload) CloseHandle(hReload);
                            }
                        }
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
                                if (PV_EnterPage->GetContentString(&v) != nullptr) page = _wtoi(PV_EnterPage->GetContentString(&v));
                                if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || page < 1 || page > g_maxPageID)
                                {
                                    MessageBeep(MB_OK);
                                    WCHAR* errorcontent = new WCHAR[256];
                                    StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), g_maxPageID);
                                    DDNotificationBanner* ddnb{};
                                    DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", LoadStrFromRes(4060).c_str(), errorcontent, 5, false);
                                    delete[] errorcontent;
                                    return 0;
                                }
                            }
                            ((DDLVActionButton*)timerPtr)->StopListening();
                            g_homePageID = page;
                            if (g_maxPageID <= 6)
                            {
                                DWORD dwReload;
                                HANDLE hReload = CreateThread(nullptr, 0, ReloadPV, nullptr, 0, &dwReload);
                                if (hReload) CloseHandle(hReload);
                            }
                        }
                        break;
                    case 3: 
                        RefreshSimpleView(0x0);
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
                pePreviewContainer->Add((Element**)&PV_IconShadowPreview, 1);
                pePreviewContainer->Add((Element**)&PV_IconPreview, 1);
                pePreviewContainer->Add(&PV_IconShortcutPreview, 1);
                DUI_SetGadgetZOrder(PV_IconShadowPreview, -1);
                DUI_SetGadgetZOrder(PV_IconPreview, -1);
                DUI_SetGadgetZOrder(PV_IconShortcutPreview, -1);
                if (pm[yV->num]->GetHiddenState() == true)
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
                    PV_IconShortcutPreview->SetX(PV_IconPreview->GetX() + (PV_IconPreview->GetWidth() - PV_IconShortcutPreview->GetWidth()) / 2.0);
                    PV_IconShortcutPreview->SetY(PV_IconPreview->GetY() + (PV_IconPreview->GetHeight() - PV_IconShortcutPreview->GetHeight()) / 2.0);
                }
                else
                {
                    PV_IconShadowPreview->SetX((pm[yV->num]->GetX() + shadowpm[yV->num]->GetX()) * yV->fl1);
                    PV_IconPreview->SetX((pm[yV->num]->GetX() + iconpm[yV->num]->GetX()) * yV->fl1);
                    PV_IconShortcutPreview->SetX((pm[yV->num]->GetX() + shortpm[yV->num]->GetX()) * yV->fl1);
                    PV_IconShadowPreview->SetY((pm[yV->num]->GetY() + shadowpm[yV->num]->GetY()) * yV->fl1);
                    PV_IconPreview->SetY((pm[yV->num]->GetY() + iconpm[yV->num]->GetY()) * yV->fl1);
                    PV_IconShortcutPreview->SetY((pm[yV->num]->GetY() + shortpm[yV->num]->GetY()) * yV->fl1);
                    PV_IconPreview->SetWidth(iconpm[yV->num]->GetWidth() * yV->fl1);
                    PV_IconPreview->SetHeight(iconpm[yV->num]->GetHeight() * yV->fl1);
                }
                if (g_treatdirasgroup && pm[yV->num]->GetGroupedDirState() == true)
                {
                    if (!g_touchmode)
                    {
                        if (PV_IconPreview->GetWidth() < 16 && PV_IconPreview->GetHeight() < 16) PV_IconPreview->SetBorderThickness(0, 0, 0, 0);
                        PV_IconPreview->SetClass(L"groupthumbnail");
                        PV_IconPreview->SetDDCPIntensity(iconpm[yV->num]->GetDDCPIntensity());
                        PV_IconPreview->SetAssociatedColor(iconpm[yV->num]->GetAssociatedColor());
                    }
                    int foldericonsize = (pm[yV->num]->GetTileSize() == LVITS_ICONONLY) ? 32 : g_iconsz;
                    CSafeElementPtr<Element> PV_FolderGroup;
                    PV_FolderGroup.Assign(regElem<Element*>(L"PV_FolderGroup", PV_IconPreview));
                    PV_FolderGroup->SetVisible(true);
                    int glyphiconsize = min(PV_IconPreview->GetWidth(), PV_IconPreview->GetHeight());
                    float sizeCoef = (log(glyphiconsize / (yV->fl1 * g_iconsz)) / log(100)) + 1;
                    PV_FolderGroup->SetFontSize(g_touchmode ? static_cast<int>(foldericonsize * g_flScaleFactor * yV->fl1) : static_cast<int>(glyphiconsize / (2.0f * sizeCoef)));
                }
                if (pm[yV->num]->GetHiddenState() == false)
                {
                    HBITMAP iconshadowbmp = di->iconshadow;
                    CValuePtr spvBitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
                    DeleteObject(iconshadowbmp);
                    if (spvBitmapShadow != nullptr) PV_IconShadowPreview->SetValue(Element::ContentProp, 1, spvBitmapShadow);
                }
                HBITMAP iconbmp = di->icon;
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconbmp);
                if (spvBitmap != nullptr) PV_IconPreview->SetValue(Element::ContentProp, 1, spvBitmap);
                HBITMAP iconshortcutbmp = di->iconshortcut;
                CValuePtr spvBitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshortcutbmp);
                if (spvBitmapShortcut != nullptr && pm[yV->num]->GetShortcutState() == true) PV_IconShortcutPreview->SetValue(Element::ContentProp, 1, spvBitmapShortcut);
                break;
            }
            case WM_USER + 2:
            {
                break;
            }
            case WM_USER + 3:
            {
                SetPos(isDefaultRes());
                PageViewer->DestroyAll(true);
                PageViewer->Destroy(true);
                Event* iev = new Event{ SimpleViewPages, Button::Click };
                ShowPageViewer(SimpleViewPages, iev);
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
    //            return 0L;
    //            break;
    //        }
    //        case WM_CLOSE:
    //            return 0;
    //            break;
    //        case WM_DESTROY:
    //            return 0;
    //            break;
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
    void PV_CreateDimRect(Element* peParent, int cx, int cy)
    {
        Element* PV_PageRow_Dim2{};
        parserEdit->CreateElement(L"PV_PageRow_Dim", nullptr, nullptr, nullptr, &PV_PageRow_Dim2);
        PV_PageRow_Dim2->SetWidth(cx);
        PV_PageRow_Dim2->SetHeight(cy);
        peParent->Add(&PV_PageRow_Dim2, 1);
    }

    DWORD WINAPI CreateDesktopPreview(LPVOID lpParam)
    {
        yValueEx* yV = (yValueEx*)lpParam;
        DesktopIcon di;
        if (!g_hiddenIcons && yV->num >= 0 && yV->peOptionalTarget1)
        {
            if (pm[yV->num]->GetHasAdvancedIcon())
                HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            ApplyIcons(pm, &di, false, yV->num, yV->fl1, -1);
            SendMessageW(editwnd->GetHWND(), WM_USER + 1, (WPARAM)&di, (LPARAM)yV);
            if (pm[yV->num]->GetHasAdvancedIcon())
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
        Sleep(100 * (animCoef / 100.0f));
        SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
        Sleep(250 * (animCoef / 100.0f));
        //pEdit->DestroyAll(true);
        editwnd->DestroyWindow();
        //pEditBG->DestroyAll(true);
        //editbgwnd->DestroyWindow();
        return 0;
    }

    void fullscreenAnimation3(int width, int height, float animstartscale, bool animate)
    {
        parserEdit->CreateElement(L"fullscreeninner", nullptr, nullptr, nullptr, (Element**)&fullscreeninnerE);
        centeredE->Add((Element**)&fullscreeninnerE, 1);
        SetPopupSize(centeredE, width, height);
        SetPopupSize(fullscreeninnerE, width, height);
        fullscreenpopupbaseE->SetVisible(true);
        fullscreeninnerE->SetVisible(true);
    }

    void fullscreenAnimation4()
    {
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(nullptr, 0, animate7, nullptr, 0, &animThread);
        if (animThreadHandle) CloseHandle(animThreadHandle);
    }

    void HideSimpleView(bool animate)
    {
        if (g_touchmode) g_iconsz = 32;
        UIContainer->SetVisible(true);
        if (animate)
        {
            GTRANS_DESC transDesc[2];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerFade(UIContainer, transDesc, 0, 0.0f, 0.167f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleIn(UIContainer, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 0.7f, 0.7f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
            TriggerFade(fullscreenpopupbaseE, transDesc, 0, 0.0f, 0.167f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            TriggerScaleOut(fullscreenpopupbaseE, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, 0.5f, true, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, fullscreenpopupbaseE->GetDisplayNode(), &tsbInfo);
            TriggerFade(SimpleViewTop, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
            TriggerScaleOut(SimpleViewTop, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, 3.33f, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewTop->GetDisplayNode(), &tsbInfo);
            TriggerFade(SimpleViewBottom, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
            TriggerScaleOut(SimpleViewBottom, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, -3.33f, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewBottom->GetDisplayNode(), &tsbInfo);
            fullscreenAnimation4();
        }
        else
        {
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerFade(UIContainer, transDesc, 0, 0.0f, 0.167f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
            fullscreeninnerE->DestroyAll(true);
            fullscreeninnerE->Destroy(true);
            prevpage->DestroyAll(true);
            prevpage->Destroy(true);
            nextpage->DestroyAll(true);
            nextpage->Destroy(true);
            pEdit->DestroyAll(true);
            editwnd->DestroyWindow();
            //pEditBG->DestroyAll(true);
            //editbgwnd->DestroyWindow();
        }
        g_editmode = false;
        DUI_SetGadgetZOrder(UIContainer, -1);
    }

    void TriggerHSV(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click && !prevpage->GetMouseFocused() && !nextpage->GetMouseFocused())
        {
            HideSimpleView(true);
            return;
        }
        if (iev->uidType == Button::Click && !SimpleViewPages->GetMouseFocused() && !SimpleViewClose->GetMouseFocused() && !SimpleViewPower->GetMouseWithin())
        {
            if (SimpleViewSettings->GetMouseWithin() || SimpleViewSearch->GetMouseWithin()) HideSimpleView(false);
            else HideSimpleView(true);
            return;
        }
    }

    void ShowShutdownDialog(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            DisplayShutdownDialog();
        }
    }

    void ShowSearchUI(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CreateSearchPage();
        }
    }

    void TriggerEMToPV(bool fReverse)
    {
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
        if (fReverse)
        {
            g_animatePVEnter = true;
            g_invokedpagechange = true;
        }
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
                        if (((LVItem*)PV_Children->GetItem(i))->GetPage() == g_currentPageID) continue;
                        DynamicArray<Element*>* PageChildren = PV_Children->GetItem(i)->GetChildren(&v);
                        for (int j = 0; j < PageChildren->GetSize(); j++)
                        {
                            if (PageChildren->GetItem(j)->GetID() == StrToID(L"animateddimming"))
                            {
                                PageChildren->GetItem(j)->SetVisible(true);
                                TriggerFade(PageChildren->GetItem(j), transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade3, flFade4, !fReverse, false, !fReverse);
                            }
                            else TriggerFade(PageChildren->GetItem(j), transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, fReverse);
                            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, PageChildren->GetItem(j)->GetDisplayNode(), &tsbInfo);
                        }
                    }
                }
            }
            PV_Children = pagesrow2->GetChildren(&v);
        }
        if (fReverse)
        {
            TriggerFade(SimpleViewTop, transDesc, 0, 0.367f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewTop->GetDisplayNode(), &tsbInfo);
            TriggerFade(SimpleViewBottom, transDesc, 0, 0.367f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewBottom->GetDisplayNode(), &tsbInfo);
            TriggerFade(PageViewerTop, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, PageViewerTop->GetDisplayNode(), &tsbInfo);
        }
        else
        {
            CSafeElementPtr<Button> PV_Add;
            PV_Add.Assign(regElem<Button*>(L"PV_Add", PageViewer));
            if (!PV_Add->GetEnabled())
            {
                TriggerFade(PV_Add, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.375f, false, false, true);
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, PV_Add->GetDisplayNode(), &tsbInfo);
            }
            TriggerFade(SimpleViewTop, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewTop->GetDisplayNode(), &tsbInfo);
            TriggerFade(SimpleViewBottom, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewBottom->GetDisplayNode(), &tsbInfo);
            TriggerFade(PageViewerTop, transDesc, 0, 0.367f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, true);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, PageViewerTop->GetDisplayNode(), &tsbInfo);
        }
        if (g_maxPageID <= 6)
        {
            if (fReverse) TriggerScaleOut(PV_Inner, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 2.8f, 2.8f, ptPage.x, ptPage.y, false, true);
            else TriggerScaleIn(PV_Inner, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 2.8f, 2.8f, ptPage.x, ptPage.y, 1.0f, 1.0f, ptPage.x, ptPage.y, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, PV_Inner->GetDisplayNode(), &tsbInfo);
        }
        else
        {
            CSafeElementPtr<Element> overflow; overflow.Assign(regElem<Element*>(L"overflow", PV_Inner));
            if (fReverse) TriggerFade(overflow, transDesc, 0, 0.033f, 0.216f, 0.0f, 0.0f, 0.58f, 1.0f, 1.0f, 0.0f, false, false, true);
            else TriggerFade(overflow, transDesc, 0, 0.0f, 0.183f, 0.25f, 0.1f, 0.25f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, overflow->GetDisplayNode(), &tsbInfo);
            if (fReverse) TriggerScaleOut(PV_Inner, transDesc, 0, 0.033f, 0.3f, 1.0f, 1.0f, 0.0f, 1.0f, 1.15f, 1.15f, 0.5f, 0.5f, false, true);
            else TriggerScaleIn(PV_Inner, transDesc, 0, 0.0f, 0.267f, 0.0f, 0.0f, 0.0f, 1.0f, 1.15f, 1.15f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, PV_Inner->GetDisplayNode(), &tsbInfo);
        }
        if (fReverse)
        {
            TriggerFade(PageViewer, transDesc, 0, 0.3f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, true, true);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, PageViewer->GetDisplayNode(), &tsbInfo);
            TriggerFade(fullscreenpopupbaseE, transDesc, 0, 0.29f, 0.29f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, true, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, fullscreenpopupbaseE->GetDisplayNode(), &tsbInfo);
            DUI_SetGadgetZOrder(fullscreenpopupbaseE, -1);
        }
    }

    DWORD WINAPI ReloadPV(LPVOID lpParam)
    {
        g_animatePVEnter = false;
        Sleep(100);
        SendMessageW(editwnd->GetHWND(), WM_USER + 3, NULL, NULL);
        return 0;
    }

    void UpdateEnterPagePreview(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::KeyWithinProp())
        {
            CSafeElementPtr<DDScalableElement> PV_EnterPageBackground;
            PV_EnterPageBackground.Assign(regElem<DDScalableElement*>(L"PV_EnterPageBackground", PageViewer));
            CSafeElementPtr<Element> PV_EnterPagePreview;
            PV_EnterPagePreview.Assign(regElem<Element*>(L"PV_EnterPagePreview", PageViewer));
            CValuePtr v;
            PV_EnterPagePreview->SetVisible(!elem->GetKeyWithin());
            PV_EnterPagePreview->SetContentString(elem->GetContentString(&v));
            PV_EnterPageBackground->SetSelected(elem->GetKeyWithin());
        }
        if (pProp == Element::MouseWithinProp())
        {
            CSafeElementPtr<DDScalableElement> PV_EnterPageBackground;
            PV_EnterPageBackground.Assign(regElem<DDScalableElement*>(L"PV_EnterPageBackground", PageViewer));
            PV_EnterPageBackground->SetOverhang(elem->GetMouseWithin());
        }
        if (pProp == Element::EnabledProp())
        {
            CSafeElementPtr<DDScalableElement> PV_EnterPageBackground;
            PV_EnterPageBackground.Assign(regElem<DDScalableElement*>(L"PV_EnterPageBackground", PageViewer));
            PV_EnterPageBackground->SetEnabled(elem->GetEnabled());
        }
    }

    void EnterSelectedPage(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CValuePtr v;
            int page{};
            if (PV_EnterPage->GetContentString(&v) != nullptr) page = _wtoi(PV_EnterPage->GetContentString(&v));
            if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || page < 1 || page > g_maxPageID)
            {
                MessageBeep(MB_OK);
                WCHAR* errorcontent = new WCHAR[256];
                StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), g_maxPageID);
                DDNotificationBanner* ddnb{};
                DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", LoadStrFromRes(4060).c_str(), errorcontent, 5, false);
                delete[] errorcontent;
                return;
            }
            ((LVItem*)elem)->StopListening();
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
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDLVActionButton> PV_Home; PV_Home.Assign(regElem<DDLVActionButton*>(L"PV_Home", elem));
            if (PV_Home)
            {
                if (PV_Home->GetMouseWithin()) return;
            }
            RefreshSimpleView(0x0);
            TriggerEMToPV(true);
        }
    }

    void ShowPageViewer(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            fullscreenpopupbaseE->SetVisible(false);
            RECT dimensions;
            GetClientRect(editwnd->GetHWND(), &dimensions);
            parserEdit->CreateElement(L"PageViewer", nullptr, nullptr, nullptr, (Element**)&PageViewer);
            pEdit->Add((Element**)&PageViewer, 1);
            CSafeElementPtr<Element> PageViewerTop;
            PageViewerTop.Assign(regElem<Element*>(L"PageViewerTop", PageViewer));
            PageViewerTop->SetHeight(dimensions.bottom * 0.15);
            if (dimensions.bottom * 0.15 < 80 * g_flScaleFactor) PageViewerTop->SetHeight(80 * g_flScaleFactor);
            CSafeElementPtr<Button> PV_Back;
            PV_Back.Assign(regElem<Button*>(L"PV_Back", PageViewer));
            assignFn(PV_Back, ClosePageViewer);
            CSafeElementPtr<Button> PV_Add;
            PV_Add.Assign(regElem<Button*>(L"PV_Add", PageViewer));
            PV_Add->SetEnabled(isDefaultRes());
            if (!isDefaultRes()) PV_Add->SetAlpha(96);
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
                pagesrow1->SetHeight(round(dimensions.bottom * 0.25));
                int row1 = g_maxPageID;
                int row2 = 0;
                CSafeElementPtr<Element> bg_left; bg_left.Assign(regElem<Element*>(L"bg_left", PageViewer));
                CSafeElementPtr<Element> bg_top; bg_top.Assign(regElem<Element*>(L"bg_top", PageViewer));
                CSafeElementPtr<Element> bg_right; bg_right.Assign(regElem<Element*>(L"bg_right", PageViewer));
                CSafeElementPtr<Element> bg_bottom; bg_bottom.Assign(regElem<Element*>(L"bg_bottom", PageViewer));
                int horizEdgeFill = g_maxPageID >= 2 ? g_maxPageID >= 5 ? round(dimensions.right * 0.1) : round(dimensions.right * 0.2375) : round(dimensions.right * 0.375);
                int bottomYPos = floor(dimensions.bottom * 0.375) + round(dimensions.bottom * 0.25);
                SetTransElementPosition(bg_top, 0, 0, dimensions.right, floor(dimensions.bottom * 0.375));
                SetTransElementPosition(bg_bottom, 0, bottomYPos, dimensions.right, dimensions.bottom - bottomYPos);
                if (g_maxPageID >= 3)
                {
                    CSafeElementPtr<Element> rowpadding; rowpadding.Assign(regElem<Element*>(L"rowpadding", PageViewer));
                    rowpadding->SetHeight(round(dimensions.right * 0.025));
                    rowpadding->SetWidth(g_maxPageID >= 5 ? round(dimensions.right * 0.8) : round(dimensions.right * 0.525));
                    pagesrow2->SetHeight(round(dimensions.bottom * 0.25));
                    row1 = ceil(g_maxPageID / 2.0);
                    row2 = floor(g_maxPageID / 2.0);
                    bottomYPos = floor(dimensions.bottom * 0.25 - dimensions.right * 0.0125) + round(dimensions.right * 0.025) + round(dimensions.bottom * 0.25) * 2;
                    SetTransElementPosition(bg_top, 0, 0, dimensions.right, floor(dimensions.bottom * 0.25 - dimensions.right * 0.0125));
                    SetTransElementPosition(bg_bottom, 0, round(dimensions.bottom * 0.75 + dimensions.right * 0.0125), dimensions.right, round(dimensions.bottom * 0.25 - dimensions.right * 0.0125));
                }
                SetTransElementPosition(bg_left, 0, bg_top->GetHeight(), horizEdgeFill, dimensions.bottom - bg_top->GetHeight() - bg_bottom->GetHeight() + (g_maxPageID >= 3 ? 1 : 0));
                SetTransElementPosition(bg_right, dimensions.right - horizEdgeFill, bg_top->GetHeight(), horizEdgeFill, dimensions.bottom - bg_top->GetHeight() - bg_bottom->GetHeight() + (g_maxPageID >= 3 ? 1 : 0));
                for (int i = 1; i <= g_maxPageID; i++)
                {
                    LVItem* PV_Page{};
                    parserEdit->CreateElement(L"PV_Page", nullptr, nullptr, nullptr, (Element**)&PV_Page);
                    PV_Page->SetWidth(round(dimensions.right * 0.25));
                    PV_Page->SetHeight(round(dimensions.bottom * 0.25));
                    PV_Page->SetPage(i);
                    Element* PV_PageRow_Dim{};
                    parserEdit->CreateElement(L"PV_PageRow_Dim", nullptr, nullptr, nullptr, &PV_PageRow_Dim);
                    PV_PageRow_Dim->SetWidth(round(dimensions.right * 0.025));
                    PV_PageRow_Dim->SetHeight(round(dimensions.bottom * 0.25));
                    if (i <= row1)
                    {
                        pagesrow1->Add((Element**)&PV_Page, 1);
                        if (i < row1)
                            pagesrow1->Add(&PV_PageRow_Dim, 1);
                    }
                    else
                    {
                        if (g_maxPageID & 1 && i == row1 + 1)
                            PV_CreateDimRect(pagesrow2, round(dimensions.right * 0.1375), round(dimensions.bottom * 0.25));
                        pagesrow2->Add((Element**)&PV_Page, 1);
                        if (i < g_maxPageID)
                            pagesrow2->Add(&PV_PageRow_Dim, 1);
                        if (g_maxPageID & 1 && i == g_maxPageID)
                            PV_CreateDimRect(pagesrow2, round(dimensions.right * 0.1375), round(dimensions.bottom * 0.25));
                    }
                    if (i == g_currentPageID) assignFn(PV_Page, ClosePageViewer);
                    else if (i < g_currentPageID) assignFn(PV_Page, GoToPrevPage);
                    else if (i > g_currentPageID) assignFn(PV_Page, GoToNextPage);
                    int remainingIcons = 1;
                    PV_Page->SetHiddenState(true);
                    CSafeElementPtr<Element> PV_PageIcons;
                    PV_PageIcons.Assign(regElem<Element*>(L"PV_PageIcons", PV_Page));
                    for (int j = 0; j < pm.size(); j++)
                    {
                        if (pm[j]->GetPage() != i) continue;
                        remainingIcons++;
                        yValueEx* yV = new yValueEx{ j, 0.25, NULL, nullptr, nullptr, nullptr, nullptr, nullptr, PV_PageIcons };
                        PV_Page->SetHiddenState(false);
                        QueueUserWorkItem(CreateDesktopPreviewHelper, yV, 0);
                    }
                    CSafeElementPtr<RichText> number;
                    number.Assign(regElem<RichText*>(L"number", PV_Page));
                    number->SetContentString(to_wstring(i).c_str());
                    DUI_SetGadgetZOrder(number, 0);
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
                    assignExtendedFn(PV_Page, ShowPageOptionsOnHover);
                }
            }
            else
            {
                CSafeElementPtr<Element> bg; bg.Assign(regElem<Element*>(L"bg", PageViewer));
                bg->SetVisible(true);
                CSafeElementPtr<Element> overflow;
                overflow.Assign(regElem<Element*>(L"overflow", PageViewer));
                overflow->SetVisible(true);
                PV_EnterPage = (TouchEdit2*)PageViewer->FindDescendent(StrToID(L"PV_EnterPage"));
                CSafeElementPtr<LVItem> PV_ConfirmEnterPage;
                PV_ConfirmEnterPage.Assign(regElem<LVItem*>(L"PV_ConfirmEnterPage", PageViewer));
                assignFn(PV_ConfirmEnterPage, EnterSelectedPage);
                CSafeElementPtr<Element> PV_EnterPagePreview;
                PV_EnterPagePreview.Assign(regElem<Element*>(L"PV_EnterPagePreview", PageViewer));
                assignExtendedFn(PV_EnterPage, UpdateEnterPagePreview);
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
        if (iev->uidType == Button::Click())
        {
            g_animatePVEnter = false;
            g_maxPageID++;
            if (g_maxPageID <= 7)
            {
                SetPos(isDefaultRes());
                PageViewer->DestroyAll(true);
                PageViewer->Destroy(true);
                ShowPageViewer(elem, iev);
            }
        }
    }

    void RemoveSelectedPage(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            timerPtr = elem;
            SetTimer(editwnd->GetHWND(), 1, 50, nullptr);
        }
    }

    void SetSelectedPageHome(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click && g_maxPageID > 1)
        {
            timerPtr = elem;
            SetTimer(editwnd->GetHWND(), 2, 50, nullptr);
        }
    }

    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseWithinProp())
        {
            if (((LVItem*)elem)->GetHiddenState())
            {
                DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", elem);
                if (PV_Remove)
                {
                    PV_Remove->SetEnabled(isDefaultRes());
                    PV_Remove->SetVisible(elem->GetMouseWithin());
                    PV_Remove->SetAssociatedItem((LVItem*)elem);
                    assignFn(PV_Remove, RemoveSelectedPage);
                    DUI_SetGadgetZOrder(PV_Remove, 1);
                }
            }
            DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", elem);
            if (PV_Home)
            {
                PV_Home->SetEnabled(isDefaultRes());
                PV_Home->SetVisible(elem->GetMouseWithin());
                PV_Home->SetAssociatedItem((LVItem*)elem);
                if (((LVItem*)elem)->GetPage() != g_homePageID) assignFn(PV_Home, SetSelectedPageHome);
                DUI_SetGadgetZOrder(PV_Home, 1);
            }
        }
    }

    void ExitWindow(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
            SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, NULL);
        }
    }

    void _UpdateSimpleViewContent(bool animate, DWORD animFlags)
    {
        RECT dimensions;
        GetClientRect(editwnd->GetHWND(), &dimensions);
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
            yValueEx* yV = new yValueEx{ j, peContainerScale, NULL, nullptr, nullptr, nullptr, nullptr, nullptr, peContainer };
            QueueUserWorkItem(CreateDesktopPreviewHelper, yV, 0);
        }

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
        if (animate)
        {
            GTRANS_DESC transDesc[2];
            TriggerFade(UIContainer, transDesc, 0, 0.0f, 0.167f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            TriggerScaleOut(UIContainer, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 0.7f, 0.7f, 0.5f, 0.5f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
            TriggerFade(fullscreenpopupbaseE, transDesc, 0, 0.0f, 0.167f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleIn(fullscreenpopupbaseE, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, fullscreenpopupbaseE->GetDisplayNode(), &tsbInfo);
            DUI_SetGadgetZOrder(fullscreenpopupbaseE, -1);
            TriggerFade(SimpleViewTop, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleIn(SimpleViewTop, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, 3.33f, 1.0f, 1.0f, 0.5f, 3.33f, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewTop->GetDisplayNode(), &tsbInfo);
            DUI_SetGadgetZOrder(SimpleViewTop, 0);
            TriggerFade(SimpleViewBottom, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleIn(SimpleViewBottom, transDesc, 1, 0.0f, 0.33f, 0.1f, 0.9f, 0.2f, 1.0f, 1.4285f, 1.4285f, 0.5f, -3.33f, 1.0f, 1.0f, 0.5f, -3.33f, false, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, SimpleViewBottom->GetDisplayNode(), &tsbInfo);
            DUI_SetGadgetZOrder(SimpleViewBottom, 0);
        }
        else
        {
            UIContainer->SetVisible(false);
            GTRANS_DESC transDesc[2];
            TransitionStoryboardInfo tsbInfo = {};
            if (animFlags & 1)
            {

            }
            if (animFlags & 2)
            {

            }
            if (animFlags & 4)
            {

            }
            if (animFlags & 8)
            {

            }
        }

        SetPopupSize(centeredE, dimensions.right * 0.7, dimensions.bottom * 0.7);
        SetPopupSize(fullscreeninnerE, dimensions.right * 0.7, dimensions.bottom * 0.7);
        SetPopupSize(simpleviewoverlay, dimensions.right * 0.7, dimensions.bottom * 0.7);
        SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
        SimpleViewTopInner->SetHeight(dimensions.bottom * 0.15);
        SimpleViewBottom->SetHeight(dimensions.bottom - SimpleViewTop->GetHeight() - fullscreeninnerE->GetHeight());
        SimpleViewBottomInner->SetHeight(dimensions.bottom - SimpleViewTopInner->GetHeight() - fullscreeninnerE->GetHeight());
        if (dimensions.bottom * 0.15 < 80 * g_flScaleFactor) SimpleViewTop->SetHeight(80 * g_flScaleFactor);
        if (dimensions.bottom * 0.15 < 96 * g_flScaleFactor) SimpleViewBottom->SetHeight(96 * g_flScaleFactor);

        if (prevpage->GetWidth() > 1)
        {
            SetTransElementPosition(bg_left_top, round((localeType == 1) ? dimensions.right * 0.85 : 0), SimpleViewTop->GetHeight(),
                round(dimensions.right * 0.15), prevpage->GetY() - SimpleViewTop->GetHeight());

            SetTransElementPosition(bg_left_middle, round((localeType == 1) ? dimensions.right * 0.85 : dimensions.right * 0.1), bg_left_top->GetY() + bg_left_top->GetHeight(),
                round(dimensions.right * 0.05), prevpage->GetHeight());

            SetTransElementPosition(bg_left_bottom, round((localeType == 1) ? dimensions.right * 0.85 : 0), bg_left_middle->GetY() + bg_left_middle->GetHeight(),
                round(dimensions.right * 0.15), dimensions.bottom - SimpleViewBottom->GetHeight() - bg_left_middle->GetY() - bg_left_middle->GetHeight());
        }
        else
        {
            SetTransElementPosition(bg_left_top, 0, 0, 0, 0);
            SetTransElementPosition(bg_left_middle, round((localeType == 1) ? dimensions.right * 0.85 : 0), SimpleViewTop->GetHeight(),
                round(dimensions.right * 0.15), dimensions.bottom - SimpleViewTop->GetHeight() - SimpleViewBottom->GetHeight());
            SetTransElementPosition(bg_left_bottom, 0, 0, 0, 0);
        }
        if (nextpage->GetWidth() > 1)
        {
            SetTransElementPosition(bg_right_top, round((localeType == 1) ? 0 : dimensions.right * 0.85), SimpleViewTop->GetHeight(),
                round(dimensions.right * 0.15), nextpage->GetY() - SimpleViewTop->GetHeight());

            SetTransElementPosition(bg_right_middle, round((localeType == 1) ? dimensions.right * 0.1 : dimensions.right * 0.85), bg_right_top->GetY() + bg_right_top->GetHeight(),
                round(dimensions.right * 0.05), nextpage->GetHeight());

            SetTransElementPosition(bg_right_bottom, round((localeType == 1) ? 0 : dimensions.right * 0.85), bg_right_middle->GetY() + bg_right_middle->GetHeight(),
                round(dimensions.right * 0.15), dimensions.bottom - SimpleViewBottom->GetHeight() - bg_right_middle->GetY() - bg_right_middle->GetHeight());
        }
        else
        {
            SetTransElementPosition(bg_right_top, 0, 0, 0, 0);
            SetTransElementPosition(bg_right_middle, round((localeType == 1) ? 0 : dimensions.right * 0.85), SimpleViewTop->GetHeight(),
                round(dimensions.right * 0.15), dimensions.bottom - SimpleViewTop->GetHeight() - SimpleViewBottom->GetHeight());
            SetTransElementPosition(bg_right_bottom, 0, 0, 0, 0);
        }
        g_invokedpagechange = false;
    }
    void RefreshSimpleView(DWORD animFlags)
    {
        fullscreeninnerE->DestroyAll(true);
        prevpage->DestroyAll(true);
        nextpage->DestroyAll(true);
        _UpdateSimpleViewContent(false, animFlags);
    }
    void ShowSimpleView(bool animate, DWORD animFlags)
    {
        if (g_touchmode) g_iconsz = 64;
        g_editmode = true;
        g_animatePVEnter = true;
        if (!g_invokedpagechange) SendMessageW(g_hWndTaskbar, WM_COMMAND, 419, 0);
        static IElementListener *pel_GoToPrevPage, *pel_GoToNextPage, *pel_TriggerHSV1, *pel_ShowShutdownDialog, *pel_TriggerHSV2,
                                *pel_ShowSearchUI, *pel_ShowSettings, *pel_TriggerHSV3, *pel_ShowPageViewer, *pel_ExitWindow;
        RECT dimensions;
        POINT topLeftMon = GetTopLeftMonitor();
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        if (localeType == 1)
        {
            int rightMon = GetRightMonitor();
            topLeftMon.x = dimensions.right + dimensions.left - rightMon;
        }
        NativeHWNDHost::Create(L"DD_EditModeHost", L"DirectDesktop Edit Mode", nullptr, nullptr, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y,
                               9999, 9999, WS_EX_NOINHERITLAYOUT | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, WS_POPUP, nullptr, 0x43, &editwnd);
        HWNDElement::Create(editwnd->GetHWND(), true, 0x38, nullptr, &key5, (Element**)&editparent);
        //NativeHWNDHost::Create(L"DD_EditModeBlur", L"DirectDesktop Edit Mode Blur Helper", nullptr, NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y,
        //dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, NULL, WS_POPUP, nullptr, 0x43, &editbgwnd);
        //HWNDElement::Create(editbgwnd->GetHWND(), true, NULL, nullptr, &key6, (Element**)&editbgparent);
        DUIXmlParser::Create(&parserEdit, nullptr, nullptr, DUI_ParserErrorCB, nullptr);

        parserEdit->SetXMLFromResource(IDR_UIFILE6, HINST_THISCOMPONENT, HINST_THISCOMPONENT);

        parserEdit->CreateElement(L"editmode", editparent, nullptr, nullptr, &pEdit);
        //parserEdit->CreateElement(L"editmodeblur", editbgparent, nullptr, NULL, &pEditBG);

        SetWindowLongPtrW(editwnd->GetHWND(), GWL_STYLE, 0x56003A40L);
        //SetWindowLongPtrW(editbgwnd->GetHWND(), GWL_STYLE, 0x56003A40L);

        SetWindowPos(editwnd->GetHWND(), nullptr, NULL, NULL, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOMOVE | SWP_NOZORDER);
        GetClientRect(editwnd->GetHWND(), &dimensions);

        pEdit->SetVisible(true);
        pEdit->EndDefer(key5);
        //pEditBG->SetVisible(true);
        //pEditBG->EndDefer(key6);

        fullscreenpopupbaseE = regElem<TouchButton*>(L"fullscreenpopupbase", pEdit);
        popupcontainerE = regElem<Button*>(L"popupcontainer", pEdit);
        centeredE = regElem<Button*>(L"centered", pEdit);
        //centeredEBG = regElem<Button*>(L"centered", pEditBG);
        SimpleViewTop = regElem<TouchButton*>(L"SimpleViewTop", pEdit);
        SimpleViewBottom = regElem<TouchButton*>(L"SimpleViewBottom", pEdit);
        SimpleViewTopInner = regElem<Element*>(L"SimpleViewTopInner", pEdit);
        SimpleViewBottomInner = regElem<Element*>(L"SimpleViewBottomInner", pEdit);
        SimpleViewPower = regElem<Button*>(L"SimpleViewPower", pEdit);
        SimpleViewSearch = regElem<Button*>(L"SimpleViewSearch", pEdit);
        SimpleViewSettings = regElem<Button*>(L"SimpleViewSettings", pEdit);
        SimpleViewPages = regElem<Button*>(L"SimpleViewPages", pEdit);
        SimpleViewClose = regElem<Button*>(L"SimpleViewClose", pEdit);
        EM_Dim = regElem<Element*>(L"EM_Dim", pEdit);
        bg_left_top = regElem<Element*>(L"bg_left_top", pEdit);
        bg_left_middle = regElem<Element*>(L"bg_left_middle", pEdit);
        bg_left_bottom = regElem<Element*>(L"bg_left_bottom", pEdit);
        bg_right_top = regElem<Element*>(L"bg_right_top", pEdit);
        bg_right_middle = regElem<Element*>(L"bg_right_middle", pEdit);
        bg_right_bottom = regElem<Element*>(L"bg_right_bottom", pEdit);
        prevpage = (DDScalableTouchButton*)pEdit->FindDescendent(StrToID(L"prevpage"));
        nextpage = (DDScalableTouchButton*)pEdit->FindDescendent(StrToID(L"nextpage"));
        pageinfo = regElem<DDScalableRichText*>(L"pageinfo", pEdit);

        free(pel_GoToPrevPage), free(pel_GoToNextPage), free(pel_TriggerHSV1), free(pel_ShowShutdownDialog), free(pel_TriggerHSV2),
            free(pel_ShowSearchUI), free(pel_ShowSettings), free(pel_TriggerHSV3), free(pel_ShowPageViewer), free(pel_ExitWindow);
        pel_GoToPrevPage = (IElementListener*)assignFn(prevpage, GoToPrevPage, true);
        pel_GoToNextPage = (IElementListener*)assignFn(nextpage, GoToNextPage, true);
        pel_TriggerHSV1 = (IElementListener*)assignFn(fullscreenpopupbaseE, TriggerHSV, true);
        pel_ShowShutdownDialog = (IElementListener*)assignFn(SimpleViewPower, ShowShutdownDialog, true);
        pel_TriggerHSV2 = (IElementListener*)assignFn(SimpleViewTop, TriggerHSV, true);
        pel_ShowSearchUI = (IElementListener*)assignFn(SimpleViewSearch, ShowSearchUI, true);
        pel_ShowSettings = (IElementListener*)assignFn(SimpleViewSettings, ShowSettings, true);
        pel_TriggerHSV3 = (IElementListener*)assignFn(SimpleViewBottom, TriggerHSV, true);
        pel_ShowPageViewer = (IElementListener*)assignFn(SimpleViewPages, ShowPageViewer, true);
        pel_ExitWindow = (IElementListener*)assignFn(SimpleViewClose, ExitWindow, true);

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
        DwmExtendFrameIntoClientArea(editwnd->GetHWND(), &m);
        //DwmExtendFrameIntoClientArea(editbgwnd->GetHWND(), &m);

        fullscreenAnimation3(dimensions.right * 0.7, dimensions.bottom * 0.7, 1.4285f, animate);

        parserEdit->CreateElement(L"simpleviewoverlay", nullptr, nullptr, nullptr, (Element**)&simpleviewoverlay);
        centeredE->Add((Element**)&simpleviewoverlay, 1);
        SetPopupSize(simpleviewoverlay, round(dimensions.right * 0.7), round(dimensions.bottom * 0.7));
        parserEdit->CreateElement(L"deskpreviewmask", nullptr, nullptr, nullptr, (Element**)&deskpreviewmask);
        //centeredEBG->Add((Element**)&deskpreviewmask, 1);
        SetPopupSize(deskpreviewmask, round(dimensions.right * 0.7), round(dimensions.bottom * 0.7));
        deskpreviewmask->SetX(dimensions.right * 0.15);
        deskpreviewmask->SetY(dimensions.bottom * 0.15);

        _UpdateSimpleViewContent(animate, animFlags);

        wnd->ShowWindow(SW_SHOW);
        //editbgwnd->ShowWindow(SW_SHOW);
    }
}
