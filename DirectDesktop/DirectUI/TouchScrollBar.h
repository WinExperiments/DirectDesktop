#pragma once

struct IDirectManipulationContent;
struct IDirectManipulationViewport;
struct IDirectManipulationCompositor;
struct IDirectManipulationManager;

namespace DirectUI
{
	class TouchScrollBar : public ScrollBar
	{
	public:
		UILIB_API static UID WINAPI InteractionStart();
		UILIB_API static UID WINAPI InteractionEnd();
		UILIB_API static UID WINAPI ActiveStateChanged();
		UILIB_API static UID WINAPI AnimateScroll();

		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		// ReSharper disable once CppHidingFunction
		UILIB_API HRESULT Initialize(bool fBuildSubTree, Element* pParent, DWORD* pdwDeferCookie);

		UILIB_API void OnInput(InputEvent* pInput) override;
		UILIB_API void OnEvent(Event* pEvent) override;
		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnHosted(Element* peNewHost) override;

		UILIB_API void _SelfLayoutDoLayout(int cx, int cy) override;
		UILIB_API SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf) override;

		UILIB_API TouchScrollBar();
		UILIB_API ~TouchScrollBar() override;

		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;
		UILIB_API static HRESULT WINAPI Register();

		UILIB_API ActiveState GetActiveState();
		UILIB_API void SetActiveState(ActiveState newActiveState, bool fNoAnimation);

		UILIB_API bool IsThumbActive();
		UILIB_API Element* GetThumb();

		UILIB_API HRESULT CreateButtons() override;

		UILIB_API void LineUp(UINT nCount) override;
		UILIB_API void LineDown(UINT nCount) override;
		UILIB_API void PageUp(UINT nCount) override;
		UILIB_API void PageDown(UINT nCount) override;

		HRESULT CreateSecondaryContent(IDirectManipulationManager* pManager, IDirectManipulationCompositor* pManipulationCompositor, IDirectManipulationViewport* pManipulationViewport);
		HRESULT DeleteSecondaryContent();

		HRESULT SetOffsetDelta(float flOffsetDelta);

		HRESULT UpdateIndicatorTransforms();

	private:
		void _UpdateVisualState();
		void _SetAccNames();
		void _SetPositionOrStop(int docPos);
		HRESULT _FadeAnimation(Element* peToFade, UINT uiState);
		HRESULT _CreateAndAddInnerRichText(Element* peParent, const WCHAR* pszID, const WCHAR* ppRichText, Element** ppRichTexta);
		void _OnListenedPropertyChanged(Element* peFrom, const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

		class InnerTouchScrollBarListener : public CSafeElementListenerCB
		{
		public:
			InnerTouchScrollBarListener();
			InnerTouchScrollBarListener(const InnerTouchScrollBarListener&) = default;
			InnerTouchScrollBarListener(InnerTouchScrollBarListener&&) noexcept = default;

			void SetTouchScrollbar(TouchScrollBar*);

			//~ Begin DirectUI::IElementListener Interface
			void OnListenedPropertyChanged(Element* peFrom, const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
			//~ End DirectUI::IElementListener Interface

		private:
			TouchScrollBar* _peTouchScrollbar;
		};

		InnerTouchScrollBarListener _innerListener;

		void _GetThumbSize(int nTrackSize, int* pnDisplaySize, int* pnProportionalSize);
		HRESULT _UpdateIndicatorTransforms(bool fForceUpdate);
		HRESULT _UpdateIndicatorTransforms(float flScaleFactor);
		bool _HasIndicatorTransformChanged(float flScaleFactor);
		bool _HasViewportSizeChanged();

		enum PositionType
		{
			POS_THUMBDRAG = 0,
			POS_TRACK = 1,
			POS_NONE = 2
		};

		ActiveState _activeState;
		bool _fVisualStateChanging;
		int _cTrack;
		PositionType _posType;
		float _flUnitsPerPixel;
		DWORD _dwLastFadeOutCookie;
		bool _fNoScrollBarAnimation;
		IDirectManipulationContent* _pDManipContent;
		IDirectManipulationCompositor* _pManipulationCompositor;
		IDirectManipulationViewport* _pManipulationViewport;
		IDirectManipulationManager* _pManipulationManager;
		float _flOffsetDelta;
		float _flScaleFactor;
		RECT _rcViewport;
		CSafeElementListenerPtr<Element> _speLineUpRichText;
		CSafeElementListenerPtr<Element> _speLineDownRichText;
	};
}
