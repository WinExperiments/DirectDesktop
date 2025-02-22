#pragma once

typedef const DirectUI::PropertyInfo* (WINAPI *PropertyProcT)();

namespace DirectUI
{
	struct UpdateCache
	{
	};

	struct ElementIndexPair
	{
		Element* pElement;
		UINT uIndex;
	};

	class BehaviorStore;

	template <typename TKey, typename TValue>
	class BTreeLookup
	{
	};

	namespace Internal
	{
		struct ListenerData;
	}

	class DuiAccessible;

	typedef int (__cdecl *CompareCallback)(const void*, const void*);

	class UILIB_API Element
	{
	public:
		static HRESULT WINAPI Create(UINT nCreate, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		HRESULT Destroy(bool fDelayed);
		HRESULT DestroyAll(bool fDelayed);

		Element();
		Element(const Element&) = default;

		virtual ~Element();

		// ReSharper disable once CppHiddenFunction
		HRESULT Initialize(UINT nCreate, Element* peInitialParent, DWORD* pdwDeferCookie);

		Value* GetRawValue(const PropertyInfo* ppi, int iIndex, UpdateCache* puc); // TODO Check when was this added

		Value* GetValue(const PropertyInfo* ppi, int iIndex, UpdateCache* puc);
		Value* GetValue(PropertyProcT pPropertyProc, int iIndex, UpdateCache* puc);

		HRESULT SetValue(const PropertyInfo* ppi, int iIndex, Value* pv);
		HRESULT SetValue(PropertyProcT pPropertyProc, int iIndex, Value* pv);

		HRESULT RemoveLocalValue(const PropertyInfo* ppi);
		HRESULT RemoveLocalValue(PropertyProcT pPropertyProc);

		void StartDefer(DWORD* pdwDeferCookie);
		void EndDefer(DWORD dwDeferCookie);
		void UpdateLayout();

		DeferCycle* GetDeferObject();
		DeferCycle* TestDeferObject();

		bool IsValidAccessor(const PropertyInfo* ppi, int iIndex, bool bSetting);
		static bool IsValidValue(const PropertyInfo* ppi, Value* pv);

		bool IsRTL();
		virtual bool IsRTLReading();
		virtual bool IsContentProtected();
		virtual const WCHAR* GetContentStringAsDisplayed(Value** ppv);
		const WCHAR* GetAccNameAsDisplayed(Value** ppv);

		virtual bool OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
		virtual bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
		virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
		virtual void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

		virtual void OnGroupChanged(int fGroups, bool bLowPri);
		virtual void OnInput(InputEvent* pInput);
		virtual void OnKeyFocusMoved(Element* peFrom, Element* peTo);
		virtual void OnMouseFocusMoved(Element* peFrom, Element* peTo);
		virtual void OnDestroy();

		void FireEvent(Event* pEvent, bool fFull, bool fUseSpecifiedTarget);
		void BroadcastEvent(Event* pEvent);
		virtual void OnEvent(Event* pEvent);

		virtual void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent);
		void PaintContent(HDC hDC, const RECT* prcContent);
		void PaintStringContent(HDC hDC, const RECT* prcContent, Value* pvContent, int dCAlign);
		void PaintFocusRect(HDC hDC, const RECT* prcBounds, const RECT* prcContent);
		void PaintBorder(HDC hDC, Value* pvBackgnd, RECT* prcPaint, const RECT& rcBorder);
		void PaintBackground(HDC hDC, Value* pvBackgnd, const RECT& rcBounds, const RECT& rcInvalid, const RECT& rcPadding, const RECT& rcBorder);
		void PaintEdgeHighlight(HDC hDC, const RECT& rcPaint, const RECT& rcInvalid); // TODO Check when was this added

		virtual SIZE GetContentSize(int dConstW, int dConstH, Surface* psrf);

		float GetTreeAlphaLevel();

		HRESULT Add(Element* pe);
		virtual HRESULT Add(Element** ppe, UINT cCount);
		HRESULT Add(Element* pe, CompareCallback lpfnCompare); // TODO Check when was this added

		HRESULT Insert(Element* pe, UINT iInsertIdx);
		virtual HRESULT Insert(Element** ppe, UINT cCount, UINT iInsertIdx);

		HRESULT SortChildren(CompareCallback lpfnCompare);
		HRESULT ShiftChild(UINT iOldIndex, UINT iNewIndex); // TODO Check when was this added

		HRESULT Remove(Element* pe);
		virtual HRESULT Remove(Element** ppe, UINT cCount);
		HRESULT RemoveAll();

		Element* FindDescendent(ATOM atomID);
		void MapElementPoint(Element* peFrom, const POINT* pptFrom, POINT* pptTo);
		Element* GetImmediateChild(Element* peFrom);
		bool IsDescendent(Element* pe);
		virtual Element* GetAdjacent(Element* peFrom, int iNavDir, const NavReference* pnr, DWORD dwFlags);
		Element* GetKeyWithinChild();
		Element* GetMouseWithinChild();

		bool EnsureVisible();
		bool EnsureVisible(UINT uChild);
		virtual bool EnsureVisible(int x, int y, int cx, int cy);

		virtual void SetKeyFocus();

		HRESULT AddListener(IElementListener* pel);
		void RemoveListener(IElementListener* pel);

		virtual HRESULT AddBehavior(IDuiBehavior* pBehavior);
		virtual HRESULT RemoveBehavior(IDuiBehavior* pBehavior);

		void InvokeAnimation(int dAni, UINT nTypeMask);
		void InvokeAnimation(UINT nTypes, UINT nInterpol, float flDuration, float flDelay, bool fPushToChildren);
		void StopAnimation(UINT nTypes);

		virtual UINT MessageCallback(GMSG* pgMsg);

		SIZE _UpdateDesiredSize(int cxConstraint, int cyConstraint, Surface* psrf);
		void _UpdateLayoutPosition(int dX, int dY);
		void _UpdateLayoutSize(int dWidth, int dHeight);
		void _StartOptimizedLayoutQ();
		void _EndOptimizedLayoutQ();
		UINT _GetNeedsLayout();
		static bool _SetGroupChanges(Element* pe, int fGroups, DeferCycle* pdc);
		static void _TransferGroupFlags(Element* pe, int fGroups);
		int _SetNeedsLayout(UINT fNeedsLayout);
		static int _MarkElementForLayout(Element* pe, UINT fNeedsLayout);
		static int _MarkElementForDS(Element* pe);
		void _ClearNeedsLayout();
		static void _AddDependency(Element* pe, const PropertyInfo* ppi, int iIndex, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr);
		static HRESULT _DisplayNodeCallback(HGADGET hgadCur, void* pvCur, EventMsg* pGMsg);

		virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
		STDMETHODIMP_(ULONG) AddRef(); // @Note: Not virtual
		STDMETHODIMP_(ULONG) Release(); // @Note: Not virtual

		int _GetChangesUpdatePass();
		void Detach(DeferCycle* pdc);
		bool UiaEvents();
		void EnableUiaEvents(bool fEnable);
		bool GetClickablePoint(POINT* ptClick);
		virtual void GetImmersiveFocusRectOffsets(RECT* prc);

		template <typename T>
		void PostEvent(T* pEvent) { _PostEvent(pEvent, 0x83F8); }

	private:
		Element* FindDescendentWorker(ATOM atomID);
		void _GetBuriedSheetDependencies(const PropertyInfo* ppi, Element* peNewParent, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr);
		HRESULT _GetDependencies(const PropertyInfo* ppi, int iIndex, DepRecs* pdr, int iPCSrcRoot, Value* pvNewRoot, DeferCycle* pdc);
		static void _VoidPCNotifyTree(int iPCPos, DeferCycle* pdc);
		int _CachedValueIsEqual(const PropertyInfo* ppi, Element* peParent);
		Value* _GetLocalValueFromVM(const PropertyInfo* ppi);
		Value* _GetLocalValue(const PropertyInfo* ppi);
		Value* _GetSpecifiedValueIgnoreCache(const PropertyInfo* ppi);
		Value* _GetSpecifiedValue(const PropertyInfo* ppi, UpdateCache* puc);
		Value* _GetComputedValue(const PropertyInfo* ppi, UpdateCache* puc);
		void _UpdatePropertyInCache(const PropertyInfo* ppi);
		void _InheritProperties();
		void _FlushDS(DeferCycle*);
		HRESULT _PreSourceChange(PropertyProcT proc, int iIndex, Value* pvOld, Value* pvNew);
		HRESULT _PreSourceChange(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
		HRESULT _PostSourceChange();
		void _BroadcastEventWorker(Event* pEvent);
		void _PostEvent(Event* pEvent, int nMsg); // TODO Check when was this added
		static bool CALLBACK s_HandleDUIEventMessage(Element* pe, EventMsg* pEventMsg); // TODO Check when was this added
		UINT GetCommonDrawTextFlags(int dCAlign);
		WCHAR* RemoveShortcutFromName(const WCHAR* pszName);
		void _SyncVisible();
		void _SyncBackground();
		void _SyncRedrawStyle();
		bool IsPointValid(double x, double y);
		bool TryLinePattern(POINT* pt, const RECT& rcParent);
		bool TryPattern(double x, double y, POINT* pt, const RECT& rcParent);
		bool TrySparsePattern(POINT* pt, const RECT& rcParent);
		HRESULT _SetRelPixValue(const PropertyInfo* ppi, int nValue); // TODO Check when was this added
		HRESULT _SetRelPixRect(const PropertyInfo* ppi, int l, int t, int r, int b); // TODO Check when was this added
		HRESULT GetTheme(const WCHAR* pszClass, HTHEME* phTheme); // TODO Check when was this added

	protected:
		static void _FlushLayout(Element* pe, DeferCycle* pdc);
		static void _InvalidateCachedDSConstraints(Element* pe);
		virtual void _SelfLayoutDoLayout(int cx, int cy);
		virtual SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);
		HRESULT _SetValue(const PropertyInfo* ppi, int iIndex, Value* pv, bool fInternalCall);
		HRESULT _SetValue(PropertyProcT pPropertyProc, int iIndex, Value* pv, bool fInternalCall);
		HRESULT _RemoveLocalValue(const PropertyInfo* ppi, bool fInternalCall);
		HRESULT _RemoveLocalValue(PropertyProcT pPropertyProc, bool fInternalCall);
		virtual void OnHosted(Element* peNewRoot);
		virtual void OnUnHosted(Element* peOldRoot);
		void MarkHosted();
		void MarkSelfLayout();
		virtual void UpdateTooltip(Element* pe);
		virtual void ActivateTooltip(Element* pe, DWORD dwFlags);
		virtual void RemoveTooltip(Element* pe);
		void _OnFontPropChanged(Value* pvFont);

	public:
		void MarkNeedsDSUpdate();
		bool NeedsDSUpdate();
		void DoubleBuffered(bool fEnabled);
		int IsRoot();
		Element* GetRoot();
		Element* GetTopLevel();
		HRESULT GetRootRelativeBounds(RECT* prc);

		static RTL_CRITICAL_SECTION* GetFactoryLock();

		static UID WINAPI KeyboardNavigate();
		static UID WINAPI AnimationChange();
		static UID WINAPI DCompDeviceRebuilt(); // TODO Check when was this added

		static const PropertyInfo* WINAPI ParentProp();
		static const PropertyInfo* WINAPI ChildrenProp();
		static const PropertyInfo* WINAPI VisibleProp();
		static const PropertyInfo* WINAPI WidthProp();
		static const PropertyInfo* WINAPI HeightProp();
		static const PropertyInfo* WINAPI LocationProp();
		static const PropertyInfo* WINAPI ExtentProp();
		static const PropertyInfo* WINAPI XProp();
		static const PropertyInfo* WINAPI YProp();
		static const PropertyInfo* WINAPI PosInLayoutProp();
		static const PropertyInfo* WINAPI SizeInLayoutProp();
		static const PropertyInfo* WINAPI DesiredSizeProp();
		static const PropertyInfo* WINAPI LastDSConstProp();
		static const PropertyInfo* WINAPI LayoutProp();
		static const PropertyInfo* WINAPI LayoutPosProp();
		static const PropertyInfo* WINAPI BorderThicknessProp();
		static const PropertyInfo* WINAPI BorderStyleProp();
		static const PropertyInfo* WINAPI BorderColorProp();
		static const PropertyInfo* WINAPI PaddingProp();
		static const PropertyInfo* WINAPI MarginProp();
		static const PropertyInfo* WINAPI ForegroundProp();
		static const PropertyInfo* WINAPI BackgroundProp();
		static const PropertyInfo* WINAPI ContentProp();
		static const PropertyInfo* WINAPI FontFaceProp();
		static const PropertyInfo* WINAPI FontSizeProp();
		static const PropertyInfo* WINAPI FontWeightProp();
		static const PropertyInfo* WINAPI FontStyleProp();
		static const PropertyInfo* WINAPI FontQualityProp();
		static const PropertyInfo* WINAPI ActiveProp();
		static const PropertyInfo* WINAPI ContentAlignProp();
		static const PropertyInfo* WINAPI KeyFocusedProp();
		static const PropertyInfo* WINAPI KeyWithinProp();
		static const PropertyInfo* WINAPI MouseFocusedProp();
		static const PropertyInfo* WINAPI MouseWithinProp();
		static const PropertyInfo* WINAPI ClassProp();
		static const PropertyInfo* WINAPI IDProp();
		static const PropertyInfo* WINAPI SheetProp();
		static const PropertyInfo* WINAPI SelectedProp();
		static const PropertyInfo* WINAPI AlphaProp();
		static const PropertyInfo* WINAPI AnimationProp();
		static const PropertyInfo* WINAPI CursorProp();
		static const PropertyInfo* WINAPI DirectionProp();
		static const PropertyInfo* WINAPI AccessibleProp();
		static const PropertyInfo* WINAPI AccRoleProp();
		static const PropertyInfo* WINAPI AccStateProp();
		static const PropertyInfo* WINAPI AccNameProp();
		static const PropertyInfo* WINAPI AccDescProp();
		static const PropertyInfo* WINAPI AccValueProp();
		static const PropertyInfo* WINAPI AccDefActionProp();
		static const PropertyInfo* WINAPI AccHelpProp();
		static const PropertyInfo* WINAPI AccItemTypeProp();
		static const PropertyInfo* WINAPI AccItemStatusProp();
		static const PropertyInfo* WINAPI ShortcutProp();
		static const PropertyInfo* WINAPI EnabledProp();
		static const PropertyInfo* WINAPI MinSizeProp();
		static const PropertyInfo* WINAPI OverhangProp();
		static const PropertyInfo* WINAPI TooltipProp();
		static const PropertyInfo* WINAPI TooltipMaxWidthProp();
		static const PropertyInfo* WINAPI FontProp();
		static const PropertyInfo* WINAPI WindowActiveProp();
		static const PropertyInfo* WINAPI AbsorbsShortcutProp();
		static const PropertyInfo* WINAPI CompositedTextProp();
		static const PropertyInfo* WINAPI TextGlowSizeProp();
		static const PropertyInfo* WINAPI HighDPIProp();
		static const PropertyInfo* WINAPI DPIProp();
		static const PropertyInfo* WINAPI CustomProp();
		static const PropertyInfo* WINAPI ShadowIntensityProp();
		static const PropertyInfo* WINAPI EdgeHighlightThicknessProp(); // TODO Check when was this added
		static const PropertyInfo* WINAPI EdgeHighlightColorProp(); // TODO Check when was this added
		static const PropertyInfo* WINAPI ScaleFactorProp(); // TODO Check when was this added
		static const PropertyInfo* WINAPI UsesDesktopPerMonitorScalingProp(); // TODO Check when was this added

	protected:
		HGADGET _hgDisplayNode;
		BehaviorStore* _pBehaviorStore;

	private:
		int _iIndex;
		BTreeLookup<const PropertyInfo*, Value*>* _pvmLocal;
		int _iGCSlot;
		int _iGCLPSlot;
		int _iPCTail;
		DeferCycle* _pDeferCycle;
		Internal::ListenerData* _pld;
		Element* _peInitialParent;
		Element* _peLocParent;
		POINT _ptLocPosInLayt;
		SIZE _sizeLocSizeInLayt;
		SIZE _sizeLocLastDSConst;
		SIZE _sizeLocDesiredSize;
		int _dSpecLayoutPos;
		Value* _pvSpecSheet;
		int _dSpecAlpha;
		int _dPVLAnimationState;
		ATOM _atomSpecID;

		struct _BitMap
		{
			bool bLocKeyWithin : 1;
			bool bLocMouseWithin : 1;
			bool bCmpVisible : 1;
			bool bSpecVisible : 1;
			bool bSpecSelected : 1;
			bool bSpecKeyFocused : 1;
			bool bSpecMouseFocused : 1;
			bool bSpecAccessible : 1;
			bool bSpecEnabled : 1;
			bool bHasChildren : 1;
			bool bHasLayout : 1;
			bool bHasBorder : 1;
			bool bHasPadding : 1;
			bool bHasMargin : 1;
			bool bHasContent : 1;
			bool bDefaultCAlign : 1;
			bool bWordWrap : 1;
			bool bHasAnimation : 1;
			bool bDefaultCursor : 1;
			bool bDefaultBorderColor : 1;
			bool bDefaultForeground : 1;
			bool bDefaultFontWeight : 1;
			bool bDefaultFontStyle : 1;
			bool bDefaultFontQuality : 1;
			bool bSelfLayout : 1;
			bool bNeedsDSUpdate : 1;
			bool bDestroyed : 1;
			bool bHosted : 1;
			bool bHasTooltip : 1;
			bool bCompositedText : 1;
			bool bUiaEvents : 1;
			bool bPreserveAlphaChannel : 1;
			UINT fNeedsLayout : 2;
			UINT fSpecActive : 4;
			UINT nSpecDirection : 1;
			UINT nUseDefaultFontSize : 1;
			UINT bHasEdgeHighlight : 1;
			UINT nGetsLayoutCompleteGC : 1;
			UINT fFontSizeInRP : 1;
			UINT nScaleFactor : 10;
			UINT bUsesDesktopPerMonitorScaling : 1;
		};

		_BitMap _fBit;
		int _dSpecFontSize;
		Value* _pvSpecFontFace;
		Value* _pvSpecBackground;

	public:
		DuiAccessible* _pDuiAccessible;

		HGADGET GetDisplayNode();
		int GetIndex();
		bool IsDestroyed();
		bool IsHosted();
		bool IsSelfLayout();
		bool IsBehaviorLayout() const; // TODO Check when was this added
		bool HasChildren();
		bool HasLayout();
		bool HasBorder();
		bool HasPadding();
		bool HasMargin();
		bool HasContent();
		bool IsDefaultCAlign();
		bool IsWordWrap();
		bool HasAnimation();
		bool IsDefaultCursor();
		bool HasEdgeHighlight(); // TODO Check when was this added
		bool HasPVLAnimationState(UINT dState); // TODO Check when was this added
		void GetRenderBorderThickness(RECT* prc); // TODO Check when was this added
		void GetRenderPadding(RECT* prc); // TODO Check when was this added
		void GetRenderMargin(RECT* prc); // TODO Check when was this added
		void GetRenderEdgeHighlightThickness(RECT* prc); // TODO Check when was this added
		void GetRenderMinSize(SIZE* prc); // TODO Check when was this added
		Element* GetParent();
		bool GetVisible();
		int GetWidth();
		int GetHeight();
		DynamicArray<Element*>* GetChildren(Value** ppv);
		int GetX();
		int GetY();
		Layout* GetLayout(Value** ppv);
		int GetLayoutPos();
		const RECT* GetBorderThickness(Value** ppv);
		int GetBorderStyle();
		int GetBorderStdColor();
		const Fill* GetBorderColor(Value** ppv);
		const RECT* GetPadding(Value** ppv);
		const RECT* GetMargin(Value** ppv);
		const POINT* GetLocation(Value** ppv);
		const SIZE* GetExtent(Value** ppv);
		const SIZE* GetDesiredSize();
		int GetForegroundStdColor();
		const Fill* GetForegroundColor(Value** ppv);
		int GetBackgroundStdColor();
		const Fill* GetBackgroundColor(Value** ppv);
		const RECT* GetEdgeHighlightThickness(Value** ppv); // TODO Check when was this added
		const Fill* GetEdgeHighlightColor(Value** ppv); // TODO Check when was this added
		float GetElementScaleFactor(); // TODO Check when was this added
		const WCHAR* GetContentString(Value** ppv);
		HRESULT GetEncodedContentString(WCHAR*, size_t cchString);
		size_t GetEncodedContentStringLength(); // TODO Check when was this added
		const WCHAR* GetFontFace(Value** ppv);
		int GetFontSize();
		int GetFontWeight();
		int GetFontStyle();
		int GetFontQuality();
		bool IsCompositedText();
		int GetTextGlowSize();
		int GetActive();
		int GetContentAlign();
		virtual bool GetKeyFocused();
		bool GetKeyWithin();
		bool GetMouseFocused();
		bool GetMouseWithin();
		const WCHAR* GetClass(Value** ppv);
		ATOM GetID();
		StyleSheet* GetSheet();
		bool GetSelected();
		int GetAlpha();
		bool GetPreserveAlphaChannel() const; // TODO Check when was this added
		void SetPreserveAlphaChannel(bool fPreserveAlpha); // TODO Check when was this added
		int GetAnimation();
		int GetPVLAnimationState(); // TODO Check when was this added
		int GetDirection();
		bool GetAccessible();
		int GetAccRole();
		int GetAccState();
		const WCHAR* GetAccName(Value** ppv);
		const WCHAR* GetAccDesc(Value** ppv);
		const WCHAR* GetAccValue(Value** ppv);
		const WCHAR* GetAccDefAction(Value** ppv);
		const WCHAR* GetAccHelp(Value** ppv);
		const WCHAR* GetAccItemType(Value** ppv);
		const WCHAR* GetAccItemStatus(Value** ppv);
		int GetShortcut();
		bool GetEnabled();
		const SIZE* GetMinSize(Value** ppv);
		bool GetOverhang();
		bool GetTooltip();
		int GetTooltipMaxWidth();
		const WCHAR* GetFont(Value** ppv);
		int GetColorize();
		bool GetWindowActive();
		bool GetAbsorbsShortcut();
		bool GetHighDPI();
		int GetDPI();
		int GetShadowIntensity();

		void SetLayoutCompletionNotify(bool v); // TODO Check when was this added
		void SetPVLAnimationState(int v); // TODO Check when was this added
		HRESULT SetVisible(bool v);
		HRESULT SetWidth(int v);
		HRESULT SetHeight(int v);
		HRESULT SetRelPixWidth(int v); // TODO Check when was this added
		HRESULT SetRelPixHeight(int v); // TODO Check when was this added
		HRESULT SetX(int v);
		HRESULT SetY(int v);
		HRESULT SetLayout(Layout* v);
		HRESULT SetLayoutPos(int v);
		HRESULT SetBorderThickness(int l, int t , int r, int b);
		HRESULT SetBorderStyle(int v);
		HRESULT SetBorderStdColor(int v);
		HRESULT SetBorderColor(COLORREF cr);
		HRESULT SetBorderGradientColor(COLORREF cr0, COLORREF cr1, BYTE dType);
		HRESULT SetPadding(int l, int t, int r, int b);
		HRESULT SetRelPixPadding(int l, int t, int r, int b); // TODO Check when was this added
		HRESULT SetMargin(int l, int t, int r, int b);
		HRESULT SetRelPixMargin(int l, int t, int r, int b); // TODO Check when was this added
		HRESULT SetForegroundStdColor(int v);
		HRESULT SetForegroundColor(COLORREF cr);
		HRESULT SetForegroundColor(COLORREF cr0, COLORREF cr1, BYTE dType);
		HRESULT SetForegroundColor(COLORREF cr0, COLORREF cr1, COLORREF cr2, BYTE dType);
		HRESULT SetBackgroundStdColor(int v);
		HRESULT SetBackgroundColor(const Fill& fill);
		HRESULT SetBackgroundColor(const WCHAR* pszClassName, int iPartId, int iStateId);
		HRESULT SetBackgroundColor(COLORREF cr);
		HRESULT SetBackgroundColor(COLORREF cr0, COLORREF cr1, BYTE dType);
		HRESULT SetBackgroundColor(COLORREF cr0, COLORREF cr1, COLORREF cr2, BYTE dType);
		HRESULT SetContentString(const WCHAR* v);
		HRESULT SetEncodedContentString(const WCHAR* v);
		HRESULT SetContentGraphic(const WCHAR* v, BYTE dBlendMode, UINT dBlendValue);
		HRESULT SetContentGraphic(const WCHAR* v, WORD cxDesired, WORD cyDesired);
		HRESULT SetFontFace(const WCHAR* v);
		HRESULT SetFontSize(int v);
		HRESULT SetFontWeight(int v);
		HRESULT SetFontStyle(int v);
		HRESULT SetFontQuality(int v);
		HRESULT SetActive(int v);
		HRESULT SetCompositedText(bool v);
		HRESULT SetTextGlowSize(int v);
		HRESULT SetContentAlign(int v);
		HRESULT SetClass(const WCHAR* v);
		HRESULT SetID(const WCHAR* v);
		HRESULT SetSheet(StyleSheet* v);
		HRESULT SetSelected(bool v);
		HRESULT SetAlpha(int v);
		HRESULT SetAnimation(int v);
		HRESULT SetStdCursor(int v);
		HRESULT SetCursor(const WCHAR* v);
		HRESULT SetCursorHandle(HCURSOR hCursor);
		HRESULT SetDirection(int v);
		HRESULT SetAccessible(bool v);
		HRESULT SetAccRole(int v);
		HRESULT SetAccState(int v);
		HRESULT SetAccName(const WCHAR* v);
		HRESULT SetAccDesc(const WCHAR* v);
		HRESULT SetAccValue(const WCHAR* v);
		HRESULT SetAccDefAction(const WCHAR* v);
		HRESULT SetAccHelp(const WCHAR* v);
		HRESULT SetAccItemType(const WCHAR* v);
		HRESULT SetAccItemStatus(const WCHAR* v);
		HRESULT SetShortcut(int v);
		HRESULT SetEnabled(bool v);
		HRESULT SetMinSize(int cx, int cy);
		HRESULT SetOverhang(bool v);
		HRESULT SetTooltip(bool v);
		HRESULT SetTooltipMaxWidth(int v);
		HRESULT SetFont(const WCHAR* v);
		HRESULT SetWindowActive(bool v);
		HRESULT SetAbsorbsShortcut(bool v);
		HRESULT SetEdgeHighlightThickness(int l, int t, int r, int b); // TODO Check when was this added
		HRESULT SetEdgeHighlightColor(COLORREF cr); // TODO Check when was this added
		// ReSharper disable once CppHiddenFunction
		WCHAR GetShortcutChar();
		HRESULT SetShadowIntensity(int v);

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		virtual IClassInfo* GetClassInfoW();

		static HRESULT WINAPI Register();
		static HRESULT WINAPI UnRegister(IClassInfo** ppClassInfo);
		virtual HRESULT GetAccessibleImpl(IAccessible** ppAccessible);
		HRESULT QueueDefaultAction();
		virtual HRESULT DefaultAction();
		virtual HRESULT GetUIAElementProvider(REFIID riid, void** ppv);
		virtual HRESULT GetElementProviderImpl(InvokeHelper* pih, ElementProvider** ppprv);
		virtual void HandleUiaDestroyListener();
		virtual void HandleUiaPropertyListener(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
		virtual void HandleUiaPropertyChangingListener(const PropertyInfo* ppi);
		virtual void HandleUiaEventListener(Event* pEvent);
		virtual Element* GetUiaFocusDelegate();

		void SetOverrideScaleFactor(float flscale); // TODO Check when was this added

	protected:
		void _Fill(HDC hDC, DWORD crFill, int left, int top, int right, int bottom, bool fForceOpaque); // TODO Check when was this added

	private:
		class Impl
		{
		};

		Impl* _pImpl;
		HWND _rootWindowForTheming;

		friend class __Element_Check;
	};

	typedef const WCHAR* (Element::*PfnStringVal)(Value**);

	class UILIB_API DECLSPEC_NOVTABLE ElementProxy : public ProviderProxy
	{
	public:
		static ElementProxy* WINAPI Create(Element* pe);

		HRESULT DoMethod(MethodId methodId, va_list args) override;

		ElementProxy(const ElementProxy&) = default;

	protected:
		ElementProxy();

		void Init(Element* pe) override;

		HRESULT Navigate(NavigateDirection direction, IRawElementProviderFragment** ppprv);
		HRESULT GetBoundingRect(UiaRect* prect);
		HRESULT GetProperty(VARIANT* pvar, PROPERTYID propertyId);
		HRESULT GetRuntimeId(SAFEARRAY** ppArray);
		HRESULT SetString(VARIANT* pvar, PfnStringVal pfn);
		HRESULT GetFragmentRoot(IRawElementProviderFragmentRoot** ppprv);
		HRESULT IsPatternSupported(Schema::Pattern pattern, bool* pf);
		HRESULT GetLabel(VARIANT* pvar);
		HRESULT GetContent(VARIANT* pvar, IAccessible* pAccessible);
		HRESULT GetProviderOptions(ProviderOptions* ppo);
		HRESULT GetHwnd(HWND* phwnd);
		HRESULT GetAutomationId(VARIANT* pvar);
		void GetControlType(VARIANT* pvar, IAccessible* pAccessible);
		BOOL _UsesUIAProxies();

	private:
		bool _IsSemanticZoomControl(int id);
		bool _IsWindowHostUsingDoNotStealFocusFlag();
	};

	enum MethodIdTag
	{
		MethodId_Element_GetHWND = 0,
		MethodId_Element_ElementFromPoint = 1,
		MethodId_Element_GetFocus = 2,
		MethodId_Element_AdviseEventAdded = 3,
		MethodId_Element_AdviseEventRemoved = 4,
		MethodId_Element_SetFocus = 5,
		MethodId_Element_GetRoot = 6,
		MethodId_Element_Navigate = 7,
		MethodId_Element_GetBoundingRect = 8,
		MethodId_Element_GetProperty = 9,
		MethodId_Element_IsPatternSupported = 10,
		MethodId_Element_GetRuntimeId = 11,
		MethodId_Element_GetProviderOptions = 12,
		MethodId_Drag_GetIsGrabbed = 13,
		MethodId_Drag_GetDropEffect = 14,
		MethodId_Drag_GetDropEffects = 15,
		MethodId_Drag_GetGrabbedItems = 16,
		MethodId_ExpandCollapse_Expand = 17,
		MethodId_ExpandCollapse_Collapse = 18,
		MethodId_ExpandCollapse_GetExpandCollapseState = 19,
		MethodId_Grid_GetItem = 20,
		MethodId_Grid_GetRowCount = 21,
		MethodId_Grid_GetColumnCount = 22,
		MethodId_GridItem_GetRow = 23,
		MethodId_GridItem_GetColumn = 24,
		MethodId_GridItem_GetContainingGrid = 25,
		MethodId_Invoke_Invoke = 26,
		MethodId_RangeValue_SetValue = 27,
		MethodId_RangeValue_GetValue = 28,
		MethodId_RangeValue_GetIsReadOnly = 29,
		MethodId_RangeValue_GetMaximum = 30,
		MethodId_RangeValue_GetMinimum = 31,
		MethodId_RangeValue_GetLargeChange = 32,
		MethodId_RangeValue_GetSmallChange = 33,
		MethodId_Scroll_Scroll = 34,
		MethodId_Scroll_SetScrollPercent = 35,
		MethodId_Scroll_GetHorizontalScrollPercent = 36,
		MethodId_Scroll_GetVerticalScrollPercent = 37,
		MethodId_Scroll_GetHorizontalViewSize = 38,
		MethodId_Scroll_GetVerticalViewSize = 39,
		MethodId_Scroll_GetHorizontallyScrollable = 40,
		MethodId_Scroll_GetVerticallyScrollable = 41,
		MethodId_ScrollItem_ScrollIntoView = 42,
		MethodId_SelectionItem_Select = 43,
		MethodId_SelectionItem_GetIsSelected = 44,
		MethodId_SelectionItem_GetSelectionContainer = 45,
		MethodId_SelectionItem_AddToSelection = 46,
		MethodId_SelectionItem_RemoveFromSelection = 47,
		MethodId_Selection_GetIsSelectionRequired = 48,
		MethodId_Selection_GetSelection = 49,
		MethodId_Table_GetColumnHeaders = 50,
		MethodId_TableItem_GetColumnHeaders = 51,
		MethodId_Toggle_Toggle = 52,
		MethodId_Toggle_GetToggleState = 53,
		MethodId_Value_SetValue = 54,
		MethodId_Value_GetValue = 55,
		MethodId_Value_GetIsReadOnly = 56,
		MethodId_Window_SetVisualState = 57,
		MethodId_Window_Close = 58,
		MethodId_Window_WaitForInputIdle = 59,
		MethodId_Window_GetCanMaximize = 60,
		MethodId_Window_GetCanMinimize = 61,
		MethodId_Window_GetIsModal = 62,
		MethodId_Window_GetWindowVisualState = 63,
		MethodId_Window_GetWindowInteractionState = 64,
		MethodId_Window_GetIsTopmost = 65,
		MethodId_Count = 66,
	};

	class UILIB_API ElementProvider
		: public RefcountBase
		, public IRawElementProviderSimple
		, public IRawElementProviderFragment
		, public IRawElementProviderAdviseEvents
	{
	public:
		ElementProvider();
		ElementProvider(const ElementProvider&) = delete;

		~ElementProvider() override;

		virtual PfnCreateProxy GetProxyCreator();
		const Element* GetElementKey();
		virtual const volatile Element* GetElement();
		void TossPatternProvider(Schema::Pattern pattern);
		virtual void TossElement();

		static HRESULT WINAPI Create(Element* pe, InvokeHelper* pih, ElementProvider** ppprv);

		//~ Begin IUnknown Interface
		STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
		// ReSharper disable once CppHidingFunction
		STDMETHODIMP_(ULONG) AddRef() override;
		// ReSharper disable once CppHidingFunction
		STDMETHODIMP_(ULONG) Release() override;
		//~ End IUnknown Interface

		//~ Begin IRawElementProviderSimple Interface
		STDMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal) override;
		STDMETHODIMP GetPatternProvider(PATTERNID id, IUnknown** ppunk) override;
		STDMETHODIMP GetPropertyValue(PROPERTYID propertyID, VARIANT* pvar) override;
		STDMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** ppprv) override;
		//~ End IRawElementProviderSimple Interface

		//~ Begin IRawElementProviderFragment Interface
		STDMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** ppprv) override;
		STDMETHODIMP GetRuntimeId(SAFEARRAY** pparray) override;
		STDMETHODIMP get_BoundingRectangle(UiaRect* prect) override;
		STDMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override;
		STDMETHODIMP SetFocus() override;
		STDMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** ppprv) override;
		//~ End IRawElementProviderFragment Interface

		//~ Begin IRawElementProviderAdviseEvents Interface
		STDMETHODIMP AdviseEventAdded(EVENTID id, SAFEARRAY* propertyIds) override;
		STDMETHODIMP AdviseEventRemoved(EVENTID id, SAFEARRAY* propertyIds) override;
		//~ End IRawElementProviderAdviseEvents Interface

		HRESULT DoInvokeArgs(MethodId methodId, PfnCreateProxy pfnCreate, va_list args);

	protected:
		virtual HRESULT Init(Element* pe, InvokeHelper* pih);
		HRESULT DoInvoke(MethodId methodId, ...);

		Element* _pe;
		InvokeHelper* _pih;
		IUnknown* _patternProviders[21];
		RTL_CRITICAL_SECTION _cs;
		bool _fDeleteCS;

	private:
		Element* _peKey;
	};
}
