// https://anton.maurovic.com/posts/win32-api-approach-to-windows-drag-and-drop/
// https://sourceforge.net/projects/win32cdnd/files/
// https://devblogs.microsoft.com/oldnewthing/20041206-00/?p=37133

#include "pch.h"
#include <wrl.h>
#include "DragAndDrop.h"
#include "DirectoryHelper.h"
#include "..\DirectDesktop.h"

using namespace Microsoft::WRL;

namespace DirectDesktop
{
	HANDLE g_hHeap;
#define MYDD_HEAP (g_hHeap == NULL ? (g_hHeap = GetProcessHeap()) : g_hHeap)

	void qmemcpy(void* dest, const void* src, size_t num)
	{
		if (num == 0) return;
		size_t align = alignof(void*);
		size_t offset = (uintptr_t)src & (align - 1);
		if (offset)
		{
			size_t align_num = align - offset;
			if (align_num > num) align_num = num;
			memcpy((char*)dest, (const char*)src, align_num);
			dest = (char*)dest + align_num;
			src = (const char*)src + align_num;
			num -= align_num;
		}
		while (num >= sizeof(size_t))
		{
			*(size_t*)dest = *(const size_t*)src;
			dest = (char*)dest + sizeof(size_t);
			src = (const char*)src + sizeof(size_t);
			num -= sizeof(size_t);
		}
		if (num)
			memcpy(dest, src, num);
	}

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
			if (pszDest) wcscpy_s(pDropDesc->szInsert, pszDest);
			GlobalUnlock(medium.hGlobal);
		}

		if (FAILED(pDataObject->SetData(&fmtetc, &medium, TRUE)))
			ReleaseStgMedium(&medium);
	}

	HRESULT DataObj_GetBlobWithIndex(IDataObject* pdtobj, CLIPFORMAT cf, void* pvData, size_t cbData, LONG lindex)
	{
		void* pv;
		FORMATETC fmtetc = { cf, 0, DVASPECT_CONTENT, lindex, TYMED_HGLOBAL };
		STGMEDIUM medium = { 0 };
		HRESULT hr = pdtobj->GetData(&fmtetc, &medium);
		if (SUCCEEDED(hr))
		{
			pv = GlobalLock(medium.hBitmap);
			if (pv)
			{
				if (GlobalSize(medium.hBitmap) >= cbData)
					CopyMemory(pvData, pv, cbData);
				else
					hr = E_UNEXPECTED;
				GlobalUnlock(medium.hBitmap);
			}
			else
				hr = E_UNEXPECTED;
			ReleaseStgMedium(&medium);
		}
		return hr;
	}

	HRESULT DataObj_SetBlobWithIndex(IDataObject* pdtobj, CLIPFORMAT cf, const void* pvData, size_t cbData, LONG lindex)
	{
		HRESULT hr = E_OUTOFMEMORY;
		HGLOBAL hglob = GlobalAlloc(GMEM_ZEROINIT, cbData);
		if (hglob)
		{
			CopyMemory(hglob, pvData, cbData);
			FORMATETC fmtetc = { cf, 0, DVASPECT_CONTENT, lindex, TYMED_HGLOBAL };
			STGMEDIUM medium = { 0 };
			medium.tymed = TYMED_HGLOBAL;
			medium.hGlobal = hglob;
			hr = pdtobj->SetData(&fmtetc, &medium, TRUE);
			if (FAILED(hr))
				GlobalFree(hglob);
		}
		return hr;
	}

	// This function takes a string with "%1" format and creates two strings (prefix & suffix) for later use
	// Example: pszFormat = "Using %1 is cool" -> ppszPrefix = "Using ", ppszSuffix = " is cool"
	HRESULT GetStringsFromFormat(WCHAR* pszFormat, WCHAR** ppszPrefix, WCHAR** ppszSuffix)
	{
		LPWSTR pszLoc = nullptr;
		HRESULT hr = SHLocalStrDupW(pszFormat, &pszLoc);
		if (SUCCEEDED(hr))
		{
			LPWSTR pszR = pszLoc;
			LPWSTR pszW = pszLoc;
			LPWSTR pszSplit = nullptr;
			while (*pszR)
			{
				if (*pszR == L'%')
				{
					pszR++;
					if (*pszR == L'%')
					{
						*pszW = L'%';
						pszW++;
					}
					else if (*pszR == L'1')
					{
						*pszW = L'\0';
						pszW++;
						pszSplit = pszW;
					}
					else
					{
						LocalFree(pszLoc);
						return E_FAIL;
					}
				}
				else
				{
					*pszW = *pszR;
					pszW++;
				}
				pszR++;
			}
			*pszW = L'\0';
			if (pszSplit)
			{
				*ppszPrefix = pszLoc;
				*ppszSuffix = pszSplit;
				hr = S_OK;
			}
			else
				hr = E_FAIL;
			LocalFree(pszLoc);
		}
		return hr;
	}

	CDropSource::CDropSource() : lRefCount(1), bRightClick(FALSE)
	{
		if (pMinimal)
			pMinimal->QueryInterface(IID_IDragSourceHelper, (void**)&pDragSourceHelper);
	}

	CDropSource::CDropSource(IDataObject* pdtobj) : lRefCount(1), bRightClick(FALSE)
	{
		if (pMinimal)
			pMinimal->QueryInterface(IID_IDragSourceHelper, (void**)&pDragSourceHelper);
		this->pdtobj = pdtobj;
		pdtobj->AddRef();
	}

	CDropSource::~CDropSource()
	{
		pDragSourceHelper->Release();
		if (pdtobj)
			pdtobj->Release();
	}

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
		if (IsAppThemed())
		{
			UINT uData = 0;
			CLIPFORMAT cf = RegisterClipboardFormatW(L"IsShowingLayered");
			if (SUCCEEDED(DataObj_GetBlobWithIndex(pdtobj, cf, &uData, sizeof(uData), -1)))
			{
				SetCursor(LoadCursorW(NULL, IDC_ARROW));
				UINT effectID = 2;
				switch (dwEffect & 7)
				{
				case DROPEFFECT_NONE:
					effectID = 1;
					break;
				case DROPEFFECT_COPY:
					effectID = 3;
					break;
				case DROPEFFECT_LINK:
					effectID = 4;
					break;
				}
				HWND hWnd{};
				cf = RegisterClipboardFormatW(L"DragWindow");
				if (SUCCEEDED(DataObj_GetBlobWithIndex(pdtobj, cf, &hWnd, 4, -1)))
				{
					RECT rcWindow;
					GetWindowRect(hWnd, &rcWindow);
					if (rcWindow.right - rcWindow.left > 0 || rcWindow.bottom - rcWindow.top > 0)
					{
						SendMessageW(hWnd, WM_USER + 2, effectID, NULL);
						return S_OK;
					}
				}
			}
		}
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
				FOCUS:
					;
				}
			}
			///////////////////////////////////////////////////////////
			SetFocus(this->hWnd);
		}
		else
			*pdwEffect = DROPEFFECT_NONE;

		if (pDropTargetHelper)
			pDropTargetHelper->DragEnter(this->hWnd, pDataObject, (POINT*)&pt, *pdwEffect);

		GetClientRect(hWnd, &rcDimensions);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CDropTarget::DragOver(DWORD dwKeyState, POINTL pt, DWORD* pdwEffect)
	{
		if (bAllowDrop)
		{
			this->dwKeyState = dwKeyState;
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
					if (isIconPressed)
						this->_SetDropDescription(g_lockiconpos ? DROPIMAGE_NONE : DROPIMAGE_NOIMAGE, LoadStrFromRes(4044).c_str(), nullptr);
					else
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
		else
			*pdwEffect = DROPEFFECT_NONE;
		if (pDropTargetHelper)
			pDropTargetHelper->DragOver((POINT*)&pt, *pdwEffect);

		if (pt.x <= 16 * g_flScaleFactor)
		{
			if (!(HIBYTE(ptLocFlags)))
				dwTickCountL = GetTickCount64();
			ptLocFlags = 256;
		}
		else if (pt.x > rcDimensions.right - 16 * g_flScaleFactor)
		{
			if (!(LOBYTE(ptLocFlags)))
				dwTickCountR = GetTickCount64();
			ptLocFlags = 1;
		}
		else
			ptLocFlags = 0;
		if (GetTickCount64() > dwTickCountL + 500 && ptLocFlags == 256 && g_currentPageID > 1)
		{
			g_currentPageID--;
			for (int i = 0; i < selectedLVItems.size(); i++)
				selectedLVItems[i]->SetPage(g_currentPageID);
			TriggerPageTransition(-1, rcDimensions);
			nextpageMain->SetVisible(true);
			if (g_currentPageID == 1) prevpageMain->SetVisible(false);
			dwTickCountL += 680;
		}
		if (GetTickCount64() > dwTickCountR + 500 && ptLocFlags == 1 && g_currentPageID < g_maxPageID)
		{
			g_currentPageID++;
			for (int i = 0; i < selectedLVItems.size(); i++)
				selectedLVItems[i]->SetPage(g_currentPageID);
			TriggerPageTransition(1, rcDimensions);
			prevpageMain->SetVisible(true);
			if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
			dwTickCountR += 680;
		}

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
			DWORD pdwEffect2 = _DropEffect(dwKeyState, pt, *pdwEffect);
			if (pdwEffect2 == DROPEFFECT_MOVE)
			{
				SendMessageW(wnd->GetHWND(), WM_USER + 18, g_lockiconpos ? NULL : (WPARAM)&selectedLVItems, 0);
				if (pDataObject)
				{
					pDataObject->Release();
					pDataObject = nullptr;
				}
				dwLastEffect = DROPEFFECT_NONE;
				return S_OK;
			}
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
		DWORD dwEffect = DROPEFFECT_MOVE;
		if (!bSameDrive && !isIconPressed)
			dwEffect = DROPEFFECT_COPY;
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
		if (!pDataObject || !pszMsg) return;
		SetDropDescriptionBase(pDataObject, type, pszMsg, pszDest);
	}

	DragImageFlags operator&(DragImageFlags lhs, DragImageFlags rhs)
	{
		return static_cast<DragImageFlags>(static_cast<DWORD>(lhs) & static_cast<DWORD>(rhs));
	}

	DragImageFlags operator|(DragImageFlags lhs, DragImageFlags rhs)
	{
		return static_cast<DragImageFlags>(static_cast<DWORD>(lhs) | static_cast<DWORD>(rhs));
	}

	const LARGE_INTEGER g_li0 = { 0 };

	CMinimalDragImage::~CMinimalDragImage()
	{
		_FreeDragData();
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

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject* pdtobj)
	{
		if (!pshdi || !pdtobj)
			return E_INVALIDARG;
		_FreeDragData();
		HRESULT hr = _SetLayeredDragging(pshdi);
		if (SUCCEEDED(hr))
		{
			hr = _SaveToDataObject(pdtobj);
			if (FAILED(hr))
				_FreeDragData();
		}
		return hr;
	}

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::InitializeFromWindow(HWND hwnd, POINT* ppt, IDataObject* pdtobj)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::DragEnter(HWND hwndTarget, IDataObject* pdtobj, POINT* ppt, DWORD dwEffect)
	{
		IUnknown_Set((IUnknown**)&_pdtobj, (IUnknown*)pdtobj);
		if (_pdtobj)
			_ExtractOneTimeData();
		HRESULT hr = _LoadFromDataObject(pdtobj);
		if (SUCCEEDED(hr))
		{
			_hwndTarget = hwndTarget ? hwndTarget : GetDesktopWindow();
			hr = _AddInfoToWindow();
			if (_shdi.hbmpDragImage)
			{
				if (SUCCEEDED(hr))
				{
					if (_CreateDragWindow() && _hdcDragImage)
					{
						DragOver(ppt, dwEffect);
						hr = S_OK;
					}
					else
						hr = E_FAIL;
				}
			}
		}
		return hr;
	}

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::DragLeave()
	{
		if (_flags & DIF_CURDATAINITED)
			_FreeDragData();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::DragOver(POINT* ppt, DWORD dwEffect)
	{
		if (_flags & DIF_CURDATAINITED)
		{
			POINT pt = { ppt->x, ppt->y };
			SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			HDC hdc = GetDC(_hwnd);
			if (hdc)
			{
				GetCursorPos(&pt);
				pt.x -= (_shdi.ptOffset.x + _rc.left);
				pt.y -= (_shdi.ptOffset.y + _rc.top);
				POINT ptSrc = { 0, 0 };
				SIZE sz = { _rc.right, _rc.bottom };
				BLENDFUNCTION blend = { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA };
				HDC hdcComposite = _hdcDragImage;
				if (_hbmpUnk)
				{
					hdcComposite = _hdcWindow;
					if (_flags & DIF_DEFAULTIMAGE)
					{
						// No idea why the -4 is needed
						pt.x -= 4;
						pt.y -= 4;
					}
				}
				UpdateLayeredWindow(_hwnd, hdc, &pt, &sz, hdcComposite, &ptSrc, _shdi.crColorKey, &blend, ULW_ALPHA);
				ReleaseDC(_hwnd, hdc);
			}
			_ExtractContinualData();
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::Drop(IDataObject* pdtobj, POINT* ppt, DWORD dwEffect)
	{
		return DragLeave();
	}

	HRESULT STDMETHODCALLTYPE CMinimalDragImage::Show(BOOL bShow)
	{
		return E_NOTIMPL;
	}

	IDataObject* CMinimalDragImage::GetDataObject()
	{
		return _pdtobj;
	}

	HRESULT CMinimalDragImage::_Create32BitHBITMAP(HBITMAP* phbm, SIZE* psz, HDC* phdc, HDC* phdc2, void** ppvBits)
	{
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = psz->cx;
		bmi.bmiHeader.biHeight = psz->cy;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		*phdc2 = CreateCompatibleDC(*phdc);
		if (*phdc2)
		{
			*phbm = CreateDIBSection(*phdc2, &bmi, DIB_RGB_COLORS, ppvBits, NULL, NULL);
			if (*phbm)
				return S_OK;
			DeleteObject(*phdc2);
		}
		return E_OUTOFMEMORY;
	}

	void CMinimalDragImage::_FreeDragData()
	{
		if (_hwnd)
		{
			SendMessageW(_hwnd, WM_USER + 1, NULL, NULL);
			_hwnd = NULL;
		}
		if (_hbmpOld)
		{
			SelectObject(_hdcDragImage, _hbmpOld);
			_hbmpOld = NULL;
		}
		if (_hbmpUnk)
		{
			DeleteObject(_hbmpUnk);
			_hbmpUnk = NULL;
		}
		if (_hdcDragImage)
		{
			DeleteDC(_hdcDragImage);
			_hdcDragImage = NULL;
		}
		if (_hdcWindow)
		{
			DeleteDC(_hdcWindow);
			_hdcWindow = NULL;
		}
		if (_hTheme)
		{
			CloseThemeData(_hTheme);
			_hTheme = NULL;
		}
		if (_shdi.hbmpDragImage)
			DeleteObject(_shdi.hbmpDragImage);
		ZeroMemory(&_shdi, sizeof(_shdi));
		if (_pdtobj)
		{
			BOOL fFlag = FALSE;
			CLIPFORMAT cf = RegisterClipboardFormatW(L"IsShowingLayered");
			DataObj_SetBlobWithIndex(_pdtobj, cf, &fFlag, sizeof(fFlag), -1);
			cf = RegisterClipboardFormatW(L"IsShowingText");
			DataObj_SetBlobWithIndex(_pdtobj, cf, &fFlag, sizeof(fFlag), -1);
		}
		_hwndTarget = _hwnd = NULL;
		_flags = DIF_NONE;
		_imgType = 0;
		_rc.left = 0;
		_rc.top = 0;
		ZeroMemory(&_desc, sizeof(DROPDESCRIPTION));
	}

	void CMinimalDragImage::_InitDragData()
	{
		_flags = _flags | DIF_CURDATAINITED;
		_pt.x = _shdi.ptOffset.x;
		_pt.y = _shdi.ptOffset.y;
	}

	HRESULT CMinimalDragImage::_LoadFromDataObject(IDataObject* pdtobj)
	{
		HRESULT hr;
		if ((_flags & DIF_CURDATAINITED) || !pdtobj)
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
		if (_flags & DIF_CURDATAINITED)
		{
			STGMEDIUM medium = { 0 };
			medium.tymed = TYMED_ISTREAM;
			if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &medium.pstm)))
			{
				DragContextHeader hdr = { 0 };
				hdr.fLayered = TRUE;
				ULONG ulWritten;
				if (SUCCEEDED(medium.pstm->Write(&hdr, sizeof(hdr), &ulWritten)) &&	(ulWritten == sizeof(hdr)))
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
				UINT uFlag = 1;
				CLIPFORMAT cf = RegisterClipboardFormatW(L"DragSourceHelperFlags");
				hr = DataObj_SetBlobWithIndex(pdtobj, cf, &uFlag, sizeof(uFlag), -1);
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_LoadLayeredBitmapBits(HGLOBAL hGlobal)
	{
		HRESULT hr = E_FAIL;
		if (!(_flags & DIF_CURDATAINITED))
		{
			HDC hdcScreen = GetDC(NULL);
			if (hdcScreen)
			{
				void* pvDragStuff = (void*)GlobalLock(hGlobal);
				if (pvDragStuff)
				{
					qmemcpy(&_shdi, pvDragStuff, sizeof(SHDRAGIMAGE) - 8);
					void* pvBits;
					_Create32BitHBITMAP(&_shdi.hbmpDragImage, &_shdi.sizeDragImage, &hdcScreen, &_hdcDragImage, &pvBits);
					if (_shdi.hbmpDragImage)
					{
						_hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, _shdi.hbmpDragImage);
						RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE) - 8);
						DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
						CopyMemory((RGBQUAD*)pvBits, (RGBQUAD*)pvStart, dwCount);
						hr = S_OK;
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
		if (_flags & DIF_CURDATAINITED)
		{
			DWORD cbImageSize = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
			*phGlobal = GlobalAlloc(GPTR, cbImageSize + sizeof(SHDRAGIMAGE) - 8);
			if (*phGlobal)
			{
				void* pvDragStuff = GlobalLock(*phGlobal);
				CopyMemory(pvDragStuff, &_shdi, sizeof(SHDRAGIMAGE) - 8);

				if (_shdi.hbmpDragImage)
				{
					void* pvBits;
					hr = _PreProcessDragBitmap(&pvBits) ? S_OK : E_FAIL;
					if (SUCCEEDED(hr))
					{
						RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE) - 8);
						DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
						CopyMemory((RGBQUAD*)pvStart, (RGBQUAD*)pvBits, dwCount);
					}
					else
						hr = E_FAIL;
				}
				else
					hr = S_OK;
				GlobalUnlock(*phGlobal);
				if (FAILED(hr))
				{
					GlobalFree(*phGlobal);
					*phGlobal = NULL;
				}
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_SetLayeredDragging(LPSHDRAGIMAGE pshdi)
	{
		if (pshdi->hbmpDragImage == INVALID_HANDLE_VALUE)
			return E_FAIL;
		qmemcpy(&_shdi, pshdi, sizeof(SHDRAGIMAGE) - 8);
		_InitDragData();
		return S_OK;
	}

	struct WINDOWCOMPOSITIONATTRIBDATA
	{
		DWORD dwAttribute;
		PVOID pvData;
		SIZE_T cbData;
	};
	typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

	BOOL CMinimalDragImage::_CreateDragWindow()
	{
		if (!_hwnd)
		{
			WNDCLASS wc = { 0 };
			wc.hInstance = HINST_THISCOMPONENT;
			wc.lpfnWndProc = s_DragWndProc;
			wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
			wc.lpszClassName = TEXT("SysDragImage");
			wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;
			RegisterClassW(&wc);

			_hwnd = CreateWindowExW(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
				L"SysDragImage", L"Drag", WS_POPUPWINDOW, 0, 0, 50, 50, NULL, NULL, HINST_THISCOMPONENT, this);

			if (!_hwnd)
				return FALSE;

			HMODULE hUser = GetModuleHandleW(L"user32.dll");
			if (hUser)
			{
				pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
					(pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

				if (SetWindowCompositionAttribute)
				{
					int exclude = 1;
					WINDOWCOMPOSITIONATTRIBDATA data = { 13, &exclude, sizeof(exclude) };
					SetWindowCompositionAttribute(_hwnd, &data);
				}
			}
			if (_pdtobj)
			{
				BOOL fLayered = TRUE;
				CLIPFORMAT cf = RegisterClipboardFormatW(L"IsShowingLayered");
				HRESULT hr = DataObj_SetBlobWithIndex(_pdtobj, cf, &fLayered, sizeof(fLayered), -1);
				if (SUCCEEDED(hr))
				{
					cf = RegisterClipboardFormatW(L"DragWindow");
					hr = DataObj_SetBlobWithIndex(_pdtobj, cf, &_hwnd, 4, -1);
				}
			}

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
			HBITMAP hbmp = _shdi.hbmpDragImage;

			if (SUCCEEDED(_Create32BitHBITMAP(&hbmpResult, &_shdi.sizeDragImage, &_hdcDragImage, &hdcSource, ppvBits)))
			{
				_hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, hbmpResult);
				hbmpOld = (HBITMAP)SelectObject(hdcSource, hbmp);
				BitBlt(_hdcDragImage, 0, 0, _shdi.sizeDragImage.cx, _shdi.sizeDragImage.cy,	hdcSource, 0, 0, SRCCOPY);
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
				DeleteObject(hdcSource);
			}
		}
		return fRet;
	}

	void CMinimalDragImage::_PreProcessGDIBitmap(RECT* prc)
	{
		int pIndex{};
		for (LONG top = prc->top; top <= prc->bottom; top++)
		{
			for (LONG left = prc->left; left <= prc->right; left++)
			{
				pIndex = left + _rc.right * (_rc.bottom - top);
				*((BYTE*)_pvBits + 4 * pIndex + 3) = 0xFF;
			}
		}
	}

	void CMinimalDragImage::_ExtractOneTimeData()
	{
		ComPtr<IShellItemArray> pItemArray = nullptr;
		HRESULT hr = SHCreateShellItemArrayFromDataObject(_pdtobj, IID_PPV_ARGS(&pItemArray));
		pItemArray->GetCount(&_dwCount);
		UINT uData = 0;
		CLIPFORMAT cf = RegisterClipboardFormatW(L"IsShowingText");
		if ((SUCCEEDED(DataObj_GetBlobWithIndex(_pdtobj, cf, &uData, sizeof(uData), -1)), uData))
			_flags = _flags | DIF_SHOWTEXT;
		cf = RegisterClipboardFormatW(L"UsingDefaultDragImage");
		if ((SUCCEEDED(DataObj_GetBlobWithIndex(_pdtobj, cf, &uData, sizeof(uData), -1)), uData))
			_flags = _flags | DIF_DEFAULTIMAGE;
		cf = RegisterClipboardFormatW(L"DragSourceHelperFlags");
		if ((SUCCEEDED(DataObj_GetBlobWithIndex(_pdtobj, cf, &uData, sizeof(uData), -1)), uData))
			_flags = _flags | DIF_HELPERFLAG;
		cf = RegisterClipboardFormatW(L"IsComputingImage");
		if ((SUCCEEDED(DataObj_GetBlobWithIndex(_pdtobj, cf, &uData, sizeof(uData), -1)), uData))
			_flags = _flags | DIF_COMPUTINGIMG;
		_ExtractContinualData();
	}

	void CMinimalDragImage::_ExtractContinualData()
	{
		UINT uOld = (_flags & DIF_DISABLETEXT) ? 1 : 0, uNew = 0;
		if (_pdtobj)
		{
			CLIPFORMAT cf = RegisterClipboardFormatW(L"DisableDragText");
			if (FAILED(DataObj_GetBlobWithIndex(_pdtobj, cf, &uNew, sizeof(uNew), -1)))
				uNew = 0;
		}
		if (uNew)
			_flags = _flags | DIF_DISABLETEXT;
		else
			_flags = _flags & static_cast<DragImageFlags>(0xFFFFFFFF - DIF_DISABLETEXT);
		if (uOld != uNew)
			_flags = _flags | DIF_CANADDINFO;
		uNew = (_flags & DIF_COMPUTINGIMG) ? 1 : 0;
		if (_pdtobj)
		{
			CLIPFORMAT cf = RegisterClipboardFormatW(L"IsComputingImage");
			if (FAILED(DataObj_GetBlobWithIndex(_pdtobj, cf, &uOld, sizeof(uOld), -1)))
				uOld = 0;
		}
		else
			uOld = 0;
		if (uNew)
			_flags = _flags | DIF_COMPUTINGIMG;
		else
			_flags = _flags & static_cast<DragImageFlags>(0xFFFFFFFF - DIF_COMPUTINGIMG);

		// 0.5.6.1: Faulty logic, will be dealt with later...

		HWND hWnd = nullptr;
		CLIPFORMAT cf = RegisterClipboardFormatW(L"DragWindow");
		if (FAILED(DataObj_GetBlobWithIndex(_pdtobj, cf, &hWnd, 4, -1)))
			hWnd = nullptr;
		if (/*uNew && !uOld &&*/ _pdtobj && hWnd)
		{
			//if (_hdcDragImage)
			//{
			//	DeleteDC(_hdcDragImage);
			//	_hdcDragImage = 0;
			//}
			//cf = RegisterClipboardFormatW(L"ComputedDragImage");
			//HRESULT hr = DataObj_GetBlobWithIndex(_pdtobj, cf, &uOld, sizeof(uOld), -1);
			//if (FAILED(hr))
			//{
			//	uOld = 0;
			//}
			//_shdi.hbmpDragImage = (HBITMAP)uOld;
			//if (uOld)
			//{
			//	_SaveToDataObject(_pdtobj);
			//	if (_hdcDragImage)
			//	{
			//		DeleteDC(_hdcDragImage);
			//		_hdcDragImage = 0;
			//	}
			//	_LoadFromDataObject(_pdtobj);
			//}
			_flags = _flags | DIF_CANADDINFO;

			SendMessageW(hWnd, WM_USER + 3, NULL, NULL);
		}
		FORMATETC fmtetc = { RegisterClipboardFormatW(CFSTR_DROPDESCRIPTION), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM medium = { 0 };
		if (_pdtobj && SUCCEEDED(_pdtobj->GetData(&fmtetc, &medium)))
		{
			void* pv = GlobalLock(medium.hBitmap);
			if (pv)
			{
				if (!RtlEqualMemory(pv, &_desc, sizeof(DROPDESCRIPTION)))
				{
					_flags = _flags | DIF_CANADDINFO;
					qmemcpy(&_desc, pv, sizeof(DROPDESCRIPTION));
				}
				GlobalUnlock(medium.hBitmap);
			}
			ReleaseStgMedium(&medium);
		}
	}

	HRESULT CMinimalDragImage::_AddInfoToWindow()
	{
		RECT rc, rcContent, rcExtent;
		HRESULT hr;
		if (!_hdcWindow)
			_hdcWindow = CreateCompatibleDC(_hdcDragImage);
		if (!_hTheme)
			_hTheme = OpenThemeData(_hwnd, L"DragDrop");
		if (_hdcWindow && _hTheme)
		{
			SIZE sz = {};
			HDC hdcDragBackground;
			if (_hbmpUnk || (_rc.right = _shdi.sizeDragImage.cx, _rc.bottom = _shdi.sizeDragImage.cy,
				_ComputeFinalSize(), sz.cx = _rc.right, sz.cy = _rc.bottom,
				_Create32BitHBITMAP(&_hbmpUnk, &sz, &_hdcWindow, &hdcDragBackground, &_pvBits), _hbmpUnk))
			{
				ZeroMemory(_pvBits, _rc.right * _rc.bottom * 4);
				SelectObject(_hdcWindow, _hbmpUnk);
				hr = S_OK;
				SIZE sz = { _rc.left, _rc.top };
				rcContent.left = sz.cx, rcContent.top = sz.cy;
				rcContent.right = sz.cx + _shdi.sizeDragImage.cx;
				rcContent.bottom = sz.cy + _shdi.sizeDragImage.cy;
				if (_flags & DIF_DEFAULTIMAGE)
				{
					ZeroMemory(&rc, sizeof(rc));
					if (SUCCEEDED(_GetImageBackgroundRect(&rc)))
					{
						OffsetRect(&rc, _rc.left, _rc.top);
						DrawThemeBackground(_hTheme, _hdcWindow, 7, 0, &rc, nullptr);
						GetThemeBackgroundContentRect(_hTheme, _hdcWindow, 7, 0, &rc, &rcContent);
					}
					sz.cx = rcContent.left;
					sz.cy = rcContent.top;
				}
				BLENDFUNCTION blend = { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA };
				GdiAlphaBlend(_hdcWindow, rcContent.left, rcContent.top, rcContent.right - rcContent.left, rcContent.bottom - rcContent.top,
					_hdcDragImage, 0, 0, _shdi.sizeDragImage.cx, _shdi.sizeDragImage.cy, blend);
				_pt.x = rcContent.right + _shdi.ptOffset.x - _rc.left;
				_pt.y = rcContent.bottom + _shdi.ptOffset.y - _rc.top;
				if ((_flags & DIF_DEFAULTIMAGE) && _dwCount > 1)
				{
					WCHAR pszCount[12];
					StringCchPrintfW(pszCount, 12, L"%d", _dwCount);
					if (SUCCEEDED(GetThemeTextExtent(_hTheme, _hdcWindow, 8, 0, pszCount, -1, 0, 0, &rcExtent)))
					{
						rc.left = (rcContent.right + rcContent.left - rcExtent.right + rcExtent.left) / 2;
						rc.top = (rcContent.right + rcContent.left - rcExtent.bottom + rcExtent.top) / 2;
						rc.right = rc.left + rcExtent.right - rcExtent.left;
						rc.bottom = rc.top + rcExtent.bottom - rcExtent.top;
						if (SUCCEEDED(GetThemeBackgroundExtent(_hTheme, _hdcWindow, 8, 0, &rc, &rcExtent)))
						{
							DrawThemeBackground(_hTheme, _hdcWindow, 8, 0, &rcExtent, nullptr);
							DrawThemeText(_hTheme, _hdcWindow, 8, 0, pszCount, -1, NULL, NULL, &rc);
							_PreProcessGDIBitmap(&rc);
						}
					}
				}
				if (wcslen(_desc.szMessage))
				{
					if (_pdtobj)
					{
						UINT uShowText = TRUE;
						CLIPFORMAT cf = RegisterClipboardFormatW(L"IsShowingText");
						DataObj_SetBlobWithIndex(_pdtobj, cf, &uShowText, sizeof(uShowText), -1);
					}
					_flags = _flags | DIF_SHOWTEXT;
				}
				if (!(_flags & DIF_DISABLETEXT) || _imgType != 0 || GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_SHIFT) < 0 || GetKeyState(VK_MENU) < 0)
					_DrawImageAndDesc(_desc.szMessage, _desc.szInsert);
			}
			else
				hr = E_FAIL;
		}
		else
		{
			hr = S_OK;
			_pt.x = _shdi.ptOffset.x;
			_pt.y = _shdi.ptOffset.y;
		}
		_flags = _flags & static_cast<DragImageFlags>(0xFFFFFFFF - DIF_CANADDINFO);
		return hr;
	}

	void CMinimalDragImage::_ComputeFinalSize()
	{
		RECT rc, rc2, rc3, rcFinal = { 0, 0, _rc.right, _rc.bottom };
		if (rcFinal.right < 1)
			rcFinal.right = 1;
		if (rcFinal.bottom < 1)
			rcFinal.bottom = 1;
		if (_flags & DIF_DEFAULTIMAGE)
		{
			ZeroMemory(&rc, sizeof(rc));
			_GetImageBackgroundRect(&rc);
			UnionRect(&rcFinal, &rcFinal, &rc);
		}
		ZeroMemory(&rc2, sizeof(rc2));
		_GetEffectImageRect(&rc2);
		UnionRect(&rcFinal, &rcFinal, &rc2);
		if (_flags & (DIF_DEFAULTIMAGE | DIF_HELPERFLAG))
		{
			ZeroMemory(&rc, sizeof(rc));
			_GetTextRect(&rc);
			ZeroMemory(&rc3, sizeof(rc3));
			_GetTooltipRect(&rc2, &rc, &rc3);
			UnionRect(&rcFinal, &rcFinal, &rc3);
		}
		_rc.right = rcFinal.right - rcFinal.left;
		_rc.bottom = rcFinal.bottom - rcFinal.top;
	}

	HRESULT CMinimalDragImage::_GetEffectImageRect(RECT* prc)
	{
		SIZE sz = {};
		HRESULT hr = GetThemePartSize(_hTheme, _hdcWindow, 1, 0, 0, TS_DRAW, &sz);
		if (SUCCEEDED(hr))
		{
			RECT rcBounds = { 0, 0, sz.cx, sz.cy }, rcContent;
			hr = GetThemeBackgroundContentRect(_hTheme, _hdcWindow, 1, 0, &rcBounds, &rcContent);
			if (SUCCEEDED(hr))
			{
				int x = ((_flags & DIF_DEFAULTIMAGE) ? 16 : 12) * g_flScaleFactor;
				int y = ((_flags & DIF_DEFAULTIMAGE) ? 17 : 13) * g_flScaleFactor;
				prc->left = x + _shdi.ptOffset.x;
				prc->top = y + _shdi.ptOffset.y;
				int iVal{};
				hr = GetThemeMetric(_hTheme, NULL, 0, 0, 2417, &iVal);
				if (SUCCEEDED(hr))
				{
					prc->right = prc->left + rcContent.right - rcContent.left;
					if (iVal < sz.cy)
						iVal = sz.cy;
					prc->bottom = iVal + prc->top;
				}
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_GetImageBackgroundRect(RECT* prc)
	{
		SIZE sz = {};
		HRESULT hr = GetThemePartSize(_hTheme, _hdcWindow, 7, 0, 0, TS_DRAW, &sz);
		if (SUCCEEDED(hr))
		{
			prc->left = 0, prc->top = 0;
			prc->right = sz.cx, prc->bottom = sz.cy;
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_GetTextRect(RECT* prc)
	{
		RECT rcExtent;
		HRESULT hr = GetThemeTextExtent(_hTheme, _hdcWindow, 0, 0, L"Mq", -1, 0, 0, &rcExtent);
		if (SUCCEEDED(hr))
		{
			RECT rc;
			hr = _GetEffectImageRect(&rc);
			if (SUCCEEDED(hr))
			{
				prc->left = rc.right;
				prc->top = rc.top;
				prc->right = 300 * g_flScaleFactor + prc->left;
				int iVal{};
				hr = GetThemeMetric(_hTheme, NULL, 0, 0, 2417, &iVal);
				if (SUCCEEDED(hr))
				{
					if (iVal < rcExtent.bottom - rcExtent.top)
						iVal = rcExtent.bottom - rcExtent.top;
					prc->bottom = iVal + prc->top;
				}
			}
		}
		return hr;
	}

	HRESULT CMinimalDragImage::_GetTooltipRect(RECT* prcSrc, RECT* prcSrc2, RECT* prcDst)
	{
		prcDst->left = prcSrc->left - g_flScaleFactor;
		prcDst->top = prcSrc2->top;
		prcDst->right = prcSrc2->right + 7 * g_flScaleFactor;
		prcDst->bottom = prcSrc2->bottom;
		return S_OK;
	}

	void CMinimalDragImage::_DrawImageAndDesc(LPWSTR szMessage, LPWSTR szInsert)
	{
		if (_desc.type != DROPIMAGE_NOIMAGE)
		{
			int iPartId;
			int width;
			RECT rc = { 0 }, rcBounds = { 0 }, rcExtent, rcSize1, rcSize2, rcEnsure1 = { 0 }, rcEnsure2 = { 0 };
			bool hasText;
			switch (_desc.type)
			{
			case DROPIMAGE_NONE:
				iPartId = 6;
				break;
			case DROPIMAGE_COPY:
			case DROPIMAGE_MOVE:
			case DROPIMAGE_LINK:
				iPartId = _desc.type;
				break;
			case DROPIMAGE_LABEL:
				iPartId = 3;
				break;
			case DROPIMAGE_WARNING:
				iPartId = 5;
				break;
			case DROPIMAGE_INVALID:
				switch (_imgType)
				{
				case 1:
				case 2:
					iPartId = _imgType;
					break;
				case 3:
				case 4:
					iPartId = _imgType + 1;
					break;
				default:
					iPartId = 6;
					break;
				}
				break;
			}
			HRESULT hr = _GetEffectImageRect(&rc);
			if ((_flags & (DIF_DEFAULTIMAGE | DIF_HELPERFLAG)) && (_flags & DIF_SHOWTEXT) && SUCCEEDED(_GetTextRect(&rcBounds)))
			{
				SHStripMneumonicW(szMessage);
				SHStripMneumonicW(szInsert);

				WCHAR* pszPrefix;
				WCHAR* pszSuffix;
				if (SUCCEEDED(GetStringsFromFormat(szMessage, &pszPrefix, &pszSuffix)))
				{
					hasText = true;
					width = _SizeDescriptionLine(iPartId, pszPrefix, szInsert, pszSuffix, &rcBounds, &rcSize1, &rcSize2, &rcExtent);
				}
				else
				{
					hasText = false;
					ZeroMemory(&rcExtent, sizeof(rcExtent));
					GetThemeTextExtent(_hTheme, _hdcWindow, iPartId, 2, szMessage, -1, 0, &rcBounds, &rcExtent);
					width = rcExtent.right - rcExtent.left;
				}
				if (width <= rcBounds.right - rcBounds.left)
					rcBounds.right = rcBounds.left + width;
				_GetTooltipRect(&rc, &rcBounds, &rcEnsure1);
				LONG left = rcEnsure1.left;
				LONG top = rcEnsure1.top;
				int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
				int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
				int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN) + x;
				int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN) + y;
				if (_hwnd)
				{
					GetWindowRect(_hwnd, &rcEnsure2);
					if (_rc.left > 0)
						OffsetRect(&rcEnsure1, -rcEnsure1.left, 0);
					if (_rc.top > 0)
						OffsetRect(&rcEnsure1, 0, -rcEnsure1.top);
					OffsetRect(&rcEnsure1, rcEnsure2.left, rcEnsure2.top);
					if (rcEnsure1.right > cx)
						OffsetRect(&rcEnsure1, cx - rcEnsure1.right, 0);
					if (rcEnsure1.bottom > cy)
						OffsetRect(&rcEnsure1, 0, cy - rcEnsure1.bottom);
					if (rcEnsure1.left < x)
						OffsetRect(&rcEnsure1, x - rcEnsure1.left, 0);
					if (rcEnsure1.top < y)
						OffsetRect(&rcEnsure1, 0, y - rcEnsure1.top);
					OffsetRect(&rcEnsure1, -rcEnsure2.left, -rcEnsure2.top);
					if (rcEnsure1.left >= 0)
					{
						if (_rc.left > 0)
						{
							int boundX = cx - rcEnsure1.right - rcEnsure2.left;
							if (boundX > 0)
							{
								if (boundX > _rc.left)
								{
									_rc.left = 0;
									OffsetRect(&rcEnsure1, boundX, 0);
								}
								else
								{
									_rc.left -= boundX;
									OffsetRect(&rcEnsure1, -rcEnsure1.left, 0);
								}
							}
						}
					}
					else
					{
						_rc.left -= rcEnsure1.left;
						OffsetRect(&rcEnsure1, -rcEnsure1.left, 0);
					}
					if (rcEnsure1.top >= 0)
					{
						if (_rc.top > 0)
						{
							int boundY = cy - rcEnsure1.bottom - rcEnsure2.top;
							if (boundY > 0)
							{
								if (boundY > _rc.top)
								{
									_rc.top = 0;
									OffsetRect(&rcEnsure1, 0, boundY);
								}
								else
								{
									_rc.top -= boundY;
									OffsetRect(&rcEnsure1, 0, -rcEnsure1.top);
								}
							}
						}
					}
					else
					{
						_rc.top -= rcEnsure1.top;
						OffsetRect(&rcEnsure1, 0, -rcEnsure1.top);
					}
					OffsetRect(&rc, rcEnsure1.left - left, rcEnsure1.top - top);
					OffsetRect(&rcBounds, rcEnsure1.left - left, rcEnsure1.top - top);
				}
				_SizeDescriptionLine(iPartId, pszPrefix, szInsert, pszSuffix, &rcBounds, &rcSize1, &rcSize2, &rcExtent);
				_DrawTooltipBackground(&rc, &rcBounds, width);
				if (localeType == 1)
				{
					int cxNew = rc.right - rc.left;
					int cxBoundsNew = rcBounds.right - rcBounds.left;
					rcBounds.left = rc.left + 6 * g_flScaleFactor;
					rcBounds.right = rcBounds.left + cxBoundsNew;
					rc.left = rcBounds.right + 2 * g_flScaleFactor;
					rc.right = rc.left + cxNew;
					if (hasText)
					{
						int cxSize1New = rcSize1.right - rcSize1.left;
						int cxSize2New = rcSize2.right - rcSize2.left;
						int cxExtentNew = rcExtent.right - rcExtent.left;
						rcExtent.left = rcBounds.left;
						rcExtent.right = cxExtentNew + rcExtent.left;
						rcSize2.left = cxExtentNew + rcExtent.left;
						rcSize2.right = cxSize2New + rcSize2.left;
						rcSize1.left = cxSize2New + rcSize2.left;
						rcSize1.right = cxSize1New + rcSize1.left;
					}
				}
				if (hasText)
					_DrawDescriptionLineComp(iPartId, pszPrefix, szInsert, pszSuffix, &rcSize1, &rcSize2, &rcExtent);
				else
					_DrawDescriptionLine(iPartId, 1, szMessage, &rcBounds);
			}
			if (SUCCEEDED(hr))
				DrawThemeBackground(_hTheme, _hdcWindow, iPartId, 0, &rc, nullptr);
		}
	}

	int CMinimalDragImage::_SizeDescriptionLine(int iPartId, LPCWSTR pszPrefix, LPCWSTR pszInsert, LPCWSTR pszSuffix, RECT* prcBounds, RECT* rc1, RECT* rc2, RECT* rc3)
	{
		RECT rcExtent = { 0 };
		LONG leftBounds = prcBounds->left;
		GetThemeTextExtent(_hTheme, _hdcWindow, iPartId, 1, pszPrefix, -1, 0, prcBounds, &rcExtent);
		int acc1 = rcExtent.right - rcExtent.left;
		*rc1 = *prcBounds;
		rc1->right = prcBounds->left + rcExtent.right - rcExtent.left;
		prcBounds->left = rc1->right;
		GetThemeTextExtent(_hTheme, _hdcWindow, iPartId, 1, pszInsert, -1, 0, prcBounds, &rcExtent);
		int acc2 = rcExtent.right - rcExtent.left + acc1;
		*rc2 = *prcBounds;
		rc2->right = prcBounds->left + rcExtent.right - rcExtent.left;
		prcBounds->left += rcExtent.right - rcExtent.left;
		if (rc2->right > prcBounds->right)
		{
			rc2->right = prcBounds->right;
			prcBounds->left = prcBounds->right;
		}
		GetThemeTextExtent(_hTheme, _hdcWindow, iPartId, 1, pszSuffix, -1, 0, prcBounds, &rcExtent);
		int acc3 = rcExtent.right - rcExtent.left + acc2;
		*rc3 = *prcBounds;
		rc3->right = prcBounds->left + rcExtent.right - rcExtent.left;
		if (rc3->right > prcBounds->right)
			rc3->right = prcBounds->right;
		prcBounds->left = leftBounds;
		return acc3;
	}

	void CMinimalDragImage::_DrawDescriptionLine(int iPartId, int iStateId, LPCWSTR pszText, RECT* prcBounds)
	{
		RECT rc;
		POINT pt = {};
		GetThemePosition(_hTheme, iPartId, iStateId, 3401, &pt);
		rc = *prcBounds;
		rc.top += pt.y;
		DrawThemeTextEx(_hTheme, _hdcWindow, iPartId, iStateId, pszText, -1, 0, &rc, 0);
		_PreProcessGDIBitmap(&rc);
	}

	void CMinimalDragImage::_DrawDescriptionLineComp(int iPartId, LPCWSTR pszPrefix, LPCWSTR pszInsert, LPCWSTR pszSuffix,
		RECT* prcBounds1, RECT* prcBounds2, RECT* prcBounds3)
	{
		_DrawDescriptionLine(iPartId, 1, pszPrefix, prcBounds1);
		_DrawDescriptionLine(iPartId, 2, pszInsert, prcBounds2);
		_DrawDescriptionLine(iPartId, 1, pszSuffix, prcBounds3);
	}

	void CMinimalDragImage::_DrawTooltipBackground(RECT* prcSrc, RECT* prcBounds, int width)
	{
		RECT rc;
		if (width <= prcBounds->right - prcBounds->left)
			prcBounds->right = prcBounds->left + width;
		_GetTooltipRect(prcSrc, prcBounds, &rc);
		HTHEME hThemeTT = OpenThemeData(_hwnd, L"Tooltip");
		if (hThemeTT)
		{
			DrawThemeBackground(hThemeTT, _hdcWindow, 1, 0, &rc, nullptr);
			CloseThemeData(hThemeTT);
		}
	}

	LRESULT CALLBACK CMinimalDragImage::_DragWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg != WM_NCDESTROY)
		{
				switch (uMsg)
				{
				case WM_USER + 1:
					DestroyWindow(_hwnd);
					return 0;
				case WM_USER + 2:
					if (wParam != _imgType)
					{
						_imgType = wParam;
						_flags = _flags | DIF_CANADDINFO;
					}
					break;
				case WM_USER + 3:
					break;
				default:
					return DefWindowProc(_hwnd, uMsg, wParam, lParam);
				}
				if (_flags & DIF_CANADDINFO)
					_AddInfoToWindow();
		}
		return 0;
	}


	LRESULT CALLBACK CMinimalDragImage::s_DragWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_NCCREATE)
		{
			if (lParam)
			{
				SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW*)lParam)->lpCreateParams);
			}
		}
		else if (GetWindowLongPtrW(hwnd, GWLP_USERDATA))
		{
			return ((CMinimalDragImage*)GetWindowLongPtrW(hwnd, GWLP_USERDATA))->_DragWndProc(uMsg, wParam, lParam);
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
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

	void PerformShellFileOp(HWND hWnd, LPCWSTR destDir, IShellItemArray* pItemArray, DWORD effect, POINTL pt)
	{
		RECT dimensions;
		GetClientRect(wnd->GetHWND(), &dimensions);
		UINT page = g_currentPageID;
		g_overridefilelistener = true;
		HRESULT hr = S_OK;
		ComPtr<CFileOperationProgressSink> pfops = new CFileOperationProgressSink();
		pfops->InitDimensions(&dimensions, &pt, &page);
		if (effect == DROPEFFECT_LINK)
		{
			DWORD dwItemCount = 0;
			pItemArray->GetCount(&dwItemCount);
			{
				for (UINT i = 0; i < dwItemCount; i++)
				{
					ComPtr<IShellItem> pItem;
					hr = pItemArray->GetItemAt(i, &pItem);
					if (SUCCEEDED(hr))
					{
						ComPtr<IShellLinkW> psl;
						hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
						if (SUCCEEDED(hr))
						{
							ComPtr<IPersistFile> ppf;
							LPWSTR pszFilePath = nullptr;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
							if (SUCCEEDED(hr))
							{
								psl->SetPath(pszFilePath);
								hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
								if (SUCCEEDED(hr))
								{
									LPCWSTR fileName = PathFindFileNameW(pszFilePath);
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
									if (SUCCEEDED(hr))
									{
										std::wstring destDir2(destDir, wcslen(destDir) - 1);
										std::wstring linkPath2(linkPath, wcslen(destDir), std::wstring::npos);
										InitNewLVItem(destDir2, linkPath2, &pt, page);
										pfops->PrepDimensions();
									}
								}
							}
						}
					}
				}
			}
			g_overridefilelistener = false;
			return;
		}

		DWORD dwItemCount = 0;
		pItemArray->GetCount(&dwItemCount);

		ComPtr<IFileOperation> pfo;
		hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, IID_IFileOperation, (LPVOID*)&pfo);
		if (SUCCEEDED(hr))
		{
			std::wstring destDir2(destDir, wcslen(destDir) - 1);
			pfops->SetDestinationDirectory(destDir2.c_str());
			ComPtr<IShellItem> pItem;
			hr = SHCreateItemFromParsingName(destDir, nullptr, IID_PPV_ARGS(&pItem));
			if (SUCCEEDED(hr))
			{
				pfo->SetOperationFlags(FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR);
				switch (effect)
				{
				case DROPEFFECT_COPY:
					for (UINT i = 0; i < dwItemCount; i++)
					{
						ComPtr<IShellItem> pCopied;
						pItemArray->GetItemAt(i, &pCopied);
						pfo->CopyItem(pCopied.Get(), pItem.Get(), nullptr, pfops.Get());
					}
					break;
				case DROPEFFECT_MOVE:
					for (UINT i = 0; i < dwItemCount; i++)
					{
						ComPtr<IShellItem> pMoved;
						pItemArray->GetItemAt(i, &pMoved);
						pfo->MoveItem(pMoved.Get(), pItem.Get(), nullptr, pfops.Get());
					}
					break;
				}
				pfo->PerformOperations();
			}
		}

		g_overridefilelistener = false;
	}

	DWORD TheDropProc(IDataObject* pDataObject, CLIPFORMAT cf, HGLOBAL hdata, HWND hwnd, DWORD key_state, POINTL pt, void* param)
	{
		if (cf == CF_HDROP)
		{
			DWORD effect = DROPEFFECT_MOVE;
			ComPtr<IShellItemArray> pItemArray = nullptr;
			HRESULT hr = SHCreateShellItemArrayFromDataObject(pDataObject, IID_PPV_ARGS(&pItemArray));
			if (SUCCEEDED(hr))
			{
				DWORD dwItemCount = 0;
				pItemArray->GetCount(&dwItemCount);
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
				for (UINT i = 0; i < dwItemCount; i++)
				{
					ComPtr<IShellItem> pItem;
					if (SUCCEEDED(pItemArray->GetItemAt(i, &pItem)))
					{
						SFGAOF attributes;
						pItem->GetAttributes(SHCIDS_COLUMNMASK, &attributes);
						LPWSTR pszFilePath = nullptr;
						if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
						{
							if (key_state & MK_SHIFT)
							{
								if (key_state & MK_CONTROL)
									goto LINKEFFECT;
								effect = DROPEFFECT_MOVE;
								CoTaskMemFree(pszFilePath);
								break;
							}
							if (key_state & MK_CONTROL || pszFilePath[0] != desktopPath[0])
							{
								effect = DROPEFFECT_COPY;
								CoTaskMemFree(pszFilePath);
								break;
							}
							if (key_state & MK_ALT || !(attributes & SFGAO_CANMOVE))
							{
							LINKEFFECT:
								effect = DROPEFFECT_LINK;
								CoTaskMemFree(pszFilePath);
								break;
							}
							CoTaskMemFree(pszFilePath);
						}
					}
				}
				PerformShellFileOp(hwnd, desktopPath.c_str(), pItemArray.Get(), effect, pt);
			}
			return effect;
		}
		/* Default reaction is to do nothing: */
		return DROPEFFECT_NONE;
	}
}