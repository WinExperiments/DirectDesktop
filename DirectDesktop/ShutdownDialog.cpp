#include "ShutdownDialog.h"
#include "DirectoryHelper.h"
#include "resource.h"
#include "BitmapHelper.h"
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <algorithm>

#pragma comment (lib, "uxtheme.lib")


EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

using namespace std;
using namespace DirectUI;

NativeHWNDHost* shutdownwnd;
DUIXmlParser* parser3;
HWNDElement* parent2;
Element* pShutdown;
WNDPROC WndProc4;
Button* SwitchUser, *SignOut, *SleepButton, *Hibernate, *Shutdown, *Restart, *StatusCancel;
Button* SUInner, *SOInner, *SlInner, *HiInner, *ShInner, *ReInner;
Element* StatusText;
TouchEdit2* delayseconds;

HANDLE ActionThread, TimerThread;
int savedremaining; // Display remaining time immediately when the dialog is invoked

struct DialogValues {
	int buttonID{};
	int delay{};
};

wstring GetDialogCaption(HMODULE hDLL, int id) {
	HRSRC hRes = FindResourceW(hDLL, MAKEINTRESOURCE(id), RT_DIALOG);
	wstring caption;
	if (!hRes) return L"";
	DWORD resSize = SizeofResource(hDLL, hRes);
	if (resSize < 24) return L""; // DIALOGEX header size (24)
	HGLOBAL hData = LoadResource(hDLL, hRes);
	if (!hData) return L"";
	BYTE* pData{};
	if (hData) pData = (BYTE*)LockResource(hData);
	if (!pData) return L"";
	const BYTE* pEnd = pData + resSize;
	const BYTE* pCurrent = pData;
	if (pCurrent + 24 > pEnd) return L""; // Check header
	pCurrent += 24; // DIALOGEX offset

	// Skip menu
	if (pCurrent + 2 > pEnd) return L"";
	if (*((const WORD*)pCurrent) == 0xFFFF) {
		pCurrent += 4;
		if (pCurrent > pEnd) return L"";
	}
	else {
		while (pCurrent < pEnd && *((const wchar_t*)pCurrent)) pCurrent += 2;
		pCurrent += 2;
		if (pCurrent > pEnd) return L"";
	}

	// Skip class
	if (pCurrent + 2 > pEnd) return L"";
	if (*((const WORD*)pCurrent) == 0xFFFF) {
		pCurrent += 4;
		if (pCurrent > pEnd) return L"";
	}
	else {
		while (pCurrent < pEnd && *((const wchar_t*)pCurrent)) pCurrent += 2;
		pCurrent += 2;
		if (pCurrent > pEnd) return L"";
	}

	while (pCurrent < pEnd && *((const wchar_t*)pCurrent)) {
		caption += *((const wchar_t*)pCurrent);
		pCurrent += 2;
		if (pCurrent > pEnd) return L"";
	}
	return caption;
}

void ShowNotification(wstring title, wstring content) {
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_INFO;
	wcscpy_s(nid.szInfo, content.c_str());
	wcscpy_s(nid.szInfoTitle, title.c_str());
	nid.dwInfoFlags = NIIF_INFO;
	Shell_NotifyIconW(NIM_ADD, &nid);
	Shell_NotifyIconW(NIM_DELETE, &nid);
}
wstring GetNotificationString(int id, int delay) {
	wstring action;
	switch (id) {
	case 1:
		action = LoadStrFromRes(3052, L"ShutdownUX.dll");
		break;
	case 2:
		action = LoadStrFromRes(3034, L"ShutdownUX.dll");
		break;
	case 3:
		action = LoadStrFromRes(3019, L"ShutdownUX.dll");
		break;
	case 4:
		action = LoadStrFromRes(3022, L"ShutdownUX.dll");
		break;
	case 5:
		action = LoadStrFromRes(3013, L"ShutdownUX.dll");
		break;
	case 6:
		action = LoadStrFromRes(3016, L"ShutdownUX.dll");
		break;
	default:
		action = L"0";
		break;
	}
	transform(action.begin(), action.end(), action.begin(), ::tolower);
	WCHAR* cStatus = new WCHAR[128];
	StringCchPrintfW(cStatus, 128, LoadStrFromRes(4037).c_str(), action.c_str(), delay);
	wstring result = cStatus;
	if (cStatus) delete[] cStatus;
	return result;
}

LRESULT CALLBACK ShutdownWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		return 0;
		break;
	case WM_DESTROY:
		return 0;
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) DestroyShutdownDialog();
		break;
	case WM_USER + 1: {
		if (StatusText) StatusText->SetContentString(GetNotificationString(wParam, lParam).c_str());
		break;
	}
	}
	return CallWindowProc(WndProc4, hWnd, uMsg, wParam, lParam);
}

unsigned long ShowTimerStatus(LPVOID lpParam) {
	DialogValues* dv = (DialogValues*)lpParam;
	int id = dv->buttonID;
	int remaining = dv->delay;
	while (remaining >= 0) {
		if (IsWindowVisible(shutdownwnd->GetHWND())) SendMessageW(shutdownwnd->GetHWND(), WM_USER + 1, id, remaining);
		Sleep(1000);
		remaining--;
		savedremaining = remaining;
	}
	return 0;
}

unsigned long DelayedAction(LPVOID lpParam) {
	DialogValues* dv = (DialogValues*)lpParam;
	delayedshutdownstatuses[dv->buttonID - 1] = true;
	int seconds = dv->delay;
	int id = dv->buttonID;
	TimerThread = CreateThread(0, 0, ShowTimerStatus, dv, NULL, NULL);
	Sleep(seconds * 1000);
	SendMessageW(wnd->GetHWND(), WM_USER + 19, NULL, id);
	return 0;
}

void PerformOperation(Element* elem, Event* iev) {
	if (iev->uidType == Button::Click) {
		static bool validation{};
		validation = !validation;
		if (validation) {
			Value* v;
			int pressedID{};
			if (delayseconds->GetContentString(&v) == nullptr) delayseconds->SetContentString(L"0");
			StatusText = nullptr;

			// TODO: Find a better way to stop older threads or make the bool array not global
			if (ActionThread) TerminateThread(ActionThread, 1);
			if (TimerThread) TerminateThread(TimerThread, 1);

			for (int i = 0; i < 6; i++) {
				delayedshutdownstatuses[i] = false;
			}
			if (elem == StatusCancel) {
				DestroyShutdownDialog();
				ShowNotification(LoadStrFromRes(4024), LoadStrFromRes(801, L"user32.dll"));
				return;
			}
			if (elem == SwitchUser) pressedID = 1;
			if (elem == SignOut) pressedID = 2;
			if (elem == SleepButton) pressedID = 3;
			if (elem == Hibernate) pressedID = 4;
			if (elem == Shutdown) pressedID = 5;
			if (elem == Restart) pressedID = 6;
			int delay = _wtol(delayseconds->GetContentString(&v));
			if (delay < 0) delay = 0;
			DialogValues dv{};
			dv.buttonID = pressedID;
			dv.delay = delay;
			DWORD dwAction{};
			ActionThread = CreateThread(0, 0, DelayedAction, (LPVOID)&dv, NULL, &dwAction);
			DestroyShutdownDialog();
			ShowNotification(LoadStrFromRes(4024), GetNotificationString(pressedID, delay));
		}
	}
}

void PressSync(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
	if (pProp == Button::MouseWithinProp()) {
		if (elem->GetMouseWithin() == false) ((Button*)elem)->SetPressed(false);
	}
	if (pProp == Button::PressedProp()) {
		Button* parent = (Button*)elem->GetParent();
		parent->SetPressed(((Button*)elem)->GetPressed());
	}
	if (pProp == Button::EnabledProp()) {
		Button* parent = (Button*)elem->GetParent();
		parent->SetEnabled(((Button*)elem)->GetEnabled());
	}
}

void UpdateDelaySecondsPreview(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
	if (pProp == Element::KeyWithinProp()) {
		Element* delaysecondspreview = regElem(L"delaysecondspreview", pShutdown);
		Value* v;
		delaysecondspreview->SetVisible(!elem->GetKeyWithin());
		delaysecondspreview->SetContentString(elem->GetContentString(&v));
	}
}

void DisplayShutdownDialog() {
	HMODULE hDLL = LoadLibraryExW(L"ShutdownUX.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
	wstring caption = GetDialogCaption(hDLL, 2000);
	if (hDLL) FreeLibrary(hDLL);
	HWND hWndShutdown = FindWindowW(L"NativeHWNDHost", caption.c_str());
	if (hWndShutdown) return;
	unsigned long key3 = 0;
	int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 2) * 2;
	int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CYEDGE, dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
	int sizeX = 480 * flScaleFactor + windowsThemeX;
	int sizeY = 360 * flScaleFactor + windowsThemeY;
	NativeHWNDHost::Create(caption.c_str(), NULL, LoadIconW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(108)), (GetSystemMetrics(SM_CXSCREEN) - sizeX) / 2, (GetSystemMetrics(SM_CYSCREEN) - sizeY) / 2, sizeX, sizeY, NULL, WS_POPUP | WS_BORDER, 0, &shutdownwnd);
	DUIXmlParser::Create(&parser3, NULL, NULL, NULL, NULL);
	parser3->SetXMLFromResource(IDR_UIFILE4, HINST_THISCOMPONENT, HINST_THISCOMPONENT);
	HWNDElement::Create(shutdownwnd->GetHWND(), true, NULL, NULL, &key3, (Element**)&parent2);
	ITaskbarList* pTaskbarList = nullptr;
	if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
		IID_ITaskbarList, (void**)&pTaskbarList))) {
		if (SUCCEEDED(pTaskbarList->HrInit())) {
			pTaskbarList->DeleteTab(shutdownwnd->GetHWND());
		}
		pTaskbarList->Release();
	}
	parser3->CreateElement(L"ShutDownWindows", parent2, NULL, NULL, &pShutdown);
	WndProc4 = (WNDPROC)SetWindowLongPtrW(shutdownwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)ShutdownWindowProc);
	pShutdown->SetVisible(true);
	pShutdown->EndDefer(key3);
	shutdownwnd->Host(pShutdown);
	dialogopen = true;
	DDScalableElement* FakeTitlebar = (DDScalableElement*)pShutdown->FindDescendent(StrToID(L"FakeTitlebar"));
	DDScalableElement* TitlebarText = (DDScalableElement*)pShutdown->FindDescendent(StrToID(L"TitlebarText"));
	DDScalableElement* Logo = (DDScalableElement*)pShutdown->FindDescendent(StrToID(L"Logo"));
	Element* StatusBar = regElem(L"StatusBar", pShutdown);
	Element* SeparatorLine = regElem(L"SeparatorLine", pShutdown);
	Element* ShutdownActions = regElem(L"ShutdownActions", pShutdown);
	Element* DelayBar = regElem(L"DelayBar", pShutdown);
	RichText* moon = regRichText(L"moon", pShutdown);
	RichText* stars = regRichText(L"stars", pShutdown);
	for (int i = 0; i < 6; i++) {
		if (delayedshutdownstatuses[i] == true) {
			Element* StatusBarResid{};
			parser3->CreateElement(L"StatusBar", NULL, NULL, NULL, (Element**)&StatusBarResid);
			StatusBar->Add((Element**)&StatusBarResid, 1);
			StatusText = regRichText(L"StatusText", StatusBarResid);
			StatusCancel = regBtn(L"StatusCancel", StatusBarResid);
			Button* SCInner = regBtn(L"SCInner", StatusBarResid);
			assignFn(StatusCancel, PerformOperation);
			assignExtendedFn(SCInner, PressSync);
			SetWindowPos(shutdownwnd->GetHWND(), NULL, 0, 0, sizeX, sizeY + StatusBarResid->GetHeight(), SWP_NOMOVE | SWP_NOZORDER);
			StatusText->SetContentString(GetNotificationString(i + 1, savedremaining).c_str());
			break;
		}
	}
	WCHAR* cBuffer = new WCHAR[64];
	int dpiAdjusted = (dpi * 96.0) / dpiLaunch;
	StringCchPrintfW(cBuffer, 64, LoadStrFromRes(208).c_str(), static_cast<int>((min(GetSystemMetricsForDpi(SM_CXSMICON, dpiAdjusted), GetSystemMetricsForDpi(SM_CYSMICON, dpiAdjusted))) * 0.75));
	moon->SetFont(cBuffer);
	StringCchPrintfW(cBuffer, 64, LoadStrFromRes(210).c_str(), static_cast<int>((min(GetSystemMetricsForDpi(SM_CXSMICON, dpiAdjusted), GetSystemMetricsForDpi(SM_CYSMICON, dpiAdjusted))) * 0.375));
	stars->SetFont(cBuffer);
	delete[] cBuffer;
	TitlebarText->SetContentString(caption.c_str());
	HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, 16, 16, LR_SHARED);
	HBITMAP colorBMP = IconToBitmap(dummyi);
	HBITMAP colorBMP2 = IconToBitmap(dummyi);
	COLORREF separator = theme ? RGB(0, 0, 0) : RGB(255, 255, 255);
	float bodyalpha = theme ? 0.4 : 0.05;
	IterateBitmap(colorBMP, SimpleBitmapPixelHandler, 3, separator, 0.125);
	Value* colorBMPV = DirectUI::Value::CreateGraphic(colorBMP, 7, 0xffffffff, false, false, false);
	SeparatorLine->SetValue(Element::BackgroundProp, 1, colorBMPV);
	DeleteObject(colorBMP);
	colorBMPV->Release();
	COLORREF white = RGB(255, 255, 255);
	IterateBitmap(colorBMP2, SimpleBitmapPixelHandler, 3, white, bodyalpha);
	Value* colorBMPV2 = DirectUI::Value::CreateGraphic(colorBMP2, 7, 0xffffffff, false, false, false);
	ShutdownActions->SetValue(Element::BackgroundProp, 1, colorBMPV2);
	DelayBar->SetValue(Element::BackgroundProp, 1, colorBMPV2);
	StatusBar->SetValue(Element::BackgroundProp, 1, colorBMPV2);
	DeleteObject(dummyi);
	DeleteObject(colorBMP2);
	colorBMPV2->Release();
	int WindowsBuild = _wtoi(GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber"));
	BOOL value = TRUE;
	LPWSTR sheetName = (LPWSTR)L"shutdownstyle";
	if (!theme) {
		sheetName = (LPWSTR)L"shutdownstyledark";
		DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
	}
	StyleSheet* sheet = pShutdown->GetSheet();
	Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
	parser3->GetSheet(sheetName, &sheetStorage);
	pShutdown->SetValue(Element::SheetProp, 1, sheetStorage);
	sheetStorage->Release();
	shutdownwnd->ShowWindow(SW_SHOW);
	if (WindowsBuild >= 22000) {
		MARGINS margins = { -1, -1, -1, -1 };
		DwmExtendFrameIntoClientArea(shutdownwnd->GetHWND(), &margins);
		DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_USE_HOSTBACKDROPBRUSH, &value, sizeof(value));
		DWM_SYSTEMBACKDROP_TYPE backdrop_type = DWMSBT_MAINWINDOW;
		DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
		DWORD cornerPreference = DWMWCP_ROUND;
		DwmSetWindowAttribute(shutdownwnd->GetHWND(), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
		pShutdown->SetBackgroundStdColor(7);
	}
	if (WindowsBuild < 21996) {
		Logo->SetEnableAccent(1);
		Logo->SetFirstScaledImage(theme ? 1101 : 1201);
	}
	else Logo->SetFirstScaledImage(theme ? 1108 : 1208);
	SwitchUser = regBtn(L"SwitchUser", pShutdown), SignOut = regBtn(L"SignOut", pShutdown), SleepButton = regBtn(L"SleepButton", pShutdown),
	Hibernate = regBtn(L"Hibernate", pShutdown), Shutdown = regBtn(L"Shutdown", pShutdown), Restart = regBtn(L"Restart", pShutdown);
	SUInner = regBtn(L"SUInner", pShutdown), SOInner = regBtn(L"SOInner", pShutdown), SlInner = regBtn(L"SlInner", pShutdown),
	HiInner = regBtn(L"HiInner", pShutdown), ShInner = regBtn(L"ShInner", pShutdown), ReInner = regBtn(L"ReInner", pShutdown);
	Button* buttons[6] = { SwitchUser, SignOut, SleepButton, Hibernate, Shutdown, Restart };
	Button* innerbuttons[6] = { SUInner, SOInner, SlInner, HiInner, ShInner, ReInner };
	delayseconds = (TouchEdit2*)pShutdown->FindDescendent(StrToID(L"delayseconds"));
	for (auto btn : buttons) {
		assignFn(btn, PerformOperation);
	}
	for (auto btn : innerbuttons) {
		assignExtendedFn(btn, PressSync);
	}
	assignExtendedFn(delayseconds, UpdateDelaySecondsPreview);
}
void DestroyShutdownDialog() {
	shutdownwnd->DestroyWindow();
	dialogopen = false;
}