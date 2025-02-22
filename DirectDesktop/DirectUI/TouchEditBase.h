#pragma once

struct ITextDocument;
class ITextServices;

namespace DirectUI
{
	enum TouchEditTextMode
	{
		TETM_PlainText = 0,
		TETM_RichTextFormat = 1
	};

	enum TouchEditFilteredKeyComboFlags
	{
		TEFKCF_None = 0x0,
		TEFKCF_Ctrl_C = 0x1,
		TEFKCF_Ctrl_V = 0x2,
		TEFKCF_Ctrl_X = 0x4,
		TEFKCF_Shift_Ins = 0x8,
		TEFKCF_Ctrl_Ins = 0x10,
		TEFKCF_Ctrl_Del = 0x20,
		TEFKCF_Copy = TEFKCF_Ctrl_C | TEFKCF_Ctrl_Ins,
		TEFKCF_Cut = TEFKCF_Ctrl_X | TEFKCF_Ctrl_Del,
		TEFKCF_Paste = TEFKCF_Ctrl_V | TEFKCF_Shift_Ins
	};

	enum TouchEditPasswordRevealMode
	{
		TEPRM_Normal = 0,
		TEPRM_Disabled = 1
	};

	enum TouchEditKeyboardNavigationCapture
	{
		TEKNC_Default = 0,
		TEKNC_None = 1
	};

	class TouchEditBase : public Element
	{
	public:
		UILIB_API static UID WINAPI Enter();
		UILIB_API static UID WINAPI Paste();
		UILIB_API static UID WINAPI Cut();
		UILIB_API static UID WINAPI CaretMoved();
		UILIB_API static UID WINAPI UserTextChanged();
		UILIB_API static UID WINAPI UserTextUpdateNoChange();

		UILIB_API static const PropertyInfo* WINAPI PasswordCharacterProp();
		UILIB_API int GetPasswordCharacter();
		UILIB_API HRESULT SetPasswordCharacter(int v);

		UILIB_API static const PropertyInfo* WINAPI MaxLengthProp();
		UILIB_API int GetMaxLength();
		UILIB_API HRESULT SetMaxLength(int v);

		UILIB_API static const PropertyInfo* WINAPI MultilineProp();
		UILIB_API bool GetMultiline();
		UILIB_API HRESULT SetMultiline(bool v);

		UILIB_API static const PropertyInfo* WINAPI ReadOnlyProp();
		UILIB_API bool GetReadOnly();
		UILIB_API HRESULT SetReadOnly(bool v);

		UILIB_API static const PropertyInfo* WINAPI TextModeProp();
		UILIB_API TouchEditTextMode GetTextMode();
		UILIB_API HRESULT SetTextMode(TouchEditTextMode v);

		UILIB_API static const PropertyInfo* WINAPI SelectionForegroundColorProp();
		UILIB_API Value* GetSelectionForegroundColor();
		UILIB_API HRESULT SetSelectionForegroundColor(Value* v);

		UILIB_API static const PropertyInfo* WINAPI SelectionBackgroundColorProp();
		UILIB_API Value* GetSelectionBackgroundColor();
		UILIB_API HRESULT SetSelectionBackgroundColor(Value* v);

		UILIB_API static const PropertyInfo* WINAPI IgnoredKeyCombosProp();
		UILIB_API TouchEditFilteredKeyComboFlags GetIgnoredKeyCombos();
		UILIB_API HRESULT SetIgnoredKeyCombos(TouchEditFilteredKeyComboFlags v, TouchEditFilteredKeyComboFlags vmask);

		UILIB_API static const PropertyInfo* WINAPI KeyboardNavigationCaptureProp();
		UILIB_API TouchEditKeyboardNavigationCapture GetKeyboardNavigationCapture();
		UILIB_API HRESULT SetKeyboardNavigationCapture(TouchEditKeyboardNavigationCapture v);

		UILIB_API static const PropertyInfo* WINAPI IMEComposingProp();
		UILIB_API bool GetIMEComposing();
		UILIB_API HRESULT SetIMEComposing(bool v);

		UILIB_API static const PropertyInfo* WINAPI SyncContentWhileIMEComposingProp();
		UILIB_API bool GetSyncContentWhileIMEComposing();
		UILIB_API HRESULT SetSyncContentWhileIMEComposing(bool v);

		UILIB_API static const PropertyInfo* WINAPI ElementMovesOnIHMNotifyProp();
		UILIB_API bool GetElementMovesOnIHMNotify();
		UILIB_API HRESULT SetElementMovesOnIHMNotify(bool v);

		UILIB_API static const PropertyInfo* WINAPI FilterOnPasteProp();
		UILIB_API bool GetFilterOnPaste();
		UILIB_API HRESULT SetFilterOnPaste(bool v);

		UILIB_API static const PropertyInfo* WINAPI MoveCaretToEndOnSyncContentProp();
		UILIB_API bool GetMoveCaretToEndOnSyncContent();
		UILIB_API HRESULT SetMoveCaretToEndOnSyncContent(bool v);

		UILIB_API static const PropertyInfo* WINAPI IntegrateIMECandidateListProp();
		UILIB_API bool GetIntegrateIMECandidateList();
		UILIB_API HRESULT SetIntegrateIMECandidateList(bool v);

		UILIB_API static const PropertyInfo* WINAPI ForceEditTextToLTRProp();
		UILIB_API bool GetForceEditTextToLTR();
		UILIB_API HRESULT SetForceEditTextToLTR(bool v);

		UILIB_API static const PropertyInfo* WINAPI PreventFormatChangeUpdatingModifiedStateProp();
		UILIB_API bool GetPreventFormatChangeUpdatingModifiedState();
		UILIB_API HRESULT SetPreventFormatChangeUpdatingModifiedState(bool v);

		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;

		UILIB_API static HRESULT WINAPI Register();
		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		UILIB_API bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API bool IsContentProtected() override;

		UILIB_API const WCHAR* GetContentStringAsDisplayed(Value** ppv) override;

		UILIB_API HRESULT Insert(Element** ppe, UINT cCount, UINT iInsertIdx) override;
		UILIB_API void SetKeyFocus() override;

		UILIB_API virtual HRESULT GetTextDocument(ITextDocument** pptd);
		UILIB_API virtual HRESULT GetTextServices(ITextServices** ppts);
		UILIB_API virtual HRESULT RefreshContent();

		UILIB_API virtual HRESULT FinalizeCurrentIMEComposition();
		UILIB_API virtual HRESULT GetControllerFor(IUnknown**);

		TouchEditBase();
		TouchEditBase(const TouchEditBase&) = default;
		TouchEditBase(TouchEditBase&&) = default;

		enum PasswordReveal
		{
			PR_Show = 0,
			PR_Hide = 1
		};

	protected:
		void _SecureDelete(WCHAR* psz, UINT cch);
		void _SetPasswordReveal(PasswordReveal reveal);
		HRESULT _SaveModified();
		HRESULT _RestoreModified();
		HRESULT _GetEncodedContentString(Value* pv, WCHAR** ppszContent, UINT* pcchBuf);
		HRESULT _GetEncodedContentString(WCHAR** ppszConent, UINT* pcchBuf);

	private:
		PasswordReveal _reveal;
		bool _fSavedModified;
	};
}
