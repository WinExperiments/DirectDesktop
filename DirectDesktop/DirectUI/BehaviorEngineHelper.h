#pragma once

class BehaviorEngineHelper
{
public:
	BehaviorEngineHelper()
		: m_pde(nullptr)
	{
	}

	~BehaviorEngineHelper()
	{
		if (m_pde)
		{
			m_pde->Release();
			m_pde = nullptr;
		}
	}

	HRESULT RegisterBehaviorFactory(const WCHAR* pwzNamespace, IDuiBehaviorFactory* pFactory)
	{
		HRESULT hr = EnsureObject();
		if (SUCCEEDED(hr))
		{
			hr = m_pde->RegisterBehaviorFactory(pwzNamespace, pFactory);
		}
		return hr;
	}

	HRESULT UnregisterBehaviorFactory(const WCHAR* pwzNamespace, IDuiBehaviorFactory* pFactory)
	{
		HRESULT hr = EnsureObject();
		if (SUCCEEDED(hr))
		{
			hr = m_pde->UnregisterBehaviorFactory(pwzNamespace, pFactory);
		}
		return hr;
	}

	HRESULT CreateBehavior(const WCHAR* pwzNamespace, const WCHAR* pwzName, DirectUI::Value* pvArgs, IDuiBehavior** ppBehavior)
	{
		HRESULT hr = EnsureObject();
		if (SUCCEEDED(hr))
		{
			hr = m_pde->CreateBehavior(pwzNamespace, pwzName, pvArgs, ppBehavior);
		}
		return hr;
	}

protected:
	IDuiBehaviorEngine* m_pde;

	HRESULT EnsureObject()
	{
		return !m_pde ? DuiCreateObject(__uuidof(DirectUI::BehaviorEngine), IID_PPV_ARGS(&m_pde)) : S_OK;
	}
};
