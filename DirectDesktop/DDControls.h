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
extern struct EventListener2;
extern void assignExtendedFn(Element* elemName, void(*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2));

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
    int GetFirstScaledImage();
    int GetScaledImageIntervals();
    int GetDrawType();
    int GetEnableAccent();
    int GetNeedsFontResize();
    void SetFirstScaledImage(int iFirstImage);
    void SetScaledImageIntervals(int iScaleIntervals);
    void SetDrawType(int iDrawType);
    void SetEnableAccent(int iEnableAccent);
    void SetNeedsFontResize(int iNeedsFontResize);
    void InitDrawImage();
    static void RedrawImages();
    void InitDrawFont();
    static void RedrawFonts();
protected:
    static vector<DDScalableElement*> _arrCreatedElements;
    int GetPropCommon(const PropertyProcT pPropertyProc);
    void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);
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
    int GetFirstScaledImage();
    int GetScaledImageIntervals();
    int GetDrawType();
    int GetEnableAccent();
    int GetNeedsFontResize();
    void SetFirstScaledImage(int iFirstImage);
    void SetScaledImageIntervals(int iScaleIntervals);
    void SetDrawType(int iDrawType);
    void SetEnableAccent(int iEnableAccent);
    void SetNeedsFontResize(int iNeedsFontResize);
    void InitDrawImage();
    static void RedrawImages();
    void InitDrawFont();
    static void RedrawFonts();

    RegKeyValue GetRegKeyValue();
    void(*GetAssociatedFn())(bool, bool);
    bool* GetAssociatedBool();
    void SetRegKeyValue(RegKeyValue rkvNew);
    void SetAssociatedFn(void(*pfn)(bool, bool));
    void SetAssociatedBool(bool* pb);
    void ExecAssociatedFn(void(*pfn)(bool, bool), bool fnb1, bool fnb2);
protected:
    static vector<DDScalableButton*> _arrCreatedButtons;
    RegKeyValue _rkv{};
    void(*_assocFn)(bool, bool) = nullptr;
    bool* _assocBool = nullptr;
    int GetPropCommon(const PropertyProcT pPropertyProc);
    void SetPropCommon(const PropertyProcT pPropertyProc, int iCreateInt);
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
private:
    static IClassInfo* s_pClassInfo;
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