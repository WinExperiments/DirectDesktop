#pragma once

namespace DirectUI
{
	class TouchHWNDElement;

	struct ButtonClickEvent : Event
	{
		UINT nCount;
		UINT uModifiers;
		POINT pt;

		ButtonClickEvent()
			: nCount(0)
			, uModifiers(0)
			, pt()
		{
		}
	};

	class TouchButton : public RichText
	{
	public:
		enum ClickDevice
		{
			ClickDevice_None = 0,
			ClickDevice_Keyboard = 1,
			ClickDevice_Mouse = 2,
			ClickDevice_Pointer = 3,
		};

		struct TouchButtonClickEvent : ButtonClickEvent
		{
			ClickDevice clickDevice;

			TouchButtonClickEvent()
				: clickDevice(ClickDevice_None)
			{
			}
		};

		UILIB_API static HRESULT WINAPI Create(UINT nActive, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		UILIB_API void OnHosted(Element* peNewRoot) override;
		UILIB_API void OnUnHosted(Element* peOldRoot) override;
		UILIB_API void OnInput(InputEvent* pie) override;
		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnEvent(Event* pEvent) override;
		UILIB_API HRESULT DefaultAction() override;
		UILIB_API void GetImmersiveFocusRectOffsets(RECT* prc) override;
		UILIB_API HRESULT GetForegroundColorRef(COLORREF* pclrForeground) override;

		UILIB_API static UID WINAPI Click();
		UILIB_API static UID WINAPI MultipleClick();
		UILIB_API static UID WINAPI RightClick();

		UILIB_API static const PropertyInfo* WINAPI PressedProp();
		UILIB_API bool GetPressed();
		UILIB_API HRESULT SetPressed(bool v);

		UILIB_API static const PropertyInfo* WINAPI CapturedProp();
		UILIB_API bool GetCaptured();
		UILIB_API HRESULT SetCaptured(bool v);

		UILIB_API static const PropertyInfo* WINAPI HandleEnterProp();
		UILIB_API bool GetHandleEnter();
		UILIB_API HRESULT SetHandleEnter(bool v);

		UILIB_API static const PropertyInfo* WINAPI ShowKeyFocusProp();
		UILIB_API bool GetShowKeyFocus();
		UILIB_API HRESULT SetShowKeyFocus(bool v);

		UILIB_API static const PropertyInfo* WINAPI TreatRightMouseButtonAsLeftProp();
		UILIB_API bool GetTreatRightMouseButtonAsLeft();
		UILIB_API HRESULT SetTreatRightMouseButtonAsLeft(bool v);

		UILIB_API static const PropertyInfo* WINAPI HandleGlobalEnterProp();
		UILIB_API bool GetHandleGlobalEnter();
		UILIB_API HRESULT SetHandleGlobalEnter(bool v);

		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		UILIB_API static void WINAPI SetClassInfoPtr(IClassInfo*);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;
		UILIB_API static HRESULT WINAPI Register();

		UILIB_API TouchButton();
		TouchButton(const TouchButton&) = default;

		// ReSharper disable once CppHidingFunction
		UILIB_API HRESULT Initialize(UINT nActive, Element* pParent, DWORD* pdwDeferCookie);

		UILIB_API bool CancelClick(ClickDevice nClickDevice);
		UILIB_API ClickDevice GetClickDevice();

		UILIB_API ~TouchButton() override;

		UILIB_API void FireClickEvent(UINT cClick, UINT uModifiers, ClickDevice clickDevice, POINT* ppt);
		UILIB_API void FireRightClickEvent(UINT uModifiers, POINT* ppt);

	private:
		class TouchButtonListenerHelper : public CSafeElementListenerCB
		{
		public:
			TouchButtonListenerHelper();

			TouchButtonListenerHelper(const TouchButtonListenerHelper&) = default;
			TouchButtonListenerHelper(TouchButtonListenerHelper&&) noexcept = default;

			void SetThisElement(TouchButton* peTouchButton);

			void OnListenedInput(Element* peFrom, InputEvent* pInput) override;

		private:
			bool _IsPressable();

			TouchButton* _peTouchButton;
		};

		void _UpdateAccState(bool, bool);
		void _OnKeyboardEvent(KeyboardEvent*);
		void _OnMouseEvent(MouseEvent*);
		void _OnPointerEvent(PointerEvent*);
		bool _StartClick(ClickDevice);
		bool _UpdateClick(ClickDevice, bool);
		bool _FinishClick(ClickDevice, UINT, UINT, POINT*);
		bool _TriggerRightClick(UINT, POINT*);
		void _SyncDefaultEnterHandling(TouchHWNDElement*);

		ClickDevice _nClickDevice;
		UINT _nClickPointerID;
		UINT _nLastCanceledClickPointerID;
		TouchButtonListenerHelper _listenerHelper;
		bool _fRegisteredForDefaultEnterHandling;

		UID _UpdateFocusVisibility();
	};
}
