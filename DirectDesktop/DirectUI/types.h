#pragma once

typedef class UID (WINAPI *UIDPROC)();

class UID
{
public:
	// user defined ctor
	// to enable correct return by outptr sematics
	// https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170#return-values
	UID() : _address(nullptr)
	{
	}

	UID(const BYTE* address) : _address(address)
	{
	}

	UID(UIDPROC proc) : _address(proc()._address)
	{
	}

	const BYTE* _address;
};

inline bool operator==(const UID& lhs, const UID& rhs)
{
	return lhs._address == rhs._address;
}

inline bool operator==(const UID& lhs, const UIDPROC& rhs)
{
	return lhs._address == rhs()._address;
}

typedef int (CALLBACK *PfnUiaLookupId)(AutomationIdentifierType, const GUID*);
typedef LRESULT (CALLBACK *PfnUiaReturnRawElementProvider)(HWND hwnd, WPARAM wParam, LPARAM lParam, IRawElementProviderSimple*);
typedef HRESULT (CALLBACK *PfnUiaHostProviderFromHwnd)(HWND, IRawElementProviderSimple**);
typedef HRESULT (CALLBACK *PfnUiaRaiseAutomationEvent)(IRawElementProviderSimple*, int);
typedef HRESULT (CALLBACK *PfnUiaRaiseAutomationPropertyChangedEvent)(IRawElementProviderSimple*, int, VARIANT, VARIANT);
typedef HRESULT (CALLBACK *PfnUiaRaiseStructureChangedEvent)(IRawElementProviderSimple*, StructureChangeType, int*, int);


//forward declares
namespace DirectUI
{
	template <typename T, typename U>
	struct _IsSame
	{
		static constexpr bool Value = false;
	};

	template <typename T>
	struct _IsSame<T, T>
	{
		static constexpr bool Value = true;
	};

	static_assert(_IsSame<wchar_t, unsigned short>::Value, "Please enable \"Treat WChar_t As Built in Type\" in the project settings");


	typedef class ProviderProxy* (CALLBACK *PfnCreateProxy)(class Element*);
	typedef int MethodId;


	struct ThemeChangedEvent
	{

	};

	/*struct KeyboardEvent
	{

	};*/

	struct CellInfo
	{

	};

	class ElementProvider;
	class InvokeHelper;
	class Layout;
	class Value;
	struct PropertyInfo;

	//// Misc
	//class CritSecLock;
	//class LinkedList;
	//class DeferCycle;
	//class UID;
	//class StyleSheet;
	//class Value;
	//class Expression;
	//class InvokeHelper;
	//class ProviderProxy;
	//enum NavigateDirection;
	//enum ProviderOptions;
	//template<typename T, int> class DynamicArray;
	//template<typename> class SafeArrayAccessor;

 //  // Primitives
	//struct Fill;

	//struct Cursor;

	//struct Graphic;

	//struct LINEINFO;

	//struct XMLParserCond;

	//struct EnumMap;

	//struct UiaRect;

	//struct PropertyInfo;

	//struct DepRecs;

	//struct NavReference;
 //  
	//// Base
 //  class BaseScrollBar;
 //  class BaseScrollViewer;
	//class ClassInfoBase;
	//class RefcountBase;

 //  // Elements
 //  class Element;
	//class ElementWithHWND;
	//class UnknownElement;
 //  class HWNDElement;
	//class DialogElement;
	//class AnimationStrip;
	//class RefPointElement;
 //  class Button;
 //  class AutoButton;
	//class PushButton;
	//class RepeatButton;
	//class Progress;
	//class Page;
	//class Pages;
	//class ScrollBar;
	//class Combobox;
	//class Edit;
	//class Expandable;
	//class Expando;
	//class XElement;

	//// Accessibility
	//class DuiAccessible;
 //  class AccessibleButton;
	//class HWNDElementAccessible;
	//class HWNDHostAccessible;
	//class HWNDHostClientAccessible;


	//// Layouts
	//class Layout;
 //  class BorderLayout;
	//class GridLayout;
	//class FillLayout;
	//class FlowLayout;
	//class RowLayout;
	//class NineGridLayout;
	//class ShellBorderLayout;
	//class TableLayout;
	//class VerticalFlowLayout;
	//
	//// Common Controls
	//class CCAVI;
	//class CCBase;
	//class CCBaseCheckRadioButton;
	//class CCBaseScrollBar;
	//class CCCheckBox;
	//class CCCommandLink;
	//class CCHScrollBar;
	//class CCListBox;
	//class CCListView;
	//class CCProgressBar;
	//class CCPushButton;
	//class CCRadioButton;
	//class CCSysLink;
	//class CCTrackBar;
	//class CCTreeView;
	//class CCVScrollBar;
	//class CCVScrollBar;
	//class CCSysLink;

	//// Browser
	//class Browser;
	//class BrowserSelectionProxy;

 //  // Providers
 //  class ElementProvider;
	//class ExpandCollapseProvider;
	//class GridItemProvider;
	//class GridProvider;
	//class HWNDElementProvider;
	//class InvokeProvider;
 //  template <class X, class Y, int> class PatternProvider;
	//class RangeValueProvider;
	//class ScrollItemProvider;
	//class ScrollProvider;
	//class SelectionItemProvider;
	//class SelectionProvider;
	//class TableItemProvider;
	//class TableProvider;
	//class ToggleProvider;
	//class ValueProvider;
 //  class XProvider;
 //  class XResourceProvider;
 //  
	//// Proxies
	//class Proxy;
	//class ElementProxy;
	//class ExpandCollapseProxy;
	//class GridItemProxy;
	//class HWNDElementProxy;
	//class InvokeProxy;
	//class NavigatorSelectionItemProxy;
	//class ProgressRangeValueProxy;
	//class ProviderProxy;
	//class RangeValueProxy;
	//class ScrollBarRangeValueProxy;
	//class ScrollItemProxy;
	//class ScrollProxy;
	//class SelectionItemProxy;
	//class SelectionProxy;
	//class SelectorSelectionItemProxy;
	//class SelectorSelectionProxy;
	//class TableItemProxy;
	//class TableProxy;
	//class ToggleProxy;
	//class ValueProxy;

	//// Hosts
	//class NativeHWNDHost;
	//class HWNDHost;
	//class XHost;

 //  // Events
	//struct Event;
	//struct InputEvent;
	//struct KeyboardEvent;
	//struct ThemeChangedEvent;
 //  
 //  // Interfaces
	//class IElementListener;
	//struct IClassInfo;
	//class IDataEngine;
	//class IDataEntry;
	//class IProvider;
	//class ISBLeak;
	//class IXProviderCP;
	//class IXElementCP;
	//struct IXmlReader;
	//struct IRawElementProviderFragment;
	//struct IRawElementProviderFragmentRoot;
	//struct IRawElementProviderSimple;

 //  // XML Parser
 //  class DUIXmlParser;
 //  class Schema;
	//enum _DUI_PARSE_STATE;
	//namespace ParserTools
	//{
	//	struct ExprNode;
	//	class ValueParser;
	//}
 //  
	//// Auto Objects
	//class AutoButton;
	//class AutoLock;
	//class AutoThread;
	//class AutoVariant;

 // 
	//
	//// Unknown
 //  class Bind;
	//class CallstackTracker;
	//class CheckBoxGlyph;
	//class Clipper;
	//class DCSurface;
	//class DUIFactory;
	//class ExpandoButtonGlyph;
	//class FontCache;
	//class FontCheckOut;
	//class Movie;
	//class Navigator;
	//class Surface;
	//class Thumb;
	//class Viewer;
	//class InvokeHelper;
	//class Macro;
	//class PText;
	//class RadioButtonGlyph;
	//class Repeater;
	//class ResourceModuleHandles;
	//class ScrollViewer;
	//class Selector;
	//class SelectorNoDefault;
	//class StyledScrollViewer;
	//class TaskPage;
	//class TextGraphic;
	//class XBaby;
}
