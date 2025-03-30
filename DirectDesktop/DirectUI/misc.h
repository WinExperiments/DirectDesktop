#pragma once

#ifdef _DEBUG
#define DUI_ASSERT(msg) \
	{ \
		HMODULE hModule = GetModuleHandleW(L"ntdll.dll"); \
		if (hModule) \
		{ \
			typedef ULONG (*DbgPrintEx_t)(ULONG ComponentId, ULONG Level, PCSTR Format, ...); \
			DbgPrintEx_t pfnDbgPrintEx = (DbgPrintEx_t)GetProcAddress(hModule, "DbgPrintEx"); \
			if (pfnDbgPrintEx) \
			{ \
				pfnDbgPrintEx(101, 0, msg); \
			} \
		} \
		_ASSERTE(false); \
	}

#define DUI_ASSERT_EXPR(expr, msg) \
	if (!(expr)) \
	{ \
	   DUI_ASSERT("(" #expr ")\r\n" msg "\r\n"); \
	}
#else
#define DUI_ASSERT(msg)
#define DUI_ASSERT_EXPR(expr, msg)
#endif

namespace DirectUI
{
	inline void* HAlloc(SIZE_T cbBlockSize)
	{
		return HeapAlloc(GetProcessHeap(), 0, cbBlockSize);
	}

	inline void* HReAlloc(void* p, SIZE_T s)
	{
		return HeapReAlloc(GetProcessHeap(), 0, p, s);
	}

	inline void HFree(void* pv)
	{
		HeapFree(GetProcessHeap(), 0, pv);
	}

	template <typename T>
	T* HNewAndZero()
	{
		T* p = (T*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(T));
		if (p)
			new (p) T();
		return p;
	}

	template <typename T>
	T* HNew()
	{
		return HNewAndZero<T>();
	}

	template <typename T>
	void HDelete(T* p)
	{
		p->~T();
		HFree(p);
	}

	// exported for int
	template <typename T>
	class SafeArrayAccessor
	{
		SAFEARRAY* _pSafeArray;
		int _count;
		T* _pData;

	public:
		SafeArrayAccessor()
			: _pSafeArray(nullptr)
			, _count(0)
			, _pData(nullptr)
		{
		}

	private:
		SafeArrayAccessor(const SafeArrayAccessor&) = delete;
		void operator=(const SafeArrayAccessor&) = delete;

	public:
		~SafeArrayAccessor()
		{
			SafeArrayUnaccessData(_pSafeArray);
		}

		HRESULT Access(SAFEARRAY* pSafeArray, VARTYPE vtExpected)
		{
			_pSafeArray = pSafeArray;

			if (SafeArrayGetDim(pSafeArray) != 1)
				return E_FAIL;

			VARTYPE actualType;
			HRESULT hr = SafeArrayGetVartype(_pSafeArray, &actualType);
			if (SUCCEEDED(hr))
			if (SUCCEEDED(hr))
			{
				if (actualType != vtExpected)
					return E_FAIL;

				LONG lb;
				hr = SafeArrayGetLBound(_pSafeArray, 1, &lb);
				if (SUCCEEDED(hr))
				{
					LONG ub;
					hr = SafeArrayGetUBound(_pSafeArray, 1, &ub);
					if (SUCCEEDED(hr))
					{
						_count = ub - lb + 1;
						hr = SafeArrayAccessData(_pSafeArray, (void**)&_pData);
					}
				}
			}

			return hr;
		}

		int Count() { return _count; }
		operator T*() { return _pData; }
	};

	class UILIB_API CritSecLock
	{
	public:
		CritSecLock(CRITICAL_SECTION* pacs);
		~CritSecLock();

		void Unlock();

	private:
		CRITICAL_SECTION* _pacs;
	};

	template <typename T>
	class DoubleAllocationPolicy
	{
	public:
		struct HeapT
		{
			T* _pData;
			UINT _uCapacity;
		};

		static UINT GetCapacity(const HeapT __unaligned& heap, UINT uRequiredCapacity)
		{
			return heap._uCapacity;
		}

		static void SetCapacity(HeapT __unaligned* pHeap, UINT uCapacity)
		{
			pHeap->_uCapacity = uCapacity;
		}

		static UINT NewCapacity(UINT uRequiredCapacity)
		{
			return uRequiredCapacity;
		}

		static UINT NewCapacity(const HeapT __unaligned& heap, UINT uRequiredCapacity)
		{
			UINT uNewCapacity;
			if (SUCCEEDED(ULongLongToUInt(2 * heap._uCapacity, &uNewCapacity)))
			{
				return max(uRequiredCapacity, uNewCapacity);
			}
			return uRequiredCapacity;
		}
	};

	// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
	template <typename T, typename TAllocationPolicy, int MinInplaceCapacity = 1, int MinHeapCapacity = 0>
	class DynamicArrayBase
	{
		using HeapT = typename TAllocationPolicy::HeapT;

	public:
		static const UINT INPLACE_BUFFER_SIZE_CALC = sizeof(HeapT) / sizeof(T);
		static const UINT INPLACE_BUFFER_SIZE = max(MinInplaceCapacity, INPLACE_BUFFER_SIZE_CALC);
		static const UINT MAX_DYNAMIC_ARRAY_SIZE = 0xFFFFFFF;

		void Destroy()
		{
			Release();
			HFree(this);
		}

		void Release()
		{
			if (_fHeapBuffer)
			{
				CHeapBuffer_Release(this);
			}

			Reset();
		}

		void DestroyDeep()
		{
			T* pBuffer = Buffer();
			for (UINT i = 0; i < _uSize; i++)
			{
				HDelete(pBuffer[i]);
			}
		}

		static HRESULT Create(bool fZeroData, DynamicArrayBase** ppArray)
		{
			return Create(0, fZeroData, ppArray);
		}

		static HRESULT Create(UINT uCapacity, bool fZeroData, DynamicArrayBase** ppArray)
		{
			DynamicArrayBase* p = HNew<DynamicArrayBase>();
			if (!p)
				return E_OUTOFMEMORY;

			HRESULT hr = p->Initialize(uCapacity, fZeroData);
			if (SUCCEEDED(hr))
			{
				*ppArray = p;
				hr = S_OK;
			}
			else
			{
				p->Destroy();
			}

			return hr;
		}

		HRESULT Initialize(bool fZeroData)
		{
			return Initialize(0, fZeroData);
		}

		HRESULT Initialize(UINT uCapacity, bool fZeroData)
		{
			HRESULT hr = S_OK;

			Reset();
			_fZeroData = fZeroData;
			_fImmutable = false;
			_Heap = {};

			if (uCapacity > MAX_DYNAMIC_ARRAY_SIZE)
			{
				hr = E_INVALIDARG;
			}
			else if (uCapacity > INPLACE_BUFFER_SIZE)
			{
				SwitchToHeap(uCapacity);
			}

			return hr;
		}

		HRESULT SizeIncrement(UINT uSize, UINT* puNewSize)
		{
			HRESULT hr;

			UINT uNewSize = uSize + 1;
			if (uNewSize >= uSize && uNewSize <= MAX_DYNAMIC_ARRAY_SIZE)
			{
				*puNewSize = uNewSize;
				hr = S_OK;
			}
			else
			{
				hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
				*puNewSize = -1;
			}

			return hr;
		}

		HRESULT InsertPtr(UINT uIndex, T** ppNewItem)
		{
			UINT uNewSize;
			HRESULT hr = SizeIncrement(_uSize, &uNewSize);
			if (SUCCEEDED(hr))
			{
				if (!_fHeapBuffer && _uSize == INPLACE_BUFFER_SIZE)
				{
					hr = SwitchToHeap(uNewSize);
				}

				if (SUCCEEDED(hr))
				{
					T* pNewItem;
					if (_fHeapBuffer)
					{
						hr = CHeapBuffer_InsertPtr(this, uIndex, uNewSize, &pNewItem);
					}
					else
					{
						hr = CInplaceBuffer_InsertPtr(this, uIndex, uNewSize, &pNewItem);
					}

					if (SUCCEEDED(hr))
					{
						memmove(pNewItem + 1, pNewItem, sizeof(T) * (_uSize - uIndex));
						_uSize = uNewSize;
						if (_fZeroData)
						{
							*pNewItem = {};
						}
						*ppNewItem = pNewItem;
					}
				}
			}

			return hr;
		}

		HRESULT Reserve(UINT uCapacity)
		{
			HRESULT hr = S_OK;

			if (_fHeapBuffer)
			{
				hr = CHeapBuffer_Reserve(this, uCapacity);
			}
			else if (uCapacity > INPLACE_BUFFER_SIZE)
			{
				hr = SwitchToHeap(uCapacity);
			}

			return hr;
		}

		HRESULT Add(T tItem)
		{
			return Insert(_uSize, tItem);
		}

		// Assumed function body
		HRESULT AddPtr(T** ppNewItem, UINT* puIndex)
		{
			if (puIndex)
				*puIndex = -1;

			HRESULT hr = InsertPtr(_uSize, ppNewItem);
			if (SUCCEEDED(hr))
			{
				if (puIndex)
					*puIndex = _uSize - 1;
			}

			return hr;
		}

		HRESULT AddPtr(T** ppNewItem)
		{
			return AddPtr(ppNewItem, nullptr);
		}

		HRESULT Insert(UINT uIndex, T tItem)
		{
			T* pNewItem;
			HRESULT hr = InsertPtr(uIndex, &pNewItem);
			if (SUCCEEDED(hr))
			{
				*pNewItem = tItem;
			}

			return hr;
		}

		void SetItem(UINT uIndex, T tItem)
		{
			*GetItemPtr(uIndex) = tItem;
		}

		T GetItem(UINT uIndex) const
		{
			return *GetItemPtr(uIndex);
		}

		T* GetItemPtr(UINT uIndex)
		{
			DUI_ASSERT_EXPR(uIndex < _uSize, "DynamicArray index out of bounds");
			return Buffer() + uIndex;
		}

		const T* GetItemPtr(UINT uIndex) const
		{
			DUI_ASSERT_EXPR(uIndex < _uSize, "DynamicArray index out of bounds");
			return Buffer() + uIndex;
		}

		UINT GetIndexPtr(T* ptItem) const
		{
			return GetIndexOf(*ptItem);
		}

		int GetIndexOf(T tItem) const
		{
			const T* pBuffer = Buffer();
			for (UINT i = 0; i < _uSize; i++)
			{
				if (pBuffer[i] == tItem)
				{
					return i;
				}
			}

			return -1;
		}

		UINT GetSize() const
		{
			return _uSize;
		}

		UINT GetCapacity() const
		{
			return _fHeapBuffer ? CHeapBuffer_GetCapacity(this) : CInplaceBuffer_GetCapacity(this);
		}

		void Remove(UINT uIndex)
		{
			Remove(uIndex, 1);
		}

		void Remove(UINT uIndex, UINT cItems)
		{
			if (_uSize - uIndex != cItems)
			{
				T* pBuffer = Buffer();
				memmove(pBuffer + uIndex, pBuffer + (uIndex + cItems), sizeof(T) * (_uSize - uIndex - cItems));
			}
			_uSize -= cItems;
		}

		void Reset()
		{
			_uSize = 0;
			_fHeapBuffer = false;
		}

		bool IsEqual(const DynamicArrayBase* pOther) const
		{
			if (!pOther)
				return false;

			if (_uSize != pOther->_uSize)
				return false;

			const T* pBuffer = Buffer();
			const T* pOtherBuffer = pOther->Buffer();
			return !pBuffer || memcmp(pBuffer, pOtherBuffer, sizeof(T) * _uSize) == 0;
		}

		HRESULT Clone(DynamicArrayBase** ppClone) const
		{
			*ppClone = nullptr;
			HRESULT hr;

			DynamicArrayBase* p = HNew<DynamicArrayBase>();
			if (!p)
				return E_OUTOFMEMORY;

			p->Reset();
			p->_fZeroData = false;
			p->_fImmutable = false;
			p->_fZeroData = _fZeroData;
			p->_Heap = {};

			if (p->_fHeapBuffer)
			{
				hr = CHeapBuffer_Reserve(p, _uSize);
			}
			else if (_uSize > INPLACE_BUFFER_SIZE)
			{
				hr = p->SwitchToHeap(_uSize);
			}
			else
			{
				hr = S_OK;
			}

			if (SUCCEEDED(hr))
			{
				const T* pBuffer = Buffer();
				if (pBuffer)
				{
					memcpy(p->Buffer(), pBuffer, sizeof(T) * _uSize);
				}
				*ppClone = p;
				p->_uSize = _uSize;
			}
			else
			{
				p->Destroy();
			}

			return hr;
		}

		HRESULT SetSize(UINT uCapacity)
		{
			HRESULT hr = Reserve(uCapacity);
			if (SUCCEEDED(hr))
			{
				_uSize = uCapacity;
			}

			return hr;
		}

		typedef int (__cdecl *QSORTCMP)(const void*, const void*);

		void Sort(QSORTCMP fpCmp)
		{
			if (_uSize)
			{
				qsort(Buffer(), _uSize, sizeof(T), fpCmp);
			}
		}

		void MakeImmutable()
		{
#ifdef _DEBUG
			_fImmutable = true;
#endif
		}

		void MakeWritable()
		{
#ifdef _DEBUG
			_fImmutable = false;
#endif
		}

		bool Find(const T& item, UINT* piFound)
		{
			if (piFound)
				*piFound = -1;

			bool fFound = false;

			T* pBuffer = Buffer();
			for (UINT i = 0; i < _uSize; i++)
			{
				if (pBuffer[i] == item)
				{
					fFound = true;
					if (piFound)
						*piFound = i;
					break;
				}
			}

			return fFound;
		}

		template <typename TFunc>
		bool Find(UINT* piFound, const TFunc& func)
		{
			if (piFound)
				*piFound = -1;

			bool fFound = false;

			T* pBuffer = Buffer();
			for (UINT i = 0; i < _uSize; i++)
			{
				if (func(pBuffer[i]))
				{
					fFound = true;
					if (piFound)
						*piFound = i;
					break;
				}
			}

			return fFound;
		}

	private:
		HRESULT SwitchToHeap(UINT uMinCapacity)
		{
			UINT uInitial = TAllocationPolicy::NewCapacity(max(uMinCapacity, MinHeapCapacity));

			UINT cbToAlloc;
			HRESULT hr = ULongLongToUInt(sizeof(T) * uInitial, &cbToAlloc);
			if (SUCCEEDED(hr))
			{
				T* pNewBuffer = (T*)HAlloc(cbToAlloc);
				hr = pNewBuffer ? S_OK : E_OUTOFMEMORY;
				if (SUCCEEDED(hr))
				{
					memcpy(pNewBuffer, Buffer(), sizeof(T) * _uSize);
					_fHeapBuffer = true;
					_Heap._pData = pNewBuffer;
					TAllocationPolicy::SetCapacity(&_Heap, uInitial);
				}
			}

			return hr;
		}

		static void CHeapBuffer_Release(DynamicArrayBase* pThis)
		{
			if (pThis->_Heap._pData)
			{
				HFree(pThis->_Heap._pData);
				pThis->_Heap._pData = nullptr;
			}
			TAllocationPolicy::SetCapacity(&pThis->_Heap, 0);
		}

		static HRESULT CHeapBuffer_Reserve(DynamicArrayBase* pThis, UINT uMinCapacity)
		{
			HRESULT hr = S_OK;

			if (uMinCapacity > TAllocationPolicy::GetCapacity(pThis->_Heap, pThis->_uSize))
			{
				UINT cbToAlloc;
				hr = ULongLongToUInt(sizeof(T) * TAllocationPolicy::NewCapacity(pThis->_Heap, uMinCapacity), &cbToAlloc);
				if (SUCCEEDED(hr))
				{
					if (pThis->_Heap._pData)
					{
						T* pNewData = (T*)HReAlloc(pThis->_Heap._pData, cbToAlloc);
						if (!pNewData)
							return E_OUTOFMEMORY;

						pThis->_Heap._pData = pNewData;
					}
					else
					{
						pThis->_Heap._pData = (T*)HAlloc(cbToAlloc);
						if (!pThis->_Heap._pData)
							return E_OUTOFMEMORY;
					}
				}
			}

			return hr;
		}

		static HRESULT CHeapBuffer_InsertPtr(DynamicArrayBase* pThis, UINT uIndex, UINT uNewSize, T** ppNewItem)
		{
			HRESULT hr = CHeapBuffer_Reserve(pThis, uNewSize);
			if (SUCCEEDED(hr))
			{
				*ppNewItem = &pThis->_Heap._pData[uIndex];
			}
			return hr;
		}

		static UINT CHeapBuffer_GetCapacity(DynamicArrayBase* pThis)
		{
			return pThis->_Heap._uCapacity;
		}

		static void CInplaceBuffer_Release(DynamicArrayBase* pThis)
		{
			// No-op
		}

		static HRESULT CInplaceBuffer_Reserve(DynamicArrayBase* pThis, UINT uMinCapacity)
		{
			if (uMinCapacity > INPLACE_BUFFER_SIZE)
			{
				_ASSERTE(false);
			}

			return S_OK;
		}

		static HRESULT CInplaceBuffer_InsertPtr(DynamicArrayBase* pThis, UINT uIndex, UINT uNewSize, T** ppNewItem)
		{
			if (uNewSize > INPLACE_BUFFER_SIZE)
			{
				_ASSERTE(false);
			}

			*ppNewItem = &pThis->_Inplace[uIndex];
			return S_OK;
		}

		static UINT CInplaceBuffer_GetCapacity(DynamicArrayBase* pThis)
		{
			return INPLACE_BUFFER_SIZE;
		}

		T* Buffer()
		{
			if (_fHeapBuffer)
			{
				return _Heap._pData;
			}
			else
			{
				if (_uSize > INPLACE_BUFFER_SIZE)
					_ASSERT(false);
				return _Inplace;
			}
		}

		const T* Buffer() const
		{
			if (_fHeapBuffer)
			{
				return _Heap._pData;
			}
			else
			{
				if (_uSize > INPLACE_BUFFER_SIZE)
					_ASSERT(false);
				return _Inplace;
			}
		}

		UINT _uSize : 28; // bits=0-27 mask=0xFFFFFFF
		UINT _fHeapBuffer : 1; // bits=28-28 mask=0x10000000 aka=((*(_DWORD*)this >> 28) & 1) != 0
		UINT _fZeroData : 1; // bits=29-29 mask=0x20000000 aka=((*(_DWORD*)this >> 29) & 1) != 0
		UINT _fImmutable : 1; // bits=30-30 mask=0x40000000 aka=((*(_DWORD*)this >> 30) & 1) != 0

		// _fHeapBuffer | _fZeroData | _fImmutable is 0x70000000

		union
		{
#ifdef _WIN64
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif
			// ReSharper disable once CppUninitializedNonStaticDataMember
			typename TAllocationPolicy::HeapT _Heap;
			// ReSharper disable once CppUninitializedNonStaticDataMember
			T _Inplace[INPLACE_BUFFER_SIZE];
#include <poppack.h>
		};
	};

	template <typename T, int MinHeapCapacity = 0>
	class DynamicArray : public DynamicArrayBase<T, DoubleAllocationPolicy<T>, 1, MinHeapCapacity>
	{
		using BaseT = DynamicArrayBase<T, DoubleAllocationPolicy<T>, 1, MinHeapCapacity>;

	public:
		~DynamicArray()
		{
			BaseT::Release();
		}

		void Destroy()
		{
			BaseT::Destroy();
		}

		static HRESULT Create(bool fZeroData, DynamicArray** ppArray)
		{
			return Create(0, fZeroData, ppArray);
		}

		static HRESULT Create(UINT uCapacity, bool fZeroData, DynamicArray** ppArray)
		{
			BaseT* pTmp;
			HRESULT hr = BaseT::Create(uCapacity, fZeroData, &pTmp);

			if (SUCCEEDED(hr))
			{
				*ppArray = (DynamicArray*)pTmp;
			}
			else
			{
				*ppArray = nullptr;
			}

			return hr;
		}

		HRESULT Clone(DynamicArray** ppClone) const
		{
			BaseT* pTmp;
			HRESULT hr = BaseT::Clone(&pTmp);

			if (SUCCEEDED(hr))
			{
				*ppClone = (DynamicArray*)pTmp;
			}
			else
			{
				*ppClone = nullptr;
			}

			return hr;
		}
	};

	class UILIB_API Expression
	{
	public:
		Expression() = delete;
		Expression(const Expression&) = delete;
		~Expression() = delete;

		Expression& operator=(Expression const&);
		void Destroy();
	};

	/*class UID
	{
	public:
	};*/

	class UILIB_API Surface
	{
	public:
		enum EType
		{
		};

		Surface(Surface const&);
		Surface();
		virtual ~Surface();
		Surface& operator=(Surface const&);

		static enum EType __stdcall GetSurfaceType(unsigned int);
		static unsigned int __stdcall GetSurfaceType(enum EType);

		virtual void T1() = 0;
	};

	class UILIB_API DCSurface
	{
	public:
		DCSurface(DCSurface const&);
		DCSurface(HDC);
		virtual ~DCSurface();
		DCSurface& operator=(DCSurface const&);

		HDC GetHDC();
		virtual Surface::EType GetType() const;
	};

	enum DUSER_MSG_FLAG : UINT32
	{
		GMF_DIRECT = 0x00000000, // OnMessage
		GMF_ROUTED = 0x00000001, // PreviewMessage
		GMF_BUBBLED = 0x00000002, // PostMessage
		GMF_EVENT = 0x00000003, // Message -> Event
		GMF_DESTINATION = 0x00000003, // Message reach dest (same as event)
	};

	enum DUSER_INPUT_DEVICE : UINT32
	{
		GINPUT_MOUSE = 0,
		GINPUT_KEYBOARD = 1,
		GINPUT_JOYSTICK = 2,
	};

	enum DUSER_INPUT_CODE : UINT32
	{
		GMOUSE_MOVE = 0,
		GMOUSE_DOWN = 1,
		GMOUSE_UP = 2,
		GMOUSE_DRAG = 3,
		GMOUSE_HOVER = 4,
		GMOUSE_WHEEL = 5,
		GMOUSE_MAX = 5,

		GBUTTON_NONE = 0,
		GBUTTON_LEFT = 1,
		GBUTTON_RIGHT = 2,
		GBUTTON_MIDDLE = 3,
		GBUTTON_MAX = 3,
	};

	// bitflags
	enum DUSER_INPUT_MODIFIERS : UINT32
	{
		GMODIFIER_NONE = 0x00000000,

		GMODIFIER_LCONTROL = 0x00000001,
		GMODIFIER_RCONTROL = 0x00000002,
		GMODIFIER_LSHIFT = 0x00000004,
		GMODIFIER_RSHIFT = 0x00000008,
		GMODIFIER_LALT = 0x00000010,
		GMODIFIER_RALT = 0x00000020,
		GMODIFIER_LBUTTON = 0x00000040,
		GMODIFIER_RBUTTON = 0x00000080,
		GMODIFIER_MBUTTON = 0x00000100,

		GMODIFIER_CONTROL = (GMODIFIER_LCONTROL | GMODIFIER_RCONTROL),
		GMODIFIER_SHIFT = (GMODIFIER_LSHIFT | GMODIFIER_RSHIFT),
		GMODIFIER_ALT = (GMODIFIER_LALT | GMODIFIER_RALT),
	};

	enum TOUCHTOOLTIP_CREATE_FLAGS
	{
		TTTCF_DEFAULT = 0,
		TTTCF_DESKTOP_DPI = 1,
		TTTCF_CONSTRAIN_TO_WORKSPACE = 2,
		TTTCF_CONSTRAIN_TO_MONITOR = 4
	};

	enum TOUCHTOOLTIP_OPTION_FLAGS
	{
		TTTOF_DEFAULT = 0,
		TTTOF_LAST_LINE_SUBTITLE = 1,
		TTTOF_SLIDER = 2,
		TTTOF_NO_WRAP = 4,
		TTTOF_NO_UIA_OPENED_EVENT = 8,
		TTTOF_SCALE_WITH_OWNER = 16
	};

	enum TOUCHTOOLTIP_INPUT
	{
		TTTI_PROGRAMMATIC = 0,
		TTTI_KEYBOARD = 1,
		TTTI_MOUSE = 2,
		TTTI_POINTER = 3,
		TTTI_COUNT = 4
	};

	enum TOUCHTOOLTIP_PLACEMENT
	{
		TTTP_ABOVE = 0,
		TTTP_BELOW = 1,
		TTTP_LEFT = 2,
		TTTP_RIGHT = 3,
		TTTP_COUNT = 4
	};

	enum TOUCHTOOLTIP_TYPE
	{
		TTTT_TEXT = 0,
		TTTT_RICH = 1,
		TTTT_COUNT = 2
	};

	enum TOUCHTOOLTIP_DELAY
	{
		TTTD_NORMAL = 0,
		TTTD_RESHOW = 1,
		TTTD_COUNT = 2
	};

	enum TOUCHTOOLTIP_LAYOUT_DIRECTION
	{
		TTTLD_INHERIT = 0,
		TTTLD_LTR = 1,
		TTTLD_RTL = 2,
		TTTLD_COUNT = 3
	};

	enum ActiveState
	{
		AS_HIDDEN = 0,
		AS_REST = 1,
		AS_MOUSE = 2,
		AS_PAN = 3,
	};

	struct Event
	{
		Element* peTarget;
		UID uidType;
		bool fHandled;
		DUSER_MSG_FLAG nStage;
		bool fUIAHandled;
	};

	struct InputEvent
	{
		Element* peTarget;
		bool fHandled;
		DUSER_MSG_FLAG nStage;
		DUSER_INPUT_DEVICE nDevice;
		DUSER_INPUT_CODE nCode;
		DUSER_INPUT_MODIFIERS uModifiers;
	};

	struct PointerEvent : InputEvent
	{
		POINT ptClientPxl;
		UINT nPointerID;
		BOOL fPrimary;
	};

	struct PointerTapEvent : PointerEvent
	{
		UINT cTaps;
		BOOL fPressAndTap;
		BOOL fWithin;
		UINT nFlags;
		BOOL fPressAndHold;
	};

	struct PointerEnterEvent : PointerEvent
	{
	};

	struct PointerLeaveEvent : PointerEvent
	{
	};

	struct PointerCancelEvent : PointerEvent
	{
		UINT nCancelReason;
	};

	struct PointerManipulationEvent : PointerEvent
	{
		UINT nBeginDuringEnd;
		SIZE szOffsetFromPanStart;
		float fltZoom;
		float fltExpansion;
		float fltRotation;
		UINT nFlags;
	};

	struct PointerDragEvent : PointerEvent
	{
		SIZE sizeDelta;
		BOOL fWithin;
	};

	struct MouseEvent : InputEvent
	{
		POINT ptClientPxl;
		BYTE bButton;
		UINT nFlags;
	};

	struct MouseWheelEvent : MouseEvent
	{
		short sWheel;
	};

	struct MouseDragEvent : MouseEvent
	{
		SIZE sizeDelta;
		BOOL fWithin;
	};

	struct MouseClickEvent : MouseEvent
	{
		UINT cClicks;
	};

	struct KeyboardEvent : InputEvent
	{
		WCHAR ch;
		WORD cRep;
		WORD wFlags;
	};

	struct KeyboardNavigateEvent : Event
	{
		int iNavDir;
		bool fSetFocus;
	};

	struct AnimateScrollEvent : Event
	{
		int dDistance;
		bool fVertical;
	};

	struct RectangleChange
	{
	};

	struct ElementRuntimeId
	{
	};

	enum DIRECTMANIPULATION_CONFIGURATION
	{
		DIRECTMANIPULATION_CONFIGURATION_NONE = 0,
		DIRECTMANIPULATION_CONFIGURATION_INTERACTION = 1,
		DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X = 2,
		DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y = 4,
		DIRECTMANIPULATION_CONFIGURATION_SCALING = 16,
		DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA = 32,
		DIRECTMANIPULATION_CONFIGURATION_SCALING_INERTIA = 128,
		DIRECTMANIPULATION_CONFIGURATION_RAILS_X = 256,
		DIRECTMANIPULATION_CONFIGURATION_RAILS_Y = 512
	};

	enum DIRECTMANIPULATION_GESTURE_CONFIGURATION
	{
		DIRECTMANIPULATION_GESTURE_NONE = 0,
		DIRECTMANIPULATION_GESTURE_DEFAULT = 0,
		DIRECTMANIPULATION_GESTURE_CROSS_SLIDE_VERTICAL = 8,
		DIRECTMANIPULATION_GESTURE_CROSS_SLIDE_HORIZONTAL = 16,
		DIRECTMANIPULATION_GESTURE_PINCH_ZOOM = 32
	};

	enum DIRECTMANIPULATION_HITTEST_TYPE
	{
		DIRECTMANIPULATION_HITTEST_TYPE_ASYNCHRONOUS = 0,
		DIRECTMANIPULATION_HITTEST_TYPE_SYNCHRONOUS = 1,
		DIRECTMANIPULATION_HITTEST_TYPE_AUTO_SYNCHRONOUS = 2
	};

	enum DIRECTMANIPULATION_HORIZONTALALIGNMENT
	{
		DIRECTMANIPULATION_HORIZONTALALIGNMENT_NONE = 0,
		DIRECTMANIPULATION_HORIZONTALALIGNMENT_LEFT = 1,
		DIRECTMANIPULATION_HORIZONTALALIGNMENT_CENTER = 2,
		DIRECTMANIPULATION_HORIZONTALALIGNMENT_RIGHT = 4,
		DIRECTMANIPULATION_HORIZONTALALIGNMENT_UNLOCKCENTER = 8
	};

	enum DIRECTMANIPULATION_INPUT_MODE
	{
		DIRECTMANIPULATION_INPUT_MODE_AUTOMATIC = 0,
		DIRECTMANIPULATION_INPUT_MODE_MANUAL = 1
	};

	enum DIRECTMANIPULATION_MOTION_TYPES
	{
		DIRECTMANIPULATION_MOTION_NONE = 0,
		DIRECTMANIPULATION_MOTION_TRANSLATEX = 1,
		DIRECTMANIPULATION_MOTION_TRANSLATEY = 2,
		DIRECTMANIPULATION_MOTION_ZOOM = 4,
		DIRECTMANIPULATION_MOTION_CENTERX = 16,
		DIRECTMANIPULATION_MOTION_CENTERY = 32,
		DIRECTMANIPULATION_MOTION_ALL = 55
	};

	enum DIRECTMANIPULATION_SNAPPOINT_COORDINATE
	{
		DIRECTMANIPULATION_COORDINATE_BOUNDARY = 0,
		DIRECTMANIPULATION_COORDINATE_ORIGIN = 1,
		DIRECTMANIPULATION_COORDINATE_MIRRORED = 16
	};

	enum DIRECTMANIPULATION_SNAPPOINT_TYPE
	{
		DIRECTMANIPULATION_SNAPPOINT_MANDATORY = 0,
		DIRECTMANIPULATION_SNAPPOINT_OPTIONAL = 1,
		DIRECTMANIPULATION_SNAPPOINT_MANDATORY_SINGLE = 2,
		DIRECTMANIPULATION_SNAPPOINT_OPTIONAL_SINGLE = 3
	};

	enum DIRECTMANIPULATION_STATUS
	{
		DIRECTMANIPULATION_BUILDING = 0,
		DIRECTMANIPULATION_ENABLED = 1,
		DIRECTMANIPULATION_DISABLED = 2,
		DIRECTMANIPULATION_RUNNING = 3,
		DIRECTMANIPULATION_INERTIA = 4,
		DIRECTMANIPULATION_READY = 5,
		DIRECTMANIPULATION_SUSPENDED = 6
	};

	enum DIRECTMANIPULATION_VERTICALALIGNMENT
	{
		DIRECTMANIPULATION_VERTICALALIGNMENT_NONE = 0,
		DIRECTMANIPULATION_VERTICALALIGNMENT_TOP = 1,
		DIRECTMANIPULATION_VERTICALALIGNMENT_CENTER = 2,
		DIRECTMANIPULATION_VERTICALALIGNMENT_BOTTOM = 4,
		DIRECTMANIPULATION_VERTICALALIGNMENT_UNLOCKCENTER = 8
	};

	enum DIRECTMANIPULATION_VIEWPORT_OPTIONS
	{
		DIRECTMANIPULATION_VIEWPORT_OPTIONS_DEFAULT = 0,
		DIRECTMANIPULATION_VIEWPORT_OPTIONS_AUTODISABLE = 1,
		DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE = 2,
		DIRECTMANIPULATION_VIEWPORT_OPTIONS_INPUT = 4,
		DIRECTMANIPULATION_VIEWPORT_OPTIONS_EXPLICITHITTEST = 8,
		DIRECTMANIPULATION_VIEWPORT_OPTIONS_DISABLEPIXELSNAPPING = 16
	};
	
	class UILIB_API ResourceModuleHandles
	{
	public:
		ResourceModuleHandles();
		~ResourceModuleHandles();
		ResourceModuleHandles& operator=(ResourceModuleHandles const&);
		long GetHandle(unsigned short const*, HINSTANCE*);

	private:
		static long volatile g_cRefCount;
	};

	template <class T>
	class UiaArray
	{
	public:
	};

	class DeferCycle
	{
	};
} // namespace DirectUI
