#pragma once
#pragma warning(disable:6258)

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    extern int g_dpi, g_dpiOld, g_dpiLaunch;
    extern bool g_atleastonesetting;
    extern DWORD g_animCoef;
    extern DUIXmlParser* parser;
    extern int GetCurrentScaleInterval();
    struct yValue;

    HRESULT WINAPI CreateAndSetLayout(Element* pe, HRESULT (*pfnCreate)(int, int*, Value**), int dNumParams, int* pParams);

    struct RegKeyValue
    {
        HKEY _hKeyName;
        LPCWSTR _path;
        const wchar_t* _valueToFind;
        DWORD _dwValue;
    };

    class DDScalableElement : public Element
    {
    public:
        DDScalableElement()
            : _gc(0)
        {
        }

        virtual ~DDScalableElement();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI DDCPIntensityProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        int GetAssociatedColor();
        int GetDDCPIntensity();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetAssociatedColor(int iAssociatedColor);
        void SetDDCPIntensity(int intensity);
        unsigned short GetGroupColor();
        void SetGroupColor(unsigned short sGC);

    protected:
        unsigned short _gc;
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableButton : public Button
    {
    public:
        DDScalableButton()
            : _rkv{}
            , _assocFn(nullptr)
            , _fnb1(false)
            , _fnb2(false)
            , _fnb3(false)
            , _assocSetting(nullptr)
            , _gc(0)
            , _shellinteraction(false)
        {
        }

        virtual ~DDScalableButton();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI DDCPIntensityProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        int GetAssociatedColor();
        int GetDDCPIntensity();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetAssociatedColor(int iAssociatedColor);
        void SetDDCPIntensity(int intensity);

        RegKeyValue GetRegKeyValue();
        void (*GetAssociatedFn())(bool, bool, bool);
        void* GetAssociatedSetting();
        unsigned short GetGroupColor();
        bool GetShellInteraction();
        void SetRegKeyValue(RegKeyValue rkvNew);
        void SetAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3);
        void SetAssociatedSetting(void* pb);
        void SetGroupColor(unsigned short sGC);
        void SetShellInteraction(bool bShellInteraction);
        void ExecAssociatedFn(void (*pfn)(bool, bool, bool));

    protected:
        RegKeyValue _rkv;
        void (*_assocFn)(bool, bool, bool);
        bool _fnb1;
        bool _fnb2;
        bool _fnb3;
        void* _assocSetting;
        unsigned short _gc;
        bool _shellinteraction;
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableRichText : public RichText
    {
    public:
        DDScalableRichText()
        {
        }

        virtual ~DDScalableRichText();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI DDCPIntensityProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        int GetAssociatedColor();
        int GetDDCPIntensity();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetAssociatedColor(int iAssociatedColor);
        void SetDDCPIntensity(int intensity);

    protected:
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableTouchButton : public TouchButton
    {
    public:
        DDScalableTouchButton()
            : _rkv{}
            , _assocFn(nullptr)
            , _fnb1(false)
            , _fnb2(false)
            , _fnb3(false)
            , _assocSetting(nullptr)
            , _gc(0)
            , _shellinteraction(false)
        {
        }

        virtual ~DDScalableTouchButton();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI DDCPIntensityProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        int GetAssociatedColor();
        int GetDDCPIntensity();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetAssociatedColor(int iAssociatedColor);
        void SetDDCPIntensity(int intensity);

        RegKeyValue GetRegKeyValue();
        void (*GetAssociatedFn())(bool, bool, bool);
        void* GetAssociatedSetting();
        unsigned short GetGroupColor();
        bool GetShellInteraction();
        void SetRegKeyValue(RegKeyValue rkvNew);
        void SetAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3);
        void SetAssociatedSetting(void* pb);
        void SetGroupColor(unsigned short sGC);
        void SetShellInteraction(bool bShellInteraction);
        void ExecAssociatedFn(void (*pfn)(bool, bool, bool));

    protected:
        RegKeyValue _rkv;
        void (*_assocFn)(bool, bool, bool);
        bool _fnb1;
        bool _fnb2;
        bool _fnb3;
        void* _assocSetting;
        unsigned short _gc;
        bool _shellinteraction;
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableTouchEdit final : public Element
    {
    public:
        DDScalableTouchEdit()
            : _peBackground(nullptr)
            , _peEdit(nullptr)
            , _pePreview(nullptr)
        {
        }

        ~DDScalableTouchEdit();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        static HRESULT Register();
        static const PropertyInfo* WINAPI PromptTextProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        const WCHAR* GetPromptText(Value** ppv);
        const WCHAR* GetContentString(Value** ppv);
        bool GetNeedsFontResize();
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetKeyFocus() override;

    private:
        static IClassInfo* s_pClassInfo;
        DDScalableElement* _peBackground;
        TouchEdit2* _peEdit;
        DDScalableElement* _pePreview;
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);
        HRESULT _CreateTEVisual();
    };

    enum LVItemGroupSize
    {
        LVIGS_NORMAL = 0,
        LVIGS_SMALL = 1,
        LVIGS_MEDIUM = 2,
        LVIGS_WIDE = 3,
        LVIGS_LARGE = 4
    };

    enum LVItemTileSize
    {
        LVITS_ICONONLY = 0,
        LVITS_NONE = 1,
        LVITS_DETAILED = 2
    };

    enum LVItemOpenDirState
    {
        LVIODS_NONE = 0,
        LVIODS_FULLSCREEN = 1,
        LVIODS_PINNED = 2
    };

    class LVItemTouchGrid;

    class LVItem final : public DDScalableTouchButton
    {
    public:
        LVItem()
            : _filename{}
            , _simplefilename{}
            , _isDirectory(false)
            , _isGrouped(false)
            , _isHidden(false)
            , _mem_isSelected(false)
            , _isShortcut(false)
            , _colorLock(false)
            , _dragged(false)
            , _refreshable(false)
            , _sfg(false)
            , _flying(false)
            , _moving(false)
            , _hai(false)
            , _xPos(65535)
            , _yPos(65535)
            , _mem_xPos(0)
            , _mem_yPos(0)
            , _page(0)
            , _mem_page(0)
            , _prmem_page(0)
            , _mem_iconsize(0)
            , _itemCount(0)
            , _itemIndex(0)
            , _groupsize(LVIGS_NORMAL)
            , _tilesize(LVITS_NONE)
            , _opendirstate(LVIODS_NONE)
            , _smallPos(1)
            , _touchGrid(nullptr)
            , _peIcon(nullptr)
            , _peShortcutArrow(nullptr)
            , _peText(nullptr)
            , _peCheckbox(nullptr)
            , _childItemss(nullptr)
            , _pels{}
        {
        }

        ~LVItem();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        unsigned short GetInternalXPos();
        unsigned short GetInternalYPos();
        unsigned short GetMemXPos();
        unsigned short GetMemYPos();
        void SetInternalXPos(unsigned short iXPos);
        void SetInternalYPos(unsigned short iYPos);
        void SetMemXPos(unsigned short iXPos);
        void SetMemYPos(unsigned short iYPos);
        wstring GetFilename();
        wstring GetSimpleFilename();
        void SetFilename(const wstring& wsFilename);
        void SetSimpleFilename(const wstring& wsSimpleFilename);
        bool GetDirState();
        bool GetGroupedDirState();
        bool GetHiddenState();
        bool GetMemorySelected();
        bool GetShortcutState();
        bool GetColorLock();
        bool GetDragState();
        bool GetRefreshState();
        bool GetSizedFromGroup();
        bool GetFlying();
        bool GetMoving();
        bool GetHasAdvancedIcon();
        void SetDirState(bool dirState);
        void SetGroupedDirState(bool groupedDirState);
        void SetHiddenState(bool hiddenState);
        void SetMemorySelected(bool mem_isSelectedState);
        void SetShortcutState(bool shortcutState);
        void SetColorLock(bool colorLockState);
        void SetDragState(bool dragstate);
        void SetRefreshState(bool refreshstate);
        void SetSizedFromGroup(bool sfg);
        void SetFlying(bool flying);
        void SetMoving(bool moving);
        void SetHasAdvancedIcon(bool hai);
        unsigned short GetPage();
        unsigned short GetMemPage();
        unsigned short GetPreRefreshMemPage();
        unsigned short GetMemIconSize();
        unsigned short GetItemCount();
        unsigned short GetItemIndex();
        void SetPage(unsigned short pageID);
        void SetMemPage(unsigned short pageID);
        void SetPreRefreshMemPage(unsigned short pageID);
        void SetMemIconSize(unsigned short iconsz);
        void SetItemCount(unsigned short itemCount);
        void SetItemIndex(unsigned short itemIndex);
        LVItemGroupSize GetGroupSize();
        void SetGroupSize(LVItemGroupSize lvigs);
        LVItemTileSize GetTileSize();
        void SetTileSize(LVItemTileSize lvits);
        LVItemOpenDirState GetOpenDirState();
        void SetOpenDirState(LVItemOpenDirState lviods);
        BYTE GetSmallPos();
        void SetSmallPos(BYTE smPos);
        LVItemTouchGrid* GetTouchGrid();
        void SetTouchGrid(LVItemTouchGrid* lvitg);
        void SetTouchGrid(LVItemTouchGrid* lvitg, BYTE index);
        DDScalableElement* GetIcon();
        Element* GetShortcutArrow();
        RichText* GetText();
        TouchButton* GetCheckbox();
        void SetIcon(DDScalableElement* peIcon);
        void SetShortcutArrow(Element* peShortcutArrow);
        void SetText(RichText* peText);
        void SetCheckbox(TouchButton* peCheckbox);
        vector<LVItem*>* GetChildItems();
        void SetChildItems(vector<LVItem*>* vpm);
        void SetListeners(vector<IElementListener*> pels);
        void ClearAllListeners();

    private:
        static IClassInfo* s_pClassInfo;
        wstring _filename{};
        wstring _simplefilename{};
        bool _isDirectory = false;
        bool _isGrouped = false;
        bool _isHidden = false;
        bool _mem_isSelected = false;
        bool _isShortcut = false;
        bool _colorLock = false;
        bool _dragged = false;
        bool _refreshable = false;
        bool _sfg = false;
        bool _flying = false;
        bool _moving = false;
        bool _hai = false;
        unsigned short _xPos = 65535;
        unsigned short _yPos = 65535;
        unsigned short _mem_xPos = 0;
        unsigned short _mem_yPos = 0;
        unsigned short _page{};
        unsigned short _mem_page{};
        unsigned short _prmem_page{};
        unsigned short _mem_iconsize{};
        unsigned short _itemCount{};
        unsigned short _itemIndex{};
        LVItemGroupSize _groupsize = LVIGS_NORMAL;
        LVItemTileSize _tilesize = LVITS_NONE;
        LVItemOpenDirState _opendirstate = LVIODS_NONE;
        BYTE _smallPos = 1;
        LVItemTouchGrid* _touchGrid{};
        DDScalableElement* _peIcon{};
        Element* _peShortcutArrow{};
        RichText* _peText{};
        TouchButton* _peCheckbox{};
        vector<LVItem*>* _childItemss{};
        vector<IElementListener*> _pels;
    };

    class LVItemTouchGrid final
    {
    public:
        LVItemTouchGrid()
            : _items{}
            , _xFirstTile(0)
            , _yFirstTile(0)
            , _maxCount(4)
            , _itemCount(0)
        {
        }

        ~LVItemTouchGrid()
        {
        }

        void Insert(LVItem* lvi);
        void Insert(LVItem* lvi, BYTE index);
        void Erase(BYTE index);
        BYTE GetItemCount();

    private:
        LVItem* _items[4];
        unsigned short _xFirstTile;
        unsigned short _yFirstTile;
        BYTE _maxCount;
        BYTE _itemCount;
        void _RefreshLVItemPositions(BYTE index);
    };

    class DDLVActionButton final : public DDScalableTouchButton
    {
    public:
        DDLVActionButton()
            : _assocItem(nullptr)
        {
        }

        ~DDLVActionButton();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        LVItem* GetAssociatedItem();
        void SetAssociatedItem(LVItem* lvi);

    private:
        static IClassInfo* s_pClassInfo;
        LVItem* _assocItem;
    };

    class DDToggleButton final : public DDScalableTouchButton
    {
    public:
        DDToggleButton()
        {
        }

        ~DDToggleButton()
        {
        }

        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        static const PropertyInfo* WINAPI CheckedStateProp();
        bool GetCheckedState();
        void SetCheckedState(bool bChecked);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDCheckBoxGlyph final : public DDScalableElement
    {
    public:
        DDCheckBoxGlyph()
        {
        }

        ~DDCheckBoxGlyph()
        {
        }

        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        static const PropertyInfo* WINAPI CheckedStateProp();
        bool GetCheckedState();
        void SetCheckedState(bool bChecked);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDCheckBox final : public DDScalableTouchButton
    {
    public:
        DDCheckBox()
            : _peGlyph(nullptr)
            , _peText(nullptr)
        {
        }

        ~DDCheckBox()
        {
        }

        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        static HRESULT Register();
        static const PropertyInfo* WINAPI CheckedStateProp();
        bool GetCheckedState();
        void SetCheckedState(bool bChecked);

    private:
        static IClassInfo* s_pClassInfo;
        DDCheckBoxGlyph* _peGlyph;
        DDScalableElement* _peText;
        HRESULT _CreateCBVisual();
    };

    class DDNumberedButton : public DDScalableTouchButton
    {
    public:
        DDNumberedButton()
            : _id(0)
            , _peLinked(nullptr)
        {
        }

        ~DDNumberedButton()
        {
        }
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        void OnEvent(Event* pEvent) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        void SetNumberID(BYTE id);
        void SetLinkedElement(void* peLinked);

    private:
        static IClassInfo* s_pClassInfo;
        BYTE _id;
        void* _peLinked;
    };

    class DDCombobox final : public DDScalableTouchButton
    {
    public:
        DDCombobox()
            : _selID(0)
            , _selSize(0)
            , _peDropDownGlyph(nullptr)
            , _wndSelectionMenu(nullptr)
            , _peSelectionMenu(nullptr)
            , _tsvSelectionMenu(nullptr)
            , _peHostInner(nullptr)
            , _peSelections{}
        {
        }

        ~DDCombobox();
        enum { MAX_SELECTIONS = 255 };
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        void OnEvent(Event* pEvent) override;
        static UID WINAPI SelectionChange();
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        static HRESULT Register();
        static const PropertyInfo* WINAPI ListMaxHeightProp();
        int GetListMaxHeight();
        void SetListMaxHeight(int iListMaxHeight);
        void InsertSelection(BYTE index, LPCWSTR pszSelectionStr);
        void EraseSelection(BYTE index);
        BYTE GetSelection();
        void SetSelection(BYTE index);
        void ToggleSelectionList(bool fForceHide);

    private:
        static IClassInfo* s_pClassInfo;
        BYTE _selID;
        BYTE _selSize;
        DDScalableRichText* _peDropDownGlyph;
        NativeHWNDHost* _wndSelectionMenu;
        HWNDElement* _peSelectionMenu;
        TouchScrollViewer* _tsvSelectionMenu;
        Element* _peHostInner;
        DDNumberedButton* _peSelections[MAX_SELECTIONS];
        HRESULT _CreateCMBVisual();
    };

    class DDSlider final : public TouchButton
    {
    public:
        DDSlider()
            : _rkv{}
            , _minValue(0)
            , _maxValue(0)
            , _tickValue(0)
            , _currValue(0)
            , _ptBeforeClick{}
            , _ptOnClick{}
            , _assocVal(nullptr)
            , _coef(1)
            , _szFormatted(nullptr)
            , _peTrackBase(nullptr)
            , _peFillBase(nullptr)
            , _peSliderInner(nullptr)
            , _peTrackHolder(nullptr)
            , _peThumb(nullptr)
            , _peTrack(nullptr)
            , _peFill(nullptr)
            , _peThumbInner(nullptr)
            , _peText(nullptr)
        {
        }

        ~DDSlider();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        void OnInput(InputEvent* pInput) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        static HRESULT Register();
        static const PropertyInfo* WINAPI IsVerticalProp();
        static const PropertyInfo* WINAPI TextWidthProp();
        static const PropertyInfo* WINAPI TextHeightProp();
        bool GetIsVertical();
        int GetTextWidth();
        int GetTextHeight();
        void SetIsVertical(bool bIsVertical);
        void SetTextWidth(int iTextWidth);
        void SetTextHeight(int iTextHeight);
        RegKeyValue GetRegKeyValue();
        void SetRegKeyValue(RegKeyValue rkvNew);
        float GetMinValue();
        float GetMaxValue();
        float GetCurrentValue();
        float GetTickValue();
        int* GetAssociatedValue();
        void SetMinValue(float minValue);
        void SetMaxValue(float maxValue);
        void SetCurrentValue(float currValue, bool fExternal);
        void SetTickValue(float tickValue);
        void SetAssociatedValue(int* assocVal, int extValueMultiplier);
        LPCWSTR GetFormattedString();
        void SetFormattedString(LPCWSTR szFormatted);

    private:
        static IClassInfo* s_pClassInfo;
        RegKeyValue _rkv;
        float _minValue;
        float _maxValue;
        float _tickValue;
        float _currValue;
        POINT _ptBeforeClick;
        POINT _ptOnClick;
        int* _assocVal;
        int _coef;
        LPCWSTR _szFormatted;
        TouchButton* _peTrackBase;
        TouchButton* _peFillBase;
        Element* _peSliderInner;
        Element* _peTrackHolder;
        DDScalableTouchButton* _peThumb;
        DDScalableElement* _peTrack;
        DDScalableElement* _peFill;
        DDScalableElement* _peThumbInner;
        DDScalableRichText* _peText;
        int GetPropCommon(const PropertyProcT pPropertyProc);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);
        HRESULT _CreateDDSVisual();
        void _RedrawSlider();
        static void s_AnimateThumb(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    };

    class DDColorPickerButton final : public TouchButton
    {
    public:
        DDColorPickerButton()
            : _assocCR(0)
            , _order(0)
        {
        }

        ~DDColorPickerButton();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        COLORREF GetAssociatedColor();
        BYTE GetOrder();
        void SetAssociatedColor(COLORREF cr);
        void SetOrder(BYTE bOrder);

    private:
        static IClassInfo* s_pClassInfo;
        COLORREF _assocCR;
        BYTE _order;
    };

    class DDColorPicker final : public Element
    {
    public:
        DDColorPicker()
            : _btnX(0)
            , _btnWidth(0)
            , _currentColorID(0)
            , _ptBeforeClick{}
            , _ptOnClick{}
            , _rkv{}
            , _peOverlayHover(nullptr)
            , _peOverlayCheck(nullptr)
            , _rgpeColorButtons{}
            , _targetElems{}
            , _targetBtns{}
            , _targetTouchBtns{}
            , _themeAwareness(false)
        {
        }

        ~DDColorPicker();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        void OnInput(InputEvent* pInput) override;
        void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        static HRESULT Register();
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI ColorIntensityProp();
        static const PropertyInfo* WINAPI DefaultColorProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetColorIntensity();
        int GetDefaultColor();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetColorIntensity(int iColorIntensity);
        void SetDefaultColor(int iDefaultColor);
        RegKeyValue GetRegKeyValue();
        vector<DDScalableElement*> GetTargetElements();
        vector<DDScalableButton*> GetTargetButtons();
        vector<DDScalableTouchButton*> GetTargetTouchButtons();
        bool GetThemeAwareness();
        void SetRegKeyValue(RegKeyValue rkvNew);
        void SetTargetElements(vector<DDScalableElement*> vte);
        void SetTargetButtons(vector<DDScalableButton*> vtb);
        void SetTargetTouchButtons(vector<DDScalableTouchButton*> vttb);
        void SetThemeAwareness(bool ta);

    private:
        static IClassInfo* s_pClassInfo;
        int _btnX;
        int _btnWidth;
        short _currentColorID;
        POINT _ptBeforeClick;
        POINT _ptOnClick;
        RegKeyValue _rkv;
        DDScalableElement* _peOverlayHover;
        DDScalableElement* _peOverlayCheck;
        DDColorPickerButton* _rgpeColorButtons[8];
        vector<DDScalableElement*> _targetElems;
        vector<DDScalableButton*> _targetBtns;
        vector<DDScalableTouchButton*> _targetTouchBtns;
        bool _themeAwareness;
        int GetPropCommon(const PropertyProcT pPropertyProc);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);
        HRESULT _CreateCLRVisual();
        template <typename T>
        void _ColorizeAssociatedItems(vector<T*> vElems);
    };

    class DDTabbedPages final : public Element
    {
    public:
        typedef void(*GenericTabFunction)(Element*);
        DDTabbedPages()
            : _pParser(nullptr)
            , _tsvTabCtrl(nullptr)
            , _peTabCtrl(nullptr)
            , _peTabs{}
            , _tsvPage(nullptr)
            , _peSubUIContainer(nullptr)
            , _pszPageIDs{}
            , _pfnTabs{}
            , _pageID(0)
            , _pageSize(0)
        {
        }

        ~DDTabbedPages();
        enum { MAX_TABPAGES = 32 };
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        void OnInput(InputEvent* pInput) override;
        bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        static HRESULT Register();
        void SetKeyFocus() override;
        void BindParser(DUIXmlParser* pParser);
        void InsertTab(BYTE index, LPCWSTR pszResIDPage, LPCWSTR pszTabLabel, GenericTabFunction ptfn);
        void EraseTab(BYTE index);
        void TraversePage(BYTE index);

    private:
        static IClassInfo* s_pClassInfo;
        DUIXmlParser* _pParser;
        TouchScrollViewer* _tsvTabCtrl;
        Element* _peTabCtrl;
        DDNumberedButton* _peTabs[MAX_TABPAGES];
        TouchScrollViewer* _tsvPage;
        DDScalableElement* _peSubUIContainer;
        LPCWSTR _pszPageIDs[MAX_TABPAGES];
        GenericTabFunction _pfnTabs[MAX_TABPAGES];
        BYTE _pageID;
        BYTE _pageSize;
        vector<Element*> _vecAnimating;
        HRESULT _CreateTPVisual();
        static DWORD WINAPI s_RemoveFromVec(LPVOID lpParam);
    };

    enum DDNotificationType
    {
        DDNT_SUCCESS = 0,
        DDNT_INFO = 1,
        DDNT_WARNING = 2,
        DDNT_ERROR = 3
    };

    class DDNotificationBanner final : public HWNDElement
    {
    public:
        DDNotificationBanner()
            : _wnd(nullptr)
            , _notificationType(DDNT_INFO)
            , _titleStr{}
            , _pDDNB(nullptr)
            , _icon(nullptr)
            , _title(nullptr)
            , _content(nullptr)
            , _peButtonSection(nullptr)
            , _btnCount(0)
        {
        }

        ~DDNotificationBanner();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(HWND hParent, bool fDblBuffer, UINT nCreate, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        NativeHWNDHost* GetWindowHost();
        void CreateBanner(DDNotificationType type, LPCWSTR title, LPCWSTR content, short timeout);
        static void s_RepositionBanners();
        void DestroyBanner(bool* notificationopen, bool manual);
        static void s_DestroyBannerByButton(Element* elem, Event* iev);
        void AppendButton(LPCWSTR szButtonText, void(*pListener)(Element* elem, Event* iev), bool fClose);

    private:
        static IClassInfo* s_pClassInfo;
        NativeHWNDHost* _wnd;
        DDNotificationType _notificationType;
        WCHAR _titleStr[64];
        DDNotificationBanner* _pDDNB;
        DDScalableElement* _icon;
        DDScalableElement* _title;
        DDScalableElement* _content;
        Element* _peButtonSection;
        BYTE _btnCount;
    };
}
