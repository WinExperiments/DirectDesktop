#pragma once
#pragma warning(disable:28159)

#include "ui\DDControls.h"
#include "coreui\AnimationHelper.h"

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
    extern int g_touchSizeX, g_touchSizeY;
    extern unsigned short g_defWidth, g_defHeight, g_lastWidth, g_lastHeight;
    extern int g_iconsz, g_shiconsz, g_gpiconsz;
    extern int g_currentPageID, g_maxPageID, g_homePageID;
    extern HWND g_msgwnd;
    extern bool DWMActive;

    struct yValue
    {
        int num{};
        float fl1{};
        float fl2{};
    };

    struct yValueEx
    {
        int num{};
        float fl1{};
        float fl2{};
        std::vector<LVItem*>* vpm{};
        Element* peOptionalTarget1{};
        Element* peOptionalTarget2{};
    };

    struct yValuePtrs
    {
        void* ptr1{};
        void* ptr2{};
        DWORD dwMillis;
    };

    struct DesktopIcon
    {
        HBITMAP icon{};
        HBITMAP iconshadow{};
        HBITMAP iconshortcut{};
        HBITMAP text{};
        HBITMAP textshadow{};
        COLORREF crDominantTile{};
        HBITMAP dominantTile{};
    };

    struct DelayedElementActions
    {
        DWORD dwMillis;
        Element* pe;
        float val1;
        float val2;
    };

    struct EventListener : public IElementListener
    {
        void (*f)(Element*, Event*);

        EventListener(void (*func)(Element*, Event*))
        {
            f = func;
        }

        void OnListenerAttach(Element* elem) override
        {
        }

        void OnListenerDetach(Element* elem) override
        {
        }

        bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override
        {
            return true;
        }

        void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2) override
        {
        }

        void OnListenedEvent(Element* elem, struct Event* iev) override
        {
            f(elem, iev);
        }

        void OnListenedInput(Element* elem, struct InputEvent* ev) override
        {
        }
    };

    struct EventListener2 : public IElementListener
    {
        void (*f)(Element*, const PropertyInfo*, int, Value*, Value*);

        EventListener2(void (*func)(Element*, const PropertyInfo*, int, Value*, Value*))
        {
            f = func;
        }

        void OnListenerAttach(Element* elem) override
        {
        }

        void OnListenerDetach(Element* elem) override
        {
        }

        bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override
        {
            return true;
        }

        void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2) override
        {
            f(elem, prop, type, v1, v2);
        }

        void OnListenedEvent(Element* elem, struct Event* iev) override
        {
        }

        void OnListenedInput(Element* elem, struct InputEvent* ev) override
        {
        }
    };

    // Common functions
    std::wstring LoadStrFromRes(UINT id);
    std::wstring LoadStrFromRes(UINT id, LPCWSTR dllName);
    std::wstring RemoveQuotes(const std::wstring& input);
    extern void DUI_SetGadgetZOrder(DirectUI::Element* pe, UINT uZOrder);
    BOOL ScheduleGadgetTransitions_DWMCheck(UINT uOrder, UINT rgTransSize, const GTRANS_DESC* rgTrans, HGADGET hgad, TransitionStoryboardInfo* ptsbInfo);
    extern void CALLBACK DUI_ParserErrorCB(const WCHAR* pszError, const WCHAR* pszToken, int dLine, void* pContext);
    extern bool EnsureRegValueExists(HKEY hKeyName, LPCWSTR path, LPCWSTR valueToFind);
    extern int GetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName);
    extern void SetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, DWORD dwValue, bool find, bool* isNewValue);
    extern bool GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, WCHAR** outStr);
    extern bool GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE** outBytes);
    extern void SetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE* bValue, DWORD length, bool find, bool* isNewValue);
    extern HRESULT CloakWindow(HWND hwnd, bool fCloak);

    extern bool isDefaultRes();
    extern bool IsDesktopActive();
    extern bool IsDesktopOrSubviewActive();

    DWORD WINAPI fastin(LPVOID lpParam);

    template <typename elemType>
    elemType regElem(const wchar_t* elemName, Element* peParent)
    {
        elemType result = (elemType)peParent->FindDescendent(StrToID(elemName));
        return result;
    }
    EventListener* assignFn(Element* elemName, void (*fnName)(Element* elem, Event* iev), bool fReturn = false);
    EventListener2* assignExtendedFn(Element* elemName, void (*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2), bool fReturn = false);
}
