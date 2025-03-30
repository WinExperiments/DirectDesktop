#pragma once

namespace DirectUI
{
	class DECLSPEC_UUID("b1c98acc-93dc-4494-91cd-ec9d668da5d2")
	PVLLauncherAnimationTriggers;

	class DuiPVLLauncherTrigger
	{
	public:
		DuiPVLLauncherTrigger() : m_pde(nullptr)
		{
		}

		~DuiPVLLauncherTrigger()
		{
			if (m_pde)
			{
				m_pde->Release();
				m_pde = nullptr;
			}
		}

		HRESULT TriggerTap(Element* peToTap, UINT uiState, POINT ptContact, UINT uiVersion, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerTap(peToTap, uiState, ptContact, uiVersion, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerLaunch(Element* peToLaunch, DynamicArray<Element*>* pelUnk, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerLaunch(peToLaunch, pelUnk, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerDrag(
			Element* peToDrag, DynamicArray<Element*>* pelRemaining, Element* peContent, Element* peAdornment,
			DynamicArray<ClipRectWithElement*>* pTowerClippingRects, UINT uiState, POINT ptContact, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerDrag(
					peToDrag, pelRemaining, peContent, peAdornment, pTowerClippingRects, uiState, ptContact, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerEnterEditModeScaling(
			DynamicArray<Element*>* pelAffected, DynamicArray<ClipRectWithElement*>* pTowerClippingRects,
			DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				POINT ptTemp = {};
				hr = m_pde->TriggerDrag(
					nullptr, pelAffected, nullptr, nullptr, pTowerClippingRects, 4, ptTemp, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerEnterEditModeScaling(DynamicArray<Element*>* pelAffected, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				POINT ptTemp = {};
				hr = m_pde->TriggerDrag(nullptr, pelAffected, nullptr, nullptr, nullptr, 4, ptTemp, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerDrop(
			Element* peToDrop, DynamicArray<Element*>* pelRemaining, Element* peContent, Element* peAdornment,
			DynamicArray<ClipRectWithElement*>* pTowerClippingRects, Element* peParent, int nIndex, int nLayoutPos,
			POINT ptDest, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerDrop(
					peToDrop, pelRemaining, peContent, peAdornment, pTowerClippingRects, peParent, nIndex, nLayoutPos,
					ptDest, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerExitEditModeScaling(
			DynamicArray<Element*>* pelAffected, DynamicArray<ClipRectWithElement*>* pTowerClippingRects,
			DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				POINT ptTemp = {};
				hr = m_pde->TriggerDrop(
					nullptr, pelAffected, nullptr, nullptr, pTowerClippingRects, nullptr, 0, -3, ptTemp, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerLauncher(
			UINT, UINT, ElementOffsetPair*, DynamicArray<Element*>*, DynamicArray<Element*>*, Element*, Element*,
			Element*, Element*, Element*, POINT, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // Nuked in 1607
			}
			return hr;
		}

		HRESULT TriggerSession(
			UINT uiVersion, UINT uiState, ElementOffsetPair* peopLauncher, DynamicArray<Element*>* pelTowers,
			DynamicArray<Element*>* pelGroupHeaders, ElementOffsetPair* peopStartText,
			ElementOffsetPair* peopUserTileIcon, ElementOffsetPair* peopUserTileText, Element* peLauncherContent,
			POINT ptTowerOffset, POINT ptGroupHeaderOffset, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerSession(
					uiVersion, uiState, peopLauncher, pelTowers, pelGroupHeaders, peopStartText, peopUserTileIcon,
					peopUserTileText, peLauncherContent, ptTowerOffset, ptGroupHeaderOffset, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerLogin(
			UINT uiVersion, Element* peLoginUserTile, Element* peLoginUserText, POINT ptOffset, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerLogin(uiVersion, peLoginUserTile, peLoginUserText, ptOffset, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerLogonEntrance(
			DynamicArray<Element*>* pelToEnter, DynamicArray<POINT>* pdaOffsets, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerLogonEntrance(pelToEnter, pdaOffsets, pdwCookie);
			}
			return hr;
		}

		HRESULT TriggerCharmLogoAnimation(
			Element*, Element*, Element*, Element*, Element*, Element*, int, DEVICE_SCALE_FACTOR, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = E_NOTIMPL; // Nuked
			}
			return hr;
		}

		HRESULT TriggerCrossfade(
			DynamicArray<Element*>* pelToFadeOut, DynamicArray<Element*>* pelToFadeIn, bool fCloneTransforms,
			bool fLayoutAlreadySet, bool fDestroy, bool fInplace, DWORD* pdwCookie)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->TriggerCrossfade(
					pelToFadeOut, pelToFadeIn, fCloneTransforms, fLayoutAlreadySet, fDestroy, fInplace, pdwCookie);
			}
			return hr;
		}

		HRESULT AddScale(
			int nStoryboardId, Element* peToScale, POINTFLOAT ptflOrigin, POINTFLOAT ptflScale, int nOrderNumber,
			int nCustomDuration)
		{
			HRESULT hr = EnsureObject();
			if (SUCCEEDED(hr))
			{
				hr = m_pde->AddScale(nStoryboardId, peToScale, ptflOrigin, ptflScale, nOrderNumber, nCustomDuration);
			}
			return hr;
		}

	protected:
		IDuiLauncherAnimationTriggers* m_pde;

		HRESULT EnsureObject()
		{
			return !m_pde ? DuiCreateObject(__uuidof(PVLLauncherAnimationTriggers), IID_PPV_ARGS(&m_pde)) : S_OK;
		}
	};
}
