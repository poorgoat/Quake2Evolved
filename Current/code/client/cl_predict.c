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


#include "client.h"


static entity_state_t	*cl_solidEntities[MAX_PARSE_ENTITIES];
static int				cl_numSolidEntities;


/*
 =================
 CL_BuildSolidList
 =================
*/
void CL_BuildSolidList (void){

	entity_state_t	*ent;
	int				i;

	cl_numSolidEntities = 0;

	for (i = 0; i < cl.frame.numEntities; i++){
		ent = &cl.parseEntities[(cl.frame.parseEntitiesIndex+i) & (MAX_PARSE_ENTITIES-1)];
		if (!ent->solid)
			continue;

		cl_solidEntities[cl_numSolidEntities++] = ent;
	}
}

/*
 =================
 CL_Trace
 =================
*/
trace_t CL_Trace (const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int brushMask, qboolean brushOnly, int *entNumber){

	trace_t			trace, tmp;
	entity_state_t	*ent;
	cmodel_t		*cmodel;
	vec3_t			bmins, bmaxs;
	int				i, xy, zd, zu, headNode;

	// Check against world
	trace = CM_BoxTrace(start, end, mins, maxs, 0, brushMask);
	if (trace.fraction < 1.0){
		if (entNumber)
			*entNumber = 0;

		trace.ent = (struct edict_s *)1;
	}

	if (trace.allsolid || trace.fraction == 0.0)
		return trace;

	// Check all other solid models
	for (i = 0; i < cl_numSolidEntities; i++){
		ent = cl_solidEntities[i];

		if (ent->number == skipNumber)
			continue;

		if (ent->solid == 31){
			// Special value for brush model
			cmodel = cl.media.gameCModels[ent->modelindex];
			if (!cmodel)
				continue;

			tmp = CM_TransformedBoxTrace(start, end, mins, maxs, cmodel->headNode, brushMask, ent->origin, ent->angles);
		}
		else {
			if (brushOnly)
				continue;

			// Encoded bounding box
			xy = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -xy;
			bmaxs[0] = bmaxs[1] = xy;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headNode = CM_HeadNodeForBox(bmins, bmaxs);
			tmp = CM_TransformedBoxTrace(start, end, mins, maxs, headNode, brushMask, ent->origin, vec3_origin);
		}

		if (tmp.allsolid || tmp.startsolid || tmp.fraction < trace.fraction){
			if (entNumber)
				*entNumber = ent->number;

			tmp.ent = (struct edict_s *)ent;
			if (trace.startsolid){
				trace = tmp;
				trace.startsolid = true;
			}
			else
				trace = tmp;
		}
		else if (tmp.startsolid)
			trace.startsolid = true;

		if (trace.allsolid)
			break;
	}

	return trace;
}

/*
 =================
 CL_PointContents
 =================
*/
int	CL_PointContents (const vec3_t point, int skipNumber){

	entity_state_t	*ent;
	cmodel_t		*cmodel;
	int				i, contents;

	contents = CM_PointContents(point, 0);

	for (i = 0; i < cl_numSolidEntities; i++){
		ent = cl_solidEntities[i];

		if (ent->number == skipNumber)
			continue;

		if (ent->solid != 31)	// Special value for brush model
			continue;

		cmodel = cl.media.gameCModels[ent->modelindex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents(point, cmodel->headNode, ent->origin, ent->angles);
	}

	return contents;
}

/*
 =================
 CL_PMTrace
 =================
*/
static trace_t CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end){

	return CL_Trace(start, mins, maxs, end, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
}

/*
 =================
 CL_PMPointContents
 =================
*/
static int CL_PMPointContents (vec3_t point){

	return CL_PointContents(point, -1);
}

/*
 =================
 CL_CheckPredictionError
 =================
*/
void CL_CheckPredictionError (void){

	int		frame;
	int		delta[3];

	if (!cl_predict->integerValue || (cl.frame.playerState.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// Calculate the last usercmd_t we sent that the server has
	// processed
	frame = cls.netChan.incomingAcknowledged & CMD_MASK;

	// Compare what the server returned with what we had predicted it to
	// be
	delta[0] = cl.frame.playerState.pmove.origin[0] - cl.predictedOrigins[frame][0];
	delta[1] = cl.frame.playerState.pmove.origin[1] - cl.predictedOrigins[frame][1];
	delta[2] = cl.frame.playerState.pmove.origin[2] - cl.predictedOrigins[frame][2];

	// Save the prediction error for interpolation
	if (abs(delta[0]) + abs(delta[1]) + abs(delta[2]) > 640)
		// A teleport or something
		VectorClear(cl.predictedError);
	else {
		if (cl_showMiss->integerValue && (delta[0] || delta[1] || delta[2]))
			Com_Printf("Prediction miss on %i: %i\n", cl.frame.serverFrame, delta[0] + delta[1] + delta[2]);

		cl.predictedOrigins[frame][0] = cl.frame.playerState.pmove.origin[0];
		cl.predictedOrigins[frame][1] = cl.frame.playerState.pmove.origin[1];
		cl.predictedOrigins[frame][2] = cl.frame.playerState.pmove.origin[2];

		// Save for error interpolation
		cl.predictedError[0] = delta[0] * 0.125;
		cl.predictedError[1] = delta[1] * 0.125;
		cl.predictedError[2] = delta[2] * 0.125;
	}
}

/*
 =================
 CL_PredictMovement

 Sets cl.predictedOrigin and cl.predictedAngles
 =================
*/
void CL_PredictMovement (void){

	int		ack, current;
	int		frame, step;
	pmove_t	pm;

	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || com_paused->integerValue)
		return;

	if (!cl_predict->integerValue || (cl.frame.playerState.pmove.pm_flags & PMF_NO_PREDICTION)){
		// Just set angles
		cl.predictedAngles[0] = cl.viewAngles[0] + SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[0]);
		cl.predictedAngles[1] = cl.viewAngles[1] + SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[1]);
		cl.predictedAngles[2] = cl.viewAngles[2] + SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[2]);
		
		return;
	}

	ack = cls.netChan.incomingAcknowledged;
	current = cls.netChan.outgoingSequence;

	// If we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP){
		if (cl_showMiss->integerValue)
			Com_Printf("CL_PredictMovement: exceeded CMD_BACKUP\n");

		return;	
	}

	// Copy current state to pmove
	memset(&pm, 0, sizeof(pmove_t));

	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMPointContents;
	pm_airAccelerate = atof(cl.configStrings[CS_AIRACCEL]);
	pm.s = cl.frame.playerState.pmove;

	// Run frames
	while (++ack < current){
		frame = ack & CMD_MASK;
		pm.cmd = cl.cmds[frame];

		PMove(&pm);

		// Save for debug checking
		cl.predictedOrigins[frame][0] = pm.s.origin[0];
		cl.predictedOrigins[frame][1] = pm.s.origin[1];
		cl.predictedOrigins[frame][2] = pm.s.origin[2];
	}

	// Smooth out stair climbing
	if (pm.s.pm_flags & PMF_ON_GROUND){
		step = pm.s.origin[2] - cl.predictedOrigins[(ack-2) & CMD_MASK][2];
		if (step > 63 && step < 160){
			cl.predictedStep = step * 0.125;
			cl.predictedStepTime = cls.realTime - cls.frameTime * 500;
		}
	}

	// Copy results out for rendering
	cl.predictedOrigin[0] = pm.s.origin[0] * 0.125;
	cl.predictedOrigin[1] = pm.s.origin[1] * 0.125;
	cl.predictedOrigin[2] = pm.s.origin[2] * 0.125;

	VectorCopy(pm.viewangles, cl.predictedAngles);
}
