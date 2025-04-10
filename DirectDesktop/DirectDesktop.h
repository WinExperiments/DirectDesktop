#pragma once
#pragma warning(disable:28159)

#include "resource.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#define DESKPADDING_NORMAL 4
#define DESKPADDING_TOUCH 16 