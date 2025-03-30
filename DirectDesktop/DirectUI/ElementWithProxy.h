#pragma once

struct ISafeElementProxyInvokeHelper
{
	virtual void DoInvoke() = 0;

	virtual ~ISafeElementProxyInvokeHelper() = default;
};

struct ProxyInvokeEvent : EventMsg
{
	ISafeElementProxyInvokeHelper* pInvokeHelper;
};

class CSafeElementProxy
{
public:
	ULONG AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	ULONG Release()
	{
		ULONG cRef = InterlockedDecrement(&_cRef);
		if (cRef == 0 && this)
		{
			delete this;
		}
		return cRef;
	}

	UILIB_API static HRESULT WINAPI CreateInstance(DirectUI::Element* pe, CSafeElementProxy** ppProxy);
	UILIB_API void Detach();

	UILIB_API static HRESULT CALLBACK s_SyncCallback(HGADGET, void*, EventMsg* pmsg);

	template <typename T>
	HRESULT GetAttachedElement(T** ppElement) const
	{
		return ElementCast<T>(_speAttached, ppElement);
	}

	template <typename TFuncInvoke>
	class InvokeHelper final : public ISafeElementProxyInvokeHelper
	{
	public:
		InvokeHelper(const TFuncInvoke& funcInvoke, CSafeElementProxy* pProxy, HANDLE hCompletionEvent)
			: _funcInvoke(funcInvoke)
			, _pProxy(pProxy)
			, _hCompletionEvent(hCompletionEvent)
		{
			pProxy->AddRef();
		}

		InvokeHelper(const InvokeHelper& other) = delete;

		~InvokeHelper() override
		{
			_pProxy->Release();
		}

		void DoInvoke() override
		{
			_funcInvoke(_pProxy);
			if (_hCompletionEvent)
			{
				SetEvent(_hCompletionEvent);
			}
		}

	private:
		TFuncInvoke _funcInvoke;
		CSafeElementProxy* _pProxy;
		HANDLE _hCompletionEvent;
	};

	template <typename TFunc>
	HRESULT Invoke(const TFunc& func)
	{
		HRESULT hr;

		if (GetCurrentThreadId() == _dwUIThreadId)
		{
			if (_hgSync)
			{
				InvokeHelper<TFunc> invokeHelper(func, this, nullptr);

				ProxyInvokeEvent msg;
				msg.cbSize = sizeof(msg);
				msg.nMsg = 0x8400;
				msg.hgadMsg = _hgSync;
				msg.nMsgFlags = 0;
				msg.pInvokeHelper = &invokeHelper;
				hr = DUserSendEvent(&msg, 0) == 0x40001 ? S_OK : E_FAIL;
			}
			else
			{
				hr = E_FAIL;
			}
		}
		else
		{
			HANDLE hEvent = CreateEventExW(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
			if (hEvent)
			{
				hr = InvokeAsync(func, hEvent);
				if (SUCCEEDED(hr))
				{
					hr = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0 ? S_OK : E_UNEXPECTED;
				}

				CloseHandle(hEvent);
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}

		return hr;
	}

	template <typename TFunc>
	HRESULT InvokeAsync(const TFunc& func, HANDLE hCompletionEvent)
	{
		AcquireSRWLockShared(&_srwlock);
		HRESULT hr = _hgSync ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			ISafeElementProxyInvokeHelper* pInvokeHelper = new(std::nothrow) InvokeHelper<TFunc>(func, this, hCompletionEvent);
			hr = pInvokeHelper ? S_OK : E_OUTOFMEMORY;
			if (SUCCEEDED(hr))
			{
				HDCONTEXT hCtx;
				hr = _InitDUserContext(&hCtx);
				if (SUCCEEDED(hr))
				{
					ProxyInvokeEvent msg;
					msg.cbSize = sizeof(msg);
					msg.nMsg = 0x8401;
					msg.hgadMsg = _hgSync;
					msg.nMsgFlags = 0;
					msg.pInvokeHelper = pInvokeHelper;
					hr = DUserPostEvent(&msg, 0x2);
					if (FAILED(hr))
					{
						delete pInvokeHelper;
					}

					DeleteHandle(hCtx);
				}
			}
		}

		ReleaseSRWLockShared(&_srwlock);
		return hr;
	}

protected:
	~CSafeElementProxy()
	{
	}

public:
	CSafeElementProxy(const CSafeElementProxy&) = delete;

protected:
	CSafeElementProxy()
		: _hgSync(nullptr)
		, _cRef(1)
		, _dwUIThreadId(GetCurrentThreadId())
	{
		InitializeSRWLock(&_srwlock);
	}

	UILIB_API HRESULT Initialize(DirectUI::Element*);

private:
	HRESULT _InitDUserContext(HDCONTEXT* phCtx)
	{
		INITGADGET ig;
		ig.cbSize = sizeof(ig);
		ig.nThreadMode = 2;
		ig.nMsgMode = 3;
		ig.nPerfMode = 0;
		ig.hctxShare = nullptr;
		*phCtx = InitGadgets(&ig);
		return *phCtx ? S_OK : E_FAIL;
	}

	void _UnInitDUserContext(HDCONTEXT hCtx)
	{
		DeleteHandle(hCtx);
	}

	CSafeElementPtr<DirectUI::Element> _speAttached;
	HGADGET _hgSync;
	ULONG _cRef;
	DWORD _dwUIThreadId;
	RTL_SRWLOCK _srwlock;
};

template <typename T>
class CElementWithProxy : public T
{
public:
	CElementWithProxy()
		: _hrInit(HRESULT_FROM_WIN32(ERROR_NOT_READY))
		, _pProxy(nullptr)
	{
	}

	CElementWithProxy(const CElementWithProxy&) = default;

	~CElementWithProxy()
	{
		if (_pProxy)
		{
			_pProxy->Detach();
			_pProxy->Release();
		}
	}

protected:
	HRESULT _EnsureInitializedProxy()
	{
		if (_hrInit == HRESULT_FROM_WIN32(ERROR_NOT_READY))
		{
			_hrInit = CSafeElementProxy::CreateInstance(this, &_pProxy);
		}

		return _hrInit;
	}

	CSafeElementProxy* _GetProxy() const
	{
		return _pProxy;
	}

private:
	HRESULT _hrInit;
	CSafeElementProxy* _pProxy;
};
