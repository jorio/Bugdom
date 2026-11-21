#pragma once

void WebClearPresses(void);
void WebSyncFiles(void);
void WebGetMovement(float* x, float* y);
void WebConsumeCamera(float* yaw, float* pitch, float* zoom);
int WebGetClick(void);
int WebGetPointer(float* x, float* y);
void WebPrepareFrame(void);
void WebGetSafeArea(float* top, float* right, float* bottom, float* left);

// Stable action IDs shared with controls.js. Append new actions; do not reorder.
enum { WEB_UP, WEB_DOWN, WEB_LEFT, WEB_RIGHT, WEB_JUMP, WEB_KICK,
    WEB_ROLL, WEB_BUDDY, WEB_PAUSE, WEB_CONFIRM, WEB_CAMERA_LEFT,
    WEB_CAMERA_RIGHT, WEB_ZOOM_IN, WEB_ZOOM_OUT, WEB_PICKUP, WEB_JET,
    WEB_NEXT_WEAPON, WEB_PREV_WEAPON, WEB_CAMERA_MODE, WEB_BUTTON_COUNT };
int WebGetButton(int button);
// True once after the page asked for a pause (e.g. the tab was hidden).
int WebConsumePauseRequest(void);
void WebPresent(int playing, int paused);
// Implemented by the small engine adapter in each game.
void WebGameResize(int width, int height);
