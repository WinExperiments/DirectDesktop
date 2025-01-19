#include "ColorHelper.h"

DWORD rgb2bgr(COLORREF color)
{
	return (color & 0xFF000000) | ((color & 0xFF0000) >> 16) | (color & 0x00FF00) | ((color & 0x0000FF) << 16);
}


// https://www.niwa.nu/2013/05/math-behind-colorspace-conversions-rgb-hsl/

hsl_t rgb2hsl(rgb_t in)
{
	hsl_t       out{};
	double      min, max, sigma, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max > in.b ? max : in.b;

	sigma = max + min;
	out.l = sigma / 2;
	delta = max - min;
	if (max > 0.0) { // NOTE: if max is == 0, this divide would cause a crash
		if (out.l <= 0.5)
		{
			out.s = delta / sigma;
		}
		if (out.l > 0.5)
		{
			out.s = delta / (2 - sigma);
		}
		/* if (out.s > 1)
		{
			out.s = out.s - (out.s - 1);
		}
		if (out.s < 0)
		{
			out.s = (0 - out.s);
		} */
	}
	else {
		// if max is 0, then r = g = b = 0   
		out.h = 210.0;
		out.s = 0.0;
		return out;
	}
	if (in.r >= max)                           // ">" is useless, just keeps compiler happy
		out.h = (((in.g - in.b) / 6) / delta) * 360.0;        // between yellow & magenta
	else
		if (in.g >= max)
			out.h = ((1.0 / 3.0) + ((in.b - in.r) / 6) / delta) * 360.0;  // between cyan & yellow
		else
			out.h = ((2.0 / 3.0) + ((in.r - in.g) / 6) / delta) * 360.0;  // between magenta & cyan

	if (out.h < 0.0)
		out.h += 360.0;

	if (out.h > 360.0)
		out.h -= 360.0;

	return out;
}

rgb_t hsl2rgb(hsl_t in)
{
	double      ot{}, tt{}, ht{}, rt{}, gt{}, bt{}; // temporary values one, two, hue, red, green, blue
	rgb_t       out{};

	/*if (in.s <= 0.0) {       // "<" is useless, just shuts up warnings
		out.r = in.l;
		out.g = in.l;
		out.b = in.l;
		return out;
	}*/ // this was making saturation always return 0
	if (in.l < 0.5)
	{
		ot = in.l * (1.0 + in.s);
	}
	else if (in.l >= 0.5)
	{
		ot = in.l + in.s - (in.l * in.s);
	}
	tt = (2 * in.l) - ot;
	ht = in.h / 360.0;
	if (ht > 0.6666667) {
		rt = ht - (0.6666667);
	}
	if (ht <= 0.6666667) {
		rt = ht + (0.3333333);
	}
	gt = ht;
	if (ht < 0.3333333) {
		bt = ht + (0.6666667);
	}
	if (ht >= 0.3333333) {
		bt = ht - (0.3333333);
	}
	if (rt * 6 <= 1) {
		out.r = tt + (ot - tt) * 6 * rt;
	}
	else if (rt * 2 <= 1) {
		out.r = ot;
	}
	else if (rt * 3 <= 2) {
		out.r = tt + (ot - tt) * (0.6666667 - rt) * 6;
	}
	else
		out.r = tt;
	if (gt * 6 <= 1) {
		out.g = tt + (ot - tt) * 6 * gt;
	}
	else if (gt * 2 <= 1) {
		out.g = ot;
	}
	else if (gt * 3 <= 2) {
		out.g = tt + (ot - tt) * (0.6666667 - gt) * 6;
	}
	else
		out.g = tt;
	if (bt * 6 <= 1) {
		out.b = tt + (ot - tt) * 6 * bt;
	}
	else if (bt * 2 <= 1) {
		out.b = ot;
	}
	else if (bt * 3 <= 2) {
		out.b = tt + (ot - tt) * (0.6666667 - bt) * 6;
	}
	else
		out.b = tt;

	if (out.r > 255) {
		out.r = out.r - (out.r - 255);
	}
	if (out.g > 255) {
		out.g = out.g - (out.g - 255);
	}
	if (out.b > 255) {
		out.b = out.b - (out.b - 255);
	}

	if (out.r < 0) {
		out.r = (0 - out.r);
	}
	if (out.g < 0) {
		out.g = (0 - out.g);
	}
	if (out.b < 0) {
		out.b = (0 - out.b);
	}

	return out;
}