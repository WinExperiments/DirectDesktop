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

	class CDataObject : public IDataObject
	{
	public:
		CDataObject();
		CDataObject(IDataObject* pDataObject);
		~CDataObject();
		FORMATETC pFmtEtc[2];
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pFmtEtcIn, STGMEDIUM* pMedium);
		HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC* pFmtEtc, STGMEDIUM* pMedium);
		HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pFmtEtc);
		HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC* pFmtEtcIn, FORMATETC* pFmtEtcOut);
		HRESULT STDMETHODCALLTYPE SetData(FORMATETC* pFmtEtc, STGMEDIUM* pMedium, BOOL fRelease);
		HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFmtEtc);
		HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC* pFmtEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
		HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection);
		HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA** ppEnumAdvise);

	private:
		IDataObject* pDataObject;
		LONG lRefCount;
		std::vector<MYOBJDATA> pData;
		HRESULT _CopyMedium(STGMEDIUM* pMediumSrc, STGMEDIUM* pMediumDst, FORMATETC* pFmtEtc);
		int _GetDataIndex(const FORMATETC* pfe);
		void _SetFORMATETC(FORMATETC* pfe, UINT cf, TYMED tymed, LONG lindex, DWORD dwAspect, DVTARGETDEVICE* ptd);
	};

	class CDropSource : public IDropSource
	{
	public:
		CDropSource();
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD dwKeyState);
		HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);

	private:
		LONG lRefCount;
		BOOL bRightClick;
		HWND hWndMenu;
		HMENU hPopup;
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
		HWND hWnd;
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
		void FreeDragData();
	private:
		SHDRAGIMAGE  _shdi;
		HWND         _hwndTarget;
		HWND         _hwnd;
		HDC          _hdcDragImage;
		HBITMAP      _hbmpOld;
		BOOL         _fLayeredSupported;
		BOOL         _fCursorDataInited;
		POINT       _ptDebounce;
		struct
		{
			BOOL    bDragging;
			BOOL    bLocked;
			HWND    hwndLock;
			BOOL    bSingle;
			DWORD   idThreadEntered;
		} _Single;
		void _InitDragData();
		HRESULT _LoadFromDataObject(IDataObject* pdtobj);
		HRESULT _SaveToDataObject(IDataObject* pdtobj);
		HRESULT _LoadLayeredBitmapBits(HGLOBAL hGlobal);
		HRESULT _SaveLayeredBitmapBits(HGLOBAL* phGlobal);
		HRESULT _SetLayeredDragging(LPSHDRAGIMAGE pshdi);
		BOOL _CreateDragWindow();
		BOOL _PreProcessDragBitmap(void** ppvBits);
	};

	void MyDragDropInit(HANDLE hHeap);
	void SetDropDescriptionBase(IDataObject* pDataObject, DROPIMAGETYPE type, LPCWSTR pszMsg, LPCWSTR pszDest);
	HRESULT SetDragImageManual(IDataObject* pdo, HBITMAP hbmp, SIZE size, POINT offset);
	CDropTarget* MyRegisterDragDrop(HWND hWnd, CLIPFORMAT* pFormat, ULONG lFmt, UINT nMsg, MYDDCALLBACK pDropProc, void* pUserData);
	CDropTarget* MyRevokeDragDrop(IDropTarget* pTarget);
	extern DWORD TheDropProc(IDataObject* pDataObject, CLIPFORMAT cf, HGLOBAL hdata, HWND hwnd, DWORD key_state, POINTL pt, void* param);

	extern HANDLE g_hHeap;
	extern bool isIconPressed;
	extern CMinimalDragImage* pMinimal;
	extern vector<LVItem*> selectedLVItems;
}