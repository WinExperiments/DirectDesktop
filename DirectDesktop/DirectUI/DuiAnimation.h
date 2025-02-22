#pragma once

namespace DirectUI
{
	enum PVLANIMATIONDIRECTION
	{
		PANI_DIR_LEFT = 1,
		PANI_DIR_RIGHT = 2,
		PANI_DIR_TOP = 3,
		PANI_DIR_BOTTOM = 4
	};

	enum PVLANIMATIONHINTVALUE
	{
		PANI_HINT_INT = 1,
		PANI_HINT_ELEMENT = 2
	};

	enum PVLANIMATIONSTATE
	{
		PANI_STATE_SELECTDOWN = 1,
		PANI_STATE_SELECTUP = 2,
		PANI_STATE_SELECTREVEAL = 3,
		PANI_STATE_DOWN = 4,
		PANI_STATE_UP = 5,
		PANI_STATE_IN = 6,
		PANI_STATE_OUT = 7,
		PANI_STATE_FULL = 8,
		PANI_STATE_IN_LONG = 9,
		PANI_STATE_OUT_LONG = 10,
		PANI_STATE_OUT_LEAVE_VISIBLE = 11
	};

	struct SafeElementIndexPair
	{
		CSafeElementPtr<Element> spElement;
		UINT uIndex;
	};

	struct ElementOffsetPair
	{
		Element* peTarget;
		POINT ptOffset;
	};

	struct ElementGadgetTicketPair
	{
		HGADGET hgad;
		CSafeElementPtr<Element> speTarget;
		DWORD dwTicket;
	};

	struct PANI_FromToElement
	{
		Element* peFrom;
		Element* peTo;
	};

	struct PANI_FromToInt
	{
		int nFrom;
		int nTo;
	};

	union PANI_FromTo
	{
		PANI_FromToInt nVal;
		PANI_FromToElement pElemVal;
	};

	struct PANI_FromToValue
	{
		PVLANIMATIONHINTVALUE type;
		PANI_FromTo ftv;
	};

	typedef struct _POINTFLOAT3D
	{
		FLOAT x;
		FLOAT y;
		FLOAT z;
	} POINTFLOAT3D, *PPOINTFLOAT3D;

	struct PVLAnimationZOrder
	{
		bool fUseCustomZOrder;
		DWORD dwZOrder;
	};

	struct PVLAnimationSkew
	{
		bool fInitSkew;
		bool fOriginSkew;
		POINTFLOAT ptflInitSkew;
		POINTFLOAT ptflSkew;
		POINTFLOAT ptflOriginSkew;
	};

	struct PVLAnimationSize
	{
		bool fInitSize;
		SIZE szInit;
		SIZE szDest;
	};

	struct PVLAnimationAlpha
	{
		bool fInitAlpha;
		FLOAT flInit;
		FLOAT flDest;
	};

	struct PVLAnimationClip
	{
		bool fInitClip;
		POINTFLOAT ptflInitialTopLeft;
		POINTFLOAT ptflInitialBottomRight;
		POINTFLOAT ptflFinalTopLeft;
		POINTFLOAT ptflFinalBottomRight;
	};

	struct ClipRectWithElement : PVLAnimationClip
	{
		CSafeElementPtr<Element> speTarget;
	};

	struct PVLAnimationRotation
	{
		bool fInitRotation;
		bool fOriginRotation;
		FLOAT flInitRotation;
		FLOAT flRotation;
		POINTFLOAT ptflOriginRotation;
	};

	struct PVLAnimationRotation3D
	{
		bool fInitRotation3D;
		bool fOriginRotation3D;
		POINTFLOAT3D ptfl3dInitRotation;
		POINTFLOAT3D ptfl3dRotation;
		POINTFLOAT3D ptfl3dOriginRotation;
	};

	struct PVLAnimationTranslation
	{
		bool fInitTranslation;
		POINT ptInit;
		POINT ptDest;
		bool fUseAsOffset;
		bool fChopStart;
		bool fChopStop;
		UINT uiChopIndex;
		LONG lChopStart;
		LONG lChopStop;
	};

	struct PVLAnimationTranslation3D
	{
		bool fInitTranslation3D;
		POINTFLOAT3D ptfl3dInit;
		POINTFLOAT3D ptfl3dDest;
	};

	struct PVLAnimationScale
	{
		bool fInitScale;
		bool fOriginScale;
		POINTFLOAT ptflInit;
		POINTFLOAT ptflScale;
		POINTFLOAT ptflOrigin;
	};

	struct PVLAnimationNotifyEvent : Event
	{
		int iAnimationId;
		bool fTrapped;
		bool fNotifyComplete;
		DWORD dwCookie;
		bool fManualSB;
		int nManualSBID;
		int nInsertionIndex;
		bool fLauncherTriggered;
	};

	struct PVLAnimationActionSpaceEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation, PVLAnimationRotation3D
	{
		int nAnimationOptions;
		CSafeElementPtr<Element> speNotify;
		CSafeElementPtr<Element> speLauncherCharm;
		CSafeElementPtr<Element> speLogoBackground;
		CSafeElementPtr<Element> speWash;
		CSafeElementPtr<Element> speLogoPane;
		CSafeElementPtr<Element> speLogoPaneVCrossbar;
		DEVICE_SCALE_FACTOR scaleFactor;
	};

	struct PVLAnimationAddEvent : PVLAnimationNotifyEvent
	{
		DynamicArray<CSafeElementPtr<Element>*>* pspelParents;
		DynamicArray<DynamicArray<SafeElementIndexPair*>*>* pdaToAdd;
		int nLayoutPos;
		bool fFilter;
		bool fIgnoreDelay;
	};

	struct PVLAnimationBadgeUpdateEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		CSafeElementPtr<Element> speOldBadge;
		CSafeElementPtr<Element> speNewBadge;
		POINT ptOffset;
	};

	struct PVLAnimationContentTransitionEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		DynamicArray<CSafeElementPtr<Element>*>* pspelSources;
		DynamicArray<CSafeElementPtr<Element>*>* pspelDests;
		CSafeElementPtr<Element> speParent;
		POINT ptOffset;
		int nLayoutPos;
	};

	struct PVLAnimationCrossfadeEvent : PVLAnimationNotifyEvent
	{
		CSafeElementPtr<Element> speToFadeOut;
		CSafeElementPtr<Element> speToFadeIn;
		DynamicArray<CSafeElementPtr<Element>*>* pspelToFadeOut;
		DynamicArray<CSafeElementPtr<Element>*>* pspelToFadeIn;
		bool fCloneTransforms;
		bool fLayoutAlreadySet;
		bool fDestroy;
		bool fInplace;
		bool fCollection;
	};

	struct PVLAnimationCustomDragDropScalingHintEvent : Event
	{
		bool fDrag;
		POINTFLOAT ptflScaleFactor;
		POINTFLOAT ptflScaleOrigin;
	};

	struct PVLAnimationCustomReflowHintEvent : Event
	{
		Element* peToMove;
		POINT ptInitial;
		POINT ptDest;
		bool fRowChange;
		bool fColumnChange;
		bool fParentChange;
		bool fUseNewCoordinateSpace;
		bool fUseLinearReflow;
		bool fUseAbsoluteColumnNumbers;
		bool fClipInside;
		PANI_FromToValue ftvRowChange;
		PANI_FromToValue ftvColumnChange;
		PANI_FromToValue ftvParentChange;
		bool fUseNewScalingOrigin;
		POINTFLOAT ptflScalingOrigin;
		int nNumberOfTowersToSkip;
		int nMaxTowerHeight;
		POINT ptExtraPortalingDistance;
		int nStaggerOrder;
	};

	struct PVLAnimationCustomTapHintEvent : Event
	{
		RECT rcTileBounds;
	};

	struct PVLAnimationDeleteEvent : PVLAnimationNotifyEvent
	{
		CSafeElementPtr<Element> speToDelete;
		DynamicArray<CSafeElementPtr<Element>*>* pspelToDelete;
		bool fCollection;
		bool fDestroy;
		bool fFilter;
	};

	struct PVLAnimationDragEvent : PVLAnimationNotifyEvent, PVLAnimationScale, PVLAnimationClip
	{
		CSafeElementPtr<Element> speToDrag;
		DynamicArray<CSafeElementPtr<Element>*>* pspelRemaining;
		CSafeElementPtr<Element> speContent;
		CSafeElementPtr<Element> speAdornment;
		DynamicArray<ClipRectWithElement*>* pTowerClippingRects;
		POINT ptContact;
		bool fUseCustomScale;
		bool fAnimatingTowerClip;
	};

	struct PVLAnimationDropEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation, PVLAnimationScale, PVLAnimationClip
	{
		CSafeElementPtr<Element> speToDrop;
		DynamicArray<CSafeElementPtr<Element>*>* pspelRemaining;
		CSafeElementPtr<Element> speContent;
		CSafeElementPtr<Element> speAdornment;
		DynamicArray<ClipRectWithElement*>* pTowerClippingRects;
		CSafeElementPtr<Element> speParent;
		int nIndex;
		int nLayoutPos;
		bool fUseCustomScale;
		bool fAnimatingTowerClip;
	};

	struct PVLAnimationEntranceEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		DynamicArray<CSafeElementPtr<Element>*>* pspelToEnter;
		DynamicArray<POINT>* pdaOffsets;
	};

	struct PVLAnimationExpandCollapseEvent : PVLAnimationNotifyEvent, PVLAnimationClip
	{
		CSafeElementPtr<Element> speToChange;
		CSafeElementPtr<Element> speAdditional;
		CSafeElementPtr<Element> speContainer;
		bool fExpand;
		int nLayoutPos;
		bool fConsiderAsClip;
		bool fInterruptCollapse;
	};

	struct PVLAnimationFadeEvent : PVLAnimationNotifyEvent, PVLAnimationAlpha
	{
		CSafeElementPtr<Element> speToFade;
		DynamicArray<CSafeElementPtr<Element>*>* pspelToFade;
		bool fCollection;
		bool fLeaveElementsVisible;
		int nOrderNumber;
	};

	struct PVLAnimationLaunchEvent : PVLAnimationNotifyEvent, PVLAnimationScale
	{
		CSafeElementPtr<Element> speToLaunch;
		DynamicArray<CSafeElementPtr<Element>*>* pspelRemaining;
	};

	struct PVLAnimationLauncherEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation, PVLAnimationScale, PVLAnimationZOrder
	{
		bool fLaunch;
		UINT uiVersion;
		CSafeElementPtr<Element> speLauncher;
		CSafeElementPtr<Element> speLauncherContent;
		DynamicArray<CSafeElementPtr<Element>*>* pspelTowers;
		DynamicArray<CSafeElementPtr<Element>*>* pspelGroupHeaders;
		CSafeElementPtr<Element> speStartText;
		CSafeElementPtr<Element> speUserTile;
		CSafeElementPtr<Element> speSortDropdown;
		CSafeElementPtr<Element> speSearchBox;
		POINT ptOffsetLauncher;
		POINT ptOffsetTower;
	};

	struct PVLAnimationManualStoryboard : PVLAnimationNotifyEvent
	{
		int nCode;
		int nManualStoryboardId;
		CSafeElementPtr<Element> speFinishNotify;
		bool fVerified;
	};

	struct PVLAnimationMoveEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		CSafeElementPtr<Element> speToMove;
		CSafeElementPtr<Element> speParent;
		CSafeElementPtr<Element> speCurrentParent;
		int nIndex;
		bool fTopMost;
	};

	struct PVLAnimationNotifyCompleteEvent : PVLAnimationNotifyEvent
	{
		int iStoryboardId;
		PVLAnimationNotifyEvent* panEvent;
		bool fForceUpdateIfDumped;
		CSafeElementPtr<Element> speNotify;
	};

	struct PVLAnimationNudgeEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		DynamicArray<CSafeElementPtr<Element>*>* pspelToNudge;
		DynamicArray<POINT>* pdaOffsets;
	};

	struct PVLAnimationParentChangeEvent : PVLAnimationNotifyEvent
	{
		CSafeElementPtr<Element> speReparentTarget;
		CSafeElementPtr<Element> speOldParent;
		CSafeElementPtr<Element> speNewParent;
	};

	struct PVLAnimationPopEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		CSafeElementPtr<Element> speToPop;
		CSafeElementPtr<Element> speParent;
		POINT ptOffset;
		bool fLayoutAlreadySet;
		bool fUseCurrentOpacity;
	};

	struct PVLAnimationReflowEvent : PVLAnimationNotifyEvent, PVLAnimationSize
	{
		CSafeElementPtr<Element> speContainer;
		bool fUseNewSize;
	};

	struct PVLAnimationReflowStyleEvent : Event
	{
		bool fFoundReflowStyle;
		bool fSetReflowStyle;
		int nNewReflowStyle;
		int nCurrentReflowStlye;
	};

	struct PVLAnimationRepositionEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		CSafeElementPtr<Element> speToMove;
		bool fReverse;
		int nOrderNumber;
		UINT uiVersion;
		UINT uiState;
		int nReflowStyle;
		CSafeElementPtr<Element> speAttached;
		bool fProcessed;
		ElementGadgetTicketPair* pegtpValidation;
	};

	struct PVLAnimationResizeEvent : PVLAnimationNotifyEvent, PVLAnimationSize
	{
		CSafeElementPtr<Element> speToResize;
		bool fReverse;
	};

	struct PVLAnimationRevealHideEvent : PVLAnimationNotifyEvent, PVLAnimationClip
	{
		CSafeElementPtr<Element> speTapped;
		CSafeElementPtr<Element> speOutline;
		CSafeElementPtr<Element> speBackground;
		CSafeElementPtr<Element> speContent;
		RECT rcContentClip;
		bool fReveal;
	};

	struct PVLAnimationScaleEvent : PVLAnimationNotifyEvent, PVLAnimationScale
	{
		CSafeElementPtr<Element> speToScale;
		int nOrderNumber;
		int nCustomDuration;
	};

	struct PVLAnimationSessionEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation, PVLAnimationScale, PVLAnimationZOrder
	{
		bool fLauncher;
		UINT uiVersion;
		bool fLogin;
		CSafeElementPtr<Element> speLauncher;
		CSafeElementPtr<Element> speLauncherContent;
		DynamicArray<CSafeElementPtr<Element>*>* pspelTowers;
		DynamicArray<CSafeElementPtr<Element>*>* pspelGroupHeaders;
		CSafeElementPtr<Element> speStartText;
		CSafeElementPtr<Element> speUserTileIcon;
		CSafeElementPtr<Element> speUserTileText;
		POINT ptOffsetLauncher;
		POINT ptOffsetTower;
		POINT ptOffsetStartText;
		POINT ptOffsetUserTileIcon;
		POINT ptOffsetUserTileText;
		POINT ptGroupHeaderOffset;
		CSafeElementPtr<Element> speLoginUserTile;
		CSafeElementPtr<Element> speLoginUserText;
		POINT ptOffsetLogonUI;
	};

	struct PVLAnimationSlideEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		CSafeElementPtr<Element> speToSlide;
		CSafeElementPtr<Element> speParent;
		UINT uiState;
		UINT uiDir;
		UINT uiVersion;
		bool fLayoutAlreadySet;
		bool fAffected;
		bool fPositionAlreadySet;
	};

	struct PVLAnimationSwipeEvent : PVLAnimationNotifyEvent, PVLAnimationTranslation
	{
		CSafeElementPtr<Element> speToSwipe;
		POINT ptOffset;
		CSafeElementPtr<Element> speCheckmark;
	};

	struct PVLAnimationTapEvent : PVLAnimationNotifyEvent, PVLAnimationScale, PVLAnimationRotation3D
	{
		CSafeElementPtr<Element> speToTap;
		UINT uiState;
		UINT uiVersion;
		POINT ptContact;
		bool fSkipScaleTransform;
	};

	MIDL_INTERFACE("0372C617-AEB1-4BCC-8703-DAD13E0C7D8A")
	IDuiAnimationTriggers : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE TriggerDelete(Element* peToDelete, bool fDestroy, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerAdd(Element* peToAdd, Element* peParent, UINT uIndex, int nLayoutPos, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerExpand(Element* peToExpand, Element* peAdditional, int nLayoutPos, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerCollapse(Element* peToCollapse, Element* peAdditional, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerSlide(UINT uiVersion, Element* peToSlide, Element* peParent, UINT uiState, UINT uiDir, POINT ptMove, bool fLayoutAlreadySet, bool fAffected, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerFade(Element* peToFade, UINT uiState, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerTap(Element* peToTap, UINT uiState, POINT ptContact, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerDrag(Element* peToDrag, DynamicArray<Element*>* pelRemaining, UINT uiState, POINT ptContact, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerDrop(Element* peToDrop, DynamicArray<Element*>* pelRemaining, Element* peParent, int nIndex, int nLayoutPos, POINT ptDest, bool fUseCustomScale, POINTFLOAT ptflScaleTo, POINTFLOAT ptflScaleOrigin, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerReveal(Element* peTapped, Element* peOutline, Element* peBackground, Element* peContent, const RECT* prcContentClipFrom, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerHide(Element* peTapped, Element* peOutline, Element* peBackground, Element* peContent, const RECT* prcContentClipTo, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerReflow(Element* peContainer, bool fUseNewSize, SIZE* pszDest, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerResize(Element* peToResize, SIZE szDest, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerReposition(Element* peToMove, POINT ptDest, bool fReverse, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerMove(Element* peToMove, Element* peParent, int nIndex, bool fTopMost, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerContentTransition(DynamicArray<Element*>* pelSources, DynamicArray<Element*>* pelDests, Element* peParent, POINT ptOffset, int nLayoutPos, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerPageTransition(DynamicArray<Element*>* pelSources, DynamicArray<Element*>* pelDests, Element* peParent, POINT ptOffset, int nLayoutPos, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerBadgeUpdate(Element* peOldBadge, Element* peNewBadge, POINT ptOffset, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerPop(Element* peToPop, Element* peParent, UINT uiState, POINT ptDest, POINT ptOffset, bool fLayoutAlreadySet, bool fUseCurrentOpacity, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerCrossfade(Element* peToFadeOut, Element* peToFadeIn, bool fCloneTransforms, bool fLayoutAlreadySet, bool fDestroy, bool fInplace, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerSwipe(Element* peToSwipe, UINT uiState, POINT ptOffset, Element* peCheckmark, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerShowTileNotification(Element* peOutgoing, POINT ptOutgoingDest, Element* peIncoming, POINT ptIncomingDest, UINT uTileNotificationOption, Element* peLogo, Element* peDisplayName, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerPeekTileNotification(Element* peTargetMessage, POINT ptDest, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerDeleteGroup(DynamicArray<Element*>* pelToDelete, bool fDestroy, bool fFilter, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerAddGroup(DynamicArray<Element*>* pelToAdd, Element* peParent, DynamicArray<UINT>* pdaIndices, int nLayoutPos, bool fFilter, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerAddGroupMultiParent(DynamicArray<Element*>* pelParents, DynamicArray<DynamicArray<ElementIndexPair*>*>* pdaToAdd, int nLayoutPos, bool fFilter, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerFadeGroup(DynamicArray<Element*>* pelToFade, UINT uiState, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerEntrance(DynamicArray<Element*>* pelToEnter, DynamicArray<POINT>* pdaOffsets, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerNudge(DynamicArray<Element*>* pelToNudge, DynamicArray<POINT>* pdaOffsets, UINT uiState, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerBeginStoryboard(Element* peTrapElement, int* pnStoryboardId, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerEndStoryboard(Element* peTrapElement, int nStoryboardId) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerCancelStoryboard(Element* peTrapElement, int nStoryboardId) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddReposition(int nStoryboardId, Element* peToMove, POINT ptDest, int nOrderNumber, bool fReverse, UINT uiVersion, UINT uiState) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddFade(int nStoryboardId, Element* peToFade, UINT uiState, int nOrderNumber) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddScale(int nStoryboardId, Element* peToScale, POINTFLOAT ptflOrigin, POINTFLOAT ptflScale, int nOrderNumber) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddInputPaneSlide(int nStoryboardId, Element* peToSlide, const POINT* pptInit, const POINT* pptDest, const RECT* prcInputPane, int, int iInputPaneDeltaY) = 0;
	};

	MIDL_INTERFACE("390E8C35-5643-4FFA-A0CC-BEE64802A534")
	IDuiLauncherAnimationTriggers : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE TriggerTap(Element* peToTap, UINT uiState, POINT ptContact, UINT uiVersion, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerLaunch(Element* peToLaunch, DynamicArray<Element*>* pelUnk, DWORD* pdwCookie) = 0; // @NOTE: The parameter names here are guessed (therefore not accurate) based on other common naming schemes microsoft uses in Dui70, originally they were all __formal
		virtual HRESULT STDMETHODCALLTYPE TriggerDrag(Element* peToDrag, DynamicArray<Element*>* pelRemaining, Element* peContent, Element* peAdornment, DynamicArray<ClipRectWithElement*>* pTowerClippingRects, UINT uiState, POINT ptContact, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerDrop(Element* peToDrop, DynamicArray<Element*>* pelRemaining, Element* peContent, Element* peAdornment, DynamicArray<ClipRectWithElement*>* pTowerClippingRects, Element* peParent, int nIndex, int nLayoutPos, POINT ptDest, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerSession(UINT uiVersion, UINT uiState, ElementOffsetPair* peopLauncher, DynamicArray<Element*>* pelTowers, DynamicArray<Element*>* pelGroupHeaders, ElementOffsetPair* peopStartText, ElementOffsetPair* peopUserTileIcon, ElementOffsetPair* peopUserTileText, Element* peLauncherContent, POINT ptTowerOffset, POINT ptGroupHeaderOffset, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerLogin(UINT uiVersion, Element* peLoginUserTile, Element* peLoginUserText, POINT ptOffset, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerLogonEntrance(DynamicArray<Element*>* pelToEnter, DynamicArray<POINT>* pdaOffsets, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE TriggerCrossfade(DynamicArray<Element*>* pelToFadeOut, DynamicArray<Element*>* pelToFadeIn, bool fCloneTransforms, bool fLayoutAlreadySet, bool fDestroy, bool fInplace, DWORD* pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddScale(int nStoryboardId, Element* peToScale, POINTFLOAT ptflOrigin, POINTFLOAT ptflScale, int nOrderNumber, int nCustomDuration) = 0;
	};

	class PVLAnimation
	{
	public:
		UILIB_API static UID WINAPI NotifyStart();
		UILIB_API static UID WINAPI NotifyComplete();
		UILIB_API static UID WINAPI NotifyImplicit();
		UILIB_API static UID WINAPI NotifyStoryboardComplete();
		UILIB_API static UID WINAPI ManualStoryboardVerify();
		UILIB_API static UID WINAPI ReflowStyle();
		UILIB_API static UID WINAPI CustomReflowHint();
		UILIB_API static UID WINAPI CustomDragDropScalingHint();
		UILIB_API static UID WINAPI CustomTapHint();
		UILIB_API static UID WINAPI TriggeredAnimationComplete();
		UILIB_API static UID WINAPI AnimationStatusChange();
	};

	class PVLLauncherAnimationTriggers : public IDuiLauncherAnimationTriggers
	{
	public:
		static HRESULT WINAPI Startup();
		static void Shutdown();
		static PVLLauncherAnimationTriggers* GetInstance();

		//~ Begin IDuiLauncherAnimationTriggers Interface
		STDMETHODIMP TriggerTap(Element* peToTap, UINT uiState, POINT ptContact, UINT uiVersion, DWORD* pdwCookie) override;
		STDMETHODIMP TriggerLaunch(Element* peToLaunch, DynamicArray<Element*>* pelUnk, DWORD* pdwCookie) override;
		STDMETHODIMP TriggerDrag(Element* peToDrag, DynamicArray<Element*>* pelRemaining, Element* peContent, Element* peAdornment, DynamicArray<ClipRectWithElement*>* pTowerClippingRects, UINT uiState, POINT ptContact, DWORD* pdwCookie) override;
		STDMETHODIMP TriggerDrop(Element* peToDrop, DynamicArray<Element*>* pelRemaining, Element* peContent, Element* peAdornment, DynamicArray<ClipRectWithElement*>* pTowerClippingRects, Element* peParent, int nIndex, int nLayoutPos, POINT ptDest, DWORD* pdwCookie) override;
		STDMETHODIMP TriggerSession(UINT uiVersion, UINT uiState, ElementOffsetPair* peopLauncher, DynamicArray<Element*>* pelTowers, DynamicArray<Element*>* pelGroupHeaders, ElementOffsetPair* peopStartText, ElementOffsetPair* peopUserTileIcon, ElementOffsetPair* peopUserTileText, Element* peLauncherContent, POINT ptTowerOffset, POINT ptGroupHeaderOffset, DWORD* pdwCookie) override;
		STDMETHODIMP TriggerLogin(UINT uiVersion, Element* peLoginUserTile, Element* peLoginUserText, POINT ptOffset, DWORD* pdwCookie) override;
		STDMETHODIMP TriggerLogonEntrance(DynamicArray<Element*>* pelToEnter, DynamicArray<POINT>* pdaOffsets, DWORD* pdwCookie) override;
		STDMETHODIMP TriggerCrossfade(DynamicArray<Element*>* pelToFadeOut, DynamicArray<Element*>* pelToFadeIn, bool fCloneTransforms, bool fLayoutAlreadySet, bool fDestroy, bool fInplace, DWORD* pdwCookie) override;
		STDMETHODIMP AddScale(int nStoryboardId, Element* peToScale, POINTFLOAT ptflOrigin, POINTFLOAT ptflScale, int nOrderNumber, int nCustomDuration) override;
		//~ End IDuiLauncherAnimationTriggers Interface

	protected:
		static const IID* const s_rgpIID[];

	public:
		PVLLauncherAnimationTriggers(const PVLLauncherAnimationTriggers&) = delete;

	protected:
		PVLLauncherAnimationTriggers();
		~PVLLauncherAnimationTriggers();

	private:
		static PVLLauncherAnimationTriggers* s_pLauncherInstance;
		bool _fDestroyed;
	};
}
