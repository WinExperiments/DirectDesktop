#include "pch.h"

#include "DirectDesktop.h"

#include <cmath>
#include <list>
#include <powrprof.h>
#include <shlwapi.h>
#include <ShObjIdl.h>
#include <WinUser.h>
#include <wrl.h>
#include <wtsapi32.h>

#include "build_timestamp.h"
#include "backend\ContextMenus.h"
#include "backend\DirectoryHelper.h"
#include "backend\RenameCore.h"
#include "backend\SettingsHelper.h"
#include "coreui\BitmapHelper.h"
#include "coreui\ColorHelper.h"
#include "coreui\StyleModifier.h"
#include "ui\EditMode.h"
#include "ui\SearchPage.h"
#include "ui\ShutdownDialog.h"
#include "ui\Subview.h"

#pragma comment(lib, "version.lib")

using namespace DirectUI;
using namespace std;
using namespace Microsoft::WRL;

namespace DirectDesktop
{
    NativeHWNDHost *wnd;
    HWNDElement *parent;
    DUIXmlParser *parser, *g_parser;
    Element *pMain;
    unsigned long key = 0;

    Element* sampleText;
    Element* mainContainer;
    TouchButton* emptyspace;
    LVItem* g_outerElem;
    Element* selector;
    TouchButton *prevpageMain, *nextpageMain;
    Element* g_dragpreview;
    Element* dragpreview, *dragpreviewTouch;

    DDScalableElement* RegistryListener;
    wstring path1, path2, path3;

    HRESULT err;
    HWND g_hWorkerW = nullptr;
    HWND g_hSHELLDLL_DefView = nullptr;
    HWND g_hWndTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    HWND g_msgwnd;
    Logger MainLogger;

    DWORD shutdownReason = SHTDN_REASON_UNKNOWN;
    int g_maxPageID = 1, g_currentPageID = 1, g_homePageID = 1;
    int localeType{};
    int g_touchSizeX, g_touchSizeY;
    unsigned short g_defWidth, g_defHeight, g_lastWidth, g_lastHeight;
    int g_lastDpiChangeTick;
    bool g_ignoreWorkAreaChange = false;
    bool DWMActive;

    wstring LoadStrFromRes(UINT id)
    {
        WCHAR* loadedStrBuffer = new WCHAR[512]{};
        LoadStringW((HINSTANCE)HINST_THISCOMPONENT, id, loadedStrBuffer, 512);
        wstring loadedStr = loadedStrBuffer;
        delete[] loadedStrBuffer;
        return loadedStr;
    }

    wstring LoadStrFromRes(UINT id, LPCWSTR dllName)
    {
        WCHAR* loadedStrBuffer = new WCHAR[512]{};
        HINSTANCE hInst = (HINSTANCE)LoadLibraryExW(dllName, nullptr, LOAD_LIBRARY_AS_DATAFILE);
        LoadStringW(hInst, id, loadedStrBuffer, 512);
        wstring loadedStr = loadedStrBuffer;
        delete[] loadedStrBuffer;
        return loadedStr;
    }

    wstring RemoveQuotes(const wstring& input)
    {
        if (input.size() >= 2 && input.front() == L'\"' && input.back() == L'\"')
        {
            return input.substr(1, input.size() - 2);
        }
        return input;
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

    wstring GetDialogString(UINT id, LPCWSTR dllName, UINT optCtrlID, short sCtrlIDOrder)
    {
        HMODULE hDLL = LoadLibraryW(dllName);
        HRSRC hRes = FindResourceW(hDLL, MAKEINTRESOURCE(id), RT_DIALOG);
        if (!hRes) return L"";
        DWORD resSize = SizeofResource(hDLL, hRes);
        if (resSize < 24) return L""; // DIALOGEX header size (24)
        HGLOBAL hData = LoadResource(hDLL, hRes);
        if (!hData) return L"";
        BYTE* pData{};
        if (hData) pData = (BYTE*)LockResource(hData);
        if (!pData) return L"";
        const BYTE* pEnd = pData + resSize;
        const BYTE* pCurrent = pData;
        WORD itemCount = *(WORD*)(pCurrent + 0x10);
        if (pCurrent + 26 > pEnd) return L""; // Check header
        pCurrent += 26; // DIALOGEX offset

        // Skip menu
        SkipDlgSection(pCurrent, pEnd);

        // Skip class
        SkipDlgSection(pCurrent, pEnd);

        if (optCtrlID > 0)
        {
            wstring caption;

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
                    while (*(WCHAR*)pCurrent)
                    {
                        if (ctrlID == optCtrlID && ctrlIDOrder == sCtrlIDOrder)
                            caption += *((const WCHAR*)pCurrent);
                        pCurrent += 2;
                    }
                    if (ctrlID == optCtrlID)
                    {
                        if (ctrlIDOrder == sCtrlIDOrder) return caption;
                        else ctrlIDOrder++;
                    }
                }

                pCurrent += 4;
            }
        }
        else
        {
            wstring caption;
            while (pCurrent < pEnd && *((const WCHAR*)pCurrent))
            {
                caption += *((const WCHAR*)pCurrent);
                pCurrent += 2;
                if (pCurrent > pEnd) return L"";
            }
            return caption;
        }
        return L"";
    }

    int g_dpi = 96, g_dpiOld = 1, g_dpiLaunch{};
    float g_flScaleFactor = 1.0;
    bool g_isDpiPreviouslyChanged;

    bool isDefaultRes()
    {
        int w = (int)(g_lastWidth / g_flScaleFactor);
        int h = (int)(g_lastHeight / g_flScaleFactor);
        return (w <= g_defWidth + 10 && w >= g_defWidth - 10 && h <= g_defHeight + 10 && h >= g_defHeight - 10);
    }

    void InitialUpdateScale()
    {
        HDC screen = GetDC(nullptr);
        g_dpi = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(nullptr, screen);
        g_flScaleFactor = g_dpi / 96.0;
        g_dpiLaunch = g_dpi;
        g_touchSizeX *= g_flScaleFactor;
        g_touchSizeY *= g_flScaleFactor;
    }

    void UpdateScale()
    {
        HWND hWnd = subviewwnd->GetHWND();
        g_dpiOld = g_dpi;
        g_dpi = GetDpiForWindow(hWnd);
        g_isDpiPreviouslyChanged = true;
        g_flScaleFactor = g_dpi / 96.0;
        g_touchSizeX *= static_cast<float>(g_dpi) / g_dpiOld;
        g_touchSizeY *= static_cast<float>(g_dpi) / g_dpiOld;
    }

    int GetCurrentScaleInterval()
    {
        if (g_dpi >= 384) return 6;
        if (g_dpi >= 288) return 5;
        if (g_dpi >= 240) return 4;
        if (g_dpi >= 192) return 3;
        if (g_dpi >= 144) return 2;
        if (g_dpi >= 120) return 1;
        return 0;
    }

    void DUI_SetGadgetZOrder(DirectUI::Element* pe, UINT uZOrder)
    {
        if (DWMActive)
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
        if (DWMActive)
            return ScheduleGadgetTransitions(uOrder, rgTransSize, rgTrans, hgad, ptsbInfo);
        else
            return FALSE;
    }

    EventListener* assignFn(Element* elemName, void (*fnName)(Element* elem, Event* iev), bool fReturn)
    {
        EventListener* pel = new EventListener(fnName);
        elemName->AddListener(pel);
        if (fReturn) return pel;
        return nullptr;
    }

    EventListener2* assignExtendedFn(Element* elemName, void (*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2), bool fReturn)
    {
        EventListener2* pel = new EventListener2(fnName);
        elemName->AddListener(pel);
        if (fReturn) return pel;
        return nullptr;
    }

    struct FileInfo
    {
        wstring filepath;
        wstring filename;
    };

    struct ThumbnailIcon
    {
        int x{};
        int y{};
        ThumbIcons str;
        HBITMAP icon;
    };

    int origX{}, origY{}, g_iconsz, g_shiconsz, g_gpiconsz;
    int g_emptyclicks = 1;
    bool g_touchmode{};
    bool g_renameactive{};
    bool isPressed = 0, isIconPressed = 0;
    bool delayedshutdownstatuses[6] = { false, false, false, false, false, false };

    void SetTheme()
    {
        StyleSheet* sheet = pMain->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parser->GetSheet(g_theme ? L"default" : L"defaultdark", &sheetStorage);
        pMain->SetValue(Element::SheetProp, 1, sheetStorage);
        StyleSheet* sheet2 = pSubview->GetSheet();
        CValuePtr sheetStorage2 = DirectUI::Value::CreateStyleSheet(sheet2);
        parserSubview->GetSheet(g_theme ? L"popup" : L"popupdark", &sheetStorage2);
        pSubview->SetValue(Element::SheetProp, 1, sheetStorage2);
    }

    WNDPROC WndProc, WndProcMessagesOnly;
    HANDLE hMutex;
    constexpr LPCWSTR szWindowClass = L"DIRECTDESKTOP";
    BYTE* shellstate;
    vector<LVItem*> pm;
    vector<LVItem*> selectedLVItems;
    bool g_launch = true;
    bool g_setcolors = true;
    bool g_canRefreshMain = true;
    bool g_hiddenIcons;
    bool g_editmode = false;
    bool g_invokedpagechange = false;
    bool g_ensureNoRefresh = false;
    bool g_pageviewer = false;
    bool g_searchopen = false;
    void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
    void ApplyIcons(vector<LVItem*>* pmLVItem, DesktopIcon* di, bool subdirectory, int id, float scale, COLORREF crSubdir);
    void IconThumbHelper(int id);
    DWORD WINAPI CreateIndividualThumbnail(LPVOID lpParam);
    DWORD WINAPI SetElemPos(LPVOID lpParam);
    DWORD WINAPI RearrangeIconsHelper(LPVOID lpParam);
    void ShowDirAsGroupDesktop(LVItem* lvi, bool fNew);
    void SelectItem(Element* elem, Event* iev);
    void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void ItemDragListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void UpdateGroupOnColorChange(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);

    enum WTS_STREAMTYPE
    {
        WTSST_UNKNOWN = 0,
        WTSST_JPEG = 1,
        WTSST_BMP = 2,
        WTSST_PNG = 3,
    };

    enum WTS_THUMBNAILTYPE
    {
        WTSTT_IMAGE = 0,
        WTSTT_ICON = 1,
    };

    MIDL_INTERFACE("8a322201-0a87-46c1-9c77-7620e0cc5bbc")
        IShellItemImageFactoryPriv : IShellItemImageFactory
    {
        virtual HRESULT STDMETHODCALLTYPE GetSharedBitmap(SIZE, SIIGBF, ISharedBitmap**) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetAdornedBitmap(SIZE, SIIGBF, ISharedBitmap**) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetImageStream(SIZE, SIIGBF, WTS_STREAMTYPE*, WTS_THUMBNAILTYPE*, WTS_CACHEFLAGS*, SIZE*, REFIID, void**) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetImageStreamForRequestedIconSize(SIZE, SIZE, SIIGBF, UINT64, WTS_STREAMTYPE*, WTS_THUMBNAILTYPE*, WTS_CACHEFLAGS*, SIZE*, REFIID, void**) = 0;
    };

    HRESULT GetShellItemImage(HBITMAP& hBitmap, LPCWSTR filePath, int width, int height, bool hasThumbnail)
    {
        static const HINSTANCE hImageres = GetModuleHandleW(L"imageres.dll");
        static const HICON fallback = (HICON)LoadImageW(hImageres, MAKEINTRESOURCE(2), IMAGE_ICON, width * g_flScaleFactor, height * g_flScaleFactor, LR_SHARED);

        if (hBitmap)
        {
            DeleteObject(hBitmap);
            hBitmap = nullptr;
        }
        HRESULT hr{};

        ComPtr<IShellItem2> pShellItem{};
        hr = SHCreateItemFromParsingName(filePath, nullptr, IID_PPV_ARGS(&pShellItem));

        if (SUCCEEDED(hr) && pShellItem)
        {
            ComPtr<IShellItemImageFactoryPriv> pImageFactory{};
            hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
            if (SUCCEEDED(hr))
            {
                ComPtr<ISharedBitmap> pSharedBitmap;
                SIZE size = { width * g_flScaleFactor, height * g_flScaleFactor };
                if (pImageFactory)
                {
                    SIIGBF flags = SIIGBF_RESIZETOFIT;
                    if (!hasThumbnail) flags |= SIIGBF_ICONONLY;
                    hr = pImageFactory->GetSharedBitmap(size, flags, &pSharedBitmap);
                    if (SUCCEEDED(hr))
                    {
                        hr = pSharedBitmap->Detach(&hBitmap);
                    }
                }
            }
        }
        if (FAILED(hr)) IconToBitmap(fallback, hBitmap, width * g_flScaleFactor, height * g_flScaleFactor);

        return hr;
    }

    void GetFontHeight()
    {
        LOGFONTW lf{};
        RECT rc = { 0, 0, 100, 100 };
        HDC hdcBuffer = CreateCompatibleDC(nullptr);
        SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, g_dpi);
        DrawTextW(hdcBuffer, L" ", -1, &rc, DT_CENTER);
        GetTextMetricsW(hdcBuffer, &textm);
        DeleteDC(hdcBuffer);
    }

    float CalcTextLines(const wchar_t* str, int width)
    {
        HDC hdcBuffer = CreateCompatibleDC(nullptr);
        LOGFONTW lf{};
        SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, NULL, g_dpi);
        if (g_touchmode) lf.lfHeight *= 1.25;
        HFONT hFont = CreateFontIndirectW(&lf);
        HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);
        RECT rc = { 0, 0, width - 4, textm.tmHeight };
        wchar_t filenameBuffer[260]{};
        int lines_b1 = 1;
        int tilelines = 1;
        while (lines_b1 != 0)
        {
            rc.bottom = textm.tmHeight * tilelines;
            tilelines++;
            wcscpy_s(filenameBuffer, str);
            DWORD direction = (localeType == 1) ? DT_RIGHT : DT_LEFT;
            DWORD alignment = g_touchmode ? direction | DT_WORD_ELLIPSIS : DT_CENTER;
            DrawTextExW(hdcBuffer, filenameBuffer, -1, &rc, alignment | DT_MODIFYSTRING | DT_END_ELLIPSIS | DT_LVICON, nullptr);
            lines_b1 = wcscmp(str, filenameBuffer);
            if (!g_touchmode || tilelines > 5) break;
        }
        if (g_touchmode)
        {
            DeleteObject(hFont);
            DeleteObject(hOldFont);
            DeleteDC(hdcBuffer);
            return tilelines;
        }
        RECT rc2 = { 0, 0, width - 4, textm.tmHeight * 2 };
        wchar_t filenameBuffer2[260]{};
        wcscpy_s(filenameBuffer2, str);
        DrawTextExW(hdcBuffer, filenameBuffer2, -1, &rc2, DT_MODIFYSTRING | DT_WORD_ELLIPSIS | DT_CENTER | DT_LVICON, nullptr);
        int lines_b2 = wcscmp(str, filenameBuffer2);
        DeleteObject(hFont);
        DeleteObject(hOldFont);
        DeleteDC(hdcBuffer);
        if (lines_b1 == 1 && lines_b2 == 0) return 2.0;
        else if (lines_b1 == 1 && lines_b2 == 1) return 1.5;
        else return 1;
    }

    void CalcDesktopIconInfo(yValue* yV, int* lines_basedOnEllipsis, DWORD* alignment, bool subdirectory, vector<LVItem*>* pmLVItem)
    {
        if (!((*pmLVItem)[yV->num]->IsDestroyed()))
        {
            *alignment = DT_CENTER | DT_END_ELLIPSIS;
            if (!g_touchmode)
            {
                *lines_basedOnEllipsis = floor(CalcTextLines((*pmLVItem)[yV->num]->GetSimpleFilename().c_str(), yV->fl1 - 4 * g_flScaleFactor)) * textm.tmHeight;
            }
            if (g_touchmode)
            {
                DWORD direction = (localeType == 1) ? DT_RIGHT : DT_LEFT;
                *alignment = direction | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS;
                int maxlines_basedOnEllipsis{};
                if ((*pmLVItem)[yV->num]->GetText())
                {
                    maxlines_basedOnEllipsis = (*pmLVItem)[yV->num]->GetText()->GetHeight();
                    yV->fl1 = (*pmLVItem)[yV->num]->GetText()->GetWidth();
                }
                *lines_basedOnEllipsis = CalcTextLines((*pmLVItem)[yV->num]->GetSimpleFilename().c_str(), yV->fl1) * textm.tmHeight;
                if (*lines_basedOnEllipsis > maxlines_basedOnEllipsis) *lines_basedOnEllipsis = maxlines_basedOnEllipsis;
            }
        }
    }

    void SetPopupSize(Element* elem, int width, int height)
    {
        elem->SetWidth(width);
        elem->SetHeight(height);
    }

    void TriggerPageTransition(int direction, RECT& dimensions)
    {
        for (int items = 0; items < pm.size(); items++)
        {
            if (pm[items]->GetMemPage() == g_currentPageID)
            {
                pm[items]->SetVisible(!g_hiddenIcons);
                GTRANS_DESC transDesc[2];
                TransitionStoryboardInfo tsbInfo = {};
                TriggerTranslate(pm[items], transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 9999, pm[items]->GetY(), 9999, pm[items]->GetY(), false, false, false);
                TriggerTranslate(pm[items], transDesc, 1, 0.05f, 0.05f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX(), pm[items]->GetY(), false, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(pm[items], -1);
            }
            else if (DWMActive && !g_editmode && pm[items]->GetMemPage() == g_currentPageID - direction) pm[items]->SetVisible(!g_hiddenIcons);
            else pm[items]->SetVisible(false);
        }
        short animSrc = (localeType == 1) ? direction * -1 : direction;
        float originX = 0.5f + 0.28f * animSrc;
        float originXAmplified = 0.5f + 0.32f * animSrc;
        animSrc *= dimensions.right;
        GTRANS_DESC transDesc[5];
        TransitionStoryboardInfo tsbInfo = {};
        for (int items = 0; items < pm.size(); items++)
        {
            if (pm[items]->GetMemPage() == g_currentPageID - direction)
            {
                float offset = animSrc * -1.14f;
                TriggerTranslate(pm[items], transDesc, 0, 0.05f, 0.05f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX() + offset, pm[items]->GetY(), pm[items]->GetX() + offset, pm[items]->GetY(), false, false, false);
                TriggerFade(pm[items], transDesc, 1, 0.083f, 0.217f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                TriggerFade(pm[items], transDesc, 2, 0.33f, 0.33f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, false, false, true);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(pm[items], -1);
            }
        }
        TriggerScaleIn(UIContainer, transDesc, 0, 0.0f, 0.25f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.88f, 0.88f, 0.5f, 0.5f, false, false);
        TriggerTranslate(UIContainer, transDesc, 1, 0.05f, 0.55f, 0.1f, 0.9f, 0.2f, 1.0f, animSrc, 0.0f, 0.0f, 0.0f, false, false, false);
        TriggerScaleIn(UIContainer, transDesc, 2, 0.2f, 0.55f, 0.15f, 0.54f, 0.4f, 0.88f, 0.88f, 0.88f, 0.5f, 0.5f, 1.0f, 1.0f, originX, 0.5f, false, false);
        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
        DUI_SetGadgetZOrder(UIContainer, -1);
        Element* pageVisual[2];
        for (int i = 0; i < 2; i++)
        {
            static float fade, fadedelay, animSrc2;
            if (i == 0)
            {
                fade = 0.0f;
                fadedelay = 0.083f;
                animSrc2 = 0;
            }
            parser->CreateElement(L"pageVisual", nullptr, nullptr, nullptr, &pageVisual[i]);
            mainContainer->Add(&pageVisual[i], 1);
            pageVisual[i]->SetWidth(dimensions.right);
            pageVisual[i]->SetHeight(dimensions.bottom);
            TriggerFade(pageVisual[i], transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerScaleOut(pageVisual[i], transDesc, 1, 0.0f, 0.25f, 0.1f, 0.9f, 0.2f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, false);
            TriggerTranslate(pageVisual[i], transDesc, 2, 0.05f, 0.55f, 0.1f, 0.9f, 0.2f, 1.0f, animSrc2, 0.0f, animSrc * -1, 0.0f, false, false, false);
            TriggerFade(pageVisual[i], transDesc, 3, fadedelay, fadedelay + 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            TriggerScaleIn(pageVisual[i], transDesc, 4, 0.2f, 0.55f, 0.15f, 0.54f, 0.4f, 0.88f, 0.88f, 0.88f, 0.5f, 0.5f, 1.0f, 1.0f, originX, 0.5f, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pageVisual[i]->GetDisplayNode(), &tsbInfo);
            DUI_SetGadgetZOrder(pageVisual[i], -2);
            animSrc2 = animSrc;
            animSrc = 0;
            fade = 1.0f;
            fadedelay = 0.33f;
        }
    }

    DWORD WINAPI EndExplorer(LPVOID lpParam)
    {
        Sleep(250);
        HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
        DWORD pid{};
        GetWindowThreadProcessId(hWndProgman, &pid);
        HANDLE hExplorer = OpenProcess(PROCESS_TERMINATE, false, pid);
        TerminateProcess(hExplorer, 2);
        CloseHandle(hExplorer);
        return 0;
    }

    void AdjustWindowSizes(bool firsttime)
    {
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        POINT topLeftMon = GetTopLeftMonitor();
        UINT swpFlags = SWP_NOZORDER;
        SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
        if (firsttime) swpFlags |= SWP_NOMOVE | SWP_NOSIZE;
        if (localeType == 1)
        {
            int rightMon = GetRightMonitor();
            topLeftMon.x = dimensions.right + dimensions.left - rightMon;
        }

        HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");

        SetWindowPos(wnd->GetHWND(), nullptr, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
        SetWindowPos(subviewwnd->GetHWND(), nullptr, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
        if (editwnd)
        {
            SetWindowPos(editwnd->GetHWND(), nullptr, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
            //SetWindowPos(editbgwnd->GetHWND(), NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
        }
        SetWindowPos(hWndProgman, nullptr, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
        SetWindowPos(g_hWorkerW, nullptr, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
        SetWindowPos(g_hSHELLDLL_DefView, nullptr, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
        UIContainer->SetWidth(dimensions.right - dimensions.left);
        UIContainer->SetHeight(dimensions.bottom - dimensions.top);
        if (emptyspace)
        {
            emptyspace->SetWidth(dimensions.right - dimensions.left);
            emptyspace->SetHeight(dimensions.bottom - dimensions.top);
        }
        SetWindowPos(g_hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        int w = (int)((dimensions.right - dimensions.left) / g_flScaleFactor);
        int h = (int)((dimensions.bottom - dimensions.top) / g_flScaleFactor);
        if ((dimensions.right - dimensions.left != g_lastWidth || dimensions.bottom - dimensions.top != g_lastHeight) &&
            !(w <= g_defWidth + 10 && w >= g_defWidth - 10 && h <= g_defHeight + 10 && h >= g_defHeight - 10))
        {
            WCHAR notice[256];
            if (g_debugmode)
                StringCchPrintfW(notice, 256, L"%s", LoadStrFromRes(4098).c_str());
            else
                StringCchPrintfW(notice, 256, L"%s", LoadStrFromRes(4097).c_str());
            CSafeElementPtr<DDNotificationBanner> ddnb;
            ddnb.Assign(new DDNotificationBanner);
            ddnb->CreateBanner(DDNT_INFO, LoadStrFromRes(4096).c_str(), notice, 10);
            ddnb->AppendButton(LoadStrFromRes(4240, L"comctl32.dll").c_str(), SetDefaultRes, true);
            ddnb->AppendButton(LoadStrFromRes(4241, L"comctl32.dll").c_str(), nullptr, true);
        }
        else DDNotificationBanner::s_RepositionBanners();
    }

    LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
        int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor - textm.tmHeight;
        int iconPaddingX = (GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) - 48 * g_flScaleFactor) / 2;
        int iconPaddingY = (GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) - 48 * g_flScaleFactor) / 2;
        int i = 20002;
        if (g_menu)
        {
            LRESULT lResult{};
            if (SUCCEEDED(g_menu->HandleMenuMsg(uMsg, wParam, lParam, &lResult)))
                return lResult;
        }
        switch (uMsg)
        {
            case WM_SETTINGCHANGE:
            {
                if (wParam == SPI_SETWORKAREA && !g_ignoreWorkAreaChange)
                {
                    if (isDefaultRes()) SetPos(true);
                    AdjustWindowSizes(false);
                    SetTimer(hWnd, 11, 100, nullptr);
                }
                if (lParam && wcscmp((LPCWSTR)lParam, L"ShellState") == 0)
                {
                    RegKeyValue DDKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", nullptr, NULL);
                    g_showcheckboxes = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AutoCheckSelect");
                    g_showHidden = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"Hidden");
                    g_showSuperHidden = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"ShowSuperHidden");
                    g_hideFileExt = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"HideFileExt");
                    g_isThumbnailHidden = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"IconsOnly");
                    g_iconunderline = GetRegistryValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"IconUnderline");
                    free(shellstate);
                    GetRegistryBinValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState", &shellstate);
                    if (g_canRefreshMain)
                    {
                        SetTimer(wnd->GetHWND(), 10, 200, nullptr);
                        SetTimer(wnd->GetHWND(), 13, 600, nullptr);
                    }
                }
                if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)
                {
                    UpdateModeInfo();
                    // This message is sent 4-5 times upon changing accent color so this mitigation is applied
                    // 0.4.5.2 test case: seems to be sent 3-4 times. Maybe dependent on Windows install?
                    static int messagemitigation{};
                    SetTheme();
                    SetPos(false);
                    GetPos2(false);
                    if (g_themeOld != g_theme)
                    {
                        if (g_automaticDark)
                        {
                            g_isDarkIconsEnabled = !g_theme;
                            RearrangeIcons(false, true, true);
                        }
                        for (int j = 0; j < pm.size(); j++)
                        {
                            if (g_touchmode)
                            {
                                if (g_treatdirasgroup && pm[j]->GetGroupSize() == LVIGS_NORMAL && pm[j]->GetFlags() & LVIF_GROUP && pm[j]->GetIcon()->GetGroupColor() == 0)
                                {
                                    pm[j]->AddFlags(LVIF_REFRESH);
                                    yValue* yV = new yValue{ j };
                                    QueueUserWorkItem(RearrangeIconsHelper, yV, 0);
                                }
                            }
                            if (pm[j]->GetOpenDirState() == LVIODS_PINNED)
                            {
                                CSafeElementPtr<Element> groupdirectory;
                                groupdirectory.Assign(regElem<Element*>(L"groupdirectory", pm[j]));
                                StyleSheet* sheet = pSubview->GetSheet();
                                CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
                                parserSubview->GetSheet(g_theme ? L"popup" : L"popupdark", &sheetStorage);
                                groupdirectory->SetValue(Element::SheetProp, 1, sheetStorage);
                                if (!g_isColorized || pm[j]->GetIcon()->GetAssociatedColor() == 0 || pm[j]->GetIcon()->GetAssociatedColor() == -1)
                                    UpdateGroupOnColorChange(pm[j]->GetIcon(), DDScalableElement::AssociatedColorProp(), NULL, nullptr, nullptr); // to refresh neutrally colored ones
                            }
                        }
                    }
                    else if (g_isColorized)
                    {
                        messagemitigation++;
                        // 0.4.5.2: was originally "% 5"
                        // 0.5: was originally "== 2"
                        if (messagemitigation % 4 == 3)
                        {
                            RearrangeIcons(false, true, true);
                            for (int j = 0; j < pm.size(); j++)
                            {
                                if (pm[j]->GetOpenDirState() == LVIODS_PINNED || pm[j]->GetOpenDirState() == LVIODS_FULLSCREEN)
                                    if (pm[j]->GetIcon()->GetAssociatedColor() == 0 || pm[j]->GetIcon()->GetAssociatedColor() == -1)
                                        UpdateGroupOnColorChange(pm[j]->GetIcon(), DDScalableElement::AssociatedColorProp(), NULL, nullptr, nullptr); // to refresh neutrally colored ones
                            }
                            messagemitigation = 0;
                        }
                    }
                    g_themeOld = g_theme;
                }
                break;
            }
            case WM_CLOSE:
            {
                SetPos(isDefaultRes());
                subviewwnd->ShowWindow(SW_HIDE);
                if (lParam == 420)
                {
                    wchar_t* desktoplog = new wchar_t[260];
                    wchar_t* cBuffer = new wchar_t[260];
                    DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
                    StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
                    ShellExecuteW(nullptr, L"open", L"notepad.exe", desktoplog, nullptr, SW_SHOW);
                    delete[] desktoplog;
                    delete[] cBuffer;
                }
                if (lParam != 69)
                {
                    DWORD dwTermination{};
                    HANDLE termThread = CreateThread(nullptr, 0, EndExplorer, nullptr, 0, &dwTermination);
                    if (termThread) CloseHandle(termThread);
                }
                Sleep(500);
                pMain->Destroy(true);
                pSubview->Destroy(true);
                if (lParam == 69)
                {
                    HWND hSysListView32 = FindWindowExW(g_hSHELLDLL_DefView, nullptr, L"SysListView32", L"FolderView");
                    if (hSysListView32 && !g_hiddenIcons)
                        ShowWindow(hSysListView32, SW_SHOW);
                }
                //if (pEdit) pEdit->Destroy(true);
                StopMessagePump();
                break;
            }
            case WM_COMMAND:
            {
                break;
            }
            case WM_TIMER:
            {
                KillTimer(hWnd, wParam);
                GTRANS_DESC transDesc[4];
                TransitionStoryboardInfo tsbInfo = {};
                Event* iev = new Event{ nullptr, TouchButton::Click };
                CValuePtr v;
                DynamicArray<Element*>* Children;
                short lastWidth{}, lastHeight{};
                switch (wParam)
                {
                    case 1:
                        if (g_editmode) HideSimpleView(true);
                        else ShowSimpleView(true, 0x0);
                        break;
                    case 2:
                        InitLayout(true, true, true);
                        g_canRefreshMain = false;
                        break;
                    case 3:
                        break;
                    case 4:
                        switch (g_dialogopen)
                        {
                            case false:
                                DisplayShutdownDialog();
                                break;
                            case true:
                                DestroyShutdownDialog();
                                break;
                        }
                        break;
                    case 5:
                        if (abs((int)(GetTickCount64() - g_lastDpiChangeTick)) > 1500)
                        {
                            RearrangeIcons(!g_editmode, false, false);
                            g_ignoreWorkAreaChange = false;
                        }
                        break;
                    case 6:
                        MessageBeep(MB_OK);
                        if (g_editmode)
                        {
                            TriggerNoMorePagesOnEdit();
                            TriggerScaleIn(centeredE, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, 0.5f, 0.5f, 0.9f, 0.9f, 0.5f, 0.5f, false, false);
                            TriggerScaleIn(centeredE, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, centeredE->GetDisplayNode(), &tsbInfo);
                            DUI_SetGadgetZOrder(centeredE, -1);
                        }
                        else
                        {
                            Children = mainContainer->GetChildren(&v);
                            for (int i = 0; i < Children->GetSize(); i++)
                            {
                                if (Children->GetItem(i)->GetID() == StrToID(L"pageVisual"))
                                {
                                    TriggerFade(Children->GetItem(i), transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 3, transDesc, Children->GetItem(i)->GetDisplayNode(), &tsbInfo);
                                }
                            }
                            Element* pageVisual;
                            parser->CreateElement(L"pageVisual", nullptr, nullptr, nullptr, &pageVisual);
                            mainContainer->Add(&pageVisual, 1);
                            pageVisual->SetWidth(dimensions.right);
                            pageVisual->SetHeight(dimensions.bottom);
                            TriggerScaleIn(UIContainer, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 1.0f, 1.0f, 0.5f, 0.5f, 0.9f, 0.9f, 0.5f, 0.5f, false, false);
                            TriggerScaleIn(UIContainer, transDesc, 1, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
                            DUI_SetGadgetZOrder(UIContainer, -1);
                            TriggerFade(pageVisual, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, true);
                            TriggerScaleOut(pageVisual, transDesc, 1, 0.0f, 0.25f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, false, false);
                            TriggerFade(pageVisual, transDesc, 2, 0.267f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                            TriggerScaleIn(pageVisual, transDesc, 3, 0.3f, 0.5f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, true);
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pageVisual->GetDisplayNode(), &tsbInfo);
                            DUI_SetGadgetZOrder(pageVisual, -2);
                        }
                        break;
                    case 7:
                        SetForegroundWindow(subviewwnd->GetHWND());
                        break;
                    case 8:
                        GoToPrevPage(prevpageMain, iev);
                        break;
                    case 9:
                        GoToNextPage(nextpageMain, iev);
                        break;
                    case 10:
                        InitLayout(false, false, true);
                        g_ignoreWorkAreaChange = false;
                        g_canRefreshMain = false;
                        break;
                    case 11:
                        lastWidth = g_lastWidth, lastHeight = g_lastHeight;
                        g_lastWidth = 0, g_lastHeight = 0;
                        if (dimensions.right - dimensions.left != lastWidth || dimensions.bottom - dimensions.top != lastHeight)
                            RearrangeIcons(!g_editmode, false, true);
                        else
                        {
                            g_lastWidth = dimensions.right - dimensions.left;
                            g_lastHeight = dimensions.bottom - dimensions.top;
                        }
                        break;
                    case 12:
                        g_setcolors = true;
                        break;
                    case 13:
                        g_canRefreshMain = true;
                        break;
                    case 14:
                    case 15:
                        if (g_editmode) HideSimpleView(false);
                        CreateSearchPage(wParam - 14);
                        break;
                    case 16:
                        InitLayout(true, false, false);
                        g_canRefreshMain = false;
                        break;
                }
                delete iev;
                break;
            }
            case WM_USER + 1:
            {
                for (LVItem* lvi : pm)
                {
                    if (lvi->GetPage() == g_currentPageID)
                    {
                        if (lvi->GetFlags() & LVIF_FLYING)
                        {
                            int coef = g_launch ? 2 : 3;
                            float delay = (lvi->GetY() + lvi->GetHeight() / 2) / static_cast<float>(dimensions.bottom * coef);
                            float startXPos = ((dimensions.right / 2.0f) - (lvi->GetX() + (lvi->GetWidth() / 2))) * 0.2f;
                            float startYPos = ((dimensions.bottom / 2.0f) - (lvi->GetY() + (lvi->GetHeight() / 2))) * 0.2f;
                            float scale = g_launch ? 1.33f : 0.67f;
                            GTRANS_DESC transDesc[3];
                            TriggerTranslate(lvi, transDesc, 0, delay, delay + scale, 0.1f, 0.9f, 0.2f, 1.0f, lvi->GetX() + startXPos, lvi->GetY() + startYPos, lvi->GetX(), lvi->GetY(), false, false, false);
                            TriggerFade(lvi, transDesc, 1, delay, delay + 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                            TriggerScaleIn(lvi, transDesc, 2, delay, delay + scale, 0.1f, 0.9f, 0.2f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                            TransitionStoryboardInfo tsbInfo = {};
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, lvi->GetDisplayNode(), &tsbInfo);
                            lvi->RemoveFlags(LVIF_FLYING);
                        }
                        else if (lvi->GetFlags() & LVIF_MOVING)
                        {
                            GTRANS_DESC transDesc[1];
                            TransitionStoryboardInfo tsbInfo = {};
                            if (lvi->GetPreRefreshMemPage() == lvi->GetPage())
                            {
                                TriggerTranslate(lvi, transDesc, 0, 0.0f, 0.4f, 0.75f, 0.45f, 0.0f, 1.0f, lvi->GetX(), lvi->GetY(), lvi->GetMemXPos(), lvi->GetMemYPos(), false, false, false);
                                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, lvi->GetDisplayNode(), &tsbInfo);
                                if (lvi->GetGroupSize() == LVIGS_NORMAL && lvi->GetMemIconSize() != g_iconsz && !g_touchmode)
                                {
                                    CSafeElementPtr<DDScalableElement> icon; icon.Assign(regElem<DDScalableElement*>(L"iconElem", lvi));
                                    float scaleOrigX = lvi->GetX() / static_cast<float>(dimensions.right);
                                    float scaleOrigY = lvi->GetY() / static_cast<float>(dimensions.bottom);
                                    float scaling = lvi->GetMemIconSize() / static_cast<float>(g_iconsz);
                                    TriggerScaleIn(icon, transDesc, 0, 0.0f, 0.4f, 0.75f, 0.45f, 0.0f, 1.0f, scaling, scaling, scaleOrigX, scaleOrigY, 1.0f, 1.0f, scaleOrigX, scaleOrigY, false, false);
                                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, icon->GetDisplayNode(), &tsbInfo);
                                    DUI_SetGadgetZOrder(icon, -2);
                                }
                            }
                            else
                            {
                                TriggerFade(lvi, transDesc, 0, 0.0f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, lvi->GetDisplayNode(), &tsbInfo);
                            }
                            DWORD animCoef = g_animCoef;
                            if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                            DWORD dwDEA = DWMActive ? 400 * (animCoef / 100.0f) : 0;
                            DelayedElementActions* dea = new DelayedElementActions{ dwDEA, lvi, static_cast<float>(lvi->GetMemXPos()), static_cast<float>(lvi->GetMemYPos()) };
                            DWORD DelayedSetPos;
                            HANDLE hDelayedSetPos = CreateThread(nullptr, 0, SetElemPos, dea, NULL, nullptr);
                            if (hDelayedSetPos) CloseHandle(hDelayedSetPos);
                            lvi->RemoveFlags(LVIF_MOVING);
                        }
                        DUI_SetGadgetZOrder(lvi, -1);
                    }
                    lvi->SetVisible(lvi->GetPage() == g_currentPageID && !g_hiddenIcons);
                    lvi->SetMemIconSize(g_iconsz);
                }
                if (g_launch) g_launch = false;
                break;
            }
            case WM_USER + 2:
            {
                static int icons;
                ++icons;
                if (icons == pm.size())
                {
                    SendMessageW(hWnd, WM_USER + 1, NULL, NULL);
                    icons = 0;
                }
                break;
            }
            case WM_USER + 3:
            {
                int lines_basedOnEllipsis{};
                pm[lParam]->ClearAllListeners();
                vector<IElementListener*> v_pels;
                DDScalableElement* peInner = pm[lParam]->GetInnerElement();
                DDScalableElement* peIcon = pm[lParam]->GetIcon();
                Element* peShortcutArrow = pm[lParam]->GetShortcutArrow();
                RichText* peText = pm[lParam]->GetText();
                TouchButton* peCheckbox = pm[lParam]->GetCheckbox();
                v_pels.push_back(assignExtendedFn(pm[lParam], ItemDragListener, true));
                v_pels.push_back(assignFn(pm[lParam], ItemRightClick, true));
                v_pels.push_back(assignExtendedFn(peIcon, UpdateGroupOnColorChange, true));
                if (!g_treatdirasgroup || pm[lParam]->GetGroupSize() == LVIGS_NORMAL)
                {
                    v_pels.push_back(assignFn(pm[lParam], SelectItem, true));
                    v_pels.push_back(assignExtendedFn(pm[lParam], SelectItemListener, true));
                    v_pels.push_back(assignExtendedFn(pm[lParam], ShowCheckboxIfNeeded, true));
                    v_pels.push_back(assignExtendedFn(peCheckbox, CheckboxHandler, true));
                }
                CSafeElementPtr<Element> groupdirectoryOld;
                groupdirectoryOld.Assign(regElem<Element*>(L"groupdirectory", pm[lParam]));
                if (groupdirectoryOld)
                {
                    groupdirectoryOld->Destroy(true);
                    if (pm[lParam]->GetChildItems())
                    {
                        if (pm[lParam]->GetChildItems()->size() > 0)
                        {
                            pm[lParam]->GetChildItems()->clear();
                            pm[lParam]->SetChildItems(nullptr);
                        }
                    }
                }
                if (g_treatdirasgroup && pm[lParam]->GetGroupSize() != LVIGS_NORMAL)
                {
                    peIcon->SetX(0);
                    peIcon->SetY(0);
                    pm[lParam]->SetSelected(false);
                    if (localeType == 1)
                    {
                        switch (pm[lParam]->GetGroupSize())
                        {
                        case LVIGS_SMALL:
                            pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 316);
                            break;
                        case LVIGS_MEDIUM:
                            pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 476);
                            break;
                        case LVIGS_WIDE:
                            pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 716);
                            break;
                        case LVIGS_LARGE:
                            pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 716);
                            break;
                        }
                    }
                    pm[lParam]->SetTooltip(false);
                    peInner->SetLayoutPos(-3);
                    peCheckbox->SetLayoutPos(-3);
                    peText->SetLayoutPos(-3);
                    if (g_touchmode)
                    {
                        CSafeElementPtr<Element> containerElem;
                        containerElem.Assign(regElem<Element*>(L"containerElem", pm[lParam]));
                        containerElem->SetPadding(0, 0, 0, 0);
                    }
                    pm[lParam]->SetBackgroundColor(0);
                    pm[lParam]->SetDrawType(0);
                    ShowDirAsGroupDesktop(pm[lParam], true);
                }
                else
                {
                    CSafeElementPtr<Element> g_innerElem;
                    g_innerElem.Assign(regElem<Element*>(L"innerElem", g_outerElem));
                    CSafeElementPtr<Element> checkboxElem;
                    checkboxElem.Assign(regElem<Element*>(L"checkboxElem", g_outerElem));
                    peInner->SetLayoutPos(g_innerElem->GetLayoutPos()), pm[lParam]->GetCheckbox()->SetLayoutPos(checkboxElem->GetLayoutPos());
                    if (g_touchmode)
                    {
                        // had to hardcode it as GetPadding is VERY unreliable on high dpi
                        short space = 6 * g_flScaleFactor;
                        CSafeElementPtr<Element> containerElem;
                        containerElem.Assign(regElem<Element*>(L"containerElem", pm[lParam]));
                        containerElem->SetPadding(space, space, space, space);
                        if (pm[lParam]->GetTileSize() == LVITS_ICONONLY) peText->SetVisible(false);
                        if (pm[lParam]->GetFlags() & LVIF_SFG)
                        {
                            pm[lParam]->SetWidth(g_touchSizeX);
                            pm[lParam]->SetHeight(g_touchSizeY);
                        }
                    }
                    else
                    {
                        lines_basedOnEllipsis = floor(CalcTextLines(pm[lParam]->GetSimpleFilename().c_str(), innerSizeX - 4 * g_flScaleFactor)) * textm.tmHeight;
                        pm[lParam]->SetWidth(innerSizeX);
                        pm[lParam]->SetHeight(innerSizeY + lines_basedOnEllipsis + 6 * g_flScaleFactor);
                        peText->SetHeight(lines_basedOnEllipsis + 5 * g_flScaleFactor);
                        short shadedSize{}, shadedX{}, shadedY{};
                        if (!(pm[lParam]->GetFlags() & LVIF_HIDDEN) && (!g_treatdirasgroup || !(pm[lParam]->GetFlags() & LVIF_GROUP)))
                        {
                            shadedSize = 16;
                            shadedX = 8 * g_flScaleFactor;
                            shadedY = 6 * g_flScaleFactor;
                        }
                        peIcon->SetWidth((g_iconsz + shadedSize) * g_flScaleFactor);
                        peIcon->SetHeight((g_iconsz + shadedSize) * g_flScaleFactor);
                        peIcon->SetX(iconPaddingX - shadedX);
                        peIcon->SetY((iconPaddingY * 0.575) - shadedY);
                        peShortcutArrow->SetWidth(g_shiconsz * g_flScaleFactor);
                        peShortcutArrow->SetHeight(g_shiconsz * g_flScaleFactor);
                        peShortcutArrow->SetX(iconPaddingX);
                        peShortcutArrow->SetY((iconPaddingY * 0.575) + (g_iconsz - g_shiconsz) * g_flScaleFactor);
                    }
                    SelectItemListener(pm[lParam], Element::SelectedProp(), 69, nullptr, nullptr);
                    pm[lParam]->RemoveFlags(LVIF_SFG);
                }
                pm[lParam]->SetListeners(v_pels);
                v_pels.clear();
                static int icons;
                ++icons;
                if (icons == pm.size())
                {
                    SendMessageW(hWnd, WM_USER + 1, NULL, NULL);
                    icons = 0;
                }
                break;
            }
            case WM_USER + 4:
            {
                DDScalableElement* peIcon = pm[lParam]->GetIcon();
                Element* peShortcutArrow = pm[lParam]->GetShortcutArrow();
                RichText* peText = pm[lParam]->GetText();
                HBITMAP iconbmp = ((DesktopIcon*)wParam)->icon;
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconbmp);
                if (spvBitmap) peIcon->SetValue(Element::ContentProp, 1, spvBitmap);
                HBITMAP iconshortcutbmp = ((DesktopIcon*)wParam)->iconshortcut;
                CValuePtr spvBitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshortcutbmp);
                DWORD lviFlags = pm[lParam]->GetFlags();
                if (spvBitmapShortcut && lviFlags & LVIF_SHORTCUT)
                    peShortcutArrow->SetValue(Element::ContentProp, 1, spvBitmapShortcut);
                if (g_touchmode)
                {
                    HBITMAP textbmp = ((DesktopIcon*)wParam)->text;
                    CValuePtr spvBitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
                    DeleteObject(textbmp);
                    if (spvBitmapText) peText->SetValue(Element::ContentProp, 1, spvBitmapText);
                    BYTE intensity = (lviFlags & LVIF_HIDDEN) ? g_isGlass ? 16 : 192 : g_isGlass ? 128 : 255;
                    pm[lParam]->SetDDCPIntensity(intensity);
                    pm[lParam]->SetAssociatedColor(((DesktopIcon*)wParam)->crDominantTile);
                    pm[lParam]->GetInnerElement()->SetAssociatedColor(CreateGlowColor(((DesktopIcon*)wParam)->crDominantTile));
                    if (lviFlags & LVIF_HIDDEN)
                    {
                        short iconspace = 8 * g_flScaleFactor;
                        peIcon->SetPadding(iconspace, iconspace, iconspace, iconspace);
                    }
                }
                break;
            }
            case WM_USER + 5:
            {
                WCHAR *cxDragStr{}, *cyDragStr{};
                GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragWidth", &cxDragStr);
                GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragHeight", &cyDragStr);
                static const int dragWidth = _wtoi(cxDragStr);
                static const int dragHeight = _wtoi(cyDragStr);
                free(cxDragStr), free(cyDragStr);
                POINT ppt;
                GetCursorPos(&ppt);
                ScreenToClient(wnd->GetHWND(), &ppt);
                if (localeType == 1) ppt.x = dimensions.right - ppt.x;
                if (abs(ppt.x - origX) > dragWidth || abs(ppt.y - origY) > dragHeight)
                {
                    g_emptyclicks = 1;
                }
                MARGINS borders = {
                    (ppt.x < origX) ? ppt.x : origX, abs(ppt.x - origX),
                    (ppt.y < origY) ? ppt.y : origY, abs(ppt.y - origY)
                };
                if (borders.cxRightWidth == 0) borders.cxRightWidth = 1;
                if (borders.cyBottomHeight == 0) borders.cyBottomHeight = 1;
                selector->SetWidth(borders.cxRightWidth);
                selector->SetX(borders.cxLeftWidth);
                selector->SetHeight(borders.cyBottomHeight);
                selector->SetY(borders.cyTopHeight);
                for (int items = 0; items < pm.size(); items++)
                {
                    MARGINS iconborders = { pm[items]->GetX(), pm[items]->GetX() + pm[items]->GetWidth(), pm[items]->GetY(), pm[items]->GetY() + pm[items]->GetHeight() };
                    bool selectstate = (borders.cxRightWidth + borders.cxLeftWidth > iconborders.cxLeftWidth &&
                        iconborders.cxRightWidth > borders.cxLeftWidth &&
                        borders.cyBottomHeight + borders.cyTopHeight > iconborders.cyTopHeight &&
                        iconborders.cyBottomHeight > borders.cyTopHeight);
                    if (pm[items]->GetPage() == g_currentPageID && !(g_treatdirasgroup && pm[items]->GetGroupSize() != LVIGS_NORMAL))
                        pm[items]->SetSelected(selectstate);
                }
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
            case WM_USER + 8:
            {
                break;
            }
            case WM_USER + 9:
            {
                break;
            }
            case WM_USER + 10:
            {
                break;
            }
            case WM_USER + 11:
            {
                break;
            }
            case WM_USER + 12:
            {
                break;
            }
            case WM_USER + 13:
            {
                break;
            }
            case WM_USER + 14:
            {
                vector<HANDLE> smThumbnailThreadHandle(pm.size(), nullptr);
                for (int icon = 0; icon < pm.size(); icon++)
                {
                    IconThumbHelper(icon);
                }
                for (int icon2 = 0; icon2 < pm.size(); icon2++)
                {
                    yValue* yV = new (nothrow) yValue{ icon2 };
                    smThumbnailThreadHandle[icon2] = CreateThread(nullptr, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, nullptr);
                    if (smThumbnailThreadHandle[icon2]) CloseHandle(smThumbnailThreadHandle[icon2]);
                }
                smThumbnailThreadHandle.clear();
                break;
            }
            case WM_USER + 15:
            {
                break;
            }
            case WM_USER + 16:
            {
                ThumbnailIcon* ti = (ThumbnailIcon*)wParam;
                HBITMAP thumbIcon = ti->icon;
                CValuePtr spvThumbIcon = DirectUI::Value::CreateGraphic(thumbIcon, 2, 0xffffffff, false, false, false);
                Element* GroupedIcon{};
                parser->CreateElement(L"GroupedIcon", nullptr, nullptr, nullptr, (Element**)&GroupedIcon);
                pm[lParam]->GetIcon()->Add((Element**)&GroupedIcon, 1);
                GroupedIcon->SetWidth(g_gpiconsz * g_flScaleFactor), GroupedIcon->SetHeight(g_gpiconsz * g_flScaleFactor);
                GroupedIcon->SetX(ti->x), GroupedIcon->SetY(ti->y);
                if (ti->str.GetHiddenState()) GroupedIcon->SetAlpha(128);
                if (spvThumbIcon != nullptr) GroupedIcon->SetValue(Element::ContentProp, 1, spvThumbIcon);
                DeleteObject(thumbIcon);
                delete ti;
                break;
            }
            case WM_USER + 17:
            {
                static int dragToPrev{}, dragToNext{};
                CValuePtr v;
                POINT ppt;
                GetCursorPos(&ppt);
                ScreenToClient(wnd->GetHWND(), &ppt);
                WCHAR *cxDragStr{}, *cyDragStr{};
                GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragWidth", &cxDragStr);
                GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragHeight", &cyDragStr);
                static const int dragWidth = _wtoi(cxDragStr);
                static const int dragHeight = _wtoi(cyDragStr);
                free(cxDragStr), free(cyDragStr);
                vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
                if (abs(ppt.x - ((POINT*)lParam)->x) > dragWidth || abs(ppt.y - ((POINT*)lParam)->y) > dragHeight)
                {
                    internalselectedLVItems[0]->AddFlags(LVIF_DRAG);
                    g_dragpreview->SetVisible(!g_lockiconpos);
                }
                if (ppt.x < 16 * g_flScaleFactor) dragToPrev++;
                else dragToPrev = 0;
                if (ppt.x > dimensions.right - 16 * g_flScaleFactor) dragToNext++;
                else dragToNext = 0;
                if (dragToPrev > 50 && g_currentPageID > 1)
                {
                    g_currentPageID--;
                    short animSrc = (localeType == 1) ? 1 : -1;
                    animSrc *= dimensions.right;
                    for (int i = 0; i < internalselectedLVItems.size(); i++)
                        internalselectedLVItems[i]->SetPage(g_currentPageID);
                    TriggerPageTransition(-1, dimensions);
                    nextpageMain->SetVisible(true);
                    if (g_currentPageID == 1) prevpageMain->SetVisible(false);
                    dragToPrev = 18;
                }
                if (dragToNext > 50 && g_currentPageID < g_maxPageID)
                {
                    g_currentPageID++;
                    short animSrc = (localeType == 1) ? -1 : 1;
                    animSrc *= dimensions.right;
                    for (int i = 0; i < internalselectedLVItems.size(); i++)
                        internalselectedLVItems[i]->SetPage(g_currentPageID);
                    TriggerPageTransition(1, dimensions);
                    prevpageMain->SetVisible(true);
                    if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
                    dragToNext = 18;
                }
                if (localeType == 1) ppt.x = dimensions.right - ppt.x;
                g_dragpreview->SetX(ppt.x - origX);
                g_dragpreview->SetY(ppt.y - origY);
                if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) && isIconPressed)
                {
                    isIconPressed = 0;
                }
                break;
            }
            case WM_USER + 18:
            {
                switch (lParam)
                {
                    case 0:
                    {
                        if (wParam != 0)
                        {
                            vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
                            if (internalselectedLVItems[0]->GetFlags() & LVIF_DRAG)
                            {
                                POINT ppt;
                                CValuePtr v;
                                GetCursorPos(&ppt);
                                ScreenToClient(wnd->GetHWND(), &ppt);
                                short localeDirection = (localeType == 1) ? -1 : 1;
                                short outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 44) * g_flScaleFactor;
                                short outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 22) * g_flScaleFactor;
                                short desktoppadding = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH : DESKPADDING_NORMAL);
                                short desktoppadding_x = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH_X : DESKPADDING_NORMAL_X);
                                short desktoppadding_y = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH_Y : DESKPADDING_NORMAL_Y);
                                if (g_touchmode)
                                {
                                    outerSizeX = g_touchSizeX + desktoppadding;
                                    outerSizeY = g_touchSizeY + desktoppadding;
                                }
                                short largestXPos = (dimensions.right - (2 * desktoppadding_x) + desktoppadding) / outerSizeX;
                                short largestYPos = (dimensions.bottom - (2 * desktoppadding_y) + desktoppadding) / outerSizeY;
                                if (largestXPos == 0) largestXPos = 1;
                                if (largestYPos == 0) largestYPos = 1;
                                short desiredWidthMain = internalselectedLVItems[0]->GetWidth();
                                if (g_touchmode)
                                {
                                    desiredWidthMain = g_touchSizeX;
                                    desktoppadding_x = (dimensions.right - largestXPos * outerSizeX + desktoppadding) / 2;
                                    desktoppadding_y = (dimensions.bottom - largestYPos * outerSizeY + desktoppadding) / 2;
                                }
                                short xRender = ppt.x - origX - desktoppadding_x;
                                if (localeType == 1)
                                {
                                    xRender = ppt.x + origX - desktoppadding_x - desiredWidthMain;
                                }
                                short paddingmitigation = (localeType == 1) ? desktoppadding : 0;
                                short destX = desktoppadding_x + round(xRender / static_cast<float>(outerSizeX)) * outerSizeX;
                                short destY = desktoppadding_y + round((ppt.y - origY - desktoppadding_y) / static_cast<float>(outerSizeY)) * outerSizeY;
                                if (localeType == 1)
                                {
                                    destX = dimensions.right - destX - desiredWidthMain;
                                }
                                short mainElementX = internalselectedLVItems[0]->GetMemXPos();
                                short mainElementY = internalselectedLVItems[0]->GetMemYPos();
                                if (g_touchmode && internalselectedLVItems[0]->GetTileSize() == LVITS_ICONONLY)
                                {
                                    if (localeType == 1) mainElementX -= outerSizeX / 2;
                                    BYTE smallPos = internalselectedLVItems[0]->GetSmallPos() - 1;
                                    mainElementY -= (outerSizeY / 2) * (smallPos / 2);
                                    if (smallPos & 1)
                                        mainElementX -= outerSizeX / 2 * localeDirection;
                                }
                                short itemstodrag = internalselectedLVItems.size();
                                for (short items = 0; items < itemstodrag; items++)
                                {
                                    internalselectedLVItems[items]->AddFlags(LVIF_DRAG);
                                }
                                for (short items = 0; items < itemstodrag; items++)
                                {
                                    short finaldestX = destX - mainElementX + internalselectedLVItems[items]->GetMemXPos();
                                    short finaldestY = destY - mainElementY + internalselectedLVItems[items]->GetMemYPos();
                                    short desiredWidth = internalselectedLVItems[items]->GetWidth();
                                    short desiredHeight = internalselectedLVItems[items]->GetHeight();
                                    if (g_touchmode && internalselectedLVItems[items]->GetTileSize() == LVITS_ICONONLY)
                                    {
                                        desiredWidth = g_touchSizeX;
                                        desiredHeight = g_touchSizeY;
                                        BYTE smallPos = internalselectedLVItems[items]->GetSmallPos() - 1;
                                        finaldestY -= (outerSizeY / 2) * (smallPos / 2);
                                        if (smallPos & 1)
                                            finaldestX -= outerSizeX / 2 * localeDirection;
                                    }
                                    if (localeType == 1)
                                    {
                                        if (finaldestX < desktoppadding_x) finaldestX = dimensions.right - round((dimensions.right - outerSizeX) / static_cast<float>(outerSizeX)) * outerSizeX - desktoppadding_x + desktoppadding;
                                        if (finaldestX > dimensions.right - desiredWidth + desktoppadding_x) finaldestX = dimensions.right - desiredWidth - desktoppadding_x;
                                    }
                                    else
                                    {
                                        if (finaldestX < desktoppadding_x) finaldestX = desktoppadding_x;
                                        if (finaldestX > dimensions.right - desiredWidth - desktoppadding_x) finaldestX = round((dimensions.right - desiredWidth) / static_cast<float>(outerSizeX)) * outerSizeX - outerSizeX + desktoppadding_x;
                                    }
                                    if (finaldestY < desktoppadding_y) finaldestY = desktoppadding_y;
                                    if (finaldestY > dimensions.bottom - desiredHeight - desktoppadding_y - desktoppadding) finaldestY = round((dimensions.bottom - desiredHeight - 2 * desktoppadding_y) / static_cast<float>(outerSizeY)) * outerSizeY + desktoppadding_y;
                                    short saveddestX = (localeType == 1) ? dimensions.right - finaldestX - internalselectedLVItems[items]->GetWidth() - desktoppadding_x : finaldestX - desktoppadding_x;
                                    short textheight = g_touchmode ? 0 : internalselectedLVItems[items]->GetText()->GetHeight();
                                    for (short items2 = 0; items2 < pm.size(); items2++)
                                    {
                                        bool existingTouchGrid = false;
                                        short textheight2 = g_touchmode ? 0 : pm[items2]->GetText()->GetHeight();
                                        if (pm[items2]->GetMemXPos() + pm[items2]->GetWidth() >= finaldestX &&
                                            pm[items2]->GetMemXPos() <= finaldestX + internalselectedLVItems[items]->GetWidth() &&
                                            pm[items2]->GetMemYPos() + pm[items2]->GetHeight() - textheight2 >= finaldestY &&
                                            pm[items2]->GetMemYPos() <= finaldestY + internalselectedLVItems[items]->GetHeight() - textheight &&
                                            pm[items2]->GetPage() == internalselectedLVItems[items]->GetPage() &&
                                            (!(pm[items2]->GetFlags() & LVIF_DRAG) || g_touchmode && internalselectedLVItems[items]->GetTileSize() == LVITS_ICONONLY))
                                        {
                                            if (g_touchmode && internalselectedLVItems[items]->GetTileSize() == LVITS_ICONONLY &&
                                                pm[items2]->GetTileSize() == LVITS_ICONONLY && pm[items2]->GetTouchGrid()->GetItemCount() < 4 &&
                                                (internalselectedLVItems[items]->GetInternalXPos() != (saveddestX / outerSizeX) ||
                                                    internalselectedLVItems[items]->GetInternalYPos() != ((finaldestY - desktoppadding_y) / outerSizeY)))
                                            {
                                                internalselectedLVItems[items]->SetTouchGrid(pm[items2]->GetTouchGrid());
                                                existingTouchGrid = true;
                                                items2 = pm.size() - 1;
                                            }
                                            else break;
                                        }
                                        if (items2 == pm.size() - 1)
                                        {
                                            if (g_touchmode && internalselectedLVItems[items]->GetTileSize() == LVITS_ICONONLY)
                                            {
                                                if (!existingTouchGrid)
                                                {
                                                    internalselectedLVItems[items]->SetMemXPos(finaldestX);
                                                    internalselectedLVItems[items]->SetMemYPos(finaldestY);
                                                    internalselectedLVItems[items]->SetTouchGrid(new LVItemTouchGrid);
                                                }
                                            }
                                            else
                                            {
                                                internalselectedLVItems[items]->SetMemXPos(finaldestX);
                                                internalselectedLVItems[items]->SetMemYPos(finaldestY);
                                                DWORD animCoef = g_animCoef;
                                                if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                                                DWORD dwDEA = DWMActive ? 400 * (animCoef / 100.0f) : 0;
                                                DelayedElementActions* dea = new DelayedElementActions{ dwDEA, internalselectedLVItems[items], static_cast<float>(finaldestX), static_cast<float>(finaldestY) };
                                                DWORD DelayedSetPos;
                                                HANDLE hDelayedSetPos = CreateThread(nullptr, 0, SetElemPos, dea, NULL, nullptr);
                                                if (hDelayedSetPos) CloseHandle(hDelayedSetPos);
                                                GTRANS_DESC transDesc[1];
                                                TransitionStoryboardInfo tsbInfo = {};
                                                TriggerTranslate(internalselectedLVItems[items], transDesc, 0, 0.0f, 0.4f, 0.75f, 0.45f, 0.0f, 1.0f,
                                                    internalselectedLVItems[items]->GetX() - (g_currentPageID - internalselectedLVItems[items]->GetMemPage()) * dimensions.right * localeDirection,
                                                    internalselectedLVItems[items]->GetY(), finaldestX, finaldestY, false, false, false);
                                                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, internalselectedLVItems[items]->GetDisplayNode(), &tsbInfo);
                                                DUI_SetGadgetZOrder(internalselectedLVItems[items], -1);
                                            }
                                            internalselectedLVItems[items]->SetInternalXPos(saveddestX / outerSizeX);
                                            internalselectedLVItems[items]->SetInternalYPos((finaldestY - desktoppadding_y) / outerSizeY);
                                            internalselectedLVItems[items]->SetMemPage(internalselectedLVItems[items]->GetPage());
                                            internalselectedLVItems[items]->SetVisible(true);
                                        }
                                    }
                                }
                            }
                            for (LVItem* lvi : internalselectedLVItems)
                            {
                                lvi->RemoveFlags(LVIF_DRAG);
                            }
                            internalselectedLVItems.clear();
                        }
                        g_dragpreview->SetVisible(false);
                        break;
                    }
                    case 1:
                    {
                        if (wParam != NULL)
                        {
                            LVItem* item = (*(vector<LVItem*>*)wParam)[0];
                            item->RemoveFlags(LVIF_DRAG);
                        }
                        break;
                    }
                    case 2:
                    {
                        vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
                        LVItem* item = internalselectedLVItems[0];
                        item->RemoveFlags(LVIF_DRAG);
                        internalselectedLVItems.clear();
                        g_dragpreview->SetVisible(false);
                        MessageBeep(MB_OK);
                        CSafeElementPtr<DDNotificationBanner> ddnb;
                        ddnb.Assign(new DDNotificationBanner);
                        ddnb->CreateBanner(DDNT_INFO, LoadStrFromRes(4044).c_str(), LoadStrFromRes(4045).c_str(), 5);
                        break;
                    }
                }
                break;
            }
            case WM_USER + 19:
            {
                if (delayedshutdownstatuses[lParam - 1] == false) break;
                switch (lParam)
                {
                    case 1:
                    {
                        WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, FALSE);
                        break;
                    }
                    case 2:
                    {
                        ExitWindowsEx(EWX_LOGOFF, 0);
                        break;
                    }
                    case 3:
                    {
                        SetSuspendState(FALSE, FALSE, FALSE);
                        break;
                    }
                    case 4:
                    {
                        SetSuspendState(TRUE, FALSE, FALSE);
                        break;
                    }
                    case 5:
                    {
                        ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF, shutdownReason);
                        break;
                    }
                    case 6:
                    {
                        ExitWindowsEx(EWX_REBOOT, shutdownReason);
                        break;
                    }
                }
                delayedshutdownstatuses[lParam - 1] = false;
                break;
            }
            case WM_USER + 20:
            {
                LVItem* outerElem;
                yValue* yV = (yValue*)wParam;
                FileInfo* fi = (FileInfo*)lParam;
                if (g_touchmode)
                {
                    parser->CreateElement(L"outerElemTouch", nullptr, nullptr, nullptr, (Element**)&outerElem);
                }
                else parser->CreateElement(L"outerElem", nullptr, nullptr, nullptr, (Element**)&outerElem);
                CSafeElementPtr<DDScalableElement> iconElem;
                iconElem.Assign(regElem<DDScalableElement*>(L"iconElem", outerElem));
                CSafeElementPtr<RichText> textElem;
                textElem.Assign(regElem<RichText*>(L"textElem", outerElem));

                int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
                int isThumbnailHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"IconsOnly");
                wstring foundfilename = (wstring)L"\"" + fi->filepath + (wstring)L"\\" + fi->filename + (wstring)L"\"";
                DWORD attr = GetFileAttributesW(RemoveQuotes(foundfilename).c_str());
                wstring foundsimplefilename = hideExt((wstring)fi->filename, isFileExtHidden, (attr & 16), outerElem);
                if (attr & 16)
                {
                    outerElem->AddFlags(LVIF_DIR);
                    unsigned short itemsInside = EnumerateFolder_Helper((LPWSTR)RemoveQuotes(foundfilename).c_str());
                    if (itemsInside <= 192) outerElem->AddFlags(LVIF_GROUP);
                    outerElem->SetItemCount(itemsInside);
                }
                if (attr & 2) outerElem->AddFlags(LVIF_HIDDEN);
                if (isThumbnailHidden == 0)
                {
                    bool image;
                    isSpecialProp(foundfilename, true, &image, &imageExts);
                    if (image) outerElem->AddFlags(LVIF_COLORLOCK);
                }
                bool advancedicon;
                isSpecialProp(foundfilename, true, &advancedicon, &advancedIconExts);
                outerElem->AddFlags(LVIF_ADVANCEDICON);
                outerElem->SetSimpleFilename(foundsimplefilename);
                outerElem->SetFilename(foundfilename);
                outerElem->SetAccDesc(GetExplorerTooltipText(foundfilename).c_str());
                if (outerElem->GetFlags() & LVIF_HIDDEN)
                {
                    iconElem->SetAlpha(128);
                    textElem->SetAlpha(g_touchmode ? 128 : 192);
                }
                if (!g_touchmode)
                {
                    if (shellstate[4] & 0x20)
                    {
                        outerElem->SetClass(L"doubleclicked");
                    }
                    else outerElem->SetClass(L"singleclicked");
                }
                UIContainer->Add((Element**)&outerElem, 1);
                if (DWMActive)
                {
                    AddLayeredRef(outerElem->GetDisplayNode());
                    SetGadgetFlags(outerElem->GetDisplayNode(), NULL, NULL);
                }

                outerElem->SetInnerElement(regElem<DDScalableElement*>(L"innerElem", outerElem));
                outerElem->SetIcon(iconElem);
                outerElem->SetShortcutArrow(regElem<Element*>(L"shortcutElem", outerElem));
                outerElem->SetText(textElem);
                outerElem->SetCheckbox(regElem<TouchButton*>(L"checkboxElem", outerElem));
                pm.push_back(outerElem);

                int currentID = pm.size() - 1;
                IconThumbHelper(currentID);
                yValue* yV2 = new yValue{ currentID };
                HANDLE smThumbnailThreadHandle = CreateThread(nullptr, 0, CreateIndividualThumbnail, (LPVOID)yV2, 0, nullptr);
                if (smThumbnailThreadHandle) CloseHandle(smThumbnailThreadHandle);
                if (yV)
                {
                    pm[currentID]->SetPage(yV->num);
                    pm[currentID]->SetInternalXPos(yV->fl1);
                    pm[currentID]->SetInternalYPos(yV->fl2);
                    delete yV;
                }
                else {
                    pm[currentID]->SetPage(1);
                    pm[currentID]->SetInternalXPos(0);
                    pm[currentID]->SetInternalYPos(0);
                }
                RearrangeIcons(false, false, true);
                pm[currentID]->AddFlags(LVIF_REFRESH);
                if (!yV)
                {
                    GTRANS_DESC transDesc[2];
                    TriggerFade(pm[currentID], transDesc, 0, 0.0f, 0.083f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                    TriggerScaleIn(pm[currentID], transDesc, 1, 0.0f, 0.25f, 0.0f, 0.0f, 0.0f, 1.0f, 0.7f, 0.7f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                    TransitionStoryboardInfo tsbInfo = {};
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pm[currentID]->GetDisplayNode(), &tsbInfo);
                }
                DUI_SetGadgetZOrder(pm[currentID], -1);
                delete fi;
                break;
            }
            case WM_USER + 21:
            case WM_USER + 22:
            {
                LVItem* toRemove = (LVItem*)wParam;
                pm.erase(pm.begin() + lParam);
                if (uMsg == WM_USER + 21)
                {
                    GTRANS_DESC transDesc[2];
                    TriggerFade(toRemove, transDesc, 0, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                    TriggerScaleOut(toRemove, transDesc, 1, 0.0f, 0.175f, 1.0f, 1.0f, 0.0f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, true);
                    TransitionStoryboardInfo tsbInfo = {};
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, toRemove->GetDisplayNode(), &tsbInfo);
                }
                else
                {
                    toRemove->Destroy(true);
                    toRemove = nullptr;
                }
                break;
            }
            case WM_USER + 23:
            {
                break;
            }
            case WM_USER + 24:
            {
                break;
            }
            case WM_USER + 25:
            {
                if (wParam) ((Element*)wParam)->SetLayoutPos(-3);
                if (lParam) ((Element*)lParam)->SetLayoutPos(2);
                break;
            }
            case WM_USER + 26:
            {
                break;
            }
        }
        return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK MsgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
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
                if (dea->pe)
                {
                    if (!dea->pe->IsDestroyed())
                    {
                        switch (lParam)
                        {
                        case 1:
                            dea->pe->SetVisible(false);
                            dea->pe->DestroyAll(true);
                            dea->pe->Destroy(true);
                            break;
                        case 2:
                            dea->pe->SetVisible(!dea->pe->GetVisible());
                            break;
                        case 3:
                            if (!dea->pe->GetMouseWithin()) dea->pe->SetSelected(false);
                            break;
                        case 4:
                            dea->pe->SetX(dea->val1);
                            dea->pe->SetY(dea->val2);
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

    DWORD WINAPI fastin(LPVOID lpParam)
    {
        yValue* yV = (yValue*)lpParam;
        DWORD lviFlags = pm[yV->num]->GetFlags();
        if (lviFlags & LVIF_REFRESH)
        {
            if (lviFlags & LVIF_ADVANCEDICON)
                HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            DesktopIcon di;
            ApplyIcons(&pm, &di, false, yV->num, 1, -1);
            if (g_touchmode)
            {
                int lines_basedOnEllipsis{};
                DWORD alignment{};
                RECT g_touchmoderect{};
                CalcDesktopIconInfo(yV, &lines_basedOnEllipsis, &alignment, false, &pm);
                HBITMAP capturedBitmap{};
                CreateTextBitmap(capturedBitmap, pm[yV->num]->GetSimpleFilename().c_str(), yV->fl1 - 4 * g_flScaleFactor, lines_basedOnEllipsis, alignment, g_touchmode, NULL);
                if (!g_isGlass && g_treatdirasgroup && lviFlags & LVIF_GROUP)
                {
                    COLORREF crDefault = g_theme ? RGB(208, 208, 208) : RGB(48, 48, 48);
                    di.crDominantTile = (pm[yV->num]->GetAssociatedColor() == 0 || pm[yV->num]->GetAssociatedColor() == -1 ||
                        (pm[yV->num]->GetGroupSize() == LVIGS_NORMAL && pm[yV->num]->GetIcon()->GetGroupColor() == 0)) ?
                        crDefault : pm[yV->num]->GetAssociatedColor();
                }
                if (GetRValue(di.crDominantTile) * 0.299 + GetGValue(di.crDominantTile) * 0.587 + GetBValue(di.crDominantTile) * 0.114 > 152)
                {
                    IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
                    IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 1, NULL);
                }
                else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
                if (capturedBitmap != nullptr) di.text = capturedBitmap;
            }
            SendMessageW(wnd->GetHWND(), WM_USER + 4, (WPARAM)&di, yV->num);
            SendMessageW(wnd->GetHWND(), WM_USER + 3, NULL, yV->num);
            if (lviFlags & LVIF_ADVANCEDICON)
                CoUninitialize();
        }
        if (!(lviFlags & LVIF_FLYING)) SendMessageW(wnd->GetHWND(), WM_USER + 2, NULL, NULL);
        return 0;
    }

    void LaunchItem(LPCWSTR filename)
    {
        LPITEMIDLIST pidl = nullptr;
        HRESULT hr = SHParseDisplayName(filename, nullptr, &pidl, 0, nullptr);
        if (SUCCEEDED(hr))
        {
            IShellFolder* ppFolder = nullptr;
            LPITEMIDLIST pidlChild = nullptr;
            hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&ppFolder, (LPCITEMIDLIST*)&pidlChild);
            if (SUCCEEDED(hr))
            {
                LPCONTEXTMENU pICv1 = nullptr;
                ppFolder->GetUIObjectOf(nullptr, 1, (LPCITEMIDLIST*)&pidlChild, IID_IContextMenu, nullptr, (void**)&pICv1);
                if (SUCCEEDED(hr))
                {
                    HMENU hmDummy = CreatePopupMenu();
                    pICv1->QueryContextMenu(hmDummy, 0, MIN_SHELL_ID, MAX_SHELL_ID, CMF_DEFAULTONLY);

                    CMINVOKECOMMANDINFO ici;
                    ZeroMemory(&ici, sizeof(ici));
                    ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
                    ici.lpVerb = "open";
                    ici.nShow = SW_SHOWNORMAL;
                    hr = pICv1->InvokeCommand(&ici);
                    if (hr == E_OUTOFMEMORY)
                    {
                        ici.lpVerb = "openas";
                        hr = pICv1->InvokeCommand(&ici);
                    }
                }
                pICv1->Release();
            }
            ppFolder->Release();
        }
        CoTaskMemFree(pidl);
    }

    void CloseCustomizePage(Element* elem, Event* iev)
    {
        if (iev->uidType == DDLVActionButton::Click || iev->uidType == DDLVActionButton::MultipleClick)
        {
            LVItem* lvi = ((DDLVActionButton*)elem)->GetAssociatedItem();
            CSafeElementPtr<Element> groupdirectory;
            groupdirectory.Assign(regElem<Element*>(L"groupdirectory", elem->GetParent()->GetParent()->GetParent()->GetParent()->GetParent()));
            CSafeElementPtr<Element> customizegroup;
            customizegroup.Assign(regElem<Element*>(L"customizegroup", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirtitle;
            dirtitle.Assign(regElem<DDScalableRichText*>(L"dirtitle", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirname;
            dirname.Assign(regElem<DDScalableRichText*>(L"dirname", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirdetails;
            dirdetails.Assign(regElem<DDScalableRichText*>(L"dirdetails", groupdirectory));
            CSafeElementPtr<Element> tasks;
            tasks.Assign(regElem<Element*>(L"tasks", groupdirectory));
            CSafeElementPtr<DDLVActionButton> More;
            More.Assign(regElem<DDLVActionButton*>(L"More", groupdirectory));
            CSafeElementPtr<TouchScrollViewer> groupdirlist;
            groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory));
            CSafeElementPtr<Element> Group_BackContainer;
            Group_BackContainer.Assign(regElem<Element*>(L"Group_BackContainer", groupdirectory));
            CSafeElementPtr<DDLVActionButton> Group_Back;
            Group_Back.Assign(regElem<DDLVActionButton*>(L"Group_Back", groupdirectory));
            CSafeElementPtr<Element> emptyview;
            emptyview.Assign(regElem<Element*>(L"emptyview", groupdirectory));
            CSafeElementPtr<DDScalableElement> emptygraphic;
            emptygraphic.Assign(regElem<DDScalableElement*>(L"emptygraphic", groupdirectory));
            bool fRefreshIcons = ((g_isColorized && g_themeOld == g_theme) || (g_automaticDark && g_themeOld != g_theme));
            bool fRefreshText = (g_themeOld != g_theme);
            if (lvi->GetGroupSize() == LVIGS_NORMAL && g_isColorized)
            {
                int icon2{};
                for (LVItem* lvi2 : pm)
                {
                    if (pm[icon2] == lvi) break;
                    icon2++;
                }
                lvi->GetIcon()->DestroyAll(true);
                yValue* yV = new yValue{ icon2 };
                HANDLE smThumbnailThreadHandle = CreateThread(nullptr, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, nullptr);
                if (smThumbnailThreadHandle) CloseHandle(smThumbnailThreadHandle);
            }
            if (emptygraphic)
            {
                if (lvi->GetIcon()->GetGroupColor() > 0) emptygraphic->SetAssociatedColor(g_colorPickerPalette[lvi->GetIcon()->GetGroupColor()]);
                else emptygraphic->SetAssociatedColor(-1);
            }
            else if (g_isColorized)
            {
                yValueEx* yV = new yValueEx{ static_cast<int>(lvi->GetChildItems()->size()), NULL, NULL, lvi->GetChildItems(), nullptr, lvi };
                DWORD animThread2;
                HANDLE animThreadHandle2 = CreateThread(nullptr, 0, subfastin, (LPVOID)yV, 0, &animThread2);
                if (animThreadHandle2) CloseHandle(animThreadHandle2);
            }
            if (emptyview) emptyview->SetVisible(true);
            Group_BackContainer->SetLayoutPos(-3);
            Group_Back->SetVisible(false);
            dirname->SetContentString(lvi->GetSimpleFilename().c_str());
            dirdetails->SetVisible(true);
            tasks->SetVisible(true);
            More->SetVisible(true);
            groupdirlist->SetVisible(true);
            groupdirlist->SetKeyFocus();

            short localeDirection = (localeType == 1) ? -1 : 1;
            RECT rcList;
            GetGadgetRect(groupdirlist->GetDisplayNode(), &rcList, 0);
            GTRANS_DESC transDesc[3];
            TriggerTranslate(dirtitle, transDesc, 0, 0.0f, 0.5f, 0.1f, 0.9f, 0.2f, 1.0f, 38.0f * g_flScaleFactor * localeDirection, 0.0f, 0.0f, 0.0f, false, false, true);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, dirtitle->GetDisplayNode(), &tsbInfo);
            TriggerTranslate(tasks, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -38.0f * g_flScaleFactor * localeDirection, 0.0f, false, false, true);
            TriggerFade(tasks, transDesc, 1, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, tasks->GetDisplayNode(), &tsbInfo);
            TriggerFade(More, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, More->GetDisplayNode(), &tsbInfo);
            TriggerTranslate(groupdirlist, transDesc, 0, 0.1f, 0.6f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, -100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false, false);
            TriggerFade(groupdirlist, transDesc, 1, 0.1f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            TriggerClip(groupdirlist, transDesc, 2, 0.1f, 0.6f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100 * g_flScaleFactor / (rcList.bottom - rcList.top), 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, groupdirlist->GetDisplayNode(), &tsbInfo);

            if (customizegroup)
            {
                TriggerTranslate(customizegroup, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.5f, 0.24f, 0.96f, 0.0f, 0.0f, 0.0f, 100.0f * g_flScaleFactor, false, true, false);
                TriggerFade(customizegroup, transDesc, 1, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, customizegroup->GetDisplayNode(), &tsbInfo);
            }
        }
    }

    void OpenCustomizePage(Element* elem, Event* iev)
    {
        if (iev->uidType == DDLVActionButton::Click || iev->uidType == DDLVActionButton::MultipleClick)
        {
            CSafeElementPtr<Element> groupdirectory;
            groupdirectory.Assign(regElem<Element*>(L"groupdirectory", elem->GetParent()->GetParent()->GetParent()->GetParent()->GetParent()));
            CSafeElementPtr<Element> customizegroupOld;
            customizegroupOld.Assign(regElem<Element*>(L"customizegroup", groupdirectory));
            if (customizegroupOld) return;
            CSafeElementPtr<DDScalableRichText> dirtitle;
            dirtitle.Assign(regElem<DDScalableRichText*>(L"dirtitle", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirname;
            dirname.Assign(regElem<DDScalableRichText*>(L"dirname", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirdetails;
            dirdetails.Assign(regElem<DDScalableRichText*>(L"dirdetails", groupdirectory));
            CSafeElementPtr<Element> tasks;
            tasks.Assign(regElem<Element*>(L"tasks", groupdirectory));
            CSafeElementPtr<DDLVActionButton> More;
            More.Assign(regElem<DDLVActionButton*>(L"More", groupdirectory));
            CSafeElementPtr<TouchScrollViewer> groupdirlist;
            groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory));
            CSafeElementPtr<Element> Group_BackContainer;
            Group_BackContainer.Assign(regElem<Element*>(L"Group_BackContainer", groupdirectory));
            CSafeElementPtr<DDLVActionButton> Group_Back;
            Group_Back.Assign(regElem<DDLVActionButton*>(L"Group_Back", groupdirectory));
            CSafeElementPtr<Element> emptyview;
            emptyview.Assign(regElem<Element*>(L"emptyview", groupdirectory));
            if (emptyview) emptyview->SetVisible(false);
            Group_BackContainer->SetLayoutPos(0);
            WCHAR backTo[256];
            StringCchPrintfW(backTo, 256, LoadStrFromRes(49856, L"shell32.dll").c_str(), ((DDLVActionButton*)elem)->GetAssociatedItem()->GetSimpleFilename().c_str());
            Group_Back->SetVisible(true);
            Group_Back->SetAssociatedItem(((DDLVActionButton*)elem)->GetAssociatedItem());
            Group_Back->SetAccDesc(backTo);
            assignFn(Group_Back, CloseCustomizePage);
            dirname->SetContentString(LoadStrFromRes(4027).c_str());
            dirdetails->SetVisible(false);
            More->SetVisible(false);
            Element* customizegroup;
            parserSubview->CreateElement(L"customizegroup", nullptr, groupdirlist->GetParent(), nullptr, &customizegroup);
            groupdirlist->GetParent()->Add(&customizegroup, 1);

            short localeDirection = (localeType == 1) ? -1 : 1;
            RECT rcList;
            groupdirlist->GetVisibleRect(&rcList);
            GTRANS_DESC transDesc[3];
            TriggerTranslate(dirtitle, transDesc, 0, 0.0f, 0.5f, 0.1f, 0.9f, 0.2f, 1.0f, -38.0f * g_flScaleFactor * localeDirection, 0.0f, 0.0f, 0.0f, false, false, true);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, dirtitle->GetDisplayNode(), &tsbInfo);
            TriggerFade(Group_Back, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 2, transDesc, Group_Back->GetDisplayNode(), &tsbInfo);
            TriggerTranslate(tasks, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 38.0f * g_flScaleFactor * localeDirection, 0.0f, false, false, true);
            TriggerFade(tasks, transDesc, 1, 0.0f, 0.083f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, tasks->GetDisplayNode(), &tsbInfo);
            TriggerTranslate(customizegroup, transDesc, 0, 0.1f, 0.6f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false, false);
            TriggerFade(customizegroup, transDesc, 1, 0.1f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc) - 1, transDesc, customizegroup->GetDisplayNode(), &tsbInfo);
            TriggerTranslate(groupdirlist, transDesc, 0, 0.0f, 0.25f, 0.11f, 0.5f, 0.24f, 0.96f, 0.0f, 0.0f, 0.0f, -100.0f * g_flScaleFactor, false, false, false);
            TriggerFade(groupdirlist, transDesc, 1, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, false);
            TriggerClip(groupdirlist, transDesc, 2, 0.0f, 0.25f, 0.11f, 0.5f, 0.24f, 0.96f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 100 * g_flScaleFactor / (rcList.bottom - rcList.top), 1.0f, 1.0f, true, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, groupdirlist->GetDisplayNode(), &tsbInfo);

            CSafeElementPtr<TouchScrollViewer> svOptions;
            svOptions.Assign(regElem<TouchScrollViewer*>(L"svOptions", customizegroup));
            Element* XScrollbar;
            svOptions->GetHScrollbar(&XScrollbar);
            svOptions->SetXScrollable(XScrollbar->GetVisible());
            CSafeElementPtr<TouchButton> peContent;
            peContent.Assign(regElem<TouchButton*>(L"content", customizegroup));
            CSafeElementPtr<DDColorPicker> DDCP_Group;
            DDCP_Group.Assign(regElem<DDColorPicker*>(L"DDCP_Group", customizegroup));
            DDCP_Group->SetThemeAwareness(true);
            vector<DDScalableElement*> elemTargets{};
            vector<DDScalableTouchButton*> btnTargets{};
            CSafeElementPtr<DDScalableTouchButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableTouchButton*>(L"fullscreeninner", centered));
            if (g_issubviewopen) btnTargets.push_back(fullscreeninner);
            CSafeElementPtr<DDScalableElement> iconElement;
            iconElement.Assign(regElem<DDScalableElement*>(L"iconElem", ((DDLVActionButton*)elem)->GetAssociatedItem()));
            elemTargets.push_back(iconElement);
            DDCP_Group->SetTargetElements(elemTargets);
            DDCP_Group->SetTargetTouchButtons(btnTargets);
            elemTargets.clear();
            btnTargets.clear();
            peContent->SetMinSize(320 * g_flScaleFactor, 0);
            RegKeyValue rkv(nullptr, nullptr, nullptr, iconElement->GetGroupColor());
            DDCP_Group->SetRegKeyValue(rkv);
        }
    }

    void PinGroup(Element* elem, Event* iev)
    {
        static int i{};
        if (iev->uidType == DDLVActionButton::Click || iev->uidType == DDLVActionButton::MultipleClick)
        {
            CSafeElementPtr<LVItem> lviTarget;
            lviTarget.Assign(((DDLVActionButton*)elem)->GetAssociatedItem());
            lviTarget->RemoveFlags(LVIF_MEMSELECT);
            for (i = 0; i < pm.size(); i++)
                if (lviTarget == pm[i]) break;
            if (lviTarget->GetGroupSize() == LVIGS_NORMAL)
                lviTarget->SetGroupSize(LVIGS_MEDIUM);
            else
            {
                lviTarget->SetGroupSize(LVIGS_NORMAL);
                if (g_touchmode) lviTarget->SetDrawType(1);
                if (localeType == 1) lviTarget->SetX(lviTarget->GetX() + lviTarget->GetWidth());
                lviTarget->SetTooltip(true);
                yValue* yV = new yValue{ i };
                HANDLE smThumbnailThreadHandle = CreateThread(nullptr, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, nullptr);
                if (smThumbnailThreadHandle) CloseHandle(smThumbnailThreadHandle);
            }
            IconThumbHelper(i);
            lviTarget->AddFlags(LVIF_SFG);
            RearrangeIcons(true, false, true);
            lviTarget->AddFlags(LVIF_REFRESH);
            if (lviTarget->GetGroupSize() != LVIGS_NORMAL)
            {
                lviTarget->SetOpenDirState(LVIODS_FULLSCREEN);
                HidePopupCore(false, true);
            }
        }
    }

    void AdjustGroupSize(Element* elem, Event* iev)
    {
        if (iev->uidType == DDLVActionButton::Click || iev->uidType == DDLVActionButton::MultipleClick)
        {
            CSafeElementPtr<LVItem> lviTarget;
            lviTarget.Assign(((DDLVActionButton*)elem)->GetAssociatedItem());
            CSafeElementPtr<DDScalableElement> iconElement;
            iconElement.Assign(regElem<DDScalableElement*>(L"iconElem", lviTarget));
            CSafeElementPtr<TouchScrollViewer> groupdirlist;
            groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", lviTarget));
            float scaleX{}, scaleY{}, scaleX2{}, scaleY2{}, clipX{}, clipX2{};
            int widthOld = iconElement->GetWidth();
            int heightOld = iconElement->GetHeight();
            RECT scrollsize{};
            groupdirlist->GetVisibleRect(&scrollsize);
            int widthOld2 = (scrollsize.right - scrollsize.left);
            int heightOld2 = (scrollsize.bottom - scrollsize.top);
            if (elem->GetID() == StrToID(L"Smaller"))
                lviTarget->SetGroupSize((LVItemGroupSize)((int)lviTarget->GetGroupSize() - 1));
            if (elem->GetID() == StrToID(L"Larger"))
                lviTarget->SetGroupSize((LVItemGroupSize)((int)lviTarget->GetGroupSize() + 1));
            switch (lviTarget->GetGroupSize())
            {
            case LVIGS_SMALL:
                if (localeType == 1) lviTarget->SetX(lviTarget->GetX() + lviTarget->GetWidth() - 316);
                lviTarget->SetWidth(316 * g_flScaleFactor);
                lviTarget->SetHeight(200 * g_flScaleFactor);
                iconElement->SetWidth(316 * g_flScaleFactor);
                iconElement->SetHeight(200 * g_flScaleFactor);
                break;
            case LVIGS_MEDIUM:
                if (localeType == 1) lviTarget->SetX(lviTarget->GetX() + lviTarget->GetWidth() - 476);
                lviTarget->SetWidth(476 * g_flScaleFactor);
                lviTarget->SetHeight(300 * g_flScaleFactor);
                iconElement->SetWidth(476 * g_flScaleFactor);
                iconElement->SetHeight(300 * g_flScaleFactor);
                break;
            case LVIGS_WIDE:
                if (localeType == 1) lviTarget->SetX(lviTarget->GetX() + lviTarget->GetWidth() - 716);
                lviTarget->SetWidth(716 * g_flScaleFactor);
                lviTarget->SetHeight(300 * g_flScaleFactor);
                iconElement->SetWidth(716 * g_flScaleFactor);
                iconElement->SetHeight(300 * g_flScaleFactor);
                break;
            case LVIGS_LARGE:
                if (localeType == 1) lviTarget->SetX(lviTarget->GetX() + lviTarget->GetWidth() - 716);
                lviTarget->SetWidth(716 * g_flScaleFactor);
                lviTarget->SetHeight(450 * g_flScaleFactor);
                iconElement->SetWidth(716 * g_flScaleFactor);
                iconElement->SetHeight(450 * g_flScaleFactor);
                break;
            }
            scaleX = static_cast<float>(widthOld) / iconElement->GetWidth();
            scaleY = static_cast<float>(heightOld) / iconElement->GetHeight();
            float originX = (localeType == 1) ? 1.0f : 0.0f;
            GTRANS_DESC transDesc[1];
            TriggerScaleIn(lviTarget, transDesc, 0, 0.0f, 0.25f, 0.75f, 0.45f, 0.0f, 1.0f, scaleX, scaleY, originX, 0.0f, 1.0f, 1.0f, originX, 0.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, lviTarget->GetDisplayNode(), &tsbInfo);

            ShowDirAsGroupDesktop(lviTarget, false);

            groupdirlist->GetVisibleRect(&scrollsize);
            scaleX2 = static_cast<float>(widthOld2) / (scrollsize.right - scrollsize.left);
            scaleY2 = static_cast<float>(heightOld2) / (scrollsize.bottom - scrollsize.top);
            GTRANS_DESC transDesc2[2];
            clipX = (localeType == 1) ? 1.0f : 1.0f - scaleX;
            clipX2 = (localeType == 1) ? 1.0f - scaleX : 1.0f;
            if (elem->GetID() == StrToID(L"Smaller"))
                TriggerClip(groupdirlist, transDesc2, 0, 0.0f, 0.25f, 0.75f, 0.45f, 0.0f, 1.0f, 0.0f, 0.0f, scaleX, scaleY, 0.0f, 0.0f, 1.0f, 1.0f, false, false);
            if (elem->GetID() == StrToID(L"Larger"))
                TriggerClip(groupdirlist, transDesc2, 0, 0.0f, 0.25f, 0.75f, 0.45f, 0.0f, 1.0f, clipX, 1.0f - scaleY, clipX2, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, false, false);
            TriggerScaleIn(groupdirlist, transDesc2, 1, 0.0f, 0.25f, 0.75f, 0.45f, 0.0f, 1.0f, 1 / scaleX, 1 / scaleY, 1.0f - originX, 1.0f, 1.0f, 1.0f, 1.0f - originX, 1.0f, false, false);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, groupdirlist->GetDisplayNode(), &tsbInfo);
            RearrangeIcons(true, false, true);
        }
    }

    DWORD WINAPI AutoHideMoreOptions(LPVOID lpParam)
    {
        CSafeElementPtr<Element> tasksOld;
        tasksOld.Assign(regElem<Element*>(L"tasks", (LVItem*)lpParam));
        Sleep(10000);
        if (g_ensureNoRefresh && lpParam && !((LVItem*)lpParam)->IsDestroyed() && ((LVItem*)lpParam)->GetGroupSize() != LVIGS_NORMAL)
        {
            Element* tasks = regElem<Element*>(L"tasks", (LVItem*)lpParam);
            Element* More = regElem<Element*>(L"More", (LVItem*)lpParam);
            if (tasks == tasksOld) SendMessageW(wnd->GetHWND(), WM_USER + 25, (WPARAM)tasks, (LPARAM)More);
        }
        return 0;
    }

    void ShowMoreOptions(Element* elem, Event* iev)
    {
        HANDLE hAutoHide{};
        if (iev->uidType == DDLVActionButton::Click || iev->uidType == DDLVActionButton::MultipleClick)
        {
            g_ensureNoRefresh = true;
            elem->SetLayoutPos(-3);
            CSafeElementPtr<Element> tasks;
            tasks.Assign(regElem<Element*>(L"tasks", ((DDLVActionButton*)elem)->GetAssociatedItem()));
            tasks->SetLayoutPos(2);
            hAutoHide = CreateThread(nullptr, 0, AutoHideMoreOptions, (LPVOID)((DDLVActionButton*)elem)->GetAssociatedItem(), 0, nullptr);
            if (hAutoHide) CloseHandle(hAutoHide);
        }
    }

    void OpenGroupInExplorer(Element* elem, Event* iev)
    {
        if (iev->uidType == DDLVActionButton::Click || iev->uidType == DDLVActionButton::MultipleClick)
        {
            wstring fileStr = RemoveQuotes(((DDLVActionButton*)elem)->GetAssociatedItem()->GetFilename());
            LaunchItem(fileStr.c_str());
        }
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

    void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB)
    {
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        pageElem->SetX(dimensions.right * offsetL);
        pageElem->SetY(dimensions.bottom * offsetT);
        pageElem->SetWidth(dimensions.right * offsetR);
        pageElem->SetHeight(dimensions.bottom * offsetB);
        WCHAR currentPage[64];
        StringCchPrintfW(currentPage, 64, LoadStrFromRes(4026).c_str(), g_currentPageID, g_maxPageID);
        pageinfo->SetContentString(currentPage);
    }

    DWORD WINAPI DeselectElement(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        if (dea->pe)
        {
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 3);
        }
        else delete dea;
        return 0;
    }
    void GoToPrevPage(Element* elem, Event* iev)
    {
        static RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        if (iev->uidType == TouchButton::Click && !g_pageviewer)
        {
            g_currentPageID--;
            if (g_editmode)
            {
                for (int items = 0; items < pm.size(); items++)
                {
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    if (pm[items]->GetPage() == g_currentPageID)
                    {
                        pm[items]->SetVisible(!g_hiddenIcons);
                        TriggerTranslate(pm[items], transDesc, 0, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX(), pm[items]->GetY(), false, false, false);
                        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                        DUI_SetGadgetZOrder(pm[items], -1);
                    }
                    else pm[items]->SetVisible(false);
                }
                g_invokedpagechange = true;
                DWORD animFlags = 0x1;
                if (g_currentPageID > 1) animFlags |= 0x2;
                RefreshSimpleView(animFlags);
            }
            else
            {
                TriggerPageTransition(-1, dimensions);
            }
            nextpageMain->SetVisible(true);
            if (g_currentPageID == 1) prevpageMain->SetVisible(false);
        }
        if (iev->uidType == LVItem::Click && g_pageviewer && elem->GetMouseFocused())
        {
            g_currentPageID = ((LVItem*)elem)->GetPage();
            for (int items = 0; items < pm.size(); items++)
            {
                if (pm[items]->GetPage() == g_currentPageID) pm[items]->SetVisible(!g_hiddenIcons);
                else pm[items]->SetVisible(false);
            }
            nextpageMain->SetVisible(true);
            if (g_currentPageID == 1) prevpageMain->SetVisible(false);
            RefreshSimpleView(0x0);
            TriggerEMToPV(true);
        }
    }
    void GoToNextPage(Element* elem, Event* iev)
    {
        static RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        if (iev->uidType == TouchButton::Click && !g_pageviewer)
        {
            g_currentPageID++;
            if (g_editmode)
            {
                for (int items = 0; items < pm.size(); items++)
                {
                    GTRANS_DESC transDesc[1];
                    TransitionStoryboardInfo tsbInfo = {};
                    if (pm[items]->GetPage() == g_currentPageID)
                    {
                        pm[items]->SetVisible(!g_hiddenIcons);
                        TriggerTranslate(pm[items], transDesc, 0, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX(), pm[items]->GetY(), false, false, false);
                        ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                        DUI_SetGadgetZOrder(pm[items], -1);
                    }
                    else pm[items]->SetVisible(false);
                }
                g_invokedpagechange = true;
                DWORD animFlags = 0x4;
                if (g_currentPageID < g_maxPageID) animFlags |= 0x8;
                RefreshSimpleView(animFlags);
            }
            else
            {
                TriggerPageTransition(1, dimensions);
            }
            prevpageMain->SetVisible(true);
            if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
        }
        if (iev->uidType == LVItem::Click && g_pageviewer && elem->GetMouseFocused())
        {
            g_currentPageID = ((LVItem*)elem)->GetPage();
            for (int items = 0; items < pm.size(); items++)
            {
                if (pm[items]->GetPage() == g_currentPageID) pm[items]->SetVisible(!g_hiddenIcons);
                else pm[items]->SetVisible(false);
            }
            prevpageMain->SetVisible(true);
            if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
            RefreshSimpleView(0x0);
            TriggerEMToPV(true);
        }
    }
    void ShowPageToggle(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseWithinProp())
        {
            float ElemOrigin = (elem == prevpageMain) ? 0.0f : 1.0f;
            if (localeType == 1) ElemOrigin = 1.0f - ElemOrigin;
            if (elem->GetMouseWithin() == true)
            {
                GTRANS_DESC transDesc2[1];
                TriggerScaleIn(elem, transDesc2, 0, 0.0f, 0.33f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, ElemOrigin, 0.5f, 1.0f, 1.0f, ElemOrigin, 0.5f, false, false);
                TransitionStoryboardInfo tsbInfo2 = {};
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, elem->GetDisplayNode(), &tsbInfo2);
                elem->SetSelected(true);
            }
            else
            {
                GTRANS_DESC transDesc2[1];
                TriggerScaleOut(elem, transDesc2, 0, 0.0f, 0.2f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, ElemOrigin, 0.5f, false, false);
                TransitionStoryboardInfo tsbInfo2 = {};
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc2), transDesc2, elem->GetDisplayNode(), &tsbInfo2);
                DWORD animCoef = g_animCoef;
                if (g_AnimShiftKey && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) animCoef = 100;
                DWORD dwDEA = DWMActive ? 200 * (animCoef / 100.0f) : 0;
                DelayedElementActions* dea = new DelayedElementActions{ dwDEA, elem };
                DWORD DelayedSelect;
                HANDLE hDelayedSelect = CreateThread(nullptr, 0, DeselectElement, dea, NULL, nullptr);
                if (hDelayedSelect) CloseHandle(hDelayedSelect);
            }
        }
        if (pProp == TouchButton::PressedProp())
        {
            CSafeElementPtr<RichText> togglepageGlyph;
            togglepageGlyph.Assign(regElem<RichText*>(L"togglepageGlyph", elem));
            GTRANS_DESC transDesc[1];
            if (((TouchButton*)elem)->GetPressed())
                TriggerScaleOut(togglepageGlyph, transDesc, 0, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, false, false);
            else
                TriggerScaleIn(togglepageGlyph, transDesc, 0, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, togglepageGlyph->GetDisplayNode(), &tsbInfo);
        }
    }

    DWORD WINAPI ApplyThumbnailIcons(LPVOID lpParam)
    {
        Sleep(150);
        PostMessageW(wnd->GetHWND(), WM_USER + 14, NULL, NULL);
        return 0;
    }

    DWORD WINAPI CreateIndividualThumbnail(LPVOID lpParam)
    {
        yValue* yV = (yValue*)lpParam;
        if (!g_treatdirasgroup || pm[yV->num]->GetGroupSize() != LVIGS_NORMAL)
        {
            delete yV;
            return 1;
        }
        int paddingInner = 2;
        if (g_iconsz > 120)
            paddingInner = 12;
        else if (g_iconsz > 80)
            paddingInner = 8;
        else if (g_iconsz > 40)
            paddingInner = 4;
        int padding = (g_iconsz - paddingInner - g_gpiconsz * 2) / 2;
        if (pm[yV->num]->GetFlags() & LVIF_GROUP && g_treatdirasgroup == true)
        {
            int x = padding * g_flScaleFactor, y = padding * g_flScaleFactor;
            vector<ThumbIcons> strs;
            wstring folderPath = RemoveQuotes(pm[yV->num]->GetFilename());
            unsigned short count = pm[yV->num]->GetItemCount();
            if (count > 4) count = 4;
            EnumerateFolderForThumbnails((LPWSTR)folderPath.c_str(), &strs, 4);
            if (strs.size() == 0)
            {
                delete yV;
                return 1;
            }
            for (int thumbs = 0; thumbs < count; thumbs++)
            {
                if (strs[thumbs].GetHasAdvancedIcon())
                {
                    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                    break;
                }
            }
            for (int thumbs = 0; thumbs < count; thumbs++)
            {
                HBITMAP thumbIcon{};
                GetShellItemImage(thumbIcon, (strs[thumbs].GetFilename()).c_str(), g_gpiconsz, g_gpiconsz, strs[thumbs].GetColorLock());
                if (strs[thumbs].GetColorLock() == false)
                {
                    if (g_isDarkIconsEnabled)
                    {
                        HBITMAP bmpOverlay{};
                        AddPaddingToBitmap(thumbIcon, bmpOverlay, 0, 0, 0, 0);
                        COLORREF lightness = GetMostFrequentLightnessFromIcon(thumbIcon, g_iconsz * g_flScaleFactor);
                        IterateBitmap(thumbIcon, UndoPremultiplication, 3, 0, 1, RGB(18, 18, 18));
                        bool compEffects = (GetGValue(lightness) < 208);
                        IterateBitmap(bmpOverlay, ColorToAlpha, 1, 0, 1, lightness);
                        CompositeBitmaps(thumbIcon, bmpOverlay, compEffects, 0.44);
                        DeleteObject(bmpOverlay);
                    }
                    if (g_isGlass && !g_isDarkIconsEnabled && !g_isColorized)
                    {
                        HBITMAP bmpOverlay{};
                        AddPaddingToBitmap(thumbIcon, bmpOverlay, 0, 0, 0, 0);
                        IterateBitmap(thumbIcon, SimpleBitmapPixelHandler, 0, 0, 1, GetLightestPixel(thumbIcon));
                        CompositeBitmaps(thumbIcon, bmpOverlay, true, 1);
                        IterateBitmap(thumbIcon, DesaturateWhitenGlass, 1, 0, 0.4, GetLightestPixel(thumbIcon));
                        DeleteObject(bmpOverlay);
                    }
                    if (g_isColorized)
                    {
                        COLORREF colorPickerPalette[8] =
                        {
                            -1, ImmersiveColor,
                            RGB(0, 120, 215), RGB(177, 70, 194), RGB(232, 17, 35),
                            RGB(247, 99, 12), RGB(255, 185, 0), RGB(0, 204, 106)
                        };
                        COLORREF iconcolor = (pm[yV->num]->GetIcon()->GetGroupColor() == 0) ? (iconColorID == 1) ? ImmersiveColor : IconColorizationColor : colorPickerPalette[pm[yV->num]->GetIcon()->GetGroupColor()];
                        IterateBitmap(thumbIcon, EnhancedBitmapPixelHandler, 1, 0, 1, iconcolor);
                    }
                }
                int xRender = (localeType == 1) ? (g_iconsz - g_gpiconsz) * g_flScaleFactor - x : x;
                x += ((g_gpiconsz + paddingInner) * g_flScaleFactor);
                ThumbnailIcon* ti = new ThumbnailIcon{ xRender, y, strs[thumbs], thumbIcon };
                if (x > (g_iconsz - g_gpiconsz) * g_flScaleFactor)
                {
                    x = padding * g_flScaleFactor;
                    y += ((g_gpiconsz + paddingInner) * g_flScaleFactor);
                }
                PostMessageW(wnd->GetHWND(), WM_USER + 16, (WPARAM)ti, yV->num);
            }
            for (int thumbs = 0; thumbs < count; thumbs++)
            {
                if (strs[thumbs].GetHasAdvancedIcon())
                {
                    CoUninitialize();
                    break;
                }
            }
        }
        delete yV;
        return 0;
    }

    void ApplyIcons(vector<LVItem*>* pmLVItem, DesktopIcon* di, bool subdirectory, int id, float scale, COLORREF crSubdir)
    {
        if (id >= (*pmLVItem).size()) return;
        wstring dllName{}, iconID, iconFinal;
        bool customExists = EnsureRegValueExists(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons", L"29");
        if (customExists)
        {
            WCHAR* customIconStr{};
            GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons", L"29", &customIconStr);
            wstring customIcon = customIconStr;
            size_t pathEnd = customIcon.find_last_of(L"\\");
            size_t idStart = customIcon.find(L",-");
            if (idStart == wstring::npos)
            {
                iconFinal = customIcon;
            }
            else if (pathEnd != wstring::npos)
            {
                dllName = customIcon.substr(pathEnd + 1, idStart - pathEnd - 1);
                iconID = customIcon.substr(idStart + 2, wstring::npos);
            }
            else
            {
                dllName = L"imageres.dll";
                iconID = L"163";
            }
            free(customIconStr);
        }
        else
        {
            dllName = L"imageres.dll";
            iconID = L"163";
        }
        bool isCustomPath = (iconFinal.length() > 1);
        HICON icoShortcut{};
        if (isCustomPath) icoShortcut = (HICON)LoadImageW(nullptr, iconFinal.c_str(), IMAGE_ICON, g_shiconsz * scale * g_flScaleFactor, g_shiconsz * scale * g_flScaleFactor, LR_LOADFROMFILE);
        else icoShortcut = (HICON)LoadImageW(LoadLibraryW(dllName.c_str()), MAKEINTRESOURCE(_wtoi(iconID.c_str())), IMAGE_ICON, g_shiconsz * scale * g_flScaleFactor, g_shiconsz * scale * g_flScaleFactor, LR_SHARED);
        // The use of a dummy icon is because we can't use a fully transparent bitmap
        static const HICON dummyi = (HICON)LoadImageW(LoadLibraryW(L"shell32.dll"), MAKEINTRESOURCE(24), IMAGE_ICON, 16, 16, LR_SHARED);
        HBITMAP dummyii{};
        IconToBitmap(dummyi, dummyii, 16, 16);
        IterateBitmap(dummyii, SimpleBitmapPixelHandler, 0, 0, 0.005, NULL);
        HBITMAP bmpForeground{};
        DWORD lviFlags = (*pmLVItem)[id]->GetFlags();
        if (!(lviFlags & LVIF_GROUP) || g_treatdirasgroup == false)
        {
            bool useThumbnail = (lviFlags & (LVIF_COLORLOCK | LVIF_DIR)) && !(lviFlags & LVIF_GROUP && g_treatdirasgroup);
            if (g_touchmode && (*pmLVItem)[id]->GetTileSize() == LVITS_ICONONLY) GetShellItemImage(bmpForeground, RemoveQuotes((*pmLVItem)[id]->GetFilename()).c_str(), 32 * scale, 32 * scale, useThumbnail);
            else GetShellItemImage(bmpForeground, RemoveQuotes((*pmLVItem)[id]->GetFilename()).c_str(), g_iconsz * scale, g_iconsz * scale, useThumbnail);
        }
        else bmpForeground = dummyii;
        HBITMAP bmpShortcut{}, bmpBackground{};
        int shadowSpace = 8 * scale * g_flScaleFactor;
        IconToBitmap(icoShortcut, bmpShortcut, g_shiconsz * scale * g_flScaleFactor, g_shiconsz * scale * g_flScaleFactor);
        DestroyIcon(icoShortcut);
        IterateBitmap(bmpShortcut, UndoPremultiplication, 1, 0, 1, NULL);
        if (bmpForeground != dummyii)
        {
            float shadowintensity = (g_touchmode || g_isGlass) ? 0.4 : 0.33;
            AddPaddingToBitmap(bmpForeground, bmpBackground, shadowSpace, shadowSpace, shadowSpace, shadowSpace);
            IterateBitmap(bmpBackground, SimpleBitmapPixelHandler, 0, (int)(4 * scale * g_flScaleFactor), shadowintensity, NULL);
            if (g_isDarkIconsEnabled)
            {
                if (!(lviFlags & LVIF_COLORLOCK))
                {
                    HBITMAP bmpOverlay{};
                    AddPaddingToBitmap(bmpForeground, bmpOverlay, 0, 0, 0, 0);
                    COLORREF lightness = GetMostFrequentLightnessFromIcon(bmpForeground, g_iconsz * scale * g_flScaleFactor);
                    if (bmpForeground != dummyii) IterateBitmap(bmpForeground, UndoPremultiplication, 3, 0, 1, RGB(18, 18, 18));
                    bool compEffects = (GetGValue(lightness) < 208);
                    IterateBitmap(bmpOverlay, ColorToAlpha, 1, 0, 1, lightness);
                    CompositeBitmaps(bmpForeground, bmpOverlay, compEffects, 0.44);
                    DeleteObject(bmpOverlay);
                }
                if (GetGValue(GetMostFrequentLightnessFromIcon(bmpShortcut, g_iconsz * scale * g_flScaleFactor)) > 208) IterateBitmap(bmpShortcut, InvertConstHue, 1, 0, 1, NULL);
            }
            if (g_isGlass && !g_isDarkIconsEnabled && !g_isColorized && !(lviFlags & LVIF_COLORLOCK))
            {
                if (pmLVItem == &pm)
                {
                    HDC hdc = GetDC(nullptr);
                    HBITMAP bmpOverlay{};
                    AddPaddingToBitmap(bmpForeground, bmpOverlay, 0, 0, 0, 0);
                    HBITMAP bmpOverlay2{};
                    AddPaddingToBitmap(bmpForeground, bmpOverlay2, 0, 0, 0, 0);
                    IterateBitmap(bmpOverlay, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
                    CompositeBitmaps(bmpOverlay, bmpOverlay2, true, 0.5);
                    IterateBitmap(bmpForeground, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
                    CompositeBitmaps(bmpForeground, bmpOverlay, true, 1);
                    DeleteObject(bmpOverlay);
                    DeleteObject(bmpOverlay2);
                    HBITMAP bmpOverlay3{};
                    AddPaddingToBitmap(bmpForeground, bmpOverlay3, 0, 0, 0, 0);
                    IterateBitmap(bmpOverlay3, DesaturateWhitenGlass, 1, 0, 0.4, 16777215);
                    POINT iconmidpoint;
                    if (g_touchmode)
                    {
                        iconmidpoint.x = pm[id]->GetX() + g_iconsz * scale * g_flScaleFactor / 2;
                        iconmidpoint.y = pm[id]->GetY() + pm[id]->GetHeight() - g_iconsz * scale * g_flScaleFactor / 2;
                    }
                    else
                    {
                        iconmidpoint.x = pm[id]->GetX() + pm[id]->GetIcon()->GetX() + g_iconsz * scale * g_flScaleFactor / 2;
                        iconmidpoint.y = pm[id]->GetY() + pm[id]->GetIcon()->GetY() + g_iconsz * scale * g_flScaleFactor / 2;
                    }
                    IterateBitmap(bmpForeground, DesaturateWhitenGlass, 1, 0, 1, GetLightestPixel(bmpForeground));
                    COLORREF glassColor = GetColorFromPixel(hdc, iconmidpoint);
                    IncreaseBrightness(glassColor);
                    IterateBitmap(bmpForeground, StandardBitmapPixelHandler, 3, 0, 0.8, glassColor);
                    CompositeBitmaps(bmpForeground, bmpOverlay3, false, 0);
                    DeleteObject(bmpOverlay3);
                    ReleaseDC(nullptr, hdc);
                }
                else
                {
                    HBITMAP bmpOverlay{};
                    AddPaddingToBitmap(bmpForeground, bmpOverlay, 0, 0, 0, 0);
                    IterateBitmap(bmpForeground, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
                    CompositeBitmaps(bmpForeground, bmpOverlay, true, 1);
                    IterateBitmap(bmpForeground, DesaturateWhitenGlass, 1, 0, 0.4, GetLightestPixel(bmpForeground));
                    DeleteObject(bmpOverlay);
                }
            }
        }
        if (g_isColorized)
        {
            COLORREF iconcolor{};
            if (subdirectory) iconcolor = (crSubdir == 0 || crSubdir == -1) ? (iconColorID == 1) ? ImmersiveColor : IconColorizationColor : crSubdir;
            else iconcolor = (iconColorID == 1) ? ImmersiveColor : IconColorizationColor;
            if (!(lviFlags & LVIF_COLORLOCK)) IterateBitmap(bmpForeground, EnhancedBitmapPixelHandler, 1, 0, 1, iconcolor);
            IterateBitmap(bmpShortcut, StandardBitmapPixelHandler, 1, 0, 1, iconcolor);
        }
        if (g_touchmode)
        {
            if (!subdirectory && g_isGlass && !g_isDarkIconsEnabled && !g_isColorized)
            {
                HDC hdc = GetDC(nullptr);
                POINT iconmidpoint;
                iconmidpoint.x = pm[id]->GetX() + pm[id]->GetWidth() / 2;
                iconmidpoint.y = pm[id]->GetY() + pm[id]->GetHeight() / 2;
                di->crDominantTile = GetColorFromPixel(hdc, iconmidpoint);
                ReleaseDC(nullptr, hdc);
            }
            else if (!g_isGlass) di->crDominantTile = GetDominantColorFromIcon(bmpForeground, g_iconsz, 48);
        }

        if ((!g_isGlass || pmLVItem == &pm) && !(lviFlags & LVIF_HIDDEN))
        {
            HBITMAP bmpBuf{};
            if (g_touchmode) AddPaddingToBitmap(bmpForeground, bmpBuf, shadowSpace, shadowSpace, shadowSpace, shadowSpace);
            else AddPaddingToBitmap(bmpForeground, bmpBuf, shadowSpace, ceil(shadowSpace - 2 * scale * g_flScaleFactor), shadowSpace, floor(shadowSpace + 2 * scale * g_flScaleFactor));
            CompositeBitmaps(bmpBackground, bmpBuf, false, NULL);
            di->icon = bmpBackground;
            if (!bmpBackground || !bmpForeground) di->icon = nullptr;
            if (bmpForeground) DeleteObject(bmpForeground);
            DeleteObject(bmpBuf);
        }
        else
        {
            if (bmpBackground) DeleteObject(bmpBackground);
            di->icon = bmpForeground;
        }
        di->iconshortcut = bmpShortcut;
    }

    void IconThumbHelper(int id)
    {
        DDScalableElement* peIcon = pm[id]->GetIcon();
        UpdateCache* uc{};
        CValuePtr v = emptyspace->GetValue(Element::BackgroundProp, 1, uc);
        peIcon->DestroyAll(true);
        peIcon->SetClass(L"");
        peIcon->SetValue(Element::BackgroundProp, 1, v);
        short groupspace = 8 * g_flScaleFactor;
        if (g_touchmode && pm[id]->GetGroupSize() == LVIGS_NORMAL)
        {
            peIcon->SetWidth(g_iconsz * g_flScaleFactor + 2 * groupspace);
            peIcon->SetHeight(g_iconsz * g_flScaleFactor + 2 * groupspace);
        }
        CSafeElementPtr<Element> iconcontainer;
        iconcontainer.Assign(regElem<Element*>(L"iconcontainer", pm[id]));
        if (pm[id]->GetFlags() & LVIF_GROUP && g_treatdirasgroup == true)
        {
            peIcon->SetClass(L"groupthumbnail");

            if (g_touchmode)
            {
                if (pm[id]->GetGroupSize() == LVIGS_NORMAL)
                {
                    iconcontainer->SetPadding(groupspace, groupspace, groupspace, groupspace);
                    peIcon->SetWidth(g_iconsz * g_flScaleFactor);
                    peIcon->SetHeight(g_iconsz * g_flScaleFactor);
                    peIcon->SetPadding(-groupspace, -groupspace, -groupspace, -groupspace);
                }
                else
                {
                    // Reliability of this code to be checked.
                    iconcontainer->SetPadding(0, 0, 0, 0);
                    peIcon->SetPadding(0, 0, 0, 0);
                }
            }
        }
        free(uc);
    }

    void UpdateTileOnColorChange(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == DDScalableElement::AssociatedColorProp())
        {
            if (!g_isGlass)
            {
                int i;
                for (i = 0; i < pm.size(); i++)
                {
                    if (elem == pm[i]->GetIcon()) break;
                }
                COLORREF crDefault = g_theme ? RGB(208, 208, 208) : RGB(48, 48, 48);
                COLORREF crAssoc = ((DDScalableElement*)elem)->GetAssociatedColor();
                pm[i]->SetAssociatedColor((crAssoc == 0 || crAssoc == -1) ? crDefault : crAssoc);
                pm[i]->GetInnerElement()->SetAssociatedColor(CreateGlowColor((crAssoc == 0 || crAssoc == -1) ? crDefault : crAssoc));
                if (pm[i]->GetOpenDirState() == LVIODS_NONE)
                {
                    pm[i]->AddFlags(LVIF_REFRESH);
                    yValue* yV = new yValue{ i };
                    QueueUserWorkItem(RearrangeIconsHelper, yV, 0);
                }
            }
        }
    }

    void UpdateGroupOnColorChange(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == DDScalableElement::AssociatedColorProp())
        {
            int icon2{};
            for (LVItem* lvi : pm)
            {
                if (pm[icon2]->GetIcon() == elem) break;
                icon2++;
            }
            if (pm[icon2]->GetOpenDirState() == LVIODS_FULLSCREEN || pm[icon2]->GetOpenDirState() == LVIODS_PINNED)
            {
                if (pm[icon2]->GetOpenDirState() == LVIODS_FULLSCREEN && ((DDScalableElement*)elem)->GetGroupColor() == 1)
                    fullscreeninner->SetAssociatedColor(((DDScalableElement*)elem)->GetAssociatedColor());
                else if (pm[icon2]->GetOpenDirState() == LVIODS_PINNED)
                {
                    CSafeElementPtr<Element> groupdirlist;
                    groupdirlist.Assign(regElem<Element*>(L"groupdirlist", pm[icon2]));
                    if (pm[icon2]->GetChildItems() && groupdirlist->GetVisible())
                    {
                        bool fRefreshIcons = ((g_isColorized && g_themeOld == g_theme) || (g_automaticDark && g_themeOld != g_theme));
                        bool fRefreshText = (g_themeOld != g_theme);
                        yValueEx* yV = new yValueEx{ static_cast<int>(pm[icon2]->GetChildItems()->size()), NULL, NULL,
                            pm[icon2]->GetChildItems(), nullptr, pm[icon2] };
                        DWORD animThread2;
                        HANDLE animThreadHandle2 = CreateThread(nullptr, 0, subfastin, (LPVOID)yV, 0, &animThread2);
                        if (animThreadHandle2) CloseHandle(animThreadHandle2);
                    }
                }
            }
        }
    }

    void ShowDirAsGroupDesktop(LVItem* lvi, bool fNew)
    {
        Element* groupdirectory{};
        unsigned short lviCount = 0;
        StyleSheet* sheet = pSubview->GetSheet();
        int count2{};
        if (fNew)
        {
            lviCount = lvi->GetItemCount();
            CSafeElementPtr<Element> groupdirectoryOld;
            groupdirectoryOld.Assign(regElem<Element*>(L"groupdirectory", lvi));
            if (groupdirectoryOld) return;
            CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
            parserSubview->GetSheet(g_theme ? L"popup" : L"popupdark", &sheetStorage);
            lvi->AddFlags(LVIF_MEMSELECT);
            parserSubview->CreateElement(L"groupdirectory", nullptr, lvi, nullptr, (Element**)&groupdirectory);
            groupdirectory->SetValue(Element::SheetProp, 1, sheetStorage);
            lvi->Add((Element**)&groupdirectory, 1);
            DUI_SetGadgetZOrder(groupdirectory, 0);
        }
        else
        {
            groupdirectory = regElem<Element*>(L"groupdirectory", lvi);
            if (lvi->GetChildItems()) lviCount = lvi->GetChildItems()->size();
        }
        CSafeElementPtr<TouchScrollViewer> groupdirlist;
        groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory));
        CSafeElementPtr<DDScalableElement> lvi_SubUIContainer;
        lvi_SubUIContainer.Assign(regElem<DDScalableElement*>(L"SubUIContainer", groupdirlist));
        lvi_SubUIContainer->SetVisible(true);
        if (lviCount > 0)
        {
            vector<IElementListener*> v_pels;
            vector<LVItem*>* d_subpm{};
            if (fNew)
            {
                d_subpm = new vector<LVItem*>;
            }
            else
            {
                d_subpm = lvi->GetChildItems();
            }
            if (fNew)
            {
                sheet = pMain->GetSheet();
                CValuePtr sheetStorage2 = DirectUI::Value::CreateStyleSheet(sheet);
                parser->GetSheet(g_theme ? L"default" : L"defaultdark", &sheetStorage2);
                const WCHAR* elemname = g_touchmode ? L"outerElemTouch" : L"outerElem";
                for (int i = 0; i < lviCount; i++)
                {
                    LVItem* outerElemGrouped;
                    parser->CreateElement(elemname, nullptr, nullptr, nullptr, (Element**)&outerElemGrouped);
                    outerElemGrouped->SetValue(Element::SheetProp, 1, sheetStorage2);
                    lvi_SubUIContainer->Add((Element**)&outerElemGrouped, 1);
                    outerElemGrouped->SetInnerElement(regElem<DDScalableElement*>(L"innerElem", outerElemGrouped));
                    outerElemGrouped->SetIcon(regElem<DDScalableElement*>(L"iconElem", outerElemGrouped));
                    outerElemGrouped->SetShortcutArrow(regElem<Element*>(L"shortcutElem", outerElemGrouped));
                    outerElemGrouped->SetText(regElem<RichText*>(L"textElem", outerElemGrouped));
                    d_subpm->push_back(outerElemGrouped);
                }
                EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), &(*d_subpm), &count2, lviCount);
            }
            int x = 0, y = 0;
            int maxX{}, xRuns{};
            CValuePtr v;
            RECT dimensions;
            dimensions = *(groupdirectory->GetPadding(&v));
            int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 44) * g_flScaleFactor;
            int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 21) * g_flScaleFactor;
            if (g_touchmode)
            {
                outerSizeX = g_touchSizeX + DESKPADDING_TOUCH * g_flScaleFactor;
                outerSizeY = g_touchSizeY + DESKPADDING_TOUCH * g_flScaleFactor;
            }
            for (int j = 0; j < lviCount; j++)
            {
                if (fNew)
                {
                    if ((*d_subpm)[j]->GetFlags() & LVIF_HIDDEN)
                    {
                        (*d_subpm)[j]->GetIcon()->SetAlpha(128);
                        (*d_subpm)[j]->GetText()->SetAlpha(128);
                    }
                    v_pels.push_back(assignFn((*d_subpm)[j], SelectSubItem, true));
                    v_pels.push_back(assignFn((*d_subpm)[j], ItemRightClick, true));
                    v_pels.push_back(assignExtendedFn((*d_subpm)[j], SelectSubItemListener, true));
                    v_pels.push_back(assignExtendedFn((*d_subpm)[j], ShowCheckboxIfNeeded, true));
                    (*d_subpm)[j]->SetListeners(v_pels);
                    v_pels.clear();
                    if (!g_touchmode) (*d_subpm)[j]->SetClass(L"singleclicked");
                }
                int xRender = (localeType == 1) ? (lvi->GetWidth() - (dimensions.left + dimensions.right + outerSizeX)) - x : x;
                (*d_subpm)[j]->SetX(xRender), (*d_subpm)[j]->SetY(y);
                x += outerSizeX;
                xRuns++;
                if (x > lvi->GetWidth() - (dimensions.left + dimensions.right + outerSizeX + GetSystemMetricsForDpi(SM_CXVSCROLL, g_dpi)))
                {
                    maxX = xRuns;
                    xRuns = 0;
                    x = 0;
                    y += outerSizeY;
                }
            }
            x -= outerSizeX;
            if (maxX != 0 && xRuns % maxX != 0) y += outerSizeY;
            lvi_SubUIContainer->SetHeight(y);
            CSafeElementPtr<Element> dirtitle;
            dirtitle.Assign(regElem<Element*>(L"dirtitle", groupdirectory));
            RECT rcList;
            groupdirlist->GetVisibleRect(&rcList);
            for (int j = 0; j < lviCount; j++)
            {
                if (localeType == 1 && y > rcList.bottom - rcList.top)
                    (*d_subpm)[j]->SetX((*d_subpm)[j]->GetX() - GetSystemMetricsForDpi(SM_CXVSCROLL, g_dpi));
            }
            if (fNew)
            {
                CSafeElementPtr<LVItem> PendingContainer;
                PendingContainer.Assign(regElem<LVItem*>(L"PendingContainer", groupdirectory));
                PendingContainer->SetVisible(true);
                lvi->SetChildItems(d_subpm);
                DWORD animThread2;
                yValueEx* yV = new yValueEx{ lviCount, NULL, NULL, d_subpm, PendingContainer, lvi };
                HANDLE animThreadHandle2 = CreateThread(nullptr, 0, subfastin, (LPVOID)yV, 0, &animThread2);
                if (animThreadHandle2) CloseHandle(animThreadHandle2);
            }
        }
        else
        {
            if (fNew)
            {
                CSafeElementPtr<Element> emptyview;
                emptyview.Assign(regElem<Element*>(L"emptyview", groupdirectory));
                emptyview->SetVisible(true);
                CSafeElementPtr<DDScalableElement> emptygraphic;
                emptygraphic.Assign(regElem<DDScalableElement*>(L"emptygraphic", groupdirectory));
                if (lvi->GetIcon()->GetGroupColor() >= 1)
                    emptygraphic->SetAssociatedColor(g_colorPickerPalette[lvi->GetIcon()->GetGroupColor()]);
                CSafeElementPtr<Element> dirtitle;
                dirtitle.Assign(regElem<Element*>(L"dirtitle", groupdirectory));
                dirtitle->SetVisible(true);
            }
        }
        CSafeElementPtr<DDLVActionButton> More;
        More.Assign(regElem<DDLVActionButton*>(L"More", groupdirectory));
        CSafeElementPtr<DDLVActionButton> Smaller;
        Smaller.Assign(regElem<DDLVActionButton*>(L"Smaller", groupdirectory));
        CSafeElementPtr<DDLVActionButton> Larger;
        Larger.Assign(regElem<DDLVActionButton*>(L"Larger", groupdirectory));
        CSafeElementPtr<DDLVActionButton> Unpin;
        Unpin.Assign(regElem<DDLVActionButton*>(L"Unpin", groupdirectory));
        CSafeElementPtr<DDLVActionButton> Customize;
        Customize.Assign(regElem<DDLVActionButton*>(L"Customize", groupdirectory));
        CSafeElementPtr<DDLVActionButton> OpenInExplorer;
        OpenInExplorer.Assign(regElem<DDLVActionButton*>(L"OpenInExplorer", groupdirectory));
        if (fNew)
        {
            CSafeElementPtr<DDScalableElement> dirname;
            dirname.Assign(regElem<DDScalableElement*>(L"dirname", groupdirectory));
            dirname->SetContentString(lvi->GetSimpleFilename().c_str());
            CSafeElementPtr<DDScalableElement> dirdetails;
            dirdetails.Assign(regElem<DDScalableElement*>(L"dirdetails", groupdirectory));
            WCHAR itemCount[64];
            if (lviCount == 1) StringCchPrintfW(itemCount, 64, LoadStrFromRes(4031).c_str());
            else StringCchPrintfW(itemCount, 64, LoadStrFromRes(4032).c_str(), lviCount);
            dirdetails->SetContentString(itemCount);
            if (lviCount == 0) dirdetails->SetContentString(L"");
            CSafeElementPtr<Element> tasks;
            tasks.Assign(regElem<Element*>(L"tasks", groupdirectory));
            tasks->SetLayoutPos(-3);
            More->SetLayoutPos(2);
            Smaller->SetVisible(true), Larger->SetVisible(true), Unpin->SetVisible(true), Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
            assignFn(More, ShowMoreOptions);
            assignFn(Smaller, AdjustGroupSize);
            assignFn(Larger, AdjustGroupSize);
            assignFn(OpenInExplorer, OpenGroupInExplorer);
            assignFn(Customize, OpenCustomizePage);
            assignFn(Unpin, PinGroup);
        }
        Smaller->SetEnabled(true);
        Larger->SetEnabled(true);
        if (lvi->GetGroupSize() == LVIGS_SMALL) Smaller->SetEnabled(false);
        if (lvi->GetGroupSize() == LVIGS_LARGE) Larger->SetEnabled(false);
        Unpin->SetEnabled(isDefaultRes());
        More->SetAssociatedItem(lvi);
        Smaller->SetAssociatedItem(lvi);
        Larger->SetAssociatedItem(lvi);
        OpenInExplorer->SetAssociatedItem(lvi);
        Customize->SetAssociatedItem(lvi);
        Unpin->SetAssociatedItem(lvi);
    }

    bool fileopened{};

    void SelectItem(Element* elem, Event* iev)
    {
        static int validation = 0;
        short ctrlKey = GetAsyncKeyState(VK_CONTROL);
        if (iev->uidType == LVItem::Click)
        {
            validation++;
            TouchButton* checkbox = ((LVItem*)elem)->GetCheckbox();
            if (!(ctrlKey & 0x8000) && (elem == emptyspace || checkbox->GetMouseWithin() == false))
            {
                for (int items = 0; items < pm.size(); items++)
                {
                    pm[items]->SetSelected(false);
                    if (pm[items]->GetPage() != g_currentPageID)
                        pm[items]->SetVisible(false);
                }
            }
            if (elem != emptyspace)
            {
                if (checkbox->GetMouseWithin() == false) elem->SetSelected(!elem->GetSelected());
                else if (validation & 1) elem->SetSelected(!elem->GetSelected());
                if (!(shellstate[4] & 0x20) || g_touchmode)
                {
                    goto CLICKACTION;
                }
            }
        }
        if (iev->uidType == LVItem::MultipleClick && shellstate[4] & 0x20 && !g_touchmode && elem != emptyspace)
        {
        CLICKACTION:
            if (!(ctrlKey & 0x8000))
            {
                TouchButton* checkbox = ((LVItem*)elem)->GetCheckbox();
                DWORD lviFlags = ((LVItem*)elem)->GetFlags();
                if (checkbox->GetMouseFocused() == false && !(lviFlags & LVIF_DRAG))
                {
                    wstring temp = RemoveQuotes(((LVItem*)elem)->GetFilename());
                    if (lviFlags & LVIF_GROUP && g_treatdirasgroup == true) ShowDirAsGroup((LVItem*)elem);
                    else LaunchItem(temp.c_str());
                }
            }
        }
    }

    void SelectItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::SelectedProp() || (!(shellstate[4] & 0x20) && pProp == Element::MouseWithinProp() && !(g_iconunderline & 1)))
        {
            if (!g_touchmode)
            {
                float spacingInternal = CalcTextLines(((LVItem*)elem)->GetSimpleFilename().c_str(), elem->GetWidth() - 4 * g_flScaleFactor);
                int extraBottomSpacing = (elem->GetSelected() == true) ? ceil(spacingInternal) * textm.tmHeight : floor(spacingInternal) * textm.tmHeight;
                RichText* textElem = ((LVItem*)elem)->GetText();
                if (pProp == Element::SelectedProp())
                {
                    if (type == 69)
                    {
                        int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
                        int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor - textm.tmHeight;
                        int lines_basedOnEllipsis = ceil(CalcTextLines(((LVItem*)elem)->GetSimpleFilename().c_str(), innerSizeX - 4 * g_flScaleFactor)) * textm.tmHeight;
                        elem->SetHeight(innerSizeY + lines_basedOnEllipsis + 6 * g_flScaleFactor);
                        CSafeElementPtr<RichText> g_textElem;
                        g_textElem.Assign(regElem<RichText*>(L"textElem", g_outerElem));
                        textElem->SetLayoutPos(g_textElem->GetLayoutPos());
                        textElem->SetHeight(lines_basedOnEllipsis + 5 * g_flScaleFactor);
                    }
                    if (spacingInternal == 1.5)
                    {
                        if (elem->GetSelected() == true) elem->SetHeight(elem->GetHeight() + extraBottomSpacing * 0.5);
                        else elem->SetHeight(elem->GetHeight() - extraBottomSpacing);
                    }
                    textElem->SetHeight(extraBottomSpacing + 5 * g_flScaleFactor);
                }
                int textSpace = 2 * g_flScaleFactor;
                DWORD fontStyle = (!(shellstate[4] & 0x20) && (elem->GetMouseWithin() || g_iconunderline & 1)) ? 0x2 : NULL;
                HBITMAP capturedBitmap{};
                CreateTextBitmap(capturedBitmap, ((LVItem*)elem)->GetSimpleFilename().c_str(), elem->GetWidth() - 4 * g_flScaleFactor, extraBottomSpacing, DT_CENTER | DT_END_ELLIPSIS, false, fontStyle);
                IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
                HBITMAP shadowBitmap{};
                AddPaddingToBitmap(capturedBitmap, shadowBitmap, textSpace, textSpace, textSpace, textSpace);
                IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * g_flScaleFactor), 2, NULL);
                HBITMAP capturedBitmapAdjusted{};
                AddPaddingToBitmap(capturedBitmap, capturedBitmapAdjusted, textSpace, ceil(textSpace - g_flScaleFactor), textSpace, floor(textSpace + g_flScaleFactor));
                CompositeBitmaps(shadowBitmap, capturedBitmapAdjusted, false, NULL);
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(shadowBitmap, 2, 0xffffffff, false, false, false);
                if (spvBitmap != nullptr) textElem->SetValue(Element::ContentProp, 1, spvBitmap);
                DeleteObject(capturedBitmap);
                DeleteObject(capturedBitmapAdjusted);
                DeleteObject(shadowBitmap);
            }
            else if (type == 69)
            {
                RichText* textElem = ((LVItem*)elem)->GetText();
                CSafeElementPtr<RichText> g_textElem;
                g_textElem.Assign(regElem<RichText*>(L"textElem", g_outerElem));
                textElem->SetLayoutPos(g_textElem->GetLayoutPos());
            }
        }
        if (g_touchmode)
        {
            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            float coef{};
            if (!(g_treatdirasgroup && ((LVItem*)elem)->GetGroupSize() != LVIGS_NORMAL))
            {
                CSafeElementPtr<DDScalableElement> selectionElem;
                selectionElem.Assign(regElem<DDScalableElement*>(L"selectionElem", elem));
                if (selectionElem) selectionElem->SetVisible(elem->GetSelected());
            }
            if (pProp == TouchButton::PressedProp())
            {
                ((LVItem*)elem)->GetInnerElement()->SetEnabled(!((LVItem*)elem)->GetPressed());
                coef = ((LVItem*)elem)->GetPressed() ? 0.9325f : 1.0f;
                goto TLVITEMANIMATION;
            }
            if (pProp == TouchButton::MouseWithinProp() && ((LVItem*)elem)->GetOpenDirState() == LVIODS_NONE)
            {
                coef = ((LVItem*)elem)->GetMouseWithin() ? 1.0625f : 1.0f;
            TLVITEMANIMATION:
                TriggerScaleOut(elem, transDesc, 0, 0.0f, 0.25f, 0.25f, 0.1f, 0.25f, 1.0f, coef, coef, 0.5f, 0.5f, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, elem->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(elem, -1);
            }
        }
    }

    void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        TouchButton* checkboxElem = ((LVItem*)elem)->GetCheckbox();
        if (pProp == Element::MouseWithinProp() || pProp == Element::SelectedProp())
        {
            if (!g_touchmode)
            {
                CSafeElementPtr<Element> innerElem;
                innerElem.Assign(((LVItem*)elem)->GetInnerElement());
                float initialFade = elem->GetSelected() ? 1.0f : elem->GetMouseWithin() ? 0.0f : 1.0f;
                float finalFade = elem->GetMouseWithin() || elem->GetSelected() ? 1.0f : 0.0f;
                GTRANS_DESC transDesc[1];
                TransitionStoryboardInfo tsbInfo = {};
                innerElem->SetVisible(true);
                TriggerFade(innerElem, transDesc, 0, 0.0f, 0.125f, 0.1f, 0.25f, 0.75f, 0.9f, initialFade, finalFade, false, false, !elem->GetMouseWithin() && !elem->GetSelected());
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, innerElem->GetDisplayNode(), &tsbInfo);
                static int n{};
                DUI_SetGadgetZOrder(innerElem, n--);
                // Apparently this needs to be done otherwise there will be ugly overlays...
                ////////////////////////////////////
                CValuePtr v;
                DynamicArray<Element*>* pel = elem->GetChildren(&v);
                for (int id = 0; id < pel->GetSize(); id++)
                {
                    if (pel->GetItem(id) == innerElem) continue;
                    if (pel->GetItem(id) == ((LVItem*)elem)->GetIcon())
                    {
                        continue;
                    }
                    TriggerFade(pel->GetItem(id), transDesc, 0, 0.0f, 0.125f, 0.0f, 0.0f, 1.0f, 1.0f, 0.99f, 1.0f, false, false, false);
                    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pel->GetItem(id)->GetDisplayNode(), &tsbInfo);
                }
                ////////////////////////////////////
            }
            if (g_showcheckboxes == 1 && checkboxElem)
                checkboxElem->SetVisible(elem->GetMouseWithin() || elem->GetSelected());
        }
    }

    void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseFocusedProp())
        {
            UpdateCache* uc{};
            LVItem* grandparent = (LVItem*)elem->GetParent()->GetParent();
            CValuePtr v = elem->GetValue(Element::MouseFocusedProp, 1, uc);
            if (grandparent->GetInnerElement())
                grandparent->GetInnerElement()->SetValue(Element::MouseFocusedProp(), 1, v);
            if (uc) free(uc);
        }
    }

    DWORD WINAPI UpdateMarqueeSelectorPosition(LPVOID lpParam)
    {
        while (true)
        {
            if (!isPressed) break;
            Sleep(10);
            SendMessageW(wnd->GetHWND(), WM_USER + 5, NULL, NULL);
        }
        return 0;
    }

    DWORD WINAPI UpdateIconPosition(LPVOID lpParam)
    {
        if (fileopened) return 0;
        POINT ppt, ppt2;
        GetCursorPos(&ppt);
        ScreenToClient(wnd->GetHWND(), &ppt);
        WCHAR *cxDragStr{}, *cyDragStr{};
        GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragWidth", &cxDragStr);
        GetRegistryStrValues(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"DragHeight", &cyDragStr);
        static const int dragWidth = _wtoi(cxDragStr);
        static const int dragHeight = _wtoi(cyDragStr);
        free(cxDragStr), free(cyDragStr);
        while (true)
        {
            GetCursorPos(&ppt2);
            ScreenToClient(wnd->GetHWND(), &ppt2);
            Sleep(10);
            SendMessageW(wnd->GetHWND(), WM_USER + 17, (WPARAM)((vector<LVItem*>*)lpParam), (LPARAM)&ppt);
            if (g_lockiconpos && (abs(ppt.x - ppt2.x) > dragWidth || abs(ppt.y - ppt2.y) > dragHeight))
            {
                SendMessageW(wnd->GetHWND(), WM_USER + 18, (WPARAM)((vector<LVItem*>*)lpParam), 2);
                isIconPressed = false;
            }
            if (!isIconPressed)
            {
                SendMessageW(wnd->GetHWND(), WM_USER + 18, g_lockiconpos ? NULL : (WPARAM)((vector<LVItem*>*)lpParam), 0);
                Sleep(100);
                SendMessageW(wnd->GetHWND(), WM_USER + 18, g_lockiconpos ? NULL : (WPARAM)((vector<LVItem*>*)lpParam), 1);
                break;
            }
        }
        return 0;
    }

    void MarqueeSelector(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pv2)
    {
        DWORD marqueeThread;
        HANDLE marqueeThreadHandle;
        if (pProp == TouchButton::PressedProp())
        {
            if (g_tripleclickandhide == true && ((TouchButton*)elem)->GetPressed() == true)
            {
                g_emptyclicks++;
                HANDLE tripleClickThreadHandle = CreateThread(nullptr, 0, MultiClickHandler, &g_emptyclicks, 0, nullptr);
                if (tripleClickThreadHandle) CloseHandle(tripleClickThreadHandle);
                if (g_emptyclicks % 3 == 1)
                {
                    RECT dimensions;
                    GetClientRect(wnd->GetHWND(), &dimensions);
                    for (int items = 0; items < pm.size(); items++)
                    {
                        if (pm[items]->GetPage() == g_currentPageID)
                        {
                            float delay = (pm[items]->GetY() + pm[items]->GetHeight() / 2) / static_cast<float>(dimensions.bottom * 9);
                            float startXPos = ((dimensions.right / 2.0f) - (pm[items]->GetX() + (pm[items]->GetWidth() / 2))) * 0.2f;
                            float startYPos = ((dimensions.bottom / 2.0f) - (pm[items]->GetY() + (pm[items]->GetHeight() / 2))) * 0.2f;
                            GTRANS_DESC transDesc[3];
                            switch (g_hiddenIcons)
                            {
                            case 0:
                                TriggerTranslate(pm[items], transDesc, 0, delay, delay + 0.22f, 1.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX() + startXPos, pm[items]->GetY() + startYPos, false, false, false);
                                TriggerFade(pm[items], transDesc, 1, delay + 0.11f, delay + 0.22f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, true);
                                TriggerScaleOut(pm[items], transDesc, 2, delay, delay + 0.22f, 1.0f, 0.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, true, false);
                                break;
                            case 1:
                                delay *= 2;
                                TriggerTranslate(pm[items], transDesc, 0, delay, delay + 0.44f, 0.1f, 0.9f, 0.2f, 1.0f, pm[items]->GetX() + startXPos, pm[items]->GetY() + startYPos, pm[items]->GetX(), pm[items]->GetY(), true, false, false);
                                TriggerFade(pm[items], transDesc, 1, delay, delay + 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                                TriggerScaleIn(pm[items], transDesc, 2, delay, delay + 0.44f, 0.1f, 0.9f, 0.2f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                                break;
                            }
                            TransitionStoryboardInfo tsbInfo = {};
                            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                            DUI_SetGadgetZOrder(pm[items], -1);
                        }
                    }
                    g_hiddenIcons = !g_hiddenIcons;
                    SetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons", g_hiddenIcons, false, nullptr);
                    g_emptyclicks = 1;
                }
            }
            if (!isPressed)
            {
                RECT dimensions{};
                GetClientRect(wnd->GetHWND(), &dimensions);
                POINT ppt;
                GetCursorPos(&ppt);
                ScreenToClient(wnd->GetHWND(), &ppt);
                const int smallX = (localeType == 1) ? dimensions.right - ppt.x - 4 : ppt.x - 4;
                elem->SetX(smallX);
                elem->SetY(ppt.y - 4);
                elem->SetWidth(9);
                elem->SetHeight(9);
                if (localeType == 1) origX = dimensions.right - ppt.x;
                else origX = ppt.x;
                origY = ppt.y;
                GTRANS_DESC transDesc[1];
                TransitionStoryboardInfo tsbInfo = {};
                TriggerFade(selector, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false, false);
                ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, selector->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(selector, -1);
                selector->SetWidth(0);
                selector->SetHeight(0);
                selector->SetX(origX);
                selector->SetY(origY);
                selector->SetVisible(true);
                marqueeThreadHandle = CreateThread(nullptr, 0, UpdateMarqueeSelectorPosition, nullptr, 0, &marqueeThread);
                if (marqueeThreadHandle) CloseHandle(marqueeThreadHandle);
            }
            isPressed = 1;
        }
        else if ((!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) || !elem->GetMouseWithin()) && isPressed)
        {
            RECT dimensions{};
            GetClientRect(wnd->GetHWND(), &dimensions);
            elem->SetX(dimensions.left);
            elem->SetY(dimensions.top);
            elem->SetWidth(dimensions.right);
            elem->SetHeight(dimensions.bottom);

            GTRANS_DESC transDesc[1];
            TransitionStoryboardInfo tsbInfo = {};
            TriggerFade(selector, transDesc, 0, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false, true);
            ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, selector->GetDisplayNode(), &tsbInfo);
            DUI_SetGadgetZOrder(selector, -1);
            isPressed = 0;
        }
    }

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

    void ItemDragListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        DWORD dragThread;
        HANDLE dragThreadHandle;
        POINT ppt;
        if (pProp == LVItem::CapturedProp())
        {
            if (((LVItem*)elem)->GetCaptured())
            {
                selectedLVItems.clear();
                selectedLVItems.push_back((LVItem*)elem);
                int selectedItems{};
                if (elem->GetSelected() == false && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
                {
                    for (int items = 0; items < pm.size(); items++)
                        pm[items]->SetSelected(false);
                    elem->SetSelected(true);
                }
                /////// TEMP(?): I don't want to make folder groups selectable at the moment, maybe later, maybe not
                if (g_treatdirasgroup && ((LVItem*)elem)->GetGroupSize() != LVIGS_NORMAL)
                {
                    elem->SetSelected(false);
                }
                for (int items = 0; items < pm.size(); items++)
                {
                    if (pm[items]->GetSelected() == true)
                    {
                        selectedItems++;
                        if (pm[items] != elem) selectedLVItems.push_back(pm[items]);
                    }
                }
                g_dragpreview = g_touchmode ? dragpreviewTouch : dragpreview;
                CSafeElementPtr<Element> multipleitems;
                multipleitems.Assign(regElem<Element*>(L"multipleitems", g_dragpreview));
                multipleitems->SetVisible(false);
                if (selectedItems >= 2)
                {
                    multipleitems->SetVisible(true);
                    multipleitems->SetContentString(to_wstring(selectedItems).c_str());
                }
                if (g_showcheckboxes)
                {
                    CSafeElementPtr<TouchButton> checkbox;
                    checkbox.Assign(regElem<TouchButton*>(L"checkboxElem", (LVItem*)elem));
                    checkbox->SetVisible(true);
                }
                fileopened = false;
                GetCursorPos(&ppt);
                ScreenToClient(wnd->GetHWND(), &ppt);
                RECT dimensions{};
                GetClientRect(wnd->GetHWND(), &dimensions);
                if (localeType == 1) origX = dimensions.right - ppt.x - ((LVItem*)elem)->GetMemXPos();
                else origX = ppt.x - ((LVItem*)elem)->GetMemXPos();
                origY = ppt.y - ((LVItem*)elem)->GetMemYPos();
                InitializePreviewComponent(elem, g_dragpreview, g_touchmode, false);
                CSafeElementPtr<Element> DP_FolderGroup;
                DP_FolderGroup.Assign(regElem<Element*>(L"DP_FolderGroup", g_dragpreview));
                DP_FolderGroup->SetVisible(g_treatdirasgroup && ((LVItem*)elem)->GetFlags() & LVIF_GROUP);
                if (g_treatdirasgroup && ((LVItem*)elem)->GetFlags() & LVIF_GROUP)
                {
                    DDScalableElement* peIcon = ((LVItem*)elem)->GetIcon();
                    if (peIcon->GetGroupColor() == 0)
                    {
                        if (g_isColorized)
                            DP_FolderGroup->SetForegroundColor((iconColorID == 1) ? g_colorPickerPalette[1] : g_colorPickerPalette[iconColorID]);
                        else DP_FolderGroup->SetForegroundColor(g_colorPickerPalette[1]);
                    }
                    else DP_FolderGroup->SetForegroundColor(g_colorPickerPalette[peIcon->GetGroupColor()]);
                    int glyphiconsize = min(peIcon->GetWidth(), peIcon->GetHeight());
                    float sizeCoef = (log(glyphiconsize / (g_iconsz * g_flScaleFactor)) / log(100)) + 1;
                    DP_FolderGroup->SetFontSize(static_cast<int>(glyphiconsize / (2.0f * sizeCoef)));
                    if (((LVItem*)elem)->GetGroupSize() != LVIGS_NORMAL)
                    {
                        Element* rgpeUnwanted[4];
                        rgpeUnwanted[0] = (regElem<Element*>(L"innerElem", g_dragpreview));
                        rgpeUnwanted[1] = (regElem<Element*>(L"textElem", g_dragpreview));
                        rgpeUnwanted[2] = (regElem<Element*>(L"textElemShadow", g_dragpreview));
                        rgpeUnwanted[3] = (regElem<Element*>(L"checkboxElem", g_dragpreview));
                        for (int i = 0; i < ARRAYSIZE(rgpeUnwanted); i++)
                            if (rgpeUnwanted[i]) rgpeUnwanted[i]->SetVisible(false);
                    }
                }
                dragThreadHandle = CreateThread(nullptr, 0, UpdateIconPosition, &selectedLVItems, 0, &dragThread);
                if (dragThreadHandle) CloseHandle(dragThreadHandle);
                isIconPressed = 1;
            }
        }
    }

    void UpdateIconColorizationColor(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == DDScalableElement::AssociatedColorProp())
        {
            IconColorizationColor = ((DDScalableElement*)elem)->GetAssociatedColor();
            iconColorID = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop\\Personalize", L"IconColorID");
            SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop\\Personalize", L"IconColorizationColor", IconColorizationColor, false, nullptr);
            g_atleastonesetting = true;
            if (g_setcolors)
            {
                RearrangeIcons(false, true, true);
                for (int j = 0; j < pm.size(); j++)
                {
                    if (pm[j]->GetOpenDirState() == LVIODS_PINNED)
                        if (pm[j]->GetIcon()->GetAssociatedColor() == 0 || pm[j]->GetIcon()->GetAssociatedColor() == -1)
                            UpdateGroupOnColorChange(pm[j]->GetIcon(), DDScalableElement::AssociatedColorProp(), NULL, nullptr, nullptr); // to refresh neutrally colored ones
                }
                g_setcolors = false;
                SetTimer(wnd->GetHWND(), 12, 500, nullptr);
            }
        }
    }

    void testEventListener3(Element* elem, Event* iev)
    {
        if (iev->uidType == TouchButton::Click)
        {
            switch (g_issubviewopen)
            {
                case false:
                    if (elem != fullscreenpopupbase)
                    {
                        ShowPopupCore(nullptr);
                    }
                    break;
                case true:
                    if (centered->GetMouseWithin() == false && elem->GetMouseFocused() == true)
                    {
                        HidePopupCore(false, true);
                    }
                    break;
            }
        }
    }

    DWORD WINAPI SetElemPos(LPVOID lpParam)
    {
        DelayedElementActions* dea = (DelayedElementActions*)lpParam;
        Sleep(dea->dwMillis);
        dea = (DelayedElementActions*)lpParam;
        if (dea->pe)
        {
            SendMessageW(g_msgwnd, WM_USER + 5, (WPARAM)dea, 4);
        }
        else delete dea;
        return 0;
    }

    HANDLE g_iconSemaphore = CreateSemaphoreW(nullptr, 16, 16, nullptr);

    DWORD WINAPI RearrangeIconsHelper(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        yValue* yV = static_cast<yValue*>(lpParam);
        fastin(yV);
        delete yV;
        ReleaseSemaphore(g_iconSemaphore, 1, nullptr);
        UnInitThread();
        return 0;
    }

    void RearrangeIcons(bool animation, bool reloadicons, bool bAlreadyOpen)
    {
        unsigned int count = pm.size();
        for (int j = 0; j < count; j++)
        {
            pm[j]->SetPreRefreshMemPage(pm[j]->GetPage());
        }
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        short localeDirection = (localeType == 1) ? -1 : 1;
        if (bAlreadyOpen) SetPos(isDefaultRes());
        prevpageMain->SetVisible(false);
        nextpageMain->SetVisible(false);
        WCHAR DesktopLayoutWithSize[24];
        if (!g_touchmode) StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_%d", g_iconsz);
        else StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_Touch");
        if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) GetPos2(true);
        else
        {
            GetPos2(false);
            GetPos(false, nullptr);
            g_maxPageID = 1;
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 1 of 5 complete: Imported your desktop icon positions.");
        if (reloadicons)
        {
            DWORD dd;
            HANDLE thumbnailThread = CreateThread(nullptr, 0, ApplyThumbnailIcons, nullptr, 0, &dd);
            if (thumbnailThread) CloseHandle(thumbnailThread);
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 2 of 5 complete: Applied icons to the relevant desktop items.");
        int desktoppadding = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH : DESKPADDING_NORMAL);
        int desktoppadding_x = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH_X : DESKPADDING_NORMAL_X);
        int desktoppadding_y = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH_Y : DESKPADDING_NORMAL_Y);
        int x = desktoppadding_x, y = desktoppadding_y;
        if (g_currentPageID > g_maxPageID) g_currentPageID = g_maxPageID;
        if (count >= 1)
        {
            int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 44) * g_flScaleFactor;
            int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 22) * g_flScaleFactor;
            int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
            int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor - textm.tmHeight;
            LVItemTouchGrid**** lvitgMap{};
            if (g_touchmode)
            {
                outerSizeX = g_touchSizeX + desktoppadding;
                outerSizeY = g_touchSizeY + desktoppadding;
                innerSizeX = g_touchSizeX;
                innerSizeY = g_touchSizeY;
            }
            int largestXPos = (dimensions.right - (2 * desktoppadding_x) + desktoppadding) / outerSizeX;
            int largestYPos = (dimensions.bottom - (2 * desktoppadding_y) + desktoppadding) / outerSizeY;
            if (largestXPos == 0) largestXPos = 1;
            if (largestYPos == 0) largestYPos = 1;
            bool*** positions = new (nothrow) bool**[g_maxPageID];
            for (int page = 0; page < g_maxPageID; page++)
            {
                positions[page] = new (nothrow) bool*[largestXPos];
                for (int x = 0; x < largestXPos; x++)
                {
                    positions[page][x] = new (nothrow) bool[largestYPos]{};
                }
            }
            if (g_touchmode)
            {
                x = (dimensions.right - largestXPos * outerSizeX + desktoppadding) / 2;
                y = (dimensions.bottom - largestYPos * outerSizeY + desktoppadding) / 2;
                if (!bAlreadyOpen)
                {
                    lvitgMap = new (nothrow) LVItemTouchGrid***[g_maxPageID];
                    for (int page = 0; page < g_maxPageID; page++)
                    {
                        lvitgMap[page] = new (nothrow) LVItemTouchGrid**[largestXPos];
                        for (int x = 0; x < largestXPos; x++)
                        {
                            lvitgMap[page][x] = new (nothrow) LVItemTouchGrid*[largestYPos] {};
                        }
                    }
                }
            }
            if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 3 of 5 complete: Created an array of positions.");
            for (int j = 0; j < count; j++)
            {
                if (reloadicons && !(pm[j]->GetGroupSize() != LVIGS_NORMAL && bAlreadyOpen && g_isColorizedOld == g_isColorized)) pm[j]->AddFlags(LVIF_REFRESH);
                else pm[j]->RemoveFlags(LVIF_REFRESH);
                if (animation && pm[j]->GetPage() == g_currentPageID) pm[j]->AddFlags(LVIF_MOVING);
                else pm[j]->RemoveFlags(LVIF_MOVING);
                if (pm[j]->GetPage() != g_currentPageID && bAlreadyOpen) pm[j]->RemoveFlags(LVIF_FLYING);
                if (g_touchmode && !(g_treatdirasgroup && pm[j]->GetGroupSize() != LVIGS_NORMAL))
                {
                    switch (pm[j]->GetTileSize())
                    {
                    case LVITS_ICONONLY:
                        pm[j]->SetWidth((innerSizeX - desktoppadding) / 2);
                        pm[j]->SetHeight((innerSizeY - desktoppadding) / 2);
                        break;
                    case LVITS_NONE:
                        pm[j]->SetWidth(innerSizeX);
                        pm[j]->SetHeight(innerSizeY);
                        break;
                    case LVITS_DETAILED:
                        pm[j]->SetWidth(innerSizeX * 2 + desktoppadding);
                        pm[j]->SetHeight(innerSizeY);
                        break;
                    }
                }
                if (g_treatdirasgroup)
                {
                    Element* peIcon = pm[j]->GetIcon();
                    switch (pm[j]->GetGroupSize())
                    {
                        case LVIGS_SMALL:
                            pm[j]->SetWidth(316 * g_flScaleFactor);
                            pm[j]->SetHeight(200 * g_flScaleFactor);
                            peIcon->SetWidth(316 * g_flScaleFactor);
                            peIcon->SetHeight(200 * g_flScaleFactor);
                            break;
                        case LVIGS_MEDIUM:
                            pm[j]->SetWidth(476 * g_flScaleFactor);
                            pm[j]->SetHeight(300 * g_flScaleFactor);
                            peIcon->SetWidth(476 * g_flScaleFactor);
                            peIcon->SetHeight(300 * g_flScaleFactor);
                            break;
                        case LVIGS_WIDE:
                            pm[j]->SetWidth(716 * g_flScaleFactor);
                            pm[j]->SetHeight(300 * g_flScaleFactor);
                            peIcon->SetWidth(716 * g_flScaleFactor);
                            peIcon->SetHeight(300 * g_flScaleFactor);
                            break;
                        case LVIGS_LARGE:
                            pm[j]->SetWidth(716 * g_flScaleFactor);
                            pm[j]->SetHeight(450 * g_flScaleFactor);
                            peIcon->SetWidth(716 * g_flScaleFactor);
                            peIcon->SetHeight(450 * g_flScaleFactor);
                            break;
                    }
                }
                if (((g_treatdirasgroup && pm[j]->GetGroupSize() != LVIGS_NORMAL) || (g_touchmode && pm[j]->GetTileSize() > LVITS_NONE)) &&
                    pm[j]->GetInternalXPos() <= largestXPos - ceil((pm[j]->GetWidth() + desktoppadding) / static_cast<float>(outerSizeX)) &&
                    pm[j]->GetInternalYPos() <= largestYPos - ceil((pm[j]->GetHeight() + desktoppadding) / static_cast<float>(outerSizeY)))
                {
                    if (!EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) pm[j]->SetPage(g_maxPageID);
                    short page = pm[j]->GetPage();
                    short xPos = pm[j]->GetInternalXPos();
                    short yPos = pm[j]->GetInternalYPos();
                    short widthForRender = (!g_touchmode && (!g_treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL)) ? innerSizeX : pm[j]->GetWidth();
                    short xRender = (localeType == 1) ? dimensions.right - (xPos * outerSizeX) - widthForRender - x : xPos * outerSizeX + x;
                    short yRender = yPos * outerSizeY + y;
                    if (positions[page - 1][xPos][yPos] == true)
                    {
                        pm[j]->SetInternalXPos(65535);
                        pm[j]->SetInternalYPos(65535);
                    }
                    else
                    {
                        for (int i = 0; i < ceil((pm[j]->GetWidth() + desktoppadding) / static_cast<float>(outerSizeX)) && xPos + i < largestXPos; i++)
                        {
                            if (pm[j]->GetHeight() > outerSizeY)
                                for (int k = 0; k < ceil((pm[j]->GetHeight() + desktoppadding) / static_cast<float>(outerSizeY)) && yPos + k < largestYPos; k++)
                                    positions[page - 1][xPos + i][yPos + k] = true;
                            else positions[page - 1][xPos + i][yPos] = true;
                        }
                        if (!(pm[j]->GetFlags() & (LVIF_MOVING | LVIF_SFG)) || pm[j]->GetPreRefreshMemPage() != page)
                        {
                            pm[j]->SetX(xRender);
                            pm[j]->SetY(yRender);
                        }
                        pm[j]->SetMemXPos(xRender);
                        pm[j]->SetMemYPos(yRender);
                    }
                }
            }
            for (int j = 0; j < count; j++)
            {
                if (((!g_treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL) && (!g_touchmode || pm[j]->GetTileSize() <= LVITS_NONE)) && pm[j]->GetInternalXPos() < largestXPos && pm[j]->GetInternalYPos() < largestYPos)
                {
                    if (!EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) pm[j]->SetPage(g_maxPageID);
                    short page = pm[j]->GetPage();
                    short xPos = pm[j]->GetInternalXPos();
                    short yPos = pm[j]->GetInternalYPos();
                    short widthForRender = (!g_touchmode && (!g_treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL)) ? innerSizeX : pm[j]->GetWidth();
                    short xRender = (localeType == 1) ? dimensions.right - (xPos * outerSizeX) - widthForRender - x : xPos * outerSizeX + x;
                    short yRender = yPos * outerSizeY + y;
                    if (positions[page - 1][xPos][yPos] == true && !(g_touchmode && pm[j]->GetTileSize() == LVITS_ICONONLY))
                    {
                        pm[j]->SetInternalXPos(65535);
                        pm[j]->SetInternalYPos(65535);
                        continue;
                    }
                    else
                    {
                        if (!(pm[j]->GetFlags() & (LVIF_MOVING | LVIF_SFG)) || pm[j]->GetPreRefreshMemPage() != pm[j]->GetPage())
                        {
                            pm[j]->SetX(xRender);
                            pm[j]->SetY(yRender);
                        }
                        pm[j]->SetMemXPos(xRender);
                        pm[j]->SetMemYPos(yRender);
                        positions[page - 1][xPos][yPos] = true;
                    }
                }
            }
            if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 4 of 5 complete: Assigned positions to items that are in your resolution's bounds.");
            bool forcenewpage{};
            for (int j = 0; j < count; j++)
            {
                int modifierX = 0;
                int modifierY = 0;
                if (pm[j]->GetGroupSize() != LVIGS_NORMAL || pm[j]->GetTileSize() > LVITS_NONE)
                {
                    modifierX = (pm[j]->GetWidth() - outerSizeX + desktoppadding) / outerSizeX;
                    modifierY = (pm[j]->GetHeight() - outerSizeY + desktoppadding) / outerSizeY;
                }
                if (pm[j]->GetInternalXPos() >= largestXPos - modifierX ||
                    pm[j]->GetInternalYPos() >= largestYPos - modifierY)
                {
                    int arrX{}, arrY{}, arrPage{};
                    while (positions[arrPage][arrX][arrY] == true)
                    {
                        arrY++;
                        if (arrY == largestYPos)
                        {
                            arrY = 0;
                            arrX++;
                        }
                        if (arrX == largestXPos)
                        {
                            arrX = 0;
                            arrPage++;
                        }
                        if (arrPage == g_maxPageID)
                        {
                            g_maxPageID++;
                            bool*** positionsTemp = new (nothrow) bool** [g_maxPageID];
                            for (int page = 0; page < g_maxPageID; page++)
                            {
                                positionsTemp[page] = new (nothrow) bool* [largestXPos];
                                for (int x = 0; x < largestXPos; x++)
                                {
                                    positionsTemp[page][x] = new (nothrow) bool[largestYPos] {};
                                    for (int y = 0; y < largestYPos && page < g_maxPageID - 1; y++)
                                        positionsTemp[page][x][y] = positions[page][x][y];
                                }
                            }
                            for (int page = 0; page < g_maxPageID - 1; page++)
                            {
                                for (int x = 0; x < largestXPos; x++)
                                {
                                    delete[] positions[page][x];
                                }
                                delete[] positions[page];
                            }
                            delete[] positions;
                            positions = positionsTemp;
                            if (g_touchmode && !bAlreadyOpen)
                            {
                                LVItemTouchGrid**** lvitgMapTemp = new (nothrow) LVItemTouchGrid***[g_maxPageID];
                                for (int page = 0; page < g_maxPageID; page++)
                                {
                                    lvitgMapTemp[page] = new (nothrow) LVItemTouchGrid**[largestXPos];
                                    for (int x = 0; x < largestXPos; x++)
                                    {
                                        lvitgMapTemp[page][x] = new (nothrow) LVItemTouchGrid*[largestYPos]{};
                                        for (int y = 0; y < largestYPos && page < g_maxPageID - 1; y++)
                                            lvitgMapTemp[page][x][y] = lvitgMap[page][x][y];
                                    }
                                }
                                for (int page = 0; page < g_maxPageID - 1; page++)
                                {
                                    for (int x = 0; x < largestXPos; x++)
                                    {
                                        delete[] lvitgMap[page][x];
                                    }
                                    delete[] lvitgMap[page];
                                }
                                delete[] lvitgMap;
                                lvitgMap = lvitgMapTemp;
                            }
                            forcenewpage = true;
                            break;
                        }
                    }
                    pm[j]->SetInternalXPos(arrX);
                    pm[j]->SetInternalYPos(arrY);
                    if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize) && !forcenewpage)
                    {
                        pm[j]->SetPage(arrPage + 1);
                    }
                    else pm[j]->SetPage(g_maxPageID);
                    if (pm[j]->GetPage() != g_currentPageID) pm[j]->RemoveFlags(LVIF_FLYING);
                    positions[arrPage][arrX][arrY] = true;
                    if ((g_treatdirasgroup && pm[j]->GetGroupSize() != LVIGS_NORMAL) || (g_touchmode && pm[j]->GetTileSize() > LVITS_NONE))
                    {
                        for (int i = 0; i < ceil((pm[j]->GetWidth() + desktoppadding) / static_cast<float>(outerSizeX)) && arrX + i < largestXPos; i++)
                        {
                            if (pm[j]->GetHeight() > outerSizeY)
                                for (int k = 0; k < ceil((pm[j]->GetHeight() + desktoppadding) / static_cast<float>(outerSizeY)) && arrY + k < largestYPos; k++)
                                    positions[arrPage][arrX + i][arrY + k] = true;
                            else positions[arrPage][arrX + i][arrY] = true;
                        }
                    }
                }
                short widthForRender = (!g_touchmode && (!g_treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL)) ? innerSizeX : pm[j]->GetWidth();
                short xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - widthForRender - x : pm[j]->GetInternalXPos() * outerSizeX + x;
                short yRender = pm[j]->GetInternalYPos() * outerSizeY + y;
                BYTE smPos = pm[j]->GetSmallPos() - 1;
                if (smPos >= 0 && smPos < 4 && g_touchmode && pm[j]->GetTileSize() == LVITS_ICONONLY)
                {
                    if (!bAlreadyOpen)
                    {
                        if (!lvitgMap[pm[j]->GetPage() - 1][pm[j]->GetInternalXPos()][pm[j]->GetInternalYPos()])
                        {
                            pm[j]->SetMemXPos(xRender);
                            pm[j]->SetMemYPos(yRender);
                            lvitgMap[pm[j]->GetPage() - 1][pm[j]->GetInternalXPos()][pm[j]->GetInternalYPos()] = new LVItemTouchGrid;
                        }
                        pm[j]->SetTouchGrid(lvitgMap[pm[j]->GetPage() - 1][pm[j]->GetInternalXPos()][pm[j]->GetInternalYPos()], smPos);
                        goto SKIPXY;
                    }
                    else
                    {
                        yRender += (outerSizeY / 2) * (smPos / 2);
                        if (smPos & 1)
                            xRender += outerSizeX / 2 * localeDirection;
                    }
                }
                if (!(pm[j]->GetFlags() & (LVIF_MOVING | LVIF_SFG)) || pm[j]->GetPreRefreshMemPage() != pm[j]->GetPage())
                {
                    pm[j]->SetX(xRender);
                    pm[j]->SetY(yRender);
                }
                pm[j]->SetMemXPos(xRender);
                pm[j]->SetMemYPos(yRender);
            SKIPXY:
                if ((pm[j]->GetPage() == g_currentPageID && !(pm[j]->GetFlags() & LVIF_FLYING)) || pm[j]->GetFlags() & LVIF_SFG) pm[j]->SetVisible(!g_hiddenIcons);
                else pm[j]->SetVisible(false);
            }

            if (g_maxPageID > 1 && g_currentPageID < g_maxPageID) nextpageMain->SetVisible(true);
            if (g_currentPageID != 1) prevpageMain->SetVisible(true);

            for (int j = 0; j < count; j++)
            {
                pm[j]->SetMemPage(pm[j]->GetPage());
                if (pm[j]->GetPreRefreshMemPage() == 0)
                    pm[j]->SetPreRefreshMemPage(pm[j]->GetPage());
            }
            for (int j = 0; j < count; j++)
            {
                yValue* yV = new (nothrow) yValue{ j, (float)innerSizeX, (float)innerSizeY };
                QueueUserWorkItem(RearrangeIconsHelper, yV, 0);
            }
            for (int page = 0; page < g_maxPageID; page++)
            {
                for (int x = 0; x < largestXPos; x++)
                {
                    delete[] positions[page][x];
                }
                delete[] positions[page];
            }
            delete[] positions;
            if (g_touchmode && !bAlreadyOpen)
            {
                for (int page = 0; page < g_maxPageID; page++)
                {
                    for (int x = 0; x < largestXPos; x++)
                    {
                        delete[] lvitgMap[page][x];
                    }
                    delete[] lvitgMap[page];
                }
                delete[] lvitgMap;
            }
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 5 of 5 complete: Successfully arranged the desktop items.");
        if (reloadicons) g_isColorizedOld = g_isColorized;
        SetPos(isDefaultRes());
        g_lastWidth = dimensions.right;
        g_lastHeight = dimensions.bottom;
    }

    void InitLayout(bool animation, bool fResetUIState, bool bAlreadyOpen)
    {
        if (fResetUIState) SendMessageW(wnd->GetHWND(), WM_CHANGEUISTATE, 3, NULL);
        g_ensureNoRefresh = false;
        const WCHAR* elemname = g_touchmode ? L"outerElemTouch" : L"outerElem";
        if (g_outerElem)
        {
            g_outerElem->DestroyAll(true);
            g_outerElem->Destroy(true);
        }
        static IElementListener *pel_SelectItem, *pel_MarqueeSelector, *pel_DesktopRightClick;
        parser->CreateElement(elemname, nullptr, nullptr, nullptr, (Element**)&g_outerElem);
        if (bAlreadyOpen && isDefaultRes()) SetPos(true);
        UIContainer->DestroyAll(true);
        for (LVItem* lvi : pm)
        {
            lvi = nullptr;
        }
        pm.clear();
        GetFontHeight();
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 1 of 6 complete: Prepared DirectDesktop to receive desktop data.");
        WCHAR* path{};
        GetRegistryStrValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", L"Desktop", &path);
        WCHAR* secondaryPath = new WCHAR[260];
        WCHAR* cBuffer = new WCHAR[260];

        BYTE* value{};
        GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconLayouts", &value);
        size_t offset = 0x10;
        vector<uint16_t> head;
        for (int i = 0; i < 4; ++i)
        {
            head.push_back(*reinterpret_cast<uint16_t*>(&value[offset + i * 2]));
        }
        head.push_back(*reinterpret_cast<uint32_t*>(&value[offset + 8]));
        uint32_t lviCount = head[4];
        int count2{};
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 2 of 6 complete: Obtained desktop item count.");

        parser->CreateElement(L"emptyspace", nullptr, nullptr, nullptr, (Element**)&emptyspace);
        UIContainer->Add((Element**)&emptyspace, 1);
        RECT dimensions{};
        GetClientRect(wnd->GetHWND(), &dimensions);
        emptyspace->SetX(dimensions.left);
        emptyspace->SetY(dimensions.top);
        emptyspace->SetWidth(dimensions.right);
        emptyspace->SetHeight(dimensions.bottom);

        for (int i = 0; i < lviCount; i++)
        {
            LVItem* outerElem;
            parser->CreateElement(elemname, nullptr, nullptr, nullptr, (Element**)&outerElem);
            UIContainer->Add((Element**)&outerElem, 1);
            if (DWMActive)
            {
                AddLayeredRef(outerElem->GetDisplayNode());
                SetGadgetFlags(outerElem->GetDisplayNode(), NULL, NULL);
            }
            CSafeElementPtr<DDScalableElement> iconElem;
            iconElem.Assign(regElem<DDScalableElement*>(L"iconElem", outerElem));
            if (g_touchmode) assignExtendedFn(iconElem, UpdateTileOnColorChange);
            outerElem->SetInnerElement(regElem<DDScalableElement*>(L"innerElem", outerElem));
            outerElem->SetIcon(iconElem);
            outerElem->SetShortcutArrow(regElem<Element*>(L"shortcutElem", outerElem));
            outerElem->SetText(regElem<RichText*>(L"textElem", outerElem));
            outerElem->SetCheckbox(regElem<TouchButton*>(L"checkboxElem", outerElem));
            pm.push_back(outerElem);
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 3 of 6 complete: Created elements, preparing to enumerate desktop folders.");
        EnumerateFolder((LPWSTR)L"InternalCodeForNamespace", &pm, &count2, lviCount);
        DWORD d = GetEnvironmentVariableW(L"PUBLIC", cBuffer, 260);
        StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        EnumerateFolder(secondaryPath, &pm, &count2, lviCount - count2);
        path1 = secondaryPath;
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        EnumerateFolder(path, &pm, &count2, lviCount - count2);
        path2 = path;
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        d = GetEnvironmentVariableW(L"OneDrive", cBuffer, 260);
        StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
        EnumerateFolder(secondaryPath, &pm, &count2, lviCount - count2);
        path3 = secondaryPath;
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 4 of 6 complete: Created arrays according to your desktop items.");
        for (int i = lviCount - 1; i >= count2; i--)
        {
            pm[i]->Destroy(true);
            pm.erase(pm.begin() + i);
        }
        for (int i = 0; i < lviCount; i++)
        {
            if (pm[i]->GetFlags() & LVIF_HIDDEN)
            {
                pm[i]->GetIcon()->SetAlpha(128);
                pm[i]->GetText()->SetAlpha(128);
            }
            if (animation) pm[i]->AddFlags(LVIF_FLYING);
            else pm[i]->RemoveFlags(LVIF_FLYING);
            if (!g_touchmode)
            {
                if (shellstate[4] & 0x20)
                {
                    pm[i]->SetClass(L"doubleclicked");
                }
                else pm[i]->SetClass(L"singleclicked");
            }
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 5 of 6 complete: Filled the arrays with relevant desktop icon data.");
        RearrangeIcons(false, true, false);
        free(pel_SelectItem), free(pel_MarqueeSelector), free(pel_DesktopRightClick);
        pel_SelectItem = assignFn(emptyspace, SelectItem, true);
        pel_MarqueeSelector = assignExtendedFn(emptyspace, MarqueeSelector, true);
        pel_DesktopRightClick = assignFn(emptyspace, DesktopRightClick, true);
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 6 of 6 complete: Arranged the icons according to your icon placements.");
        if (cBuffer) delete[] cBuffer;
        if (secondaryPath) delete[] secondaryPath;
        if (path) free(path);
        if (value) free(value);
    }

    void InitNewLVItem(const wstring& filepath, const wstring& filename)
    {
        FileInfo* fi = new FileInfo{ filepath, filename };
        PostMessageW(wnd->GetHWND(), WM_USER + 20, NULL, (LPARAM)fi);
    }

    void RemoveLVItem(const wstring& filepath, const wstring& filename)
    {
        wstring foundfilename = (wstring)L"\"" + filepath + (wstring)L"\\" + filename + (wstring)L"\"";
        for (int i = 0; i < pm.size(); i++)
        {
            if (pm[i]->GetFilename() == foundfilename)
            {
                PostMessageW(wnd->GetHWND(), WM_USER + 21, (WPARAM)pm[i], i);
                break;
            }
        }
    }

    void UpdateLVItem(const wstring& filepath, const wstring& filename, BYTE type)
    {
        static int xpos{}, ypos{}, page;
        switch (type)
        {
            case 1:
            {
                wstring foundfilename = (wstring)L"\"" + filepath + (wstring)L"\\" + filename + (wstring)L"\"";
                for (int i = 0; i < pm.size(); i++)
                {
                    if (pm[i]->GetFilename() == foundfilename)
                    {
                        xpos = pm[i]->GetInternalXPos();
                        ypos = pm[i]->GetInternalYPos();
                        page = pm[i]->GetPage();
                        PostMessageW(wnd->GetHWND(), WM_USER + 22, (WPARAM)pm[i], i);
                        break;
                    }
                }
                break;
            }
            case 2:
            {
                yValue* yV = new yValue{ page, static_cast<float>(xpos), static_cast<float>(ypos) };
                FileInfo* fi = new FileInfo{ filepath, filename };
                PostMessageW(wnd->GetHWND(), WM_USER + 20, (WPARAM)yV, (LPARAM)fi);
                break;
            }
        }
    }

    wstring GetExeVersion()
    {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);

        DWORD size = GetFileVersionInfoSizeW(path, nullptr);
        if (size == 0) return L"";

        vector<BYTE> buffer(size);
        if (!GetFileVersionInfoW(path, 0, size, buffer.data()))
            return L"";

        VS_FIXEDFILEINFO* pFileInfo = nullptr;
        UINT len = 0;
        if (VerQueryValueW(buffer.data(), L"\\", (LPVOID*)&pFileInfo, &len)) {
            int edition = HIWORD(pFileInfo->dwFileVersionMS);
            int major = LOWORD(pFileInfo->dwFileVersionMS);
            int minor = HIWORD(pFileInfo->dwFileVersionLS);
            int rev = LOWORD(pFileInfo->dwFileVersionLS);

            WCHAR ver[16];
            StringCchPrintfW(ver, 16, L"%d.%d.%d.%d", edition, major, minor, rev);
            return ver;
        }
        return L"";
    }

    void ShowDebugInfoOnDesktop(bool bUnused1, bool bUnused2, bool bUnused3)
    {
        if (g_debuginfo)
        {
            Element* peBackground;
            Element* peTemp[4];
            Element::Create(0, mainContainer, nullptr, &peBackground);
            peBackground->SetLayoutPos(-2);
            peBackground->SetX(0);
            peBackground->SetY(0);
            peBackground->SetWidth(220 * g_flScaleFactor);
            peBackground->SetHeight(80 * g_flScaleFactor);
            CValuePtr spvLayout;
            BorderLayout::Create(0, nullptr, &spvLayout);
            peBackground->SetValue(Element::LayoutProp, 1, spvLayout);
            peBackground->SetBackgroundStdColor(10005);
            peBackground->SetVisible(true);
            peBackground->SetID(L"DesktopDebugInfo");
            mainContainer->Add(&peBackground, 1);

            for (int i = 0; i < ARRAYSIZE(peTemp); i++)
            {
                Element::Create(0, peBackground, nullptr, &peTemp[i]);
                peBackground->Add(&peTemp[i], 1);
                peTemp[i]->SetFont(L";Normal;None;Consolas");
                peTemp[i]->SetFontSize(14 * g_flScaleFactor);
                peTemp[i]->SetForegroundStdColor(10008);
                peTemp[i]->SetCompositedText(true);
                peTemp[i]->SetTextGlowSize(0);
                peTemp[i]->SetLayoutPos(1);
                peTemp[i]->SetHeight(20 * g_flScaleFactor);
            }
            WCHAR info[256];
            StringCchPrintfW(info, 256, L"Version %s", GetExeVersion().c_str());
            peTemp[0]->SetContentString(info);
            peTemp[1]->SetContentString(L"Build 86");
            StringCchPrintfW(info, 256, L"Build date: %s", BUILD_TIMESTAMP);
            peTemp[2]->SetContentString(info);
            StringCchPrintfW(info, 256, L"Desktop composition: %s", DWMActive ? L"Yes" : L"No");
            peTemp[3]->SetContentString(info);
            DUI_SetGadgetZOrder(peBackground, 4);
        }
        else
        {
            CSafeElementPtr<Element> DesktopDebugInfo;
            DesktopDebugInfo.Assign(regElem<Element*>(L"DesktopDebugInfo", mainContainer));
            if (DesktopDebugInfo)
            {
                DesktopDebugInfo->DestroyAll(true);
                DesktopDebugInfo->Destroy(true);
            }
        }
    }

    void ExitThenOpenLog(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click())
            SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, 420);
    }

    HWND GetWindowIfPresent(NativeHWNDHost* host)
    {
        if (host) return host->GetHWND();
        else return nullptr;
    }

    DWORD GetDesktopActivityFlags()
    {
        DWORD result{};
        HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
        HWND hWnd = GetForegroundWindow();
        if (hWnd == hWndProgman || hWnd == g_hWorkerW) result |= 0x1;
        if (hWnd == g_hWndTaskbar && !g_editmode && !g_issubviewopen && !g_searchopen) result |= 0x2;
        if (hWnd == GetWindowIfPresent(shutdownwnd)) result |= 0x4;
        if (hWnd == GetWindowIfPresent(subviewwnd) || g_issubviewopen) result |= 0x8;
        if (hWnd == GetWindowIfPresent(editwnd) || g_editmode) result |= 0x10;
        if (hWnd == GetWindowIfPresent(searchwnd) || g_searchopen) result |= 0x20;
        return result;
    }

    HHOOK KeyHook = nullptr;
    bool g_dialogopen{};

    LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        static bool keyHold[256]{};
        if (nCode == HC_ACTION)
        {
            KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
            DWORD activity = GetDesktopActivityFlags();
            if ((pKeyInfo->vkCode == 'D' || pKeyInfo->vkCode == 'M') && GetAsyncKeyState(VK_LWIN) & 0x8000)
            {
                if (!keyHold[pKeyInfo->vkCode])
                {
                    if (activity & 0x20) DestroySearchPage();
                    HidePopupCore(true, true);
                    SetWindowPos(g_hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    keyHold[pKeyInfo->vkCode] = true;
                }
            }
            if (activity & 0x5)
            {
                if (pKeyInfo->vkCode == VK_F2)
                {
                    if (!keyHold[pKeyInfo->vkCode] && !g_renameactive && !isIconPressed)
                    {
                        ShowRename(nullptr);
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
                if (pKeyInfo->vkCode == VK_F5)
                {
                    if (!keyHold[pKeyInfo->vkCode] && g_canRefreshMain)
                    {
                        // DO NOT REMOVE THIS TIMER OTHERWISE CRASHING HAPPENS MORE OFTEN
                        SetTimer(wnd->GetHWND(), 2, 200, nullptr);
                        SetTimer(wnd->GetHWND(), 13, 600, nullptr);
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
                if (pKeyInfo->vkCode >= '1' && pKeyInfo->vkCode <= '5' && GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        switch (pKeyInfo->vkCode)
                        {
                            case '1':
                                SetView(144, 64, 48, false);
                                break;
                            case '2':
                                SetView(96, 48, 32, false);
                                break;
                            case '3':
                                SetView(48, 32, 16, false);
                                break;
                            case '4':
                                SetView(32, 32, 12, false);
                                break;
                            case '5':
                                SetView(32, 32, 12, true);
                                break;
                        }
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
            }
            if (activity & 0x7)
            {
                if ((pKeyInfo->vkCode == VK_F4) && GetAsyncKeyState(VK_MENU) & 0x8000)
                {
                    static bool valid{};
                    valid = !valid;
                    if (valid) SetTimer(wnd->GetHWND(), 4, 100, nullptr);
                    return 1;
                }
            }
            if (activity & 0x11)
            {
                if ((pKeyInfo->vkCode == VK_F10 && GetAsyncKeyState(VK_SHIFT) & 0x8000))
                {
                    Event* iev = new Event{ nullptr, TouchButton::RightClick };
                    DesktopRightClick(emptyspace, iev);
                    delete iev;
                }
                if ((pKeyInfo->vkCode == VK_LEFT || pKeyInfo->vkCode == VK_RIGHT) && GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        DWORD delay = g_editmode ? 200 : 20;
                        RECT dimensions{};
                        GetClientRect(wnd->GetHWND(), &dimensions);
                        switch (pKeyInfo->vkCode)
                        {
                        case VK_LEFT:
                            if (localeType == 1 && g_currentPageID < g_maxPageID) SetTimer(wnd->GetHWND(), 9, delay, nullptr);
                            else if (localeType != 1 && g_currentPageID > 1) SetTimer(wnd->GetHWND(), 8, delay, nullptr);
                            else SetTimer(wnd->GetHWND(), 6, 60, nullptr);
                            break;
                        case VK_RIGHT:
                            if (localeType == 1 && g_currentPageID > 1) SetTimer(wnd->GetHWND(), 8, delay, nullptr);
                            else if (localeType != 1 && g_currentPageID < g_maxPageID) SetTimer(wnd->GetHWND(), 9, delay, nullptr);
                            else SetTimer(wnd->GetHWND(), 6, 60, nullptr);
                            break;
                        }
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
            }
            if (activity & 0x15)
            {
                if (pKeyInfo->vkCode == VK_F3)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        SetTimer(wnd->GetHWND(), 14, 150, nullptr);
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
            }
            if (activity & 0x17)
            {
                if (pKeyInfo->vkCode == 'E' && GetAsyncKeyState(VK_LWIN) & 0x8000 && GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        DWORD delay = g_editmode ? 0 : 400;
                        SetTimer(wnd->GetHWND(), 1, delay, nullptr);
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
            }
            if (!(activity & 0x28))
            {
                if ((pKeyInfo->vkCode == 'Q' && GetAsyncKeyState(VK_LWIN) & 0x8000 && GetAsyncKeyState(VK_MENU) & 0x8000))
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        SetTimer(wnd->GetHWND(), 15, 150, nullptr);
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
            }
            if (activity & 0x3D)
            {
                if (pKeyInfo->vkCode == VK_ESCAPE)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        if (activity & 0x4) DestroyShutdownDialog();
                        if (activity & 0x8) HidePopupCore(false, true);
                        if (g_pageviewer)
                        {
                            RefreshSimpleView(0x0);
                            TriggerEMToPV(true);
                        }
                        else if (activity & 0x11) HideSimpleView(true);
                        if (activity & 0x20) DestroySearchPage();
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
            }
            if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                keyHold[pKeyInfo->vkCode] = false;
            }
        }
        return CallNextHookEx(KeyHook, nCode, wParam, lParam);
    }

    static HWND InitializeCallbackWindow()
    {
        NativeHWNDHost* msgwnd{};
        NativeHWNDHost::Create(L"DD_MessageCallback", L"", nullptr, nullptr, 0, 0, 0, 0, NULL, NULL, nullptr, NULL, &msgwnd);
        WndProcMessagesOnly = (WNDPROC)SetWindowLongPtrW(msgwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)MsgWindowProc);

        return msgwnd->GetHWND();
    }

    // Windows.UI.Immersive.dll ordinal 100
    typedef HRESULT (WINAPI*RegisterImmersiveBehaviors_t)();

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
    typedef void (WINAPI*UnregisterImmersiveBehaviors_t)();

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

    HMODULE g_hModTWinUI = GetModuleHandleW(L"twinui.dll");
    HANDLE g_hToken;

    static HRESULT WINAPI s_InitializeDUI(HMODULE hModule = g_hModTWinUI)
    {
        LoadLibraryW(L"twinui.dll");
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
}

using namespace DirectDesktop;

// @TODO: Split into functions
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    WCHAR* WindowsBuildStr;
    GetRegistryStrValues(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", &WindowsBuildStr);
    int WindowsBuild = _wtoi(WindowsBuildStr);
    free(WindowsBuildStr);
    if (WindowsBuild < 18362)
    {
        WCHAR currentBuild[260];
        StringCchPrintfW(currentBuild, 260, LoadStrFromRes(4093).c_str(), WindowsBuild);
        TaskDialog(nullptr, HINST_THISCOMPONENT, L"DirectDesktop", LoadStrFromRes(4092).c_str(),
            currentBuild, TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, nullptr);
        return 1;
    }
    hMutex = CreateMutex(nullptr, TRUE, szWindowClass);
    if (!hMutex || ERROR_ALREADY_EXISTS == GetLastError())
    {
        TaskDialog(nullptr, HINST_THISCOMPONENT, LoadStrFromRes(4025).c_str(), nullptr,
                   LoadStrFromRes(4021).c_str(), TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, nullptr);
        return 1;
    }
    if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"FFlags") & 0x4);
    else
    {
        TaskDialog(nullptr, HINST_THISCOMPONENT, L"DirectDesktop", LoadStrFromRes(4022).c_str(),
                   LoadStrFromRes(4023).c_str(), TDCBF_CLOSE_BUTTON, TD_WARNING_ICON, nullptr);
        return 1;
    }

    DWMActive = IsCompositionActive();

    TOKEN_PRIVILEGES tkp;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &g_hToken);
    LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(g_hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)nullptr, nullptr);

    s_InitializeDUI();
    g_msgwnd = InitializeCallbackWindow();
    RegisterAllControls();
    DDScalableElement::Register();
    DDScalableButton::Register();
    DDScalableRichText::Register();
    DDScalableTouchButton::Register();
    DDScalableTouchEdit::Register();
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
    DDNotificationBanner::Register();
    HRESULT hr = OleInitialize(nullptr);

    WCHAR localeName[256]{};
    ULONG numLanguages{};
    ULONG bufferSize = sizeof(localeName) / sizeof(WCHAR);
    GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, localeName, &bufferSize);
    GetLocaleInfoEx(localeName, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER, (LPWSTR)&localeType, sizeof(localeType) / sizeof(WCHAR));
    RECT dimensions;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, g_dpi) + GetSystemMetricsForDpi(SM_CXEDGE, g_dpi) * 2) * 2;
    int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, g_dpi) + GetSystemMetricsForDpi(SM_CYEDGE, g_dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
    InitialUpdateScale();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated scaling.");
    bool checklog{};
    if (GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop\\Debug", L"DebugMode") == 1)
    {
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop\\Debug", L"Logging", 0, true, &checklog);
        if (checklog)
        {
            TaskDialog(nullptr, HINST_THISCOMPONENT, L"DirectDesktop", LoadStrFromRes(4017).c_str(),
                LoadStrFromRes(4018).c_str(), TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &logging);
            SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop\\Debug", L"Logging", logging, false, nullptr);
        }
        else logging = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop\\Debug", L"Logging");
    }
    if (logging == IDYES)
    {
        wchar_t* docsfolder = new wchar_t[260];
        wchar_t* cBuffer = new wchar_t[260];
        DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
        StringCchPrintfW(docsfolder, 260, L"%s\\Documents", cBuffer);
        MainLogger.StartLogger(((wstring)docsfolder + L"\\DirectDesktop.log").c_str());
        delete[] docsfolder;
        delete[] cBuffer;
    }
    HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
    if (hWndProgman)
    {
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Found the Program Manager window.");
        g_hSHELLDLL_DefView = FindWindowExW(hWndProgman, nullptr, L"SHELLDLL_DefView", nullptr);
        if (logging == IDYES && g_hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: Found a SHELLDLL_DefView window.");
        if (WindowsBuild >= 26002 && logging == IDYES) MainLogger.WriteLine(L"Information: Version is 24H2, skipping WorkerW creation!!!");
        SendMessageTimeoutW(hWndProgman, 0x052C, 13, 1, SMTO_NORMAL, 200, nullptr);
        Sleep(100);
        if (g_hSHELLDLL_DefView)
        {
            bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &g_hWorkerW, &g_hSHELLDLL_DefView, false);
        }
    }
    if (!g_hSHELLDLL_DefView)
    {
        if (logging == IDYES) MainLogger.WriteLine(L"Information: SHELLDLL_DefView was not inside Program Manager, retrying...");
        bool pos = PlaceDesktopInPos(&WindowsBuild, &hWndProgman, &g_hWorkerW, &g_hSHELLDLL_DefView, true);
    }
    if (logging == IDYES && g_hSHELLDLL_DefView) MainLogger.WriteLine(L"Information: Found a SHELLDLL_DefView window.");
    HWND hSysListView32 = FindWindowExW(g_hSHELLDLL_DefView, nullptr, L"SysListView32", L"FolderView");
    if (hSysListView32)
    {
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Found SysListView32 window to hide.");
        ShowWindow(hSysListView32, SW_HIDE);
    }
    KeyHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, HINST_THISCOMPONENT, 0);
    DWORD dwExStyle = NULL, dwCreateFlags = 0x10;
    if (DWMActive)
    {
        dwExStyle |= WS_EX_NOINHERITLAYOUT | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP;
        dwCreateFlags |= 0x28;
    }
    DUIXmlParser::Create(&g_parser, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
    g_parser->SetXMLFromResource(IDR_UIFILE1, hInstance, HINST_THISCOMPONENT);
    DUIXmlParser::Create(&parser, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
    parser->SetXMLFromResource(IDR_UIFILE2, hInstance, HINST_THISCOMPONENT);
    NativeHWNDHost::Create(L"DD_DesktopHost", L"DirectDesktop", nullptr, nullptr, dimensions.left, dimensions.top, 9999, 9999, dwExStyle, WS_POPUP, nullptr, 0x43, &wnd);
    HWNDElement::Create(wnd->GetHWND(), true, dwCreateFlags, nullptr, &key, (Element**)&parent);
    WTSRegisterSessionNotification(wnd->GetHWND(), NOTIFY_FOR_THIS_SESSION);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_STYLE, 0x56003A40L);
    SetWindowPos(wnd->GetHWND(), nullptr, NULL, NULL, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOMOVE | SWP_NOZORDER);
    WndProc = (WNDPROC)SetWindowLongPtrW(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);

    parser->CreateElement(L"main", parent, nullptr, nullptr, &pMain);
    pMain->SetVisible(true);
    pMain->EndDefer(key);

    LVItem* outerElemTouch;
    parser->CreateElement(L"outerElemTouch", nullptr, nullptr, nullptr, (Element**)&outerElemTouch);
    g_touchSizeX = outerElemTouch->GetWidth();
    g_touchSizeY = outerElemTouch->GetHeight();

    UpdateModeInfo();
    g_themeOld = g_theme;
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated color mode information.");

    sampleText = regElem<Element*>(L"sampleText", pMain);
    mainContainer = regElem<Element*>(L"mainContainer", pMain);
    UIContainer = regElem<Element*>(L"UIContainer", pMain);
    selector = regElem<Element*>(L"selector", pMain);
    prevpageMain = regElem<TouchButton*>(L"prevpageMain", pMain);
    nextpageMain = regElem<TouchButton*>(L"nextpageMain", pMain);
    dragpreview = regElem<Element*>(L"dragpreview", pMain);
    dragpreviewTouch = regElem<Element*>(L"dragpreviewTouch", pMain);

    if (DWMActive)
    {
        AddLayeredRef(selector->GetDisplayNode());
        SetGadgetFlags(selector->GetDisplayNode(), NULL, NULL);
        AddLayeredRef(prevpageMain->GetDisplayNode());
        SetGadgetFlags(prevpageMain->GetDisplayNode(), NULL, NULL);
        AddLayeredRef(nextpageMain->GetDisplayNode());
        SetGadgetFlags(nextpageMain->GetDisplayNode(), NULL, NULL);
        AddLayeredRef(dragpreview->GetDisplayNode());
        SetGadgetFlags(dragpreview->GetDisplayNode(), NULL, NULL);
        AddLayeredRef(dragpreviewTouch->GetDisplayNode());
        SetGadgetFlags(dragpreviewTouch->GetDisplayNode(), NULL, NULL);
    }

    assignFn(prevpageMain, GoToPrevPage);
    assignFn(nextpageMain, GoToNextPage);
    assignExtendedFn(prevpageMain, ShowPageToggle);
    assignExtendedFn(nextpageMain, ShowPageToggle);

    wnd->Host(pMain);
    wnd->ShowWindow(SW_SHOW);
    InitSubview();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been created and shown.");
    SetTheme();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Set the theme successfully.");

    HWND dummyHWnd{};
    if (WindowsBuild >= 26002)
    {
        dummyHWnd = SetParent(wnd->GetHWND(), hWndProgman);
    }
    else dummyHWnd = SetParent(wnd->GetHWND(), g_hSHELLDLL_DefView);
    if (!DWMActive) SetParent(g_hSHELLDLL_DefView, hWndProgman);
    if (logging == IDYES)
    {
        if (dummyHWnd != nullptr) MainLogger.WriteLine(L"Information: DirectDesktop is now a part of Explorer.");
        else MainLogger.WriteLine(L"Error: DirectDesktop is still hosted in its own window.");
    }
    MARGINS m = { -1, -1, -1, -1 };
    if (DWMActive) DwmExtendFrameIntoClientArea(wnd->GetHWND(), &m);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been made transparent.");

    GTRANS_DESC transDesc[1];
    TriggerScaleOut(prevpageMain, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.5f, false, false);
    TransitionStoryboardInfo tsbInfo = {};
    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, prevpageMain->GetDisplayNode(), &tsbInfo);
    TriggerScaleOut(nextpageMain, transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f, false, false);
    ScheduleGadgetTransitions_DWMCheck(0, ARRAYSIZE(transDesc), transDesc, nextpageMain->GetDisplayNode(), &tsbInfo);

    RegKeyValue DDKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", nullptr, NULL);
    g_showcheckboxes = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AutoCheckSelect");
    g_showHidden = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"Hidden");
    g_showSuperHidden = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"ShowSuperHidden");
    g_hideFileExt = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"HideFileExt");
    g_isThumbnailHidden = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"IconsOnly");
    g_iconunderline = GetRegistryValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"IconUnderline");
    g_hiddenIcons = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"HideIcons");
    g_iconsz = GetRegistryValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize");
    GetRegistryBinValues(DDKey.GetHKeyName(), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState", &shellstate);

    DDKey.SetPath(L"Software\\DirectDesktop");
    if (!EnsureRegValueExists(DDKey.GetHKeyName(), DDKey.GetPath(), L"DefaultWidth"))
    {
        g_defWidth = dimensions.right / g_flScaleFactor;
        SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DefaultWidth", g_defWidth, false, nullptr);
    }
    else g_defWidth = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DefaultWidth");
    if (!EnsureRegValueExists(DDKey.GetHKeyName(), DDKey.GetPath(), L"DefaultHeight"))
    {
        g_defHeight = dimensions.bottom / g_flScaleFactor;
        SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DefaultHeight", g_defHeight, false, nullptr);
    }
    else g_defHeight = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DefaultHeight");
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"TreatDirAsGroup", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"TripleClickAndHide", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"LockIconPos", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"TouchView", 0, true, nullptr);
    g_treatdirasgroup = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"TreatDirAsGroup");
    g_tripleclickandhide = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"TripleClickAndHide");
    g_lockiconpos = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"LockIconPos");
    g_touchmode = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"TouchView");
    DDKey.SetPath(L"Software\\DirectDesktop\\Personalize");
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AccentColorIcons", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DarkIcons", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AutoDarkIcons", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"GlassIcons", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"IconColorID", 1, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"IconColorizationColor", 0, true, nullptr);
    g_isColorized = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AccentColorIcons");
    g_isDarkIconsEnabled = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DarkIcons");
    g_automaticDark = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AutoDarkIcons");
    g_isGlass = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"GlassIcons");
    iconColorID = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"IconColorID");
    IconColorizationColor = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"IconColorizationColor");
    DDKey.SetPath(L"Software\\DirectDesktop\\Debug");

    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DebugMode", 0, true, nullptr);
    g_debugmode = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"DebugMode");
    while (*lpCmdLine && iswspace(*lpCmdLine)) {
        ++lpCmdLine;
    }

    if (wcsstr(lpCmdLine, L"-d") || wcsstr(lpCmdLine, L"/d")) {
        g_debugmode = true;
    }

    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AnimationSpeed", 100, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AnimationsShiftKey", 0, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"ShowDebugInfo", 1, true, nullptr);
    SetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"EnableExiting", 1, true, nullptr);
    g_animCoef = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AnimationSpeed");
    g_AnimShiftKey = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"AnimationsShiftKey");
    g_debuginfo = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"ShowDebugInfo");
    g_enableexit = GetRegistryValues(DDKey.GetHKeyName(), DDKey.GetPath(), L"EnableExiting");
    if (!g_debugmode)
    {
        g_animCoef = 100;
        g_AnimShiftKey = false;
        g_debuginfo = false;
        g_enableexit = false;
    }

    AdjustWindowSizes(true);

    WCHAR DesktopLayoutWithSize[24];
    if (!g_touchmode) StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_%d", g_iconsz);
    else StringCchPrintfW(DesktopLayoutWithSize, 24, L"DesktopLayout_Touch");
    if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize))
    {
        BYTE* value2;
        GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize, &value2);
        g_currentPageID = *reinterpret_cast<unsigned short*>(&value2[2]);
        free(value2);
    }

    if (g_automaticDark) g_isDarkIconsEnabled = !g_theme;
    DDScalableElement::Create(nullptr, nullptr, (Element**)&RegistryListener);
    assignExtendedFn(RegistryListener, UpdateIconColorizationColor);
    if (g_touchmode) g_iconsz = 32;
    g_shiconsz = 32;
    if (g_iconsz > 96) g_shiconsz = 64;
    else if (g_iconsz > 48) g_shiconsz = 48;
    g_gpiconsz = 12;
    if (g_iconsz > 120) g_gpiconsz = 48;
    else if (g_iconsz >= 80) g_gpiconsz = 32;
    else if (g_iconsz > 40) g_gpiconsz = 16;
    InitLayout(true, true, false);

    StartMonitorFileChanges(path1);
    StartMonitorFileChanges(path2);
    StartMonitorFileChanges(path3);

    if (g_debuginfo) ShowDebugInfoOnDesktop(false, false, false);

    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialized layout successfully.\n\nLogging is now complete.");

    if (logging == IDYES)
    {
        CSafeElementPtr<DDNotificationBanner> ddnb;
        ddnb.Assign(new DDNotificationBanner);
        ddnb->CreateBanner(DDNT_SUCCESS, LoadStrFromRes(4019).c_str(), LoadStrFromRes(4020).c_str(), NULL);
        ddnb->AppendButton(LoadStrFromRes(4160, L"comctl32.dll").c_str(), ExitThenOpenLog, true);
        ddnb->AppendButton(LoadStrFromRes(4240, L"comctl32.dll").c_str(), nullptr, true);
        logging = IDNO;
    }
    StartMessagePump();
    UnInitProcess();
    WTSUnRegisterSessionNotification(wnd->GetHWND());
    CoUninitialize();
    if (KeyHook)
    {
        UnhookWindowsHookEx(KeyHook);
        KeyHook = nullptr;
    }

    return 0;
}