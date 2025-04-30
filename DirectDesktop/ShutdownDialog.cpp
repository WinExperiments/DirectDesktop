#include "ShutdownDialog.h"
#include "DirectoryHelper.h"
#include "resource.h"
#include "BitmapHelper.h"
#include "DirectDesktop.h"
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <algorithm>

#pragma comment (lib, "uxtheme.lib")

using namespace std;
using namespace DirectUI;

NativeHWNDHost* shutdownwnd;
DUIXmlParser* parser3;
HWNDElement* parent2;
Element* pShutdown;
WNDPROC WndProc3;
Button* SwitchUser, *SignOut, *SleepButton, *Hibernate, *Shutdown, *Restart, *StatusCancel;
Button* SUInner, *SOInner, *SlInner, *HiInner, *ShInner, *ReInner;
Element* StatusBarResid;
Element* StatusText;
Element* AdvancedOptions;
DDScalableButton* RestartWinRE, *RestartBIOS;
TouchEdit2* delayseconds;

HANDLE ActionThread, TimerThread;
int savedremaining; // Display remaining time immediately when the dialog is invoked
wstring reasonStr = LoadStrFromRes(8261, L"user32.dll");

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
		DestroyShutdownDialog();
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
	return CallWindowProc(WndProc3, hWnd, uMsg, wParam, lParam);
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
	if (seconds > 0) TimerThread = CreateThread(0, 0, ShowTimerStatus, dv, NULL, NULL);
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
				ShowNotification(LoadStrFromRes(4024), LoadStrFromRes(4039));
				return;
			}
			if (elem == SwitchUser) pressedID = 1;
			if (elem == SignOut) pressedID = 2;
			if (elem == SleepButton) pressedID = 3;
			if (elem == Hibernate) pressedID = 4;
			if (elem == Shutdown) pressedID = 5;
			if (elem == Restart) pressedID = 6;
			int delay = (AdvancedOptions->GetLayoutPos() == -3) ? 0 : _wtoi(delayseconds->GetContentString(&v));
			DialogValues* dv = new DialogValues{ pressedID, delay };
			DWORD dwAction{};
			ActionThread = CreateThread(0, 0, DelayedAction, dv, NULL, &dwAction);
			DestroyShutdownDialog();
			if (delay > 0) ShowNotification(LoadStrFromRes(4024), GetNotificationString(pressedID, delay));
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

void ToggleAdvancedOptions(Element* elem, Event* iev) {
	static int expanded{};
	if (iev->uidType == Button::Click) {
		expanded++;
		short pos = expanded & 2 ? 3 : -3;
		bool arrowselection = expanded & 2 ? true : false;
		AdvancedOptions->SetLayoutPos(pos);
		RichText* AdvancedOptionsArrow = regElem<RichText*>(L"AdvancedOptionsArrow", elem);
		AdvancedOptionsArrow->SetSelected(arrowselection);
		int ShutdownReasonUI = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability", L"ShutdownReasonUI");
		int sizeX = 500 * flScaleFactor;
		int sizeY = 400 * flScaleFactor;
		if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2) sizeY += 120 * flScaleFactor;
		for (int i = 0; i < 6; i++) {
			if (delayedshutdownstatuses[i] == true) {
				sizeY += StatusBarResid->GetHeight();
				break;
			}
		}
		if (expanded & 2) sizeY += AdvancedOptions->GetHeight();
		SetWindowPos(shutdownwnd->GetHWND(), NULL, 0, 0, sizeX, sizeY, SWP_NOMOVE | SWP_NOZORDER);
	}
}

void ToggleDelayOption(Element* elem, Event* iev) {
	if (iev->uidType == TouchButton::Click) {
		Element* delaysecondspreview = regElem<Element*>(L"delaysecondspreview", pShutdown);
		CheckedStateFlags newChecked = (((TouchCheckBox*)elem)->GetCheckedState() == CSF_Unchecked) ? CSF_Checked : CSF_Unchecked;
		((TouchCheckBox*)elem)->SetCheckedState(newChecked);
		delayseconds->SetEnabled((newChecked == CSF_Checked) ? true : false);
		delaysecondspreview->SetVisible((newChecked == CSF_Checked) ? true : false);
		if (newChecked == CSF_Unchecked) {
			delayseconds->SetContentString(L"0");
			delaysecondspreview->SetContentString(L"0");
		}
	}
}


void AdvancedShutdown(Element* elem, Event* iev) {
	if (iev->uidType == Button::Click) {
		if (elem == RestartWinRE) {
			WinExec("reagentc /boottore", SW_HIDE);
			WinExec("shutdown /r /f /t 0", SW_HIDE);
		}
		if (elem == RestartBIOS) {
			WinExec("shutdown /r /fw", SW_HIDE);
		}
	}
}

void UpdateDelaySecondsPreview(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
	if (pProp == Element::KeyWithinProp()) {
		Element* delaysecondspreview = regElem<Element*>(L"delaysecondspreview", pShutdown);
		Value* v;
		delaysecondspreview->SetVisible(!elem->GetKeyWithin());
		delaysecondspreview->SetContentString(elem->GetContentString(&v));
	}
}

void UpdateShutdownReasonCode(Element* elem, Event* iev) {
	if (iev->uidType == Combobox::SelectionChange()) {
		switch (((Combobox*)elem)->GetSelection()) {
		case 0:
			shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER;
			reasonStr = LoadStrFromRes(8261, L"user32.dll");
			break;
		case 1:
			shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8262, L"user32.dll");
			break;
		case 2:
			shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE;
			reasonStr = LoadStrFromRes(8250, L"user32.dll");
			break;
		case 3:
			shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8251, L"user32.dll");
			break;
		case 4:
			shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION;
			reasonStr = LoadStrFromRes(8252, L"user32.dll");
			break;
		case 5:
			shutdownReason = SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8253, L"user32.dll");
			break;
		case 6:
			shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SYSTEMRESTORE;
			reasonStr = LoadStrFromRes(8272, L"user32.dll");
			break;
		case 7:
			shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SYSTEMRESTORE | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8271, L"user32.dll");
			break;
		case 8:
			shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG;
			reasonStr = LoadStrFromRes(8256, L"user32.dll");
			break;
		case 9:
			shutdownReason = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8257, L"user32.dll");
			break;
		case 10:
			shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE;
			reasonStr = LoadStrFromRes(8260, L"user32.dll");
			break;
		case 11:
			shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8268, L"user32.dll");
			break;
		case 12:
			shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8293, L"user32.dll");
			break;
		case 13:
			shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_HUNG;
			reasonStr = LoadStrFromRes(8258, L"user32.dll");
			break;
		case 14:
			shutdownReason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_UNSTABLE;
			reasonStr = LoadStrFromRes(8259, L"user32.dll");
			break;
		case 15:
			shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY;
			reasonStr = LoadStrFromRes(8299, L"user32.dll");
			break;
		case 16:
			shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY | SHTDN_REASON_FLAG_PLANNED;
			reasonStr = LoadStrFromRes(8300, L"user32.dll");
			break;
		case 17:
			shutdownReason = SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_NETWORK_CONNECTIVITY;
			reasonStr = LoadStrFromRes(8301, L"user32.dll");
			break;
		}
	}
}

void DisplayShutdownDialog() {
	HMODULE hDLL = LoadLibraryExW(L"ShutdownUX.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
	wstring caption = GetDialogCaption(hDLL, 2000);
	if (hDLL) FreeLibrary(hDLL);
	HWND hWndShutdown = FindWindowW(L"DD_ShutdownHost", caption.c_str());
	if (hWndShutdown) return;
	unsigned long key3 = 0;
	int ShutdownReasonUI = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability", L"ShutdownReasonUI");
	int sizeX = 500 * flScaleFactor;
	int sizeY = 400 * flScaleFactor;
	if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2) sizeY += 120 * flScaleFactor;
	RECT dimensions;
	SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
	NativeHWNDHost::Create(L"DD_ShutdownHost", caption.c_str(), NULL, NULL, (dimensions.left + dimensions.right - sizeX) / 2, (dimensions.bottom - sizeY) / 3 + dimensions.top / 1.33, sizeX, sizeY, NULL, WS_POPUP | WS_BORDER, NULL, 0, &shutdownwnd);
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
	WndProc3 = (WNDPROC)SetWindowLongPtrW(shutdownwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)ShutdownWindowProc);
	pShutdown->SetVisible(true);
	pShutdown->EndDefer(key3);
	shutdownwnd->Host(pShutdown);
	dialogopen = true;
	DDScalableElement* FakeTitlebar = regElem<DDScalableElement*>(L"FakeTitlebar", pShutdown);
	DDScalableElement* TitlebarText = regElem<DDScalableElement*>(L"TitlebarText", pShutdown);
	DDScalableElement* Logo = regElem<DDScalableElement*>(L"Logo", pShutdown);
	Element* StatusBar = regElem<Element*>(L"StatusBar", pShutdown);
	Element* SeparatorLine = regElem<Element*>(L"SeparatorLine", pShutdown);
	Element* SeparatorLine2 = regElem<Element*>(L"SeparatorLine2", pShutdown);
	Element* ShutdownActions = regElem<Element*>(L"ShutdownActions", pShutdown);
	Element* ShutdownEventTracker = regElem<Element*>(L"ShutdownEventTracker", pShutdown);
	Element* AdvancedOptionsBar = regElem<Element*>(L"AdvancedOptionsBar", pShutdown);
	RestartWinRE = regElem<DDScalableButton*>(L"RestartWinRE", pShutdown);
	RestartBIOS = regElem<DDScalableButton*>(L"RestartBIOS", pShutdown);
	FIRMWARE_TYPE firmwareType{};
	GetFirmwareType(&firmwareType);
	if (firmwareType == FirmwareTypeUefi) RestartBIOS->SetLayoutPos(4);
	AdvancedOptions = regElem<Element*>(L"AdvancedOptions", pShutdown);
	RichText* moon = regElem<RichText*>(L"moon", pShutdown);
	RichText* stars = regElem<RichText*>(L"stars", pShutdown);
	for (int i = 0; i < 6; i++) {
		if (delayedshutdownstatuses[i] == true) {
			parser3->CreateElement(L"StatusBar", NULL, NULL, NULL, (Element**)&StatusBarResid);
			StatusBar->Add((Element**)&StatusBarResid, 1);
			StatusText = regElem<DDScalableElement*>(L"StatusText", StatusBarResid);
			StatusCancel = regElem<Button*>(L"StatusCancel", StatusBarResid);
			Button* SCInner = regElem<Button*>(L"SCInner", StatusBarResid);
			assignFn(StatusCancel, PerformOperation);
			assignExtendedFn(SCInner, PressSync);
			sizeY += StatusBarResid->GetHeight();
			SetWindowPos(shutdownwnd->GetHWND(), NULL, 0, 0, sizeX, sizeY, SWP_NOMOVE | SWP_NOZORDER);
			StatusText->SetContentString(GetNotificationString(i + 1, savedremaining).c_str());
			if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2) {
				DDScalableElement* ReasonText = regElem<DDScalableElement*>(L"ReasonText", StatusBarResid);
				ReasonText->SetLayoutPos(4);
				WCHAR* cReason = new WCHAR[128];
				StringCchPrintfW(cReason, 128, LoadStrFromRes(4038).c_str(), reasonStr.c_str());
				ReasonText->SetContentString(cReason);
				if (cReason) delete[] cReason;
			}
			break;
		}
	}
	if (ShutdownReasonUI == 1 || ShutdownReasonUI == 2) {
		shutdownReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER;
		Element* ShutdownEventTrackerResid{};
		parser3->CreateElement(L"ShutdownEventTracker", NULL, NULL, NULL, (Element**)&ShutdownEventTrackerResid);
		ShutdownEventTracker->Add((Element**)&ShutdownEventTrackerResid, 1);
		DDScalableElement* SETText = regElem<DDScalableElement*>(L"SETText", ShutdownEventTrackerResid);
		HMODULE hDLL2 = LoadLibraryExW(L"shutdownext.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
		SETText->SetContentString(GetDialogCaption(hDLL2, 2210).c_str());
		if (hDLL2) FreeLibrary(hDLL2);
		Combobox* SETReason = (Combobox*)ShutdownEventTracker->FindDescendent(StrToID(L"SETReason"));
		for (short s = 8261; s <= 8262; s++) SETReason->AddString(LoadStrFromRes(s, L"user32.dll").c_str());
		for (short s = 8250; s <= 8253; s++) SETReason->AddString(LoadStrFromRes(s, L"user32.dll").c_str());
		for (short s = 8272; s >= 8271; s--) SETReason->AddString(LoadStrFromRes(s, L"user32.dll").c_str());
		for (short s = 8256; s <= 8257; s++) SETReason->AddString(LoadStrFromRes(s, L"user32.dll").c_str());
		SETReason->AddString(LoadStrFromRes(8260, L"user32.dll").c_str());
		SETReason->AddString(LoadStrFromRes(8268, L"user32.dll").c_str());
		SETReason->AddString(LoadStrFromRes(8293, L"user32.dll").c_str());
		for (short s = 8258; s <= 8259; s++) SETReason->AddString(LoadStrFromRes(s, L"user32.dll").c_str());
		for (short s = 8299; s <= 8301; s++) SETReason->AddString(LoadStrFromRes(s, L"user32.dll").c_str());
		SETReason->SetSelection(0);
		HWND hCtrl = FindWindowExW(parent2->GetHWND(), NULL, L"CtrlNotifySink", NULL);
		HWND hCombobox = FindWindowExW(hCtrl, NULL, L"ComboBox", NULL);
		SetWindowLongPtrW(hCombobox, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP);
		SetLayeredWindowAttributes(hCombobox, 0, 255, LWA_ALPHA);
		if (!theme) SetWindowTheme(hCtrl, L"DarkMode_CFD", NULL);
		assignFn(SETReason, UpdateShutdownReasonCode);
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
	SeparatorLine2->SetValue(Element::BackgroundProp, 1, colorBMPV);
	DeleteObject(colorBMP);
	colorBMPV->Release();
	COLORREF white = RGB(255, 255, 255);
	IterateBitmap(colorBMP2, SimpleBitmapPixelHandler, 3, white, bodyalpha);
	Value* colorBMPV2 = DirectUI::Value::CreateGraphic(colorBMP2, 7, 0xffffffff, false, false, false);
	ShutdownActions->SetValue(Element::BackgroundProp, 1, colorBMPV2);
	AdvancedOptions->SetValue(Element::BackgroundProp, 1, colorBMPV2);
	StatusBar->SetValue(Element::BackgroundProp, 1, colorBMPV2);
	ShutdownEventTracker->SetValue(Element::BackgroundProp, 1, colorBMPV2);
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
	//AnimateWindow(shutdownwnd->GetHWND(), 180, AW_BLEND);
	shutdownwnd->ShowWindow(SW_SHOW);
	//SetFocus(shutdownwnd->GetHWND());
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
	SwitchUser = regElem<Button*>(L"SwitchUser", pShutdown), SignOut = regElem<Button*>(L"SignOut", pShutdown), SleepButton = regElem<Button*>(L"SleepButton", pShutdown),
	Hibernate = regElem<Button*>(L"Hibernate", pShutdown), Shutdown = regElem<Button*>(L"Shutdown", pShutdown), Restart = regElem<Button*>(L"Restart", pShutdown);
	SUInner = regElem<Button*>(L"SUInner", pShutdown), SOInner = regElem<Button*>(L"SOInner", pShutdown), SlInner = regElem<Button*>(L"SlInner", pShutdown),
	HiInner = regElem<Button*>(L"HiInner", pShutdown), ShInner = regElem<Button*>(L"ShInner", pShutdown), ReInner = regElem<Button*>(L"ReInner", pShutdown);
	Button* buttons[6] = { SwitchUser, SignOut, SleepButton, Hibernate, Shutdown, Restart };
	Button* innerbuttons[6] = { SUInner, SOInner, SlInner, HiInner, ShInner, ReInner };
	TouchCheckBox* delaytoggle = (TouchCheckBox*)pShutdown->FindDescendent(StrToID(L"delaytoggle"));
	delayseconds = (TouchEdit2*)pShutdown->FindDescendent(StrToID(L"delayseconds"));
	Element* delaysecondspreview = regElem<Element*>(L"delaysecondspreview", pShutdown);
	delayseconds->SetContentString(L"0");
	delaysecondspreview->SetContentString(L"0");
	for (auto btn : buttons) {
		assignFn(btn, PerformOperation);
	}
	for (auto btn : innerbuttons) {
		assignExtendedFn(btn, PressSync);
	}
	assignFn(AdvancedOptionsBar, ToggleAdvancedOptions);
	assignFn(delaytoggle, ToggleDelayOption);
	assignExtendedFn(delayseconds, UpdateDelaySecondsPreview);
	assignFn(RestartWinRE, AdvancedShutdown);
	assignFn(RestartBIOS, AdvancedShutdown);
}
void DestroyShutdownDialog() {
	//AnimateWindow(shutdownwnd->GetHWND(), 120, AW_BLEND | AW_HIDE);
	shutdownwnd->DestroyWindow();
	dialogopen = false;
}