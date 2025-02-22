#pragma once
#include <Windows.h>
#include <oleacc.h>
#include <objbase.h>
#include <initguid.h>
#include <oleacc.h>
#include <Commctrl.h>
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>
#include <DbgHelp.h>
#include <thumbcache.h>
#include <XmlLite.h>
#include <InputScope.h> // for InputScope enum
#include <intsafe.h>
#include <crtdbg.h>

#include "../DUser/DUser.h"

#if	defined(DIRECTUI_EXPORTS)
#define UILIB_API __declspec(dllexport)
#else
#define UILIB_API __declspec(dllimport)
#endif

#include "Types.h"
#include "Misc.h"
#include "Interfaces.h"

#include "AutoLock.h"
#include "AutoThread.h"
#include "AutoVariant.h"
#include "SafeElementPtr.h"
#include "SimpleTimer.h"

#include "Primitives.h"
#include "UiaSchema.h"
#include "Element.h"
#include "PatternProvider.h"
#include "Value.h"
#include "Parser.h"
#include "Browser.h"
#include "Bind.h"
#include "AnimationStrip.h"
#include "Button.h"
#include "Base.h"
#include "ClassInfo.h"
#include "AccessibleButton.h"
#include "AutoButton.h"
#include "PushButton.h"
#include "Event.h"
#include "Layout.h"
#include "BorderLayout.h"
#include "Host.h"
#include "Accessibility.h"
#include "Provider.h"
#include "Movie.h"
#include "ElementWithHWND.h"
#include "HWNDElement.h"
#include "DialogElement.h"
#include "NativeHWNDHost.h"
#include "HWNDHost.h"
#include "CBase.h"
#include "CAVI.h"
#include "CPushButton.h"
#include "CBaseCheckRadioButton.h"
#include "CBaseScrollBar.h"
#include "CHScrollBar.h"
#include "CCheckBox.h"
#include "CCommandLink.h"
#include "CListBox.h"
#include "CListView.h"
#include "CProgressBar.h"
#include "CRadioButton.h"
#include "CCSysLink.h"
#include "CTrackBar.h"
#include "CTreeView.h"
#include "CVScrollBar.h"
#include "CallstackTracker.h"
#include "CheckBoxGlyph.h"
#include "Expandable.h"
#include "Clipper.h"
#include "Combobox.h"
#include "DuiNavigate.h"
#include "Edit.h"
#include "EventManager.h"
#include "ExpandCollapse.h"
#include "Expando.h"
#include "ExpandoButtonGlyph.h"
#include "FontCache.h"
#include "FontCheckOut.h"
#include "GridItem.h"
#include "Grid.h"
#include "HWNDElement.h"
#include "InvokeHelper.h"
#include "LinkedList.h"
#include "Macro.h"
#include "NavScoring.h"
#include "Navigator.h"
#include "TextGraphic.h"
#include "PText.h"
#include "Page.h"
#include "Progress.h"
#include "RadioButtonGlyph.h"
#include "RangeValue.h"
#include "RefPointElement.h"
#include "RepeatButton.h"
#include "Repeater.h"
#include "ScrollBar.h"
#include "ScrollItem.h"
#include "Scroll.h"
#include "ScrollViewer.h"
#include "SelectionItem.h"
#include "Selection.h"
#include "Selector.h"
#include "SelectorNoDefault.h"
#include "SelectorSelectionItem.h"
#include "SelectorSelection.h"
#include "StyledScrollViewer.h"
#include "TableItem.h"
#include "Table.h"
#include "TaskPage.h"
#include "Thumb.h"
#include "Toggle.h"
#include "UnknownElement.h"
#include "Viewer.h"
#include "XBaby.h"
#include "XElement.h"
#include "XProvider.h"
#include "BrowserSelection.h"
#include "RichText.h"
#include "SemanticController.h"
#include "ManipulationHelper.h"
#include "PromptText.h"
#include "DuiAnimation.h"

// Touch elements
#include "TouchButton.h"
#include "TouchCheckBox.h"
#include "TouchHWNDElement.h"
#include "TouchScrollBar.h"
#include "TouchScrollViewer.h"
#include "TouchTooltip.h"

#include "TouchEditBase.h"
#include "TouchEditInner.h"
#include "TouchEdit2.h"

#include "SemanticZoomToggle.h"

#include "ElementProviderManager.h"

//UnknownElement

UILIB_API void WINAPI DumpDuiTree(DirectUI::Element* pe, BOOL fShowProperties);
UILIB_API void WINAPI DumpDuiProperties(DirectUI::Element* pe);
UILIB_API HRESULT WINAPI DuiCreateObject(REFCLSID rclsid, REFIID riid, void** ppv);

namespace DirectUI
{
	extern UILIB_API unsigned long g_dwElSlot;

	extern "C"
	{
		// HRESULT WINAPI InitProcessPriv(int duiVersion, unsigned short*unk1, char unk2, bool bEnableUIAutomationProvider);
		// @Careful: fInitCommctl is new in Windows 10
		HRESULT WINAPI InitProcessPriv(DWORD dwExpectedVersion, HMODULE hModule, bool fRegisterControls, bool fEnableUIAutomationProvider, bool fInitCommctl);
		HRESULT WINAPI UnInitProcessPriv(HMODULE hModule);
		EXTERN_C HRESULT WINAPI InitThread(UINT nThreadMode);
		void WINAPI UnInitThread();

		HRESULT WINAPI CreateDUIWrapper(Element* pe, IUnknown** ppunk);
		HRESULT WINAPI CreateDUIWrapperEx(Element* pe, IXProviderCP* pprovCP, IUnknown** ppunk);
		HRESULT WINAPI CreateDUIWrapperFromResource(HINSTANCE hRes, const WCHAR* pszResource, const WCHAR* pszResID, const WCHAR* pszFile, IUnknown** ppunk);

		// int WINAPI GetScreenDPI();

		HRESULT WINAPI RegisterAllControls();
		HRESULT WINAPI RegisterBaseControls();
		HRESULT WINAPI RegisterBrowserControls();
		HRESULT WINAPI RegisterCommonControls();
		HRESULT WINAPI RegisterExtendedControls();
		HRESULT WINAPI RegisterMacroControls();
		HRESULT WINAPI RegisterMiscControls();
		HRESULT WINAPI RegisterStandardControls();
		HRESULT WINAPI RegisterXControls();

		BOOL WINAPI StartMessagePump();
		void WINAPI StopMessagePump();

		ATOM WINAPI StrToID(const WCHAR* psz);
		CHAR* WINAPI UnicodeToMultiByte(const WCHAR* pszUnicode, int cChars, int* pMultiBytes);
		WCHAR* WINAPI MultiByteToUnicode(WCHAR* pszMulti, int dBytes, int* pUniChars);

		BOOL WINAPI IsAnimationsEnabled();
		bool WINAPI IsPalette(HWND hwnd);
		BOOL WINAPI IsUIAutomationProviderEnabled();
		int WINAPI DUIDrawShadowText(HDC hdc, const WCHAR* pszText, int cch, RECT* prc, DWORD dwFlags, COLORREF crText);

		void WINAPI BlurBitmap(UINT* plBitmapBits, int cx, int cy, int cxRow, COLORREF crFill);
		void WINAPI BlurBitmapNormal(UINT* prgb, int cx, int cy, int cxRow, COLORREF crFill);

		HBRUSH WINAPI BrushFromEnumI(int c);
		COLORREF WINAPI ColorFromEnumI(int c);
		COLORREF WINAPI ARGBColorEnumI(int c);

		DWORD* WINAPI DisableAnimations();
		int WINAPI DrawShadowTextEx(HDC hdc, const WCHAR *pszText, int cch, RECT* prc, DWORD dwFlags, COLORREF crText, COLORREF crShadow, int ixOffset, int iyOffset, BYTE bInitialAlpha, BOOL fAPIInit);
		Element* WINAPI ElementFromGadget(HGADGET hGadget);
		DWORD* WINAPI EnableAnimations();
		void WINAPI FlushThemeHandles(WPARAM wParam);

		void WINAPI ForceDebugBreak();

		IDataEntry* WINAPI GetElementDataEntry(Element* pe);
		Macro* WINAPI GetElementMacro(Element* pe);
		void* WINAPI GetFontCache();

		HRESULT WINAPI GetThemeHandle(const WCHAR* pszClass, HTHEME *phTheme);

		HRESULT WINAPI HrSysAllocString(const OLECHAR* psz, BSTR* ppbstrOut);
		HRESULT WINAPI HStrDup(const WCHAR* pszSrc, WCHAR** ppszOut);

		BOOL WINAPI InitPreprocessor();

		HRESULT WINAPI SetDefAction(Element* pe, DWORD oleacc);

		BOOL WINAPI UiaHideOnGetObject(HWND hwnd, WPARAM wParam, LPARAM lParam);
		HANDLE WINAPI UiaOnDestroySink(HWND hwnd);
		HRESULT WINAPI UiaOnGetObject(Element* pe, WPARAM wParam, LPARAM lParam, bool* pfHandled, int* plResult);
		BOOL WINAPI UiaOnToolTip(Element* pe, DWORD dwFlags);

		void WINAPI NotifyAccessibilityEvent(DWORD dwEvent, Element* pe);
		WCHAR* WINAPI PreprocessBuffer(const WCHAR* pszBuf, UINT cchBuf, bool fInsertMainResId);
		HBITMAP WINAPI ProcessAlphaBitmapI(HBITMAP hbmSource);
		void WINAPI PurgeThemeHandles();

		HRESULT WINAPI RegisterPVLBehaviorFactory();
		void WINAPI DUIStopPVLAnimation(Element* peAnimating, UINT nDCProperty, BOOL fFinal);
	}
}
