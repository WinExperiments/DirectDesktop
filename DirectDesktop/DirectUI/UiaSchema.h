#pragma once

namespace DirectUI
{
	typedef HRESULT (CALLBACK *PfnCreate)(ElementProvider*, IUnknown** ppunk);
	typedef bool (CALLBACK *PfnIsPatternSupported)(Element*);

	class Schema
	{
	public:
		enum Pattern
		{
			First = 0,
			Invoke = 0,
			ExpandCollapse = 1,
			GridItem = 2,
			Grid = 3,
			RangeValue = 4,
			Scroll = 5,
			ScrollItem = 6,
			SelectionItem = 7,
			Selection = 8,
			Table = 9,
			TableItem = 10,
			Toggle = 11,
			Value = 12,
			Dock = 13,
			Drag = 14,
			MultipleView = 15,
			Text = 16,
			Transform = 17,
			Window = 18,
			ItemContainer = 19,
			VirtualizedItem = 20,
			PatternCount = 21
		};

		enum Event
		{
			FirstEvent = 0,
			ToolTipOpened = 0,
			ToolTipClosed = 1,
			StructureChanged = 2,
			MenuOpened = 3,
			AutomationPropertyChanged = 4,
			AutomationFocusChanged = 5,
			AsyncContentLoaded = 6,
			DragDragStart = 7,
			DragDragComplete = 8,
			DragDragCancel = 9,
			MenuClosed = 10,
			InvokeInvoked = 11,
			SelectionItemElementAddedToSelection = 12,
			SelectionItemElementRemovedFromSelection = 13,
			SelectionItemElementSelected = 14,
			SelectionInvalidated = 15,
			TextTextSelectionChanged = 16,
			WindowWindowOpened = 17,
			WindowWindowClosed = 18,
			LayoutInvalidated = 19,
			SystemAlert = 20,
			EventCount = 21
		};

		UILIB_API static HRESULT WINAPI Init();
		UILIB_API static int LookupAccessibleRole(int, bool*);
		UILIB_API static HRESULT WINAPI CreatePatternProvider(Pattern pattern, ElementProvider* pprv, IUnknown** ppunk);
		UILIB_API static Pattern WINAPI PatternFromPatternId(int id);
		UILIB_API static Event WINAPI EventFromEventId(int id);
		UILIB_API static PfnIsPatternSupported PfnIsSupportedFromPattern(Pattern pattern);

		UILIB_API static PfnUiaLookupId UiaLookupId;
		UILIB_API static PfnUiaReturnRawElementProvider UiaReturnRawElementProvider;
		UILIB_API static PfnUiaRaiseAutomationPropertyChangedEvent UiaRaiseAutomationPropertyChangedEvent;
		UILIB_API static PfnUiaRaiseAutomationEvent UiaRaiseAutomationEvent;
		UILIB_API static PfnUiaRaiseStructureChangedEvent UiaRaiseStructureChangedEvent;
		UILIB_API static PfnUiaHostProviderFromHwnd UiaHostProviderFromHwnd;

		UILIB_API static int RuntimeIdProperty;
		UILIB_API static int BoundingRectangleProperty;
		UILIB_API static int ProcessIdProperty;
		UILIB_API static int ControlTypeProperty;
		UILIB_API static int LocalizedControlTypeProperty;
		UILIB_API static int NameProperty;
		UILIB_API static int AcceleratorKeyProperty;
		UILIB_API static int AccessKeyProperty;
		UILIB_API static int HasKeyboardFocusProperty;
		UILIB_API static int IsKeyboardFocusableProperty;
		UILIB_API static int IsEnabledProperty;
		UILIB_API static int AutomationIdProperty;
		UILIB_API static int ClassNameProperty;
		UILIB_API static int HelpTextProperty;
		UILIB_API static int ClickablePointProperty;
		UILIB_API static int CultureProperty;
		UILIB_API static int IsControlElementProperty;
		UILIB_API static int IsContentElementProperty;
		UILIB_API static int LabeledByProperty;
		UILIB_API static int IsPasswordProperty;
		UILIB_API static int NewNativeWindowHandleProperty;
		UILIB_API static int ItemTypeProperty;
		UILIB_API static int ItemStatusProperty;
		UILIB_API static int IsOffscreen;
		UILIB_API static int Orientation;
		UILIB_API static int FrameworkId;
		UILIB_API static int IsPeripheral;
		UILIB_API static int Drag_IsGrabbed_Property;
		UILIB_API static int Drag_DropEffect_Property;
		UILIB_API static int Drag_DropEffects_Property;
		UILIB_API static int ExpandCollapse_ExpandCollapseState_Property;
		UILIB_API static int Grid_ColumnCount_Property;
		UILIB_API static int Grid_RowCount_Property;
		UILIB_API static int GridItem_Column_Property;
		UILIB_API static int GridItem_ColumnSpan_Property;
		UILIB_API static int GridItem_Parent_Property;
		UILIB_API static int GridItem_Row_Property;
		UILIB_API static int GridItem_RowSpan_Property;
		UILIB_API static int RangeValue_IsReadOnly_Property;
		UILIB_API static int RangeValue_LargeChange_Property;
		UILIB_API static int RangeValue_Maximum_Property;
		UILIB_API static int RangeValue_Minimum_Property;
		UILIB_API static int RangeValue_SmallChange_Property;
		UILIB_API static int RangeValue_Value_Property;
		UILIB_API static int Scroll_HorizontalScrollPercent_Property;
		UILIB_API static int Scroll_HorizontallyScrollable_Property;
		UILIB_API static int Scroll_HorizontalViewSize_Property;
		UILIB_API static int Scroll_VerticalScrollPercent_Property;
		UILIB_API static int Scroll_VerticallyScrollable_Property;
		UILIB_API static int Scroll_VerticalViewSize_Property;
		UILIB_API static int Selection_CanSelectMultiple_Property;
		UILIB_API static int Selection_IsSelectionRequired_Property;
		UILIB_API static int Selection_Selection_Property;
		UILIB_API static int SelectionItem_IsSelected_Property;
		UILIB_API static int SelectionItem_SelectionContainer_Property;
		UILIB_API static int Table_ColumnHeaders_Property;
		UILIB_API static int Table_RowHeaders_Property;
		UILIB_API static int Table_RowOrColumnMajor_Property;
		UILIB_API static int TableItem_RowHeaderItems_Property;
		UILIB_API static int TableItem_ColumnHeaderItems_Property;
		UILIB_API static int Toggle_ToggleState_Property;
		UILIB_API static int Value_Value_Property;
		UILIB_API static int Value_IsReadOnly_Property;
		UILIB_API static int NullControlType;
		UILIB_API static int ButtonControlType;
		UILIB_API static int CalendarControlType;
		UILIB_API static int CheckBoxControlType;
		UILIB_API static int ComboBoxControlType;
		UILIB_API static int CustomControlType;
		UILIB_API static int DataGridControlType;
		UILIB_API static int DataItemControlType;
		UILIB_API static int DocumentControlType;
		UILIB_API static int EditControlType;
		UILIB_API static int GroupControlType;
		UILIB_API static int HeaderControlType;
		UILIB_API static int HeaderItemControlType;
		UILIB_API static int HyperlinkControlType;
		UILIB_API static int ImageControlType;
		UILIB_API static int ListControlType;
		UILIB_API static int ListItemControlType;
		UILIB_API static int MenuBarControlType;
		UILIB_API static int MenuControlType;
		UILIB_API static int MenuItemControlType;
		UILIB_API static int PaneControlType;
		UILIB_API static int ProgressBarControlType;
		UILIB_API static int RadioButtonControlType;
		UILIB_API static int ScrollBarControlType;
		UILIB_API static int SeparatorControlType;
		UILIB_API static int SliderControlType;
		UILIB_API static int SpinnerControlType;
		UILIB_API static int SplitButtonControlType;
		UILIB_API static int StatusBarControlType;
		UILIB_API static int TabControlType;
		UILIB_API static int TabItemControlType;
		UILIB_API static int TableControlType;
		UILIB_API static int TextControlType;
		UILIB_API static int ThumbControlType;
		UILIB_API static int TitleBarControlType;
		UILIB_API static int ToolBarControlType;
		UILIB_API static int ToolTipControlType;
		UILIB_API static int TreeControlType;
		UILIB_API static int TreeItemControlType;
		UILIB_API static int WindowControlType;
		UILIB_API static int SemanticZoomControlType;
		UILIB_API static int InvokePattern;
		UILIB_API static int ExpandCollapsePattern;
		UILIB_API static int GridItemPattern;
		UILIB_API static int GridPattern;
		UILIB_API static int RangeValuePattern;
		UILIB_API static int ScrollPattern;
		UILIB_API static int ScrollItemPattern;
		UILIB_API static int SelectionItemPattern;
		UILIB_API static int SelectionPattern;
		UILIB_API static int TablePattern;
		UILIB_API static int TableItemPattern;
		UILIB_API static int TogglePattern;
		UILIB_API static int ValuePattern;
		UILIB_API static int DockPattern;
		UILIB_API static int DragPattern;
		UILIB_API static int MultipleViewPattern;
		UILIB_API static int TextPattern;
		UILIB_API static int TransformPattern;
		UILIB_API static int WindowPattern;
		UILIB_API static int ItemContainerPattern;
		UILIB_API static int VirtualizedItemPattern;
		UILIB_API static int ToolTipOpenedEvent;
		UILIB_API static int ToolTipClosedEvent;
		UILIB_API static int StructureChangedEvent;
		UILIB_API static int MenuOpenedEvent;
		UILIB_API static int AutomationPropertyChangedEvent;
		UILIB_API static int AutomationFocusChangedEvent;
		UILIB_API static int AsyncContentLoadedEvent;
		UILIB_API static int DragDragStartEvent;
		UILIB_API static int DragDragCompleteEvent;
		UILIB_API static int DragDragCancelEvent;
		UILIB_API static int MenuClosedEvent;
		UILIB_API static int InvokeInvokedEvent;
		UILIB_API static int SelectionItemElementAddedToSelectionEvent;
		UILIB_API static int SelectionItemElementRemovedFromSelectionEvent;
		UILIB_API static int SelectionItemElementSelectedEvent;
		UILIB_API static int SelectionInvalidatedEvent;
		UILIB_API static int TextTextSelectionChangedEvent;
		UILIB_API static int WindowWindowOpenedEvent;
		UILIB_API static int WindowWindowClosedEvent;
		UILIB_API static int LayoutInvalidatedEvent;
		UILIB_API static int SystemAlertEvent;

	private:
		UILIB_API static HRESULT WINAPI GetProcs();

		struct PropertyInfo
		{
			int* _pPropertyId;
			const GUID* _pguid;
		};

		static const PropertyInfo g_propertyInfoTable[];
		UILIB_API static HRESULT WINAPI LookupPropertyInfos();

		struct ControlInfo
		{
			int* _pControlTypeId;
			const GUID* _pguid;
		};

		static const ControlInfo g_controlInfoTable[];
		UILIB_API static HRESULT WINAPI LookupControlInfos();

		struct PatternInfo
		{
			int* _pPatternId;
			const GUID* _pguid;
		};

		static const PatternInfo g_patternInfoTable[];
		UILIB_API static HRESULT WINAPI LookupPatternInfos();

		struct EventInfo
		{
			int* _pEventId;
			const GUID* _pguid;
		};

		static const EventInfo g_eventInfoTable[];
		UILIB_API static HRESULT WINAPI LookupEventInfos();

		struct RoleMap
		{
			int _accRole;
			bool _fIsContentElement;
			const int* _pid;
		};

		static const RoleMap _roleMapping[];

		struct PatternMap
		{
			const int* _pid;
			PfnCreate _pfnCreate;
			PfnIsPatternSupported _pfnSupported;
		};

		static const PatternMap g_patternMapping[];

		struct EventMap
		{
			const int* _pid;
		};

		static const EventMap g_eventMapping[];
		static bool g_fInited;
	};
}
