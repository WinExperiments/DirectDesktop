#pragma once

class ITextHost;
interface ITextSelection;

namespace DirectUI
{
	class TouchEdit2 : public TouchEditBase, IElementListener
	{
	public:
		UILIB_API static UID WINAPI ClearButtonClicked();
		UILIB_API static UID WINAPI ContextMenuRequested();

		UILIB_API static const PropertyInfo* WINAPI PromptTextProp();
		UILIB_API const WCHAR* GetPromptText(Value** ppv);
		UILIB_API HRESULT SetPromptText(const WCHAR* psz);

		UILIB_API static const PropertyInfo* WINAPI PromptWithCaretProp();
		UILIB_API bool GetPromptWithCaret();
		UILIB_API HRESULT SetPromptWithCaret(bool v);

		UILIB_API static const PropertyInfo* WINAPI SuppressClearButtonProp();
		UILIB_API bool GetSuppressClearButton();
		UILIB_API HRESULT SetSuppressClearButton(bool v);

		UILIB_API static const PropertyInfo* WINAPI ShowClearButtonMinWidthProp();
		UILIB_API int GetShowClearButtonMinWidth();
		UILIB_API HRESULT SetShowClearButtonMinWidth(int v);

		UILIB_API static const PropertyInfo* WINAPI PasswordRevealModeProp();
		UILIB_API TouchEditPasswordRevealMode GetPasswordRevealMode();
		UILIB_API HRESULT SetPasswordRevealMode(TouchEditPasswordRevealMode v);

		UILIB_API static const PropertyInfo* WINAPI InnerBorderThicknessProp();
		UILIB_API const RECT* GetInnerBorderThickness(Value** ppv);
		UILIB_API HRESULT SetInnerBorderThickness(int l, int t, int r, int b);

		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;
		UILIB_API static HRESULT WINAPI Register();
		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		// ReSharper disable once CppHiddenFunction
		UILIB_API HRESULT Initialize(Element* pParent, DWORD* pdwDeferCookie);

		UILIB_API TouchEdit2();
		TouchEdit2(const TouchEdit2&) = default;
		TouchEdit2(TouchEdit2&&) noexcept = default;

		UILIB_API void OnHosted(Element* peNewHost) override;
		UILIB_API void OnInput(InputEvent* pie) override;
		UILIB_API bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnEvent(Event* pEvent) override;

		UILIB_API void Paint(HDC hdc, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent) override;
		UILIB_API HRESULT Add(Element** ppe, UINT cCount) override;

		UILIB_API HRESULT GetAccessibleImpl(IAccessible** ppAccessible) override;
		UILIB_API Element* GetUiaFocusDelegate() override;

		UILIB_API HRESULT GetTextDocument(ITextDocument** pptd) override;
		UILIB_API HRESULT GetTextServices(ITextServices** ppts) override;
		UILIB_API HRESULT RefreshContent() override;
		UILIB_API HRESULT FinalizeCurrentIMEComposition() override;
		UILIB_API HRESULT GetTextHost(ITextHost** ppth);

		UILIB_API HRESULT GetTextSelection(ITextSelection** pptsel);
		UILIB_API HRESULT PasteText(const WCHAR* pszText);

		UILIB_API HRESULT GetSelection(long* piStart, long* pcchSel);
		UILIB_API HRESULT SetSelection(long iStart, long cch);

		UILIB_API HRESULT SetCaretPosition(long iPos);

		UILIB_API InputScope GetInputScope(); // __MIDL___MIDL_itf_inputscope_0000_0000_0001
		UILIB_API HRESULT SetInputScope(InputScope inputscope); // __MIDL___MIDL_itf_inputscope_0000_0000_0001

		UILIB_API bool HasSelection();
		UILIB_API HRESULT SelectAll();
		UILIB_API HRESULT SelectNone();
		UILIB_API void DisableSelectionHandlesOnEmptyContent();

	private:
		void OnListenerAttach(Element* peFrom) override {}
		UILIB_API void OnListenerDetach(Element* peFrom) override;

		bool OnListenedPropertyChanging(Element* peFrom, const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override { return true; }
		UILIB_API void OnListenedPropertyChanged(Element* peFrom, const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;

		void OnListenedInput(Element* peFrom, InputEvent* pInput) override {}
		void OnListenedEvent(Element* peFrom, Event* pEvent) override {}

		HRESULT _CreateDirectChildren();
		HRESULT _CreateTextElements();
		void _UpdatePrompt();
		void _UpdateButton();
		void _RevealPassword(PasswordReveal);
		bool _IsRevealButtonActive();

		TouchEditInner* _peTouchEditInner;
		PromptText* _pePrompt;
		Element* _peTextContainer;
		TouchButton* _peButton;
		Element* _peExtra;
	};
}
