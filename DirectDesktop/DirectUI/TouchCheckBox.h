#pragma once

namespace DirectUI
{
	enum CheckedStateFlags
	{
		CSF_Unchecked = 0,
		CSF_Checked = 1,
	};

	class TouchCheckBox : public TouchButton
	{
	public:
		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
		UILIB_API static HRESULT WINAPI Create(UINT nActive, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		UILIB_API void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent) override;
		UILIB_API bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnEvent(Event* pEvent) override;
		UILIB_API HRESULT Insert(Element** ppe, UINT cCount, UINT iInsertIdx) override;
		UILIB_API void GetImmersiveFocusRectOffsets(RECT* prc) override;

		UILIB_API static const PropertyInfo* WINAPI CheckedStateProp();
		UILIB_API CheckedStateFlags GetCheckedState();
		UILIB_API HRESULT SetCheckedState(CheckedStateFlags v);

		UILIB_API static const PropertyInfo* WINAPI ToggleOnClickProp();
		UILIB_API bool GetToggleOnClick();
		UILIB_API HRESULT SetToggleOnClick(bool v);

		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;

		UILIB_API static HRESULT WINAPI Register();

		UILIB_API TouchCheckBox();
		TouchCheckBox(const TouchCheckBox&) = default;

		// ReSharper disable once CppHidingFunction
		UILIB_API HRESULT Initialize(UINT nActive, Element* pParent, DWORD* pdwDeferCookie);

		UILIB_API ~TouchCheckBox() override;

	private:
		HRESULT _CreateAndAddGlyph();
		HRESULT _CreateAndAddLabel();
		Element* _GetGlyph();
		Element* _GetLabel();
		void _UpdateAccState(bool, CheckedStateFlags);
	};
}
