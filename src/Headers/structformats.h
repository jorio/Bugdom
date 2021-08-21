#pragma once

// Structure packing formats

#define STRUCTFORMAT_SkeletonFile_Header_Type		"4h"
#define STRUCTFORMAT_File_BoneDefinitionType		"i32c3f2H8I"
#define STRUCTFORMAT_SkeletonFile_AnimHeader_Type	"B32cxh"
#define STRUCTFORMAT_TQ3Point3D						"3f"
#define STRUCTFORMAT_AnimEventType					"hBB"
#define STRUCTFORMAT_JointKeyframeType				"ii9f"
#define STRUCTFORMAT_PlayfieldHeaderType			"I5i3f2i"
#define STRUCTFORMAT_TerrainItemEntryType			"3H4bH"
#define STRUCTFORMAT_SplinePointType				"2f"
#define STRUCTFORMAT_SplineItemType					"fH4bH"
#define STRUCTFORMAT_File_SplineDefType				"hxxiiihxxi4h"
#define STRUCTFORMAT_File_FenceDefType				"Hhi4h"
#define STRUCTFORMAT_FencePointType					"2i"

#define BYTESWAP_STRUCTS(type, n, ptr) ByteswapStructs(STRUCTFORMAT_##type, sizeof(type), n, ptr)

#define BYTESWAP_SCALAR_ARRAY_HANDLE(type, handle) \
	ByteswapInts(sizeof(type), GetHandleSize(handle)/sizeof(type), *(handle))

#define BYTESWAP_SCALAR_ARRAY_HANDLE_2(type, n, handle)                                      \
{                                                                                            \
	if ((n) * sizeof(type) != (unsigned long) GetHandleSize((Handle) (handle)))              \
		DoFatalAlert2("byteswap scalars: size mismatch", #n);                                \
	ByteswapInts(sizeof(type), (n), *(handle));                                              \
}

#define BYTESWAP_STRUCT_ARRAY_HANDLE_2(type, n, handle)                                      \
{                                                                                            \
	if ((n) * sizeof(type) != (unsigned long) GetHandleSize((Handle) (handle)))              \
		DoFatalAlert2("byteswap structs: size mismatch", #n);                                \
	ByteswapStructs(STRUCTFORMAT_##type, sizeof(type), (n), *(handle));                      \
}

