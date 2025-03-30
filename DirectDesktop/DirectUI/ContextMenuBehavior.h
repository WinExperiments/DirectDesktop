#pragma once

namespace DirectUI
{
	enum CONTEXT_MENU_DEVICE_ORIGIN
	{
		CMDO_KEYBOARD = 0,
		CMDO_MOUSE = 1,
		CMDO_TOUCH = 2,
	};

	enum CONTEXT_MENU_INVOKE_AREA
	{
		CMIA_ELEMENTRECT = 0,
		CMIA_INVOKINGPOINT = 1,
	};

	class ContextMenuBehavior : public IDuiBehaviorImpl
	{
	public:
		UILIB_API static HRESULT WINAPI Create(IDuiBehavior** ppBehavior);

		UILIB_API static UID WINAPI ContextMenuRequested();
		UILIB_API static UID WINAPI ContextMenuHintShowing();

		//~ Begin IDuiBehavior Interface
		STDMETHODIMP Init(Value* pvArgs) override;
		STDMETHODIMP GetCallbackFilters(DuiBehaviorFilters::Flags* pFilters) override;
		STDMETHODIMP OnEvent(Element* peFrom, Event* pEvent) override;
		STDMETHODIMP OnInput(Element* peFrom, InputEvent* pInput) override;
		STDMETHODIMP OnAttach(Element* pe) override;
		STDMETHODIMP OnDetach(Element* pe) override;
		//~ End IDuiBehavior Interface

		ContextMenuBehavior(ContextMenuBehavior& other) = delete;
		ContextMenuBehavior(ContextMenuBehavior&& other) noexcept = delete;

	protected:
		ContextMenuBehavior();

		static const GUID* const s_rgpIID[];

	private:
		void _OnButtonContextEvent(ButtonContextEvent* pEvent);
		void _OnKeyboardEvent(KeyboardEvent* pEvent);
		void _OnMouseEvent(MouseEvent* pEvent);
		void _OnPointerEvent(PointerEvent* pEvent);
		void _FireContextMenuRequested(const RECT& rcAvoidRelativeToAttached, CONTEXT_MENU_DEVICE_ORIGIN deviceOrigin);
		void _FireContextMenuRequestedFromKeyboard();
		void _FireContextMenuRequestedFromMouse(Element* peTarget, const POINT& ptRelativeToTarget);

		Element* _peAttached;
		CONTEXT_MENU_INVOKE_AREA _invokeArea;
		DWORD _dwPointerID;
		POINT _ptTouchScreenRelative;
	};

	struct ContextMenuRequestedEvent : Event
	{
		RECT rcAvoidRelativeToRoot;
		POINT ptInvokeRelativeToRoot;
		CONTEXT_MENU_DEVICE_ORIGIN deviceOrigin;
	};
}
