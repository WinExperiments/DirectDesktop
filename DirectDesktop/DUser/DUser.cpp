#include <Windows.h>

#include "DUser.h"

DUSER_API HGADGET WINAPI DUserCastHandle(DUser::Gadget* pg) { return {}; }
DUSER_API HRESULT WINAPI DUserDeleteGadget(DUser::Gadget* pg) { return {}; }
// DUSER_API Gdiplus::Brush* WINAPI GetStdColorBrushF(UINT c) { return {}; }
// DUSER_API Gdiplus::Color WINAPI GetStdColorF(UINT c) { return {}; }
// DUSER_API Gdiplus::Pen* WINAPI GetStdColorPenF(UINT c) { return {}; }
DUSER_API BOOL WINAPI UtilDrawOutlineRect(HDC hdc, const RECT* prcPxl, HBRUSH hbrDraw, int nThickness) { return {}; }
DUSER_API BOOL WINAPI AddGadgetMessageHandler(HGADGET hgadMsg, MSGID nMsg, HGADGET hgadHandler) { return {}; }
DUSER_API BOOL WINAPI AddLayeredRef(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI AdjustClipInsideRef(HGADGET, int) { return {}; }
DUSER_API BOOL WINAPI AttachWndProcA(HWND hwnd, ATTACHWNDPROC pfn, void* pvThis) { return {}; }
DUSER_API BOOL WINAPI AttachWndProcW(HWND hwnd, ATTACHWNDPROC pfn, void* pvThis) { return {}; }
// DUSER_API void WINAPI AutoTrace(int, ...) { return {}; }
DUSER_API BOOL WINAPI BeginHideInputPaneAnimation() { return {}; }
DUSER_API BOOL WINAPI BeginShowInputPaneAnimation() { return {}; }
DUSER_API BOOL WINAPI BuildAnimation(UINT, BOOL, GANI_DESC*, REFIID, void**) { return {}; }
DUSER_API BOOL WINAPI BuildDropTarget(HGADGET, HWND) { return {}; }
DUSER_API BOOL WINAPI BuildInterpolation(UINT, BOOL, REFIID, void**) { return {}; }
DUSER_API BOOL WINAPI CacheDWriteRenderTarget(IUnknown*) { return {}; }
DUSER_API BOOL WINAPI ChangeCurrentAnimationScenario(bool, REFGUID) { return {}; }
DUSER_API BOOL WINAPI ClearPushedOpacitiesFromGadgetTree(HGADGET) { return {}; }
DUSER_API BOOL WINAPI ClearTopmostVisual(HGADGET) { return {}; }
DUSER_API HACTION WINAPI CreateAction(const GMA_ACTION* pma) { return {}; }
DUSER_API HGADGET WINAPI CreateGadget(HWND hParent, UINT nFlags, GADGETPROC pfnProc, void* pvGadgetData) { return {}; }
DUSER_API BOOL WINAPI CustomGadgetHitTestQuery(UINT, HGADGET, GMSG_QUERYHITTEST*) { return {}; }
// DUSER_API DUser::Gadget* WINAPI DUserBuildGadget(HCLASS hcl, DUser::Gadget::ConstructInfo* pciData) { return {}; }
DUSER_API DUser::Gadget* WINAPI DUserCastClass(DUser::Gadget* pg, HCLASS hclTest) { return {}; }
DUSER_API DUser::Gadget* WINAPI DUserCastDirect(HGADGET hgad) { return {}; }
DUSER_API HCLASS WINAPI DUserFindClass(const WCHAR* pszName, DWORD nVersion) { return {}; }
DUSER_API BOOL WINAPI DUserFlushDeferredMessages() { return {}; }
DUSER_API BOOL WINAPI DUserFlushMessages() { return {}; }
DUSER_API int WINAPI DUserGetAlphaPRID() { return {}; }
DUSER_API void* WINAPI DUserGetGutsData(DUser::Gadget* pg, HCLASS hclData) { return {}; }
DUSER_API int WINAPI DUserGetRectPRID() { return {}; }
DUSER_API int WINAPI DUserGetRotatePRID() { return {}; }
DUSER_API int WINAPI DUserGetScalePRID() { return {}; }
DUSER_API BOOL WINAPI DUserInstanceOf(DUser::Gadget* pg, HCLASS hclTest) { return {}; }
DUSER_API HRESULT WINAPI DUserPostEvent(EventMsg* pmsg, UINT nFlags) { return {}; }
DUSER_API HRESULT WINAPI DUserPostMethod(MethodMsg* pmsg) { return {}; }
// DUSER_API HCLASS WINAPI DUserRegisterGuts(DUser::MessageClassGuts* pmcInfo) { return {}; }
// DUSER_API HCLASS WINAPI DUserRegisterStub(DUser::MessageClassStub* pmcInfo) { return {}; }
// DUSER_API HCLASS WINAPI DUserRegisterSuper(DUser::MessageClassSuper* pmcInfo) { return {}; }
DUSER_API HRESULT WINAPI DUserSendEvent(EventMsg* pmsg, UINT nFlags) { return {}; }
DUSER_API HRESULT WINAPI DUserSendMethod(MethodMsg* pmsg) { return {}; }
DUSER_API void WINAPI DUserStopAnimation(DUser::Gadget*, int) {}
DUSER_API BOOL WINAPI DUserStopPVLAnimation(UINT, HGADGET, UINT, bool) { return {}; }
DUSER_API BOOL WINAPI DeleteHandle(HANDLE h) { return {}; }
DUSER_API BOOL WINAPI DestroyPendingDCVisuals() { return {}; }
DUSER_API BOOL WINAPI DetachGadgetVisuals(UINT, HGADGET, DWORD*, IUnknown**, IUnknown**, IUnknown**, IUnknown**) { return {}; }
DUSER_API BOOL WINAPI DetachWndProc(HWND hwnd, ATTACHWNDPROC pfn, void* pvThis) { return {}; }
DUSER_API void WINAPI DisableContainerHwnd(HWND) {}
// DUSER_API BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { return {}; }
DUSER_API BOOL WINAPI DrawGadgetTree(HGADGET hgadDraw, HDC hdcDraw, const RECT* prcDraw, UINT nFlags) { return {}; }
DUSER_API BOOL WINAPI EndInputPaneAnimation() { return {}; }
DUSER_API BOOL WINAPI EnsureAnimationsEnabled() { return {}; }
DUSER_API BOOL WINAPI EnsureGadgetTransInitialized() { return {}; }
DUSER_API BOOL WINAPI EnumGadgets(HGADGET hgadEnum, GADGETENUMPROC pfnProc, void* pvData, UINT nFlags) { return {}; }
DUSER_API HGADGET WINAPI FindGadgetFromPoint(HGADGET hgad, POINT ptContainerPxl, UINT nStyle, POINT* pptClientPxl) { return {}; }
DUSER_API BOOL WINAPI FindGadgetMessages(const GUID** rgpguid, MSGID* rgnMsg, int cMsgs) { return {}; }
DUSER_API BOOL WINAPI FindGadgetTargetingInfo(HGADGET, TOUCH_HIT_TESTING_INPUT*, POINT, UINT, TOUCH_HIT_TESTING_PROXIMITY_EVALUATION*) { return {}; }
DUSER_API UINT WINAPI FindStdColor(const WCHAR* pszName) { return {}; }
DUSER_API BOOL WINAPI FireGadgetMessages(FGM_INFO* rgFGM, int cMsgs, UINT idQueue) { return {}; }
DUSER_API BOOL WINAPI ForwardGadgetMessage(HGADGET hgadRoot, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* pr) { return {}; }
DUSER_API BOOL WINAPI FreeGdiDxInteropStagingBuffer() { return {}; }
DUSER_API BOOL WINAPI GadgetTransCompositionChanged(HGADGET) { return {}; }
DUSER_API BOOL WINAPI GadgetTransSettingChanged() { return {}; }
DUSER_API BOOL WINAPI GetActionTimeslice(DWORD* pdwTimeslice) { return {}; }
DUSER_API BOOL WINAPI GetCachedDWriteRenderTarget(REFIID, void**) { return {}; }
DUSER_API HMODULE WINAPI GetDUserModule() { return {}; }
DUSER_API void* WINAPI GetDebug() { return {}; }
DUSER_API BOOL WINAPI GetFinalAnimatingPosition(HGADGET, POINT*, bool) { return {}; }
DUSER_API HGADGET WINAPI GetGadget(HGADGET hgad, UINT nCmd) { return {}; }
DUSER_API BOOL WINAPI GetGadgetAnimation(HGADGET, UINT, REFIID, void**) { return {}; }
DUSER_API BOOL WINAPI GetGadgetBitmap(HGADGET hgad, HBITMAP* phbmp, RECT* prcPxl) { return {}; }
DUSER_API BOOL WINAPI GetGadgetBufferInfo(HGADGET hgad, BUFFER_INFO* pbi) { return {}; }
DUSER_API BOOL WINAPI GetGadgetCenterPoint(HGADGET hgad, float* pflX, float* pflY) { return {}; }
DUSER_API UINT WINAPI GetGadgetFlags(HGADGET hgad) { return {}; }
DUSER_API HGADGET WINAPI GetGadgetFocus() { return {}; }
DUSER_API BOOL WINAPI GetGadgetLayerInfo(HGADGET hgad, LAYER_INFO* pli) { return {}; }
DUSER_API UINT WINAPI GetGadgetMessageFilter(HGADGET hgad, void* pvCookie) { return {}; }
DUSER_API BOOL WINAPI GetGadgetProperty(HGADGET hgad, PRID id, void** ppvValue) { return {}; }
DUSER_API BOOL WINAPI GetGadgetRect(HGADGET hgad, RECT* prcPxl, DWORD nFlags) { return {}; }
DUSER_API BOOL WINAPI GetGadgetRgn(HGADGET hgad, UINT nRgnType, HRGN hrgn, UINT nFlags) { return {}; }
DUSER_API BOOL WINAPI GetGadgetRootInfo(HGADGET hgadRoot, ROOT_INFO* pri) { return {}; }
DUSER_API BOOL WINAPI GetGadgetRotation(HGADGET hgad, float* pflRotationRad) { return {}; }
DUSER_API BOOL WINAPI GetGadgetScale(HGADGET hgad, float* pflX, float* pflY) { return {}; }
DUSER_API BOOL WINAPI GetGadgetSize(HGADGET hgad, SIZE* psizeLogicalPxl) { return {}; }
DUSER_API UINT WINAPI GetGadgetStyle(HGADGET hgad) { return {}; }
DUSER_API DWORD WINAPI GetGadgetTicket(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI GetGadgetVisual(UINT, HGADGET, void**, void**, void**) { return {}; }
DUSER_API BOOL WINAPI GetMessageExA(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) { return {}; }
DUSER_API BOOL WINAPI GetMessageExW(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) { return {}; }
DUSER_API HBRUSH WINAPI GetStdColorBrushI(UINT c) { return {}; }
DUSER_API COLORREF WINAPI GetStdColorI(UINT c) { return {}; }
DUSER_API const WCHAR* WINAPI GetStdColorName(UINT c) { return {}; }
DUSER_API HPEN WINAPI GetStdColorPenI(UINT c) { return {}; }
DUSER_API HPALETTE WINAPI GetStdPalette() { return {}; }
DUSER_API BOOL WINAPI InitGadgetComponent(UINT nOptionalComponent) { return {}; }
DUSER_API HDCONTEXT WINAPI InitGadgets(INITGADGET* pInit) { return {}; }
DUSER_API BOOL WINAPI InvalidateGadget(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI InvalidateLayeredDescendants(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI IsGadgetParentChainStyle(HGADGET hgad, UINT nStyle, BOOL* pfChain, UINT nFlags) { return {}; }
DUSER_API BOOL WINAPI IsInsideContext(HANDLE h) { return {}; }
DUSER_API BOOL WINAPI IsStartDelete(HANDLE hobj, BOOL* pfStarted) { return {}; }
DUSER_API HGADGET WINAPI LookupGadgetTicket(DWORD dwTicket) { return {}; }
DUSER_API BOOL WINAPI MapGadgetPoints(HGADGET hgadFrom, HGADGET hgadTo, POINT* rgptClientPxl, int cPts) { return {}; }
DUSER_API BOOL WINAPI PeekMessageExA(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) { return {}; }
DUSER_API BOOL WINAPI PeekMessageExW(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) { return {}; }
DUSER_API MSGID WINAPI RegisterGadgetMessage(const GUID* pguid) { return {}; }
DUSER_API BOOL WINAPI RegisterGadgetMessageString(const WCHAR* pszName) { return {}; }
DUSER_API PRID WINAPI RegisterGadgetProperty(const GUID* pguid) { return {}; }
DUSER_API BOOL WINAPI ReleaseDetachedObjects(UINT, DWORD) { return {}; }
DUSER_API BOOL WINAPI ReleaseLayeredRef(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI ReleaseMouseCapture(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI RemoveClippingImmunityFromVisual(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI RemoveGadgetMessageHandler(HGADGET hgadMsg, MSGID nMsg, HGADGET hgadHandler) { return {}; }
DUSER_API BOOL WINAPI RemoveGadgetProperty(HGADGET hgad, PRID id) { return {}; }
DUSER_API BOOL WINAPI ResetDUserDevice(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI ScheduleGadgetTransitions(UINT, UINT, const GTRANS_DESC*, HGADGET, TransitionStoryboardInfo*) { return {}; }
DUSER_API BOOL WINAPI SetActionTimeslice(DWORD dwTimeslice) { return {}; }
DUSER_API BOOL WINAPI SetAtlasingHints(SIZE* psizeAtlas, RECT* prcSmall, RECT* prcWide, RECT* prcApp, RECT* prcSmallWide, RECT* prcSmallApp) { return {}; }
DUSER_API BOOL WINAPI SetGadgetBufferInfo(HGADGET hgadChange, const BUFFER_INFO* pbi) { return {}; }
DUSER_API BOOL WINAPI SetGadgetCenterPoint(HGADGET hgadChange, float flX, float flY) { return {}; }
// DUSER_API BOOL WINAPI SetGadgetFillF(HGADGET hgadChange, Gdiplus::Brush* pgpbr) { return {}; }
DUSER_API BOOL WINAPI SetGadgetFillI(HGADGET hgadChange, HBRUSH hbrFill, BYTE bAlpha, int w, int h) { return {}; }
DUSER_API BOOL WINAPI SetGadgetFlags(HGADGET hgadChange, UINT nNewFlags, UINT nMask) { return {}; }
DUSER_API BOOL WINAPI SetGadgetFocus(HGADGET hgadFocus) { return {}; }
DUSER_API BOOL WINAPI SetGadgetFocusEx(HGADGET hgadFocus, BOOL) { return {}; }
DUSER_API BOOL WINAPI SetGadgetLayerInfo(HGADGET hgadChange, const LAYER_INFO* pli) { return {}; }
DUSER_API BOOL WINAPI SetGadgetMessageFilter(HGADGET hgadChange, void* pvCookie, UINT nNewFilter, UINT nMask) { return {}; }
DUSER_API BOOL WINAPI SetGadgetOrder(HGADGET hgadMove, HGADGET hgadOther, UINT nCmd) { return {}; }
DUSER_API BOOL WINAPI SetGadgetParent(HGADGET hgadMove, HGADGET hgadParent, HGADGET hgadOther, UINT nCmd) { return {}; }
DUSER_API BOOL WINAPI SetGadgetProperty(HGADGET hgad, PRID id, void* pvValue) { return {}; }
DUSER_API BOOL WINAPI SetGadgetRect(HGADGET hgadChange, int x, int y, int w, int h, UINT nFlags) { return {}; }
DUSER_API BOOL WINAPI SetGadgetRootInfo(HGADGET hgadRoot, const ROOT_INFO* pri) { return {}; }
DUSER_API BOOL WINAPI SetGadgetRotation(HGADGET hgadChange, float flRotationRad) { return {}; }
DUSER_API BOOL WINAPI SetGadgetScale(HGADGET hgadChange, float flX, float flY) { return {}; }
DUSER_API BOOL WINAPI SetGadgetStyle(HGADGET hgadChange, UINT nNewStyle, UINT nMask) { return {}; }
DUSER_API BOOL WINAPI SetHardwareDeviceUsage(int) { return {}; }
DUSER_API BOOL WINAPI SetMinimumDCompVersion(UINT) { return {}; }
DUSER_API BOOL WINAPI SetRestoreCachedLayeredRefFlag(HGADGET hgad) { return {}; }
DUSER_API BOOL WINAPI SetTransitionVisualProperties(UINT, UINT, const GTRANS_DESC*) { return {}; }
DUSER_API BOOL WINAPI SetWindowResizeFlag(HGADGET hgad, UINT nFlags) { return {}; }
DUSER_API BOOL WINAPI UnregisterGadgetMessage(const GUID* pguid) { return {}; }
DUSER_API BOOL WINAPI UnregisterGadgetMessageString(const WCHAR* pszName) { return {}; }
DUSER_API BOOL WINAPI UnregisterGadgetProperty(const GUID* pguid) { return {}; }
DUSER_API HFONT WINAPI UtilBuildFont(LPCWSTR pszName, int idxDeciSize, DWORD nFlags, HDC hdcDevice) { return {}; }
DUSER_API BOOL WINAPI UtilDrawBlendRect(HDC hdcDest, const RECT* prcDest, HBRUSH hbrFill, BYTE bAlpha, int wBrush, int hBrush) { return {}; }
DUSER_API COLORREF WINAPI UtilGetColor(HBITMAP hbmp, POINT* pptPxl) { return {}; }
DUSER_API BOOL WINAPI UtilSetBackground(HGADGET hgadChange, HBRUSH hbrBack) { return {}; }
DUSER_API BOOL WINAPI WaitMessageEx() { return {}; }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
