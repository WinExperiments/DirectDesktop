#if !defined(_DUSER_H_)

#define _DUSER_H_

#ifndef __cplusplus
#error C++ required to include this header
#endif

#if	defined(DUSER_EXPORTS)
#define DUSER_API __declspec(dllexport)
#else
#define DUSER_API __declspec(dllimport)
#endif

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int MSGID;
typedef int PRID;

DECLARE_HANDLE(HACTION);
DECLARE_HANDLE(HCLASS);
DECLARE_HANDLE(HDCONTEXT);
DECLARE_HANDLE(HGADGET);

namespace DUser
{
	class Gadget;
}

struct GMA_ACTIONINFO
{
	HACTION hact;
	void* pvData;
	float flDuration;
	float flProgress;
	int cEvent;
	int cPeriods;
	BOOL fFinished;
};

typedef void (CALLBACK *ACTIONPROC)(GMA_ACTIONINFO*);

struct GMA_ACTION
{
	DWORD cbSize;
	float flDelay;
	float flDuration;
	float flPeriod;
	UINT cRepeat;
	DWORD dwPause;
	ACTIONPROC pfnProc;
	void* pvData;
};

typedef struct tagGMSG
{
	ULONG cbSize;
	int nMsg;
	HGADGET hgadMsg;
} GMSG, *LPGMSG;

struct MethodMsg : GMSG
{
};

struct EventMsg : MethodMsg
{
	UINT nMsgFlags;
};

typedef HRESULT (CALLBACK *GADGETPROC)(HGADGET, void*, EventMsg*);

typedef struct tagBUFFER_INFO
{
	DWORD cbSize;
	UINT nMask;
	UINT nStyle;
	BYTE bAlpha;
	COLORREF crFill;
} BUFFER_INFO;

struct GTRANS_VALUE
{
	float flScalar;
	float flX;
	float flY;
	float flZ;
	float flOriginX;
	float flOriginY;
	float flOriginZ;
};

struct GTRANS_CURVE
{
	POINTFLOAT ptfl1;
	POINTFLOAT ptfl2;
};

struct GTRANS_DESC
{
	HGADGET hgadChange;
	HGADGET hgadClone;
	UINT nFlags;
	UINT nProperty;
	DWORD dwTicket;
	float flDelay;
	float flDuration;
	int zOrder;
	GTRANS_VALUE vInitial;
	GTRANS_VALUE vEnd;
	GTRANS_CURVE Curve;
	UINT uiChopIndex;
	float flChopStart;
	float flChopStop;
};

typedef BOOL (CALLBACK *ATTACHWNDPROC)(void*, HWND, UINT, WPARAM, LPARAM, LRESULT*);

struct GANI_DESC
{
	DWORD cbSize;
	HGADGET hgadChange;
	GMA_ACTION act;
	UINT nAniFlags;
	interface IInterpolation* pipol;
	interface IAnimationCallback* pcb;
};

struct GANI_SCALEDESC : GANI_DESC
{
	enum EAlignment
	{
		aTopLeft = 0,
		aTopCenter = 1,
		aTopRight = 2,
		aMiddleLeft = 3,
		aMiddleCenter = 4,
		aMiddleRight = 5,
		aBottomLeft = 6,
		aBottomCenter = 7,
		aBottomRight = 8,
	};

	EAlignment al;
	float flStart;
	float flEnd;
};

struct GANI_ALPHADESC : GANI_DESC
{
	float flStart;
	float flEnd;
	int fPushToChildren;
	UINT nOnComplete;
};

struct GANI_RECTDESC : GANI_DESC
{
	POINT ptStart;
	POINT ptEnd;
	SIZE sizeStart;
	SIZE sizeEnd;
	UINT nChangeFlags;
};

struct GANI_ROTATEDESC : GANI_DESC
{
	float flStart;
	float flEnd;
	UINT nDir;
};

struct GMSG_QUERY : EventMsg
{
	UINT nCode;
};

struct GMSG_QUERYHITTEST : GMSG_QUERY
{
	POINT ptClientPxl;
	UINT nResultCode;
	void* pvResultData;
};

typedef BOOL (CALLBACK *GADGETENUMPROC)(HGADGET, void*);

typedef struct tagFGM_INFO
{
	EventMsg* pmsg;
	UINT nFlags;
	HRESULT hr;
	void* pvReserved;
} FGM_INFO;

typedef struct tagLAYER_INFO
{
	DWORD cbSize;
	UINT nValidArgs;
	int xOffset;
	int yOffset;
	int zOrder;
	bool fIgnoreOffsetsFromLayout;
	float flXPerspective;
	float flYPerspective;
	float flDegree;
	float flOpacity;
	bool fResetPerspective;
	bool fOwnsSurface;
} LAYER_INFO;

typedef struct tagROOT_INFO
{
	DWORD cbSize;
	UINT nMask;
	UINT nOptions;
	UINT nSurface;
	UINT nDropTarget;
	BOOL fMultiTouch;
	BOOL fPreserveAlphaChannel;
	BOOL fMouseInputFocus;
	BOOL fSendAllPointerEvents;
	BOOL fDoNotStealFocus;
	void* pvData;
	HPALETTE hpal;
} ROOT_INFO;

typedef struct tagINITGADGET
{
	DWORD cbSize;
	UINT nThreadMode;
	UINT nMsgMode;
	UINT nPerfMode;
	HDCONTEXT hctxShare;
} INITGADGET;

struct TransitionStoryboardInfo
{
	bool fDUIPVLAnimation;
	int nStoryboardId;
	int nAnimationId;
	void* pvContext;
};

DUSER_API HGADGET WINAPI DUserCastHandle(DUser::Gadget* pg);
DUSER_API HRESULT WINAPI DUserDeleteGadget(DUser::Gadget* pg);
// DUSER_API Gdiplus::Brush* WINAPI GetStdColorBrushF(UINT c);
// DUSER_API Gdiplus::Color WINAPI GetStdColorF(UINT c);
// DUSER_API Gdiplus::Pen* WINAPI GetStdColorPenF(UINT c);
DUSER_API BOOL WINAPI UtilDrawOutlineRect(HDC hdc, const RECT* prcPxl, HBRUSH hbrDraw, int nThickness);
DUSER_API BOOL WINAPI AddGadgetMessageHandler(HGADGET hgadMsg, MSGID nMsg, HGADGET hgadHandler);
DUSER_API BOOL WINAPI AddLayeredRef(HGADGET hgad);
DUSER_API BOOL WINAPI AdjustClipInsideRef(HGADGET, int);
DUSER_API BOOL WINAPI AttachWndProcA(HWND hwnd, ATTACHWNDPROC pfn, void* pvThis);
DUSER_API BOOL WINAPI AttachWndProcW(HWND hwnd, ATTACHWNDPROC pfn, void* pvThis);
// DUSER_API void WINAPI AutoTrace(int, ...);
DUSER_API BOOL WINAPI BeginHideInputPaneAnimation();
DUSER_API BOOL WINAPI BeginShowInputPaneAnimation();
DUSER_API BOOL WINAPI BuildAnimation(UINT, BOOL, GANI_DESC*, REFIID, void**);
DUSER_API BOOL WINAPI BuildDropTarget(HGADGET, HWND);
DUSER_API BOOL WINAPI BuildInterpolation(UINT, BOOL, REFIID, void**);
DUSER_API BOOL WINAPI CacheDWriteRenderTarget(IUnknown*);
DUSER_API BOOL WINAPI ChangeCurrentAnimationScenario(bool, REFGUID);
DUSER_API BOOL WINAPI ClearPushedOpacitiesFromGadgetTree(HGADGET);
DUSER_API BOOL WINAPI ClearTopmostVisual(HGADGET);
DUSER_API HACTION WINAPI CreateAction(const GMA_ACTION* pma);
DUSER_API HGADGET WINAPI CreateGadget(HWND hParent, UINT nFlags, GADGETPROC pfnProc, void* pvGadgetData);
DUSER_API BOOL WINAPI CustomGadgetHitTestQuery(UINT, HGADGET, GMSG_QUERYHITTEST*);
// DUSER_API DUser::Gadget* WINAPI DUserBuildGadget(HCLASS hcl, DUser::Gadget::ConstructInfo* pciData);
DUSER_API DUser::Gadget* WINAPI DUserCastClass(DUser::Gadget* pg, HCLASS hclTest);
DUSER_API DUser::Gadget* WINAPI DUserCastDirect(HGADGET hgad);
DUSER_API HCLASS WINAPI DUserFindClass(const WCHAR* pszName, DWORD nVersion);
DUSER_API BOOL WINAPI DUserFlushDeferredMessages();
DUSER_API BOOL WINAPI DUserFlushMessages();
DUSER_API int WINAPI DUserGetAlphaPRID();
DUSER_API void* WINAPI DUserGetGutsData(DUser::Gadget* pg, HCLASS hclData);
DUSER_API int WINAPI DUserGetRectPRID();
DUSER_API int WINAPI DUserGetRotatePRID();
DUSER_API int WINAPI DUserGetScalePRID();
DUSER_API BOOL WINAPI DUserInstanceOf(DUser::Gadget* pg, HCLASS hclTest);
DUSER_API HRESULT WINAPI DUserPostEvent(EventMsg* pmsg, UINT nFlags);
DUSER_API HRESULT WINAPI DUserPostMethod(MethodMsg* pmsg);
// DUSER_API HCLASS WINAPI DUserRegisterGuts(DUser::MessageClassGuts* pmcInfo);
// DUSER_API HCLASS WINAPI DUserRegisterStub(DUser::MessageClassStub* pmcInfo);
// DUSER_API HCLASS WINAPI DUserRegisterSuper(DUser::MessageClassSuper* pmcInfo);
DUSER_API HRESULT WINAPI DUserSendEvent(EventMsg* pmsg, UINT nFlags);
DUSER_API HRESULT WINAPI DUserSendMethod(MethodMsg* pmsg);
DUSER_API void WINAPI DUserStopAnimation(DUser::Gadget*, int);
DUSER_API BOOL WINAPI DUserStopPVLAnimation(UINT, HGADGET, UINT, bool);
DUSER_API BOOL WINAPI DeleteHandle(HANDLE h);
DUSER_API BOOL WINAPI DestroyPendingDCVisuals();
DUSER_API BOOL WINAPI DetachGadgetVisuals(UINT, HGADGET, DWORD*, IUnknown**, IUnknown**, IUnknown**, IUnknown**);
DUSER_API BOOL WINAPI DetachWndProc(HWND hwnd, ATTACHWNDPROC pfn, void* pvThis);
DUSER_API void WINAPI DisableContainerHwnd(HWND);
// DUSER_API BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
DUSER_API BOOL WINAPI DrawGadgetTree(HGADGET hgadDraw, HDC hdcDraw, const RECT* prcDraw, UINT nFlags);
DUSER_API BOOL WINAPI EndInputPaneAnimation();
DUSER_API BOOL WINAPI EnsureAnimationsEnabled();
DUSER_API BOOL WINAPI EnsureGadgetTransInitialized();
DUSER_API BOOL WINAPI EnumGadgets(HGADGET hgadEnum, GADGETENUMPROC pfnProc, void* pvData, UINT nFlags);
DUSER_API HGADGET WINAPI FindGadgetFromPoint(HGADGET hgad, POINT ptContainerPxl, UINT nStyle, POINT* pptClientPxl);
DUSER_API BOOL WINAPI FindGadgetMessages(const GUID** rgpguid, MSGID* rgnMsg, int cMsgs);
DUSER_API BOOL WINAPI FindGadgetTargetingInfo(HGADGET, TOUCH_HIT_TESTING_INPUT*, POINT, UINT, TOUCH_HIT_TESTING_PROXIMITY_EVALUATION*);
DUSER_API UINT WINAPI FindStdColor(const WCHAR* pszName);
DUSER_API BOOL WINAPI FireGadgetMessages(FGM_INFO* rgFGM, int cMsgs, UINT idQueue);
DUSER_API BOOL WINAPI ForwardGadgetMessage(HGADGET hgadRoot, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* pr);
DUSER_API BOOL WINAPI FreeGdiDxInteropStagingBuffer();
DUSER_API BOOL WINAPI GadgetTransCompositionChanged(HGADGET);
DUSER_API BOOL WINAPI GadgetTransSettingChanged();
DUSER_API BOOL WINAPI GetActionTimeslice(DWORD* pdwTimeslice);
DUSER_API BOOL WINAPI GetCachedDWriteRenderTarget(REFIID, void**);
DUSER_API HMODULE WINAPI GetDUserModule();
DUSER_API void* WINAPI GetDebug();
DUSER_API BOOL WINAPI GetFinalAnimatingPosition(HGADGET, POINT*, bool);
DUSER_API HGADGET WINAPI GetGadget(HGADGET hgad, UINT nCmd);
DUSER_API BOOL WINAPI GetGadgetAnimation(HGADGET, UINT, REFIID, void**);
DUSER_API BOOL WINAPI GetGadgetBitmap(HGADGET hgad, HBITMAP* phbmp, RECT* prcPxl);
DUSER_API BOOL WINAPI GetGadgetBufferInfo(HGADGET hgad, BUFFER_INFO* pbi);
DUSER_API BOOL WINAPI GetGadgetCenterPoint(HGADGET hgad, float* pflX, float* pflY);
DUSER_API UINT WINAPI GetGadgetFlags(HGADGET hgad);
DUSER_API HGADGET WINAPI GetGadgetFocus();
DUSER_API BOOL WINAPI GetGadgetLayerInfo(HGADGET hgad, LAYER_INFO* pli);
DUSER_API UINT WINAPI GetGadgetMessageFilter(HGADGET hgad, void* pvCookie);
DUSER_API BOOL WINAPI GetGadgetProperty(HGADGET hgad, PRID id, void** ppvValue);
DUSER_API BOOL WINAPI GetGadgetRect(HGADGET hgad, RECT* prcPxl, DWORD nFlags);
DUSER_API BOOL WINAPI GetGadgetRgn(HGADGET hgad, UINT nRgnType, HRGN hrgn, UINT nFlags);
DUSER_API BOOL WINAPI GetGadgetRootInfo(HGADGET hgadRoot, ROOT_INFO* pri);
DUSER_API BOOL WINAPI GetGadgetRotation(HGADGET hgad, float* pflRotationRad);
DUSER_API BOOL WINAPI GetGadgetScale(HGADGET hgad, float* pflX, float* pflY);
DUSER_API BOOL WINAPI GetGadgetSize(HGADGET hgad, SIZE* psizeLogicalPxl);
DUSER_API UINT WINAPI GetGadgetStyle(HGADGET hgad);
DUSER_API DWORD WINAPI GetGadgetTicket(HGADGET hgad);
DUSER_API BOOL WINAPI GetGadgetVisual(UINT, HGADGET, void**, void**, void**);
DUSER_API BOOL WINAPI GetMessageExA(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
DUSER_API BOOL WINAPI GetMessageExW(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
DUSER_API HBRUSH WINAPI GetStdColorBrushI(UINT c);
DUSER_API COLORREF WINAPI GetStdColorI(UINT c);
DUSER_API const WCHAR* WINAPI GetStdColorName(UINT c);
DUSER_API HPEN WINAPI GetStdColorPenI(UINT c);
DUSER_API HPALETTE WINAPI GetStdPalette();
DUSER_API BOOL WINAPI InitGadgetComponent(UINT nOptionalComponent);
DUSER_API HDCONTEXT WINAPI InitGadgets(INITGADGET* pInit);
DUSER_API BOOL WINAPI InvalidateGadget(HGADGET hgad);
DUSER_API BOOL WINAPI InvalidateLayeredDescendants(HGADGET hgad);
DUSER_API BOOL WINAPI IsGadgetParentChainStyle(HGADGET hgad, UINT nStyle, BOOL* pfChain, UINT nFlags);
DUSER_API BOOL WINAPI IsInsideContext(HANDLE h);
DUSER_API BOOL WINAPI IsStartDelete(HANDLE hobj, BOOL* pfStarted);
DUSER_API HGADGET WINAPI LookupGadgetTicket(DWORD dwTicket);
DUSER_API BOOL WINAPI MapGadgetPoints(HGADGET hgadFrom, HGADGET hgadTo, POINT* rgptClientPxl, int cPts);
DUSER_API BOOL WINAPI PeekMessageExA(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
DUSER_API BOOL WINAPI PeekMessageExW(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
DUSER_API MSGID WINAPI RegisterGadgetMessage(const GUID* pguid);
DUSER_API BOOL WINAPI RegisterGadgetMessageString(const WCHAR* pszName);
DUSER_API PRID WINAPI RegisterGadgetProperty(const GUID* pguid);
DUSER_API BOOL WINAPI ReleaseDetachedObjects(UINT, DWORD);
DUSER_API BOOL WINAPI ReleaseLayeredRef(HGADGET hgad);
DUSER_API BOOL WINAPI ReleaseMouseCapture(HGADGET hgad);
DUSER_API BOOL WINAPI RemoveClippingImmunityFromVisual(HGADGET hgad);
DUSER_API BOOL WINAPI RemoveGadgetMessageHandler(HGADGET hgadMsg, MSGID nMsg, HGADGET hgadHandler);
DUSER_API BOOL WINAPI RemoveGadgetProperty(HGADGET hgad, PRID id);
DUSER_API BOOL WINAPI ResetDUserDevice(HGADGET hgad);
DUSER_API BOOL WINAPI ScheduleGadgetTransitions(UINT, UINT, const GTRANS_DESC*, HGADGET, TransitionStoryboardInfo*);
DUSER_API BOOL WINAPI SetActionTimeslice(DWORD dwTimeslice);
DUSER_API BOOL WINAPI SetAtlasingHints(SIZE* psizeAtlas, RECT* prcSmall, RECT* prcWide, RECT* prcApp, RECT* prcSmallWide, RECT* prcSmallApp);
DUSER_API BOOL WINAPI SetGadgetBufferInfo(HGADGET hgadChange, const BUFFER_INFO* pbi);
DUSER_API BOOL WINAPI SetGadgetCenterPoint(HGADGET hgadChange, float flX, float flY);
// DUSER_API BOOL WINAPI SetGadgetFillF(HGADGET hgadChange, Gdiplus::Brush* pgpbr);
DUSER_API BOOL WINAPI SetGadgetFillI(HGADGET hgadChange, HBRUSH hbrFill, BYTE bAlpha, int w, int h);
DUSER_API BOOL WINAPI SetGadgetFlags(HGADGET hgadChange, UINT nNewFlags, UINT nMask);
DUSER_API BOOL WINAPI SetGadgetFocus(HGADGET hgadFocus);
DUSER_API BOOL WINAPI SetGadgetFocusEx(HGADGET hgadFocus, BOOL);
DUSER_API BOOL WINAPI SetGadgetLayerInfo(HGADGET hgadChange, const LAYER_INFO* pli);
DUSER_API BOOL WINAPI SetGadgetMessageFilter(HGADGET hgadChange, void* pvCookie, UINT nNewFilter, UINT nMask);
DUSER_API BOOL WINAPI SetGadgetOrder(HGADGET hgadMove, HGADGET hgadOther, UINT nCmd);
DUSER_API BOOL WINAPI SetGadgetParent(HGADGET hgadMove, HGADGET hgadParent, HGADGET hgadOther, UINT nCmd);
DUSER_API BOOL WINAPI SetGadgetProperty(HGADGET hgad, PRID id, void* pvValue);
DUSER_API BOOL WINAPI SetGadgetRect(HGADGET hgadChange, int x, int y, int w, int h, UINT nFlags);
DUSER_API BOOL WINAPI SetGadgetRootInfo(HGADGET hgadRoot, const ROOT_INFO* pri);
DUSER_API BOOL WINAPI SetGadgetRotation(HGADGET hgadChange, float flRotationRad);
DUSER_API BOOL WINAPI SetGadgetScale(HGADGET hgadChange, float flX, float flY);
DUSER_API BOOL WINAPI SetGadgetStyle(HGADGET hgadChange, UINT nNewStyle, UINT nMask);
DUSER_API BOOL WINAPI SetHardwareDeviceUsage(int);
DUSER_API BOOL WINAPI SetMinimumDCompVersion(UINT);
DUSER_API BOOL WINAPI SetRestoreCachedLayeredRefFlag(HGADGET hgad);
DUSER_API BOOL WINAPI SetTransitionVisualProperties(UINT, UINT, const GTRANS_DESC*);
DUSER_API BOOL WINAPI SetWindowResizeFlag(HGADGET hgad, UINT nFlags);
DUSER_API BOOL WINAPI UnregisterGadgetMessage(const GUID* pguid);
DUSER_API BOOL WINAPI UnregisterGadgetMessageString(const WCHAR* pszName);
DUSER_API BOOL WINAPI UnregisterGadgetProperty(const GUID* pguid);
DUSER_API HFONT WINAPI UtilBuildFont(LPCWSTR pszName, int idxDeciSize, DWORD nFlags, HDC hdcDevice);
DUSER_API BOOL WINAPI UtilDrawBlendRect(HDC hdcDest, const RECT* prcDest, HBRUSH hbrFill, BYTE bAlpha, int wBrush, int hBrush);
DUSER_API COLORREF WINAPI UtilGetColor(HBITMAP hbmp, POINT* pptPxl);
DUSER_API BOOL WINAPI UtilSetBackground(HGADGET hgadChange, HBRUSH hbrBack);
DUSER_API BOOL WINAPI WaitMessageEx();

#ifdef __cplusplus
}
#endif

#endif // !defined(_DUSER_H_)
