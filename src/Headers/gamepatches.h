#pragma once

#ifdef __cplusplus
extern "C" {
#endif

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec);

TQ3Area GetAdjustedPane(int windowWidth, int windowHeight, Rect paneClip);

void DoSDLMaintenance(void);

#ifdef __cplusplus
}
#endif
