#pragma once

//-----------------------------------------------------------------------------
// Kill and restore macOS mouse acceleration

#if __APPLE__

#if __cplusplus
extern "C"
{
#endif

void KillMacMouseAcceleration(void);

void RestoreMacMouseAcceleration(void);

#if __cplusplus
};
#endif

#endif
