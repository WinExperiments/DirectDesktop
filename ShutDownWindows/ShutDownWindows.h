#pragma once

#ifdef SHUTDOWNWINDOWS_EXPORTS
#define SHUTDOWNWINDOWAPI __declspec(dllexport)
#else
#define SHUTDOWNWINDOWAPI __declspec(dllimport)
#endif

void SHUTDOWNWINDOWAPI SDDummyFunc();