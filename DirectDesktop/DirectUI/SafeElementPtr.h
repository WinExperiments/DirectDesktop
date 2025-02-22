#pragma once

template <typename T>
class CSafeElementPtrBase : protected DirectUI::IElementListener
{
public:
	operator T*() const { return _pe; }
	T* operator->() const { return _pe; }
	bool operator!() const { return !_pe; }
	bool operator!=(T* pe) const { return _pe != pe; }
	bool operator==(T* pe) const { return _pe == pe; }

	CSafeElementPtrBase(const CSafeElementPtrBase&) = delete;

protected:
	CSafeElementPtrBase()
		: _pe(nullptr)
	{
	}

	CSafeElementPtrBase(int)
	{
		// Function body unknown
	}

	HRESULT _Assign(T* pe)
	{
		HRESULT hr = E_INVALIDARG;
		if (pe)
		{
			_pe = pe;
			hr = pe->AddListener(this);
			if (FAILED(hr))
			{
				_pe = nullptr;
			}
		}

		return hr;
	}

	//~ Begin DirectUI::IElementListener Interface
	void OnListenerAttach(DirectUI::Element* peFrom) override {}
	void OnListenerDetach(DirectUI::Element* peFrom) override { if (_pe) _pe = nullptr; }
	bool OnListenedPropertyChanging(DirectUI::Element* peFrom, const DirectUI::PropertyInfo* ppi, int iIndex, DirectUI::Value* pvOld, DirectUI::Value* pvNew) override { return true; }
	void OnListenedPropertyChanged(DirectUI::Element* peFrom, const DirectUI::PropertyInfo* ppi, int iIndex, DirectUI::Value* pvOld, DirectUI::Value* pvNew) override {}
	void OnListenedInput(DirectUI::Element* peFrom, DirectUI::InputEvent* pInput) override {}
	void OnListenedEvent(DirectUI::Element* peFrom, DirectUI::Event* pEvent) override {}
	//~ End DirectUI::IElementListener Interface

	T* _pe;

private:
	T** operator&() = delete;
};

template <typename T>
class CSafeElementPtr : public CSafeElementPtrBase<T>
{
	CSafeElementPtr(T* pe)
	{
		this->_Assign(pe); // Function body assumed
	}

	CSafeElementPtr(const CSafeElementPtr&) = delete;

public:
	CSafeElementPtr()
	{
	}

	CSafeElementPtr(int)
	{
		// Function body unknown
	}

	~CSafeElementPtr()
	{
		Unassign();
	}

	HRESULT Assign(T* pe)
	{
		Unassign();
		return this->_Assign(pe);
	}

	void Unassign()
	{
		if (this->_pe)
		{
			this->_pe->RemoveListener(this);
			this->_pe = nullptr;
		}
	}

	void DestroyAndUnassign()
	{
		if (this->_pe)
		{
			this->_pe->Destroy(true);
		}
		Unassign();
	}

	CSafeElementPtr* AddressOfClass()
	{
		return this;
	}
};

template <typename T>
class CSafeElementListenerPtr : public CSafeElementPtrBase<T>
{
protected:
	enum CALLBACK_STATE
	{
		CBS_NONE = 0,
		CBS_INCALLBACK = 1,
		CBS_INDETACH = 2,
	};

private:
	CSafeElementListenerPtr(T*) = delete;
	CSafeElementListenerPtr(const CSafeElementListenerPtr&) = delete;

public:
	CSafeElementListenerPtr()
		: _pselcbOwner(nullptr)
		, _cbs(CBS_NONE)
	{
	}

	// ReSharper disable once CppPossiblyUninitializedMember
	CSafeElementListenerPtr(int)
	{
	}

	~CSafeElementListenerPtr()
	{
		ReleaseListener();
	}

	HRESULT AssignListener(T* pe, CSafeElementListenerCB* pselcbOwner)
	{
		HRESULT hr = E_FAIL;

		if (_cbs == CBS_NONE)
		{
			ReleaseListener();
			_pselcbOwner = pselcbOwner;
			hr = this->_Assign(pe);
			if (FAILED(hr))
			{
				_pselcbOwner = nullptr;
			}
		}

		return hr;
	}

	void ReleaseListener()
	{
		if (_cbs == CBS_NONE)
		{
			if (this->_pe)
			{
				this->_pe->RemoveListener(this);
				if (this->_pe)
				{
					OnListenerDetach(this->_pe);
				}
			}
		}
		else
		{
			if (_cbs == CBS_INDETACH)
			{
				this->_pe = nullptr;
			}
			_pselcbOwner = nullptr;
		}
	}

protected:
	//~ Begin DirectUI::IElementListener Interface
	void OnListenerAttach(DirectUI::Element* peFrom) override
	{
		if (_pselcbOwner)
		{
			_cbs = CBS_INCALLBACK;
			_pselcbOwner->OnListenerAttach(peFrom);
			_cbs = CBS_NONE;
		}
	}

	void OnListenerDetach(DirectUI::Element* peFrom) override
	{
		if (this->_pe)
		{
			this->_pe = nullptr;
		}
	}

	bool OnListenedPropertyChanging(DirectUI::Element* peFrom, const DirectUI::PropertyInfo* ppi, int iIndex, DirectUI::Value* pvOld, DirectUI::Value* pvNew) override
	{
		bool fRet = false;

		if (_pselcbOwner)
		{
			_cbs = CBS_INCALLBACK;
			fRet = _pselcbOwner->OnListenedPropertyChanging(peFrom, ppi, iIndex, pvOld, pvNew);
			_cbs = CBS_NONE;
		}

		return fRet;
	}

	void OnListenedPropertyChanged(DirectUI::Element* peFrom, const DirectUI::PropertyInfo* ppi, int iIndex, DirectUI::Value* pvOld, DirectUI::Value* pvNew) override
	{
		if (_pselcbOwner)
		{
			_cbs = CBS_INCALLBACK;
			_pselcbOwner->OnListenedPropertyChanged(peFrom, ppi, iIndex, pvOld, pvNew);
			_cbs = CBS_NONE;
		}
	}

	void OnListenedInput(DirectUI::Element* peFrom, DirectUI::InputEvent* pInput) override
	{
		if (_pselcbOwner)
		{
			_cbs = CBS_INCALLBACK;
			_pselcbOwner->OnListenedInput(peFrom, pInput);
			_cbs = CBS_NONE;
		}
	}

	void OnListenedEvent(DirectUI::Element* peFrom, DirectUI::Event* pEvent) override
	{
		if (_pselcbOwner)
		{
			_cbs = CBS_INCALLBACK;
			_pselcbOwner->OnListenedEvent(peFrom, pEvent);
			_cbs = CBS_NONE;
		}
	}
	//~ End DirectUI::IElementListener Interface

	CSafeElementListenerCB* _pselcbOwner;
	CALLBACK_STATE _cbs;
};
