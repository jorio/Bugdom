#include "tween.h"
#include "math.h"

float TweenFloat(float(*easingFunction)(float), float elapsed, float duration, float start, float end)
{
	if (elapsed <= 0) return start;
	if (elapsed >= duration) return end;
	float x = elapsed/duration;
	float y = easingFunction(x);
	return (1-y)*start + y*end;
}

float EaseLerp(float x)
{
	return x;
}

float EaseInSine(float x)
{
	return 1 - cosf(x * M_PI_2);
}

float EaseOutSine(float x)
{
	return sinf(x * M_PI_2);
}

float EaseInOutSine(float x)
{
	return -(cosf(M_PI*x) - 1) / 2;
}

float EaseInQuad(float x)
{
	return x*x;
}

float EaseOutQuad(float x)
{
	return 1 - (1-x) * (1-x);
}

float EaseInOutQuad(float x)
{
	return x < 0.5f
		? 2*x*x
		: 1-powf(-2*x + 2, 2) / 2;
}
