#pragma once

namespace DirectUI
{
	// Forward declaration of unexported class
	class DUIParsePlayer;
	class DUIParseRecorder;

	typedef HRESULT (CALLBACK *PLAYTCREATE)(int, int*, Value** ppVal);

	enum DynamicScaleParsing
	{
		DSP_None = 0,
		DSP_Enabled = 1
	};

	typedef enum _DUI_PARSE_STATE
	{
		DPS_CalculateStringLength = 0,
		DPS_GlobalAllocStringCopy = 1,
		DPS_GlobalLockStringCopy = 2,
		DPS_StringCopy = 3,
		DPS_CreateXmlReaderFromHGlobal = 4,
		DPS_CreateStreamOnHGlobal = 5,
		DPS_CreateXmlReader = 6,
		DPS_CreateXmlReaderInputWithEncodingName = 7,
		DPS_SetInput = 8,
		DPS_InitializeParserFromXmlLiteReader = 9,
		DPS_CreateBinaryXmlReader = 10,
		DPS_BuildStylesFromBinary = 11,
		DPS_ReadyOnBinarySource = 12
	} DUI_PARSE_STATE;

	struct LINEINFO
	{
		WORD iLine;
		WORD iCol;
	};

	enum LogicalOperators
	{
		PSLO_NULL = 0,
		PSLO_Equal = 1,
		PSLO_NotEqual = 2,
		PSLO_Greater = 3,
		PSLO_GreaterEqual = 4,
		PSLO_Lesser = 5,
		PSLO_LesserEqual = 6
	};

	struct XMLParserCond
	{
		LogicalOperators nLogOp;
		WCHAR* szProp;
		WCHAR* szVal;
	};

	namespace ParserTools
	{
		enum ExprNodeType
		{
			ENT_LITERAL = 0,
			ENT_STRING = 1,
			ENT_FUNCALL = 2,
			ENT_OR = 3
		};

		struct ExprNode
		{
			ExprNodeType ent;
			WCHAR* pszName;
			UINT cArgs;
			ExprNode* pArgs;
			UINT uiArgs;
		};

		enum TokenType
		{
			ttInvalid = 0,
			ttLiteral = 1,
			ttString = 2,
			ttEOI = 3
		};

		class Tokenizer
		{
		public:
			void Initialize(WCHAR*);
			TokenType Next();
			WCHAR PeekChar();
			void SkipChar(WCHAR);
			WCHAR* CurrentName();
			bool IsWhiteSpace(WCHAR);
			bool IsLiteralChar(WCHAR wch);
			void SkipWhiteSpace();

		private:
			WCHAR* _psz;
			WCHAR* _pszLast;
			WCHAR _wchSave;
		};

		class ValueParser : Tokenizer
		{
		public:
			ValueParser();
			~ValueParser();
			static HRESULT WINAPI Create(ValueParser** ppvp);
			void Destroy();
			HRESULT Parse(const WCHAR* pszValue, const ExprNode** ppenRoot);

		private:
			HRESULT Initialize();
			HRESULT ParseWorker();
			HRESULT ParseFuncall();
			HRESULT ParseOr();
			HRESULT FinishComplexNodeT(UINT uiFirstArgT);
			HRESULT AddExprNodeT(ExprNodeType);
			HRESULT InsertExprNodeT(ExprNodeType ent, int iDelta);
			HRESULT TransferNodes(UINT uiStartT);
			void ResetBuf();
			DynamicArray<ExprNode>* _pdaNodes;
			DynamicArray<ExprNode>* _pdaNodesT;
			WCHAR* _pszBuf;
			WCHAR _szBuf[260];
		};


		struct ClassicValue
		{
			const WCHAR* pszClass;
			int iPart;
			int iState;
			int iProp;
			const Value* pvParsed;
			const ExprNode* pen;
		};
	}

	typedef Value* (CALLBACK* PXMLGETSHEETCB)(const WCHAR*, void* pContext);
	typedef void (CALLBACK* PXMLPARSEERRORCB)(const WCHAR* pszError, const WCHAR* pszToken, int dLine, void* pContext);
	typedef bool (CALLBACK* PUNKNOWNATTRCB)(const WCHAR*, void* pContext);

	class UILIB_API DUIXmlParser
	{
	public:
		DUIXmlParser(const DUIXmlParser&);
		DUIXmlParser();
		virtual ~DUIXmlParser();

		static HRESULT WINAPI Create(DUIXmlParser** ppParserOut, PXMLGETSHEETCB pfnGetSheetCallback, void* pGetSheetContext, PXMLPARSEERRORCB pfnErrorCallback, void* pErrorContext);
		void Destroy();

		HRESULT _InitializeTables();
		void _DestroyTables();

		HRESULT SetXML(const WCHAR* pBuffer, HINSTANCE hResourceInstance, HINSTANCE hControlsInstance);
		HRESULT SetPreprocessedXML(const WCHAR* pPreProcessed, HINSTANCE hResourceInstance, HINSTANCE hControlsInstance);
		HRESULT SetXMLFromResourceWithTheme(UINT uRCID, HINSTANCE hResInstance, HINSTANCE hResTheme, HINSTANCE hControlsInstance);
		HRESULT SetXMLFromResource(const WCHAR* pszResource, const WCHAR* pszResType, HINSTANCE hResourceInstance, HINSTANCE hControlsInstance);
		HRESULT SetXMLFromResource(const WCHAR* pszResid, HINSTANCE hResInstance, HINSTANCE hControlsInstance);
		HRESULT SetXMLFromResource(UINT uRCID, const WCHAR* pszResType, HINSTANCE hResInstance, HINSTANCE hControlsInstance);
		HRESULT SetXMLFromResource(UINT uRCID, HINSTANCE hResInstance, HINSTANCE hControlsInstance);
		
		void SetRootWindowForTheming(HWND);
		Value* GetMappedValue(WCHAR*);

		HRESULT CreateElement(const WCHAR* pszResID, Element* pEleSubstitute, Element* pParent, DWORD* pdwDeferCookie, Element** ppElementOut);

		HRESULT GetSheet(const WCHAR* pszResID, Value** ppValueOut);

		void SetParseErrorCallback(PXMLPARSEERRORCB pfnErrorCallback, void* pContext);
		void SetGetSheetCallback(PXMLGETSHEETCB pfnGetSheetCallback, void* pContext);

		PXMLGETSHEETCB GetGetSheetCallback();
		void* GetSheetContext();
		void EnableDesignMode();
		void SetUnknownAttrCallback(PUNKNOWNATTRCB pfnUnknownAttrCallback, void* pContext);
		HINSTANCE GetResourceHInstance();
		HINSTANCE GetHInstance();
		HRESULT LookupElement(LINEINFO li, const WCHAR* pszElem, HINSTANCE hControlsInstance, IClassInfo** ppciOut);
		HRESULT LookupElement(IXmlReader* pReader, const WCHAR* pszElem, HINSTANCE hInstance, IClassInfo** ppciOut);
		HRESULT UpdateSheets(Element* pe);

		void SetDefaultHInstance(HINSTANCE hInst);
		void SetUnavailableIcon(HICON hIcon);
		void SetScaleFactor(float scaleFactor);
		void SetDynamicScaling(DynamicScaleParsing dsp);
		bool IsDynamicScaling();
		HRESULT CopySheets(DynamicArray<Value*>** ppdaSheets);
		void SetOverrideScaleFactor(float scaleFactor);
		bool GetOverrideScaleFactor(float* pScaleFactor);

	protected:
		HRESULT Initialize();
		HRESULT InitializeParserFromXmlReader(IXmlReader* pReader, HINSTANCE hResourceInstance, HINSTANCE hControlsInstance);

		HRESULT ParseStyleSheets(IXmlReader* pReader);
		HRESULT ParseStyleSheets();

		HRESULT _EnterOnCurrentThread();
		void _LeaveOnCurrentThread();

		static HRESULT GetXmlLiteDll(HMODULE* phXmlLiteDll);
		HRESULT CreateXmlReader(IXmlReader** ppReader);
		HRESULT CreateXmlReaderFromHGLOBAL(HGLOBAL hglob, IXmlReader** ppReader);
		HRESULT CreateXmlReaderInputWithEncodingName(IStream* pInput, const WCHAR* pszEncodingName, IUnknown** ppReaderInput);

		HRESULT _SetXMLFromResource(const WCHAR* pszResource, const WCHAR* pszResType, HINSTANCE hResourceInstance, HINSTANCE hControlsInstance, HINSTANCE hResTheme);
		HRESULT _SetBinaryXml(const BYTE* pBuffer, UINT cbBuffer, HINSTANCE hControlsInstance);
		HRESULT _BuildFromBinary(Element* peSub, Element* peParent, const WCHAR* pszResource, DWORD* pdwDefer, Element** ppElement);
		HRESULT _BuildElement(IXmlReader* pReader, Element* peParent, Element** ppElement);
		HRESULT _BuildChildren(IXmlReader* pReader, Element* peParent);
		HRESULT _SetProperties(IXmlReader* pReader, IClassInfo* pClassInfo, Element* peTarget);
		HRESULT _ParseLayout(const WCHAR* pszLayoutString, Value** ppvLayout);
		HRESULT _ParseBehavior(Element* pTarget, const WCHAR* pszBehavior);
		HRESULT _CreateValue(const WCHAR* pszValue, const PropertyInfo* pPropInfo, Value** ppValue);
		HRESULT _GetClassForElement(IXmlReader* pReader, IClassInfo** ppClassInfo);
		HRESULT _GetClassForElementByName(const WCHAR* pszName, IClassInfo** ppClassInfo);
		HRESULT _GetPropertyForAttribute(IXmlReader* pReader, IClassInfo* pClassInfo, const PropertyInfo** ppPropInfo);
		HRESULT _BuildStyles(IXmlReader* pReader);
		HRESULT _GetValueForStyleSheet(IClassInfo* pClassInfo, const WCHAR* pszName, const WCHAR* pszValue, const PropertyInfo** ppPropInfo, Value** ppValue);
		LINEINFO _GetLineInfo(IXmlReader* pReader);
		HRESULT _SetupParserState(HINSTANCE hResourceInstance, HINSTANCE hControlsInstance);

		HRESULT _RecordElementTrees(IXmlReader* pReader);
		HRESULT _RecordElementWithChildren(IXmlReader* pReader, bool fEmptyElem, WCHAR** ppszElemName);
		HRESULT _RecordInstantiateElement(IXmlReader* pReader, WCHAR** ppszElemName);
		HRESULT _RecordSetElementProperties(IXmlReader* pReader);
		HRESULT _RecordElementLayout(IXmlReader* pReader, const WCHAR* pszValue);
		HRESULT _RecordElementBehaviors(IXmlReader* pReader, const WCHAR* pszValue);
		HRESULT _RecordSetValue(IXmlReader* pReader, const WCHAR* pszName, const WCHAR* pszValue);
		HRESULT _RecordElementStyleSheet(const WCHAR* pszSheetName, bool fIgnoreMissingSheet);

		HRESULT CreateStyleSheet(IXmlReader* pReader, const WCHAR* pszSheetResid, StyleSheet** ppSheet);
		HRESULT AddRulesToStyleSheet(IXmlReader* pReader, StyleSheet* pSheet, const WCHAR* pszSheetResid, DynamicArray<XMLParserCond>* pdaXMLConds, DynamicArray<WCHAR*>* pdaCondStrings);
		HRESULT _ResolveStyleSheet(const WCHAR* pszReference, Value** ppvSheet, UINT* pSheetId);


		HRESULT MapPropertyNameToPropertyInfo(LINEINFO li, IClassInfo* pci, const WCHAR* pszName, const PropertyInfo** ppInfo);
		HRESULT GetPropValPairInfo(LINEINFO li, IClassInfo* pci, const WCHAR* pszProperty, const WCHAR* pszValue, const PropertyInfo** ppInfo, Value** ppValue);
		HRESULT GetPropValPairInfo(IXmlReader* pReader, IClassInfo* pci, const WCHAR* pszProperty, const WCHAR* pszValue, const PropertyInfo** ppInfo, Value** ppValue);

		HRESULT _ParseValue(const PropertyInfo* ppi, WCHAR* pszValue, Value** ppValue);

		void SendParseError(const WCHAR* pszError, const WCHAR* pszToken, int dLine, int dCol, HRESULT hr);
		void SendParseError(const WCHAR* pszError, const WCHAR* pszToken, IXmlReader* pReader, HRESULT hr);
		int QuerySysMetric(int idx, bool* pfDynamicScaling);
		const WCHAR* QuerySysMetricStr(int idx, WCHAR* pszMetric, UINT c);

		
		template <typename T>
		struct UILIB_API FunctionDefinition
		{
			typedef HRESULT (*FUNCPARSER)(const ParserTools::ExprNode* pen, T*, bool*);

			const WCHAR* pszName;
			FUNCPARSER pfnParse;
		};

		static const FunctionDefinition<int> s_fdInt[3];
		static const FunctionDefinition<COLORREF> s_fdClr[4];
		static const FunctionDefinition<Value*> s_fdString[4];
		static const FunctionDefinition<ScaledRECT> s_fdRect[3];
		static const FunctionDefinition<ScaledSIZE> s_fdSize[3];
		static const FunctionDefinition<Value*> s_fdFill[4];
		static const FunctionDefinition<Value*> s_fdGraphic[5];

		struct NUMBER
		{
			int i;
			bool fDynamicScaling;
		};

		union ParsedArg
		{
			NUMBER number;
			LPCWSTR psz;
			COLORREF clr;
			HTHEME htheme;
			HMODULE hmod;
			HINSTANCE hinst;
			const ParserTools::ExprNode* pen;
		};

		HRESULT ParseArgs(const ParserTools::ExprNode* pen, ParsedArg* rgpa, UINT cpa, const CHAR* pszSignature);
		HRESULT ParseFunction(const WCHAR* pszName, const ParserTools::ExprNode* pen, ParsedArg* rgpa, UINT cpa, const CHAR* pszSignature);

		static bool WINAPI IsThemeClassName(const ParserTools::ExprNode* pen);

		HRESULT ParseMagnitude(const WCHAR* pszMag, int* pMagOut, bool* pfDynamicScaling);
		HRESULT ParseMagnitudeFloat(const WCHAR* pszMag, float* pMagOut, bool* pfDynamicScaling);

		HRESULT ParseLiteralNumber(const WCHAR* psz, int* pnOut, bool* pfDynamicScaling);
		HRESULT ParseColorInt(WCHAR*, int*);
		HRESULT ParseLiteralColorInt(const WCHAR* pszValue, int* piOut);
		HRESULT ParseLiteralColor(const WCHAR* pszValue, COLORREF* pclrOut);

		HRESULT ParseSysMetricInt(const ParserTools::ExprNode* pen, int* piOut, bool* pfDynamicScaling);
		HRESULT ParseGTMetInt(const ParserTools::ExprNode* pen, int* piOut, bool* pfDynamicScaling);

		HRESULT ParseARGBColor(const ParserTools::ExprNode* pen, COLORREF* pclrOut, bool* pfDynamicScaling);
		HRESULT ParseRGBColor(const ParserTools::ExprNode* pen, COLORREF* pclrOut, bool* pfDynamicScaling);
		HRESULT ParseGTCColor(const ParserTools::ExprNode* pen, COLORREF* pclrOut, bool* pfDynamicScaling);

		HRESULT ParseResStr(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);
		HRESULT ParseSysMetricStr(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);
		HRESULT ParseGTFStr(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);

		HRESULT ParseRectRect(const ParserTools::ExprNode* pen, ScaledRECT* prcOut, bool* pfDynamicScaling);
		HRESULT ParseGTMarRect(const ParserTools::ExprNode* pen, ScaledRECT* prcOut, bool* pfDynamicScaling);

		HRESULT ParseSizeSize(const ParserTools::ExprNode* pen, ScaledSIZE* pszOut, bool* pfDynamicScaling);
		HRESULT ParseGTPartSize(const ParserTools::ExprNode* pen, ScaledSIZE* pszOut, bool* pfDynamicScaling);

		HRESULT ParseSGraphicHelper(bool fGraphic, const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseSGraphicGraphic(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);

		HRESULT ParseGraphicHelper(bool fGraphic, const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseGraphicGraphic(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);

		HRESULT ParseImageGraphic(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);
		HRESULT ParseIconGraphic(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);

		HRESULT ParseLiteral(const ParserTools::ExprNode* pen, const WCHAR** ppszOut);
		HRESULT ParseNumber(const ParserTools::ExprNode* pen, int* pnOut, bool* pfDynamicScaling);
		HRESULT ParseFloat(const ParserTools::ExprNode* pen, float* pflOut, bool* pfDynamicScaling);
		HRESULT ParseColor(const ParserTools::ExprNode* pen, COLORREF* pclrOut);
		HRESULT ParseQuotedString(const ParserTools::ExprNode* pen, WCHAR** ppszOut);
		HRESULT ParseResid(const ParserTools::ExprNode* pen, const WCHAR** ppszOut);
		HRESULT ParseTheme(const ParserTools::ExprNode* pen, bool use96dpi, HTHEME* phtheme);
		HRESULT ParseLibrary(const ParserTools::ExprNode* pen, HMODULE* phmod);

		HRESULT ParseRect(const ParserTools::ExprNode* pen, ScaledRECT* prcOut);
		HRESULT ParseSize(const ParserTools::ExprNode* pen, ScaledSIZE* pszOut);

		HRESULT ParseGradientFill(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);
		HRESULT ParseDFCFill(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);
		HRESULT ParseDTBFill(const ParserTools::ExprNode* pen, Value** ppVal, bool* pfDynamicScaling);

		HRESULT CreateLayout(const ParserTools::ExprNode* pen, PLAYTCREATE pfnCreate);

		HRESULT ParseIntValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseFloatValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseBoolValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseStringValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseRectValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseSizeValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParsePointValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseFillValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseLayoutValue(const ParserTools::ExprNode* pen);
		HRESULT ParseGraphicValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseSheetValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseAtomValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseCursorValue(const ParserTools::ExprNode* pen, Value** ppVal);

		HRESULT ParseBehaviorArgValue(const ParserTools::ExprNode* pen, Value** ppVal);
		HRESULT ParseBehavior(const ParserTools::ExprNode* pen);
		HRESULT ParseDoubleListValue(const ParserTools::ExprNode* pen, Value** ppVal);

		HRESULT MapPropertyEnumValue(const EnumMap* pMap, const WCHAR* pszValue, int* piVal);
		HRESULT GetValueParser(ParserTools::ValueParser** ppvp);
		void ReturnValueParser(ParserTools::ValueParser* pvp);
		HRESULT GetParserCommon(DUIXmlParser** ppParserCommon);

		HMODULE _hXmlLiteDll;
		HINSTANCE _hControlsInstance;
		HANDLE _hDefault;
		HICON _hUnavailableIcon;

		PXMLPARSEERRORCB _pfnErrorCB;
		PXMLGETSHEETCB _pfnGetSheetCB;
		PUNKNOWNATTRCB _pfnUnknownAttrCallback;
		
		void* _pErrorContext;
		void* _pGetSheetContext;
		void* _pfnUnknownAttrContext;

		DynamicScaleParsing _dynamicScaleParsing;
		float _flScaleFactor;
		bool _fScaleFactorSet;
		bool _fParseErrorSent;
		bool _fDesignMode;
		int _fThemed;
		int _fHighContrast;
		int _fComposited;
		bool _fOverrideScale;
		
		const PropertyInfo* _ppiValue;
		const WCHAR* _pszError;
		
		ParserTools::ValueParser* _pvpCache;
		DynamicArray<Value*>* _pdaSheetList;
		DynamicArray<Value*>* _pdaTrackedValues;
		DUIXmlParser* _pParserCommon;
		IXmlReader* _pCurrentReader;
		
		IDuiParserCache* _pParserCache;
		IDuiBinaryReader* _pBinaryReader;
		
		int _nCreateDepth;
		
		DynamicArray<int>* _prgLayoutArgs;
		
		
	private:
		float _ScaleRelativePixels(float nRelPix);
		int _ScaleRelativePixels(int nRelPix);
		float _ScalePointsToPixels(float flPoint);
		int _ScalePointsToPixels(int nPoint);

		DUIParseRecorder* _pRecorder;
		DynamicArray<DUIParsePlayer*>* _prgPlayers;
		void SetParseState(DUI_PARSE_STATE eParseState);
		DUI_PARSE_STATE _eParseState;
		ResourceModuleHandles _rmhResourcesModules;
		DWORD _dwThreadId;
		ULONG _cRefThread;
		HWND _rootWindowForTheming;

	public:
		DUIXmlParser& operator=(DUIXmlParser const&);
	};

	class DUIFactory
	{
	public:
		UILIB_API DUIFactory(HWND hwnd);
		UILIB_API ~DUIFactory();
		UILIB_API DUIFactory& operator=(DUIFactory const&);

		UILIB_API HRESULT CreateParser();

		UILIB_API HRESULT LoadFromBuffer(const WCHAR* pszBuf, UINT cchBuf, const WCHAR* pszResid, Element* pParent, DWORD* pdwDeferCookie, Element** ppe);
		UILIB_API HRESULT LoadFromFile(const WCHAR*, const WCHAR*, Element*, unsigned long*, Element* *);
		UILIB_API HRESULT LoadFromResource(HINSTANCE, const WCHAR*, const WCHAR*, Element*, unsigned long*, Element* *, const WCHAR*);

		UILIB_API void SetError(const WCHAR*, ...);

		UILIB_API DUIXmlParser* DetachParser();
		UILIB_API DUIXmlParser* GetParser();

	private:
		UILIB_API static void WINAPI s_XMLParseError(const WCHAR* pszError, const WCHAR* pszToken, int dLine, DUIFactory* pContext);
		UILIB_API void ClearParser();

		int _fError;
		HINSTANCE _hInst;
		DUIXmlParser* _pParser;
		HWND _hwndParent;
	};
}
