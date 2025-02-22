#pragma once

namespace DirectUI
{
	class Viewer;

	class UILIB_API BaseScrollBar
	{
	public:
		BaseScrollBar(BaseScrollBar const&);
		BaseScrollBar();
		BaseScrollBar& operator=(BaseScrollBar const&);
		//0
		virtual Element* GetElement() = 0;
		//1
		virtual int GetPosition() = 0;
		//2
		virtual int GetMaximum() = 0;
		//3
		virtual int GetMinimum() = 0;
		//4
		virtual int GetPage() = 0;
		//5
		virtual int GetLine() = 0;
		//6
		virtual long SetPosition(int) = 0;
		//7
		virtual long SetMaximum(int) = 0;
		//8
		virtual long SetMinimum(int) = 0;
		//9
		virtual long SetPage(int) = 0;
		//10
		virtual long SetLine(int) = 0;
		//11
		virtual bool CanSetFocus() = 0;
		//12
		virtual void LineUp(unsigned int);
		//13
		virtual void LineDown(unsigned int);
		//14
		virtual void PageUp(unsigned int);
		//15
		virtual void PageDown(unsigned int);
		//16
		virtual void End();
		//17
		virtual void Home();

		bool IsPinned();
		bool IsScrollable();
		void OnMaximumChanged(Value*);
		void OnMinimumChanged(Value*);
		void OnPageChanged(Value*);
		bool OnPageChanging(Value*);
		void OnPositionChanged(Value*);
		bool OnPositionChanging(Value*);
		static UID WINAPI Scroll();
		void SetPinned(bool);

	private:
		int GetPageInc();
	};

	class UILIB_API BaseScrollViewer
		: public Element
		, public IElementListener
	{
	public:
		BaseScrollViewer();
		BaseScrollViewer(const BaseScrollViewer&) = default;

		~BaseScrollViewer() override;

		// ReSharper disable once CppHiddenFunction
		HRESULT Initialize(Element* pParent, DWORD* pdwDeferCookie);

		void OnEvent(Event* pEvent) override;
		void OnInput(InputEvent* pie) override;
		bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		HRESULT Add(Element** ppe, UINT cCount) override;

		//~ Begin DirectUI::IElementListener Interface
		void OnListenerAttach(Element* peFrom) override;
		void OnListenerDetach(Element* peFrom) override;
		bool OnListenedPropertyChanging(Element* peFrom, const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		void OnListenedPropertyChanged(Element* peFrom, const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		void OnListenedInput(Element* peFrom, InputEvent* pInput) override;
		void OnListenedEvent(Element* peFrom, Event* pEvent) override;
		//~ End DirectUI::IElementListener Interface

		static const PropertyInfo* WINAPI XOffsetProp();
		static const PropertyInfo* WINAPI YOffsetProp();
		static const PropertyInfo* WINAPI XScrollableProp();
		static const PropertyInfo* WINAPI YScrollableProp();
		static const PropertyInfo* WINAPI XBarVisibilityProp();
		static const PropertyInfo* WINAPI YBarVisibilityProp();
		static const PropertyInfo* WINAPI PinningProp();

		int GetXOffset();
		int GetYOffset();
		bool GetXScrollable();
		bool GetYScrollable();
		int GetXBarVisibility();
		int GetYBarVisibility();
		int GetPinning();
		int GetXScrollHeight();
		int GetYScrollWidth();
		HRESULT SetXOffset(int v);
		HRESULT SetYOffset(int v);
		HRESULT SetXScrollable(bool v);
		HRESULT SetYScrollable(bool v);
		HRESULT SetXBarVisibility(int v);
		HRESULT SetYBarVisibility(int v);
		HRESULT SetPinning(int v);

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		IClassInfo* GetClassInfoW() override;

		static HRESULT WINAPI Register();

	protected:
		Viewer* _peViewer;
		Element* _peContent;
		int _iLastLayoutPassShown;
		int _cLastLayoutPassShownCount;

		virtual HRESULT CreateScrollBars() = 0;
		virtual HRESULT AddChildren() = 0;
		virtual BaseScrollBar* GetHScroll() = 0;
		virtual BaseScrollBar* GetVScroll() = 0;

		void FireAnimationChangeEvent(bool fStart);

	private:
		int _iMouseWheelDeltaCarryOver;

		void CheckScroll(BaseScrollBar* psb, BOOL fScrollable, int iVisibility, BOOL fPreventLoop);
	};
}