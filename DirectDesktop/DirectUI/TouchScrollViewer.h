#pragma once

interface IDirectManipulationCompositor;
interface IDirectManipulationContent;
interface IDirectManipulationManager;
interface IDirectManipulationPrimaryContent;
interface IDirectManipulationViewport;

namespace DirectUI
{
	struct ManipulationEvent : Event
	{
		bool isInertia;
		float scale;
		float translateX;
		float translateY;
		float centerX;
		float centerY;
	};

	struct SemanticChangeEvent : Event
	{
		float scaleFactor;
		float x;
		float y;
		float translateX;
		float translateY;
	};

	struct SetContactEvent : Event
	{
		UINT uiPointerID;
		bool fCancel;
	};

	struct TILE
	{
		UINT64 uiID;
		RECT rcArea;
		DynamicArray<CSafeElementPtr<Element>*>* pspelElements;
	};

	class TouchScrollViewer : public BaseScrollViewer
	{
	public:
		UILIB_API static UID WINAPI ManipulationStarting();
		UILIB_API static UID WINAPI ManipulationStarted();
		UILIB_API static UID WINAPI ManipulationDelta();
		UILIB_API static UID WINAPI ManipulationCompleted();
		UILIB_API static UID WINAPI SemanticChange();
		UILIB_API static UID WINAPI SnapshotTransformElement();
		UILIB_API static UID WINAPI ReleaseSnapshot();
		UILIB_API static UID WINAPI MapContentVisuals();
		UILIB_API static UID WINAPI SetContactNeeded();
		UILIB_API static UID WINAPI SetContactNotify();
		UILIB_API static UID WINAPI UpdateView();
		UILIB_API static UID WINAPI UpdateContentSize();

		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		// ReSharper disable once CppHidingFunction
		HRESULT Initialize(Element* pParent, DWORD* pdwDeferCookie);

		void OnInput(InputEvent* pInput) override;
		void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;

		bool IsLayered() const { return _fLayered; }
		bool IsContentElement(Element*);

	protected:
		void OnHosted(Element* peNewHost) override;
		void OnUnHosted(Element* peOldHost) override;
		void OnGroupChanged(int fGroups, bool bLowPri) override;

	public:
		void OnListenedPropertyChanged(Element* peFrom, const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;

		static const PropertyInfo* WINAPI InteractionModeProp();
		UILIB_API static const PropertyInfo* WINAPI SnapModeProp();
		static const PropertyInfo* WINAPI AllowArrowOutProp();

		UILIB_API static const PropertyInfo* WINAPI ScrollPaddingProp();
		UILIB_API static const PropertyInfo* WINAPI SnapIntervalXProp();
		UILIB_API static const PropertyInfo* WINAPI SnapOffsetXProp();
		UILIB_API static const PropertyInfo* WINAPI SnapPointCollectionXProp();
		UILIB_API static const PropertyInfo* WINAPI SnapIntervalYProp();
		UILIB_API static const PropertyInfo* WINAPI SnapOffsetYProp();
		UILIB_API static const PropertyInfo* WINAPI SnapPointCollectionYProp();

		UILIB_API static const PropertyInfo* WINAPI ManipulationVerticalAlignmentProp();
		UILIB_API static const PropertyInfo* WINAPI ManipulationHorizontalAlignmentProp();

		static const PropertyInfo* WINAPI LinkIndicatorsToContentProp();
		static const PropertyInfo* WINAPI ZoomMinimumProp();
		static const PropertyInfo* WINAPI ZoomMaximumProp();

		UILIB_API int GetInteractionMode();
		UILIB_API HRESULT SetInteractionMode(int v);

		UILIB_API int GetSnapMode();
		UILIB_API HRESULT SetSnapMode(int v);

		UILIB_API bool GetAllowArrowOut();
		UILIB_API HRESULT SetAllowArrowOut(bool v);

		UILIB_API const RECT* GetScrollPadding(Value** ppv);
		UILIB_API HRESULT SetScrollPadding(int l, int t, int r, int b);

		UILIB_API float GetZoomMinimum();
		UILIB_API HRESULT SetZoomMinimum(float v);

		UILIB_API float GetZoomMaximum();
		UILIB_API HRESULT SetZoomMaximum(float v);

		UILIB_API float GetSnapIntervalX();
		UILIB_API float GetSnapOffsetX();
		UILIB_API DynamicArray<double>* GetSnapPointCollectionX(Value** ppv);

		UILIB_API float GetSnapIntervalY();
		UILIB_API float GetSnapOffsetY();
		UILIB_API DynamicArray<double>* GetSnapPointCollectionY(Value** ppv);

		UILIB_API bool GetLinkIndicatorsToContent();

		UILIB_API HRESULT SetSnapIntervalX(float v);
		UILIB_API HRESULT SetSnapOffsetX(float v);
		UILIB_API HRESULT SetSnapPointCollectionX(const double* rgdbl, int cdbl);
		UILIB_API HRESULT SetSnapPointCollectionX(DynamicArray<double>* v);

		UILIB_API HRESULT SetSnapIntervalY(float v);
		UILIB_API HRESULT SetSnapOffsetY(float v);
		UILIB_API HRESULT SetSnapPointCollectionY(const double* rgdbl, int cdbl);
		UILIB_API HRESULT SetSnapPointCollectionY(DynamicArray<double>* v);

		UILIB_API HRESULT SetLinkIndicatorsToContent(bool v);
		UILIB_API HRESULT SetContact(UINT uiPointerID, bool fRelease);

		UILIB_API HRESULT ZoomToRect(const RECT* pcrcTarget, bool fAnimate);
		UILIB_API bool CanPerformManualVisualSwap();
		UILIB_API bool IsManualVisualSwapInProgress();
		UILIB_API HRESULT ZoomToRectManualVisualSwap(float left, float top, float right, float bottom, const int* pnHorizontalDelta, float panX, float panY, float scaleDestination, bool fAnimate);

		UILIB_API HRESULT PrepareManualSwapDeferredZoomToRect(const RECT* pcrcTarget, const float* ppanX, const float* ppanY, float* centerXOverride, float* centerYOverride, float scaleDestination);
		UILIB_API HRESULT ExecuteManualSwapDeferredZoomToRect(bool fAnimate);

		UILIB_API HRESULT SetSuppressSetContact(bool fDisable);
		UILIB_API HRESULT ResetManipulations();
		UILIB_API HRESULT ResetInputState();

		UILIB_API HRESULT SetVirtualizeElements(bool fVirtualize);
		UILIB_API void SetDisableOffscreenCaching(bool fDisableOffscreen);

		UILIB_API int GetManipulationVerticalAlignment();
		UILIB_API HRESULT SetManipulationVerticalAlignment(int v);
		UILIB_API int GetManipulationHorizontalAlignment();
		UILIB_API HRESULT SetManipulationHorizontalAlignment(int v);

		UILIB_API HRESULT SetDelegateEventHandler(IUnknown* punk);

		UILIB_API HRESULT GetVScrollbar(Element** ppeScrollbar);
		UILIB_API HRESULT GetHScrollbar(Element** ppeScrollbar);

		UILIB_API void GetInertiaEndpointVisibleRect(RECT* pRectEndpoint);
		UILIB_API float GetInertiaEndpointZoomLevel(float defaultZoomLevel);

		UILIB_API void GetVisibleRect(RECT* pRectVisibleRegion);
		UILIB_API void GetPredictedVisibleRect(RECT* pRectVisibleRegion);

		UILIB_API bool GetMouseWithinHorizontalScrollRegion();

		UILIB_API HRESULT SetScrollControlHost(Element* peHost);

		UILIB_API void GetTranslatedTileRects(RECT* prgrcTranslatedTiles, UINT* prguiIds, UINT uiBufferSize);
		UILIB_API bool IsTileMember(UINT uiTileID, Element* pe);
		UILIB_API bool IsCrossfadeInProgress();

		UILIB_API HRESULT OverrideZoomThreshold(float upperThresholdDelta, float lowerThresholdDelta, int flags);
		UILIB_API float GetContentCrossfadeOpacity();

		bool IsManipulationConfigured();
		bool IsManipulating();
		bool IsPerformingSemanticZoom();

		TouchScrollViewer();
		~TouchScrollViewer() override;
		void ProcessAnimationEvent(AnimateScrollEvent*);

		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;

		static HRESULT WINAPI Register();

		virtual HRESULT OnViewportStatusChanged(IDirectManipulationViewport*, DIRECTMANIPULATION_STATUS current, DIRECTMANIPULATION_STATUS previous);
		virtual HRESULT OnViewportUpdated(IDirectManipulationViewport*);
		virtual HRESULT OnContentUpdated(IDirectManipulationViewport*, IDirectManipulationContent*);

		UILIB_API IDirectManipulationManager* GetManipulationManager();
		UILIB_API IDirectManipulationViewport* GetManipulationViewport(bool);
		UILIB_API IDirectManipulationCompositor* GetManipulationCompositor();

		HRESULT CreateTile(const RECT* pcrcRect, UINT* puiTileID);
		HRESULT RemoveTile(UINT uiTileID);

		void InvalidateVirtualizedContainers();
		bool IsVirtualizedContainerInvalidationPending();
		void SetVirtualizeUsingElementPosition(bool fVirtualizeUsingElementPosition);

		UILIB_API void NewChildElementsAdded();
		void InvalidateChangedChildElements();
		HRESULT OnContentLayoutUpdated();
		UILIB_API HRESULT TelemetrySetDescription(const WCHAR* pszDescription);

	protected:
		HRESULT EnableManipulations(bool fEnable);
		HRESULT InitializeManipulationHelper();
		HRESULT InitializeViewport();
		void ReleaseViewport();
		void _ResetManipulations(Element* peContent);
		HRESULT SetMessage(const MSG* pMsg, bool* pbHandled);

		void OnEvent(Event* pEvent) override;
		void OnKeyFocusMoved(Element* peFrom, Element* peTo) override;
		void OnMouseFocusMoved(Element* peFrom, Element* peTo) override;

	private:
		bool _SetElementContentVisualTransform(Element* pElement, float _11, float _12, float _21, float _22, float _31, float _32); // Olive6841: Any clue what these arguments mean?
		void _MapVisuals(float newTargetX, float newTargetY, float xOffset, float yOffset, bool fAllowSubpixelMapping);

	protected:
		HRESULT CreateScrollBars() override;
		HRESULT AddChildren() override;

	public:
		BaseScrollBar* BaseScrollViewer::GetHScroll() override;

	protected:
		virtual TouchScrollBar* TouchScrollViewer_GetHScroll(); // OG name: GetHScroll

	public:
		BaseScrollBar* BaseScrollViewer::GetVScroll() override;

	protected:
		virtual TouchScrollBar* TouchScrollViewer_GetVScroll(); // OG name: GetVScroll

		void _SetLastInputType(UINT nLastInputType);

		UINT MessageCallback(GMSG* pMsg) override;
		void OnDestroy() override;

		void _OnManipulationStart();
		void _OnManipulationDelta(float panX, float panY, float scale, float centerX, float centerY);
		void _OnManipulationComplete();
		void _OnRequestManualVisualSwap(float panX, float panY, float scaleDestination, float centerX, float centerY);
		void _OnManualVisualSwapComplete();

		SimpleTimer _timerHide;
		void _OnHideScrollbarTimer();

	private:
		void _SetScrollbarStates(ActiveState, bool);

		CSafeElementPtr<Element> _peSnapshot;
		DWORD _dwCrossfadeSnapshotCookie;
		bool _fIgnoreActionComplete;
		Element* _pePendingDManipContent;
		bool _fDManipContentSet;

		HRESULT _GoLayered();
		HRESULT _GoUnlayered();
		void _SynchPendingDManipLayer();

		bool _fLayered;

		bool _fContentLayered;
		HRESULT _LayerContent();

		bool _fViewerLayered;
		HRESULT _LayerViewer();

		bool _fVirtualizeElements;
		bool _fDisableOffscreenCaching;
		bool _fLayerAlreadyIgnoringOffsets;
		int _nDraggingMode;
		int _iMouseWheelDeltaCarryOver;

		HRESULT _GetMouseMovedDelta(MouseEvent*, POINT&);
		POINT _GetScreenPosition(MouseEvent*);

		bool _fSetContactPending;
		bool _fComputingTiles;
		UINT _uiTileIndex;
		DynamicArray<TILE*>* _pdaTiles;
		bool _fTileCollectionLayered;
		DynamicArray<CSafeElementPtr<Element>*>* _pspelDeferredReleaseList;

		void _FlipRect(RECT*, RECT*);
		POINT _GetContentOrigin();
		HRESULT _AddTileRect(RECT*, TILE**);
		HRESULT _GetTileRect(UINT, RECT*);
		HRESULT _RemoveTileRect(UINT);
		bool _GadgetExistsInRect(Element*, RECT*);
		bool _ElementExistsInRect(Element*, RECT*);
		void _SetLayeredTiles(bool);
		void _LayerList(DynamicArray<CSafeElementPtr<Element>*>*, bool, DynamicArray<CSafeElementPtr<Element>*>*);
		void _ClearTiles();
		void _RecomputeTiles();
		void _SetupDeferList(DynamicArray<CSafeElementPtr<Element>*>**);
		void _FlushDeferList(DynamicArray<CSafeElementPtr<Element>*>**);
		HRESULT _UpdateTiles();
		HRESULT _UpdateTile(TILE*, DynamicArray<CSafeElementPtr<Element>*>*);
		HRESULT _UpdateTileCollectElements_Recursive(Element*, RECT*, DynamicArray<CSafeElementPtr<Element>*>*, bool, int);
		void _UpdateSizes();
		HRESULT _PrepareZoomToRect(float, const float, float, const float, float*, float*, float*, float*, float*, bool, float*, float*, float*, float*);
		HRESULT _ExecuteZoomToRect(float, float, float, float, bool);
		void _FakeDirectManipulationCallbacksToResync();
		HRESULT _GetTransformedContentRect(float*, RECT*);
		void _GetContentRectFromScrollBars(RECT*);
		void _GetSemanticZoomDeltasForDirectManipulation(float*, float*);
		void _RemoveOrRestoreSubpixelMappingOffset(bool);
		void _ShowScrollbars();
		void _RestoreScrollbarsIfNeeded();
		void _HideScrollbarsForSemanticZoom();
		void _DetectKeyboardInput();
		DIRECTMANIPULATION_CONFIGURATION _GetManipulationConfiguration(int);
		HRESULT _UpdateManipulationConfiguration(int, int);
		HRESULT _SetSnapPointBehavior(IDirectManipulationPrimaryContent*, DIRECTMANIPULATION_MOTION_TYPES, float, float);
		HRESULT _SetSnapPointCoordinate(IDirectManipulationPrimaryContent*, DIRECTMANIPULATION_MOTION_TYPES);
		HRESULT _UpdateContentSnapPoints(DIRECTMANIPULATION_MOTION_TYPES, PropertyInfo*);
		HRESULT _UpdateContentSnapPoints(DIRECTMANIPULATION_MOTION_TYPES, PropertyInfo*, PropertyInfo*);
		HRESULT _UpdateContentSnapInterval(DIRECTMANIPULATION_MOTION_TYPES, PropertyInfo*, PropertyInfo*);
		HRESULT _UpdateZoomBoundaries(PropertyInfo*, PropertyInfo*);
		HRESULT _UpdateSnapType(DIRECTMANIPULATION_MOTION_TYPES, DIRECTMANIPULATION_SNAPPOINT_TYPE);
		HRESULT _SetManipulationVerticalAlignment(DIRECTMANIPULATION_VERTICALALIGNMENT);
		HRESULT _SetManipulationHorizontalAlignment(DIRECTMANIPULATION_HORIZONTALALIGNMENT);
		HRESULT _UpdateContentSize();
		HRESULT _UpdateViewportSize();
		void _TranslateSnapPoints();
		void _SetProcessContentSizeUpdates(bool, bool);
		void _GetContentExtent(SIZE*);
		void _GetContentLocation(POINT*);
		void _CleanupSnapshotVisual();
		void _CommitDCompDevice();
		void _TelemetryUpdateViewportDescription();
		HRESULT _ReinsertDManipIntoVisualTree();
		HRESULT _CreateIndicatorContents();
		HRESULT _DeleteIndicatorContents();

		HWND _hwndRoot;
		bool _fActivated;
		bool _fEnabled;
		bool _fInertia;
		bool _fManipulation;
		bool _fListeningToContentChanges;
		bool _fOffsetPropIsChanging;
		UINT _nLastInputType;
		SIZE _sizeContentOverride;
		POINT _positionContentOverride;
		int _nOverrideContentSize;
		WCHAR* _pszTelemetryDescription;
		ManipulationHelper* _pManipulationHelper;
		IDirectManipulationCompositor* _pManipulationCompositor;
		IDirectManipulationViewport* _pManipulationViewport;
		bool _fInitializedForRealContent;
		bool _fChangedChildElements;
		bool _fSuppressSetContact;
		bool _fIsContentSizeInitialized;
		bool _fIsTiling;
		float _c11;
		float _c12;
		float _c21;
		float _c22;
		float _c31;
		float _c32;
		int _nLayerOffsetRTL;
		float _c31SemanticZoomDelta;
		float _c32SemanticZoomDelta;
		int _layerOffsetX;
		int _layerOffsetY;
		float _scaleManualSwap;
		float _rectTargetManualSwap_left;
		float _rectTargetManualSwap_top;
		float _rectTargetManualSwap_right;
		float _rectTargetManualSwap_bottom;
		float _centerXManualSwap;
		float _centerYManualSwap;
		float _panXManualSwap;
		float _panYManualSwap;
		float _subPixelMappingOffsetX;
		float _subPixelMappingOffsetY;
		bool _fCanPerformManualVisualSwap;
		bool _fManualVisualSwapRequested;
		bool _fNeedToRestoreZoomMinimum;
		POINT _ptLastMousePos;
		long _lVertScrollEventDistance;
		long _lHorizScrollEventDistance;
		TouchScrollBar* _peHScroll;
		TouchScrollBar* _peVScroll;
		SemanticController* _pSemanticController;
		bool _fDeleting;
		bool _fProcessContentSizeUpdates;
		bool _fNeedInitialMouseLocation;
		bool _fContentSizeInvalid;
		bool _fVisualsMapped;
		bool _fAllowSubPixelMapping;
		bool _fCrossfadeInProgress;
		IDuiBehavior* _pContentLayoutBehavior;
		CSafeElementPtr<Element> _speScrollHost;
		bool _fVirtualizedContainerInvalidationPending;
		bool _fVirtualizeUsingElementPosition;

		static UID WINAPI _InvalidateVirtualizedContainersEvent();

		bool _fIsPerformingSemanticZoom;
		bool _fWasAsMouseBeforeSemanticZoom;
	};
}
