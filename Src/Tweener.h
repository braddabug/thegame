#ifndef TWEENER_H
#define TWEENER_H

#include <cmath>
#include "MyNxna2.h"

class Tweener
{
public:
	// these formulas stolen from
	// http://libclaw.sourceforge.net/tweeners.html

	static float EaseIn(float t)
	{
		return t * t *  (2.70158f * t - 1.70158f);
	}

	static float EaseOut(float t)
	{
		return 1.0f - EaseIn(1.0f - t);
	}

	static float ElasticIn(float t)
	{
		return -(float)pow(2.0f, 10.0f * (t - 1.0f)) * (float)sin(((t - 1.0f) - 0.3f / 4.0f) * 2.0f * Nxna::Pi / 0.3f);
	}

	static float ElasticOut(float t)
	{
		return 1.0f - ElasticIn(1.0f - t);
	}

	static float SpringIn(float t)
	{
		return (float)sin(t * 5.0f * Nxna::Pi) * (float)pow(1.0f, (1.0f - t)) * (1.0f - t);

		//return -(float)Math.Pow(2.0f, 10.0f * (t - 1.0f)) * (float)Math.Sin(((t - 1.0f) - 0.6f / 4.0f) * 2.0f * MathHelper.Pi / 0.3f);
	}

	static float SpringOut(float t)
	{
		return SpringIn(t);
		//return 1.0f - SpringIn(1.0f - t);
	}

	static float BounceIn(float t)
	{
		if (1.0f - t < 1.0f / 2.75f)
			return 1.0f - 7.5625f * (1.0f - t) * (1.0f - t);
		if (1.0f - t < 2.0f / 2.75f)
			return 1.0f - (7.5625f * (1.0f - t - 1.5f / 2.75f) * (1.0f - t - 1.5f / 2.75f) + 0.75f);
		if (1.0f - t < 2.5f / 2.75f)
			return 1.0f - (7.5625f * (1.0f - t - 2.25f / 2.75f) * (1.0f - t - 2.25f / 2.75f) + 0.9375f);

		return 1.0f - (7.5625f * (1.0f - t - 2.625f / 2.75f) * (1.0f - t - 2.625f / 2.75f) + 0.984375f);
	}

	static float BounceOut(float t)
	{
		return 1.0f - BounceIn(1.0f - t);
	}

	static float CubicIn(float t)
	{
		return t * t * t;
	}

	static float CubicOut(float t)
	{
		return 1.0f - CubicIn(1.0f - t);
	}

	static float CubicInOut(float t)
	{
		if (t < 0.5f)
			return CubicIn(t * 2.0f) * 0.5f;

		return 0.5f + CubicOut(t * 2.0f - 1.0f) * 0.5f;
	}
};

#endif // TWEENER_H