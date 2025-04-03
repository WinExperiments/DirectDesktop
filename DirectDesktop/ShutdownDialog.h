#pragma once
#pragma warning(disable:6258)
#include "framework.h"
#include "DirectUI/DirectUI.h"
#include "StyleModifier.h"
#include <string>
#include <strsafe.h>

using namespace std;
using namespace DirectUI;

extern NativeHWNDHost* wnd;
extern float flScaleFactor;
extern int dpi, dpiLaunch;
extern bool dialogopen;
extern bool theme;
extern bool delayedshutdownstatuses[6];
extern Element* regElem(const wchar_t* elemName, Element* peParent);
extern Button* regBtn(const wchar_t* btnName, Element* peParent);
extern RichText* regRichText(const wchar_t* elemName, Element* peParent);
extern wstring LoadStrFromRes(UINT id);
extern wstring LoadStrFromRes(UINT id, LPCWSTR dllName);
extern struct EventListener;
extern struct EventListener2;
extern void assignFn(Element* btnName, void(*fnName)(Element* elem, Event* iev));
extern void assignExtendedFn(Element* elemName, void(*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2));

void DisplayShutdownDialog();
void DestroyShutdownDialog();