#include "EditMode.h"
#include "DirectDesktop.h"
#include "BitmapHelper.h"
#include "StyleModifier.h"
#include "ShutdownDialog.h"
#include "SettingsHelper.h"
#include "SearchPage.h"
#include "DirectoryHelper.h"
#include <strsafe.h>
#include <uxtheme.h>
#include <dwmapi.h>

using namespace DirectUI; 

namespace DirectDesktop
{
    NativeHWNDHost* editwnd, * editbgwnd;
    HWNDElement* editparent, * editbgparent;
    DUIXmlParser* parser5;
    Element* pEdit, * pEditBG;
    unsigned long key5 = 0, key6 = 0;
    WNDPROC WndProc6, WndProc7;
    DDScalableElement* fullscreeninnerE;
    Element* popupcontainerE;
    TouchButton* fullscreenpopupbaseE;
    Button* centeredE;
    DDScalableElement* simpleviewoverlay;
    TouchButton* SimpleViewTop, * SimpleViewBottom;
    Button* SimpleViewPower, * SimpleViewSearch, * SimpleViewSettings, * SimpleViewPages, * SimpleViewClose;
    TouchButton* nextpage, * prevpage;
    DDScalableRichText* pageinfo;
    Button* PageViewer;
    DDScalableTouchEdit* PV_EnterPage;

    LPVOID timerPtr;

    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pv1, Value* pV2);
    void ShowPageViewer(Element* elem, Event* iev);
    void RemoveSelectedPage(Element* elem, Event* iev);
    void SetSelectedPageHome(Element* elem, Event* iev);
    bool ValidateStrDigits(const WCHAR* str);
    DWORD WINAPI ReloadPV(LPVOID lpParam);

    LRESULT CALLBACK EditModeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CLOSE:
            HideSimpleView(false);
            return 0;
            break;
        case WM_DESTROY:
            return 0;
            break;
        case WM_TIMER: {
            KillTimer(hWnd, wParam);
            switch (wParam) {
            case 1:
                if (timerPtr) {
                    Value* v;
                    int removedPage{};
                    if (((DDLVActionButton*)timerPtr)->GetAssociatedItem()) {
                        removedPage = ((DDLVActionButton*)timerPtr)->GetAssociatedItem()->GetPage();
                    }
                    else {
                        if (PV_EnterPage->GetContentString(&v) != nullptr) removedPage = _wtoi(PV_EnterPage->GetContentString(&v));
                        if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || removedPage < 1 || removedPage > maxPageID) {
                            MessageBeep(MB_OK);
                            WCHAR* errorcontent = new WCHAR[256];
                            StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), maxPageID);
                            DDNotificationBanner* ddnb{};
                            DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", LoadStrFromRes(4060).c_str(), errorcontent, 5, false);
                            delete[] errorcontent;
                            return 0;
                        }
                        for (int i = 0; i <= maxPageID; i++) {
                            int items = 0;
                            for (int j = 0; j < pm.size(); j++) {
                                if (pm[j]->GetPage() != i) continue;
                                items++;
                            }
                            if (items != 0 && i == removedPage) {
                                MessageBeep(MB_OK);
                                DDNotificationBanner* ddnb{};
                                DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_INFO, L"DDNB", LoadStrFromRes(4062).c_str(), LoadStrFromRes(4063).c_str(), 5, false);
                                return 0;
                            }
                        }
                    }
                    if (removedPage == homePageID) homePageID = 1;
                    else if (removedPage < maxPageID && removedPage < homePageID) homePageID--;
                    if (removedPage < maxPageID) {
                        for (int i = removedPage + 1; i <= maxPageID; i++) {
                            for (int j = 0; j < pm.size(); j++) {
                                if (pm[j]->GetPage() != i) continue;
                                pm[j]->SetPage(i - 1);
                            }
                        }
                    }
                    maxPageID--;
                    DWORD dwReload;
                    HANDLE hReload = CreateThread(0, 0, ReloadPV, NULL, 0, &dwReload);
                }
                break;
            case 2:
                if (timerPtr) {
                    Value* v;
                    int page{};
                    if (((DDLVActionButton*)timerPtr)->GetAssociatedItem()) {
                        page = ((DDLVActionButton*)timerPtr)->GetAssociatedItem()->GetPage();
                    }
                    else {
                        if (PV_EnterPage->GetContentString(&v) != nullptr) page = _wtoi(PV_EnterPage->GetContentString(&v));
                        if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || page < 1 || page > maxPageID) {
                            MessageBeep(MB_OK);
                            WCHAR* errorcontent = new WCHAR[256];
                            StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), maxPageID);
                            DDNotificationBanner* ddnb{};
                            DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", LoadStrFromRes(4060).c_str(), errorcontent, 5, false);
                            delete[] errorcontent;
                            return 0;
                        }
                    }
                    homePageID = page;
                    DWORD dwReload;
                    HANDLE hReload = CreateThread(0, 0, ReloadPV, NULL, 0, &dwReload);
                }
                break;
            }
            break;
        }
        case WM_USER + 1: {
            DesktopIcon* di = (DesktopIcon*)wParam;
            yValueEx* yV = (yValueEx*)lParam;
            DDScalableElement* PV_IconShadowPreview;
            DDScalableElement* PV_IconPreview;
            Element* PV_IconShortcutPreview;
            const WCHAR* iconshadow = touchmode ? L"PV_IconShadowTouchPreview" : L"PV_IconShadowPreview";
            const WCHAR* icon = touchmode ? L"PV_IconTouchPreview" : L"PV_IconPreview";
            const WCHAR* iconshortcut = touchmode ? L"PV_IconShortcutTouchPreview" : L"PV_IconShortcutPreview";
            parser5->CreateElement(iconshadow, NULL, NULL, NULL, (Element**)&PV_IconShadowPreview);
            parser5->CreateElement(icon, NULL, NULL, NULL, (Element**)&PV_IconPreview);
            parser5->CreateElement(iconshortcut, NULL, NULL, NULL, &PV_IconShortcutPreview);
            Element* pePreviewContainer = yV->peOptionalTarget;
            pePreviewContainer->Add((Element**)&PV_IconShadowPreview, 1);
            pePreviewContainer->Add((Element**)&PV_IconPreview, 1);
            pePreviewContainer->Add(&PV_IconShortcutPreview, 1);
            if (pm[yV->y]->GetHiddenState() == true) {
                PV_IconShadowPreview->SetAlpha(192);
                PV_IconPreview->SetAlpha(128);
            }
            if (touchmode) {
                PV_IconShadowPreview->SetX(pm[yV->y]->GetX() * yV->innerSizeX);
                PV_IconPreview->SetX(pm[yV->y]->GetX() * yV->innerSizeX);

                PV_IconShadowPreview->SetY(pm[yV->y]->GetY() * yV->innerSizeX);
                PV_IconPreview->SetY(pm[yV->y]->GetY() * yV->innerSizeX);

                PV_IconShadowPreview->SetWidth(pm[yV->y]->GetWidth() * yV->innerSizeX);
                PV_IconPreview->SetWidth(pm[yV->y]->GetWidth() * yV->innerSizeX);
                PV_IconShortcutPreview->SetWidth(globaliconsz * flScaleFactor * yV->innerSizeX);
                PV_IconShadowPreview->SetHeight(pm[yV->y]->GetHeight() * yV->innerSizeX);
                PV_IconPreview->SetHeight(pm[yV->y]->GetHeight() * yV->innerSizeX);
                PV_IconShortcutPreview->SetHeight(globaliconsz * flScaleFactor * yV->innerSizeX);

                PV_IconShadowPreview->SetDDCPIntensity(pm[yV->y]->GetDDCPIntensity());
                PV_IconShadowPreview->SetAssociatedColor(pm[yV->y]->GetAssociatedColor());
                PV_IconShortcutPreview->SetX(PV_IconPreview->GetX() + (PV_IconPreview->GetWidth() - PV_IconShortcutPreview->GetWidth()) / 2.0);
                PV_IconShortcutPreview->SetY(PV_IconPreview->GetY() + (PV_IconPreview->GetHeight() - PV_IconShortcutPreview->GetHeight()) / 2.0);
            }
            else {
                PV_IconShadowPreview->SetX((pm[yV->y]->GetX() + shadowpm[yV->y]->GetX()) * yV->innerSizeX);
                PV_IconPreview->SetX((pm[yV->y]->GetX() + iconpm[yV->y]->GetX()) * yV->innerSizeX);
                PV_IconShortcutPreview->SetX((pm[yV->y]->GetX() + shortpm[yV->y]->GetX()) * yV->innerSizeX);
                PV_IconShadowPreview->SetY((pm[yV->y]->GetY() + shadowpm[yV->y]->GetY()) * yV->innerSizeX);
                PV_IconPreview->SetY((pm[yV->y]->GetY() + iconpm[yV->y]->GetY()) * yV->innerSizeX);
                PV_IconShortcutPreview->SetY((pm[yV->y]->GetY() + shortpm[yV->y]->GetY()) * yV->innerSizeX);
                PV_IconPreview->SetWidth(iconpm[yV->y]->GetWidth() * yV->innerSizeX);
                PV_IconPreview->SetHeight(iconpm[yV->y]->GetHeight() * yV->innerSizeX);
            }
            if (treatdirasgroup && pm[yV->y]->GetGroupedDirState() == true) {
                if (!touchmode) {
                    if (PV_IconPreview->GetWidth() < 16 * flScaleFactor) PV_IconPreview->SetWidth(16 * flScaleFactor);
                    if (PV_IconPreview->GetHeight() < 16 * flScaleFactor) PV_IconPreview->SetHeight(16 * flScaleFactor);
                    PV_IconPreview->SetClass(L"groupthumbnail");
                    PV_IconPreview->SetDDCPIntensity(iconpm[yV->y]->GetDDCPIntensity());
                    PV_IconPreview->SetAssociatedColor(iconpm[yV->y]->GetAssociatedColor());
                }
                Element* PV_FolderGroup = regElem<Element*>(L"PV_FolderGroup", PV_IconPreview);
                PV_FolderGroup->SetVisible(true);
                PV_FolderGroup->SetFontSize(touchmode ? static_cast<int>(globaliconsz * flScaleFactor * yV->innerSizeX) : static_cast<int>(min(PV_IconPreview->GetWidth(), PV_IconPreview->GetHeight()) / 2.0));
            }
            if (pm[yV->y]->GetHiddenState() == false) {
                HBITMAP iconshadowbmp = di->iconshadow;
                Value* bitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshadowbmp);
                if (bitmapShadow != nullptr) {
                    PV_IconShadowPreview->SetValue(Element::ContentProp, 1, bitmapShadow);
                    bitmapShadow->Release();
                }
            }
            HBITMAP iconbmp = di->icon;
            Value* bitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
            DeleteObject(iconbmp);
            if (bitmap != nullptr) {
                PV_IconPreview->SetValue(Element::ContentProp, 1, bitmap);
                bitmap->Release();
            }
            HBITMAP iconshortcutbmp = di->iconshortcut;
            Value* bitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
            DeleteObject(iconshortcutbmp);
            if (bitmapShortcut != nullptr) {
                if (pm[yV->y]->GetShortcutState() == true) PV_IconShortcutPreview->SetValue(Element::ContentProp, 1, bitmapShortcut);
                bitmapShortcut->Release();
            }
            break;
        }
        case WM_USER + 2: {
            Value* v;
            DDScalableElement* PV_PageInner{};
            parser5->CreateElement(L"PV_PageInner", NULL, NULL, NULL, (Element**)&PV_PageInner);
            ((Element*)wParam)->Add((Element**)&PV_PageInner, 1);
            RichText* number = regElem<RichText*>(L"number", PV_PageInner);
            number->SetContentString(to_wstring(lParam).c_str());
            Element* pagetasks{};
            parser5->CreateElement(L"pagetasks", NULL, NULL, NULL, &pagetasks);
            ((Element*)wParam)->Add(&pagetasks, 1);
            Element* PV_HomeBadge = regElem<Element*>(L"PV_HomeBadge", pagetasks);
            DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", pagetasks);
            if (((LVItem*)wParam)->GetPage() == homePageID) {
                PV_Home->SetSelected(true);
                PV_HomeBadge->SetSelected(true);
            }
            assignExtendedFn((Element*)wParam, ShowPageOptionsOnHover);
            break;
        }
        case WM_USER + 3: {
            RearrangeIcons(true, false, true);
            PageViewer->DestroyAll(true);
            PageViewer->Destroy(true);
            Event* iev = new Event{ SimpleViewPages, Button::Click };
            ShowPageViewer(SimpleViewPages, iev);
            break;
        }
        }
        return CallWindowProc(WndProc6, hWnd, uMsg, wParam, lParam);
    }
    LRESULT CALLBACK EditModeBGWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_WINDOWPOSCHANGING: {
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
        return CallWindowProc(WndProc7, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI CreateDesktopPreview(LPVOID lpParam) {
        InitThread(TSM_DESKTOP_DYNAMIC);
        yValueEx* yV = (yValueEx*)lpParam;
        DesktopIcon di;
        if (!hiddenIcons && yV->y >= 0 && yV->peOptionalTarget) {
            ApplyIcons(pm, iconpm, &di, false, yV->y, yV->innerSizeX);
            SendMessageW(editwnd->GetHWND(), WM_USER + 1, (WPARAM)&di, (LPARAM)yV);
        }
        ((LVItem*)yV->peOptionalTarget)->SetInternalXPos(((LVItem*)yV->peOptionalTarget)->GetInternalXPos() - 1); // hack
        if (((LVItem*)yV->peOptionalTarget)->GetInternalXPos() == 0) {
            PostMessageW(editwnd->GetHWND(), WM_USER + 2, (WPARAM)yV->peOptionalTarget, ((LVItem*)yV->peOptionalTarget)->GetPage());
        }
        return 0;
    }

    bool ValidateStrDigits(const WCHAR* str) {
        if (!str || *str == L'\0') return false;
        while (*str) {
            if (!iswdigit(*str)) return false;
            ++str;
        }
        return true;
    }

    DWORD WINAPI animate7(LPVOID lpParam) {
        SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
        Sleep(350);
        editwnd->DestroyWindow();
        //editbgwnd->DestroyWindow();
        return 0;
    }

    static int g_savedanim, g_savedanim2, g_savedanim3, g_savedanim4;
    void fullscreenAnimation3(int width, int height, float animstartscale, bool animate) {
        parser5->CreateElement(L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninnerE);
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

    void fullscreenAnimation4() {
        SimpleViewTop->SetAlpha(0);
        SimpleViewBottom->SetAlpha(0);
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(0, 0, animate7, NULL, 0, &animThread);
    }

    void HideSimpleView(bool animate) {
        if (touchmode) globaliconsz = 32;
        if (animate) {
            RECT dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            PlaySimpleViewAnimation(centeredE, dimensions.right, dimensions.bottom, animate ? g_savedanim : NULL, 0.7);
            PlaySimpleViewAnimation(fullscreeninnerE, dimensions.right, dimensions.bottom, animate ? g_savedanim2 : NULL, 0.7);
            PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right, dimensions.bottom, animate ? g_savedanim3 : NULL, 0.7);
            mainContainer->SetVisible(true);
            mainContainer->SetAlpha(255);
            editmode = false;
            fullscreenAnimation4();
        }
        else {
            mainContainer->SetVisible(true);
            mainContainer->SetAlpha(255);
            editmode = false;
            editwnd->DestroyWindow();
            //editbgwnd->DestroyWindow();
        }
    }

    void TriggerHSV(Element* elem, Event* iev) {
        static int validation;
        if (iev->uidType == TouchButton::Click && prevpage->GetMouseFocused() == false && nextpage->GetMouseFocused() == false) {
            HideSimpleView(false);
            return;
        }
        if (iev->uidType == Button::Click && SimpleViewPages->GetMouseFocused() == false) {
            validation++;
            if (validation % 3 == 1) {
                HideSimpleView(false);
                return;
            }
        }
    }

    void ShowShutdownDialog(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            DisplayShutdownDialog();
        }
    }
    void ShowSearchUI(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            CreateSearchPage();
        }
    }

    DWORD WINAPI LoadNewPages(LPVOID lpParam) {
        Sleep(25);
        PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
        PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
        return 0;
    }
    DWORD WINAPI ReloadPV(LPVOID lpParam) {
        Sleep(100);
        SendMessageW(editwnd->GetHWND(), WM_USER + 3, NULL, NULL);
        return 0;
    }

    void UpdateEnterPagePreview(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
        if (pProp == Element::KeyWithinProp()) {
            Element* PV_EnterPagePreview = regElem<Element*>(L"PV_EnterPagePreview", PageViewer);
            Value* v;
            PV_EnterPagePreview->SetVisible(!elem->GetKeyWithin());
            PV_EnterPagePreview->SetContentString(elem->GetContentString(&v));
        }
    }
    void EnterSelectedPage(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            Value* v;
            int page{};
            if (PV_EnterPage->GetContentString(&v) != nullptr) page = _wtoi(PV_EnterPage->GetContentString(&v));
            if (!ValidateStrDigits(PV_EnterPage->GetContentString(&v)) || page < 1 || page > maxPageID) {
                MessageBeep(MB_OK);
                WCHAR* errorcontent = new WCHAR[256];
                StringCchPrintfW(errorcontent, 256, LoadStrFromRes(4061).c_str(), maxPageID);
                DDNotificationBanner* ddnb{};
                DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_ERROR, L"DDNB", LoadStrFromRes(4060).c_str(), errorcontent, 5, false);
                delete[] errorcontent;
                return;
            }
            ((LVItem*)elem)->SetPage(page);
            if (page == 1) GoToPrevPage(elem, iev);
            else if (page == maxPageID) GoToNextPage(elem, iev);
            else if (page < currentPageID) GoToPrevPage(elem, iev);
            else if (page > currentPageID) GoToNextPage(elem, iev);
        }
    }
    void AddNewPage(Element* elem, Event* iev);
    void ClosePageViewer(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            PageViewer->DestroyAll(true);
            PageViewer->Destroy(true);
            invokedpagechange = true;
            DWORD dd;
            HANDLE thumbnailThread = CreateThread(0, 0, LoadNewPages, NULL, 0, &dd);
        }
    }
    void ShowPageViewer(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            RECT dimensions;
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            parser5->CreateElement(L"PageViewer", NULL, NULL, NULL, (Element**)&PageViewer);
            pEdit->Add((Element**)&PageViewer, 1);
            Element* PageViewerTop = regElem<Element*>(L"PageViewerTop", PageViewer);
            PageViewerTop->SetAlpha(255);
            PageViewerTop->SetHeight(dimensions.bottom * 0.15);
            if (dimensions.bottom * 0.15 < 80 * flScaleFactor) PageViewerTop->SetHeight(80 * flScaleFactor);
            Button* PV_Back = regElem<Button*>(L"PV_Back", PageViewer);
            assignFn(PV_Back, ClosePageViewer);
            Button* PV_Add = regElem<Button*>(L"PV_Add", PageViewer);
            PV_Add->SetEnabled(isDefaultRes());
            if (!isDefaultRes()) PV_Add->SetAlpha(96);
            assignFn(PV_Add, AddNewPage);
            if (maxPageID <= 6) {
                Element* pagesrow1 = regElem<Element*>(L"pagesrow1", PageViewer);
                Element* pagesrow2 = regElem<Element*>(L"pagesrow2", PageViewer);
                pagesrow1->SetHeight(dimensions.bottom * 0.25);
                int row1 = maxPageID;
                int row2 = 0;
                if (maxPageID >= 3) {
                    pagesrow1->SetMargin(0, 0, 0, dimensions.right * 0.025);
                    pagesrow2->SetHeight(dimensions.bottom * 0.25);
                    row1 = ceil(maxPageID / 2.0);
                    row2 = floor(maxPageID / 2.0);
                }
                for (int i = 1; i <= maxPageID; i++) {
                    LVItem* PV_Page{};
                    parser5->CreateElement(L"PV_Page", NULL, NULL, NULL, (Element**)&PV_Page);
                    PV_Page->SetWidth(dimensions.right * 0.25);
                    PV_Page->SetHeight(dimensions.bottom * 0.25);
                    PV_Page->SetMargin(dimensions.right * 0.025, 0, dimensions.right * 0.025, 0);
                    PV_Page->SetPage(i);
                    if (i <= row1) {
                        pagesrow1->Add((Element**)&PV_Page, 1);
                    }
                    else pagesrow2->Add((Element**)&PV_Page, 1);
                    if (i == 1) assignFn(PV_Page, GoToPrevPage);
                    else if (i == maxPageID) assignFn(PV_Page, GoToNextPage);
                    else if (i < currentPageID) assignFn(PV_Page, GoToPrevPage);
                    else if (i > currentPageID) assignFn(PV_Page, GoToNextPage);
                    int remainingIcons = 1;
                    for (int j = 0; j < pm.size(); j++) {
                        if (pm[j]->GetPage() != i) continue;
                        remainingIcons++;
                    }
                    PV_Page->SetInternalXPos(remainingIcons); // hack
                    yValueEx* yV = new yValueEx{ -1, 0.25, NULL, NULL, NULL, NULL, NULL, NULL, PV_Page };
                    DWORD dwDeskPreview;
                    HANDLE hDeskPreview = CreateThread(0, 0, CreateDesktopPreview, yV, 0, &dwDeskPreview);
                    for (int j = 0; j < pm.size(); j++) {
                        if (pm[j]->GetPage() != i) continue;
                        yValueEx* yV = new yValueEx{ j, 0.25, NULL, NULL, NULL, NULL, NULL, NULL, PV_Page };
                        DWORD dwDeskPreview;
                        HANDLE hDeskPreview = CreateThread(0, 0, CreateDesktopPreview, yV, 0, &dwDeskPreview);
                    }
                }
            }
            else {
                Element* overflow = regElem<Element*>(L"overflow", PageViewer);
                overflow->SetVisible(true);
                PV_EnterPage = (DDScalableTouchEdit*)PageViewer->FindDescendent(StrToID(L"PV_EnterPage"));
                DDScalableButton* PV_ConfirmEnterPage = regElem<DDScalableButton*>(L"PV_ConfirmEnterPage", PageViewer);
                assignFn(PV_ConfirmEnterPage, EnterSelectedPage);
                Element* PV_EnterPagePreview = regElem<Element*>(L"delaysecondspreview", PageViewer);
                assignExtendedFn(PV_EnterPage, UpdateEnterPagePreview);
                DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", PageViewer);
                if (PV_Remove) {
                    ((DDScalableElement*)PV_Remove)->SetDDCPIntensity(255);
                    ((DDScalableElement*)PV_Remove)->SetAssociatedColor(RGB(196, 43, 28));
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
                DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", PageViewer);
                if (PV_Home) {
                    ((DDScalableElement*)PV_Home)->SetDDCPIntensity(255);
                    ((DDScalableElement*)PV_Home)->SetAssociatedColor(RGB(255, 102, 0));
                    assignFn(PV_Home, SetSelectedPageHome);
                }
            }
        }
    }
    void AddNewPage(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click()) {
            maxPageID++;
            PageViewer->DestroyAll(true);
            PageViewer->Destroy(true);
            RearrangeIcons(true, false, true);
            ShowPageViewer(elem, iev);
        }
    }
    void RemoveSelectedPage(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            timerPtr = elem;
            SetTimer(editwnd->GetHWND(), 1, 50, NULL);
        }
    }
    void SetSelectedPageHome(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            timerPtr = elem;
            SetTimer(editwnd->GetHWND(), 2, 50, NULL);
        }
    }
    void ShowPageOptionsOnHover(Element* elem, const PropertyInfo* pProp, int type, Value* pv1, Value* pV2) {
        if (pProp == Element::MouseWithinProp()) {
            static Value* v;
            DynamicArray<Element*>* Children = elem->GetChildren(&v);
            if (Children->GetSize() == 2) {
                DDLVActionButton* PV_Remove = regElem<DDLVActionButton*>(L"PV_Remove", elem);
                if (PV_Remove) {
                    ((DDScalableElement*)PV_Remove)->SetDDCPIntensity(255);
                    ((DDScalableElement*)PV_Remove)->SetAssociatedColor(RGB(196, 43, 28));
                    PV_Remove->SetVisible(elem->GetMouseWithin());
                    PV_Remove->SetAssociatedItem((LVItem*)elem);
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
            }
            DDLVActionButton* PV_Home = regElem<DDLVActionButton*>(L"PV_Home", elem);
            if (PV_Home) {
                ((DDScalableElement*)PV_Home)->SetDDCPIntensity(255);
                ((DDScalableElement*)PV_Home)->SetAssociatedColor(RGB(255, 102, 0));
                PV_Home->SetVisible(elem->GetMouseWithin());
                PV_Home->SetAssociatedItem((LVItem*)elem);
                assignFn(PV_Home, SetSelectedPageHome);
            }
        }
    }
    void ExitWindow(Element* elem, Event* iev) {
        if (iev->uidType == Button::Click) {
            SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
            SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, NULL);
        }
    }

    void ShowSimpleView(bool animate) {
        if (touchmode) globaliconsz = 64;
        editmode = true;
        if (!invokedpagechange) SendMessageW(hWndTaskbar, WM_COMMAND, 419, 0);
        RECT dimensions;
        POINT topLeftMon = GetTopLeftMonitor();
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        if (localeType == 1) {
            int rightMon = GetRightMonitor();
            topLeftMon.x = dimensions.right + dimensions.left - rightMon;
        }
        NativeHWNDHost::Create(L"DD_EditModeHost", L"DirectDesktop Edit Mode", NULL, NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y,
            dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_NOINHERITLAYOUT, WS_POPUP, NULL, 0, &editwnd);
        HWNDElement::Create(editwnd->GetHWND(), true, NULL, NULL, &key5, (Element**)&editparent);
        DUIXmlParser::Create(&parser5, NULL, NULL, NULL, NULL);
        parser5->SetXMLFromResource(IDR_UIFILE6, HINST_THISCOMPONENT, HINST_THISCOMPONENT);

        parser5->CreateElement(L"editmode", editparent, NULL, NULL, &pEdit);
        //parser5->CreateElement(L"editmodeblur", editbgparent, NULL, NULL, &pEditBG);

        SetWindowLongPtrW(editwnd->GetHWND(), GWL_STYLE, 0x56003A40L);
        //SetWindowLongPtrW(editbgwnd->GetHWND(), GWL_EXSTYLE, 0xC0080800L);
        //SetParent(editbgwnd->GetHWND(), NULL);

        pEdit->SetVisible(true);
        AddLayeredRef(pEdit->GetDisplayNode());
        SetGadgetFlags(pEdit->GetDisplayNode(), 0x1, 0x1);
        pEdit->EndDefer(key5);
        //pEditBG->SetVisible(true);
        //AddLayeredRef(pEditBG->GetDisplayNode());
        //SetGadgetFlags(pEditBG->GetDisplayNode(), 0x1, 0x1);
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

        assignFn(prevpage, GoToPrevPage);
        assignFn(nextpage, GoToNextPage);
        assignFn(fullscreenpopupbaseE, TriggerHSV);
        assignFn(SimpleViewPower, ShowShutdownDialog);
        assignFn(SimpleViewPower, TriggerHSV);
        assignFn(SimpleViewSearch, ShowSearchUI);
        assignFn(SimpleViewSettings, ShowSettings);
        assignFn(SimpleViewSettings, TriggerHSV);
        assignFn(SimpleViewPages, ShowPageViewer);
        assignFn(SimpleViewClose, ExitWindow);

        WndProc6 = (WNDPROC)SetWindowLongPtrW(editwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeWindowProc);
        //WndProc7 = (WNDPROC)SetWindowLongPtrW(editbgwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeBGWindowProc);

        LPWSTR sheetName = theme ? (LPWSTR)L"edit" : (LPWSTR)L"editdark";
        StyleSheet* sheet = pEdit->GetSheet();
        Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parser5->GetSheet(sheetName, &sheetStorage);
        pEdit->SetValue(Element::SheetProp, 1, sheetStorage);

        editwnd->Host(pEdit);
        editwnd->ShowWindow(SW_SHOW);
        //editbgwnd->Host(pEditBG);
        //editbgwnd->ShowWindow(SW_SHOW);
        SetParent(editwnd->GetHWND(), hSHELLDLL_DefView);
        MARGINS m = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(editwnd->GetHWND(), &m);
        //DwmExtendFrameIntoClientArea(editbgwnd->GetHWND(), &m);

        fullscreenAnimation3(dimensions.right * 0.7, dimensions.bottom * 0.7, 1.4285, animate);
        for (int j = 0; j < pm.size(); j++) {
            Element* peContainer{};
            float peContainerScale{};
            if (pm[j]->GetPage() == currentPageID) {
                peContainer = fullscreeninnerE;
                peContainerScale = 0.7;
            }
            else if (pm[j]->GetPage() == currentPageID - 1) {
                peContainer = prevpage;
                peContainerScale = 0.5;
            }
            else if (pm[j]->GetPage() == currentPageID + 1) {
                peContainer = nextpage;
                peContainerScale = 0.5;
            }
            else continue;
            yValueEx* yV = new yValueEx{ j, peContainerScale, NULL, NULL, NULL, NULL, NULL, NULL, peContainer };
            DWORD dwDeskPreview;
            HANDLE hDeskPreview = CreateThread(0, 0, CreateDesktopPreview, yV, 0, &dwDeskPreview);
        }

        if (maxPageID != 1) {
            WCHAR currentPage[64];
            StringCchPrintfW(currentPage, 64, LoadStrFromRes(4026).c_str(), currentPageID, maxPageID);
            pageinfo->SetContentString(currentPage);
            if (currentPageID == homePageID) {
                RichText* SimpleViewHomeBadge = regElem<RichText*>(L"SimpleViewHomeBadge", pEdit);
                SimpleViewHomeBadge->SetLayoutPos(0);
            }
        }
        else pageinfo->SetContentString(L" ");
        if (currentPageID != maxPageID) {
            float xLoc = (localeType == 1) ? -0.4 : 0.9;
            TogglePage(nextpage, xLoc, 0.25, 0.5, 0.5);
        }
        if (currentPageID != 1) {
            float xLoc = (localeType == 1) ? 0.9 : -0.4;
            TogglePage(prevpage, xLoc, 0.25, 0.5, 0.5);
        }
        if (!invokedpagechange) {
            mainContainer->SetAlpha(0);
        }
        mainContainer->SetVisible(false);
        SimpleViewTop->SetHeight(dimensions.bottom * 0.15);
        SimpleViewBottom->SetHeight(dimensions.bottom * 0.15);
        if (dimensions.bottom * 0.15 < 80 * flScaleFactor) SimpleViewTop->SetHeight(80 * flScaleFactor);
        if (dimensions.bottom * 0.15 < 112 * flScaleFactor) SimpleViewBottom->SetHeight(112 * flScaleFactor);
        parser5->CreateElement(L"simpleviewoverlay", NULL, NULL, NULL, (Element**)&simpleviewoverlay);
        centeredE->Add((Element**)&simpleviewoverlay, 1);
        static const int savedanim3 = simpleviewoverlay->GetAnimation();
        g_savedanim3 = savedanim3;
        PlaySimpleViewAnimation(simpleviewoverlay, dimensions.right * 0.7, dimensions.bottom * 0.7, animate ? savedanim3 : NULL, 1.4285);
        wnd->ShowWindow(SW_SHOW);

        //NativeHWNDHost::Create(L"DD_EditModeBlur", L"DirectDesktop Edit Mode Blur Helper", NULL, NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, WS_POPUP | WS_VISIBLE, NULL, 0, &editbgwnd);
        //HWNDElement::Create(editbgwnd->GetHWND(), true, NULL, NULL, &key6, (Element**)&editbgparent);

        //editbgwnd->ShowWindow(SW_SHOW);

        invokedpagechange = false;
    }
}