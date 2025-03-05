#pragma once

namespace DirectUI
{
	class UILIB_API Proxy
	{
	public:
		Proxy();
		virtual ~Proxy();

		static HRESULT CALLBACK SyncCallback(HGADGET hgadCur, void* pvCur, EventMsg* pGMsg);

	protected:
		void Invoke(UINT nType, void* pData);

		virtual void OnInvoke(UINT nType, void* pData);

		HGADGET _hgSync;
	};

	class UILIB_API DECLSPEC_NOVTABLE ProviderProxy
	{
	public:
		virtual HRESULT DoMethod(MethodId methodId, va_list args) = 0;

	protected:
		virtual void Init(Element* pe);

	public:
		ProviderProxy(const ProviderProxy&) = default;

	protected:
		ProviderProxy();

		Element* _pe;
	};
}
