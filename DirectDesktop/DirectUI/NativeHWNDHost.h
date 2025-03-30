#pragma once

namespace DirectUI
{
	class UILIB_API NativeHWNDHost
	{
	public:
		static UINT WINAPI DestroyMsg();

		static HRESULT WINAPI Create(const WCHAR* pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, UINT nOptions, NativeHWNDHost** ppHost);
		static HRESULT WINAPI Create(const WCHAR* pszClassName, const WCHAR* pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, HINSTANCE hInstance, UINT nOptions, NativeHWNDHost** ppHost);

		void Destroy();

		HWND GetHWND();
		Element* GetElement();

		void Host(Element* pe);

		void ShowWindow(int iShow);
		void HideWindow();

		void DestroyWindow();
		void SyncDestroyWindow();

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		NativeHWNDHost();
		NativeHWNDHost(const NativeHWNDHost&) = default;

		HRESULT Initialize(const WCHAR* pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, UINT nOptions);
		HRESULT Initialize(const WCHAR* pszClassName, const WCHAR* pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, HINSTANCE hInstance, UINT nOptions);

		virtual ~NativeHWNDHost();

		void SetDefaultFocusID(const WCHAR* pszID);
		void SaveFocus();
		int RestoreFocus();

		virtual HWND CreateHostWindow(DWORD dwExStyle, const WCHAR* lpClassName, const WCHAR* lpWindowName, DWORD dwStyle, int nX, int nY, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
		virtual HRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);

	private:
		HWND _hWnd;
		Element* _pe;
		UINT _nOptions;
		const WCHAR* _pszDefaultFocusID;
		HWND _hwndFocus;
	};
}
