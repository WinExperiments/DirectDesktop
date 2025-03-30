#pragma once

interface IDCompositionAnimation;
interface IDCompositionDevice;
interface IDCompositionVisual;

namespace DirectUI
{
	class CActivityOverlay;

	class ModernProgressBar : public CElementWithProxy<Element>
	{
	public:
		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;

		UILIB_API ModernProgressBar();
		ModernProgressBar(const ModernProgressBar&) = default;

		UILIB_API ~ModernProgressBar() override;

		UILIB_API static const PropertyInfo* WINAPI PositionProp();
		UILIB_API static const PropertyInfo* WINAPI MinimumProp();
		UILIB_API static const PropertyInfo* WINAPI MaximumProp();
		UILIB_API static const PropertyInfo* WINAPI StateProp();
		UILIB_API static const PropertyInfo* WINAPI DeterminateProp();
		static const PropertyInfo* WINAPI ActivityOccuringProp();
		static const PropertyInfo* WINAPI AutoHeightProp();
		static const PropertyInfo* WINAPI SmoothFillAnimationProp();
		static const PropertyInfo* WINAPI AddLayeredRefProp();
		static const PropertyInfo* WINAPI IndependentAnimationsProp();

		UILIB_API int GetPosition();
		UILIB_API int GetMaximum();
		UILIB_API int GetMinimum();
		UILIB_API int GetState();
		UILIB_API bool IsDeterminate();
		UILIB_API bool IsActivityOccuring();
		UILIB_API bool IsAutoHeight();
		UILIB_API bool IsSmoothFillAnimation();
		UILIB_API bool IsAddLayeredRef();
		UILIB_API bool IsIndependentAnimations();

		UILIB_API HRESULT SetPosition(int v);
		UILIB_API HRESULT SetMaximum(int v);
		UILIB_API HRESULT SetMinimum(int v);
		UILIB_API HRESULT SetState(int v);
		UILIB_API HRESULT SetDeterminate(bool v);
		UILIB_API HRESULT SetActivityOccuring(bool v);
		UILIB_API HRESULT SetAutoHeight(bool v);
		UILIB_API HRESULT SetSmoothFillAnimation(bool v);
		UILIB_API HRESULT SetAddLayeredRef(bool v);
		UILIB_API HRESULT SetIndependentAnimations(bool v);

		UILIB_API static HRESULT WINAPI Register();
		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		// ReSharper disable once CppHidingFunction
		HRESULT Initialize(UINT nCreate, Element* pParent, DWORD* pdwDeferCookie);

		void StopIndicatorAnimation();

	protected:
		UILIB_API void OnDestroy() override;
		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent) override;
		UILIB_API void OnHosted(Element* peNewHost) override;
		UILIB_API void OnUnHosted(Element* peOldHost) override;

	private:
		enum INDICATOR_STATE
		{
			INDICATOR_NORMAL = 0,
			INDICATOR_ERROR = 1,
			INDICATOR_PAUSE = 2,
			INDICATOR_COMPLETED = 3,
		};

		enum INDICATOR_ANIMATIONTYPE
		{
			INDICATOR_ANIMATIONTYPE_NORMAL = 0,
			INDICATOR_ANIMATIONTYPE_PAUSE = 1,
			INDICATOR_ANIMATIONTYPE_TRANSITTODETERMINATE = 2,
			INDETERMINATE_ANIMATION = 3,
		};

		struct INDICATOR_PARAMS
		{
			float flPos;
			float flCurrPos;
			INDICATOR_STATE curState;
			INDICATOR_STATE preState;
			float flStartPos;
			DWORD dwStartTime;
			bool fNotFirst;
			INDICATOR_ANIMATIONTYPE animationType;
		};

		void _SetAccName(bool fIsDeterminate);
		void _SetAccValue();
		void _TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
		HRESULT _StartIndicatorAnimation(INDICATOR_ANIMATIONTYPE type);
		float _UpdateIndicatorPosition(const int nPos);
		void _AddLayeredRef();
		HRESULT _UpdateTimerState();
		void _InitIndicator();
		void _InitBarState(bool fTransitAnimating);
		void _SetHeight();
		static float WINAPI s_ComputeExpFunc(float t);
		static DWORD WINAPI s_PowerNotifyCallback(void* pvContext, ULONG ulType, void* pvSetting);
		void _CreateDotVisuals();
		void _RenderPrimaryDot(HDC hdcPaint, const RECT* prcBounds);

		DynamicArray<IDCompositionVisual*>* _pdaDots;
		IDCompositionVisual* _pDotsParent;

		HRESULT _UpdateAnimations(IDCompositionDevice* pDev, IDCompositionVisual* pVis);
		HRESULT _GetProgressAnimation(IDCompositionDevice* pDevice, double dblBeginOffset, float flBiasOffset, IDCompositionAnimation** ppAnim);
		void _DestroyAnimationsAndVisuals();

		static const UINT s_rpDotDiameter = 4;
		static const UINT s_rpDotSpacingNarrow = 8;
		static const UINT s_rpNarrowWidthThreshold = 180;
		static const UINT s_rpDotSpacingMedium = 12;
		static const UINT s_rpMediumWidthThreshold = 280;
		static const UINT s_rpDotSpacingWide = 15;

		DWORD _dwHeight;
		int _nMax;
		int _nMin;
		int _nPos;
		bool _fDeterminate;
		CActivityOverlay* _pActivityOverlay;
		CValuePtr _spvIndeterminateBarAccName;
		CValuePtr _spvDeterminateBarAccName;
		CValuePtr _spvDeterminateFormat;
		COLORREF _crBackground;
		COLORREF _crForeground;
		COLORREF _crPreForeground;
		UINT_PTR _timerID;
		HWND _hwndTimer;
		HPOWERNOTIFY _hPowerNotify;
		MONITOR_DISPLAY_STATE _monitorDisplayState;
		INDICATOR_PARAMS _indicatorParams;
		bool _fAutoHeight;
		bool _fDeterminateBarInitialized;
		bool _fAddedLayeredRef;
		bool _fAnimationRunning;
	};
}
