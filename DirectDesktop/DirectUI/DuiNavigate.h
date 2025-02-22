#pragma once

namespace DirectUI
{
	class UILIB_API DuiNavigate
	{
	public:
		static Element* WINAPI Navigate(Element* peFrom, DynamicArray<Element*>* pelConsider, int nNavDir);
	};
}
