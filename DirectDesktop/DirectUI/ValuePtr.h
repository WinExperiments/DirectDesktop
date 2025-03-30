#pragma once

class CValuePtr
{
public:
	CValuePtr() : _pv(nullptr)
	{
	}

	// ReSharper disable once CppPossiblyUninitializedMember
	CValuePtr(int)
	{
	}

	CValuePtr(DirectUI::Value* pv) : _pv(nullptr)
	{
		Attach(pv);
	}

	CValuePtr(const CValuePtr& other) : _pv(nullptr)
	{
		Attach(other);
	}

	~CValuePtr()
	{
		Release();
	}

	operator DirectUI::Value*() const { return _pv; }
	DirectUI::Value** operator&() { return &_pv; }
	DirectUI::Value* operator->() const { return _pv; }
	bool operator!() const { return !_pv; }
	bool operator!=(DirectUI::Value* pv) const { return pv != _pv; }
	bool operator==(DirectUI::Value* pv) const { return pv == _pv; }

private:
	DirectUI::Value* operator=(DirectUI::Value* pValue)
	{
		Attach(pValue);
		return _pv;
	}

public:
	DirectUI::Value* operator=(const CValuePtr& other)
	{
		Attach(other);
		return _pv;
	}

	void Release()
	{
		if (_pv)
		{
			DirectUI::Value* pvTemp = _pv;
			_pv = nullptr;
			pvTemp->Release();
		}
	}

	void AttachAndAddRef(DirectUI::Value* pv)
	{
		Release();
		_pv = pv;
		if (pv)
		{
			pv->AddRef();
		}
	}

private:
	void Attach(const CValuePtr& other)
	{
		Attach(other._pv);
	}

public:
	void Attach(DirectUI::Value* pvNew)
	{
		Release();
		_pv = pvNew;
	}

	DirectUI::Value* Detach()
	{
		DirectUI::Value* pvTemp = _pv;
		_pv = nullptr;
		return pvTemp;
	}

protected:
	DirectUI::Value* _pv;
};
