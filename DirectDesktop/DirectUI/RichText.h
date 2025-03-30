#pragma once

#include <dwrite.h>

struct RichTextCacheMetrics
{
	UINT sizeBytes;
	UINT glyphRunCount;
	UINT entryCount;
};

typedef void (CALLBACK *RichTextCacheEnumerateCallback)(DWORD, void*);

MIDL_INTERFACE("a483cc92-093b-4e44-b39b-3fc767e2ffc6")
IDUIRichTextCache : IUnknown
{
	virtual void STDMETHODCALLTYPE CalculateMetrics(RichTextCacheMetrics*) = 0;
	virtual HRESULT STDMETHODCALLTYPE Clear() = 0;
	virtual void STDMETHODCALLTYPE Enumerate(RichTextCacheEnumerateCallback, void*) = 0;
	virtual HRESULT STDMETHODCALLTYPE Remove(DWORD) = 0;
};

namespace DirectUI
{
	class RichTextCache;
	class RichTextShared;
	class GdiTextRenderer;

	enum StringRunStyles
	{
		RUN_FONTCOLOR = 0,
		RUN_FONTSIZE = 1,
		RUN_FONTWEIGHT = 2,
		RUN_TYPOGRAPHY = 3,
	};

	class RichText : public Element
	{
	public:
		UILIB_API static IClassInfo* WINAPI GetClassInfoPtr();
		static void WINAPI SetClassInfoPtr(IClassInfo* pClass);

	private:
		static IClassInfo* s_pClassInfo;

	public:
		UILIB_API IClassInfo* GetClassInfoW() override;

		UILIB_API RichText();
		RichText(const RichText&) = default;

		UILIB_API ~RichText() override;

		UILIB_API static HRESULT WINAPI Register();
		UILIB_API static HRESULT WINAPI Create(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement);

		UILIB_API HRESULT Initialize(Element* pParent, DWORD* pdwDeferCookie);

		UILIB_API void OnPropertyChanged(const PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) override;
		UILIB_API void OnEvent(Event* pEvent) override;
		UILIB_API void OnHosted(Element* peNew) override;
		UILIB_API void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent) override;
		UILIB_API SIZE GetContentSize(int pxConstW, int pxConstH, Surface* psrf) override;

		UILIB_API static const PropertyInfo* WINAPI FontColorRunsProp();
		UILIB_API static const PropertyInfo* WINAPI FontSizeRunsProp();
		UILIB_API static const PropertyInfo* WINAPI FontWeightRunsProp();
		UILIB_API static const PropertyInfo* WINAPI OverhangOffsetProp();
		UILIB_API static const PropertyInfo* WINAPI TypographyProp();
		UILIB_API static const PropertyInfo* WINAPI TypographyRunsProp();
		UILIB_API static const PropertyInfo* WINAPI LocaleProp();
		UILIB_API static const PropertyInfo* WINAPI BaselineProp();
		UILIB_API static const PropertyInfo* WINAPI LineSpacingProp();
		UILIB_API static const PropertyInfo* WINAPI AliasedRenderingProp();
		UILIB_API static const PropertyInfo* WINAPI DisableAccTextExtendProp();
		UILIB_API static const PropertyInfo* WINAPI MapRunsToClustersProp();
		UILIB_API static const PropertyInfo* WINAPI VerticalScriptProp();
		UILIB_API static const PropertyInfo* WINAPI ConstrainLayoutProp();
		UILIB_API static const PropertyInfo* WINAPI ColorFontPaletteIndexProp();

		const WCHAR* GetFontColorRuns(Value** ppv);
		const WCHAR* GetFontSizeRuns(Value** ppv);
		const WCHAR* GetFontWeightRuns(Value** ppv);
		const WCHAR* GetTypography(Value** ppv);
		const WCHAR* GetTypographyRuns(Value** ppv);
		const WCHAR* GetLocale(Value** ppv);

		int GetBaseline() { return GetPixelHelper(this, BaselineProp()); }
		int GetLineSpacing() { return GetPixelHelper(this, LineSpacingProp()); }
		int GetOverhangOffset() { return GetPixelHelper(this, OverhangOffsetProp()); }
		bool GetAliasedRendering();
		bool GetDisableAccTextExtend();
		bool GetMapRunsToClusters();
		bool GetVerticalScript();
		int GetConstrainLayout();
		int GetColorFontPaletteIndex();

		UILIB_API HRESULT SetFontColorRuns(const WCHAR* v);
		UILIB_API HRESULT SetFontSizeRuns(const WCHAR* v);
		UILIB_API HRESULT SetFontWeightRuns(const WCHAR* v);
		UILIB_API HRESULT SetTypography(const WCHAR* v);
		UILIB_API HRESULT SetTypographyRuns(const WCHAR* v);
		UILIB_API HRESULT SetLocale(const WCHAR* v);
		UILIB_API HRESULT SetBaseline(int v);
		UILIB_API HRESULT SetLineSpacing(int v);
		UILIB_API HRESULT SetOverhangOffset(int v);
		UILIB_API HRESULT SetAliasedRendering(bool v);
		UILIB_API HRESULT SetDisableAccTextExtend(bool v);
		UILIB_API HRESULT SetMapRunsToClusters(bool v);
		UILIB_API HRESULT SetVerticalScript(bool v);
		UILIB_API HRESULT SetConstrainLayout(int v);
		UILIB_API HRESULT SetColorFontPaletteIndex(int v);

		UILIB_API virtual HRESULT GetForegroundColorRef(COLORREF* pclrForeground);

		// ReSharper disable once CppHidingFunction
		UILIB_API WCHAR GetShortcutChar();
		UILIB_API void SetDWriteTextLayout(IDWriteTextLayout* pdwTextLayout);
		UILIB_API DWORD GetTrimmedLineCount();
		UILIB_API DWORD GetLineCount();
		UILIB_API IDWriteFactory* GetFactory();
		UILIB_API void SetDWriteFontCollection(IDWriteFontCollection* pdwFontCollection);
		UILIB_API void SetMaxLineCount(UINT cLinesMax);
		UILIB_API static HRESULT WINAPI CreateCache(UINT cAverageNumberOfEntries, IDUIRichTextCache** ppCache);
		UILIB_API void SetCache(DWORD dwKey, IDUIRichTextCache* pCache);
		UILIB_API void StopUsingCache();

	private:
		HRESULT _SetRangedStringRunsWithValue(const WCHAR* pcszString, StringRunStyles iRunStyle);
		void _AdjustRangeForPathJoinCharacters(DWRITE_TEXT_RANGE* prange);
		void _UpdateRangeForClusterMetrics(DWRITE_TEXT_RANGE* prange);
		HRESULT _SetFontColorRun(const WCHAR* pcszString, DWRITE_TEXT_RANGE range);
		HRESULT _SetFontSizeRun(const WCHAR* pcszString, DWRITE_TEXT_RANGE range);
		HRESULT _SetFontStretchRun(const WCHAR* pcszString, DWRITE_TEXT_RANGE range);
		HRESULT _SetFontWeightRun(const WCHAR* pcszString, DWRITE_TEXT_RANGE range);
		HRESULT _SetTypographyRun(const WCHAR* pcszString, DWRITE_TEXT_RANGE range);
		HRESULT _SetInlineImageRun(const WCHAR* pcszString, DWRITE_TEXT_RANGE range);
		HRESULT _SetFontColorRunsInternal();
		HRESULT _SetFontSizeRunsInternal();
		HRESULT _SetFontWeightRunsInternal();
		HRESULT _SetStrikethrough();
		HRESULT _SetTypographyInternal();
		HRESULT _SetTypographyRunsInternal();
		HRESULT _SetUnderline();
		HRESULT _SetLineSpacingInternal();

		int _nFontStyle;
		int _nDrawRectLeadOffset;
		int _fExternalDWriteLayout;
		int _fHasStrikethrough;
		int _fHasUnderline;
		bool _fAcceleratorIsUnderlined;
		bool _fCanReuseOverhangValues;
		UINT _uiShortcutTextPosition;
		WCHAR _chShortcutChar;
		RECT _rcLayoutRect;
		RECT _rcOverhangRect;
		UINT _cLinesMax;
		DWORD _dwCacheKey;
		RichTextCache* _pCache;
		RichTextShared* _pShared;
		IDWriteFontCollection* _pDWFontCollection;
		IDWriteTextFormat* _pDWTextFormat;
		IDWriteTextLayout* _pDWLayout;
		IDWriteInlineObject* _pDWEllipsisInlineObject;
		GdiTextRenderer* _pDWTextRender;

		DWRITE_FONT_WEIGHT _GetDWFontWeight();
		DWRITE_FONT_STYLE _GetDWFontStyle();
		float _GetDWFontSize(float flSize, bool fScaleByPlateau);
		HRESULT _GetDWMetrics(DWRITE_TEXT_METRICS* pMetrics);
		HRESULT _GetDWOverhangMetrics(DWRITE_OVERHANG_METRICS* pMetrics);
		int _GetScaledOverhangOffset();
		int _GetDefaultOverhang();
		void _EnsureLeadDrawOffsetIsSet();
		int _GetMinSizeWidth();
		HRESULT _InitDWrite();
		void _FlushDWrite();
		UINT _GetTextHeight();
		HRESULT _EnsureTextFormat();
		HRESULT _ApplyDrawTextFlags();
		HRESULT _ApplyIntratextFormatting();
		void _SetAcceleratorAccentIfAppropriate();
		HRESULT _CreateDWriteLayout(const RECT* prcContent, Value* pvContent);
		HRESULT _EnsureDWriteLayout(const RECT* prcContent, Value* pvContent);
		const WCHAR* _BuildRenderStringForDWrite(const WCHAR* pszSrc, WCHAR wcShortcut, bool fGluePaths);
		void _PaintStringContentDWrite(HDC hDC, const RECT* prcContent, const RECT* prcInvalid, Value* pvContent);
		void _PrepareTextRender();
	};
}
