#pragma once

namespace DirectUI
{
	struct ButtonContextEvent : Event
	{
		UINT uModifiers;
		POINT pt;
	};

	class UILIB_API Button : public Element
	{
	public:
		static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
		static HRESULT WINAPI Create(UINT nActive, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		void OnInput(InputEvent* pie) override;

		static UID WINAPI Click();
		static UID WINAPI Context();

		static const PropertyInfo* WINAPI PressedProp();
		static const PropertyInfo* WINAPI CapturedProp();

		bool GetPressed();
		bool GetCaptured();
		HRESULT SetPressed(bool v);
		HRESULT SetCaptured(bool v);

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		IClassInfo* GetClassInfoW() override;

		static HRESULT WINAPI Register();

		void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;

		virtual bool OnLostDialogFocus(IDialogElement* pDE);
		virtual bool OnReceivedDialogFocus(IDialogElement* pDE);

		HRESULT DefaultAction() override;

		Button();
		Button(const Button&) = default;

		// ReSharper disable once CppHidingFunction
		HRESULT Initialize(UINT nActive, Element* pParent, DWORD* pdwDeferCookie);

		~Button() override;

	private:
		int _bRightButtonPressed;
	};
}