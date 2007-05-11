//
// misc.h
//

        /* REGISTRATION */


#define REG_LENGTH      12



extern	void ShowSystemErr(long err);
extern void	DoAlert(Str255);
extern void	DoFatalAlert(Str255);
void Wait(u_long ticks);
extern unsigned char	*NumToHex(unsigned short);
extern unsigned char	*NumToHex2(unsigned long, short);
extern unsigned char	*NumToDec(unsigned long);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	void FloatToString(float num, Str255 string);
extern	Handle	AllocHandle(long size);
extern	Ptr	AllocPtr(long size);
extern	void PStringToC(char *pString, char *cString);
extern	void DrawCString(char *string);
extern	void InitMyRandomSeed(void);
extern	void VerifySystem(void);
extern	void DoFatalAlert2(Str255 s1, Str255 s2);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
extern	void RegulateSpeed(short fps);
extern	void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr);
extern	void ShowSystemErr_NonFatal(long err);
extern	void ApplyFrictionToDeltas(float f,TQ3Vector3D *d);
void DoAlertNum(int n);
void CheckGameRegistration(void);








