#pragma once
#pragma warning(disable:6258)
#include <string>
#include <vector>
#include "DirectUI/DirectUI.h"

using namespace std;
using namespace DirectUI;

extern int dpi, dpiLaunch;
extern float flScaleFactor;
extern NativeHWNDHost* subviewwnd;
extern int GetCurrentScaleInterval();
extern struct yValue;

struct RegKeyValue {
    HKEY _hKeyName;
    LPCWSTR _path;
    const wchar_t* _valueToFind;
    DWORD _dwValue;
};

class DDScalableElement : public Element {
public:
    DDScalableElement();
    ~DDScalableElement();
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
    static const PropertyInfo* WINAPI AssociatedColorProp();
    int GetFirstScaledImage();
    int GetScaledImageIntervals();
    int GetDrawType();
    bool GetEnableAccent();
    bool GetNeedsFontResize();
    int GetAssociatedColor();
    void SetFirstScaledImage(int iFirstImage);
    void SetScaledImageIntervals(int iScaleIntervals);
    void SetDrawType(int iDrawType);
    void SetEnableAccent(bool bEnableAccent);
    void SetNeedsFontResize(bool bNeedsFontResize);
    void SetAssociatedColor(int iAssociatedColor);
    int GetDDCPIntensity();
    void SetDDCPIntensity(int intensity);
    void InitDrawImage();
    static void RedrawImages();
    void InitDrawFont();
    static void RedrawFonts();
protected:
    static vector<DDScalableElement*> _arrCreatedElements;
    int _intensity = 255;
    auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
    void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);
private:
    static IClassInfo* s_pClassInfo;
};

class DDScalableButton : public Button {
public:
    DDScalableButton();
    ~DDScalableButton();
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
    static const PropertyInfo* WINAPI AssociatedColorProp();
    int GetFirstScaledImage();
    int GetScaledImageIntervals();
    int GetDrawType();
    bool GetEnableAccent();
    bool GetNeedsFontResize();
    int GetAssociatedColor();
    void SetFirstScaledImage(int iFirstImage);
    void SetScaledImageIntervals(int iScaleIntervals);
    void SetDrawType(int iDrawType);
    void SetEnableAccent(bool bEnableAccent);
    void SetNeedsFontResize(bool bNeedsFontResize);
    void SetAssociatedColor(int iAssociatedColor);
    void InitDrawImage();
    static void RedrawImages();
    void InitDrawFont();
    static void RedrawFonts();

    RegKeyValue GetRegKeyValue();
    void(*GetAssociatedFn())(bool, bool, bool);
    bool* GetAssociatedBool();
    int GetDDCPIntensity();
    void SetRegKeyValue(RegKeyValue rkvNew);
    void SetAssociatedFn(void(*pfn)(bool, bool, bool));
    void SetAssociatedBool(bool* pb);
    void SetDDCPIntensity(int intensity);
    void ExecAssociatedFn(void(*pfn)(bool, bool, bool), bool fnb1, bool fnb2, bool fnb3);
protected:
    static vector<DDScalableButton*> _arrCreatedButtons;
    RegKeyValue _rkv{};
    void(*_assocFn)(bool, bool, bool) = nullptr;
    bool* _assocBool = nullptr;
    int _intensity = 255;
    auto GetPropCommon(const PropertyProcT pPropertyProc, bool useInt);
    void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt, bool useInt);
private:
    static IClassInfo* s_pClassInfo;
};

class LVItem final : public DDScalableButton {
public:
    LVItem() {

    }
    virtual ~LVItem() {

    }
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
    bool GetHiddenState();
    bool GetMemorySelected();
    bool GetShortcutState();
    bool GetColorLock();
    bool GetDragState();
    void SetDirState(bool dirState);
    void SetHiddenState(bool hiddenState);
    void SetMemorySelected(bool mem_isSelectedState);
    void SetShortcutState(bool shortcutState);
    void SetColorLock(bool colorLockState);
    void SetDragState(bool dragstate);
    unsigned short GetPage();
    void SetPage(unsigned short pageID);
private:
    static IClassInfo* s_pClassInfo;
    wstring _filename{};
    wstring _simplefilename{};
    bool _isDirectory = false;
    bool _isHidden = false;
    bool _mem_isSelected = false;
    bool _isShortcut = false;
    bool _colorLock = false;
    bool _dragged = false;
    unsigned short _xPos = 999;
    unsigned short _yPos = 999;
    unsigned short _page{};
};

class DDToggleButton final : public DDScalableButton {
public:
    DDToggleButton() {

    }
    virtual ~DDToggleButton() {

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

class DDCheckBox final : public DDScalableButton {
public:
    DDCheckBox() {

    }
    virtual ~DDCheckBox() {

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

class DDCheckBoxGlyph final : public DDScalableElement {
public:
    DDCheckBoxGlyph() {

    }
    virtual ~DDCheckBoxGlyph() {

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

class DDColorPicker final : public Element {
public:
    DDColorPicker() {

    }
    virtual ~DDColorPicker() {

    }
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
    void SetRegKeyValue(RegKeyValue rkvNew);
    vector<DDScalableElement*> GetTargetElements();
    void SetTargetElements(vector<DDScalableElement*> vte);
private:
    static IClassInfo* s_pClassInfo;
    RegKeyValue _rkv{};
    vector<DDScalableElement*> _targetElems{};
    int GetPropCommon(const PropertyProcT pPropertyProc);
    void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);
};

class DDColorPickerButton final : public Button {
public:
    DDColorPickerButton() {

    }
    virtual ~DDColorPickerButton() {

    }
    static IClassInfo* GetClassInfoPtr();
    static void SetClassInfoPtr(IClassInfo* pClass);
    IClassInfo* GetClassInfoW() override;
    static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
    static HRESULT Register();
    COLORREF GetAssociatedColor();
    BYTE GetOrder();
    vector<DDScalableElement*> GetTargetElements();
    void SetAssociatedColor(COLORREF cr);
    void SetOrder(BYTE bOrder);
    void SetTargetElements(vector<DDScalableElement*> vte);
private:
    static IClassInfo* s_pClassInfo;
    COLORREF _assocCR{};
    BYTE _order{};
    vector<DDScalableElement*> _targetElems{};
};

enum DDNotificationType {
    DDNT_SUCCESS = 0,
    DDNT_INFO = 1,
    DDNT_WARNING = 2,
    DDNT_ERROR = 3
};

class DDNotificationBanner final : public HWNDElement {
public:
    DDNotificationBanner() {

    }
    virtual ~DDNotificationBanner() {

    }
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