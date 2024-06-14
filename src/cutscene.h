//
// cutscene.h
//
// cutscene stuff
//

#ifndef CUTSCENE_H_INCLUDED
#define CUTSCENE_H_INCLUDED

#include "misc.h"

typedef struct Vec3us { u16 x, y, z; } Vec3us;
typedef struct Vec3i { s32 x, y, z; } Vec3i;

#if 1 // region: common camera data types

typedef union {
	struct {
		/* 0x00 */ s8 continueFlag;
		/* 0x01 */ s8 cameraRoll;
		/* 0x02 */ u16 nextPointFrame;
		/* 0x04 */ f32 viewAngle; // in degrees
		/* 0x08 */ Vec3s pos;
	};
	s32 _words[4];
} CutsceneCameraPoint; // size = 0x10

#define CS_CAM_CONTINUE 0
#define CS_CAM_STOP -1

#define CS_CAM_DATA_NOT_APPLIED 0xFFFF

typedef struct {
	/* 0x00 */ Vec3f at;
	/* 0x0C */ Vec3f eye;
	/* 0x18 */ s16 roll;
	/* 0x1A */ s16 fov;
} CutsceneCameraDirection; // size = 0x1C

typedef struct {
	/* 0x0 */ CutsceneCameraPoint* atPoints;
	/* 0x4 */ CutsceneCameraPoint* eyePoints;
	/* 0x8 */ s16 relativeToPlayer;
} CutsceneCameraMove; // size = 0xC

#endif

#if 1 // region: oot

#if 1 // enum macro
#define ENUM_CS_CMD_OOT(_) \
	/* 0x0001 */ _(CS_CMD_OOT_CAM_EYE_SPLINE, = 0x01) \
	/* 0x0002 */ _(CS_CMD_OOT_CAM_AT_SPLINE, ) \
	/* 0x0003 */ _(CS_CMD_OOT_MISC, ) \
	/* 0x0004 */ _(CS_CMD_OOT_LIGHT_SETTING, ) \
	/* 0x0005 */ _(CS_CMD_OOT_CAM_EYE_SPLINE_REL_TO_PLAYER, ) \
	/* 0x0006 */ _(CS_CMD_OOT_CAM_AT_SPLINE_REL_TO_PLAYER, ) \
	/* 0x0007 */ _(CS_CMD_OOT_CAM_EYE, ) \
	/* 0x0008 */ _(CS_CMD_OOT_CAM_AT, ) \
	/* 0x0009 */ _(CS_CMD_OOT_RUMBLE_CONTROLLER, ) \
	/* 0x000A */ _(CS_CMD_OOT_PLAYER_CUE, ) \
	/* 0x000B */ _(CS_CMD_OOT_UNIMPLEMENTED_B, ) \
	/* 0x000D */ _(CS_CMD_OOT_UNIMPLEMENTED_D, = 0x0D) \
	/* 0x000E */ _(CS_CMD_OOT_ACTOR_CUE_1_0, ) \
	/* 0x000F */ _(CS_CMD_OOT_ACTOR_CUE_0_0, ) \
	/* 0x0010 */ _(CS_CMD_OOT_ACTOR_CUE_1_1, ) \
	/* 0x0011 */ _(CS_CMD_OOT_ACTOR_CUE_0_1, ) \
	/* 0x0012 */ _(CS_CMD_OOT_ACTOR_CUE_0_2, ) \
	/* 0x0013 */ _(CS_CMD_OOT_TEXT, ) \
	/* 0x0015 */ _(CS_CMD_OOT_UNIMPLEMENTED_15, = 0x15) \
	/* 0x0016 */ _(CS_CMD_OOT_UNIMPLEMENTED_16, ) \
	/* 0x0017 */ _(CS_CMD_OOT_ACTOR_CUE_0_3, ) \
	/* 0x0018 */ _(CS_CMD_OOT_ACTOR_CUE_1_2, ) \
	/* 0x0019 */ _(CS_CMD_OOT_ACTOR_CUE_2_0, ) \
	/* 0x001A */ _(CS_CMD_OOT_UNIMPLEMENTED_1A, = 0x1A) \
	/* 0x001B */ _(CS_CMD_OOT_UNIMPLEMENTED_1B, = 0x1B) \
	/* 0x001C */ _(CS_CMD_OOT_UNIMPLEMENTED_1C, ) \
	/* 0x001D */ _(CS_CMD_OOT_ACTOR_CUE_3_0, ) \
	/* 0x001E */ _(CS_CMD_OOT_ACTOR_CUE_4_0, ) \
	/* 0x001F */ _(CS_CMD_OOT_ACTOR_CUE_6_0, ) \
	/* 0x0020 */ _(CS_CMD_OOT_UNIMPLEMENTED_20, ) \
	/* 0x0021 */ _(CS_CMD_OOT_UNIMPLEMENTED_21, ) \
	/* 0x0022 */ _(CS_CMD_OOT_ACTOR_CUE_0_4, ) \
	/* 0x0023 */ _(CS_CMD_OOT_ACTOR_CUE_1_3, ) \
	/* 0x0024 */ _(CS_CMD_OOT_ACTOR_CUE_2_1, ) \
	/* 0x0025 */ _(CS_CMD_OOT_ACTOR_CUE_3_1, ) \
	/* 0x0026 */ _(CS_CMD_OOT_ACTOR_CUE_4_1, ) \
	/* 0x0027 */ _(CS_CMD_OOT_ACTOR_CUE_0_5, ) \
	/* 0x0028 */ _(CS_CMD_OOT_ACTOR_CUE_1_4, ) \
	/* 0x0029 */ _(CS_CMD_OOT_ACTOR_CUE_2_2, ) \
	/* 0x002A */ _(CS_CMD_OOT_ACTOR_CUE_3_2, ) \
	/* 0x002B */ _(CS_CMD_OOT_ACTOR_CUE_4_2, ) \
	/* 0x002C */ _(CS_CMD_OOT_ACTOR_CUE_5_0, ) \
	/* 0x002D */ _(CS_CMD_OOT_TRANSITION, ) \
	/* 0x002E */ _(CS_CMD_OOT_ACTOR_CUE_0_6, ) \
	/* 0x002F */ _(CS_CMD_OOT_ACTOR_CUE_4_3, ) \
	/* 0x0030 */ _(CS_CMD_OOT_ACTOR_CUE_1_5, ) \
	/* 0x0031 */ _(CS_CMD_OOT_ACTOR_CUE_7_0, ) \
	/* 0x0032 */ _(CS_CMD_OOT_ACTOR_CUE_2_3, ) \
	/* 0x0033 */ _(CS_CMD_OOT_ACTOR_CUE_3_3, ) \
	/* 0x0034 */ _(CS_CMD_OOT_ACTOR_CUE_6_1, ) \
	/* 0x0035 */ _(CS_CMD_OOT_ACTOR_CUE_3_4, ) \
	/* 0x0036 */ _(CS_CMD_OOT_ACTOR_CUE_4_4, ) \
	/* 0x0037 */ _(CS_CMD_OOT_ACTOR_CUE_5_1, ) \
	/* 0x0039 */ _(CS_CMD_OOT_ACTOR_CUE_6_2, = 0x39) \
	/* 0x003A */ _(CS_CMD_OOT_ACTOR_CUE_6_3, ) \
	/* 0x003B */ _(CS_CMD_OOT_UNIMPLEMENTED_3B, ) \
	/* 0x003C */ _(CS_CMD_OOT_ACTOR_CUE_7_1, ) \
	/* 0x003D */ _(CS_CMD_OOT_UNIMPLEMENTED_3D, ) \
	/* 0x003E */ _(CS_CMD_OOT_ACTOR_CUE_8_0, ) \
	/* 0x003F */ _(CS_CMD_OOT_ACTOR_CUE_3_5, ) \
	/* 0x0040 */ _(CS_CMD_OOT_ACTOR_CUE_1_6, ) \
	/* 0x0041 */ _(CS_CMD_OOT_ACTOR_CUE_3_6, ) \
	/* 0x0042 */ _(CS_CMD_OOT_ACTOR_CUE_3_7, ) \
	/* 0x0043 */ _(CS_CMD_OOT_ACTOR_CUE_2_4, ) \
	/* 0x0044 */ _(CS_CMD_OOT_ACTOR_CUE_1_7, ) \
	/* 0x0045 */ _(CS_CMD_OOT_ACTOR_CUE_2_5, ) \
	/* 0x0046 */ _(CS_CMD_OOT_ACTOR_CUE_1_8, ) \
	/* 0x0047 */ _(CS_CMD_OOT_UNIMPLEMENTED_47, ) \
	/* 0x0048 */ _(CS_CMD_OOT_ACTOR_CUE_2_6, ) \
	/* 0x0049 */ _(CS_CMD_OOT_UNIMPLEMENTED_49, ) \
	/* 0x004A */ _(CS_CMD_OOT_ACTOR_CUE_2_7, ) \
	/* 0x004B */ _(CS_CMD_OOT_ACTOR_CUE_3_8, ) \
	/* 0x004C */ _(CS_CMD_OOT_ACTOR_CUE_0_7, ) \
	/* 0x004D */ _(CS_CMD_OOT_ACTOR_CUE_5_2, ) \
	/* 0x004E */ _(CS_CMD_OOT_ACTOR_CUE_1_9, ) \
	/* 0x004F */ _(CS_CMD_OOT_ACTOR_CUE_4_5, ) \
	/* 0x0050 */ _(CS_CMD_OOT_ACTOR_CUE_1_10, ) \
	/* 0x0051 */ _(CS_CMD_OOT_ACTOR_CUE_2_8, ) \
	/* 0x0052 */ _(CS_CMD_OOT_ACTOR_CUE_3_9, ) \
	/* 0x0053 */ _(CS_CMD_OOT_ACTOR_CUE_4_6, ) \
	/* 0x0054 */ _(CS_CMD_OOT_ACTOR_CUE_5_3, ) \
	/* 0x0055 */ _(CS_CMD_OOT_ACTOR_CUE_0_8, ) \
	/* 0x0056 */ _(CS_CMD_OOT_START_SEQ, ) \
	/* 0x0057 */ _(CS_CMD_OOT_STOP_SEQ, ) \
	/* 0x0058 */ _(CS_CMD_OOT_ACTOR_CUE_6_4, ) \
	/* 0x0059 */ _(CS_CMD_OOT_ACTOR_CUE_7_2, ) \
	/* 0x005A */ _(CS_CMD_OOT_ACTOR_CUE_5_4, ) \
	/* 0x005D */ _(CS_CMD_OOT_ACTOR_CUE_0_9, = 0x5D) \
	/* 0x005E */ _(CS_CMD_OOT_ACTOR_CUE_1_11, ) \
	/* 0x0069 */ _(CS_CMD_OOT_ACTOR_CUE_0_10, = 0x69) \
	/* 0x006A */ _(CS_CMD_OOT_ACTOR_CUE_2_9, ) \
	/* 0x006B */ _(CS_CMD_OOT_ACTOR_CUE_0_11, ) \
	/* 0x006C */ _(CS_CMD_OOT_ACTOR_CUE_3_10, ) \
	/* 0x006D */ _(CS_CMD_OOT_UNIMPLEMENTED_6D, ) \
	/* 0x006E */ _(CS_CMD_OOT_ACTOR_CUE_0_12, ) \
	/* 0x006F */ _(CS_CMD_OOT_ACTOR_CUE_7_3, ) \
	/* 0x0070 */ _(CS_CMD_OOT_UNIMPLEMENTED_70, ) \
	/* 0x0071 */ _(CS_CMD_OOT_UNIMPLEMENTED_71, ) \
	/* 0x0072 */ _(CS_CMD_OOT_ACTOR_CUE_7_4, ) \
	/* 0x0073 */ _(CS_CMD_OOT_ACTOR_CUE_6_5, ) \
	/* 0x0074 */ _(CS_CMD_OOT_ACTOR_CUE_1_12, ) \
	/* 0x0075 */ _(CS_CMD_OOT_ACTOR_CUE_2_10, ) \
	/* 0x0076 */ _(CS_CMD_OOT_ACTOR_CUE_1_13, ) \
	/* 0x0077 */ _(CS_CMD_OOT_ACTOR_CUE_0_13, ) \
	/* 0x0078 */ _(CS_CMD_OOT_ACTOR_CUE_1_14, ) \
	/* 0x0079 */ _(CS_CMD_OOT_ACTOR_CUE_2_11, ) \
	/* 0x007B */ _(CS_CMD_OOT_ACTOR_CUE_0_14, = 0x7B) \
	/* 0x007C */ _(CS_CMD_OOT_FADE_OUT_SEQ, ) \
	/* 0x007D */ _(CS_CMD_OOT_ACTOR_CUE_1_15, ) \
	/* 0x007E */ _(CS_CMD_OOT_ACTOR_CUE_2_12, ) \
	/* 0x007F */ _(CS_CMD_OOT_ACTOR_CUE_3_11, ) \
	/* 0x0080 */ _(CS_CMD_OOT_ACTOR_CUE_4_7, ) \
	/* 0x0081 */ _(CS_CMD_OOT_ACTOR_CUE_5_5, ) \
	/* 0x0082 */ _(CS_CMD_OOT_ACTOR_CUE_6_6, ) \
	/* 0x0083 */ _(CS_CMD_OOT_ACTOR_CUE_1_16, ) \
	/* 0x0084 */ _(CS_CMD_OOT_ACTOR_CUE_2_13, ) \
	/* 0x0085 */ _(CS_CMD_OOT_ACTOR_CUE_3_12, ) \
	/* 0x0086 */ _(CS_CMD_OOT_ACTOR_CUE_7_5, ) \
	/* 0x0087 */ _(CS_CMD_OOT_ACTOR_CUE_4_8, ) \
	/* 0x0088 */ _(CS_CMD_OOT_ACTOR_CUE_5_6, ) \
	/* 0x0089 */ _(CS_CMD_OOT_ACTOR_CUE_6_7, ) \
	/* 0x008A */ _(CS_CMD_OOT_ACTOR_CUE_0_15, ) \
	/* 0x008B */ _(CS_CMD_OOT_ACTOR_CUE_0_16, ) \
	/* 0x008C */ _(CS_CMD_OOT_TIME, ) \
	/* 0x008D */ _(CS_CMD_OOT_ACTOR_CUE_1_17, ) \
	/* 0x008E */ _(CS_CMD_OOT_ACTOR_CUE_7_6, ) \
	/* 0x008F */ _(CS_CMD_OOT_ACTOR_CUE_9_0, ) \
	/* 0x0090 */ _(CS_CMD_OOT_ACTOR_CUE_0_17, ) \
	/* 0x03E8 */ _(CS_CMD_OOT_DESTINATION, = 0x03E8) \
	/* 0xFFFF */ _(CS_CMD_OOT_END, = 0xFFFF)
#endif // enum macro

DEFINE_ENUM(CutsceneCmdOot, ENUM_CS_CMD_OOT)
AS_STRING_DEC(CutsceneCmdOot, ENUM_CS_CMD_OOT)

typedef union {
	struct {
		/* 0x00 */ u16 unused0;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame;
		// extra stuff added
		sb_array(CutsceneCameraPoint, points);
	};
	s32 _words[2];
} CsCmdOotCam; // size = 0x8

typedef union {
	struct {
		/* 0x00 */ u16 type;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame;
	};
	s32 _words[12];
} CsCmdOotMisc; // size = 0x30

typedef union {
	struct {
		/* 0x00 */ u8 unused0;
		/* 0x01 */ u8 settingPlusOne;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame; // unused
	};
	s32 _words[12];
} CsCmdOotLightSetting; // size = 0x30

typedef union {
	struct {
		/* 0x00 */ u8 unused0;
		/* 0x01 */ u8 seqIdPlusOne;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame; // unused
	};
	s32 _words[12];
} CsCmdOotStartSeq; // size = 0x30

typedef union {
	struct {
		/* 0x00 */ u8 unused0;
		/* 0x01 */ u8 seqIdPlusOne;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame; // unused
	};
	s32 _words[12];
} CsCmdOotStopSeq; // size = 0x30

typedef union {
	struct {
		/* 0x00 */ u16 seqPlayer;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame;
	};
	s32 _words[12];
} CsCmdOotFadeOutSeq; // size = 0x30

typedef union {
	struct {
		/* 0x00 */ u16 unused0;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame; // unused
		/* 0x06 */ u8 sourceStrength;
		/* 0x07 */ u8 duration;
		/* 0x08 */ u8 decreaseRate;
	};
	s32 _words[3];
} CsCmdOotRumble; // size = 0xC

typedef union {
	struct {
		/* 0x00 */ u16 unused0;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame; // unused
		/* 0x06 */ u8  hour;
		/* 0x07 */ u8  minute;
	};
	s32 _words[3];
} CsCmdOotTime; // size = 0xC

typedef union {
	struct {
		/* 0x00 */ u16 destination;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdOotDestination; // size = 0x8

typedef union {
	struct {
		/* 0x00 */ u16 textId; // can also be an ocarina action for `CS_TEXT_OCARINA_ACTION`
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame;
		/* 0x06 */ u16 type;
		/* 0x08 */ u16 altTextId1;
		/* 0x0A */ u16 altTextId2;
	};
	s32 _words[3];
} CsCmdOotText; // size = 0xC

#define CS_TEXT_ID_NONE 0xFFFF

typedef union {
	struct {
		/* 0x00 */ u16 type;
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame;
	};
	s32 _words[2];
} CsCmdOotTransition; // size = 0x8

typedef union {
	struct {
		/* 0x00 */ u16 id; // "dousa"
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame;
		/* 0x06 */ Vec3us rot;
		/* 0x0C */ Vec3i startPos;
		/* 0x18 */ Vec3i endPos;
		Vec3f maybeUnused;
	};
	s32 _words[12];
} CsCmdOotActorCue; // size = 0x30

typedef union {
	u8 _bytes[0x30];
} CsCmdOotUnimplemented;

typedef struct
{
	CutsceneCmdOot type;
	union
	{
		sb_array(CsCmdOotCam,            cam);
		sb_array(CsCmdOotMisc,           misc);
		sb_array(CsCmdOotLightSetting,   lightSetting);
		sb_array(CsCmdOotStartSeq,       startSeq);
		sb_array(CsCmdOotStopSeq,        stopSeq);
		sb_array(CsCmdOotFadeOutSeq,     fadeOutSeq);
		sb_array(CsCmdOotRumble,         rumble);
		sb_array(CsCmdOotTime,           time);
		sb_array(CsCmdOotDestination,    destination);
		sb_array(CsCmdOotText,           text);
		sb_array(CsCmdOotTransition,     transition);
		sb_array(CsCmdOotActorCue,       actorCue);
		sb_array(CsCmdOotUnimplemented,  unimplemented);
	};
} CsCmdOot;

typedef struct CutsceneOot
{
	int32_t frameCount;
	sb_array(CsCmdOot, commands);
} CutsceneOot;

#endif // endregion

#if 1 // region: mm

typedef struct CutsceneListMm
{
} CutsceneListMm;

#endif // endregion

#if 1 // region: function prototypes

struct CutsceneOot *CutsceneOotNew(void);
struct CutsceneOot *CutsceneOotNewFromData(const u8 *data, const u8 *dataEnd);
void CutsceneOotFree(struct CutsceneOot *cs);
void CutsceneOotToWorkblob(
	struct CutsceneOot *cs
	, void WorkblobPush(uint8_t alignBytes)
	, uint32_t WorkblobPop(void)
	, void WorkblobPut8(uint8_t data)
	, void WorkblobPut16(uint16_t data)
	, void WorkblobPut32(uint32_t data)
	, void WorkblobThisExactlyBegin(uint32_t wantThisSize)
	, void WorkblobThisExactlyEnd(void)
);

struct CutsceneListMm *CutsceneListMmNewFromData(const u8 *data, const u8 *dataEnd, const u8 num);

#endif // endregion

#endif // CUTSCENE_H_INCLUDED
