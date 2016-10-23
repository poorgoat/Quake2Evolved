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

	char	**fileList;
	int		numFiles;
	char	name[MAX_OSPATH];
	int		i;

	Com_DPrintf("SV_WipeSaveGame( %s )\n", saveName);

	// Delete the savegame
	Q_snprintfz(name, sizeof(name), "save/%s/server.ssv", saveName);
	FS_RemoveFile(name);

	Q_snprintfz(name, sizeof(name), "save/%s/game.ssv", saveName);
	FS_RemoveFile(name);

	// Find .sav files
	Q_snprintfz(name, sizeof(name), "save/%s", saveName);

	fileList = FS_ListFiles(name, ".sav", false, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Delete .sav
		Q_snprintfz(name, sizeof(name), "save/%s/%s", saveName, fileList[i]);
		FS_RemoveFile(name);
	}

	FS_FreeFileList(fileList);

	// Find .sv2 files
	Q_snprintfz(name, sizeof(name), "save/%s", saveName);

	fileList = FS_ListFiles(name, ".sv2", false, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Delete .sv2
		Q_snprintfz(name, sizeof(name), "save/%s/%s", saveName, fileList[i]);
		FS_RemoveFile(name);
	}

	FS_FreeFileList(fileList);
}

/*
 =================
 SV_CopySaveGame
 =================
*/
static void SV_CopySaveGame (const char *src, const char *dst){

	char	**fileList;
	int		numFiles;
	char	name[MAX_OSPATH], nameSrc[MAX_OSPATH], nameDst[MAX_OSPATH];
	int		i;

	SV_WipeSaveGame(dst);

	Com_DPrintf("SV_CopySaveGame( %s, %s )\n", src, dst);

	// Copy the savegame over
	Q_snprintfz(nameSrc, sizeof(nameSrc), "save/%s/server.ssv", src);
	Q_snprintfz(nameDst, sizeof(nameDst), "save/%s/server.ssv", dst);

	FS_CopyFile(nameSrc, nameDst);

	Q_snprintfz(nameSrc, sizeof(nameSrc), "save/%s/game.ssv", src);
	Q_snprintfz(nameDst, sizeof(nameDst), "save/%s/game.ssv", dst);

	FS_CopyFile(nameSrc, nameDst);

	// Find .sav files
	Q_snprintfz(name, sizeof(name), "save/%s", src);

	fileList = FS_ListFiles(name, ".sav", false, &numFiles);

	for (i = 0; i < numFiles; i++){
		Q_snprintfz(nameSrc, sizeof(nameSrc), "save/%s/%s", src, fileList[i]);
		Q_snprintfz(nameDst, sizeof(nameDst), "save/%s/%s", dst, fileList[i]);

		FS_CopyFile(nameSrc, nameDst);

		// Change .sav to .sv2
		nameSrc[strlen(nameSrc)-3] = 0;
		Q_strncatz(nameSrc, "sv2", sizeof(nameSrc));
		nameDst[strlen(nameDst)-3] = 0;
		Q_strncatz(nameDst, "sv2", sizeof(nameDst));

		FS_CopyFile(nameSrc, nameDst);
	}

	FS_FreeFileList(fileList);
}

/*
 =================
 SV_WriteServerFile
 =================
*/
static void SV_WriteServerFile (qboolean autoSave){

	fileHandle_t	f;
	cvar_t			*cvar;
	char			comment[32];
	char			name[128], value[128];
	time_t			clock;
	struct tm		*ltime;

	Com_DPrintf("SV_WriteServerFile( %s )\n", autoSave ? "true" : "false");

	Q_snprintfz(name, sizeof(name), "save/current/server.ssv");
	FS_OpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}
	
	// Write the comment field
	if (!autoSave){
		time(&clock);
#ifdef SECURE
		ltime = localtime_s(&ltime, &clock);
#else
		ltime = localtime(&clock);
#endif
		Q_snprintfz(comment, sizeof(comment), "%2i:%i%i %2i/%2i  ", ltime->tm_hour, ltime->tm_min/10, ltime->tm_min%10, ltime->tm_mon+1, ltime->tm_mday);
		Q_strncatz(comment, sv.configStrings[CS_NAME], sizeof(comment));
	}
	else
		// Autosaved
		Q_snprintfz(comment, sizeof(comment), "ENTERING %s", sv.configStrings[CS_NAME]);
	
	FS_Write(comment, sizeof(comment), f);

	// Write the map cmd
	FS_Write(svs.mapCmd, sizeof(svs.mapCmd), f);

	// Write all CVAR_LATCH variables
	// These will be things like coop, skill, deathmatch, etc...
	for (cvar = cvar_vars; cvar; cvar = cvar->next){
		if (!(cvar->flags & CVAR_SERVERINFO))
			continue;
		if (!(cvar->flags & CVAR_LATCH))
			continue;

		if (strlen(cvar->name) >= sizeof(name) || strlen(cvar->value) >= sizeof(value)){
			Com_DPrintf("Variable too long: %s = %s\n", cvar->name, cvar->value);
			continue;
		}

		Q_strncpyz(name, cvar->name, sizeof(name));
		Q_strncpyz(value, cvar->value, sizeof(value));
		FS_Write(name, sizeof(name), f);
		FS_Write(value, sizeof(value), f);
	}

	FS_CloseFile(f);

	// Write game state
	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/game.ssv", Cvar_GetString("fs_homePath"), Cvar_GetString("fs_game"));
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
	size = FS_OpenFile(name, &f, FS_READ);
	if (!f){
		Com_Printf("Couldn't read %s\n", name);
		return;
	}

	// Read the comment field
	FS_Read(comment, sizeof(comment), f);

	// Read the map cmd
	FS_Read(mapCmd, sizeof(mapCmd), f);

	// Read all CVAR_LATCH variables
	// These will be things like coop, skill, deathmatch, etc...
	while (FS_Tell(f) < size){
		if (!FS_Read(name, sizeof(name), f))
			break;

		FS_Read(string, sizeof(string), f);
		Cvar_SetString(name, string);
	}

	FS_CloseFile(f);

	// Start a new game fresh with new variables
	SV_InitGame();

	Q_strncpyz(svs.mapCmd, mapCmd, sizeof(svs.mapCmd));

	// Read game state
	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/game.ssv", Cvar_GetString("fs_homePath"), Cvar_GetString("fs_game"));
	ge->ReadGame(name);
}

/*
 =================
 SV_WriteLevelFile
 =================
*/
void SV_WriteLevelFile (void){

	char			name[MAX_OSPATH];
	fileHandle_t	f;

	Com_DPrintf("SV_WriteLevelFile()\n");

	Q_snprintfz(name, sizeof(name), "save/current/%s.sv2", sv.name);
	FS_OpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Failed to open %s\n", name);
		return;
	}

	FS_Write(sv.configStrings, sizeof(sv.configStrings), f);
	CM_WritePortalState(f);
	FS_CloseFile(f);

	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/%s.sav", Cvar_GetString("fs_homePath"), Cvar_GetString("fs_game"), sv.name);
	ge->WriteLevel(name);
}

/*
 =================
 SV_ReadLevelFile
 =================
*/
void SV_ReadLevelFile (void){

	char			name[MAX_OSPATH];
	fileHandle_t	f;

	Com_DPrintf("SV_ReadLevelFile()\n");

	Q_snprintfz(name, sizeof(name), "save/current/%s.sv2", sv.name);
	FS_OpenFile(name, &f, FS_READ);
	if (!f){
		Com_Printf("Failed to open %s\n", name);
		return;
	}

	FS_Read(sv.configStrings, sizeof(sv.configStrings), f);
	CM_ReadPortalState(f);
	FS_CloseFile(f);

	Q_snprintfz(name, sizeof(name), "%s/%s/save/current/%s.sav", Cvar_GetString("fs_homePath"), Cvar_GetString("fs_game"), sv.name);
	ge->ReadLevel(name);
}

/*
 =================
 SV_LoadGame_f
 =================
*/
void SV_LoadGame_f (void){

	char	name[MAX_OSPATH];
	char	*dir;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: loadGame <directory>\n");
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
	if (!FS_FileExists(name)){
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
		Com_Printf("Usage: saveGame <directory>\n");
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

	if (Cvar_GetInteger("deathmatch")){
		Com_Printf("Can't save in a deathmatch game\n");
		return;
	}

	if (sv_maxClients->integerValue == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0){
		Com_Printf("Can't save while dead!\n");
		return;
	}

	if (!Q_stricmp(dir, "current")){
		Com_Printf("Can't save to 'current'\n");
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
			for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
				savedInuse[i] = cl->edict->inuse;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile();

			// We must restore these for clients to transfer over 
			// correctly
			for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++)
				cl->edict->inuse = savedInuse[i];
		}
	}

	// Start up the next map
	SV_Map(map, false, false);

	// Archive server state
	Q_strncpyz(svs.mapCmd, Cmd_Argv(1), sizeof(svs.mapCmd));

	// Copy off the level to the autosave slot
	if (!com_dedicated->integerValue){
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
		Com_Printf("Usage: gameMap <map>\n");
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

	char	map[MAX_OSPATH];
	char	checkName[MAX_OSPATH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: map <mapName>\n");
		return;
	}

	Q_strncpyz(map, Cmd_Argv(1), sizeof(map));

	// Make sure it exists
	Q_snprintfz(checkName, sizeof(checkName), "maps/%s.bsp", map);
	if (!FS_FileExists(checkName)){
		Com_Printf("Can't find %s\n", checkName);
		return;
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

	char	map[MAX_OSPATH];
	char	checkName[MAX_OSPATH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: demo <demoName>\n");
		return;
	}

	Q_strncpyz(map, Cmd_Argv(1), sizeof(map));
	Com_DefaultExtension(map, sizeof(map), ".dm2");

	// Make sure it exists
	Q_snprintfz(checkName, sizeof(checkName), "demos/%s", map);
	if (!FS_FileExists(checkName)){
		Com_Printf("Can't find %s\n", checkName);
		return;
	}

	SV_Map(map, true, false);
}


// =====================================================================


/*
 =================
 SV_GetPlayer

 Returns the player with userId
 =================
*/
static client_t *SV_GetPlayer (const char *userId){

	client_t	*cl;
	char		name[32];
	int			i, idNum;

	// Check for a name match
	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state == CS_FREE)
			continue;

		if (!Q_stricmp(cl->name, userId))
			return cl;

		// Remove color escapes in client name
		Q_strncpyz(name, cl->name, sizeof(name));
		Q_CleanStr(name);

		if (!Q_stricmp(name, userId))
			return cl;
	}

	// If a numeric value, check slots
	for (i = 0; userId[i]; i++){
		if (userId[i] < '0' || userId[i] > '9'){
			Com_Printf("Player '%s' is not on the server\n", userId);
			return NULL;
		}
	}

	idNum = atoi(userId);
	if (idNum < 0 || idNum >= sv_maxClients->integerValue){
		Com_Printf("Bad player slot %i\n", idNum);
		return NULL;
	}

	if (svs.clients[idNum].state == CS_FREE){
		Com_Printf("Player %i is not active\n", idNum);
		return NULL;
	}

	return &svs.clients[idNum];
}

/*
 =================
 SV_Kick_f

 Kick a user off the server
 =================
*/
void SV_Kick_f (void){

	client_t	*cl;

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: kick <userId>\n");
		return;
	}

	cl = SV_GetPlayer(Cmd_Argv(1));
	if (!cl)
		return;

	if (NET_IsLocalAddress(cl->netChan.remoteAddress)){
		Com_Printf("Cannot kick host player\n");
		return;
	}

	SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", cl->name);

	// Print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf(cl, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient(cl);

	cl->lastMessage = svs.realTime;		// In case there is a funny zombie
}

/*
 =================
 SV_Status_f
 =================
*/
void SV_Status_f (void){

	client_t	*cl;
	int			i;

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	Com_Printf("Map: %s\n", sv.name);
	Com_Printf("num score ping lastmsg address               qport  rate  name\n");
	Com_Printf("--- ----- ---- ------- --------------------- ------ ----- ---------------\n");

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state == CS_FREE)
			continue;

		Com_Printf("%3i %5i ", i, cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == CS_ZOMBIE)
			Com_Printf("ZMBI ");
		else if (cl->state == CS_CONNECTED)
			Com_Printf("CNCT ");
		else
			Com_Printf("%4i ", Clamp(cl->ping, 0, 9999));

		Com_Printf("%7i ", svs.realTime - cl->lastMessage);

		Com_Printf("%-21s %6i %5i %s\n", NET_AdrToString(cl->netChan.remoteAddress), cl->netChan.qport, cl->rate, cl->name);
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
		Com_Printf("Server is not running\n");
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

	client_t	*cl;

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: dumpUser <userId>\n");
		return;
	}

	cl = SV_GetPlayer(Cmd_Argv(1));
	if (!cl)
		return;

	Com_Printf("User info settings:\n");
	Com_Printf("-------------------\n");
	Info_Print(cl->userInfo);
}

/*
 =================
 SV_ServerRecord_f

 Begins server demo recording. Every entity and every message will be
 recorded, but no playerinfo will be stored. Primarily for demo merging.
 =================
*/
void SV_ServerRecord_f (void){

	char	name[MAX_OSPATH];
	char	data[32768];
	msg_t	msg;
	int		i, len;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: serverRecord [demoName]\n");
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
			if (!FS_FileExists(name))
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
	FS_OpenFile(name, &svs.demoFile, FS_WRITE);
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
	MSG_WriteString(&msg, Cvar_GetString("fs_game"));
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

	FS_CloseFile(svs.demoFile);
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
		Com_Printf("Server is not running\n");
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

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state != CS_SPAWNED)
			continue;
		
		SV_ClientPrintf(cl, PRINT_CHAT, "%s\n", text);
	}
}
