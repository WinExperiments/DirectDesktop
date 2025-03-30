#pragma once

namespace DirectUI
{
	class PropNotify
	{
		PropNotify() = delete;
		PropNotify(const PropNotify&) = delete;

	public:
		PropNotify(const PropertyInfo* ppiNotify, const int iIndexNotify)
			: _ppiNotify(ppiNotify)
			, _iIndexNotify(iIndexNotify)
		{
		}

		bool IsEqual(const PropertyInfo* ppiCompare)
		{
			return IsEqual(_ppiNotify, _iIndexNotify, ppiCompare);
		}

		bool IsEqual(PropertyProcT pPropProcCompare)
		{
			return IsEqual(pPropProcCompare());
		}

		static bool IsEqual(const PropertyInfo* ppiNotify, int iIndexNotify, const PropertyInfo* ppiCompare)
		{
			return ppiNotify == ppiCompare && (ppiNotify->fFlags & 0x3) == iIndexNotify;
		}

		static bool IsEqual(const PropertyInfo* ppiNotify, int iIndexNotify, PropertyProcT pPropProcCompare)
		{
			return IsEqual(ppiNotify, iIndexNotify, pPropProcCompare());
		}

	private:
		void operator=(const PropNotify&) = delete;

		void* operator new(size_t); // Unknown
		void* operator new[](size_t); // Unknown

		const PropertyInfo* _ppiNotify;
		const int _iIndexNotify;
	};
}
