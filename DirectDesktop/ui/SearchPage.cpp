#include "pch.h"

#include "SearchPage.h"
#include "..\DirectDesktop.h"
#include "..\coreui\BitmapHelper.h"
#include "..\backend\DirectoryHelper.h"

#ifdef HAS_SEARCH
#include "EverythingSearch/Everything.h"
#pragma comment (lib, "Everything64.lib")
#endif

using namespace DirectUI;

namespace DirectDesktop
{
    NativeHWNDHost* searchwnd;
    DUIXmlParser* parserSearch;
    HWNDElement* parentSearch;
    Element* pSearch;
    TouchEdit2* searchbox;
    WNDPROC WndProcSearch;

    void DestroySearchPage();

    LRESULT CALLBACK SearchWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_CLOSE:
                SetTimer(hWnd, 1, 50, nullptr);
                return 0;
                break;
            case WM_DESTROY:
                return 0;
                break;
            case WM_TIMER:
                KillTimer(hWnd, wParam);
                switch (wParam)
                {
                case 1:
                    DestroySearchPage();
                    break;
                }
        }
        return CallWindowProc(WndProcSearch, hWnd, uMsg, wParam, lParam);
    }

    void LaunchSearchResult(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
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

    void DisplayResults(Element* elem, Event* iev)
    {
        static LPWSTR path{};
        GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop", &path);
        if (iev->uidType == Button::Click)
        {
            MessageBeep(MB_OK);
            //CValuePtr v;
            //if (wcslen(searchbox->GetContentString(&v)) < 2) return;
            //WCHAR* PublicPath = new WCHAR[260];
            //WCHAR* OneDrivePath = new WCHAR[260];
            //WCHAR* cBuffer = new WCHAR[260];
            //DWORD d = GetEnvironmentVariableW(L"PUBLIC", cBuffer, 260);
            //StringCchPrintfW(PublicPath, 260, L"%s\\Desktop", cBuffer);
            //d = GetEnvironmentVariableW(L"OneDrive", cBuffer, 260);
            //StringCchPrintfW(OneDrivePath, 260, L"%s\\Desktop", cBuffer);
            CSafeElementPtr<Element> rescontainer;
            rescontainer.Assign(regElem<Element*>(L"rescontainer", pSearch));
            //rescontainer->DestroyAll(true);
            //WCHAR* searchquery = new WCHAR[1024];
            //StringCchPrintfW(searchquery, 1024, L"%s | %s | %s %s", path, PublicPath, OneDrivePath, searchbox->GetContentString(&v));
            //Everything_SetSearchW(searchquery);
            //Everything_QueryW(TRUE);
            //delete[] searchquery;
            //delete[] cBuffer;
            //delete[] PublicPath;
            //delete[] OneDrivePath;
            //LVItem* SearchResultPlaceholder{};
            //parserSearch->CreateElement(L"SearchResult", NULL, NULL, NULL, (Element**)&SearchResultPlaceholder);
            RichText* ResultCount{};
            parserSearch->CreateElement(L"ResultCount", nullptr, nullptr, nullptr, (Element**)&ResultCount);
            rescontainer->Add((Element**)&ResultCount, 1);
            //WCHAR* resultc = new WCHAR[64];
            //StringCchPrintfW(resultc, 64, L"%d items", Everything_GetNumResults());
            MessageBeep(MB_OK);
            DDNotificationBanner* ddnb{};
            DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_INFO, L"DDNB", nullptr, L"Search will be available by version 0.6", 5, false);
            //rescontainer->SetHeight(Everything_GetNumResults() * SearchResultPlaceholder->GetHeight() + 40);
            //for (int i = 0; i < Everything_GetNumResults(); i++) {
            //	WCHAR* nameStr = new WCHAR[256];
            //	WCHAR* pathStr = new WCHAR[256];
            //	StringCchPrintfW(nameStr, 256, L"NAME: %s", Everything_GetResultFileNameW(i));
            //	StringCchPrintfW(pathStr, 256, L"PATH: %s", Everything_GetResultPathW(i));
            //	LVItem* SearchResult{};
            //	parserSearch->CreateElement(L"SearchResult", NULL, NULL, NULL, (Element**)&SearchResult);
            //	rescontainer->Add((Element**)&SearchResult, 1);
            //	RichText* name = regElem<RichText*>(L"name", SearchResult);
            //	RichText* path = regElem<RichText*>(L"path", SearchResult);
            //	name->SetContentString(nameStr);
            //	path->SetContentString(pathStr);
            //	SearchResult->SetFilename((wstring)Everything_GetResultPathW(i) + L"\\" + Everything_GetResultFileNameW(i));
            //	assignFn(SearchResult, LaunchSearchResult);
            //	delete[] nameStr;
            //	delete[] pathStr;
            //}
            //delete[] resultc;
            //free(SearchResultPlaceholder);
        }
    }

    void UpdateSearchBox(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::KeyWithinProp())
        {
            CSafeElementPtr<Element> searchboxtext;
            searchboxtext.Assign(regElem<Element*>(L"searchboxtext", pSearch));
            CSafeElementPtr<DDScalableElement> searchboxbackground;
            searchboxbackground.Assign(regElem<DDScalableElement*>(L"searchboxbackground", pSearch));
            CValuePtr v;
            searchboxtext->SetVisible(!elem->GetKeyWithin());
            searchboxtext->SetContentString(elem->GetContentString(&v));
            if (searchboxbackground) searchboxbackground->SetSelected(elem->GetKeyWithin());
        }
        if (pProp == Element::MouseWithinProp())
        {
            CSafeElementPtr<DDScalableElement> searchboxbackground;
            searchboxbackground.Assign(regElem<DDScalableElement*>(L"searchboxbackground", pSearch));
            if (searchboxbackground) searchboxbackground->SetOverhang(elem->GetMouseWithin());
        }
        if (pProp == Element::EnabledProp())
        {
            CSafeElementPtr<DDScalableElement> searchboxbackground;
            searchboxbackground.Assign(regElem<DDScalableElement*>(L"searchboxbackground", pSearch));
            if (searchboxbackground) searchboxbackground->SetEnabled(elem->GetEnabled());
        }
    }

    void CloseSearch(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            ((DDScalableButton*)elem)->StopListening();
            SetTimer(searchwnd->GetHWND(), 1, 50, nullptr);
        }
    }

    void CreateSearchPage()
    {
        unsigned long key4 = 0;
        RECT dimensions;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        static IElementListener *pel_DisplayResults, *pel_CloseSearch, *pel_UpdateSearchBox;
        NativeHWNDHost::Create(L"DD_SearchHost", L"DirectDesktop Everything Search Wrapper", nullptr, nullptr, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, WS_POPUP, nullptr, 0x43, &searchwnd);
        DUIXmlParser::Create(&parserSearch, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        parserSearch->SetXMLFromResource(IDR_UIFILE5, HINST_THISCOMPONENT, HINST_THISCOMPONENT);
        HWNDElement::Create(searchwnd->GetHWND(), true, 0x38, nullptr, &key4, (Element**)&parentSearch);
        parserSearch->CreateElement(L"SearchUI", parentSearch, nullptr, nullptr, &pSearch);
        WndProcSearch = (WNDPROC)SetWindowLongPtrW(searchwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SearchWindowProc);
        pSearch->SetVisible(true);
        pSearch->EndDefer(key4);
        searchwnd->Host(pSearch);
        CSafeElementPtr<Element> searchbase;
        searchbase.Assign(regElem<Element*>(L"searchbase", pSearch));
        MARGINS m = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(searchwnd->GetHWND(), &m);
        BlurBackground(searchwnd->GetHWND(), true, true, searchbase);
        LPWSTR sheetName = g_theme ? (LPWSTR)L"searchstyle" : (LPWSTR)L"searchstyledark";
        StyleSheet* sheet = pSearch->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parserSearch->GetSheet(sheetName, &sheetStorage);
        pSearch->SetValue(Element::SheetProp, 1, sheetStorage);
        searchwnd->ShowWindow(SW_SHOW);
        searchbox = (TouchEdit2*)pSearch->FindDescendent(StrToID(L"searchbox"));
        free(pel_DisplayResults), free(pel_CloseSearch), free(pel_UpdateSearchBox);
        CSafeElementPtr<DDScalableButton> searchbutton;
        searchbutton.Assign(regElem<DDScalableButton*>(L"searchbutton", pSearch));
        pel_DisplayResults = (IElementListener*)assignFn(searchbutton, DisplayResults, true);
        CSafeElementPtr<DDScalableButton> closebutton;
        closebutton.Assign(regElem<DDScalableButton*>(L"closebutton", pSearch));
        pel_CloseSearch = (IElementListener*)assignFn(closebutton, CloseSearch, true);
        pel_UpdateSearchBox = (IElementListener*)assignExtendedFn(searchbox, UpdateSearchBox, true);
        CSafeElementPtr<TouchScrollViewer> SearchResults;
        SearchResults.Assign(regElem<TouchScrollViewer*>(L"SearchResults", pSearch));
        GTRANS_DESC transDesc2[1];
        TriggerScaleOut(UIContainer, transDesc2, 0, 0.0f, 0.67f, 0.1f, 0.9f, 0.2f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, false);
        TransitionStoryboardInfo tsbInfo2 = {};
        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, UIContainer->GetDisplayNode(), &tsbInfo2);
    }

    void DestroySearchPage()
    {
        pSearch->DestroyAll(true);
        searchwnd->DestroyWindow();
        GTRANS_DESC transDesc2[1];
        TriggerScaleOut(UIContainer, transDesc2, 0, 0.175f, 0.675f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
        TransitionStoryboardInfo tsbInfo2 = {};
        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, UIContainer->GetDisplayNode(), &tsbInfo2);
        DUI_SetGadgetZOrder(UIContainer, -1);
        SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
    }
}
