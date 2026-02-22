#pragma once

#include "..\ui\DDControls.h"

namespace DirectDesktop
{
	typedef struct tagMYDROPDATA
	{
		CLIPFORMAT cf;
		POINTL pt;
		DWORD dwKeyState;
		HGLOBAL hData;
	} MYDROPDATA, *PMYDROPDATA;

	typedef struct tagMYDROPSOURCE
	{
		IDataObject* pObj;
		IDropSource* pSrc;
		HANDLE hData;
	} MYDROPSOURCE, *PMYDROPSOURCE;

	typedef struct tagMYOBJDATA
	{
		FORMATETC fe;
		STGMEDIUM stgm;
	} MYOBJDATA, *PMYOBJDATA;

	typedef struct
	{
		BOOL fImage;
		BOOL fLayered;
		POINT ptOffset;
	} DragContextHeader;

	typedef DWORD(*MYDDCALLBACK)(IDataObject* pDataObject, CLIPFORMAT cf, HGLOBAL hData, HWND hWnd, DWORD dwKeyState, POINTL pt, void* pUserData);

	class CDropSource : public IDropSource
	{
	public:
		CDropSource();
		CDropSource(IDataObject* pdtobj);
		~CDropSource();
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD dwKeyState);
		HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);

	private:
		LONG lRefCount;
		BOOL bRightClick;
		IDragSourceHelper* pDragSourceHelper;
		IDataObject* pdtobj;
	};

	class CDropTarget : public IDropTarget
	{
	public:
		CDropTarget();
		~CDropTarget();
		CLIPFORMAT* pFormat;
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObject, DWORD dwKeyState, POINTL pt, DWORD* pdwEffect);
		HRESULT STDMETHODCALLTYPE DragOver(DWORD dwKeyState, POINTL pt, DWORD* pdwEffect);
		HRESULT STDMETHODCALLTYPE DragLeave();
		HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObject, DWORD dwKeyState, POINTL pt, DWORD* pdwEffect);
		void Initialize(LONG lRefCount, HWND hWnd, UINT nMsg, BOOL bAllowDrop, DWORD dwKeyState, ULONG lNumFormats, MYDDCALLBACK pDropProc, void* pUserData);
		inline ULONG GetNumOfFormats();
		inline HWND GetHWND();

	private:
		LONG lRefCount;
		ULONG lNumFormats;
		ULONGLONG dwTickCountL;
		ULONGLONG dwTickCountR;
		WORD ptLocFlags;
		HWND hWnd;
		RECT rcDimensions;
		BOOL bAllowDrop;
		BOOL bSameDrive;
		BOOL bVirtual;
		DWORD dwKeyState;
		IDataObject* pDataObject;
		IDropTargetHelper* pDropTargetHelper;
		DWORD dwLastEffect;
		UINT nMsg;
		void* pUserData;
		MYDDCALLBACK pDropProc;
		BOOL _QueryDataObject();
		DWORD _DropEffect(DWORD dwKeyState, POINTL pt, DWORD dwAllowed);
		void _SetDropDescription(DROPIMAGETYPE type, LPCWSTR pszMsg, LPCWSTR pszDest);
	};

	enum DragImageFlags : DWORD
	{
		DIF_NONE = 0x00000000,
		DIF_CURDATAINITED = 0x00000001,
		DIF_SHOWTEXT = 0x00000002,
		DIF_DEFAULTIMAGE = 0x00000004,
		DIF_HELPERFLAG = 0x00000008,
		DIF_COMPUTINGIMG = 0x00000010,
		DIF_DISABLETEXT = 0x00000020,
		DIF_CANADDINFO = 0x00000040
	};

	class CMinimalDragImage : public IDragSourceHelper, public IDropTargetHelper
	{
	public:
		CMinimalDragImage() {};
		~CMinimalDragImage();
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		// IDragSourceHelper
		HRESULT STDMETHODCALLTYPE InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject* pdtobj);
		HRESULT STDMETHODCALLTYPE InitializeFromWindow(HWND hwnd, POINT* ppt, IDataObject* pdtobj);
		// IDropTargetHelper
		HRESULT STDMETHODCALLTYPE DragEnter(HWND hwndTarget, IDataObject* pdtobj, POINT* ppt, DWORD dwEffect);
		HRESULT STDMETHODCALLTYPE DragLeave();
		HRESULT STDMETHODCALLTYPE DragOver(POINT* ppt, DWORD dwEffect);
		HRESULT STDMETHODCALLTYPE Drop(IDataObject* pdtobj, POINT* ppt, DWORD dwEffect);
		HRESULT STDMETHODCALLTYPE Show(BOOL fShow);
		inline IDataObject* GetDataObject();
	private:
		SHDRAGIMAGE _shdi;
		HWND _hwndTarget;
		HWND _hwnd;
		HDC _hdcDragImage;
		HDC _hdcWindow;
		HTHEME _hTheme;
		HBITMAP _hbmpOld;
		HBITMAP _hbmpUnk;
		void* _pvBits;
		RECT _rc;
		POINT _pt;
		DragImageFlags _flags;
		IDataObject* _pdtobj;
		DROPDESCRIPTION _desc;
		UINT _imgType;
		DWORD _dwCount;
		HRESULT _Create32BitHBITMAP(HBITMAP* phbm, SIZE* psz, HDC* phdc, HDC* phdc2, void** ppvBits);
		void FreeDragData();
		void _InitDragData();
		HRESULT _LoadFromDataObject(IDataObject* pdtobj);
		HRESULT _SaveToDataObject(IDataObject* pdtobj);
		HRESULT _LoadLayeredBitmapBits(HGLOBAL hGlobal);
		HRESULT _SaveLayeredBitmapBits(HGLOBAL* phGlobal);
		HRESULT _SetLayeredDragging(LPSHDRAGIMAGE pshdi);
		BOOL _CreateDragWindow();
		BOOL _PreProcessDragBitmap(void** ppvBits);
		void _PreProcessGDIBitmap(RECT* prc);
		void _ExtractOneTimeData();
		void _ExtractContinualData();
		HRESULT _AddInfoToWindow();
		void _ComputeFinalSize();
		HRESULT _GetEffectImageRect(RECT* prc);
		HRESULT _GetImageBackgroundRect(RECT* prc);
		HRESULT _GetTextRect(RECT* prc);
		HRESULT _GetTooltipRect(RECT* prcSrc, RECT* prcSrc2, RECT* prcDst);
		void _DrawImageAndDesc(LPWSTR szMessage, LPWSTR szInsert);
		int _SizeDescriptionLine(int iPartId, LPCWSTR pszPrefix, LPCWSTR pszInsert, LPCWSTR pszSuffix, RECT* prcBounds, RECT* rc1, RECT* rc2, RECT* rc3);
		void _DrawDescriptionLine(int iPartId, int iStateId, LPCWSTR pszText, RECT* prcBounds);
		void _DrawDescriptionLineComp(int iPartId, LPCWSTR pszPrefix, LPCWSTR pszInsert, LPCWSTR pszSuffix,
			RECT* prcBounds1, RECT* prcBounds2, RECT* prcBounds3);
		void _DrawTooltipBackground(RECT* prcSrc, RECT* prcBounds, int width);
		LRESULT CALLBACK _DragWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK s_DragWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};

	void MyDragDropInit(HANDLE hHeap);
	void SetDropDescriptionBase(IDataObject* pDataObject, DROPIMAGETYPE type, LPCWSTR pszMsg, LPCWSTR pszDest);
	HRESULT SetDragImageManual(IDataObject* pdo, HBITMAP hbmp, SIZE size, POINT offset);
	CDropTarget* MyRegisterDragDrop(HWND hWnd, CLIPFORMAT* pFormat, ULONG lFmt, UINT nMsg, MYDDCALLBACK pDropProc, void* pUserData);
	CDropTarget* MyRevokeDragDrop(IDropTarget* pTarget);
	HRESULT DataObj_GetBlobWithIndex(IDataObject* pdtobj, CLIPFORMAT cf, void* pvData, size_t cbData, LONG lindex);
	HRESULT DataObj_SetBlobWithIndex(IDataObject* pdtobj, CLIPFORMAT cf, const void* pvData, size_t cbData, LONG lindex);
	extern DWORD TheDropProc(IDataObject* pDataObject, CLIPFORMAT cf, HGLOBAL hdata, HWND hwnd, DWORD key_state, POINTL pt, void* param);

	extern HANDLE g_hHeap;
	extern bool isIconPressed;
	extern bool g_touchmode;
	extern CMinimalDragImage* pMinimal;
	extern vector<LVItem*> selectedLVItems;
	extern DirectUI::TouchButton* prevpageMain, *nextpageMain;
	extern void TriggerPageTransition(int direction, RECT& dimensions);
	extern void InitNewLVItem(const wstring& filepath, const wstring& filename, POINTL* ppt, const UINT page);
}