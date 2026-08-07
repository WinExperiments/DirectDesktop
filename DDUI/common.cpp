#include "pch.h"
#include "common.h"
#include "DDControls.h"
#include "coreui\StyleModifier.h"
#include <intrin.h>

using namespace std;
using namespace DirectUI;

namespace DDUI
{
    HWND g_msgwnd;
    HMODULE g_hmod;
    DUIXmlParser* g_parser;
    WNDPROC WndProcMessagesOnly;
    DDUICtx g_ctx;
    DDUIColors g_colors;

    EventListener::EventListener(bool (*funcChanging)(Element*, const PropertyInfo*, int, Value*, Value*))
    {
        fChanging = funcChanging;
        fChanged = nullptr;
        fInput = nullptr;
        fEvent = nullptr;
        _bType = 1;
    }

    EventListener::EventListener(void (*funcChanged)(Element*, const PropertyInfo*, int, Value*, Value*))
    {
        fChanging = nullptr;
        fChanged = funcChanged;
        fInput = nullptr;
        fEvent = nullptr;
        _bType = 2;
    }

    EventListener::EventListener(void (*funcInput)(Element*, InputEvent*))
    {
        fChanging = nullptr;
        fChanged = nullptr;
        fInput = funcInput;
        fEvent = nullptr;
        _bType = 3;
    }

    EventListener::EventListener(void (*funcEvent)(Element*, Event*))
    {
        fChanging = nullptr;
        fChanged = nullptr;
        fInput = nullptr;
        fEvent = funcEvent;
        _bType = 4;
    }

    void EventListener::OnListenerDetach(Element* elem)
    {
        delete this;
    }

    bool EventListener::OnListenedPropertyChanging(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2)
    {
        if (_bType == 1)
            return fChanging(elem, prop, type, v1, v2);
        return true;
    }

    void EventListener::OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2)
    {
        if (_bType == 2)
            fChanged(elem, prop, type, v1, v2);
    }

    void EventListener::OnListenedInput(Element* elem, InputEvent* ev)
    {
        if (_bType == 3)
            fInput(elem, ev);
    }

    void EventListener::OnListenedEvent(Element* elem, Event* iev)
    {
        if (_bType == 4)
            fEvent(elem, iev);
    }


    HRESULT LoadStrFromRes(LPWSTR pszOut, UINT cSize, UINT id, LPCWSTR dllName)
    {
        if (dllName)
        {
            HINSTANCE hInst = (HINSTANCE)LoadLibraryExW(dllName, nullptr, LOAD_LIBRARY_AS_DATAFILE);
            if (hInst)
            {
                LoadStringW(hInst, id, pszOut, cSize);
                FreeLibrary(hInst);
            }
            else return E_INVALIDARG;
        }
        else
        {
            HMODULE hCaller{};
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, (LPCWSTR)_ReturnAddress(), &hCaller))
                LoadStringW((HINSTANCE)hCaller, id, pszOut, cSize);
            else return E_FAIL;
        }
        return S_OK;
    }

    bool ValidateStrDigits(const WCHAR* str)
    {
        if (!str || *str == L'\0') return false;
        while (*str)
        {
            if (!iswdigit(*str)) return false;
            ++str;
        }
        return true;
    }

    void SkipDlgSection(const BYTE*& p, const BYTE*& pEnd)
    {
        if (p + 2 > pEnd) return;
        if (*((const WORD*)p) == 0xFFFF)
        {
            p += 4;
            if (p > pEnd) return;
        }
        else
        {
            while (p < pEnd && *((const wchar_t*)p)) p += 2;
            p += 2;
            if (p > pEnd) return;
        }
    }

    HRESULT GetDialogString(LPWSTR pszOut, UINT cSize, UINT id, LPCWSTR dllName, UINT optCtrlID, short sCtrlIDOrder)
    {
        HMODULE hDLL = LoadLibraryW(dllName);
        HRSRC hRes = FindResourceW(hDLL, MAKEINTRESOURCE(id), RT_DIALOG);
        if (!hRes) return E_FAIL;
        DWORD resSize = SizeofResource(hDLL, hRes);
        if (resSize < 24) return E_POINTER; // DIALOGEX header size (24)
        HGLOBAL hData = LoadResource(hDLL, hRes);
        if (!hData) return E_FAIL;
        BYTE* pData{};
        if (hData) pData = (BYTE*)LockResource(hData);
        if (!pData) return E_FAIL;
        const BYTE* pEnd = pData + resSize;
        const BYTE* pCurrent = pData;
        WORD itemCount = *(WORD*)(pCurrent + 0x10);
        if (pCurrent + 26 > pEnd) return E_POINTER; // Check header
        pCurrent += 26; // DIALOGEX offset

        // Skip menu
        SkipDlgSection(pCurrent, pEnd);

        // Skip class
        SkipDlgSection(pCurrent, pEnd);

        if (optCtrlID > 0)
        {
            UINT caption = 0;

            // Skip caption
            SkipDlgSection(pCurrent, pEnd);

            // Skip font info
            pCurrent += 2; // point size
            pCurrent += 2; // weight
            pCurrent += 1; // italic
            pCurrent += 1; // charset

            // Skip font face name
            while (*(WCHAR*)pCurrent)
            {
                pCurrent += 2;
            }

            pCurrent += 2;
            short ctrlIDOrder = 0;
            for (int i = 0; i < itemCount && pCurrent < pEnd; ++i)
            {
                // Align to DWORD
                pCurrent = (const BYTE*)(((uintptr_t)pCurrent + 3) & ~3);
                if (pCurrent + 20 > pEnd) break;

                WORD ctrlID = *(WORD*)(pCurrent + 20);
                pCurrent += 28; // DIALOGITEMTEMPLATEEX is 20 bytes + 8 till the string

                if (*(WORD*)pCurrent == 0x0000)
                {
                    pCurrent += 4; // ordinal
                    continue;
                }
                else
                {
                    while (*(const WCHAR*)pCurrent && caption < cSize - 1)
                    {
                        if (ctrlID == optCtrlID && ctrlIDOrder == sCtrlIDOrder)
                        {
                            pszOut[caption] = *((const WCHAR*)pCurrent);
                            caption++;
                        }
                        pCurrent += 2;
                    }
                    if (ctrlID == optCtrlID)
                    {
                        if (ctrlIDOrder == sCtrlIDOrder)
                        {
                            pszOut[min(caption, cSize - 1)] = L'\0';
                            return S_OK;
                        }
                        else ctrlIDOrder++;
                    }
                }

                pCurrent += 4;
            }
        }
        else
        {
            UINT caption = 0;
            while (pCurrent < pEnd && *((const WCHAR*)pCurrent) && caption < cSize - 1)
            {
                pszOut[caption] = *((const WCHAR*)pCurrent);
                caption++;
                pCurrent += 2;
                if (pCurrent > pEnd) return E_POINTER;
            }
            pszOut[min(caption, cSize - 1)] = L'\0';
            return S_OK;
        }
        return E_FAIL;
    }

    bool EnsureRegValueExists(HKEY hKeyName, LPCWSTR path, LPCWSTR valueToFind)
    {
        HKEY hKey = nullptr;
        LONG lResult;
        lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_READ, &hKey);
        if (lResult == ERROR_FILE_NOT_FOUND) return false;

        DWORD type;
        DWORD dataSize = 0;
        lResult = RegQueryValueExW(hKey, valueToFind, nullptr, &type, nullptr, &dataSize);
        RegCloseKey(hKey);

        if (lResult == ERROR_FILE_NOT_FOUND) return false;
        else if (lResult != ERROR_SUCCESS) return false;

        return true;
    }

    int GetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName)
    {
        int result = -1;
        DWORD dwSize{};
        LONG lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, nullptr, &dwSize);
        if (lResult == ERROR_SUCCESS)
        {
            DWORD* dwValue = (DWORD*)malloc(dwSize);
            lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, dwValue, &dwSize);
            if (dwValue != nullptr)
            {
                result = *dwValue;
                free(dwValue);
            }
        }
        return result;
    }

    void SetRegistryValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, DWORD dwValue, bool find, bool* isNewValue)
    {
        int result{};
        DWORD dwSize{};
        HKEY hKey;
        LONG lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, nullptr, &dwSize);
        lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_SET_VALUE, &hKey);
        if (lResult == ERROR_FILE_NOT_FOUND)
        {
            lResult = RegCreateKeyExW(hKeyName, path, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
            if (lResult == ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
                if (isNewValue != nullptr) *isNewValue = true;
            }
        }
        else if (lResult == ERROR_SUCCESS)
        {
            DWORD* dwValueInternal = (DWORD*)malloc(dwSize);
            lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_ANY, nullptr, dwValueInternal, &dwSize);
            if (lResult == ERROR_SUCCESS && find == false)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
                if (isNewValue != nullptr) *isNewValue = false;
            }
            else if (lResult != ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
                if (isNewValue != nullptr) *isNewValue = true;
            }
            free(dwValueInternal);
        }
        RegCloseKey(hKey);
    }

    bool GetRegistryStrValues(HKEY hKey, LPCWSTR path, LPCWSTR valueName, WCHAR** outStr)
    {
        if (!outStr) return false;

        DWORD dwSize{};
        LONG lResult = RegGetValueW(hKey, path, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            return false;
        }

        WCHAR* buffer = (WCHAR*)malloc(dwSize);
        if (!buffer)
        {
            return false;
        }

        lResult = RegGetValueW(hKey, path, valueName, RRF_RT_REG_SZ, nullptr, buffer, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            free(buffer);
            return false;
        }

        *outStr = buffer;
        return true;
    }

    bool GetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueName, BYTE** outBytes)
    {
        if (!outBytes) return false;

        DWORD dwSize{};
        LONG lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_REG_BINARY, nullptr, nullptr, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            return false;
        }

        BYTE* buffer = (BYTE*)malloc(dwSize);
        if (!buffer)
        {
            return false;
        }

        lResult = RegGetValueW(hKeyName, path, valueName, RRF_RT_REG_BINARY, nullptr, buffer, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            free(buffer);
            return false;
        }

        *outBytes = buffer;
        return true;
    }

    void SetRegistryBinValues(HKEY hKeyName, LPCWSTR path, LPCWSTR valueToSet, BYTE* bValue, DWORD length, bool find, bool* isNewValue)
    {
        int result{};
        DWORD dwSize{};
        HKEY hKey;
        LONG lResult = RegOpenKeyExW(hKeyName, path, 0, KEY_SET_VALUE, &hKey);
        if (lResult == ERROR_FILE_NOT_FOUND)
        {
            lResult = RegCreateKeyExW(hKeyName, path, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
            if (lResult == ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueToSet, 0, REG_BINARY, bValue, length);
                if (isNewValue != nullptr) *isNewValue = true;
            }
        }
        else if (lResult == ERROR_SUCCESS)
        {
            if (lResult == ERROR_SUCCESS && find == false)
            {
                lResult = RegSetValueExW(hKey, valueToSet, 0, REG_BINARY, bValue, length);
                if (isNewValue != nullptr) *isNewValue = false;
            }
            else if (lResult != ERROR_SUCCESS)
            {
                lResult = RegSetValueExW(hKey, valueToSet, 0, REG_BINARY, bValue, length);
                if (isNewValue != nullptr) *isNewValue = true;
            }
        }
        RegCloseKey(hKey);
    }

    DDUICtx* GetProcContext()
    {
        return &g_ctx;
    }

    DDUIColors* GetProcColors()
    {
        return &g_colors;
    }

    bool g_isDpiPreviouslyChanged;

    void InitialUpdateScale()
    {
        HDC screen = GetDC(nullptr);
        g_ctx.dpi = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(nullptr, screen);
        g_ctx.flScaleFactor = g_ctx.dpi / 96.0;
        g_ctx.dpiLaunch = g_ctx.dpi;
    }

    // Win10 1607+
    void UpdateScale()
    {
        g_ctx.dpiOld = g_ctx.dpi;
        g_ctx.dpi = GetDpiForWindow(g_msgwnd);
        g_isDpiPreviouslyChanged = true;
        g_ctx.flScaleFactor = g_ctx.dpi / 96.0;
    }

    int GetCurrentScaleInterval()
    {
        if (g_ctx.dpi >= 384) return 6;
        if (g_ctx.dpi >= 288) return 5;
        if (g_ctx.dpi >= 240) return 4;
        if (g_ctx.dpi >= 192) return 3;
        if (g_ctx.dpi >= 144) return 2;
        if (g_ctx.dpi >= 120) return 1;
        return 0;
    }

    void DUI_SetGadgetZOrder(DirectUI::Element* pe, UINT uZOrder)
    {
        if (g_ctx.DWMActive)
        {
            GTRANS_DESC rTrans = {};
            rTrans.hgadChange = pe->GetDisplayNode();
            rTrans.nProperty = 10;
            rTrans.zOrder = (int)uZOrder;
            SetTransitionVisualProperties(0, 1, &rTrans);
        }
    }

    BOOL ScheduleGadgetTransitions_DWMCheck(UINT uOrder, UINT rgTransSize, const GTRANS_DESC* rgTrans, HGADGET hgad, TransitionStoryboardInfo* ptsbInfo)
    {
        if (g_ctx.DWMActive)
            return ScheduleGadgetTransitions(uOrder, rgTransSize, rgTrans, hgad, ptsbInfo);
        else
            return FALSE;
    }

    Element* regElem(const wchar_t* elemName, Element* peParent)
    {
        return peParent->FindDescendent(StrToID(elemName));
    }

    EventListener* assignFn(Element* elemName, void (*fnName)(Element* elem, Event* iev), bool fReturn)
    {
        EventListener* pel = new EventListener(fnName);
        elemName->AddListener(pel);
        if (fReturn) return pel;
        return nullptr;
    }

    EventListener* assignExtendedFn(Element* elemName, void (*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2), bool fReturn)
    {
        EventListener* pel = new EventListener(fnName);
        elemName->AddListener(pel);
        if (fReturn) return pel;
        return nullptr;
    }

    LRESULT CALLBACK MsgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_DPICHANGED:
        {
            UpdateScale();
            break;
        }
        case WM_SETTINGCHANGE:
        {
            if (wParam == SPI_SETFONTSMOOTHING)
            {
                WCHAR* fontsmoothingStr;
                GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"FontSmoothing", &fontsmoothingStr);
                g_ctx.fontsmoothing = _wtoi(fontsmoothingStr);
                free(fontsmoothingStr);
            }
            BOOL bTemp;
            switch (wParam)
            {
            case SPI_SETANIMATION:
            {
                ANIMATIONINFO animInfo;
                animInfo.cbSize = sizeof(animInfo);
                SystemParametersInfoW(SPI_GETANIMATION, sizeof(animInfo), &animInfo, NULL);
                g_ctx.windowAnim = animInfo.iMinAnimate;
                break;
            }
            case SPI_SETCLIENTAREAANIMATION:
                SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, NULL, &bTemp, NULL);
                g_ctx.clientAnim = bTemp;
                break;
            case SPI_SETCOMBOBOXANIMATION:
                SystemParametersInfoW(SPI_GETCOMBOBOXANIMATION, NULL, &bTemp, NULL);
                g_ctx.comboAnim = bTemp;
                break;
            case SPI_SETMENUANIMATION:
                SystemParametersInfoW(SPI_GETMENUANIMATION, NULL, &bTemp, NULL);
                g_ctx.menuAnim = bTemp;
                break;
            case SPI_SETTOOLTIPANIMATION:
                SystemParametersInfoW(SPI_GETTOOLTIPANIMATION, NULL, &bTemp, NULL);
                g_ctx.tooltipAnim = bTemp;
                break;
            }
            RegKeyValue DDKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", nullptr, NULL);
            g_ctx.selectionrect = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"ListviewAlphaSelect");
            g_ctx.labelshadow = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"ListviewShadow");
            if (lParam && wcscmp((LPCWSTR)lParam, L"ShellState") == 0)
            {
                g_ctx.showcheckboxes = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AutoCheckSelect");
                g_ctx.iconunderline = GetRegistryValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"IconUnderline");
            }
            if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)
            {
                UpdateModeInfo(false);
            }
            break;
        }
        case WM_USER + 1:
        {
            break;
        }
        case WM_USER + 2:
        {
            break;
        }
        case WM_USER + 3:
        {
            break;
        }
        case WM_USER + 4:
        {
            break;
        }
        case WM_USER + 5:
        {
            DelayedElementActions* dea = (DelayedElementActions*)wParam;
            Element* pe;
            if (dea->ppe)
                pe = *(dea->ppe);
            else
                pe = dea->pe;
            if (pe)
            {
                if (!pe->IsDestroyed())
                {
                    switch (lParam)
                    {
                    case 1:
                        pe->SetVisible(false);
                        pe->DestroyAll(true);
                        pe->Destroy(true);
                        break;
                    case 2:
                        pe->SetVisible(!pe->GetVisible());
                        break;
                    case 3:
                        if (!pe->GetMouseWithin()) pe->SetSelected(false);
                        break;
                    case 4:
                        pe->SetX(dea->val1);
                        pe->SetY(dea->val2);
                        break;
                    case 5:
                        RedrawBorderCore<DDScalableElement>((DDScalableElement*)(pe));
                        break;
                    case 6:
                        pe->SetVisible(pe->GetSelected() || pe->GetMouseFocused());
                        break;
                    }
                }
            }
            delete dea;
            break;
        }
        case WM_USER + 6:
        {
            break;
        }
        case WM_USER + 7:
        {
            break;
        }
        }
        return CallWindowProc(WndProcMessagesOnly, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI DeselectElement(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        Element* pe;
        if (dea->ppe)
            pe = *(dea->ppe);
        else
            pe = dea->pe;
        if (pe)
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 3);
        else delete dea;
        return 0;
    }

    DWORD WINAPI SetElemPos(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        Element* pe;
        if (dea->ppe)
            pe = *(dea->ppe);
        else
            pe = dea->pe;
        if (pe)
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 4);
        else delete dea;
        return 0;
    }

    DWORD WINAPI MultiClickHandler(LPVOID lpParam)
    {
        int clicks = *(int*)lpParam;
        wchar_t* dcms{};
        GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"DoubleClickSpeed", &dcms);
        Sleep(_wtoi(dcms));
        free(dcms);
        if (clicks == *(int*)lpParam)
            *(int*)lpParam = 1;
        return 0;
    }

    bool IsServer()
    {
        wchar_t* productType = nullptr;
        GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions", L"ProductType", &productType);
        return (wcscmp(productType, L"ServerNT") == 0);
        free(productType);
    }

    // might not use it but this is a cool way to clone elements
    void InitializePreviewComponent(Element* peSrc, Element* peDst, bool fSetBG, bool fChild)
    {
        peDst->SetX(peSrc->GetX());
        peDst->SetY(peSrc->GetY());
        peDst->SetWidth(peSrc->GetWidth());
        peDst->SetHeight(peSrc->GetHeight());
        Value* v = peSrc->GetValue(Element::ContentProp, 1, nullptr);
        peDst->SetValue(Element::ContentProp, 1, v);
        v->Release();
        RECT rc{};
        peSrc->GetRenderBorderThickness(&rc);
        peDst->SetBorderThickness(rc.left, rc.top, rc.right, rc.bottom);
        peSrc->GetRenderPadding(&rc);
        peDst->SetPadding(rc.left, rc.top, rc.right, rc.bottom);
        if (fSetBG)
        {
            v = peSrc->GetValue(Element::BackgroundProp, 1, nullptr);
            peDst->SetValue(Element::BackgroundProp, 1, v);
            v->Release();
        }
        if (fChild)
        {
            peDst->SetVisible(peSrc->GetVisible());
            peDst->SetAlpha(peDst->GetParent()->GetAlpha());
        }
        DynamicArray<Element*>* peSrcChildren = peSrc->GetChildren(&v);
        DynamicArray<Element*>* peDstChildren = peDst->GetChildren(&v);
        if (peSrcChildren && peDstChildren)
            if (peSrcChildren->GetSize() > 0)
                for (int i = 0; i < peSrcChildren->GetSize() && i < peDstChildren->GetSize(); i++)
                    if (peSrcChildren->GetItem(i)->GetID() == peDstChildren->GetItem(i)->GetID())
                        InitializePreviewComponent(peSrcChildren->GetItem(i), peDstChildren->GetItem(i), true, true);
    }

    HWND InitializeCallbackWindow()
    {
        NativeHWNDHost* msgwnd{};
        NativeHWNDHost::Create(L"DD_MessageCallback", L"", nullptr, nullptr, 0, 0, 0, 0, NULL, NULL, nullptr, NULL, &msgwnd);
        WndProcMessagesOnly = (WNDPROC)SetWindowLongPtrW(msgwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)MsgWindowProc);

        return msgwnd->GetHWND();
    }

    // Windows.UI.Immersive.dll ordinal 100
    typedef HRESULT(WINAPI* RegisterImmersiveBehaviors_t)();

    HRESULT RegisterImmersiveBehaviors()
    {
        static RegisterImmersiveBehaviors_t fn = nullptr;
        if (!fn)
        {
            HMODULE h = LoadLibraryW(L"Windows.UI.Immersive.dll");
            if (h) fn = (RegisterImmersiveBehaviors_t)GetProcAddress(h, MAKEINTRESOURCEA(100));
        }
        if (fn == nullptr) return E_FAIL;
        else return fn();
    }

    // Windows.UI.Immersive.dll ordinal 101
    typedef void (WINAPI* UnregisterImmersiveBehaviors_t)();

    void UnregisterImmersiveBehaviors()
    {
        static UnregisterImmersiveBehaviors_t fn = nullptr;
        if (!fn)
        {
            HMODULE h = LoadLibraryW(L"Windows.UI.Immersive.dll");
            if (h) fn = (UnregisterImmersiveBehaviors_t)GetProcAddress(h, MAKEINTRESOURCEA(101));
        }
        if (fn == nullptr) return;
        else return fn();
    }

    HRESULT WINAPI InitializeImmersive()
    {
        HRESULT hr = RegisterPVLBehaviorFactory();
        if (SUCCEEDED(hr))
        {
            hr = RegisterImmersiveBehaviors();
            if (FAILED(hr)) UnregisterImmersiveBehaviors();
        }
        return hr;
    }

    HRESULT WINAPI s_InitializeDUI(HMODULE hModule)
    {
        HMODULE hTwinui = LoadLibraryW(L"twinui.dll");
        if (!hModule)
            hModule = hTwinui;
        HRESULT hr = InitProcessPriv(DUI_VERSION, hModule, true, true, true);
        if (SUCCEEDED(hr))
        {
            hr = InitThread(TSM_IMMERSIVE);
            if (SUCCEEDED(hr))
            {
                hr = InitializeImmersive();
                if (FAILED(hr)) hr = UnInitProcess();
            }
        }
        if (hTwinui)
            FreeLibrary(hTwinui);
        return hr;
    }

    void CALLBACK DUI_ParserErrorCB(const WCHAR* pszError, const WCHAR* pszToken, int dLine, void* pContext)
    {
        if (pszError != nullptr)
        {
            TaskDialog(nullptr, nullptr, L"DUIXMLPARSER FAILED", L"Error while parsing DirectUI", pszError, TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
            OutputDebugString(pszError);
            DebugBreak();
        }
    }

    void InitializeDDUI(HMODULE hModule)
    {
        g_ctx.DWMActive = IsCompositionActive();

        s_InitializeDUI(nullptr);
        g_msgwnd = InitializeCallbackWindow();
        RegisterAllControls();
        DDScalableElement::Register();
        DDScalableButton::Register();
        DDScalableRichText::Register();
        DDScalableTouchButton::Register();
        DDScalableTouchEdit::Register();
        LVCommon::Register();
        LVGrid::Register();
        LVTiles::Register();
        LVItem::Register();
        DDLVActionButton::Register();
        DDIconButton::Register();
        DDToggleButton::Register();
        DDCheckBox::Register();
        DDCheckBoxGlyph::Register();
        DDNumberedButton::Register();
        DDCombobox::Register();
        DDSlider::Register();
        DDColorPicker::Register();
        DDColorPickerButton::Register();
        DDTabbedPages::Register();
        DDMenuButton::Register();

        WCHAR localeName[256]{};
        ULONG numLanguages{};
        ULONG bufferSize = sizeof(localeName) / sizeof(WCHAR);
        int localeTemp{};
        GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, localeName, &bufferSize);
        GetLocaleInfoEx(localeName, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER, (LPWSTR)&localeTemp, sizeof(localeTemp) / sizeof(WCHAR));
        g_ctx.localeType = localeTemp;

        DUIXmlParser::Create(&g_parser, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
        g_parser->SetXMLFromResource(IDR_UIFILE1, g_hmod, g_hmod);

        UpdateModeInfo(true);
        g_ctx.themeOld = g_ctx.theme;

        RegKeyValue DDKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", nullptr, NULL);
        g_ctx.labelshadow = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"ListviewShadow");
        g_ctx.showcheckboxes = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AutoCheckSelect");
        g_ctx.iconunderline = GetRegistryValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"IconUnderline");
        g_ctx.selectionrect = GetRegistryValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"ListviewAlphaSelect");
        ANIMATIONINFO animInfo;
        animInfo.cbSize = sizeof(animInfo);
        SystemParametersInfoW(SPI_GETANIMATION, sizeof(animInfo), &animInfo, NULL);
        g_ctx.windowAnim = animInfo.iMinAnimate;
        BOOL bTemp;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, NULL, &bTemp, NULL);
        g_ctx.clientAnim = bTemp;
        SystemParametersInfoW(SPI_GETCOMBOBOXANIMATION, NULL, &bTemp, NULL);
        g_ctx.comboAnim = bTemp;
        SystemParametersInfoW(SPI_GETMENUANIMATION, NULL, &bTemp, NULL);
        g_ctx.menuAnim = bTemp;
        SystemParametersInfoW(SPI_GETTOOLTIPANIMATION, NULL, &bTemp, NULL);
        g_ctx.tooltipAnim = bTemp;
        WCHAR* fontsmoothingStr;
        GetRegistryStrValues(DDKey.GetHKeyName(), L"Control Panel\\Desktop", L"FontSmoothing", &fontsmoothingStr);
        g_ctx.fontsmoothing = _wtoi(fontsmoothingStr);
        free(fontsmoothingStr);
        DDKey.SetPath(L"Software\\DirectDesktop\\Debug");
        g_ctx.animCoef = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AnimationSpeed");
        g_ctx.AnimShiftKey = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AnimationsShiftKey");
    }

    extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
    {
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
            g_hmod = hModule;
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_ATTACH:
            return FALSE;
        case DLL_THREAD_DETACH:
            return FALSE;
        }
        return TRUE;
    }
}