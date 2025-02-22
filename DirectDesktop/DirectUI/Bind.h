#pragma once

namespace DirectUI
{
	class UILIB_API Bind :public Element
	{
	public:
		Bind(const Bind &);
		Bind(void);
		Bind & operator=(const Bind &);
		virtual ~Bind(void);

		static const PropertyInfo * __stdcall ConnectProp(void);
		static long __stdcall Create(Element *, unsigned long *, Element * *);
		static IClassInfo * __stdcall GetClassInfoPtr(void);
		static const PropertyInfo * __stdcall PropertyProp(void);
		static long __stdcall Register(void);
		static void __stdcall SetClassInfoPtr(IClassInfo *);

		virtual IClassInfo * GetClassInfoW(void);

		const WCHAR* GetConnect(Value * *);
		const WCHAR* GetProperty(Value * *);
		long Initialize(Element *, unsigned long *);
		long SetConnect(const WCHAR*);
		long SetProperty(const WCHAR*);
	private:
		static IClassInfo* s_pClassInfo;
	};
}