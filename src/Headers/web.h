#pragma once

int WebGetKey(int key);
void WebClearPresses(void);
void WebEndFrame(void);
void WebSyncFiles(void);
void WebGetMovement(float* x, float* y);
void WebConsumeCamera(float* yaw, float* pitch, float* zoom);
int WebGetClick(void);
int WebGetPointer(float* x, float* y);
void WebPrepareFrame(void);
void WebGetSafeArea(float* top, float* right, float* bottom, float* left);
