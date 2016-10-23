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


// sv_ccmds.c -- operator console only commands


#include "server.h"


/*
 =======================================================================

 SAVEGAME FILES

 =======================================================================
*/


/*
 =================
 SV_WipeSaveGame

 Delete save/<XXX>/
 =================
*/
static void SV_WipeSaveGame (const char *saveName){

	char	name[MAX_QPATH];
	char	files[4096], *ptr;
	int		num, len, i;

	Com_DPrintf("SV_WipeSaveGame( %s )\n", saveName);

	Q_snprintfz(name, sizeof(name), "save/%s/server.ssv", saveName);
	FS_DeleteFile(name);
	Q_snprintfz(name, sizeof(name), "save/%s/game.ssv", saveName);
	FS_DeleteFile(name);

	Q_snprintfz(name, sizeof(name), "save/%s", saveName);
	num = FS_GetFileList(name, "sav", files, sizeof(files));
	for (i = 0, ptr = files; i < num; i++, ptr += len){
		len = strlen(ptr) + 1;

		Q_snprintfz(name, sizeof(name), "save/%s/%s", saveName, ptr);
		FS_DeleteFile(name);
	}

	Q_snprintfz(name, sizeof(name), "save/%s", saveName);
	num = FS_GetFileList(name, "sv2", files, sizeof(files));
	for (i = 0, ptr = files; i < num; i++, ptr += len){
		len = strlen(ptr) + 1;

		Q_snprintfz(name, sizeof(name), "save/%s/%s", saveName, ptr);
		FS_DeleteFile(name);
	}
}

/*
 =================
 SV_CopySaveGame
 =================
*/
static void SV_CopySaveGame (const char *src, const char *dst){

	char	path[MAX_QPATH], nameSrc[MAX_QPATH], nameDst[MAX_QPATH];
	char	files[4096], *ptr;
	int		num, len, i;

	SV_WipeSaveGame(dst);

	Com_DPrintf("SV_CopySaveGame( %s, %s )\n", src, dst);

	// Copy the savegame over
	Q_snprintfz(nameSrc, sizeof(nameSrc), "save/%s/server.ssv", src);
	Q_snprintfz(nameDst, sizeof(nameDst), "save/%s/server.ssv", dst);
	FS_CopyFile(nameSrc, nameDst);

	Q_snprintfz(nameSrc, sizeof(nameSrc), "save/%s/game.ssv", src);
	Q_snprintfz(nameDst, sizeof(nameDst), "save/%s/game.ssv", dst);
	FS_CopyFile(nameSrc, nameDst);

	Q_snprintfz(path, sizeof(path), "save/%s", src);
	num = FS_GetFileList(path, "sav", files, sizeof(files));
	for (i = 0, ptr = files; i < num; i++, ptr += len){
		len = strlen(ptr) + 1;

		Q_snprintfz(nameSrc, sizeof(nameSrc), "save/%s/%s", src, ptr);
		Q_snprintfz(nameDst, sizeof(nameDst), "save/%s/%s", dst, ptr);
		FS_CopyFile(nameSrc, nameDst);

		// Change sav to sv2
		nameSrc[strlen(nameSrc)-3] = 0;
		Q_strncatz(nameSrc, "sv2", sizeof(nameSrc));
		nameDst[strlen(nameDst)-3] = 0;
		Q_strncatz(nameDst, "sv2", sizeof(nameDst));
		FS_CopyFile(nameSrc, nameDst);
	}
}

/*
 =================
 SV_WriteServerFile
 =================
*/
static void SV_WriteServerFile (qboolean autoSave){

	fileHandle_t	f;
	cvar_t			*var;
	char			comment[32];
	char			name[128], string[128];
	time_t			clock;
	struct tm		*ltime;

	Com_DPrintf("SV_WriteServerFile( %s )\n", autoSave ? "true" : "false");

	Q_snprintfz(name, sizeof(name), "save/current/server.ssv");
	FS_FOpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}
	
	// Write the comment field
	if (!autoSave){
		time(&clock);
		ltime = localtime(&clock);
		Q_snprintfz(comment, sizeof(comment), "%2i:%i%i %2i/%2i  ", ltime->tm_hour, ltime->tm_min/10, ltime->tm_min%10, ltime->tm_mon+1, ltime->tm_mday);
		Q_strncatz(comment, sv.configStrings[CS_NAME], sizeof(comment));
	}
	else
		// Autosaved
		Q_snprintfz(comment, sizeof(comment), "ENTERING %s", sv.configStrings[CS_NAME]);
	
	FS_Write(comment, sizeof(comment), f);

	// Write the map cmd
	FS_Write(svs.mapCmd, sizeof(svs.mapCmd), f);

	// Write all CVAR_LATCH cvars
	// These will be things like coop, skill, deathmatch, etc...
	for (var = cvar_vars; var; var = var->next){
		if (!(var->flags & CVAR_LATCH))
			continue;

		if (strlen(var->name) >= sizeof(name) || strlen(var->string) >= sizeof(string)){
			Com_DPrintf("Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}

		Q_strncpyz(name, var->name, sizeof(name));
		Q_strncpyz(string, var->string, sizeof(string));
		FS_Write(name, sizeof(name), f);
		FS_Write(string, sizeof(string), f);
	}

	FS_FCloseFile(f);

	// Write game state
	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/game.ssv", Cvar_VariableString("fs_homePath"), Cvar_VariableString("fs_game"));
	ge->WriteGame(name, autoSave);
}

/*
 =================
 SV_ReadServerFile
 =================
*/
static void SV_ReadServerFile (void){

	fileHandle_t	f;
	int				size;
	char			comment[32], mapCmd[128];
	char			name[128], string[128];

	Com_DPrintf("SV_ReadServerFile()\n");

	Q_snprintfz(name, sizeof(name), "save/current/server.ssv");
	size = FS_FOpenFile(name, &f, FS_READ);
	if (!f){
		Com_Printf("Couldn't read %s\n", name);
		return;
	}

	// Read the comment field
	FS_Read(comment, sizeof(comment), f);

	// Read the map cmd
	FS_Read(mapCmd, sizeof(mapCmd), f);

	// Read all CVAR_LATCH cvars
	// These will be things like coop, skill, deathmatch, etc...
	while (FS_Tell(f) < size){
		if (!FS_Read(name, sizeof(name), f))
			break;

		FS_Read(string, sizeof(string), f);
		Cvar_Set(name, string);
	}

	FS_FCloseFile(f);

	// Start a new game fresh with new cvars
	SV_InitGame();

	Q_strncpyz(svs.mapCmd, mapCmd, sizeof(svs.mapCmd));

	// Read game state
	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/game.ssv", Cvar_VariableString("fs_homePath"), Cvar_VariableString("fs_game"));
	ge->ReadGame(name);
}

/*
 =================
 SV_WriteLevelFile
 =================
*/
void SV_WriteLevelFile (void){

	char			name[MAX_QPATH];
	fileHandle_t	f;

	Com_DPrintf("SV_WriteLevelFile()\n");

	Q_snprintfz(name, sizeof(name), "save/current/%s.sv2", sv.name);
	FS_FOpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Failed to open %s\n", name);
		return;
	}

	FS_Write(sv.configStrings, sizeof(sv.configStrings), f);
	CM_WritePortalState(f);
	FS_FCloseFile(f);

	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/%s.sav", Cvar_VariableString("fs_homePath"), Cvar_VariableString("fs_game"), sv.name);
	ge->WriteLevel(name);
}

/*
 =================
 SV_ReadLevelFile
 =================
*/
void SV_ReadLevelFile (void){

	char			name[MAX_QPATH];
	fileHandle_t	f;

	Com_DPrintf("SV_ReadLevelFile()\n");

	Q_snprintfz(name, sizeof(name), "save/current/%s.sv2", sv.name);
	FS_FOpenFile(name, &f, FS_READ);
	if (!f){
		Com_Printf("Failed to open %s\n", name);
		return;
	}

	FS_Read(sv.configStrings, sizeof(sv.configStrings), f);
	CM_ReadPortalState(f);
	FS_FCloseFile(f);

	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/%s.sav", Cvar_VariableString("fs_homePath"), Cvar_VariableString("fs_game"), sv.name);
	ge->ReadLevel(name);
}

/*
 =================
 SV_LoadGame_f
 =================
*/
void SV_LoadGame_f (void){

	char	name[MAX_QPATH];
	char	*dir;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: loadgame <directory>\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (strstr(dir, "..") || strstr(dir, "/") || strstr(dir, "\\")){
		Com_Printf("Bad save directory\n");
		return;
	}

	if (!Q_stricmp(dir, "current")){
		Com_Printf("Can't load from 'current'\n");
		return;
	}

	// Make sure the server.ssv file exists
	Q_snprintfz(name, sizeof(name), "save/%s/server.ssv", dir);
	if (FS_LoadFile(name, NULL) == -1){
		Com_Printf("No such savegame '%s'\n", name);
		return;
	}

	Com_Printf("Loading game...\n");

	SV_CopySaveGame(dir, "current");

	SV_ReadServerFile();

	// Don't save current level when changing
	sv.state = SS_DEAD;
	Com_SetServerState(sv.state);

	// Go to the map
	SV_Map(svs.mapCmd, false, true);
}

/*
 =================
 SV_SaveGame_f
 =================
*/
void SV_SaveGame_f (void){

	char	*dir;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: savegame <directory>\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (strstr(dir, "..") || strstr(dir, "/") || strstr(dir, "\\")){
		Com_Printf("Bad save directory\n");
		return;
	}

	if (sv.state != SS_GAME){
		Com_Printf("You must be in a game to save\n");
		return;
	}

	if (Cvar_VariableInteger("deathmatch")){
		Com_Printf("Can't save in a deathmatch\n");
		return;
	}

	if (!Q_stricmp(dir, "current")){
		Com_Printf("Can't save to 'current'\n");
		return;
	}

	if (sv_maxClients->integer == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0){
		Com_Printf("Can't save while dead!\n");
		return;
	}

	Com_Printf("Saving game...\n");

	// Archive current level, including all client edicts.
	// When the level is reloaded, they will be shells awaiting a
	// connecting client.
	SV_WriteLevelFile();

	// Save server state
	SV_WriteServerFile(false);

	// Copy it off
	SV_CopySaveGame("current", dir);

	Com_Printf("Done\n");
}

/*
 =================
 SV_DeleteGame_f
 =================
*/
void SV_DeleteGame_f (void){

	char	name[MAX_QPATH];
	char	*dir;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: deletegame <directory>\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (strstr(dir, "..") || strstr(dir, "/") || strstr(dir, "\\")){
		Com_Printf("Bad save directory\n");
		return;
	}

	// Make sure the server.ssv file exists
	Q_snprintfz(name, sizeof(name), "save/%s/server.ssv", dir);
	if (FS_LoadFile(name, NULL) == -1){
		Com_Printf("No such savegame '%s'\n", name);
		return;
	}

	Com_Printf("Deleting game...\n");

	SV_WipeSaveGame(dir);

	Q_snprintfz(name, sizeof(name), "save/%s", dir);
	FS_RemovePath(name);

	Com_Printf("Done\n");
}


// =====================================================================


/*
 =================
 SV_GameMap

 Saves the state of the map just being exited and goes to a new map.

 If the initial character of the map string is '*', the next map is in a
 new unit, so the current savegame directory is cleared of map files.

 Example:

 *ntro.cin+base1

 Clears the archived maps, plays the ntro.cin cinematic, then goes to 
 map base1.bsp.
 =================
*/
static void SV_GameMap (const char *map){

	int			i;
	client_t	*cl;
	qboolean	savedInuse[MAX_CLIENTS];

	Com_DPrintf("SV_GameMap( %s )\n", map);

	// Check for clearing the current savegame
	if (map[0] == '*')
		// Wipe all the *.sav files
		SV_WipeSaveGame("current");
	else {
		// Save the map just exited
		if (sv.state == SS_GAME){
			// Clear all the client inuse flags before saving so that
			// when the level is re-entered, the clients will spawn at
			// spawn points instead of occupying body shells
			for (i = 0, cl = svs.clients; i < sv_maxClients->integer; i++, cl++){
				savedInuse[i] = cl->edict->inuse;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile();

			// We must restore these for clients to transfer over 
			// correctly
			for (i = 0, cl = svs.clients; i < sv_maxClients->integer; i++, cl++)
				cl->edict->inuse = savedInuse[i];
		}
	}

	// Start up the next map
	SV_Map(map, false, false);

	// Archive server state
	Q_strncpyz(svs.mapCmd, Cmd_Argv(1), sizeof(svs.mapCmd));

	// Copy off the level to the autosave slot
	if (!dedicated->integer){
		SV_WriteServerFile(true);
		SV_CopySaveGame("current", "save0");
	}
}

/*
 =================
 SV_GameMap_f

 Can be map, demo or cinematic
 =================
*/
void SV_GameMap_f (void){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: gamemap <map>\n");
		return;
	}

	SV_GameMap(Cmd_Argv(1));
}

/*
 =================
 SV_Map_f

 Goes directly to a given map without any savegame archiving.
 For development work.
 =================
*/
void SV_Map_f (void){

	char	map[MAX_QPATH];
	char	checkName[MAX_QPATH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: map <mapname>\n");
		return;
	}

	Q_strncpyz(map, Cmd_Argv(1), sizeof(map));

	// If not a demo or cinematic, check to make sure the level exists
	if (!strchr(map, '.')){
		Q_snprintfz(checkName, sizeof(checkName), "maps/%s.bsp", map);
		if (FS_LoadFile(checkName, NULL) == -1){
			Com_Printf("Can't find %s\n", checkName);
			return;
		}
	}

	// Don't save current level when changing
	sv.state = SS_DEAD;
	Com_SetServerState(sv.state);

	SV_WipeSaveGame("current");

	SV_GameMap(map);
}

/*
 =================
 SV_Demo_f

 Runs a demo
 =================
*/
void SV_Demo_f (void){

	char	map[MAX_QPATH];
	char	checkName[MAX_QPATH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: demo <demoname>\n");
		return;
	}

	Q_strncpyz(map, Cmd_Argv(1), sizeof(map));
	Com_DefaultExtension(map, sizeof(map), ".dm2");

	// Make sure it exists
	Q_snprintfz(checkName, sizeof(checkName), "demos/%s", map);
	if (FS_LoadFile(checkName, NULL) == -1){
		Com_Printf("Can't find %s\n", checkName);
		return;
	}

	SV_Map(map, true, false);
}


// =====================================================================


/*
 =================
 SV_SetPlayer

 Sets sv_client and sv_player to the player with userId
 =================
*/
static qboolean SV_SetPlayer (const char *userId){

	client_t	*cl;
	char		name[32];
	int			i, idNum;

	// Check for a name match
	for (i = 0, cl = svs.clients; i < sv_maxClients->integer; i++, cl++){
		if (cl->state == CS_FREE)
			continue;

		// Remove color escapes in client name
		Q_strncpyz(name, cl->name, sizeof(name));
		Q_CleanStr(name);

		if (!Q_stricmp(cl->name, userId) || !Q_stricmp(name, userId)){
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	// If a numeric value, check slots
	for (i = 0; userId[i]; i++){
		if (userId[i] < '0' || userId[i] > '9'){
			Com_Printf("Client '%s' is not on the server\n", userId);
			return false;
		}
	}

	idNum = atoi(userId);
	if (idNum >= sv_maxClients->integer){
		Com_Printf("Bad client slot %i\n", idNum);
		return false;
	}

	if (svs.clients[idNum].state == CS_FREE){
		Com_Printf("Client %i is not active\n", idNum);
		return false;
	}

	sv_client = &svs.clients[idNum];
	sv_player = sv_client->edict;

	return true;
}

/*
 =================
 SV_Kick_f

 Kick a user off the server
 =================
*/
void SV_Kick_f (void){

	if (!svs.initialized){
		Com_Printf("No server running\n");
		return;
	}

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: kick <userid>\n");
		return;
	}

	if (!SV_SetPlayer(Cmd_Argv(1)))
		return;

	SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", sv_client->name);

	// Print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf(sv_client, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient(sv_client);

	sv_client->lastMessage = svs.realTime;	// Min case there is a funny zombie
}

/*
 =================
 SV_Status_f
 =================
*/
void SV_Status_f (void){

	int			i, j;
	client_t	*cl;
	char		*s;
	int			ping, len;

	if (!svs.initialized){
		Com_Printf("No server running\n");
		return;
	}

	Com_Printf("Map: %s\n", sv.name);

	Com_Printf("num score ping lastmsg address               qport  rate  name\n");
	Com_Printf("--- ----- ---- ------- --------------------- ------ ----- ---------------\n");
	for (i = 0, cl = svs.clients; i < sv_maxClients->integer; i++, cl++){
		if (cl->state == CS_FREE)
			continue;

		Com_Printf("%3i ", i);
		Com_Printf("%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == CS_ZOMBIE)
			Com_Printf("ZMBI ");
		else if (cl->state == CS_CONNECTED)
			Com_Printf("CNCT ");
		else {
			ping = Clamp(cl->ping, 0, 9999);
			Com_Printf("%4i ", ping);
		}

		Com_Printf("%7i ", svs.realTime - cl->lastMessage);

		s = NET_AdrToString(cl->netChan.remoteAddress);
		Com_Printf("%s", s);
		len = 22 - strlen(s);
		for (j = 0; j < len; j++)
			Com_Printf(" ");
		
		Com_Printf("%6i ", cl->netChan.qport);

		Com_Printf("%5i ", cl->rate);

		Com_Printf("%s", cl->name);

		Com_Printf("\n");
	}
	Com_Printf("\n");
}

/*
 =================
 SV_Heartbeat_f
 =================
*/
void SV_Heartbeat_f (void){

	if (!svs.initialized){
		Com_Printf("No server running\n");
		return;
	}

	svs.lastHeartbeat = -9999999;
}

/*
 =================
 SV_ServerInfo_f

 Examine the server info string
 =================
*/
void SV_ServerInfo_f (void){

	Com_Printf("Server info settings:\n");
	Com_Printf("---------------------\n");
	Info_Print(Cvar_ServerInfo());
}

/*
 =================
 SV_DumpUser_f

 Examine a user's info string
 =================
*/
void SV_DumpUser_f (void){

	if (!svs.initialized){
		Com_Printf("No server running\n");
		return;
	}

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: dumpuser <userid>\n");
		return;
	}

	if (!SV_SetPlayer(Cmd_Argv(1)))
		return;

	Com_Printf("User info settings:\n");
	Com_Printf("-------------------\n");
	Info_Print(sv_client->userInfo);
}

/*
 =================
 SV_ServerRecord_f

 Begins server demo recording. Every entity and every message will be
 recorded, but no playerinfo will be stored. Primarily for demo merging.
 =================
*/
void SV_ServerRecord_f (void){

	char	name[MAX_QPATH];
	char	data[32768];
	msg_t	msg;
	int		i, len;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: serverrecord [demoname]\n");
		return;
	}

	if (svs.demoFile){
		Com_Printf("Already recording\n");
		return;
	}

	if (sv.state != SS_GAME){
		Com_Printf("You must be in a level to record\n");
		return;
	}

	if (Cmd_Argc() == 1){
		// Find a file name to record to
		for (i = 0; i <= 9999; i++){
			Q_snprintfz(name, sizeof(name), "demos/demo%04i.dm2", i);
			if (FS_LoadFile(name, NULL) == -1)
				break;	// File doesn't exist
		}

		if (i == 10000){
			Com_Printf("Demos directory is full!\n");
			return;
		}
	}
	else {
		Q_snprintfz(name, sizeof(name), "demos/%s", Cmd_Argv(1));
		Com_DefaultExtension(name, sizeof(name), ".dm2");
	}

	// Open the demo file
	FS_FOpenFile(name, &svs.demoFile, FS_WRITE);
	if (!svs.demoFile){
		Com_Printf("Couldn't open %s\n", name);
		return;
	}

	Com_Printf("Recording to %s\n", name);

	// Setup a buffer to catch all multicasts
	MSG_Init(&svs.demoMulticast, svs.demoMulticastBuffer, sizeof(svs.demoMulticastBuffer), false);

	// Write a single giant fake message with all the startup info
	MSG_Init(&msg, data, sizeof(data), false);

	// Send the server data
	MSG_WriteByte(&msg, SVC_SERVERDATA);
	MSG_WriteLong(&msg, PROTOCOL_VERSION);
	MSG_WriteLong(&msg, svs.spawnCount);
	MSG_WriteByte(&msg, 2);		// Demos are always attract loops (2 means server demo)
	MSG_WriteString(&msg, Cvar_VariableString("fs_game"));
	MSG_WriteShort(&msg, -1);
	MSG_WriteString(&msg, sv.configStrings[CS_NAME]);

	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
		if (sv.configStrings[i][0]){
			MSG_WriteByte(&msg, SVC_CONFIGSTRING);
			MSG_WriteShort(&msg, i);
			MSG_WriteString(&msg, sv.configStrings[i]);
		}

	// Write it to the demo file
	Com_DPrintf("Signon message length: %i\n", msg.curSize);

	len = LittleLong(msg.curSize);
	FS_Write(&len, sizeof(len), svs.demoFile);
	FS_Write(msg.data, msg.curSize, svs.demoFile);

	// The rest of the demo file will be individual frames
}

/*
 =================
 SV_ServerStopRecord_f

 Ends server demo recording
 =================
*/
void SV_ServerStopRecord_f (void){

	if (!svs.demoFile){
		Com_Printf("Not recording a demo\n");
		return;
	}

	FS_FCloseFile(svs.demoFile);
	svs.demoFile = 0;

	Com_Printf("Stopped recording\n");
}

/*
 =================
 SV_KillServer_f

 Kick everyone off, possibly in preparation for a new game
 =================
*/
void SV_KillServer_f (void){

	SV_Shutdown("Server was killed\n", false);
}

/*
 =================
 SV_ServerCommand_f

 Let the game library handle a command
 =================
*/
void SV_ServerCommand_f (void){

	if (!ge){
		Com_Printf("No game loaded\n");
		return;
	}

	ge->ServerCommand();
}

/*
 =================
 SV_ConSay_f
 =================
*/
void SV_ConSay_f (void){

	client_t *cl;
	char	text[1024], *p;
	int		i;

	if (!svs.initialized){
		Com_Printf("No server running\n");
		return;
	}

	if (Cmd_Argc () < 2)
		return;

	Q_strncpyz(text, "Console: ", sizeof(text));

	p = Cmd_Args();
	if (*p == '"'){
		p++;

		if (p[strlen(p)-1] == '"')
			p[strlen(p)-1] = 0;
	}

	Q_strncatz(text, p, sizeof(text));

	for (i = 0, cl = svs.clients; i < sv_maxClients->integer; i++, cl++){
		if (cl->state != CS_SPAWNED)
			continue;
		
		SV_ClientPrintf(cl, PRINT_CHAT, "%s\n", text);
	}
}
