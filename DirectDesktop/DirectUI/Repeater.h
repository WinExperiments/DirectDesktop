#pragma once

namespace DirectUI
{
	class UILIB_API Repeater : public Macro
	{
	public:
		static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		// ReSharper disable once CppHidingFunction
		HRESULT Initialize(Element* pParent, DWORD* pdwDeferCookie);

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		IClassInfo* GetClassInfoW() override;

		static HRESULT WINAPI Register();
		void SetDataEngine(IDataEngine* pie);
		void SetGraphicType(BYTE dDefaultGraphicType);

	protected:
		IDataEngine* _piDataEngine;

		HRESULT BuildElement() override;

	private:
		void SetDataEntry(IDataEntry* pide)
		{
			// Not exported
		}
	};
}
