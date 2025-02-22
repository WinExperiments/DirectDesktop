#pragma once

namespace DirectUI
{

	class UILIB_API DuiAccessible
		: public IAccessible
		, public IEnumVARIANT
		, public IOleWindow
		, public IAccIdentity
		, public IServiceProvider
	{
	public:
		DuiAccessible();
		DuiAccessible(const DuiAccessible&) = delete;
		DuiAccessible& operator = (const DuiAccessible&) = delete;

		virtual ~DuiAccessible();
		
		void Initialize(Element* pe);
		static HRESULT WINAPI Create(Element*, DuiAccessible**);
		static Element* WINAPI GetAccessibleParent(Element*);
		static HRESULT WINAPI AccNavigate(Element*, long, Element**);
	
		virtual HRESULT Disconnect(void);

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
		STDMETHODIMP_(ULONG) AddRef() override;
		STDMETHODIMP_(ULONG) Release() override;
		
		// IDispatch
		STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;
		STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo ) override;
		STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) override;	
		STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) override;
		
		// IAccIdentity
		STDMETHODIMP GetIdentityString(DWORD dwIDChild, BYTE** ppIDString, DWORD* pdwIDStringLen) override;
		
		// IAccessible
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
		
		virtual HRESULT WINAPI accSelect(long flagsSelect, VARIANT varID);
		virtual HRESULT WINAPI accLocation(long*pxLeft, long*pyTop, long*pcxWidth, long*pcyHeight, VARIANT varID);
		virtual HRESULT WINAPI accNavigate(long navDir, VARIANT varStart, VARIANT*pvarEnd);
		virtual HRESULT WINAPI accHitTest(long xLeft, long yTop, VARIANT*pvarID);
		virtual HRESULT WINAPI accDoDefaultAction(VARIANT varID);
		
		virtual HRESULT WINAPI put_accName(VARIANT varID, BSTR pszValue);
		virtual HRESULT WINAPI put_accValue(VARIANT varID, BSTR pszValue);		
		
		// IEnumVARIANT
		virtual HRESULT WINAPI Clone(IEnumVARIANT**ppEnum);
		virtual HRESULT WINAPI Next(unsigned long celt, VARIANT*rgVar, unsigned long*pCeltFetched);
		virtual HRESULT WINAPI Reset();
		virtual HRESULT WINAPI Skip(unsigned long celt);
		
		// IOleWindow
		virtual HRESULT WINAPI GetWindow(HWND*phwnd);
		virtual HRESULT WINAPI ContextSensitiveHelp(BOOL fEnterMode);

		// IServiceProvider
		virtual HRESULT WINAPI QueryService(REFGUID guidService, REFIID riid, void**ppv);
		
	protected: 
		long GetAccName(VARIANT, int, unsigned short**);
		long GetAccNameFromContent(unsigned short**);
		long GetDispatchFromElement(Element*, IDispatch**);
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
		virtual HRESULT WINAPI QueryInterface(const IID &, void**);
		virtual HRESULT WINAPI QueryService(const IID &, const IID &, void**);
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

	class UILIB_API HWNDHostClientAccessible: public HWNDHostAccessible
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

		long Initialize(class HWNDElement*);
		static HRESULT WINAPI Create(class HWNDElement*, class DuiAccessible**);

		virtual HRESULT Disconnect();
		virtual HRESULT WINAPI get_accParent(IDispatch**);
	};

  
}