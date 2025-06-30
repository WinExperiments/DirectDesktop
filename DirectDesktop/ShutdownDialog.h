#pragma once
#pragma warning(disable:6258)
#pragma warning(disable:28159)

#include "framework.h"

#include "Include\dui70\DirectUI\DirectUI.h"

#include "StyleModifier.h"

#include <string>
#include <strsafe.h>

using namespace std;
using namespace DirectUI;

namespace DirectDesktop
{
	extern DWORD shutdownReason;
	extern NativeHWNDHost* wnd;
	extern NativeHWNDHost* shutdownwnd;
	extern int dpi, dpiLaunch;
	extern bool dialogopen;
	extern bool theme;
	extern bool delayedshutdownstatuses[6];

	void DisplayShutdownDialog();
	void DestroyShutdownDialog();
}