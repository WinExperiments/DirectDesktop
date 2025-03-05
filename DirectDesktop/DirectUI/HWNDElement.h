#pragma once

namespace DirectUI
{
	class UILIB_API HWNDElement : public ElementWithHWND
	{
	public:
		HWNDElement(const HWNDElement &);
		HWNDElement();
		virtual ~HWNDElement();
		HWNDElement& operator=(const HWNDElement &);

		//0
		virtual void OnPropertyChanged(const PropertyInfo*, int, Value*, Value*);
		//1
		virtual void OnGroupChanged(int, bool);
		//2
		virtual void OnInput(InputEvent*);
		//3
		virtual void OnDestroy();
		//4
		virtual void OnEvent(Event*);
		//5
		virtual void UpdateTooltip(Element*);

		//6
		virtual void ActivateTooltip(Element*, DWORD);
		//7
		virtual void RemoveTooltip(Element*);

		//8
		virtual IClassInfo* GetClassInfoW();
		//9
		virtual long GetAccessibleImpl(IAccessible**);


		//HWNDElement 新增函数
		//10
		virtual HWND GetHWND();

		//11
		virtual void OnThemeChanged(ThemeChangedEvent*);
		virtual void OnImmersiveColorSchemeChanged();

		//12
		virtual void OnNoChildWithShortcutFound(KeyboardEvent*);

		//13
		virtual void OnGetDlgCode(LPMSG, LRESULT*);
		//14
		virtual void OnWmThemeChanged(WPARAM wParam, LPARAM lParam);
		virtual void OnWmSettingChanged(WPARAM wParam, LPARAM lParam);
		//15
		virtual void OnCompositionChanged();

		//16
		virtual bool CanSetFocus();
		virtual bool IsMSAAEnabled();

		static UID WINAPI CompositionChange();
		static HRESULT WINAPI Create(HWND, bool, unsigned int, Element*, unsigned long*, Element**pOut);
		//17
		virtual long CreateStyleParser(DUIXmlParser**);
		void DelayActivateTooltip();
		Element* ElementFromPoint(LPPOINT);
		static bool WINAPI FindShortcut(unsigned short, Element*, Element**, int*, int*, int);
		void FlushWorkingSet();
		static IClassInfo* WINAPI GetClassInfoPtr();

		static HWNDElement* WINAPI GetFocusedHWNDElement();

		static Element* WINAPI GetKeyFocusedElement();
		unsigned short GetUIState();
		bool GetWrapKeyboardNavigate();
		long Initialize(HWND, bool, unsigned int, Element*, unsigned long*);
		bool IsFirstElement(Element*);
		bool IsLastElement(Element*);


		static long WINAPI Register();
		static void WINAPI SetClassInfoPtr(IClassInfo*);

		void SetFocus(bool);
		void SetParentSizeControl(bool);
		void SetScreenCenter(bool);
		long SetWrapKeyboardNavigate(bool);
		bool ShowAccel(void);
		bool ShowFocus(void);
		void ShowUIState(bool, bool);
		static UID WINAPI ThemeChange();

		void ToggleUIState(bool, bool);
		static const PropertyInfo* WINAPI WrapKeyboardNavigateProp();

		static LRESULT WINAPI StaticWndProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);
		//18
		virtual LRESULT WndProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual void GetWindowClassNameAndStyle(const unsigned short**, UINT*);

	protected:
		virtual void _OnUIStateChanged(WORD, WORD);
		static bool WINAPI FindShortcutRecursive(unsigned short, Element*, Element**, int*, int*, int);

	private:
		static IClassInfo* s_pClassInfo;

	protected:
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
	};

	class UILIB_API HWNDElementProvider
		: public ElementProvider
		, public IRawElementProviderFragmentRoot
	{
	public:
		HWNDElementProvider(void);
		virtual ~HWNDElementProvider(void);

		HWNDElementProvider& operator=(const HWNDElementProvider&) = delete;

		static long __stdcall Create(HWNDElement *, InvokeHelper *, HWNDElementProvider * *);

		virtual unsigned long __stdcall AddRef(void);
		virtual long __stdcall QueryInterface(struct _GUID const &, void * *);
		virtual unsigned long __stdcall Release(void);

		virtual PfnCreateProxy GetProxyCreator(void);

		//IRawElementProviderFragmentRoot
		//1
		virtual long __stdcall ElementProviderFromPoint(double, double, IRawElementProviderFragment * *);
		//2
		virtual long __stdcall GetFocus(IRawElementProviderFragment * *);

	protected:
		virtual long Init(HWNDElement *, InvokeHelper *);

	};

	class UILIB_API DECLSPEC_NOVTABLE HWNDElementProxy : public ElementProxy
	{
	public:
		static HWNDElementProxy* WINAPI Create(HWNDElement* pe);
		
		virtual void Init(HWNDElement* pe);
		
		HRESULT DoMethod(MethodId methodId, va_list args) override;

		HWNDElementProxy(const HWNDElementProxy&) = default;

	protected:
		HWNDElementProxy();
		
		HRESULT ElementFromPoint(double x, double y, IRawElementProviderFragment** ppprv);
		HRESULT GetFocus(IRawElementProviderFragment** ppprv);
	};
}