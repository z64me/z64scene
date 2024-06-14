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

#if 1 // region: enums and enum macros
#define ENUM_CutsceneCmdMm(_) \
	/* 0x00A */ _(CS_CMD_MM_TEXT, = 10) \
	/* 0x05A */ _(CS_CMD_MM_CAMERA_SPLINE, = 90) \
	/* 0x064 */ _(CS_CMD_MM_ACTOR_CUE_100, = 100) \
	/* 0x065 */ _(CS_CMD_MM_ACTOR_CUE_101, ) \
	/* 0x066 */ _(CS_CMD_MM_ACTOR_CUE_102, ) \
	/* 0x067 */ _(CS_CMD_MM_ACTOR_CUE_103, ) \
	/* 0x068 */ _(CS_CMD_MM_ACTOR_CUE_104, ) \
	/* 0x069 */ _(CS_CMD_MM_ACTOR_CUE_105, ) \
	/* 0x06A */ _(CS_CMD_MM_ACTOR_CUE_106, ) \
	/* 0x06B */ _(CS_CMD_MM_ACTOR_CUE_107, ) \
	/* 0x06C */ _(CS_CMD_MM_ACTOR_CUE_108, ) \
	/* 0x06D */ _(CS_CMD_MM_ACTOR_CUE_109, ) \
	/* 0x06E */ _(CS_CMD_MM_ACTOR_CUE_110, ) \
	/* 0x06F */ _(CS_CMD_MM_ACTOR_CUE_111, ) \
	/* 0x070 */ _(CS_CMD_MM_ACTOR_CUE_112, ) \
	/* 0x071 */ _(CS_CMD_MM_ACTOR_CUE_113, ) \
	/* 0x072 */ _(CS_CMD_MM_ACTOR_CUE_114, ) \
	/* 0x073 */ _(CS_CMD_MM_ACTOR_CUE_115, ) \
	/* 0x074 */ _(CS_CMD_MM_ACTOR_CUE_116, ) \
	/* 0x075 */ _(CS_CMD_MM_ACTOR_CUE_117, ) \
	/* 0x076 */ _(CS_CMD_MM_ACTOR_CUE_118, ) \
	/* 0x077 */ _(CS_CMD_MM_ACTOR_CUE_119, ) \
	/* 0x078 */ _(CS_CMD_MM_ACTOR_CUE_120, ) \
	/* 0x079 */ _(CS_CMD_MM_ACTOR_CUE_121, ) \
	/* 0x07A */ _(CS_CMD_MM_ACTOR_CUE_122, ) \
	/* 0x07B */ _(CS_CMD_MM_ACTOR_CUE_123, ) \
	/* 0x07C */ _(CS_CMD_MM_ACTOR_CUE_124, ) \
	/* 0x07D */ _(CS_CMD_MM_ACTOR_CUE_125, ) \
	/* 0x07E */ _(CS_CMD_MM_ACTOR_CUE_126, ) \
	/* 0x07F */ _(CS_CMD_MM_ACTOR_CUE_127, ) \
	/* 0x080 */ _(CS_CMD_MM_ACTOR_CUE_128, ) \
	/* 0x081 */ _(CS_CMD_MM_ACTOR_CUE_129, ) \
	/* 0x082 */ _(CS_CMD_MM_ACTOR_CUE_130, ) \
	/* 0x083 */ _(CS_CMD_MM_ACTOR_CUE_131, ) \
	/* 0x084 */ _(CS_CMD_MM_ACTOR_CUE_132, ) \
	/* 0x085 */ _(CS_CMD_MM_ACTOR_CUE_133, ) \
	/* 0x086 */ _(CS_CMD_MM_ACTOR_CUE_134, ) \
	/* 0x087 */ _(CS_CMD_MM_ACTOR_CUE_135, ) \
	/* 0x088 */ _(CS_CMD_MM_ACTOR_CUE_136, ) \
	/* 0x089 */ _(CS_CMD_MM_ACTOR_CUE_137, ) \
	/* 0x08A */ _(CS_CMD_MM_ACTOR_CUE_138, ) \
	/* 0x08B */ _(CS_CMD_MM_ACTOR_CUE_139, ) \
	/* 0x08C */ _(CS_CMD_MM_ACTOR_CUE_140, ) \
	/* 0x08D */ _(CS_CMD_MM_ACTOR_CUE_141, ) \
	/* 0x08E */ _(CS_CMD_MM_ACTOR_CUE_142, ) \
	/* 0x08F */ _(CS_CMD_MM_ACTOR_CUE_143, ) \
	/* 0x090 */ _(CS_CMD_MM_ACTOR_CUE_144, ) \
	/* 0x091 */ _(CS_CMD_MM_ACTOR_CUE_145, ) \
	/* 0x092 */ _(CS_CMD_MM_ACTOR_CUE_146, ) \
	/* 0x093 */ _(CS_CMD_MM_ACTOR_CUE_147, ) \
	/* 0x094 */ _(CS_CMD_MM_ACTOR_CUE_148, ) \
	/* 0x095 */ _(CS_CMD_MM_ACTOR_CUE_149, ) \
	/* 0x096 */ _(CS_CMD_MM_MISC, ) \
	/* 0x097 */ _(CS_CMD_MM_LIGHT_SETTING, ) \
	/* 0x098 */ _(CS_CMD_MM_TRANSITION, ) \
	/* 0x099 */ _(CS_CMD_MM_MOTION_BLUR, ) \
	/* 0x09A */ _(CS_CMD_MM_GIVE_TATL, ) \
	/* 0x09B */ _(CS_CMD_MM_TRANSITION_GENERAL, ) \
	/* 0x09C */ _(CS_CMD_MM_FADE_OUT_SEQ, ) \
	/* 0x09D */ _(CS_CMD_MM_TIME, ) \
	/* 0x0C8 */ _(CS_CMD_MM_PLAYER_CUE, = 200) \
	/* 0x0C9 */ _(CS_CMD_MM_ACTOR_CUE_201, ) \
	/* 0x0FA */ _(CS_CMD_MM_UNK_DATA_FA, = 0xFA) \
	/* 0x0FE */ _(CS_CMD_MM_UNK_DATA_FE, = 0xFE) \
	/* 0x0FF */ _(CS_CMD_MM_UNK_DATA_FF, ) \
	/* 0x100 */ _(CS_CMD_MM_UNK_DATA_100, ) \
	/* 0x101 */ _(CS_CMD_MM_UNK_DATA_101, ) \
	/* 0x102 */ _(CS_CMD_MM_UNK_DATA_102, ) \
	/* 0x103 */ _(CS_CMD_MM_UNK_DATA_103, ) \
	/* 0x104 */ _(CS_CMD_MM_UNK_DATA_104, ) \
	/* 0x105 */ _(CS_CMD_MM_UNK_DATA_105, ) \
	/* 0x108 */ _(CS_CMD_MM_UNK_DATA_108, = 0x108) \
	/* 0x109 */ _(CS_CMD_MM_UNK_DATA_109, ) \
	/* 0x12C */ _(CS_CMD_MM_START_SEQ, = 300) \
	/* 0x12D */ _(CS_CMD_MM_STOP_SEQ, ) \
	/* 0x12E */ _(CS_CMD_MM_START_AMBIENCE, ) \
	/* 0x12F */ _(CS_CMD_MM_FADE_OUT_AMBIENCE, ) \
	/* 0x130 */ _(CS_CMD_MM_SFX_REVERB_INDEX_2, ) \
	/* 0x131 */ _(CS_CMD_MM_SFX_REVERB_INDEX_1, ) \
	/* 0x132 */ _(CS_CMD_MM_MODIFY_SEQ, ) \
	/* 0x15E */ _(CS_CMD_MM_DESTINATION, = 350) \
	/* 0x15F */ _(CS_CMD_MM_CHOOSE_CREDITS_SCENES, ) \
	/* 0x190 */ _(CS_CMD_MM_RUMBLE, = 400) \
	/* 0x1C2 */ _(CS_CMD_MM_ACTOR_CUE_450, = 450) \
	/* 0x1C3 */ _(CS_CMD_MM_ACTOR_CUE_451, ) \
	/* 0x1C4 */ _(CS_CMD_MM_ACTOR_CUE_452, ) \
	/* 0x1C5 */ _(CS_CMD_MM_ACTOR_CUE_453, ) \
	/* 0x1C6 */ _(CS_CMD_MM_ACTOR_CUE_454, ) \
	/* 0x1C7 */ _(CS_CMD_MM_ACTOR_CUE_455, ) \
	/* 0x1C8 */ _(CS_CMD_MM_ACTOR_CUE_456, ) \
	/* 0x1C9 */ _(CS_CMD_MM_ACTOR_CUE_457, ) \
	/* 0x1CA */ _(CS_CMD_MM_ACTOR_CUE_458, ) \
	/* 0x1CB */ _(CS_CMD_MM_ACTOR_CUE_459, ) \
	/* 0x1CC */ _(CS_CMD_MM_ACTOR_CUE_460, ) \
	/* 0x1CD */ _(CS_CMD_MM_ACTOR_CUE_461, ) \
	/* 0x1CE */ _(CS_CMD_MM_ACTOR_CUE_462, ) \
	/* 0x1CF */ _(CS_CMD_MM_ACTOR_CUE_463, ) \
	/* 0x1D0 */ _(CS_CMD_MM_ACTOR_CUE_464, ) \
	/* 0x1D1 */ _(CS_CMD_MM_ACTOR_CUE_465, ) \
	/* 0x1D2 */ _(CS_CMD_MM_ACTOR_CUE_466, ) \
	/* 0x1D3 */ _(CS_CMD_MM_ACTOR_CUE_467, ) \
	/* 0x1D4 */ _(CS_CMD_MM_ACTOR_CUE_468, ) \
	/* 0x1D5 */ _(CS_CMD_MM_ACTOR_CUE_469, ) \
	/* 0x1D6 */ _(CS_CMD_MM_ACTOR_CUE_470, ) \
	/* 0x1D7 */ _(CS_CMD_MM_ACTOR_CUE_471, ) \
	/* 0x1D8 */ _(CS_CMD_MM_ACTOR_CUE_472, ) \
	/* 0x1D9 */ _(CS_CMD_MM_ACTOR_CUE_473, ) \
	/* 0x1DA */ _(CS_CMD_MM_ACTOR_CUE_474, ) \
	/* 0x1DB */ _(CS_CMD_MM_ACTOR_CUE_475, ) \
	/* 0x1DC */ _(CS_CMD_MM_ACTOR_CUE_476, ) \
	/* 0x1DD */ _(CS_CMD_MM_ACTOR_CUE_477, ) \
	/* 0x1DE */ _(CS_CMD_MM_ACTOR_CUE_478, ) \
	/* 0x1DF */ _(CS_CMD_MM_ACTOR_CUE_479, ) \
	/* 0x1E0 */ _(CS_CMD_MM_ACTOR_CUE_480, ) \
	/* 0x1E1 */ _(CS_CMD_MM_ACTOR_CUE_481, ) \
	/* 0x1E2 */ _(CS_CMD_MM_ACTOR_CUE_482, ) \
	/* 0x1E3 */ _(CS_CMD_MM_ACTOR_CUE_483, ) \
	/* 0x1E4 */ _(CS_CMD_MM_ACTOR_CUE_484, ) \
	/* 0x1E5 */ _(CS_CMD_MM_ACTOR_CUE_485, ) \
	/* 0x1E6 */ _(CS_CMD_MM_ACTOR_CUE_486, ) \
	/* 0x1E7 */ _(CS_CMD_MM_ACTOR_CUE_487, ) \
	/* 0x1E8 */ _(CS_CMD_MM_ACTOR_CUE_488, ) \
	/* 0x1E9 */ _(CS_CMD_MM_ACTOR_CUE_489, ) \
	/* 0x1EA */ _(CS_CMD_MM_ACTOR_CUE_490, ) \
	/* 0x1EB */ _(CS_CMD_MM_ACTOR_CUE_491, ) \
	/* 0x1EC */ _(CS_CMD_MM_ACTOR_CUE_492, ) \
	/* 0x1ED */ _(CS_CMD_MM_ACTOR_CUE_493, ) \
	/* 0x1EE */ _(CS_CMD_MM_ACTOR_CUE_494, ) \
	/* 0x1EF */ _(CS_CMD_MM_ACTOR_CUE_495, ) \
	/* 0x1F0 */ _(CS_CMD_MM_ACTOR_CUE_496, ) \
	/* 0x1F1 */ _(CS_CMD_MM_ACTOR_CUE_497, ) \
	/* 0x1F2 */ _(CS_CMD_MM_ACTOR_CUE_498, ) \
	/* 0x1F3 */ _(CS_CMD_MM_ACTOR_CUE_499, ) \
	/* 0x1F4 */ _(CS_CMD_MM_ACTOR_CUE_500, ) \
	/* 0x1F5 */ _(CS_CMD_MM_ACTOR_CUE_501, ) \
	/* 0x1F6 */ _(CS_CMD_MM_ACTOR_CUE_502, ) \
	/* 0x1F7 */ _(CS_CMD_MM_ACTOR_CUE_503, ) \
	/* 0x1F8 */ _(CS_CMD_MM_ACTOR_CUE_504, ) \
	/* 0x1F9 */ _(CS_CMD_MM_ACTOR_CUE_SOTCS, ) /* Song of Time Cutscenes (Double SoT, ) / Three-Day Reset SoT) */ \
	/* 0x1FA */ _(CS_CMD_MM_ACTOR_CUE_506, ) \
	/* 0x1FB */ _(CS_CMD_MM_ACTOR_CUE_507, ) \
	/* 0x1FC */ _(CS_CMD_MM_ACTOR_CUE_508, ) \
	/* 0x1FD */ _(CS_CMD_MM_ACTOR_CUE_509, ) \
	/* 0x1FE */ _(CS_CMD_MM_ACTOR_CUE_510, ) \
	/* 0x1FF */ _(CS_CMD_MM_ACTOR_CUE_511, ) \
	/* 0x200 */ _(CS_CMD_MM_ACTOR_CUE_512, ) \
	/* 0x201 */ _(CS_CMD_MM_ACTOR_CUE_513, ) \
	/* 0x202 */ _(CS_CMD_MM_ACTOR_CUE_514, ) \
	/* 0x203 */ _(CS_CMD_MM_ACTOR_CUE_515, ) \
	/* 0x204 */ _(CS_CMD_MM_ACTOR_CUE_516, ) \
	/* 0x205 */ _(CS_CMD_MM_ACTOR_CUE_517, ) \
	/* 0x206 */ _(CS_CMD_MM_ACTOR_CUE_518, ) \
	/* 0x207 */ _(CS_CMD_MM_ACTOR_CUE_519, ) \
	/* 0x208 */ _(CS_CMD_MM_ACTOR_CUE_520, ) \
	/* 0x209 */ _(CS_CMD_MM_ACTOR_CUE_521, ) \
	/* 0x20A */ _(CS_CMD_MM_ACTOR_CUE_522, ) \
	/* 0x20B */ _(CS_CMD_MM_ACTOR_CUE_523, ) \
	/* 0x20C */ _(CS_CMD_MM_ACTOR_CUE_524, ) \
	/* 0x20D */ _(CS_CMD_MM_ACTOR_CUE_525, ) \
	/* 0x20E */ _(CS_CMD_MM_ACTOR_CUE_526, ) \
	/* 0x20F */ _(CS_CMD_MM_ACTOR_CUE_527, ) \
	/* 0x210 */ _(CS_CMD_MM_ACTOR_CUE_528, ) \
	/* 0x211 */ _(CS_CMD_MM_ACTOR_CUE_529, ) \
	/* 0x212 */ _(CS_CMD_MM_ACTOR_CUE_530, ) \
	/* 0x213 */ _(CS_CMD_MM_ACTOR_CUE_531, ) \
	/* 0x214 */ _(CS_CMD_MM_ACTOR_CUE_532, ) \
	/* 0x215 */ _(CS_CMD_MM_ACTOR_CUE_533, ) \
	/* 0x216 */ _(CS_CMD_MM_ACTOR_CUE_534, ) \
	/* 0x217 */ _(CS_CMD_MM_ACTOR_CUE_535, ) \
	/* 0x218 */ _(CS_CMD_MM_ACTOR_CUE_536, ) \
	/* 0x219 */ _(CS_CMD_MM_ACTOR_CUE_537, ) \
	/* 0x21A */ _(CS_CMD_MM_ACTOR_CUE_538, ) \
	/* 0x21B */ _(CS_CMD_MM_ACTOR_CUE_539, ) \
	/* 0x21C */ _(CS_CMD_MM_ACTOR_CUE_540, ) \
	/* 0x21D */ _(CS_CMD_MM_ACTOR_CUE_541, ) \
	/* 0x21E */ _(CS_CMD_MM_ACTOR_CUE_542, ) \
	/* 0x21F */ _(CS_CMD_MM_ACTOR_CUE_543, ) \
	/* 0x220 */ _(CS_CMD_MM_ACTOR_CUE_544, ) \
	/* 0x221 */ _(CS_CMD_MM_ACTOR_CUE_545, ) \
	/* 0x222 */ _(CS_CMD_MM_ACTOR_CUE_546, ) \
	/* 0x223 */ _(CS_CMD_MM_ACTOR_CUE_547, ) \
	/* 0x224 */ _(CS_CMD_MM_ACTOR_CUE_548, ) \
	/* 0x225 */ _(CS_CMD_MM_ACTOR_CUE_549, ) \
	/* 0x226 */ _(CS_CMD_MM_ACTOR_CUE_550, ) \
	/* 0x227 */ _(CS_CMD_MM_ACTOR_CUE_551, ) \
	/* 0x228 */ _(CS_CMD_MM_ACTOR_CUE_552, ) \
	/* 0x229 */ _(CS_CMD_MM_ACTOR_CUE_553, ) \
	/* 0x22A */ _(CS_CMD_MM_ACTOR_CUE_554, ) \
	/* 0x22B */ _(CS_CMD_MM_ACTOR_CUE_555, ) \
	/* 0x22C */ _(CS_CMD_MM_ACTOR_CUE_556, ) \
	/* 0x22D */ _(CS_CMD_MM_ACTOR_CUE_557, ) /* Couple's Mask cs, ) / Anju cues */ \
	/* 0x22E */ _(CS_CMD_MM_ACTOR_CUE_558, ) \
	/* 0x22F */ _(CS_CMD_MM_ACTOR_CUE_559, ) \
	/* 0x230 */ _(CS_CMD_MM_ACTOR_CUE_560, ) \
	/* 0x231 */ _(CS_CMD_MM_ACTOR_CUE_561, ) \
	/* 0x232 */ _(CS_CMD_MM_ACTOR_CUE_562, ) \
	/* 0x233 */ _(CS_CMD_MM_ACTOR_CUE_563, ) \
	/* 0x234 */ _(CS_CMD_MM_ACTOR_CUE_564, ) \
	/* 0x235 */ _(CS_CMD_MM_ACTOR_CUE_565, ) \
	/* 0x236 */ _(CS_CMD_MM_ACTOR_CUE_566, ) \
	/* 0x237 */ _(CS_CMD_MM_ACTOR_CUE_567, ) \
	/* 0x238 */ _(CS_CMD_MM_ACTOR_CUE_568, ) \
	/* 0x239 */ _(CS_CMD_MM_ACTOR_CUE_569, ) \
	/* 0x23A */ _(CS_CMD_MM_ACTOR_CUE_570, ) \
	/* 0x23B */ _(CS_CMD_MM_ACTOR_CUE_571, ) \
	/* 0x23C */ _(CS_CMD_MM_ACTOR_CUE_572, ) \
	/* 0x23D */ _(CS_CMD_MM_ACTOR_CUE_573, ) \
	/* 0x23E */ _(CS_CMD_MM_ACTOR_CUE_574, ) \
	/* 0x23F */ _(CS_CMD_MM_ACTOR_CUE_575, ) \
	/* 0x240 */ _(CS_CMD_MM_ACTOR_CUE_576, ) \
	/* 0x241 */ _(CS_CMD_MM_ACTOR_CUE_577, ) \
	/* 0x242 */ _(CS_CMD_MM_ACTOR_CUE_578, ) \
	/* 0x243 */ _(CS_CMD_MM_ACTOR_CUE_579, ) \
	/* 0x244 */ _(CS_CMD_MM_ACTOR_CUE_580, ) \
	/* 0x245 */ _(CS_CMD_MM_ACTOR_CUE_581, ) \
	/* 0x246 */ _(CS_CMD_MM_ACTOR_CUE_582, ) \
	/* 0x247 */ _(CS_CMD_MM_ACTOR_CUE_583, ) \
	/* 0x248 */ _(CS_CMD_MM_ACTOR_CUE_584, ) \
	/* 0x249 */ _(CS_CMD_MM_ACTOR_CUE_585, ) \
	/* 0x24A */ _(CS_CMD_MM_ACTOR_CUE_586, ) \
	/* 0x24B */ _(CS_CMD_MM_ACTOR_CUE_587, ) \
	/* 0x24C */ _(CS_CMD_MM_ACTOR_CUE_588, ) \
	/* 0x24D */ _(CS_CMD_MM_ACTOR_CUE_589, ) \
	/* 0x24E */ _(CS_CMD_MM_ACTOR_CUE_590, ) \
	/* 0x24F */ _(CS_CMD_MM_ACTOR_CUE_591, ) \
	/* 0x250 */ _(CS_CMD_MM_ACTOR_CUE_592, ) \
	/* 0x251 */ _(CS_CMD_MM_ACTOR_CUE_593, ) \
	/* 0x252 */ _(CS_CMD_MM_ACTOR_CUE_594, ) \
	/* 0x253 */ _(CS_CMD_MM_ACTOR_CUE_595, ) \
	/* 0x254 */ _(CS_CMD_MM_ACTOR_CUE_596, ) \
	/* 0x255 */ _(CS_CMD_MM_ACTOR_CUE_597, ) \
	/* 0x256 */ _(CS_CMD_MM_ACTOR_CUE_598, ) \
	/* 0x257 */ _(CS_CMD_MM_ACTOR_CUE_599, ) \
	/*    -2 */ _(CS_CMD_MM_ACTOR_CUE_POST_PROCESS, = 0xFFFFFFFE)

#define ENUM_CutsceneMiscTypeMm(_) \
	/* 0x00 */ _(CS_MISC_MM_UNIMPLEMENTED_0, ) \
	/* 0x01 */ _(CS_MISC_MM_RAIN, ) \
	/* 0x02 */ _(CS_MISC_MM_LIGHTNING, ) \
	/* 0x03 */ _(CS_MISC_MM_LIFT_FOG, ) \
	/* 0x04 */ _(CS_MISC_MM_CLOUDY_SKY, ) \
	/* 0x05 */ _(CS_MISC_MM_STOP_CUTSCENE, ) \
	/* 0x06 */ _(CS_MISC_MM_UNIMPLEMENTED_6, ) \
	/* 0x07 */ _(CS_MISC_MM_SHOW_TITLE_CARD, ) \
	/* 0x08 */ _(CS_MISC_MM_EARTHQUAKE_MEDIUM, ) \
	/* 0x09 */ _(CS_MISC_MM_EARTHQUAKE_STOP, ) \
	/* 0x0A */ _(CS_MISC_MM_VISMONO_BLACK_AND_WHITE, ) \
	/* 0x0B */ _(CS_MISC_MM_VISMONO_SEPIA, ) \
	/* 0x0C */ _(CS_MISC_MM_HIDE_ROOM, ) \
	/* 0x0D */ _(CS_MISC_MM_RED_PULSATING_LIGHTS, ) \
	/* 0x0E */ _(CS_MISC_MM_HALT_ALL_ACTORS, ) \
	/* 0x0F */ _(CS_MISC_MM_RESUME_ALL_ACTORS, ) \
	/* 0x10 */ _(CS_MISC_MM_SANDSTORM_FILL, ) \
	/* 0x11 */ _(CS_MISC_MM_SUNSSONG_START, ) \
	/* 0x12 */ _(CS_MISC_MM_FREEZE_TIME, ) \
	/* 0x13 */ _(CS_MISC_MM_LONG_SCARECROW_SONG, ) \
	/* 0x14 */ _(CS_MISC_MM_SET_CSFLAG_3, ) \
	/* 0x15 */ _(CS_MISC_MM_SET_CSFLAG_4, ) \
	/* 0x16 */ _(CS_MISC_MM_PLAYER_FORM_DEKU, ) \
	/* 0x17 */ _(CS_MISC_MM_ENABLE_PLAYER_REFLECTION, ) \
	/* 0x18 */ _(CS_MISC_MM_DISABLE_PLAYER_REFLECTION, ) \
	/* 0x19 */ _(CS_MISC_MM_PLAYER_FORM_HUMAN, ) \
	/* 0x1A */ _(CS_MISC_MM_EARTHQUAKE_STRONG, ) \
	/* 0x1B */ _(CS_MISC_MM_DEST_MOON_CRASH_FIRE_WALL, ) \
	/* 0x1C */ _(CS_MISC_MM_MOON_CRASH_SKYBOX, ) \
	/* 0x1D */ _(CS_MISC_MM_PLAYER_FORM_RESTORED, ) \
	/* 0x1E */ _(CS_MISC_MM_DISABLE_PLAYER_CSACTION_START_POS, ) \
	/* 0x1F */ _(CS_MISC_MM_ENABLE_PLAYER_CSACTION_START_POS, ) \
	/* 0x20 */ _(CS_MISC_MM_UNIMPLEMENTED_20, ) \
	/* 0x21 */ _(CS_MISC_MM_SAVE_ENTER_CLOCK_TOWN, ) \
	/* 0x22 */ _(CS_MISC_MM_RESET_SAVE_FROM_MOON_CRASH, ) \
	/* 0x23 */ _(CS_MISC_MM_TIME_ADVANCE, ) \
	/* 0x24 */ _(CS_MISC_MM_EARTHQUAKE_WEAK, ) \
	/* 0x25 */ _(CS_MISC_MM_UNIMPLEMENTED_25, ) \
	/* 0x26 */ _(CS_MISC_MM_DAWN_OF_A_NEW_DAY, ) \
	/* 0x27 */ _(CS_MISC_MM_PLAYER_FORM_ZORA, ) \
	/* 0x28 */ _(CS_MISC_MM_FINALE, )

#define ENUM_CutsceneTransitionTypeMm(_) \
	/*  1 */ _(CS_TRANSITION_MM_GRAY_FILL_IN, = 1) /* has hardcoded sounds for some scenes */ \
	/*  2 */ _(CS_TRANSITION_MM_BLUE_FILL_IN, ) \
	/*  3 */ _(CS_TRANSITION_MM_RED_FILL_OUT, ) \
	/*  4 */ _(CS_TRANSITION_MM_GREEN_FILL_OUT, ) \
	/*  5 */ _(CS_TRANSITION_MM_GRAY_FILL_OUT, ) \
	/*  6 */ _(CS_TRANSITION_MM_BLUE_FILL_OUT, ) \
	/*  7 */ _(CS_TRANSITION_MM_RED_FILL_IN, ) \
	/*  8 */ _(CS_TRANSITION_MM_GREEN_FILL_IN, ) \
	/*  9 */ _(CS_TRANSITION_MM_TRIGGER_INSTANCE, ) /* used with `TRANSITION_MODE_INSTANCE_WAIT` */ \
	/* 10 */ _(CS_TRANSITION_MM_BLACK_FILL_OUT, ) \
	/* 11 */ _(CS_TRANSITION_MM_BLACK_FILL_IN, ) \
	/* 12 */ _(CS_TRANSITION_MM_GRAY_TO_BLACK, ) \
	/* 13 */ _(CS_TRANSITION_MM_BLACK_TO_GRAY, )

#define ENUM_CsTransitionGeneralTypeMm(_) \
	/* 1 */ _(CS_TRANSITION_MM_GENERAL_FILL_IN, = 1) \
	/* 2 */ _(CS_TRANSITION_MM_GENERAL_FILL_OUT, )

#define ENUM_CsDestinationTypeMm(_) \
	/* 1 */ _(CS_DESTINATION_MM_DEFAULT, = 1) \
	/* 2 */ _(CS_DESTINATION_MM_BOSS_WARP, )

#define ENUM_CsChooseCreditsSceneTypeMm(_) \
	/*  1 */ _(CS_CREDITS_MM_DESTINATION, = CS_DESTINATION_MM_DEFAULT) \
	/*  2 */ _(CS_CREDITS_MM_MASK_KAMARO, ) \
	/*  3 */ _(CS_CREDITS_MM_MASK_GREAT_FAIRY, ) \
	/*  4 */ _(CS_CREDITS_MM_MASK_ROMANI, ) \
	/*  5 */ _(CS_CREDITS_MM_MASK_BLAST, ) \
	/*  6 */ _(CS_CREDITS_MM_MASK_CIRCUS_LEADER, ) \
	/*  7 */ _(CS_CREDITS_MM_MASK_BREMEN, ) \
	/*  8 */ _(CS_CREDITS_MM_IKANA, ) \
	/*  9 */ _(CS_CREDITS_MM_MASK_COUPLE, ) \
	/* 10 */ _(CS_CREDITS_MM_MASK_BUNNY, ) \
	/* 11 */ _(CS_CREDITS_MM_MASK_POSTMAN, )

#define ENUM_CsModifySeqTypeMm(_) \
	/* 1 */ _(CS_MODIFY_MM_SEQ_0, = 1) \
	/* 2 */ _(CS_MODIFY_MM_SEQ_1, ) \
	/* 3 */ _(CS_MODIFY_MM_SEQ_2, ) \
	/* 4 */ _(CS_MODIFY_MM_AMBIENCE_0, ) \
	/* 5 */ _(CS_MODIFY_MM_AMBIENCE_1, ) \
	/* 6 */ _(CS_MODIFY_MM_AMBIENCE_2, ) \
	/* 7 */ _(CS_MODIFY_MM_SEQ_STORE, ) \
	/* 8 */ _(CS_MODIFY_MM_SEQ_RESTORE, )

#define ENUM_CutsceneTextTypeMm(_) \
	/* -1 */ _(CS_TEXT_MM_TYPE_NONE, = -1) \
	/*  0 */ _(CS_TEXT_MM_TYPE_DEFAULT, ) \
	/*  1 */ _(CS_TEXT_MM_TYPE_1, ) \
	/*  2 */ _(CS_TEXT_MM_OCARINA_ACTION, ) \
	/*  3 */ _(CS_TEXT_MM_TYPE_3, ) \
	/*  4 */ _(CS_TEXT_MM_TYPE_BOSSES_REMAINS, ) /* use `altText1` in the Giant Cutscene if all remains are already obtained */ \
	/*  5 */ _(CS_TEXT_MM_TYPE_ALL_NORMAL_MASKS, )

#define ENUM_CutsceneFadeOutSeqPlayerMm(_) \
	/* 1 */ _(CS_FADE_OUT_MM_BGM_MAIN, = 1) \
	/* 2 */ _(CS_FADE_OUT_MM_FANFARE, )

#define ENUM_CsMotionBlurTypeMm(_) \
	/* 1 */ _(CS_MOTION_BLUR_MM_ENABLE, = 1) /* enable instantly */ \
	/* 2 */ _(CS_MOTION_BLUR_MM_DISABLE, ) /* disable gradually */

#define ENUM_CutsceneRumbleTypeMm(_) \
	/* 1 */ _(CS_RUMBLE_MM_ONCE, = 1) /* rumble once when startFrame is reached */ \
	/* 2 */ _(CS_RUMBLE_MM_PULSE, ) /* rumble every 64 frames between startFrame and endFrame */

#define ENUM_CutsceneStateMm(_) \
	/* 0 */ _(CS_STATE_MM_IDLE, ) \
	/* 1 */ _(CS_STATE_MM_START, ) \
	/* 2 */ _(CS_STATE_MM_RUN, ) \
	/* 3 */ _(CS_STATE_MM_STOP, ) \
	/* 4 */ _(CS_STATE_MM_RUN_UNSTOPPABLE, )

#endif // endregion: enums and enum macros

DEFINE_ENUM(CutsceneCmdMm, ENUM_CutsceneCmdMm)
DEFINE_ENUM(CutsceneMiscTypeMm, ENUM_CutsceneMiscTypeMm)
DEFINE_ENUM(CutsceneTransitionTypeMm, ENUM_CutsceneTransitionTypeMm)
DEFINE_ENUM(CsTransitionGeneralTypeMm, ENUM_CsTransitionGeneralTypeMm)
DEFINE_ENUM(CsDestinationTypeMm, ENUM_CsDestinationTypeMm)
DEFINE_ENUM(CsChooseCreditsSceneTypeMm, ENUM_CsChooseCreditsSceneTypeMm)
DEFINE_ENUM(CsModifySeqTypeMm, ENUM_CsModifySeqTypeMm)
DEFINE_ENUM(CutsceneTextTypeMm, ENUM_CutsceneTextTypeMm)
DEFINE_ENUM(CutsceneFadeOutSeqPlayerMm, ENUM_CutsceneFadeOutSeqPlayerMm)
DEFINE_ENUM(CsMotionBlurTypeMm, ENUM_CsMotionBlurTypeMm)
DEFINE_ENUM(CutsceneRumbleTypeMm, ENUM_CutsceneRumbleTypeMm)
DEFINE_ENUM(CutsceneStateMm, ENUM_CutsceneStateMm)

AS_STRING_DEC(CutsceneCmdMm, ENUM_CutsceneCmdMm)

#if 1 // region: command type structs

typedef union {
	struct {
		/* 0x00 */ u16 id; // "dousa"
		/* 0x02 */ u16 startFrame;
		/* 0x04 */ u16 endFrame;
		/* 0x06 */ Vec3us rot;
		/* 0x0C */ Vec3i startPos;
		/* 0x18 */ Vec3i endPos;
		/* 0x24 */ Vec3f normal;
	};
	s32 _words[12];
} CsCmdMmActorCue; // size = 0x30

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
	};
	s32 _words[2];
} CsCmdMmMisc; // size = 0x30

typedef union {
	struct {
		/* 0x0 */ u16 unused0;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
		/* 0x6 */ u8  hour;
		/* 0x7 */ u8  minute;
	};
	s32 _words[3];
} CsCmdMmTime; // size = 0xC

typedef union {
	struct {
		/* 0x0 */ u16 settingPlusOne;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmLightSetting; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 seqIdPlusOne;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
	};
	s32 _words[2];
} CsCmdMmStartSeq; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 seqIdPlusOne;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
	};
	s32 _words[2];
} CsCmdMmStopSeq; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 seqPlayer;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
	};
	s32 _words[3];
} CsCmdMmFadeOutSeq; // size = 0xC

typedef union {
	struct {
		/* 0x0 */ u16 unused0;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmStartAmbience; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 unused0;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
	};
	s32 _words[2];
} CsCmdMmFadeOutAmbience; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmModifySeq; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmDestination; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmChooseCreditsScene; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
	};
	s32 _words[2];
} CsCmdMmMotionBlur; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 unused0;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmSfxReverbIndexTo2; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 unused0;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmSfxReverbIndexTo1; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
	};
	s32 _words[2];
} CsCmdMmTransition; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 textId; // can also be an ocarina action for `CS_TEXT_OCARINA_ACTION`
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
		/* 0x6 */ u16 type;
		/* 0x8 */ u16 altTextId1;
		/* 0xA */ u16 altTextId2;
	};
	s32 _words[3];
} CsCmdMmText; // size = 0xC

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
		/* 0x6 */ u8 intensity;
		/* 0x7 */ u8 decayTimer;
		/* 0x8 */ u8 decayStep;
	};
	s32 _words[3];
} CsCmdMmRumble; // size = 0xC

typedef union {
	struct {
		/* 0x0 */ u16 type;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame;
		/* 0x6 */ u8 color[3];
	};
	s32 _words[3];
} CsCmdMmTransitionGeneral; // size = 0xC

typedef union {
	struct {
		/* 0x0 */ u16 giveTatl;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmGiveTatl; // size = 0x8

typedef union {
	struct {
		/* 0x0 */ u16 unused0;
		/* 0x2 */ u16 startFrame;
		/* 0x4 */ u16 endFrame; // unused
	};
	s32 _words[2];
} CsCmdMmUnimplemented; // size = 0x8

#endif // endregion: command type structs

typedef struct {
	CutsceneCmdMm type;
	union
	{
		sb_array(CsCmdMmActorCue,            actorCue);
		sb_array(CsCmdMmMisc,                misc);
		sb_array(CsCmdMmTime,                time);
		sb_array(CsCmdMmLightSetting,        lightSetting);
		sb_array(CsCmdMmStartSeq,            startSeq);
		sb_array(CsCmdMmStopSeq,             stopSeq);
		sb_array(CsCmdMmFadeOutSeq,          fadeOutSeq);
		sb_array(CsCmdMmStartAmbience,       startAmbience);
		sb_array(CsCmdMmFadeOutAmbience,     fadeOutAmbience);
		sb_array(CsCmdMmModifySeq,           modifySeq);
		sb_array(CsCmdMmDestination,         destination);
		sb_array(CsCmdMmChooseCreditsScene,  chooseCreditsScene);
		sb_array(CsCmdMmMotionBlur,          motionBlur);
		sb_array(CsCmdMmSfxReverbIndexTo2,   sfxReverbIndexTo2);
		sb_array(CsCmdMmSfxReverbIndexTo1,   sfxReverbIndexTo1);
		sb_array(CsCmdMmTransition,          transition);
		sb_array(CsCmdMmText,                text);
		sb_array(CsCmdMmRumble,              rumble);
		sb_array(CsCmdMmTransitionGeneral,   transitionGeneral);
		sb_array(CsCmdMmGiveTatl,            giveTatl);
		sb_array(CsCmdMmUnimplemented,       unimplemented);
		sb_array(u8,                         cameraSplineBytes);
	};
} CsCmdMm;

typedef struct CutsceneMm {
	int32_t frameCount;
	sb_array(CsCmdMm, commands);
} CutsceneMm;

typedef struct CutsceneListMm {
	/* 0x0 */ CutsceneMm *script;
	/* 0x4 */ s16 nextEntrance;
	/* 0x6 */ u8 spawn;
	/* 0x7 */ u8 spawnFlags; // See `CS_SPAWN_FLAG_`
} CutsceneListMm; // size = 0x8

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
