#pragma once

namespace DirectUI
{
	class CIHMCallback;

	enum IHMState
	{
		IHMState_AnimatingToShow = 0,
		IHMState_Shown = 1,
		IHMState_AnimatingToHide = 2,
		IHMState_Hidden = 3
	};

	enum TouchHWNDElementFlags
	{
		THEF_None = 0,
		THEF_AllowBackgroundTooltip = 1,
		THEF_TreatRightButtonAsLeft = 2,
		THEF_ConstrainTooltipToWorkspace = 4,
		THEF_NoAutoFocusRectVisibility = 8,
		THEF_DisableTooltips = 16
	};

	// NOTE: 56 exports for DirectUI::TouchHWNDElement
	class TouchHWNDElement : public HWNDElement
	{
	public:
		UILIB_API static HRESULT WINAPI Create(HWND hwndParent, bool fDblBuffer, UINT nCreate, Element* peParent, DWORD* pdwDeferCookie, Element** ppe);

		UILIB_API static UID WINAPI TooltipTimerStarting();
		UILIB_API static UID WINAPI TextTooltipShowing();
		UILIB_API static UID WINAPI RichTooltipShowing();
		UILIB_API static UID WINAPI ForwardingWindowMessage();
		UILIB_API static UID WINAPI ProcessingKeyboardNavigation();
		UILIB_API static UID WINAPI ScaleChanged();
		UILIB_API static UID WINAPI MonitorPowerSettingsChange();
		UILIB_API static UID WINAPI IHMNotify();

		UILIB_API static const PropertyInfo* WINAPI FlagsProp();
		UILIB_API TouchHWNDElementFlags GetFlags();
		UILIB_API HRESULT SetFlags(TouchHWNDElementFlags v, TouchHWNDElementFlags vmask);

		UILIB_API static const PropertyInfo* WINAPI WindowAccessGradientColorProp();
		UILIB_API Value* GetWindowAccessGradientColor();
		UILIB_API HRESULT SetWindowAccessGradientColor(Value* v);

		UILIB_API static const PropertyInfo* WINAPI TooltipMaximumLineCountProp();
		UILIB_API int GetTooltipMaximumLineCount();
		UILIB_API HRESULT SetTooltipMaximumLineCount(int v);

		UILIB_API static const PropertyInfo* WINAPI LightDismissIHMProp();
		UILIB_API bool GetLightDismissIHM();
		UILIB_API HRESULT SetLightDismissIHM(bool v);

		// ReSharper disable once CppHidingFunction
		UILIB_API HRESULT Initialize(HWND hwndParent, bool fDblBuffer, UINT nCreate, Element* peParent, DWORD* pdwDeferCookie);

		UILIB_API UINT MessageCallback(GMSG* pGMsg) override;

		UILIB_API void OnInput(InputEvent* pInput) override;
		UILIB_API void OnEvent(Event* pEvent) override;
		UILIB_API void OnKeyFocusMoved(Element* peFrom, Element* peTo) override;
		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;

		UILIB_API LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		UILIB_API bool IsMSAAEnabled() override;

		UILIB_API void OnDestroy() override;
		UILIB_API void UpdateTooltip(Element* pe) override;
		UILIB_API void ActivateTooltip(Element*, DWORD dwFlags) override;
		UILIB_API void RemoveTooltip(Element* pe) override;

		UILIB_API HRESULT StartRichTooltipTimer(TOUCHTOOLTIP_INPUT touchTooltipInput);
		UILIB_API HRESULT ShowRichTooltip(TOUCHTOOLTIP_INPUT touchTooltipInput, TOUCHTOOLTIP_OPTION_FLAGS touchTooltipOptions, Element* peTooltipContents);
		UILIB_API HRESULT HideTouchTooltip();

		UILIB_API HRESULT RegisterForMonitorPowerChanges();
		UILIB_API HRESULT UnregisterForMonitorPowerChanges();

		UILIB_API bool ShouldUsePerMonitorScaling();
		UILIB_API void UsePerMonitorScaling(HMONITOR hMon);

		UILIB_API HRESULT RegisterForIHMChanges();
		UILIB_API void UnregisterForIHMChanges();

		UILIB_API HRESULT GetIHMRect(RECT* prc);
		UILIB_API IHMState GetIHMState();
		UILIB_API HRESULT DismissIHMAsync();

		UILIB_API void RegisterForAnimationStatusChanges();
		UILIB_API void UnregisterForAnimationStatusChanges();
		UILIB_API bool IsRegisteredForAnimationStatusChanges();

		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;

		UILIB_API static HRESULT WINAPI Register();

		UILIB_API TouchHWNDElement();
		UILIB_API ~TouchHWNDElement() override;

	protected:
		UILIB_API void _OnUIStateChanged(WORD wUIStateOld, WORD wUIStateNew) override;
		UILIB_API void _ClearTooltipState();
		UILIB_API void _DestroyTooltip();

	private:
		static bool WINAPI _IsElementContentTruncated(RichText* pe);
		UID _DismissIHM();
		void _OnIHMNotify(IHMState ihmstate);
		HRESULT _EnsureTooltip();

		Element* _TargetElementFromEventInfo(Element* peTarget, const POINT* ppt);

		void _SetActiveTooltipElement(Element* pe, TOUCHTOOLTIP_INPUT touchTooltipInput);
		void _OnTooltipTimerFired(TOUCHTOOLTIP_INPUT touchTooltipInput, TOUCHTOOLTIP_TYPE touchTooltipType, TOUCHTOOLTIP_DELAY touchTooltipDelay);
		HRESULT _ShowTooltipForElement(TOUCHTOOLTIP_INPUT touchTooltipInput, Element* pe);
		DEVICE_SCALE_FACTOR _UpdateImmersiveScaleFactor();
		void _UpdateChildrenForAcceleratorDisplayChange(Element* pe);
		bool _ShouldUseImmersivePerWindowScaling();
		void _HideFocusRect();
		void _FireScaleChangeEvent();
		void _UpdateImmersiveScaleFactorAndMaybeFireScaleChangeEvent();
		void _OnListenerDetach(Element* pe);
		CSafeElementListenerPtr<Element> _speWithTooltip;
		ITouchTooltip* _pTooltip;
		POINT _ptLastMousePosition;
		UINT _nLastPointerID;
		POINT _ptLastPointerPosition;
		bool _fProcessingKeyboardNavigationEvent;
		DWORD _dwScaleChangeCookie;
		HMONITOR _hMonitorForScaling;
		HWND _hwndScaleChange;
		TOUCHTOOLTIP_CREATE_FLAGS _touchTooltipCreateFlags;
		UINT _cMonitorRegistrations;
		HPOWERNOTIFY _hPower;
		UINT _cIHMRegistrations;
		CIHMCallback* _pIHMCallback;
		UINT _cAnimationStatusRegistrations;
		bool _fDismissIHMOnEvent;
		bool _fDismissIHMEventPosted;
		bool _fLightDismissIHMUnregisterIHMChanges;

		class TooltipEventSink : public CSafeElementListenerCB, public ITouchTooltipEventSink
		{
		public:
			static HRESULT WINAPI s_CreateInstance(TouchHWNDElement* peTouchHWNDElement, REFIID riid, void** ppv);

			//~ Begin ITouchTooltipEventSink Interface
			STDMETHODIMP OnTooltipTimerFired(TOUCHTOOLTIP_INPUT touchTooltipInput, TOUCHTOOLTIP_TYPE touchTooltipType, TOUCHTOOLTIP_DELAY touchTooltipDelay) override;
			STDMETHODIMP OnTooltipHidden() override;
			//~ End ITouchTooltipEventSink Interface

			//~ Begin IElementListener Interface
			void OnListenerDetach(Element* peFrom) override;
			//~ End IElementListener Interface

		protected:
			static const IID* const s_rgpIID[];
			~TooltipEventSink();
			HRESULT SetTouchHWNDElement(TouchHWNDElement*);

		private:
			CSafeElementListenerPtr<TouchHWNDElement> _speTouchHWNDElement;

		public:
			TooltipEventSink();
		};

		class ElementWithTooltipListener : public CSafeElementListenerCB
		{
		public:
			void SetTouchHWNDElement(TouchHWNDElement*);

			void OnListenerDetach(Element*) override;

		private:
			TouchHWNDElement* _peTouchHWNDElement;
		};

		ElementWithTooltipListener elementWithTooltipListener;
	};
}
