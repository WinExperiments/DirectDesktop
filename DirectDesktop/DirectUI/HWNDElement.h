#pragma once

namespace DirectUI
{
	class UILIB_API HWNDElement : public ElementWithHWND
	{
	public:
		static HRESULT WINAPI Create(HWND hParent, bool fDblBuffer, UINT nCreate, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		void OnGroupChanged(int fGroups, bool bLowPri) override;
		void OnDestroy() override;
		void OnEvent(Event* pEvent) override;
		virtual void OnThemeChanged(ThemeChangedEvent* pev);
		virtual void OnImmersiveColorSchemeChanged();
		void OnInput(InputEvent* pie) override;
		virtual void OnNoChildWithShortcutFound(KeyboardEvent* pke);
		virtual void OnGetDlgCode(MSG* msg, LRESULT* plResult);
		virtual void OnWmThemeChanged(WPARAM wParam, LPARAM lParam);
		virtual void OnWmSettingChanged(WPARAM wParam, LPARAM lParam);
		virtual void OnCompositionChanged();
		virtual bool CanSetFocus();
		virtual bool IsMSAAEnabled();
		void UpdateTooltip(Element* pe) override;
		void ActivateTooltip(Element* pe, DWORD dwFlags) override;
		void RemoveTooltip(Element* pe) override;

		void DelayActivateTooltip();
		virtual HRESULT CreateStyleParser(DUIXmlParser** ppParser);
		Element* ElementFromPoint(POINT* ppt);

		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void FlushWorkingSet();

		HWND GetHWND() override;
		virtual void GetWindowClassNameAndStyle(const WCHAR** ppchClassName, UINT* puiClassStyle);

		static Element* WINAPI GetKeyFocusedElement();
		static HWNDElement* WINAPI GetFocusedHWNDElement();
		static bool WINAPI FindShortcut(WCHAR ch, Element* pe, Element** ppeFound, BOOL* pfMultiple, BOOL* pfUseNext, BOOL fSysKey);

		bool IsFirstElement(Element* peTarget);
		bool IsLastElement(Element* peTarget);
		void SetFocus(bool fFirstElement);
		void ToggleUIState(bool fUpdateAccel, bool fUpdateFocus);
		void ShowUIState(bool fUpdateAccel, bool fUpdateFocus);
		WORD GetUIState();
		bool ShowAccel();
		bool ShowFocus();
		void SetParentSizeControl(bool bParentSizeControl);
		void SetScreenCenter(bool bScreenCenter);
		bool ShouldUseDesktopPerMonitorScaling(); // TODO Check when was this added

		static UID WINAPI ThemeChange();
		static UID WINAPI CompositionChange();
		static UID WINAPI ImmersiveColorSchemeChange(); // TODO Check when was this added
		static UID WINAPI WindowDpiChanged(); // TODO Check when was this added

		static const PropertyInfo* WINAPI WrapKeyboardNavigateProp();
		bool GetWrapKeyboardNavigate();
		HRESULT SetWrapKeyboardNavigate(bool v);

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		IClassInfo* GetClassInfoW() override;

		static HRESULT WINAPI Register();

		HRESULT GetAccessibleImpl(IAccessible**) override;

		HWNDElement();
		HWNDElement(const HWNDElement&) = default;

		~HWNDElement() override;

		// ReSharper disable once CppHiddenFunction
		HRESULT Initialize(HWND hParent, bool fDblBuffer, UINT nCreate, Element* pParent, DWORD* pdwDeferCookie);

	protected:
		static bool WINAPI FindShortcutRecursive(WCHAR ch, Element* pe, Element** ppeFound, BOOL* pfMultiple, BOOL* pfUseNext, BOOL fSysKey);
		void UpdateStyleSheets();
		virtual void _OnUIStateChanged(WORD wUIStateOld, WORD wUIStateNew);
		void _HandleImmersiveColorSchemeChange();
		int _GetPerMonitorScaleFactorForDesktopWindow(HWND hwnd);
		void _FireWindowDpiChangeEvent();

		HWND _hWnd;
		HWND _hWndTooltip;
		HPALETTE _hPal;
		bool _bParentSizeControl;
		bool _bScreenCenter;
		WORD _wUIState;
		Element* _peDeepTouchElement; // @WARNING: NEW SOMEWHERE IN 10
		Element* _peLastTrackingTooltip;
		bool _bMouseTrackTooltip;
		bool _bTooltipVisible;
		DWORD _dwTooltipHiddenTime;

	private:
		int _UpdateDesktopScaleFactor();
	};

	class UILIB_API HWNDElementProvider
		: public ElementProvider
		, public IRawElementProviderFragmentRoot
	{
	public:
		PfnCreateProxy GetProxyCreator() override;

		static HRESULT WINAPI Create(HWNDElement* pe, InvokeHelper* pih, HWNDElementProvider** ppprv);

		HWNDElementProvider();

		STDMETHODIMP_(ULONG) AddRef() override;
		STDMETHODIMP_(ULONG) Release() override;
		STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;

		//~ Begin IRawElementProviderFragmentRoot Interface
		STDMETHODIMP ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** ppprv) override;
		STDMETHODIMP GetFocus(IRawElementProviderFragment** ppprv) override;
		//~ End IRawElementProviderFragmentRoot Interface

	protected:
		virtual HRESULT Init(HWNDElement* pe, InvokeHelper* pih);
	};

	class UILIB_API DECLSPEC_NOVTABLE HWNDElementProxy : public ElementProxy
	{
	public:
		static HWNDElementProxy* WINAPI Create(HWNDElement* pe);

		virtual void Init(HWNDElement* pe);

		//~ Begin DirectUI::ElementProxy Interface
		HRESULT DoMethod(MethodId methodId, va_list args) override;
		//~ End DirectUI::ElementProxy Interface

		HWNDElementProxy(const HWNDElementProxy&) = default;

	protected:
		HWNDElementProxy();

		HRESULT ElementFromPoint(double x, double y, IRawElementProviderFragment** ppprv);
		HRESULT GetFocus(IRawElementProviderFragment** ppprv);
	};
}
