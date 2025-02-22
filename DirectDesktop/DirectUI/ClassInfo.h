#pragma once

#define DUI_SET_CLASS_INFO(className, pClassInfo) \
    __if_exists(className::Class) \
    { \
        className::Class = pClassInfo; \
    } \
    __if_not_exists(className::Class) \
    { \
        className::SetClassInfoPtr(pClassInfo); \
    }

namespace DirectUI
{
	class UILIB_API ClassInfoBase : public IClassInfo
	{
		class Impl
		{
		};

		Impl* _pImpl;

	public:
		ClassInfoBase();
		ClassInfoBase(const ClassInfoBase&) = default;

		virtual ~ClassInfoBase();

		HRESULT Initialize(HINSTANCE hModule, const WCHAR* pszName, bool fGlobal, const PropertyInfo* const* ppPI, UINT cPI);
		HRESULT Register();

		static bool ClassExist(IClassInfo** ppCI, const PropertyInfo* const* ppPI, UINT cPI, IClassInfo* pCIBase, HMODULE hModule, const WCHAR* pszName, bool fGlobal);

		//~ Begin DirectUI::IClassInfo Interface
		void AddRef() override;
		int Release() override;
		const PropertyInfo* EnumPropertyInfo(UINT nEnum) override;
		const PropertyInfo* GetByClassIndex(UINT iIndex) override;
		UINT GetPICount() const override;
		UINT GetGlobalIndex() const override;
		const WCHAR* GetName() const override;
		bool IsValidProperty(const PropertyInfo* ppi) const override;
		bool IsSubclassOf(IClassInfo* pci) const override;
		void Destroy() override;
		HINSTANCE GetModule() const override;
		bool IsGlobal() const override;
		void AddChild() override;
		void RemoveChild() override;
		int GetChildren() const override;
		void AssertPIZeroRef() const override;
		//~ End DirectUI::IClassInfo Interface
	};

	template <typename T>
	class StandardCreator
	{
	public:
		static HRESULT CreateInstance(Element* pElement, DWORD* pdwFlags, Element** ppElement)
		{
			return T::Create(pElement, pdwFlags, ppElement);
		}
	};

	template <typename T>
	class EmptyCreator
	{
	public:
		static HRESULT CreateInstance(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
		{
			DUI_ASSERT("Cannot instantiate this type via parser. Must use substitution.");
			return E_NOTIMPL;
		}
	};

	template <typename ControlType, typename SuperType, typename CreatorType = StandardCreator<ControlType>>
	class ClassInfo : public ClassInfoBase
	{
		static HRESULT Create(HMODULE hModule, const WCHAR* pszName, bool fGlobal, const PropertyInfo* const* ppPI, UINT cPI, ClassInfo** ppCI)
		{
			*ppCI = nullptr;
			ClassInfo* pCI = DirectUI::HNew<ClassInfo>();
			HRESULT hr = pCI ? S_OK : E_OUTOFMEMORY;
			if (SUCCEEDED(hr))
			{
				hr = pCI->Initialize(hModule, pszName, fGlobal, ppPI, cPI);
				if (SUCCEEDED(hr))
				{
					*ppCI = pCI;
				}
				else
				{
					HDelete(pCI);
				}
			}
			return hr;
		}

		static HRESULT Register(HMODULE hModule, const WCHAR* pszName, const PropertyInfo* const* ppPI, UINT cPI, bool fGlobal)
		{
			HRESULT hr = S_OK;

			if (SuperType::GetClassInfoPtr())
			{
				SuperType::GetClassInfoPtr()->AddRef();
			}
			else
			{
				hr = SuperType::Register();
			}

			if (SUCCEEDED(hr))
			{
				CritSecLock lock(Element::GetFactoryLock());

				IClassInfo* pClassExisting;
				if (ClassExist(&pClassExisting, ppPI, cPI, SuperType::GetClassInfoPtr(), hModule, pszName, fGlobal))
				{
					DUI_SET_CLASS_INFO(ControlType, pClassExisting);
					hr = S_OK;
				}
				else
				{
					DUI_SET_CLASS_INFO(ControlType, nullptr);

					ClassInfo* pCI;
					hr = Create(hModule, pszName, fGlobal, ppPI, cPI, &pCI);
					if (SUCCEEDED(hr))
					{
						hr = pCI->ClassInfoBase::Register();
						if (SUCCEEDED(hr))
						{
							DUI_SET_CLASS_INFO(ControlType, pCI);
						}
						else
						{
							pCI->Destroy();
						}
					}
				}
			}

			return hr;
		}

	public:
		static HRESULT Register(const WCHAR* pszName, const PropertyInfo* const* ppPI, UINT cPI)
		{
			return Register(nullptr, pszName, ppPI, cPI, false); // Don't know the real one yet
		}

		static HRESULT RegisterGlobal(HMODULE hModule, const WCHAR* pszName, const PropertyInfo* const* ppPI, UINT cPI)
		{
			return Register(hModule, pszName, ppPI, cPI, false);
		}

		HRESULT CreateInstance(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement) override
		{
			return CreatorType::CreateInstance(pParent, pdwDeferCookie, ppElement);
		}

		IClassInfo* GetBaseClass() override
		{
			return SuperType::GetClassInfoPtr();
		}

		void Destroy() override
		{
			HDelete(this);
			DUI_SET_CLASS_INFO(ControlType, nullptr);
		}
	};

	template <typename T>
	HRESULT CreateAndInit(Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
	{
		*ppElement = nullptr;
		T* pT = HNew<T>();
		HRESULT hr = pT ? S_OK : E_OUTOFMEMORY;
		if (SUCCEEDED(hr))
		{
			hr = pT->Initialize(pParent, pdwDeferCookie);
			if (SUCCEEDED(hr))
			{
				*ppElement = pT;
			}
			else
			{
				pT->Destroy(false);
			}
		}
		return hr;
	}

	template <typename T, typename A1T>
	HRESULT CreateAndInit(A1T arg1, Element* pParent, DWORD* pdwDeferCookie, Element** ppElement)
	{
		*ppElement = nullptr;
		T* pT = HNew<T>();
		HRESULT hr = pT ? S_OK : E_OUTOFMEMORY;
		if (SUCCEEDED(hr))
		{
			hr = pT->Initialize(arg1, pParent, pdwDeferCookie);
			if (SUCCEEDED(hr))
			{
				*ppElement = pT;
			}
			else
			{
				pT->Destroy(false);
			}
		}
		return hr;
	}
}

#undef DUI_SET_CLASS_INFO
