#include "LVItem.h"
using namespace std;

DirectUI::IClassInfo* LVItem::s_pClassInfo;

DirectUI::IClassInfo* LVItem::GetClassInfoPtr() {
    return s_pClassInfo;
}
void LVItem::SetClassInfoPtr(DirectUI::IClassInfo* pClass) {
    s_pClassInfo = pClass;
}
DirectUI::IClassInfo* LVItem::GetClassInfoW() {
    return s_pClassInfo;
}
HRESULT LVItem::Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement) {
    return DirectUI::CreateAndInit<LVItem, int>(0x1 | 0x2, pParent, pdwDeferCookie, ppElement);
}
HRESULT LVItem::Register() {
    return DirectUI::ClassInfo<LVItem, DirectUI::Button, DirectUI::StandardCreator<LVItem>>::Register(L"LVItem", nullptr, 0);
}
unsigned short LVItem::GetInternalXPos() {
    return _xPos;
}
unsigned short LVItem::GetInternalYPos() {
    return _yPos;
}
void LVItem::SetInternalXPos(unsigned short iXPos) {
    _xPos = iXPos;
}
void LVItem::SetInternalYPos(unsigned short iYPos) {
    _yPos = iYPos;
}
wstring LVItem::GetFilename() {
    return _filename;
}
wstring LVItem::GetSimpleFilename() {
    return _simplefilename;
}
void LVItem::SetFilename(const wstring& wsFilename) {
    _filename = wsFilename;
}
void LVItem::SetSimpleFilename(const wstring& wsSimpleFilename) {
    _simplefilename = wsSimpleFilename;
}
bool LVItem::GetDirState() {
    return _isDirectory;
}
bool LVItem::GetHiddenState() {
    return _isHidden;
}
bool LVItem::GetMemorySelected() {
    return _mem_isSelected;
}
bool LVItem::GetShortcutState() {
    return _isShortcut;
}
void LVItem::SetDirState(bool dirState) {
    _isDirectory = dirState;
}
void LVItem::SetHiddenState(bool hiddenState) {
    _isHidden = hiddenState;
}
void LVItem::SetMemorySelected(bool mem_isSelectedState) {
    _mem_isSelected = mem_isSelectedState;
}
void LVItem::SetShortcutState(bool shortcutState) {
    _isShortcut = shortcutState;
}
unsigned short LVItem::GetPage() {
    return _page;
}
void LVItem::SetPage(unsigned short pageID) {
    _page = pageID;
}