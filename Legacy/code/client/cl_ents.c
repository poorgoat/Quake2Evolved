/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
  
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
    
See the GNU General Public License for more details.
      
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
        
*/


// cl_ents.c -- entity parsing and management


#include "client.h"


/*
 =======================================================================

 FRAME PARSING
  
 =======================================================================
*/


/*
 =================
 CL_ParseEntityBits

 Returns the entity number and the header bits
 =================
*/
static int CL_ParseEntityBits (unsigned *bits){

    unsigned    b, total;
    int         number;

    total = MSG_ReadByte(&net_message);

    if (total & U_MOREBITS1){
        b = MSG_ReadByte(&net_message);
        total |= b<<8;
    }
    if (total & U_MOREBITS2){
        b = MSG_ReadByte(&net_message);
        total |= b<<16;
    }
    if (total & U_MOREBITS3){
        b = MSG_ReadByte(&net_message);
        total |= b<<24;
    }

	if (total & U_NUMBER16)
		number = MSG_ReadShort(&net_message);
    else
		number = MSG_ReadByte(&net_message);

    *bits = total;

    return number;
}

/*
 =================
 CL_ParseDelta

 Can go from either a baseline or a previous packet entity
 =================
*/
static void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits){

    // Set everything to the state we are delta'ing from
    *to = *from;

    VectorCopy(from->origin, to->old_origin);
    to->number = number;

    if (bits & U_MODEL)
        to->modelindex = MSG_ReadByte(&net_message);
    if (bits & U_MODEL2)
        to->modelindex2 = MSG_ReadByte(&net_message);
    if (bits & U_MODEL3)
        to->modelindex3 = MSG_ReadByte(&net_message);
    if (bits & U_MODEL4)
        to->modelindex4 = MSG_ReadByte(&net_message);

    if (bits & U_FRAME8)
        to->frame = MSG_ReadByte(&net_message);
    if (bits & U_FRAME16)
        to->frame = MSG_ReadShort(&net_message);

    if ((bits & U_SKIN8) && (bits & U_SKIN16))
        to->skinnum = MSG_ReadLong(&net_message);
    else if (bits & U_SKIN8)
        to->skinnum = MSG_ReadByte(&net_message);
    else if (bits & U_SKIN16)
        to->skinnum = MSG_ReadShort(&net_message);

    if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))
        to->effects = MSG_ReadLong(&net_message);
    else if (bits & U_EFFECTS8)
        to->effects = MSG_ReadByte(&net_message);
    else if (bits & U_EFFECTS16)
        to->effects = MSG_ReadShort(&net_message);

    if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
        to->renderfx = MSG_ReadLong(&net_message);
    else if (bits & U_RENDERFX8)
        to->renderfx = MSG_ReadByte(&net_message);
    else if (bits & U_RENDERFX16)
        to->renderfx = MSG_ReadShort(&net_message);

    if (bits & U_ORIGIN1)
        to->origin[0] = MSG_ReadCoord(&net_message);
    if (bits & U_ORIGIN2)
        to->origin[1] = MSG_ReadCoord(&net_message);
    if (bits & U_ORIGIN3)
        to->origin[2] = MSG_ReadCoord(&net_message);

    if (bits & U_ANGLE1)
        to->angles[0] = MSG_ReadAngle(&net_message);
    if (bits & U_ANGLE2)
        to->angles[1] = MSG_ReadAngle(&net_message);
    if (bits & U_ANGLE3)
        to->angles[2] = MSG_ReadAngle(&net_message);

    if (bits & U_OLDORIGIN)
        MSG_ReadPos(&net_message, to->old_origin);

    if (bits & U_SOUND)
        to->sound = MSG_ReadByte(&net_message);

    if (bits & U_EVENT)
        to->event = MSG_ReadByte(&net_message);
    else
        to->event = 0;

    if (bits & U_SOLID)
        to->solid = MSG_ReadShort(&net_message);
}

/*
 =================
 CL_DeltaEntity

 Parses deltas from the given base and adds the resulting entity to the
 current frame
 =================
*/
static void CL_DeltaEntity (frame_t *frame, entity_state_t *from, int number, int bits){

    centity_t		*cent;
    entity_state_t  *to;

	frame->numEntities++;

	cent = &cl.entities[number];

    to = &cl.parseEntities[cl.parseEntitiesIndex & (MAX_PARSE_ENTITIES-1)];
    cl.parseEntitiesIndex++;

    CL_ParseDelta(from, to, number, bits);

    // Some data changes will force no lerping
    if (to->modelindex != cent->current.modelindex || to->modelindex2 != cent->current.modelindex2 || to->modelindex3 != cent->current.modelindex3 || to->modelindex4 != cent->current.modelindex4)
		cent->serverFrame = -99;
	if (fabs(to->origin[0] - cent->current.origin[0]) > 512 || fabs(to->origin[1] - cent->current.origin[1]) > 512 || fabs(to->origin[2] - cent->current.origin[2]) > 512)
		cent->serverFrame = -99;
	if (to->event == EV_PLAYER_TELEPORT || to->event == EV_OTHER_TELEPORT)
        cent->serverFrame = -99;

    if (cent->serverFrame != frame->serverFrame - 1){
		// Wasn't in last update, so initialize some things.
        // Duplicate the current state so lerping doesn't hurt anything.
        cent->prev = *to;

        if (to->event == EV_OTHER_TELEPORT){
            VectorCopy(to->origin, cent->prev.origin);
            VectorCopy(to->origin, cent->lerpOrigin);
        }
        else {
            VectorCopy(to->old_origin, cent->prev.origin);
            VectorCopy(to->old_origin, cent->lerpOrigin);
        }
    }
    else
		// Shuffle the last state to previous
        cent->prev = cent->current;

    cent->serverFrame = frame->serverFrame;
    cent->current = *to;
}

/*
 =================
 CL_ParsePacketEntities

 A SVC_PACKETENTITIES has just been parsed, deal with the rest of the
 data stream
 =================
*/
static void CL_ParsePacketEntities (frame_t *oldFrame, frame_t *newFrame){

    entity_state_t	*oldState;
    int				newNum, oldNum;
    int				bits, oldIndex;

    newFrame->parseEntitiesIndex = cl.parseEntitiesIndex;
    newFrame->numEntities = 0;

    // Delta from the entities present in oldFrame
    oldIndex = 0;
    if (!oldFrame)
        oldNum = 99999;
    else {
        if (oldIndex >= oldFrame->numEntities)
            oldNum = 99999;
        else {
            oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES-1)];
            oldNum = oldState->number;
        }
    }

    while (1){
        newNum = CL_ParseEntityBits(&bits);
        if (newNum >= MAX_EDICTS)
            Com_Error(ERR_DROP, "CL_ParsePacketEntities: newNum = %i", newNum);

        if (net_message.readCount > net_message.curSize)
            Com_Error(ERR_DROP, "CL_ParsePacketEntities: end of message");

        if (!newNum)
            break;

        while (oldNum < newNum){
			// One or more entities from the old packet are unchanged
			CL_ShowNet(3, "Unchanged: %i", oldNum);

			CL_DeltaEntity(newFrame, oldState, oldNum, 0);

            oldIndex++;

            if (oldIndex >= oldFrame->numEntities)
                oldNum = 99999;
            else {
                oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES-1)];
                oldNum = oldState->number;
            }
        }

        if (bits & U_REMOVE){
			// The entity present in oldFrame is not in the current 
			// frame
			CL_ShowNet(3, "Remove: %i", newNum);

            if (oldNum != newNum)
                Com_DPrintf(S_COLOR_YELLOW "CL_ParsePacketEntities: oldNum != newNum\n");

            oldIndex++;

            if (oldIndex >= oldFrame->numEntities)
                oldNum = 99999;
            else {
                oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES-1)];
                oldNum = oldState->number;
            }

            continue;
        }

        if (oldNum == newNum){
			// Delta from previous state
			CL_ShowNet(3, "Delta: %i", newNum);

            CL_DeltaEntity(newFrame, oldState, newNum, bits);

            oldIndex++;

            if (oldIndex >= oldFrame->numEntities)
                oldNum = 99999;
            else {
                oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES-1)];
                oldNum = oldState->number;
            }

            continue;
        }

        if (oldNum > newNum){
			// Delta from baseline
			CL_ShowNet(3, "Baseline: %i", newNum);

            CL_DeltaEntity(newFrame, &cl.entities[newNum].baseline, newNum, bits);
            continue;
        }
    }

    // Any remaining entities in the old frame are copied over
    while (oldNum != 99999){
		// One or more entities from the old packet are unchanged
		CL_ShowNet(3, "Unchanged: %i", oldNum);

        CL_DeltaEntity(newFrame, oldState, oldNum, 0);

        oldIndex++;

        if (oldIndex >= oldFrame->numEntities)
            oldNum = 99999;
        else {
            oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES-1)];
            oldNum = oldState->number;
        }
    }
}

/*
 =================
 CL_ParsePlayerState
 =================
*/
static void CL_ParsePlayerState (frame_t *oldFrame, frame_t *newFrame){

    player_state_t  *state;
    int				i, flags, statBits;

    state = &newFrame->playerState;

    // Clear to old value before delta parsing
    if (oldFrame)
        *state = oldFrame->playerState;
    else
        memset(state, 0, sizeof(*state));

    flags = MSG_ReadShort(&net_message);

    // Parse the pmove_state_t
    if (flags & PS_M_TYPE)
        state->pmove.pm_type = MSG_ReadByte(&net_message);

    if (flags & PS_M_ORIGIN){
        state->pmove.origin[0] = MSG_ReadShort(&net_message);
        state->pmove.origin[1] = MSG_ReadShort(&net_message);
        state->pmove.origin[2] = MSG_ReadShort(&net_message);
    }

    if (flags & PS_M_VELOCITY){
        state->pmove.velocity[0] = MSG_ReadShort(&net_message);
        state->pmove.velocity[1] = MSG_ReadShort(&net_message);
        state->pmove.velocity[2] = MSG_ReadShort(&net_message);
    }

    if (flags & PS_M_TIME)
        state->pmove.pm_time = MSG_ReadByte(&net_message);

    if (flags & PS_M_FLAGS)
        state->pmove.pm_flags = MSG_ReadByte(&net_message);

    if (flags & PS_M_GRAVITY)
        state->pmove.gravity = MSG_ReadShort(&net_message);

    if (flags & PS_M_DELTA_ANGLES){
        state->pmove.delta_angles[0] = MSG_ReadShort(&net_message);
        state->pmove.delta_angles[1] = MSG_ReadShort(&net_message);
        state->pmove.delta_angles[2] = MSG_ReadShort(&net_message);
    }

    if (cl.demoPlayback)
        state->pmove.pm_type = PM_FREEZE;       // Demo playback

    // Parse the rest of the player_state_t
    if (flags & PS_VIEWOFFSET){
        state->viewoffset[0] = MSG_ReadChar(&net_message) * 0.25;
        state->viewoffset[1] = MSG_ReadChar(&net_message) * 0.25;
        state->viewoffset[2] = MSG_ReadChar(&net_message) * 0.25;
    }

    if (flags & PS_VIEWANGLES){
        state->viewangles[0] = MSG_ReadAngle16(&net_message);
        state->viewangles[1] = MSG_ReadAngle16(&net_message);
        state->viewangles[2] = MSG_ReadAngle16(&net_message);
    }

    if (flags & PS_KICKANGLES){
        state->kick_angles[0] = MSG_ReadChar(&net_message) * 0.25;
        state->kick_angles[1] = MSG_ReadChar(&net_message) * 0.25;
        state->kick_angles[2] = MSG_ReadChar(&net_message) * 0.25;
    }

    if (flags & PS_WEAPONINDEX)
        state->gunindex = MSG_ReadByte(&net_message);

    if (flags & PS_WEAPONFRAME){
        state->gunframe = MSG_ReadByte(&net_message);
        state->gunoffset[0] = MSG_ReadChar(&net_message) * 0.25;
        state->gunoffset[1] = MSG_ReadChar(&net_message) * 0.25;
        state->gunoffset[2] = MSG_ReadChar(&net_message) * 0.25;
        state->gunangles[0] = MSG_ReadChar(&net_message) * 0.25;
        state->gunangles[1] = MSG_ReadChar(&net_message) * 0.25;
        state->gunangles[2] = MSG_ReadChar(&net_message) * 0.25;
    }

    if (flags & PS_BLEND){
        state->blend[0] = MSG_ReadByte(&net_message) / 255.0;
        state->blend[1] = MSG_ReadByte(&net_message) / 255.0;
        state->blend[2] = MSG_ReadByte(&net_message) / 255.0;
        state->blend[3] = MSG_ReadByte(&net_message) / 255.0;
    }

    if (flags & PS_FOV)
        state->fov = MSG_ReadByte(&net_message);

    if (flags & PS_RDFLAGS)
        state->rdflags = MSG_ReadByte(&net_message);

    // Parse stats
    statBits = MSG_ReadLong(&net_message);

    for (i = 0; i < MAX_STATS; i++){
        if (statBits & (1<<i))
            state->stats[i] = MSG_ReadShort(&net_message);
	}
}

/*
 =================
 CL_FireEntityEvents
 =================
*/
static void CL_FireEntityEvents (void){

	entity_state_t  *state;
    int				i;

	for (i = 0; i < cl.frame.numEntities; i++){
        state = &cl.parseEntities[(cl.frame.parseEntitiesIndex+i) & (MAX_PARSE_ENTITIES-1)];

		// EF_TELEPORTER acts like an event, but is not cleared each
		// frame
		if (state->effects & EF_TELEPORTER)
			CL_TeleporterParticles(state->origin);

		if (!state->event)
			continue;

		switch (state->event){
		case EV_ITEM_RESPAWN:
			S_StartSound(NULL, state->number, CHAN_WEAPON, S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
			CL_ItemRespawnParticles(state->origin);

			break;
		case EV_PLAYER_TELEPORT:
			S_StartSound(NULL, state->number, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
			CL_TeleportParticles(state->origin);

			break;
		case EV_FOOTSTEP:
			if (cl_footSteps->integer)
				S_StartSound(NULL, state->number, CHAN_BODY, cl.media.sfxFootSteps[rand()&3], 1, ATTN_NORM, 0);

			break;
		case EV_FALLSHORT:
			S_StartSound(NULL, state->number, CHAN_AUTO, S_RegisterSound("player/land1.wav"), 1, ATTN_NORM, 0);

			break;
		case EV_FALL:
			S_StartSound(NULL, state->number, CHAN_AUTO, S_RegisterSound("*fall2.wav"), 1, ATTN_NORM, 0);

			break;
		case EV_FALLFAR:
			S_StartSound(NULL, state->number, CHAN_AUTO, S_RegisterSound("*fall1.wav"), 1, ATTN_NORM, 0);

			break;
		}
	}
}

/*
 =================
 CL_ParseBaseLine
 =================
*/
void CL_ParseBaseLine (void){

	entity_state_t	*state, nullState;
	int				bits, number;

	memset(&nullState, 0, sizeof(nullState));

	number = CL_ParseEntityBits(&bits);
	state = &cl.entities[number].baseline;
	CL_ParseDelta(&nullState, state, number, bits);
}
  
/*
 =================
 CL_ParseFrame
 =================
*/
void CL_ParseFrame (void){

	frame_t	*oldFrame;
	int     cmd, len;

	memset(&cl.frame, 0, sizeof(cl.frame));

	cl.frame.serverFrame = MSG_ReadLong(&net_message);
	cl.frame.deltaFrame = MSG_ReadLong(&net_message);
	cl.frame.serverTime = cl.frame.serverFrame * 100;

	// BIG HACK to let old demos continue to work
	if (cls.serverProtocol != 26)
		cl.suppressCount = MSG_ReadByte(&net_message);

	CL_ShowNet(3, "Frame: %i   Delta: %i", cl.frame.serverFrame, cl.frame.deltaFrame);

	// If the frame is delta compressed from data that we no longer have
	// available, we must suck up the rest of the frame, but not use it,
	// then ask for a non-compressed message 
	if (cl.frame.deltaFrame <= 0){
		oldFrame = NULL;
		cl.frame.valid = true;		// Uncompressed frame
		cls.demoWaiting = false;	// We can start recording now
	}
	else {
		oldFrame = &cl.frames[cl.frame.deltaFrame & UPDATE_MASK];
		if (!oldFrame->valid)
			// Should never happen
			Com_DPrintf(S_COLOR_YELLOW "Delta from invalid frame (not supposed to happen!)\n");

		if (cl.frame.deltaFrame != oldFrame->serverFrame)
			Com_DPrintf(S_COLOR_YELLOW "Delta frame too old\n");
		else if (cl.parseEntitiesIndex - oldFrame->parseEntitiesIndex > MAX_PARSE_ENTITIES-128)
			Com_DPrintf(S_COLOR_YELLOW "Delta parseEntitiesIndex too old\n");
		else
			cl.frame.valid = true;	// Valid delta parse
	}

	// Clamp time
	if (cl.time > cl.frame.serverTime)
		cl.time = cl.frame.serverTime;
	else if (cl.time < cl.frame.serverTime - 100)
		cl.time = cl.frame.serverTime - 100;

	// Read areabits
	len = MSG_ReadByte(&net_message);
	MSG_ReadData(&net_message, &cl.frame.areaBits, len);

	// Read player state
	cmd = MSG_ReadByte(&net_message);
	if (cmd != SVC_PLAYERINFO)
		Com_Error(ERR_DROP, "CL_ParseFrame: not player state");

	CL_ShowNet(2, "%3i: %s", net_message.readCount-1, svc_strings[cmd]);

	CL_ParsePlayerState(oldFrame, &cl.frame);

	// Read packet entities
	cmd = MSG_ReadByte(&net_message);
	if (cmd != SVC_PACKETENTITIES)
		Com_Error(ERR_DROP, "CL_ParseFrame: not packet entities");

	CL_ShowNet(2, "%3i: %s", net_message.readCount-1, svc_strings[cmd]);

	CL_ParsePacketEntities(oldFrame, &cl.frame);

	// Save the frame off in the backup array for later delta 
	// comparisons
	cl.frames[cl.frame.serverFrame & UPDATE_MASK] = cl.frame;

	if (!cl.frame.valid)
		return;

	// Getting a valid frame message ends the connection/loading process
	if (cls.state == CA_PRIMED){
		cls.state = CA_ACTIVE;
		cls.loading = false;

		cl.predictedOrigin[0] = cl.frame.playerState.pmove.origin[0] * 0.125;
		cl.predictedOrigin[1] = cl.frame.playerState.pmove.origin[1] * 0.125;
		cl.predictedOrigin[2] = cl.frame.playerState.pmove.origin[2] * 0.125;

		VectorCopy(cl.frame.playerState.viewangles, cl.predictedAngles);
	}

	if (cls.state != CA_ACTIVE)
		return;

	CL_BuildSolidList();

	CL_FireEntityEvents();

	CL_CheckPredictionError();
}


/*
 =======================================================================

 INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS
  
 =======================================================================
*/


/*
 =================
 CL_FlyEffect
 =================
*/
static void CL_FlyEffect (centity_t *cent, const vec3_t org){

	int		n, count, start;

	if (cent->flyStopTime < cl.time){
		start = cl.time;
		cent->flyStopTime = cl.time + 60000;
	}
	else
		start = cent->flyStopTime - 60000;

	n = cl.time - start;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else {
		n = cent->flyStopTime - cl.time;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	if (count > NUM_VERTEX_NORMALS)
		count = NUM_VERTEX_NORMALS;

	CL_FlyParticles(org, count);
}

/*
 =================
 CL_AddShellEntity
 =================
*/
static void CL_AddShellEntity (entity_t *ent, unsigned effects, qboolean light){

	vec3_t	rgb;

	if (!(effects & (EF_PENT|EF_QUAD|EF_DOUBLE|EF_HALF_DAMAGE|EF_COLOR_SHELL)))
		return;

	if (effects & EF_PENT){
		ent->customShader = cl.media.invulnerabilityShellShader;
		MakeRGBA(ent->shaderRGBA, 255, 0, 0, 75);

		R_AddEntityToScene(ent);

		if (light)
			R_AddLightToScene(ent->origin, 200 + (rand() & 31), 1, 0, 0);
	}

	if (effects & EF_QUAD){
		ent->customShader = cl.media.quadDamageShellShader;
		MakeRGBA(ent->shaderRGBA, 0, 0, 255, 75);

		R_AddEntityToScene(ent);

		if (light)
			R_AddLightToScene(ent->origin, 200 + (rand() & 31), 0, 0, 1);
	}

	if (effects & EF_DOUBLE){
		ent->customShader = cl.media.doubleDamageShellShader;
		MakeRGBA(ent->shaderRGBA, 230, 178, 0, 75);

		R_AddEntityToScene(ent);

		if (light)
			R_AddLightToScene(ent->origin, 200 + (rand() & 31), 0.9, 0.7, 0.0);
	}

	if (effects & EF_HALF_DAMAGE){
		ent->customShader = cl.media.halfDamageShellShader;
		MakeRGBA(ent->shaderRGBA, 142, 150, 114, 75);

		R_AddEntityToScene(ent);

		if (light)
			R_AddLightToScene(ent->origin, 200 + (rand() & 31), 0.56, 0.59, 0.45);
	}

	if (effects & EF_COLOR_SHELL){
		ent->customShader = cl.media.genericShellShader;
		MakeRGBA(ent->shaderRGBA, 0, 0, 0, 75);

		VectorClear(rgb);

		if (ent->renderFX & RF_SHELL_RED){
			ent->shaderRGBA[0] = 255;
			rgb[0] = 1;
		}
		if (ent->renderFX & RF_SHELL_GREEN){
			ent->shaderRGBA[1] = 255;
			rgb[1] = 1;
		}
		if (ent->renderFX & RF_SHELL_BLUE){
			ent->shaderRGBA[2] = 255;
			rgb[2] = 1;
		}

		R_AddEntityToScene(ent);

		if (light)
			R_AddLightToScene(ent->origin, 200 + (rand() & 31), rgb[0], rgb[1], rgb[2]);
	}

	// Make sure these get reset
	ent->customShader = NULL;
	MakeRGBA(ent->shaderRGBA, 255, 255, 255, 255);
}

/*
 =================
 CL_AddEntityTrails
 =================
*/
static void CL_AddEntityTrails (centity_t *cent, const vec3_t org, unsigned effects, qboolean viewer){

	if (effects & EF_ROCKET){
		CL_RocketTrail(cent->lerpOrigin, org);
		R_AddLightToScene(org, 200, 1, 1, 0);
	}
	else if (effects & EF_BLASTER){
		if (effects & EF_TRACKER){
			CL_BlasterTrail(cent->lerpOrigin, org, 0.00, 1.00, 0.00);
			R_AddLightToScene(org, 200, 0, 1, 0);
		}
		else {
			CL_BlasterTrail(cent->lerpOrigin, org, 0.97, 0.46, 0.14);
			R_AddLightToScene(org, 200, 1, 1, 0);
		}
	}
	else if (effects & EF_HYPERBLASTER){
		if (effects & EF_TRACKER)
			R_AddLightToScene(org, 200, 0, 1, 0);
		else
			R_AddLightToScene(org, 200, 1, 1, 0);
	}
	else if (effects & EF_GIB)
		CL_BloodTrail(cent->lerpOrigin, org, false);
	else if (effects & EF_GREENGIB)
		CL_BloodTrail(cent->lerpOrigin, org, true);
	else if (effects & EF_GRENADE)
		CL_GrenadeTrail(cent->lerpOrigin, org);
	else if (effects & EF_FLIES)
		CL_FlyEffect(cent, org);
	else if (effects & EF_TRAP){
		CL_TrapParticles(org);
		R_AddLightToScene(org, 100 + (rand() % 100), 1, 0.8, 0.1);
	}
	else if (effects & EF_FLAG1){
		if (!viewer)
			CL_FlagTrail(cent->lerpOrigin, org, 1.0, 0.1, 0.1);

		R_AddLightToScene(org, 225, 1, 0.1, 0.1);
	}
	else if (effects & EF_FLAG2){
		if (!viewer)
			CL_FlagTrail(cent->lerpOrigin, org, 0.1, 0.1, 1.0);

		R_AddLightToScene(org, 225, 0.1, 0.1, 1);
	}
	else if (effects & EF_TAGTRAIL){
		if (!viewer)
			CL_TagTrail(cent->lerpOrigin, org);

		R_AddLightToScene(org, 225, 1, 1, 0);
	}
	else if (effects & EF_TRACKERTRAIL){
		if (viewer)
			R_AddLightToScene(org, 255, -1, -1, -1);
		else {
			if (effects & EF_TRACKER)
				R_AddLightToScene(org, 50 + (500 * (sin(cl.time / 500.0) + 1.0)), -1, -1, -1);
			else {
				CL_TrackerShellParticles(cent->lerpOrigin);
				R_AddLightToScene(org, 155, -1, -1, -1);
			}
		}
	}
	else if (effects & EF_TRACKER){
		CL_TrackerTrail(cent->lerpOrigin, org);
		R_AddLightToScene(org, 200, -1, -1, -1);
	}
	else if (effects & EF_IONRIPPER)
		R_AddLightToScene(org, 100, 1, 0.5, 0.5);
	else if (effects & EF_BLUEHYPERBLASTER)
		R_AddLightToScene(org, 200, 0, 0, 1);
}

/*
 =================
 CL_AddPacketEntities
 =================
*/
void CL_AddPacketEntities (void){

	entity_t        ent;
	centity_t		*cent;
    entity_state_t	*state;
	vec3_t			origin, angles, autoRotateAxis[3];
	int				animAll, animAllFast, anim01, anim23;
    int             i, weapon;
    clientInfo_t    *ci;

	// Some items auto-rotate
	VectorSet(angles, 0, AngleMod(cl.time * 0.1), 0);
	AnglesToAxis(angles, autoRotateAxis);

	// Brush models can auto-animate their frames
	animAll = 2 * cl.time / 1000;
	animAllFast = cl.time / 100;
	anim01 = (animAll & 1);
	anim23 = (animAll & 1) + 2;

	memset(&ent, 0, sizeof(ent));

	for (i = 0; i < cl.frame.numEntities; i++){
		state = &cl.parseEntities[(cl.frame.parseEntitiesIndex+i) & (MAX_PARSE_ENTITIES-1)];

		cent = &cl.entities[state->number];

		// Calculate origin
		if (state->renderfx & (RF_FRAMELERP|RF_BEAM)){
			// Step origin discretely, because the frames do the
			// animation properly
			VectorCopy(cent->current.origin, ent.origin);
			VectorCopy(cent->current.old_origin, ent.oldOrigin);
		}
		else {
			// Interpolate origin
			ent.origin[0] = ent.oldOrigin[0] = cent->prev.origin[0] + (cent->current.origin[0] - cent->prev.origin[0]) * cl.lerpFrac;
			ent.origin[1] = ent.oldOrigin[1] = cent->prev.origin[1] + (cent->current.origin[1] - cent->prev.origin[1]) * cl.lerpFrac;
			ent.origin[2] = ent.oldOrigin[2] = cent->prev.origin[2] + (cent->current.origin[2] - cent->prev.origin[2]) * cl.lerpFrac;
		}

		// Add laser beams
		if (state->renderfx & RF_BEAM){
			CL_LaserBeam(ent.origin, ent.oldOrigin, state->frame, state->skinnum, 75, 1, cl.media.laserBeamShader);

			VectorCopy(ent.origin, cent->lerpOrigin);
			continue;
		}

		// BFG and Phalanx effects are just sprites
		if (state->effects & EF_BFG){
			if (state->effects & EF_ANIM_ALLFAST){
				CL_Sprite(ent.origin, 40, cl.media.bfgBallShader);
				CL_BFGTrail(cent->lerpOrigin, ent.origin);
				R_AddLightToScene(ent.origin, 200, 0, 1, 0);
			}

			VectorCopy(ent.origin, cent->lerpOrigin);
			continue;
		}
		if (state->effects & EF_PLASMA){
			if (state->effects & EF_ANIM_ALLFAST){
				CL_Sprite(ent.origin, 25, cl.media.plasmaBallShader);
				CL_BlasterTrail(cent->lerpOrigin, ent.origin, 0.97, 0.46, 0.14);
				R_AddLightToScene(ent.origin, 130, 1, 0.5, 0.5);
			}

			VectorCopy(ent.origin, cent->lerpOrigin);
			continue;
		}

		// Calculate angles
		if (state->effects & EF_ROTATE)
			AxisCopy(autoRotateAxis, ent.axis);
		else if (state->effects & EF_SPINNINGLIGHTS){
			VectorSet(angles, 0, AngleMod(cl.time * 0.5) + state->angles[1], 180);
			AnglesToAxis(angles, ent.axis);

			VectorMA(ent.origin, 64, ent.axis[0], origin);
			R_AddLightToScene(origin, 100, 1, 0, 0);
		}
		else {
			// Interpolate angles
			angles[0] = LerpAngle(cent->prev.angles[0], cent->current.angles[0], cl.lerpFrac);
			angles[1] = LerpAngle(cent->prev.angles[1], cent->current.angles[1], cl.lerpFrac);
			angles[2] = LerpAngle(cent->prev.angles[2], cent->current.angles[2], cl.lerpFrac);

			AnglesToAxis(angles, ent.axis);
		}

		// If set to invisible, skip
		if (!state->modelindex){
			VectorCopy(ent.origin, cent->lerpOrigin);
			continue;
		}

		// Create a new entity
		ent.entityType = ET_MODEL;

		// Set model and skin
		if (state->modelindex == 255){
			// Use custom player skin
			ci = &cl.clientInfo[state->skinnum & 0xff];
			if (!ci->valid)
				ci = &cl.baseClientInfo;

			ent.skinNum = 0;
			ent.model = ci->model;
			ent.customShader = ci->skin;

			if (state->renderfx & RF_USE_DISGUISE){
				if (!Q_strnicmp(ci->info, "male", 4))
					ent.customShader = R_RegisterShaderSkin("players/male/disguise");
				else if (!Q_strnicmp(ci->info, "female", 6))
					ent.customShader = R_RegisterShaderSkin("players/female/disguise");
				else if (!Q_strnicmp(ci->info, "cyborg", 6))
					ent.customShader = R_RegisterShaderSkin("players/cyborg/disguise");
			}
		}
		else {
			ent.skinNum = state->skinnum;
			ent.model = cl.media.gameModels[state->modelindex];
			ent.customShader = NULL;
		}

		// Set frame
		if (state->effects & EF_ANIM_ALL)
			ent.frame = animAll;
		else if (state->effects & EF_ANIM_ALLFAST)
			ent.frame = animAllFast;
		else if (state->effects & EF_ANIM01)
			ent.frame = anim01;
		else if (state->effects & EF_ANIM23)
			ent.frame = anim23;
		else
			ent.frame = state->frame;

		ent.oldFrame = cent->prev.frame;
		ent.backLerp = 1.0 - cl.lerpFrac;
		MakeRGBA(ent.shaderRGBA, 255, 255, 255, 255);

		// Only used for black hole model
		if (state->renderfx == RF_TRANSLUCENT)
			ent.shaderRGBA[3] = 178;

		if (state->effects & EF_SPHERETRANS){
			if (state->effects & EF_TRACKERTRAIL)
				ent.shaderRGBA[3] = 150;
			else
				ent.shaderRGBA[3] = 75;
		}

		// Render effects
		ent.renderFX = state->renderfx;

		if (state->number == cl.clientNum && !cl_thirdPerson->integer)
			ent.renderFX |= RF_VIEWERMODEL;		// Only draw from mirrors

		// Add to refresh list
		R_AddEntityToScene(&ent);

		// Color shells generate a separate entity for the main model
		if (cl_drawShells->integer)
			CL_AddShellEntity(&ent, state->effects, !(ent.renderFX & RF_VIEWERMODEL));

		// Make sure these get reset
		ent.skinNum = 0;
		ent.customShader = NULL;
		MakeRGBA(ent.shaderRGBA, 255, 255, 255, 255);

		// Duplicate for linked models
		if (state->modelindex2){
			if (state->modelindex2 == 255){
				// Use custom weapon
				ci = &cl.clientInfo[state->skinnum & 0xff];
				if (!ci->valid)
					ci = &cl.baseClientInfo;

				weapon = state->skinnum >> 8;
				if ((weapon < 0 || weapon >= MAX_CLIENTWEAPONMODELS) || !cl_visibleWeapons->integer)
					weapon = 0;

				if (ci->weaponModel[weapon])
					ent.model = ci->weaponModel[weapon];
				else
					ent.model = ci->weaponModel[0];
			}
			else {
				ent.model = cl.media.gameModels[state->modelindex2];

				// HACK: check for the defender sphere shell. Make it
				// translucent.
				if (!Q_stricmp(cl.configStrings[CS_MODELS+state->modelindex2], "models/items/shell/tris.md2"))
					ent.shaderRGBA[3] = 81;
			}

			R_AddEntityToScene(&ent);

			// Color shells generate a separate entity for the main model
			if (cl_drawShells->integer)
				CL_AddShellEntity(&ent, state->effects, false);

			// Make sure this gets reset
            ent.shaderRGBA[3] = 255;
		}
        if (state->modelindex3){
			ent.model = cl.media.gameModels[state->modelindex3];
            R_AddEntityToScene(&ent);

			// Color shells generate a separate entity for the main model
			if (cl_drawShells->integer)
				CL_AddShellEntity(&ent, state->effects, false);
		}
        if (state->modelindex4){
			ent.model = cl.media.gameModels[state->modelindex4];
            R_AddEntityToScene(&ent);

			// Color shells generate a separate entity for the main model
			if (cl_drawShells->integer)
				CL_AddShellEntity(&ent, state->effects, false);
		}

		// Power screen
        if (state->effects & EF_POWERSCREEN){
			ent.model = cl.media.modPowerScreenShell;
            ent.frame = 0;
            ent.oldFrame = 0;
			ent.customShader = cl.media.powerScreenShellShader;
			MakeRGBA(ent.shaderRGBA, 0, 255, 0, 75);

            R_AddEntityToScene(&ent);
			R_AddLightToScene(ent.origin, 250, 0, 1, 0);
		}

        // Add automatic trails
        if (state->effects & ~EF_ROTATE)
			CL_AddEntityTrails(cent, ent.origin, state->effects, (ent.renderFX & RF_VIEWERMODEL));

		VectorCopy(ent.origin, cent->lerpOrigin);
    }
}

/*
 =================
 CL_AddViewWeapon
 =================
*/
void CL_AddViewWeapon (void){

    entity_t		gun;
	entity_state_t	*state;
	vec3_t			angles;

	// Don't add if in third person view
	if (cl_thirdPerson->integer)
		return;

    // Allow the gun to be completely removed
    if (!cl_drawGun->integer || cl_hand->integer == 2)
        return;

	// Don't add if in wide angle view
	if (cl.playerState->fov > 90)
		return;

	// Don't add if testing a gun model
	if (cl.testGun)
		return;

    if (!cl.media.gameModels[cl.playerState->gunindex])
        return;

	memset(&gun, 0, sizeof(gun));

	gun.entityType = ET_MODEL;
	gun.renderFX = RF_MINLIGHT | RF_DEPTHHACK | RF_WEAPONMODEL;
	gun.model = cl.media.gameModels[cl.playerState->gunindex];
	gun.backLerp = 1.0 - cl.lerpFrac;
	gun.ammoValue = cl.playerState->stats[STAT_AMMO];
	MakeRGBA(gun.shaderRGBA, 255, 255, 255, 255);

    // Set up gun position and angles
	gun.origin[0] = gun.oldOrigin[0] = cl.refDef.viewOrigin[0] + cl.oldPlayerState->gunoffset[0] + (cl.playerState->gunoffset[0] - cl.oldPlayerState->gunoffset[0]) * cl.lerpFrac;
	gun.origin[1] = gun.oldOrigin[1] = cl.refDef.viewOrigin[1] + cl.oldPlayerState->gunoffset[1] + (cl.playerState->gunoffset[1] - cl.oldPlayerState->gunoffset[1]) * cl.lerpFrac;
	gun.origin[2] = gun.oldOrigin[2] = cl.refDef.viewOrigin[2] + cl.oldPlayerState->gunoffset[2] + (cl.playerState->gunoffset[2] - cl.oldPlayerState->gunoffset[2]) * cl.lerpFrac;

	angles[0] = cl.refDefViewAngles[0] + LerpAngle(cl.oldPlayerState->gunangles[0], cl.playerState->gunangles[0], cl.lerpFrac);
	angles[1] = cl.refDefViewAngles[1] + LerpAngle(cl.oldPlayerState->gunangles[1], cl.playerState->gunangles[1], cl.lerpFrac);
	angles[2] = cl.refDefViewAngles[2] + LerpAngle(cl.oldPlayerState->gunangles[2], cl.playerState->gunangles[2], cl.lerpFrac);

	AnglesToAxis(angles, gun.axis);

	if (cl_hand->integer == 1){
		VectorInverse(gun.axis[1]);

		gun.renderFX |= RF_CULLHACK;
	}

	// Set up gun frames
	gun.frame = cl.playerState->gunframe;
	gun.oldFrame = cl.oldPlayerState->gunframe;

	if (gun.frame == 0)
		gun.oldFrame = 0;	// Just changed weapons, don't lerp from old

    R_AddEntityToScene(&gun);

	// Color shells generate a separate entity for the main model
	if (cl_drawShells->integer){
		state = &cl.entities[cl.clientNum].current;
		if (state->number != cl.clientNum)
			return;		// This shouldn't happen

		gun.renderFX |= state->renderfx;

		if (state->effects & EF_COLOR_SHELL){
			// Remove godmode for weapon
			if ((gun.renderFX & RF_SHELL_RED) && (gun.renderFX & RF_SHELL_GREEN) && (gun.renderFX & RF_SHELL_BLUE)){
				CL_AddShellEntity(&gun, (state->effects & ~EF_COLOR_SHELL), true);
				return;
			}
		}

		CL_AddShellEntity(&gun, state->effects, true);
	}
}

/*
 =================
 CL_GetEntitySoundSpatialization

 Called by the sound system to get the sound spatialization origin and
 velocity for the given entity
 =================
*/
void CL_GetEntitySoundSpatialization (int ent, vec3_t origin, vec3_t velocity){

	centity_t	*cent;
	cmodel_t	*cmodel;
	vec3_t		midPoint;

	if (ent < 0 || ent >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_GetEntitySoundSpatialization: ent = %i", ent);

	cent = &cl.entities[ent];

	if (cent->current.renderfx & (RF_FRAMELERP|RF_BEAM)){
		// Calculate origin
		origin[0] = cent->current.old_origin[0] + (cent->current.origin[0] - cent->current.old_origin[0]) * cl.lerpFrac;
		origin[1] = cent->current.old_origin[1] + (cent->current.origin[1] - cent->current.old_origin[1]) * cl.lerpFrac;
		origin[2] = cent->current.old_origin[2] + (cent->current.origin[2] - cent->current.old_origin[2]) * cl.lerpFrac;

		// Calculate velocity
		VectorSubtract(cent->current.origin, cent->current.old_origin, velocity);
		VectorScale(velocity, 10, velocity);
	}
	else {
		// Calculate origin
		origin[0] = cent->prev.origin[0] + (cent->current.origin[0] - cent->prev.origin[0]) * cl.lerpFrac;
		origin[1] = cent->prev.origin[1] + (cent->current.origin[1] - cent->prev.origin[1]) * cl.lerpFrac;
		origin[2] = cent->prev.origin[2] + (cent->current.origin[2] - cent->prev.origin[2]) * cl.lerpFrac;

		// Calculate velocity
		VectorSubtract(cent->current.origin, cent->prev.origin, velocity);
		VectorScale(velocity, 10, velocity);
	}

	// If a brush model, offset the origin
	if (cent->current.solid == 31){
		cmodel = cl.media.gameCModels[cent->current.modelindex];
		if (!cmodel)
			return;

		VectorAverage(cmodel->mins, cmodel->maxs, midPoint);
		VectorAdd(origin, midPoint, origin);
	}
}
