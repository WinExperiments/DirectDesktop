#include "pch.h"

#include "SearchPage.h"
#include "..\DirectDesktop.h"
#include "..\backend\ContextMenus.h"
#include "..\backend\DirectoryHelper.h"
#include "..\backend\SettingsHelper.h"
#include <wrl.h>


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
    HANDLE g_searchSemaphore = CreateSemaphoreW(nullptr, 16, 16, nullptr);
    NativeHWNDHost* searchwnd;
    DUIXmlParser* parserSearch;
    HWNDElement* parentSearch;
    Element* pSearch;
    DDScalableTouchEdit* searchbox;
    DDScalableTouchButton* PreviousResults, *NextResults;
    WNDPROC WndProcSearch;
    SearchParams g_sp;
    vector<LVItem*> spm;
    UINT g_offset;

    DWORD WINAPI AnimateSearchWindow(LPVOID lpParam);
    void DestroySearchPage();
    void ChangeSearchFolder(Element* elem, Event* iev);

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
                        TriggerScaleIn(pagecontent, transDesc, 0, 0.05f, 0.3f, 0.25f, 0.1f, 0.25f, 1.0f, 0.98f, 0.98f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, true, false);
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
            case WM_USER + 2:
            {
                DesktopIcon* pdi = (DesktopIcon*)wParam;
                yValueEx* yV = (yValueEx*)lParam;
                vector<LVItem*>* l_pm = yV->vpm;
                HBITMAP iconbmp = pdi->icon;
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconbmp);
                if (spvBitmap) (*l_pm)[yV->num]->GetIcon()->SetValue(Element::ContentProp, 1, spvBitmap);
                HBITMAP iconshortcutbmp = pdi->iconshortcut;
                CValuePtr spvBitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshortcutbmp);
                if (spvBitmapShortcut && (*l_pm)[yV->num]->GetFlags() & LVIF_SHORTCUT)
                    (*l_pm)[yV->num]->GetShortcutArrow()->SetValue(Element::ContentProp, 1, spvBitmapShortcut);
                break;
            }
            case WM_USER + 3:
            {
                pSearch->DestroyAll(true);
                pSearch->Destroy(true);
                searchwnd->DestroyWindow();
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
        SendMessageW(searchwnd->GetHWND(), WM_USER + 3, NULL, NULL);
        g_searchopen = false;
        return 0;
    }

    DWORD WINAPI CreateSRIcon(LPVOID lpParam)
    {
        yValueEx* yV = (yValueEx*)lpParam;
        DesktopIcon di;
        ApplyIcons(yV->vpm, &di, false, yV->num, 32 / static_cast<float>(g_iconsz), -1, false);
        SendMessageW(searchwnd->GetHWND(), WM_USER + 2, (WPARAM)&di, (LPARAM)yV);
        Sleep(250);
        delete yV;
        return 0;
    }

    DWORD WINAPI CreateSRIconHelper(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        WaitForSingleObject(g_searchSemaphore, 0);
        yValueEx* yV = static_cast<yValueEx*>(lpParam);
        CreateSRIcon(yV);
        ReleaseSemaphore(g_searchSemaphore, 1, nullptr);
        UnInitThread();
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
                LPITEMIDLIST pidl = nullptr;
                HRESULT hr = SHParseDisplayName(temp.c_str(), nullptr, &pidl, 0, nullptr);
                if (SUCCEEDED(hr))
                {
                    Microsoft::WRL::ComPtr<IShellFolder> ppFolder = nullptr;
                    LPITEMIDLIST pidlChild = nullptr;
                    hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&ppFolder, (LPCITEMIDLIST*)&pidlChild);
                    if (SUCCEEDED(hr))
                    {
                        Microsoft::WRL::ComPtr<IContextMenu> pICv1 = nullptr;
                        ppFolder->GetUIObjectOf(nullptr, 1, (LPCITEMIDLIST*)&pidlChild, IID_IContextMenu, nullptr, (void**)&pICv1);
                        if (SUCCEEDED(hr))
                        {
                            HMENU hmDummy = CreatePopupMenu();
                            pICv1->QueryContextMenu(hmDummy, 0, MIN_SHELL_ID, MAX_SHELL_ID, CMF_DEFAULTONLY);

                            CMINVOKECOMMANDINFO ici;
                            ZeroMemory(&ici, sizeof(ici));
                            ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
                            ici.lpVerb = "open";
                            ici.nShow = SW_SHOWNORMAL;
                            hr = pICv1->InvokeCommand(&ici);
                            if (hr == E_OUTOFMEMORY)
                            {
                                ici.lpVerb = "openas";
                                hr = pICv1->InvokeCommand(&ici);
                            }
                        }
                    }
                }
                ILFree(pidl);
            }
        }
    }

    void DisplayResults(UINT idxOffset)
    {
        LPWSTR path{};
        GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop", &path);
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
                StringCchPrintfW(searchquery, 1024, L"path:\"%s\\\" %s", RemoveQuotes(g_sp.path).c_str(), searchbox->GetContentString(&v));
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
                StringCchPrintfW(searchquery, 1024, L"path:\"%s\\\" | path:\"%s\\\" | path:\"%s\\\" %s", path, PublicPath, OneDrivePath, searchbox->GetContentString(&v));
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

            CSafeElementPtr<DDScalableRichText> ResultInfo;
            ResultInfo.Assign((DDScalableRichText*)regElem(L"ResultInfo", pSearch));
            ResultInfo->SetLayoutPos(1);
            CSafeElementPtr<DDScalableRichText> ResultCount;
            ResultCount.Assign((DDScalableRichText*)regElem(L"ResultCount", pSearch));
            int rescount = Everything_GetNumResults();
            WCHAR itemCount[32], temp[32];
            LoadStrFromRes(temp, 32, 4032);
            if (rescount == 1) LoadStrFromRes(itemCount, 32, 4031);
            else StringCchPrintfW(itemCount, 32, temp, rescount);
            ResultCount->SetContentString(itemCount);
            WCHAR resultc[64]{};
            PreviousResults->SetVisible(rescount > 100);
            NextResults->SetVisible(rescount > 100);
            if (rescount > 100)
            {
                StringCchPrintfW(resultc, 64, L"%d-%d", 1 + idxOffset, min(rescount, 100 + idxOffset));
                PreviousResults->SetEnabled(idxOffset >= 100);
                NextResults->SetEnabled(idxOffset + 100 < rescount);
            }
            CSafeElementPtr<DDScalableRichText> CurrentResults;
            CurrentResults.Assign((DDScalableRichText*)regElem(L"CurrentResults", pSearch));
            CurrentResults->SetContentString(resultc);
            
            spm.clear();
            spm.resize(min(rescount - idxOffset, 100));
            for (int i = 0; i < min(rescount - idxOffset, 100); i++)
            {
            	parserSearch->CreateElement(L"SearchResult", NULL, NULL, NULL, (Element**)&spm[i]);
            	DDScalableRichText* name = (DDScalableRichText*)regElem(L"name", spm[i]);
                DDScalableRichText* path = (DDScalableRichText*)regElem(L"path", spm[i]);
                DDScalableElement* iconElem = (DDScalableElement*)regElem(L"iconElem", spm[i]);
                Element* shortcutElem = regElem(L"shortcutElem", spm[i]);
                LPCWSTR evName = Everything_GetResultFileNameW(i + idxOffset);
                LPCWSTR evPath = Everything_GetResultPathW(i + idxOffset);
            	path->SetContentString(evPath);
            	spm[i]->SetFilename((wstring)evPath + L"\\" + evName);
            	spm[i]->SetIcon(iconElem);
            	spm[i]->SetShortcutArrow(shortcutElem);
                wstring filenameNew = hideExt(evName, g_hideFileExt, false, spm[i]);
                name->SetContentString(filenameNew.c_str());
                if (g_isThumbnailHidden == 0)
                {
                    bool image;
                    isSpecialProp(spm[i]->GetFilename(), true, &image, &imageExts);
                    if (image) spm[i]->AddFlags(LVIF_COLORLOCK);
                }
                bool advancedicon;
                isSpecialProp(spm[i]->GetFilename(), true, &advancedicon, &advancedIconExts);
                if (advancedicon) spm[i]->AddFlags(LVIF_ADVANCEDICON);
            	assignFn(spm[i], LaunchSearchResult);
            	assignFn(spm[i], ItemRightClick);
                yValueEx* yV = new yValueEx{ i, NULL, NULL, &spm, nullptr, nullptr };
                QueueUserWorkItem(CreateSRIconHelper, yV, 0);
            }
            LVSearchResults->AddFlags(LVCF_NOANIMATE);
            LVSearchResults->Add((Element**)&spm[0], min(rescount - idxOffset, 100));
            LVSearchResults->RemoveFlags(LVCF_NOANIMATE);
            GTRANS_DESC transDesc[2];
            TriggerTranslate(LVSearchResults, transDesc, 0, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_pctx->flScaleFactor, 0.0f, 0.0f, false, false, false);
            TriggerFade(LVSearchResults, transDesc, 1, 0.2f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, LVSearchResults->GetDisplayNode(), &tsbInfo);
            SearchResultPlaceholder->DestroyAll(true);
            SearchResultPlaceholder->Destroy(true);
    }

    void DisplayResultsFromButton(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click || iev->uidType == TouchButton::MultipleClick)
        {
            WCHAR atomName[32];
            GetAtomNameW(elem->GetID(), atomName, 32);
            if (wcscmp(atomName, L"PreviousResults") == 0)
                g_offset -= 100;
            else if (wcscmp(atomName, L"NextResults") == 0)
                g_offset += 100;
            else
                g_offset = 0;
            DisplayResults(g_offset);
        }
    }

    void DisplayResultsFromBox(Element* elem, InputEvent* ev)
    {
        if (ev->nDevice == GINPUT_KEYBOARD && ev->nCode == GMOUSE_DOWN && ev->nStage == GMF_BUBBLED)
        {
            if (GetAsyncKeyState(VK_RETURN) & 1)
            {
                g_offset = 0;
                DisplayResults(0);
            }
        }
    }

    void CloseSearch(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click || iev->uidType == TouchButton::Click)
        {
            SetTimer(searchwnd->GetHWND(), 1, 50, nullptr);
        }
    }

    void PreparePathFilterButton(DDIconButton* peButton, SearchParams* psp)
    {
        LVItem* lvi;
        LVItem::Create(nullptr, nullptr, (Element**)&lvi);
        lvi->SetFilename(psp->path);
        vector<LVItem*> v;
        v.push_back(lvi);
        DesktopIcon di;
        ApplyIcons(&v, &di, false, 0, 16 / static_cast<float>(g_iconsz), -1, false);
        HBITMAP iconbmp = di.icon;
        CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
        if (spvBitmap) peButton->SetValue(DDIconButton::IconContentProp, 1, spvBitmap);
        peButton->SetContentString(psp->displaypath ? psp->displaypath : psp->path);
        peButton->SetAccDesc(RemoveQuotes(psp->path).c_str());
        peButton->SetTooltip(true);
        peButton->SetTooltipMaxWidth(400 * g_pctx->flScaleFactor);
        DeleteObject(iconbmp);
        DeleteObject(di.iconshortcut);
        lvi->Destroy(true);
    }

    void ChangeSearchFolder(Element* elem, Event* iev)
    {
        if (iev->uidType == DDIconButton::Click)
        {
            Microsoft::WRL::ComPtr<IFileOpenDialog> pfd = nullptr;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfd));
            if (SUCCEEDED(hr))
            {
                FILEOPENDIALOGOPTIONS fos;
                hr = pfd->GetOptions(&fos);
                pfd->SetOptions(fos | FOS_PICKFOLDERS);
                g_peek = true; // Prevent peek animation because WM_CANCELMODE is sent on IFileOpenDialog::Show
                if (SUCCEEDED(pfd->Show(searchwnd->GetHWND())))
                {
                    Microsoft::WRL::ComPtr<IShellItem> psi = nullptr;
                    if (SUCCEEDED(pfd->GetResult(&psi)))
                    {
                        LPWSTR pszAbs, pszDisp;
                        psi->GetDisplayName(SIGDN_FILESYSPATH, &pszAbs);
                        psi->GetDisplayName(SIGDN_NORMALDISPLAY, &pszDisp);
                        if (!g_sp.path)
                            g_sp.path = new WCHAR[260];
                        if (!g_sp.displaypath)
                            g_sp.displaypath = new WCHAR[260];
                        LPWSTR defpath{};
                        GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop", &defpath);
                        if (pszAbs && wcscmp(pszAbs, defpath) == 0)
                            g_sp.flags &= 0xFFFFFFFD;
                        else
                            g_sp.flags |= 0x2;
                        StringCchPrintfW(g_sp.path, 260, L"\"%s\"", pszAbs);
                        StringCchPrintfW(g_sp.displaypath, 260, L"%s", pszDisp);
                        PreparePathFilterButton((DDIconButton*)elem, &g_sp);
                        CoTaskMemFree(pszAbs);
                        CoTaskMemFree(pszDisp);
                        free(defpath);
                    }
                }
                g_peek = false;
            }
        }
    }

    HRESULT CreateSearchFilter(UINT type, LPCWSTR pszPrefix, LPVOID pvFilter, LPCWSTR pszSuffix)
    {
        CSafeElementPtr<Element> SearchFilters;
        SearchFilters.Assign(regElem(L"SearchFilters", pSearch));
        Element* SearchFilter;
        HRESULT hr = parserSearch->CreateElement(L"SearchFilter", nullptr, nullptr, nullptr, &SearchFilter);
        if (SUCCEEDED(hr))
        {
            hr = SearchFilters->Add(&SearchFilter, 1);
            if (SUCCEEDED(hr))
            {
                CSafeElementPtr<DDScalableRichText> pePrefix;
                pePrefix.Assign((DDScalableRichText*)regElem(L"SFPrefix", SearchFilter));
                CSafeElementPtr<DDIconButton> peButton;
                peButton.Assign((DDIconButton*)regElem(L"SFInfo", SearchFilter));
                CSafeElementPtr<DDScalableRichText> peSuffix;
                peSuffix.Assign((DDScalableRichText*)regElem(L"SFSuffix", SearchFilter));
                if (pszPrefix)
                    pePrefix->SetContentString(pszPrefix);
                if (pszSuffix)
                    peSuffix->SetContentString(pszSuffix);
                // Types:
                // 1: Filename strings
                switch (type)
                {
                case 1:
                {
                    SearchParams* psp = (SearchParams*)pvFilter;
                    PreparePathFilterButton(peButton, psp);
                    assignFn(peButton, ChangeSearchFolder);
                    break;
                }
                }
            }
        }
        return hr;
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
                    delete[] g_sp.path;
                g_sp.path = new WCHAR[260];
                if (g_sp.displaypath)
                    delete[] g_sp.displaypath;
                g_sp.displaypath = new WCHAR[260];
                if (psp->path)
                {
                    StringCchPrintfW(g_sp.path, 260, L"%s", psp->path);
                    if (psp->displaypath)
                        StringCchPrintfW(g_sp.displaypath, 260, L"%s", psp->displaypath);
                }
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
        assignInputFn(searchbox, DisplayResultsFromBox);
        CSafeElementPtr<DDScalableTouchButton> searchbutton;
        searchbutton.Assign((DDScalableTouchButton*)regElem(L"searchbutton", pSearch));
        assignFn(searchbutton, DisplayResultsFromButton);
        CSafeElementPtr<DDScalableTouchButton> closebutton;
        closebutton.Assign((DDScalableTouchButton*)regElem(L"closebutton", pSearch));
        assignFn(closebutton, CloseSearch);
        PreviousResults = (DDScalableTouchButton*)regElem(L"PreviousResults", pSearch);
        NextResults = (DDScalableTouchButton*)regElem(L"NextResults", pSearch);
        assignFn(PreviousResults, DisplayResultsFromButton);
        assignFn(NextResults, DisplayResultsFromButton);
        CSafeElementPtr<TouchScrollViewer> SearchResults;
        SearchResults.Assign((TouchScrollViewer*)regElem(L"SearchResults", pSearch));
        CSafeElementPtr<Element> pagecontent;
        pagecontent.Assign(regElem(L"pagecontent", pSearch));
        searchwnd->Host(pSearch);
        searchwnd->ShowWindow(SW_HIDE);
        TransitionStoryboardInfo tsbInfo = {};
        if (!(g_sp.flags & 0x4))
        {
            float flBackFade = 1.0f;
            SYSTEM_POWER_STATUS sps;
            GetSystemPowerStatus(&sps);
            if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"EnableTransparency") != 1 || sps.SystemStatusFlag)
                flBackFade = 0.33f;
            GTRANS_DESC transDesc[2];
            TriggerScaleOut(UIContainer, transDesc, 0, 0.0f, 0.67f, 0.1f, 0.9f, 0.2f, 1.0f, 0.92f, 0.92f, 0.5f, 0.5f, false, false);
            if (!g_editmode) TriggerFade(UIContainer, transDesc, 1, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, flBackFade, false, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, g_editmode ? 1 : 2, transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
        }
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

        if (g_sp.flags & 0x2)
        {
            if (g_sp.path)
                CreateSearchFilter(1, L"Searching in", &g_sp, nullptr);
        }
        else
        {
            LPWSTR path{};
            GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop", &path);
            LPITEMIDLIST pidl = nullptr;
            HRESULT hr = SHParseDisplayName(path, nullptr, &pidl, 0, nullptr);
            STRRET pStr;
            pStr.uType = STRRET_WSTR;
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<IShellFolder> ppFolder = nullptr;
                hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&ppFolder, nullptr);
                ppFolder->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &pStr);
            }
            SearchParams sp = { g_sp.flags, path, pStr.pOleStr };
            CreateSearchFilter(1, L"Searching in", &sp, nullptr);
            free(path);
        }

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
        TransitionStoryboardInfo tsbInfo = {};
        if (!(g_sp.flags & 0x4))
        {
            float flBackFade = 1.0f;
            SYSTEM_POWER_STATUS sps;
            GetSystemPowerStatus(&sps);
            if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"EnableTransparency") != 1 || sps.SystemStatusFlag)
                flBackFade = 0.33f;
            GTRANS_DESC transDesc[2];
            TriggerScaleOut(UIContainer, transDesc, 0, 0.175f, 0.675f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            TriggerFade(UIContainer, transDesc, 1, 0.175f, 0.375f, 0.0f, 0.0f, 1.0f, 1.0f, flBackFade, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
            DUI_SetGadgetZOrder(UIContainer, -1);
        }
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
