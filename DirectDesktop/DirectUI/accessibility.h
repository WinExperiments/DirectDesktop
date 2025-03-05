#pragma once

namespace DirectUI
{
	class UILIB_API DuiAccessible :
		public IAccessible
		, public IEnumVARIANT
		, public IOleWindow
		, public IAccIdentity
		, public IServiceProvider
	{
	public:
		HRESULT Create(Element*, DuiAccessible**);
		DuiAccessible();

	private:
		DuiAccessible(DuiAccessible&);
		DuiAccessible& operator=(DuiAccessible&);

	public:
		virtual HRESULT Disconnect();
		void Initialize(Element* pe);

		virtual ~DuiAccessible();

		//~ Begin IUnknown Interface
		STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
		STDMETHODIMP_(ULONG) AddRef() override;
		STDMETHODIMP_(ULONG) Release() override;
		//~ End IUnknown Interface

		//~ Begin IDispatch Interface
		STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;
		STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) override;
		STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) override;
		STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) override;
		//~ End IDispatch Interface

		//~ Begin IAccessible Interface
		STDMETHODIMP get_accParent(IDispatch** ppdispParent) override;
		STDMETHODIMP get_accChildCount(long* pcountChildren) override;
		STDMETHODIMP get_accChild(VARIANT varChild, IDispatch** ppdispChild) override;
		STDMETHODIMP get_accName(VARIANT varChild, BSTR* pszName) override;
		STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pszValue) override;
		STDMETHODIMP get_accDescription(VARIANT varChild, BSTR* pszDescription) override;
		STDMETHODIMP get_accRole(VARIANT varChild, VARIANT* pvarRole) override;
		STDMETHODIMP get_accState(VARIANT varChild, VARIANT* pvarState) override;
		STDMETHODIMP get_accHelp(VARIANT varChild, BSTR* pszHelp) override;
		STDMETHODIMP get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic) override;
		STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut) override;
		STDMETHODIMP get_accFocus(VARIANT* pvarChild) override;
		STDMETHODIMP get_accSelection(VARIANT* pvarChildren) override;
		STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction) override;
		STDMETHODIMP accSelect(long flagsSelect, VARIANT varChild) override;
		STDMETHODIMP accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) override;
		STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt) override;
		STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT* pvarChild) override;
		STDMETHODIMP accDoDefaultAction(VARIANT varChild) override;
		STDMETHODIMP put_accName(VARIANT varChild, BSTR szName) override;
		STDMETHODIMP put_accValue(VARIANT varChild, BSTR szValue) override;
		//~ End IAccessible Interface

		//~ Begin IEnumVARIANT Interface
		STDMETHODIMP Next(ULONG celt, VARIANT* rgVar, ULONG* pCeltFetched) override;
		STDMETHODIMP Skip(ULONG celt) override;
		STDMETHODIMP Reset() override;
		STDMETHODIMP Clone(IEnumVARIANT** ppenum) override;
		//~ End IEnumVARIANT Interface

		//~ Begin IOleWindow Interface
		STDMETHODIMP GetWindow(HWND* phwnd) override;
		STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override;
		//~ End IOleWindow Interface

		//~ Begin IAccIdentity Interface
		STDMETHODIMP GetIdentityString(DWORD dwIDChild, BYTE** ppIDString, DWORD* pdwIDStringLen) override;
		//~ End IAccIdentity Interface

		//~ Begin IServiceProvider Interface
		STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void** ppvObject) override;
		//~ End IServiceProvider Interface

		static HRESULT WINAPI AccNavigate(Element* pe, LONG navDir, Element** ppe);
		static Element* WINAPI GetAccessibleParent(Element* pe);

	protected:
		HRESULT GetDispatchFromElement(Element* pe, IDispatch** ppDispatch);
		HRESULT GetAccName(VARIANT varChild, BOOL fAllowSubstitution, BSTR* pbstrName);
		HRESULT GetAccNameFromContent(BSTR* pbstrName);

		Element* _pe;

	private:
		LONG _cRefs;
	};

	class UILIB_API HWNDHostAccessible : public DuiAccessible
	{
	public:
		HWNDHostAccessible();
		virtual ~HWNDHostAccessible();

		long Initialize(Element*, IAccessible*);
		static HRESULT WINAPI Create(Element*, IAccessible*, DuiAccessible**);

		//IAccIdentity
		virtual HRESULT WINAPI GetIdentityString(unsigned long, unsigned char**, unsigned long*);

		virtual HRESULT WINAPI Clone(IEnumVARIANT**);
		virtual HRESULT WINAPI ContextSensitiveHelp(int);
		virtual HRESULT Disconnect();
		virtual HRESULT WINAPI GetWindow(HWND*);
		virtual HRESULT WINAPI Next(unsigned long, VARIANT*, unsigned long*);
		virtual HRESULT WINAPI QueryInterface(const IID&, void**);
		virtual HRESULT WINAPI QueryService(const IID&, const IID&, void**);
		virtual HRESULT WINAPI Reset();
		virtual HRESULT WINAPI Skip(unsigned long);
		virtual HRESULT WINAPI accDoDefaultAction(VARIANT);
		virtual HRESULT WINAPI accHitTest(long, long, VARIANT*);
		virtual HRESULT WINAPI accLocation(long*, long*, long*, long*, VARIANT);
		virtual HRESULT WINAPI accNavigate(long, VARIANT, VARIANT*);
		virtual HRESULT WINAPI accSelect(long, VARIANT);

		// IAccessible
		virtual HRESULT WINAPI get_accChild(VARIANT, IDispatch**);
		virtual HRESULT WINAPI get_accChildCount(long*);
		virtual HRESULT WINAPI get_accDefaultAction(VARIANT, BSTR*);
		virtual HRESULT WINAPI get_accDescription(VARIANT, BSTR*);
		virtual HRESULT WINAPI get_accFocus(VARIANT*);
		virtual HRESULT WINAPI get_accHelp(VARIANT, BSTR*);
		virtual HRESULT WINAPI get_accHelpTopic(BSTR*, VARIANT, long*);
		virtual HRESULT WINAPI get_accKeyboardShortcut(VARIANT, BSTR*);
		virtual HRESULT WINAPI get_accName(VARIANT, BSTR*);
		virtual HRESULT WINAPI get_accParent(IDispatch**);
		virtual HRESULT WINAPI get_accRole(VARIANT, VARIANT*);
		virtual HRESULT WINAPI get_accSelection(VARIANT*);
		virtual HRESULT WINAPI get_accState(VARIANT, VARIANT*);
		virtual HRESULT WINAPI get_accValue(VARIANT, BSTR*);
		virtual HRESULT WINAPI put_accName(VARIANT, BSTR);
		virtual HRESULT WINAPI put_accValue(VARIANT, BSTR);
	};

	class UILIB_API HWNDHostClientAccessible : public HWNDHostAccessible
	{
	public:
		HWNDHostClientAccessible();
		HWNDHostClientAccessible(const HWNDHostClientAccessible&) = delete;
		HWNDHostClientAccessible& operator=(const HWNDHostClientAccessible&) = delete;

		virtual ~HWNDHostClientAccessible();

		static HRESULT WINAPI Create(Element*, IAccessible*, DuiAccessible**);
		virtual HRESULT WINAPI accNavigate(long, VARIANT, LPVARIANT);
		virtual HRESULT WINAPI get_accParent(IDispatch**);
		virtual HRESULT WINAPI get_accRole(VARIANT, VARIANT*);
	};

	class UILIB_API HWNDElementAccessible : DuiAccessible
	{
	public:
		HWNDElementAccessible();
		virtual ~HWNDElementAccessible();

		long Initialize(HWNDElement*);
		static HRESULT WINAPI Create(HWNDElement*, DuiAccessible**);

		virtual HRESULT Disconnect();
		virtual HRESULT WINAPI get_accParent(IDispatch**);
	};
}
