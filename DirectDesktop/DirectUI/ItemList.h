 #pragma once

namespace DirectUI
{
	class ItemList : public Selector
	{
	public:
		UILIB_API static IClassInfo* GetClassInfoPtr();
		static void SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;

		UILIB_API ItemList();
		ItemList(const ItemList&) = default;

		UILIB_API ~ItemList() override;

		static const PropertyInfo* ReorderableProp();
		UILIB_API HRESULT SetReorderable(bool v);
		UILIB_API bool IsReorderable();

		UILIB_API static HRESULT Register();
		UILIB_API static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API Element* GetAdjacent(Element* peFrom, int iNavDir, const NavReference* pnr, DWORD dwFlags) override;
	};
}
