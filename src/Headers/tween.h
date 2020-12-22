#pragma once

float TweenFloat(float(*easingFunction)(float), float elapsed, float duration, float start, float end);

float EaseLerp		(float x);
float EaseInSine	(float x);
float EaseOutSine	(float x);
float EaseInOutSine	(float x);
float EaseInQuad	(float x);
float EaseOutQuad	(float x);
float EaseInOutQuad	(float x);
