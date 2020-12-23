//
// 3dmf.h
//

#include "qd3d_support.h"




//====================================

extern	void Init3DMFManager(void);
extern	TQ3Object	Load3DMFModel(const FSSpec *inFile);
extern	void Save3DMFModel(QD3DSetupOutputType *setupInfo,FSSpec *outFile, void (*callBack)(QD3DSetupOutputType *));
extern	void LoadGrouped3DMF(FSSpec *spec, Byte groupNum);
extern	void Free3DMFGroup(Byte groupNum);
extern	void DeleteAll3DMFGroups(void);


