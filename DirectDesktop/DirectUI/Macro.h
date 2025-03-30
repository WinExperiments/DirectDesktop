#pragma once

namespace DirectUI
{
	class UILIB_API Macro : public Element
	{
	public:
		static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		// ReSharper disable once CppHiddenFunction
		HRESULT Initialize(Element* pParent, DWORD* pdwDeferCookie);

		static const PropertyInfo* WINAPI ExpandProp();
		const WCHAR* GetExpand(Value** ppv);
		HRESULT SetExpand(const WCHAR* v);

		void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		HRESULT Add(Element** ppe, UINT cCount) override;

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		IClassInfo* GetClassInfoW() override;

		static HRESULT WINAPI Register();

		void SetDataEntry(IDataEntry* pide, Element* peBindings);
		IDataEntry* GetDataEntry();
		void SetParser(DUIXmlParser* pParser);
		void SetDefaultGraphicType(BYTE defaultGraphicType, bool fOffOpaqueGraphicType);

	protected:
		Element* _peBindings;
		IDataEntry* _pide;
		DUIXmlParser* _pXmlParser;

		virtual HRESULT BuildElement();

		void ResolveBindings();
		void _BitAccurateFillRect(HDC hDC, int x, int y, int w, int h, BYTE r, BYTE g, BYTE b, BYTE a, DWORD dwROP);
		bool _GetBitmapSize(HBITMAP hBitmap, SIZE* pSize);
		Value* _LoadImage32BitsPerPixel(const WCHAR* pszPath);

		int _fResolveBindings;
		BYTE _dDefaultGraphicType;
		bool _fOffOpaqueGraphicType;
	};
}
