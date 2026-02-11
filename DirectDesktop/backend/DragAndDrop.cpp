// 0.5.6: This was a tedious task, therefore it is not well done and may leak!
// It will be improved later.
// References:
// https://anton.maurovic.com/posts/win32-api-approach-to-windows-drag-and-drop/
// https://sourceforge.net/projects/win32cdnd/files/
// https://devblogs.microsoft.com/oldnewthing/20041206-00/?p=37133
// https://devblogs.microsoft.com/oldnewthing/20080311-00/?p=23153

#include "pch.h"
#include "DragAndDrop.h"
#include "..\DirectDesktop.h"

namespace DirectDesktop
{
	HANDLE g_hHeap;
#define MYDD_HEAP (g_hHeap == NULL ? (g_hHeap = GetProcessHeap()) : g_hHeap)

	HRESULT CreateHGlobalFromBlob(const void* pvData, size_t cbData, UINT uFlags, HGLOBAL* phglob)
	{
		HGLOBAL hglob = GlobalAlloc(uFlags, cbData);
		if (hglob)
		{
			void* pvAlloc = GlobalLock(hglob);
			if (pvAlloc)
			{
				CopyMemory(pvAlloc, pvData, cbData);
				GlobalUnlock(hglob);
			}
			else
			{
				GlobalFree(hglob);
				hglob = NULL;
			}
		}
		*phglob = hglob;
		return hglob ? S_OK : E_OUTOFMEMORY;
	}

	void SetDropDescriptionBase(IDataObject* pDataObject, DROPIMAGETYPE type, LPCWSTR pszMsg, LPCWSTR pszDest)
	{
		FORMATETC fmtetc = { (CLIPFORMAT)RegisterClipboardFormatW(CFSTR_DROPDESCRIPTION), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM medium;

		medium.hGlobal = GlobalAlloc(GHND, sizeof(DROPDESCRIPTION));
		if (!medium.hGlobal) return;
		medium.tymed = TYMED_HGLOBAL;
		medium.pUnkForRelease = nullptr;

		DROPDESCRIPTION* pDropDesc = (DROPDESCRIPTION*)GlobalLock(medium.hGlobal);
		if (pDropDesc)
		{
			pDropDesc->type = type;
			wcscpy_s(pDropDesc->szMessage, pszMsg);
			wcscpy_s(pDropDesc->szInsert, pszDest);
			GlobalUnlock(medium.hGlobal);
		}

		if (FAILED(pDataObject->SetData(&fmtetc, &medium, TRUE)))
			ReleaseStgMedium(&medium);
	}

	CDataObject::CDataObject() : lRefCount(1)
	{
		this->_SetFORMATETC(&pFmtEtc[0], RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR), TYMED_HGLOBAL, -1, DVASPECT_CONTENT, nullptr);
		this->_SetFORMATETC(&pFmtEtc[1], RegisterClipboardFormat(CFSTR_FILECONTENTS), TYMED_HGLOBAL, 0, DVASPECT_CONTENT, nullptr);
	}

	CDataObject::CDataObject(IDataObject* pDataObject) : lRefCount(1), pDataObject(pDataObject)
	{
		if (this->pDataObject)
			this->pDataObject->AddRef();
		this->_SetFORMATETC(&pFmtEtc[0], RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR), TYMED_HGLOBAL, -1, DVASPECT_CONTENT, nullptr);
		this->_SetFORMATETC(&pFmtEtc[1], RegisterClipboardFormat(CFSTR_FILECONTENTS), TYMED_HGLOBAL, 0, DVASPECT_CONTENT, nullptr);
	}

	CDataObject::~CDataObject()
	{
		if (this->pDataObject)
			this->pDataObject->Release();
		for (MYOBJDATA& entry : pData)
			ReleaseStgMedium(&entry.stgm);
	}

	HRESULT STDMETHODCALLTYPE CDataObject::QueryInterface(REFIID riid, LPVOID* ppvObject)
	{
		if (riid == IID_IDataObject || riid == IID_IUnknown)
		{
			*ppvObject = static_cast<IDataObject*>(this);
			AddRef();
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE CDataObject::AddRef()
	{
		return InterlockedIncrement(&lRefCount);
	}

	ULONG STDMETHODCALLTYPE CDataObject::Release()
	{
		LONG nCount;
		if ((nCount = InterlockedDecrement(&lRefCount)) == 0)
			delete this;
		return nCount;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::GetData(LPFORMATETC pFmtEtc, LPSTGMEDIUM pMedium)
	{
		for (MYOBJDATA& entry : pData)
		{
			if (pFmtEtc->cfFormat == entry.fe.cfFormat && (pFmtEtc->tymed & entry.fe.tymed) && pFmtEtc->lindex == entry.fe.lindex)
				return _CopyMedium(&entry.stgm, pMedium, pFmtEtc);
		}
		if (pDataObject)
			return pDataObject->GetData(pFmtEtc, pMedium);
		ZeroMemory(pMedium, sizeof(*pMedium));
		switch (this->_GetDataIndex(pFmtEtc))
		{
		case 0:
			FILEGROUPDESCRIPTORW fgd;
			ZeroMemory(&fgd, sizeof(fgd));
			fgd.cItems = 1;
			StringCchCopyW(fgd.fgd[0].cFileName, ARRAYSIZE(fgd.fgd[0].cFileName), L"Unspecified");
			pMedium->tymed = TYMED_HGLOBAL;
			return CreateHGlobalFromBlob(&fgd, sizeof(fgd),	GMEM_MOVEABLE, &pMedium->hGlobal);
		case 1:
			pMedium->tymed = TYMED_HGLOBAL;
			return CreateHGlobalFromBlob("Unspecified", 11, GMEM_MOVEABLE, &pMedium->hGlobal);
		}

		return DV_E_FORMATETC;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::GetDataHere(LPFORMATETC pFmtEtc, LPSTGMEDIUM pMedium)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::QueryGetData(LPFORMATETC pFmtEtc)
	{
		if (!pFmtEtc) return E_INVALIDARG;
		for (MYOBJDATA& entry : pData)
		{
			if (pFmtEtc->cfFormat == entry.fe.cfFormat && (pFmtEtc->tymed & entry.fe.tymed))
				return S_OK;
		}
		if (this->_GetDataIndex(pFmtEtc) != -1)
			return S_OK;
		if (pDataObject)
			return pDataObject->QueryGetData(pFmtEtc);
		return DV_E_FORMATETC;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::GetCanonicalFormatEtc(LPFORMATETC pFmtEtcIn, LPFORMATETC pFmtEtcOut)
	{
		*pFmtEtcOut = *pFmtEtcIn;
		pFmtEtcOut->ptd = nullptr;
		return DATA_S_SAMEFORMATETC;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::SetData(LPFORMATETC pFmtEtc, LPSTGMEDIUM pMedium, BOOL fRelease)
	{
		if (!pFmtEtc || !pMedium)
			return E_INVALIDARG;
		for (MYOBJDATA& entry : pData)
		{
			if (entry.fe.cfFormat == pFmtEtc->cfFormat && entry.fe.lindex == pFmtEtc->lindex)
			{
				ReleaseStgMedium(&entry.stgm);
				if (fRelease)
					entry.stgm = *pMedium;
				else
					_CopyMedium(pMedium, &entry.stgm, pFmtEtc);
				return S_OK;
			}
		}
		MYOBJDATA newData;
		newData.fe = *pFmtEtc;
		if (fRelease)
			newData.stgm = *pMedium;
		else
			_CopyMedium(pMedium, &newData.stgm, pFmtEtc);
		pData.push_back(newData);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFmtEtc)
	{
		if (dwDirection != DATADIR_GET)
		{
			*ppEnumFmtEtc = nullptr;
			return E_NOTIMPL;
		}
		/* Note that this is for W2K and up only. Before this, this will NOT work! */
		return SHCreateStdEnumFmtEtc(ARRAYSIZE(pFmtEtc), pFmtEtc, ppEnumFmtEtc);
	}

	HRESULT STDMETHODCALLTYPE CDataObject::DAdvise(LPFORMATETC pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD* pdwConnection)
	{
		return OLE_E_ADVISENOTSUPPORTED;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::DUnadvise(DWORD dwConnection)
	{
		return OLE_E_ADVISENOTSUPPORTED;
	}

	HRESULT STDMETHODCALLTYPE CDataObject::EnumDAdvise(LPENUMSTATDATA* ppEnumAdvise)
	{
		return OLE_E_ADVISENOTSUPPORTED;
	}


	HRESULT CDataObject::_CopyMedium(STGMEDIUM* pMediumSrc, STGMEDIUM* pMediumDst, FORMATETC* pFmtEtc)
	{
		if (!pMediumSrc || !pMediumDst || !pFmtEtc)
			return E_INVALIDARG;
		pMediumDst->tymed = pMediumSrc->tymed;
		pMediumDst->pUnkForRelease = pMediumSrc->pUnkForRelease;
		if (pMediumDst->pUnkForRelease)
			pMediumDst->pUnkForRelease->AddRef();

		switch (pMediumSrc->tymed)
		{
		case TYMED_HGLOBAL:
		{
			pMediumDst->hGlobal = (HGLOBAL)OleDuplicateData(pMediumSrc->hGlobal, pFmtEtc->cfFormat, GMEM_MOVEABLE);
			return pMediumDst->hGlobal ? S_OK : E_OUTOFMEMORY;
		}

		case TYMED_ISTREAM:
		{
			pMediumDst->pstm = pMediumSrc->pstm;
			if (!pMediumDst->pstm)
				return E_UNEXPECTED;
			pMediumDst->pstm->AddRef();
			return S_OK;
		}

		default:
			return DV_E_TYMED;
		}
	}

	int CDataObject::_GetDataIndex(const FORMATETC* pfe)
	{
		for (int i = 0; i < ARRAYSIZE(pFmtEtc); i++)
		{
			if (pfe->cfFormat == pFmtEtc[i].cfFormat && (pfe->tymed & pFmtEtc[i].tymed))
				return i;
		}
		return -1;
	}

	void CDataObject::_SetFORMATETC(FORMATETC* pfe, UINT cf, TYMED tymed, LONG lindex, DWORD dwAspect, DVTARGETDEVICE* ptd)
	{
		pfe->cfFormat = (CLIPFORMAT)cf;
		pfe->tymed = tymed;
		pfe->lindex = lindex;
		pfe->dwAspect = dwAspect;
		pfe->ptd = ptd;
	}

	CDropSource::CDropSource() : lRefCount(1), bRightClick(FALSE) {}

	HRESULT STDMETHODCALLTYPE CDropSource::QueryInterface(REFIID riid, LPVOID* ppvObject)
	{
		if (riid == IID_IDropSource || riid == IID_IUnknown)
		{
			*ppvObject = static_cast<IDropSource*>(this);
			AddRef();
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE CDropSource::AddRef()
	{
		return InterlockedIncrement(&lRefCount);
	}

	ULONG STDMETHODCALLTYPE CDropSource::Release()
	{
		LONG nCount;
		if ((nCount = InterlockedDecrement(&lRefCount)) == 0)
			delete this;
		return nCount;
	}

	HRESULT STDMETHODCALLTYPE CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD dwKeyState)
	{
		if (fEscapePressed)
			return DRAGDROP_S_CANCEL;
		else if ((dwKeyState & MK_LBUTTON) == 0 && !bRightClick)
			return DRAGDROP_S_DROP;
		else if ((dwKeyState & MK_RBUTTON) == 0 && bRightClick)
			return DRAGDROP_S_DROP;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CDropSource::GiveFeedback(DWORD dwEffect)
	{
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}

	CDropTarget::CDropTarget() : pFormat(nullptr), lRefCount(1), dwLastEffect(DROPEFFECT_NONE)
	{
		if (pMinimal)
			pMinimal->QueryInterface(IID_IDropTargetHelper, (void**)&pDropTargetHelper);
	}

	CDropTarget::~CDropTarget()
	{
		pDropTargetHelper->Release();
		if (pFormat) delete[] pFormat;
	}

	HRESULT STDMETHODCALLTYPE CDropTarget::QueryInterface(REFIID riid, LPVOID* ppvObject)
	{
		if (riid == IID_IDropTarget || riid == IID_IUnknown)
		{
			*ppvObject = static_cast<IDropTarget*>(this);
			AddRef();
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE CDropTarget::AddRef()
	{
		return InterlockedIncrement(&lRefCount);
	}

	ULONG STDMETHODCALLTYPE CDropTarget::Release()
	{
		LONG nCount;
		if ((nCount = InterlockedDecrement(&lRefCount)) == 0)
			delete this;
		return nCount;
	}

	HRESULT STDMETHODCALLTYPE CDropTarget::DragEnter(IDataObject* pDataObject, DWORD dwKeyState, POINTL pt, DWORD* pdwEffect)
	{
		this->pDataObject = pDataObject;
		if (this->pDataObject) this->pDataObject->AddRef();
		if (isIconPressed)
		{
			*pdwEffect = DROPEFFECT_MOVE;
			bAllowDrop = TRUE;
		}
		else if (bAllowDrop = this->_QueryDataObject())
		{
			// Determine copy/move/link from item type and drive letter
			IShellItemArray* pItemArray = nullptr;
			HRESULT hr = SHCreateShellItemArrayFromDataObject(pDataObject, IID_PPV_ARGS(&pItemArray));
			if (SUCCEEDED(hr))
			{
				DWORD dwItemCount = 0;
				pItemArray->GetCount(&dwItemCount);
				if (dwItemCount > 0)
				{
					IShellItem* pItem = nullptr;
					if (SUCCEEDED(pItemArray->GetItemAt(0, &pItem)))
					{
						SFGAOF attributes;
						if (SUCCEEDED(pItem->GetAttributes(SHCIDS_COLUMNMASK, &attributes)))
						{
							bVirtual = !(attributes & SFGAO_CANMOVE);
							if (bVirtual)
								goto FOCUS;
						}
						LPWSTR pszFilePath = nullptr;
						if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
						{
							LPWSTR pszDesktopPath{};
							std::wstring desktopPath;
							HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &pszDesktopPath);
							if (SUCCEEDED(hr))
							{
								desktopPath = pszDesktopPath;
								CoTaskMemFree(pszDesktopPath);
							}
							if (!desktopPath.empty() && desktopPath.back() != L'\\')
								desktopPath += L'\\';
							bSameDrive = (pszFilePath[0] == desktopPath[0]);
							CoTaskMemFree(pszFilePath);
						}
					}
					pItem->Release();
				}
				pItemArray->Release();
			}
			///////////////////////////////////////////////////////////
		FOCUS:
			SetFocus(this->hWnd);
		}
		else
			*pdwEffect = DROPEFFECT_NONE;

		if (pDropTargetHelper)
			pDropTargetHelper->DragEnter(this->hWnd, pDataObject, (POINT*)&pt, *pdwEffect);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CDropTarget::DragOver(DWORD dwKeyState, POINTL pt, DWORD* pdwEffect)
	{
		if (bAllowDrop)
		{
			this->dwKeyState = dwKeyState;
			if (isIconPressed)
				*pdwEffect = DROPEFFECT_MOVE;
			else
			{
				*pdwEffect = this->_DropEffect(dwKeyState, pt, *pdwEffect);
				if (*pdwEffect != dwLastEffect)
				{
					dwLastEffect = *pdwEffect;
					switch (*pdwEffect)
					{
					case DROPEFFECT_COPY:
						this->_SetDropDescription(DROPIMAGE_COPY, LoadStrFromRes(49872, L"shell32.dll").c_str(), LoadStrFromRes(21769, L"shell32.dll").c_str());
						break;
					case DROPEFFECT_MOVE:
						this->_SetDropDescription(DROPIMAGE_MOVE, LoadStrFromRes(49873, L"shell32.dll").c_str(), LoadStrFromRes(21769, L"shell32.dll").c_str());
						break;
					case DROPEFFECT_LINK:
						this->_SetDropDescription(DROPIMAGE_LINK, LoadStrFromRes(49874, L"shell32.dll").c_str(), LoadStrFromRes(21769, L"shell32.dll").c_str());
						break;
					default:
						this->_SetDropDescription(DROPIMAGE_NONE, LoadStrFromRes(49879, L"shell32.dll").c_str(), LoadStrFromRes(21769, L"shell32.dll").c_str());
						break;
					}
				}
			}
			if (pDropTargetHelper)
				pDropTargetHelper->DragOver((POINT*)&pt, *pdwEffect);
		}
		else
			*pdwEffect = DROPEFFECT_NONE;

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CDropTarget::DragLeave()
	{
		if (pDataObject)
		{
			pDataObject->Release();
			pDataObject = nullptr;
		}
		dwLastEffect = DROPEFFECT_NONE;
		if (pDropTargetHelper)
			pDropTargetHelper->DragLeave();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CDropTarget::Drop(IDataObject* pDataObject, DWORD dwKeyState, POINTL pt, DWORD* pdwEffect)
	{
		if (pDropTargetHelper)
			pDropTargetHelper->Drop(pDataObject, (POINT*)&pt, *pdwEffect);

		if (isIconPressed)
		{
			SendMessageW(wnd->GetHWND(), WM_USER + 18, g_lockiconpos ? NULL : (WPARAM)&selectedLVItems, 0);
			return S_OK;
		}

		FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE };
		STGMEDIUM medium;
		ULONG lFmt;
		MYDROPDATA DropData;

		if (bAllowDrop)
		{
			/* Find the first matching CLIPFORMAT that I can handle. */
			for (lFmt = 0; lFmt < lNumFormats; lFmt++)
			{
				fmtetc.cfFormat = pFormat[lFmt];
				if (pDataObject->QueryGetData(&fmtetc) == S_OK)
					break;
			}
			/* If we found a matching format, then handle it now. */
			if (lFmt < lNumFormats)
			{
				/* Get the data being dragged. */
				pDataObject->GetData(&fmtetc, &medium);
				*pdwEffect = DROPEFFECT_NONE;
				/* If a callback procedure is defined, then use that. */
				if (pDropProc != NULL)
					*pdwEffect = (*pDropProc)(pDataObject, pFormat[lFmt], medium.hGlobal, this->hWnd, dwKeyState, pt, pUserData);
				/* Else, if a message is valid, then send that. */
				else if (nMsg != WM_NULL)
				{
					/* Fill the struct with the relevant data. */
					DropData.cf = pFormat[lFmt];
					DropData.dwKeyState = this->dwKeyState;
					DropData.hData = medium.hGlobal;
					DropData.pt = pt;

					/* And send the message. */
					*pdwEffect = (DWORD)SendMessageW(this->hWnd, nMsg, (WPARAM)&DropData, (LPARAM)pUserData);
				}
				ReleaseStgMedium(&medium);
			}
		}
		else
			*pdwEffect = DROPEFFECT_NONE;
		if (pDataObject)
		{
			pDataObject->Release();
			pDataObject = nullptr;
		}
		dwLastEffect = DROPEFFECT_NONE;
		return S_OK;
	}

	void CDropTarget::Initialize(LONG lRefCount, HWND hWnd, UINT nMsg, BOOL bAllowDrop, DWORD dwKeyState, ULONG lNumFormats, MYDDCALLBACK pDropProc, void* pUserData)
	{
		this->lRefCount = lRefCount;
		this->hWnd = hWnd;
		this->nMsg = nMsg;
		this->bAllowDrop = bAllowDrop;
		this->dwKeyState = dwKeyState;
		this->lNumFormats = lNumFormats;
		this->pDropProc = pDropProc;
		this->pUserData = pUserData;
	}

	ULONG CDropTarget::GetNumOfFormats()
	{
		return lNumFormats;
	}

	HWND CDropTarget::GetHWND()
	{
		return this->hWnd;
	}

	BOOL CDropTarget::_QueryDataObject()
	{
		for (ULONG i = 0; i < lNumFormats; i++)
		{
			FORMATETC fmtetc = { pFormat[i], NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE };
			if (pDataObject->QueryGetData(&fmtetc) == S_OK)
				return TRUE;
		}
		return FALSE;
	}

	DWORD CDropTarget::_DropEffect(DWORD dwKeyState, POINTL pt, DWORD dwAllowed)
	{
		DWORD dwEffect = DROPEFFECT_COPY;
		if (bSameDrive)
			dwEffect = DROPEFFECT_MOVE;
		if (dwKeyState & MK_CONTROL)
			dwEffect = dwAllowed & DROPEFFECT_COPY;
		if (dwKeyState & MK_SHIFT)
			dwEffect = dwAllowed & DROPEFFECT_MOVE;
		if ((dwKeyState & MK_SHIFT && dwKeyState & MK_CONTROL) || dwKeyState & MK_ALT)
			dwEffect = DROPEFFECT_LINK;
		if (bVirtual)
			dwEffect = DROPEFFECT_LINK;
		if (dwEffect == 0)
		{
			if (dwAllowed & DROPEFFECT_COPY)
				dwEffect = DROPEFFECT_COPY;
			else if (dwAllowed & DROPEFFECT_MOVE)
				dwEffect = DROPEFFECT_MOVE;
			else if (dwAllowed & DROPEFFECT_LINK)
				dwEffect = DROPEFFECT_LINK;
		}
		return dwEffect;
	}

	void CDropTarget::_SetDropDescription(DROPIMAGETYPE type, LPCWSTR pszMsg, LPCWSTR pszDest)
	{
		if (!pDataObject || !pszDest) return;
		SetDropDescriptionBase(pDataObject, type, pszMsg, pszDest);
	}

	UINT g_cRev = 0;
	const LARGE_INTEGER g_li0 = { 0 };

	LRESULT CALLBACK DragWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DESTROY:
			DestroyWindow(hwnd);
			return 0;
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	CMinimalDragImage::~CMinimalDragImage()
	{
		FreeDragData();
	}

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::QueryInterface(REFIID riid, LPVOID* ppvObject)
	{
		if (riid == IID_IDragSourceHelper || riid == IID_IDropTargetHelper)
		{
			if (riid == IID_IDragSourceHelper)
				*ppvObject = static_cast<IDragSourceHelper*>(this);
			if (riid == IID_IDropTargetHelper)
				*ppvObject = static_cast<IDropTargetHelper*>(this);
			AddRef();
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE CMinimalDragImage::AddRef()
	{
		return 2;
	}

	ULONG STDMETHODCALLTYPE CMinimalDragImage::Release()
	{
		return 1;
	}

	STDMETHODIMP CMinimalDragImage::InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject* pdtobj)
	{
		FreeDragData();
		HRESULT hr = _SetLayeredDragging(pshdi);
		if (SUCCEEDED(hr))
		{
			hr = _SaveToDataObject(pdtobj);
			if (FAILED(hr))
				FreeDragData();
		}
		return E_FAIL;
	}

	STDMETHODIMP CMinimalDragImage::InitializeFromWindow(HWND hwnd, POINT* ppt, IDataObject* pdtobj)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP CMinimalDragImage::DragEnter(HWND hwndTarget, IDataObject* pdtobj, POINT* ppt, DWORD dwEffect)
	{
		HRESULT hr = _LoadFromDataObject(pdtobj);
		if (SUCCEEDED(hr))
		{
			_hwndTarget = hwndTarget ? hwndTarget : GetDesktopWindow();
			_Single.bDragging = TRUE;
			_Single.hwndLock = _hwndTarget;
			_Single.bLocked = FALSE;
			_Single.idThreadEntered = GetCurrentThreadId();

			_ptDebounce.x = 0;
			_ptDebounce.y = 0;

			if (_shdi.hbmpDragImage)
			{
				if (_CreateDragWindow() && _hdcDragImage)
				{
					POINT ptSrc = { 0, 0 };
					POINT pt;

					SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
					DWORD dw = GetMessagePos();
					pt.x = ((int)LOWORD(dw)) - _shdi.ptOffset.x;
					pt.y = ((int)HIWORD(dw)) - _shdi.ptOffset.y;

					BLENDFUNCTION blend;
					blend.BlendOp = AC_SRC_OVER;
					blend.BlendFlags = 0;
					blend.AlphaFormat = AC_SRC_ALPHA;
					blend.SourceConstantAlpha = 0xFF;

					HDC hdc = GetDC(_hwnd);
					if (hdc)
					{
						DWORD fULWType = ULW_ALPHA;
						UpdateLayeredWindow(_hwnd, hdc, &pt, &(_shdi.sizeDragImage), _hdcDragImage, &ptSrc, NULL, &blend, fULWType);
						ReleaseDC(_hwnd, hdc);
					}
					hr = S_OK;
				}
			}
		}
		return hr;
	}

	STDMETHODIMP CMinimalDragImage::DragLeave()
	{
		if (_fCursorDataInited)
		{
			if (_hwnd)
				FreeDragData();
			else if (_Single.bDragging && _Single.idThreadEntered == GetCurrentThreadId())
			{
				_Single.bDragging = FALSE;
				DAD_SetDragImage((HIMAGELIST)-1, NULL);
			}
			_ptDebounce.x = 0;
			_ptDebounce.y = 0;
		}
		return S_OK;
	}

	STDMETHODIMP CMinimalDragImage::DragOver(POINT* ppt, DWORD dwEffect)
	{
		if (_fCursorDataInited)
		{
			// Avoid flickering
			ppt->x &= ~1;
			ppt->y &= ~1;

			if (_ptDebounce.x != ppt->x || _ptDebounce.y != ppt->y)
			{
				_ptDebounce.x = ppt->x;
				_ptDebounce.y = ppt->y;
				POINT pt;
				GetCursorPos(&pt);
				pt.x -= _shdi.ptOffset.x;
				pt.y -= _shdi.ptOffset.y;

				SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				UpdateLayeredWindow(_hwnd, NULL, &pt, NULL, NULL, NULL, 0, NULL, 0);
			}
		}
		return S_OK;
	}

	STDMETHODIMP CMinimalDragImage::Drop(IDataObject* pdtobj, POINT* ppt, DWORD dwEffect)
	{
		return DragLeave();
	}

	STDMETHODIMP CMinimalDragImage::Show(BOOL bShow)
	{
		return E_NOTIMPL;
	}

	void CMinimalDragImage::FreeDragData()
	{
		if (_hwnd)
		{
			SendMessageW(_hwnd, WM_DESTROY, NULL, NULL);
			_hwnd = NULL;
		}
		_fCursorDataInited = FALSE;
		if (_hbmpOld)
		{
			SelectObject(_hdcDragImage, _hbmpOld);
			_hbmpOld = NULL;
		}
		if (_hdcDragImage)
		{
			DeleteDC(_hdcDragImage);
			_hdcDragImage = NULL;
		}
		if (_shdi.hbmpDragImage)
			DeleteObject(_shdi.hbmpDragImage);
		ZeroMemory(&_Single, sizeof(_Single));
		ZeroMemory(&_shdi, sizeof(_shdi));
		_ptDebounce.x = 0;
		_ptDebounce.y = 0;
		_hwndTarget = _hwnd = NULL;
		_fCursorDataInited = _fLayeredSupported = FALSE;
	}

	void CMinimalDragImage::_InitDragData()
	{
		UINT uFlags = ILC_MASK | 0x100;
		if (localeType == 1)
			uFlags |= ILC_MIRROR;
		HDC hdc = GetDC(NULL);
		if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
			uFlags |= ILC_COLORDDB;
		ReleaseDC(NULL, hdc);
		_fCursorDataInited = TRUE;
	}

	HRESULT CMinimalDragImage::_LoadFromDataObject(IDataObject* pdtobj)
	{
		HRESULT hr;
		if (_fCursorDataInited || !pdtobj)
			hr = S_OK;
		else
		{
			FORMATETC fmte = { RegisterClipboardFormatW(L"DragContext"), NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM };
			STGMEDIUM medium = { 0 };
			hr = pdtobj->GetData(&fmte, &medium);
			if (SUCCEEDED(hr))
			{
				if (medium.hBitmap)
				{
					medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);
					DragContextHeader hdr;
					if (SUCCEEDED(IStream_Read(medium.pstm, &hdr, sizeof(hdr))))
					{
						if (hdr.fLayered)
						{
							STGMEDIUM mediumBits;
							FORMATETC fmte = { RegisterClipboardFormatW(L"DragImageBits"), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

							hr = pdtobj->GetData(&fmte, &mediumBits);
							if (SUCCEEDED(hr))
							{
								hr = _LoadLayeredBitmapBits(mediumBits.hGlobal);
								ReleaseStgMedium(&mediumBits);
							}
						}
					}
					medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);
				}
				if (SUCCEEDED(hr))
					_InitDragData();
				ReleaseStgMedium(&medium);
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_SaveToDataObject(IDataObject* pdtobj)
	{
		HRESULT hr = E_FAIL;
		if (_fCursorDataInited)
		{
			STGMEDIUM medium = { 0 };
			medium.tymed = TYMED_ISTREAM;

			if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &medium.pstm)))
			{
				DragContextHeader hdr = { 0 };
				hdr.fLayered = TRUE;

				ULONG ulWritten;
				if (SUCCEEDED(medium.pstm->Write(&hdr, sizeof(hdr), &ulWritten)) &&
					(ulWritten == sizeof(hdr)))
				{
					STGMEDIUM mediumBits = { 0 };
					mediumBits.tymed = TYMED_HGLOBAL;

					hr = _SaveLayeredBitmapBits(&mediumBits.hGlobal);
					if (SUCCEEDED(hr))
					{
						FORMATETC fmte = { RegisterClipboardFormatW(L"DragImageBits"), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
						hr = pdtobj->SetData(&fmte, &mediumBits, TRUE);
						if (FAILED(hr))
							ReleaseStgMedium(&mediumBits);
					}

					if (SUCCEEDED(hr))
					{
						medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);
						FORMATETC fmte = { RegisterClipboardFormatW(L"DragContext"), NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM };
						hr = pdtobj->SetData(&fmte, &medium, TRUE);
					}
				}

				if (FAILED(hr))
					ReleaseStgMedium(&medium);
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_LoadLayeredBitmapBits(HGLOBAL hGlobal)
	{
		HRESULT hr = E_FAIL;
		if (!_fCursorDataInited)
		{
			HDC hdcScreen = GetDC(NULL);
			if (hdcScreen)
			{
				void* pvDragStuff = (void*)GlobalLock(hGlobal);
				if (pvDragStuff)
				{
					CopyMemory(&_shdi, pvDragStuff, sizeof(SHDRAGIMAGE));
					BITMAPINFO bmi = { 0 };
					bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bmi.bmiHeader.biWidth = _shdi.sizeDragImage.cx;
					bmi.bmiHeader.biHeight = _shdi.sizeDragImage.cy;
					bmi.bmiHeader.biPlanes = 1;
					bmi.bmiHeader.biBitCount = 32;
					bmi.bmiHeader.biCompression = BI_RGB;
					_hdcDragImage = CreateCompatibleDC(hdcScreen);
					if (_hdcDragImage)
					{
						void* pvBits;
						_shdi.hbmpDragImage = CreateDIBSection(_hdcDragImage, &bmi, DIB_RGB_COLORS, &pvBits, NULL, NULL);
						if (_shdi.hbmpDragImage)
						{
							_hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, _shdi.hbmpDragImage);
							RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE) - 8);
							DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
							CopyMemory((RGBQUAD*)pvBits, (RGBQUAD*)pvStart, dwCount);
							hr = S_OK;
						}
					}
					GlobalUnlock(hGlobal);
				}
				ReleaseDC(NULL, hdcScreen);
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_SaveLayeredBitmapBits(HGLOBAL* phGlobal)
	{
		HRESULT hr = E_FAIL;
		if (_fCursorDataInited)
		{
			DWORD cbImageSize = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
			*phGlobal = GlobalAlloc(GPTR, cbImageSize + sizeof(SHDRAGIMAGE));
			if (*phGlobal)
			{
				void* pvDragStuff = GlobalLock(*phGlobal);
				CopyMemory(pvDragStuff, &_shdi, sizeof(SHDRAGIMAGE));

				void* pvBits;
				hr = _PreProcessDragBitmap(&pvBits) ? S_OK : E_FAIL;
				if (SUCCEEDED(hr))
				{
					RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE) - 8);
					DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
					CopyMemory((RGBQUAD*)pvStart, (RGBQUAD*)pvBits, dwCount);
				}
				GlobalUnlock(*phGlobal);
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_SetLayeredDragging(LPSHDRAGIMAGE pshdi)
	{
		if (pshdi->hbmpDragImage == INVALID_HANDLE_VALUE)
			return E_FAIL;
		_shdi = *pshdi;
		_InitDragData();
		return S_OK;
	}

	BOOL CMinimalDragImage::_CreateDragWindow()
	{
		if (_hwnd == NULL)
		{
			WNDCLASS wc = { 0 };
			wc.hInstance = HINST_THISCOMPONENT;
			wc.lpfnWndProc = DragWndProc;
			wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
			wc.lpszClassName = TEXT("SysDragImage");
			wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;
			RegisterClassW(&wc);

			_hwnd = CreateWindowExW(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
				L"SysDragImage", L"Drag", WS_POPUPWINDOW, 0, 0, 50, 50, NULL, NULL, HINST_THISCOMPONENT, NULL);

			if (!_hwnd)
				return FALSE;

			DWORD dwStyle = GetWindowLongW(_hwnd, GWL_EXSTYLE);
			DWORD dwNewStyle = (dwStyle & ~WS_EX_LAYOUTRTL);
			if (dwStyle != dwNewStyle)
				SetWindowLongW(_hwnd, GWL_EXSTYLE, dwNewStyle);
		}
		return TRUE;
	}

	BOOL CMinimalDragImage::_PreProcessDragBitmap(void** ppvBits)
	{
		BOOL fRet = FALSE;
		_hdcDragImage = CreateCompatibleDC(NULL);
		if (_hdcDragImage)
		{
			ULONG* pul;
			HBITMAP hbmpResult = NULL;
			HBITMAP hbmpOld;
			HDC hdcSource = NULL;
			BITMAPINFO bmi = { 0 };
			HBITMAP hbmp = _shdi.hbmpDragImage;
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = _shdi.sizeDragImage.cx;
			bmi.bmiHeader.biHeight = _shdi.sizeDragImage.cy;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			hdcSource = CreateCompatibleDC(_hdcDragImage);
			if (hdcSource)
			{
				hbmpResult = CreateDIBSection(_hdcDragImage, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
				if (hbmpResult)
				{
					_hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, hbmpResult);
					hbmpOld = (HBITMAP)SelectObject(hdcSource, hbmp);
					BitBlt(_hdcDragImage, 0, 0, _shdi.sizeDragImage.cx, _shdi.sizeDragImage.cy,
						hdcSource, 0, 0, SRCCOPY);
					pul = (ULONG*)*ppvBits;
					for (int Y = 0; Y < _shdi.sizeDragImage.cy; Y++)
					{
						int y = _shdi.sizeDragImage.cy - Y;
						for (int x = 0; x < _shdi.sizeDragImage.cx; x++)
						{
							RGBQUAD* prgb = (RGBQUAD*)&pul[Y * _shdi.sizeDragImage.cx + x];

							// Color key not supported for now
							int Alpha = prgb->rgbReserved;
							prgb->rgbRed = ((prgb->rgbRed * Alpha) + 128) / 255;
							prgb->rgbGreen = ((prgb->rgbGreen * Alpha) + 128) / 255;
							prgb->rgbBlue = ((prgb->rgbBlue * Alpha) + 128) / 255;
						}
					}

					DeleteObject(hbmp);
					_shdi.hbmpDragImage = hbmpResult;
					fRet = TRUE;
					if (hbmpOld)
						SelectObject(hdcSource, hbmpOld);
				}

				DeleteObject(hdcSource);
			}
		}
		return fRet;
	}

	void MyDragDropInit(HANDLE hHeap)
	{
		/* Only initialize this if not already set or used before. */
		if (g_hHeap == NULL && hHeap == NULL)
			g_hHeap = GetProcessHeap();
		else if (g_hHeap == NULL)
			g_hHeap = hHeap;

		/* Initialize OLE, to be sure. */
		OleInitialize(NULL);
	}

	CDropTarget* CreateDropTarget(CLIPFORMAT* pFormat, ULONG lFmt, HWND hWnd, UINT nMsg,
		DWORD(*pDropProc)(IDataObject* pDataObject, CLIPFORMAT cf, HGLOBAL hData, HWND hWnd, DWORD dwKeyState, POINTL pt, void* pUserData),
		void* pUserData)
	{
		CDropTarget* pRet = new CDropTarget();

		/* Allocate the nasty little thing, and supply space for CLIPFORMAT array in the process. */
		if (!pRet)
			return NULL;

		/* Set up the struct members. */
		pRet->Initialize(1, hWnd, nMsg, FALSE, 0, lFmt, pDropProc, pUserData);

		/* Set the format members. */
		pRet->pFormat = new (std::nothrow) CLIPFORMAT[lFmt];
		if (!pRet->pFormat)
		{
			delete pRet;
			return nullptr;
		}
		for (ULONG i = 0; i < lFmt; i++)
			pRet->pFormat[i] = pFormat[i];

		return pRet;
	}

	CDropTarget* MyRegisterDragDrop(HWND hWnd, CLIPFORMAT* pFormat, ULONG lFmt, UINT nMsg,
		MYDDCALLBACK pDropProc, void* pUserData)
	{
		IDropTarget* pTarget = CreateDropTarget(pFormat, lFmt, hWnd, nMsg, pDropProc, pUserData);

		/* First, create the target. */
		if (!pTarget)
			return NULL;

		/* Now, register for drop. If this fails, free my target the old-fashioned way, as no one knows about it anyway. */
		if (RegisterDragDrop(hWnd, pTarget) != S_OK)
		{
			pTarget->Release();
			return nullptr;
		}

		return (CDropTarget*)pTarget;
	}

	CDropTarget* MyRevokeDragDrop(IDropTarget* pTarget)
	{
		if (!pTarget)
			return nullptr;

		/* If there is a HWND, then revoke it as a drop object. */
		if (((CDropTarget*)pTarget)->GetHWND())
		{
			/* Now, this is a little precaution to know that this is an OK PMIIDROPTARGET object. */
			if (GetWindowLongW(((CDropTarget*)pTarget)->GetHWND(), GWLP_WNDPROC) != 0)
				RevokeDragDrop(((CDropTarget*)pTarget)->GetHWND());
		}

		/* Now, release the target. */
		pTarget->Release();

		return nullptr;
	}

	void PerformShellFileOp(HWND hWnd, LPCWSTR source, LPCWSTR destDir, DWORD effect)
	{
		if (effect == DROPEFFECT_LINK)
		{
			IShellLinkW* psl;
			HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
			if (SUCCEEDED(hr))
			{
				IPersistFile* ppf;
				psl->SetPath(source); 
				hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
				if (SUCCEEDED(hr))
				{
					LPCWSTR fileName = PathFindFileNameW(source);
					std::wstring linkPathIntermediate = destDir + (wstring)fileName;
					std::wstring linkPathIntermediate2 = LoadStrFromRes(4154, L"shell32.dll");
					size_t pos = linkPathIntermediate2.find(L" ()");
					if (pos != std::wstring::npos)
						linkPathIntermediate2.erase(pos, 3);
					UINT fileDup = 1;
					WCHAR linkPath[MAX_PATH];
					StringCchPrintfW(linkPath, MAX_PATH, linkPathIntermediate2.c_str(), linkPathIntermediate.c_str());
					while (PathFileExistsW(linkPath))
					{
						linkPathIntermediate2 = LoadStrFromRes(4154, L"shell32.dll");
						size_t pos = linkPathIntermediate2.find(L"(") + 1;
						linkPathIntermediate2.insert(pos, L"%d");
						StringCchPrintfW(linkPath, MAX_PATH, linkPathIntermediate2.c_str(), linkPathIntermediate.c_str(), ++fileDup);
					}
					hr = ppf->Save(linkPath, TRUE);
					ppf->Release();
				}
				psl->Release();
			}
			return;
		}

		std::wstring from = source + L'\0';
		std::wstring to = destDir + L'\0';

		SHFILEOPSTRUCTW sfos = { 0 };
		sfos.hwnd = hWnd;
		switch (effect)
		{
		case DROPEFFECT_COPY:
			sfos.wFunc = FO_COPY;
			break;
		case DROPEFFECT_MOVE:
			sfos.wFunc = FO_MOVE;
			break;
		}
		sfos.pFrom = from.c_str();
		sfos.pTo = to.c_str();
		sfos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;

		SHFileOperationW(&sfos);
	}

	DWORD TheDropProc(IDataObject* pDataObject, CLIPFORMAT cf, HGLOBAL hdata, HWND hwnd, DWORD key_state, POINTL pt, void* param)
	{
		if (cf == CF_HDROP)
		{
			//HDROP hDrop = (HDROP)hdata;
			//UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
			//DWORD effect = DROPEFFECT_COPY;
			//LPWSTR pszDesktopPath{};
			//std::wstring desktopPath;
			//HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &pszDesktopPath);
			//if (SUCCEEDED(hr))
			//{
			//	desktopPath = pszDesktopPath;
			//	CoTaskMemFree(pszDesktopPath);
			//}
			//if (!desktopPath.empty() && desktopPath.back() != L'\\')
			//	desktopPath += L'\\';

			//for (UINT i = 0; i < fileCount; i++)
			//{
			//	UINT cch = DragQueryFileW(hDrop, i, nullptr, 0);
			//	if (cch > 0)
			//	{
			//		WCHAR szFilePath[MAX_PATH];
			//		DragQueryFileW(hDrop, i, szFilePath, cch + 1);
			//		if (szFilePath[0] == desktopPath[0]) effect = DROPEFFECT_MOVE;
			//		if (key_state & MK_SHIFT) effect = DROPEFFECT_MOVE;
			//		if (key_state & MK_CONTROL) effect = DROPEFFECT_COPY;
			//		if ((key_state & MK_SHIFT && key_state & MK_CONTROL) || key_state & MK_ALT) effect = DROPEFFECT_LINK;

			//		PerformShellFileOp(hwnd, szFilePath, desktopPath.c_str(), effect);
			//	}
			//}
			DWORD effect = DROPEFFECT_COPY;
			IShellItemArray* pItemArray = nullptr;
			HRESULT hr = SHCreateShellItemArrayFromDataObject(pDataObject, IID_PPV_ARGS(&pItemArray));
			if (SUCCEEDED(hr))
			{
				DWORD dwItemCount = 0;
				pItemArray->GetCount(&dwItemCount);
				for (UINT i = 0; i < dwItemCount; i++)
				{
					IShellItem* pItem = nullptr;
					if (SUCCEEDED(pItemArray->GetItemAt(i, &pItem)))
					{
						SFGAOF attributes;
						pItem->GetAttributes(SHCIDS_COLUMNMASK, &attributes);
						LPWSTR pszFilePath = nullptr;
						if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
						{
							LPWSTR pszDesktopPath{};
							std::wstring desktopPath;
							HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &pszDesktopPath);
							if (SUCCEEDED(hr))
							{
								desktopPath = pszDesktopPath;
								CoTaskMemFree(pszDesktopPath);
							}
							if (!desktopPath.empty() && desktopPath.back() != L'\\')
								desktopPath += L'\\';
							if (pszFilePath[0] == desktopPath[0]) effect = DROPEFFECT_MOVE;
							if (key_state & MK_SHIFT) effect = DROPEFFECT_MOVE;
							if (key_state & MK_CONTROL) effect = DROPEFFECT_COPY;
							if ((key_state & MK_SHIFT && key_state & MK_CONTROL) || key_state & MK_ALT || !(attributes & SFGAO_CANMOVE))
								effect = DROPEFFECT_LINK;
							PerformShellFileOp(hwnd, pszFilePath, desktopPath.c_str(), effect);
							CoTaskMemFree(pszFilePath);
						}
					}
					pItem->Release();
				}
				pItemArray->Release();
			}
			return effect;
		}
		/* Default reaction is to do nothing: */
		return DROPEFFECT_NONE;
	}
}