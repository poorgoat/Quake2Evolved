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


// sv_game.c -- interface to the game library


#include "server.h"


game_export_t	*ge;


/*
 =================
 SVG_Unicast

 Sends the contents of the multicast buffer to a single client
 =================
*/
static void SVG_Unicast (edict_t *edict, qboolean reliable){

	int			e;
	client_t	*client;

	if (!edict)
		return;

	e = NUM_FOR_EDICT(edict);
	if (e < 1 || e > sv_maxClients->integerValue)
		return;

	client = svs.clients + (e-1);

	if (reliable)
		MSG_Write(&client->netChan.message, sv.multicast.data, sv.multicast.curSize);
	else
		MSG_Write(&client->datagram, sv.multicast.data, sv.multicast.curSize);

	MSG_Clear(&sv.multicast);
}

/*
 =================
 SVG_DPrintf

 Debug print to server console
 =================
*/
static void SVG_DPrintf (char *fmt, ...){

	char	string[1024];
	va_list	argPtr;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), 1024, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	Com_Printf("%s", string);
}

/*
 =================
 SVG_ClientPrintf

 Print to a single client
 =================
*/
static void SVG_ClientPrintf (edict_t *edict, int level, char *fmt, ...){

	char	string[1024];
	va_list	argPtr;
	int		e;

	if (edict){
		e = NUM_FOR_EDICT(edict);
		if (e < 1 || e > sv_maxClients->integerValue)
			Com_Error(ERR_DROP, "SVG_ClientPrintf: client = %i", e);
	}

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), 1024, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	if (edict)
		SV_ClientPrintf(svs.clients + (e-1), level, "%s", string);
	else
		Com_Printf("%s", string);
}

/*
 =================
 SVG_CenterPrintf

 Center print to a single client
 =================
*/
static void SVG_CenterPrintf (edict_t *edict, char *fmt, ...){

	char	string[1024];
	va_list	argPtr;
	int		e;

	e = NUM_FOR_EDICT(edict);
	if (e < 1 || e > sv_maxClients->integerValue)
		return;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), 1024, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	MSG_WriteByte(&sv.multicast, SVC_CENTERPRINT);
	MSG_WriteString(&sv.multicast, string);
	SVG_Unicast(edict, true);
}

/*
 =================
 SVG_Error

 Abort the server with a game error
 =================
*/
static void SVG_Error (char *fmt, ...){

	char string[1024];
	va_list	argPtr;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), 1024, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	Com_Error(ERR_DROP, "Game Error: %s", string);
}

/*
 =================
 SVG_SetModel

 Also sets mins and maxs for inline models
 =================
*/
static void SVG_SetModel (edict_t *edict, char *name){

	cmodel_t	*mod;

	if (!edict || !name)
		Com_Error(ERR_DROP, "SVG_SetModel: NULL parameter");

	edict->s.modelindex = SV_ModelIndex(name);

	// If it is an inline model, get the size information for it
	if (name[0] == '*'){
		mod = CM_InlineModel(name);
		VectorCopy(mod->mins, edict->mins);
		VectorCopy(mod->maxs, edict->maxs);
		SV_LinkEdict(edict);
	}
}

/*
 =================
 SVG_ConfigString
 =================
*/
static void SVG_ConfigString (int index, char *string){

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error(ERR_DROP, "SVG_ConfigString: index = %i", index);

	if (!string)
		string = "";

	// Change the string in sv
#ifdef SECURE
	strcpy_s(sv.configStrings[index], sizeof(sv.configStrings[index]), string);
#else
	strcpy(sv.configStrings[index], string);
#endif

	if (sv.state != SS_LOADING){
		// Send the update to everyone
		MSG_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, SVC_CONFIGSTRING);
		MSG_WriteShort(&sv.multicast, index);
		MSG_WriteString(&sv.multicast, string);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}
}

/*
 =================
 SVG_WriteChar
 =================
*/
static void SVG_WriteChar (int c){

	MSG_WriteChar(&sv.multicast, c);
}

/*
 =================
 SVG_WriteByte
 =================
*/
static void SVG_WriteByte (int b){

	MSG_WriteByte(&sv.multicast, b);
}

/*
 =================
 SVG_WriteShort
 =================
*/
static void SVG_WriteShort (int s){

	MSG_WriteShort(&sv.multicast, s);
}

/*
 =================
 SVG_WriteLong
 =================
*/
static void SVG_WriteLong (int l){

	MSG_WriteLong(&sv.multicast, l);
}

/*
 =================
 SVG_WriteFloat
 =================
*/
static void SVG_WriteFloat (float f){

	MSG_WriteFloat(&sv.multicast, f);
}

/*
 =================
 SVG_WriteString
 =================
*/
static void SVG_WriteString (char *s){

	MSG_WriteString(&sv.multicast, s);
}

/*
 =================
 SVG_WritePos
 =================
*/
static void SVG_WritePos (vec3_t pos){

	MSG_WritePos(&sv.multicast, pos);
}

/*
 =================
 SVG_WriteDir
 =================
*/
static void SVG_WriteDir (vec3_t dir){

	MSG_WriteDir(&sv.multicast, dir);
}

/*
 =================
 SVG_WriteAngle
 =================
*/
static void SVG_WriteAngle (float a){

	MSG_WriteAngle(&sv.multicast, a);
}

/*
 =================
 SVG_InPVS

 Also checks portal areas so that doors block sight
 =================
*/
static qboolean SVG_InPVS (vec3_t p1, vec3_t p2){

	int		leafNum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafNum = CM_PointLeafNum(p1);
	cluster = CM_LeafCluster(leafNum);
	area1 = CM_LeafArea(leafNum);
	mask = CM_ClusterPVS(cluster);

	leafNum = CM_PointLeafNum(p2);
	cluster = CM_LeafCluster(leafNum);
	area2 = CM_LeafArea(leafNum);

	if (!(mask[cluster>>3] & (1<<(cluster&7))))
		return false;

	if (!CM_AreasConnected(area1, area2))
		return false;		// A door blocks sight

	return true;
}

/*
 =================
 SVG_InPHS

 Also checks portal areas so that doors block sound
 =================
*/
static qboolean SVG_InPHS (vec3_t p1, vec3_t p2){

	int		leafNum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafNum = CM_PointLeafNum(p1);
	cluster = CM_LeafCluster(leafNum);
	area1 = CM_LeafArea(leafNum);
	mask = CM_ClusterPHS(cluster);

	leafNum = CM_PointLeafNum(p2);
	cluster = CM_LeafCluster(leafNum);
	area2 = CM_LeafArea(leafNum);

	if (!(mask[cluster>>3] & (1<<(cluster&7))))
		return false;

	if (!CM_AreasConnected(area1, area2))
		return false;		// A door blocks hearing

	return true;
}

/*
 =================
 SVG_StartSound
 =================
*/
static void SVG_StartSound (edict_t *edict, int channel, int sound, float volume, float attenuation, float timeOfs){

	if (!edict)
		return;

	SV_StartSound(NULL, edict, channel, sound, volume, attenuation, timeOfs);
}

/*
 =================
 SVG_DebugGraph

 Does nothing. Just a dummy function to keep backwards compatibility.
 =================
*/
static void SVG_DebugGraph (float value, int color){

}

/*
 =================
 SVG_Alloc
 =================
*/
static void *SVG_Alloc (int size, int tag){

	byte	*ptr;

	ptr = Z_TagMalloc(size, tag);
	memset(ptr, 0, size);

	return ptr;
}

/*
 =================
 SVG_Free
 =================
*/
static void SVG_Free (void *ptr){

	Z_Free(ptr);
}

/*
 =================
 SVG_FreeTags
 =================
*/
static void SVG_FreeTags (int tag){

	Z_FreeTags(tag);
}


// =====================================================================


/*
 =================
 SV_InitGameProgs

 Init the game subsystem for a new map
 =================
*/
void SV_InitGameProgs (void){

	game_import_t	import;

	// Unload anything we have now
	if (ge)
		SV_ShutdownGameProgs();

	// Load a new game library
	import.unicast = SVG_Unicast;
	import.dprintf = SVG_DPrintf;
	import.cprintf = SVG_ClientPrintf;
	import.centerprintf = SVG_CenterPrintf;
	import.error = SVG_Error;
	import.setmodel = SVG_SetModel;
	import.configstring = SVG_ConfigString;
	import.WriteChar = SVG_WriteChar;
	import.WriteByte = SVG_WriteByte;
	import.WriteShort = SVG_WriteShort;
	import.WriteLong = SVG_WriteLong;
	import.WriteFloat = SVG_WriteFloat;
	import.WriteString = SVG_WriteString;
	import.WritePosition = SVG_WritePos;
	import.WriteDir = SVG_WriteDir;
	import.WriteAngle = SVG_WriteAngle;
	import.inPVS = SVG_InPVS;
	import.inPHS = SVG_InPHS;
	import.sound = SVG_StartSound;
	import.DebugGraph = SVG_DebugGraph;
	import.TagMalloc = SVG_Alloc;
	import.TagFree = SVG_Free;
	import.FreeTags = SVG_FreeTags;

	import.multicast = SV_Multicast;
	import.bprintf = SV_BroadcastPrintf;
	import.positioned_sound = SV_StartSound;
	import.linkentity = SV_LinkEdict;
	import.unlinkentity = SV_UnlinkEdict;
	import.BoxEdicts = SV_AreaEdicts;
	import.trace = SV_Trace;
	import.pointcontents = SV_PointContents;
	import.Pmove = PMove;

	import.modelindex = SV_ModelIndex;
	import.soundindex = SV_SoundIndex;
	import.imageindex = SV_ImageIndex;

	import.cvar = Cvar_GameGet;
	import.cvar_set = Cvar_GameSet;
	import.cvar_forceset = Cvar_GameForceSet;

	import.argc = Cmd_Argc;
	import.argv = Cmd_Argv;
	import.args = Cmd_Args;
	import.AddCommandString = Cbuf_AddText;

	import.SetAreaPortalState = CM_SetAreaPortalState;
	import.AreasConnected = CM_AreasConnected;

	ge = (game_export_t *)Sys_LoadGame(&import);

	if (ge->apiversion != GAME_API_VERSION)
		Com_Error(ERR_FATAL, "Game version is %i, expected %i", ge->apiversion, GAME_API_VERSION);

	ge->Init();
}

/*
 =================
 SV_ShutdownGameProgs

 Called when either the entire server is being killed, or it is changing 
 to a different game directory
 =================
*/
void SV_ShutdownGameProgs (void){

	if (!ge)
		return;
	
	ge->Shutdown();
	ge = NULL;

	Sys_UnloadGame();
}
