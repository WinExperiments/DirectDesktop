#pragma once
#include "framework.h"
#include "DirectUI/DirectUI.h"

using namespace DirectUI;

template <typename elemType>
extern elemType regElem(const wchar_t* elemName, Element* peParent);
extern struct EventListener;
extern struct EventListener2;
extern void assignFn(Element* btnName, void(*fnName)(Element* elem, Event* iev));
extern void assignExtendedFn(Element* elemName, void(*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2));

void CreateSearchPage();