#pragma once

#include <Quesa.h>

float TweenFloat(
		float(*easingFunction)(float),
		float elapsed,
		float duration,
		float start,
		float end);

TQ3Vector3D TweenTQ3Vector3D(
		float(*easingFunction)(float),
		float elapsed,
		float duration,
		TQ3Vector3D start,
		TQ3Vector3D target);

TQ3Point3D TweenTQ3Point3D(
		float(*easingFunction)(float),
		float elapsed,
		float duration,
		TQ3Point3D start,
		TQ3Point3D target);

void TweenTowardsTQ3Vector3D(
		float elapsed,
		float duration,
		TQ3Vector3D* current,
		TQ3Vector3D target);

void TweenTowardsTQ3Point3D(
		float elapsed,
		float duration,
		TQ3Point3D* current,
		TQ3Point3D target);

void TweenTowardsFloat(
		float elapsed,
		float duration,
		float* current,
		float target);

float EaseLerp		(float x);
float EaseInSine	(float x);
float EaseOutSine	(float x);
float EaseInOutSine	(float x);
float EaseInQuad	(float x);
float EaseOutQuad	(float x);
float EaseInOutQuad	(float x);
float EaseOutExpo	(float x);