#include "pch.h"

#include "DirectDesktop.h"

#include <cmath>
#include <list>
#include <powrprof.h>
#include <shlwapi.h>
#include <ShObjIdl.h>
#include <vector>
#include <WinUser.h>
#include <wrl.h>
#include <wtsapi32.h>

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

using namespace DirectUI;
using namespace std;
using namespace Microsoft::WRL;

namespace DirectDesktop
{
    NativeHWNDHost *wnd, *subviewwnd;
    HWNDElement *parent, *subviewparent;
    DUIXmlParser *parser, *parserSubview;
    Element *pMain, *pSubview;
    unsigned long key = 0, key2 = 0;

    Element* sampleText;
    Element* mainContainer;
    DDScalableButton* fullscreeninner;
    Element* popupcontainer;
    Element* fullscreenpopupbg;
    Button *fullscreenpopupbase, *centered;
    Button* emptyspace;
    LVItem* g_outerElem;
    Element* selector;
    DDScalableButton *PageTab1, *PageTab2, *PageTab3;
    DDScalableButton* SubUIContainer;
    TouchButton *prevpageMain, *nextpageMain;
    Element* dragpreview;

    DDScalableElement* RegistryListener;
    void *g_tempElem, *g_tempElem2;
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
        GTRANS_DESC rTrans = {};
        rTrans.hgadChange = pe->GetDisplayNode();
        rTrans.nProperty = 10;
        rTrans.zOrder = (int)uZOrder;
        SetTransitionVisualProperties(0, 1, &rTrans);
    }

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

    template <typename elemType>
    elemType regElem(const wchar_t* elemName, Element* peParent)
    {
        elemType result = (elemType)peParent->FindDescendent(StrToID(elemName));
        return result;
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

    double px[80]{};
    double py[80]{};
    int origX{}, origY{}, g_iconsz, g_iconszOld, g_shiconsz, g_gpiconsz;
    int g_emptyclicks = 1;
    bool g_touchmode{};
    bool g_renameactive{};
    bool isPressed = 0, isIconPressed = 0;
    bool delayedshutdownstatuses[6] = { false, false, false, false, false, false };

    void CubicBezier(const int frames, double px[], double py[], double x0, double y0, double x1, double y1)
    {
        for (int c = 0; c < frames; c++)
        {
            double t = (1.0 / frames) * c;
            px[c] = (3 * t * pow(1 - t, 2) * x0) + (3 * pow(t, 2) * (1 - t) * x1) + pow(t, 3);
            py[c] = (3 * t * pow(1 - t, 2) * y0) + (3 * pow(t, 2) * (1 - t) * y1) + pow(t, 3);
        }
        px[frames - 1] = 1;
        py[frames - 1] = 1;
    }

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

    WNDPROC WndProc, WndProcSubview, WndProcMessagesOnly;
    HANDLE hMutex;
    constexpr LPCWSTR szWindowClass = L"DIRECTDESKTOP";
    BYTE* shellstate;
    vector<LVItem*> pm;
    vector<Element*> shortpm;
    vector<DDScalableElement*> iconpm;
    vector<Element*> shadowpm;
    vector<RichText*> filepm;
    vector<RichText*> fileshadowpm;
    vector<Element*> cbpm;
    vector<LVItem*> selectedLVItems;
    bool g_launch = true;
    bool g_checkifelemexists = false;
    bool g_issubviewopen = false;
    bool g_issettingsopen = false;
    bool g_hiddenIcons;
    bool g_editmode = false;
    bool g_pendingaction = false;
    bool g_invokedpagechange = false;
    bool g_delayGroupsForDpi = false;
    bool g_ensureNoRefresh = false;
    void fullscreenAnimation(int width, int height, float animstartscale, float desktopanimstartscale);
    void HidePopupCore(bool WinDInvoked);
    void TogglePage(Element* pageElem, float offsetL, float offsetT, float offsetR, float offsetB);
    void ApplyIcons(vector<LVItem*> pmLVItem, vector<DDScalableElement*> pmIcon, DesktopIcon* di, bool subdirectory, int id, float scale);
    void IconThumbHelper(int id);
    DWORD WINAPI CreateIndividualThumbnail(LPVOID lpParam);
    DWORD WINAPI InitDesktopGroup(LPVOID lpParam);
    DWORD WINAPI SetLVIPos(LPVOID lpParam);
    DWORD WINAPI RearrangeIconsHelper(LPVOID lpParam);
    void ShowDirAsGroupDesktop(LVItem* lvi);
    void SelectItem(Element* elem, Event* iev);
    void SelectItem2(Element* elem, Event* iev);
    void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void ItemDragListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);
    void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2);

    bool GetShellItemImage(HBITMAP& hBitmap, LPCWSTR filePath, int width, int height)
    {
        static const HINSTANCE hImageres = LoadLibraryW(L"imageres.dll");

        if (hBitmap)
        {
            DeleteObject(hBitmap);
            hBitmap = nullptr;
        }
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) return false;

        ComPtr<IShellItem2> pShellItem{};
        hr = SHCreateItemFromParsingName(filePath, nullptr, IID_PPV_ARGS(&pShellItem));

        if (SUCCEEDED(hr) && pShellItem)
        {
            ComPtr<IShellItemImageFactory> pImageFactory{};
            hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
            if (SUCCEEDED(hr))
            {
                SIZE size = { width * g_flScaleFactor, height * g_flScaleFactor };
                if (pImageFactory)
                {
                    if (SUCCEEDED(pImageFactory->GetImage(size, SIIGBF_RESIZETOFIT, &hBitmap)));
                    else hBitmap = nullptr;
                }
            }
            else
            {
                HICON fallback = (HICON)LoadImageW(hImageres, MAKEINTRESOURCE(2), IMAGE_ICON, width * g_flScaleFactor, height * g_flScaleFactor, LR_SHARED);
                IconToBitmap(fallback, hBitmap, width * g_flScaleFactor, height * g_flScaleFactor);
            }
        }
        else
        {
            HICON fallback = (HICON)LoadImageW(hImageres, MAKEINTRESOURCE(2), IMAGE_ICON, width * g_flScaleFactor, height * g_flScaleFactor, LR_SHARED);
            IconToBitmap(fallback, hBitmap, width * g_flScaleFactor, height * g_flScaleFactor);
        }

        CoUninitialize();
        return true;
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

    void CalcDesktopIconInfo(yValue* yV, int* lines_basedOnEllipsis, DWORD* alignment, bool subdirectory, vector<LVItem*>* pmLVItem, vector<RichText*>* pmFile)
    {
        vector<RichText*>* pmFileShadow = &fileshadowpm;
        *alignment = DT_CENTER | DT_END_ELLIPSIS;
        if (!g_touchmode)
        {
            *lines_basedOnEllipsis = floor(CalcTextLines((*pmLVItem)[yV->num]->GetSimpleFilename().c_str(), yV->fl1 - 4 * g_flScaleFactor)) * textm.tmHeight;
        }
        if (g_touchmode)
        {
            DWORD direction = (localeType == 1) ? DT_RIGHT : DT_LEFT;
            *alignment = direction | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS;
            int maxlines_basedOnEllipsis = (*pmFile)[yV->num]->GetHeight();
            yV->fl1 = (*pmFile)[yV->num]->GetWidth();
            *lines_basedOnEllipsis = CalcTextLines((*pmLVItem)[yV->num]->GetSimpleFilename().c_str(), yV->fl1) * textm.tmHeight;
            if (*lines_basedOnEllipsis > maxlines_basedOnEllipsis) *lines_basedOnEllipsis = maxlines_basedOnEllipsis;
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
            if (pm[items]->GetPage() == g_currentPageID)
            {
                pm[items]->SetVisible(!g_hiddenIcons);
                GTRANS_DESC transDesc[2];
                TransitionStoryboardInfo tsbInfo = {};
                TriggerTranslate(pm[items], transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 9999, pm[items]->GetY(), 9999, pm[items]->GetY(), false, false);
                TriggerTranslate(pm[items], transDesc, 1, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX(), pm[items]->GetY(), false, false);
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(pm[items], -1);
            }
            else if (!g_editmode && pm[items]->GetPage() == g_currentPageID - direction) pm[items]->SetVisible(!g_hiddenIcons);
            else pm[items]->SetVisible(false);
        }
        short animSrc = (localeType == 1) ? direction * -1 : direction;
        float originX = 0.5f + 0.2f * animSrc;
        animSrc *= dimensions.right;
        GTRANS_DESC transDesc[5];
        TransitionStoryboardInfo tsbInfo = {};
        for (int items = 0; items < pm.size(); items++)
        {
            if (pm[items]->GetPage() == g_currentPageID - direction)
            {
                float offset = animSrc * -1.14f;
                TriggerTranslate(pm[items], transDesc, 0, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX() + offset, pm[items]->GetY(), pm[items]->GetX() + offset, pm[items]->GetY(), false, false);
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc) - 4, transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(pm[items], -1);
            }
        }
        TriggerScaleOut(UIContainer, transDesc, 0, 0.0f, 0.45f, 0.1f, 0.9f, 0.2f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, false);
        TriggerTranslate(UIContainer, transDesc, 1, 0.1f, 0.6f, 0.1f, 0.9f, 0.2f, 1.0f, animSrc, 0.0f, 0.0f, 0.0f, false, false);
        TriggerScaleIn(UIContainer, transDesc, 2, 0.25f, 0.6f, 0.15f, 0.54f, 0.4f, 0.88f, 0.88f, 0.88f, originX, 0.5f, 1.0f, 1.0f, originX, 0.5f, false, false);
        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc) - 2, transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
        DUI_SetGadgetZOrder(UIContainer, -1);
        Element* pageVisual[2];
        for (int i = 0; i < 2; i++)
        {
            static float animSrc2 = 0;
            // 0.5 M9: Temporarily looks like that in case I change my mind and make it animate 2 overlays side by side
            if (i == 1)
            {
                parser->CreateElement(L"pageVisual", nullptr, nullptr, nullptr, &pageVisual[i]);
                mainContainer->Add(&pageVisual[i], 1);
                pageVisual[i]->SetWidth(dimensions.right);
                pageVisual[i]->SetHeight(dimensions.bottom);
                //TriggerScaleOut(pageVisual[i], transDesc, 0, 0.0f, 0.45f, 0.1f, 0.9f, 0.2f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, false);
                TriggerTranslate(pageVisual[i], transDesc, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, animSrc2, 0.0f, animSrc2, 0.0f, false, false);
                TriggerScaleOut(pageVisual[i], transDesc, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.88f, 0.88f, 0.5f, 0.5f, false, false);
                TriggerTranslate(pageVisual[i], transDesc, 2, 0.1f, 0.6f, 0.1f, 0.9f, 0.2f, 1.0f, animSrc2, 0.0f, animSrc * -1, 0.0f, false, false);
                TriggerFade(pageVisual[i], transDesc, 3, 0.367f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false);
                TriggerScaleIn(pageVisual[i], transDesc, 4, 0.25f, 0.6f, 0.15f, 0.54f, 0.4f, 0.88f, 0.88f, 0.88f, originX, 0.5f, 1.0f, 1.0f, originX, 0.5f, false, true);
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, pageVisual[i]->GetDisplayNode(), &tsbInfo);
                DUI_SetGadgetZOrder(pageVisual[i], -2);
            }
            animSrc2 = animSrc;
            animSrc = 0;
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
        SetWindowPos(wnd->GetHWND(), nullptr, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
        SetWindowPos(subviewwnd->GetHWND(), nullptr, dimensions.left, dimensions.top, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
        if (editwnd)
        {
            SetWindowPos(editwnd->GetHWND(), nullptr, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
            //SetWindowPos(editbgwnd->GetHWND(), NULL, dimensions.left - topLeftMon.x, dimensions.top - topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, SWP_NOZORDER);
        }
        HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
        SetWindowPos(hWndProgman, nullptr, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
        SetWindowPos(g_hWorkerW, nullptr, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
        SetWindowPos(g_hSHELLDLL_DefView, nullptr, dimensions.left + topLeftMon.x, dimensions.top + topLeftMon.y, dimensions.right - dimensions.left, dimensions.bottom - dimensions.top, swpFlags);
        UIContainer->SetWidth(dimensions.right - dimensions.left);
        UIContainer->SetHeight(dimensions.bottom - dimensions.top);
        SetWindowPos(g_hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
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
        switch (uMsg)
        {
            case WM_SETTINGCHANGE:
            {
                if (wParam == SPI_SETWORKAREA)
                {
                    if (isDefaultRes()) SetPos(true);
                    g_lastWidth = 0, g_lastHeight = 0;
                    AdjustWindowSizes(false);
                    RearrangeIcons(true, false, true);
                }
                if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)
                {
                    UpdateModeInfo();
                    // This message is sent 4-5 times upon changing accent color so this mitigation is applied
                    // 0.4.5.2 test case: seems to be sent 3-4 times. Maybe dependent on Windows install?
                    static int messagemitigation{};
                    messagemitigation++;
                    SetTheme();
                    SetPos(false);
                    GetPos2(false);
                    g_delayGroupsForDpi = true;
                    //DDScalableElement::RedrawImages();
                    //DDScalableButton::RedrawImages();
                    RearrangeIcons(false, true, true);
                    if (g_automaticDark)
                    {
                        g_isDarkIconsEnabled = !g_theme;
                    }
                    if (messagemitigation % 4 == 2)
                    {
                        // was originally 5
                        if (g_isColorized) RearrangeIcons(false, true, true);
                    }
                }
                break;
            }
            case WM_WINDOWPOSCHANGING:
            {
                ((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
                return 0L;
                break;
            }
            case WM_WTSSESSION_CHANGE:
            {
                //int WindowsBuild = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\ShellCommon", L"BuildNumber");
                //yValue* yV = new yValue{ WindowsBuild, 2000, NULL };
                //switch (wParam)
                //{
                //    case WTS_SESSION_LOCK:
                //    {
                //        Perform24H2Fixes(false);
                //        break;
                //    }
                //    case WTS_SESSION_UNLOCK:
                //    {
                //        if (WindowsBuild >= 26002)
                //        {
                //            WallpaperHelper24H2(yV);
                //        }
                //        break;
                //    }
                //}
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
                DWORD dwTermination{};
                HANDLE termThread = CreateThread(nullptr, 0, EndExplorer, nullptr, 0, &dwTermination);
                if (termThread) CloseHandle(termThread);
                Sleep(500);
                pMain->Destroy(true);
                pSubview->Destroy(true);
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
                switch (wParam)
                {
                    case 1:
                        SendMessageW(hWnd, WM_USER + 15, NULL, 1);
                        break;
                    case 2:
                        InitLayout(false, true, true);
                        break;
                    case 3:
                        CreateSearchPage();
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
                        if (GetTickCount64() - g_lastDpiChangeTick > 1250 || g_lastDpiChangeTick - GetTickCount64() > 1250)
                            RearrangeIcons(true, false, false);
                        break;
                    case 6:
                        MessageBeep(MB_OK);
                        GTRANS_DESC transDesc[4];
                        TransitionStoryboardInfo tsbInfo = {};
                        TriggerScaleOut(UIContainer, transDesc, 0, 0.0f, 0.3f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, false, false);
                        TriggerScaleIn(UIContainer, transDesc, 1, 0.35f, 0.6f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc) - 2, transDesc, UIContainer->GetDisplayNode(), &tsbInfo);
                        DUI_SetGadgetZOrder(UIContainer, -1);
                        Element* pageVisual;
                        parser->CreateElement(L"pageVisual", nullptr, nullptr, nullptr, &pageVisual);
                        mainContainer->Add(&pageVisual, 1);
                        pageVisual->SetWidth(dimensions.right);
                        pageVisual->SetHeight(dimensions.bottom);
                        TriggerFade(pageVisual, transDesc, 0, 0.0f, 0.133f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                        TriggerScaleOut(pageVisual, transDesc, 1, 0.0f, 0.3f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, false, false);
                        TriggerFade(pageVisual, transDesc, 2, 0.4f, 0.533f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false);
                        TriggerScaleIn(pageVisual, transDesc, 3, 0.35f, 0.6f, 0.11f, 0.6f, 0.23f, 0.97f, 0.9f, 0.9f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, true);
                        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, pageVisual->GetDisplayNode(), &tsbInfo);
                        DUI_SetGadgetZOrder(pageVisual, -2);
                        break;
                }
                break;
            }
            case WM_USER + 1:
            {
                for (LVItem* lvi : pm)
                {
                    if (lvi->GetPage() == g_currentPageID)
                    {
                        if (lvi->GetFlying())
                        {
                            int coef = g_launch ? 2 : 3;
                            float delay = (lvi->GetY() + lvi->GetHeight() / 2) / static_cast<float>(dimensions.bottom * coef);
                            float startXPos = ((dimensions.right / 2.0f) - (lvi->GetX() + (lvi->GetWidth() / 2))) * 0.2f;
                            float startYPos = ((dimensions.bottom / 2.0f) - (lvi->GetY() + (lvi->GetHeight() / 2))) * 0.2f;
                            float scale = g_launch ? 1.33f : 0.67f;
                            GTRANS_DESC transDesc[3];
                            TriggerTranslate(lvi, transDesc, 0, delay, delay + scale, 0.1f, 0.9f, 0.2f, 1.0f, lvi->GetX() + startXPos, lvi->GetY() + startYPos, lvi->GetX(), lvi->GetY(), false, false);
                            TriggerFade(lvi, transDesc, 1, delay, delay + 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                            TriggerScaleIn(lvi, transDesc, 2, delay, delay + scale, 0.1f, 0.9f, 0.2f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                            TransitionStoryboardInfo tsbInfo = {};
                            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, lvi->GetDisplayNode(), &tsbInfo);
                            lvi->SetFlying(false);
                        }
                        else if (lvi->GetMoving())
                        {
                            GTRANS_DESC transDesc[1];
                            TransitionStoryboardInfo tsbInfo = {};
                            if (lvi->GetMemPage() == lvi->GetPage())
                            {
                                TriggerTranslate(lvi, transDesc, 0, 0.0f, 0.4f, 0.75f, 0.45f, 0.0f, 1.0f, lvi->GetX(), lvi->GetY(), lvi->GetMemXPos(), lvi->GetMemYPos(), false, false);
                                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, lvi->GetDisplayNode(), &tsbInfo);
                                if (lvi->GetGroupSize() == LVIGS_NORMAL && g_iconszOld != g_iconsz && !g_touchmode)
                                {
                                    CSafeElementPtr<DDScalableElement> icon; icon.Assign(regElem<DDScalableElement*>(L"iconElem", lvi));
                                    CSafeElementPtr<DDScalableElement> shadow; shadow.Assign(regElem<DDScalableElement*>(L"iconElemshadow", lvi));
                                    float scaleOrigX = lvi->GetX() / static_cast<float>(dimensions.right);
                                    float scaleOrigY = lvi->GetY() / static_cast<float>(dimensions.bottom);
                                    float scaling = g_iconszOld / static_cast<float>(g_iconsz);
                                    TriggerScaleIn(icon, transDesc, 0, 0.0f, 0.4f, 0.75f, 0.45f, 0.0f, 1.0f, scaling, scaling, scaleOrigX, scaleOrigY, 1.0f, 1.0f, scaleOrigX, scaleOrigY, false, false);
                                    ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, icon->GetDisplayNode(), &tsbInfo);
                                    scaling = (g_iconszOld + 16) / static_cast<float>(g_iconsz + 16);
                                    TriggerScaleIn(shadow, transDesc, 0, 0.0f, 0.4f, 0.75f, 0.45f, 0.0f, 1.0f, scaling, scaling, scaleOrigX, scaleOrigY, 1.0f, 1.0f, scaleOrigX, scaleOrigY, false, false);
                                    ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, shadow->GetDisplayNode(), &tsbInfo);
                                    DUI_SetGadgetZOrder(shadow, -3);
                                    DUI_SetGadgetZOrder(icon, -2);
                                }
                            }
                            else
                            {
                                TriggerFade(lvi, transDesc, 0, 0.0f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, lvi->GetDisplayNode(), &tsbInfo);
                            }
                            DelayedElementActions* dea = new DelayedElementActions{ 500, lvi, static_cast<float>(lvi->GetMemXPos()), static_cast<float>(lvi->GetMemYPos()) };
                            DWORD DelayedSetPos;
                            HANDLE hDelayedSetPos = CreateThread(nullptr, 0, SetLVIPos, dea, NULL, nullptr);
                            if (hDelayedSetPos) CloseHandle(hDelayedSetPos);
                            lvi->SetMoving(false);
                        }
                        DUI_SetGadgetZOrder(lvi, -1);
                    }
                    lvi->SetVisible(lvi->GetPage() == g_currentPageID && !g_hiddenIcons);
                }
                g_iconszOld = g_iconsz;
                if (g_launch) g_launch = false;
                break;
            }
            case WM_USER + 2:
            {
                yValueEx* yV = (yValueEx*)lParam;
                vector<LVItem*>* l_pm = yV->vpm;
                for (int num = 0; num < yV->num; num++)
                {
                    (*l_pm)[num]->SetVisible(true);
                }
                break;
            }
            case WM_USER + 3:
            {
                int lines_basedOnEllipsis{};
                pm[lParam]->ClearAllListeners();
                vector<IElementListener*> v_pels;
                v_pels.push_back(assignExtendedFn(pm[lParam], ItemDragListener, true));
                v_pels.push_back(assignFn(pm[lParam], ItemRightClick, true));
                if (!g_treatdirasgroup || pm[lParam]->GetGroupSize() == LVIGS_NORMAL)
                {
                    v_pels.push_back(assignFn(pm[lParam], SelectItem, true));
                    v_pels.push_back(assignExtendedFn(pm[lParam], SelectItemListener, true));
                    v_pels.push_back(assignExtendedFn(pm[lParam], ShowCheckboxIfNeeded, true));
                    v_pels.push_back(assignExtendedFn(cbpm[lParam], CheckboxHandler, true));
                }
                CSafeElementPtr<Element> groupdirectoryOld;
                groupdirectoryOld.Assign(regElem<Element*>(L"groupdirectory", pm[lParam]));
                if (groupdirectoryOld)
                {
                    groupdirectoryOld->Destroy(true);
                    pm[lParam]->GetChildItems().clear();
                    pm[lParam]->GetChildIcons().clear();
                    pm[lParam]->GetChildShadows().clear();
                    pm[lParam]->GetChildShortcutArrows().clear();
                    pm[lParam]->GetChildFilenames().clear();
                }
                if (g_treatdirasgroup && pm[lParam]->GetGroupSize() != LVIGS_NORMAL)
                {
                    iconpm[lParam]->SetX(0);
                    iconpm[lParam]->SetY(0);
                    pm[lParam]->SetSelected(false);
                    switch (pm[lParam]->GetGroupSize())
                    {
                        case LVIGS_SMALL:
                            if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 316);
                            pm[lParam]->SetWidth(316 * g_flScaleFactor);
                            pm[lParam]->SetHeight(200 * g_flScaleFactor);
                            iconpm[lParam]->SetWidth(316 * g_flScaleFactor);
                            iconpm[lParam]->SetHeight(200 * g_flScaleFactor);
                            break;
                        case LVIGS_MEDIUM:
                            if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 476);
                            pm[lParam]->SetWidth(476 * g_flScaleFactor);
                            pm[lParam]->SetHeight(300 * g_flScaleFactor);
                            iconpm[lParam]->SetWidth(476 * g_flScaleFactor);
                            iconpm[lParam]->SetHeight(300 * g_flScaleFactor);
                            break;
                        case LVIGS_WIDE:
                            if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 716);
                            pm[lParam]->SetWidth(716 * g_flScaleFactor);
                            pm[lParam]->SetHeight(300 * g_flScaleFactor);
                            iconpm[lParam]->SetWidth(716 * g_flScaleFactor);
                            iconpm[lParam]->SetHeight(300 * g_flScaleFactor);
                            break;
                        case LVIGS_LARGE:
                            if (localeType == 1) pm[lParam]->SetX(pm[lParam]->GetX() + pm[lParam]->GetWidth() - 716);
                            pm[lParam]->SetWidth(716 * g_flScaleFactor);
                            pm[lParam]->SetHeight(450 * g_flScaleFactor);
                            iconpm[lParam]->SetWidth(716 * g_flScaleFactor);
                            iconpm[lParam]->SetHeight(450 * g_flScaleFactor);
                            break;
                    }
                    pm[lParam]->SetTooltip(false);
                    CSafeElementPtr<Element> innerElem;
                    innerElem.Assign(regElem<Element*>(L"innerElem", pm[lParam]));
                    innerElem->SetLayoutPos(-3);
                    cbpm[lParam]->SetLayoutPos(-3);
                    filepm[lParam]->SetLayoutPos(-3);
                    if (!g_touchmode) fileshadowpm[lParam]->SetLayoutPos(-3);
                    else
                    {
                        CSafeElementPtr<Element> containerElem;
                        containerElem.Assign(regElem<Element*>(L"containerElem", pm[lParam]));
                        containerElem->SetPadding(0, 0, 0, 0);
                    }
                    pm[lParam]->SetBackgroundStdColor(20575);
                    pm[lParam]->SetDrawType(0);
                    v_pels.push_back(assignFn(pm[lParam], SelectItem2, true));
                    int* itemID = (int*)(&lParam);
                    HANDLE hCreateGroup = CreateThread(nullptr, 0, InitDesktopGroup, itemID, 0, nullptr);
                    if (hCreateGroup) CloseHandle(hCreateGroup);
                }
                else if (!g_touchmode)
                {
                    lines_basedOnEllipsis = floor(CalcTextLines(pm[lParam]->GetSimpleFilename().c_str(), innerSizeX - 4 * g_flScaleFactor)) * textm.tmHeight;
                    pm[lParam]->SetWidth(innerSizeX);
                    pm[lParam]->SetHeight(innerSizeY + lines_basedOnEllipsis + 6 * g_flScaleFactor);
                    filepm[lParam]->SetHeight(lines_basedOnEllipsis + 4 * g_flScaleFactor);
                    fileshadowpm[lParam]->SetHeight(lines_basedOnEllipsis + 5 * g_flScaleFactor);
                    iconpm[lParam]->SetWidth(round(g_iconsz * g_flScaleFactor));
                    iconpm[lParam]->SetHeight(round(g_iconsz * g_flScaleFactor));
                    iconpm[lParam]->SetX(iconPaddingX);
                    iconpm[lParam]->SetY(round(iconPaddingY * 0.575));
                    if (pm[lParam]->GetSizedFromGroup() == true) pm[lParam]->SetSizedFromGroup(false);
                }
                else
                {
                    if (pm[lParam]->GetTileSize() == LVITS_ICONONLY) filepm[lParam]->SetVisible(false);
                }
                if (g_touchmode && pm[lParam]->GetSizedFromGroup() == true)
                {
                    pm[lParam]->SetWidth(g_touchSizeX);
                    pm[lParam]->SetHeight(g_touchSizeY);
                    pm[lParam]->SetSizedFromGroup(false);
                }
                pm[lParam]->SetListeners(v_pels);
                v_pels.clear();
                if (!wParam) break;
                HBITMAP iconbmp = ((DesktopIcon*)wParam)->icon;
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconbmp);
                if (spvBitmap != nullptr) iconpm[lParam]->SetValue(Element::ContentProp, 1, spvBitmap);
                HBITMAP iconshadowbmp = ((DesktopIcon*)wParam)->iconshadow;
                CValuePtr spvBitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshadowbmp);
                if (spvBitmapShadow != nullptr) shadowpm[lParam]->SetValue(Element::ContentProp, 1, spvBitmapShadow);
                HBITMAP iconshortcutbmp = ((DesktopIcon*)wParam)->iconshortcut;
                CValuePtr spvBitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(iconshortcutbmp);
                if (spvBitmapShortcut != nullptr && pm[lParam]->GetShortcutState() == true) shortpm[lParam]->SetValue(Element::ContentProp, 1, spvBitmapShortcut);
                HBITMAP textbmp = ((DesktopIcon*)wParam)->text;
                CValuePtr spvBitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(textbmp);
                if (spvBitmapText != nullptr) filepm[lParam]->SetValue(Element::ContentProp, 1, spvBitmapText);
                HBITMAP textshadowbmp = ((DesktopIcon*)wParam)->textshadow;
                CValuePtr spvBitmapTextShadow = DirectUI::Value::CreateGraphic(textshadowbmp, 2, 0xffffffff, false, false, false);
                DeleteObject(textshadowbmp);
                if (spvBitmapTextShadow != nullptr) fileshadowpm[lParam]->SetValue(Element::ContentProp, 1, spvBitmapTextShadow);
                if (g_touchmode)
                {
                    ((DDScalableElement*)pm[lParam])->SetDDCPIntensity((pm[lParam]->GetHiddenState() == true) ? 192 : 255);
                    ((DDScalableElement*)pm[lParam])->SetAssociatedColor(((DesktopIcon*)wParam)->crDominantTile);
                }
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
                if (!g_touchmode)
                {
                    shadowpm[lParam]->SetWidth((g_iconsz + 16) * g_flScaleFactor);
                    shadowpm[lParam]->SetHeight((g_iconsz + 16) * g_flScaleFactor);
                    shadowpm[lParam]->SetX(iconPaddingX - 8 * g_flScaleFactor);
                    shadowpm[lParam]->SetY((iconPaddingY * 0.575) - 6 * g_flScaleFactor);
                    shortpm[lParam]->SetWidth(g_shiconsz * g_flScaleFactor);
                    shortpm[lParam]->SetHeight(g_shiconsz * g_flScaleFactor);
                    shortpm[lParam]->SetX(iconPaddingX);
                    shortpm[lParam]->SetY((iconPaddingY * 0.575) + (g_iconsz - g_shiconsz) * g_flScaleFactor);
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
                    if (pm[items]->GetPage() == g_currentPageID)
                    {
                        pm[items]->SetSelected(selectstate);
                        if (g_showcheckboxes == 1) cbpm[items]->SetVisible(selectstate);
                    }
                }
                break;
            }
            case WM_USER + 6:
            {
                CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
                fullscreenpopupbase->SetVisible(true);
                fullscreeninner->SetVisible(true);
                break;
            }
            case WM_USER + 7:
            {
                g_checkifelemexists = false;
                subviewwnd->ShowWindow(SW_HIDE);
                if (g_pendingaction) Sleep(700);
                if (lParam == 1)
                {
                    HideSimpleView(false);
                }
                centered->DestroyAll(true);
                break;
            }
            case WM_USER + 8:
            {
                break;
            }
            case WM_USER + 9:
            {
                yValueEx* yV = (yValueEx*)lParam;
                vector<LVItem*>* l_pm = yV->vpm;
                vector<DDScalableElement*>* l_iconpm = yV->vipm;
                vector<Element*>* l_shadowpm = yV->vispm;
                vector<Element*>* l_shortpm = yV->vspm;
                vector<RichText*>* l_filepm = yV->vfpm;
                vector<DesktopIcon*>* vdi = (vector<DesktopIcon*>*)wParam;
                for (int num = 0; num < yV->num; num++)
                {
                    if (!g_touchmode && (*l_pm)[num])
                    {
                        (*l_pm)[num]->SetWidth(innerSizeX);
                        (*l_pm)[num]->SetHeight(innerSizeY + textm.tmHeight + 23 * g_flScaleFactor);
                        int textlines = 1;
                        if (textm.tmHeight <= 18 * g_flScaleFactor) textlines = 2;
                        (*l_filepm)[num]->SetHeight(textm.tmHeight * textlines + 4 * g_flScaleFactor);
                        if ((*l_filepm)[num]->GetHeight() > (iconPaddingY * 0.575 + 48)) (*l_filepm)[num]->SetHeight(iconPaddingY * 0.575 + 48);
                        (*l_iconpm)[num]->SetWidth(round(g_iconsz * g_flScaleFactor));
                        (*l_iconpm)[num]->SetHeight(round(g_iconsz * g_flScaleFactor));
                        (*l_iconpm)[num]->SetX(iconPaddingX);
                        (*l_iconpm)[num]->SetY(round(iconPaddingY * 0.575));
                    }
                    HBITMAP iconbmp = (*vdi)[num]->icon;
                    CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(iconbmp, 2, 0xffffffff, false, false, false);
                    DeleteObject(iconbmp);
                    if (spvBitmap != nullptr) (*l_iconpm)[num]->SetValue(Element::ContentProp, 1, spvBitmap);
                    HBITMAP iconshadowbmp = (*vdi)[num]->iconshadow;
                    CValuePtr spvBitmapShadow = DirectUI::Value::CreateGraphic(iconshadowbmp, 2, 0xffffffff, false, false, false);
                    DeleteObject(iconshadowbmp);
                    if (spvBitmapShadow != nullptr) (*l_shadowpm)[num]->SetValue(Element::ContentProp, 1, spvBitmapShadow);
                    HBITMAP iconshortcutbmp = (*vdi)[num]->iconshortcut;
                    CValuePtr spvBitmapShortcut = DirectUI::Value::CreateGraphic(iconshortcutbmp, 2, 0xffffffff, false, false, false);
                    DeleteObject(iconshortcutbmp);
                    if (spvBitmapShortcut != nullptr && (*l_pm)[num]->GetShortcutState() == true) (*l_shortpm)[num]->SetValue(Element::ContentProp, 1, spvBitmapShortcut);
                    HBITMAP textbmp = (*vdi)[num]->text;
                    CValuePtr spvBitmapText = DirectUI::Value::CreateGraphic(textbmp, 2, 0xffffffff, false, false, false);
                    DeleteObject(textbmp);
                    if (spvBitmapText != nullptr) (*l_filepm)[num]->SetValue(Element::ContentProp, 1, spvBitmapText);
                    if (g_touchmode)
                    {
                        ((DDScalableElement*)(*l_pm)[num])->SetDDCPIntensity(((*l_pm)[num]->GetHiddenState() == true) ? 192 : 255);
                        ((DDScalableElement*)(*l_pm)[num])->SetAssociatedColor((*vdi)[num]->crDominantTile);
                    }
                }
                CSafeElementPtr<TouchScrollViewer> groupdirlist;
                groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", yV->peOptionalTarget1->GetParent()->GetParent()));
                groupdirlist->SetVisible(true);

                GTRANS_DESC transDesc[3];
                TriggerTranslate(groupdirlist, transDesc, 0, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false);
                TriggerFade(groupdirlist, transDesc, 1, 0.2f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                TriggerClip(groupdirlist, transDesc, 2, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, ((groupdirlist->GetHeight() - 100 * g_flScaleFactor) / groupdirlist->GetHeight()), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, false, false);
                TransitionStoryboardInfo tsbInfo = {};
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, groupdirlist->GetDisplayNode(), &tsbInfo);

                CSafeElementPtr<Element> dirtitle; dirtitle.Assign(regElem<Element*>(L"dirtitle", yV->peOptionalTarget1->GetParent()->GetParent()));
                dirtitle->SetVisible(true);
                TriggerFade(dirtitle, transDesc, 0, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc) - 2, transDesc, dirtitle->GetDisplayNode(), &tsbInfo);

                CSafeElementPtr<Element> emptyview;
                emptyview.Assign(regElem<Element*>(L"emptyview", yV->peOptionalTarget1->GetParent()->GetParent()));
                emptyview->DestroyAll(true);
                emptyview->Destroy(true);
                yV->peOptionalTarget1->DestroyAll(true);
                yV->peOptionalTarget1->Destroy(true);
                for (int i = 0; i < vdi->size(); i++)
                {
                    free((*vdi)[i]);
                }
                free(l_pm), free(l_iconpm), free(l_shadowpm), free(l_shortpm), free(l_filepm);
                delete yV;
                break;
            }
            case WM_USER + 10:
            {
                yValueEx* yV = (yValueEx*)lParam;
                vector<LVItem*>* l_pm = yV->vpm;
                vector<Element*>* l_shadowpm = yV->vispm;
                vector<Element*>* l_shortpm = yV->vspm;
                for (int num = 0; num < yV->num; num++)
                {
                    if ((*l_pm)[num])
                    {
                        if (!g_touchmode && (*l_shadowpm)[num] && (*l_shortpm)[num])
                        {
                            (*l_shadowpm)[num]->SetWidth((g_iconsz + 16) * g_flScaleFactor);
                            (*l_shadowpm)[num]->SetHeight((g_iconsz + 16) * g_flScaleFactor);
                            (*l_shadowpm)[num]->SetX(iconPaddingX - 8 * g_flScaleFactor);
                            (*l_shadowpm)[num]->SetY((iconPaddingY * 0.575) - 6 * g_flScaleFactor);
                            (*l_shortpm)[num]->SetWidth(g_shiconsz * g_flScaleFactor);
                            (*l_shortpm)[num]->SetHeight(g_shiconsz * g_flScaleFactor);
                            (*l_shortpm)[num]->SetX(iconPaddingX);
                            (*l_shortpm)[num]->SetY((iconPaddingY * 0.575) + (g_iconsz - g_shiconsz) * g_flScaleFactor);
                        }
                    }
                }
                break;
            }
            case WM_USER + 11:
            {
                g_pendingaction = true;
                Element* peTemp = reinterpret_cast<Element*>(wParam);
                peTemp->SetEnabled(!peTemp->GetEnabled());
                if (lParam == 1 && ((DDScalableButton*)peTemp)->GetAssociatedFn() != nullptr)
                {
                    ((DDScalableButton*)peTemp)->ExecAssociatedFn(((DDScalableButton*)peTemp)->GetAssociatedFn(), false, true, true);
                    g_pendingaction = false;
                }
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
                    yValue* yV = new yValue{ icon2 };
                    smThumbnailThreadHandle[icon2] = CreateThread(nullptr, 0, CreateIndividualThumbnail, (LPVOID)yV, 0, nullptr);
                    if (smThumbnailThreadHandle[icon2]) CloseHandle(smThumbnailThreadHandle[icon2]);
                }
                smThumbnailThreadHandle.clear();
                break;
            }
            case WM_USER + 15:
            {
                if (!g_editmode || lParam == 0) ShowSimpleView(lParam, wParam);
                else HideSimpleView(true);
                break;
            }
            case WM_USER + 16:
            {
                ThumbnailIcon* ti = (ThumbnailIcon*)wParam;
                HBITMAP thumbIcon = ti->icon;
                CValuePtr spvThumbIcon = DirectUI::Value::CreateGraphic(thumbIcon, 2, 0xffffffff, false, false, false);
                Element* GroupedIcon{};
                parser->CreateElement(L"GroupedIcon", nullptr, nullptr, nullptr, (Element**)&GroupedIcon);
                iconpm[lParam]->Add((Element**)&GroupedIcon, 1);
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
                    internalselectedLVItems[0]->SetDragState(true);
                    dragpreview->SetVisible(!g_lockiconpos);
                }
                if (ppt.x < 16 * g_flScaleFactor) dragToPrev++;
                else dragToPrev = 0;
                if (ppt.x > dimensions.right - 16 * g_flScaleFactor) dragToNext++;
                else dragToNext = 0;
                if (dragToPrev > 40 && g_currentPageID > 1)
                {
                    g_currentPageID--;
                    short animSrc = (localeType == 1) ? 1 : -1;
                    animSrc *= dimensions.right;
                    for (int i = 0; i < internalselectedLVItems.size(); i++)
                    {
                        internalselectedLVItems[i]->SetPage(g_currentPageID);
                        if (g_currentPageID == internalselectedLVItems[i]->GetMemPage())
                            internalselectedLVItems[i]->SetX(internalselectedLVItems[i]->GetMemXPos());
                        else
                            internalselectedLVItems[i]->SetX(internalselectedLVItems[i]->GetMemXPos() - animSrc);
                    }
                    TriggerPageTransition(-1, dimensions);
                    nextpageMain->SetVisible(true);
                    if (g_currentPageID == 1) prevpageMain->SetVisible(false);
                    dragToPrev = 0;
                }
                if (dragToNext > 40 && g_currentPageID < g_maxPageID)
                {
                    g_currentPageID++;
                    short animSrc = (localeType == 1) ? -1 : 1;
                    animSrc *= dimensions.right;
                    for (int i = 0; i < internalselectedLVItems.size(); i++)
                    {
                        internalselectedLVItems[i]->SetPage(g_currentPageID);
                        if (g_currentPageID == internalselectedLVItems[i]->GetMemPage())
                            internalselectedLVItems[i]->SetX(internalselectedLVItems[i]->GetMemXPos());
                        else
                            internalselectedLVItems[i]->SetX(internalselectedLVItems[i]->GetMemXPos() - animSrc);
                    }
                    TriggerPageTransition(1, dimensions);
                    prevpageMain->SetVisible(true);
                    if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
                    dragToNext = 0;
                }
                if (localeType == 1) ppt.x = dimensions.right - ppt.x;
                dragpreview->SetX(ppt.x - origX);
                dragpreview->SetY(ppt.y - origY);
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
                            POINT ppt;
                            CValuePtr v;
                            GetCursorPos(&ppt);
                            ScreenToClient(wnd->GetHWND(), &ppt);
                            int outerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 44) * g_flScaleFactor;
                            int outerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 22) * g_flScaleFactor;
                            int desktoppadding = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH : DESKPADDING_NORMAL);
                            int desktoppadding_x = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH_X : DESKPADDING_NORMAL_X);
                            int desktoppadding_y = g_flScaleFactor * (g_touchmode ? DESKPADDING_TOUCH_Y : DESKPADDING_NORMAL_Y);
                            if (g_touchmode)
                            {
                                outerSizeX = g_touchSizeX + desktoppadding;
                                outerSizeY = g_touchSizeY + desktoppadding;
                            }
                            int largestXPos = (dimensions.right - (2 * desktoppadding_x) + desktoppadding) / outerSizeX;
                            int largestYPos = (dimensions.bottom - (2 * desktoppadding_y) + desktoppadding) / outerSizeY;
                            if (largestXPos == 0) largestXPos = 1;
                            if (largestYPos == 0) largestYPos = 1;
                            if (g_touchmode)
                            {
                                desktoppadding_x = (dimensions.right - largestXPos * outerSizeX + desktoppadding) / 2;
                                desktoppadding_y = (dimensions.bottom - largestYPos * outerSizeY + desktoppadding) / 2;
                            }
                            int xRender = (localeType == 1) ? ppt.x + origX - desktoppadding_x - internalselectedLVItems[0]->GetWidth() : ppt.x - origX - desktoppadding_x;
                            int paddingmitigation = (localeType == 1) ? desktoppadding : 0;
                            int destX = desktoppadding_x + round(xRender / static_cast<float>(outerSizeX)) * outerSizeX;
                            int destY = desktoppadding_y + round((ppt.y - origY - desktoppadding_y) / static_cast<float>(outerSizeY)) * outerSizeY;
                            if (localeType == 1)
                            {
                                destX = dimensions.right - destX - internalselectedLVItems[0]->GetWidth();
                            }
                            const int mainElementX = internalselectedLVItems[0]->GetMemXPos();
                            const int mainElementY = internalselectedLVItems[0]->GetMemYPos();
                            int itemstodrag = internalselectedLVItems.size();
                            if (itemstodrag == 0) itemstodrag = 1;
                            for (int items = 0; items < itemstodrag; items++)
                            {
                                internalselectedLVItems[items]->SetDragState(true);
                            }
                            for (int items = 0; items < itemstodrag; items++)
                            {
                                int finaldestX = destX - mainElementX + internalselectedLVItems[items]->GetMemXPos();
                                int finaldestY = destY - mainElementY + internalselectedLVItems[items]->GetMemYPos();
                                if (localeType == 1)
                                {
                                    if (finaldestX < desktoppadding_x) finaldestX = dimensions.right - round((dimensions.right - outerSizeX) / static_cast<float>(outerSizeX)) * outerSizeX - desktoppadding_x + desktoppadding;
                                    if (finaldestX > dimensions.right - internalselectedLVItems[items]->GetWidth() + desktoppadding_x) finaldestX = dimensions.right - internalselectedLVItems[items]->GetWidth() - desktoppadding_x;
                                }
                                else
                                {
                                    if (finaldestX < desktoppadding_x) finaldestX = desktoppadding_x;
                                    if (finaldestX > dimensions.right - internalselectedLVItems[items]->GetWidth() - desktoppadding_x) finaldestX = round((dimensions.right - internalselectedLVItems[items]->GetWidth()) / static_cast<float>(outerSizeX)) * outerSizeX - outerSizeX + desktoppadding_x;
                                }
                                if (finaldestY < desktoppadding_y) finaldestY = desktoppadding_y;
                                if (finaldestY > dimensions.bottom - internalselectedLVItems[items]->GetHeight() - desktoppadding_y - desktoppadding) finaldestY = round((dimensions.bottom - internalselectedLVItems[items]->GetHeight() - 2 * desktoppadding_y) / static_cast<float>(outerSizeY)) * outerSizeY + desktoppadding_y;
                                int saveddestX = (localeType == 1) ? dimensions.right - finaldestX - internalselectedLVItems[items]->GetWidth() - desktoppadding_x : finaldestX - desktoppadding_x;
                                for (int items2 = 0; items2 < pm.size(); items2++)
                                {
                                    if (pm[items2]->GetMemXPos() == finaldestX && pm[items2]->GetMemYPos() == finaldestY && pm[items2]->GetPage() == internalselectedLVItems[items]->GetPage() && pm[items2]->GetDragState() == false) break;
                                    if (items2 == pm.size() - 1)
                                    {
                                        internalselectedLVItems[items]->SetMemXPos(finaldestX);
                                        internalselectedLVItems[items]->SetMemYPos(finaldestY);
                                        internalselectedLVItems[items]->SetInternalXPos(saveddestX / outerSizeX);
                                        internalselectedLVItems[items]->SetInternalYPos((finaldestY - desktoppadding_y) / outerSizeY);
                                        internalselectedLVItems[items]->SetMemPage(internalselectedLVItems[items]->GetPage());
                                        DelayedElementActions* dea = new DelayedElementActions{ 400, internalselectedLVItems[items], static_cast<float>(finaldestX), static_cast<float>(finaldestY) };
                                        DWORD DelayedSetPos;
                                        HANDLE hDelayedSetPos = CreateThread(nullptr, 0, SetLVIPos, dea, NULL, nullptr);
                                        if (hDelayedSetPos) CloseHandle(hDelayedSetPos);
                                        GTRANS_DESC transDesc[1];
                                        TransitionStoryboardInfo tsbInfo = {};
                                        TriggerTranslate(internalselectedLVItems[items], transDesc, 0, 0.0f, 0.4f, 0.75f, 0.45f, 0.0f, 1.0f, internalselectedLVItems[items]->GetX(), internalselectedLVItems[items]->GetY(), finaldestX, finaldestY, false, false);
                                        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, internalselectedLVItems[items]->GetDisplayNode(), &tsbInfo);
                                        DUI_SetGadgetZOrder(internalselectedLVItems[items], -1);
                                    }
                                }
                            }
                            for (LVItem* lvi : internalselectedLVItems)
                            {
                                lvi->SetDragState(false);
                            }
                            internalselectedLVItems.clear();
                        }
                        dragpreview->SetVisible(false);
                        break;
                    }
                    case 1:
                    {
                        if (wParam != NULL)
                        {
                            LVItem* item = (*(vector<LVItem*>*)wParam)[0];
                            item->SetDragState(false);
                        }
                        break;
                    }
                    case 2:
                    {
                        vector<LVItem*> internalselectedLVItems = (*(vector<LVItem*>*)wParam);
                        LVItem* item = internalselectedLVItems[0];
                        item->SetDragState(false);
                        internalselectedLVItems.clear();
                        dragpreview->SetVisible(false);
                        MessageBeep(MB_OK);
                        DDNotificationBanner* ddnb{};
                        DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_INFO, L"DDNB", LoadStrFromRes(4044).c_str(), LoadStrFromRes(4045).c_str(), 5, false);
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
                CSafeElementPtr<Element> shortcutElem;
                shortcutElem.Assign(regElem<Element*>(L"shortcutElem", outerElem));
                CSafeElementPtr<Element> iconElemShadow;
                iconElemShadow.Assign(regElem<Element*>(L"iconElemShadow", outerElem));
                CSafeElementPtr<RichText> textElem;
                textElem.Assign(regElem<RichText*>(L"textElem", outerElem));
                CSafeElementPtr<RichText> textElemShadow;
                textElemShadow.Assign(regElem<RichText*>(L"textElemShadow", outerElem));
                CSafeElementPtr<Button> checkboxElem;
                checkboxElem.Assign(regElem<Button*>(L"checkboxElem", outerElem));

                int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
                int isThumbnailHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"IconsOnly");
                wstring foundfilename = (wstring)L"\"" + fi->filepath + (wstring)L"\\" + fi->filename + (wstring)L"\"";
                DWORD attr = GetFileAttributesW(RemoveQuotes(foundfilename).c_str());
                wstring foundsimplefilename = hideExt((wstring)fi->filename, isFileExtHidden, (attr & 16), outerElem);
                if (attr & 16)
                {
                    outerElem->SetDirState(true);
                    unsigned short itemsInside{};
                    EnumerateFolder((LPWSTR)RemoveQuotes(foundfilename).c_str(), nullptr, true, &itemsInside);
                    if (itemsInside <= 192) outerElem->SetGroupedDirState(true);
                }
                else outerElem->SetDirState(false);
                if (attr & 2) outerElem->SetHiddenState(true);
                else outerElem->SetHiddenState(false);
                if (isThumbnailHidden == 0)
                {
                    bool image;
                    isImage(foundfilename, true, imageExts[0], &image);
                    outerElem->SetColorLock(image);
                }
                outerElem->SetSimpleFilename(foundsimplefilename);
                outerElem->SetFilename(foundfilename);
                outerElem->SetAccDesc(GetExplorerTooltipText(foundfilename).c_str());
                if (outerElem->GetHiddenState() == true)
                {
                    iconElem->SetAlpha(128);
                    iconElemShadow->SetAlpha(0);
                    textElem->SetAlpha(g_touchmode ? 128 : 192);
                    if (!g_touchmode) textElemShadow->SetAlpha(128);
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

                pm.push_back(outerElem);
                iconpm.push_back(iconElem);
                shortpm.push_back(shortcutElem);
                shadowpm.push_back(iconElemShadow);
                filepm.push_back(textElem);
                fileshadowpm.push_back(textElemShadow);
                cbpm.push_back(checkboxElem);

                int currentID = pm.size() - 1;
                IconThumbHelper(currentID);
                yValue* yV2 = new yValue{ currentID };
                HANDLE smThumbnailThreadHandle = CreateThread(nullptr, 0, CreateIndividualThumbnail, (LPVOID)yV2, 0, nullptr);
                if (smThumbnailThreadHandle) CloseHandle(smThumbnailThreadHandle);;
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
                pm[currentID]->SetRefreshState(true);
                delete fi;
                break;
            }
            case WM_USER + 21:
            {
                LVItem* toRemove = (LVItem*)wParam;
                pm.erase(pm.begin() + lParam);
                iconpm.erase(iconpm.begin() + lParam);
                shadowpm.erase(shadowpm.begin() + lParam);
                shortpm.erase(shortpm.begin() + lParam);
                filepm.erase(filepm.begin() + lParam);
                fileshadowpm.erase(fileshadowpm.begin() + lParam);
                cbpm.erase(cbpm.begin() + lParam);
                toRemove->Destroy(true);
                toRemove = nullptr;
                break;
            }
            //case WM_USER + 22: {
            //    int isFileExtHidden = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt");
            //    LVItem* toRename = (LVItem*)wParam;
            //    FileInfo* fi = (FileInfo*)lParam;
            //    wstring foundfilename = (wstring)L"\"" + fi->filepath + (wstring)L"\\" + fi->filename + (wstring)L"\"";
            //    toRename->SetFilename(foundfilename);
            //    DWORD attr = GetFileAttributesW(RemoveQuotes(foundfilename).c_str());
            //    toRename->SetSimpleFilename(hideExt(fi->filename, isFileExtHidden, (attr & 16), toRename));
            //    SelectItemListener(toRename, Element::SelectedProp(), 69, NULL, NULL);
            //    break;
            //}
            case WM_USER + 23:
            {
                break;
            }
            case WM_USER + 24:
            {
                ShowDirAsGroupDesktop(pm[lParam]);
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
                if (pm[lParam] && (!g_treatdirasgroup || pm[lParam]->GetGroupSize() == LVIGS_NORMAL))
                {
                    CSafeElementPtr<Element> innerElem;
                    innerElem.Assign(regElem<Element*>(L"innerElem", pm[lParam]));
                    CSafeElementPtr<Element> g_innerElem;
                    g_innerElem.Assign(regElem<Element*>(L"innerElem", g_outerElem));
                    CSafeElementPtr<Element> checkboxElem;
                    checkboxElem.Assign(regElem<Element*>(L"checkboxElem", g_outerElem));
                    innerElem->SetLayoutPos(g_innerElem->GetLayoutPos()), cbpm[lParam]->SetLayoutPos(checkboxElem->GetLayoutPos());
                    if (g_touchmode)
                    {
                        // had to hardcode it as GetPadding is VERY unreliable on high dpi
                        int space = 6 * g_flScaleFactor;
                        CSafeElementPtr<Element> containerElem;
                        containerElem.Assign(regElem<Element*>(L"containerElem", pm[lParam]));
                        containerElem->SetPadding(space, space, space, space);
                    }
                    SelectItemListener(pm[lParam], Element::SelectedProp(), 69, nullptr, nullptr);
                }
                break;
            }
        }
        return CallWindowProc(WndProc, hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK TopLevelWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_DPICHANGED:
            {
                g_lastDpiChangeTick = GetTickCount64();
                g_delayGroupsForDpi = true;
                UpdateScale();
                SetTimer(wnd->GetHWND(), 2, 1000, nullptr);
                break;
            }
            case WM_DISPLAYCHANGE:
            {
                AdjustWindowSizes(true);
                g_lastWidth = 0, g_lastHeight = 0;
                g_lastDpiChangeTick = GetTickCount64();
                SetTimer(wnd->GetHWND(), 5, 150, nullptr);
                break;
            }
            case WM_CLOSE:
            {
                return 0L;
                break;
            }
        }
        return CallWindowProc(WndProcSubview, hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK MsgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_USER + 1:
            {
                if (wParam < 4096) break;
                RedrawImageCore((DDScalableElement*)wParam);
                break;
            }
            case WM_USER + 2:
            {
                if (wParam < 4096) break;
                RedrawFontCore((DDScalableElement*)wParam);
                break;
            }
            case WM_USER + 3:
            {
                DDCheckBoxGlyph* peGlyph;
                DDScalableElement* peText;
                DDCheckBoxGlyph::Create((Element*)wParam, nullptr, (Element**)&peGlyph);
                ((Element*)wParam)->Add((Element**)&peGlyph, 1);
                peGlyph->SetCheckedState(((DDCheckBox*)wParam)->GetCheckedState());
                peGlyph->SetID(L"DDCB_Glyph");
                DDScalableElement::Create((Element*)wParam, nullptr, (Element**)&peText);
                ((Element*)wParam)->Add((Element**)&peText, 1);
                CValuePtr v;
                peText->SetContentString(((Element*)wParam)->GetContentString(&v));
                peText->SetID(L"DDCB_Text");
                ((Element*)wParam)->SetContentString(L"");
                break;
            }
            case WM_USER + 4:
            {
                DDColorPickerButton* peTemp;
                int scaleInterval = GetCurrentScaleInterval();
                int scaleIntervalImage = ((DDColorPicker*)wParam)->GetScaledImageIntervals();
                if (scaleInterval > scaleIntervalImage - 1) scaleInterval = scaleIntervalImage - 1;
                int imageID = ((DDColorPicker*)wParam)->GetFirstScaledImage() + scaleInterval;
                HBITMAP newImage = (HBITMAP)LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
                if (newImage == nullptr)
                {
                    LoadPNGAsBitmap(newImage, imageID);
                    IterateBitmap(newImage, UndoPremultiplication, 1, 0, 1, NULL);
                }
                COLORREF* pImmersiveColor = ((DDColorPicker*)wParam)->GetThemeAwareness() ? g_theme ? &ImmersiveColorL : &ImmersiveColorD : &ImmersiveColor;
                COLORREF colorPickerPalette[8] =
                {
                    ((DDColorPicker*)wParam)->GetDefaultColor(),
                    *pImmersiveColor,
                    ((DDColorPicker*)wParam)->GetThemeAwareness() ? g_theme ? RGB(76, 194, 255) : RGB(0, 103, 192) : RGB(0, 120, 215),
                    ((DDColorPicker*)wParam)->GetThemeAwareness() ? g_theme ? RGB(216, 141, 225) : RGB(158, 58, 176) : RGB(177, 70, 194),
                    ((DDColorPicker*)wParam)->GetThemeAwareness() ? g_theme ? RGB(244, 103, 98) : RGB(210, 14, 30) : RGB(232, 17, 35),
                    ((DDColorPicker*)wParam)->GetThemeAwareness() ? g_theme ? RGB(251, 154, 68) : RGB(224, 83, 7) : RGB(247, 99, 12),
                    ((DDColorPicker*)wParam)->GetThemeAwareness() ? g_theme ? RGB(255, 213, 42) : RGB(225, 157, 0) : RGB(255, 185, 0),
                    ((DDColorPicker*)wParam)->GetThemeAwareness() ? g_theme ? RGB(38, 255, 142) : RGB(0, 178, 90) : RGB(0, 204, 106)
                };
                BITMAP bm{};
                GetObject(newImage, sizeof(BITMAP), &bm);
                int btnWidth = bm.bmWidth / 8;
                int btnHeight = bm.bmHeight;
                int btnX = ((Element*)wParam)->GetWidth() / 8;
                int btnY = (((Element*)wParam)->GetHeight() - bm.bmHeight) / 2;

                HDC hdc = GetDC(nullptr);
                HDC hdcSrc = CreateCompatibleDC(hdc);
                HDC hdcDst = CreateCompatibleDC(hdc);
                SelectObject(hdcSrc, newImage);
                for (int i = 0; i < 8; i++)
                {
                    int xPos = (localeType == 1) ? ((Element*)wParam)->GetWidth() - i * btnX - btnWidth : i * btnX;
                    HBITMAP hbmPickerBtn = CreateCompatibleBitmap(hdc, btnWidth, btnHeight);
                    SelectObject(hdcDst, hbmPickerBtn);
                    BitBlt(hdcDst, 0, 0, btnWidth, btnHeight, hdcSrc, i * btnWidth, 0, SRCCOPY);
                    DDColorPickerButton::Create((Element*)wParam, nullptr, (Element**)&peTemp);
                    ((Element*)wParam)->Add((Element**)&peTemp, 1);
                    peTemp->SetLayoutPos(-2);
                    peTemp->SetX(xPos);
                    peTemp->SetY(btnY);
                    peTemp->SetWidth(bm.bmWidth / 8);
                    peTemp->SetHeight(btnHeight);
                    CValuePtr spvPickerBtn = Value::CreateGraphic(hbmPickerBtn, 2, 0xffffffff, true, false, false);
                    if (spvPickerBtn) peTemp->SetValue(Element::ContentProp, 1, spvPickerBtn);
                    peTemp->SetAssociatedColor(colorPickerPalette[i]);
                    peTemp->SetOrder(i);
                    peTemp->SetTargetElements(((DDColorPicker*)wParam)->GetTargetElements());
                    peTemp->SetTargetButtons(((DDColorPicker*)wParam)->GetTargetButtons());
                    DeleteObject(hbmPickerBtn);
                    peTemp->SetPropChangeListener(assignExtendedFn(peTemp, ShowHoverCircle, true));
                }
                if (newImage) DeleteObject(newImage);
                DeleteDC(hdcSrc);
                DeleteDC(hdcDst);
                ReleaseDC(nullptr, hdc);

                RegKeyValue rkv = ((DDColorPicker*)wParam)->GetRegKeyValue();
                int order = (rkv._hKeyName) ? GetRegistryValues(rkv._hKeyName, rkv._path, rkv._valueToFind) * btnX : rkv._dwValue * btnX;
                int checkedBtnX = (localeType == 1) ? ((Element*)wParam)->GetWidth() - order - btnWidth : order;
                DDScalableElement* peCircle;
                DDScalableElement::Create((Element*)wParam, nullptr, (Element**)&peCircle);
                ((Element*)wParam)->Add((Element**)&peCircle, 1);
                peCircle->SetLayoutPos(-2);
                peCircle->SetX(-9999);
                peCircle->SetWidth(bm.bmWidth / 8);
                peCircle->SetHeight(btnHeight);
                peCircle->SetID(L"DDColorPicker_HoverCircle");
                DDScalableElement::Create((Element*)wParam, nullptr, (Element**)&peCircle);
                ((Element*)wParam)->Add((Element**)&peCircle, 1);
                peCircle->SetLayoutPos(-2);
                peCircle->SetX(checkedBtnX);
                peCircle->SetWidth(bm.bmWidth / 8);
                peCircle->SetHeight(btnHeight);
                peCircle->SetID(L"DDColorPicker_CheckedCircle");
                break;
            }
            case WM_USER + 5:
            {
                DelayedElementActions* dea = (DelayedElementActions*)wParam;
                switch (lParam)
                {
                case 1:
                    dea->pe->SetVisible(!dea->pe->GetVisible());
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
                }
                delete dea;
                break;
            }
        }
        return CallWindowProc(WndProcMessagesOnly, hWnd, uMsg, wParam, lParam);
    }

    DWORD WINAPI fastin(LPVOID lpParam)
    {
        yValue* yV = (yValue*)lpParam;
        if (pm[yV->num]->GetRefreshState())
        {
            int lines_basedOnEllipsis{};
            DWORD alignment{};
            RECT g_touchmoderect{};
            CalcDesktopIconInfo(yV, &lines_basedOnEllipsis, &alignment, false, &pm, &filepm);
            HBITMAP capturedBitmap{};
            CreateTextBitmap(capturedBitmap, pm[yV->num]->GetSimpleFilename().c_str(), yV->fl1 - 4 * g_flScaleFactor, lines_basedOnEllipsis, alignment, g_touchmode);
            DesktopIcon di;
            ApplyIcons(pm, iconpm, &di, false, yV->num, 1);
            if (g_touchmode)
            {
                di.crDominantTile = GetDominantColorFromIcon(di.icon, g_iconsz, 48);
                if (g_treatdirasgroup && pm[yV->num]->GetGroupedDirState() == true)
                {
                    COLORREF crDefault = g_theme ? RGB(208, 208, 208) : RGB(48, 48, 48);
                    di.crDominantTile = iconpm[yV->num]->GetAssociatedColor() == -1 ? crDefault : iconpm[yV->num]->GetAssociatedColor();
                }
                rgb_t saturatedColor = { GetRValue(di.crDominantTile), GetGValue(di.crDominantTile), GetBValue(di.crDominantTile) };
                hsl_t saturatedColor2 = rgb2hsl(saturatedColor);
                saturatedColor2.l /= 4;
                saturatedColor2.s *= 4;
                saturatedColor = hsl2rgb(saturatedColor2);
                IterateBitmap(di.iconshadow, StandardBitmapPixelHandler, 3, 0, 1, RGB(saturatedColor.r, saturatedColor.g, saturatedColor.b));
                if (GetRValue(di.crDominantTile) * 0.299 + GetGValue(di.crDominantTile) * 0.587 + GetBValue(di.crDominantTile) * 0.114 > 156)
                {
                    IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
                    IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 1, NULL);
                }
                else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
            }
            else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
            if (capturedBitmap != nullptr) di.text = capturedBitmap;
            if (!g_touchmode)
            {
                HBITMAP shadowBitmap{};
                AddPaddingToBitmap(capturedBitmap, shadowBitmap, 2 * g_flScaleFactor, 2 * g_flScaleFactor, 2 * g_flScaleFactor, 2 * g_flScaleFactor);
                IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * g_flScaleFactor), 2, NULL);
                if (shadowBitmap != nullptr) di.textshadow = shadowBitmap;
            }
            Sleep(100);
            SendMessageW(wnd->GetHWND(), WM_USER + 4, NULL, yV->num);
            SendMessageW(wnd->GetHWND(), WM_USER + 3, (WPARAM)&di, yV->num);
            Sleep(250);
            SendMessageW(wnd->GetHWND(), WM_USER + 26, NULL, yV->num);
        }
        else if (pm[yV->num]->GetMoving()) SendMessageW(wnd->GetHWND(), WM_USER + 1, NULL, yV->num);
        return 0;
    }

    DWORD WINAPI subfastin(LPVOID lpParam)
    {
        InitThread(TSM_DESKTOP_DYNAMIC);
        Sleep(25);
        yValueEx* yV = (yValueEx*)lpParam;
        vector<LVItem*>* l_pm = yV->vpm;
        vector<DesktopIcon*> vdi;
        for (int num = 0; num < yV->num; num++)
        {
            int lines_basedOnEllipsis{};
            DWORD alignment{};
            RECT g_touchmoderect{};
            int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
            int textlines = 1;
            if (textm.tmHeight <= 18 * g_flScaleFactor) textlines = 2;
            yValue* yV2 = new yValue{ num, yV->fl1, yV->fl2 };
            if (g_touchmode) CalcDesktopIconInfo(yV2, &lines_basedOnEllipsis, &alignment, true, yV->vpm, yV->vfpm);
            HBITMAP capturedBitmap{};
            if (g_touchmode) CreateTextBitmap(capturedBitmap, (*l_pm)[num]->GetSimpleFilename().c_str(), yV2->fl1 - 4 * g_flScaleFactor, lines_basedOnEllipsis, alignment, g_touchmode);
            else CreateTextBitmap(capturedBitmap, (*l_pm)[num]->GetSimpleFilename().c_str(), innerSizeX, textm.tmHeight * textlines, DT_CENTER | DT_END_ELLIPSIS, g_touchmode);
            delete yV2;
            DesktopIcon* di = new DesktopIcon;
            ApplyIcons(*l_pm, *(yV->vipm), di, false, num, 1);
            if (((LVItem*)yV->peOptionalTarget2)->GetMemorySelected() == false)
            {
                for (int num2 = 0; num2 < num; num2++)
                {
                    DeleteObject(vdi[num2]->icon);
                    DeleteObject(vdi[num2]->iconshadow);
                    DeleteObject(vdi[num2]->iconshortcut);
                    DeleteObject(vdi[num2]->text);
                    DeleteObject(vdi[num2]->textshadow);
                    free(vdi[num2]);
                    UnInitThread();
                }
                vdi.clear();
                delete yV;
                return 0;
            }
            if (g_touchmode)
            {
                di->crDominantTile = GetDominantColorFromIcon(di->icon, g_iconsz, 48);
                rgb_t saturatedColor = { GetRValue(di->crDominantTile), GetGValue(di->crDominantTile), GetBValue(di->crDominantTile) };
                hsl_t saturatedColor2 = rgb2hsl(saturatedColor);
                saturatedColor2.l /= 4;
                saturatedColor2.s *= 4;
                saturatedColor = hsl2rgb(saturatedColor2);
                IterateBitmap(di->iconshadow, StandardBitmapPixelHandler, 3, 0, 1, RGB(saturatedColor.r, saturatedColor.g, saturatedColor.b));
                if (GetRValue(di->crDominantTile) * 0.299 + GetGValue(di->crDominantTile) * 0.587 + GetBValue(di->crDominantTile) * 0.114 > 156)
                {
                    IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
                    IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 1, NULL);
                }
                else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
            }
            else if (g_theme && !g_touchmode)
            {
                IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1, NULL);
                IterateBitmap(capturedBitmap, SimpleBitmapPixelHandler, 1, 0, 0.9, NULL);
            }
            else IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
            if (capturedBitmap != nullptr) di->text = capturedBitmap;
            vdi.push_back(di);
        }
        SendMessageW(wnd->GetHWND(), WM_USER + 2, NULL, (LPARAM)yV);
        SendMessageW(wnd->GetHWND(), WM_USER + 10, NULL, (LPARAM)yV);
        SendMessageW(wnd->GetHWND(), WM_USER + 9, (WPARAM)&vdi, (LPARAM)yV);
        UnInitThread();
        return 0;
    }

    DWORD WINAPI InitDesktopGroup(LPVOID lpParam)
    {
        int itemID = *((int*)lpParam);
        if (pm[itemID]->GetGroupSize() == LVIGS_NORMAL) return 1;
        if (g_touchmode) Sleep(500);
        if (g_delayGroupsForDpi) Sleep(2250);
        Sleep(250);
        g_delayGroupsForDpi = false;
        PostMessageW(wnd->GetHWND(), WM_USER + 24, NULL, itemID);
        return 0;
    }

    DWORD WINAPI animate6(LPVOID lpParam)
    {
        Sleep(175);
        AnimateWindow(subviewwnd->GetHWND(), 120, AW_BLEND | AW_HIDE);
        BlurBackground(subviewwnd->GetHWND(), false, true, fullscreenpopupbg);
        SendMessageW(wnd->GetHWND(), WM_USER + 7, NULL, NULL);
        return 0;
    }

    DWORD WINAPI AnimateWindowWrapper2(LPVOID lpParam)
    {
        AnimateWindow(subviewwnd->GetHWND(), 180, AW_BLEND);
        return 0;
    }

    void fullscreenAnimation(int width, int height, float animstartscale, float desktopanimstartscale)
    {
        CValuePtr v;
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        RECT padding = *(popupcontainer->GetPadding(&v));
        int maxwidth = dimensions.right - dimensions.left - padding.left - padding.right;
        int maxheight = dimensions.bottom - dimensions.top - padding.top - padding.bottom;
        if (width > maxwidth) width = maxwidth;
        if (height > maxheight) height = maxheight;
        parserSubview->CreateElement(L"fullscreeninner", nullptr, nullptr, nullptr, (Element**)&fullscreeninner);
        centered->Add((Element**)&fullscreeninner, 1);
        SetPopupSize(centered, width, height);
        SetPopupSize(fullscreeninner, width, height);
        centered->SetBackgroundColor(0);
        fullscreenpopupbase->SetVisible(true);
        fullscreeninner->SetVisible(true);
        GTRANS_DESC transDesc[2];
        TriggerFade(fullscreeninner, transDesc, 0, 0.2f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
        TriggerScaleIn(fullscreeninner, transDesc, 1, 0.2f, 0.45f, 0.0f, 0.0f, 0.0f, 1.0f, animstartscale, animstartscale, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
        TransitionStoryboardInfo tsbInfo = {};
        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, fullscreeninner->GetDisplayNode(), &tsbInfo);
        GTRANS_DESC transDesc2[1];
        TriggerScaleOut(UIContainer, transDesc2, 0, 0.0f, 0.67f, 0.1f, 0.9f, 0.2f, 1.0f, desktopanimstartscale, desktopanimstartscale, 0.5f, 0.5f, false, false);
        TransitionStoryboardInfo tsbInfo2 = {};
        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, UIContainer->GetDisplayNode(), &tsbInfo2);
        if (!g_editmode) BlurBackground(subviewwnd->GetHWND(), true, true, fullscreenpopupbg);
        HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateWindowWrapper2, nullptr, NULL, nullptr);
        if (AnimHandle) CloseHandle(AnimHandle);
        g_issubviewopen = true;
    }

    void fullscreenAnimation2()
    {
        DWORD animThread;
        HANDLE animThreadHandle = CreateThread(nullptr, 0, animate6, nullptr, 0, &animThread);
        if (animThreadHandle) CloseHandle(animThreadHandle);
    }

    void ShowPopupCore()
    {
        fullscreenAnimation(800 * g_flScaleFactor, 480 * g_flScaleFactor, 0.8, 0.88);
        HANDLE AnimHandle = CreateThread(nullptr, 0, AnimateWindowWrapper2, nullptr, NULL, nullptr);
        if (AnimHandle) CloseHandle(AnimHandle);
    }

    void HidePopupCore(bool WinDInvoked)
    {
        if (!WinDInvoked) SendMessageW(g_hWndTaskbar, WM_COMMAND, 416, 0);
        if (g_tempElem2) ((LVItem*)g_tempElem2)->SetMemorySelected(false);
        if (g_issubviewopen)
        {
            CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
            GTRANS_DESC transDesc[2];
            TriggerScaleOut(fullscreeninner, transDesc, 0, 0.0f, 0.175f, 0.0f, 0.0f, 0.0f, 1.0f, 0.95f, 0.95f, 0.5f, 0.5f, false, false);
            TriggerFade(fullscreeninner, transDesc, 1, 0.025f, 0.175f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, fullscreeninner->GetDisplayNode(), &tsbInfo);
            GTRANS_DESC transDesc2[1];
            TriggerScaleOut(UIContainer, transDesc2, 0, 0.175f, 0.675f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
            TransitionStoryboardInfo tsbInfo2 = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, UIContainer->GetDisplayNode(), &tsbInfo2);
        }
        if (g_issettingsopen && g_atleastonesetting)
        {
            DDNotificationBanner* ddnb{};
            DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_SUCCESS, L"DDNB", LoadStrFromRes(4042).c_str(), nullptr, 3, false);
        }
        DUI_SetGadgetZOrder(UIContainer, -1);
        g_issubviewopen = false;
        g_issettingsopen = false;
        g_atleastonesetting = false;
        SetWindowPos(subviewwnd->GetHWND(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        fullscreenAnimation2();
    }

    void CloseCustomizePage(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<Element> groupdirectory;
            groupdirectory.Assign(regElem<Element*>(L"groupdirectory", elem->GetParent()->GetParent()->GetParent()->GetParent()->GetParent()));
            CSafeElementPtr<Element> customizegroup;
            customizegroup.Assign(regElem<Element*>(L"customizegroup", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirname;
            dirname.Assign(regElem<DDScalableRichText*>(L"dirname", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirdetails;
            dirdetails.Assign(regElem<DDScalableRichText*>(L"dirdetails", groupdirectory));
            CSafeElementPtr<Element> tasks;
            tasks.Assign(regElem<Element*>(L"tasks", groupdirectory));
            CSafeElementPtr<DDScalableButton> More;
            More.Assign(regElem<DDScalableButton*>(L"More", groupdirectory));
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
            if (emptygraphic)
            {
                CSafeElementPtr<DDScalableElement> iconElement;
                iconElement.Assign(regElem<DDScalableElement*>(L"iconElem", ((DDLVActionButton*)elem)->GetAssociatedItem()));
                COLORREF colorPickerPalette[6] =
                {
                    RGB(0, 145, 248), RGB(189, 91, 203), RGB(239, 39, 51), RGB(248, 116, 29), RGB(255, 195, 0), RGB(0, 204, 106)
                };
                if (iconElement->GetGroupColor() >= 2)
                    emptygraphic->SetAssociatedColor(colorPickerPalette[iconElement->GetGroupColor() - 2]);
                else
                    emptygraphic->SetAssociatedColor(-1);
            }
            if (emptyview) emptyview->SetVisible(true);
            Group_BackContainer->SetLayoutPos(-3);
            Group_Back->SetVisible(false);
            dirname->SetContentString(((DDLVActionButton*)elem)->GetAssociatedItem()->GetSimpleFilename().c_str());
            dirdetails->SetLayoutPos(3);
            tasks->SetVisible(true);
            More->SetVisible(true);
            groupdirlist->SetLayoutPos(4);
            groupdirlist->SetKeyFocus();

            GTRANS_DESC transDesc[3];
            TriggerTranslate(groupdirlist, transDesc, 0, 0.0f, 0.5f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false);
            TriggerFade(groupdirlist, transDesc, 1, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
            TriggerClip(groupdirlist, transDesc, 2, 0.0f, 0.5f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, ((groupdirlist->GetHeight() - 100 * g_flScaleFactor) / groupdirlist->GetHeight()), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, groupdirlist->GetDisplayNode(), &tsbInfo);

            if (customizegroup)
            {
                customizegroup->SetLayoutPos(-3);
                customizegroup->DestroyAll(true);
                customizegroup->Destroy(true);
            }
        }
    }

    void OpenCustomizePage(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            Element* customizegroup;
            CSafeElementPtr<Element> groupdirectory;
            groupdirectory.Assign(regElem<Element*>(L"groupdirectory", elem->GetParent()->GetParent()->GetParent()->GetParent()->GetParent()));
            CSafeElementPtr<DDScalableRichText> dirname;
            dirname.Assign(regElem<DDScalableRichText*>(L"dirname", groupdirectory));
            CSafeElementPtr<DDScalableRichText> dirdetails;
            dirdetails.Assign(regElem<DDScalableRichText*>(L"dirdetails", groupdirectory));
            CSafeElementPtr<Element> tasks;
            tasks.Assign(regElem<Element*>(L"tasks", groupdirectory));
            CSafeElementPtr<DDScalableButton> More;
            More.Assign(regElem<DDScalableButton*>(L"More", groupdirectory));
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
            dirdetails->SetLayoutPos(-3);
            tasks->SetVisible(false);
            More->SetVisible(false);
            groupdirlist->SetLayoutPos(-3);
            parserSubview->CreateElement(L"customizegroup", nullptr, groupdirlist->GetParent(), nullptr, &customizegroup);
            groupdirlist->GetParent()->Add(&customizegroup, 1);

            GTRANS_DESC transDesc[2];
            TriggerTranslate(customizegroup, transDesc, 0, 0.0f, 0.5f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false);
            TriggerFade(customizegroup, transDesc, 1, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, customizegroup->GetDisplayNode(), &tsbInfo);

            CSafeElementPtr<DDColorPicker> DDCP_Group;
            DDCP_Group.Assign(regElem<DDColorPicker*>(L"DDCP_Group", customizegroup));
            DDCP_Group->SetThemeAwareness(true);
            vector<DDScalableElement*> elemTargets{};
            vector<DDScalableButton*> btnTargets{};
            CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
            if (g_issubviewopen) btnTargets.push_back(fullscreeninner);
            CSafeElementPtr<DDScalableElement> iconElement;
            iconElement.Assign(regElem<DDScalableElement*>(L"iconElem", ((DDLVActionButton*)elem)->GetAssociatedItem()));
            elemTargets.push_back(iconElement);
            DDCP_Group->SetTargetElements(elemTargets);
            DDCP_Group->SetTargetButtons(btnTargets);
            elemTargets.clear();
            btnTargets.clear();

            RegKeyValue rkv = { nullptr, nullptr, nullptr, iconElement->GetGroupColor() };
            DDCP_Group->SetRegKeyValue(rkv);
        }
    }

    void PinGroup(Element* elem, Event* iev)
    {
        static int i{};
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<LVItem> lviTarget;
            lviTarget.Assign(((DDLVActionButton*)elem)->GetAssociatedItem());
            lviTarget->SetMemorySelected(false);
            for (i = 0; i < pm.size(); i++)
            {
                if (lviTarget == pm[i]) break;
            }
            CSafeElementPtr<Element> innerElem;
            innerElem.Assign(regElem<Element*>(L"innerElem", lviTarget));
            if (lviTarget->GetGroupSize() == LVIGS_NORMAL)
            {
                HidePopupCore(false);
                lviTarget->SetGroupSize(LVIGS_MEDIUM);
                GTRANS_DESC transDesc[2];
                TriggerFade(lviTarget, transDesc, 0, 0.5f, 0.633f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                TriggerScaleIn(lviTarget, transDesc, 1, 0.5f, 0.75f, 0.0f, 0.0f, 0.0f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                TransitionStoryboardInfo tsbInfo = {};
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, lviTarget->GetDisplayNode(), &tsbInfo);
            }
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
            lviTarget->SetSizedFromGroup(true);
            RearrangeIcons(true, false, true);
            lviTarget->SetRefreshState(true);
            lviTarget->SetMoving(false);
        }
    }

    void AdjustGroupSize(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<LVItem> lviTarget;
            lviTarget.Assign(((DDLVActionButton*)elem)->GetAssociatedItem());
            lviTarget->SetMemorySelected(false);
            float scaleX{}, scaleY{};
            CSafeElementPtr<Element> groupdirectory;
            groupdirectory.Assign(regElem<Element*>(L"groupdirectory", ((DDLVActionButton*)elem)->GetAssociatedItem()));
            groupdirectory->DestroyAll(true);
            if (elem->GetID() == StrToID(L"Smaller"))
            {
                switch (lviTarget->GetGroupSize())
                {
                case LVIGS_MEDIUM:
                    scaleX = 1.506f, scaleY = 1.5f;
                    break;
                case LVIGS_WIDE:
                    scaleX = 1.504f, scaleY = 1.0f;
                    break;
                case LVIGS_LARGE:
                    scaleX = 1.0f, scaleY = 1.5f;
                    break;
                }
                lviTarget->SetGroupSize((LVItemGroupSize)((int)lviTarget->GetGroupSize() - 1));
            }
            if (elem->GetID() == StrToID(L"Larger"))
            {
                switch (lviTarget->GetGroupSize())
                {
                case LVIGS_SMALL:
                    scaleX = 0.663f, scaleY = 0.667f;
                    break;
                case LVIGS_MEDIUM:
                    scaleX = 0.664f, scaleY = 1.0f;
                    break;
                case LVIGS_WIDE:
                    scaleX = 1.0f, scaleY = 0.667f;
                    break;
                }
                lviTarget->SetGroupSize((LVItemGroupSize)((int)lviTarget->GetGroupSize() + 1));
            }
            lviTarget->SetSizedFromGroup(true);
            RearrangeIcons(true, false, true);
            lviTarget->SetRefreshState(true);
            lviTarget->SetMoving(false);
            float originX = (localeType  == 1) ? 1.0f : 0.0f;
            GTRANS_DESC transDesc[1];
            TriggerScaleIn(lviTarget, transDesc, 0, 0.0f, 0.25f, 0.75f, 0.45f, 0.0f, 1.0f, scaleX, scaleY, originX, 0.0f, 1.0f, 1.0f, originX, 0.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, lviTarget->GetDisplayNode(), &tsbInfo);
        }
    }

    DWORD WINAPI AutoHideMoreOptions(LPVOID lpParam)
    {
        CSafeElementPtr<Element> tasksOld;
        tasksOld.Assign(regElem<Element*>(L"tasks", (LVItem*)lpParam));
        Sleep(5000);
        if (g_ensureNoRefresh && ((LVItem*)lpParam)->GetGroupSize() != LVIGS_NORMAL)
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
        if (iev->uidType == Button::Click)
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
        if (iev->uidType == Button::Click)
        {
            SHELLEXECUTEINFOW execInfo = {};
            execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
            execInfo.lpVerb = L"open";
            execInfo.nShow = SW_SHOWNORMAL;
            wstring fileStr = (((DDLVActionButton*)elem)->GetAssociatedItem())->GetFilename();
            LPCWSTR file = fileStr.c_str();
            execInfo.lpFile = file;
            ShellExecuteExW(&execInfo);
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
        if (iev->uidType == TouchButton::Click)
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
                        TriggerTranslate(pm[items], transDesc, 0, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX(), pm[items]->GetY(), false, false);
                        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                        DUI_SetGadgetZOrder(pm[items], -1);
                    }
                    else pm[items]->SetVisible(false);
                }
                g_invokedpagechange = true;
                PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
                PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
            }
            else
            {
                TriggerPageTransition(-1, dimensions);
            }
            nextpageMain->SetVisible(true);
            if (g_currentPageID == 1) prevpageMain->SetVisible(false);
        }
        if (iev->uidType == LVItem::Click && elem->GetMouseFocused() == true)
        {
            g_currentPageID = ((LVItem*)elem)->GetPage();
            for (int items = 0; items < pm.size(); items++)
            {
                if (pm[items]->GetPage() == g_currentPageID) pm[items]->SetVisible(!g_hiddenIcons);
                else pm[items]->SetVisible(false);
            }
            g_invokedpagechange = true;
            float xLoc = (localeType == 1) ? -0.4 : 0.9;
            float xLoc2 = (localeType == 1) ? 0.9 : -0.4;
            TogglePage(nextpage, xLoc, 0.25, 0.5, 0.5);
            if (g_currentPageID == 1) TogglePage(prevpage, xLoc2, 0.25, 0, 0.5);
            PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
            PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
            nextpageMain->SetVisible(true);
            if (g_currentPageID == 1) prevpageMain->SetVisible(false);
        }
    }
    void GoToNextPage(Element* elem, Event* iev)
    {
        static RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
        if (iev->uidType == TouchButton::Click)
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
                        TriggerTranslate(pm[items], transDesc, 0, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX(), pm[items]->GetY(), false, false);
                        ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
                        DUI_SetGadgetZOrder(pm[items], -1);
                    }
                    else pm[items]->SetVisible(false);
                }
                g_invokedpagechange = true;
                PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
                PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
            }
            else
            {
                TriggerPageTransition(1, dimensions);
            }
            prevpageMain->SetVisible(true);
            if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
        }
        if (iev->uidType == LVItem::Click && elem->GetMouseFocused() == true)
        {
            g_currentPageID = ((LVItem*)elem)->GetPage();
            for (int items = 0; items < pm.size(); items++)
            {
                if (pm[items]->GetPage() == g_currentPageID) pm[items]->SetVisible(!g_hiddenIcons);
                else pm[items]->SetVisible(false);
            }
            g_invokedpagechange = true;
            float xLoc = (localeType == 1) ? -0.4 : 0.9;
            float xLoc2 = (localeType == 1) ? 0.9 : -0.4;
            TogglePage(prevpage, xLoc, 0.25, 0.5, 0.5);
            if (g_currentPageID == g_maxPageID) TogglePage(nextpage, xLoc2, 0.25, 0, 0.5);
            PostMessageW(wnd->GetHWND(), WM_USER + 7, NULL, 1);
            PostMessageW(wnd->GetHWND(), WM_USER + 15, NULL, 0);
            prevpageMain->SetVisible(true);
            if (g_currentPageID == g_maxPageID) nextpageMain->SetVisible(false);
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
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, elem->GetDisplayNode(), &tsbInfo2);
                elem->SetSelected(true);
            }
            else
            {
                GTRANS_DESC transDesc2[1];
                TriggerScaleOut(elem, transDesc2, 0, 0.0f, 0.25f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, ElemOrigin, 0.5f, false, false);
                TransitionStoryboardInfo tsbInfo2 = {};
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, elem->GetDisplayNode(), &tsbInfo2);
                DelayedElementActions* dea = new DelayedElementActions{ 250, elem };
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
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, togglepageGlyph->GetDisplayNode(), &tsbInfo);
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
        if (pm[yV->num]->GetGroupedDirState() == true && g_treatdirasgroup == true)
        {
            int x = padding * g_flScaleFactor, y = padding * g_flScaleFactor;
            vector<ThumbIcons> strs;
            unsigned short count = 0;
            wstring folderPath = RemoveQuotes(pm[yV->num]->GetFilename());
            EnumerateFolder((LPWSTR)folderPath.c_str(), nullptr, true, &count, nullptr, 4);
            EnumerateFolderForThumbnails((LPWSTR)folderPath.c_str(), &strs, 4);
            for (int thumbs = 0; thumbs < count; thumbs++)
            {
                HBITMAP thumbIcon{};
                GetShellItemImage(thumbIcon, (strs[thumbs].GetFilename()).c_str(), g_gpiconsz, g_gpiconsz);
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
                        COLORREF iconcolor = (iconColorID == 1) ? ImmersiveColor : IconColorizationColor;
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
        }
        delete yV;
        return 0;
    }

    void ApplyIcons(vector<LVItem*> pmLVItem, vector<DDScalableElement*> pmIcon, DesktopIcon* di, bool subdirectory, int id, float scale)
    {
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
        HBITMAP bmp{};
        if (pmLVItem[id]->GetGroupedDirState() == false || g_treatdirasgroup == false)
        {
            if (g_touchmode && pmLVItem[id]->GetTileSize() == LVITS_ICONONLY) GetShellItemImage(bmp, RemoveQuotes(pmLVItem[id]->GetFilename()).c_str(), 32 * scale, 32 * scale);
            else GetShellItemImage(bmp, RemoveQuotes(pmLVItem[id]->GetFilename()).c_str(), g_iconsz * scale, g_iconsz * scale);
        }
        else bmp = dummyii;
        HBITMAP bmpShortcut{};
        IconToBitmap(icoShortcut, bmpShortcut, g_shiconsz * scale * g_flScaleFactor, g_shiconsz * scale * g_flScaleFactor);
        DestroyIcon(icoShortcut);
        IterateBitmap(bmpShortcut, UndoPremultiplication, 1, 0, 1, NULL);
        if (bmp != dummyii)
        {
            float shadowintensity = g_touchmode ? 0.8 : 0.33;
            HBITMAP bmpShadow{};
            AddPaddingToBitmap(bmp, bmpShadow, 8 * scale * g_flScaleFactor, 8 * scale * g_flScaleFactor, 8 * scale * g_flScaleFactor, 8 * scale * g_flScaleFactor);
            IterateBitmap(bmpShadow, SimpleBitmapPixelHandler, 0, (int)(4 * scale * g_flScaleFactor), shadowintensity, NULL);
            if (!g_isGlass || &pmLVItem == &pm) di->iconshadow = bmpShadow;
            if (g_isDarkIconsEnabled)
            {
                if (pmLVItem[id]->GetColorLock() == false)
                {
                    HBITMAP bmpOverlay{};
                    AddPaddingToBitmap(bmp, bmpOverlay, 0, 0, 0, 0);
                    COLORREF lightness = GetMostFrequentLightnessFromIcon(bmp, g_iconsz * scale * g_flScaleFactor);
                    if (bmp != dummyii) IterateBitmap(bmp, UndoPremultiplication, 3, 0, 1, RGB(18, 18, 18));
                    bool compEffects = (GetGValue(lightness) < 208);
                    IterateBitmap(bmpOverlay, ColorToAlpha, 1, 0, 1, lightness);
                    CompositeBitmaps(bmp, bmpOverlay, compEffects, 0.44);
                    DeleteObject(bmpOverlay);
                }
                if (GetGValue(GetMostFrequentLightnessFromIcon(bmpShortcut, g_iconsz * scale * g_flScaleFactor)) > 208) IterateBitmap(bmpShortcut, InvertConstHue, 1, 0, 1, NULL);
            }
            if (g_isGlass && !g_isDarkIconsEnabled && !g_isColorized && pmLVItem[id]->GetColorLock() == false)
            {
                if (&pmLVItem == &pm)
                {
                    HDC hdc = GetDC(nullptr);
                    HBITMAP bmpOverlay{};
                    AddPaddingToBitmap(bmp, bmpOverlay, 0, 0, 0, 0);
                    HBITMAP bmpOverlay2{};
                    AddPaddingToBitmap(bmp, bmpOverlay2, 0, 0, 0, 0);
                    IterateBitmap(bmpOverlay, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
                    CompositeBitmaps(bmpOverlay, bmpOverlay2, true, 0.5);
                    IterateBitmap(bmp, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
                    CompositeBitmaps(bmp, bmpOverlay, true, 1);
                    DeleteObject(bmpOverlay);
                    DeleteObject(bmpOverlay2);
                    HBITMAP bmpOverlay3{};
                    AddPaddingToBitmap(bmp, bmpOverlay3, 0, 0, 0, 0);
                    IterateBitmap(bmpOverlay3, DesaturateWhitenGlass, 1, 0, 0.4, 16777215);
                    POINT iconmidpoint = { pm[id]->GetX() + iconpm[id]->GetX() + g_iconsz * scale * g_flScaleFactor / 2, pm[id]->GetY() + iconpm[id]->GetY() + g_iconsz * scale * g_flScaleFactor / 2 };
                    IterateBitmap(bmp, DesaturateWhitenGlass, 1, 0, 1, GetLightestPixel(bmp));
                    COLORREF glassColor = GetColorFromPixel(hdc, iconmidpoint);
                    IncreaseBrightness(glassColor);
                    IterateBitmap(bmp, StandardBitmapPixelHandler, 3, 0, 0.8, glassColor);
                    CompositeBitmaps(bmp, bmpOverlay3, false, 0);
                    DeleteObject(bmpOverlay3);
                    ReleaseDC(nullptr, hdc);
                }
                else
                {
                    HBITMAP bmpOverlay{};
                    AddPaddingToBitmap(bmp, bmpOverlay, 0, 0, 0, 0);
                    IterateBitmap(bmp, SimpleBitmapPixelHandler, 0, 0, 1, RGB(0, 0, 0));
                    CompositeBitmaps(bmp, bmpOverlay, true, 1);
                    IterateBitmap(bmp, DesaturateWhitenGlass, 1, 0, 0.4, GetLightestPixel(bmp));
                    DeleteObject(bmpOverlay);
                }
            }
        }
        if (g_isColorized)
        {
            COLORREF iconcolor = (iconColorID == 1) ? ImmersiveColor : IconColorizationColor;
            if (pmLVItem[id]->GetColorLock() == false) IterateBitmap(bmp, EnhancedBitmapPixelHandler, 1, 0, 1, iconcolor);
            IterateBitmap(bmpShortcut, StandardBitmapPixelHandler, 1, 0, 1, iconcolor);
        }
        di->icon = bmp;
        di->iconshortcut = bmpShortcut;
    }

    void IconThumbHelper(int id)
    {
        UpdateCache* uc{};
        CValuePtr v = emptyspace->GetValue(Element::BackgroundProp, 1, uc);
        iconpm[id]->DestroyAll(true);
        iconpm[id]->SetClass(L"");
        iconpm[id]->SetValue(Element::BackgroundProp, 1, v);
        shadowpm[id]->SetVisible(true);
        int groupspace = 8 * g_flScaleFactor;
        if (g_touchmode && pm[id]->GetGroupSize() == LVIGS_NORMAL)
        {
            iconpm[id]->SetWidth(g_iconsz * g_flScaleFactor + 2 * groupspace);
            iconpm[id]->SetHeight(g_iconsz * g_flScaleFactor + 2 * groupspace);
        }
        CSafeElementPtr<Element> iconcontainer;
        iconcontainer.Assign(regElem<Element*>(L"iconcontainer", pm[id]));
        if (pm[id]->GetGroupedDirState() == true && g_treatdirasgroup == true)
        {
            iconpm[id]->SetClass(L"groupthumbnail");
            shadowpm[id]->SetVisible(false);

            if (g_touchmode && pm[id]->GetGroupSize() == LVIGS_NORMAL)
            {
                iconcontainer->SetPadding(groupspace, groupspace, groupspace, groupspace);
                iconpm[id]->SetWidth(g_iconsz * g_flScaleFactor);
                iconpm[id]->SetHeight(g_iconsz * g_flScaleFactor);
            }
        }
        free(uc);
    }

    void UpdateTileOnColorChange(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == DDScalableElement::AssociatedColorProp())
        {
            int i;
            for (i = 0; i < pm.size(); i++)
            {
                if (elem == iconpm[i]) break;
            }
            COLORREF crDefault = g_theme ? RGB(208, 208, 208) : RGB(48, 48, 48);
            pm[i]->SetAssociatedColor(((DDScalableElement*)elem)->GetAssociatedColor() == -1 ? crDefault : ((DDScalableElement*)elem)->GetAssociatedColor());
            pm[i]->SetRefreshState(true);
            yValue* yV = new yValue{ i };
            QueueUserWorkItem(RearrangeIconsHelper, yV, 0);
        }
    }

    void SelectSubItem(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            SHELLEXECUTEINFOW execInfo = {};
            execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
            execInfo.lpVerb = L"open";
            execInfo.nShow = SW_SHOWNORMAL;
            wstring temp = RemoveQuotes(((LVItem*)elem)->GetFilename());
            execInfo.lpFile = temp.c_str();
            ShellExecuteExW(&execInfo);
        }
    }

    void SelectSubItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (g_touchmode && pProp == Button::PressedProp())
        {
            CSafeElementPtr<Element> innerElem;
            innerElem.Assign(regElem<Element*>(L"innerElem", elem));
            innerElem->SetEnabled(!((LVItem*)elem)->GetPressed());
        }
    }

    void ShowDirAsGroup(LVItem* lvi)
    {
        unsigned short lviCount = 0;
        int count2{};
        EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), nullptr, true, &lviCount);
        SendMessageW(g_hWndTaskbar, WM_COMMAND, 419, 0);
        fullscreenAnimation(800 * g_flScaleFactor, 480 * g_flScaleFactor, 0.8, 0.88);
        SetWindowPos(subviewwnd->GetHWND(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        lvi->SetMemorySelected(true);
        g_tempElem2 = lvi;
        Element* groupdirectory{};
        parserSubview->CreateElement(L"groupdirectory", nullptr, nullptr, nullptr, (Element**)&groupdirectory);
        CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
        fullscreeninner->Add((Element**)&groupdirectory, 1);
        CSafeElementPtr<DDScalableElement> iconElement;
        iconElement.Assign(regElem<DDScalableElement*>(L"iconElem", lvi));
        fullscreeninner->SetDDCPIntensity(iconElement->GetDDCPIntensity());
        fullscreeninner->SetAssociatedColor(iconElement->GetAssociatedColor());
        CSafeElementPtr<TouchScrollViewer> groupdirlist;
        groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory));
        CSafeElementPtr<DDScalableButton> lvi_SubUIContainer;
        lvi_SubUIContainer.Assign(regElem<DDScalableButton*>(L"SubUIContainer", groupdirlist));
        COLORREF colorPickerPalette[6] =
        {
            RGB(0, 145, 248), RGB(189, 91, 203), RGB(239, 39, 51), RGB(248, 116, 29), RGB(255, 195, 0), RGB(0, 204, 106)
        };
        if (lviCount > 0)
        {
            vector<IElementListener*> v_pels;
            vector<LVItem*>* subpm = new vector<LVItem*>;
            vector<DDScalableElement*>* subiconpm = new vector<DDScalableElement*>;
            vector<Element*>* subshadowpm = new vector<Element*>;
            vector<Element*>* subshortpm = new vector<Element*>;
            vector<RichText*>* subfilepm = new vector<RichText*>;
            StyleSheet* sheet = pMain->GetSheet();
            CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
            parser->GetSheet(g_theme ? L"default" : L"defaultdark", &sheetStorage);
            const WCHAR* elemname = g_touchmode ? L"outerElemTouch" : L"outerElemGrouped";
            for (int i = 0; i < lviCount; i++)
            {
                LVItem* outerElemGrouped;
                parser->CreateElement(elemname, nullptr, nullptr, nullptr, (Element**)&outerElemGrouped);
                outerElemGrouped->SetValue(Element::SheetProp, 1, sheetStorage);
                lvi_SubUIContainer->Add((Element**)&outerElemGrouped, 1);
                CSafeElementPtr<DDScalableElement> iconElem;
                iconElem.Assign(regElem<DDScalableElement*>(L"iconElem", outerElemGrouped));
                CSafeElementPtr<Element> shortcutElem;
                shortcutElem.Assign(regElem<Element*>(L"shortcutElem", outerElemGrouped));
                CSafeElementPtr<Element> iconElemShadow;
                iconElemShadow.Assign(regElem<Element*>(L"iconElemShadow", outerElemGrouped));
                CSafeElementPtr<RichText> textElem;
                textElem.Assign(regElem<RichText*>(L"textElem", outerElemGrouped));
                subpm->push_back(outerElemGrouped);
                subiconpm->push_back(iconElem);
                subshortpm->push_back(shortcutElem);
                subshadowpm->push_back(iconElemShadow);
                subfilepm->push_back(textElem);
            }
            CSafeElementPtr<LVItem> PendingContainer;
            PendingContainer.Assign(regElem<LVItem*>(L"PendingContainer", groupdirectory));
            PendingContainer->SetVisible(true);
            EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), subpm, false, nullptr, &count2, lviCount);
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
                if ((*subpm)[j]->GetHiddenState() == true)
                {
                    (*subiconpm)[j]->SetAlpha(128);
                    (*subshadowpm)[j]->SetAlpha(0);
                    (*subfilepm)[j]->SetAlpha(128);
                }
                v_pels.push_back(assignFn((*subpm)[j], SelectSubItem, true));
                v_pels.push_back(assignFn((*subpm)[j], ItemRightClick, true));
                v_pels.push_back(assignExtendedFn((*subpm)[j], SelectSubItemListener, true));
                (*subpm)[j]->SetListeners(v_pels);
                v_pels.clear();
                if (!g_touchmode) (*subpm)[j]->SetClass(L"singleclicked");
                int xRender = (localeType == 1) ? (centered->GetWidth() - (dimensions.left + dimensions.right + outerSizeX)) - x : x;
                (*subpm)[j]->SetX(xRender), (*subpm)[j]->SetY(y);
                x += outerSizeX;
                xRuns++;
                if (x > centered->GetWidth() - (dimensions.left + dimensions.right + outerSizeX))
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
            groupdirlist->SetHeight((480 * g_flScaleFactor - (dirtitle->GetHeight() + dimensions.top + dimensions.bottom)));
            for (int j = 0; j < lviCount; j++)
            {
                if (localeType == 1 && y > groupdirlist->GetHeight())
                    (*subpm)[j]->SetX((*subpm)[j]->GetX() - GetSystemMetricsForDpi(SM_CXVSCROLL, g_dpi));
            }
            lvi->SetChildItems((*subpm));
            lvi->SetChildIcons((*subiconpm));
            lvi->SetChildShadows((*subshadowpm));
            lvi->SetChildShortcutArrows((*subshortpm));
            lvi->SetChildFilenames((*subfilepm));
            DWORD animThread2;
            yValueEx* yV = new yValueEx{ lviCount, NULL, NULL, subpm, subiconpm, subshadowpm, subshortpm, subfilepm, PendingContainer, lvi };
            HANDLE animThreadHandle2 = CreateThread(nullptr, 0, subfastin, (LPVOID)yV, 0, &animThread2);
            if (animThreadHandle2) CloseHandle(animThreadHandle2);
        }
        else
        {
            CSafeElementPtr<Element> emptyview;
            emptyview.Assign(regElem<Element*>(L"emptyview", groupdirectory));
            emptyview->SetVisible(true);
            CSafeElementPtr<DDScalableElement> emptygraphic;
            emptygraphic.Assign(regElem<DDScalableElement*>(L"emptygraphic", groupdirectory));
            if (iconElement->GetGroupColor() >= 2)
                emptygraphic->SetAssociatedColor(colorPickerPalette[iconElement->GetGroupColor() - 2]);
            CSafeElementPtr<Element> dirtitle;
            dirtitle.Assign(regElem<Element*>(L"dirtitle", groupdirectory));
            dirtitle->SetVisible(true);
        }
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
        g_checkifelemexists = true;
        CSafeElementPtr<DDLVActionButton> Pin;
        Pin.Assign(regElem<DDLVActionButton*>(L"Pin", groupdirectory));
        CSafeElementPtr<DDLVActionButton> Customize;
        Customize.Assign(regElem<DDLVActionButton*>(L"Customize", groupdirectory));
        CSafeElementPtr<DDLVActionButton> OpenInExplorer;
        OpenInExplorer.Assign(regElem<DDLVActionButton*>(L"OpenInExplorer", groupdirectory));
        Pin->SetVisible(true), Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
        assignFn(OpenInExplorer, OpenGroupInExplorer);
        assignFn(Customize, OpenCustomizePage);
        assignFn(Pin, PinGroup);
        OpenInExplorer->SetAssociatedItem(lvi);
        Customize->SetAssociatedItem(lvi);
        Pin->SetAssociatedItem(lvi);
    }

    void ShowDirAsGroupDesktop(LVItem* lvi)
    {
        unsigned short lviCount = 0;
        int count2{};
        EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), nullptr, true, &lviCount);
        CSafeElementPtr<Element> groupdirectoryOld;
        groupdirectoryOld.Assign(regElem<Element*>(L"groupdirectory", lvi));
        if (groupdirectoryOld) return;
        StyleSheet* sheet = pSubview->GetSheet();
        CValuePtr sheetStorage = DirectUI::Value::CreateStyleSheet(sheet);
        parserSubview->GetSheet(g_theme ? L"popup" : L"popupdark", &sheetStorage);
        lvi->SetMemorySelected(true);
        g_tempElem2 = lvi;
        Element* groupdirectory{};
        parserSubview->CreateElement(L"groupdirectory", nullptr, lvi, nullptr, (Element**)&groupdirectory);
        groupdirectory->SetValue(Element::SheetProp, 1, sheetStorage);
        lvi->Add((Element**)&groupdirectory, 1);
        CSafeElementPtr<DDScalableElement> iconElement;
        iconElement.Assign(regElem<DDScalableElement*>(L"iconElem", lvi));
        CSafeElementPtr<TouchScrollViewer> groupdirlist;
        groupdirlist.Assign(regElem<TouchScrollViewer*>(L"groupdirlist", groupdirectory));
        CSafeElementPtr<DDScalableButton> lvi_SubUIContainer;
        lvi_SubUIContainer.Assign(regElem<DDScalableButton*>(L"SubUIContainer", groupdirlist));
        lvi_SubUIContainer->SetVisible(true);
        COLORREF colorPickerPalette[6] =
        {
            RGB(0, 145, 248), RGB(189, 91, 203), RGB(239, 39, 51), RGB(248, 116, 29), RGB(255, 195, 0), RGB(0, 204, 106)
        };
        if (lviCount > 0)
        {
            vector<IElementListener*> v_pels;
            vector<LVItem*>* d_subpm = new vector<LVItem*>;
            vector<DDScalableElement*>* d_subiconpm = new vector<DDScalableElement*>;
            vector<Element*>* d_subshadowpm = new vector<Element*>;
            vector<Element*>* d_subshortpm = new vector<Element*>;
            vector<RichText*>* d_subfilepm = new vector<RichText*>;
            sheet = pMain->GetSheet();
            CValuePtr sheetStorage2 = DirectUI::Value::CreateStyleSheet(sheet);
            parser->GetSheet(g_theme ? L"default" : L"defaultdark", &sheetStorage2);
            const WCHAR* elemname = g_touchmode ? L"outerElemTouch" : L"outerElemGrouped";
            for (int i = 0; i < lviCount; i++)
            {
                LVItem* outerElemGrouped;
                parser->CreateElement(elemname, nullptr, nullptr, nullptr, (Element**)&outerElemGrouped);
                outerElemGrouped->SetValue(Element::SheetProp, 1, sheetStorage2);
                lvi_SubUIContainer->Add((Element**)&outerElemGrouped, 1);
                CSafeElementPtr<DDScalableElement> iconElem;
                iconElem.Assign(regElem<DDScalableElement*>(L"iconElem", outerElemGrouped));
                CSafeElementPtr<Element> shortcutElem;
                shortcutElem.Assign(regElem<Element*>(L"shortcutElem", outerElemGrouped));
                CSafeElementPtr<Element> iconElemShadow;
                iconElemShadow.Assign(regElem<Element*>(L"iconElemShadow", outerElemGrouped));
                CSafeElementPtr<RichText> textElem;
                textElem.Assign(regElem<RichText*>(L"textElem", outerElemGrouped));
                d_subpm->push_back(outerElemGrouped);
                d_subiconpm->push_back(iconElem);
                d_subshortpm->push_back(shortcutElem);
                d_subshadowpm->push_back(iconElemShadow);
                d_subfilepm->push_back(textElem);
            }
            CSafeElementPtr<LVItem> PendingContainer;
            PendingContainer.Assign(regElem<LVItem*>(L"PendingContainer", groupdirectory));
            PendingContainer->SetVisible(true);
            EnumerateFolder((LPWSTR)RemoveQuotes(lvi->GetFilename()).c_str(), &(*d_subpm), false, nullptr, &count2, lviCount);
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
                if ((*d_subpm)[j]->GetHiddenState() == true)
                {
                    (*d_subiconpm)[j]->SetAlpha(128);
                    (*d_subshadowpm)[j]->SetAlpha(0);
                    (*d_subfilepm)[j]->SetAlpha(128);
                }
                v_pels.push_back(assignFn((*d_subpm)[j], SelectSubItem, true));
                v_pels.push_back(assignFn((*d_subpm)[j], ItemRightClick, true));
                v_pels.push_back(assignExtendedFn((*d_subpm)[j], SelectSubItemListener, true));
                (*d_subpm)[j]->SetListeners(v_pels);
                v_pels.clear();
                if (!g_touchmode) (*d_subpm)[j]->SetClass(L"singleclicked");
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
            groupdirlist->SetHeight((lvi->GetHeight() - (dirtitle->GetHeight() + dimensions.top + dimensions.bottom)));
            for (int j = 0; j < lviCount; j++)
            {
                if (localeType == 1 && y > groupdirlist->GetHeight())
                    (*d_subpm)[j]->SetX((*d_subpm)[j]->GetX() - GetSystemMetricsForDpi(SM_CXVSCROLL, g_dpi));
            }
            lvi->SetChildItems((*d_subpm));
            lvi->SetChildIcons((*d_subiconpm));
            lvi->SetChildShadows((*d_subshadowpm));
            lvi->SetChildShortcutArrows((*d_subshortpm));
            lvi->SetChildFilenames((*d_subfilepm));
            DWORD animThread2;
            yValueEx* yV = new yValueEx{ lviCount, NULL, NULL, d_subpm, d_subiconpm, d_subshadowpm, d_subshortpm, d_subfilepm, PendingContainer, lvi };
            HANDLE animThreadHandle2 = CreateThread(nullptr, 0, subfastin, (LPVOID)yV, 0, &animThread2);
            if (animThreadHandle2) CloseHandle(animThreadHandle2);
        }
        else
        {
            CSafeElementPtr<Element> emptyview;
            emptyview.Assign(regElem<Element*>(L"emptyview", groupdirectory));
            emptyview->SetVisible(true);
            CSafeElementPtr<DDScalableElement> emptygraphic;
            if (iconElement->GetGroupColor() >= 2)
                emptygraphic->SetAssociatedColor(colorPickerPalette[iconElement->GetGroupColor() - 2]);
            CSafeElementPtr<Element> dirtitle;
            dirtitle.Assign(regElem<Element*>(L"dirtitle", groupdirectory));
            dirtitle->SetVisible(true);
        }
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
        CSafeElementPtr<DDLVActionButton> More;
        More.Assign(regElem<DDLVActionButton*>(L"More", groupdirectory));
        CSafeElementPtr<Element> tasks;
        tasks.Assign(regElem<Element*>(L"tasks", groupdirectory));
        tasks->SetLayoutPos(-3);
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
        More->SetLayoutPos(2);
        Smaller->SetVisible(true), Larger->SetVisible(true), Unpin->SetVisible(true), Customize->SetVisible(true), OpenInExplorer->SetVisible(true);
        assignFn(More, ShowMoreOptions);
        assignFn(Smaller, AdjustGroupSize);
        assignFn(Larger, AdjustGroupSize);
        assignFn(OpenInExplorer, OpenGroupInExplorer);
        assignFn(Customize, OpenCustomizePage);
        assignFn(Unpin, PinGroup);
        if (lvi->GetGroupSize() == LVIGS_SMALL) Smaller->SetEnabled(false);
        if (lvi->GetGroupSize() == LVIGS_LARGE) Larger->SetEnabled(false);
        More->SetAssociatedItem(lvi);
        Smaller->SetAssociatedItem(lvi);
        Larger->SetAssociatedItem(lvi);
        OpenInExplorer->SetAssociatedItem(lvi);
        Customize->SetAssociatedItem(lvi);
        Unpin->SetAssociatedItem(lvi);
    }

    void OpenDeskCpl(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click) ShellExecuteW(nullptr, L"open", L"control.exe", L"desk.cpl,Web,0", nullptr, SW_SHOW);
    }

    void OpenLog(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            wchar_t* desktoplog = new wchar_t[260];
            wchar_t* cBuffer = new wchar_t[260];
            DWORD d = GetEnvironmentVariableW(L"userprofile", cBuffer, 260);
            StringCchPrintfW(desktoplog, 260, L"%s\\Documents\\DirectDesktop.log", cBuffer);
            ShellExecuteW(nullptr, L"open", L"notepad.exe", desktoplog, nullptr, SW_SHOW);
            delete[] desktoplog;
            delete[] cBuffer;
        }
    }

    void DisableColorPicker(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDColorPicker> DDCP_Icons;
            DDCP_Icons.Assign(regElem<DDColorPicker*>(L"DDCP_Icons", (Element*)g_tempElem));
            DDCP_Icons->SetEnabled(((DDToggleButton*)elem)->GetCheckedState());
        }
    }

    void DisableDarkToggle(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            CSafeElementPtr<DDToggleButton> EnableDarkIcons;
            EnableDarkIcons.Assign(regElem<DDToggleButton*>(L"EnableDarkIcons", (Element*)g_tempElem));
            if (((DDCheckBox*)elem)->GetCheckedState() == true)
            {
                EnableDarkIcons->SetCheckedState(!g_theme);
                g_isDarkIconsEnabled = !g_theme;
                SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"DarkIcons", !g_theme, false, nullptr);
            }
            EnableDarkIcons->SetEnabled(!((DDCheckBox*)elem)->GetCheckedState());
        }
    }

    void ShowPage1(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            PageTab1->SetSelected(true);
            PageTab2->SetSelected(false);
            PageTab3->SetSelected(false);
            SubUIContainer->DestroyAll(true);
            Element* SettingsPage1;
            parserSubview->CreateElement(L"SettingsPage1", nullptr, nullptr, nullptr, (Element**)&SettingsPage1);
            SubUIContainer->Add((Element**)&SettingsPage1, 1);
            CSafeElementPtr<DDToggleButton> ItemCheckboxes;
            ItemCheckboxes.Assign(regElem<DDToggleButton*>(L"ItemCheckboxes", SettingsPage1));
            CSafeElementPtr<DDToggleButton> ShowHiddenFiles;
            ShowHiddenFiles.Assign(regElem<DDToggleButton*>(L"ShowHiddenFiles", SettingsPage1));
            CSafeElementPtr<DDToggleButton> FilenameExts;
            FilenameExts.Assign(regElem<DDToggleButton*>(L"FilenameExts", SettingsPage1));
            CSafeElementPtr<DDToggleButton> TreatDirAsGroup;
            TreatDirAsGroup.Assign(regElem<DDToggleButton*>(L"TreatDirAsGroup", SettingsPage1));
            CSafeElementPtr<DDToggleButton> TripleClickAndHide;
            TripleClickAndHide.Assign(regElem<DDToggleButton*>(L"TripleClickAndHide", SettingsPage1));
            CSafeElementPtr<DDToggleButton> LockIconPos;
            LockIconPos.Assign(regElem<DDToggleButton*>(L"LockIconPos", SettingsPage1));
            RegKeyValue rkvTemp{};
            rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
            rkvTemp._valueToFind = L"AutoCheckSelect";
            ItemCheckboxes->SetCheckedState(g_showcheckboxes);
            ItemCheckboxes->SetAssociatedBool(&g_showcheckboxes);
            ItemCheckboxes->SetRegKeyValue(rkvTemp);
            rkvTemp._valueToFind = L"Hidden";
            if (GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind) == 1) ShowHiddenFiles->SetCheckedState(true);
            else ShowHiddenFiles->SetCheckedState(false);
            ShowHiddenFiles->SetAssociatedFn(InitLayout);
            ShowHiddenFiles->SetRegKeyValue(rkvTemp);
            rkvTemp._valueToFind = L"HideFileExt";
            FilenameExts->SetCheckedState(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
            FilenameExts->SetAssociatedFn(InitLayout);
            FilenameExts->SetRegKeyValue(rkvTemp);
            rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"TreatDirAsGroup";
            TreatDirAsGroup->SetCheckedState(g_treatdirasgroup);
            TreatDirAsGroup->SetAssociatedBool(&g_treatdirasgroup);
            TreatDirAsGroup->SetAssociatedFn(InitLayout);
            TreatDirAsGroup->SetRegKeyValue(rkvTemp);
            rkvTemp._valueToFind = L"TripleClickAndHide";
            TripleClickAndHide->SetCheckedState(g_tripleclickandhide);
            TripleClickAndHide->SetAssociatedBool(&g_tripleclickandhide);
            TripleClickAndHide->SetRegKeyValue(rkvTemp);
            rkvTemp._valueToFind = L"LockIconPos";
            LockIconPos->SetCheckedState(g_lockiconpos);
            LockIconPos->SetAssociatedBool(&g_lockiconpos);
            LockIconPos->SetRegKeyValue(rkvTemp);
            assignFn(ItemCheckboxes, ToggleSetting);
            assignFn(ShowHiddenFiles, ToggleSetting);
            assignFn(FilenameExts, ToggleSetting);
            assignFn(TreatDirAsGroup, ToggleSetting);
            assignFn(TripleClickAndHide, ToggleSetting);
            assignFn(LockIconPos, ToggleSetting);
        }
    }

    void ShowPage2(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            PageTab1->SetSelected(false);
            PageTab2->SetSelected(true);
            PageTab3->SetSelected(false);
            SubUIContainer->DestroyAll(true);
            Element* SettingsPage2;
            parserSubview->CreateElement(L"SettingsPage2", nullptr, nullptr, nullptr, (Element**)&SettingsPage2);
            SubUIContainer->Add((Element**)&SettingsPage2, 1);
            CSafeElementPtr<DDToggleButton> EnableAccent;
            EnableAccent.Assign(regElem<DDToggleButton*>(L"EnableAccent", SettingsPage2));
            CSafeElementPtr<DDColorPicker> DDCP_Icons;
            DDCP_Icons.Assign(regElem<DDColorPicker*>(L"DDCP_Icons", SettingsPage2));
            CSafeElementPtr<DDToggleButton> EnableDarkIcons;
            EnableDarkIcons.Assign(regElem<DDToggleButton*>(L"EnableDarkIcons", SettingsPage2));
            CSafeElementPtr<DDCheckBox> AutoDarkIcons;
            AutoDarkIcons.Assign(regElem<DDCheckBox*>(L"AutoDarkIcons", SettingsPage2));
            CSafeElementPtr<DDToggleButton> IconThumbnails;
            IconThumbnails.Assign(regElem<DDToggleButton*>(L"IconThumbnails", SettingsPage2));
            CSafeElementPtr<DDScalableButton> DesktopIconSettings;
            DesktopIconSettings.Assign(regElem<DDScalableButton*>(L"DesktopIconSettings", SettingsPage2));
            RegKeyValue rkvTemp{};
            rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"AccentColorIcons";
            EnableAccent->SetCheckedState(g_isColorized);
            EnableAccent->SetAssociatedBool(&g_isColorized);
            EnableAccent->SetAssociatedFn(RearrangeIcons);
            EnableAccent->SetRegKeyValue(rkvTemp);
            rkvTemp._valueToFind = L"IconColorID";
            DDCP_Icons->SetThemeAwareness(false);
            DDCP_Icons->SetEnabled(g_isColorized);
            DDCP_Icons->SetRegKeyValue(rkvTemp);
            vector<DDScalableElement*> elemTargets{};
            elemTargets.push_back(RegistryListener);
            DDCP_Icons->SetTargetElements(elemTargets);
            elemTargets.clear();
            rkvTemp._valueToFind = L"DarkIcons";
            EnableDarkIcons->SetEnabled(!g_automaticDark);
            EnableDarkIcons->SetCheckedState(g_isDarkIconsEnabled);
            EnableDarkIcons->SetAssociatedBool(&g_isDarkIconsEnabled);
            EnableDarkIcons->SetAssociatedFn(RearrangeIcons);
            EnableDarkIcons->SetRegKeyValue(rkvTemp);
            rkvTemp._valueToFind = L"AutoDarkIcons";
            AutoDarkIcons->SetCheckedState(g_automaticDark);
            AutoDarkIcons->SetAssociatedBool(&g_automaticDark);
            AutoDarkIcons->SetAssociatedFn(RearrangeIcons);
            AutoDarkIcons->SetRegKeyValue(rkvTemp);
            rkvTemp._path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", rkvTemp._valueToFind = L"IconsOnly";
            IconThumbnails->SetCheckedState(GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
            IconThumbnails->SetAssociatedFn(RearrangeIcons);
            IconThumbnails->SetRegKeyValue(rkvTemp);
            assignFn(EnableAccent, ToggleSetting);
            assignFn(EnableAccent, DisableColorPicker);
            assignFn(EnableDarkIcons, ToggleSetting);
            assignFn(AutoDarkIcons, ToggleSetting);
            assignFn(AutoDarkIcons, DisableDarkToggle);
            assignFn(IconThumbnails, ToggleSetting);
            assignFn(DesktopIconSettings, OpenDeskCpl);
            g_tempElem = (void*)SettingsPage2;
        }
    }

    void ShowPage3(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            PageTab1->SetSelected(false);
            PageTab2->SetSelected(false);
            PageTab3->SetSelected(true);
            SubUIContainer->DestroyAll(true);
            Element* SettingsPage3;
            parserSubview->CreateElement(L"SettingsPage3", nullptr, nullptr, nullptr, (Element**)&SettingsPage3);
            SubUIContainer->Add((Element**)&SettingsPage3, 1);
            CSafeElementPtr<DDToggleButton> EnableLogging;
            EnableLogging.Assign(regElem<DDToggleButton*>(L"EnableLogging", SettingsPage3));
            CSafeElementPtr<DDScalableButton> ViewLastLog;
            ViewLastLog.Assign(regElem<DDScalableButton*>(L"ViewLastLog", SettingsPage3));
            RegKeyValue rkvTemp{};
            rkvTemp._hKeyName = HKEY_CURRENT_USER, rkvTemp._path = L"Software\\DirectDesktop", rkvTemp._valueToFind = L"Logging";
            EnableLogging->SetCheckedState(7 - GetRegistryValues(rkvTemp._hKeyName, rkvTemp._path, rkvTemp._valueToFind));
            EnableLogging->SetRegKeyValue(rkvTemp);
            assignFn(EnableLogging, ToggleSetting);
            assignFn(ViewLastLog, OpenLog);
        }
    }

    void ShowSettings(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            fullscreenAnimation(800 * g_flScaleFactor, 480 * g_flScaleFactor, 0.8, 0.88);
            SetWindowPos(subviewwnd->GetHWND(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            BlurBackground(subviewwnd->GetHWND(), true, true, fullscreenpopupbg);
            g_issubviewopen = true;
            g_issettingsopen = true;
            Element* settingsview{};
            parserSubview->CreateElement(L"settingsview", nullptr, nullptr, nullptr, (Element**)&settingsview);
            CSafeElementPtr<DDScalableButton> fullscreeninner; fullscreeninner.Assign(regElem<DDScalableButton*>(L"fullscreeninner", centered));
            fullscreeninner->Add((Element**)&settingsview, 1);
            CSafeElementPtr<TouchScrollViewer> settingslist;
            settingslist.Assign(regElem<TouchScrollViewer*>(L"settingslist", settingsview));
            SubUIContainer = regElem<DDScalableButton*>(L"SubUIContainer", settingsview);
            PageTab1 = regElem<DDScalableButton*>(L"PageTab1", settingsview);
            PageTab2 = regElem<DDScalableButton*>(L"PageTab2", settingsview);
            PageTab3 = regElem<DDScalableButton*>(L"PageTab3", settingsview);
            assignFn(PageTab1, ShowPage1);
            assignFn(PageTab2, ShowPage2);
            assignFn(PageTab3, ShowPage3);
            ShowPage1(elem, iev);

            CValuePtr v;
            RECT dimensions;
            dimensions = *(settingsview->GetPadding(&v));
            CSafeElementPtr<DDScalableRichText> title;
            title.Assign(regElem<DDScalableRichText*>(L"title", settingsview));
            settingslist->SetHeight((480 * g_flScaleFactor - (title->GetHeight() + dimensions.top + dimensions.bottom)));

            CSafeElementPtr<DDScalableRichText> name;
            name.Assign(regElem<DDScalableRichText*>(L"name", settingsview));

            GTRANS_DESC transDesc[3];
            TriggerTranslate(settingslist, transDesc, 0, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 0.0f, 100.0f * g_flScaleFactor, 0.0f, 0.0f, false, false);
            TriggerFade(settingslist, transDesc, 1, 0.2f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
            TriggerClip(settingslist, transDesc, 2, 0.2f, 0.7f, 0.1f, 0.9f, 0.2f, 1.0f, 1.0f, ((settingslist->GetHeight() - 100 * g_flScaleFactor) / settingslist->GetHeight()), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, false, false);
            TransitionStoryboardInfo tsbInfo = {};
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, settingslist->GetDisplayNode(), &tsbInfo);

            g_checkifelemexists = true;
        }
    }

    Element* elemStorage;
    bool fileopened{};

    void SelectItem(Element* elem, Event* iev)
    {
        static int clicks = 1;
        static int validation = 0;
        if (iev->uidType == Button::Click)
        {
            validation++;
            CSafeElementPtr<Button> checkbox;
            checkbox.Assign(regElem<Button*>(L"checkboxElem", elem));
            if (GetAsyncKeyState(VK_CONTROL) == 0 && checkbox->GetMouseFocused() == false)
            {
                for (int items = 0; items < pm.size(); items++)
                {
                    pm[items]->SetSelected(false);
                    if (cbpm[items]->GetSelected() == false && g_showcheckboxes == 1) cbpm[items]->SetVisible(false);
                }
            }
            if (elem != emptyspace && checkbox->GetMouseFocused() == false && GetAsyncKeyState(VK_CONTROL) == 0) elem->SetSelected(!elem->GetSelected());
            if (validation & 1)
            {
                if (elem != emptyspace && checkbox->GetMouseFocused() == true && GetAsyncKeyState(VK_CONTROL) == 0) elem->SetSelected(!elem->GetSelected());
            }
            if (g_showcheckboxes == 1) checkbox->SetVisible(true);
            if (shellstate[4] & 0x20 && !g_touchmode)
            {
                if (elem == elemStorage) clicks++;
                else clicks = 0;
                DWORD doubleClickThread{};
                HANDLE doubleClickThreadHandle = CreateThread(nullptr, 0, MultiClickHandler, &clicks, 0, &doubleClickThread);
                if (doubleClickThreadHandle) CloseHandle(doubleClickThreadHandle);
                elemStorage = elem;
            }
            if (elem != emptyspace && clicks & 1 && checkbox->GetMouseFocused() == false && ((LVItem*)elem)->GetDragState() == false)
            {
                wstring temp = RemoveQuotes(((LVItem*)elem)->GetFilename());
                SHELLEXECUTEINFOW execInfo = {};
                execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
                execInfo.lpVerb = L"open";
                execInfo.nShow = SW_SHOWNORMAL;
                execInfo.lpFile = temp.c_str();
                fileopened = true;
                if (((LVItem*)elem)->GetGroupedDirState() == true && g_treatdirasgroup == true)
                {
                    ShowDirAsGroup((LVItem*)elem);
                }
                else ShellExecuteExW(&execInfo);
            }
        }
    }

    void SelectItem2(Element* elem, Event* iev)
    {
        static int validation = 0;
        if (iev->uidType == Button::Click)
        {
            elem->SetSelected(!elem->GetSelected());
            if (validation & 1)
            {
                elem->SetSelected(!elem->GetSelected());
            }
        }
    }

    void SelectItemListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::SelectedProp())
        {
            if (!g_touchmode)
            {
                float spacingInternal = CalcTextLines(((LVItem*)elem)->GetSimpleFilename().c_str(), elem->GetWidth() - 4 * g_flScaleFactor);
                int extraBottomSpacing = (elem->GetSelected() == true) ? ceil(spacingInternal) * textm.tmHeight : floor(spacingInternal) * textm.tmHeight;
                CSafeElementPtr<RichText> textElem;
                textElem.Assign(regElem<RichText*>(L"textElem", elem));
                CSafeElementPtr<RichText> textElemShadow;
                textElemShadow.Assign(regElem<RichText*>(L"textElemShadow", elem));
                if (type == 69)
                {
                    int innerSizeX = GetSystemMetricsForDpi(SM_CXICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor;
                    int innerSizeY = GetSystemMetricsForDpi(SM_CYICONSPACING, g_dpi) + (g_iconsz - 48) * g_flScaleFactor - textm.tmHeight;
                    int lines_basedOnEllipsis = ceil(CalcTextLines(((LVItem*)elem)->GetSimpleFilename().c_str(), innerSizeX - 4 * g_flScaleFactor)) * textm.tmHeight;
                    elem->SetHeight(innerSizeY + lines_basedOnEllipsis + 6 * g_flScaleFactor);
                    CSafeElementPtr<RichText> g_textElem;
                    g_textElem.Assign(regElem<RichText*>(L"textElem", g_outerElem));
                    CSafeElementPtr<RichText> g_textElemShadow;
                    g_textElemShadow.Assign(regElem<RichText*>(L"textElemShadow", g_outerElem));
                    textElem->SetLayoutPos(g_textElem->GetLayoutPos());
                    textElemShadow->SetLayoutPos(g_textElemShadow->GetLayoutPos());
                    textElem->SetHeight(lines_basedOnEllipsis + 4 * g_flScaleFactor);
                    textElemShadow->SetHeight(lines_basedOnEllipsis + 5 * g_flScaleFactor);
                }
                if (spacingInternal == 1.5)
                {
                    if (elem->GetSelected() == true) elem->SetHeight(elem->GetHeight() + extraBottomSpacing * 0.5);
                    else elem->SetHeight(elem->GetHeight() - extraBottomSpacing);
                }
                textElem->SetHeight(extraBottomSpacing + 4 * g_flScaleFactor);
                textElemShadow->SetHeight(extraBottomSpacing + 5 * g_flScaleFactor);
                HBITMAP capturedBitmap{};
                CreateTextBitmap(capturedBitmap, ((LVItem*)elem)->GetSimpleFilename().c_str(), elem->GetWidth() - 4 * g_flScaleFactor, extraBottomSpacing, DT_CENTER | DT_END_ELLIPSIS, false);
                IterateBitmap(capturedBitmap, DesaturateWhiten, 1, 0, 1.33, NULL);
                HBITMAP shadowBitmap{};
                AddPaddingToBitmap(capturedBitmap, shadowBitmap, 2 * g_flScaleFactor, 2 * g_flScaleFactor, 2 * g_flScaleFactor, 2 * g_flScaleFactor);
                IterateBitmap(shadowBitmap, SimpleBitmapPixelHandler, 0, (int)(2 * g_flScaleFactor), 2, NULL);
                CValuePtr spvBitmap = DirectUI::Value::CreateGraphic(capturedBitmap, 2, 0xffffffff, false, false, false);
                CValuePtr spvBitmapSh = DirectUI::Value::CreateGraphic(shadowBitmap, 2, 0xffffffff, false, false, false);
                if (spvBitmap != nullptr) textElem->SetValue(Element::ContentProp, 1, spvBitmap);
                if (spvBitmapSh != nullptr) textElemShadow->SetValue(Element::ContentProp, 1, spvBitmapSh);
                DeleteObject(capturedBitmap);
                DeleteObject(shadowBitmap);
            }
            else if (type == 69)
            {
                CSafeElementPtr<RichText> textElem;
                textElem.Assign(regElem<RichText*>(L"textElem", elem));
                CSafeElementPtr<RichText> g_textElem;
                g_textElem.Assign(regElem<RichText*>(L"textElem", g_outerElem));
                textElem->SetLayoutPos(g_textElem->GetLayoutPos());
            }
        }
        if (g_touchmode && pProp == Button::PressedProp())
        {
            CSafeElementPtr<Element> innerElem;
            innerElem.Assign(regElem<Element*>(L"innerElem", elem));
            innerElem->SetEnabled(!((LVItem*)elem)->GetPressed());
        }
    }

    void ShowCheckboxIfNeeded(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        CSafeElementPtr<Button> checkboxElem;
        checkboxElem.Assign(regElem<Button*>(L"checkboxElem", (LVItem*)elem));
        if (pProp == Element::MouseWithinProp() && g_showcheckboxes == 1 && checkboxElem)
        {
            checkboxElem->SetVisible(elem->GetMouseWithin() || elem->GetSelected());
        }
    }

    void CheckboxHandler(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == Element::MouseFocusedProp())
        {
            UpdateCache* uc{};
            Element* grandparent = elem->GetParent()->GetParent();
            CValuePtr v = elem->GetValue(Element::MouseFocusedProp, 1, uc);
            CSafeElementPtr<Element> item;
            item.Assign(regElem<Element*>(L"innerElem", grandparent));
            if (item != nullptr) item->SetValue(Element::MouseFocusedProp(), 1, v);
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
        if (pProp == Button::PressedProp())
        {
            if (g_tripleclickandhide == true && ((Button*)elem)->GetPressed() == true)
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
                                TriggerTranslate(pm[items], transDesc, 0, delay, delay + 0.22f, 1.0f, 0.0f, 1.0f, 1.0f, pm[items]->GetX(), pm[items]->GetY(), pm[items]->GetX() + startXPos, pm[items]->GetY() + startYPos, false, false);
                                TriggerFade(pm[items], transDesc, 1, delay + 0.11f, delay + 0.22f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false);
                                TriggerScaleOut(pm[items], transDesc, 2, delay, delay + 0.22f, 1.0f, 0.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, true, false);
                                break;
                            case 1:
                                delay *= 2;
                                pm[items]->SetVisible(true);
                                TriggerTranslate(pm[items], transDesc, 0, delay, delay + 0.44f, 0.1f, 0.9f, 0.2f, 1.0f, pm[items]->GetX() + startXPos, pm[items]->GetY() + startYPos, pm[items]->GetX(), pm[items]->GetY(), false, false);
                                TriggerFade(pm[items], transDesc, 1, delay, delay + 0.15f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                                TriggerScaleIn(pm[items], transDesc, 2, delay, delay + 0.44f, 0.1f, 0.9f, 0.2f, 1.0f, 0.8f, 0.8f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, false, false);
                                break;
                            }
                            TransitionStoryboardInfo tsbInfo = {};
                            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, pm[items]->GetDisplayNode(), &tsbInfo);
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
                TriggerFade(selector, transDesc, 0, 0.0f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, false, false);
                ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, selector->GetDisplayNode(), &tsbInfo);
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
            TriggerFade(selector, transDesc, 0, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, true, false);
            ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, selector->GetDisplayNode(), &tsbInfo);
            isPressed = 0;
        }
    }

    void ItemDragListener(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        DWORD dragThread;
        HANDLE dragThreadHandle;
        POINT ppt;
        if (pProp == Button::CapturedProp())
        {
            if (((Button*)elem)->GetCaptured())
            {
                selectedLVItems.clear();
                selectedLVItems.push_back((LVItem*)elem);
                int selectedItems{};
                if (elem->GetSelected() == false && GetAsyncKeyState(VK_CONTROL) == 0)
                {
                    for (int items = 0; items < pm.size(); items++)
                    {
                        pm[items]->SetSelected(false);
                        if (cbpm[items]->GetSelected() == false && g_showcheckboxes == 1) cbpm[items]->SetVisible(false);
                    }
                }
                elem->SetSelected(true);
                for (int items = 0; items < pm.size(); items++)
                {
                    if (pm[items]->GetSelected() == true)
                    {
                        selectedItems++;
                        if (pm[items] != elem) selectedLVItems.push_back(pm[items]);
                    }
                }
                CSafeElementPtr<Element> multipleitems;
                multipleitems.Assign(regElem<Element*>(L"multipleitems", pMain));
                multipleitems->SetVisible(false);
                if (selectedItems >= 2)
                {
                    multipleitems->SetVisible(true);
                    multipleitems->SetContentString(to_wstring(selectedItems).c_str());
                }
                if (g_showcheckboxes)
                {
                    CSafeElementPtr<Button> checkbox;
                    checkbox.Assign(regElem<Button*>(L"checkboxElem", (LVItem*)elem));
                    checkbox->SetVisible(true);
                }
                fileopened = false;
                if (GetAsyncKeyState(VK_CONTROL) == 0)
                {
                    GetCursorPos(&ppt);
                    ScreenToClient(wnd->GetHWND(), &ppt);
                    RECT dimensions{};
                    GetClientRect(wnd->GetHWND(), &dimensions);
                    if (localeType == 1) origX = dimensions.right - ppt.x - ((LVItem*)elem)->GetMemXPos();
                    else origX = ppt.x - ((LVItem*)elem)->GetMemXPos();
                    origY = ppt.y - ((LVItem*)elem)->GetMemYPos();
                    dragpreview->SetWidth(elem->GetWidth());
                    dragpreview->SetHeight(elem->GetHeight());
                    dragThreadHandle = CreateThread(nullptr, 0, UpdateIconPosition, &selectedLVItems, 0, &dragThread);
                    if (dragThreadHandle) CloseHandle(dragThreadHandle);
                }
            }
            isIconPressed = 1;
        }
    }

    void UpdateIconColorizationColor(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2)
    {
        if (pProp == DDScalableElement::AssociatedColorProp())
        {
            IconColorizationColor = ((DDScalableElement*)elem)->GetAssociatedColor();
            iconColorID = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"IconColorID");
            SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"IconColorizationColor", IconColorizationColor, false, nullptr);
            g_atleastonesetting = true;
            RearrangeIcons(false, true, true);
        }
    }

    void testEventListener3(Element* elem, Event* iev)
    {
        if (iev->uidType == Button::Click)
        {
            switch (g_issubviewopen)
            {
                case false:
                    if (elem != fullscreenpopupbase)
                    {
                        ShowPopupCore();
                    }
                    break;
                case true:
                    if (centered->GetMouseWithin() == false && elem->GetMouseFocused() == true)
                    {
                        HidePopupCore(false);
                    }
                    break;
            }
        }
    }

    DWORD WINAPI SetLVIPos(LPVOID lpParam)
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
            pm[j]->SetMemPage(pm[j]->GetPage());
        }
        RECT dimensions;
        GetClientRect(wnd->GetHWND(), &dimensions);
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
            if (g_touchmode)
            {
                x = (dimensions.right - largestXPos * outerSizeX + desktoppadding) / 2;
                y = (dimensions.bottom - largestYPos * outerSizeY + desktoppadding) / 2;
            }
            vector<bool> positions{};
            positions.resize(g_maxPageID * largestXPos * largestYPos - 1);
            if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 3 of 5 complete: Created an array of positions.");
            for (int j = 0; j < count; j++)
            {
                pm[j]->SetRefreshState(reloadicons);
                pm[j]->SetMoving(animation && pm[j]->GetPage() == g_currentPageID);
                if (pm[j]->GetPage() != g_currentPageID) pm[j]->SetFlying(false);
                if (g_touchmode)
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
                    switch (pm[j]->GetGroupSize())
                    {
                        case LVIGS_SMALL:
                            pm[j]->SetWidth(316 * g_flScaleFactor);
                            pm[j]->SetHeight(200 * g_flScaleFactor);
                            break;
                        case LVIGS_MEDIUM:
                            pm[j]->SetWidth(476 * g_flScaleFactor);
                            pm[j]->SetHeight(300 * g_flScaleFactor);
                            break;
                        case LVIGS_WIDE:
                            pm[j]->SetWidth(716 * g_flScaleFactor);
                            pm[j]->SetHeight(300 * g_flScaleFactor);
                            break;
                        case LVIGS_LARGE:
                            pm[j]->SetWidth(716 * g_flScaleFactor);
                            pm[j]->SetHeight(450 * g_flScaleFactor);
                            break;
                    }
                }
                if (((g_treatdirasgroup && pm[j]->GetGroupSize() != LVIGS_NORMAL) || (g_touchmode && pm[j]->GetTileSize() != LVITS_NONE)) &&
                    pm[j]->GetInternalXPos() < largestXPos - ceil((pm[j]->GetWidth() + desktoppadding) / static_cast<float>(outerSizeX)) &&
                    pm[j]->GetInternalYPos() < largestYPos - ceil((pm[j]->GetHeight() + desktoppadding) / static_cast<float>(outerSizeY)))
                {
                    if (!EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) pm[j]->SetPage(g_maxPageID);
                    int occupiedPos = ((pm[j]->GetPage() - 1) * largestXPos * largestYPos) + pm[j]->GetInternalYPos() + pm[j]->GetInternalXPos() * largestYPos;
                    if (positions[occupiedPos] == true)
                    {
                        pm[j]->SetInternalXPos(65535);
                        pm[j]->SetInternalYPos(65535);
                    }
                    else
                    {
                        int pt = 0;
                        int occupiedHeight = 0;
                        for (int i = 1; i <= ceil((pm[j]->GetWidth() + desktoppadding) / static_cast<float>(outerSizeX)); i++)
                        {
                            if (pm[j]->GetHeight() > outerSizeY)
                            {
                                for (int k = 1; k <= ceil((pm[j]->GetHeight() + desktoppadding) / static_cast<float>(outerSizeY)); k++)
                                {
                                    positions[occupiedPos + pt] = true;
                                    pt++;
                                    occupiedHeight++;
                                }
                            }
                            pt += (largestYPos - occupiedHeight);
                            occupiedHeight = 0;
                        }
                        int widthForRender = (!g_touchmode && (!g_treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL)) ? innerSizeX : pm[j]->GetWidth();
                        int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - widthForRender - x : pm[j]->GetInternalXPos() * outerSizeX + x;
                        if (!pm[j]->GetMoving() || pm[j]->GetMemPage() != pm[j]->GetPage() || pm[j]->GetSizedFromGroup())
                        {
                            pm[j]->SetX(xRender);
                            pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
                        }
                        pm[j]->SetMemXPos(xRender);
                        pm[j]->SetMemYPos(pm[j]->GetInternalYPos() * outerSizeY + y);
                    }
                }
            }
            for (int j = 0; j < count; j++)
            {
                if (pm[j]->GetGroupSize() == LVIGS_NORMAL && pm[j]->GetInternalXPos() < largestXPos && pm[j]->GetInternalYPos() < largestYPos)
                {
                    int widthForRender = (!g_touchmode && (!g_treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL)) ? innerSizeX : pm[j]->GetWidth();
                    int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - widthForRender - x : pm[j]->GetInternalXPos() * outerSizeX + x;
                    if (!EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize)) pm[j]->SetPage(g_maxPageID);
                    int occupiedPos = ((pm[j]->GetPage() - 1) * largestXPos * largestYPos) + pm[j]->GetInternalYPos() + pm[j]->GetInternalXPos() * largestYPos;
                    if (positions[occupiedPos] == true)
                    {
                        pm[j]->SetInternalXPos(65535);
                        pm[j]->SetInternalYPos(65535);
                    }
                    else
                    {
                        if (!pm[j]->GetMoving() || pm[j]->GetMemPage() != pm[j]->GetPage() || pm[j]->GetSizedFromGroup())
                        {
                            pm[j]->SetX(xRender);
                            pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
                        }
                        pm[j]->SetMemXPos(xRender);
                        pm[j]->SetMemYPos(pm[j]->GetInternalYPos() * outerSizeY + y);
                        positions[occupiedPos] = true;
                    }
                }
            }
            if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 4 of 5 complete: Assigned positions to items that are in your resolution's bounds.");
            bool forcenewpage{};
            for (int j = 0; j < count; j++)
            {
                int modifierX = 0;
                int modifierY = 0;
                if (pm[j]->GetGroupSize() != LVIGS_NORMAL || pm[j]->GetTileSize() != LVITS_NONE)
                {
                    modifierX = (pm[j]->GetWidth() - outerSizeX + desktoppadding) / outerSizeX;
                    modifierY = (pm[j]->GetHeight() - outerSizeY + desktoppadding) / outerSizeY;
                }
                if (pm[j]->GetInternalXPos() >= largestXPos - modifierX ||
                    pm[j]->GetInternalYPos() >= largestYPos - modifierY)
                {
                    int y{};
                    while (positions[y] == true)
                    {
                        y++;
                        if (y > positions.size())
                        {
                            y = 0;
                            positions.resize((g_maxPageID + 1) * largestXPos * largestYPos - 1);
                            for (int p = 0; p <= g_maxPageID * largestXPos * largestYPos; p++)
                            {
                                positions[p] = true;
                            }
                            for (int p = g_maxPageID * largestXPos * largestYPos + 1; p <= positions.size(); p++)
                            {
                                positions[p] = false;
                            }
                            g_maxPageID++;
                            forcenewpage = true;
                            break;
                        }
                    }
                    int pageID = 1;
                    int y2 = y;
                    while (y2 >= largestXPos * largestYPos)
                    {
                        y2 -= largestXPos * largestYPos;
                        pageID++;
                    }
                    int xRenderPos = y2 / largestYPos;
                    if (xRenderPos == largestXPos) pm[j]->SetInternalXPos(0);
                    else pm[j]->SetInternalXPos(xRenderPos);
                    pm[j]->SetInternalYPos(y % largestYPos);
                    if (EnsureRegValueExists(HKEY_CURRENT_USER, L"Software\\DirectDesktop", DesktopLayoutWithSize) && !forcenewpage)
                    {
                        pm[j]->SetPage(pageID);
                    }
                    else pm[j]->SetPage(g_maxPageID);
                    positions[y] = true;
                    if ((pm[j]->GetGroupSize() != LVIGS_NORMAL || pm[j]->GetTileSize() != LVITS_NONE) && pm[j]->GetWidth() > outerSizeX)
                    {
                        int pt = 0;
                        int occupiedHeight = 0;
                        for (int i = 1; i <= ceil((pm[j]->GetWidth() + desktoppadding) / static_cast<float>(outerSizeX)); i++)
                        {
                            if (pm[j]->GetHeight() > outerSizeY)
                            {
                                for (int k = 1; k <= ceil((pm[j]->GetHeight() + desktoppadding) / static_cast<float>(outerSizeY)); k++)
                                {
                                    positions[y + pt] = true;
                                    pt++;
                                    occupiedHeight++;
                                }
                            }
                            pt += (largestYPos - occupiedHeight);
                            occupiedHeight = 0;
                        }
                    }
                }
                int widthForRender = (!g_touchmode && (!g_treatdirasgroup || pm[j]->GetGroupSize() == LVIGS_NORMAL)) ? innerSizeX : pm[j]->GetWidth();
                int xRender = (localeType == 1) ? dimensions.right - (pm[j]->GetInternalXPos() * outerSizeX) - widthForRender - x : pm[j]->GetInternalXPos() * outerSizeX + x;
                if (!pm[j]->GetMoving() || pm[j]->GetMemPage() != pm[j]->GetPage() || pm[j]->GetSizedFromGroup())
                {
                    pm[j]->SetX(xRender);
                    pm[j]->SetY(pm[j]->GetInternalYPos() * outerSizeY + y);
                }
                pm[j]->SetMemXPos(xRender);
                pm[j]->SetMemYPos(pm[j]->GetInternalYPos() * outerSizeY + y);
                if (pm[j]->GetMemPage() == pm[j]->GetPage() == g_currentPageID && !pm[j]->GetFlying()) pm[j]->SetVisible(!g_hiddenIcons);
                else pm[j]->SetVisible(false);
            }

            if (g_maxPageID > 1 && g_currentPageID < g_maxPageID) nextpageMain->SetVisible(true);
            if (g_currentPageID != 1) prevpageMain->SetVisible(true);

            for (int j = 0; j < count; j++)
            {
                yValue* yV = new yValue{ j, (float)innerSizeX, (float)innerSizeY };
                QueueUserWorkItem(RearrangeIconsHelper, yV, 0);
            }
            positions.clear();
            for (int j = 0; j < count; j++)
            {
                if (pm[j]->GetMemPage() == 0)
                    pm[j]->SetMemPage(pm[j]->GetPage());
            }
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Icon arrangement: 5 of 5 complete: Successfully arranged the desktop items.");
        SetPos(isDefaultRes());
        g_lastWidth = dimensions.right;
        g_lastHeight = dimensions.bottom;
    }

    void InitLayout(bool bUnused1, bool bUnused2, bool bAlreadyOpen)
    {
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
        pm.clear();
        iconpm.clear();
        shortpm.clear();
        shadowpm.clear();
        filepm.clear();
        fileshadowpm.clear();
        cbpm.clear();
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
            CSafeElementPtr<DDScalableElement> iconElem;
            iconElem.Assign(regElem<DDScalableElement*>(L"iconElem", outerElem));
            CSafeElementPtr<Element> shortcutElem;
            shortcutElem.Assign(regElem<Element*>(L"shortcutElem", outerElem));
            CSafeElementPtr<Element> iconElemShadow;
            iconElemShadow.Assign(regElem<Element*>(L"iconElemShadow", outerElem));
            CSafeElementPtr<RichText> textElem;
            textElem.Assign(regElem<RichText*>(L"textElem", outerElem));
            CSafeElementPtr<RichText> textElemShadow;
            textElemShadow.Assign(regElem<RichText*>(L"textElemShadow", outerElem));
            CSafeElementPtr<Button> checkboxElem;
            checkboxElem.Assign(regElem<Button*>(L"checkboxElem", outerElem));
            if (g_touchmode) assignExtendedFn(iconElem, UpdateTileOnColorChange);
            pm.push_back(outerElem);
            iconpm.push_back(iconElem);
            shortpm.push_back(shortcutElem);
            shadowpm.push_back(iconElemShadow);
            filepm.push_back(textElem);
            fileshadowpm.push_back(textElemShadow);
            cbpm.push_back(checkboxElem);
            outerElem->SetFlying(true);
        }
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 3 of 6 complete: Created elements, preparing to enumerate desktop folders.");
        EnumerateFolder((LPWSTR)L"InternalCodeForNamespace", &pm, false, nullptr, &count2, lviCount);
        DWORD d = GetEnvironmentVariableW(L"PUBLIC", cBuffer, 260);
        StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        EnumerateFolder(secondaryPath, &pm, false, nullptr, &count2, lviCount - count2);
        path1 = secondaryPath;
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        EnumerateFolder(path, &pm, false, nullptr, &count2, lviCount - count2);
        path2 = path;
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        d = GetEnvironmentVariableW(L"OneDrive", cBuffer, 260);
        StringCchPrintfW(secondaryPath, 260, L"%s\\Desktop", cBuffer);
        EnumerateFolder(secondaryPath, &pm, false, nullptr, &count2, lviCount - count2);
        path3 = secondaryPath;
        if (logging == IDYES) MainLogger.WriteLine(to_wstring(count2).c_str());
        if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialization: 4 of 6 complete: Created arrays according to your desktop items.");
        for (int i = 0; i < lviCount; i++)
        {
            if (pm[i]->GetHiddenState() == true)
            {
                iconpm[i]->SetAlpha(128);
                shadowpm[i]->SetAlpha(0);
                filepm[i]->SetAlpha(g_touchmode ? 128 : 192);
                if (!g_touchmode) fileshadowpm[i]->SetAlpha(128);
            }
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
        delete[] cBuffer;
        delete[] secondaryPath;
        free(path);
        free(value);
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
                        PostMessageW(wnd->GetHWND(), WM_USER + 21, (WPARAM)pm[i], i);
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

    DWORD WINAPI FinishedLogging(LPVOID lpParam)
    {
        int logresponse{};
        TaskDialog(nullptr, nullptr, LoadStrFromRes(4024).c_str(), LoadStrFromRes(4019).c_str(), LoadStrFromRes(4020).c_str(), TDCBF_OK_BUTTON | TDCBF_CLOSE_BUTTON, TD_INFORMATION_ICON, &logresponse);
        if (logresponse == IDCLOSE) SendMessageW(wnd->GetHWND(), WM_CLOSE, NULL, 420);
        return 0;
    }

    HWND GetShutdownWindowIfPresent()
    {
        if (shutdownwnd) return shutdownwnd->GetHWND();
        else return nullptr;
    }

    HWND GetEditWindowIfPresent()
    {
        if (editwnd) return editwnd->GetHWND();
        else return nullptr;
    }

    bool IsDesktopActive()
    {
        HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
        HWND hWnd = GetForegroundWindow();
        if (hWnd == nullptr) return false;
        return (hWnd == hWndProgman || hWnd == g_hWorkerW || hWnd == g_hWndTaskbar || hWnd == GetShutdownWindowIfPresent());
    }

    bool IsDesktopOrSubviewActive()
    {
        HWND hWndProgman = FindWindowW(L"Progman", L"Program Manager");
        HWND hWnd = GetForegroundWindow();
        if (hWnd == nullptr) return false;
        return (hWnd == hWndProgman || hWnd == g_hWorkerW || hWnd == g_hWndTaskbar || hWnd == subviewwnd->GetHWND() || hWnd == GetEditWindowIfPresent() || hWnd == GetShutdownWindowIfPresent());
    }

    HHOOK KeyHook = nullptr;
    bool g_dialogopen{};

    LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        static bool keyHold[256]{};
        if (nCode == HC_ACTION)
        {
            KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
            if ((pKeyInfo->vkCode == 'D' || pKeyInfo->vkCode == 'M') && GetAsyncKeyState(VK_LWIN) & 0x8000)
            {
                HidePopupCore(true);
                SetWindowPos(g_hWndTaskbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            if (IsDesktopActive())
            {
                if (pKeyInfo->vkCode == VK_F2)
                {
                    if (!keyHold[pKeyInfo->vkCode] && !g_renameactive && !isIconPressed)
                    {
                        ShowRename();
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
                if ((pKeyInfo->vkCode == VK_F4) && GetAsyncKeyState(VK_MENU) & 0x8000)
                {
                    static bool valid{};
                    valid = !valid;
                    if (valid) SetTimer(wnd->GetHWND(), 4, 100, nullptr);
                    return 1;
                }
                if (pKeyInfo->vkCode == VK_F5)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        SetTimer(wnd->GetHWND(), 2, 150, nullptr);
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
                if ((pKeyInfo->vkCode == VK_LEFT || pKeyInfo->vkCode == VK_RIGHT) && GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        Event* iev = new Event{ nullptr, TouchButton::Click };
                        RECT dimensions{};
                        GetClientRect(wnd->GetHWND(), &dimensions);
                        switch (pKeyInfo->vkCode)
                        {
                        case VK_LEFT:
                            if (localeType == 1 && g_currentPageID < g_maxPageID) GoToNextPage(nextpageMain, iev);
                            else if (localeType != 1 && g_currentPageID > 1) GoToPrevPage(prevpageMain, iev);
                            else SetTimer(wnd->GetHWND(), 6, 60, nullptr);
                            break;
                        case VK_RIGHT:
                            if (localeType == 1 && g_currentPageID > 1) GoToPrevPage(prevpageMain, iev);
                            else if (localeType != 1 && g_currentPageID < g_maxPageID) GoToNextPage(nextpageMain, iev);
                            else SetTimer(wnd->GetHWND(), 6, 60, nullptr);
                            break;
                        }
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
            }
            if (IsDesktopOrSubviewActive())
            {
                if (pKeyInfo->vkCode == 'E' && GetAsyncKeyState(VK_LWIN) & 0x8000 && GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        SetTimer(wnd->GetHWND(), 1, 400, nullptr);
                        keyHold[pKeyInfo->vkCode] = true;
                    }
                }
                if (pKeyInfo->vkCode == 'S' && GetAsyncKeyState(VK_LWIN) & 0x8000 && GetAsyncKeyState(VK_MENU) & 0x8000)
                {
                    if (!keyHold[pKeyInfo->vkCode])
                    {
                        SetTimer(wnd->GetHWND(), 3, 150, nullptr);
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
        NativeHWNDHost::Create(L"DD_MessageCallback", L"", nullptr, nullptr, 0, 0, 0, 0, NULL, NULL, nullptr, 0x43, &msgwnd);
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
    int WindowsBuild = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\ShellCommon", L"BuildNumber");
    int WindowsBuildFallback = GetRegistryValues(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\BuildLayers\\DesktopEditions", L"BuildNumber");
    if (WindowsBuild < 18362 && WindowsBuildFallback < 18362)
    {
        WCHAR currentBuild[260];
        StringCchPrintfW(currentBuild, 260, L"Windows 10 version 1903 (OS Build 18362) or higher is required.\nCurrent build: %d", max(WindowsBuild, WindowsBuildFallback));
        TaskDialog(nullptr, HINST_THISCOMPONENT, L"DirectDesktop", L"Unsupported Windows version",
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

    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)nullptr, nullptr);

    s_InitializeDUI();
    g_msgwnd = InitializeCallbackWindow();
    RegisterAllControls();
    DDScalableElement::Register();
    DDScalableButton::Register();
    DDScalableRichText::Register();
    DDScalableTouchEdit::Register();
    LVItem::Register();
    DDLVActionButton::Register();
    DDToggleButton::Register();
    DDCheckBox::Register();
    DDCheckBoxGlyph::Register();
    DDColorPicker::Register();
    DDColorPickerButton::Register();
    DDNotificationBanner::Register();
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WCHAR localeName[256]{};
    ULONG numLanguages{};
    ULONG bufferSize = sizeof(localeName) / sizeof(WCHAR);
    GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, localeName, &bufferSize);
    GetLocaleInfoEx(localeName, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER, (LPWSTR)&localeType, sizeof(localeType) / sizeof(WCHAR));
    RECT dimensions;
    SystemParametersInfoW(SPI_GETWORKAREA, sizeof(dimensions), &dimensions, NULL);
    int windowsThemeX = (GetSystemMetricsForDpi(SM_CXSIZEFRAME, g_dpi) + GetSystemMetricsForDpi(SM_CXEDGE, g_dpi) * 2) * 2;
    int windowsThemeY = (GetSystemMetricsForDpi(SM_CYSIZEFRAME, g_dpi) + GetSystemMetricsForDpi(SM_CYEDGE, g_dpi) * 2) * 2 + GetSystemMetricsForDpi(SM_CYCAPTION, g_dpi);
    bool checklog{};
    SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging", 0, true, &checklog);
    if (checklog)
    {
        TaskDialog(nullptr, HINST_THISCOMPONENT, L"DirectDesktop", LoadStrFromRes(4017).c_str(),
                   LoadStrFromRes(4018).c_str(), TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &logging);
        SetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging", logging, false, nullptr);
    }
    else logging = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\DirectDesktop", L"Logging");
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
        SendMessageTimeoutW(hWndProgman, 0x052C, 0, 0, SMTO_NORMAL, 250, nullptr);
        Sleep(250);
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
    NativeHWNDHost::Create(L"DD_DesktopHost", L"DirectDesktop", nullptr, nullptr, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, NULL, NULL, nullptr, 0x43, &wnd);
    NativeHWNDHost::Create(L"DD_SubviewHost", L"DirectDesktop Subview", nullptr, nullptr, dimensions.left, dimensions.top, dimensions.right, dimensions.bottom, WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, WS_POPUP, nullptr, 0x43, &subviewwnd);
    DUIXmlParser::Create(&parser, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
    DUIXmlParser::Create(&parserSubview, nullptr, nullptr, DUI_ParserErrorCB, nullptr);
    parser->SetXMLFromResourceWithTheme(IDR_UIFILE2, hInstance, HINST_THISCOMPONENT, g_hModTWinUI);
    parserSubview->SetXMLFromResourceWithTheme(IDR_UIFILE3, hInstance, HINST_THISCOMPONENT, g_hModTWinUI);
    HWNDElement::Create(wnd->GetHWND(), true, 0x38, nullptr, &key, (Element**)&parent);
    HWNDElement::Create(subviewwnd->GetHWND(), true, 0x38, nullptr, &key2, (Element**)&subviewparent);
    WTSRegisterSessionNotification(wnd->GetHWND(), NOTIFY_FOR_THIS_SESSION);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_STYLE, 0x56003A40L);
    SetWindowLongPtrW(wnd->GetHWND(), GWL_EXSTYLE, 0xC0280800L);
    WndProc = (WNDPROC)SetWindowLongPtrW(wnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)SubclassWindowProc);
    WndProcSubview = (WNDPROC)SetWindowLongPtrW(subviewwnd->GetHWND(), GWLP_WNDPROC, (LONG_PTR)TopLevelWindowProc);
    HWND dummyHWnd{};
    if (WindowsBuild >= 26002)
    {
        dummyHWnd = SetParent(wnd->GetHWND(), hWndProgman);
    }
    else dummyHWnd = SetParent(wnd->GetHWND(), g_hSHELLDLL_DefView);
    if (logging == IDYES)
    {
        if (dummyHWnd != nullptr) MainLogger.WriteLine(L"Information: DirectDesktop is now a part of Explorer.");
        else MainLogger.WriteLine(L"Error: DirectDesktop is still hosted in its own window.");
    }

    parser->CreateElement(L"main", parent, nullptr, nullptr, &pMain);
    pMain->SetVisible(true);
    pMain->EndDefer(key);
    parserSubview->CreateElement(L"fullscreenpopup", subviewparent, nullptr, nullptr, &pSubview);
    pSubview->SetVisible(true);
    pSubview->EndDefer(key2);

    LVItem* outerElemTouch;
    parser->CreateElement(L"outerElemTouch", nullptr, nullptr, nullptr, (Element**)&outerElemTouch);
    g_touchSizeX = outerElemTouch->GetWidth();
    g_touchSizeY = outerElemTouch->GetHeight();

    InitialUpdateScale();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated scaling.");
    UpdateModeInfo();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Updated color mode information.");
    SetTheme();
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Set the theme successfully.");

    sampleText = regElem<Element*>(L"sampleText", pMain);
    mainContainer = regElem<Element*>(L"mainContainer", pMain);
    UIContainer = regElem<Element*>(L"UIContainer", pMain);
    fullscreenpopupbg = regElem<Element*>(L"fullscreenpopupbg", pSubview);
    fullscreenpopupbase = regElem<Button*>(L"fullscreenpopupbase", pSubview);
    popupcontainer = regElem<Button*>(L"popupcontainer", pSubview);
    centered = regElem<Button*>(L"centered", pSubview);
    selector = regElem<Element*>(L"selector", pMain);
    prevpageMain = regElem<TouchButton*>(L"prevpageMain", pMain);
    nextpageMain = regElem<TouchButton*>(L"nextpageMain", pMain);
    dragpreview = regElem<Element*>(L"dragpreview", pMain);

    assignFn(fullscreenpopupbase, testEventListener3);
    assignFn(prevpageMain, GoToPrevPage);
    assignFn(nextpageMain, GoToNextPage);
    assignExtendedFn(prevpageMain, ShowPageToggle);
    assignExtendedFn(nextpageMain, ShowPageToggle);
    GTRANS_DESC transDesc[1];
    TriggerScaleOut(prevpageMain, transDesc, 0, 0.0f, 0.01f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.5f, false, false);
    TransitionStoryboardInfo tsbInfo = {};
    ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc), transDesc, prevpageMain->GetDisplayNode(), &tsbInfo);
    GTRANS_DESC transDesc2[1];
    TriggerScaleOut(nextpageMain, transDesc2, 0, 0.0f, 0.01f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f, false, false);
    TransitionStoryboardInfo tsbInfo2 = {};
    ScheduleGadgetTransitions(0, ARRAYSIZE(transDesc2), transDesc2, nextpageMain->GetDisplayNode(), &tsbInfo2);

    AdjustWindowSizes(true);
    g_showcheckboxes = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"AutoCheckSelect");
    g_hiddenIcons = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideIcons");
    g_iconsz = GetRegistryValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop", L"IconSize");
    GetRegistryBinValues(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"ShellState", &shellstate);

    RegKeyValue DDKey = { HKEY_CURRENT_USER, L"Software\\DirectDesktop", nullptr, NULL };
    if (!EnsureRegValueExists(DDKey._hKeyName, DDKey._path, L"DefaultWidth"))
    {
        g_defWidth = dimensions.right / g_flScaleFactor;
        SetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultWidth", g_defWidth, false, nullptr);
    }
    else g_defWidth = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultWidth");
    if (!EnsureRegValueExists(DDKey._hKeyName, DDKey._path, L"DefaultHeight"))
    {
        g_defHeight = dimensions.bottom / g_flScaleFactor;
        SetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultHeight", g_defHeight, false, nullptr);
    }
    else g_defHeight = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"DefaultHeight");
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"TreatDirAsGroup", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"TripleClickAndHide", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"LockIconPos", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"AccentColorIcons", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"DarkIcons", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"AutoDarkIcons", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"GlassIcons", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"TouchView", 0, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorID", 1, true, nullptr);
    SetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorizationColor", 0, true, nullptr);
    g_treatdirasgroup = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"TreatDirAsGroup");
    g_tripleclickandhide = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"TripleClickAndHide");
    g_lockiconpos = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"LockIconPos");
    g_isColorized = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"AccentColorIcons");
    g_isDarkIconsEnabled = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"DarkIcons");
    g_automaticDark = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"AutoDarkIcons");
    g_isGlass = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"GlassIcons");
    g_touchmode = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"TouchView");
    iconColorID = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorID");
    IconColorizationColor = GetRegistryValues(DDKey._hKeyName, DDKey._path, L"IconColorizationColor");

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
    else if (g_iconsz > 80) g_gpiconsz = 32;
    else if (g_iconsz > 40) g_gpiconsz = 16;
    InitLayout(false, false, false);

    StartMonitorFileChanges(path1);
    StartMonitorFileChanges(path2);
    StartMonitorFileChanges(path3);

    DDNotificationBanner* ddnb{};
    DDNotificationBanner::CreateBanner(ddnb, parser, DDNT_WARNING, L"DDNB", L"DirectDesktop - 0.5 M9",
                                       L"This is a prerelease version of DirectDesktop not intended for public use. It may be unstable or crash.\n\nVersion 0.5_milestone9\nCompiled on 2025-08-01",
                                       10, false);

    if (logging == IDYES) MainLogger.WriteLine(L"Information: Initialized layout successfully.");

    wnd->Host(pMain);
    subviewwnd->Host(pSubview);
    wnd->ShowWindow(SW_SHOW);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been created and shown.");
    MARGINS m = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(wnd->GetHWND(), &m);
    DwmExtendFrameIntoClientArea(subviewwnd->GetHWND(), &m);
    if (logging == IDYES) MainLogger.WriteLine(L"Information: Window has been made transparent.\n\nLogging is now complete.");
    if (logging == IDYES)
    {
        DWORD dd;
        HANDLE loggingThread = CreateThread(nullptr, 0, FinishedLogging, nullptr, 0, &dd);
        if (loggingThread) CloseHandle(loggingThread);
    }
    logging = IDNO;
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