#pragma once

#include "framework.h"
#include "resource.h"

#ifdef MODERNSHELLABOUT_EXPORTS
#define MOSHELLABOUTAPI __declspec(dllexport)
#else
#define MOSHELLABOUTAPI __declspec(dllimport)
#endif

void MOSHELLABOUTAPI SHDummyFunc();