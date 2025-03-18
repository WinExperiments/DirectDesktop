#pragma once
#include <string>
#include "DirectUI/DirectUI.h"

using namespace std;

class LVItem final : public DirectUI::Button {
public:
    LVItem() {

    }
    virtual ~LVItem() {

    }
    static DirectUI::IClassInfo* GetClassInfoPtr();
    static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
    DirectUI::IClassInfo* GetClassInfoW() override;
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
    void SetDirState(bool dirState);
    void SetHiddenState(bool hiddenState);
    void SetMemorySelected(bool mem_isSelectedState);
    void SetShortcutState(bool shortcutState);
    void SetColorLock(bool colorLockState);
    unsigned short GetPage();
    void SetPage(unsigned short pageID);
private:
    static DirectUI::IClassInfo* s_pClassInfo;
    wstring _filename{};
    wstring _simplefilename{};
    bool _isDirectory = false;
    bool _isHidden = false;
    bool _mem_isSelected = false;
    bool _isShortcut = false;
    bool _colorLock = false;
    unsigned short _xPos = 999;
    unsigned short _yPos = 999;
    unsigned short _page{};
};

struct RegKeyValue {
    HKEY _hKeyName;
    LPCWSTR _path;
    const wchar_t* _valueToFind;
    DWORD _dwValue;
};

class DDButtonBase : public DirectUI::Button {
public:
    DDButtonBase() {

    }
    virtual ~DDButtonBase() {

    }
    RegKeyValue GetRegKeyValue();
    void(*GetAssociatedFn())(bool, bool);
    bool* GetAssociatedBool();
    void SetRegKeyValue(RegKeyValue rkvNew);
    void SetAssociatedFn(void(*pfn)(bool, bool));
    void SetAssociatedBool(bool* pb);
    void ExecAssociatedFn(void(*pfn)(bool, bool), bool fnb1, bool fnb2);
protected:
    RegKeyValue _rkv{};
    void(*_assocFn)(bool, bool) = nullptr;
    bool* _assocBool = nullptr;
};

class DDToggleButton final : public DDButtonBase {
public:
    DDToggleButton() {

    }
    virtual ~DDToggleButton() {

    }
    static DirectUI::IClassInfo* GetClassInfoPtr();
    static void SetClassInfoPtr(DirectUI::IClassInfo* pClass);
    DirectUI::IClassInfo* GetClassInfoW() override;
    static HRESULT Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
    static HRESULT Register();
private:
    static DirectUI::IClassInfo* s_pClassInfo;
};