/****************************/
/*         MY GLOBALS       */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

#ifndef MYGLOBALS_H
#define MYGLOBALS_H


			/* SOME FLOATING POINT HELPERS */
			
#define EPS 1e-5					// a very small number which is useful for FP compares close to 0


#define	PICT_HEADER_SIZE	512
#define REMOVE_ALL_EVENTS	 0



		/*******************/
		/* 2D ARRAY MACROS */
		/*******************/

#define Alloc_2d_array(type, array, n, m)								\
{																		\
int _i;																	\
																		\
	array = (type **) AllocPtr((long)(n) * sizeof(type *));				\
	if (array == nil)													\
		DoFatalAlert("Alloc_2d_array failed!");						\
	array[0] = (type *) AllocPtr((long)(n) * (long)(m) * sizeof(type));	\
	if (array[0] == nil)												\
		DoFatalAlert("Alloc_2d_array failed!");						\
	for (_i = 1; _i < (n); _i++)										\
		array[_i] = array[_i-1] + (m);									\
}


#define Free_2d_array(array)			\
{										\
		DisposePtr((Ptr)array[0]);		\
		DisposePtr((Ptr)array);			\
		array = nil;					\
}


typedef	unsigned char u_char;
typedef	unsigned long u_long;
typedef	unsigned short u_short;


#define	PI					kQ3Pi	//3.141592654
#define PI2					(2.0f*PI)

															
							// COLLISION SIDE INFO
							//=================================
							
#define	SIDE_BITS_TOP		(1)							// %000001	(r/l/b/t)
#define	SIDE_BITS_BOTTOM	(1<<1)						// %000010
#define	SIDE_BITS_LEFT		(1<<2)						// %000100 
#define	SIDE_BITS_RIGHT		(1<<3)						// %001000
#define	SIDE_BITS_FRONT		(1<<4)						// %010000 
#define	SIDE_BITS_BACK		(1<<5)						// %100000
#define	ALL_SOLID_SIDES		(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT|\
							SIDE_BITS_FRONT|SIDE_BITS_BACK)


							// CBITS (32 BIT VALUES)
							//==================================

enum
{
	CBITS_TOP 			= SIDE_BITS_TOP,
	CBITS_BOTTOM 		= SIDE_BITS_BOTTOM,
	CBITS_LEFT 			= SIDE_BITS_LEFT,
	CBITS_RIGHT 		= SIDE_BITS_RIGHT,
	CBITS_FRONT 		= SIDE_BITS_FRONT,
	CBITS_BACK 			= SIDE_BITS_BACK,
	CBITS_ALLSOLID		= ALL_SOLID_SIDES,
	CBITS_NOTTOP		= SIDE_BITS_LEFT|SIDE_BITS_RIGHT|SIDE_BITS_FRONT|SIDE_BITS_BACK,
	CBITS_TOUCHABLE		= (1<<6)
};


							// CTYPES (32 BIT VALUES)
							//==================================

enum
{
	CTYPE_PLAYER	=	1,			// Me
	CTYPE_ENEMY		=	(1<<1),		// Enemy
	CTYPE_VISCOUS	=	(1<<2),		// Viscous object
	CTYPE_BONUS		=	(1<<3),		// Bonus item
	CTYPE_TRIGGER	=	(1<<4),		// Trigger
	CTYPE_SKELETON	=	(1<<5),		// Skeleton
	CTYPE_MISC		=	(1<<6),		// Misc
	CTYPE_BLOCKSHADOW =	(1<<7),		// Shadows go over it
	CTYPE_MPLATFORM	=	(1<<8),		// Moving Platform
	CTYPE_BGROUND2 	=	(1<<9),		// Collide against Terrain BGround 2 path tiles
	CTYPE_HURTME	= 	(1<<10),	// Hurt Me
	CTYPE_HURTENEMY	= 	(1<<11),	// Hurt Enemy
	CTYPE_BGROUND 	=	(1<<12),	// Terrain BGround path tiles
	CTYPE_PLAYERTRIGGERONLY	=	(1<<13),	// combined with _TRIGGER, this trigger is only triggerable by player
	CTYPE_SPIKED	=	(1<<14),	// Ball player cannot hit this
	CTYPE_KICKABLE 	=	(1<<15),	// if can be kicked by player
	CTYPE_AUTOTARGET =	(1<<16),	// if can be auto-targeted by player
	CTYPE_LIQUID	=	(1<<17),	// is a liquid patch
	CTYPE_BOPPABLE	=	(1<<18),	// enemy that can be bopped on top
	CTYPE_BLOCKCAMERA =	(1<<19),	// camera goes over this
	CTYPE_DRAINBALLTIME = (1<<20),	// drain ball time
	CTYPE_HURTNOKNOCK	= (1<<21),	// CTYPE_HURTME doesnt knock me down
	CTYPE_IMPENETRABLE	= (1<<22),	// set if object must have high collision priority and cannot be pushed thru this
	CTYPE_IMPENETRABLE2	= (1<<23),	// set with CTYPE_IMPENETRABLE if dont want player to do coord=oldCoord when touched
	CTYPE_AUTOTARGETJUMP = (1<<24)	// if auto target when jumping
};



							// OBJNODE STATUS BITS
							//==================================

enum
{
	STATUS_BIT_ONGROUND		=	1,			// Player is on anything solid (terrain or objnode)
	STATUS_BIT_ISCARRYING	=	(1<<1),		// Is Carrying Something
	STATUS_BIT_DONTCULL		=	(1<<2),		// set if don't want to perform custom culling on this object
	STATUS_BIT_NOCOLLISION  = 	(1<<3),		// set if want collision code to skip testing against this object
	STATUS_BIT_NOMOVE  		= 	(1<<4),		// dont call object's move function
	STATUS_BIT_ANIM  		= 	(1<<5),		// set if can animate
	STATUS_BIT_HIDDEN		=	(1<<6),		// dont draw object
	STATUS_BIT_REFLECTIONMAP = 	(1<<7),		// use reflection mapping
	STATUS_BIT_UNDERWATER	 = 	(1<<8),		// set if inside a water patch collision volume
	STATUS_BIT_ROTXZY		 = 	(1<<9),		// set if want to do x->z->y ordered rotation
	STATUS_BIT_ISCULLED		 = 	(1<<10),	// set if culling function deemed it culled
	STATUS_BIT_GLOW			=	(1<<11),	// use additive blending for glow effect
	STATUS_BIT_ONTERRAIN	=  (1<<12),		// also set with STATUS_BIT_ONGROUND if specifically on the terrain
	STATUS_BIT_NULLSHADER	 =  (1<<13),	// used when want to render object will NULL shading (no lighting)
	STATUS_BIT_ALWAYSCULL	 =  (1<<14),	// to force a cull-check
	STATUS_BIT_NOTRICACHE 	 =  (1<<15), 	// set if want to disable triangle caching when drawing this xparent obj
	STATUS_BIT_DELETE		=	(1<<16),	// set by a function which cannot actually delete an object but we want to tell system to delete it at its convenience
	STATUS_BIT_NOZWRITE		=	(1<<17),	// set when want to turn off z buffer writes
	STATUS_BIT_NOFOG		=	(1<<18),
	STATUS_BIT_AUTOFADE		=	(1<<19),	// calculate fade xparency value for object when rendering
	STATUS_BIT_DETACHED		=	(1<<20),	// set if objnode is free-floating and not attached to linked list
	STATUS_BIT_ONSPLINE		=	(1<<21),	// if objnode is attached to spline
	STATUS_BIT_REVERSESPLINE =	(1<<22)		// if going reverse direction on spline
};


#endif










