#pragma once

#include "..\DirectDesktop.h"

using namespace DirectUI;

namespace DirectDesktop
{
	extern int g_lastDpiChangeTick;
	extern int g_settingsPageID;
	extern int g_touchSizeX, g_touchSizeY;
	extern BYTE iconColorID;
	extern COLORREF IconColorizationColor;
	extern bool g_delayGroupsForDpi;
	extern bool g_ignoreWorkAreaChange;
	extern bool g_checkifelemexists;
	extern bool g_issubviewopen;
	extern bool g_issettingsopen;

	extern void UpdateScale();
	extern void AdjustWindowSizes(bool firsttime);
	extern void CalcDesktopIconInfo(yValue* yV, int* lines_basedOnEllipsis, DWORD* alignment, bool subdirectory, vector<LVItem*>* pmLVItem, vector<RichText*>* pmFile);
	extern void ApplyIcons(vector<LVItem*> pmLVItem, DesktopIcon* di, bool subdirectory, int id, float scale, COLORREF crSubdir);
	extern void TriggerTabbedPageTransition(int pageID, Element*& peSettingsPage, LPCWSTR peSettingsPageResID, DDScalableButton*& pddsbSubUIContainer);

	extern void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
	extern void OpenGroupInExplorer(Element* elem, Event* iev);
	extern void OpenCustomizePage(Element* elem, Event* iev);
	extern void PinGroup(Element* elem, Event* iev);

	extern NativeHWNDHost* wnd;
	extern Element* UIContainer;
	extern DDScalableElement* RegistryListener;
	extern Button* fullscreenpopupbase;
	extern DDScalableButton* fullscreeninner;
	extern Button* centered;
	extern NativeHWNDHost* subviewwnd;
	extern DUIXmlParser* parserSubview;
	extern Element* pSubview;

	extern HMODULE g_hModTWinUI;

	void ShowPopupCore();
	void InitSubview();

	void ShowDirAsGroup(LVItem* lvi);
	void SelectSubItem(Element* elem, Event* iev);
	void SelectSubItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);

	DWORD WINAPI subfastin(LPVOID lpParam);
}