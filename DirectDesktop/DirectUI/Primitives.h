#pragma once

typedef DirectUI::Value* (WINAPI *DefaultValueProcT)();

enum DynamicScaleValue
{
	DSV_None = 0x0,

	DSV_Int = 0x1,

	DSV_Float = 0x1,

	DSV_Left = 0x1,
	DSV_Top = 0x2,
	DSV_Right = 0x4,
	DSV_Bottom = 0x8,

	DSV_Height = 0x1,
	DSV_Width = 0x2,

	DSV_X = 0x1,
	DSV_Y = 0x2,
};


DEFINE_ENUM_FLAG_OPERATORS(DynamicScaleValue);

namespace DirectUI
{
	// nThreadMode flags for DirectUI::InitThread
	enum THREAD_SCALING_MODE
	{
		TSM_DESKTOP = 0x1,
		TSM_IMMERSIVE = 0x2,
		TSM_PRIMARY = 0x3,
		TSM_DESKTOP_DYNAMIC = 0x4,
	};
	
	struct ScaledSIZE : SIZE
	{
		DynamicScaleValue dynamicScaleValue;
	};

	struct ScaledInt
	{
		int i;
		DynamicScaleValue dynamicScaleValue;
	};

	struct ScaledFloat
	{
		float fl;
		DynamicScaleValue dynamicScaleValue;
	};

	struct ScaledRECT : RECT
	{
		DynamicScaleValue dynamicScaleValue;
	};

	struct ScaledPOINT : POINT
	{
		DynamicScaleValue dynamicScaleValue;
	};

	struct Fill
	{
		BYTE dType;

		union
		{
			struct
			{
				COLORREF cr;
				COLORREF cr2;
				COLORREF cr3;
			} ref;

			struct
			{
				UINT uType;
				UINT uState;
			} fillDFC;

			struct
			{
				WCHAR* pszClassName;
				int iPartId;
				int iStateId;
			} fillDTB;
		};
	};

	struct Cursor
	{
		HCURSOR hCursor;
	};

	struct Graphic
	{
		HANDLE hImage;
		HANDLE hAltImage;
		USHORT cx;
		USHORT cy;

		struct
		{
			BYTE dImgType : 3;
			BYTE dMode : 4;
			bool bFlip : 1;
			bool bRTLGraphic : 1;
			bool bFreehImage : 1;
			bool bSharedResource : 1;

			union
			{
				BYTE dAlpha;

				struct
				{
					BYTE r : 8;
					BYTE g : 8;
					BYTE b : 8;
				} rgbTrans;
			};
		} BlendMode;

		HINSTANCE hResLoad;
		const WCHAR* lpszName;
		ScaledSIZE scaledSize;
		float fScaleFactor;
	};


	struct EnumMap
	{
		const WCHAR* pszEnum;
		int nEnum;
	};

	struct PropertyInfoData
	{
		Value* _pvDefault;
		int _iIndex;
		int _iGlobalIndex;
		IClassInfo* _pciOwner;
		LONG _cRef;
	};

	struct PropertyInfo
	{
		const WCHAR* pszName;
		int fFlags;
		int fGroups;
		const int* pValidValues;
		const EnumMap* pEnumMaps;
		DefaultValueProcT DefaultProc;
		PropertyInfoData* pData;
	};

	struct DepRecs
	{
		int iDepPos;
		int cDepCnt;
	};
}
