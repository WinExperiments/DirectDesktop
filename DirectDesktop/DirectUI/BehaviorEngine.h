#pragma once

namespace DirectUI
{
	class DECLSPEC_UUID("751ef5ec-0791-440d-bfaf-d5dd8ae60b63")
	BehaviorEngine;
}

MIDL_INTERFACE("230aa8d1-9b2e-4074-8880-6a7f6125dccb")
IDuiBehaviorFactory : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE CreateBehavior(const WCHAR*, IDuiBehavior**) = 0;
};

MIDL_INTERFACE("e0857125-39fc-4f59-a56d-0e8a82902f3a")
IDuiBehaviorEngine : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE RegisterBehaviorFactory(const WCHAR* pwzNamespace, IDuiBehaviorFactory* pFactory) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterBehaviorFactory(const WCHAR* pwzNamespace, IDuiBehaviorFactory* pFactory) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateBehavior(const WCHAR* pwzNamespace, const WCHAR* pwzName, DirectUI::Value* pvArgs, IDuiBehavior** ppBehavior) = 0;
};
