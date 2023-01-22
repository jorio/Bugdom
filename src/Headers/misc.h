//
// misc.h
//

void DoAlert(const char* format, ...);
POMME_NORETURN void DoFatalAlert(const char* format, ...);
POMME_NORETURN void CleanQuit(void);

#define AllocHandle(size) NewHandleClear((size))
#define AllocPtr(size) NewPtrClear((size))
void** Alloc2DArray(int sizeofType, int n, int m);
void Free2DArray(void** array);

void SetMyRandomSeed(uint32_t seed);
uint32_t MyRandomLong(void);
float RandomFloat(void);

void VerifySystem(void);
void ApplyFrictionToDeltas(float f,TQ3Vector3D *d);

static inline uint32_t POTCeil32(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

#define GAME_ASSERT(condition) \
	do { \
		if (!(condition)) \
			DoFatalAlert("Assertion failed! %s:%d: %s", __func__, __LINE__, #condition); \
	} while(0)

#define GAME_ASSERT_MESSAGE(condition, message) \
	do { \
		if (!(condition)) \
			DoFatalAlert("Assertion failed! %s:%d: %s", __func__, __LINE__, message); \
	} while(0)
