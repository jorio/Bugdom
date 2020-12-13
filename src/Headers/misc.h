//
// misc.h
//

extern	void ShowSystemErr(long err);
extern void	DoAlert(const char*);
extern void	DoFatalAlert(const char*);
extern	void DoFatalAlert2(const char* s1, const char* s2);
void DoAssert(const char* msg, const char* file, int line);
void Wait(u_long ticks);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	Handle	AllocHandle(long size);
extern	Ptr	AllocPtr(long size);
extern	void** Alloc2DArray(int sizeofType, int n, int m);
extern	void Free2DArray(void** array);
extern	void InitMyRandomSeed(void);
extern	void VerifySystem(void);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
extern	void RegulateSpeed(short fps);
extern	void ShowSystemErr_NonFatal(long err);
extern	void ApplyFrictionToDeltas(float f,TQ3Vector3D *d);
