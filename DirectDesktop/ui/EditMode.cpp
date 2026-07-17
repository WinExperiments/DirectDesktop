#include "pch.h"

#include "EditMode.h"
#include "..\DirectDesktop.h"
#include "ShutdownDialog.h"
#include "..\backend\SettingsHelper.h"
#include "SearchPage.h"
#include "..\backend\DirectoryHelper.h"

using namespace DirectUI;
using namespace DDUI;

namespace DirectDesktop
{   
    typedef HWND(WINAPI* pfnSHCreateWorkerWindowW)(WNDPROC, HWND, DWORD, DWORD, LPVOID);

    NativeHWNDHost *editwnd, *editbgwnd, *editwndOld;
    HWNDElement *editparent, *editbgparent;
    DUIXmlParser* parserEdit;
    Element *pEdit, *pEditBG;
    unsigned long key5 = 0, key6 = 0;
    HWND edit_hWorker;
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
    TouchButton* SimpleViewPower, *SimpleViewSearch, *SimpleViewPrevPage, *SimpleViewNextPage;
    DDIconButton* SimpleViewSettings, *SimpleViewPages, *SimpleViewClose;
    DDScalableTouchButton *nextpage, *prevpage;
    DDScalableRichText* pageinfo;
    Element* PageViewer;
    DDScalableTouchEdit* PV_EnterPage;
    Element* EM_Dim;
    Element* bg_left_top, *bg_left_middle, *bg_left_bottom, *bg_right_top, *bg_right_middle, *bg_right_bottom;
    Element* bg_extras[7];
    
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
    bool g_animatePVEnter = true;
    bool g_editingpages = false;
    bool g_hiddentaskbar = true;
    bool g_animateSVLaunch = true;
    BYTE g_memTaskbarState = 255;

    LRESULT CALLBACK EditModeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_SIZE:
                SetTimer(hWnd, 3, 150, nullptr);
                break;
            case WM_CLOSE:
                HideSimpleView(true);
                SetTimer(wnd->GetHWND(), 4, 100, nullptr);
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
                                    const WCHAR* content = PV_EnterPage->GetContentString(&v);
                                    if (content) removedPage = _wtoi(content);
                                    if (!ValidateStrDigits(content) || removedPage < 1 || removedPage > g_maxPageID)
                                    {
                                        MessageBeep(MB_OK);
                                        WCHAR errorcontent[260], errorBuf[192], errorTitle[64];
                                        LoadStrFromRes(errorBuf, 192, 4061);
                                        StringCchPrintfW(errorcontent, 256, errorBuf, g_maxPageID);
                                        DDNotificationBanner* ddnb = new DDNotificationBanner();
                                        LoadStrFromRes(errorTitle, 64, 4060);
                                        ddnb->CreateBanner(DDNT_ERROR, errorTitle, errorcontent, 5, nullptr);
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
                                            WCHAR infotitle[64], infocontent[192];
                                            LoadStrFromRes(infotitle, 64, 4062);
                                            LoadStrFromRes(infocontent, 192, 4063);
                                            DDNotificationBanner* ddnb = new DDNotificationBanner();
                                            ddnb->CreateBanner(DDNT_INFO, infotitle, infocontent, 5, nullptr);
                                            return 0;
                                        }
                                    }
                                }
                                else
                                {
                                    MessageBeep(MB_OK);
                                    DDNotificationBanner* ddnb = new DDNotificationBanner();
                                    ddnb->CreateBanner(DDNT_ERROR, nullptr, nullptr, 3, nullptr);
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
                                    const WCHAR* content = PV_EnterPage->GetContentString(&v);
                                    if (content) page = _wtoi(content);
                                    if (!ValidateStrDigits(content) || page < 1 || page > g_maxPageID)
                                    {
                                        MessageBeep(MB_OK);
                                        WCHAR errorcontent[260], errorBuf[192], errorTitle[64];
                                        LoadStrFromRes(errorBuf, 192, 4061);
                                        StringCchPrintfW(errorcontent, 256, errorBuf, g_maxPageID);
                                        DDNotificationBanner* ddnb = new DDNotificationBanner();
                                        LoadStrFromRes(errorTitle, 64, 4060);
                                        ddnb->CreateBanner(DDNT_ERROR, errorTitle, errorcontent, 5, nullptr);
                                        return 0;
                                    }
                                }
                                else
                                {
                                    MessageBeep(MB_OK);
                                    DDNotificationBanner* ddnb = new DDNotificationBanner();
                                    ddnb->CreateBanner(DDNT_ERROR, nullptr, nullptr, 3, nullptr);
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
                    PV_IconShortcutPreview->SetWidth(g_iconsz * g_pctx->flScaleFactor * yV->fl1);
                    PV_IconShadowPreview->SetHeight(pm[yV->num]->GetHeight() * yV->fl1);
                    PV_IconPreview->SetHeight(pm[yV->num]->GetHeight() * yV->fl1);
                    PV_IconShortcutPreview->SetHeight(g_iconsz * g_pctx->flScaleFactor * yV->fl1);

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
                    PV_FolderGroup.Assign(regElem(L"PV_FolderGroup", PV_IconPreview));
                    if (peIcon->GetGroupColor() == 0)
                    {
                        if (g_isColorized)
                            PV_FolderGroup->SetForegroundColor(g_pColors->crPalette[iconColorID]);
                        else PV_FolderGroup->SetForegroundColor(g_pColors->crPalette[1]);
                    }
                    else PV_FolderGroup->SetForegroundColor(g_pColors->crPalette[peIcon->GetGroupColor()]);
                    PV_FolderGroup->SetVisible(true);
                    int glyphiconsize = min(PV_IconPreview->GetWidth(), PV_IconPreview->GetHeight());
                    float sizeCoef = (log(glyphiconsize / (yV->fl1 * g_iconsz * g_pctx->flScaleFactor)) / log(100)) + 1;
                    PV_FolderGroup->SetFontSize(g_touchmode ? static_cast<int>(foldericonsize * g_pctx->flScaleFactor * yV->fl1) : static_cast<int>(glyphiconsize / (2.0f * sizeCoef)));
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

    LRESULT CALLBACK EditWorkerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_TIMER:
        {
            static ULONGLONG ullTick;
            static BYTE taskbarState = 0;
            static RECT rc, rcOld, rcOld2, dimensions;
            static bool hiddentaskbarOld = false;
            static bool suppressedAnim = false;
            switch (wParam)
            {
            case 1:
            {
                if (g_autohidetaskbar)
                {
                    if (g_animateSVLaunch) hiddentaskbarOld = true;
                    GetWindowRect(g_hWndTaskbar, &rc);
                    GetWindowRect(editwnd->GetHWND(), &dimensions);
                    g_hiddentaskbar = (rc.right <= 4 * g_pctx->flScaleFactor || rc.bottom <= 4 * g_pctx->flScaleFactor ||
                        rc.left >= dimensions.right - 4 * g_pctx->flScaleFactor || rc.top >= dimensions.bottom - 4 * g_pctx->flScaleFactor ||
                        (rcOld.right > rc.right && rc.left <= 16 * g_pctx->flScaleFactor) ||
                        (rcOld.bottom > rc.bottom && rc.top <= 16 * g_pctx->flScaleFactor) ||
                        (rcOld.left < rc.left && rc.right >= dimensions.right - 16 * g_pctx->flScaleFactor) ||
                        (rcOld.top < rc.top && rc.bottom >= dimensions.bottom - 16 * g_pctx->flScaleFactor));
                    if (g_hiddentaskbar != hiddentaskbarOld)
                    {
                        if (rc.right <= dimensions.left + (dimensions.right - dimensions.left) / 2 && rc.left <= 16 * g_pctx->flScaleFactor)
                        {
                            taskbarState = 0;
                        }
                        if (rc.bottom <= dimensions.top + (dimensions.bottom - dimensions.top) / 2 && rc.top <= 16 * g_pctx->flScaleFactor)
                        {
                            taskbarState = 1;
                        }
                        if (rc.left >= dimensions.left + (dimensions.right - dimensions.left) / 2 && rc.right >= dimensions.right - 16 * g_pctx->flScaleFactor)
                        {
                            taskbarState = 2;
                        }
                        if (rc.top >= dimensions.top + (dimensions.bottom - dimensions.top) / 2 && rc.bottom >= dimensions.bottom - 16 * g_pctx->flScaleFactor)
                        {
                            taskbarState = 3;
                        }
                        DWORD animCoef = g_pctx->animCoef;
                        if (g_pctx->AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                        //DWORD dwMul = (g_hiddentaskbar && taskbarState == g_memTaskbarState) ? 7 : 3;
                        DWORD dwTickDelay = 3 * animCoef - (GetTickCount64() - ullTick);
                        if (dwTickDelay > 0x7FFFFFFF) dwTickDelay = 0;
                        SetTimer(hWnd, 2, dwTickDelay, nullptr);
                    }
                    hiddentaskbarOld = g_hiddentaskbar;
                    rcOld = rc;
                    g_animateSVLaunch = false;
                }
                break;
            }
            case 2:
            {
                KillTimer(hWnd, 2);
                short direction = g_hiddentaskbar ? -1 : 1;
                short direction2 = (taskbarState & 2) ? -1 : 1;
                if (!(taskbarState & 1))
                {
                    if (taskbarState & 2)
                    {
                        if (rc.right - rc.left >= dimensions.right - prevpage->GetX() - 32 * g_pctx->flScaleFactor)
                            SimpleViewPrevPage->SetVisible(!g_hiddentaskbar && g_currentPageID > 1);
                        if (rc.right - rc.left >= dimensions.right - nextpage->GetX() - 32 * g_pctx->flScaleFactor)
                            SimpleViewNextPage->SetVisible(!g_hiddentaskbar && g_currentPageID < g_maxPageID);
                    }
                    else
                    {
                        if (rc.right - rc.left >= dimensions.left + prevpage->GetX() + prevpage->GetWidth() - 32 * g_pctx->flScaleFactor)
                            SimpleViewPrevPage->SetVisible(!g_hiddentaskbar && g_currentPageID > 1);
                        if (rc.right - rc.left >= dimensions.left + nextpage->GetX() + nextpage->GetWidth() - 32 * g_pctx->flScaleFactor)
                            SimpleViewNextPage->SetVisible(!g_hiddentaskbar && g_currentPageID < g_maxPageID);
                    }
                }
                else
                {
                    SimpleViewPrevPage->SetVisible(false);
                    SimpleViewNextPage->SetVisible(false);
                }
                BYTE transSize = 0;
                POINT extra{};
                if (taskbarState != g_memTaskbarState && g_memTaskbarState != 255)
                {
                    direction *= -1;
                    short direction3 = (g_memTaskbarState & 2) ? 1 : -1;
                    switch (g_memTaskbarState)
                    {
                    case 0:
                    case 2:
                        extra.x = (rcOld2.right - rcOld2.left) * direction3;
                        break;
                    case 1:
                    case 3:
                        extra.y = (rcOld2.bottom - rcOld2.top) * direction3;
                        break;
                    }
                }
                if (!suppressedAnim)
                {
                    GTRANS_DESC transDesc[2];
                    TransitionStoryboardInfo tsbInfo = {};
                    //float flDelay = (g_hiddentaskbar && taskbarState == g_memTaskbarState) ? 0.4f : 0.0f;
                    switch (taskbarState)
                    {
                    case 0:
                    case 2:
                        TriggerTranslate(SimpleViewTop, transDesc, 0, 0.0f, 0.3f, 0.11, 0.6f, 0.23f, 0.97f,
                            0, 0, (rc.right - rc.left) * direction * direction2, (g_memTaskbarState & 2) ? 0 : extra.y, false, false, true);
                        TriggerTranslate(SimpleViewBottom, transDesc, 1, 0.0f, 0.3f, 0.11, 0.6f, 0.23f, 0.97f,
                            0, 0, (rc.right - rc.left) * direction * direction2, (g_memTaskbarState & 2) ? extra.y : 0, false, false, true);
                        break;
                    case 1:
                    case 3:
                        TriggerTranslate((taskbarState & 2) ? SimpleViewTop : SimpleViewBottom, transDesc, 0, 0.0f, 0.3f, 0.11, 0.6f, 0.23f, 0.97f,
                            0, 0, extra.x, 0, false, false, true);
                        TriggerTranslate((taskbarState & 2) ? SimpleViewBottom : SimpleViewTop, transDesc, 1, 0.0f, 0.3f, 0.11, 0.6f, 0.23f, 0.97f,
                            0, 0, extra.x, (rc.bottom - rc.top) * direction * direction2, false, false, true);
                        break;
                    }
                    ScheduleGadgetTransitions_DWMCheck(0, 2, transDesc, nullptr, &tsbInfo);
                    ullTick = GetTickCount64();
                }
                if (taskbarState != g_memTaskbarState || g_memTaskbarState == 255)
                    rcOld2 = rc;
                suppressedAnim = taskbarState != g_memTaskbarState && g_memTaskbarState != 255;
                if (taskbarState != 255)
                    g_memTaskbarState = taskbarState;
                break;
            }
            }
            break;
        }
        default:
            break;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    void SetTransElementPosition(Element* pe, int x, int y, int cx, int cy)
    {
        pe->SetX(x);
        pe->SetY(y);
        pe->SetWidth(cx);
        pe->SetHeight(cy);
        pe->SetVisible(cx != 0 && cy != 0);
    }
    void EM_CreateDimRect(Element** ppe, Element*& peParent, int x, int y, int cx, int cy)
    {
        Element::Create(0, peParent, nullptr, ppe);
        peParent->Add(ppe, 1);
        (*ppe)->SetLayoutPos(-2);
        (*ppe)->SetClass(L"popupbg");
        SetTransElementPosition(*ppe, x, y, cx, cy);
    }
    void EM_CreateRoundOverlayRect(Element** ppe, Element*& peParent, int x, int y, int cx, int cy)
    {
        DDScalableElement::Create(peParent, nullptr, ppe);
        peParent->Add(ppe, 1);
        (*ppe)->SetLayoutPos(-2);
        (*ppe)->SetClass(L"roundoverlay");
        SetTransElementPosition(*ppe, x, y, cx, cy);
    }
    void PV_CreateDimRect(Element* peParent, int cx, int cy, int idx)
    {
        Element* PV_PageRow_Dim2{};
        parserEdit->CreateElement(L"PV_PageRow_Dim", nullptr, nullptr, nullptr, &PV_PageRow_Dim2);
        PV_PageRow_Dim2->SetWidth(cx);
        PV_PageRow_Dim2->SetHeight(cy);
        if (idx == -1)
            peParent->Add(&PV_PageRow_Dim2, 1);
        else
            peParent->Insert(&PV_PageRow_Dim2, 1, idx);
        peParent->SetWidth(peParent->GetWidth() + cx);
    }
    float EM_GetRectAniWScale(Element* pe)
    {
        return (pe->GetWidth() + centeredE->GetWidth()) / static_cast<float>(pe->GetWidth());
    }
    void PV_SetEnterPageDesc()
    {
        CSafeElementPtr<DDScalableRichText> PV_EnterPageDesc;
        PV_EnterPageDesc.Assign((DDScalableRichText*)regElem(L"PV_EnterPageDesc", PageViewer));
        WCHAR desccontent[256], descBuf[256];
        LoadStrFromRes(descBuf, 256, 4061);
        StringCchPrintfW(desccontent, 256, descBuf, g_maxPageID);
        PV_EnterPageDesc->SetContentString(desccontent);
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
        delete yV;
        return 0;
    }

    DWORD WINAPI CreateDesktopPreviewHelper(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        WaitForSingleObject(g_editSemaphore, 0);
        yValueEx* yV = static_cast<yValueEx*>(lpParam);
        CreateDesktopPreview(yV);
        ReleaseSemaphore(g_editSemaphore, 1, nullptr);
        UnInitThread();
        return 0;
    }

    DWORD WINAPI animate7(LPVOID lpParam)
    {
        DWORD animCoef = g_pctx->animCoef;
        if (g_pctx->AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        Sleep(400 * (animCoef / 100.0f));
        //pEdit->DestroyAll(true);
        editwnd->DestroyWindow();
        //pEditBG->DestroyAll(true);
        //editbgwnd->DestroyWindow();
        KillTimer(edit_hWorker, 1);
        DestroyWindow(edit_hWorker);
        g_memTaskbarState = 255;
        g_animateSVLaunch = true;
        return 0;
    }

    void fullscreenAnimation3(int width, int height)
    {
        parserEdit->CreateElement(L"fullscreeninner", nullptr, nullptr, nullptr, (Element**)&fullscreeninnerE);
        centeredE->Add((Element**)&fullscreeninnerE, 1);
        centeredE->SetWidth(width);
        centeredE->SetHeight(height);
        //SetPopupSize(fullscreeninnerE, width, height);
        fullscreenpopupbaseE->SetVisible(true);
        fullscreeninnerE->SetVisible(true);
        assignFn(fullscreeninnerE, TriggerHSV, true);
    }

    void fullscreenAnimation4()
    {
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(nullptr, 0, animate7, nullptr, 0, &animThread);
        if (animThreadHandle) CloseHandle(animThreadHandle);
    }

    void HideSimpleView(bool fullanimate)
    {
        if (g_editmode && g_editavailable)
        {
            g_editmode = false;
            g_editavailable = false;
            g_invokedpagechange = false;
            if (g_touchmode) g_iconsz = 32;
            UIContainer->SetVisible(true);
            if (fullanimate)
            {
                SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
                SetFocus(wnd->GetHWND());
            }
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
            GTRANS_DESC transDesc2[1];
            TransitionStoryboardInfo tsbInfo = {};
            for (int items = 0; items < pm.size(); items++)
            {
                if (pm[items]->GetPage() != g_currentPageID)
                    pm[items]->SetSelected(false);
                else
                {
                    TriggerTranslate(pm[items], transDesc2, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX(), pm[items]->GetY(), false, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, pm[items]->GetDisplayNode(), &tsbInfo);
                    DUI_SetGadgetZOrder(pm[items], -1);
                }
            }
            DWORD animCoef = g_pctx->animCoef;
            if (g_pctx->AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
            SetTimer(wnd->GetHWND(), 25, 500 * (animCoef / 100.0f), nullptr);
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
            WCHAR action[48];
            DDMenu* ddm = new DDMenu();
            ddm->CreatePopupMenu(false);
            LoadStrFromRes(action, 48, 3052, L"shutdownux.dll");
            ddm->AppendMenuW(MF_STRING, 2001, action);
            LoadStrFromRes(action, 48, 3034, L"shutdownux.dll");
            ddm->AppendMenuW(MF_STRING, 2002, action);
            LoadStrFromRes(action, 48, 3019, L"shutdownux.dll");
            ddm->AppendMenuW(MF_STRING, 2003, action);
            LoadStrFromRes(action, 48, 3022, L"shutdownux.dll");
            ddm->AppendMenuW(MF_STRING, 2004, action);
            LoadStrFromRes(action, 48, 3013, L"shutdownux.dll");
            ddm->AppendMenuW(MF_STRING, 2005, action);
            LoadStrFromRes(action, 48, 3016, L"shutdownux.dll");
            ddm->AppendMenuW(MF_STRING, 2006, action);
            ddm->AppendMenuW(MF_STRING, 2007, L"More...");
            for (int i = 1; i <= 6; i++)
            {
                WCHAR glyph[3];
                LoadStrFromRes(glyph, 3, i + 200);
                ddm->SetMenuItemGlyph(i + 2000, FALSE, glyph);
            }
            UINT uFlags = TPM_RIGHTBUTTON | TPM_CENTERALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_VERPOSANIMATION;
            if (g_pctx->localeType == 1) uFlags |= TPM_LAYOUTRTL;
   
            RECT rcMenu{}, rcLocation{}, rcWindow{};
            GetGadgetRect(elem->GetDisplayNode(), &rcLocation, 0xC);
            ddm->GetMenuRect(&rcMenu);
            GetWindowRect(editwnd->GetHWND(), &rcWindow);
            rcLocation.left += elem->GetWidth() / 2 + rcWindow.left;
            rcLocation.top += elem->GetHeight() + rcWindow.top;
            UINT uID = ddm->TrackPopupMenuEx(uFlags, rcLocation.left, rcLocation.top, editwnd->GetHWND(), nullptr);
            switch (uID)
            {
            case 2001:
            case 2002:
            case 2003:
            case 2004:
            case 2005:
            case 2006:
                delayedshutdownstatuses[uID - 2001] = true; // Used by shutdown dialog
                SendMessageW(shutdownwnd->GetHWND(), WM_USER + 2, NULL, uID - 2000);
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
            CreateSearchPage(false);
            HideSimpleView(false);
        }
    }

    void TriggerEMToPV(bool fReverse)
    {
        g_pageviewer = !fReverse;
        RECT dimensions, rcPage;
        GetClientRect(editwnd->GetHWND(), &dimensions);
        POINTFLOAT ptPage{};
        CSafeElementPtr<Element> PageViewerTop;
        PageViewerTop.Assign(regElem(L"PageViewerTop", PageViewer));
        CSafeElementPtr<Element> PV_Inner;
        PV_Inner.Assign(regElem(L"PV_Inner", PageViewer));
        if (!fReverse)
        {
            g_invokedpagechange = true;
        }
        g_animatePVEnter = fReverse;
        GTRANS_DESC transDesc[3];
        TransitionStoryboardInfo tsbInfo = {};
        CSafeElementPtr<Element> pagesrow1; pagesrow1.Assign(regElem(L"pagesrow1", PV_Inner));
        CSafeElementPtr<Element> pagesrow2; pagesrow2.Assign(regElem(L"pagesrow2", PV_Inner));
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
                        if (((LVItem*)PV_Children->GetItem(i))->GetPage() == g_currentPageID)
                        {
                            GetGadgetRect(PV_Children->GetItem(i)->GetDisplayNode(), &rcPage, 0xC);
                            ptPage.x = CalcAnimOrigin(0.5f, (rcPage.left + rcPage.right) / (2.0f * dimensions.right), 0.7f, 0.25f);
                            ptPage.y = CalcAnimOrigin(0.5f, (rcPage.top + rcPage.bottom) / (2.0f * dimensions.bottom), 0.7f, 0.25f);
                        }
                        DynamicArray<Element*>* PageChildren = PV_Children->GetItem(i)->GetChildren(&v);
                        if (((LVItem*)PV_Children->GetItem(i))->GetPage() == g_currentPageID)
                        {
                            for (int j = 0; j < PageChildren->GetSize(); j++)
                            {
                                Element* child = PageChildren->GetItem(j);
                                if (child->GetID() == StrToID(L"pagetasks"))
                                {
                                    child->SetVisible(true);
                                    TriggerFade(child, transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, nullptr, &tsbInfo);
                                }
                                if (child->GetID() == StrToID(L"PV_PageIcons"))
                                {
                                    DynamicArray<Element*>* PVPIChildren = child->GetChildren(&v);
                                    for (int k = 0; k < PVPIChildren->GetSize(); k++)
                                    {
                                        Element* child2 = PVPIChildren->GetItem(k);
                                        if (child2->GetID() == StrToID(L"number"))
                                        {
                                            child2->SetVisible(true);
                                            TriggerFade(child2, transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, nullptr, &tsbInfo);
                                            DUI_SetGadgetZOrder(child2, 4);
                                        }
                                    }
                                }
                            }
                            continue;
                        }
                        for (int j = 0; j < PageChildren->GetSize(); j++)
                        {
                            Element* child = PageChildren->GetItem(j);
                            if (PageChildren->GetItem(j)->GetID() == StrToID(L"animateddimming"))
                            {
                                child->SetVisible(true);
                                TriggerFade(child, transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade3, flFade4, !fReverse, false, false);
                            }
                            else
                            {
                                if (child->GetID() == StrToID(L"PV_PageIcons"))
                                {
                                    DynamicArray<Element*>* PVPIChildren = child->GetChildren(&v);
                                    for (int k = 0; k < PVPIChildren->GetSize(); k++)
                                    {
                                        Element* child2 = PVPIChildren->GetItem(k);
                                        if (PVPIChildren->GetItem(k)->GetID() == StrToID(L"number"))
                                        {
                                            child2->SetVisible(true);
                                            TriggerFade(child2, transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, nullptr, &tsbInfo);
                                            DUI_SetGadgetZOrder(child2, 4);
                                        }
                                    }
                                }
                                TriggerFade(child, transDesc, 0, flFade1, flFade2, 0.0f, 0.0f, 1.0f, 1.0f, flFade4, flFade3, false, false, true);
                            }
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, nullptr, &tsbInfo);
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
            TriggerFade(SimpleViewBottom, transDesc, 1, 0.433f, 0.566f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerFade(PageViewerTop, transDesc, 2, 0.067f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
        }
        else
        {
            TriggerFade(SimpleViewTop, transDesc, 0, 0.067f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            TriggerFade(SimpleViewBottom, transDesc, 1, 0.067f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            TriggerFade(PageViewerTop, transDesc, 2, 0.433f, 0.566f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
        }
        if (g_maxPageID <= 6)
        {
            if (fReverse) TriggerScaleOut(PV_Inner, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 2.8f, 2.8f, ptPage.x, ptPage.y, false, false);
            else TriggerScaleIn(PV_Inner, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 2.8f, 2.8f, ptPage.x, ptPage.y, 1.0f, 1.0f, ptPage.x, ptPage.y, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, nullptr, &tsbInfo);
        }
        else
        {
            CSafeElementPtr<Element> overflow; overflow.Assign(regElem(L"overflow", PV_Inner));
            if (fReverse) TriggerFade(overflow, transDesc, 0, 0.033f, 0.216f, 0.0f, 0.0f, 0.58f, 1.0f, 1.0f, 0.0f, false, false, true);
            else TriggerFade(overflow, transDesc, 1, 0.0f, 0.183f, 0.25f, 0.1f, 0.25f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, nullptr, &tsbInfo);
            if (fReverse) TriggerScaleOut(PV_Inner, transDesc, 0, 0.033f, 0.3f, 1.0f, 1.0f, 0.0f, 1.0f, 1.15f, 1.15f, 0.5f, 0.5f, false, false);
            else TriggerScaleIn(PV_Inner, transDesc, 1, 0.0f, 0.267f, 0.0f, 0.0f, 0.0f, 1.0f, 1.15f, 1.15f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, nullptr, &tsbInfo);
        }
        DUI_SetGadgetZOrder(PV_Inner, -4);
        if (fReverse)
        {
            GTRANS_DESC transDesc2[2];
            TriggerFade(PageViewer, transDesc2, 0, 0.31f, 0.36f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, true, true);
            TriggerFade(fullscreenpopupbaseE, transDesc2, 1, 0.3f, 0.35f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, true, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, nullptr, &tsbInfo);
            DUI_SetGadgetZOrder(fullscreenpopupbaseE, -1);
            float scaleOrigin = (g_pctx->localeType == 1) ? 1.0f : 0.0f;
            short direction = (g_pctx->localeType == 1) ? -1 : 1;
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
        SimpleViewClose->SetActive(fReverse ? 0xB : 0);
        SimpleViewPages->SetActive(fReverse ? 0xB : 0);
        SimpleViewSettings->SetActive(fReverse ? 0xB : 0);
        SimpleViewPages->SetKeyFocus();
    }

    void TriggerNoMorePagesOnEdit()
    {
        if (editwndOld != editwnd)
        {
            editwndOld = editwnd;
            for (int i = 0; i < ARRAYSIZE(bg_extras); i++)
                bg_extras[i] = nullptr;
        }
        if (bg_extras[0] && !bg_extras[0]->IsDestroyed())
        {
            for (int i = 0; i < ARRAYSIZE(bg_extras); i++)
            {
                bg_extras[i]->Destroy(true);
                bg_extras[i] = nullptr;
            }
        }
        float flOriginLeft = (g_pctx->localeType == 1) ? 1.0f : 0.0f;
        float flOriginRight = (g_pctx->localeType == 1) ? 0.0f : 1.0f;
        GTRANS_DESC transDesc[4], transDescL[2], transDescR[2], transDescReset[1];
        TransitionStoryboardInfo tsbInfo = {};
        TriggerScaleIn(SimpleViewTopInner, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 3.705f / 3, 0.5f, 0.0f, false, false);
        TriggerScaleIn(SimpleViewTopInner, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 3.705f / 3, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f, 0.0f, false, false);
        TriggerScaleIn(SimpleViewBottomInner, transDesc, 2, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f, 3.7f / 3, 0.5f, 1.0f, false, false);
        TriggerScaleIn(SimpleViewBottomInner, transDesc, 3, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 3.7f / 3, 0.5f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, false, false);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
        TriggerScaleIn(bg_left_middle, transDescL, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 0.5f, 3.7f / 3, 0.9f, flOriginLeft, 0.5f, false, false);
        TriggerScaleIn(bg_left_middle, transDescL, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.9f, flOriginLeft, 0.5f, 1.0f, 1.0f, flOriginLeft, 0.5f, false, false);
        TriggerScaleIn(bg_right_middle, transDescR, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 0.5f, 3.7f / 3, 0.9f, flOriginRight, 0.5f, false, false);
        TriggerScaleIn(bg_right_middle, transDescR, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.9f, flOriginRight, 0.5f, 1.0f, 1.0f, flOriginRight, 0.5f, false, false);
        if (prevpage->GetWidth() > 1)
        {
            TriggerScaleIn(bg_left_top, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 1.0f, 3.7f / 3, 0.65f, flOriginLeft, 1.0f, false, false);
            TriggerScaleIn(bg_left_top, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginLeft, 1.0f, 1.0f, 1.0f, flOriginLeft, 1.0f, false, false);
            TriggerScaleIn(bg_left_middle, transDescL, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 0.5f, 1.7f, 1.0f, flOriginLeft, 0.5f, false, false);
            TriggerScaleIn(bg_left_middle, transDescL, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.7f, 1.0f, flOriginLeft, 0.5f, 1.0f, 1.0f, flOriginLeft, 0.5f, false, false);
            TriggerScaleIn(bg_left_bottom, transDesc, 2, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginLeft, 0.0f, 3.7f / 3, 0.65f, flOriginLeft, 0.0f, false, false);
            TriggerScaleIn(bg_left_bottom, transDesc, 3, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginLeft, 0.0f, 1.0f, 1.0f, flOriginLeft, 0.0f, false, false);
            TriggerScaleOut(prevpage, transDescReset, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
        }
        if (nextpage->GetWidth() > 1)
        {
            TriggerScaleIn(bg_right_top, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 0.0f, 3.7f / 3, 0.65f, flOriginRight, 1.0f, false, false);
            TriggerScaleIn(bg_right_top, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginRight, 0.0f, 1.0f, 1.0f, flOriginRight, 1.0f, false, false);
            TriggerScaleIn(bg_right_middle, transDescR, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 0.5f, 1.7f, 1.0f, flOriginRight, 0.5f, false, false);
            TriggerScaleIn(bg_right_middle, transDescR, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 1.7f, 1.0f, flOriginRight, 0.5f, 1.0f, 1.0f, flOriginRight, 0.5f, false, false);
            TriggerScaleIn(bg_right_bottom, transDesc, 2, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, flOriginRight, 1.0f, 3.7f / 3, 0.65f, flOriginRight, 0.0f, false, false);
            TriggerScaleIn(bg_right_bottom, transDesc, 3, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 3.7f / 3, 0.65f, flOriginRight, 1.0f, 1.0f, 1.0f, flOriginRight, 0.0f, false, false);
            TriggerScaleOut(nextpage, transDescReset, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
        }
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDescL), transDescL, nullptr, &tsbInfo);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDescR), transDescR, nullptr, &tsbInfo);
        if (prevpage->GetWidth() > 1 || nextpage->GetWidth() > 1)
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDescReset), transDescReset, nullptr, &tsbInfo);
    }

    void EnterSelectedPage(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            CValuePtr v;
            int page{};
            const WCHAR* content = PV_EnterPage->GetContentString(&v);
            if (content) page = _wtoi(content);
            if (!ValidateStrDigits(content) || page < 1 || page > g_maxPageID)
            {
                MessageBeep(MB_OK);
                WCHAR errorcontent[260], errorBuf[192], errorTitle[64];
                LoadStrFromRes(errorBuf, 192, 4061);
                StringCchPrintfW(errorcontent, 256, errorBuf, g_maxPageID);
                DDNotificationBanner* ddnb = new DDNotificationBanner();
                LoadStrFromRes(errorTitle, 64, 4060);
                ddnb->CreateBanner(DDNT_ERROR, errorTitle, errorcontent, 5, nullptr);
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
            CSafeElementPtr<DDLVActionButton> PV_Home; PV_Home.Assign((DDLVActionButton*)regElem(L"PV_Home", elem));
            CSafeElementPtr<DDLVActionButton> PV_Remove; PV_Remove.Assign((DDLVActionButton*)regElem(L"PV_Remove", elem));
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
            if (g_pctx->DWMActive)
            {
                AddLayeredRef(PageViewer->GetDisplayNode());
                SetGadgetFlags(PageViewer->GetDisplayNode(), NULL, NULL);
            }
            CSafeElementPtr<Element> PageViewerTop;
            PageViewerTop.Assign(regElem(L"PageViewerTop", PageViewer));
            PageViewerTop->SetHeight(dimensions.bottom * 0.15);
            if (dimensions.bottom * 0.15 < 80 * g_pctx->flScaleFactor) PageViewerTop->SetHeight(80 * g_pctx->flScaleFactor);
            CSafeElementPtr<TouchButton> PV_Back;
            PV_Back.Assign((TouchButton*)regElem(L"PV_Back", PageViewer));
            assignFn(PV_Back, ClosePageViewer);
            CSafeElementPtr<TouchButton> PV_Add;
            PV_Add.Assign((TouchButton*)regElem(L"PV_Add", PageViewer));
            PV_Add->SetEnabled(isDefaultRes());
            assignFn(PV_Add, AddNewPage);
            CSafeElementPtr<Element> PV_Inner; PV_Inner.Assign(regElem(L"PV_Inner", PageViewer));
            CSafeElementPtr<LVItem> peAnimateFrom;
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            if (g_maxPageID <= 6)
            {
                LVItem* pages[6];
                CSafeElementPtr<Element> pagesrow1;
                pagesrow1.Assign(regElem(L"pagesrow1", PageViewer));
                CSafeElementPtr<Element> pagesrow2;
                pagesrow2.Assign(regElem(L"pagesrow2", PageViewer));
                pagesrow1->SetHeight(ceil(dimensions.bottom * 0.25));
                int row1 = g_maxPageID;
                int row2 = 0;
                CSafeElementPtr<Element> bg_left; bg_left.Assign(regElem(L"bg_left", PageViewer));
                CSafeElementPtr<Element> bg_top; bg_top.Assign(regElem(L"bg_top", PageViewer));
                CSafeElementPtr<Element> bg_right; bg_right.Assign(regElem(L"bg_right", PageViewer));
                CSafeElementPtr<Element> bg_bottom; bg_bottom.Assign(regElem(L"bg_bottom", PageViewer));
                CSafeElementPtr<Element> rowpadding; rowpadding.Assign(regElem(L"rowpadding", PageViewer));
                if (g_maxPageID >= 3)
                {
                    rowpadding->SetHeight(floor(dimensions.right * 0.025));
                    pagesrow2->SetHeight(ceil(dimensions.bottom * 0.25));
                    row1 = ceil(g_maxPageID / 2.0f);
                    row2 = floor(g_maxPageID / 2.0f);
                }
                for (int i = 1; i <= g_maxPageID; i++)
                {
                    LVItem* PV_Page{};
                    parserEdit->CreateElement(L"PV_Page", nullptr, nullptr, nullptr, (Element**)&PV_Page);
                    PV_Page->SetWidth(ceil(dimensions.right * 0.25));
                    PV_Page->SetHeight(ceil(dimensions.bottom * 0.25));
                    PV_Page->SetPage(i);
                    pages[i - 1] = PV_Page;
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
                        pagesrow2->Add((Element**)&PV_Page, 1);
                        pagesrow2->SetWidth(pagesrow2->GetWidth() + PV_Page->GetWidth());
                        if (i < g_maxPageID)
                        {
                            pagesrow2->Add(&PV_PageRow_Dim, 1);
                            pagesrow2->SetWidth(pagesrow2->GetWidth() + PV_PageRow_Dim->GetWidth());
                        }
                        if (g_maxPageID & 1 && i == g_maxPageID)
                        {
                            RECT rcRow1, rcRow2, rcSide1, rcSide2;
                            GetGadgetRect(pagesrow1->GetDisplayNode(), &rcRow1, 0xC);
                            GetGadgetRect(pagesrow2->GetDisplayNode(), &rcRow2, 0xC);
                            GetGadgetRect(pages[row1]->GetDisplayNode(), &rcSide1, 0xC);
                            GetGadgetRect(pages[g_maxPageID - 1]->GetDisplayNode(), &rcSide2, 0xC);
                            int first = (rcSide1.left > rcSide2.left) ? rcRow1.right - rcSide1.right : rcSide1.left - rcRow1.left;
                            int last = (rcSide1.left > rcSide2.left) ? rcSide2.left - rcRow1.left : rcRow1.right - rcSide2.right;
                            PV_CreateDimRect(pagesrow2, first, rcRow2.bottom - rcRow1.top, 0);
                            PV_CreateDimRect(pagesrow2, last, rcRow2.bottom - rcRow1.top, -1);
                        }
                    }
                    int remainingIcons = 1;
                    PV_Page->AddFlags(LVIF_HIDDEN);
                    CSafeElementPtr<Element> PV_PageIcons;
                    PV_PageIcons.Assign(regElem(L"PV_PageIcons", PV_Page));
                    for (int j = 0; j < pm.size(); j++)
                    {
                        if (pm[j]->GetPage() != i) continue;
                        remainingIcons++;
                        yValueEx* yV = new yValueEx{ j, 0.25, NULL, nullptr, PV_PageIcons };
                        PV_Page->RemoveFlags(LVIF_HIDDEN);
                        QueueUserWorkItem(CreateDesktopPreviewHelper, yV, 0);
                    }
                    CSafeElementPtr<RichText> number;
                    number.Assign((RichText*)regElem(L"number", PV_Page));
                    number->SetContentString(to_wstring(i).c_str());
                    DUI_SetGadgetZOrder(number, 4);
                    CSafeElementPtr<Element> PV_HomeBadge;
                    PV_HomeBadge.Assign(regElem(L"PV_HomeBadge", PV_Page));
                    DDLVActionButton* PV_Home = (DDLVActionButton*)regElem(L"PV_Home", PV_Page);
                    if (PV_Page->GetPage() == g_homePageID)
                    {
                        PV_Home->SetSelected(true);
                        PV_HomeBadge->SetSelected(true);
                    }
                    if (remainingIcons == 1)
                    {
                        DDLVActionButton* PV_Remove = (DDLVActionButton*)regElem(L"PV_Remove", PV_Page);
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
                RECT rcTop{}, rcBottom{};
                GetGadgetRect(pagesrow1->GetDisplayNode(), &rcTop, 0xC);
                rcBottom = rcTop;
                if (g_maxPageID >= 3)
                    GetGadgetRect(pagesrow2->GetDisplayNode(), &rcBottom, 0xC);
                if (g_maxPageID >= 3) rowpadding->SetWidth(rcTop.right - rcTop.left);
                SetTransElementPosition(bg_top, 0, 0, dimensions.right, rcTop.top);
                SetTransElementPosition(bg_bottom, 0, rcBottom.bottom, dimensions.right, dimensions.bottom - rcBottom.bottom);
                SetTransElementPosition(bg_left, 0, rcTop.top, rcTop.left, rcBottom.bottom - rcTop.top);
                SetTransElementPosition(bg_right, rcTop.right, rcTop.top, dimensions.right - rcTop.right, rcBottom.bottom - rcTop.top);
            }
            else
            {
                CSafeElementPtr<Element> bg; bg.Assign(regElem(L"bg", PageViewer));
                bg->SetVisible(true);
                Element* overflow;
                parserEdit->CreateElement(L"overflow", nullptr, nullptr, nullptr, &overflow);
                PV_Inner->Add(&overflow, 1);
                PV_EnterPage = (DDScalableTouchEdit*)regElem(L"PV_EnterPage", PageViewer);
                CSafeElementPtr<LVItem> PV_ConfirmEnterPage;
                PV_ConfirmEnterPage.Assign((LVItem*)regElem(L"PV_ConfirmEnterPage", PageViewer));
                assignFn(PV_ConfirmEnterPage, EnterSelectedPage);
                PV_SetEnterPageDesc();
                DDLVActionButton* PV_Remove = (DDLVActionButton*)regElem(L"PV_Remove", PageViewer);
                if (PV_Remove)
                {
                    PV_Remove->SetEnabled(isDefaultRes());
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
                DDLVActionButton* PV_Home = (DDLVActionButton*)regElem(L"PV_Home", PageViewer);
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
                DDLVActionButton* PV_Remove = (DDLVActionButton*)regElem(L"PV_Remove", elem);
                if (PV_Remove)
                {
                    PV_Remove->SetEnabled(isDefaultRes());
                    PV_Remove->SetVisible(elem->GetMouseWithin());
                    PV_Remove->SetAssociatedItem((LVItem*)elem);
                    assignFn(PV_Remove, RemoveSelectedPage);
                }
            }
            DDLVActionButton* PV_Home = (DDLVActionButton*)regElem(L"PV_Home", elem);
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
            if (g_pctx->localeType == 1) uFlags |= TPM_LAYOUTRTL;

            RECT rcMenu{}, rcLocation{}, rcWindow{};
            GetGadgetRect(elem->GetDisplayNode(), &rcLocation, 0xC);
            ddm->GetMenuRect(&rcMenu);
            GetWindowRect(editwnd->GetHWND(), &rcWindow);
            rcLocation.left += elem->GetWidth() / 2 + rcWindow.left;
            rcLocation.top += rcWindow.top;
            LPARAM lParam = ddm->TrackPopupMenuEx(uFlags, rcLocation.left, rcLocation.top, editwnd->GetHWND(), nullptr);
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
        SimpleViewHomeBadge.Assign((RichText*)regElem(L"SimpleViewHomeBadge", pEdit));
        if (g_maxPageID != 1)
        {
            WCHAR currentPage[64], currentPageBuf[64];
            LoadStrFromRes(currentPageBuf, 64, 4026);
            StringCchPrintfW(currentPage, 64, currentPageBuf, g_currentPageID, g_maxPageID);
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
            float xLoc = (g_pctx->localeType == 1) ? -0.4 : 0.9;
            TogglePage(nextpage, xLoc, 0.25, 0.5, 0.5);
        }
        if (g_currentPageID != 1)
        {
            float xLoc = (g_pctx->localeType == 1) ? 0.9 : -0.4;
            TogglePage(prevpage, xLoc, 0.25, 0.5, 0.5);
        }

        centeredE->SetWidth(ceil(dimensions.right * 0.7));
        centeredE->SetHeight(ceil(dimensions.bottom * 0.7));
        fullscreeninnerE->SetWidth(ceil(dimensions.right * 0.7));
        fullscreeninnerE->SetHeight(ceil(dimensions.bottom * 0.7));
        simpleviewoverlay->SetWidth(ceil(dimensions.right * 0.7));
        simpleviewoverlay->SetHeight(ceil(dimensions.bottom * 0.7));
        SimpleViewTop->SetHeight(floor(dimensions.bottom * 0.15));
        SimpleViewTopInner->SetHeight(floor(dimensions.bottom * 0.15));
        SimpleViewBottom->SetHeight(dimensions.bottom - SimpleViewTop->GetHeight() - fullscreeninnerE->GetHeight());
        SimpleViewBottomInner->SetHeight(dimensions.bottom - SimpleViewTopInner->GetHeight() - fullscreeninnerE->GetHeight());
        if (dimensions.bottom * 0.15 < 80 * g_pctx->flScaleFactor) SimpleViewTop->SetHeight(80 * g_pctx->flScaleFactor);
        if (dimensions.bottom * 0.15 < 106 * g_pctx->flScaleFactor) SimpleViewBottom->SetHeight(106 * g_pctx->flScaleFactor);

        unsigned short leftX = (g_pctx->localeType == 1) ? round(dimensions.right * 0.15) + fullscreeninnerE->GetWidth() : 0;
        unsigned short leftWidth = (g_pctx->localeType == 1) ? dimensions.right - leftX : floor(dimensions.right * 0.15);
        unsigned short leftXSmall = (g_pctx->localeType == 1) ? leftX : prevpage->GetX() + prevpage->GetWidth();
        unsigned short leftWidthSmall = (g_pctx->localeType == 1) ? prevpage->GetX() - leftXSmall : leftWidth - (prevpage->GetX() + prevpage->GetWidth());
        unsigned short rightX = (g_pctx->localeType == 1) ? 0 : floor(dimensions.right * 0.15) + fullscreeninnerE->GetWidth();
        unsigned short rightWidth = (g_pctx->localeType == 1) ? round(dimensions.right * 0.15) : ceil(dimensions.right * 0.15);
        unsigned short rightXSmall = (g_pctx->localeType == 1) ? nextpage->GetX() + nextpage->GetWidth() : rightX;
        unsigned short rightWidthSmall = (g_pctx->localeType == 1) ? rightWidth - (nextpage->GetX() + nextpage->GetWidth()) : nextpage->GetX() - rightXSmall;
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
            SetTransElementPosition(bg_left_top, -999, -999, 0, 0);
            SetTransElementPosition(bg_left_middle, leftX, SimpleViewTopInner->GetHeight(),
                leftWidth, dimensions.bottom - SimpleViewTopInner->GetHeight() - SimpleViewBottomInner->GetHeight());
            SetTransElementPosition(bg_left_bottom, -999, -999, 0, 0);
            leftXSmall = (g_pctx->localeType == 1) ? leftX : dimensions.right * 0.1;
            leftWidthSmall = (g_pctx->localeType == 1) ? dimensions.right * 0.9 - leftXSmall : leftWidth - dimensions.right * 0.1;
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
            SetTransElementPosition(bg_right_top, -999, -999, 0, 0);
            SetTransElementPosition(bg_right_middle, rightX, SimpleViewTopInner->GetHeight(),
                rightWidth, dimensions.bottom - SimpleViewTopInner->GetHeight() - SimpleViewBottomInner->GetHeight());
            SetTransElementPosition(bg_right_bottom, -999, -999, 0, 0);
            rightXSmall = (g_pctx->localeType == 1) ? dimensions.right * 0.1 : rightX;
            rightWidthSmall = (g_pctx->localeType == 1) ? rightWidth - dimensions.right * 0.1 : dimensions.right * 0.9 - rightXSmall;
        }
        if (g_pctx->DWMActive)
        {
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
                Element* bg_main[7]{};
                if (editwndOld != editwnd)
                {
                    editwndOld = editwnd;
                    for (int i = 0; i < ARRAYSIZE(bg_extras); i++)
                        bg_extras[i] = nullptr;
                }
                short localeDirection = (g_pctx->localeType == 1) ? -1 : 1;
                short animDirection = 1;
                float flOrig = 0.5f * (1 - localeDirection), flOrig2 = 0.5f * (1 + localeDirection);
                float fullLeftCoef = (prevpage->GetWidth() > 0) ? 1.0f : 3.0f, fullRightCoef = (nextpage->GetWidth() > 0) ? 1.0f : 3.0f;
                float flOffscreen = 0.0f;
                if (nextpage->GetX() < 0) flOffscreen -= nextpage->GetX();
                if (nextpage->GetX() + nextpage->GetWidth() > dimensions.right) flOffscreen += dimensions.right - (nextpage->GetX() + nextpage->GetWidth());
                float flMiddleAnim = -1 * localeDirection * centeredE->GetWidth() * fullRightCoef / bg_right_middle->GetWidth() + flOrig;
                float flMiddleSmAnim = localeDirection * ((centeredE->GetWidth() + bg_left_middle->GetWidth() / fullLeftCoef) / bg_left_middle->GetWidth() * fullLeftCoef - flOrig);
                float flMiddleSmAnim2 = -1 * localeDirection * nextpage->GetWidth() / static_cast<float>(bg_right_middle->GetWidth()) + flOrig;
                float flMiddleDisappearAnim = localeDirection * (nextpage->GetWidth() / static_cast<float>(bg_right_middle->GetWidth()) + 1.0f - flOrig);
                float flSidesDisappearAnim = -1 * flOffscreen / bg_right_top->GetWidth() + flOrig2;
                bool invert{};
                UIContainer->SetVisible(false);
                GTRANS_DESC transDesc[1], transDesc2[1], transDesc3[1];
                TransitionStoryboardInfo tsbInfo = {};
                if (bg_extras[0] && !bg_extras[0]->IsDestroyed() && animFlags & 5)
                {
                    for (int i = 0; i < ARRAYSIZE(bg_extras); i++)
                    {
                        bg_extras[i]->Destroy(true);
                        bg_extras[i] = nullptr;
                    }
                }
                if (animFlags & 1) // Left
                {
                    if (animFlags & 0x10)
                        EM_CreateRoundOverlayRect(&bg_extras[0], EM_Dim, nextpage->GetX(), nextpage->GetY(), nextpage->GetWidth(), nextpage->GetHeight());
                    else
                        EM_CreateDimRect(&bg_extras[0], EM_Dim, nextpage->GetX() + nextpage->GetWidth() * localeDirection, nextpage->GetY(), nextpage->GetWidth(), nextpage->GetHeight());
                    EM_CreateDimRect(&bg_extras[1], EM_Dim, leftX, SimpleViewTopInner->GetHeight(), leftWidth, prevpage->GetY() - SimpleViewTopInner->GetHeight());
                    EM_CreateDimRect(&bg_extras[2], EM_Dim, leftXSmall, bg_extras[1]->GetY() + bg_extras[1]->GetHeight(), leftWidthSmall, prevpage->GetHeight());
                    EM_CreateDimRect(&bg_extras[3], EM_Dim, leftX, bg_extras[2]->GetY() + bg_extras[2]->GetHeight(), leftWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_extras[2]->GetY() - bg_extras[2]->GetHeight());
                    EM_CreateDimRect(&bg_extras[4], EM_Dim, rightX, SimpleViewTopInner->GetHeight(), rightWidth, nextpage->GetY() - SimpleViewTopInner->GetHeight());
                    EM_CreateDimRect(&bg_extras[5], EM_Dim, rightXSmall, bg_extras[4]->GetY() + bg_extras[4]->GetHeight(), rightWidthSmall, prevpage->GetHeight());
                    EM_CreateDimRect(&bg_extras[6], EM_Dim, rightX, bg_extras[5]->GetY() + bg_extras[5]->GetHeight(), rightWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_extras[5]->GetY() - bg_extras[5]->GetHeight());
                    bg_main[0] = bg_left_middle, bg_main[1] = bg_right_top, bg_main[2] = bg_right_middle, bg_main[3] = bg_right_bottom;
                    TriggerScaleIn(centeredE, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f - 3.25f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f - 3.25f * localeDirection, 0.5f, false, false);
                    TriggerScaleIn(nextpage, transDesc3, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1.4f, 1.4f, 0.5f + 3.25f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f + 3.25f * localeDirection, 0.5f, false, false);
                }
                if (animFlags & 2) // Left, with more pages
                {
                    bg_main[4] = bg_left_top, bg_main[5] = bg_left_middle, bg_main[6] = bg_left_bottom;
                    TriggerScaleIn(prevpage, transDesc2, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f - 3.0f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f - 3.0f * localeDirection, 0.5f, false, false);
                }
                if (animFlags & 4) // Right
                {
                    invert = true;
                    if (animFlags & 0x20)
                        EM_CreateRoundOverlayRect(&bg_extras[0], EM_Dim, prevpage->GetX(), prevpage->GetY(), prevpage->GetWidth(), prevpage->GetHeight());
                    else
                        EM_CreateDimRect(&bg_extras[0], EM_Dim, prevpage->GetX() - prevpage->GetWidth() * localeDirection, prevpage->GetY(), prevpage->GetWidth(), prevpage->GetHeight());
                    EM_CreateDimRect(&bg_extras[1], EM_Dim, rightX, SimpleViewTopInner->GetHeight(), rightWidth, nextpage->GetY() - SimpleViewTopInner->GetHeight());
                    EM_CreateDimRect(&bg_extras[2], EM_Dim, rightXSmall, bg_extras[1]->GetY() + bg_extras[1]->GetHeight(), rightWidthSmall, nextpage->GetHeight());
                    EM_CreateDimRect(&bg_extras[3], EM_Dim, rightX, bg_extras[2]->GetY() + bg_extras[2]->GetHeight(), rightWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_extras[2]->GetY() - bg_extras[2]->GetHeight());
                    EM_CreateDimRect(&bg_extras[4], EM_Dim, leftX, SimpleViewTopInner->GetHeight(), leftWidth, prevpage->GetY() - SimpleViewTopInner->GetHeight());
                    EM_CreateDimRect(&bg_extras[5], EM_Dim, leftXSmall, bg_extras[4]->GetY() + bg_extras[4]->GetHeight(), leftWidthSmall, prevpage->GetHeight());
                    EM_CreateDimRect(&bg_extras[6], EM_Dim, leftX, bg_extras[5]->GetY() + bg_extras[5]->GetHeight(), leftWidth, dimensions.bottom - SimpleViewBottomInner->GetHeight() - bg_extras[5]->GetY() - bg_extras[5]->GetHeight());
                    bg_main[0] = bg_right_middle, bg_main[1] = bg_left_top, bg_main[2] = bg_left_middle, bg_main[3] = bg_left_bottom;
                    TriggerScaleIn(centeredE, transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f + 3.25f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f + 3.25f * localeDirection, 0.5f, false, false);
                    TriggerScaleIn(prevpage, transDesc2, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1.4f, 1.4f, 0.5f - 3.25f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f - 3.25f * localeDirection, 0.5f, false, false);
                }
                if (animFlags & 5) // Left or right
                {
                    if (invert)
                    {
                        animDirection = -1;
                        flOrig = 0.5f * (1 + localeDirection);
                        flOrig2 = 0.5f * (1 - localeDirection);
                        flOffscreen = 0.0f;
                        if (prevpage->GetX() < 0) flOffscreen -= prevpage->GetX();
                        if (prevpage->GetX() + prevpage->GetWidth() > dimensions.right) flOffscreen += dimensions.right - (prevpage->GetX() + prevpage->GetWidth());
                        flMiddleAnim = localeDirection * centeredE->GetWidth() * fullLeftCoef / bg_left_middle->GetWidth() + flOrig;
                        flMiddleSmAnim = -1 * localeDirection * ((centeredE->GetWidth() + bg_right_middle->GetWidth() / fullRightCoef) / bg_right_middle->GetWidth() * fullRightCoef - flOrig);
                        flMiddleSmAnim2 = localeDirection * prevpage->GetWidth() / static_cast<float>(bg_left_middle->GetWidth()) + flOrig;
                        flMiddleDisappearAnim = -1 * localeDirection * (prevpage->GetWidth() / static_cast<float>(bg_left_middle->GetWidth()) + 1.0f - flOrig);
                        flSidesDisappearAnim = -1 * flOffscreen / bg_left_top->GetWidth() + flOrig2;
                    }
                    if (!(animFlags & 0x30))
                        for (int i = 4; i < 7; i++)
                            bg_extras[i]->SetVisible(false);
                    GTRANS_DESC transDesc[11], transDescReset[2];
                    if (animFlags & 0x30)
                        TriggerScaleOut(bg_extras[0], transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f + 0.5f * localeDirection * animDirection, 0.5f, false, false);
                    else
                        TriggerScaleIn(bg_extras[0], transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1.4f, 1.4f, 0.5f + 2.25f * localeDirection * animDirection, 0.5f, 1.0f, 1.0f, 0.5f + 2.25f * localeDirection * animDirection, 0.5f, false, false);
                    TriggerScaleOut(bg_extras[1], transDesc, 1, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, EM_GetRectAniWScale(bg_extras[1]), 0.0f, flOrig, 0.0f, false, false);
                    TriggerScaleOut(bg_extras[2], transDesc, 2, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 0.0f, (centeredE->GetHeight() / static_cast<float>(bg_extras[2]->GetHeight())), flMiddleSmAnim, 0.5f, false, false);
                    TriggerScaleOut(bg_extras[3], transDesc, 3, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, EM_GetRectAniWScale(bg_extras[3]), 0.0f, flOrig, 1.0f, false, false);
                    TriggerScaleOut(bg_extras[4], transDesc, 4, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 0.0f, 2.5f, flSidesDisappearAnim, -0.67f, false, false);
                    TriggerScaleOut(bg_extras[5], transDesc, 5, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 0.0f, 0.0f, flMiddleDisappearAnim, 0.5f, false, false);
                    TriggerScaleOut(bg_extras[6], transDesc, 6, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 0.0f, 2.5f, flSidesDisappearAnim, 1.67f, false, false);
                    TriggerScaleIn(bg_main[0], transDesc, 7, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                        1 / 1.4f, 1 / 1.4f, 0.5f - 12.25f * localeDirection * animDirection, 0.5f, 1.0f, 1.0f, 0.5f - 12.25f * localeDirection * animDirection, 0.5f, false, false);
                    TriggerScaleIn(bg_main[1], transDesc, 8, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                        EM_GetRectAniWScale(bg_main[1]), 0.0f, flOrig2, 0.0f, 1.0f, 1.0f, flOrig2, 0.0f, false, false);
                    TriggerScaleIn(bg_main[2], transDesc, 9, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                        0.0f, (centeredE->GetHeight() / static_cast<float>(bg_main[2]->GetHeight())),
                        flMiddleAnim, 0.5f, 1.0f, 1.0f, flMiddleAnim, 0.5f, false, false);
                    TriggerScaleIn(bg_main[3], transDesc, 10, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                        EM_GetRectAniWScale(bg_main[3]), 0.0f, flOrig2, 1.0f, 1.0f, 1.0f, flOrig2, 1.0f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);

                    TriggerScaleOut(SimpleViewTopInner, transDescReset, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    TriggerScaleOut(SimpleViewBottomInner, transDescReset, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDescReset), transDescReset, nullptr, &tsbInfo);
                }
                if (animFlags & 6) // Left or right, left must have more pages
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, nullptr, &tsbInfo);
                if (animFlags & 8) // Right, with more pages
                {
                    bg_main[4] = bg_right_top, bg_main[5] = bg_right_middle, bg_main[6] = bg_right_bottom;
                    TriggerScaleIn(nextpage, transDesc3, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / 1.4f, 1 / 1.4f, 0.5f + 3.0f * localeDirection, 0.5f, 1.0f, 1.0f, 0.5f + 3.0f * localeDirection, 0.5f, false, false);
                }
                if (animFlags & 9) // Left or right, right must have more pages
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc3), transDesc3, nullptr, &tsbInfo);
                if (animFlags & 0xA) // More pages on either left or right
                {
                    GTRANS_DESC transDesc[3];
                    TriggerScaleIn(bg_main[4], transDesc, 0, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                        1 / 1.4f, 1 / 1.4f, 0.5f - 12.25f * localeDirection * animDirection, 3.5f, 1.0f, 1.0f, 0.5f - 12.25f * localeDirection * animDirection, 3.5f, false, false);
                    TriggerScaleIn(bg_main[5], transDesc, 1, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                        0.0f, 1 / 1.4f, flMiddleSmAnim2, 0.5f, 1.0f, 1.0f, flMiddleSmAnim2, 0.5f, false, false);
                    TriggerScaleIn(bg_main[6], transDesc, 2, 0.0f, 0.3f, 0.75f, 0.45f, 0.0f, 1.0f,
                        1 / 1.4f, 1 / 1.4f, 0.5f - 12.25f * localeDirection * animDirection, -2.5f, 1.0f, 1.0f, 0.5f - 12.25f * localeDirection * animDirection, -2.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                }
                if (animFlags & 0xF) // Always
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, centeredE->GetDisplayNode(), &tsbInfo);
            }
        }
        g_invokedpagechange = false;
        CSafeElementPtr<TouchScrollViewer> svBottomOptions;
        svBottomOptions.Assign((TouchScrollViewer*)regElem(L"svBottomOptions", SimpleViewBottom));
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
        if (g_editavailable)
        {
            if (g_touchmode) g_iconsz = 64;
            g_editmode = true;
            g_editavailable = false;
            g_animatePVEnter = true;
            if (!g_invokedpagechange) SendMessageW(g_hWndTaskbar, WM_COMMAND, 419, 0);
            RECT dimensions;
            POINT topLeftMon = GetTopLeftMonitor();
            SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
            if (g_pctx->localeType == 1)
            {
                int rightMon = GetRightMonitor();
                topLeftMon.x = dimensions.right + dimensions.left - rightMon;
            }
            DWORD dwExStyle = WS_EX_TOOLWINDOW, dwCreateFlags = 0x10;
            if (g_pctx->DWMActive)
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

            SetWindowPos(editwnd->GetHWND(), nullptr, NULL, NULL, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOMOVE | SWP_NOZORDER);
            GetClientRect(editwnd->GetHWND(), &dimensions);

            pEdit->SetVisible(true);
            pEdit->EndDefer(key5);
            //pEditBG->SetVisible(true);
            //pEditBG->EndDefer(key6);

            fullscreenpopupbaseE = regElem(L"fullscreenpopupbase", pEdit);
            popupcontainerE = regElem(L"popupcontainer", pEdit);
            centeredE = regElem(L"centered", pEdit);
            //centeredEBG = regElem(L"centered", pEditBG);
            SimpleViewTop = regElem(L"SimpleViewTop", pEdit);
            SimpleViewBottom = regElem(L"SimpleViewBottom", pEdit);
            SimpleViewTopInner = regElem(L"SimpleViewTopInner", pEdit);
            SimpleViewBottomInner = regElem(L"SimpleViewBottomInner", pEdit);
            SimpleViewPower = (TouchButton*)regElem(L"SimpleViewPower", pEdit);
            SimpleViewSearch = (TouchButton*)regElem(L"SimpleViewSearch", pEdit);
            SimpleViewSettings = (DDIconButton*)regElem(L"SimpleViewSettings", pEdit);
            SimpleViewPages = (DDIconButton*)regElem(L"SimpleViewPages", pEdit);
            SimpleViewClose = (DDIconButton*)regElem(L"SimpleViewClose", pEdit);
            EM_Dim = regElem(L"EM_Dim", pEdit);
            bg_left_top = regElem(L"bg_left_top", pEdit);
            bg_left_middle = regElem(L"bg_left_middle", pEdit);
            bg_left_bottom = regElem(L"bg_left_bottom", pEdit);
            bg_right_top = regElem(L"bg_right_top", pEdit);
            bg_right_middle = regElem(L"bg_right_middle", pEdit);
            bg_right_bottom = regElem(L"bg_right_bottom", pEdit);
            prevpage = (DDScalableTouchButton*)regElem(L"prevpage", pEdit);
            nextpage = (DDScalableTouchButton*)regElem(L"nextpage", pEdit);
            SimpleViewPrevPage = (TouchButton*)regElem(L"SimpleViewPrevPage", pEdit);
            SimpleViewNextPage = (TouchButton*)regElem(L"SimpleViewNextPage", pEdit);
            pageinfo = (DDScalableRichText*)regElem(L"pageinfo", pEdit);

            assignFn(prevpage, GoToPrevPage);
            assignFn(nextpage, GoToNextPage);
            assignFn(SimpleViewPrevPage, GoToPrevPage);
            assignFn(SimpleViewNextPage, GoToNextPage);
            assignFn(SimpleViewPower, ShowShutdownDialog);
            assignFn(SimpleViewSearch, ShowSearchUI);
            assignFn(SimpleViewSettings, ShowSettings);
            assignFn(SimpleViewPages, ShowPageViewer);
            assignFn(SimpleViewClose, ExitWindow);

            SimpleViewClose->SetLayoutPos(g_enableexit ? -1 : -3);

            WndProcEdit = (WNDPROC)SetWindowLongPtrW(editwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeWindowProc);
            //WndProcEditBG = (WNDPROC)SetWindowLongPtrW(editbgwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)EditModeBGWindowProc);

            LPWSTR sheetName = g_pctx->theme ? (LPWSTR)L"edit" : (LPWSTR)L"editdark";
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
            if (g_pctx->DWMActive)
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

            HMODULE hShlwapi = GetModuleHandleW(L"shlwapi.dll");
            if (hShlwapi)
            {
                pfnSHCreateWorkerWindowW SHCreateWorkerWindowW =
                    (pfnSHCreateWorkerWindowW)GetProcAddress(hShlwapi, "SHCreateWorkerWindowW");
                edit_hWorker = SHCreateWorkerWindowW(EditWorkerProc, HWND_MESSAGE, 0, 0, nullptr);
                SetTimer(edit_hWorker, 1, 60, nullptr);
            }

            _UpdateSimpleViewContent(animate, animFlags);

            SimpleViewSettings->SetKeyFocus();
            SetTimer(wnd->GetHWND(), 25, 50, nullptr);
        }
    }
}
