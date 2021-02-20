#include "Pomme.h"

#include <QD3D.h>

// bogus types to ease noquesa transition
typedef void* TQ3StyleObject;
typedef void* TQ3GroupObject;
typedef void* TQ3CameraObject;
typedef void* TQ3DrawContextObject;
typedef void* TQ3AttributeSet;
typedef void* TQ3Mipmap;
typedef void* TQ3PickObject;
typedef void* TQ3RendererObject;
typedef void* TQ3DisplayGroupObject;

#ifdef __cplusplus
extern "C"
{
#endif

#include "globals.h"
#include "renderer.h"
#include "structs.h"
#include "mobjtypes.h"
#include "objects.h"
#include "window.h"
#include "main.h"
#include "misc.h"
#include "bones.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "camera.h"
#include "player_control.h"
#include "sound2.h"
#include "3dmf.h"
#include "file.h"
#include 	"input.h"
#include 	"terrain.h"
#include "myguy.h"
#include "enemy.h"
#include "3dmath.h"
#include 	"selfrundemo.h"
#include "items.h"
#include "highscores.h"
#include "qd3d_geometry.h"
#include "environmentmap.h"
#include "infobar.h"
#include "morphobj.h"
#include "title.h"
#include "effects.h"
#include "miscscreens.h"
#include "fences.h"
#include "mainmenu.h"
#include "bonusscreen.h"
#include "liquids.h"
#include "collision.h"
#include "mytraps.h"
#include "triggers.h"
#include "pick.h"
#include "fileselect.h"
#include "textmesh.h"
#include "tga.h"
#include "tween.h"
#include "mousesmoothing.h"
#include "frustumculling.h"

#ifdef __cplusplus
};
#endif


#include "pickablequads.h"
#include "gamepatches.h"
#include "structformats.h"

#define GAME_ASSERT(condition) do { if (!(condition)) DoAssert(#condition, __func__, __LINE__); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoAssert(message, __func__, __LINE__); } while(0)
#define SOURCE_PORT_PLACEHOLDER() DoFatalAlert2("Source port: this function is not implemented yet", __func__)
#define SOURCE_PORT_MINOR_PLACEHOLDER() printf("Source port: TODO: %s:%d\n", __func__, __LINE__)
