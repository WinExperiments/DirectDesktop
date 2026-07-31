#include "pch.h"

#include "SearchPage.h"
#include "..\DirectDesktop.h"
#include "..\backend\DirectoryHelper.h"


using namespace DirectUI;
using namespace DDUI;

namespace DirectDesktop
{
    typedef void (WINAPI* pfnE_SetSearchW)(LPCWSTR lpSearchString);
    typedef BOOL(WINAPI* pfnE_QueryW)(BOOL bWait);
    typedef DWORD(WINAPI* pfnE_GetNumResults)(void);
    typedef LPCWSTR(WINAPI* pfnE_GetResultFileNameW)(DWORD index);
    typedef LPCWSTR(WINAPI* pfnE_GetResultPathW)(DWORD index);
    typedef DWORD(WINAPI* pfnE_GetLastError)(void);
    pfnE_SetSearchW Everything_SetSearchW = nullptr;
    pfnE_QueryW Everything_QueryW = nullptr;
    pfnE_GetNumResults Everything_GetNumResults = nullptr;
    pfnE_GetResultFileNameW Everything_GetResultFileNameW = nullptr;
    pfnE_GetResultPathW Everything_GetResultPathW = nullptr;
    pfnE_GetLastError Everything_GetLastError = nullptr;

    HMODULE g_EverythingDLL;
    NativeHWNDHost* searchwnd;
    DUIXmlParser* parserSearch;
    HWNDElement* parentSearch;
    Element* pSearch;
    DDScalableTouchEdit* searchbox;
    WNDPROC WndProcSearch;
    SearchParams g_sp;

    DWORD WINAPI AnimateSearchWindow(LPVOID lpParam);
    void DestroySearchPage();

    // 0.5.8.1: TODO: Better peek listener
    LRESULT CALLBACK SearchWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_CLOSE:
                SetTimer(hWnd, 1, 50, nullptr);
                return 0;
            case WM_DESTROY:
                return 0;
            case WM_CANCELMODE:
            {
                if (!g_peek)
                {
                    g_peek = true;
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    TriggerScaleOut(UIContainer, transDesc, 0, 0.0f, 0.5f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
                    DUI_SetGadgetZOrder(UIContainer, -1);
                }
                break;
            }
            case WM_NCHITTEST:
            {
                if (g_peek)
                {
                    g_peek = false;
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    TriggerScaleOut(UIContainer, transDesc, 0, 0.0f, 0.67f, 0.1f, 0.9f, 0.2f, 1.0f, 0.92f, 0.92f, 0.5f, 0.5f, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
                    if (g_pctx->windowAnim && g_pctx->clientAnim)
                    {
                        CSafeElementPtr<Element> pagecontent;
                        pagecontent.Assign(regElem(L"pagecontent", pSearch));
                        pagecontent->SetVisible(false);
                        TriggerScaleIn(pagecontent, transDesc, 0, 0.05f, 0.3f, 0.25f, 0.1f, 0.25f, 1.0f, 0.97f, 0.97f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, true, false);
                        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nullptr, &tsbInfo);
                        AnimateWindow(hWnd, 10, AW_BLEND | AW_HIDE);
                        HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateSearchWindow, nullptr, NULL, nullptr);
                        if (AnimHandle) CloseHandle(AnimHandle);
                    }
                }
                break;
            }
            case WM_NCACTIVATE:
            {
                if (wParam == 0)
                {
                    HWND hForeground = GetForegroundWindow();
                    if (!hForeground)
                    {
                        HWND hTrayNotify = FindWindowExW(g_hWndTaskbar, nullptr, L"TrayNotifyWnd", nullptr);
                        HWND hShowDesktop = FindWindowExW(hTrayNotify, nullptr, L"TrayShowDesktopButtonWClass", nullptr);
                        RECT rc;
                        POINT pt;
                        GetWindowRect(hShowDesktop, &rc);
                        GetCursorPos(&pt);
                        if (PtInRect(&rc, pt))
                        {
                            g_peek = false;
                            DestroySearchPage();
                        }
                    }
                }
                break;
            }
            case WM_TIMER:
                KillTimer(hWnd, wParam);
                switch (wParam)
                {
                case 1:
                    DestroySearchPage();
                    break;
                }
            case WM_USER + 1:
            {
                searchbox->SetKeyFocus();
                break;
            }
        }
        return CallWindowProc(WndProcSearch, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI AnimateSearchWindow(LPVOID lpParam)
    {
        DWORD animCoef = g_pctx->animCoef;
        if (g_pctx->AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        if (g_pctx->windowAnim && g_pctx->clientAnim)
            AnimateWindow(searchwnd->GetHWND(), 150 * (animCoef / 100.0f), AW_BLEND);
        else
            searchwnd->ShowWindow(SW_SHOW);
        SendMessageW(searchwnd->GetHWND(), WM_USER + 1, NULL, NULL);
        return 0;
    }

    DWORD WINAPI AnimateSearchWindow2(LPVOID lpParam)
    {
        DWORD animCoef = g_pctx->animCoef;
        if (g_pctx->AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
        if (!g_pctx->windowAnim || !g_pctx->clientAnim) animCoef = 0;
        Sleep(175 * (animCoef / 100.0f));
        SetForegroundWindow(wnd->GetHWND());
        if (g_pctx->windowAnim && g_pctx->clientAnim)
            AnimateWindow(searchwnd->GetHWND(), 120 * (animCoef / 100.0f), AW_BLEND | AW_HIDE);
        else
            searchwnd->ShowWindow(SW_HIDE);
        searchwnd->DestroyWindow();
        g_searchopen = false;
        return 0;
    }

    void LaunchSearchResult(Element* elem, Event* iev)
    {
        short ctrlKey = GetAsyncKeyState(VK_CONTROL);
        short shiftKey = GetAsyncKeyState(VK_SHIFT);
        short enterKey = GetAsyncKeyState(VK_RETURN);
        if (iev->uidType == LVItem::Click)
        {
            if (!(shellstate[4] & 0x20) || enterKey)
                goto CLICKACTION;
        }
        if (iev->uidType == LVItem::MultipleClick && shellstate[4] & 0x20)
        {
        CLICKACTION:
            if (!(ctrlKey & 0x8000))
            {
                wstring temp = ((LVItem*)elem)->GetFilename();
                SHELLEXECUTEINFOW execInfo = {};
                execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
                execInfo.lpVerb = L"open";
                execInfo.nShow = SW_SHOWNORMAL;
                execInfo.lpFile = temp.c_str();
                ShellExecuteExW(&execInfo);
            }
        }
    }

    void DisplayResults(Element* elem, Event* iev)
    {
        static LPWSTR path{};
        GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop", &path);
        if (iev->uidType == TouchButton::Click || iev->uidType == TouchButton::MultipleClick)
        {
            CValuePtr v;
            if (!searchbox->GetContentString(&v))
            {
                MessageBeep(MB_OK);
                DDNotificationBanner* ddnb = new DDNotificationBanner();
                ddnb->CreateBanner(DDNT_INFO, nullptr, L"Type in the search box to search.", 5, nullptr);
                return;
            }
            v->Release();
            //if (wcslen(searchbox->GetContentString(&v)) < 2) return;
            WCHAR* searchquery = new WCHAR[1024];
            if (g_sp.flags & 0x2)
            {
                StringCchPrintfW(searchquery, 1024, L"%s %s", g_sp.path, searchbox->GetContentString(&v));
            }
            else
            {
                WCHAR* PublicPath = new WCHAR[260];
                WCHAR* OneDrivePath = new WCHAR[260];
                WCHAR* cBuffer = new WCHAR[260];
                DWORD d = GetEnvironmentVariableW(L"PUBLIC", cBuffer, 260);
                StringCchPrintfW(PublicPath, 260, L"%s\\Desktop", cBuffer);
                d = GetEnvironmentVariableW(L"OneDrive", cBuffer, 260);
                StringCchPrintfW(OneDrivePath, 260, L"%s\\Desktop", cBuffer);
                StringCchPrintfW(searchquery, 1024, L"%s | %s | %s %s", path, PublicPath, OneDrivePath, searchbox->GetContentString(&v));
                delete[] cBuffer;
                delete[] PublicPath;
                delete[] OneDrivePath;
            }
            CSafeElementPtr<Element> rescontainer;
            rescontainer.Assign(regElem(L"rescontainer", pSearch));
            CSafeElementPtr<LVCommon> LVSearchResults;
            LVSearchResults.Assign((LVCommon*)regElem(L"LVSearchResults", pSearch));

            rescontainer->DestroyAll(true);
            LVSearchResults->DestroyAll(true);
            Everything_SetSearchW(searchquery);
            Everything_QueryW(TRUE);
            delete[] searchquery;
            LVItem* SearchResultPlaceholder{};
            parserSearch->CreateElement(L"SearchResult", NULL, NULL, NULL, (Element**)&SearchResultPlaceholder);

            CSafeElementPtr<DDScalableRichText> ResultCount;
            ResultCount.Assign((DDScalableRichText*)regElem(L"ResultCount", pSearch));
            int rescount = Everything_GetNumResults();
            WCHAR resultc[64];
            if (rescount <= 150)
                StringCchPrintfW(resultc, 64, L"%d items", rescount);
            else
                StringCchPrintfW(resultc, 64, L"1-150 of %d items", rescount);
            ResultCount->SetLayoutPos(1);
            ResultCount->SetContentString(resultc);
            
            vector<LVItem*> spm;
            spm.resize(min(rescount, 150));
            for (int i = 0; i < min(rescount, 150); i++)
            {
            	parserSearch->CreateElement(L"SearchResult", NULL, NULL, NULL, (Element**)&spm[i]);
            	DDScalableRichText* name = (DDScalableRichText*)regElem(L"name", spm[i]);
                DDScalableRichText* path = (DDScalableRichText*)regElem(L"path", spm[i]);
                DDScalableElement* iconElem = (DDScalableElement*)regElem(L"iconElem", spm[i]);
            	name->SetContentString(Everything_GetResultFileNameW(i));
            	path->SetContentString(Everything_GetResultPathW(i));
            	spm[i]->SetFilename((wstring)Everything_GetResultPathW(i) + L"\\" + Everything_GetResultFileNameW(i));
            	spm[i]->SetIcon(iconElem);
            	assignFn(spm[i], LaunchSearchResult);
                DesktopIcon di;
                ApplyIcons(&spm, &di, false, i, 1, -1);
                HBITMAP iconbmp = di.icon;
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                if (spvBitmap) iconElem->SetValue(Element::ContentProp, 1, spvBitmap);
                DeleteObject(iconbmp);
            }
            LVSearchResults->Add((Element**)&spm[0], min(rescount, 150));
            SearchResultPlaceholder->DestroyAll(true);
            SearchResultPlaceholder->Destroy(true);
        }
    }

    void CloseSearch(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click || iev->uidType == TouchButton::Click)
        {
            SetTimer(searchwnd->GetHWND(), 1, 50, nullptr);
        }
    }

    void CreateSearchPage(SearchParams* psp)
    {
        if (g_searchopen) return;
        if (psp)
        {
            g_sp.flags = psp->flags;
            if (psp->flags & 0x1) SendMessageW(g_hWndTaskbar, WM_COMMAND, 419, 0);
            if (psp->flags & 0x2)
            {
                if (g_sp.path)
                    delete g_sp.path;
                g_sp.path = new WCHAR[260];
                if (psp->path)
                    StringCchPrintfW(g_sp.path, 260, L"%s", psp->path);
            }
        }
        g_searchopen = true;
        unsigned long key4 = 0;
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        DWORD dwExStyle = WS_EX_TOOLWINDOW, dwCreateFlags = 0x10;
        if (g_pctx->DWMActive)
        {
            dwExStyle |= WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP;
            dwCreateFlags |= 0x28;
        }
        NativeHWNDHost::Create(L"DD_SearchHost", L"DirectDesktop Everything Search Wrapper", nullptr, nullptr,
            dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, dwExStyle, WS_POPUP, nullptr, 0x43, &searchwnd);
        DUIXmlParser::Create(&parserSearch, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        parserSearch->SetXMLFromResource(IDR_UIFILE5, HINST_THISCOMPONENT, HINST_THISCOMPONENT);
        HWNDElement::Create(searchwnd->GetHWND(), true, dwCreateFlags, nullptr, &key4, (Element**)&parentSearch);
        parserSearch->CreateElement(L"SearchUI", parentSearch, nullptr, nullptr, &pSearch);
        WndProcSearch = (WNDPROC)SetWindowLongPtrW(searchwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SearchWindowProc);
        pSearch->SetVisible(true);
        pSearch->EndDefer(key4);
        CSafeElementPtr<Element> searchbase;
        searchbase.Assign(regElem(L"searchbase", pSearch));
        LPWSTR sheetName = g_pctx->theme ? (LPWSTR)L"searchstyle" : (LPWSTR)L"searchstyledark";
        StyleSheet* sheet = pSearch->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parserSearch->GetSheet(sheetName, &sheetStorage);
        pSearch->SetValue(Element::SheetProp, 1, sheetStorage);
        searchbox = (DDScalableTouchEdit*)regElem(L"searchbox", pSearch);
        searchbox->SetContextMenu(true);
        CSafeElementPtr<DDScalableTouchButton> searchbutton;
        searchbutton.Assign((DDScalableTouchButton*)regElem(L"searchbutton", pSearch));
        assignFn(searchbutton, DisplayResults);
        CSafeElementPtr<DDScalableTouchButton> closebutton;
        closebutton.Assign((DDScalableTouchButton*)regElem(L"closebutton", pSearch));
        assignFn(closebutton, CloseSearch);
        CSafeElementPtr<TouchScrollViewer> SearchResults;
        SearchResults.Assign((TouchScrollViewer*)regElem(L"SearchResults", pSearch));
        CSafeElementPtr<Element> pagecontent;
        pagecontent.Assign(regElem(L"pagecontent", pSearch));
        searchwnd->Host(pSearch);
        searchwnd->ShowWindow(SW_HIDE);
        float flBackFade = 1.0f;
        SYSTEM_POWER_STATUS sps;
        GetSystemPowerStatus(&sps);
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"EnableTransparency") != 1 || sps.SystemStatusFlag)
            flBackFade = 0.33f;
        GTRANS_DESC transDesc[2];
        TriggerScaleOut(UIContainer, transDesc, 0, 0.0f, 0.67f, 0.1f, 0.9f, 0.2f, 1.0f, 0.92f, 0.92f, 0.5f, 0.5f, false, false);
        if (!g_editmode) TriggerFade(UIContainer, transDesc, 1, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, flBackFade, false, false, true);
        TransitionStoryboardInfo tsbInfo = {};
        ScheduleGadgetTransitions_DWMCheck(0, g_editmode ? 1 : 2, transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
        GTRANS_DESC transDesc2[2];
        TriggerFade(pagecontent, transDesc2, 0, 0.05f, 0.18f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, true, false, false);
        TriggerScaleIn(pagecontent, transDesc2, 1, 0.05f, 0.72f, 0.1f, 0.9f, 0.2f, 1.0f, 0.75f, 0.75f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, pagecontent->GetDisplayNode(), &tsbInfo);
        MARGINS m = { -1, -1, -1, -1 };
        if (g_pctx->DWMActive)
        {
            AddLayeredRef(searchbase->GetDisplayNode());
            SetGadgetFlags(searchbase->GetDisplayNode(), NULL, NULL);
            AddLayeredRef(pagecontent->GetParent()->GetDisplayNode());
            SetGadgetFlags(pagecontent->GetParent()->GetDisplayNode(), NULL, NULL);
            DwmExtendFrameIntoClientArea(searchwnd->GetHWND(), &m);
        }
        HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateSearchWindow, nullptr, NULL, nullptr);
        if (AnimHandle) CloseHandle(AnimHandle);
        BlurBackground(searchwnd->GetHWND(), true, true, 0x99, searchbase);

        g_EverythingDLL = LoadLibraryW(L"Everything64.dll");

        if (!g_EverythingDLL)
        {
            DDNotificationBanner* ddnb = new DDNotificationBanner();
            ddnb->CreateBanner(DDNT_ERROR, nullptr, L"Failed to load Everything64.dll.", 5, nullptr);
            WCHAR closeText[32];
            LoadStrFromRes(closeText, 32, 4160, L"comctl32.dll");
            ddnb->AppendButton(closeText, CloseSearch, true);
            searchbutton->SetEnabled(false);
            searchbox->SetEnabled(false);
            // 0.6 M4: Replace with DDScalableTouchEdit::SetPromptText when implemented 
            CValuePtr v = Value::CreateString(L" ", nullptr);
            searchbox->SetValue(DDScalableTouchEdit::PromptTextProp, 1, v);
            return;
        }

        Everything_SetSearchW = (pfnE_SetSearchW)GetProcAddress(g_EverythingDLL, "Everything_SetSearchW");
        Everything_QueryW = (pfnE_QueryW)GetProcAddress(g_EverythingDLL, "Everything_QueryW");
        Everything_GetNumResults = (pfnE_GetNumResults)GetProcAddress(g_EverythingDLL, "Everything_GetNumResults");
        Everything_GetResultFileNameW = (pfnE_GetResultFileNameW)GetProcAddress(g_EverythingDLL, "Everything_GetResultFileNameW");
        Everything_GetResultPathW = (pfnE_GetResultPathW)GetProcAddress(g_EverythingDLL, "Everything_GetResultPathW");
        Everything_GetLastError = (pfnE_GetLastError)GetProcAddress(g_EverythingDLL, "Everything_GetLastError");

        if (!(Everything_SetSearchW && Everything_QueryW && Everything_GetNumResults &&
            Everything_GetResultFileNameW && Everything_GetResultPathW && Everything_GetLastError))
        {
            DDNotificationBanner* ddnb = new DDNotificationBanner();
            ddnb->CreateBanner(DDNT_ERROR, nullptr, L"Failed to load Everything64.dll.", 5, nullptr);
            WCHAR closeText[32];
            LoadStrFromRes(closeText, 32, 4160, L"comctl32.dll");
            ddnb->AppendButton(closeText, CloseSearch, true);
            searchbutton->SetEnabled(false);
            searchbox->SetEnabled(false);
            // 0.6 M4: Replace with DDScalableTouchEdit::SetPromptText when implemented 
            CValuePtr v = Value::CreateString(L" ", nullptr);
            searchbox->SetValue(DDScalableTouchEdit::PromptTextProp, 1, v);
            return;
        }
    }

    void DestroySearchPage()
    {
        FreeLibrary(g_EverythingDLL);
        float flBackFade = 1.0f;
        SYSTEM_POWER_STATUS sps;
        GetSystemPowerStatus(&sps);
        if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"EnableTransparency") != 1 || sps.SystemStatusFlag)
            flBackFade = 0.33f;
        GTRANS_DESC transDesc[2];
        TriggerScaleOut(UIContainer, transDesc, 0, 0.175f, 0.675f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
        TriggerFade(UIContainer, transDesc, 1, 0.175f, 0.375f, 0.0f, 0.0f, 1.0f, 1.0f, flBackFade, 1.0f, false, false, false);
        TransitionStoryboardInfo tsbInfo = {};
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
        DUI_SetGadgetZOrder(UIContainer, -1);
        CSafeElementPtr<Element> pagecontent;
        pagecontent.Assign(regElem(L"pagecontent", pSearch));
        GTRANS_DESC transDesc2[2];
        TriggerScaleOut(pagecontent, transDesc2, 0, 0.0f, 0.175f, 1.0f, 1.0f, 0.0f, 1.0f, 0.95f, 0.95f, 0.5f, 0.5f, false, false);
        TriggerFade(pagecontent, transDesc2, 1, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, pagecontent->GetDisplayNode(), &tsbInfo);
        SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(nullptr, 0, AnimateSearchWindow2, nullptr, 0, &animThread);
        if (animThreadHandle) CloseHandle(animThreadHandle);
    }
}
