#pragma once
#include "framework.h"
#include "DirectUI/DirectUI.h"
#include "DDControls.h"
#include <string>
#include <vector>

using namespace DirectUI;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

extern bool renameactive;
extern bool touchmode;
extern std::vector<LVItem*> pm;
extern std::wstring LoadStrFromRes(UINT id);
extern std::wstring LoadStrFromRes(UINT id, LPCWSTR dllName);
extern std::wstring RemoveQuotes(const std::wstring& input);
extern int validItems;
extern int globaliconsz;
extern Element* pMain;
extern DUIXmlParser* parser;
extern NativeHWNDHost* wnd;
extern void SelectItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);

template <typename elemType>
extern elemType regElem(const wchar_t* elemName, Element* peParent);

void ShowRename();