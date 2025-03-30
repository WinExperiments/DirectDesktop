#pragma once

namespace DirectUI
{
	class UILIB_API HWNDHost : public ElementWithHWND
	{
	public:
		static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);
		static HRESULT WINAPI Create(UINT nCreate, UINT nActive, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		void OnInput(InputEvent* pInput) override;
		void OnEvent(Event* pEvent) override;
		void OnDestroy() override;
		virtual bool OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);
		virtual bool OnMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);
		virtual bool OnSysChar(WCHAR chKeyCode);
		virtual bool OnSinkThemeChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);
		virtual bool OnCtrlThemeChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);
		virtual void OnWindowStyleChanged(WPARAM wParam, const STYLESTRUCT* pstylestruct);
		virtual int OnAdjustWindowSize(int x, int y, UINT uFlags);
		UINT MessageCallback(GMSG* pmsg) override;
		void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent) override;
		bool GetKeyFocused() override;
		void SetKeyFocus() override;
		virtual void SetWindowDirection(HWND hwnd);
		HWND GetHWND() override;

		HWND GetHWNDParent();
		void Detach();

		static const PropertyInfo* WINAPI TransparentProp();
		static const PropertyInfo* WINAPI BackgroundOwnerIDProp();
		static const PropertyInfo* WINAPI OptimizeMoveProp();
		static const PropertyInfo* WINAPI ThemeChangedProp();

		bool GetTransparent();
		HRESULT SetTransparent(bool v);

		ATOM GetBackgroundOwnerID();
		HRESULT SetBackgroundOwnerID(const WCHAR* v);

		bool GetOptimizeMove();
		HRESULT SetOptimizeMove(bool v);

		static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		IClassInfo* GetClassInfoW() override;
		static HRESULT WINAPI Register();

	private:
		HRESULT GetAccessibleImpl(IAccessible** ppAccessible, bool fWindow);

	public:
		HRESULT GetAccessibleImpl(IAccessible** ppAccessible) override;
		HRESULT GetClientAccessibleImpl(IAccessible** ppAccessible);

		HWNDHost();
		HWNDHost(const HWNDHost&) = default;

		HRESULT Initialize(UINT nCreate, UINT nActive, Element* pParent, DWORD* pdwDeferCookie);

		~HWNDHost() override;

	protected:
		int GetThemeChanged();
		HRESULT SetThemeChanged(int v);

		void OnHosted(Element* peNewHost) override;
		void OnUnHosted(Element* peOldHost) override;
		virtual HWND CreateHWND(HWND hwndParent);

		static void WINAPI AttachCtrlSubclassProc(HWND hwnd);
		static LRESULT CALLBACK CtrlSubclassProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

		HWND CreateAccNameLabel(HWND hwndParent);
		void SyncRect(UINT nChangeFlags, bool bForceSync);
		void SyncParent();
		void SyncStyle();
		void SyncVisible();
		void SyncDirection();
		void SyncFont();
		HFONT GetFont();
		void SyncBackground();
		void SyncForeground();
		void SyncText();
		bool GetStaticColor(HDC hdcCtl, HBRUSH* phbrush);
		void PrintRTLControl(HDC hDC, HDC hdcBuffer, const RECT& rcSkipContent);
		virtual bool EraseBkgnd(HDC hdcCtl, LRESULT* lpRet);
		Element* GetBackgroundOwner();
		bool IsMoveDeferred();
		int VerifyParentage();

	private:
		static BOOL CALLBACK _SinkWndProc(void* pThis, HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);
		static BOOL CALLBACK _CtrlWndProc(void* pThis, HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);

		void _DeleteCtrlWnd();
		void SyncColorsAndFonts();
		// bool VirtualizePosition(const RECT*);
		void UnvirtualizePosition();
		void ApplySinkRegion(const RECT* prcSink, bool bForceSync);
		void GetSinkRect(const RECT* prcConPxl, RECT* prcSink);
		bool HaveWin32Focus();

		static const UINT g_rgMouseMap[8][3];

		bool _fHwndCreate;
		bool _fSpecOptimizeMove;
		bool _fMoveDeferred;
		HWND _hwndCtrl;
		HWND _hwndSink;
		WNDPROC _pfnCtrlOrgProc;
		HWND _hwndAccName;
		int _fCtrlAttached;
		RECT _rcBounds;
		HFONT _hFont;
		COLORREF _crBkColor;
		HBRUSH _hBkBrush;
		COLORREF _crForegroundColor;
		bool _fForegroundColorSet;
		UINT _nCreate;
		int _cAnimate;
		bool _fSentMouseWheel;
		bool _fHandledMouseWheel;
		DuiAccessible* _pClientAccessible;
	};
}
