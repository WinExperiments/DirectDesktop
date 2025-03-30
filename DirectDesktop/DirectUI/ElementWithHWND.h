#pragma once

namespace DirectUI
{
	class UILIB_API ElementWithHWND : public Element
	{
	public:
		static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		virtual HWND GetHWND() = 0;

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		IClassInfo* GetClassInfoW() override;

		static HRESULT WINAPI Register();
	};
}
