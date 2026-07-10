#pragma once
#pragma warning(disable:6258)

#ifdef DDUI_EXPORTS
#define DDUIAPI __declspec(dllexport)
#else
#define DDUIAPI __declspec(dllimport)
#endif

#include "common.h"
#include "SettingsHelper.h"
#include "coreui\AnimationHelper.h"
#include <vector>
#include <string>

using namespace std;
using namespace DirectUI;

namespace DDUI
{
    extern HMODULE g_hmod;
    extern DDUICtx g_ctx;
    extern HWND g_msgwnd;
    extern DUIXmlParser* g_parser;

    HRESULT WINAPI CreateAndSetLayout(Element* pe, HRESULT (*pfnCreate)(int, int*, Value**), int dNumParams, int* pParams);
    
    template <typename T>
    DDUIAPI void RedrawBorderCore(T* pe);

    class DDScalableElement : public Element
    {
    public:
        DDUIAPI DDScalableElement()
            : _gc(0)
        {
        }

        DDUIAPI virtual ~DDScalableElement();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI FirstScaledImageProp();
        DDUIAPI static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageCountProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageIndexProp();
        DDUIAPI static const PropertyInfo* WINAPI DrawTypeProp();
        DDUIAPI static const PropertyInfo* WINAPI EnableAccentProp();
        DDUIAPI static const PropertyInfo* WINAPI NeedsFontResizeProp();
        DDUIAPI static const PropertyInfo* WINAPI AssociatedColorProp();
        DDUIAPI static const PropertyInfo* WINAPI DDCPIntensityProp();
        DDUIAPI static const PropertyInfo* WINAPI BorderRadiusProp();
        DDUIAPI int GetFirstScaledImage();
        DDUIAPI int GetScaledImageIntervals();
        DDUIAPI int GetImageCount();
        DDUIAPI int GetImageIndex();
        DDUIAPI int GetDrawType();
        DDUIAPI bool GetEnableAccent();
        DDUIAPI bool GetNeedsFontResize();
        DDUIAPI COLORREF GetAssociatedColor();
        DDUIAPI int GetDDCPIntensity();
        DDUIAPI const RECT* GetBorderRadius(Value** ppv);
        DDUIAPI void SetFirstScaledImage(int iFirstImage);
        DDUIAPI void SetScaledImageIntervals(int iScaleIntervals);
        DDUIAPI void SetImageCount(int iImageCount);
        DDUIAPI void SetImageIndex(int iImageIndex);
        DDUIAPI void SetDrawType(int iDrawType);
        DDUIAPI void SetEnableAccent(bool bEnableAccent);
        DDUIAPI void SetNeedsFontResize(bool bNeedsFontResize);
        DDUIAPI void SetAssociatedColor(COLORREF crAssociatedColor);
        DDUIAPI void SetDDCPIntensity(int intensity);
        DDUIAPI void SetBorderRadius(int l, int t, int r, int b);
        DDUIAPI unsigned short GetGroupColor();
        DDUIAPI void SetGroupColor(unsigned short sGC);

    protected:
        unsigned short _gc;
        DDUIAPI auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        DDUIAPI void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableButton : public Button
    {
    public:
        DDUIAPI DDScalableButton()
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

        DDUIAPI virtual ~DDScalableButton();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI FirstScaledImageProp();
        DDUIAPI static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageCountProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageIndexProp();
        DDUIAPI static const PropertyInfo* WINAPI DrawTypeProp();
        DDUIAPI static const PropertyInfo* WINAPI EnableAccentProp();
        DDUIAPI static const PropertyInfo* WINAPI NeedsFontResizeProp();
        DDUIAPI static const PropertyInfo* WINAPI AssociatedColorProp();
        DDUIAPI static const PropertyInfo* WINAPI DDCPIntensityProp();
        DDUIAPI static const PropertyInfo* WINAPI BorderRadiusProp();
        DDUIAPI int GetFirstScaledImage();
        DDUIAPI int GetScaledImageIntervals();
        DDUIAPI int GetImageCount();
        DDUIAPI int GetImageIndex();
        DDUIAPI int GetDrawType();
        DDUIAPI bool GetEnableAccent();
        DDUIAPI bool GetNeedsFontResize();
        DDUIAPI COLORREF GetAssociatedColor();
        DDUIAPI int GetDDCPIntensity();
        DDUIAPI const RECT* GetBorderRadius(Value** ppv);
        DDUIAPI void SetFirstScaledImage(int iFirstImage);
        DDUIAPI void SetScaledImageIntervals(int iScaleIntervals);
        DDUIAPI void SetImageCount(int iImageCount);
        DDUIAPI void SetImageIndex(int iImageIndex);
        DDUIAPI void SetDrawType(int iDrawType);
        DDUIAPI void SetEnableAccent(bool bEnableAccent);
        DDUIAPI void SetNeedsFontResize(bool bNeedsFontResize);
        DDUIAPI void SetAssociatedColor(COLORREF crAssociatedColor);
        DDUIAPI void SetDDCPIntensity(int intensity);
        DDUIAPI void SetBorderRadius(int l, int t, int r, int b);

        DDUIAPI RegKeyValue GetRegKeyValue();
        DDUIAPI void (*GetAssociatedFn())(bool, bool, bool);
        DDUIAPI void* GetAssociatedSetting();
        DDUIAPI unsigned short GetGroupColor();
        DDUIAPI bool GetShellInteraction();
        DDUIAPI void SetRegKeyValue(RegKeyValue rkvNew);
        DDUIAPI void SetAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3);
        DDUIAPI void SetAssociatedSetting(void* pb);
        DDUIAPI void SetGroupColor(unsigned short sGC);
        DDUIAPI void SetShellInteraction(bool bShellInteraction);
        DDUIAPI void ExecAssociatedFn(void (*pfn)(bool, bool, bool));

    protected:
        RegKeyValue _rkv;
        void (*_assocFn)(bool, bool, bool);
        bool _fnb1;
        bool _fnb2;
        bool _fnb3;
        void* _assocSetting;
        unsigned short _gc;
        bool _shellinteraction;
        DDUIAPI auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        DDUIAPI void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableRichText : public RichText
    {
    public:
        DDUIAPI DDScalableRichText()
        {
        }

        DDUIAPI virtual ~DDScalableRichText();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI FirstScaledImageProp();
        DDUIAPI static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageCountProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageIndexProp();
        DDUIAPI static const PropertyInfo* WINAPI DrawTypeProp();
        DDUIAPI static const PropertyInfo* WINAPI EnableAccentProp();
        DDUIAPI static const PropertyInfo* WINAPI NeedsFontResizeProp();
        DDUIAPI static const PropertyInfo* WINAPI AssociatedColorProp();
        DDUIAPI static const PropertyInfo* WINAPI DDCPIntensityProp();
        DDUIAPI static const PropertyInfo* WINAPI BorderRadiusProp();
        DDUIAPI int GetFirstScaledImage();
        DDUIAPI int GetScaledImageIntervals();
        DDUIAPI int GetImageCount();
        DDUIAPI int GetImageIndex();
        DDUIAPI int GetDrawType();
        DDUIAPI bool GetEnableAccent();
        DDUIAPI bool GetNeedsFontResize();
        DDUIAPI COLORREF GetAssociatedColor();
        DDUIAPI int GetDDCPIntensity();
        DDUIAPI const RECT* GetBorderRadius(Value** ppv);
        DDUIAPI void SetFirstScaledImage(int iFirstImage);
        DDUIAPI void SetScaledImageIntervals(int iScaleIntervals);
        DDUIAPI void SetImageCount(int iImageCount);
        DDUIAPI void SetImageIndex(int iImageIndex);
        DDUIAPI void SetDrawType(int iDrawType);
        DDUIAPI void SetEnableAccent(bool bEnableAccent);
        DDUIAPI void SetNeedsFontResize(bool bNeedsFontResize);
        DDUIAPI void SetAssociatedColor(COLORREF crAssociatedColor);
        DDUIAPI void SetDDCPIntensity(int intensity);
        DDUIAPI void SetBorderRadius(int l, int t, int r, int b);

    protected:
        DDUIAPI auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        DDUIAPI void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableTouchButton : public TouchButton
    {
    public:
        DDUIAPI DDScalableTouchButton()
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

        DDUIAPI virtual ~DDScalableTouchButton();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI FirstScaledImageProp();
        DDUIAPI static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageCountProp();
        DDUIAPI static const PropertyInfo* WINAPI ImageIndexProp();
        DDUIAPI static const PropertyInfo* WINAPI DrawTypeProp();
        DDUIAPI static const PropertyInfo* WINAPI EnableAccentProp();
        DDUIAPI static const PropertyInfo* WINAPI NeedsFontResizeProp();
        DDUIAPI static const PropertyInfo* WINAPI AssociatedColorProp();
        DDUIAPI static const PropertyInfo* WINAPI DDCPIntensityProp();
        DDUIAPI static const PropertyInfo* WINAPI BorderRadiusProp();
        DDUIAPI int GetFirstScaledImage();
        DDUIAPI int GetScaledImageIntervals();
        DDUIAPI int GetImageCount();
        DDUIAPI int GetImageIndex();
        DDUIAPI int GetDrawType();
        DDUIAPI bool GetEnableAccent();
        DDUIAPI bool GetNeedsFontResize();
        DDUIAPI COLORREF GetAssociatedColor();
        DDUIAPI int GetDDCPIntensity();
        DDUIAPI const RECT* GetBorderRadius(Value** ppv);
        DDUIAPI void SetFirstScaledImage(int iFirstImage);
        DDUIAPI void SetScaledImageIntervals(int iScaleIntervals);
        DDUIAPI void SetImageCount(int iImageCount);
        DDUIAPI void SetImageIndex(int iImageIndex);
        DDUIAPI void SetDrawType(int iDrawType);
        DDUIAPI void SetEnableAccent(bool bEnableAccent);
        DDUIAPI void SetNeedsFontResize(bool bNeedsFontResize);
        DDUIAPI void SetAssociatedColor(COLORREF crAssociatedColor);
        DDUIAPI void SetDDCPIntensity(int intensity);
        DDUIAPI void SetBorderRadius(int l, int t, int r, int b);

        DDUIAPI RegKeyValue GetRegKeyValue();
        DDUIAPI void (*GetAssociatedFn())(bool, bool, bool);
        DDUIAPI void* GetAssociatedSetting();
        DDUIAPI unsigned short GetGroupColor();
        DDUIAPI bool GetShellInteraction();
        DDUIAPI void SetRegKeyValue(RegKeyValue rkvNew);
        DDUIAPI void SetAssociatedFn(void (*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3);
        DDUIAPI void SetAssociatedSetting(void* pb);
        DDUIAPI void SetGroupColor(unsigned short sGC);
        DDUIAPI void SetShellInteraction(bool bShellInteraction);
        DDUIAPI void ExecAssociatedFn(void (*pfn)(bool, bool, bool));

    protected:
        RegKeyValue _rkv;
        void (*_assocFn)(bool, bool, bool);
        bool _fnb1;
        bool _fnb2;
        bool _fnb3;
        void* _assocSetting;
        unsigned short _gc;
        bool _shellinteraction;
        DDUIAPI auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        DDUIAPI void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDScalableTouchEdit final : public Element
    {
    public:
        DDUIAPI DDScalableTouchEdit()
            : _peBackground(nullptr)
            , _peEdit(nullptr)
            , _pePreview(nullptr)
        {
        }

        DDUIAPI ~DDScalableTouchEdit();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI PromptTextProp();
        DDUIAPI static const PropertyInfo* WINAPI NeedsFontResizeProp();
        DDUIAPI const WCHAR* GetPromptText(Value** ppv);
        DDUIAPI const WCHAR* GetContentString(Value** ppv);
        DDUIAPI bool GetNeedsFontResize();
        DDUIAPI void SetNeedsFontResize(bool bNeedsFontResize);
        DDUIAPI void SetKeyFocus() override;

    private:
        static IClassInfo* s_pClassInfo;
        DDScalableElement* _peBackground;
        TouchEdit2* _peEdit;
        DDScalableElement* _pePreview;
        DDUIAPI auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
        DDUIAPI void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);
        DDUIAPI HRESULT _CreateTEVisual();
    };

    enum LVCommonFlags : DWORD
    {
        LVCF_NONE = 0x00000000,
        LVCF_PRESSED = 0x00000001,
        LVCF_ITEMPRESSED = 0x00000002,
        LVCF_EXPLORERDRAG = 0x00000004,
        LVCF_NOANIMATE = 0x00000008,
        LVCF_ANIMATEPARTIAL = 0x00000010,
        LVCF_NOASSIGNFUNC = 0x00000020,
        LVCF_TOUCH = 0x00000040
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

    enum LVItemFlags : DWORD
    {
        LVIF_NONE = 0x00000000,
        LVIF_DIR = 0x00000001,
        LVIF_GROUP = 0x00000002,
        LVIF_HIDDEN = 0x00000004,
        LVIF_MEMSELECT = 0x00000008,
        LVIF_SHORTCUT = 0x00000010,
        LVIF_COLORLOCK = 0x00000020,
        LVIF_DRAG = 0x00000040,
        LVIF_REFRESH = 0x00000080,
        LVIF_SFG = 0x00000100,
        LVIF_FLYING = 0x00000200,
        LVIF_MOVING = 0x00000400,
        LVIF_ADVANCEDICON = 0x00000800,
        LVIF_NOSELTRIGGER = 0x00001000,
        LVIF_NOGROUPANIM = 0x00002000,
        LVIF_NEWITEM = 0x00004000,
        LVIF_GROUPEX = 0x00008000,
        LVIF_TEXTONLY = 0x00010000
    };

    class LVItem;
    class LVItemTouchGrid;

    // 0.5.8: ListView classes not finalized, they are just a predecessor of what's coming in 0.6. Feedback is welcomed.

    class LVCommon : public Element
    {
    public:
        DDUIAPI LVCommon()
            : _pelMarqueeSelector(nullptr)
            , _hWorker(nullptr)
            , _peSelector(nullptr)
            , _peWhitespace(nullptr)
            , _peSelected(nullptr)
            , _peFocused(nullptr)
            , _idSelectedPivot(0)
            , _ptOrigin{}
            , _szDrag{}
            , _rcGadget{}
            , _flags(LVCF_NONE)
            , _dwSafeRemove(0)
            , _ullRemoveTick(0)
        {
        }

        DDUIAPI ~LVCommon();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnInput(InputEvent* pInput) override;
        DDUIAPI void OnKeyFocusMoved(Element* peFrom, Element* peTo) override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI Element* GetSelectionElement();
        DDUIAPI TouchButton* GetWhitespaceElement();
        DDUIAPI LVCommonFlags GetFlags();
        DDUIAPI void AddFlags(LVCommonFlags lvcf);
        DDUIAPI void RemoveFlags(LVCommonFlags lvcf);
        DDUIAPI void SetFlags(LVCommonFlags lvcf);
        DDUIAPI POINT GetDragOriginPoint();
        DDUIAPI SIZE GetDragSize();

        DDUIAPI HRESULT Add(Element* pe);
        DDUIAPI virtual HRESULT Add(Element** ppe, UINT cCount) override;
        DDUIAPI HRESULT Add(Element* pe, CompareCallback lpfnCompare);
        DDUIAPI HRESULT Insert(Element* pe, UINT iInsertIdx);
        DDUIAPI virtual HRESULT Insert(Element** ppe, UINT cCount, UINT iInsertIdx) override;
        DDUIAPI HRESULT Remove(Element* pe);
        DDUIAPI virtual HRESULT Remove(Element** ppe, UINT cCount) override;
        DDUIAPI HRESULT RemoveAll();
        DDUIAPI HRESULT RemoveAndDestroy(Element* pe);
        DDUIAPI HRESULT RemoveAndDestroy(Element** ppe, UINT cCount);
        DDUIAPI HRESULT Destroy(bool fDelayed = true);
        DDUIAPI HRESULT DestroyAll(bool fDelayed);

        DDUIAPI static void SelectItemBase(Element* elem, Event* iev);
        DDUIAPI static void RefineSelections(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
        DDUIAPI static void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);

    protected:
        IElementListener* _pelMarqueeSelector;
        HWND _hWorker;
        Element* _peSelector;
        TouchButton* _peWhitespace;
        Element* _peSelected;
        Element* _peFocused;
        UINT _idSelectedPivot;
        POINT _ptOrigin;
        SIZE _szDrag;
        RECT _rcGadget;
        LVCommonFlags _flags;
        DWORD _dwSafeRemove;
        ULONGLONG _ullRemoveTick;
        DDUIAPI HRESULT _CreateLVVisual();
        DDUIAPI static HRESULT _CreateAnimatingClone(Element** ppeOrig, RECT* prcOrig, Element** ppeClone, UINT cCount);
        DDUIAPI void _RemoveStuckClones(DynamicArray<Element*>* rgList);
        DDUIAPI static void _MarqueeSelector(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
        DDUIAPI static DWORD WINAPI _UpdateMarqueeSelectorPosition(LPVOID lpParam);
        DDUIAPI static LRESULT CALLBACK s_ListViewProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        static IClassInfo* s_pClassInfo;
        virtual void _OnAddOrInsert(Element** ppe, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual HRESULT _OnRemoving(Element** ppe, Element** ppeClone, RECT* prcParent, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual void _OnRemove(Element** ppe, Element** ppeClone, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual void _OnRemoveAll() {}
    };

    class LVGrid : public LVCommon
    {
    public:
        DDUIAPI LVGrid()
            : _rgXPos{}
            , _rgYPos{}
            , _rgXItems{}
            , _rgYItems{}
            , _pePivot(nullptr)
            , _peAnimating(nullptr)
            , _fAllowNav(true)
            , _fKeyDown(false)
            , _keyStateOld(0)
            , _keyStateOld2(0)
        {
            _flags = LVCF_ANIMATEPARTIAL;
        }

        DDUIAPI ~LVGrid()
        {
        }
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnInput(InputEvent* pInput) override;
        DDUIAPI void OnKeyFocusMoved(Element* peFrom, Element* peTo) override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();

        DDUIAPI void NotifyGridChanges(LVItem** rgLVItem, POINT* rgPosOld, POINT* rgPosNew, int cSize);

    protected:
        vector<int> _rgXPos;
        vector<int> _rgYPos;
        vector<LVItem*> _rgXItems;
        vector<LVItem*> _rgYItems;
        Element* _pePivot;
        Element* _peAnimating;
        bool _fAllowNav;
        bool _fKeyDown;
        BYTE _keyStateOld;
        BYTE _keyStateOld2;
        DDUIAPI int _SearchArray(vector<int>* rgPos, int iCloseValue);
        DDUIAPI int _SearchArrayExact(vector<int>* rgPos, int iTargetValue);
        DDUIAPI void _ShiftSelectionHelper(RECT* prcPivot, RECT* prcTo);
        DDUIAPI static LRESULT CALLBACK s_LVGridProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        static IClassInfo* s_pClassInfo;
        virtual void _OnAddOrInsert(Element** ppe, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual HRESULT _OnRemoving(Element** ppe, Element** ppeClone, RECT* prcParent, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual void _OnRemove(Element** ppe, Element** ppeClone, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual void _OnRemoveAll();
    };

    class LVTiles : public LVCommon
    {
    public:
        DDUIAPI LVTiles()
            : _szGridLayout{}
        {
        }

        DDUIAPI ~LVTiles()
        {
        }
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI ItemMinWidthProp();
        DDUIAPI static const PropertyInfo* WINAPI ItemHeightProp();
        DDUIAPI int GetItemMinWidth();
        DDUIAPI int GetItemHeight();
        DDUIAPI void SetItemMinWidth(int iItemMinWidth);
        DDUIAPI void SetItemHeight(int iItemHeight);

    protected:
        SIZE _szGridLayout;
        DDUIAPI void _UpdateGridLayoutParams();
        DDUIAPI int GetPropCommon(const PropertyProcT pPropertyProc);
        DDUIAPI void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);

    private:
        static IClassInfo* s_pClassInfo;
        virtual void _OnAddOrInsert(Element** ppe, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual HRESULT _OnRemoving(Element** ppe, Element** ppeClone, RECT* prcParent, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual void _OnRemove(Element** ppe, Element** ppeClone, RECT* prcGadget, RECT* prcNext, UINT cCount);
        virtual void _OnRemoveAll() {}
    };

    class LVItem final : public DDScalableTouchButton
    {
    public:
        DDUIAPI LVItem()
            : _filename{}
            , _simplefilename{}
            , _ext{}
            , _flags(LVIF_NONE)
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
            , _peInner(nullptr)
            , _peIcon(nullptr)
            , _peShortcutArrow(nullptr)
            , _peText(nullptr)
            , _peCheckbox(nullptr)
            , _peItemCount(nullptr)
            , _childItemss(nullptr)
            , _pels{}
            , _hDirEvent(nullptr)
        {
        }

        DDUIAPI ~LVItem();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI unsigned short GetInternalXPos();
        DDUIAPI unsigned short GetInternalYPos();
        DDUIAPI unsigned short GetMemXPos();
        DDUIAPI unsigned short GetMemYPos();
        DDUIAPI void SetInternalXPos(unsigned short iXPos);
        DDUIAPI void SetInternalYPos(unsigned short iYPos);
        DDUIAPI void SetMemXPos(unsigned short iXPos);
        DDUIAPI void SetMemYPos(unsigned short iYPos);
        DDUIAPI wstring GetFilename();
        DDUIAPI wstring GetSimpleFilename();
        DDUIAPI wstring GetExt();
        DDUIAPI void SetFilename(const wstring& wsFilename);
        DDUIAPI void SetSimpleFilename(const wstring& wsSimpleFilename);
        DDUIAPI void SetExt(const wstring& wsExt);
        DDUIAPI LVItemFlags GetFlags();
        DDUIAPI void AddFlags(LVItemFlags lvif);
        DDUIAPI void RemoveFlags(LVItemFlags lvif);
        DDUIAPI void SetFlags(LVItemFlags lvif);
        DDUIAPI unsigned short GetPage();
        DDUIAPI unsigned short GetMemPage();
        DDUIAPI unsigned short GetPreRefreshMemPage();
        DDUIAPI unsigned short GetMemIconSize();
        DDUIAPI unsigned short GetItemCount();
        DDUIAPI unsigned short GetItemIndex();
        DDUIAPI void SetPage(unsigned short pageID);
        DDUIAPI void SetMemPage(unsigned short pageID);
        DDUIAPI void SetPreRefreshMemPage(unsigned short pageID);
        DDUIAPI void SetMemIconSize(unsigned short iconsz);
        DDUIAPI void SetItemCount(unsigned short itemCount);
        DDUIAPI void SetItemIndex(unsigned short itemIndex);
        DDUIAPI LVItemGroupSize GetGroupSize();
        DDUIAPI void SetGroupSize(LVItemGroupSize lvigs);
        DDUIAPI LVItemTileSize GetTileSize();
        DDUIAPI void SetTileSize(LVItemTileSize lvits);
        DDUIAPI LVItemOpenDirState GetOpenDirState();
        DDUIAPI void SetOpenDirState(LVItemOpenDirState lviods);
        DDUIAPI BYTE GetSmallPos();
        DDUIAPI void SetSmallPos(BYTE smPos);
        DDUIAPI LVItemTouchGrid* GetTouchGrid();
        DDUIAPI void SetTouchGrid(LVItemTouchGrid* lvitg);
        DDUIAPI void SetTouchGrid(LVItemTouchGrid* lvitg, BYTE index);
        DDUIAPI DDScalableElement* GetInnerElement();
        DDUIAPI DDScalableElement* GetIcon();
        DDUIAPI DDScalableElement** GetIconRef();
        DDUIAPI Element* GetShortcutArrow();
        DDUIAPI RichText* GetText();
        DDUIAPI TouchButton* GetCheckbox();
        DDUIAPI DDScalableRichText* GetItemCountElement();
        DDUIAPI void SetInnerElement(DDScalableElement* peInner);
        DDUIAPI void SetIcon(DDScalableElement* peIcon);
        DDUIAPI void SetShortcutArrow(Element* peShortcutArrow);
        DDUIAPI void SetText(RichText* peText);
        DDUIAPI void SetCheckbox(TouchButton* peCheckbox);
        DDUIAPI void SetItemCountElement(DDScalableRichText* peItemCount);
        DDUIAPI void DisconnectElements();
        DDUIAPI vector<LVItem*>* GetChildItems();
        DDUIAPI void SetChildItems(vector<LVItem*>* vpm);
        DDUIAPI vector<IElementListener*>* GetListeners();
        DDUIAPI void SetListeners(vector<IElementListener*> pels);
        DDUIAPI void ClearAllListeners();
        DDUIAPI HANDLE GetDirEvent();
        DDUIAPI void SetDirEvent(HANDLE hDirEvent);
        DDUIAPI void StopListening();

    private:
        static IClassInfo* s_pClassInfo;
        wstring _filename;
        wstring _simplefilename;
        wstring _ext;
        LVItemFlags _flags;
        unsigned short _xPos;
        unsigned short _yPos;
        unsigned short _mem_xPos;
        unsigned short _mem_yPos;
        unsigned short _page;
        unsigned short _mem_page;
        unsigned short _prmem_page;
        unsigned short _mem_iconsize;
        unsigned short _itemCount;
        unsigned short _itemIndex;
        LVItemGroupSize _groupsize;
        LVItemTileSize _tilesize;
        LVItemOpenDirState _opendirstate;
        BYTE _smallPos;
        LVItemTouchGrid* _touchGrid;
        DDScalableElement* _peInner;
        DDScalableElement* _peIcon;
        Element* _peShortcutArrow;
        RichText* _peText;
        TouchButton* _peCheckbox;
        DDScalableRichText* _peItemCount;
        vector<LVItem*>* _childItemss;
        vector<IElementListener*> _pels;
        HANDLE _hDirEvent;
    };

    class LVItemTouchGrid final
    {
    public:
        DDUIAPI LVItemTouchGrid()
            : _items{}
            , _xFirstTile(0)
            , _yFirstTile(0)
            , _maxCount(4)
            , _itemCount(0)
            , _cxTile(0)
            , _cyTile(0)
            , _cxPadding(0)
            , _cyPadding(0)
        {
        }

        DDUIAPI LVItemTouchGrid(unsigned short cxTile, unsigned short cyTile, unsigned short cxPadding, unsigned short cyPadding)
            : _items{}
            , _xFirstTile(0)
            , _yFirstTile(0)
            , _maxCount(4)
            , _itemCount(0)
            , _cxTile(cxTile)
            , _cyTile(cyTile)
            , _cxPadding(cxPadding)
            , _cyPadding(cyPadding)
        {
        }

        DDUIAPI ~LVItemTouchGrid()
        {
        }

        DDUIAPI void Insert(LVItem* lvi);
        DDUIAPI void Insert(LVItem* lvi, BYTE index);
        DDUIAPI void Erase(BYTE index);
        DDUIAPI BYTE GetItemCount();

    private:
        LVItem* _items[4];
        unsigned short _xFirstTile;
        unsigned short _yFirstTile;
        unsigned short _cxTile;
        unsigned short _cyTile;
        unsigned short _cxPadding;
        unsigned short _cyPadding;
        BYTE _maxCount;
        BYTE _itemCount;
        void _RefreshLVItemPositions(BYTE index);
    };

    class DDLVActionButton final : public DDScalableTouchButton
    {
    public:
        DDUIAPI DDLVActionButton()
            : _assocItem(nullptr)
        {
        }

        DDUIAPI ~DDLVActionButton();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI LVItem* GetAssociatedItem();
        DDUIAPI void SetAssociatedItem(LVItem* lvi);

    private:
        static IClassInfo* s_pClassInfo;
        LVItem* _assocItem;
    };

    class DDIconButton final : public DDScalableTouchButton
    {
    public:
        DDUIAPI DDIconButton()
            : _peIcon(nullptr)
            , _peContent(nullptr)
        {
        }

        DDUIAPI ~DDIconButton();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI IconFontProp();
        DDUIAPI static const PropertyInfo* WINAPI IconContentProp();
        DDUIAPI const WCHAR* GetIconFont(Value** ppv);
        DDUIAPI const WCHAR* GetIconContent(Value** ppv);

    private:
        static IClassInfo* s_pClassInfo;
        DDScalableRichText* _peIcon;
        DDScalableRichText* _peContent;
        HRESULT _CreateIBVisual();
    };

    class DDToggleButton final : public DDScalableTouchButton
    {
    public:
        DDUIAPI DDToggleButton()
        {
        }

        DDUIAPI ~DDToggleButton()
        {
        }

        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI CheckedStateProp();
        DDUIAPI bool GetCheckedState();
        DDUIAPI void SetCheckedState(bool bChecked);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDCheckBoxGlyph final : public DDScalableElement
    {
    public:
        DDUIAPI DDCheckBoxGlyph()
        {
        }

        DDUIAPI ~DDCheckBoxGlyph()
        {
        }

        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI CheckedStateProp();
        DDUIAPI bool GetCheckedState();
        DDUIAPI void SetCheckedState(bool bChecked);

    private:
        static IClassInfo* s_pClassInfo;
    };

    class DDCheckBox final : public DDScalableTouchButton
    {
    public:
        DDUIAPI DDCheckBox()
            : _peGlyph(nullptr)
            , _peText(nullptr)
        {
        }

        DDUIAPI ~DDCheckBox()
        {
        }

        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI CheckedStateProp();
        DDUIAPI bool GetCheckedState();
        DDUIAPI void SetCheckedState(bool bChecked);

    private:
        static IClassInfo* s_pClassInfo;
        DDCheckBoxGlyph* _peGlyph;
        DDScalableElement* _peText;
        HRESULT _CreateCBVisual();
    };

    class DDNumberedButton : public DDScalableTouchButton
    {
    public:
        DDUIAPI DDNumberedButton()
            : _id(0)
            , _peLinked(nullptr)
        {
        }

        DDUIAPI ~DDNumberedButton()
        {
        }
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnEvent(Event* pEvent) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI void SetNumberID(short id);
        DDUIAPI void SetLinkedElement(void* peLinked);

    protected:
        static IClassInfo* s_pClassInfo;
        short _id;
        void* _peLinked;
    };

    class DDCombobox final : public DDScalableTouchButton
    {
    public:
        DDUIAPI DDCombobox()
            : _selID(0)
            , _selSize(0)
            , _hTimer(nullptr)
            , _tick(0)
            , _fDone(false)
            , _peDropDownGlyph(nullptr)
            , _wndSelectionMenu(nullptr)
            , _peSelectionMenu(nullptr)
            , _tsvSelectionMenu(nullptr)
            , _peHostInner(nullptr)
            , _peSelections{}
            , _rcDest{}
        {
        }

        DDUIAPI ~DDCombobox();
        enum { MAX_SELECTIONS = 255 };
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnEvent(Event* pEvent) override;
        DDUIAPI static UID WINAPI SelectionChange();
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI ListMaxHeightProp();
        DDUIAPI int GetListMaxHeight();
        DDUIAPI void SetListMaxHeight(int iListMaxHeight);
        DDUIAPI void InsertSelection(BYTE index, LPCWSTR pszSelectionStr);
        DDUIAPI void EraseSelection(BYTE index);
        DDUIAPI BYTE GetSelection();
        DDUIAPI void SetSelection(BYTE index);
        DDUIAPI void ToggleSelectionList(bool fForceHide);

    private:
        static IClassInfo* s_pClassInfo;
        BYTE _selID;
        BYTE _selSize;
        HWND _hTimer;
        LONGLONG _tick;
        bool _fDone;
        DDScalableRichText* _peDropDownGlyph;
        NativeHWNDHost* _wndSelectionMenu;
        HWNDElement* _peSelectionMenu;
        TouchScrollViewer* _tsvSelectionMenu;
        Element* _peHostInner;
        DDNumberedButton* _peSelections[MAX_SELECTIONS];
        RECT _rcDest;
        HRESULT _CreateCMBVisual();
        static LRESULT CALLBACK s_TimerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK s_ComboboxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };

    class DDSlider final : public TouchButton
    {
    public:
        DDUIAPI DDSlider()
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

        DDUIAPI ~DDSlider();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnInput(InputEvent* pInput) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI IsVerticalProp();
        DDUIAPI static const PropertyInfo* WINAPI TextWidthProp();
        DDUIAPI static const PropertyInfo* WINAPI TextHeightProp();
        DDUIAPI bool GetIsVertical();
        DDUIAPI int GetTextWidth();
        DDUIAPI int GetTextHeight();
        DDUIAPI void SetIsVertical(bool bIsVertical);
        DDUIAPI void SetTextWidth(int iTextWidth);
        DDUIAPI void SetTextHeight(int iTextHeight);
        DDUIAPI RegKeyValue GetRegKeyValue();
        DDUIAPI void SetRegKeyValue(RegKeyValue rkvNew);
        DDUIAPI float GetMinValue();
        DDUIAPI float GetMaxValue();
        DDUIAPI float GetCurrentValue();
        DDUIAPI float GetTickValue();
        DDUIAPI int* GetAssociatedValue();
        DDUIAPI void SetMinValue(float minValue);
        DDUIAPI void SetMaxValue(float maxValue);
        DDUIAPI void SetCurrentValue(float currValue, bool fExternal);
        DDUIAPI void SetTickValue(float tickValue);
        DDUIAPI void SetAssociatedValue(int* assocVal, int extValueMultiplier);
        DDUIAPI LPCWSTR GetFormattedString();
        DDUIAPI void SetFormattedString(LPCWSTR szFormatted);

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
        DDUIAPI DDColorPickerButton()
            : _assocCR(0)
            , _order(0)
        {
        }

        DDUIAPI ~DDColorPickerButton();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI static HRESULT Register();
        DDUIAPI COLORREF GetAssociatedColor();
        DDUIAPI BYTE GetOrder();
        DDUIAPI void SetAssociatedColor(COLORREF cr);
        DDUIAPI void SetOrder(BYTE bOrder);

    private:
        static IClassInfo* s_pClassInfo;
        COLORREF _assocCR;
        BYTE _order;
    };

    class DDColorPicker final : public Element
    {
    public:
        DDUIAPI DDColorPicker()
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

        DDUIAPI ~DDColorPicker();
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnInput(InputEvent* pInput) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI static const PropertyInfo* WINAPI FirstScaledImageProp();
        DDUIAPI static const PropertyInfo* WINAPI ScaledImageIntervalsProp();
        DDUIAPI static const PropertyInfo* WINAPI ColorIntensityProp();
        DDUIAPI static const PropertyInfo* WINAPI DefaultColorProp();
        DDUIAPI int GetFirstScaledImage();
        DDUIAPI int GetScaledImageIntervals();
        DDUIAPI int GetColorIntensity();
        DDUIAPI COLORREF GetDefaultColor();
        DDUIAPI void SetFirstScaledImage(int iFirstImage);
        DDUIAPI void SetScaledImageIntervals(int iScaleIntervals);
        DDUIAPI void SetColorIntensity(int iColorIntensity);
        DDUIAPI void SetDefaultColor(COLORREF iDefaultColor);
        DDUIAPI RegKeyValue GetRegKeyValue();
        DDUIAPI vector<DDScalableElement*> GetTargetElements();
        DDUIAPI vector<DDScalableButton*> GetTargetButtons();
        DDUIAPI vector<DDScalableTouchButton*> GetTargetTouchButtons();
        DDUIAPI bool GetThemeAwareness();
        DDUIAPI void SetRegKeyValue(RegKeyValue rkvNew);
        DDUIAPI void SetTargetElements(vector<DDScalableElement*> vte);
        DDUIAPI void SetTargetButtons(vector<DDScalableButton*> vtb);
        DDUIAPI void SetTargetTouchButtons(vector<DDScalableTouchButton*> vttb);
        DDUIAPI void SetThemeAwareness(bool ta);

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
        DDUIAPI DDTabbedPages()
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

        DDUIAPI ~DDTabbedPages();
        enum { MAX_TABPAGES = 32 };
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnInput(InputEvent* pInput) override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();
        DDUIAPI void SetKeyFocus() override;
        DDUIAPI void BindParser(DUIXmlParser* pParser);
        DDUIAPI void InsertTab(BYTE index, LPCWSTR pszResIDPage, LPCWSTR pszTabLabel, GenericTabFunction ptfn);
        DDUIAPI void EraseTab(BYTE index);
        DDUIAPI void TraversePage(BYTE index);

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

    class DDMenuButton final : public DDNumberedButton
    {
        friend class DDMenu;
    public:
        DDUIAPI DDMenuButton()
            : _peIcon(nullptr)
            , _peMainText(nullptr)
            , _peHelpText(nullptr)
            , _peSubmenuArrow(nullptr)
            , _submenu(nullptr)
            , _lpmii(nullptr)
            , _fRadio(false)
            , _fKeyFocusInit(false)
            , _uOrder(0)
        {
        }

        DDUIAPI ~DDMenuButton()
        {
        }
        DDUIAPI static IClassInfo* GetClassInfoPtr();
        DDUIAPI static void SetClassInfoPtr(IClassInfo* pClass);
        DDUIAPI IClassInfo* GetClassInfoW() override;
        DDUIAPI void OnEvent(Event* pEvent) override;
        DDUIAPI void OnInput(InputEvent* pInput) override;
        DDUIAPI void OnKeyFocusMoved(Element* peFrom, Element* peTo) override;
        DDUIAPI bool OnPropertyChanging(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
        DDUIAPI static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
        DDUIAPI HRESULT Initialize(int nCreate, Element* pParent, DWORD* pdwDeferCookie);
        DDUIAPI static HRESULT Register();

    private:
        static IClassInfo* s_pClassInfo;
        DDScalableRichText* _peIcon;
        DDScalableRichText* _peMainText;
        DDScalableRichText* _peHelpText;
        DDScalableRichText* _peSubmenuArrow;
        DDMenu* _submenu;
        LPMENUITEMINFOW _lpmii;
        bool _fRadio;
        bool _fKeyFocusInit;
        UINT _uOrder;
        HRESULT _CreateMBVisual();
    };

    struct MenuImage
    {
        HBITMAP hbmp;
        LPCWSTR glyph;
        BYTE type;
    };

    class DDMenu
    {
        friend class DDMenuButton;
#define DDM_ANIMATESUBMENUS 0x20000
    public:
        DDUIAPI DDMenu()
            : _lRefCount(1)
            , _pICv1(nullptr)
            , _hMenu(nullptr)
            , _hTimer(nullptr)
            , _hWndTrack(nullptr)
            , _interfaceLevel(0)
            , _uTrackFlags(0)
            , _uID(0)
            , _idCmdFirst(0)
            , _idCmdLast(0)
            , _hKeys{}
            , _pItemArray(nullptr)
            , _pAssoc(nullptr)
            , _count(0)
            , _width(0)
            , _fUsingLegacy(false)
            , _fDone(false)
            , _fAnimating(true) // Needed for initial displaying
            , _fDynamicInit(false)
            , _subLevel(0)
            , _selectedCommand(0)
            , _tick(0)
            , _rcMenu{}
            , _scbi(nullptr)
            , _parent(nullptr)
            , _wndSelectionMenu(nullptr)
            , _peSelectionMenu(nullptr)
            , _tsvSelectionMenu(nullptr)
            , _peHostInner(nullptr)
            , _peSelections{}
            , _rgMenuImg{}
            , _hmChildren{}
        {
        }

        DDUIAPI ~DDMenu()
        {
        }
        DDUIAPI HRESULT InitializeDesktopEntries(IShellFolder* psf, IShellView* psv);
        DDUIAPI HRESULT InitializeItemEntries(vector<LVItem**> vItems, IShellFolder* psf, LPCITEMIDLIST* ppidl, UINT cidl);
        DDUIAPI HRESULT CreatePopupMenu(bool fLegacy);
        DDUIAPI void DestroyPopupMenu();
        DDUIAPI bool GetMenuItemInfoW(UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii);
        DDUIAPI bool SetMenuItemInfoW(UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii);
        DDUIAPI void SetMenuItemGlyph(UINT item, BOOL fByPosition, LPCWSTR pszGlyph);
        DDUIAPI void AppendMenuW(UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
        DDUIAPI void EnableMenuItem(UINT uIDEnableItem, UINT uEnable);
        DDUIAPI void InsertMenuW(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
        DDUIAPI void RemoveMenu(UINT uPosition, UINT uFlags);
        DDUIAPI void QueryContextMenu(UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        DDUIAPI int TrackPopupMenuEx(UINT uFlags, int x, int y, HWND hwnd, LPTPMPARAMS lptpm);
        DDUIAPI HRESULT InvokeCommand(CMINVOKECOMMANDINFO* pici);
        DDUIAPI HRESULT HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);
        DDUIAPI HRESULT GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved, CHAR* pszName, UINT cchMax);
        DDUIAPI int GetItemCount();
        DDUIAPI HRESULT GetMenuRect(LPRECT lprc);
        DDUIAPI void ForceEnablePaste();

    private:
        enum { MAX_ITEMS = 1024 };

        LONG _lRefCount;
        IContextMenu* _pICv1;
        HMENU _hMenu;
        HWND _hTimer;
        HWND _hWndTrack;
        BYTE _interfaceLevel;
        UINT _uTrackFlags;
        UINT _uID;
        UINT _idCmdFirst;
        UINT _idCmdLast;
        HKEY _hKeys[4];
        IShellItemArray* _pItemArray;
        IQueryAssociations* _pAssoc;
        int _count;
        int _width;
        bool _fUsingLegacy;
        bool _fDone;
        bool _fAnimating;
        bool _fDynamicInit;
        BYTE _subLevel;
        int _selectedCommand;
        LONGLONG _tick;
        RECT _rcMenu;
        SimpleCubicBezierInterpolator* _scbi;
        DDMenu* _parent;
        NativeHWNDHost* _wndSelectionMenu;
        HWNDElement* _peSelectionMenu;
        TouchScrollViewer* _tsvSelectionMenu;
        Element* _peHostInner;
        DDMenuButton* _peSelections[MAX_ITEMS];
        MenuImage* _rgMenuImg[MAX_ITEMS];
        vector<HMENU> _hmChildren;
        
        void _AppendItem(LPCMENUITEMINFOW lpmii, LPCWSTR lpNewItem, bool fInternal);
        void _ApplyMII(DDMenuButton* pmb, bool fInternal, unsigned short count);
        void _DestroyUI(bool fSource);
        void _HideMenu();
        void _OnButtonClick(DDMenuButton* button, bool fInstant);
        void _PopulateFromQuery(UINT uCount, bool fCheckID);
        void _RegisterAsSubmenu(DDMenuButton* pmb, DDMenu* parent);
        void _SetVisible(int x, int y, DDMenu* menu, bool fInstant);
        static LRESULT CALLBACK s_TimerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK s_MenuProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        static LRESULT CALLBACK s_HookMenuProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    };

    enum DDNotificationType
    {
        DDNT_SUCCESS = 0,
        DDNT_INFO = 1,
        DDNT_WARNING = 2,
        DDNT_ERROR = 3
    };

    class DDNotificationBanner
    {
    public:
        DDUIAPI DDNotificationBanner()
            : _hTimer(nullptr)
            , _wnd(nullptr)
            , _notificationType(DDNT_INFO)
            , _titleStr{}
            , _pDDNB(nullptr)
            , _icon(nullptr)
            , _title(nullptr)
            , _content(nullptr)
            , _stackIndicator(nullptr)
            , _stackCount(0)
            , _peButtonSection(nullptr)
            , _btnCount(0)
            , _tick(0)
            , _rcWindow{}
            , _iDeltaY(0)
            , _fStartedAnim(false)
            , _scbi(nullptr)
        {
        }

        DDUIAPI ~DDNotificationBanner();
        DDUIAPI void CreateBanner(DDNotificationType type, LPCWSTR title, LPCWSTR content, short timeout);
        DDUIAPI static void s_RepositionBanners(bool fReverse, int iDeltaY, int iBoundY);
        DDUIAPI void DestroyBanner();
        DDUIAPI static void s_DestroyBannerByButton(Element* elem, Event* iev);
        DDUIAPI void AppendButton(LPCWSTR szButtonText, void(*pListener)(Element* elem, Event* iev), bool fClose);

    private:
        static IClassInfo* s_pClassInfo;
        HWND _hTimer;
        NativeHWNDHost* _wnd;
        DDNotificationType _notificationType;
        WCHAR _titleStr[64];
        HWNDElement* _pDDNB;
        DDScalableElement* _icon;
        DDScalableElement* _title;
        DDScalableElement* _content;
        DDScalableElement* _stackIndicator;
        BYTE _stackCount;
        Element* _peButtonSection;
        BYTE _btnCount;
        LONGLONG _tick;
        RECT _rcWindow;
        int _iDeltaY;
        bool _fStartedAnim;
        SimpleCubicBezierInterpolator* _scbi;
        static LRESULT CALLBACK s_TimerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK s_NotificationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };
}
