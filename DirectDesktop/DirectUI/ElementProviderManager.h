#pragma once

namespace DirectUI
{
	class UILIB_API ElementProviderManager
	{
	public:
		ElementProviderManager & operator=(ElementProviderManager const &);

		static long WINAPI Add(ElementProvider *);
		static void WINAPI Close();
		static ElementProvider *WINAPI Find(Element *);
		static long WINAPI Init();
		static void WINAPI Remove(ElementProvider *);

		template <typename T>
		static HRESULT GetProvider(Element* pe, T** ppprv)
		{
			InvokeHelper* pih = nullptr;
			HRESULT hr = InvokeManager::GetInvokeHelper(&pih);
			if (SUCCEEDED(hr))
			{
				hr = GetProvider(pe, pih, ppprv);
			}
			return hr;
		}

		template <typename T>
		static HRESULT GetProvider(Element* pe, InvokeHelper* pih, T** ppprv)
		{
			*ppprv = nullptr;
			HRESULT hr = S_OK;

			if (pe)
			{
				hr = pe->GetUIAElementProvider(IID_PPV_ARGS(ppprv));
				if (hr == E_NOTIMPL)
				{
					ElementProvider* pprv = Find(pe);
					if (pprv)
					{
						hr = S_OK;
					}
					else
					{
						hr = pe->GetElementProviderImpl(pih, &pprv);
						if (hr == E_NOTIMPL)
						{
							if (pe->GetClassInfoW()->IsSubclassOf(HWNDElement::GetClassInfoPtr()))
							{
								hr = HWNDElementProvider::Create((HWNDElement*)pe, pih, (HWNDElementProvider**)&pprv);
							}
							else
							{
								hr = ElementProvider::Create(pe, pih, &pprv);
							}
						}
					}

					if (SUCCEEDED(hr))
					{
						hr = pprv->QueryInterface(IID_PPV_ARGS(ppprv));
						pprv->Release();
					}
				}
			}

			return hr;
		}

		static CRITICAL_SECTION g_cs;
	private:
		static bool WINAPI FindProviderCallback(ElementProvider *, void *);

		static UiaArray<ElementProvider *> * g_pArrayPprv;
	};
}
