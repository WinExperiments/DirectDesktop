#pragma once
#include "framework.h"

DWORD rgb2bgr(COLORREF rgb);

struct rgb_t
{
	double r{}, g{}, b{};
};

struct hsl_t
{
	double h{}, s{}, l{};
};

hsl_t rgb2hsl(rgb_t in);
rgb_t hsl2rgb(hsl_t in);