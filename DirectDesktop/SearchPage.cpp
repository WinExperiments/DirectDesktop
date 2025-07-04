#include "SearchPage.h"
#include "DirectDesktop.h"
#include "DDControls.h"
//#include "EverythingSearch/Everything.h"
#include "resource.h"
#include <strsafe.h>
#include "BitmapHelper.h"
#include "DirectoryHelper.h"
#include <shellapi.h>

//#pragma comment (lib, "Everything64.lib")

using namespace DirectUI;

namespace DirectDesktop
{
	NativeHWNDHost* searchwnd;
	DUIXmlParser* parser4;
	HWNDElement* parent3;
	Element* pSearch;
	TouchEdit2* searchbox;
	WNDPROC WndProc5;

	void DestroySearchPage();

	LRESULT CALLBACK SearchWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_CLOSE:
			DestroySearchPage();
			return 0;
			break;
		case WM_DESTROY:
			return 0;
			break;
		}
		return CallWindowProc(WndProc5, hWnd, uMsg, wParam, lParam);
	}

	void LaunchSearchResult(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			wstring temp = ((LVItem*)elem)->GetFilename();
			SHELLEXECUTEINFOW execInfo = {};
			execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
			execInfo.lpVerb = L"open";
			execInfo.nShow = SW_SHOWNORMAL;
			execInfo.lpFile = temp.c_str();
			ShellExecuteExW(&execInfo);
		}
	}

	void DisplayResults(Element* elem, Event* iev) {
		static LPWSTR path{};
		GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop", &path);
		if (iev->uidType == Button::Click) {
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
			Element* rescontainer = regElem<Element*>(L"rescontainer", pSearch);
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
			//parser4->CreateElement(L"SearchResult", NULL, NULL, NULL, (Element**)&SearchResultPlaceholder);
			RichText* ResultCount{};
			parser4->CreateElement(L"ResultCount", NULL, NULL, NULL, (Element**)&ResultCount);
			rescontainer->Add((Element**)&ResultCount, 1);
			//WCHAR* resultc = new WCHAR[64];
			//StringCchPrintfW(resultc, 64, L"%d items", Everything_GetNumResults());
			MessageBeep(MB_OK);
			DDNotificationBanner* ddnb{};
			DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_INFO, L"DDNB", NULL, L"Search will be available by version 0.6", 5, false);
			//rescontainer->SetHeight(Everything_GetNumResults() * SearchResultPlaceholder->GetHeight() + 40);
			//for (int i = 0; i < Everything_GetNumResults(); i++) {
			//	WCHAR* nameStr = new WCHAR[256];
			//	WCHAR* pathStr = new WCHAR[256];
			//	StringCchPrintfW(nameStr, 256, L"NAME: %s", Everything_GetResultFileNameW(i));
			//	StringCchPrintfW(pathStr, 256, L"PATH: %s", Everything_GetResultPathW(i));
			//	LVItem* SearchResult{};
			//	parser4->CreateElement(L"SearchResult", NULL, NULL, NULL, (Element**)&SearchResult);
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
	void UpdateSearchBox(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		if (pProp == Element::KeyWithinProp()) {
			Element* searchboxtext = regElem<Element*>(L"searchboxtext", pSearch);
			DDScalableElement* searchboxbackground = regElem<DDScalableElement*>(L"searchboxbackground", pSearch);
			CValuePtr v;
			searchboxtext->SetVisible(!elem->GetKeyWithin());
			searchboxtext->SetContentString(elem->GetContentString(&v));
			if (searchboxbackground) searchboxbackground->SetSelected(elem->GetKeyWithin());
		}
		if (pProp == Element::MouseWithinProp()) {
			DDScalableElement* searchboxbackground = regElem<DDScalableElement*>(L"searchboxbackground", pSearch);
			if (searchboxbackground) searchboxbackground->SetOverhang(elem->GetMouseWithin());
		}
		if (pProp == Element::EnabledProp()) {
			DDScalableElement* searchboxbackground = regElem<DDScalableElement*>(L"searchboxbackground", pSearch);
			if (searchboxbackground) searchboxbackground->SetEnabled(elem->GetEnabled());
		}
	}
	void CloseSearch(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			DestroySearchPage();
		}
	}

	void CreateSearchPage() {
		unsigned long key4 = 0;
		RECT dimensions;
		SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
		NativeHWNDHost::Create(L"DD_SearchHost", L"DirectDesktop Everything Search Wrapper", NULL, NULL, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, WS_EX_TOOLWINDOW, WS_POPUP, NULL, 0, &searchwnd);
		DUIXmlParser::Create(&parser4, NULL, NULL, DUI_ParserErrorCB, NULL);
		parser4->SetXMLFromResource(IDR_UIFILE5, HINST_THISCOMPONENT, HINST_THISCOMPONENT);
		HWNDElement::Create(searchwnd->GetHWND(), true, NULL, NULL, &key4, (Element**)&parent3);
		parser4->CreateElement(L"SearchUI", parent3, NULL, NULL, &pSearch);
		WndProc5 = (WNDPROC)SetWindowLongPtrW(searchwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SearchWindowProc);
		pSearch->SetVisible(true);
		pSearch->EndDefer(key4);
		searchwnd->Host(pSearch);
		BlurBackground(searchwnd->GetHWND(), true, true);
		LPWSTR sheetName = theme ? (LPWSTR)L"searchstyle" : (LPWSTR)L"searchstyledark";
		StyleSheet* sheet = pSearch->GetSheet();
		CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
		parser4->GetSheet(sheetName, &sheetStorage);
		pSearch->SetValue(Element::SheetProp, 1, sheetStorage);
		searchwnd->ShowWindow(SW_SHOW);
		searchbox = (TouchEdit2*)pSearch->FindDescendent(StrToID(L"searchbox"));
		Button* searchbutton = regElem<Button*>(L"searchbutton", pSearch);
		assignFn(searchbutton, DisplayResults);
		Button* closebutton = regElem<Button*>(L"closebutton", pSearch);
		assignFn(closebutton, CloseSearch);
		assignExtendedFn(searchbox, UpdateSearchBox);
		TouchScrollViewer* SearchResults = regElem<TouchScrollViewer*>(L"SearchResults", pSearch);
		SearchResults->SetBackgroundColor(theme ? 4293980400 : 4280821800);
	}

	void DestroySearchPage()
	{
		searchwnd->DestroyWindow();
	}
}