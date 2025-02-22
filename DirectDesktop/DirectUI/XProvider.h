#pragma once

namespace DirectUI
{
	MIDL_INTERFACE("1E3D87CB-F434-4838-A004-F14E385E736F")
	IXProvider : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE CreateDUI(IXElementCP* pcp, HWND* phwnd) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetParameter(REFGUID, void*) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetDesiredSize(int cxConstraint, int cyConstraint, SIZE* ps) = 0;
		virtual HRESULT STDMETHODCALLTYPE IsDescendent(Element* pe, bool* pbIsDescendent) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetFocus(Element* pe) = 0;
		virtual HRESULT STDMETHODCALLTYPE Navigate(int iNavDir, bool* pbHandled) = 0;
		virtual HRESULT STDMETHODCALLTYPE CanSetFocus(bool* pfCanSetFocus) = 0;
		virtual BOOL STDMETHODCALLTYPE FindElementWithShortcutAndDoDefaultAction(WCHAR ch, BOOL fSysKey) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetHostedElementID(ATOM* patomID) = 0;
		virtual HRESULT STDMETHODCALLTYPE ForceThemeChange(WPARAM wParam, LPARAM lParam) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetDefaultButtonTracking(bool fEnabled) = 0;
		virtual BOOL STDMETHODCALLTYPE ClickDefaultButton() = 0;
		virtual HRESULT STDMETHODCALLTYPE SetRegisteredDefaultButton(Element* peDefaultButton) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetButtonClassAcceptsEnterKey(bool fEnabled) = 0;
	};

	interface IXBaby;

	class UILIB_API XProvider : public IXProvider
	{
	public:
		//~ Begin IUnknown Interface
		STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override;
		STDMETHODIMP_(ULONG) AddRef() override;
		STDMETHODIMP_(ULONG) Release() override;
		//~ End IUnknown Interface

		//~ Begin IXProvider Interface
		STDMETHODIMP CreateDUI(IXElementCP* pcp, HWND* phwnd) override;
		STDMETHODIMP SetParameter(REFGUID, void*) override;
		STDMETHODIMP GetDesiredSize(int cxConstraint, int cyConstraint, SIZE* ps) override;
		STDMETHODIMP IsDescendent(Element* pe, bool* pbHandled) override;
		STDMETHODIMP SetFocus(Element* pe) override;
		STDMETHODIMP Navigate(int iNavDir, bool* pbHandled) override;
		STDMETHODIMP CanSetFocus(bool* pfCanSetFocus) override;
		STDMETHODIMP_(BOOL) FindElementWithShortcutAndDoDefaultAction(WCHAR ch, BOOL fSysKey) override;
		STDMETHODIMP GetHostedElementID(ATOM* patomID) override;
		STDMETHODIMP ForceThemeChange(WPARAM wParam, LPARAM lParam) override;
		STDMETHODIMP SetDefaultButtonTracking(bool fEnabled) override;
		STDMETHODIMP_(BOOL) ClickDefaultButton() override;
		STDMETHODIMP SetRegisteredDefaultButton(Element* peDefaultButton) override;
		STDMETHODIMP SetButtonClassAcceptsEnterKey(bool fEnabled) override;
		//~ End IXProvider Interface

		static HRESULT WINAPI Create(Element* pe, IXProviderCP* pprovCP, XProvider** ppxp);
		HRESULT Initialize(Element* pe, IXProviderCP* pprovCP);

		virtual HRESULT CreateXBaby(IXElementCP* pcp, HWND hwndParent, Element* pParent, DWORD* pdwDeferCookie, IXBaby** ppIXBaby);
		HRESULT CreateParser(DUIXmlParser** ppParser);

		XProvider();
		XProvider(const XProvider&) = default;

		virtual ~XProvider();

	protected:
		IXBaby* _peRoot;

		Element* GetRoot();
		void SetHandleEnterKey(bool fHandle);

		int _cRef;
		Element* _pe;
		IXProviderCP* _pprovCP;
	};
}
