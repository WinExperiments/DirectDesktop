#pragma once

namespace DirectUI
{
	class UILIB_API ScrollBar
		: public Element
		  , public BaseScrollBar
	{
	public:
		ScrollBar(ScrollBar const&);
		ScrollBar();
		virtual ~ScrollBar();
		ScrollBar& operator=(ScrollBar const&);

		static HRESULT WINAPI Create(Element*, unsigned long*, Element* *);
		static HRESULT WINAPI Create(bool, Element*, unsigned long*, Element* *);
		static IClassInfo* __stdcall GetClassInfoPtr(void);
		static PropertyInfo const* WINAPI LineProp(void);
		static PropertyInfo const* WINAPI MaximumProp(void);
		static PropertyInfo const* WINAPI MinimumProp(void);
		static PropertyInfo const* WINAPI OrderProp(void);
		static PropertyInfo const* WINAPI PageProp(void);
		static PropertyInfo const* WINAPI PositionProp(void);
		static PropertyInfo const* WINAPI ProportionalProp(void);
		static long __stdcall Register(void);
		static void __stdcall SetClassInfoPtr(IClassInfo*);
		static PropertyInfo const* __stdcall VerticalProp(void);

		int GetOrder(void);
		bool GetVertical(void);
		HRESULT Initialize(bool, Element*, unsigned long*);
		long SetOrder(int);
		long SetProportional(bool);
		long SetVertical(bool);

		virtual IClassInfo* GetClassInfoW();
		virtual Element* GetElement();
		virtual int GetLine();
		virtual int GetMaximum();
		virtual int GetMinimum();
		virtual int GetPage();
		virtual int GetPosition();
		virtual bool GetProportional();
		virtual void OnEvent(Event*);
		virtual void OnPropertyChanged(PropertyInfo const*, int, Value*, Value*);
		virtual bool OnPropertyChanging(PropertyInfo const*, int, Value*, Value*);
		virtual long SetLine(int);
		virtual long SetMaximum(int);
		virtual long SetMinimum(int);
		virtual long SetPage(int);
		virtual long SetPosition(int);
		virtual void _SelfLayoutDoLayout(int, int);
		virtual SIZE _SelfLayoutUpdateDesiredSize(int, int, Surface*);

	protected:
		//1
		virtual long CreateButtons();

	private:
		static IClassInfo* s_pClassInfo;
	};

	class UILIB_API ScrollBarRangeValueProxy : public RangeValueProxy
	{
	public:
		ScrollBarRangeValueProxy(ScrollBarRangeValueProxy const&);
		ScrollBarRangeValueProxy();
		ScrollBarRangeValueProxy& operator=(ScrollBarRangeValueProxy const&);

		virtual long DoMethod(int, char*);

	protected:
		virtual void Init(Element*);
	};
}
