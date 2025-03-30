#pragma once

namespace DirectUI
{
	template <typename ProviderT, typename InterfaceT, Schema::Pattern PatternT>
	class PatternProvider
		: protected RefcountBase
		, public IProvider
	{
	public:
		PatternProvider()
		{
		}

		PatternProvider(const PatternProvider&) = delete;

		virtual void Init(ElementProvider* pprv)
		{
			_pprv = pprv;
		}

		~PatternProvider() override
		{
			_pprv->TossPatternProvider(PatternT);
			if (_pprv) // TODO We don't have AutoReleasePointer yet
			{
				_pprv->Release();
			}
		}

		static long WINAPI Create(ElementProvider* pprv, IUnknown** ppunk)
		{
			*ppunk = nullptr;

			ProviderT* p = HNew<ProviderT>();
			HRESULT hr = p ? S_OK : E_OUTOFMEMORY;
			if (SUCCEEDED(hr))
			{
				p->Init(pprv);
				*ppunk = (InterfaceT*)p;
			}

			return hr;
		}

		PfnCreateProxy GetProxyCreator() override
		{
			return nullptr;
		}

	protected:
		HRESULT DoInvoke(MethodId methodId, ...)
		{
			va_list args;
			va_start(args, methodId);

			HRESULT hr;

			if (_pprv->GetElement())
			{
				hr = _pprv->DoInvokeArgs(methodId, GetProxyCreator(), args);
			}
			else
			{
				hr = UIA_E_ELEMENTNOTAVAILABLE;
			}

			return hr;
		}

		// AutoReleasePointer<ElementProvider> _pprv;
		ElementProvider* _pprv = nullptr;
	};
}
