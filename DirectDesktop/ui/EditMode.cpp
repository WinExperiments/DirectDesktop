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
    DDScalableElement* simpleviewoverlay;
    TouchButton *SimpleViewTop, *SimpleViewBottom;
    Button *SimpleViewPower, *SimpleViewSearch, *SimpleViewSettings, *SimpleViewPages, *SimpleViewClose;
    TouchButton *nextpage, *prevpage;
    DDScalableRichText* pageinfo;
    Button* PageViewer;
    TouchEdit2* PV_EnterPage;

    HANDLE g_editSemaphore = CreateSemaphoreW(nullptr, 8, 8, nullptr);
    LPVOID timerPtr;

    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void ShowPageViewer(Element* elem, Event* iev);
    void RemoveSelectedPage(Element* elem, Event* iev);
    void SetSelectedPageHome(Element* elem, Event* iev);
    bool ValidateStrDigits(const WCHAR* str);
    DWORD WINAPI ReloadPV(LPVOID lpParam);

    LRESULT CALLBACK EditModeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_CLOSE:
                HideSimpleView(false);
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
                            DWORD dwReload;
                            HANDLE hReload = CreateThread(nullptr, 0, ReloadPV, nullptr, 0, &dwReload);
                            if (hReload) CloseHandle(hReload);
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
                            g_homePageID = page;
                            DWORD dwReload;
                            HANDLE hReload = CreateThread(nullptr, 0, ReloadPV, nullptr, 0, &dwReload);
                            if (hReload) CloseHandle(hReload);
                        }
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
                        if (PV_IconPreview->GetWidth() < 16 * g_flScaleFactor) PV_IconPreview->SetWidth(16 * g_flScaleFactor);
                        if (PV_IconPreview->GetHeight() < 16 * g_flScaleFactor) PV_IconPreview->SetHeight(16 * g_flScaleFactor);
                        PV_IconPreview->SetClass(L"groupthumbnail");
                        PV_IconPreview->SetDDCPIntensity(iconpm[yV->num]->GetDDCPIntensity());
                        PV_IconPreview->SetAssociatedColor(iconpm[yV->num]->GetAssociatedColor());
                    }
                    CSafeElementPtr<Element> PV_FolderGroup;
                    PV_FolderGroup.Assign(regElem<Element*>(L"PV_FolderGroup", PV_IconPreview));
                    PV_FolderGroup->SetVisible(true);
                    PV_FolderGroup->SetFontSize(g_touchmode ? static_cast<int>(g_iconsz * g_flScaleFactor * yV->fl1) : static_cast<int>(min(PV_IconPreview->GetWidth(), PV_IconPreview->GetHeight()) / 2.0));
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
                CValuePtr v;
                DDScalableElement* PV_PageInner{};
                parserEdit->CreateElement(L"PV_PageInner", nullptr, nullptr, nullptr, (Element**)&PV_PageInner);
                ((Element*)wParam)->Add((Element**)&PV_PageInner, 1);
                CSafeElementPtr<RichText> number;
                number.Assign(regElem<RichText*>(L"number", PV_PageInner));
                number->SetContentString(to_wstring(lParam).c_str());
                Element* pagetasks{};
                parserEdit->CreateElement(L"pagetasks", nullptr, nullptr, nullptr, &pagetasks);
                ((Element*)wParam)->Add(&pagetasks, 1);
                CSafeElementPtr<Element> PV_HomeBadge;
                PV_HomeBadge.Assign(regElem<Element*>(L"PV_HomeBadge", pagetasks));
                DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", pagetasks);
                if (((LVItem*)wParam)->GetPage() == g_homePageID)
                {
                    PV_Home->SetSelected(true);
                    PV_HomeBadge->SetSelected(true);
                }
                DynamicArray<Element*>* Children = ((Element*)wParam)->GetChildren(&v);
                if (Children->GetSize() == 2)
                {
                    DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", (Element*)wParam);
                    if (PV_Remove)
                    {
                        ((DDScalableElement*)PV_Remove)->SetDDCPIntensity(255);
                        ((DDScalableElement*)PV_Remove)->SetAssociatedColor(RGB(196, 43, 28));
                        PV_Remove->SetVisible(((Element*)wParam)->GetMouseWithin());
                        PV_Remove->SetAssociatedItem((LVItem*)wParam);
                        assignFn(PV_Remove, RemoveSelectedPage);
                    }
                }
                if (PV_Home)
                {
                    ((DDScalableElement*)PV_Home)->SetDDCPIntensity(255);
                    ((DDScalableElement*)PV_Home)->SetAssociatedColor(RGB(255, 102, 0));
                    PV_Home->SetVisible(((Element*)wParam)->GetMouseWithin());
                    PV_Home->SetAssociatedItem((LVItem*)wParam);
                    assignFn(PV_Home, SetSelectedPageHome);
                }
                assignExtendedFn((Element*)wParam, ShowPageOptionsOnHover);
                break;
            }
            case WM_USER + 3:
            {
                RearrangeIcons(true, false, true);
                PageViewer->DestroyAll(true);
                PageViewer->Destroy(true);
                Event* iev = new Event{ SimpleViewPages, Button::Click };
                ShowPageViewer(SimpleViewPages, iev);
                break;
            }
        }
        return CallWindowProc(WndProcEdit, hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK EditModeBGWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_WINDOWPOSCHANGING:
            {
                //((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
                return 0L;
                break;
            }
            case WM_CLOSE:
                return 0;
                break;
            case WM_DESTROY:
                return 0;
                break;
        }
        return CallWindowProc(WndProcEditBG, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI CreateDesktopPreview(LPVOID lpParam)
    {
        yValueEx* yV = (yValueEx*)lpParam; // These are NEVER deleted, they need to be deleted in a way that won't crash
        DesktopIcon di;
        if (!g_hiddenIcons && yV->num >= 0 && yV->peOptionalTarget1)
        {
            ApplyIcons(pm, iconpm, &di, false, yV->num, yV->fl1);
            SendMessageW(editwnd->GetHWND(), WM_USER + 1, (WPARAM)&di, (LPARAM)yV);
        }
        ((LVItem*)yV->peOptionalTarget1)->SetInternalXPos(((LVItem*)yV->peOptionalTarget1)->GetInternalXPos() - 1); // hack
        if (((LVItem*)yV->peOptionalTarget1)->GetInternalXPos() == 0)
        {
            PostMessageW(editwnd->GetHWND(), WM_USER + 2, (WPARAM)yV->peOptionalTarget1, ((LVItem*)yV->peOptionalTarget1)->GetPage());
        }
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
        SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
        Sleep(350);
        fullscreeninnerE->DestroyAll(true);
        fullscreeninnerE->Destroy(true);
        prevpage->DestroyAll(true);
        prevpage->Destroy(true);
        nextpage->DestroyAll(true);
        nextpage->Destroy(true);
        pEdit->DestroyAll(true);
        editwnd->DestroyWindow();
        //editbgwnd->DestroyWindow();
        return 0;
    }

    static int g_savedanim, g_savedanim2, g_savedanim3, g_savedanim4;

    void fullscreenAnimation3(int width, int height, float animstartscale, bool animate)
    {
        parserEdit->CreateElement(L"fullscreeninner", nullptr, nullptr, nullptr, (Element**)&fullscreeninnerE);
        centeredE->Add((Element**)&fullscreeninnerE, 1);
        static const int savedanim = centeredE->GetAnimation();
        static const int savedanim2 = fullscreeninnerE->GetAnimation();
        g_savedanim = savedanim;
        g_savedanim2 = savedanim2;
        PlaySimpleViewAnimation(centeredE, width, height, animate ? savedanim : NULL, animstartscale);
        PlaySimpleViewAnimation(fullscreeninnerE, width, height, animate ? savedanim2 : NULL, animstartscale);
        centeredE->SetBackgroundColor(0);
        fullscreenpopupbaseE->SetVisible(true);
        fullscreeninnerE->SetVisible(true);
        SimpleViewTop->SetAlpha(255);
        SimpleViewBottom->SetAlpha(255);
    }

    void fullscreenAnimation4()
    {
        SimpleViewTop->SetAlpha(0);
        SimpleViewBottom->SetAlpha(0);
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(nullptr, 0, animate7, nullptr, 0, &animThread);
        if (animThreadHandle) CloseHandle(animThreadHandle);
    }

    void HideSimpleView(bool animate)
    {
        if (g_touchmode) g_iconsz = 32;
        if (animate)
        {
            RECT dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            PlaySimpleViewAnimation(centeredE, dimensions.right, dimensions.bottom, animate ? g_savedanim : NULL, 0.7);
            PlaySimpleViewAnimation(fullscreeninnerE, dimensions.right, dimensions.bottom, animate ? g_savedanim2 : NULL, 0.7);
            PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right, dimensions.bottom, animate ? g_savedanim3 : NULL, 0.7);
            mainContainer->SetVisible(true);
            mainContainer->SetAlpha(255);
            g_editmode = false;
            fullscreenAnimation4();
        }
        else
        {
            mainContainer->SetVisible(true);
            mainContainer->SetAlpha(255);
            g_editmode = false;
            fullscreeninnerE->DestroyAll(true);
            fullscreeninnerE->Destroy(true);
            prevpage->DestroyAll(true);
            prevpage->Destroy(true);
            nextpage->DestroyAll(true);
            nextpage->Destroy(true);
            pEdit->DestroyAll(true);
            editwnd->DestroyWindow();
            //editbgwnd->DestroyWindow();
        }
    }

    void TriggerHSV(Element* elem, Event* iev)
    {
        static int validation;
        if (iev->uidType == TouchButton::Click && prevpage->GetMouseFocused() == false && nextpage->GetMouseFocused() == false)
        {
            HideSimpleView(false);
            if (elem == fullscreenpopupbaseE) SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
            return;
        }
        if (iev->uidType == Button::Click && SimpleViewPages->GetMouseFocused() == false)
        {
            validation++;
            if (validation % 3 == 1)
            {
                HideSimpleView(false);
                return;
            }
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

    DWORD WINAPI LoadNewPages(LPVOID lpParam)
    {
        Sleep(25);
        PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
        PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
        return 0;
    }

    DWORD WINAPI ReloadPV(LPVOID lpParam)
    {
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
            ((LVItem*)elem)->SetPage(page);
            if (page == 1) GoToPrevPage(elem, iev);
            else if (page == g_maxPageID) GoToNextPage(elem, iev);
            else if (page < g_currentPageID) GoToPrevPage(elem, iev);
            else if (page > g_currentPageID) GoToNextPage(elem, iev);
        }
    }

    void AddNewPage(Element* elem, Event* iev);

    void ClosePageViewer(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            PageViewer->DestroyAll(true);
            PageViewer->Destroy(true);
            g_invokedpagechange = true;
            DWORD dd;
            HANDLE thumbnailThread = CreateThread(nullptr, 0, LoadNewPages, nullptr, 0, &dd);
            if (thumbnailThread) CloseHandle(thumbnailThread);
        }
    }

    void ShowPageViewer(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            RECT dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            parserEdit->CreateElement(L"PageViewer", nullptr, nullptr, nullptr, (Element**)&PageViewer);
            pEdit->Add((Element**)&PageViewer, 1);
            CSafeElementPtr<Element> PageViewerTop;
            PageViewerTop.Assign(regElem<Element*>(L"PageViewerTop", PageViewer));
            PageViewerTop->SetAlpha(255);
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
            if (g_maxPageID <= 6)
            {
                CSafeElementPtr<Element> pagesrow1;
                pagesrow1.Assign(regElem<Element*>(L"pagesrow1", PageViewer));
                CSafeElementPtr<Element> pagesrow2;
                pagesrow2.Assign(regElem<Element*>(L"pagesrow2", PageViewer));
                pagesrow1->SetHeight(dimensions.bottom * 0.25);
                int row1 = g_maxPageID;
                int row2 = 0;
                if (g_maxPageID >= 3)
                {
                    pagesrow1->SetMargin(0, 0, 0, dimensions.right * 0.025);
                    pagesrow2->SetHeight(dimensions.bottom * 0.25);
                    row1 = ceil(g_maxPageID / 2.0);
                    row2 = floor(g_maxPageID / 2.0);
                }
                for (int i = 1; i <= g_maxPageID; i++)
                {
                    LVItem* PV_Page{};
                    parserEdit->CreateElement(L"PV_Page", nullptr, nullptr, nullptr, (Element**)&PV_Page);
                    PV_Page->SetWidth(dimensions.right * 0.25);
                    PV_Page->SetHeight(dimensions.bottom * 0.25);
                    PV_Page->SetMargin(dimensions.right * 0.025, 0, dimensions.right * 0.025, 0);
                    PV_Page->SetPage(i);
                    if (i <= row1)
                    {
                        pagesrow1->Add((Element**)&PV_Page, 1);
                    }
                    else pagesrow2->Add((Element**)&PV_Page, 1);
                    if (i == 1) assignFn(PV_Page, GoToPrevPage);
                    else if (i == g_maxPageID) assignFn(PV_Page, GoToNextPage);
                    else if (i < g_currentPageID) assignFn(PV_Page, GoToPrevPage);
                    else if (i > g_currentPageID) assignFn(PV_Page, GoToNextPage);
                    int remainingIcons = 1;
                    for (int j = 0; j < pm.size(); j++)
                    {
                        if (pm[j]->GetPage() != i) continue;
                        remainingIcons++;
                    }
                    PV_Page->SetInternalXPos(remainingIcons); // hack
                    yValueEx* yV = new yValueEx{ -1, 0.25, NULL, nullptr, nullptr, nullptr, nullptr, nullptr, PV_Page };
                    QueueUserWorkItem(CreateDesktopPreviewHelper, yV, 0);
                    for (int j = 0; j < pm.size(); j++)
                    {
                        if (pm[j]->GetPage() != i) continue;
                        yValueEx* yV = new yValueEx{ j, 0.25, NULL, nullptr, nullptr, nullptr, nullptr, nullptr, PV_Page };
                        QueueUserWorkItem(CreateDesktopPreviewHelper, yV, 0);
                    }
                }
            }
            else
            {
                CSafeElementPtr<Element> overflow;
                overflow.Assign(regElem<Element*>(L"overflow", PageViewer));
                overflow->SetVisible(true);
                PV_EnterPage = (TouchEdit2*)PageViewer->FindDescendent(StrToID(L"PV_EnterPage"));
                CSafeElementPtr<DDScalableButton> PV_ConfirmEnterPage;
                PV_ConfirmEnterPage.Assign(regElem<DDScalableButton*>(L"PV_ConfirmEnterPage", PageViewer));
                assignFn(PV_ConfirmEnterPage, EnterSelectedPage);
                CSafeElementPtr<Element> PV_EnterPagePreview;
                PV_EnterPagePreview.Assign(regElem<Element*>(L"PV_EnterPagePreview", PageViewer));
                assignExtendedFn(PV_EnterPage, UpdateEnterPagePreview);
                DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", PageViewer);
                if (PV_Remove)
                {
                    ((DDScalableElement*)PV_Remove)->SetDDCPIntensity(255);
                    ((DDScalableElement*)PV_Remove)->SetAssociatedColor(RGB(196, 43, 28));
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
                DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", PageViewer);
                if (PV_Home)
                {
                    ((DDScalableElement*)PV_Home)->SetDDCPIntensity(255);
                    ((DDScalableElement*)PV_Home)->SetAssociatedColor(RGB(255, 102, 0));
                    assignFn(PV_Home, SetSelectedPageHome);
                }
            }
        }
    }

    void AddNewPage(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click())
        {
            g_maxPageID++;
            PageViewer->DestroyAll(true);
            PageViewer->Destroy(true);
            RearrangeIcons(true, false, true);
            ShowPageViewer(elem, iev);
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
        if (iev->uidType == Button::Click)
        {
            timerPtr = elem;
            SetTimer(editwnd->GetHWND(), 2, 50, nullptr);
        }
    }

    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseWithinProp())
        {
            CValuePtr v;
            DynamicArray<Element*>* Children = elem->GetChildren(&v);
            if (Children->GetSize() == 2)
            {
                DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", elem);
                if (PV_Remove)
                {
                    ((DDScalableElement*)PV_Remove)->SetDDCPIntensity(255);
                    ((DDScalableElement*)PV_Remove)->SetAssociatedColor(RGB(196, 43, 28));
                    PV_Remove->SetVisible(elem->GetMouseWithin());
                    PV_Remove->SetAssociatedItem((LVItem*)elem);
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
            }
            DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", elem);
            if (PV_Home)
            {
                ((DDScalableElement*)PV_Home)->SetDDCPIntensity(255);
                ((DDScalableElement*)PV_Home)->SetAssociatedColor(RGB(255, 102, 0));
                PV_Home->SetVisible(elem->GetMouseWithin());
                PV_Home->SetAssociatedItem((LVItem*)elem);
                assignFn(PV_Home, SetSelectedPageHome);
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

    void ShowSimpleView(bool animate)
    {
        if (g_touchmode) g_iconsz = 64;
        g_editmode = true;
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
                               dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_NOINHERITLAYOUT, WS_POPUP, nullptr, 0x43, &editwnd);
        HWNDElement::Create(editwnd->GetHWND(), true, NULL, nullptr, &key5, (Element**)&editparent);
        DUIXmlParser::Create(&parserEdit, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        parserEdit->SetXMLFromResource(IDR_UIFILE6, HINST_THISCOMPONENT, HINST_THISCOMPONENT);

        parserEdit->CreateElement(L"editmode", editparent, nullptr, nullptr, &pEdit);
        //parserEdit->CreateElement(L"editmodeblur", editbgparent, NULL, NULL, &pEditBG);

        SetWindowLongPtrW(editwnd->GetHWND(), GWL_STYLE, 0x56003A40L);
        //SetWindowLongPtrW(editbgwnd->GetHWND(), GWL_EXSTYLE, 0xC0080800L);
        //SetParent(editbgwnd->GetHWND(), NULL);

        pEdit->SetVisible(true);
        pEdit->EndDefer(key5);
        //pEditBG->SetVisible(true);
        //pEditBG->EndDefer(key6);

        fullscreenpopupbaseE = regElem<TouchButton*>(L"fullscreenpopupbase", pEdit);
        popupcontainerE = regElem<Button*>(L"popupcontainer", pEdit);
        centeredE = regElem<Button*>(L"centered", pEdit);
        SimpleViewTop = regElem<TouchButton*>(L"SimpleViewTop", pEdit);
        SimpleViewBottom = regElem<TouchButton*>(L"SimpleViewBottom", pEdit);
        SimpleViewPower = regElem<Button*>(L"SimpleViewPower", pEdit);
        SimpleViewSearch = regElem<Button*>(L"SimpleViewSearch", pEdit);
        SimpleViewSettings = regElem<Button*>(L"SimpleViewSettings", pEdit);
        SimpleViewPages = regElem<Button*>(L"SimpleViewPages", pEdit);
        SimpleViewClose = regElem<Button*>(L"SimpleViewClose", pEdit);
        prevpage = regElem<TouchButton*>(L"prevpage", pEdit);
        nextpage = regElem<TouchButton*>(L"nextpage", pEdit);
        pageinfo = regElem<DDScalableRichText*>(L"pageinfo", pEdit);

        free(pel_GoToPrevPage), free(pel_GoToNextPage), free(pel_TriggerHSV1), free(pel_ShowShutdownDialog), free(pel_TriggerHSV2),
            free(pel_ShowSearchUI), free(pel_ShowSettings), free(pel_TriggerHSV3), free(pel_ShowPageViewer), free(pel_ExitWindow);
        pel_GoToPrevPage = (IElementListener*)assignFn(prevpage, GoToPrevPage, true);
        pel_GoToNextPage = (IElementListener*)assignFn(nextpage, GoToNextPage, true);
        pel_TriggerHSV1 = (IElementListener*)assignFn(fullscreenpopupbaseE, TriggerHSV, true);
        pel_ShowShutdownDialog = (IElementListener*)assignFn(SimpleViewPower, ShowShutdownDialog, true);
        pel_TriggerHSV2 = (IElementListener*)assignFn(SimpleViewPower, TriggerHSV, true);
        pel_ShowSearchUI = (IElementListener*)assignFn(SimpleViewSearch, ShowSearchUI, true);
        pel_ShowSettings = (IElementListener*)assignFn(SimpleViewSettings, ShowSettings, true);
        pel_TriggerHSV3 = (IElementListener*)assignFn(SimpleViewSettings, TriggerHSV, true);
        pel_ShowPageViewer = (IElementListener*)assignFn(SimpleViewPages, ShowPageViewer, true);
        pel_ExitWindow = (IElementListener*)assignFn(SimpleViewClose, ExitWindow, true);

        WndProcEdit = (WNDPROC)SetWindowLongPtrW(editwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeWindowProc);
        //WndProcEditBG = (WNDPROC)SetWindowLongPtrW(editbgwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeBGWindowProc);

        LPWSTR sheetName = g_theme ? (LPWSTR)L"edit" : (LPWSTR)L"editdark";
        StyleSheet* sheet = pEdit->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parserEdit->GetSheet(sheetName, &sheetStorage);
        pEdit->SetValue(Element::SheetProp, 1, sheetStorage);

        editwnd->Host(pEdit);
        editwnd->ShowWindow(SW_SHOW);
        //editbgwnd->Host(pEditBG);
        //editbgwnd->ShowWindow(SW_SHOW);
        SetParent(editwnd->GetHWND(), g_hSHELLDLL_DefView);
        MARGINS m = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(editwnd->GetHWND(), &m);
        //DwmExtendFrameIntoClientArea(editbgwnd->GetHWND(), &m);

        fullscreenAnimation3(dimensions.right * 0.7, dimensions.bottom * 0.7, 1.4285, animate);
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

        if (g_maxPageID != 1)
        {
            WCHAR currentPage[64];
            StringCchPrintfW(currentPage, 64, LoadStrFromRes(4026).c_str(), g_currentPageID, g_maxPageID);
            pageinfo->SetContentString(currentPage);
            if (g_currentPageID == g_homePageID)
            {
                CSafeElementPtr<RichText> SimpleViewHomeBadge;
                SimpleViewHomeBadge.Assign(regElem<RichText*>(L"SimpleViewHomeBadge", pEdit));
                SimpleViewHomeBadge->SetLayoutPos(0);
            }
        }
        else pageinfo->SetContentString(L" ");
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
        if (!g_invokedpagechange)
        {
            mainContainer->SetAlpha(0);
        }
        mainContainer->SetVisible(false);
        SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
        SimpleViewBottom->SetHeight(dimensions.bottom * 0.15);
        if (dimensions.bottom * 0.15 < 80 * g_flScaleFactor) SimpleViewTop->SetHeight(80 * g_flScaleFactor);
        if (dimensions.bottom * 0.15 < 112 * g_flScaleFactor) SimpleViewBottom->SetHeight(112 * g_flScaleFactor);
        parserEdit->CreateElement(L"simpleviewoverlay", nullptr, nullptr, nullptr, (Element**)&simpleviewoverlay);
        centeredE->Add((Element**)&simpleviewoverlay, 1);
        static const int savedanim3 = simpleviewoverlay->GetAnimation();
        g_savedanim3 = savedanim3;
        PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right * 0.7, dimensions.bottom * 0.7, animate ? savedanim3 : NULL, 1.4285);
        wnd->ShowWindow(SW_SHOW);

        //NativeHWNDHost::Create(L"DD_EditModeBlur", L"DirectDesktop Edit Mode Blur Helper", NULL, NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y,
        //  dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, WS_POPUP | WS_VISIBLE, NULL, 0x43, &editbgwnd);
        //HWNDElement::Create(editbgwnd->GetHWND(), true, NULL, NULL, &key6, (Element**)&editbgparent);

        //editbgwnd->ShowWindow(SW_SHOW);

        g_invokedpagechange = false;
    }
}
