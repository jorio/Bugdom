// TWEEN.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include <SDL3/SDL.h>
#include "tween.h"

#define _USE_MATH_DEFINES  // For MSVC/Win32
#include <math.h>

void TweenFloats(
		float(*easingFunction)(float),
		float elapsed,
		float duration,
		int nFloats,
		float* current,
		const float* start,
		const float* target)
{
	if (elapsed <= 0)
	{
		SDL_memcpy(current, start, nFloats*sizeof(float));
	}
	else if (elapsed >= duration)
	{
		SDL_memcpy(current, target, nFloats*sizeof(float));
	}
	else
	{
		float x = elapsed/duration;
		float y = easingFunction(x);
		for (int i = 0; i < nFloats; i++)
			current[i] = (1-y)*start[i] + y*target[i];
	}
}

float TweenFloat(
		float(*easingFunction)(float),
		float elapsed,
		float duration,
		float start,
		float end)
{
	float value = 0;
	TweenFloats(easingFunction, elapsed, duration, 1, &value, &start, &end);
	return value;
}

TQ3Vector3D TweenTQ3Vector3D(
		float(*easingFunction)(float),
		float elapsed,
		float duration,
		TQ3Vector3D start,
		TQ3Vector3D end)
{
	TQ3Vector3D value;
	TweenFloats(easingFunction, elapsed, duration, 3, &value.x, &start.x, &end.x);
	return value;
}

TQ3Point3D TweenTQ3Point3D(
		float(*easingFunction)(float),
		float elapsed,
		float duration,
		TQ3Point3D start,
		TQ3Point3D end)
{
	TQ3Point3D value;
	TweenFloats(easingFunction, elapsed, duration, 3, &value.x, &start.x, &end.x);
	return value;
}

static void TweenTowardsFloats(
		float elapsed,
		float duration,
		int nFloats,
		float* current,
		const float* target)
{
	TweenFloats(
			EaseLerp,
			elapsed,
			duration,
			nFloats,
			current,
			current,
			target
			);
}

void TweenTowardsTQ3Vector3D(
		float elapsed,
		float duration,
		TQ3Vector3D* current,
		const TQ3Vector3D target)
{
	TweenTowardsFloats(elapsed, duration, 3, &current->x, &target.x);
}

void TweenTowardsTQ3Point3D(
		float elapsed,
		float duration,
		TQ3Point3D* current,
		const TQ3Point3D target)
{
	TweenTowardsFloats(elapsed, duration, 3, &current->x, &target.x);
}

void TweenTowardsFloat(
		float elapsed,
		float duration,
		float* current,
		float target)
{
	TweenTowardsFloats(elapsed, duration, 1, current, &target);
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

float EaseOutExpo(float x)
{
	return x == 1
		? 1
		: 1 - powf(2, -10 * x);
}
