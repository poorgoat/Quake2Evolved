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


#include "server.h"


/*
 =================
 SV_FindIndex
 =================
*/
static int SV_FindIndex (const char *name, int start, int max){

	int		i;
	
	if (!name || !name[0])
		return 0;

	for (i = 1; i < max && sv.configStrings[start+i][0]; i++){
		if (!Q_strcmp(sv.configStrings[start+i], name))
			return i;
	}

	if (i == max)
		Com_Error(ERR_DROP, "SV_FindIndex: overflow");

	Q_strncpyz(sv.configStrings[start+i], name, sizeof(sv.configStrings[start+i]));

	if (sv.state != SS_LOADING){
		// Send the update to everyone
		MSG_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, SVC_CONFIGSTRING);
		MSG_WriteShort(&sv.multicast, start+i);
		MSG_WriteString(&sv.multicast, name);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}

	return i;
}

/*
 =================
 SV_ModelIndex
 =================
*/
int SV_ModelIndex (const char *name){

	return SV_FindIndex(name, CS_MODELS, MAX_MODELS);
}

/*
 =================
 SV_SoundIndex
 =================
*/
int SV_SoundIndex (const char *name){

	return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS);
}

/*
 =================
 SV_ImageIndex
 =================
*/
int SV_ImageIndex (const char *name){

	return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES);
}

/*
 =================
 SV_CreateBaseline

 Entity baselines are used to compress the update messages to the 
 clients. Only the fields that differ from the baseline will be 
 transmitted.
 =================
*/
static void SV_CreateBaseLine (void){

	edict_t	*edict;
	int		e;	

	for (e = 1; e < ge->num_edicts; e++){
		edict = EDICT_NUM(e);
		if (!edict->inuse)
			continue;
		if (!edict->s.modelindex && !edict->s.sound && !edict->s.effects)
			continue;

		edict->s.number = e;

		// Take current state as baseline
		VectorCopy(edict->s.origin, edict->s.old_origin);
		sv.baselines[e] = edict->s;
	}
}

/*
 =================
 SV_CheckForSaveGame
 =================
*/
static void SV_CheckForSaveGame (void){

	serverState_t	previousState;
	char			name[MAX_QPATH];
	int				i;

	if (sv_noReload->integer)
		return;

	if (Cvar_VariableInteger("deathmatch"))
		return;

	Q_snprintfz(name, sizeof(name), "save/current/%s.sav", sv.name);
	if (FS_LoadFile(name, NULL) == -1)
		return;		// No savegame

	SV_ClearWorld();

	// Get configstrings and areaportals
	SV_ReadLevelFile();

	if (!sv.loadGame){
		// Coming back to a level after being in a different level, so 
		// run it for ten seconds
		previousState = sv.state;

		sv.state = SS_LOADING;			
		for (i = 0; i < 100; i++)
			ge->RunFrame();

		sv.state = previousState;		
	}
}

/*
 =================
 SV_SpawnServer

 Change the server to a new map, taking all connected clients along with 
 it
 =================
*/
static void SV_SpawnServer (const char *server, const char *spawnPoint, qboolean attractLoop, qboolean loadGame, serverState_t serverState){

	unsigned	checksum;
	int			i;

	Com_Printf("------- Server Initialization -------\n");
	Com_Printf("Server: %s\n", server);

	Cvar_ForceSet("paused", "0");

	if (dedicated->integer)
		Hunk_Clear();
	else {
		CL_Loading();

		CL_UpdateScreen();

		CL_ClearMemory();

		if (sv.state != SS_DEAD)
			Hunk_Clear();
	}

	if (sv.state == SS_DEAD && !loadGame)
		SV_InitGame();		// The game is just starting

	SV_BroadcastCommand("changing\n");
	if (serverState == SS_GAME)
		SV_SendClientMessages();

	svs.spawnCount++;		// Any partially connected client will be restarted
	svs.realTime = 0;

	sv.state = SS_DEAD;
	Com_SetServerState(sv.state);

	if (sv.demoFile)
		FS_FCloseFile(sv.demoFile);

	// Wipe the entire per-level structure
	memset(&sv, 0, sizeof(sv));

	sv.attractLoop = attractLoop;
	sv.loadGame = loadGame;
	Q_strncpyz(sv.name, server, sizeof(sv.name));
	Q_strncpyz(sv.configStrings[CS_NAME], server, sizeof(sv.configStrings[CS_NAME]));
	sv.time = 1000;

	if (Cvar_VariableInteger("deathmatch")){
		Q_snprintfz(sv.configStrings[CS_AIRACCEL], sizeof(sv.configStrings[CS_AIRACCEL]), "%g", sv_airAccelerate->value);
		pm_airAccelerate = sv_airAccelerate->value;
	}
	else {
		Q_snprintfz(sv.configStrings[CS_AIRACCEL], sizeof(sv.configStrings[CS_AIRACCEL]), "0");
		pm_airAccelerate = 0;
	}

	MSG_Init(&sv.multicast, sv.multicastBuffer, sizeof(sv.multicastBuffer), false);

	// Leave slots at start for clients only
	for (i = 0; i < sv_maxClients->integer; i++){
		// Needs to reconnect
		if (svs.clients[i].state > CS_CONNECTED)
			svs.clients[i].state = CS_CONNECTED;

		svs.clients[i].lastFrame = -1;
	}

	if (serverState == SS_GAME){
		Q_snprintfz(sv.configStrings[CS_MODELS+1], sizeof(sv.configStrings[CS_MODELS+1]), "maps/%s.bsp", server);
		sv.models[1] = CM_LoadMap(sv.configStrings[CS_MODELS+1], false, &checksum);
	}
	else {
		Q_snprintfz(sv.configStrings[CS_MODELS+1], sizeof(sv.configStrings[CS_MODELS+1]), "%s", server);
		sv.models[1] = CM_LoadMap(NULL, false, &checksum);	// No real map
	}
	Q_snprintfz(sv.configStrings[CS_MAPCHECKSUM], sizeof(sv.configStrings[CS_MAPCHECKSUM]), "%i", checksum);

	// Clear physics interaction links
	SV_ClearWorld();
	
	for (i = 1; i < CM_NumInlineModels(); i++){
		Q_snprintfz(sv.configStrings[CS_MODELS+1+i], sizeof(sv.configStrings[CS_MODELS+1+i]), "*%i", i);
		sv.models[i+1] = CM_InlineModel(sv.configStrings[CS_MODELS+1+i]);
	}

	// Spawn the rest of the entities on the map

	// Precache and static commands can be issued during map 
	// initialization
	sv.state = SS_LOADING;
	Com_SetServerState(sv.state);

	// Load and spawn all other entities
	ge->SpawnEntities(sv.name, CM_EntityString(), (char *)spawnPoint);

	// Run two frames to allow everything to settle
	ge->RunFrame();
	ge->RunFrame();

	// All precaches are complete
	sv.state = serverState;
	Com_SetServerState(sv.state);

	// Create a baseline for more efficient communications
	SV_CreateBaseLine();

	// Check for a savegame
	SV_CheckForSaveGame();

	if (serverState == SS_GAME)
		Cbuf_CopyToDefer();

	Cvar_ForceSet("sv_mapName", server);
	Cvar_ForceSet("sv_mapChecksum", va("%u", checksum));

	Com_Printf("-------------------------------------\n");
}

/*
 =================
 SV_GetLatchedVars
 =================
*/
static void SV_GetLatchedVars (void){

	Cvar_Get("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("skill", "1", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("coop", "0", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("deathmatch", "0", CVAR_SERVERINFO | CVAR_LATCH);
	sv_maxClients = Cvar_Get("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	sv_airAccelerate = Cvar_Get("sv_airAccelerate", "0", CVAR_LATCH);
}

/*
 =================
 SV_InitGame

 A brand new game has been started
 =================
*/
void SV_InitGame (void){

	int		e;
	edict_t	*edict;

	if (svs.initialized)
		// Cause any connected clients to reconnect
		SV_Shutdown("Server restarted\n", true);
	else
		// Make sure the client is down
		CL_Drop();

	// Get any latched variable changes
	SV_GetLatchedVars();

	// Restart file system
	FS_Restart();

	if (Cvar_VariableInteger("coop") && Cvar_VariableInteger("deathmatch")){
		Com_Printf("'deathmatch' and 'coop' both set, disabling 'coop'\n");
		Cvar_ForceSet("coop", "0");
	}

	// Dedicated servers can't be singleplayer and are usually DM so
	// unless they explicity set coop, force it to deathmatch
	if (dedicated->integer){
		if (!Cvar_VariableInteger("coop"))
			Cvar_ForceSet("deathmatch", "1");
	}

	// Init clients
	if (Cvar_VariableInteger("deathmatch")){
		if (sv_maxClients->integer <= 1)
			Cvar_ForceSet("maxclients", "8");
		else if (sv_maxClients->integer > MAX_CLIENTS)
			Cvar_ForceSet("maxclients", va("%i", MAX_CLIENTS));
	}
	else if (Cvar_VariableInteger("coop")){
		if (sv_maxClients->integer <= 1)
			Cvar_ForceSet("maxclients", "4");
		else if (sv_maxClients->integer > 8)
			Cvar_ForceSet("maxclients", "8");
	}
	else	// Single player
		Cvar_ForceSet("maxclients", "1");

	svs.initialized = true;
	svs.spawnCount = rand();
	svs.clients = Z_Malloc(sv_maxClients->integer * sizeof(client_t));
	svs.numClientEntities = sv_maxClients->integer * UPDATE_BACKUP*64;
	svs.clientEntities = Z_Malloc(svs.numClientEntities * sizeof(entity_state_t));

	// Send a heartbeat immediately
	svs.lastHeartbeat = -9999999;

	// Init game
	SV_InitGameProgs();

	for (e = 0; e < sv_maxClients->integer; e++){
		edict = EDICT_NUM(e+1);
		edict->s.number = e+1;
		svs.clients[e].edict = edict;
	}
}

/*
 =================
 SV_Map

 The full syntax is:

 map [*]<map><$startspot><+nextserver>

 Command from the console or progs.
 Map can also be a .cin, .pcx, or .dm2 file.
 Nextserver is used to allow a cinematic to play, then proceed to 
 another level:

 map ntro.cin+base1
 =================
*/
void SV_Map (const char *levelString, qboolean attractLoop, qboolean loadGame){

	char	server[MAX_QPATH], spawnPoint[MAX_QPATH];
	char	extension[8], *gameDir, *ch;

	Q_strncpyz(server, levelString, sizeof(server));

	// If there is a + in the map, set sv_nextServer to the remainder
	ch = strchr(server, '+');
	if (ch){
		*ch = 0;
		Cvar_Set("sv_nextServer", va("gamemap \"%s\"", ch+1));
	}
	else
		Cvar_Set("sv_nextServer", "");

	// Special hack for end game screen in coop mode
	if (Cvar_VariableInteger("coop") && !Q_stricmp(server, "victory.pcx")){
		gameDir = Cvar_VariableString("fs_game");

		if (!Q_stricmp(gameDir, "baseq2"))
			Cvar_Set("sv_nextServer", "gamemap \"*base1\"");
		else if (!Q_stricmp(gameDir, "xatrix"))
			Cvar_Set("sv_nextServer", "gamemap \"*xswamp\"");
		else if (!Q_stricmp(gameDir, "rogue"))
			Cvar_Set("sv_nextServer", "gamemap \"*rmine1\"");
	}

	// If there is a $, use the remainder as a spawn point
	ch = strchr(server, '$');
	if (ch){
		*ch = 0;
		Q_strncpyz(spawnPoint, ch+1, sizeof(spawnPoint));
	}
	else
		spawnPoint[0] = 0;

	// Skip the end-of-unit flag if necessary
	if (server[0] == '*')
		Q_strncpyz(server, server+1, sizeof(server));

	Com_FileExtension(server, extension, sizeof(extension));

	if (!Q_stricmp(extension, "pcx"))
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_PIC);
	else if (!Q_stricmp(extension, "cin"))
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_CINEMATIC);
	else if (!Q_stricmp(extension, "dm2"))
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_DEMO);
	else
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_GAME);

	SV_BroadcastCommand("reconnect\n");
}
