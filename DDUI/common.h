#pragma once

#ifdef DDUI_EXPORTS
#define DDUIAPI __declspec(dllexport)
#else
#define DDUIAPI __declspec(dllimport)
#endif

#include "SettingsHelper.h"
#include "coreui\StyleModifier.h"

using namespace DirectUI;

namespace DDUI
{
    extern HMODULE g_hmod;
    extern DDUICtx g_ctx;
    extern DDUIColors g_colors;
    extern HWND g_msgwnd;

    struct yValue
    {
        int num{};
        float fl1{};
        float fl2{};
    };

    struct yValuePtrs
    {
        void* ptr1{};
        void* ptr2{};
        DWORD dwMillis;
    };

    struct DelayedElementActions
    {
        DWORD dwMillis;
        Element* pe;
        Element** ppe;
        float val1;
        float val2;
    };

    class DDUIAPI EventListener : public CSafeElementListenerCB
    {
    public:
        EventListener(void (*func)(Element*, Event*));
        bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2);
        void OnListenedEvent(Element* elem, Event* iev) override;

    private:
        void (*f)(Element*, Event*);
    };

    class DDUIAPI EventListener2 : public CSafeElementListenerCB
    {
    public:
        EventListener2(void (*func)(Element*, const PropertyInfo*, int, Value*, Value*));
        bool OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2);
        void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2);

    private:
        void (*f)(Element*, const PropertyInfo*, int, Value*, Value*);
    };

    // Common functions
    DDUIAPI void InitializeDDUI(HMODULE hModule);
    DDUIAPI __declspec(noinline) HRESULT LoadStrFromRes(LPWSTR pszOut, UINT cSize, UINT id, LPCWSTR dllName = nullptr);
    DDUIAPI HRESULT GetDialogString(LPWSTR pszOut, UINT cSize, UINT id, LPCWSTR dllName, UINT optCtrlID, short uCtrlIDOrder);
    DDUIAPI bool ValidateStrDigits(const WCHAR* str);
    DDUIAPI void InitialUpdateScale();
    DDUIAPI void UpdateScale(); // Win10 1607+
    DDUIAPI int GetCurrentScaleInterval();
    DDUIAPI DWORD WINAPI DeselectElement(LPVOID lpParam);
    DDUIAPI DWORD WINAPI SetElemPos(LPVOID lpParam);
    DDUIAPI DWORD WINAPI MultiClickHandler(LPVOID lpParam);
    DDUIAPI bool IsServer();
    DDUIAPI void DUI_SetGadgetZOrder(DirectUI::Element* pe, UINT uZOrder);
    DDUIAPI BOOL ScheduleGadgetTransitions_DWMCheck(UINT uOrder, UINT rgTransSize, const GTRANS_DESC* rgTrans, HGADGET hgad, TransitionStoryboardInfo* ptsbInfo);
    DDUIAPI HRESULT WINAPI s_InitializeDUI(HMODULE hModule);
    DDUIAPI void CALLBACK DUI_ParserErrorCB(const WCHAR* pszError, const WCHAR* pszToken, int dLine, void* pContext);
    DDUIAPI HWND InitializeCallbackWindow();
    DDUIAPI bool EnsureRegValueExists(HKEY hKeyName, LPCWSTR path, LPCWSTR valueToFind);
    DDUIAPI int GetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName);
    DDUIAPI void SetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, DWORD dwValue, bool find, bool* isNewValue);
    DDUIAPI bool GetRegistryStrValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, WCHAR** outStr);
    DDUIAPI bool GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE** outBytes);
    DDUIAPI void SetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE* bValue, DWORD length, bool find, bool* isNewValue);
    DDUIAPI DDUICtx* GetProcContext();
    DDUIAPI DDUIColors* GetProcColors();

    DDUIAPI Element* regElem(const wchar_t* elemName, Element* peParent);
    DDUIAPI EventListener* assignFn(Element* elemName, void (*fnName)(Element* elem, Event* iev), bool fReturn = false);
    DDUIAPI EventListener2* assignExtendedFn(Element* elemName, void (*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2), bool fReturn = false);

    inline HRESULT ResultFromWin32(__in DWORD dwErr)
    {
        return HRESULT_FROM_WIN32(dwErr);
    }

    inline HRESULT ResultFromLastError()
    {
        return ResultFromWin32(GetLastError());
    }

    inline HRESULT ResultFromKnownLastError()
    {
        HRESULT hr = ResultFromLastError();
        return (SUCCEEDED(hr) ? E_FAIL : hr);
    }

    inline HRESULT ResultFromWin32Bool(BOOL b)
    {
        return b ? S_OK : ResultFromKnownLastError();
    }
}