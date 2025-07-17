#pragma once
#pragma warning(disable:6258)

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
    extern int g_dpi, g_dpiLaunch;
    extern NativeHWNDHost* subviewwnd;
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
        int GetDDCPIntensity();
        unsigned short GetGroupColor();
        void SetDDCPIntensity(int intensity);
        void SetGroupColor(unsigned short sGC);
        void InitDrawImage();
        static void RedrawImages();
        void InitDrawFont();
        static void RedrawFonts();

    protected:
        IElementListener* _pelPropChange{};
        static vector<DDScalableElement*> _arrCreatedElements;
        int _intensity = 255;
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
        void InitDrawImage();
        static void RedrawImages();
        void InitDrawFont();
        static void RedrawFonts();

        RegKeyValue GetRegKeyValue();
        void (* GetAssociatedFn())(bool, bool, bool);
        bool* GetAssociatedBool();
        int GetDDCPIntensity();
        void SetRegKeyValue(RegKeyValue rkvNew);
        void SetAssociatedFn(void (*pfn)(bool, bool, bool));
        void SetAssociatedBool(bool* pb);
        void SetDDCPIntensity(int intensity);
        void ExecAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3);

    protected:
        IElementListener* _pelPropChange{};
        static vector<DDScalableButton*> _arrCreatedButtons;
        RegKeyValue _rkv{};
        void (*_assocFn)(bool, bool, bool) = nullptr;
        bool* _assocBool = nullptr;
        int _intensity = 255;
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
        void SetInternalXPos(unsigned short iXPos);
        void SetInternalYPos(unsigned short iYPos);
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
        void SetDirState(bool dirState);
        void SetGroupedDirState(bool groupedDirState);
        void SetHiddenState(bool hiddenState);
        void SetMemorySelected(bool mem_isSelectedState);
        void SetShortcutState(bool shortcutState);
        void SetColorLock(bool colorLockState);
        void SetDragState(bool dragstate);
        void SetRefreshState(bool refreshstate);
        void SetSizedFromGroup(bool sfg);
        unsigned short GetPage();
        void SetPage(unsigned short pageID);
        LVItemGroupSize GetGroupSize();
        void SetGroupSize(LVItemGroupSize lvigs);
        vector<LVItem*> GetChildItems();
        vector<DDScalableElement*> GetChildIcons();
        vector<Element*> GetChildShadows();
        vector<Element*> GetChildShortcutArrows();
        vector<RichText*> GetChildFilenames();
        void SetChildItems(vector<LVItem*> vpm);
        void SetChildIcons(vector<DDScalableElement*> vipm);
        void SetChildShadows(vector<Element*> vispm);
        void SetChildShortcutArrows(vector<Element*> vspm);
        void SetChildFilenames(vector<RichText*> vfpm);
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
        unsigned short _xPos = 65535;
        unsigned short _yPos = 65535;
        unsigned short _page{};
        LVItemGroupSize _groupsize = LVIGS_NORMAL;
        vector<LVItem*> _childItemss;
        vector<DDScalableElement*> _childIcons;
        vector<Element*> _childShadows;
        vector<Element*> _childShortcutArrows;
        vector<RichText*> _childFilenames;
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
        bool GetThemeAwareness();
        void SetRegKeyValue(RegKeyValue rkvNew);
        void SetTargetElements(vector<DDScalableElement*> vte);
        void SetThemeAwareness(bool ta);

    private:
        static IClassInfo* s_pClassInfo;
        RegKeyValue _rkv{};
        vector<DDScalableElement*> _targetElems{};
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
        void SetAssociatedColor(COLORREF cr);
        void SetOrder(BYTE bOrder);
        void SetTargetElements(vector<DDScalableElement*> vte);

    private:
        static IClassInfo* s_pClassInfo;
        IElementListener* _pelPropChange{};
        COLORREF _assocCR{};
        BYTE _order{};
        vector<DDScalableElement*> _targetElems{};
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
        static void DestroyBanner(bool* notificationopen);

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
