#pragma once
#pragma warning(disable:28159)

using namespace DirectUI;
using namespace DDUI;

#define DESKPADDING_NORMAL 4
#define DESKPADDING_NORMAL_X 4
#define DESKPADDING_NORMAL_Y 4
#define DESKPADDING_TOUCH 8
#define DESKPADDING_TOUCH_X 32
#define DESKPADDING_TOUCH_Y 128

namespace DirectDesktop
{
    extern DDUICtx* g_pctx;
    extern DDUIColors* g_pColors;

    extern int g_touchSizeX, g_touchSizeY;
    extern unsigned short g_defWidth, g_defHeight, g_lastWidth, g_lastHeight;
    extern int g_iconsz, g_shiconsz, g_gpiconsz;
    extern int g_currentPageID, g_maxPageID, g_homePageID;
    extern bool g_treatdirasgroup, g_canRefreshMain;
    extern bool g_overridefilelistener;
    extern bool g_newfolder;
    extern LVGrid* UIContainer;
    extern HANDLE g_hToken;

    struct yValueEx
    {
        int num{};
        float fl1{};
        float fl2{};
        std::vector<LVItem*>* vpm{};
        Element* peOptionalTarget1{};
        Element* peOptionalTarget2{};
    };

    struct DesktopIcon
    {
        HBITMAP icon{};
        HBITMAP iconshortcut{};
        HBITMAP text{};
        COLORREF crDominantTile{};
    };

    //class DDWindowCommon
    //{
    //public:
    //    DDWindowCommon()
    //        : _wnd(nullptr)
    //        , _parent(nullptr)
    //        , _parser(nullptr)
    //        , _peHost(nullptr)
    //    {
    //    }
    //    ~DDWindowCommon();
    //    NativeHWNDHost* GetWindowHost();
    //    HWNDElement* GetHostParentElement();
    //    DUIXmlParser* GetParser();
    //    Element* GetHostElement();

    //protected:
    //    NativeHWNDHost* _wnd;
    //    HWNDElement* _parent;
    //    DUIXmlParser* _parser;
    //    Element* _peHost;
    //    DWORD _key;
    //    virtual HRESULT CreateAndInitWindow()
    //    {
    //    }
    //};

    //class DesktopHost : public DDWindowCommon
    //{
    //public:
    //    DesktopHost()
    //    {
    //    }
    //    ~DesktopHost()
    //    {
    //    }
    //    HRESULT CreateAndInitWindow() override;
    //    Element* GetMainContainer();
    //    Element* GetUIContainer();
    //    TouchButton* GetEmptySpace();
    //    LVItem* GetLVItemTemplate();
    //    Element* GetSelector();
    //    TouchButton* GetPreviousPageButton();
    //    TouchButton* GetNextPageButton();
    //    Element* GetDragPreviewTemplate();
    //    Element* GetRegularDragPreview();
    //    Element* GetTouchDragPreview();
    //private:
    //    Element* mainContainer;
    //    Element* UIContainer;
    //    TouchButton* emptyspace;
    //    LVItem* g_outerElem;
    //    Element* selector;
    //    TouchButton* prevpageMain, *nextpageMain;
    //    Element* g_dragpreview;
    //    Element* dragpreview, *dragpreviewTouch;
    //};

    // Common functions
    EventListener* assignInputFn(Element* elemName, void (*fnName)(Element* elem, InputEvent* ev), bool fReturn = false);
    EventListener* assignExtendedFn2(Element* elemName, bool (*fnName)(Element* elem, const PropertyInfo* pProp, int type, Value* pV1, Value* pV2), bool fReturn = false);
    std::wstring RemoveQuotes(const std::wstring& input);
    extern HRESULT CloakWindow(HWND hwnd, bool fCloak);
    extern float CalcAnimOrigin(float flOriginFrom, float flOriginTo, float flScaleFrom, float flScaleTo);

    extern void LaunchItem(LPCWSTR filename);

    extern bool isDefaultRes();

    DWORD WINAPI fastin(LPVOID lpParam);
}
