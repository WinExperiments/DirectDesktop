#pragma once

namespace DirectUI
{
	class UILIB_API ToggleProvider
		: public PatternProvider<ToggleProvider, IToggleProvider, Schema::Pattern::Toggle>
		, public IToggleProvider
	{
	public:
		ToggleProvider(void);
		virtual ~ToggleProvider(void);
		virtual unsigned long __stdcall AddRef(void);
		virtual PfnCreateProxy GetProxyCreator(void);
		virtual long __stdcall QueryInterface(GUID const &, void * *);
		virtual unsigned long __stdcall Release(void);
		virtual long __stdcall Toggle(void);
		virtual long __stdcall get_ToggleState(ToggleState *);
	};

	class UILIB_API ToggleProxy : public ProviderProxy
	{
	public:
		ToggleProxy(ToggleProxy const &);
		ToggleProxy(void);
		ToggleProxy & operator=(ToggleProxy const &);
		static ToggleProxy * __stdcall Create(Element *);
		static bool __stdcall IsPatternSupported(Element *);
		virtual long DoMethod(int, char *);

	protected:
		virtual void Init(Element *);

	private:
		long GetToggleState(ToggleState *);

	};
}