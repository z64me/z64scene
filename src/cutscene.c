//
// cutscene.c
//
// cutscene stuff
//

#include "cutscene.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// debugging helper when testing exhaustively
//#define DEBUG_CUTSCENES_STRICT

#ifdef DEBUG_CUTSCENES_STRICT
	#define DATA_ASSERT(SIZE) { if (data + SIZE > dataEnd) Die("ran past eof, not a cutscene"); }
#else
	#define DATA_ASSERT(SIZE) { if (data + SIZE > dataEnd) goto L_fail; }
#endif

#if 1 // region: oot

struct CutsceneOot *CutsceneOotNew(void)
{
	struct CutsceneOot *cs = calloc(1, sizeof(*cs));
	
	return cs;
}

void CutsceneOotFree(struct CutsceneOot *cs)
{
	if (!cs)
		return;
	
	// free nested data
	sb_foreach(cs->commands, {
		if (strstr(CutsceneCmdOotAsString(each->type), "_CAM_"))
		{
			CsCmdOotCam *cams = each->cam;
			
			sb_foreach(cams, {
				sb_free(each->points);
			})
		}
		
		// is a union, so this polymorphically frees any list
		sb_free(each->misc);
	})
	
	sb_free(cs->commands);
	free(cs);
}

struct CutsceneOot *CutsceneOotNewFromData(const u8 *data, const u8 *dataEnd)
{
	const uint8_t *dataStart = data;
	struct CutsceneOot *cs = CutsceneOotNew();
	int totalEntries = u32r(data); data += 4;
	cs->frameCount = u32r(data); data += 4;
	
	fprintf(stderr, "totalEntries = %08x\n", totalEntries);
	fprintf(stderr, "frameCount = %08x\n", cs->frameCount);
	
	for (int i = 0; i < totalEntries; i++)
	{
		DATA_ASSERT(4)
		
		int cmdType = u32r(data); data += 4;
		const char *cmdName = CutsceneCmdOotAsString(cmdType);
		CsCmdOot cmd = { .type = cmdType };
		int cmdEntries = 0;
		bool hasOneCameraPoint = false;
		
		fprintf(stderr, "cmdName = %s, %08x\n", cmdName, (uint32_t)((data - 4) - dataStart));
		
		if (!strstr(cmdName, "_CAM_")) { DATA_ASSERT(4) cmdEntries = u32r(data); data += 4; }
		
		// safety
		if (cmdEntries > 255 || cmdEntries < 0)
		{
			#ifdef DEBUG_CUTSCENES_STRICT
			Die("cmdEntries = %d, suspicious\n", cmdEntries);
			#endif
			goto L_fail;
		}
		
		if (cmdType == CS_CAM_STOP)
			i = totalEntries;
		else if (strstr(cmdName, "ACTOR_CUE")
			|| cmdType == CS_CMD_OOT_PLAYER_CUE
		)
		{
			for_in(i, cmdEntries)
			{
				DATA_ASSERT(sizeof(CsCmdOotActorCue))
				
				CsCmdOotActorCue entry = {
					.id = u16r(data + 0),
					.startFrame = u16r(data + 2),
					.endFrame =  u16r(data + 4),
					.rot = u16r3(data + 6),
					.startPos = u32r3(data + 12),
					.endPos = u32r3(data + 24),
					.maybeUnused = f32r3(data + 36),
				};
				data += sizeof(entry);
				
				sb_push(cmd.actorCue, entry);
			}
		}
		else switch (cmdType)
		{
			case CS_CMD_OOT_MISC:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotMisc))
					
					CsCmdOotMisc entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.misc, entry);
				}
				break;
			
			case CS_CMD_OOT_LIGHT_SETTING:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotLightSetting))
					
					CsCmdOotLightSetting entry = {
						.unused0 = u8r(data),
						.settingPlusOne = u8r(data + 1),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.lightSetting, entry);
				}
				break;
			
			case CS_CMD_OOT_START_SEQ:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotStartSeq))
					
					CsCmdOotStartSeq entry = {
						.unused0 = u8r(data),
						.seqIdPlusOne = u8r(data + 1),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.startSeq, entry);
				}
				break;
			
			case CS_CMD_OOT_STOP_SEQ:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotStopSeq))
					
					CsCmdOotStopSeq entry = {
						.unused0 = u8r(data),
						.seqIdPlusOne = u8r(data + 1),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.stopSeq, entry);
				}
				break;
			
			case CS_CMD_OOT_FADE_OUT_SEQ:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotFadeOutSeq))
					
					CsCmdOotFadeOutSeq entry = {
						.seqPlayer = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.fadeOutSeq, entry);
				}
				break;
			
			case CS_CMD_OOT_RUMBLE_CONTROLLER:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotRumble))
					
					CsCmdOotRumble entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
						.sourceStrength = u8r(data + 6),
						.duration = u8r(data + 7),
						.decreaseRate = u8r(data + 8),
					};
					data += sizeof(entry);
					
					sb_push(cmd.rumble, entry);
				}
				break;
			
			case CS_CMD_OOT_TIME:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotTime))
					
					CsCmdOotTime entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
						.hour = u8r(data + 6),
						.minute = u8r(data + 7),
					};
					data += sizeof(entry);
					
					sb_push(cmd.time, entry);
				}
				break;
			
			case CS_CMD_OOT_TEXT:
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotText))
					
					CsCmdOotText entry = {
						.textId = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
						.type = u16r(data + 6),
						.altTextId1 = u16r(data + 8),
						.altTextId2 = u16r(data + 10),
					};
					data += sizeof(entry);
					
					sb_push(cmd.text, entry);
				}
				break;
			
			case CS_CMD_OOT_DESTINATION:
				// cmdEntries is unused
				{
					DATA_ASSERT(sizeof(CsCmdOotDestination))
					
					CsCmdOotDestination entry = {
						.destination = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.destination, entry);
				}
				break;
			
			case CS_CMD_OOT_TRANSITION:
				// cmdEntries is unused
				{
					DATA_ASSERT(sizeof(CsCmdOotTransition))
					
					CsCmdOotTransition entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.transition, entry);
				}
				break;
			
			case CS_CMD_OOT_CAM_EYE: //CutsceneCmd_SetCamEye()
			case CS_CMD_OOT_CAM_AT: //CutsceneCmd_SetCamAt()
				hasOneCameraPoint = true;
			case CS_CMD_OOT_CAM_EYE_SPLINE:
			case CS_CMD_OOT_CAM_EYE_SPLINE_REL_TO_PLAYER:
				//CutsceneCmd_UpdateCamEyeSpline()
			case CS_CMD_OOT_CAM_AT_SPLINE:
			case CS_CMD_OOT_CAM_AT_SPLINE_REL_TO_PLAYER:
				//CutsceneCmd_UpdateCamAtSpline()
				// cmdEntries is unused
				{
					DATA_ASSERT(sizeof(CsCmdOotCam))
					
					CsCmdOotCam entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += 8; // not sizeof b/c added data
					
					for (bool shouldContinue = true; shouldContinue; )
					{
						DATA_ASSERT(sizeof(CutsceneCameraPoint))
						
						CutsceneCameraPoint point = {
							.continueFlag = u8r(data),
							.cameraRoll = u8r(data + 1),
							.nextPointFrame = u16r(data + 2),
							.viewAngle = f32r(data + 4),
							.pos = u16r3(data + 8),
						};
						
						if (point.continueFlag == CS_CAM_STOP
							|| hasOneCameraPoint
						)
							shouldContinue = false;
						
						data += sizeof(point);
						sb_push(entry.points, point);
					}
					
					sb_push(cmd.cam, entry);
				}
				break;
			
			default:
				fprintf(stderr, "unhandled cutscene command '%s'\n", cmdName);
				
				// doesn't have a text-based name, so not in the enum
				// (most likely not a cutscene command)
				if (isdigit(*cmdName))
				{
					#ifdef DEBUG_CUTSCENES_STRICT
					Die("not an oot cutscene");
					#endif
					goto L_fail;
				}
				
				for_in(i, cmdEntries)
				{
					DATA_ASSERT(sizeof(CsCmdOotUnimplemented))
					
					CsCmdOotUnimplemented entry = {0};
					
					memcpy(&entry, data, sizeof(entry));
					data += sizeof(entry);
					
					sb_push(cmd.unimplemented, entry);
				}
				break;
		}
		
		sb_push(cs->commands, cmd);
	}
	
	return cs;
L_fail:
	CutsceneOotFree(cs);
	return 0;
}

void CutsceneOotToWorkblob(
	struct CutsceneOot *cs
	, void WorkblobPush(uint8_t alignBytes)
	, uint32_t WorkblobPop(void)
	, void WorkblobPut8(uint8_t data)
	, void WorkblobPut16(uint16_t data)
	, void WorkblobPut32(uint32_t data)
	, void WorkblobThisExactlyBegin(uint32_t wantThisSize)
	, void WorkblobThisExactlyEnd(void)
)
{
	WorkblobPush(4);
	
	int totalEntries = sb_count(cs->commands); WorkblobPut32(totalEntries);
	WorkblobPut32(cs->frameCount);
	
	for (int i = 0; i < totalEntries; i++)
	{
		CsCmdOot cmd = cs->commands[i];
		int cmdType = cmd.type; WorkblobPut32(cmdType);
		const char *cmdName = CutsceneCmdOotAsString(cmdType);
		int cmdEntries = 0;
		//bool hasOneCameraPoint = false; // shouldn't be necessary when writing
		
		if (!strstr(cmdName, "_CAM_")) {
			cmdEntries = sb_count(cmd.misc); // it's a union, so this works for all types
			WorkblobPut32(cmdEntries);
		}
		
		fprintf(stderr, "write cmdName = %s\n", cmdName);
		
		if (cmdType == CS_CAM_STOP)
			i = totalEntries;
		else if (strstr(cmdName, "ACTOR_CUE")
			|| cmdType == CS_CMD_OOT_PLAYER_CUE
		)
		{
			sb_foreach(cmd.actorCue, {
				WorkblobThisExactlyBegin(sizeof(*each));
					WorkblobPut16(each->id);
					WorkblobPut16(each->startFrame);
					WorkblobPut16(each->endFrame);
					WorkblobPut16(each->rot.x);
					WorkblobPut16(each->rot.y);
					WorkblobPut16(each->rot.z);
					WorkblobPut32(each->startPos.x);
					WorkblobPut32(each->startPos.y);
					WorkblobPut32(each->startPos.z);
					WorkblobPut32(each->endPos.x);
					WorkblobPut32(each->endPos.y);
					WorkblobPut32(each->endPos.z);
					WorkblobPut32(f32tou32(each->maybeUnused.x));
					WorkblobPut32(f32tou32(each->maybeUnused.y));
					WorkblobPut32(f32tou32(each->maybeUnused.z));
				WorkblobThisExactlyEnd();
			})
		}
		else switch (cmdType)
		{
			case CS_CMD_OOT_MISC:
				sb_foreach(cmd.misc, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_LIGHT_SETTING:
				sb_foreach(cmd.lightSetting, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut8(each->unused0);
						WorkblobPut8(each->settingPlusOne);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_START_SEQ:
				sb_foreach(cmd.startSeq, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut8(each->unused0);
						WorkblobPut8(each->seqIdPlusOne);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_STOP_SEQ:
				sb_foreach(cmd.stopSeq, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut8(each->unused0);
						WorkblobPut8(each->seqIdPlusOne);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_FADE_OUT_SEQ:
				sb_foreach(cmd.fadeOutSeq, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->seqPlayer);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_RUMBLE_CONTROLLER:
				sb_foreach(cmd.rumble, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
						WorkblobPut8(each->sourceStrength);
						WorkblobPut8(each->duration);
						WorkblobPut8(each->decreaseRate);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_TIME:
				sb_foreach(cmd.time, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
						WorkblobPut8(each->hour);
						WorkblobPut8(each->minute);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_TEXT:
				sb_foreach(cmd.text, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->textId);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
						WorkblobPut16(each->type);
						WorkblobPut16(each->altTextId1);
						WorkblobPut16(each->altTextId2);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_DESTINATION:
				// cmdEntries is unused
				sb_foreach(cmd.destination, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->destination);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_TRANSITION:
				// cmdEntries is unused
				sb_foreach(cmd.transition, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_OOT_CAM_EYE: //CutsceneCmd_SetCamEye()
			case CS_CMD_OOT_CAM_AT: //CutsceneCmd_SetCamAt()
				//hasOneCameraPoint = true;
			case CS_CMD_OOT_CAM_EYE_SPLINE:
			case CS_CMD_OOT_CAM_EYE_SPLINE_REL_TO_PLAYER:
				//CutsceneCmd_UpdateCamEyeSpline()
			case CS_CMD_OOT_CAM_AT_SPLINE:
			case CS_CMD_OOT_CAM_AT_SPLINE_REL_TO_PLAYER:
				//CutsceneCmd_UpdateCamAtSpline()
				// cmdEntries is unused
				sb_foreach(cmd.cam, {
					WorkblobThisExactlyBegin(8); // not sizeof b/c added data
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
					
					typeof(each->points) points = each->points;
					sb_foreach(points, {
						// TODO handle continueFlag here, or in editor?
						WorkblobThisExactlyBegin(sizeof(*each));
							WorkblobPut8(each->continueFlag);
							WorkblobPut8(each->cameraRoll);
							WorkblobPut16(each->nextPointFrame);
							WorkblobPut32(f32tou32(each->viewAngle));
							WorkblobPut16(each->pos.x);
							WorkblobPut16(each->pos.y);
							WorkblobPut16(each->pos.z);
						WorkblobThisExactlyEnd();
					})
				})
				break;
			
			default:
				//fprintf(stderr, "write unhandled cutscene command '%s'\n", cmdName);
				sb_foreach(cmd.unimplemented, {
					for (int i = 0; i < sizeof(each->_bytes); ++i)
						WorkblobPut8(each->_bytes[i]);
				})
				break;
		}
	}
	
	WorkblobPop();
}

AS_STRING_FUNC(CutsceneCmdOot, ENUM_CS_CMD_OOT)

#endif // endregion

#if 1 // region: mm

struct CutsceneMm *CutsceneMmNew(void)
{
	struct CutsceneMm *cs = calloc(1, sizeof(*cs));
	
	return cs;
}

void CutsceneMmFree(struct CutsceneMm *cs)
{
	if (!cs)
		return;
	
	// free nested data
	sb_foreach(cs->commands, {
		if (each->type == CS_CMD_MM_CAMERA_SPLINE)
		{
			// TODO free camera inner data, when it exists
		}
		
		// is a union, so this polymorphically frees any list
		sb_free(each->misc);
	})
	
	sb_free(cs->commands);
	free(cs);
}

void CutsceneListMmFree(struct CutsceneListMm *sbArr)
{
	sb_foreach(sbArr, {
		CutsceneMmFree(each->script);
	})
	sb_free(sbArr);
}

struct CutsceneMm *CutsceneMmNewFromData(const u8 *data, const u8 *dataEnd)
{
	if (!data)
		return 0;
	
	const uint8_t *dataStart = data;
	struct CutsceneMm *cs = CutsceneMmNew();
	int totalEntries = u32r(data); data += 4;
	cs->frameCount = u32r(data); data += 4;
	
	fprintf(stderr, "CutsceneMmNewFromData(%p)\n", data);
	
	// Loop over every command list
	for (int i = 0; i < totalEntries; i++)
	{
		int cmdType = u32r(data); data += 4;
		const char *cmdName = CutsceneCmdMmAsString(cmdType);
		CsCmdMm cmd = { .type = cmdType };
		int cmdEntries = 0;
		
		fprintf(stderr, "cmdName = %s, %08x\n", cmdName, (uint32_t)((data - 4) - dataStart));
		
		// cmdEntries is read by every command in mm
		if (true || !strstr(cmdName, "_CAMERA_")) { cmdEntries = u32r(data); data += 4; }
		
		if (cmdType == CS_CAM_STOP)
			i = totalEntries;
		else if (((cmdType >= CS_CMD_MM_ACTOR_CUE_100) && (cmdType <= CS_CMD_MM_ACTOR_CUE_149))
			 || (cmdType == CS_CMD_MM_ACTOR_CUE_201)
			 || ((cmdType >= CS_CMD_MM_ACTOR_CUE_450) && (cmdType <= CS_CMD_MM_ACTOR_CUE_599))
			 || cmdType == CS_CMD_MM_PLAYER_CUE
		) {
			for_in(i, cmdEntries)
			{
				CsCmdMmActorCue entry = {
					.id = u16r(data + 0),
					.startFrame = u16r(data + 2),
					.endFrame =  u16r(data + 4),
					.rot = u16r3(data + 6),
					.startPos = u32r3(data + 12),
					.endPos = u32r3(data + 24),
					.normal = f32r3(data + 36),
				};
				data += sizeof(entry);
				
				sb_push(cmd.actorCue, entry);
			}
		}
		else switch (cmdType) {
			case CS_CMD_MM_MISC:
				for_in(i, cmdEntries)
				{
					CsCmdMmMisc entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.misc, entry);
				}
				break;
			
			case CS_CMD_MM_LIGHT_SETTING:
				for_in(i, cmdEntries)
				{
					CsCmdMmLightSetting entry = {
						.settingPlusOne = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.lightSetting, entry);
				}
				break;
			
			case CS_CMD_MM_START_SEQ:
				for_in(i, cmdEntries)
				{
					CsCmdMmStartSeq entry = {
						.seqIdPlusOne = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.startSeq, entry);
				}
				break;
			
			case CS_CMD_MM_STOP_SEQ:
				for_in(i, cmdEntries)
				{
					CsCmdMmStopSeq entry = {
						.seqIdPlusOne = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.stopSeq, entry);
				}
				break;
			
			case CS_CMD_MM_FADE_OUT_SEQ:
				for_in(i, cmdEntries)
				{
					CsCmdMmFadeOutSeq entry = {
						.seqPlayer = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.fadeOutSeq, entry);
				}
				break;
			
			case CS_CMD_MM_START_AMBIENCE:
				for_in(i, cmdEntries)
				{
					CsCmdMmStartAmbience entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.startAmbience, entry);
				}
				break;
			
			case CS_CMD_MM_FADE_OUT_AMBIENCE:
				for_in(i, cmdEntries)
				{
					CsCmdMmFadeOutAmbience entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.fadeOutAmbience, entry);
				}
				break;
			
			case CS_CMD_MM_SFX_REVERB_INDEX_2:
				for_in(i, cmdEntries)
				{
					CsCmdMmSfxReverbIndexTo2 entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.sfxReverbIndexTo2, entry);
				}
				break;
			
			case CS_CMD_MM_SFX_REVERB_INDEX_1:
				for_in(i, cmdEntries)
				{
					CsCmdMmSfxReverbIndexTo1 entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.sfxReverbIndexTo1, entry);
				}
				break;
			
			case CS_CMD_MM_MODIFY_SEQ:
				for_in(i, cmdEntries)
				{
					CsCmdMmModifySeq entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.modifySeq, entry);
				}
				break;
			
			case CS_CMD_MM_RUMBLE:
				for_in(i, cmdEntries)
				{
					CsCmdMmRumble entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
						.intensity = u8r(data + 6),
						.decayTimer = u8r(data + 7),
						.decayStep = u8r(data + 8),
					};
					data += sizeof(entry);
					
					sb_push(cmd.rumble, entry);
				}
				break;
			
			case CS_CMD_MM_TRANSITION_GENERAL:
				for_in(i, cmdEntries)
				{
					CsCmdMmTransitionGeneral entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
						.color = u8r3(data + 6),
					};
					data += sizeof(entry);
					
					sb_push(cmd.transitionGeneral, entry);
				}
				break;
			
			case CS_CMD_MM_TIME:
				for_in(i, cmdEntries)
				{
					CsCmdMmTime entry = {
						.unused0 = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
						.hour = u8r(data + 6),
						.minute = u8r(data + 7),
					};
					data += sizeof(entry);
					
					sb_push(cmd.time, entry);
				}
				break;
			
			case CS_CMD_MM_CAMERA_SPLINE:
				// cmdEntries is cmdBytes (size of block in bytes)
				// TODO parse this into an intermediate format later
				for_in(i, cmdEntries)
					sb_push(cmd.cameraSplineBytes, data[i]);
				data += cmdEntries;
				break;
			
			case CS_CMD_MM_DESTINATION:
				for_in(i, cmdEntries)
				{
					CsCmdMmDestination entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.destination, entry);
				}
				break;
			
			case CS_CMD_MM_CHOOSE_CREDITS_SCENES:
				for_in(i, cmdEntries)
				{
					CsCmdMmChooseCreditsScene entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.chooseCreditsScene, entry);
				}
				break;
			
			case CS_CMD_MM_TEXT:
				for_in(i, cmdEntries)
				{
					CsCmdMmText entry = {
						.textId = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
						.type = u16r(data + 6),
						.altTextId1 = u16r(data + 8),
						.altTextId2 = u16r(data + 10),
					};
					data += sizeof(entry);
					
					sb_push(cmd.text, entry);
				}
				break;
			
			case CS_CMD_MM_TRANSITION:
				for_in(i, cmdEntries)
				{
					CsCmdMmTransition entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.transition, entry);
				}
				break;
			
			case CS_CMD_MM_MOTION_BLUR:
				for_in(i, cmdEntries)
				{
					CsCmdMmMotionBlur entry = {
						.type = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.motionBlur, entry);
				}
				break;
			
			case CS_CMD_MM_GIVE_TATL:
				for_in(i, cmdEntries)
				{
					CsCmdMmGiveTatl entry = {
						.giveTatl = u16r(data),
						.startFrame = u16r(data + 2),
						.endFrame = u16r(data + 4),
					};
					data += sizeof(entry);
					
					sb_push(cmd.giveTatl, entry);
				}
				break;
			
			default:
				fprintf(stderr, "unhandled cutscene command '%s'\n", cmdName);
				
				// doesn't have a text-based name, so not in the enum
				// (most likely not a cutscene command)
				if (isdigit(*cmdName))
				{
					#ifdef DEBUG_CUTSCENES_STRICT
					Die("not an mm cutscene");
					#endif
					goto L_fail;
				}
				
				for_in(i, cmdEntries)
				{
					CsCmdMmUnimplemented entry = {0};
					
					memcpy(&entry, data, sizeof(entry));
					data += sizeof(entry);
					
					sb_push(cmd.unimplemented, entry);
				}
				break;
		}
		
		sb_push(cs->commands, cmd);
	}
	
	return cs;
L_fail:
	CutsceneMmFree(cs);
	return 0;
}

uint32_t CutsceneMmToWorkblob(
	struct CutsceneMm *cs
	, void WorkblobPush(uint8_t alignBytes)
	, uint32_t WorkblobPop(void)
	, void WorkblobPut8(uint8_t data)
	, void WorkblobPut16(uint16_t data)
	, void WorkblobPut32(uint32_t data)
	, void WorkblobThisExactlyBegin(uint32_t wantThisSize)
	, void WorkblobThisExactlyEnd(void)
)
{
	if (!cs)
		return 0;
	
	WorkblobPush(4);
	
	int totalEntries = sb_count(cs->commands); WorkblobPut32(totalEntries);
	WorkblobPut32(cs->frameCount);
	
	// Loop over every command list
	for (int i = 0; i < totalEntries; i++)
	{
		CsCmdMm cmd = cs->commands[i];
		int cmdType = cmd.type; WorkblobPut32(cmdType);
		const char *cmdName = CutsceneCmdMmAsString(cmdType);
		int cmdEntries = sb_count(cmd.misc); // it's a union, so this counts all types
		WorkblobPut32(cmdEntries);
		
		fprintf(stderr, "write cmdName = %s\n", cmdName);
		
		// cmdEntries is read by every command in mm
		
		if (cmdType == CS_CAM_STOP)
			i = totalEntries;
		else if (strstr(cmdName, "ACTOR_CUE")
			|| cmdType == CS_CMD_MM_PLAYER_CUE
		) {
			sb_foreach(cmd.actorCue, {
				WorkblobThisExactlyBegin(sizeof(*each));
					WorkblobPut16(each->id);
					WorkblobPut16(each->startFrame);
					WorkblobPut16(each->endFrame);
					WorkblobPut16(each->rot.x);
					WorkblobPut16(each->rot.y);
					WorkblobPut16(each->rot.z);
					WorkblobPut32(each->startPos.x);
					WorkblobPut32(each->startPos.y);
					WorkblobPut32(each->startPos.z);
					WorkblobPut32(each->endPos.x);
					WorkblobPut32(each->endPos.y);
					WorkblobPut32(each->endPos.z);
					WorkblobPut32(f32tou32(each->normal.x));
					WorkblobPut32(f32tou32(each->normal.y));
					WorkblobPut32(f32tou32(each->normal.z));
				WorkblobThisExactlyEnd();
			})
		}
		else switch (cmdType) {
			case CS_CMD_MM_MISC:
				sb_foreach(cmd.misc, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_LIGHT_SETTING:
				sb_foreach(cmd.lightSetting, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->settingPlusOne);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_START_SEQ:
				sb_foreach(cmd.startSeq, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->seqIdPlusOne);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_STOP_SEQ:
				sb_foreach(cmd.stopSeq, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->seqIdPlusOne);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_FADE_OUT_SEQ:
				sb_foreach(cmd.fadeOutSeq, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->seqPlayer);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_START_AMBIENCE:
				sb_foreach(cmd.startAmbience, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_FADE_OUT_AMBIENCE:
				sb_foreach(cmd.fadeOutAmbience, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_SFX_REVERB_INDEX_2:
				sb_foreach(cmd.sfxReverbIndexTo2, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_SFX_REVERB_INDEX_1:
				sb_foreach(cmd.sfxReverbIndexTo1, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_MODIFY_SEQ:
				sb_foreach(cmd.modifySeq, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_RUMBLE:
				sb_foreach(cmd.rumble, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
						WorkblobPut8(each->intensity);
						WorkblobPut8(each->decayTimer);
						WorkblobPut8(each->decayStep);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_TRANSITION_GENERAL:
				sb_foreach(cmd.transitionGeneral, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
						for_in(i, 3) WorkblobPut8(each->color[i]);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_TIME:
				sb_foreach(cmd.time, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->unused0);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
						WorkblobPut8(each->hour);
						WorkblobPut8(each->minute);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_CAMERA_SPLINE:
				sb_foreach(cmd.cameraSplineBytes, {
					WorkblobPut8(*each);
				})
				break;
			
			case CS_CMD_MM_DESTINATION:
				sb_foreach(cmd.destination, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_CHOOSE_CREDITS_SCENES:
				sb_foreach(cmd.chooseCreditsScene, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_TEXT:
				sb_foreach(cmd.text, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->textId);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
						WorkblobPut16(each->type);
						WorkblobPut16(each->altTextId1);
						WorkblobPut16(each->altTextId2);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_TRANSITION:
				sb_foreach(cmd.transition, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_MOTION_BLUR:
				sb_foreach(cmd.motionBlur, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->type);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			case CS_CMD_MM_GIVE_TATL:
				sb_foreach(cmd.giveTatl, {
					WorkblobThisExactlyBegin(sizeof(*each));
						WorkblobPut16(each->giveTatl);
						WorkblobPut16(each->startFrame);
						WorkblobPut16(each->endFrame);
					WorkblobThisExactlyEnd();
				})
				break;
			
			default:
				//fprintf(stderr, "write unhandled cutscene command '%s'\n", cmdName);
				sb_foreach(cmd.unimplemented, {
					for (int i = 0; i < sizeof(each->_bytes); ++i)
						WorkblobPut8(each->_bytes[i]);
				})
				break;
		}
	}
	
	return WorkblobPop();
}

struct CutsceneListMm *CutsceneListMmNewFromData(const u8 *data, const u8 *dataEnd, const u8 num)
{
	sb_array(CutsceneListMm, result) = 0;
	
	for (u8 i = 0; i < num; ++i, data += 8)
	{
		fprintf(stderr, "append cutscene %08x\n", u32r(data));
		
		sb_push(result, ((CutsceneListMm) {
			.script = CutsceneMmNewFromData(n64_segment_get(u32r(data)), dataEnd),
			.nextEntrance = u16r(data + 4),
			.spawn = u8r(data + 6),
			.spawnFlags = u8r(data + 7),
		}));
	}
	
	return result;
	return 0;
}

uint32_t CutsceneListMmToWorkblob(
	struct CutsceneListMm *list
	, void WorkblobPush(uint8_t alignBytes)
	, uint32_t WorkblobPop(void)
	, void WorkblobPut8(uint8_t data)
	, void WorkblobPut16(uint16_t data)
	, void WorkblobPut32(uint32_t data)
	, void WorkblobThisExactlyBegin(uint32_t wantThisSize)
	, void WorkblobThisExactlyEnd(void)
)
{
	WorkblobPush(4);
	
	sb_foreach(list, {
		WorkblobPut32(
			CutsceneMmToWorkblob(
				each->script,
				WorkblobPush,
				WorkblobPop,
				WorkblobPut8,
				WorkblobPut16,
				WorkblobPut32,
				WorkblobThisExactlyBegin,
				WorkblobThisExactlyEnd
			)
		);
		WorkblobPut16(each->nextEntrance);
		WorkblobPut8(each->spawn);
		WorkblobPut8(each->spawnFlags);
	})
	
	return WorkblobPop();
}

AS_STRING_FUNC(CutsceneCmdMm, ENUM_CutsceneCmdMm)

#endif // endregion
