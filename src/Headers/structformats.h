#ifndef BUGDOM_STRUCTFORMATS_H
#define BUGDOM_STRUCTFORMATS_H

#define STRUCTFORMAT_SkeletonFile_Header_Type		"4h"
#define STRUCTFORMAT_File_BoneDefinitionType		"i32c3f2H8I"
#define STRUCTFORMAT_SkeletonFile_AnimHeader_Type	"B32cxh"
#define STRUCTFORMAT_TQ3Point3D						"3f"
#define STRUCTFORMAT_AnimEventType					"hBB"
#define STRUCTFORMAT_JointKeyframeType				"ii9f"

#define BYTESWAP_STRUCTS(type, n, ptr) ByteswapStructs(STRUCTFORMAT_##type, sizeof(type), n, ptr)

#endif //BUGDOM_STRUCTFORMATS_H
