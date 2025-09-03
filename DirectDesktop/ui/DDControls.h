#pragma once
#pragma warning(disable:6258)

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    extern int g_dpi, g_dpiLaunch;
    extern bool g_atleastonesetting;
    extern DWORD g_animCoef;
    extern NativeHWNDHost* subviewwnd;
    extern int GetCurrentScaleInterval();
    struct yValue;

    HRESULT WINAPI CreateAndSetLayout(Element* pe, HRESULT (*pfnCreate)(int, int*, Value**), int dNumParams, int* pParams);
    void SetThumbPosOnClick(Element* elem, Event* iev);
    void SetThumbPosOnDrag(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);

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
        DDScalableElement();
        virtual ~DDScalableElement();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        void SetPropChangeListener(IElementListener* pel);
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI NeedsFontResize2Prop();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI DDCPIntensityProp();
        static const PropertyInfo* WINAPI StopListeningProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        bool GetNeedsFontResize2();
        int GetAssociatedColor();
        int GetDDCPIntensity();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetNeedsFontResize2(bool bNeedsFontResize2);
        void SetAssociatedColor(int iAssociatedColor);
        void SetDDCPIntensity(int intensity);
        unsigned short GetGroupColor();
        void SetGroupColor(unsigned short sGC);
        bool GetStopListening();
        void StopListening();
        void InitDrawImage();
        static void RedrawImages();
        void InitDrawFont();
        static void RedrawFonts();

    protected:
        IElementListener* _pelPropChange{};
        static vector<DDScalableElement*> _arrCreatedElements;
        unsigned short _gc{};
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableButton : public Button
    {
    public:
        DDScalableButton();
        virtual ~DDScalableButton();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        void SetPropChangeListener(IElementListener* pel);
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI NeedsFontResize2Prop();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI DDCPIntensityProp();
        static const PropertyInfo* WINAPI StopListeningProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        bool GetNeedsFontResize2();
        int GetAssociatedColor();
        int GetDDCPIntensity();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetNeedsFontResize2(bool bNeedsFontResize2);
        void SetAssociatedColor(int iAssociatedColor);
        void SetDDCPIntensity(int intensity);
        bool GetStopListening();
        void StopListening();
        void InitDrawImage();
        static void RedrawImages();
        void InitDrawFont();
        static void RedrawFonts();

        RegKeyValue GetRegKeyValue();
        void (*GetAssociatedFn())(bool, bool, bool);
        bool* GetAssociatedBool();
        unsigned short GetGroupColor();
        void SetRegKeyValue(RegKeyValue rkvNew);
        void SetAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3);
        void SetAssociatedBool(bool* pb);
        void SetGroupColor(unsigned short sGC);
        void ExecAssociatedFn(void (*pfn)(bool, bool, bool));

    protected:
        IElementListener* _pelPropChange{};
        static vector<DDScalableButton*> _arrCreatedButtons;
        RegKeyValue _rkv{};
        void (*_assocFn)(bool, bool, bool) = nullptr;
        bool _fnb1 = false;
        bool _fnb2 = false;
        bool _fnb3 = false;
        bool* _assocBool = nullptr;
        unsigned short _gc{};
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableRichText : public RichText
    {
    public:
        DDScalableRichText();
        virtual ~DDScalableRichText();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        void SetPropChangeListener(IElementListener* pel);
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI NeedsFontResize2Prop();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI StopListeningProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        bool GetNeedsFontResize2();
        int GetAssociatedColor();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetNeedsFontResize2(bool bNeedsFontResize2);
        void SetAssociatedColor(int iAssociatedColor);
        bool GetStopListening();
        void StopListening();
        void InitDrawImage();
        static void RedrawImages();
        void InitDrawFont();
        static void RedrawFonts();

    protected:
        IElementListener* _pelPropChange{};
        static vector<DDScalableRichText*> _arrCreatedTexts;
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    //crudely done class
    class DDScalableTouchButton : public TouchButton
    {
    public:
        DDScalableTouchButton();
        virtual ~DDScalableTouchButton();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        void SetPropChangeListener(IElementListener* pel);
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI NeedsFontResize2Prop();
        static const PropertyInfo* WINAPI AssociatedColorProp();
        static const PropertyInfo* WINAPI StopListeningProp();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        bool GetNeedsFontResize2();
        int GetAssociatedColor();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetNeedsFontResize2(bool bNeedsFontResize2);
        void SetAssociatedColor(int iAssociatedColor);
        bool GetStopListening();
        void StopListening();
        void InitDrawImage();
        static void RedrawImages();
        void InitDrawFont();
        static void RedrawFonts();

    protected:
        IElementListener* _pelPropChange{};
        static vector<DDScalableTouchButton*> _arrCreatedTButtons;
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableTouchEdit : public TouchEdit2
    {
    public:
        DDScalableTouchEdit();
        virtual ~DDScalableTouchEdit();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        static const PropertyInfo* WINAPI FirstScaledImageProp();
        static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        static const PropertyInfo* WINAPI DrawTypeProp();
        static const PropertyInfo* WINAPI EnableAccentProp();
        static const PropertyInfo* WINAPI NeedsFontResizeProp();
        static const PropertyInfo* WINAPI NeedsFontResize2Prop();
        int GetFirstScaledImage();
        int GetScaledImageIntervals();
        int GetDrawType();
        bool GetEnableAccent();
        bool GetNeedsFontResize();
        bool GetNeedsFontResize2();
        void SetFirstScaledImage(int iFirstImage);
        void SetScaledImageIntervals(int iScaleIntervals);
        void SetDrawType(int iDrawType);
        void SetEnableAccent(bool bEnableAccent);
        void SetNeedsFontResize(bool bNeedsFontResize);
        void SetNeedsFontResize2(bool bNeedsFontResize2);
        void InitDrawImage();
        static void RedrawImages();
        void InitDrawFont();
        static void RedrawFonts();

    protected:
        static vector<DDScalableTouchEdit*> _arrCreatedBoxes;
        auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
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

    class LVItem final : public DDScalableButton
    {
    public:
        LVItem()
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
        void SetPage(unsigned short pageID);
        void SetMemPage(unsigned short pageID);
        void SetPreRefreshMemPage(unsigned short pageID);
        LVItemGroupSize GetGroupSize();
        void SetGroupSize(LVItemGroupSize lvigs);
        LVItemTileSize GetTileSize();
        void SetTileSize(LVItemTileSize lvits);
        LVItemOpenDirState GetOpenDirState();
        void SetOpenDirState(LVItemOpenDirState lviods);
        vector<LVItem*>* GetChildItems();
        vector<DDScalableElement*>* GetChildIcons();
        vector<Element*>* GetChildShadows();
        vector<Element*>* GetChildShortcutArrows();
        vector<RichText*>* GetChildFilenames();
        void SetChildItems(vector<LVItem*>* vpm);
        void SetChildIcons(vector<DDScalableElement*>* vipm);
        void SetChildShadows(vector<Element*>* vispm);
        void SetChildShortcutArrows(vector<Element*>* vspm);
        void SetChildFilenames(vector<RichText*>* vfpm);
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
        LVItemGroupSize _groupsize = LVIGS_NORMAL;
        LVItemTileSize _tilesize = LVITS_NONE;
        LVItemOpenDirState _opendirstate = LVIODS_NONE;
        vector<LVItem*>* _childItemss{};
        vector<DDScalableElement*>* _childIcons{};
        vector<Element*>* _childShadows{};
        vector<Element*>* _childShortcutArrows{};
        vector<RichText*>* _childFilenames{};
        vector<IElementListener*> _pels;
    };

    class DDLVActionButton final : public DDScalableButton
    {
    public:
        DDLVActionButton()
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
        LVItem* _assocItem{};
    };

    class DDToggleButton final : public DDScalableButton
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

    class DDCheckBox final : public DDScalableButton
    {
    public:
        DDCheckBox()
        {
        }

        ~DDCheckBox()
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

    class DDSlider final : public Button
    {
    public:
        DDSlider()
        {
        }

        ~DDSlider();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
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
        int* GetAssociatedValue();
        int GetDragStart();
        int GetFillOnDragStart();
        int GetPosOnDragStart();
        void SetMinValue(float minValue);
        void SetMaxValue(float maxValue);
        void SetCurrentValue(float currValue, bool fExternal);
        void SetAssociatedValue(int* assocVal, int extValueMultiplier);
        void SetDragStart(int dragStart);
        void SetFillOnDragStart(int fodragStart);
        void SetPosOnDragStart(int podragStart);
        LPCWSTR GetFormattedString();
        void SetFormattedString(LPCWSTR szFormatted);

    private:
        static IClassInfo* s_pClassInfo;
        RegKeyValue _rkv{};
        float _minValue{};
        float _maxValue{};
        float _currValue{};
        int _dragStart{};
        int _fodragStart{};
        int _podragStart{};
        int* _assocVal = nullptr;
        int _coef = 1;
        LPCWSTR _szFormatted = nullptr;
        int GetPropCommon(const PropertyProcT pPropertyProc);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);
    };

    class DDColorPicker final : public Element
    {
    public:
        DDColorPicker()
        {
        }

        ~DDColorPicker();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
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
        bool GetThemeAwareness();
        void SetRegKeyValue(RegKeyValue rkvNew);
        void SetTargetElements(vector<DDScalableElement*> vte);
        void SetTargetButtons(vector<DDScalableButton*> vtb);
        void SetThemeAwareness(bool ta);

    private:
        static IClassInfo* s_pClassInfo;
        RegKeyValue _rkv{};
        vector<DDScalableElement*> _targetElems{};
        vector<DDScalableButton*> _targetBtns{};
        bool _themeAwareness{};
        int GetPropCommon(const PropertyProcT pPropertyProc);
        void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);
    };

    class DDColorPickerButton final : public Button
    {
    public:
        DDColorPickerButton()
        {
        }

        ~DDColorPickerButton();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        void SetPropChangeListener(IElementListener* pel);
        COLORREF GetAssociatedColor();
        BYTE GetOrder();
        vector<DDScalableElement*> GetTargetElements();
        vector<DDScalableButton*> GetTargetButtons();
        void SetAssociatedColor(COLORREF cr);
        void SetOrder(BYTE bOrder);
        void SetTargetElements(vector<DDScalableElement*> vte);
        void SetTargetButtons(vector<DDScalableButton*> vtb);

    private:
        static IClassInfo* s_pClassInfo;
        IElementListener* _pelPropChange{};
        COLORREF _assocCR{};
        BYTE _order{};
        vector<DDScalableElement*> _targetElems{};
        vector<DDScalableButton*> _targetBtns{};
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
        {
        }

        ~DDNotificationBanner();
        static IClassInfo* GetClassInfoPtr();
        static void SetClassInfoPtr(IClassInfo* pClass);
        IClassInfo* GetClassInfoW() override;
        static HRESULT Create(HWND hParent, bool fDblBuffer, UINT nCreate, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        static HRESULT Register();
        Element* GetIconElement();
        DDScalableElement* GetTitleElement();
        DDScalableElement* GetContentElement();
        static void CreateBanner(DDNotificationBanner* pDDNB, DUIXmlParser* pParser, DDNotificationType type, LPCWSTR pszResID, LPCWSTR title, LPCWSTR content, short timeout, bool fClose);
        static void DestroyBanner(bool* notificationopen, NativeHWNDHost* wnd);

    private:
        static IClassInfo* s_pClassInfo;
        DDNotificationType _notificationType{};
        RichText* _icon{};
        wstring _titleStr{};
        DDScalableElement* _title{};
        wstring _contentStr{};
        DDScalableElement* _content{};
    };

    void RedrawImageCore(DDScalableElement* pe);
    void RedrawFontCore(DDScalableElement* pe);
    void ShowHoverCircle(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
}
