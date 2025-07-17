#pragma once
#pragma warning(disable:28159)

#include "resource.h"
#include "ui\DDControls.h"
#include <vector>
#include <string>
#include "Include\dui70\DirectUI\DirectUI.h"

using namespace DirectUI;

#define DESKPADDING_NORMAL 4
#define DESKPADDING_NORMAL_X 4
#define DESKPADDING_NORMAL_Y 4
#define DESKPADDING_TOUCH 8
#define DESKPADDING_TOUCH_X 32
#define DESKPADDING_TOUCH_Y 128

namespace DirectDesktop
{
    extern float g_flScaleFactor;
    extern unsigned short g_defWidth, g_defHeight, g_lastWidth, g_lastHeight;
    extern int g_iconsz, g_shiconsz, g_gpiconsz;
    extern int g_currentPageID, g_maxPageID, g_homePageID;

    struct yValue {
        int num{};
        float fl1{};
        float fl2{};
    };
    struct yValueEx {
        int num{};
        float fl1{};
        float fl2{};
        std::vector<LVItem*>* vpm{};
        std::vector<DDScalableElement*>* vipm{};
        std::vector<Element*>* vispm{};
        std::vector<Element*>* vspm{};
        std::vector<RichText*>* vfpm{};
        Element* peOptionalTarget1{};
        Element* peOptionalTarget2{};
    };
    struct yValuePtrs {
        void* ptr1{};
        void* ptr2{};
    };
    struct DesktopIcon {
        HBITMAP icon{};
        HBITMAP iconshadow{};
        HBITMAP iconshortcut{};
        HBITMAP text{};
        HBITMAP textshadow{};
        COLORREF crDominantTile{};
        HBITMAP dominantTile{};
    };

    // Common functions
    std::wstring LoadStrFromRes(UINT id);
    std::wstring LoadStrFromRes(UINT id, LPCWSTR dllName);
    std::wstring RemoveQuotes(const std::wstring& input);
    extern void CALLBACK DUI_ParserErrorCB(const WCHAR* pszError, const WCHAR* pszToken, int dLine, void* pContext);

    extern bool isDefaultRes();

    DWORD WINAPI fastin(LPVOID lpParam);

    template <typename elemType>
    extern elemType regElem(const wchar_t* elemName, Element* peParent);
    extern struct EventListener;
    extern struct EventListener2;
    extern EventListener* assignFn(Element* btnName, void(*fnName)(Element* elem, Event* iev), bool fReturn = false);
    extern EventListener2* assignExtendedFn(Element* elemName, void(*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2), bool fReturn = false);
}