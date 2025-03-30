#pragma once

namespace DirectUI
{
	class DECLSPEC_UUID("7627b482-f285-41e7-996d-368ed0052602")
	PVLAnimationTriggers;

	class DuiPVLTrigger
	{
	public:
		DuiPVLTrigger() : m_pde(nullptr)
		{
		}

		~DuiPVLTrigger()
		{
			if (m_pde)
			{
				m_pde->Release();
				m_pde = nullptr;
			}
		}

		HRESULT TriggerDelete(Element* peToDelete, bool fDestroy, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerDelete(peToDelete, fDestroy, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerReposition(Element* peToMove, POINT ptDest, bool fReverse, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerReposition(peToMove, ptDest, fReverse, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerAdd(Element* peToAdd, Element* peParent, UINT uIndex, int nLayoutPos, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerAdd(peToAdd, peParent, uIndex, nLayoutPos, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerExpand(Element* peToExpand, Element* peAdditional, int nLayoutPos, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerExpand(peToExpand, peAdditional, nLayoutPos, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerCollapse(Element* peToCollapse, Element* peAdditional, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerCollapse(peToCollapse, peAdditional, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerSlide(Element*, Element*, UINT, UINT, POINT, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerSlide();
			}
			return hr;
		}

		HRESULT TriggerSlideAffected(Element*, UINT, UINT, POINT, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerSlide();
			}
			return hr;
		}

		HRESULT TriggerShowEdgeUI(Element*, Element*, UINT, POINT, bool, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerShowEdgeUI();
			}
			return hr;
		}

		HRESULT TriggerHideEdgeUI(Element*, Element*, UINT, bool, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerHideEdgeUI();
			}
			return hr;
		}

		HRESULT TriggerShowPanel(Element*, Element*, UINT, POINT, bool, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerShowPanel();
			}
			return hr;
		}

		HRESULT TriggerHidePanel(Element*, Element*, UINT, bool, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerHidePanel();
			}
			return hr;
		}

		HRESULT TriggerFade(Element* peToFade, UINT uiState, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerFade(peToFade, uiState, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerTap(Element* peToTap, UINT uiState, POINT ptContact, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerTap(peToTap, uiState, ptContact, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerDrag(DynamicArray<Element*>* pelRemaining, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerDrag();
			}
			return hr;
		}

		HRESULT TriggerDrag(
			Element* peToDrag, DynamicArray<Element*>* pelRemaining, UINT uiState, POINT ptContact, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerDrag(peToDrag, pelRemaining, uiState, ptContact, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerDrop(
			Element*, DynamicArray<Element*>*, Element*, int, int, POINT, POINTFLOAT, POINTFLOAT, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerDrop();
			}
			return hr;
		}

		HRESULT TriggerDrop(Element*, DynamicArray<Element*>*, Element*, int, int, POINT, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerDrop();
			}
			return hr;
		}

		HRESULT TriggerReveal(
			Element* peTapped, Element* peOutline, Element* peBackground, Element* peContent,
			const RECT* prcContentClipFrom, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerReveal(peTapped, peOutline, peBackground, peContent, prcContentClipFrom, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerHide(
			Element* peTapped, Element* peOutline, Element* peBackground, Element* peContent,
			const RECT* prcContentClipTo, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerHide(peTapped, peOutline, peBackground, peContent, prcContentClipTo, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerReflow(Element* peContainer, bool fUseNewSize, SIZE* pszDest, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerReflow(peContainer, fUseNewSize, pszDest, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerResize(Element* peToResize, SIZE szDest, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerResize(peToResize, szDest, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerMove(Element* peToMove, Element* peParent, int nIndex, bool fTopMost, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerMove(peToMove, peParent, nIndex, fTopMost, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerMove(Element* peToMove, Element* peParent, int nIndex, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerMove(peToMove, peParent, nIndex, true, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerContentTransition(
			DynamicArray<Element*>* pelSources, DynamicArray<Element*>* pelDests, Element* peParent, POINT ptOffset,
			int nLayoutPos, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerContentTransition(pelSources, pelDests, peParent, ptOffset, nLayoutPos, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerPageTransition(
			DynamicArray<Element*>* pelSources, DynamicArray<Element*>* pelDests, Element* peParent, POINT ptOffset,
			int nLayoutPos, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerPageTransition(pelSources, pelDests, peParent, ptOffset, nLayoutPos, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerBadgeUpdate(Element* peOldBadge, Element* peNewBadge, POINT ptOffset, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerBadgeUpdate(peOldBadge, peNewBadge, ptOffset, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerPop(
			Element* peToPop, Element* peParent, UINT uiState, POINT ptDest, POINT ptOffset, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // TODO m_pde->TriggerPop();
			}
			return hr;
		}

		HRESULT TriggerPop(
			Element* peToPop, Element* peParent, UINT uiState, POINT ptDest, POINT ptOffset, bool fLayoutAlreadySet,
			bool fUseCurrentOpacity, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerPop(
					peToPop, peParent, uiState, ptDest, ptOffset, fLayoutAlreadySet, fUseCurrentOpacity, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerCrossfade(
			Element* peToFadeOut, Element* peToFadeIn, bool fCloneTransforms, bool fLayoutAlreadySet, bool fDestroy,
			bool fInplace, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerCrossfade(
					peToFadeOut, peToFadeIn, fCloneTransforms, fLayoutAlreadySet, fDestroy, fInplace, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerSwipe(Element* peToSwipe, UINT uiState, POINT ptOffset, Element* peCheckmark, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerSwipe(peToSwipe, uiState, ptOffset, peCheckmark, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerEntrance(DynamicArray<Element*>* pelToEnter, DynamicArray<POINT>* pdaOffsets, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerEntrance(pelToEnter, pdaOffsets, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerNudge(
			DynamicArray<Element*>* pelToNudge, DynamicArray<POINT>* pdaOffsets, UINT uiState, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerNudge(pelToNudge, pdaOffsets, uiState, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerShowTileNotification(
			Element* peOutgoing, POINT ptOutgoingDest, Element* peIncoming, POINT ptIncomingDest,
			UINT uTileNotificationOption, Element* peLogo, Element* peDisplayName, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerShowTileNotification(
					peOutgoing, ptOutgoingDest, peIncoming, ptIncomingDest, uTileNotificationOption, peLogo,
					peDisplayName, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerPeekTileNotification(Element* peTargetMessage, POINT ptDest, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerPeekTileNotification(peTargetMessage, ptDest, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerDeleteGroup(DynamicArray<Element*>* pelToDelete, bool fDestroy, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerDeleteGroup(pelToDelete, fDestroy, false, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerAddGroup(
			DynamicArray<Element*>* pelParents, DynamicArray<DynamicArray<ElementIndexPair*>*>* pdaToAdd,
			int nLayoutPos, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerAddGroupMultiParent(pelParents, pdaToAdd, nLayoutPos, false, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerAddGroup(
			DynamicArray<Element*>* pelToAdd, Element* peParent, DynamicArray<UINT>* pdaIndices, int nLayoutPos,
			DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerAddGroup(pelToAdd, peParent, pdaIndices, nLayoutPos, false, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerSearchFilterDelete(DynamicArray<Element*>* pelToDelete, bool fDestroy, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerDeleteGroup(pelToDelete, fDestroy, true, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerSearchFilterAdd(
			DynamicArray<Element*>* pelParents, DynamicArray<DynamicArray<ElementIndexPair*>*>* pdaToAdd,
			int nLayoutPos, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerAddGroupMultiParent(pelParents, pdaToAdd, nLayoutPos, true, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerSearchFilterAdd(
			DynamicArray<Element*>* pelToAdd, Element* peParent, DynamicArray<UINT>* pdaIndices, int nLayoutPos,
			DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerAddGroup(pelToAdd, peParent, pdaIndices, nLayoutPos, true, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerFadeGroup(DynamicArray<Element*>* pelToFade, UINT uiState, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerFadeGroup(pelToFade, uiState, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerBeginStoryboard(Element* peTrapElement, int* pnStoryboardId, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerBeginStoryboard(peTrapElement, pnStoryboardId, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerEndStoryboard(Element* peTrapElement, int nStoryboardId)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerEndStoryboard(peTrapElement, nStoryboardId);
			}
			return hr;
		}

		HRESULT TriggerCancelStoryboard(Element* peTrapElement, int nStoryboardId)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerCancelStoryboard(peTrapElement, nStoryboardId);
			}
			return hr;
		}

		HRESULT AddReposition(
			int nStoryboardId, Element* peToMove, POINT ptDest, int nOrderNumber, bool fReverse, UINT uiVersion,
			UINT uiState)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->AddReposition(nStoryboardId, peToMove, ptDest, nOrderNumber, fReverse, uiVersion, uiState);
			}
			return hr;
		}

		HRESULT AddFade(int nStoryboardId, Element* peToFade, UINT uiState, int nOrderNumber)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->AddFade(nStoryboardId, peToFade, uiState, nOrderNumber);
			}
			return hr;
		}

		HRESULT AddScale(
			int nStoryboardId, Element* peToScale, POINTFLOAT ptflOrigin, POINTFLOAT ptflScale, int nOrderNumber)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->AddScale(nStoryboardId, peToScale, ptflOrigin, ptflScale, nOrderNumber);
			}
			return hr;
		}

		HRESULT AddInputPaneSlide(
			int nStoryboardId, Element* peToSlide, const POINT* pptInit, const POINT* pptDest, const RECT* prcInputPane,
			int iInputPaneDeltaX, int iInputPaneDeltaY)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->AddInputPaneSlide(
					nStoryboardId, peToSlide, pptInit, pptDest, prcInputPane, iInputPaneDeltaX, iInputPaneDeltaY);
			}
			return hr;
		}

	protected:
		IDuiAnimationTriggers* m_pde;

		HRESULT EnsureObject()
		{
			return !m_pde ? DuiCreateObject(__uuidof(PVLAnimationTriggers), IID_PPV_ARGS(&m_pde)) : S_OK;
		}
	};
}
