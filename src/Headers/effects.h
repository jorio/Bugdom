//
// effects.h
//

enum
{
	PARTICLE_TYPE_FALLINGSPARKS,
	PARTICLE_TYPE_GRAVITOIDS
};

enum
{
	PARTICLE_FLAGS_BOUNCE = (1<<0),
	PARTICLE_FLAGS_HURTPLAYER = (1<<1),
	PARTICLE_FLAGS_HURTPLAYERBAD = (1<<2),	//combine with PARTICLE_FLAGS_HURTPLAYER
	PARTICLE_FLAGS_HURTENEMY = (1<<3),
	PARTICLE_FLAGS_ROOF = (1<<4),
	PARTICLE_FLAGS_EXTINGUISH = (1<<5),
	PARTICLE_FLAGS_HOT = (1<<6)
};

enum
{
	PARTICLE_TEXTURE_FIRE,
	PARTICLE_TEXTURE_WHITE,
	PARTICLE_TEXTURE_PATCHY,
	PARTICLE_TEXTURE_DOTS,
	PARTICLE_TEXTURE_ORANGESPOT,
	PARTICLE_TEXTURE_BLUEFIRE,
	PARTICLE_TEXTURE_GREENRING,
	PARTICLE_TEXTURE_YELLOWBALL
};


#define	FULL_ALPHA	1.0f

ObjNode *MakeRipple(float x, float y, float z, float startScale);

void InitParticleSystem(void);
void DeleteAllParticleGroups(void);
int32_t NewParticleGroup(Byte type, uint8_t flags, float gravity, float magnetism, float baseScale, float decayRate, float fadeRate, Byte particleTextureNum);
bool VerifyParticleGroup(int32_t groupID);
bool AddParticleToGroup(int32_t groupID, TQ3Point3D* where, TQ3Vector3D* delta, float scale, float alpha);
void MoveParticleGroups(void);
void DrawParticleGroup(const QD3DSetupOutputType *setupInfo);
bool ParticleHitObject(ObjNode *theNode, uint8_t inFlags);

void MakeSplash(float x, float y, float z, float force, float volume);
