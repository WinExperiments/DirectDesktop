#include "framework.h"
#include "Include\dui70\DirectUI\DirectUI.h"
#include "Include\dui70\DUser\DUser.h"
#include "DirectDesktop.h"
#include "resource.h"
#include <propkey.h>
#include "strsafe.h"
#include <cmath>
#include <vector>
#include <list>
#include <regex>
#include <WinUser.h>
#include <ShObjIdl.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <wtsapi32.h>
#include <powrprof.h>
#include "ColorHelper.h"
#include "StyleModifier.h"
#include "BitmapHelper.h"
#include "DirectoryHelper.h"
#include "SettingsHelper.h"
#include "ContextMenus.h"
#include "ShutdownDialog.h"
#include "RenameCore.h"
#include "SearchPage.h"
#include "EditMode.h"

using namespace DirectUI;
using namespace std;

namespace DirectDesktop
{
	NativeHWNDHost* wnd, * subviewwnd;
	HWNDElement* parent, * subviewparent;
	DUIXmlParser* parser, * parser2;
	Element* pMain, * pSubview;
	unsigned long key = 0, key2 = 0;

	Element* sampleText;
	Element* mainContainer;
	DDScalableButton* fullscreeninner;
	Element* popupcontainer;
	Button* fullscreenpopupbase, * centered;
	Button* emptyspace;
	LVItem* g_outerElem;
	Element* selector, * selector2;
	Element* dirnameanimator;
	Element* tasksanimator;
	DDScalableButton* PageTab1, * PageTab2, * PageTab3;
	DDScalableButton* SubUIContainer;
	TouchButton* prevpageMain, * nextpageMain;
	Element* dragpreview;

	DDScalableElement* RegistryListener;
	void* tempElem;
	wstring path1, path2, path3;

	HRESULT err;
	HWND hWorkerW = NULL;
	HWND hSHELLDLL_DefView = NULL;
	HWND hWndTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
	Logger MainLogger;

	DWORD shutdownReason = SHTDN_REASON_UNKNOWN;
	int maxPageID = 1, currentPageID = 1, homePageID = 1;
	int popupframe, dframe, tframe;
	int localeType{};
	int touchSizeX, touchSizeY;
	unsigned short defWidth, defHeight, lastWidth, lastHeight;

	wstring LoadStrFromRes(UINT id) {
		WCHAR* loadedStrBuffer = new WCHAR[512]{};
		LoadStringW((HINSTANCE)HINST_THISCOMPONENT, id, loadedStrBuffer, 512);
		wstring loadedStr = loadedStrBuffer;
		delete[] loadedStrBuffer;
		return loadedStr;
	}
	wstring LoadStrFromRes(UINT id, LPCWSTR dllName) {
		WCHAR* loadedStrBuffer = new WCHAR[512]{};
		HINSTANCE hInst = (HINSTANCE)LoadLibraryExW(dllName, NULL, LOAD_LIBRARY_AS_DATAFILE);
		LoadStringW(hInst, id, loadedStrBuffer, 512);
		wstring loadedStr = loadedStrBuffer;
		delete[] loadedStrBuffer;
		return loadedStr;
	}

	wstring RemoveQuotes(const wstring& input) {
		if (input.size() >= 2 && input.front() == L'\"' && input.back() == L'\"') {
			return input.substr(1, input.size() - 2);
		}
		return input;
	}

	int dpi = 96, dpiOld = 1, dpiLaunch{};
	int listviewAnimStorage{};
	float flScaleFactor = 1.0;
	bool isDpiPreviouslyChanged;
	bool isDefaultRes() {
		int w = (int)(lastWidth / flScaleFactor);
		int h = (int)(lastHeight / flScaleFactor);
		return (w <= defWidth + 10 && w >= defWidth - 10 && h <= defHeight + 10 && h >= defHeight - 10);
	}
	void InitialUpdateScale() {
		HDC screen = GetDC(0);
		dpi = GetDeviceCaps(screen, LOGPIXELSX);
		ReleaseDC(0, screen);
		flScaleFactor = dpi / 96.0;
		dpiLaunch = dpi;
		touchSizeX *= flScaleFactor;
		touchSizeY *= flScaleFactor;
	}

	void UpdateScale() {
		HWND hWnd = subviewwnd->GetHWND();
		dpiOld = dpi;
		dpi = GetDpiForWindow(hWnd);
		isDpiPreviouslyChanged = true;
		flScaleFactor = dpi / 96.0;
		//DDScalableElement::RedrawImages();
		//DDScalableButton::RedrawImages();
		//DDScalableElement::RedrawFonts();
		//DDScalableButton::RedrawFonts();
		touchSizeX *= static_cast<float>(dpi) / dpiOld;
		touchSizeY *= static_cast<float>(dpi) / dpiOld;
	}

	int GetCurrentScaleInterval() {
		if (dpi >= 384) return 6;
		if (dpi >= 288) return 5;
		if (dpi >= 240) return 4;
		if (dpi >= 192) return 3;
		if (dpi >= 144) return 2;
		if (dpi >= 120) return 1;
		return 0;
	}

	struct EventListener : public IElementListener {

		void (*f)(Element*, Event*);

		EventListener(void (*func)(Element*, Event*)) {
			f = func;
		}

		void OnListenerAttach(Element* elem) override {}
		void OnListenerDetach(Element* elem) override {}
		bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override {
			return true;
		}
		void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2) override {

		}
		void OnListenedEvent(Element* elem, struct Event* iev) override {
			f(elem, iev);
		}
		void OnListenedInput(Element* elem, struct InputEvent* ev) override {
		}
	};
	struct EventListener2 : public IElementListener {

		void (*f)(Element*, const PropertyInfo*, int, Value*, Value*);

		EventListener2(void (*func)(Element*, const PropertyInfo*, int, Value*, Value*)) {
			f = func;
		}

		void OnListenerAttach(Element* elem) override {}
		void OnListenerDetach(Element* elem) override {}
		bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override {
			return true;
		}
		void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2) override {
			f(elem, prop, type, v1, v2);
		}
		void OnListenedEvent(Element* elem, struct Event* iev) override {

		}
		void OnListenedInput(Element* elem, struct InputEvent* ev) override {
		}
	};

	template <typename elemType>
	elemType regElem(const wchar_t* elemName, Element* peParent) {
		elemType result = (elemType)peParent->FindDescendent(StrToID(elemName));
		return result;
	}
	EventListener* assignFn(Element* elemName, void(*fnName)(Element* elem, Event* iev), bool fReturn) {
		EventListener* pel = new EventListener(fnName);
		elemName->AddListener(pel);
		if (fReturn) return pel;
		return nullptr;
	}
	EventListener2* assignExtendedFn(Element* elemName, void(*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2), bool fReturn) {
		EventListener2* pel = new EventListener2(fnName);
		elemName->AddListener(pel);
		if (fReturn) return pel;
		return nullptr;
	}

	struct FileInfo {
		wstring filepath;
		wstring filename;
	};

	struct ThumbnailIcon {
		int x{};
		int y{};
		ThumbIcons str;
		HBITMAP icon;
	};

	double px[80]{};
	double py[80]{};
	int origX{}, origY{}, globaliconsz, globalshiconsz, globalgpiconsz;
	int emptyclicks = 1;
	bool touchmode{};
	bool renameactive{};
	bool delayedshutdownstatuses[6] = { false, false, false, false, false, false };

	void CubicBezier(const int frames, double px[], double py[], double x0, double y0, double x1, double y1) {
		for (int c = 0; c < frames; c++) {
			double t = (1.0 / frames) * c;
			px[c] = (3 * t * pow(1 - t, 2) * x0) + (3 * pow(t, 2) * (1 - t) * x1) + pow(t, 3);
			py[c] = (3 * t * pow(1 - t, 2) * y0) + (3 * pow(t, 2) * (1 - t) * y1) + pow(t, 3);
		}
		px[frames - 1] = 1;
		py[frames - 1] = 1;
	}

	void SetTheme() {
		StyleSheet* sheet = pMain->GetSheet();
		Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
		parser->GetSheet(theme ? L"default" : L"defaultdark", &sheetStorage);
		pMain->SetValue(Element::SheetProp, 1, sheetStorage);
		sheetStorage->Release();
		StyleSheet* sheet2 = pSubview->GetSheet();
		Value* sheetStorage2 = DirectUI::Value::CreateStyleSheet(sheet2);
		parser2->GetSheet(theme ? L"popup" : L"popupdark", &sheetStorage2);
		pSubview->SetValue(Element::SheetProp, 1, sheetStorage2);
		sheetStorage2->Release();
	}

	WNDPROC WndProc, WndProc2;
	HANDLE hMutex;
	constexpr LPCWSTR szWindowClass = L"DIRECTDESKTOP";
	BYTE* shellstate;
	vector<LVItem*> pm;
	vector<Element*> shortpm;
	vector<DDScalableElement*> iconpm;
	vector<Element*> shadowpm;
	vector<RichText*> filepm;
	vector<RichText*> fileshadowpm;
	vector<Element*> cbpm;
	vector<LVItem*> selectedLVItems;
	bool checkifelemexists = 0;
	bool issubviewopen = 0;
	bool issettingsopen = 0;
	bool hiddenIcons;
	bool editmode = 0;
	bool pendingaction = 0;
	bool invokedpagechange = 0;
	bool delayGroupsForDpi = 0;
	bool ensureNoRefresh = 0;
	void fullscreenAnimation(int width, int height, float animstartscale);
	void HidePopupCore(bool WinDInvoked);
	void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
	void ApplyIcons(vector<LVItem*> pmLVItem, vector<DDScalableElement*> pmIcon, DesktopIcon* di, bool subdirectory, int id, float scale);
	void IconThumbHelper(int id);
	unsigned long CreateIndividualThumbnail(LPVOID lpParam);
	unsigned long InitDesktopGroup(LPVOID lpParam);
	void ShowDirAsGroupDesktop(LVItem* lvi);
	void SelectItem(Element* elem, Event* iev);
	void SelectItem2(Element* elem, Event* iev);
	void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
	void ItemDragListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
	void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);

	HBITMAP GetShellItemImage(LPCWSTR filePath, int width, int height) {

		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (FAILED(hr)) return nullptr;

		IShellItem2* pShellItem{};
		hr = SHCreateItemFromParsingName(filePath, NULL, IID_PPV_ARGS(&pShellItem));

		HBITMAP hBitmap{};
		if (pShellItem != nullptr) {
			IShellItemImageFactory* pImageFactory{};
			hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
			pShellItem->Release();
			if (SUCCEEDED(hr)) {
				SIZE size = { width * flScaleFactor, height * flScaleFactor };
				if (pImageFactory) {
					hr = pImageFactory->GetImage(size, SIIGBF_RESIZETOFIT, &hBitmap);
					pImageFactory->Release();
				}
			}
			else {
				HICON fallback = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, width * flScaleFactor, height * flScaleFactor, LR_SHARED);
				hBitmap = IconToBitmap(fallback, width * flScaleFactor, height * flScaleFactor);
				DestroyIcon(fallback);
			}
		}
		else {
			HICON fallback = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, width * flScaleFactor, height * flScaleFactor, LR_SHARED);
			hBitmap = IconToBitmap(fallback, width * flScaleFactor, height * flScaleFactor);
			DestroyIcon(fallback);
		}

		return hBitmap;
	}

	void GetFontHeight() {
		LOGFONTW lf{};
		RECT rc = { 0, 0, 100, 100 };
		HDC hdcBuffer = CreateCompatibleDC(NULL);
		SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, dpi);
		DrawTextW(hdcBuffer, L" ", -1, &rc, DT_CENTER);
		GetTextMetricsW(hdcBuffer, &textm);
		DeleteDC(hdcBuffer);
	}
	float CalcTextLines(const wchar_t* str, int width) {
		HDC hdcBuffer = CreateCompatibleDC(NULL);
		LOGFONTW lf{};
		SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, dpi);
		if (touchmode) lf.lfHeight *= 1.25;
		HFONT hFont = CreateFontIndirectW(&lf);
		HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
		RECT rc = { 0, 0, width - 4, textm.tmHeight };
		wchar_t filenameBuffer[260]{};
		int lines_b1 = 1;
		int tilelines = 1;
		while (lines_b1 != 0) {
			rc.bottom = textm.tmHeight * tilelines;
			tilelines++;
			wcscpy_s(filenameBuffer, str);
			DWORD direction = (localeType == 1) ? DT_RIGHT : DT_LEFT;
			DWORD alignment = touchmode ? direction | DT_WORD_ELLIPSIS : DT_CENTER;
			DrawTextExW(hdcBuffer, filenameBuffer, -1, &rc, alignment | DT_MODIFYSTRING | DT_END_ELLIPSIS | DT_LVICON, NULL);
			lines_b1 = wcscmp(str, filenameBuffer);
			if (!touchmode || tilelines > 5) break;
		}
		if (touchmode) {
			DeleteObject(hFont);
			DeleteObject(hOldFont);
			DeleteDC(hdcBuffer);
			return tilelines;
		}
		RECT rc2 = { 0, 0, width - 4, textm.tmHeight * 2 };
		wchar_t filenameBuffer2[260]{};
		wcscpy_s(filenameBuffer2, str);
		DrawTextExW(hdcBuffer, filenameBuffer2, -1, &rc2, DT_MODIFYSTRING | DT_WORD_ELLIPSIS | DT_CENTER | DT_LVICON, NULL);
		int lines_b2 = wcscmp(str, filenameBuffer2);
		DeleteObject(hFont);
		DeleteObject(hOldFont);
		DeleteDC(hdcBuffer);
		if (lines_b1 == 1 && lines_b2 == 0) return 2.0; else if (lines_b1 == 1 && lines_b2 == 1) return 1.5; else return 1;
	}
	void CalcDesktopIconInfo(yValue* yV, int* lines_basedOnEllipsis, DWORD* alignment, bool subdirectory, vector<LVItem*>* pmLVItem, vector<RichText*>* pmFile) {
		vector<RichText*>* pmFileShadow = &fileshadowpm;
		*alignment = DT_CENTER | DT_END_ELLIPSIS;
		if (!touchmode) {
			*lines_basedOnEllipsis = floor(CalcTextLines((*pmLVItem)[yV->y]->GetSimpleFilename().c_str(), yV->innerSizeX - 4 * flScaleFactor)) * textm.tmHeight;
		}
		if (touchmode) {
			DWORD direction = (localeType == 1) ? DT_RIGHT : DT_LEFT;
			*alignment = direction | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS;
			int maxlines_basedOnEllipsis = (*pmFile)[yV->y]->GetHeight();
			yV->innerSizeX = (*pmFile)[yV->y]->GetWidth();
			*lines_basedOnEllipsis = CalcTextLines((*pmLVItem)[yV->y]->GetSimpleFilename().c_str(), yV->innerSizeX) * textm.tmHeight;
			if (*lines_basedOnEllipsis > maxlines_basedOnEllipsis) *lines_basedOnEllipsis = maxlines_basedOnEllipsis;
		}
	}

	void PlaySimpleViewAnimation(Element* elem, int width, int height, int animation, float startscale) {
		elem->SetAnimation(NULL);
		elem->SetWidth(width * startscale);
		elem->SetHeight(height * startscale);
		elem->SetAnimation(animation);
		elem->SetWidth(width);
		elem->SetHeight(height);
	}

	unsigned long EndExplorer(LPVOID lpParam) {
		Sleep(250);
		HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
		DWORD pid{};
		GetWindowThreadProcessId(hWndProgman, &pid);
		HANDLE hExplorer;
		hExplorer = OpenProcess(PROCESS_TERMINATE, false, pid);
		TerminateProcess(hExplorer, 2);
		CloseHandle(hExplorer);
		return 0;
	}

	void AdjustWindowSizes(bool firsttime) {
		RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		POINT topLeftMon = GetTopLeftMonitor();
		UINT swpFlags = SWP_NOZORDER;
		SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
		if (firsttime) swpFlags |= SWP_NOMOVE | SWP_NOSIZE;
		if (localeType == 1) {
			int rightMon = GetRightMonitor();
			topLeftMon.x = dimensions.right + dimensions.left - rightMon;
		}
		SetWindowPos(wnd->GetHWND(), NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
		SetWindowPos(subviewwnd->GetHWND(), NULL, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
		if (editwnd) {
			SetWindowPos(editwnd->GetHWND(), NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
			//SetWindowPos(editbgwnd->GetHWND(), NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
		}
		SetWindowPos(hWorkerW, NULL, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
		SetWindowPos(hSHELLDLL_DefView, NULL, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
		UIContainer->SetWidth(dimensions.right - dimensions.left);
		UIContainer->SetHeight(dimensions.bottom - dimensions.top);
		SetWindowPos(hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	unsigned long WallpaperHelper24H2(LPVOID lpParam) {
		yValue* yV = (yValue*)lpParam;
		Sleep(yV->innerSizeX);
		HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
		hSHELLDLL_DefView = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
		bool bTest = PlaceDesktopInPos(&(yV->y), &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, false);
		SetWindowLongPtrW(hWorkerW, GWL_STYLE, 0x96000000L);
		SetWindowLongPtrW(hWorkerW, GWL_EXSTYLE, 0x20000880L);
		free(yV);
		return 0;
	}

	void Perform24H2Fixes(bool full) {
		int WindowsBuild = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\ShellCommon", L"BuildNumber");
		if (WindowsBuild >= 26002) {
			HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
			SetParent(hSHELLDLL_DefView, hWndProgman);
			if (full) {
				yValue* yV = new yValue{ WindowsBuild, 200, NULL };
				DWORD dwWallpaper{};
				HANDLE wallpaperThread = CreateThread(0, 0, WallpaperHelper24H2, (LPVOID)yV, 0, &dwWallpaper);
			}
		}
	}

	LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
		int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor - textm.tmHeight;
		int iconPaddingX = (GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) - 48 * flScaleFactor) / 2;
		int iconPaddingY = (GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) - 48 * flScaleFactor) / 2;
		int i = 20002;
		switch (uMsg) {
		case WM_SETTINGCHANGE: {
			if (wParam == SPI_SETWORKAREA) {
				if (isDefaultRes()) SetPos(true);
				lastWidth = 0, lastHeight = 0;
				AdjustWindowSizes(false);
				RearrangeIcons(true, false, true);
			}
			if (wParam == SPI_SETDESKWALLPAPER) {
				static int messagemitigation{};
				messagemitigation++;
				if (messagemitigation & 1) {
					Perform24H2Fixes(true);
				}
			}
			if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
				UpdateModeInfo();
				// This message is sent 4-5 times upon changing accent color so this mitigation is applied
				// 0.4.5.2 test case: seems to be sent 3-4 times. Maybe dependent on Windows install?
				static int messagemitigation{};
				messagemitigation++;
				SetTheme();
				SetPos(false);
				GetPos2(false);
				delayGroupsForDpi = true;
				//DDScalableElement::RedrawImages();
				//DDScalableButton::RedrawImages();
				RearrangeIcons(false, true, true);
				if (automaticDark) {
					isDarkIconsEnabled = !theme;
				}
				if (messagemitigation % 4 == 2) { // was originally 5
					if (isColorized) RearrangeIcons(false, true, true);
				}
			}
			break;
		}
		case WM_WINDOWPOSCHANGING: {
			((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
			return 0L;
			break;
		}
		case WM_WTSSESSION_CHANGE: {
			int WindowsBuild = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\ShellCommon", L"BuildNumber");
			yValue* yV = new yValue{ WindowsBuild, 2000, NULL };
			if (wParam == WTS_SESSION_LOCK) Perform24H2Fixes(false);
			if (wParam == WTS_SESSION_UNLOCK && WindowsBuild >= 26002) WallpaperHelper24H2(yV);
			break;
		}
		case WM_CLOSE: {
			if (isDefaultRes()) SetPos(true);
			subviewwnd->ShowWindow(SW_HIDE);
			if (lParam == 420) {
				wchar_t* desktoplog = new wchar_t[260];
				wchar_t* cBuffer = new wchar_t[260];
				DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
				StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
				ShellExecuteW(NULL, L"open", L"notepad.exe", desktoplog, NULL, SW_SHOW);
				delete[] desktoplog;
				delete[] cBuffer;
			}
			DWORD dwTermination{};
			HANDLE termThread = CreateThread(0, 0, EndExplorer, NULL, 0, &dwTermination);
			Sleep(500);
			break;
		}
		case WM_COMMAND: {
			break;
		}
		case WM_TIMER: {
			KillTimer(hWnd, wParam);
			switch (wParam) {
			case 1:
				SendMessageW(hWnd, WM_USER + 15, NULL, 1);
				break;
			case 2:
				InitLayout(false, true, true);
				break;
			case 3:
				CreateSearchPage();
				break;
			}
			break;
		}
		case WM_USER + 1: {
			//pm[lParam].elem->SetAlpha(255);
			if (pm[lParam]->GetPage() == currentPageID) pm[lParam]->SetVisible(!hiddenIcons);
			else pm[lParam]->SetVisible(false);
			break;
		}
		case WM_USER + 2: {
			//subpm[lParam].elem->SetAlpha(255);
			yValueEx* yV = (yValueEx*)lParam;
			vector<LVItem*>* l_pm = yV->vpm;
			(*l_pm)[yV->y]->SetVisible(true);
			break;
		}
		case WM_USER + 3: {
			int lines_basedOnEllipsis{};
			pm[lParam]->ClearAllListeners();
			vector<IElementListener*> v_pels;
			v_pels.push_back(assignExtendedFn(pm[lParam], ItemDragListener, true));
			v_pels.push_back(assignFn(pm[lParam], ItemRightClick, true));
			if (!treatdirasgroup || pm[lParam]->GetGroupSize() == LVIGS_NORMAL) {
				v_pels.push_back(assignFn(pm[lParam], SelectItem, true));
				v_pels.push_back(assignExtendedFn(pm[lParam], SelectItemListener, true));
				v_pels.push_back(assignExtendedFn(pm[lParam], ShowCheckboxIfNeeded, true));
				v_pels.push_back(assignExtendedFn(cbpm[lParam], CheckboxHandler, true));
			}
			Element* groupdirectoryOld = regElem<Element*>(L"groupdirectory", pm[lParam]);
			if (groupdirectoryOld) {
				groupdirectoryOld->Destroy(true);
				pm[lParam]->GetChildItems().clear();
				pm[lParam]->GetChildIcons().clear();
				pm[lParam]->GetChildShadows().clear();
				pm[lParam]->GetChildShortcutArrows().clear();
				pm[lParam]->GetChildFilenames().clear();
			}
			if (treatdirasgroup && pm[lParam]->GetGroupSize() != LVIGS_NORMAL) {
				iconpm[lParam]->SetX(0);
				iconpm[lParam]->SetY(0);
				pm[lParam]->SetSelected(false);
				switch (pm[lParam]->GetGroupSize()) {
				case LVIGS_SMALL:
					if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 316);
					pm[lParam]->SetWidth(316 * flScaleFactor);
					pm[lParam]->SetHeight(200 * flScaleFactor);
					iconpm[lParam]->SetWidth(316 * flScaleFactor);
					iconpm[lParam]->SetHeight(200 * flScaleFactor);
					break;
				case LVIGS_MEDIUM:
					if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 476);
					pm[lParam]->SetWidth(476 * flScaleFactor);
					pm[lParam]->SetHeight(300 * flScaleFactor);
					iconpm[lParam]->SetWidth(476 * flScaleFactor);
					iconpm[lParam]->SetHeight(300 * flScaleFactor);
					break;
				case LVIGS_WIDE:
					if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 716);
					pm[lParam]->SetWidth(716 * flScaleFactor);
					pm[lParam]->SetHeight(300 * flScaleFactor);
					iconpm[lParam]->SetWidth(716 * flScaleFactor);
					iconpm[lParam]->SetHeight(300 * flScaleFactor);
					break;
				case LVIGS_LARGE:
					if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 716);
					pm[lParam]->SetWidth(716 * flScaleFactor);
					pm[lParam]->SetHeight(450 * flScaleFactor);
					iconpm[lParam]->SetWidth(716 * flScaleFactor);
					iconpm[lParam]->SetHeight(450 * flScaleFactor);
					break;
				}
				pm[lParam]->SetTooltip(false);
				Element* innerElem = regElem<Element*>(L"innerElem", pm[lParam]);
				innerElem->SetLayoutPos(-3);
				cbpm[lParam]->SetLayoutPos(-3);
				filepm[lParam]->SetLayoutPos(-3);
				if (!touchmode) fileshadowpm[lParam]->SetLayoutPos(-3);
				else {
					Element* containerElem = regElem<Element*>(L"containerElem", pm[lParam]);
					containerElem->SetPadding(0, 0, 0, 0);
				}
				pm[lParam]->SetBackgroundStdColor(20575);
				pm[lParam]->SetDrawType(0);
				v_pels.push_back(assignFn(pm[lParam], SelectItem2, true));
				int* itemID = (int*)(&lParam);
				HANDLE hCreateGroup = CreateThread(0, 0, InitDesktopGroup, itemID, 0, NULL);
			}
			else if (!touchmode) {
				lines_basedOnEllipsis = floor(CalcTextLines(pm[lParam]->GetSimpleFilename().c_str(), innerSizeX - 4 * flScaleFactor)) * textm.tmHeight;
				pm[lParam]->SetWidth(innerSizeX);
				pm[lParam]->SetHeight(innerSizeY + lines_basedOnEllipsis + 6 * flScaleFactor);
				filepm[lParam]->SetHeight(lines_basedOnEllipsis + 4 * flScaleFactor);
				fileshadowpm[lParam]->SetHeight(lines_basedOnEllipsis + 5 * flScaleFactor);
				iconpm[lParam]->SetWidth(round(globaliconsz * flScaleFactor));
				iconpm[lParam]->SetHeight(round(globaliconsz * flScaleFactor));
				iconpm[lParam]->SetX(iconPaddingX);
				iconpm[lParam]->SetY(round(iconPaddingY * 0.575));
				if (localeType == 1 && pm[lParam]->GetSizedFromGroup() == true) {
					pm[lParam]->SetSizedFromGroup(false);
				}
			}
			else {

			}
			if (touchmode && pm[lParam]->GetSizedFromGroup() == true) {
				pm[lParam]->SetWidth(touchSizeX);
				pm[lParam]->SetHeight(touchSizeY);
				pm[lParam]->SetSizedFromGroup(false);
			}
			pm[lParam]->SetListeners(v_pels);
			v_pels.clear();
			pm[lParam]->SetAnimation(listviewAnimStorage);
			if (!wParam) break;
			HBITMAP iconbmp = ((DesktopIcon*)wParam)->icon;
			Value* bitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(iconbmp);
			if (bitmap != nullptr) {
				iconpm[lParam]->SetValue(Element::ContentProp, 1, bitmap);
				bitmap->Release();
			}
			HBITMAP iconshadowbmp = ((DesktopIcon*)wParam)->iconshadow;
			Value* bitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(iconshadowbmp);
			if (bitmapShadow != nullptr) {
				shadowpm[lParam]->SetValue(Element::ContentProp, 1, bitmapShadow);
				bitmapShadow->Release();
			}
			HBITMAP iconshortcutbmp = ((DesktopIcon*)wParam)->iconshortcut;
			Value* bitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(iconshortcutbmp);
			if (bitmapShortcut != nullptr) {
				if (pm[lParam]->GetShortcutState() == true) shortpm[lParam]->SetValue(Element::ContentProp, 1, bitmapShortcut);
				bitmapShortcut->Release();
			}
			HBITMAP textbmp = ((DesktopIcon*)wParam)->text;
			Value* bitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(textbmp);
			if (bitmapText != nullptr) {
				filepm[lParam]->SetValue(Element::ContentProp, 1, bitmapText);
				bitmapText->Release();
			}
			HBITMAP textshadowbmp = ((DesktopIcon*)wParam)->textshadow;
			Value* bitmapTextShadow = DirectUI::Value::CreateGraphic(textshadowbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(textshadowbmp);
			if (bitmapTextShadow != nullptr) {
				fileshadowpm[lParam]->SetValue(Element::ContentProp, 1, bitmapTextShadow);
				bitmapTextShadow->Release();
			}
			if (touchmode) {
				((DDScalableElement*)pm[lParam])->SetDDCPIntensity((pm[lParam]->GetHiddenState() == true) ? 192 : 255);
				((DDScalableElement*)pm[lParam])->SetAssociatedColor(((DesktopIcon*)wParam)->crDominantTile);
			}
			break;
		}
		case WM_USER + 4: {
			if (pm[lParam]->GetHiddenState() == false) shadowpm[lParam]->SetAlpha(255);
			shortpm[lParam]->SetAlpha(255);
			if (!touchmode) {
				shadowpm[lParam]->SetWidth((globaliconsz + 16) * flScaleFactor);
				shadowpm[lParam]->SetHeight((globaliconsz + 16) * flScaleFactor);
				shadowpm[lParam]->SetX(iconPaddingX - 8 * flScaleFactor);
				shadowpm[lParam]->SetY((iconPaddingY * 0.575) - 6 * flScaleFactor);
				shortpm[lParam]->SetWidth(globalshiconsz * flScaleFactor);
				shortpm[lParam]->SetHeight(globalshiconsz * flScaleFactor);
				shortpm[lParam]->SetX(iconPaddingX);
				shortpm[lParam]->SetY((iconPaddingY * 0.575) + (globaliconsz - globalshiconsz) * flScaleFactor);
			}
			break;
		}
		case WM_USER + 5: {
			static const int dragWidth = _wtoi(GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragWidth"));
			static const int dragHeight = _wtoi(GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragHeight"));
			POINT ppt;
			GetCursorPos(&ppt);
			ScreenToClient(wnd->GetHWND(), &ppt);
			if (abs(ppt.x - origX) > dragWidth || abs(ppt.y - origY) > dragHeight) {
				emptyclicks = 1;
			}
			if (localeType == 1) ppt.x = dimensions.right - ppt.x;
			MARGINS borders = { (ppt.x < origX) ? ppt.x : origX, abs(ppt.x - origX),
				(ppt.y < origY) ? ppt.y : origY, abs(ppt.y - origY) };
			selector->SetWidth(borders.cxRightWidth);
			selector->SetX(borders.cxLeftWidth);
			selector->SetHeight(borders.cyBottomHeight);
			selector->SetY(borders.cyTopHeight);
			selector2->SetWidth(borders.cxRightWidth);
			selector2->SetX(borders.cxLeftWidth);
			selector2->SetHeight(borders.cyBottomHeight);
			selector2->SetY(borders.cyTopHeight);
			for (int items = 0; items < pm.size(); items++) {
				MARGINS iconborders = { pm[items]->GetX(), pm[items]->GetX() + pm[items]->GetWidth(), pm[items]->GetY(), pm[items]->GetY() + pm[items]->GetHeight() };
				bool selectstate = (borders.cxRightWidth + borders.cxLeftWidth > iconborders.cxLeftWidth &&
					iconborders.cxRightWidth > borders.cxLeftWidth &&
					borders.cyBottomHeight + borders.cyTopHeight > iconborders.cyTopHeight &&
					iconborders.cyBottomHeight > borders.cyTopHeight);
				if (pm[items]->GetPage() == currentPageID) {
					pm[items]->SetSelected(selectstate);
					if (showcheckboxes == 1) cbpm[items]->SetVisible(selectstate);
				}
			}
			break;
		}
		case WM_USER + 6: {
			fullscreenpopupbase->SetVisible(true);
			fullscreeninner->SetVisible(true);
			fullscreeninner->SetY(dimensions.bottom * 0.1 * (1 - py[popupframe - 1]) + 1);
			break;
		}
		case WM_USER + 7: {
			checkifelemexists = false;
			AnimateWindow(subviewwnd->GetHWND(), 120, AW_BLEND | AW_HIDE);
			if (pendingaction) Sleep(700);
			if (lParam == 1) {
				HideSimpleView(false);
				mainContainer->SetVisible(true);
			}
			centered->DestroyAll(true);
			break;
		}
		case WM_USER + 8: {
			break;
		}
		case WM_USER + 9: {
			yValueEx* yV = (yValueEx*)lParam;
			vector<LVItem*>* l_pm = yV->vpm;
			vector<DDScalableElement*>* l_iconpm = yV->vipm;
			vector<Element*>* l_shadowpm = yV->vispm;
			vector<Element*>* l_shortpm = yV->vspm;
			vector<RichText*>* l_filepm = yV->vfpm;
			if (!touchmode && (*l_pm)[yV->y]) {
				(*l_pm)[yV->y]->SetWidth(innerSizeX);
				(*l_pm)[yV->y]->SetHeight(innerSizeY + textm.tmHeight + 23 * flScaleFactor);
				int textlines = 1;
				if (textm.tmHeight <= 18 * flScaleFactor) textlines = 2;
				(*l_filepm)[yV->y]->SetHeight(textm.tmHeight * textlines + 4 * flScaleFactor);
				if ((*l_filepm)[yV->y]->GetHeight() > (iconPaddingY * 0.575 + 48)) (*l_filepm)[yV->y]->SetHeight(iconPaddingY * 0.575 + 48);
				(*l_iconpm)[yV->y]->SetWidth(round(globaliconsz * flScaleFactor));
				(*l_iconpm)[yV->y]->SetHeight(round(globaliconsz * flScaleFactor));
				(*l_iconpm)[yV->y]->SetX(iconPaddingX);
				(*l_iconpm)[yV->y]->SetY(round(iconPaddingY * 0.575));
			}
			HBITMAP iconbmp = ((DesktopIcon*)wParam)->icon;
			Value* bitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(iconbmp);
			if (bitmap != nullptr) {
				(*l_iconpm)[yV->y]->SetValue(Element::ContentProp, 1, bitmap);
				bitmap->Release();
			}
			HBITMAP iconshadowbmp = ((DesktopIcon*)wParam)->iconshadow;
			Value* bitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(iconshadowbmp);
			if (bitmapShadow != nullptr) {
				(*l_shadowpm)[yV->y]->SetValue(Element::ContentProp, 1, bitmapShadow);
				bitmapShadow->Release();
			}
			HBITMAP iconshortcutbmp = ((DesktopIcon*)wParam)->iconshortcut;
			Value* bitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(iconshortcutbmp);
			if (bitmapShortcut != nullptr) {
				if ((*l_pm)[yV->y]->GetShortcutState() == true) (*l_shortpm)[yV->y]->SetValue(Element::ContentProp, 1, bitmapShortcut);
				bitmapShortcut->Release();
			}
			HBITMAP textbmp = ((DesktopIcon*)wParam)->text;
			Value* bitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
			DeleteObject(textbmp);
			if (bitmapText != nullptr) {
				(*l_filepm)[yV->y]->SetValue(Element::ContentProp, 1, bitmapText);
				bitmapText->Release();
			}
			if (touchmode) {
				((DDScalableElement*)(*l_pm)[yV->y])->SetDDCPIntensity(((*l_pm)[yV->y]->GetHiddenState() == true) ? 192 : 255);
				((DDScalableElement*)(*l_pm)[yV->y])->SetAssociatedColor(((DesktopIcon*)wParam)->crDominantTile);
			}
			break;
		}
		case WM_USER + 10: {
			yValueEx* yV = (yValueEx*)lParam;
			vector<LVItem*>* l_pm = yV->vpm;
			vector<Element*>* l_shadowpm = yV->vispm;
			vector<Element*>* l_shortpm = yV->vspm;
			if ((*l_pm)[yV->y]) {
				if ((*l_pm)[yV->y]->GetHiddenState() == false) (*l_shadowpm)[yV->y]->SetAlpha(255);
				if ((*l_shortpm)[yV->y]) (*l_shortpm)[yV->y]->SetAlpha(255);
				if (!touchmode && (*l_shadowpm)[yV->y] && (*l_shortpm)[yV->y]) {
					(*l_shadowpm)[yV->y]->SetWidth((globaliconsz + 16) * flScaleFactor);
					(*l_shadowpm)[yV->y]->SetHeight((globaliconsz + 16) * flScaleFactor);
					(*l_shadowpm)[yV->y]->SetX(iconPaddingX - 8 * flScaleFactor);
					(*l_shadowpm)[yV->y]->SetY((iconPaddingY * 0.575) - 6 * flScaleFactor);
					(*l_shortpm)[yV->y]->SetWidth(globalshiconsz * flScaleFactor);
					(*l_shortpm)[yV->y]->SetHeight(globalshiconsz * flScaleFactor);
					(*l_shortpm)[yV->y]->SetX(iconPaddingX);
					(*l_shortpm)[yV->y]->SetY((iconPaddingY * 0.575) + (globaliconsz - globalshiconsz) * flScaleFactor);
				}
			}
			break;
		}
		case WM_USER + 11: {
			pendingaction = true;
			Element* peTemp = reinterpret_cast<Element*>(wParam);
			peTemp->SetEnabled(!peTemp->GetEnabled());
			if (lParam == 1 && ((DDScalableButton*)peTemp)->GetAssociatedFn() != nullptr) {
				((DDScalableButton*)peTemp)->ExecAssociatedFn(((DDScalableButton*)peTemp)->GetAssociatedFn(), false, true, true);
				pendingaction = false;
			}
			break;
		}
		case WM_USER + 12: {
			if (checkifelemexists == true) dirnameanimator->SetWidth((100 * (1 - py[dframe - 1])) * flScaleFactor);
			break;
		}
		case WM_USER + 13: {
			if (checkifelemexists == true) tasksanimator->SetWidth((60 * (1 - py[tframe - 1])) * flScaleFactor);
			break;
		}
		case WM_USER + 14: {
			vector<DWORD> smThumbnailThread;
			vector<HANDLE> smThumbnailThreadHandle;
			smThumbnailThread.resize(pm.size());
			smThumbnailThreadHandle.resize(pm.size());
			for (int icon = 0; icon < pm.size(); icon++) {
				IconThumbHelper(icon);
			}
			for (int icon2 = 0; icon2 < pm.size(); icon2++) {
				yValue* yV = new yValue{ icon2 };
				smThumbnailThreadHandle[icon2] = CreateThread(0, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, &(smThumbnailThread[icon2]));
			}
			smThumbnailThread.clear();
			smThumbnailThreadHandle.clear();
			break;
		}
		case WM_USER + 15: {
			if (!editmode || lParam == 0) ShowSimpleView(false);
			else HideSimpleView(false);
			break;
		}
		case WM_USER + 16: {
			ThumbnailIcon* ti = (ThumbnailIcon*)wParam;
			HBITMAP thumbIcon = ti->icon;
			Value* vThumbIcon = DirectUI::Value::CreateGraphic(thumbIcon, 2, 0xffffffff, false, false, false);
			Element* GroupedIcon{};
			parser->CreateElement(L"GroupedIcon", NULL, NULL, NULL, (Element**)&GroupedIcon);
			iconpm[lParam]->Add((Element**)&GroupedIcon, 1);
			GroupedIcon->SetWidth(globalgpiconsz * flScaleFactor), GroupedIcon->SetHeight(globalgpiconsz * flScaleFactor);
			GroupedIcon->SetX(ti->x), GroupedIcon->SetY(ti->y);
			if (ti->str.GetHiddenState()) GroupedIcon->SetAlpha(128);
			if (vThumbIcon != nullptr) GroupedIcon->SetValue(Element::ContentProp, 1, vThumbIcon);
			DeleteObject(thumbIcon);
			if (vThumbIcon != nullptr) vThumbIcon->Release();
			free(ti);
			break;
		}
		case WM_USER + 17: {
			static int dragToPrev{}, dragToNext{};
			Value* v;
			POINT ppt;
			GetCursorPos(&ppt);
			ScreenToClient(wnd->GetHWND(), &ppt);
			static const int dragWidth = 0;
			static const int dragHeight = 0;
			static const int savedanim = UIContainer->GetAnimation();
			vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
			if (abs(ppt.x - ((POINT*)lParam)->x) > dragWidth || abs(ppt.y - ((POINT*)lParam)->y) > dragHeight) {
				internalselectedLVItems[0]->SetDragState(true);
				dragpreview->SetVisible(true);
			}
			if (ppt.x < 16 * flScaleFactor) dragToPrev++; else dragToPrev = 0;
			if (ppt.x > dimensions.right - 16 * flScaleFactor) dragToNext++; else dragToNext = 0;
			if (dragToPrev > 40 && currentPageID > 1) {
				currentPageID--;
				short animSrc = (localeType == 1) ? 1 : -1;
				for (int i = 0; i < internalselectedLVItems.size(); i++) {
					internalselectedLVItems[i]->SetPage(currentPageID);
					internalselectedLVItems[i]->SetX(internalselectedLVItems[i]->GetX() - dimensions.right * animSrc);
				}
				for (int items = 0; items < pm.size(); items++) {
					if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
					else pm[items]->SetVisible(false);
				}
				UIContainer->SetAnimation(NULL);
				UIContainer->SetX((dimensions.right - dimensions.left) * animSrc);
				UIContainer->SetAnimation(savedanim);
				UIContainer->SetX(0);
				nextpageMain->SetVisible(true);
				if (currentPageID == 1) prevpageMain->SetVisible(false);
				dragToPrev = 0;
			}
			if (dragToNext > 40 && currentPageID < maxPageID) {
				currentPageID++;
				short animSrc = (localeType == 1) ? -1 : 1;
				for (int i = 0; i < internalselectedLVItems.size(); i++) {
					internalselectedLVItems[i]->SetPage(currentPageID);
					internalselectedLVItems[i]->SetX(internalselectedLVItems[i]->GetX() - dimensions.right * animSrc);
				}
				for (int items = 0; items < pm.size(); items++) {
					if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
					else pm[items]->SetVisible(false);
				}
				UIContainer->SetAnimation(NULL);
				UIContainer->SetX((dimensions.right - dimensions.left) * animSrc);
				UIContainer->SetAnimation(savedanim);
				UIContainer->SetX(0);
				prevpageMain->SetVisible(true);
				if (currentPageID == maxPageID) nextpageMain->SetVisible(false);
				dragToNext = 0;
			}
			if (localeType == 1) ppt.x = dimensions.right - ppt.x;
			dragpreview->SetX(ppt.x - origX);
			dragpreview->SetY(ppt.y - origY);
			break;
		}
		case WM_USER + 18: {
			switch (lParam) {
			case 0: {
				if (wParam != 0) {
					vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
					POINT ppt;
					Value* v;
					GetCursorPos(&ppt);
					ScreenToClient(wnd->GetHWND(), &ppt);
					int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
					int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 22) * flScaleFactor;
					int desktoppadding = flScaleFactor * (touchmode ? DESKPADDING_TOUCH : DESKPADDING_NORMAL);
					int desktoppadding_x = flScaleFactor * (touchmode ? DESKPADDING_TOUCH_X : DESKPADDING_NORMAL_X);
					int desktoppadding_y = flScaleFactor * (touchmode ? DESKPADDING_TOUCH_Y : DESKPADDING_NORMAL_Y);
					if (touchmode) {
						outerSizeX = touchSizeX + desktoppadding;
						outerSizeY = touchSizeY + desktoppadding;
					}
					int xRender = (localeType == 1) ? ppt.x + origX - desktoppadding_x - internalselectedLVItems[0]->GetWidth() : ppt.x - origX - desktoppadding_x;
					int paddingmitigation = (localeType == 1) ? desktoppadding : 0;
					int destX = desktoppadding_x + round(xRender / static_cast<float>(outerSizeX)) * outerSizeX;
					int destY = desktoppadding_y + round((ppt.y - origY - desktoppadding_y) / static_cast<float>(outerSizeY)) * outerSizeY;
					if (localeType == 1) {
						destX = dimensions.right - destX - internalselectedLVItems[0]->GetWidth();
					}
					const int mainElementX = internalselectedLVItems[0]->GetX();
					const int mainElementY = internalselectedLVItems[0]->GetY();
					int itemstodrag = internalselectedLVItems.size();
					if (itemstodrag == 0) itemstodrag = 1;
					for (int items = 0; items < itemstodrag; items++) {
						int finaldestX = destX - mainElementX + internalselectedLVItems[items]->GetX();
						int finaldestY = destY - mainElementY + internalselectedLVItems[items]->GetY();
						if (localeType == 1) {
							if (finaldestX < 0) finaldestX = dimensions.right - round((dimensions.right - internalselectedLVItems[items]->GetWidth()) / static_cast<float>(outerSizeX)) * outerSizeX - desktoppadding_x;
							if (finaldestX > dimensions.right - internalselectedLVItems[items]->GetWidth() - desktoppadding_x) finaldestX = dimensions.right - internalselectedLVItems[items]->GetWidth() - desktoppadding_x;
						}
						else {
							if (finaldestX < 0) finaldestX = desktoppadding_x;
							if (finaldestX > dimensions.right - internalselectedLVItems[items]->GetWidth() + desktoppadding_x) finaldestX = round((dimensions.right - internalselectedLVItems[items]->GetWidth()) / static_cast<float>(outerSizeX)) * outerSizeX + desktoppadding_x;
						}
						if (finaldestY < 0) finaldestY = desktoppadding_y;
						if (finaldestY > dimensions.bottom - internalselectedLVItems[items]->GetHeight() + desktoppadding_y) finaldestY = round((dimensions.bottom - internalselectedLVItems[items]->GetHeight()) / static_cast<float>(outerSizeY)) * outerSizeY + desktoppadding_y;
						int saveddestX = (localeType == 1) ? dimensions.right - finaldestX - internalselectedLVItems[items]->GetWidth() : finaldestX;
						for (int items2 = 0; items2 < pm.size(); items2++) {
							if (pm[items2]->GetX() == finaldestX && pm[items2]->GetY() == finaldestY && pm[items2]->GetPage() == internalselectedLVItems[items]->GetPage()) break;
							if (items2 == pm.size() - 1) {
								internalselectedLVItems[items]->SetX(finaldestX);
								internalselectedLVItems[items]->SetY(finaldestY);
								internalselectedLVItems[items]->SetInternalXPos(saveddestX / outerSizeX);
								internalselectedLVItems[items]->SetInternalYPos(finaldestY / outerSizeY);
							}
						}
					}
					internalselectedLVItems.clear();
				}
				dragpreview->SetVisible(false);
				break;
			}
			case 1: {
				if (wParam != NULL) {
					LVItem* item = (*(vector<LVItem*>*)wParam)[0];
					item->SetDragState(false);
				}
				break;
			}
			case 2: {
				vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
				LVItem* item = internalselectedLVItems[0];
				item->SetDragState(false);
				internalselectedLVItems.clear();
				dragpreview->SetVisible(false);
				MessageBeep(MB_OK);
				DDNotificationBanner* ddnb{};
				DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_INFO, L"DDNB", LoadStrFromRes(4044).c_str(), LoadStrFromRes(4045).c_str(), 5, false);
				break;
			}
			}
			break;
		}
		case WM_USER + 19: {
			if (delayedshutdownstatuses[lParam - 1] == false) break;
			switch (lParam) {
			case 1: {
				WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, FALSE);
				break;
			}
			case 2: {
				ExitWindowsEx(EWX_LOGOFF, 0);
				break;
			}
			case 3: {
				SetSuspendState(FALSE, FALSE, FALSE);
				break;
			}
			case 4: {
				SetSuspendState(TRUE, FALSE, FALSE);
				break;
			}
			case 5: {
				ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF, shutdownReason);
				break;
			}
			case 6: {
				ExitWindowsEx(EWX_REBOOT, shutdownReason);
				break;
			}
			}
			delayedshutdownstatuses[lParam - 1] = false;
			break;
		}
		case WM_USER + 20: {
			LVItem* outerElem;
			FileInfo* fi = (FileInfo*)lParam;
			if (touchmode) {
				parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElem);
			}
			else parser->CreateElement(L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
			DDScalableElement* iconElem = regElem<DDScalableElement*>(L"iconElem", outerElem);
			Element* shortcutElem = regElem<Element*>(L"shortcutElem", outerElem);
			Element* iconElemShadow = regElem<Element*>(L"iconElemShadow", outerElem);
			RichText* textElem = regElem<RichText*>(L"textElem", outerElem);
			RichText* textElemShadow = regElem<RichText*>(L"textElemShadow", outerElem);
			Button* checkboxElem = regElem<Button*>(L"checkboxElem", outerElem);

			int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
			int isThumbnailHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"IconsOnly");
			wstring foundfilename = (wstring)L"\"" + fi->filepath + (wstring)L"\\" + fi->filename + (wstring)L"\"";
			DWORD attr = GetFileAttributesW(RemoveQuotes(foundfilename).c_str());
			wstring foundsimplefilename = hideExt((wstring)fi->filename, isFileExtHidden, (attr & 16), outerElem);
			if (attr & 16) outerElem->SetDirState(true);
			else outerElem->SetDirState(false);
			if (attr & 2) outerElem->SetHiddenState(true);
			else outerElem->SetHiddenState(false);
			if (isThumbnailHidden == 0) {
				bool image;
				isImage(foundfilename, true, imageExts[0], &image);
				outerElem->SetColorLock(image);
			}
			outerElem->SetSimpleFilename(foundsimplefilename);
			outerElem->SetFilename(foundfilename);
			outerElem->SetAccDesc(GetExplorerTooltipText(foundfilename).c_str());
			if (outerElem->GetHiddenState() == true) {
				iconElem->SetAlpha(128);
				iconElemShadow->SetAlpha(0);
				textElem->SetAlpha(touchmode ? 128 : 192);
				if (!touchmode) textElemShadow->SetAlpha(128);
			}
			if (!touchmode) {
				if (shellstate[4] & 0x20) {
					outerElem->SetClass(L"doubleclicked");
				}
				else outerElem->SetClass(L"singleclicked");
			}
			UIContainer->Add((Element**)&outerElem, 1);

			pm.push_back(outerElem);
			iconpm.push_back(iconElem);
			shortpm.push_back(shortcutElem);
			shadowpm.push_back(iconElemShadow);
			filepm.push_back(textElem);
			fileshadowpm.push_back(textElemShadow);
			cbpm.push_back(checkboxElem);

			int currentID = pm.size() - 1;
			IconThumbHelper(currentID);
			yValue* yV = new yValue{ currentID };
			HANDLE smThumbnailThreadHandle = CreateThread(0, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, NULL);
			RearrangeIcons(false, false, true);
			pm[currentID]->SetRefreshState(true);
			break;
		}
		case WM_USER + 21: {
			LVItem* toRemove = (LVItem*)wParam;
			pm.erase(pm.begin() + lParam);
			iconpm.erase(iconpm.begin() + lParam);
			shadowpm.erase(shadowpm.begin() + lParam);
			shortpm.erase(shortpm.begin() + lParam);
			filepm.erase(filepm.begin() + lParam);
			fileshadowpm.erase(fileshadowpm.begin() + lParam);
			cbpm.erase(cbpm.begin() + lParam);
			toRemove->Destroy(true);
			toRemove = nullptr;
			break;
		}
						 //case WM_USER + 22: {
						 //    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
						 //    LVItem* toRename = (LVItem*)wParam;
						 //    FileInfo* fi = (FileInfo*)lParam;
						 //    wstring foundfilename = (wstring)L"\"" + fi->filepath + (wstring)L"\\" + fi->filename + (wstring)L"\"";
						 //    toRename->SetFilename(foundfilename);
						 //    DWORD attr = GetFileAttributesW(RemoveQuotes(foundfilename).c_str());
						 //    toRename->SetSimpleFilename(hideExt(fi->filename, isFileExtHidden, (attr & 16), toRename));
						 //    SelectItemListener(toRename, Element::SelectedProp(), 69, NULL, NULL);
						 //    break;
						 //}
		case WM_USER + 23: {
			if (wParam != NULL) ((Element*)wParam)->SetX(lParam);
			break;
		}
		case WM_USER + 24: {
			ShowDirAsGroupDesktop(pm[lParam]);
			break;
		}
		case WM_USER + 25: {
			((Element*)wParam)->SetLayoutPos(-3);
			((Element*)lParam)->SetLayoutPos(2);
			break;
		}
		case WM_USER + 26: {
			if (pm[lParam] && (!treatdirasgroup || pm[lParam]->GetGroupSize() == LVIGS_NORMAL)) {
				Element* innerElem = regElem<Element*>(L"innerElem", pm[lParam]);
				Element* g_innerElem = regElem<Element*>(L"innerElem", g_outerElem);
				Element* checkboxElem = regElem<Element*>(L"checkboxElem", g_outerElem);
				innerElem->SetLayoutPos(g_innerElem->GetLayoutPos()), cbpm[lParam]->SetLayoutPos(checkboxElem->GetLayoutPos());
				if (touchmode) {
					// had to hardcode it as GetPadding is VERY unreliable on high dpi
					int space = 4 * flScaleFactor;
					Element* containerElem = regElem<Element*>(L"containerElem", pm[lParam]);
					containerElem->SetPadding(space, space, space, space);
				}
				SelectItemListener(pm[lParam], Element::SelectedProp(), 69, NULL, NULL);
			}
			break;
		}
		}
		return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
	}

	LRESULT CALLBACK TopLevelWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_DPICHANGED: {
			delayGroupsForDpi = true;
			UpdateScale();
			InitLayout(false, true, false);
			break;
		}
		case WM_DISPLAYCHANGE: {
			if (isDefaultRes()) SetPos(true);
			AdjustWindowSizes(true);
			lastWidth = 0, lastHeight = 0;
			RearrangeIcons(true, false, true);
			break;
		}
		case WM_CLOSE: {
			return 0L;
			break;
		}
		case WM_USER + 1: {
			if (wParam < 4096) break;
			RedrawImageCore((DDScalableElement*)wParam);
			break;
		}
		case WM_USER + 2: {
			if (wParam < 4096) break;
			RedrawFontCore((DDScalableElement*)wParam);
			break;
		}
		case WM_USER + 3: {
			DDCheckBoxGlyph* peGlyph;
			DDScalableElement* peText;
			DDCheckBoxGlyph::Create((Element*)wParam, 0, (Element**)&peGlyph);
			((Element*)wParam)->Add((Element**)&peGlyph, 1);
			peGlyph->SetCheckedState(((DDCheckBox*)wParam)->GetCheckedState());
			peGlyph->SetID(L"DDCB_Glyph");
			DDScalableElement::Create((Element*)wParam, 0, (Element**)&peText);
			((Element*)wParam)->Add((Element**)&peText, 1);
			Value* v;
			peText->SetContentString(((Element*)wParam)->GetContentString(&v));
			peText->SetID(L"DDCB_Text");
			((Element*)wParam)->SetContentString(L"");
			break;
		}
		case WM_USER + 4: {
			DDColorPickerButton* peTemp;
			int scaleInterval = GetCurrentScaleInterval();
			int scaleIntervalImage = ((DDColorPicker*)wParam)->GetScaledImageIntervals();
			if (scaleInterval > scaleIntervalImage - 1) scaleInterval = scaleIntervalImage - 1;
			int imageID = ((DDColorPicker*)wParam)->GetFirstScaledImage() + scaleInterval;
			HBITMAP newImage = (HBITMAP)LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
			if (newImage == nullptr) {
				newImage = LoadPNGAsBitmap(imageID);
				IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1, NULL);
			}
			COLORREF* pImmersiveColor = ((DDColorPicker*)wParam)->GetThemeAwareness() ? theme ? &ImmersiveColorL : &ImmersiveColorD : &ImmersiveColor;
			COLORREF colorPickerPalette[8] =
			{
				((DDColorPicker*)wParam)->GetDefaultColor(),
				*pImmersiveColor,
				((DDColorPicker*)wParam)->GetThemeAwareness() ? theme ? RGB(96, 205, 255) : RGB(0, 95, 184) : RGB(0, 120, 215),
				((DDColorPicker*)wParam)->GetThemeAwareness() ? theme ? RGB(216, 141, 225) : RGB(158, 58, 176) : RGB(177, 70, 194),
				((DDColorPicker*)wParam)->GetThemeAwareness() ? theme ? RGB(244, 103, 98) : RGB(210, 14, 30) : RGB(232, 17, 35),
				((DDColorPicker*)wParam)->GetThemeAwareness() ? theme ? RGB(251, 154, 68) : RGB(224, 83, 7) : RGB(247, 99, 12),
				((DDColorPicker*)wParam)->GetThemeAwareness() ? theme ? RGB(255, 213, 42) : RGB(225, 157, 0) : RGB(255, 185, 0),
				((DDColorPicker*)wParam)->GetThemeAwareness() ? theme ? RGB(38, 255, 142) : RGB(0, 178, 90) : RGB(0, 204, 106)
			};
			BITMAP bm{};
			GetObject(newImage, sizeof(BITMAP), &bm);
			int btnWidth = bm.bmWidth / 8;
			int btnHeight = bm.bmHeight;
			int btnX = ((Element*)wParam)->GetWidth() / 8;
			int btnY = (((Element*)wParam)->GetHeight() - bm.bmHeight) / 2;

			HDC hdc = GetDC(NULL);
			HDC hdcSrc = CreateCompatibleDC(hdc);
			HDC hdcDst = CreateCompatibleDC(hdc);
			SelectObject(hdcSrc, newImage);
			for (int i = 0; i < 8; i++) {
				int xPos = (localeType == 1) ? ((Element*)wParam)->GetWidth() - i * btnX - btnWidth : i * btnX;
				HBITMAP hbmPickerBtn = CreateCompatibleBitmap(hdc, btnWidth, btnHeight);
				SelectObject(hdcDst, hbmPickerBtn);
				BitBlt(hdcDst, 0, 0, btnWidth, btnHeight, hdcSrc, i * btnWidth, 0, SRCCOPY);
				DDColorPickerButton::Create((Element*)wParam, 0, (Element**)&peTemp);
				((Element*)wParam)->Add((Element**)&peTemp, 1);
				peTemp->SetLayoutPos(-2);
				peTemp->SetX(xPos);
				peTemp->SetY(btnY);
				peTemp->SetWidth(bm.bmWidth / 8);
				peTemp->SetHeight(btnHeight);
				Value* vPickerBtn = Value::CreateGraphic(hbmPickerBtn, 2, 0xffffffff, true, false, false);
				if (vPickerBtn) {
					peTemp->SetValue(Element::ContentProp, 1, vPickerBtn);
					vPickerBtn->Release();
				}
				peTemp->SetAssociatedColor(colorPickerPalette[i]);
				peTemp->SetOrder(i);
				peTemp->SetTargetElements(((DDColorPicker*)wParam)->GetTargetElements());
				DeleteObject(hbmPickerBtn);
			}
			if (newImage) DeleteObject(newImage);
			DeleteDC(hdcSrc);
			DeleteDC(hdcDst);
			ReleaseDC(NULL, hdc);

			RegKeyValue rkv = ((DDColorPicker*)wParam)->GetRegKeyValue();
			int order = GetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind) * btnX;
			int checkedBtnX = (localeType == 1) ? ((Element*)wParam)->GetWidth() - order - btnWidth : order;
			DDScalableElement* peCircle;
			//DDScalableElement::Create((Element*)wParam, 0, (Element**)&peCircle);
			//((Element*)wParam)->Add((Element**)&peCircle, 1);
			//peCircle->SetID(L"DDColorPicker_HoverCircle");
			DDScalableElement::Create((Element*)wParam, 0, (Element**)&peCircle);
			((Element*)wParam)->Add((Element**)&peCircle, 1);
			peCircle->SetLayoutPos(-2);
			peCircle->SetX(checkedBtnX);
			peCircle->SetWidth(bm.bmWidth / 8);
			peCircle->SetHeight(btnHeight);
			peCircle->SetID(L"DDColorPicker_CheckedCircle");
			break;
		}
		}
		return CallWindowProc(WndProc2, hWnd, uMsg, wParam, lParam);
	}

	unsigned long animate(LPVOID lpParam) {
		Sleep(150);
		yValue* yV = (yValue*)lpParam;
		SendMessageW(wnd->GetHWND(), WM_USER + 1, NULL, yV->y);
		return 0;
	}

	unsigned long subanimate(LPVOID lpParam) {
		Sleep(150);
		yValueEx* yV = (yValueEx*)lpParam;
		SendMessageW(wnd->GetHWND(), WM_USER + 2, NULL, (LPARAM)yV);
		return 0;
	}

	unsigned long fastin(LPVOID lpParam) {
		InitThread(TSM_DESKTOP_DYNAMIC);
		Sleep(25);
		yValue* yV = (yValue*)lpParam;
		if (pm[yV->y]->GetRefreshState()) {
			int lines_basedOnEllipsis{};
			DWORD alignment{};
			RECT touchmoderect{};
			CalcDesktopIconInfo(yV, &lines_basedOnEllipsis, &alignment, false, &pm, &filepm);
			HBITMAP capturedBitmap{};
			capturedBitmap = CreateTextBitmap(pm[yV->y]->GetSimpleFilename().c_str(), yV->innerSizeX - 4 * flScaleFactor, lines_basedOnEllipsis, alignment, touchmode);
			DesktopIcon di;
			ApplyIcons(pm, iconpm, &di, false, yV->y, 1);
			if (touchmode) {
				di.crDominantTile = GetDominantColorFromIcon(di.icon, globaliconsz, 48);
				if (treatdirasgroup && pm[yV->y]->GetDirState() == true) {
					COLORREF crDefault = theme ? RGB(208, 208, 208) : RGB(48, 48, 48);
					di.crDominantTile = iconpm[yV->y]->GetAssociatedColor() == -1 ? crDefault : iconpm[yV->y]->GetAssociatedColor();
				}
				rgb_t saturatedColor = { GetRValue(di.crDominantTile), GetGValue(di.crDominantTile), GetBValue(di.crDominantTile) };
				hsl_t saturatedColor2 = rgb2hsl(saturatedColor);
				saturatedColor2.l /= 4;
				saturatedColor2.s *= 4;
				saturatedColor = hsl2rgb(saturatedColor2);
				IterateBitmap(di.iconshadow, StandardBitmapPixelHandler, 3, 0, 1, RGB(saturatedColor.r, saturatedColor.g, saturatedColor.b));
				if (GetRValue(di.crDominantTile) * 0.299 + GetGValue(di.crDominantTile) * 0.587 + GetBValue(di.crDominantTile) * 0.114 > 156) {
					IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
					IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 1, NULL);
				}
				else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
			}
			else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
			if (capturedBitmap != nullptr) di.text = capturedBitmap;
			if (!touchmode) {
				HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor, 2 * flScaleFactor, 2 * flScaleFactor, 2 * flScaleFactor);
				IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), 2, NULL);
				if (shadowBitmap != nullptr) di.textshadow = shadowBitmap;
			}
			if (di.icon == nullptr) fastin(lpParam);
			SendMessageW(wnd->GetHWND(), WM_USER + 4, NULL, yV->y);
			SendMessageW(wnd->GetHWND(), WM_USER + 3, (WPARAM)&di, yV->y);
			Sleep(250);
			SendMessageW(wnd->GetHWND(), WM_USER + 26, NULL, yV->y);
		}
		return 0;
	}

	unsigned long subfastin(LPVOID lpParam) {
		InitThread(TSM_DESKTOP_DYNAMIC);
		Sleep(25);
		yValueEx* yV = (yValueEx*)lpParam;
		vector<LVItem*>* l_pm = yV->vpm;
		int lines_basedOnEllipsis{};
		DWORD alignment{};
		RECT touchmoderect{};
		int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
		int textlines = 1;
		if (textm.tmHeight <= 18 * flScaleFactor) textlines = 2;
		yValue* yV2 = new yValue{ yV->y, yV->innerSizeX, yV->innerSizeY };
		if (touchmode) CalcDesktopIconInfo(yV2, &lines_basedOnEllipsis, &alignment, true, yV->vpm, yV->vfpm);
		HBITMAP capturedBitmap{};
		if (touchmode) capturedBitmap = CreateTextBitmap((*l_pm)[yV->y]->GetSimpleFilename().c_str(), yV2->innerSizeX - 4 * flScaleFactor, lines_basedOnEllipsis, alignment, touchmode);
		else capturedBitmap = CreateTextBitmap((*l_pm)[yV->y]->GetSimpleFilename().c_str(), innerSizeX, textm.tmHeight * textlines, DT_CENTER | DT_END_ELLIPSIS, touchmode);
		DesktopIcon di;
		ApplyIcons(*l_pm, *(yV->vipm), &di, false, yV->y, 1);
		if (touchmode) {
			di.crDominantTile = GetDominantColorFromIcon(di.icon, globaliconsz, 48);
			rgb_t saturatedColor = { GetRValue(di.crDominantTile), GetGValue(di.crDominantTile), GetBValue(di.crDominantTile) };
			hsl_t saturatedColor2 = rgb2hsl(saturatedColor);
			saturatedColor2.l /= 4;
			saturatedColor2.s *= 4;
			saturatedColor = hsl2rgb(saturatedColor2);
			IterateBitmap(di.iconshadow, StandardBitmapPixelHandler, 3, 0, 1, RGB(saturatedColor.r, saturatedColor.g, saturatedColor.b));
			if (GetRValue(di.crDominantTile) * 0.299 + GetGValue(di.crDominantTile) * 0.587 + GetBValue(di.crDominantTile) * 0.114 > 156) {
				IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
				IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 1, NULL);
			}
			else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
		}
		else if (theme && !touchmode) {
			IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
			IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 0.9, NULL);
		}
		else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
		if (capturedBitmap != nullptr) di.text = capturedBitmap;
		if (yV->vpm) {
			SendMessageW(wnd->GetHWND(), WM_USER + 10, NULL, (LPARAM)yV);
			SendMessageW(wnd->GetHWND(), WM_USER + 9, (WPARAM)&di, (LPARAM)yV);
		}
		else {
			DeleteObject(di.icon), DeleteObject(di.iconshadow), DeleteObject(di.iconshortcut), DeleteObject(di.text), DeleteObject(di.textshadow);
			free(yV);
		}
		return 0;
	}

	unsigned long InitDesktopGroup(LPVOID lpParam) {
		int itemID = *((int*)lpParam);
		if (pm[itemID]->GetGroupSize() == LVIGS_NORMAL) return 1;
		if (touchmode) Sleep(500);
		if (delayGroupsForDpi) Sleep(2250);
		Sleep(250);
		delayGroupsForDpi = false;
		PostMessageW(wnd->GetHWND(), WM_USER + 24, NULL, itemID);
		return 0;
	}

	unsigned long animate6(LPVOID lpParam) {
		Sleep(350);
		AnimateWindow(subviewwnd->GetHWND(), 120, AW_BLEND | AW_HIDE);
		BlurBackground(subviewwnd->GetHWND(), false, true);
		SendMessageW(wnd->GetHWND(), WM_USER + 7, NULL, NULL);
		return 0;
	}

	unsigned long AnimateWindowWrapper2(LPVOID lpParam) {
		subviewwnd->ShowWindow(SW_SHOW);
		//AnimateWindow(subviewwnd->GetHWND(), 180, AW_BLEND);
		return 0;
	}

	unsigned long grouptitlebaranimation(LPVOID lpParam) {
		Sleep(750);
		for (int m = 1; m <= 32; m++) {
			dframe = m;
			SendMessageW(wnd->GetHWND(), WM_USER + 12, NULL, NULL);
			Sleep((int)((px[m] - px[m - 1]) * 600));
		}
		return 0;
	}
	unsigned long grouptasksanimation(LPVOID lpParam) {
		for (int m = 1; m <= 32; m++) {
			tframe = m;
			SendMessageW(wnd->GetHWND(), WM_USER + 13, NULL, NULL);
			Sleep((int)((px[m] - px[m - 1]) * 450));
		}
		return 0;
	}

	void fullscreenAnimation(int width, int height, float animstartscale) {
		Value* v{};
		RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		RECT padding = *(popupcontainer->GetPadding(&v));
		int maxwidth = dimensions.right - dimensions.left - padding.left - padding.right;
		int maxheight = dimensions.bottom - dimensions.top - padding.top - padding.bottom;
		if (width > maxwidth) width = maxwidth;
		if (height > maxheight) height = maxheight;
		parser2->CreateElement(L"fullscreeninner", NULL, NULL, NULL, (Element**)&fullscreeninner);
		centered->Add((Element**)&fullscreeninner, 1);
		static const int savedanim = centered->GetAnimation();
		static const int savedanim2 = fullscreeninner->GetAnimation();
		PlaySimpleViewAnimation(centered, width, height, savedanim, animstartscale);
		PlaySimpleViewAnimation(fullscreeninner, width, height, savedanim2, animstartscale);
		centered->SetBackgroundColor(0);
		fullscreenpopupbase->SetVisible(true);
		fullscreeninner->SetVisible(true);
		if (!editmode) BlurBackground(subviewwnd->GetHWND(), true, true);
		HANDLE AnimHandle = CreateThread(0, 0, AnimateWindowWrapper2, NULL, NULL, NULL);
		issubviewopen = true;
	}
	void fullscreenAnimation2() {
		DWORD animThread;
		HANDLE animThreadHandle = CreateThread(0, 0, animate6, NULL, 0, &animThread);
	}
	void ShowPopupCore() {
		fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor, 0.9);
		HANDLE AnimHandle = CreateThread(0, 0, AnimateWindowWrapper2, NULL, NULL, NULL);
	}
	void HidePopupCore(bool WinDInvoked) {
		if (!WinDInvoked) SendMessageW(hWndTaskbar, WM_COMMAND, 416, 0);
		if (issubviewopen) {
			centered->SetWidth(centered->GetWidth() * 0.85);
			centered->SetHeight(centered->GetHeight() * 0.85);
			fullscreeninner->SetWidth(fullscreeninner->GetWidth() * 0.85);
			fullscreeninner->SetHeight(fullscreeninner->GetHeight() * 0.85);
		}
		if (issettingsopen && atleastonesetting) {
			DDNotificationBanner* ddnb{};
			DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_SUCCESS, L"DDNB", LoadStrFromRes(4042).c_str(), NULL, 3, false);
		}
		issubviewopen = false;
		issettingsopen = false;
		atleastonesetting = false;
		mainContainer->SetVisible(true);
		SetWindowPos(subviewwnd->GetHWND(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		fullscreenAnimation2();
	}

	unsigned long PositionCheckCircle(LPVOID lpParam) {
		InitThread(TSM_DESKTOP_DYNAMIC);
		Sleep(250);
		yValuePtrs* yV = (yValuePtrs*)lpParam;
		DDScalableElement* DDCPCC = regElem<DDScalableElement*>(L"DDColorPicker_CheckedCircle", (DDColorPicker*)yV->ptr1);
		int btnX = ((DDColorPicker*)yV->ptr1)->GetWidth() / 8;
		int order = ((DDScalableElement*)yV->ptr2)->GetGroupColor() * btnX;
		int checkedBtnX = (localeType == 1) ? ((DDColorPicker*)yV->ptr1)->GetWidth() - order - DDCPCC->GetWidth() : order;
		SendMessageW(wnd->GetHWND(), WM_USER + 23, (WPARAM)DDCPCC, checkedBtnX);
		return 0;
	}

	void CloseCustomizePage(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			Element* groupdirectory = elem->GetParent()->GetParent()->GetParent();
			Element* customizegroup = regElem<Element*>(L"customizegroup", groupdirectory);
			DDScalableElement* dirname = regElem<DDScalableElement*>(L"dirname", groupdirectory);
			DDScalableElement* dirdetails = regElem<DDScalableElement*>(L"dirdetails", groupdirectory);
			Element* tasks = regElem<Element*>(L"tasks", groupdirectory);
			DDScalableButton* More = regElem<DDScalableButton*>(L"More", groupdirectory);
			TouchScrollViewer* groupdirlist = regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory);
			Element* Group_BackContainer = regElem<Element*>(L"Group_BackContainer", groupdirectory);
			DDScalableButton* Group_Back = regElem<DDScalableButton*>(L"Group_Back", groupdirectory);
			Group_BackContainer->SetLayoutPos(-3);
			Group_Back->SetVisible(false);
			dirname->SetContentString((((DDLVActionButton*)elem)->GetAssociatedItem())->GetSimpleFilename().c_str());
			dirdetails->SetLayoutPos(3);
			tasks->SetVisible(true);
			More->SetVisible(true);
			groupdirlist->SetLayoutPos(4);
			if (customizegroup) { // this probably causes memory leak
				customizegroup->SetLayoutPos(-3);
				customizegroup->Destroy(true);
			}
		}
	}
	void OpenCustomizePage(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			Element* customizegroup;
			Element* groupdirectory = elem->GetParent()->GetParent()->GetParent();
			DDScalableElement* dirname = regElem<DDScalableElement*>(L"dirname", groupdirectory);
			DDScalableElement* dirdetails = regElem<DDScalableElement*>(L"dirdetails", groupdirectory);
			Element* tasks = regElem<Element*>(L"tasks", groupdirectory);
			TouchScrollViewer* groupdirlist = regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory);
			Element* Group_BackContainer = regElem<Element*>(L"Group_BackContainer", groupdirectory);
			DDLVActionButton* Group_Back = regElem<DDLVActionButton*>(L"Group_Back", groupdirectory);
			DDScalableButton* More = regElem<DDScalableButton*>(L"More", groupdirectory);
			Group_BackContainer->SetLayoutPos(0);
			WCHAR backTo[256];
			StringCchPrintfW(backTo, 256, LoadStrFromRes(49856, L"shell32.dll").c_str(), ((DDLVActionButton*)elem)->GetAssociatedItem()->GetSimpleFilename().c_str());
			Group_Back->SetVisible(true);
			Group_Back->SetAssociatedItem(((DDLVActionButton*)elem)->GetAssociatedItem());
			Group_Back->SetAccDesc(backTo);
			assignFn(Group_Back, CloseCustomizePage);
			dirname->SetContentString(LoadStrFromRes(4027).c_str());
			dirdetails->SetLayoutPos(-3);
			tasks->SetVisible(false);
			More->SetVisible(false);
			groupdirlist->SetLayoutPos(-3);
			parser2->CreateElement(L"customizegroup", NULL, groupdirectory, NULL, &customizegroup);
			groupdirectory->Add(&customizegroup, 1);
			customizegroup->SetID(L"customizegroup");
			DDColorPicker* DDCP_Group = regElem<DDColorPicker*>(L"DDCP_Group", customizegroup);
			DDCP_Group->SetThemeAwareness(true);
			vector<DDScalableElement*> btnTargets{};
			if (issubviewopen) btnTargets.push_back((DDScalableElement*)fullscreeninner);
			DDScalableElement* iconElement = regElem<DDScalableElement*>(L"iconElem", ((DDLVActionButton*)elem)->GetAssociatedItem());
			btnTargets.push_back(iconElement);
			DDCP_Group->SetTargetElements(btnTargets);
			btnTargets.clear();
			yValuePtrs* yV = new yValuePtrs{ (void*)DDCP_Group, (void*)iconElement };
			HANDLE checkedCircleThread = CreateThread(0, 0, PositionCheckCircle, (LPVOID)yV, 0, NULL);
		}
	}

	void PinGroup(Element* elem, Event* iev) {
		static int i{};
		//static BYTE lp_inner = -5, lp_cb{}, lp_file{}, lp_shadow{};
		if (iev->uidType == Button::Click) {
			LVItem* lviTarget = ((DDLVActionButton*)elem)->GetAssociatedItem();
			for (i = 0; i < pm.size(); i++) {
				if (lviTarget == pm[i]) break;
			}
			Element* innerElem = regElem<Element*>(L"innerElem", lviTarget);
			if (lviTarget->GetGroupSize() == LVIGS_NORMAL) {
				//lp_inner = innerElem->GetLayoutPos();
				//lp_cb = cbpm[i]->GetLayoutPos();
				//lp_file = filepm[i]->GetLayoutPos();
				//lp_shadow = fileshadowpm[i]->GetLayoutPos();
				HidePopupCore(false);
				lviTarget->SetGroupSize(LVIGS_MEDIUM);
			}
			else {
				lviTarget->SetGroupSize(LVIGS_NORMAL);
				lviTarget->SetSizedFromGroup(true);
				if (touchmode) lviTarget->SetDrawType(1);
				if (localeType == 1) lviTarget->SetX(lviTarget->GetX() + lviTarget->GetWidth());
				//if (lp_inner == -5) {
				//    Element* g_innerElem = regElem<Element*>(L"innerElem", g_outerElem);
				//    Element* checkboxElem = regElem<Element*>(L"checkboxElem", g_outerElem);
				//    innerElem->SetLayoutPos(g_innerElem->GetLayoutPos()), cbpm[i]->SetLayoutPos(checkboxElem->GetLayoutPos());
				//}
				//else {
				//    innerElem->SetLayoutPos(lp_inner), cbpm[i]->SetLayoutPos(lp_cb);
				//    filepm[i]->SetLayoutPos(lp_file), fileshadowpm[i]->SetLayoutPos(lp_shadow);
				//}
				lviTarget->SetTooltip(true);
				yValue* yV = new yValue{ i };
				HANDLE smThumbnailThreadHandle = CreateThread(0, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, NULL);
			}
			IconThumbHelper(i);
			RearrangeIcons(true, false, true);
			lviTarget->SetRefreshState(true);
		}
	}

	void AdjustGroupSize(Element* elem, Event* iev) {
		static int i{};
		if (iev->uidType == Button::Click) {
			LVItem* lviTarget = ((DDLVActionButton*)elem)->GetAssociatedItem();
			for (i = 0; i < pm.size(); i++) {
				if (lviTarget == pm[i]) break;
			}
			if (elem->GetID() == StrToID(L"Smaller")) {
				lviTarget->SetGroupSize((LVItemGroupSize)((int)lviTarget->GetGroupSize() - 1));
			}
			if (elem->GetID() == StrToID(L"Larger")) {
				lviTarget->SetGroupSize((LVItemGroupSize)((int)lviTarget->GetGroupSize() + 1));
			}
			RearrangeIcons(true, false, true);
			lviTarget->SetRefreshState(true);
		}
	}

	unsigned long AutoHideMoreOptions(LPVOID lpParam) {
		Element* tasksOld = regElem<Element*>(L"tasks", (LVItem*)lpParam);
		Sleep(5000);
		if (ensureNoRefresh && ((LVItem*)lpParam)->GetGroupSize() != LVIGS_NORMAL) {
			Element* tasks = regElem<Element*>(L"tasks", (LVItem*)lpParam);
			Element* More = regElem<Element*>(L"More", (LVItem*)lpParam);
			if (tasks == tasksOld) SendMessageW(wnd->GetHWND(), WM_USER + 25, (WPARAM)tasks, (LPARAM)More);
		}
		return 0;
	}
	void ShowMoreOptions(Element* elem, Event* iev) {
		HANDLE hAutoHide{};
		if (iev->uidType == Button::Click) {
			ensureNoRefresh = true;
			elem->SetLayoutPos(-3);
			Element* tasks = regElem<Element*>(L"tasks", ((DDLVActionButton*)elem)->GetAssociatedItem());
			tasks->SetLayoutPos(2);
			hAutoHide = CreateThread(0, 0, AutoHideMoreOptions, (LPVOID)((DDLVActionButton*)elem)->GetAssociatedItem(), 0, NULL);
		}
	}

	void OpenGroupInExplorer(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			SHELLEXECUTEINFOW execInfo = {};
			execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
			execInfo.lpVerb = L"open";
			execInfo.nShow = SW_SHOWNORMAL;
			wstring fileStr = (((DDLVActionButton*)elem)->GetAssociatedItem())->GetFilename();
			LPCWSTR file = fileStr.c_str();
			execInfo.lpFile = file;
			ShellExecuteExW(&execInfo);
		}
	}

	unsigned long DoubleClickHandler(LPVOID lpParam) {
		wchar_t* dcms = GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"DoubleClickSpeed");
		Sleep(_wtoi(dcms));
		*((int*)lpParam) = 1;
		return 0;
	}

	void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB) {
		RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		pageElem->SetX(dimensions.right * offsetL);
		pageElem->SetY(dimensions.bottom * offsetT);
		pageElem->SetWidth(dimensions.right * offsetR);
		pageElem->SetHeight(dimensions.bottom * offsetB);
		WCHAR currentPage[64];
		StringCchPrintfW(currentPage, 64, LoadStrFromRes(4026).c_str(), currentPageID, maxPageID);
		pageinfo->SetContentString(currentPage);
	}
	void GoToPrevPage(Element* elem, Event* iev) {
		static const int savedanim = UIContainer->GetAnimation();
		static RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		if (iev->uidType == TouchButton::Click) {
			currentPageID--;
			for (int items = 0; items < pm.size(); items++) {
				if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
				else pm[items]->SetVisible(false);
			}
			if (editmode) {
				invokedpagechange = true;
				PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
				PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
			}
			else {
				UIContainer->SetAnimation(NULL);
				short animSrc = (localeType == 1) ? 1 : -1;
				UIContainer->SetX((dimensions.right - dimensions.left) * animSrc);
				UIContainer->SetAnimation(savedanim);
				UIContainer->SetX(0);
			}
			nextpageMain->SetVisible(true);
			if (currentPageID == 1) prevpageMain->SetVisible(false);
		}
		if (iev->uidType == LVItem::Click && elem->GetMouseFocused() == true) {
			currentPageID = ((LVItem*)elem)->GetPage();
			for (int items = 0; items < pm.size(); items++) {
				if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
				else pm[items]->SetVisible(false);
			}
			invokedpagechange = true;
			float xLoc = (localeType == 1) ? -0.4 : 0.9;
			float xLoc2 = (localeType == 1) ? 0.9 : -0.4;
			TogglePage(nextpage, xLoc, 0.25, 0.5, 0.5);
			if (currentPageID == 1) TogglePage(prevpage, xLoc2, 0.25, 0, 0.5);
			PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
			PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
			nextpageMain->SetVisible(true);
			if (currentPageID == 1) prevpageMain->SetVisible(false);
		}
	}
	void GoToNextPage(Element* elem, Event* iev) {
		static const int savedanim = UIContainer->GetAnimation();
		static RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		if (iev->uidType == TouchButton::Click) {
			currentPageID++;
			for (int items = 0; items < pm.size(); items++) {
				if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
				else pm[items]->SetVisible(false);
			}
			if (editmode) {
				invokedpagechange = true;
				PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
				PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
			}
			else {
				UIContainer->SetAnimation(NULL);
				short animSrc = (localeType == 1) ? -1 : 1;
				UIContainer->SetX((dimensions.right - dimensions.left) * animSrc);
				UIContainer->SetAnimation(savedanim);
				UIContainer->SetX(0);
			}
			prevpageMain->SetVisible(true);
			if (currentPageID == maxPageID) nextpageMain->SetVisible(false);
		}
		if (iev->uidType == LVItem::Click && elem->GetMouseFocused() == true) {
			currentPageID = ((LVItem*)elem)->GetPage();
			for (int items = 0; items < pm.size(); items++) {
				if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(!hiddenIcons);
				else pm[items]->SetVisible(false);
			}
			invokedpagechange = true;
			float xLoc = (localeType == 1) ? -0.4 : 0.9;
			float xLoc2 = (localeType == 1) ? 0.9 : -0.4;
			TogglePage(prevpage, xLoc, 0.25, 0.5, 0.5);
			if (currentPageID == maxPageID) TogglePage(nextpage, xLoc2, 0.25, 0, 0.5);
			PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
			PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
			prevpageMain->SetVisible(true);
			if (currentPageID == maxPageID) nextpageMain->SetVisible(false);
		}
	}

	unsigned long ApplyThumbnailIcons(LPVOID lpParam) {
		Sleep(150);
		PostMessageW(wnd->GetHWND(), WM_USER + 14, NULL, NULL);
		return 0;
	}

	unsigned long CreateIndividualThumbnail(LPVOID lpParam) {
		yValue* yV = (yValue*)lpParam;
		if (!treatdirasgroup || pm[yV->y]->GetGroupSize() != LVIGS_NORMAL) {
			return 1;
		}
		int padding = 3, paddingInner = 2;
		if (globaliconsz > 96) {
			padding = 18;
			paddingInner = 12;
		}
		else if (globaliconsz > 48) {
			padding = 12;
			paddingInner = 8;
		}
		else if (globaliconsz > 32) {
			padding = 6;
			paddingInner = 4;
		}
		if (pm[yV->y]->GetDirState() == true && treatdirasgroup == true) {
			int x = padding * flScaleFactor, y = padding * flScaleFactor;
			vector<ThumbIcons> strs;
			unsigned short count = 0;
			wstring folderPath = RemoveQuotes(pm[yV->y]->GetFilename());
			EnumerateFolder((LPWSTR)folderPath.c_str(), nullptr, true, &count, nullptr, 4);
			EnumerateFolderForThumbnails((LPWSTR)folderPath.c_str(), &strs, 4);
			for (int thumbs = 0; thumbs < count; thumbs++) {
				HBITMAP thumbIcon = GetShellItemImage((strs[thumbs].GetFilename()).c_str(), globalgpiconsz, globalgpiconsz);
				if (strs[thumbs].GetColorLock() == false) {
					if (isDarkIconsEnabled) {
						HBITMAP bmpOverlay = AddPaddingToBitmap(thumbIcon, 0, 0, 0, 0);
						COLORREF lightness = GetMostFrequentLightnessFromIcon(thumbIcon, globaliconsz * flScaleFactor);
						IterateBitmap(thumbIcon, UndoPremultiplication, 3, 0, 1, RGB(18, 18, 18));
						bool compEffects = (GetGValue(lightness) < 208);
						IterateBitmap(bmpOverlay, ColorToAlpha, 1, 0, 1, lightness);
						CompositeBitmaps(thumbIcon, bmpOverlay, compEffects, 0.44);
						DeleteObject(bmpOverlay);
					}
					if (isGlass && !isDarkIconsEnabled && !isColorized) {
						HBITMAP bmpOverlay = AddPaddingToBitmap(thumbIcon, 0, 0, 0, 0);
						IterateBitmap(thumbIcon, SimpleBitmapPixelHandler, 0, 0, 1, GetLightestPixel(thumbIcon));
						CompositeBitmaps(thumbIcon, bmpOverlay, true, 1);
						IterateBitmap(thumbIcon, DesaturateWhitenGlass, 1, 0, 0.4, GetLightestPixel(thumbIcon));
						DeleteObject(bmpOverlay);
					}
					if (isColorized) {
						COLORREF iconcolor = (iconColorID == 1) ? ImmersiveColor : IconColorizationColor;
						IterateBitmap(thumbIcon, EnhancedBitmapPixelHandler, 1, 0, 1, iconcolor);
					}
				}
				int xRender = (localeType == 1) ? (globaliconsz - globalgpiconsz) * flScaleFactor - x : x;
				x += ((globalgpiconsz + paddingInner) * flScaleFactor);
				ThumbnailIcon* ti = new ThumbnailIcon{ xRender, y, strs[thumbs], thumbIcon };
				if (x > (globaliconsz - globalgpiconsz) * flScaleFactor) {
					x = padding * flScaleFactor;
					y += ((globalgpiconsz + paddingInner) * flScaleFactor);
				}
				PostMessageW(wnd->GetHWND(), WM_USER + 16, (WPARAM)ti, yV->y);
			}
		}
		free(yV);
		return 0;
	}

	void ApplyIcons(vector<LVItem*> pmLVItem, vector<DDScalableElement*> pmIcon, DesktopIcon* di, bool subdirectory, int id, float scale) {
		wstring dllName{}, iconID, iconFinal;
		bool customExists = EnsureRegValueExists(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons", L"29");
		if (customExists) {
			wstring customIcon = GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons", L"29");
			size_t pathEnd = customIcon.find_last_of(L"\\");
			size_t idStart = customIcon.find(L",-");
			if (idStart == wstring::npos) {
				iconFinal = customIcon;
			}
			else if (pathEnd != wstring::npos) {
				dllName = customIcon.substr(pathEnd + 1, idStart - pathEnd - 1);
				iconID = customIcon.substr(idStart + 2, wstring::npos);
			}
			else {
				dllName = L"imageres.dll";
				iconID = L"163";
			}
		}
		else {
			dllName = L"imageres.dll";
			iconID = L"163";
		}
		bool isCustomPath = (iconFinal.length() > 1);
		HICON icoShortcut{};
		if (isCustomPath) icoShortcut = (HICON)LoadImageW(NULL, iconFinal.c_str(), IMAGE_ICON, globalshiconsz * scale * flScaleFactor, globalshiconsz * scale * flScaleFactor, LR_LOADFROMFILE);
		else icoShortcut = (HICON)LoadImageW(LoadLibraryW(dllName.c_str()), MAKEINTRESOURCE(_wtoi(iconID.c_str())), IMAGE_ICON, globalshiconsz * scale * flScaleFactor, globalshiconsz * scale * flScaleFactor, LR_SHARED);
		// The use of the 3 lines below is because we can't use a fully transparent bitmap
		static const HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"shell32.dll"), MAKEINTRESOURCE(24), IMAGE_ICON, 16, 16, LR_SHARED);
		HBITMAP dummyii = IconToBitmap(dummyi, 16, 16);
		IterateBitmap(dummyii, SimpleBitmapPixelHandler, 0, 0, 0.005, NULL);
		HBITMAP bmp{};
		if (id < pm.size()) {
			if (pm[id]->GetDirState() == false || treatdirasgroup == false || pmIcon != iconpm) bmp = GetShellItemImage(RemoveQuotes(pmLVItem[id]->GetFilename()).c_str(), globaliconsz * scale, globaliconsz * scale);
			else bmp = dummyii;
		}
		else if (treatdirasgroup == false || pmIcon != iconpm) bmp = GetShellItemImage(RemoveQuotes(pmLVItem[id]->GetFilename()).c_str(), globaliconsz * scale, globaliconsz * scale);
		else bmp = dummyii;
		HBITMAP bmpShortcut = IconToBitmap(icoShortcut, globalshiconsz * scale * flScaleFactor, globalshiconsz * scale * flScaleFactor);
		DestroyIcon(icoShortcut);
		IterateBitmap(bmpShortcut, UndoPremultiplication, 1, 0, 1, NULL);
		if (bmp != dummyii) {
			float shadowintensity = touchmode ? 0.8 : 0.33;
			HBITMAP bmpShadow = AddPaddingToBitmap(bmp, 8 * scale * flScaleFactor, 8 * scale * flScaleFactor, 8 * scale * flScaleFactor, 8 * scale * flScaleFactor);
			IterateBitmap(bmpShadow, SimpleBitmapPixelHandler, 0, (int)(4 * scale * flScaleFactor), shadowintensity, NULL);
			if (!isGlass || pmLVItem == pm) di->iconshadow = bmpShadow;
			if (isDarkIconsEnabled) {
				if (pmLVItem[id]->GetColorLock() == false) {
					HBITMAP bmpOverlay = AddPaddingToBitmap(bmp, 0, 0, 0, 0);
					COLORREF lightness = GetMostFrequentLightnessFromIcon(bmp, globaliconsz * scale * flScaleFactor);
					if (bmp != dummyii) IterateBitmap(bmp, UndoPremultiplication, 3, 0, 1, RGB(18, 18, 18));
					bool compEffects = (GetGValue(lightness) < 208);
					IterateBitmap(bmpOverlay, ColorToAlpha, 1, 0, 1, lightness);
					CompositeBitmaps(bmp, bmpOverlay, compEffects, 0.44);
					DeleteObject(bmpOverlay);
				}
				if (GetGValue(GetMostFrequentLightnessFromIcon(bmpShortcut, globaliconsz * scale * flScaleFactor)) > 208) IterateBitmap(bmpShortcut, InvertConstHue, 1, 0, 1, NULL);
			}
		}
		if (isGlass && !isDarkIconsEnabled && !isColorized && pmLVItem[id]->GetColorLock() == false) {
			if (pmLVItem == pm) {
				HDC hdc = GetDC(NULL);
				HBITMAP bmpOverlay = AddPaddingToBitmap(bmp, 0, 0, 0, 0);
				HBITMAP bmpOverlay2 = AddPaddingToBitmap(bmp, 0, 0, 0, 0);
				IterateBitmap(bmpOverlay, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
				CompositeBitmaps(bmpOverlay, bmpOverlay2, true, 0.5);
				IterateBitmap(bmp, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
				CompositeBitmaps(bmp, bmpOverlay, true, 1);
				DeleteObject(bmpOverlay);
				DeleteObject(bmpOverlay2);
				HBITMAP bmpOverlay3 = AddPaddingToBitmap(bmp, 0, 0, 0, 0);
				IterateBitmap(bmpOverlay3, DesaturateWhitenGlass, 1, 0, 0.4, 16777215);
				POINT iconmidpoint = { pm[id]->GetX() + iconpm[id]->GetX() + globaliconsz * scale * flScaleFactor / 2, pm[id]->GetY() + iconpm[id]->GetY() + globaliconsz * scale * flScaleFactor / 2 };
				IterateBitmap(bmp, DesaturateWhitenGlass, 1, 0, 1, GetLightestPixel(bmp));
				COLORREF glassColor = GetColorFromPixel(hdc, iconmidpoint);
				IncreaseBrightness(glassColor);
				IterateBitmap(bmp, StandardBitmapPixelHandler, 3, 0, 0.8, glassColor);
				CompositeBitmaps(bmp, bmpOverlay3, false, 0);
				DeleteObject(bmpOverlay3);
				ReleaseDC(NULL, hdc);
			}
			else {
				HBITMAP bmpOverlay = AddPaddingToBitmap(bmp, 0, 0, 0, 0);
				IterateBitmap(bmp, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
				CompositeBitmaps(bmp, bmpOverlay, true, 1);
				IterateBitmap(bmp, DesaturateWhitenGlass, 1, 0, 0.4, GetLightestPixel(bmp));
				DeleteObject(bmpOverlay);
			}
		}
		if (isColorized) {
			COLORREF iconcolor = (iconColorID == 1) ? ImmersiveColor : IconColorizationColor;
			if (pmLVItem[id]->GetColorLock() == false) IterateBitmap(bmp, EnhancedBitmapPixelHandler, 1, 0, 1, iconcolor);
			IterateBitmap(bmpShortcut, StandardBitmapPixelHandler, 1, 0, 1, iconcolor);
		}
		di->icon = bmp;
		di->iconshortcut = bmpShortcut;
	}
	void IconThumbHelper(int id) {
		static Value* v{};
		UpdateCache* uc{};
		if (iconpm[0] != nullptr) v = iconpm[0]->GetValue(Element::BackgroundProp, 1, uc);
		iconpm[id]->DestroyAll(true);
		iconpm[id]->SetClass(L"");
		iconpm[id]->SetValue(Element::BackgroundProp, 1, v);
		shadowpm[id]->SetVisible(true);
		int groupspace = 8 * flScaleFactor;
		if (touchmode && pm[id]->GetGroupSize() == LVIGS_NORMAL) {
			iconpm[id]->SetWidth(globaliconsz * flScaleFactor + 2 * groupspace);
			iconpm[id]->SetHeight(globaliconsz * flScaleFactor + 2 * groupspace);
		}
		Element* iconcontainer = regElem<Element*>(L"iconcontainer", pm[id]);
		if (pm[id]->GetDirState() == true && treatdirasgroup == true) {
			if (pm[id]->GetGroupSize() == LVIGS_NORMAL) iconpm[id]->SetClass(L"groupthumbnail");
			else iconpm[id]->SetClass(L"groupbackground");
			shadowpm[id]->SetVisible(false);

			if (touchmode && pm[id]->GetGroupSize() == LVIGS_NORMAL) {
				iconcontainer->SetPadding(groupspace, groupspace, groupspace, groupspace);
				iconpm[id]->SetWidth(globaliconsz * flScaleFactor);
				iconpm[id]->SetHeight(globaliconsz * flScaleFactor);
			}
		}
	}
	void UpdateTileOnColorChange(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		if (pProp == DDScalableElement::AssociatedColorProp()) {
			int i;
			for (i = 0; i < pm.size(); i++) {
				if (elem == iconpm[i]) break;
			}
			COLORREF crDefault = theme ? RGB(208, 208, 208) : RGB(48, 48, 48);
			pm[i]->SetAssociatedColor(((DDScalableElement*)elem)->GetAssociatedColor() == -1 ? crDefault : ((DDScalableElement*)elem)->GetAssociatedColor());
		}
	}

	void SelectSubItem(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			SHELLEXECUTEINFOW execInfo = {};
			execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
			execInfo.lpVerb = L"open";
			execInfo.nShow = SW_SHOWNORMAL;
			wstring temp = RemoveQuotes(((LVItem*)elem)->GetFilename());
			execInfo.lpFile = temp.c_str();
			ShellExecuteExW(&execInfo);
		}
	}

	void SelectSubItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		if (touchmode && pProp == Button::PressedProp()) {
			Element* innerElem = regElem<Element*>(L"innerElem", elem);
			innerElem->SetEnabled(!((LVItem*)elem)->GetPressed());
		}
	}

	void ShowDirAsGroup(LVItem* lvi) {
		SendMessageW(hWndTaskbar, WM_COMMAND, 419, 0);
		fullscreenAnimation(800 * flScaleFactor, 480 * flScaleFactor, 0.9);
		SetWindowPos(subviewwnd->GetHWND(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		Element* groupdirectory{};
		parser2->CreateElement(L"groupdirectory", NULL, NULL, NULL, (Element**)&groupdirectory);
		fullscreeninner->Add((Element**)&groupdirectory, 1);
		DDScalableElement* iconElement = regElem<DDScalableElement*>(L"iconElem", lvi);
		((DDScalableElement*)fullscreeninner)->SetDDCPIntensity(iconElement->GetDDCPIntensity());
		((DDScalableElement*)fullscreeninner)->SetAssociatedColor(iconElement->GetAssociatedColor());
		TouchScrollViewer* groupdirlist = regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory);
		DDScalableButton* lvi_SubUIContainer = regElem<DDScalableButton*>(L"SubUIContainer", groupdirlist);
		SetGadgetFlags(groupdirlist->GetDisplayNode(), 0x1, 0x1);
		unsigned short lviCount = 0;
		int count2{};
		EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), nullptr, true, &lviCount);
		CubicBezier(32, px, py, 0.1, 0.9, 0.2, 1.0);
		if (lviCount <= 192 && lviCount > 0) {
			vector<LVItem*>* subpm = new vector<LVItem*>;
			vector<DDScalableElement*>* subiconpm = new vector<DDScalableElement*>;
			vector<Element*>* subshadowpm = new vector<Element*>;
			vector<Element*>* subshortpm = new vector<Element*>;
			vector<RichText*>* subfilepm = new vector<RichText*>;
			StyleSheet* sheet = pMain->GetSheet();
			Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
			parser->GetSheet(theme ? L"default" : L"defaultdark", &sheetStorage);
			for (int i = 0; i < lviCount; i++) {
				LVItem* outerElemGrouped;
				if (touchmode) {
					parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElemGrouped);
				}
				else parser->CreateElement(L"outerElemGrouped", NULL, NULL, NULL, (Element**)&outerElemGrouped);
				outerElemGrouped->SetValue(Element::SheetProp, 1, sheetStorage);
				lvi_SubUIContainer->Add((Element**)&outerElemGrouped, 1);
				DDScalableElement* iconElem = regElem<DDScalableElement*>(L"iconElem", outerElemGrouped);
				Element* shortcutElem = regElem<Element*>(L"shortcutElem", outerElemGrouped);
				Element* iconElemShadow = regElem<Element*>(L"iconElemShadow", outerElemGrouped);
				RichText* textElem = regElem<RichText*>(L"textElem", outerElemGrouped);
				subpm->push_back(outerElemGrouped);
				subiconpm->push_back(iconElem);
				subshortpm->push_back(shortcutElem);
				subshadowpm->push_back(iconElemShadow);
				subfilepm->push_back(textElem);
				outerElemGrouped->SetAnimation(NULL);
			}
			sheetStorage->Release();
			EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), subpm, false, nullptr, &count2, lviCount);
			int x = 0, y = 0;
			int maxX{}, xRuns{};
			Value* v;
			RECT dimensions;
			dimensions = *(groupdirectory->GetPadding(&v));
			int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
			int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 21) * flScaleFactor;
			if (touchmode) {
				outerSizeX = touchSizeX + DESKPADDING_TOUCH * flScaleFactor;
				outerSizeY = touchSizeY + DESKPADDING_TOUCH * flScaleFactor;
			}
			DWORD* animThread = new DWORD[lviCount];
			DWORD* animThread2 = new DWORD[lviCount];
			HANDLE* animThreadHandle = new HANDLE[lviCount];
			HANDLE* animThreadHandle2 = new HANDLE[lviCount];
			for (int j = 0; j < lviCount; j++) {
				if ((*subpm)[j]->GetHiddenState() == true) {
					(*subiconpm)[j]->SetAlpha(128);
					(*subshadowpm)[j]->SetAlpha(0);
					(*subfilepm)[j]->SetAlpha(128);
				}
				assignFn((*subpm)[j], SelectSubItem);
				assignFn((*subpm)[j], ItemRightClick);
				assignExtendedFn((*subpm)[j], SelectSubItemListener);
				if (!touchmode) (*subpm)[j]->SetClass(L"singleclicked");
				int xRender = (localeType == 1) ? (centered->GetWidth() - (dimensions.left + dimensions.right + outerSizeX)) - x : x;
				(*subpm)[j]->SetX(xRender), (*subpm)[j]->SetY(y);
				yValueEx* yV = new yValueEx{ j, NULL, NULL, subpm, subiconpm, subshadowpm, subshortpm, subfilepm };
				x += outerSizeX;
				xRuns++;
				if (x > centered->GetWidth() - (dimensions.left + dimensions.right + outerSizeX)) {
					maxX = xRuns;
					xRuns = 0;
					x = 0;
					y += outerSizeY;
				}
				animThreadHandle[j] = CreateThread(0, 0, subanimate, (LPVOID)yV, 0, &(animThread[j]));
				animThreadHandle2[j] = CreateThread(0, 0, subfastin, (LPVOID)yV, 0, &(animThread2[j]));
			}
			x -= outerSizeX;
			if (maxX != 0 && xRuns % maxX != 0) y += outerSizeY;
			lvi_SubUIContainer->SetHeight(y);
			Element* dirtitle = regElem<Element*>(L"dirtitle", groupdirectory);
			for (int j = 0; j < lviCount; j++) {
				if (localeType == 1 && y > (480 * flScaleFactor - (dirtitle->GetHeight() + dimensions.top + dimensions.bottom)))
					(*subpm)[j]->SetX((*subpm)[j]->GetX() - GetSystemMetricsForDpi(SM_CXVSCROLL, dpi));
			}
			delete[] animThread;
			delete[] animThread2;
			delete[] animThreadHandle;
			delete[] animThreadHandle2;
			v->Release();
			lvi->SetChildItems((*subpm));
			lvi->SetChildIcons((*subiconpm));
			lvi->SetChildShadows((*subshadowpm));
			lvi->SetChildShortcutArrows((*subshortpm));
			lvi->SetChildFilenames((*subfilepm));
		}
		else {
			if (lviCount > 128) {
				lvi_SubUIContainer->SetContentString(LoadStrFromRes(4030).c_str());
			}
			else lvi_SubUIContainer->SetContentString(LoadStrFromRes(4029).c_str());
		}
		dirnameanimator = regElem<Element*>(L"dirnameanimator", groupdirectory);
		tasksanimator = regElem<Element*>(L"tasksanimator", groupdirectory);
		DDScalableElement* dirname = regElem<DDScalableElement*>(L"dirname", groupdirectory);
		dirname->SetContentString(lvi->GetSimpleFilename().c_str());
		dirname->SetAlpha(255);
		DDScalableElement* dirdetails = regElem<DDScalableElement*>(L"dirdetails", groupdirectory);
		WCHAR itemCount[64];
		if (lviCount == 1) StringCchPrintfW(itemCount, 64, LoadStrFromRes(4031).c_str());
		else StringCchPrintfW(itemCount, 64, LoadStrFromRes(4032).c_str(), lviCount);
		dirdetails->SetContentString(itemCount);
		dirdetails->SetAlpha(theme ? 108 : 144);
		if (lviCount == 0) dirdetails->SetLayoutPos(-3);
		Element* tasks = regElem<Element*>(L"tasks", groupdirectory);
		checkifelemexists = true;
		DWORD animThread3;
		DWORD animThread4;
		HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
		HANDLE animThreadHandle4 = CreateThread(0, 0, grouptasksanimation, NULL, 0, &animThread4);
		DDLVActionButton* Pin = regElem<DDLVActionButton*>(L"Pin", groupdirectory);
		DDLVActionButton* Customize = regElem<DDLVActionButton*>(L"Customize", groupdirectory);
		DDLVActionButton* OpenInExplorer = regElem<DDLVActionButton*>(L"OpenInExplorer", groupdirectory);
		Pin->SetVisible(true), Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
		assignFn(OpenInExplorer, OpenGroupInExplorer);
		assignFn(Customize, OpenCustomizePage);
		assignFn(Pin, PinGroup);
		OpenInExplorer->SetAssociatedItem(lvi);
		Customize->SetAssociatedItem(lvi);
		Pin->SetAssociatedItem(lvi);
	}

	void ShowDirAsGroupDesktop(LVItem* lvi) {
		Element* groupdirectoryOld = regElem<Element*>(L"groupdirectory", lvi);
		if (groupdirectoryOld) return;
		StyleSheet* sheet = pSubview->GetSheet();
		Value* sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
		parser2->GetSheet(theme ? L"popup" : L"popupdark", &sheetStorage);
		Element* groupdirectory{};
		parser2->CreateElement(L"groupdirectory", NULL, lvi, NULL, (Element**)&groupdirectory);
		groupdirectory->SetValue(Element::SheetProp, 1, sheetStorage);
		groupdirectory->SetID(L"groupdirectory");
		lvi->Add((Element**)&groupdirectory, 1);
		TouchScrollViewer* groupdirlist = regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory);
		DDScalableButton* lvi_SubUIContainer = regElem<DDScalableButton*>(L"SubUIContainer", groupdirlist);
		SetGadgetFlags(groupdirlist->GetDisplayNode(), 0x1, 0x1);
		unsigned short lviCount = 0;
		int count2{};
		EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), nullptr, true, &lviCount);
		if (lviCount <= 192 && lviCount > 0) {
			vector<LVItem*>* d_subpm = new vector<LVItem*>;
			vector<DDScalableElement*>* d_subiconpm = new vector<DDScalableElement*>;
			vector<Element*>* d_subshadowpm = new vector<Element*>;
			vector<Element*>* d_subshortpm = new vector<Element*>;
			vector<RichText*>* d_subfilepm = new vector<RichText*>;
			sheet = pMain->GetSheet();
			sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
			parser->GetSheet(theme ? L"default" : L"defaultdark", &sheetStorage);
			for (int i = 0; i < lviCount; i++) {
				LVItem* outerElemGrouped;
				if (touchmode) {
					parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElemGrouped);
				}
				else parser->CreateElement(L"outerElemGrouped", NULL, NULL, NULL, (Element**)&outerElemGrouped);
				outerElemGrouped->SetValue(Element::SheetProp, 1, sheetStorage);
				lvi_SubUIContainer->Add((Element**)&outerElemGrouped, 1);
				DDScalableElement* iconElem = regElem<DDScalableElement*>(L"iconElem", outerElemGrouped);
				Element* shortcutElem = regElem<Element*>(L"shortcutElem", outerElemGrouped);
				Element* iconElemShadow = regElem<Element*>(L"iconElemShadow", outerElemGrouped);
				RichText* textElem = regElem<RichText*>(L"textElem", outerElemGrouped);
				d_subpm->push_back(outerElemGrouped);
				d_subiconpm->push_back(iconElem);
				d_subshortpm->push_back(shortcutElem);
				d_subshadowpm->push_back(iconElemShadow);
				d_subfilepm->push_back(textElem);
				outerElemGrouped->SetAnimation(NULL);
			}
			sheetStorage->Release();
			EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), &(*d_subpm), false, nullptr, &count2, lviCount);
			int x = 0, y = 0;
			int maxX{}, xRuns{};
			Value* v;
			RECT dimensions;
			dimensions = *(groupdirectory->GetPadding(&v));
			int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
			int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 21) * flScaleFactor;
			if (touchmode) {
				outerSizeX = touchSizeX + DESKPADDING_TOUCH * flScaleFactor;
				outerSizeY = touchSizeY + DESKPADDING_TOUCH * flScaleFactor;
			}
			DWORD* animThread = new DWORD[lviCount];
			DWORD* animThread2 = new DWORD[lviCount];
			HANDLE* animThreadHandle = new HANDLE[lviCount];
			HANDLE* animThreadHandle2 = new HANDLE[lviCount];
			for (int j = 0; j < lviCount; j++) {
				if ((*d_subpm)[j]->GetHiddenState() == true) {
					(*d_subiconpm)[j]->SetAlpha(128);
					(*d_subshadowpm)[j]->SetAlpha(0);
					(*d_subfilepm)[j]->SetAlpha(128);
				}
				assignFn((*d_subpm)[j], SelectSubItem);
				assignFn((*d_subpm)[j], ItemRightClick);
				assignExtendedFn((*d_subpm)[j], SelectSubItemListener);
				if (!touchmode) (*d_subpm)[j]->SetClass(L"singleclicked");
				int xRender = (localeType == 1) ? (lvi->GetWidth() - (dimensions.left + dimensions.right + outerSizeX)) - x : x;
				(*d_subpm)[j]->SetX(xRender), (*d_subpm)[j]->SetY(y);
				yValueEx* yV = new yValueEx{ j, NULL, NULL, d_subpm, d_subiconpm, d_subshadowpm, d_subshortpm, d_subfilepm };
				x += outerSizeX;
				xRuns++;
				if (x > lvi->GetWidth() - (dimensions.left + dimensions.right + outerSizeX + GetSystemMetricsForDpi(SM_CXVSCROLL, dpi))) {
					maxX = xRuns;
					xRuns = 0;
					x = 0;
					y += outerSizeY;
				}
				animThreadHandle[j] = CreateThread(0, 0, subanimate, (LPVOID)yV, 0, &(animThread[j]));
				animThreadHandle2[j] = CreateThread(0, 0, subfastin, (LPVOID)yV, 0, &(animThread2[j]));
			}
			x -= outerSizeX;
			if (maxX != 0 && xRuns % maxX != 0) y += outerSizeY;
			lvi_SubUIContainer->SetHeight(y);
			Element* dirtitle = regElem<Element*>(L"dirtitle", groupdirectory);
			for (int j = 0; j < lviCount; j++) {
				if (localeType == 1 && y > (lvi->GetHeight() - (dirtitle->GetHeight() + dimensions.top + dimensions.bottom)))
					(*d_subpm)[j]->SetX((*d_subpm)[j]->GetX() - GetSystemMetricsForDpi(SM_CXVSCROLL, dpi));
			}
			delete[] animThread;
			delete[] animThread2;
			delete[] animThreadHandle;
			delete[] animThreadHandle2;
			v->Release();
			lvi->SetChildItems((*d_subpm));
			lvi->SetChildIcons((*d_subiconpm));
			lvi->SetChildShadows((*d_subshadowpm));
			lvi->SetChildShortcutArrows((*d_subshortpm));
			lvi->SetChildFilenames((*d_subfilepm));
		}
		else {
			if (lviCount > 128) {
				lvi_SubUIContainer->SetContentString(LoadStrFromRes(4030).c_str());
			}
			else lvi_SubUIContainer->SetContentString(LoadStrFromRes(4029).c_str());
		}
		DDScalableElement* dirname = regElem<DDScalableElement*>(L"dirname", groupdirectory);
		dirname->SetContentString(lvi->GetSimpleFilename().c_str());
		dirname->SetAlpha(255);
		DDScalableElement* dirdetails = regElem<DDScalableElement*>(L"dirdetails", groupdirectory);
		WCHAR itemCount[64];
		if (lviCount == 1) StringCchPrintfW(itemCount, 64, LoadStrFromRes(4031).c_str());
		else StringCchPrintfW(itemCount, 64, LoadStrFromRes(4032).c_str(), lviCount);
		dirdetails->SetContentString(itemCount);
		dirdetails->SetAlpha(theme ? 108 : 144);
		if (lviCount == 0) dirdetails->SetLayoutPos(-3);
		DDLVActionButton* More = regElem<DDLVActionButton*>(L"More", groupdirectory);
		Element* tasks = regElem<Element*>(L"tasks", groupdirectory);
		tasks->SetLayoutPos(-3);
		DDLVActionButton* Smaller = regElem<DDLVActionButton*>(L"Smaller", groupdirectory);
		DDLVActionButton* Larger = regElem<DDLVActionButton*>(L"Larger", groupdirectory);
		DDLVActionButton* Unpin = regElem<DDLVActionButton*>(L"Unpin", groupdirectory);
		DDLVActionButton* Customize = regElem<DDLVActionButton*>(L"Customize", groupdirectory);
		DDLVActionButton* OpenInExplorer = regElem<DDLVActionButton*>(L"OpenInExplorer", groupdirectory);
		More->SetLayoutPos(2);
		Smaller->SetVisible(true), Larger->SetVisible(true), Unpin->SetVisible(true), Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
		assignFn(More, ShowMoreOptions);
		assignFn(Smaller, AdjustGroupSize);
		assignFn(Larger, AdjustGroupSize);
		assignFn(OpenInExplorer, OpenGroupInExplorer);
		assignFn(Customize, OpenCustomizePage);
		assignFn(Unpin, PinGroup);
		if (lvi->GetGroupSize() == LVIGS_SMALL) Smaller->SetEnabled(false);
		if (lvi->GetGroupSize() == LVIGS_LARGE) Larger->SetEnabled(false);
		More->SetAssociatedItem(lvi);
		Smaller->SetAssociatedItem(lvi);
		Larger->SetAssociatedItem(lvi);
		OpenInExplorer->SetAssociatedItem(lvi);
		Customize->SetAssociatedItem(lvi);
		Unpin->SetAssociatedItem(lvi);
	}

	void OpenDeskCpl(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) ShellExecuteW(NULL, L"open", L"control.exe", L"desk.cpl,Web,0", NULL, SW_SHOW);
	}
	void OpenLog(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			wchar_t* desktoplog = new wchar_t[260];
			wchar_t* cBuffer = new wchar_t[260];
			DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
			StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
			ShellExecuteW(NULL, L"open", L"notepad.exe", desktoplog, NULL, SW_SHOW);
			delete[] desktoplog;
			delete[] cBuffer;
		}
	}
	void DisableColorPicker(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			DDColorPicker* DDCP_Icons = regElem<DDColorPicker*>(L"DDCP_Icons", (Element*)tempElem);
			DDCP_Icons->SetEnabled(((DDToggleButton*)elem)->GetCheckedState());
		}
	}
	void DisableDarkToggle(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			DDToggleButton* EnableDarkIcons = regElem<DDToggleButton*>(L"EnableDarkIcons", (Element*)tempElem);
			if (((DDCheckBox*)elem)->GetCheckedState() == true) {
				EnableDarkIcons->SetCheckedState(!theme);
				isDarkIconsEnabled = !theme;
				SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"DarkIcons", !theme, false, nullptr);
			}
			EnableDarkIcons->SetEnabled(!((DDCheckBox*)elem)->GetCheckedState());
		}
	}
	void ShowPage1(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			PageTab1->SetSelected(true);
			PageTab2->SetSelected(false);
			PageTab3->SetSelected(false);
			SubUIContainer->DestroyAll(true);
			Element* SettingsPage1;
			parser2->CreateElement(L"SettingsPage1", NULL, NULL, NULL, (Element**)&SettingsPage1);
			SubUIContainer->Add((Element**)&SettingsPage1, 1);
			DDToggleButton* ItemCheckboxes = regElem<DDToggleButton*>(L"ItemCheckboxes", SettingsPage1);
			DDToggleButton* ShowHiddenFiles = regElem<DDToggleButton*>(L"ShowHiddenFiles", SettingsPage1);
			DDToggleButton* FilenameExts = regElem<DDToggleButton*>(L"FilenameExts", SettingsPage1);
			DDToggleButton* TreatDirAsGroup = regElem<DDToggleButton*>(L"TreatDirAsGroup", SettingsPage1);
			DDToggleButton* TripleClickAndHide = regElem<DDToggleButton*>(L"TripleClickAndHide", SettingsPage1);
			DDToggleButton* LockIconPos = regElem<DDToggleButton*>(L"LockIconPos", SettingsPage1);
			RegKeyValue rkvTemp{};
			rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
			rkvTemp._valueToFind = L"AutoCheckSelect";
			ItemCheckboxes->SetCheckedState(showcheckboxes);
			ItemCheckboxes->SetAssociatedBool(&showcheckboxes);
			ItemCheckboxes->SetRegKeyValue(rkvTemp);
			rkvTemp._valueToFind = L"Hidden";
			if (GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind) == 1) ShowHiddenFiles->SetCheckedState(true);
			else ShowHiddenFiles->SetCheckedState(false);
			ShowHiddenFiles->SetAssociatedFn(InitLayout);
			ShowHiddenFiles->SetRegKeyValue(rkvTemp);
			rkvTemp._valueToFind = L"HideFileExt";
			FilenameExts->SetCheckedState(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
			FilenameExts->SetAssociatedFn(InitLayout);
			FilenameExts->SetRegKeyValue(rkvTemp);
			rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"TreatDirAsGroup";
			TreatDirAsGroup->SetCheckedState(treatdirasgroup);
			TreatDirAsGroup->SetAssociatedBool(&treatdirasgroup);
			TreatDirAsGroup->SetAssociatedFn(InitLayout);
			TreatDirAsGroup->SetRegKeyValue(rkvTemp);
			rkvTemp._valueToFind = L"TripleClickAndHide";
			TripleClickAndHide->SetCheckedState(tripleclickandhide);
			TripleClickAndHide->SetAssociatedBool(&tripleclickandhide);
			TripleClickAndHide->SetRegKeyValue(rkvTemp);
			rkvTemp._valueToFind = L"LockIconPos";
			LockIconPos->SetCheckedState(lockiconpos);
			LockIconPos->SetAssociatedBool(&lockiconpos);
			LockIconPos->SetRegKeyValue(rkvTemp);
			assignFn(ItemCheckboxes, ToggleSetting);
			assignFn(ShowHiddenFiles, ToggleSetting);
			assignFn(FilenameExts, ToggleSetting);
			assignFn(TreatDirAsGroup, ToggleSetting);
			assignFn(TripleClickAndHide, ToggleSetting);
			assignFn(LockIconPos, ToggleSetting);
		}
	}
	void ShowPage2(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			PageTab1->SetSelected(false);
			PageTab2->SetSelected(true);
			PageTab3->SetSelected(false);
			SubUIContainer->DestroyAll(true);
			Element* SettingsPage2;
			parser2->CreateElement(L"SettingsPage2", NULL, NULL, NULL, (Element**)&SettingsPage2);
			SubUIContainer->Add((Element**)&SettingsPage2, 1);
			DDToggleButton* EnableAccent = regElem<DDToggleButton*>(L"EnableAccent", SettingsPage2);
			DDColorPicker* DDCP_Icons = regElem<DDColorPicker*>(L"DDCP_Icons", SettingsPage2);
			DDToggleButton* EnableDarkIcons = regElem<DDToggleButton*>(L"EnableDarkIcons", SettingsPage2);
			DDCheckBox* AutoDarkIcons = regElem<DDCheckBox*>(L"AutoDarkIcons", SettingsPage2);
			DDToggleButton* IconThumbnails = regElem<DDToggleButton*>(L"IconThumbnails", SettingsPage2);
			DDScalableButton* DesktopIconSettings = regElem<DDScalableButton*>(L"DesktopIconSettings", SettingsPage2);
			RegKeyValue rkvTemp{};
			rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"AccentColorIcons";
			EnableAccent->SetCheckedState(isColorized);
			EnableAccent->SetAssociatedBool(&isColorized);
			EnableAccent->SetAssociatedFn(RearrangeIcons);
			EnableAccent->SetRegKeyValue(rkvTemp);
			rkvTemp._valueToFind = L"IconColorID";
			DDCP_Icons->SetThemeAwareness(false);
			DDCP_Icons->SetEnabled(isColorized);
			DDCP_Icons->SetRegKeyValue(rkvTemp);
			vector<DDScalableElement*> btnTargets{};
			btnTargets.push_back(RegistryListener);
			DDCP_Icons->SetTargetElements(btnTargets);
			btnTargets.clear();
			rkvTemp._valueToFind = L"DarkIcons";
			EnableDarkIcons->SetEnabled(!automaticDark);
			EnableDarkIcons->SetCheckedState(isDarkIconsEnabled);
			EnableDarkIcons->SetAssociatedBool(&isDarkIconsEnabled);
			EnableDarkIcons->SetAssociatedFn(RearrangeIcons);
			EnableDarkIcons->SetRegKeyValue(rkvTemp);
			rkvTemp._valueToFind = L"AutoDarkIcons";
			AutoDarkIcons->SetCheckedState(automaticDark);
			AutoDarkIcons->SetAssociatedBool(&automaticDark);
			AutoDarkIcons->SetAssociatedFn(RearrangeIcons);
			AutoDarkIcons->SetRegKeyValue(rkvTemp);
			rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", rkvTemp._valueToFind = L"IconsOnly";
			IconThumbnails->SetCheckedState(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
			IconThumbnails->SetAssociatedFn(RearrangeIcons);
			IconThumbnails->SetRegKeyValue(rkvTemp);
			assignFn(EnableAccent, ToggleSetting);
			assignFn(EnableAccent, DisableColorPicker);
			assignFn(EnableDarkIcons, ToggleSetting);
			assignFn(AutoDarkIcons, ToggleSetting);
			assignFn(AutoDarkIcons, DisableDarkToggle);
			assignFn(IconThumbnails, ToggleSetting);
			assignFn(DesktopIconSettings, OpenDeskCpl);
			tempElem = (void*)SettingsPage2;
		}
	}
	void ShowPage3(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			PageTab1->SetSelected(false);
			PageTab2->SetSelected(false);
			PageTab3->SetSelected(true);
			SubUIContainer->DestroyAll(true);
			Element* SettingsPage3;
			parser2->CreateElement(L"SettingsPage3", NULL, NULL, NULL, (Element**)&SettingsPage3);
			SubUIContainer->Add((Element**)&SettingsPage3, 1);
			DDToggleButton* EnableLogging = regElem<DDToggleButton*>(L"EnableLogging", SettingsPage3);
			DDScalableButton* ViewLastLog = regElem<DDScalableButton*>(L"ViewLastLog", SettingsPage3);
			RegKeyValue rkvTemp{};
			rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"Logging";
			EnableLogging->SetCheckedState(7 - GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
			EnableLogging->SetRegKeyValue(rkvTemp);
			assignFn(EnableLogging, ToggleSetting);
			assignFn(ViewLastLog, OpenLog);
		}
	}

	void ShowSettings(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			ShowPopupCore();
			BlurBackground(subviewwnd->GetHWND(), true, true);
			issubviewopen = true;
			issettingsopen = true;
			Element* settingsview{};
			parser2->CreateElement(L"settingsview", NULL, NULL, NULL, (Element**)&settingsview);
			fullscreeninner->Add((Element**)&settingsview, 1);
			TouchScrollViewer* settingslist = regElem<TouchScrollViewer*>(L"SettingsList", settingsview);
			SubUIContainer = regElem<DDScalableButton*>(L"SubUIContainer", settingsview);
			PageTab1 = regElem<DDScalableButton*>(L"PageTab1", settingsview);
			PageTab2 = regElem<DDScalableButton*>(L"PageTab2", settingsview);
			PageTab3 = regElem<DDScalableButton*>(L"PageTab3", settingsview);
			assignFn(PageTab1, ShowPage1);
			assignFn(PageTab2, ShowPage2);
			assignFn(PageTab3, ShowPage3);
			ShowPage1(elem, iev);
			CubicBezier(32, px, py, 0.1, 0.9, 0.2, 1.0);
			dirnameanimator = regElem<Element*>(L"dirnameanimator", settingsview);
			DDScalableElement* name = regElem<DDScalableElement*>(L"name", settingsview);
			name->SetAlpha(255);
			checkifelemexists = true;
			DWORD animThread3;
			HANDLE animThreadHandle3 = CreateThread(0, 0, grouptitlebaranimation, NULL, 0, &animThread3);
		}
	}

	Element* elemStorage;
	bool fileopened{};
	void SelectItem(Element* elem, Event* iev) {
		static int clicks = 1;
		static int validation = 0;
		if (iev->uidType == Button::Click) {
			validation++;
			Button* checkbox = regElem<Button*>(L"checkboxElem", elem);
			if (GetAsyncKeyState(VK_CONTROL) == 0 && checkbox->GetMouseFocused() == false) {
				for (int items = 0; items < pm.size(); items++) {
					pm[items]->SetSelected(false);
					if (cbpm[items]->GetSelected() == false && showcheckboxes == 1) cbpm[items]->SetVisible(false);
				}
			}
			if (elem != emptyspace && checkbox->GetMouseFocused() == false && GetAsyncKeyState(VK_CONTROL) == 0) elem->SetSelected(!elem->GetSelected());
			if (validation & 1) {
				if (elem != emptyspace && checkbox->GetMouseFocused() == true && GetAsyncKeyState(VK_CONTROL) == 0) elem->SetSelected(!elem->GetSelected());
			}
			if (showcheckboxes == 1) checkbox->SetVisible(true);
			if (shellstate[4] & 0x20 && !touchmode) {
				if (elem == elemStorage) clicks++; else clicks = 0;
				DWORD doubleClickThread{};
				HANDLE doubleClickThreadHandle = CreateThread(0, 0, DoubleClickHandler, &clicks, 0, &doubleClickThread);
				elemStorage = elem;
			}
			for (int items = 0; items < pm.size(); items++) {
				pm[items]->SetMemorySelected(pm[items]->GetSelected());
			}
			if (clicks & 1 && checkbox->GetMouseFocused() == false && ((LVItem*)elem)->GetDragState() == false) {
				wstring temp = RemoveQuotes(((LVItem*)elem)->GetFilename());
				SHELLEXECUTEINFOW execInfo = {};
				execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
				execInfo.lpVerb = L"open";
				execInfo.nShow = SW_SHOWNORMAL;
				execInfo.lpFile = temp.c_str();
				fileopened = true;
				if (((LVItem*)elem)->GetDirState() == true && treatdirasgroup == true) {
					ShowDirAsGroup((LVItem*)elem);
				}
				else ShellExecuteExW(&execInfo);
			}
		}
	}
	void SelectItem2(Element* elem, Event* iev) {
		static int validation = 0;
		if (iev->uidType == Button::Click) {
			elem->SetSelected(!elem->GetSelected());
			if (validation & 1) {
				elem->SetSelected(!elem->GetSelected());
			}
		}
	}
	void SelectItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		if (pProp == Element::SelectedProp()) {
			if (!touchmode) {
				float spacingInternal = CalcTextLines(((LVItem*)elem)->GetSimpleFilename().c_str(), elem->GetWidth() - 4 * flScaleFactor);
				int extraBottomSpacing = (elem->GetSelected() == true) ? ceil(spacingInternal) * textm.tmHeight : floor(spacingInternal) * textm.tmHeight;
				RichText* textElem = regElem<RichText*>(L"textElem", elem);
				RichText* textElemShadow = regElem<RichText*>(L"textElemShadow", elem);
				if (type == 69) {
					int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
					int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor - textm.tmHeight;
					int lines_basedOnEllipsis = ceil(CalcTextLines(((LVItem*)elem)->GetSimpleFilename().c_str(), innerSizeX - 4 * flScaleFactor)) * textm.tmHeight;
					elem->SetHeight(innerSizeY + lines_basedOnEllipsis + 6 * flScaleFactor);
					RichText* g_textElem = regElem<RichText*>(L"textElem", g_outerElem);
					RichText* g_textElemShadow = regElem<RichText*>(L"textElemShadow", g_outerElem);
					textElem->SetLayoutPos(g_textElem->GetLayoutPos());
					textElemShadow->SetLayoutPos(g_textElemShadow->GetLayoutPos());
					textElem->SetHeight(lines_basedOnEllipsis + 4 * flScaleFactor);
					textElemShadow->SetHeight(lines_basedOnEllipsis + 5 * flScaleFactor);
				}
				if (spacingInternal == 1.5) {
					if (elem->GetSelected() == true) elem->SetHeight(elem->GetHeight() + extraBottomSpacing * 0.5);
					else elem->SetHeight(elem->GetHeight() - extraBottomSpacing);
				}
				textElem->SetHeight(extraBottomSpacing + 4 * flScaleFactor);
				textElemShadow->SetHeight(extraBottomSpacing + 5 * flScaleFactor);
				HBITMAP capturedBitmap = CreateTextBitmap(((LVItem*)elem)->GetSimpleFilename().c_str(), elem->GetWidth() - 4 * flScaleFactor, extraBottomSpacing, DT_CENTER | DT_END_ELLIPSIS, false);
				IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
				HBITMAP shadowBitmap = AddPaddingToBitmap(capturedBitmap, 2 * flScaleFactor, 2 * flScaleFactor, 2 * flScaleFactor, 2 * flScaleFactor);
				IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * flScaleFactor), 2, NULL);
				Value* bitmap = DirectUI::Value::CreateGraphic(capturedBitmap, 2, 0xffffffff, false, false, false);
				Value* bitmapSh = DirectUI::Value::CreateGraphic(shadowBitmap, 2, 0xffffffff, false, false, false);
				if (bitmap != nullptr) {
					textElem->SetValue(Element::ContentProp, 1, bitmap);
					bitmap->Release();
				}
				if (bitmapSh != nullptr) {
					textElemShadow->SetValue(Element::ContentProp, 1, bitmapSh);
					bitmapSh->Release();
				}
				DeleteObject(capturedBitmap);
				DeleteObject(shadowBitmap);
			}
			else if (type == 69) {
				RichText* textElem = regElem<RichText*>(L"textElem", elem);
				RichText* g_textElem = regElem<RichText*>(L"textElem", g_outerElem);
				textElem->SetLayoutPos(g_textElem->GetLayoutPos());
			}
		}
		if (touchmode && pProp == Button::PressedProp()) {
			Element* innerElem = regElem<Element*>(L"innerElem", elem);
			innerElem->SetEnabled(!((LVItem*)elem)->GetPressed());
		}
	}

	void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		Button* checkboxElem = regElem<Button*>(L"checkboxElem", (LVItem*)elem);
		if (pProp == Element::MouseFocusedProp() && showcheckboxes == 1) {
			for (int items = 0; items < pm.size(); items++) {
				if (cbpm[items]->GetSelected() == false) cbpm[items]->SetVisible(false);
			}
			checkboxElem->SetVisible(true);
		}
	}
	void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		UpdateCache u;
		if (pProp == Element::MouseFocusedProp()) {
			Element* parent = elem->GetParent();
			Element* grandparent = parent->GetParent();
			Value* v = elem->GetValue(Element::MouseFocusedProp, 1, &u);
			Element* item = regElem<Element*>(L"innerElem", grandparent);
			if (item != nullptr) item->SetValue(Element::MouseFocusedProp(), 1, v);
		}
	}

	bool isPressed = 0, isIconPressed = 0;
	unsigned long UpdateMarqueeSelectorPosition(LPVOID lpParam) {
		while (true) {
			if (!isPressed) break;
			Sleep(10);
			SendMessageW(wnd->GetHWND(), WM_USER + 5, NULL, NULL);
		}
		return 0;
	}
	unsigned long UpdateIconPosition(LPVOID lpParam) {
		if (fileopened) return 0;
		POINT ppt, ppt2;
		GetCursorPos(&ppt);
		ScreenToClient(wnd->GetHWND(), &ppt);
		static const int dragWidth = _wtoi(GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragWidth"));
		static const int dragHeight = _wtoi(GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragHeight"));
		while (true) {
			GetCursorPos(&ppt2);
			ScreenToClient(wnd->GetHWND(), &ppt2);
			Sleep(10);
			SendMessageW(wnd->GetHWND(), WM_USER + 17, (WPARAM)((vector<LVItem*>*)lpParam), (LPARAM)&ppt);
			if (lockiconpos && (abs(ppt.x - ppt2.x) > dragWidth || abs(ppt.y - ppt2.y) > dragHeight)) {
				SendMessageW(wnd->GetHWND(), WM_USER + 18, (WPARAM)((vector<LVItem*>*)lpParam), 2);
				isIconPressed = false;
			}
			if (!isIconPressed) {
				SendMessageW(wnd->GetHWND(), WM_USER + 18, lockiconpos ? NULL : (WPARAM)((vector<LVItem*>*)lpParam), 0);
				Sleep(100);
				SendMessageW(wnd->GetHWND(), WM_USER + 18, lockiconpos ? NULL : (WPARAM)((vector<LVItem*>*)lpParam), 1);
				break;
			}
		}
		return 0;
	}

	void MarqueeSelector(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pv2) {
		DWORD marqueeThread;
		HANDLE marqueeThreadHandle;
		static const HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"imageres.dll"), MAKEINTRESOURCE(2), IMAGE_ICON, 16, 16, LR_SHARED);
		static const HBITMAP selectorBmp = IconToBitmap(dummyi, 16, 16);
		if (pProp == Button::CapturedProp()) {
			if (tripleclickandhide == true && ((Button*)elem)->GetCaptured() == true) {
				emptyclicks++;
				DWORD doubleClickThread{};
				HANDLE doubleClickThreadHandle = CreateThread(0, 0, DoubleClickHandler, &emptyclicks, 0, &doubleClickThread);
				if (emptyclicks & 1) {
					for (int items = 0; items < pm.size(); items++) {
						switch (hiddenIcons) {
						case 0:
							pm[items]->SetVisible(false);
							break;
						case 1:
							if (pm[items]->GetPage() == currentPageID) pm[items]->SetVisible(true);
							break;
						}
					}
					hiddenIcons = !hiddenIcons;
					SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons", hiddenIcons, false, nullptr);
					emptyclicks = 1;
				}
			}
			if (!isPressed) {
				emptyspace->SetLayoutPos(-3);
				POINT ppt;
				GetCursorPos(&ppt);
				ScreenToClient(wnd->GetHWND(), &ppt);
				RECT dimensions{};
				GetClientRect(wnd->GetHWND(), &dimensions);
				if (localeType == 1) origX = dimensions.right - ppt.x;
				else origX = ppt.x;
				origY = ppt.y;
				selector->SetX(origX);
				selector->SetY(origY);
				selector->SetVisible(true);
				selector->SetLayoutPos(-2);
				IterateBitmap(selectorBmp, SimpleBitmapPixelHandler, 3, 0, 0.33, ImmersiveColor);
				Value* selectorBmpV = DirectUI::Value::CreateGraphic(selectorBmp, 7, 0xffffffff, false, false, false);
				selector2->SetVisible(true);
				selector2->SetLayoutPos(-2);
				marqueeThreadHandle = CreateThread(0, 0, UpdateMarqueeSelectorPosition, NULL, 0, &marqueeThread);
				if (selectorBmpV) {
					selector2->SetValue(Element::BackgroundProp, 1, selectorBmpV);
					selectorBmpV->Release();
				}
			}
			isPressed = 1;
		}
		else if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) || elem->GetMouseWithin() == false) {
			if (isPressed) {
				emptyspace->SetLayoutPos(4);
				selector->SetVisible(false);
				selector->SetLayoutPos(-3);
				selector2->SetVisible(false);
				selector2->SetLayoutPos(-3);
				isPressed = 0;
			}
		}
	}
	void ItemDragListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		DWORD dragThread;
		HANDLE dragThreadHandle;
		POINT ppt;
		if (pProp == Button::PressedProp()) {
			if (!isIconPressed) {
				selectedLVItems.clear();
				selectedLVItems.push_back((LVItem*)elem);
				int selectedItems{};
				if (elem->GetSelected() == false && GetAsyncKeyState(VK_CONTROL) == 0) {
					for (int items = 0; items < pm.size(); items++) {
						pm[items]->SetSelected(false);
						if (cbpm[items]->GetSelected() == false && showcheckboxes == 1) cbpm[items]->SetVisible(false);
					}
				}
				elem->SetSelected(true);
				for (int items = 0; items < pm.size(); items++) {
					if (pm[items]->GetSelected() == true) {
						selectedItems++;
						if (pm[items] != elem) selectedLVItems.push_back(pm[items]);
					}
				}
				Element* multipleitems = regElem<Element*>(L"multipleitems", pMain);
				multipleitems->SetVisible(false);
				if (selectedItems >= 2) {
					multipleitems->SetVisible(true);
					multipleitems->SetContentString(to_wstring(selectedItems).c_str());
				}
				if (showcheckboxes) {
					Button* checkbox = regElem<Button*>(L"checkboxElem", (LVItem*)elem);
					checkbox->SetVisible(true);
				}
				fileopened = false;
				GetCursorPos(&ppt);
				ScreenToClient(wnd->GetHWND(), &ppt);
				RECT dimensions{};
				GetClientRect(wnd->GetHWND(), &dimensions);
				if (localeType == 1) origX = dimensions.right - ppt.x - elem->GetX();
				else origX = ppt.x - elem->GetX();
				origY = ppt.y - elem->GetY();
				Value* bitmap{};
				HBITMAP hbmCapture{};
				HDC hdcWindow = GetDC(wnd->GetHWND());
				HDC hdcMem = CreateCompatibleDC(hdcWindow);
				hbmCapture = CreateCompatibleBitmap(hdcWindow, elem->GetWidth(), elem->GetHeight());
				HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmCapture);
				int iconX = (localeType == 1) ? dimensions.right - elem->GetX() - elem->GetWidth() : elem->GetX();
				BitBlt(hdcMem, 0, 0, elem->GetWidth(), elem->GetHeight(), hdcWindow, iconX, elem->GetY(), SRCCOPY);
				SelectObject(hdcMem, hbmOld);
				DeleteDC(hdcMem);
				ReleaseDC(wnd->GetHWND(), hdcWindow);
				IterateBitmap(hbmCapture, UndoPremultiplication, 1, 0, 1, NULL);
				bitmap = DirectUI::Value::CreateGraphic(hbmCapture, 7, 0xffffffff, false, false, false);
				if (bitmap != nullptr) {
					dragpreview->SetValue(Element::BackgroundProp, 1, bitmap);
					bitmap->Release();
				}
				if (hbmCapture != nullptr) DeleteObject(hbmCapture);
				dragpreview->SetWidth(elem->GetWidth());
				dragpreview->SetHeight(elem->GetHeight());
				dragThreadHandle = CreateThread(0, 0, UpdateIconPosition, &selectedLVItems, 0, &dragThread);
			}
			isIconPressed = 1;
		}
		if (pProp == Button::CapturedProp()) {
			if (((Button*)elem)->GetCaptured() == false && isIconPressed) {
				isIconPressed = 0;
			}
		}
	}

	void UpdateIconColorizationColor(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2) {
		if (pProp == DDScalableElement::AssociatedColorProp()) {
			IconColorizationColor = ((DDScalableElement*)elem)->GetAssociatedColor();
			iconColorID = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"IconColorID");
			SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"IconColorizationColor", IconColorizationColor, false, nullptr);
			atleastonesetting = true;
			RearrangeIcons(false, true, true);
		}
	}

	void testEventListener3(Element* elem, Event* iev) {
		if (iev->uidType == Button::Click) {
			switch (issubviewopen) {
			case false:
				if (elem != fullscreenpopupbase) {
					ShowPopupCore();
				}
				break;
			case true:
				if (centered->GetMouseWithin() == false && elem->GetMouseFocused() == true) {
					HidePopupCore(false);
				}
				break;
			}
		}
	}

	void RearrangeIcons(bool animation, bool reloadicons, bool bAlreadyOpen) {
		RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		if (bAlreadyOpen) SetPos(isDefaultRes());
		prevpageMain->SetVisible(false);
		nextpageMain->SetVisible(false);
		WCHAR DesktopLayoutWithSize[24];
		if (!touchmode) StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_%d", globaliconsz);
		else StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_Touch");
		if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) GetPos2(true);
		else {
			GetPos2(false);
			GetPos(false, nullptr);
		}
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 1 of 5 complete: Imported your desktop icon positions.");
		unsigned int count = pm.size();
		static const int savedanim = (pm[0] != nullptr) ? pm[0]->GetAnimation() : NULL;
		listviewAnimStorage = savedanim;
		if (reloadicons) {
			DWORD dd;
			HANDLE thumbnailThread = CreateThread(0, 0, ApplyThumbnailIcons, NULL, 0, &dd);
		}
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 2 of 5 complete: Applied icons to the relevant desktop items.");
		int desktoppadding = flScaleFactor * (touchmode ? DESKPADDING_TOUCH : DESKPADDING_NORMAL);
		int desktoppadding_x = flScaleFactor * (touchmode ? DESKPADDING_TOUCH_X : DESKPADDING_NORMAL_X);
		int desktoppadding_y = flScaleFactor * (touchmode ? DESKPADDING_TOUCH_Y : DESKPADDING_NORMAL_Y);
		int x = desktoppadding_x, y = desktoppadding_y;
		if (count >= 1) {
			DWORD* animThread = new DWORD[count];
			DWORD* animThread2 = new DWORD[count];
			HANDLE* animThreadHandle = new HANDLE[count];
			HANDLE* animThreadHandle2 = new HANDLE[count];
			int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 44) * flScaleFactor;
			int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 22) * flScaleFactor;
			int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor;
			int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, dpi) + (globaliconsz - 48) * flScaleFactor - textm.tmHeight;
			if (touchmode) {
				outerSizeX = touchSizeX + desktoppadding;
				outerSizeY = touchSizeY + desktoppadding;
				innerSizeX = touchSizeX;
				innerSizeY = touchSizeY;
			}
			int largestXPos = dimensions.right / outerSizeX;
			int largestYPos = (dimensions.bottom - (2 * desktoppadding_y)) / outerSizeY;
			if (largestXPos == 0) largestXPos = 1;
			if (largestYPos == 0) largestYPos = 1;
			vector<bool> positions{};
			positions.resize(maxPageID * largestXPos * largestYPos - 1);
			if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 3 of 5 complete: Created an array of positions.");
			for (int j = 0; j < count; j++) {
				pm[j]->SetRefreshState(reloadicons);
				if (!animation) pm[j]->SetAnimation(NULL); else pm[j]->SetAnimation(savedanim);
				if (treatdirasgroup) {
					switch (pm[j]->GetGroupSize()) {
					case LVIGS_SMALL:
						pm[j]->SetWidth(316 * flScaleFactor);
						pm[j]->SetHeight(200 * flScaleFactor);
						break;
					case LVIGS_MEDIUM:
						pm[j]->SetWidth(476 * flScaleFactor);
						pm[j]->SetHeight(300 * flScaleFactor);
						break;
					case LVIGS_WIDE:
						pm[j]->SetWidth(716 * flScaleFactor);
						pm[j]->SetHeight(300 * flScaleFactor);
						break;
					case LVIGS_LARGE:
						pm[j]->SetWidth(716 * flScaleFactor);
						pm[j]->SetHeight(450 * flScaleFactor);
						break;
					}
				}
				if (pm[j]->GetGroupSize() != LVIGS_NORMAL &&
					pm[j]->GetInternalXPos() < largestXPos - (pm[j]->GetWidth() - outerSizeX) / outerSizeX &&
					pm[j]->GetInternalYPos() < largestYPos - (pm[j]->GetHeight() - outerSizeY) / outerSizeY) {
					if (!EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) pm[j]->SetPage(maxPageID);
					int occupiedPos = ((pm[j]->GetPage() - 1) * largestXPos * largestYPos) + pm[j]->GetInternalYPos() + pm[j]->GetInternalXPos() * largestYPos;
					if (positions[occupiedPos] == true) {
						pm[j]->SetInternalXPos(65535);
						pm[j]->SetInternalYPos(65535);
					}
					else {
						int pt = 0;
						int occupiedHeight = 0;
						for (int i = 0; i <= pm[j]->GetWidth() / outerSizeX; i++) {
							if (pm[j]->GetHeight() > outerSizeY) {
								for (int k = 0; k <= pm[j]->GetHeight() / outerSizeY; k++) {
									positions[occupiedPos + pt] = true;
									pt++;
									occupiedHeight++;
								}
							}
							pt += (largestYPos - occupiedHeight);
							occupiedHeight = 0;
						}
						int widthForRender = (!treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL) ? innerSizeX : pm[j]->GetWidth();
						int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - widthForRender - x : pm[j]->GetInternalXPos() * outerSizeX + x;
						pm[j]->SetX(xRender);
						pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
					}
				}
			}
			for (int j = 0; j < count; j++) {
				if (pm[j]->GetGroupSize() == LVIGS_NORMAL && pm[j]->GetInternalXPos() < largestXPos && pm[j]->GetInternalYPos() < largestYPos) {
					int widthForRender = (!treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL) ? innerSizeX : pm[j]->GetWidth();
					int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - widthForRender - x : pm[j]->GetInternalXPos() * outerSizeX + x;
					if (!EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) pm[j]->SetPage(maxPageID);
					int occupiedPos = ((pm[j]->GetPage() - 1) * largestXPos * largestYPos) + pm[j]->GetInternalYPos() + pm[j]->GetInternalXPos() * largestYPos;
					if (positions[occupiedPos] == true) {
						pm[j]->SetInternalXPos(65535);
						pm[j]->SetInternalYPos(65535);
					}
					else {
						positions[occupiedPos] = true;
						pm[j]->SetX(xRender);
						pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
					}
				}
			}
			if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 4 of 5 complete: Assigned positions to items that are in your resolution's bounds.");
			bool forcenewpage{};
			for (int j = 0; j < count; j++) {
				int modifierX = 0;
				int modifierY = 0;
				if (pm[j]->GetGroupSize() != LVIGS_NORMAL) {
					modifierX = (pm[j]->GetWidth() - outerSizeX) / outerSizeX + 1;
					modifierY = (pm[j]->GetHeight() - outerSizeY) / outerSizeY + 1;
				}
				if (pm[j]->GetInternalXPos() >= largestXPos - modifierX ||
					pm[j]->GetInternalYPos() >= largestYPos - modifierY) {
					int y{};
					while (positions[y] == true) {
						y++;
						if (y > positions.size()) {
							y = 0;
							positions.resize((maxPageID + 1) * largestXPos * largestYPos - 1);
							for (int p = 0; p <= maxPageID * largestXPos * largestYPos; p++) {
								positions[p] = true;
							}
							for (int p = maxPageID * largestXPos * largestYPos + 1; p <= positions.size(); p++) {
								positions[p] = false;
							}
							maxPageID++;
							forcenewpage = true;
							break;
						}
					}
					int pageID = 1;
					int y2 = y;
					while (y2 >= largestXPos * largestYPos) {
						y2 -= largestXPos * largestYPos;
						pageID++;
					}
					int xRenderPos = y2 / largestYPos;
					if (xRenderPos == largestXPos) pm[j]->SetInternalXPos(0);
					else pm[j]->SetInternalXPos(xRenderPos);
					pm[j]->SetInternalYPos(y % largestYPos);
					if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize) && !forcenewpage) {
						pm[j]->SetPage(pageID);
					}
					else pm[j]->SetPage(maxPageID);
					positions[y] = true;
					if (pm[j]->GetGroupSize() != LVIGS_NORMAL && pm[j]->GetWidth() > outerSizeX) {
						int pt = 0;
						int occupiedHeight = 0;
						for (int i = 0; i <= pm[j]->GetWidth() / outerSizeX; i++) {
							if (pm[j]->GetHeight() > outerSizeY) {
								for (int k = 0; k <= pm[j]->GetHeight() / outerSizeY; k++) {
									positions[y + pt] = true;
									pt++;
									occupiedHeight++;
								}
							}
							pt += (largestYPos - occupiedHeight);
							occupiedHeight = 0;
						}
					}
				}
				int widthForRender = (!treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL) ? innerSizeX : pm[j]->GetWidth();
				int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - widthForRender - x : pm[j]->GetInternalXPos() * outerSizeX + x;
				pm[j]->SetX(xRender);
				pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
			}

			if (maxPageID > 1 && currentPageID < maxPageID) nextpageMain->SetVisible(true);

			if (currentPageID > maxPageID) currentPageID = maxPageID;
			if (currentPageID != 1) prevpageMain->SetVisible(true);
			for (int j = 0; j < count; j++) {
				yValue* yV = new yValue{ j, (float)innerSizeX, (float)innerSizeY };
				animThreadHandle[j] = CreateThread(0, 0, animate, (LPVOID)yV, 0, &(animThread[j]));
				animThreadHandle2[j] = CreateThread(0, 0, fastin, (LPVOID)yV, 0, &(animThread2[j]));
			}
			delete[] animThread;
			delete[] animThread2;
			delete[] animThreadHandle;
			delete[] animThreadHandle2;
			positions.clear();
		}
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 5 of 5 complete: Successfully arranged the desktop items.");
		SetPos(isDefaultRes());
		lastWidth = dimensions.right;
		lastHeight = dimensions.bottom;
	}

	void InitLayout(bool bUnused1, bool bUnused2, bool bAlreadyOpen) {
		ensureNoRefresh = false;
		const WCHAR* name = touchmode ? L"outerElemTouch" : L"outerElem";
		parser->CreateElement(name, NULL, NULL, NULL, (Element**)&g_outerElem);
		if (bAlreadyOpen && isDefaultRes()) SetPos(true);
		UIContainer->DestroyAll(true);
		pm.clear();
		iconpm.clear();
		shortpm.clear();
		shadowpm.clear();
		filepm.clear();
		fileshadowpm.clear();
		cbpm.clear();
		GetFontHeight();
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 1 of 6 complete: Prepared DirectDesktop to receive desktop data.");
		LPWSTR path = GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop");
		wchar_t* secondaryPath = new wchar_t[260];
		wchar_t* cBuffer = new wchar_t[260];

		BYTE* value = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconLayouts");
		size_t offset = 0x10;
		vector<uint16_t> head;
		for (int i = 0; i < 4; ++i) {
			head.push_back(*reinterpret_cast<uint16_t*>(&value[offset + i * 2]));
		}
		head.push_back(*reinterpret_cast<uint32_t*>(&value[offset + 8]));
		uint32_t lviCount = head[4];
		int count2{};
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 2 of 6 complete: Obtained desktop item count.");

		parser->CreateElement(L"emptyspace", NULL, NULL, NULL, (Element**)&emptyspace);
		UIContainer->Add((Element**)&emptyspace, 1);
		for (int i = 0; i < lviCount; i++) {
			LVItem* outerElem;
			if (touchmode) {
				parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElem);
			}
			else parser->CreateElement(L"outerElem", NULL, NULL, NULL, (Element**)&outerElem);
			UIContainer->Add((Element**)&outerElem, 1);
			DDScalableElement* iconElem = regElem<DDScalableElement*>(L"iconElem", outerElem);
			Element* shortcutElem = regElem<Element*>(L"shortcutElem", outerElem);
			Element* iconElemShadow = regElem<Element*>(L"iconElemShadow", outerElem);
			RichText* textElem = regElem<RichText*>(L"textElem", outerElem);
			RichText* textElemShadow = regElem<RichText*>(L"textElemShadow", outerElem);
			Button* checkboxElem = regElem<Button*>(L"checkboxElem", outerElem);
			if (touchmode) assignExtendedFn(iconElem, UpdateTileOnColorChange);
			pm.push_back(outerElem);
			iconpm.push_back(iconElem);
			shortpm.push_back(shortcutElem);
			shadowpm.push_back(iconElemShadow);
			filepm.push_back(textElem);
			fileshadowpm.push_back(textElemShadow);
			cbpm.push_back(checkboxElem);
		}
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 3 of 6 complete: Created elements, preparing to enumerate desktop folders.");
		EnumerateFolder((LPWSTR)L"InternalCodeForNamespace", &pm, false, nullptr, &count2, lviCount);
		DWORD d = GetEnvironmentVariableW(L"PUBLIC", cBuffer, 260);
		StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
		if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
		EnumerateFolder(secondaryPath, &pm, false, nullptr, &count2, lviCount - count2);
		path1 = secondaryPath;
		if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
		EnumerateFolder(path, &pm, false, nullptr, &count2, lviCount - count2);
		path2 = path;
		if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
		d = GetEnvironmentVariableW(L"OneDrive", cBuffer, 260);
		StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
		EnumerateFolder(secondaryPath, &pm, false, nullptr, &count2, lviCount - count2);
		path3 = secondaryPath;
		if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 4 of 6 complete: Created arrays according to your desktop items.");
		for (int i = 0; i < lviCount; i++) {
			if (pm[i]->GetHiddenState() == true) {
				iconpm[i]->SetAlpha(128);
				shadowpm[i]->SetAlpha(0);
				filepm[i]->SetAlpha(touchmode ? 128 : 192);
				if (!touchmode) fileshadowpm[i]->SetAlpha(128);
			}
			if (!touchmode) {
				if (shellstate[4] & 0x20) {
					pm[i]->SetClass(L"doubleclicked");
					//if (pm[i].isDirectory == true && treatdirasgroup == true) outerElem->SetClass(L"singleclicked");
				}
				else pm[i]->SetClass(L"singleclicked");
			}
		}
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 5 of 6 complete: Filled the arrays with relevant desktop icon data.");
		RearrangeIcons(false, true, false);
		assignFn(emptyspace, SelectItem);
		assignExtendedFn(emptyspace, ShowCheckboxIfNeeded);
		assignExtendedFn(emptyspace, MarqueeSelector);
		assignFn(emptyspace, DesktopRightClick);
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 6 of 6 complete: Arranged the icons according to your icon placements.");
		delete[] cBuffer;
		delete[] secondaryPath;
	}

	void InitNewLVItem(const wstring& filepath, const wstring& filename) {
		FileInfo* fi = new FileInfo{ filepath, filename };
		PostMessageW(wnd->GetHWND(), WM_USER + 20, NULL, (LPARAM)fi);
	}
	void RemoveLVItem(const wstring& filepath, const wstring& filename) {
		wstring foundfilename = (wstring)L"\"" + filepath + (wstring)L"\\" + filename + (wstring)L"\"";
		for (int i = 0; i < pm.size(); i++) {
			if (pm[i]->GetFilename() == foundfilename) {
				PostMessageW(wnd->GetHWND(), WM_USER + 21, (WPARAM)pm[i], i);
				break;
			}
		}
	}
	void UpdateLVItem(const wstring& filepath, const wstring& filename, BYTE type) {
		switch (type) {
		case 1: {
			wstring foundfilename = (wstring)L"\"" + filepath + (wstring)L"\\" + filename + (wstring)L"\"";
			for (int i = 0; i < pm.size(); i++) {
				if (pm[i]->GetFilename() == foundfilename) {
					PostMessageW(wnd->GetHWND(), WM_USER + 21, (WPARAM)pm[i], i);
					break;
				}
			}
			break;
		}
		case 2: {
			FileInfo* fi = new FileInfo{ filepath, filename };
			PostMessageW(wnd->GetHWND(), WM_USER + 20, NULL, (LPARAM)fi);
			break;
		}
		}
	}

	unsigned long FinishedLogging(LPVOID lpParam) {
		int logresponse{};
		TaskDialog(NULL, NULL, LoadStrFromRes(4024).c_str(), LoadStrFromRes(4019).c_str(), LoadStrFromRes(4020).c_str(), TDCBF_OK_BUTTON | TDCBF_CLOSE_BUTTON, TD_INFORMATION_ICON, &logresponse);
		if (logresponse == IDCLOSE) SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, 420);
		return 0;
	}

	HWND GetShutdownWindowIfPresent() {
		if (shutdownwnd) return shutdownwnd->GetHWND();
		else return NULL;
	}
	HWND GetEditWindowIfPresent() {
		if (editwnd) return editwnd->GetHWND();
		else return NULL;
	}
	bool IsDesktopActive() {
		HWND hWnd = GetForegroundWindow();
		if (hWnd == NULL) return false;
		return (hWnd == hWorkerW || hWnd == hWndTaskbar || hWnd == GetShutdownWindowIfPresent());
	}
	bool IsDesktopOrSubviewActive() {
		HWND hWnd = GetForegroundWindow();
		if (hWnd == NULL) return false;
		return (hWnd == hWorkerW || hWnd == hWndTaskbar || hWnd == subviewwnd->GetHWND() || hWnd == GetEditWindowIfPresent() || hWnd == GetShutdownWindowIfPresent());
	}

	HHOOK KeyHook = nullptr;
	bool dialogopen{};
	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
		static bool keyHold[256]{};
		if (nCode == HC_ACTION) {
			KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
			if ((pKeyInfo->vkCode == 'D' || pKeyInfo->vkCode == 'M') && GetAsyncKeyState(VK_LWIN) & 0x8000) {
				HidePopupCore(true);
				SetWindowPos(hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
			if (IsDesktopActive()) {
				if (pKeyInfo->vkCode == VK_F2) {
					if (!keyHold[pKeyInfo->vkCode] && !renameactive && !isIconPressed) {
						ShowRename();
						keyHold[pKeyInfo->vkCode] = true;
					}
				}
				if ((pKeyInfo->vkCode == VK_F4) && GetAsyncKeyState(VK_MENU) & 0x8000) {
					static bool valid{};
					valid = !valid;
					if (valid) {
						switch (dialogopen) {
						case false:
							DisplayShutdownDialog();
							break;
						case true:
							DestroyShutdownDialog();
							break;
						}
						Sleep(50);
					}
					return 1;
				}
				if (pKeyInfo->vkCode == VK_F5) {
					if (!keyHold[pKeyInfo->vkCode]) {
						SetTimer(wnd->GetHWND(), 2, 150, NULL);
						keyHold[pKeyInfo->vkCode] = true;
					}
				}
				if (pKeyInfo->vkCode >= '1' && pKeyInfo->vkCode <= '5' && GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState(VK_SHIFT) & 0x8000) {
					if (!keyHold[pKeyInfo->vkCode]) {
						switch (pKeyInfo->vkCode) {
						case '1':
							SetView(144, 64, 48, false);
							break;
						case '2':
							SetView(96, 48, 32, false);
							break;
						case '3':
							SetView(48, 32, 16, false);
							break;
						case '4':
							SetView(32, 32, 12, false);
							break;
						case '5':
							SetView(32, 32, 12, true);
							break;
						}
						keyHold[pKeyInfo->vkCode] = true;
					}
				}
			}
			if (IsDesktopOrSubviewActive()) {
				if (pKeyInfo->vkCode == 'E' && GetAsyncKeyState(VK_LWIN) & 0x8000 && GetAsyncKeyState(VK_CONTROL) & 0x8000) {
					if (!keyHold[pKeyInfo->vkCode]) {
						SetTimer(wnd->GetHWND(), 1, 150, NULL);
						keyHold[pKeyInfo->vkCode] = true;
					}
				}
				if (pKeyInfo->vkCode == 'S' && GetAsyncKeyState(VK_LWIN) & 0x8000 && GetAsyncKeyState(VK_MENU) & 0x8000) {
					if (!keyHold[pKeyInfo->vkCode]) {
						SetTimer(wnd->GetHWND(), 3, 150, NULL);
						keyHold[pKeyInfo->vkCode] = true;
					}
				}
			}
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
				keyHold[pKeyInfo->vkCode] = false;
			}
		}
		return CallNextHookEx(KeyHook, nCode, wParam, lParam);
	}
}

// Windows.UI.Immersive.dll ordinal 100
typedef HRESULT(WINAPI* RegisterImmersiveBehaviors_t)();
HRESULT RegisterImmersiveBehaviors()
{
	static RegisterImmersiveBehaviors_t fn = nullptr;
	if (!fn)
	{
		HMODULE h = LoadLibraryW(L"Windows.UI.Immersive.dll");
		if (h)
			fn = (RegisterImmersiveBehaviors_t)GetProcAddress(h, MAKEINTRESOURCEA(100));
	}
	if (fn == nullptr) return E_FAIL;
	else return fn();
}

// Windows.UI.Immersive.dll ordinal 101
typedef void (WINAPI* UnregisterImmersiveBehaviors_t)();
void UnregisterImmersiveBehaviors()
{
	static UnregisterImmersiveBehaviors_t fn = nullptr;
	if (!fn)
	{
		HMODULE h = LoadLibraryW(L"Windows.UI.Immersive.dll");
		if (h)
			fn = (UnregisterImmersiveBehaviors_t)GetProcAddress(h, MAKEINTRESOURCEA(101));
	}
	if (fn == nullptr) return;
	else return fn();
}

using namespace DirectDesktop;

// @TODO: Split into functions
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	hMutex = CreateMutex(NULL, TRUE, szWindowClass);
	if (!hMutex || ERROR_ALREADY_EXISTS == GetLastError()) {
		TaskDialog(NULL, HINST_THISCOMPONENT, LoadStrFromRes(4025).c_str(), NULL,
			LoadStrFromRes(4021).c_str(), TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
		return 1;
	}
	if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"FFlags") & 0x4);
	else {
		TaskDialog(NULL, HINST_THISCOMPONENT, L"DirectDesktop", LoadStrFromRes(4022).c_str(),
			LoadStrFromRes(4023).c_str(), TDCBF_CLOSE_BUTTON, TD_WARNING_ICON, NULL);
		return 1;
	}

	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	InitProcessPriv(14, HINST_THISCOMPONENT, true, true, true);
	InitThread(TSM_IMMERSIVE);
	RegisterAllControls();
	DDScalableElement::Register();
	DDScalableButton::Register();
	LVItem::Register();
	DDLVActionButton::Register();
	DDToggleButton::Register();
	DDCheckBox::Register();
	DDCheckBoxGlyph::Register();
	DDColorPicker::Register();
	DDColorPickerButton::Register();
	DDNotificationBanner::Register();
	RegisterImmersiveBehaviors();
	RegisterPVLBehaviorFactory();
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	WCHAR localeName[256]{};
	ULONG numLanguages{};
	ULONG bufferSize = sizeof(localeName) / sizeof(WCHAR);
	GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, localeName, &bufferSize);
	GetLocaleInfoEx(localeName, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER, (LPWSTR)&localeType, sizeof(localeType) / sizeof(WCHAR));
	RECT dimensions;
	SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
	int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 2) * 2;
	int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CYEDGE, dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
	bool checklog{};
	SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging", 0, true, &checklog);
	if (checklog) {
		TaskDialog(NULL, HINST_THISCOMPONENT, L"DirectDesktop", LoadStrFromRes(4017).c_str(),
			LoadStrFromRes(4018).c_str(), TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &logging);
		SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging", logging, false, nullptr);
	}
	else logging = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging");
	if (logging == IDYES) {
		wchar_t* docsfolder = new wchar_t[260];
		wchar_t* cBuffer = new wchar_t[260];
		DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
		StringCchPrintfW(docsfolder, 260, L"%s\\Documents", cBuffer);
		MainLogger.StartLogger(((wstring)docsfolder + L"\\DirectDesktop.log").c_str());
	}
	HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
	int WindowsBuild = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\ShellCommon", L"BuildNumber");
	if (hWndProgman) {
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Found the Program Manager window.");
		hSHELLDLL_DefView = FindWindowExW(hWndProgman, NULL, L"SHELLDLL_DefView", NULL);
		if (logging == IDYES && hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: Found a SHELLDLL_DefView window.");
		if (WindowsBuild >= 26002 && logging == IDYES) MainLogger.WriteLine(L"Information: Version is 24H2, skipping WorkerW creation!!!");
		SendMessageTimeoutW(hWndProgman, 0x052C, 0, 0, SMTO_NORMAL, 250, NULL);
		Sleep(250);
		if (hSHELLDLL_DefView) {
			bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, false);
		}
	}
	if (logging == IDYES && !hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: SHELLDLL_DefView was not inside Program Manager, retrying...");
	bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &hWorkerW, &hSHELLDLL_DefView, true);
	if (logging == IDYES && hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: Found a SHELLDLL_DefView window.");
	HWND hSysListView32 = FindWindowExW(hSHELLDLL_DefView, NULL, L"SysListView32", L"FolderView");
	if (hSysListView32) {
		if (logging == IDYES) MainLogger.WriteLine(L"Information: Found SysListView32 window to hide.");
		ShowWindow(hSysListView32, SW_HIDE);
	}
	KeyHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, HINST_THISCOMPONENT, 0);
	NativeHWNDHost::Create(L"DD_DesktopHost", L"DirectDesktop", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, NULL, NULL, NULL, 0, &wnd);
	NativeHWNDHost::Create(L"DD_SubviewHost", L"DirectDesktop Subview", NULL, NULL, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, WS_EX_TOOLWINDOW, WS_POPUP, NULL, 0, &subviewwnd);
	DUIXmlParser::Create(&parser, NULL, NULL, NULL, NULL);
	DUIXmlParser::Create(&parser2, NULL, NULL, NULL, NULL);
	parser->SetXMLFromResource(IDR_UIFILE2, hInstance, hInstance);
	parser2->SetXMLFromResource(IDR_UIFILE3, hInstance, hInstance);
	HWNDElement::Create(wnd->GetHWND(), true, NULL, NULL, &key, (Element**)&parent);
	HWNDElement::Create(subviewwnd->GetHWND(), true, NULL, NULL, &key2, (Element**)&subviewparent);
	WTSRegisterSessionNotification(wnd->GetHWND(), NOTIFY_FOR_THIS_SESSION);
	SetWindowLongPtrW(wnd->GetHWND(), GWL_STYLE, 0x56003A40L);
	SetWindowLongPtrW(wnd->GetHWND(), GWL_EXSTYLE, 0xC0000800L);
	WndProc = (WNDPROC)SetWindowLongPtrW(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);
	WndProc2 = (WNDPROC)SetWindowLongPtrW(subviewwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)TopLevelWindowProc);
	if (WindowsBuild >= 26002) {
		SetWindowLongPtrW(hWorkerW, GWL_STYLE, 0x96000000L);
		SetWindowLongPtrW(hWorkerW, GWL_EXSTYLE, 0x20000880L);
	}
	HWND dummyHWnd;
	dummyHWnd = SetParent(wnd->GetHWND(), hSHELLDLL_DefView);
	HBRUSH hbr = CreateSolidBrush(RGB(0, 0, 0));
	SetClassLongPtrW(wnd->GetHWND(), GCLP_HBRBACKGROUND, (LONG_PTR)hbr);
	if (logging == IDYES) {
		if (dummyHWnd != nullptr) MainLogger.WriteLine(L"Information: DirectDesktop is now a part of Explorer.");
		else MainLogger.WriteLine(L"Error: DirectDesktop is still hosted in its own window.");
	}

	parser->CreateElement(L"main", parent, NULL, NULL, &pMain);
	pMain->SetVisible(true);
	AddLayeredRef(pMain->GetDisplayNode());
	SetGadgetFlags(pMain->GetDisplayNode(), 0x1, 0x1);
	pMain->EndDefer(key);
	parser2->CreateElement(L"fullscreenpopup", subviewparent, NULL, NULL, &pSubview);
	pSubview->SetVisible(true);
	AddLayeredRef(pSubview->GetDisplayNode());
	SetGadgetFlags(pSubview->GetDisplayNode(), 0x1, 0x1);
	pSubview->EndDefer(key2);

	LVItem* outerElemTouch;
	parser->CreateElement(L"outerElemTouch", NULL, NULL, NULL, (Element**)&outerElemTouch);
	touchSizeX = outerElemTouch->GetWidth();
	touchSizeY = outerElemTouch->GetHeight();

	InitialUpdateScale();
	if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated scaling.");
	UpdateModeInfo();
	if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated color mode information.");
	SetTheme();
	if (logging == IDYES) MainLogger.WriteLine(L"Information: Set the theme successfully.");

	sampleText = regElem<Element*>(L"sampleText", pMain);
	mainContainer = regElem<Element*>(L"mainContainer", pMain);
	UIContainer = regElem<Element*>(L"UIContainer", pMain);
	fullscreenpopupbase = regElem<Button*>(L"fullscreenpopupbase", pSubview);
	popupcontainer = regElem<Button*>(L"popupcontainer", pSubview);
	centered = regElem<Button*>(L"centered", pSubview);
	selector = regElem<Element*>(L"selector", pMain);
	selector2 = regElem<Element*>(L"selector2", pMain);
	prevpageMain = regElem<TouchButton*>(L"prevpageMain", pMain);
	nextpageMain = regElem<TouchButton*>(L"nextpageMain", pMain);
	dragpreview = regElem<Element*>(L"dragpreview", pMain);

	assignFn(fullscreenpopupbase, testEventListener3);
	assignFn(prevpageMain, GoToPrevPage);
	assignFn(nextpageMain, GoToNextPage);

	AdjustWindowSizes(true);
	WCHAR DesktopLayoutWithSize[24];
	if (!touchmode) StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_%d", globaliconsz);
	else StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_Touch");
	if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize))
		currentPageID = *reinterpret_cast<unsigned short*>(&GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)[2]);
	showcheckboxes = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect");
	hiddenIcons = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons");
	globaliconsz = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize");
	shellstate = GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState");
	RegKeyValue DDKey = { HKEY_CURRENT_USER, L"Software\\DirectDesktop", NULL, NULL };
	if (!EnsureRegValueExists(DDKey._hKeyName, DDKey._path, L"DefaultWidth")) {
		defWidth = dimensions.right / flScaleFactor;
		SetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultWidth", defWidth, false, nullptr);
	}
	else defWidth = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultWidth");
	if (!EnsureRegValueExists(DDKey._hKeyName, DDKey._path, L"DefaultHeight")) {
		defHeight = dimensions.bottom / flScaleFactor;
		SetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultHeight", defHeight, false, nullptr);
	}
	else defHeight = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultHeight");
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"TreatDirAsGroup", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"TripleClickAndHide", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"LockIconPos", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"AccentColorIcons", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"DarkIcons", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"AutoDarkIcons", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"GlassIcons", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"TouchView", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorID", 0, true, nullptr);
	SetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorizationColor", 0, true, nullptr);
	treatdirasgroup = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"TreatDirAsGroup");
	tripleclickandhide = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"TripleClickAndHide");
	lockiconpos = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"LockIconPos");
	isColorized = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"AccentColorIcons");
	isDarkIconsEnabled = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"DarkIcons");
	automaticDark = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"AutoDarkIcons");
	isGlass = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"GlassIcons");
	touchmode = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"TouchView");
	iconColorID = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorID");
	IconColorizationColor = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorizationColor");
	if (automaticDark) isDarkIconsEnabled = !theme;
	DDScalableElement::Create(NULL, NULL, (Element**)&RegistryListener);
	assignExtendedFn(RegistryListener, UpdateIconColorizationColor);
	if (touchmode) globaliconsz = 32;
	globalshiconsz = 32;
	if (globaliconsz > 96) globalshiconsz = 64;
	else if (globaliconsz > 48) globalshiconsz = 48;
	globalgpiconsz = 12;
	if (globaliconsz > 96) globalgpiconsz = 48;
	else if (globaliconsz > 48) globalgpiconsz = 32;
	else if (globaliconsz > 32) globalgpiconsz = 16;
	InitLayout(false, false, false);

	StartMonitorFileChanges(path1);
	StartMonitorFileChanges(path2);
	StartMonitorFileChanges(path3);

	if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialized layout successfully.");

	wnd->Host(pMain);
	subviewwnd->Host(pSubview);
	wnd->ShowWindow(SW_SHOW);
	if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been created and shown.");
	MARGINS m = { -1, -1, -1, -1 };
	DwmExtendFrameIntoClientArea(wnd->GetHWND(), &m);
	DwmExtendFrameIntoClientArea(subviewwnd->GetHWND(), &m);
	if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been made transparent.\n\nLogging is now complete.");
	if (logging == IDYES) {
		DWORD dd;
		HANDLE loggingThread = CreateThread(0, 0, FinishedLogging, NULL, 0, &dd);
	}
	logging = IDNO;
	StartMessagePump();
	UnInitProcessPriv(HINST_THISCOMPONENT);
	WTSUnRegisterSessionNotification(wnd->GetHWND());
	CoUninitialize();
	if (KeyHook) {
		UnhookWindowsHookEx(KeyHook);
		KeyHook = nullptr;
	}

	return 0;
}