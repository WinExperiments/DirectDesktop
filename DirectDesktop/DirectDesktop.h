#pragma once
#pragma warning(disable:28159)

#include "resource.h"
#include <string>
#include "DirectUI\DirectUI.h"

using namespace DirectUI;

#define DESKPADDING_NORMAL 4
#define DESKPADDING_TOUCH 16 

// Common functions
std::wstring LoadStrFromRes(UINT id);
std::wstring LoadStrFromRes(UINT id, LPCWSTR dllName);

template <typename elemType>
extern elemType regElem(const wchar_t* elemName, Element* peParent);
extern struct EventListener;
extern struct EventListener2;
extern void assignFn(Element* btnName, void(*fnName)(Element* elem, Event* iev));
extern void assignExtendedFn(Element* elemName, void(*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2));