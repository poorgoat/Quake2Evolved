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


// sv_user.c -- server code for moving users


#include "server.h"


/*
 =================
 SV_New_f

 Sends the first message from the server to a connected client. 
 This will be sent on the initial connection and upon each server load.
 =================
*/
static void SV_New_f (void){

	char		name[MAX_QPATH];
	int			playerNum;
	edict_t		*ent;

	Com_DPrintf("SV_New_f() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED){
		Com_DPrintf("SV_New_f() not valid. Already spawned\n");
		return;
	}

	// Demo servers just dump the file message
	if (sv.state == SS_DEMO){
		Q_snprintfz(name, sizeof(name), "demos/%s", sv.name);
		FS_FOpenFile(name, &sv.demoFile, FS_READ);
		if (!sv.demoFile)
			Com_Error(ERR_DROP, "Couldn't open demo %s", name);

		return;
	}

	// SVC_SERVERDATA needs to go over for all types of servers to make
	// sure the protocol is right, and to set the game dir
	if (sv.state == SS_CINEMATIC || sv.state == SS_PIC)
		playerNum = -1;
	else
		playerNum = sv_client - svs.clients;

	// Send the server data
	MSG_WriteByte(&sv_client->netChan.message, SVC_SERVERDATA);
	MSG_WriteLong(&sv_client->netChan.message, PROTOCOL_VERSION);
	MSG_WriteLong(&sv_client->netChan.message, svs.spawnCount);
	MSG_WriteByte(&sv_client->netChan.message, sv.attractLoop);
	MSG_WriteString(&sv_client->netChan.message, Cvar_VariableString("fs_game"));
	MSG_WriteShort(&sv_client->netChan.message, playerNum);
	MSG_WriteString(&sv_client->netChan.message, sv.configStrings[CS_NAME]);

	// Game server
	if (sv.state == SS_GAME){
		// Set up the entity for the client
		ent = EDICT_NUM(playerNum+1);
		ent->s.number = playerNum+1;
		sv_client->edict = ent;
		memset(&sv_client->lastCmd, 0, sizeof(sv_client->lastCmd));

		// Begin fetching configstrings
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, va("cmd configstrings %i 0\n", svs.spawnCount));
	}
}

/*
 =================
 SV_ConfigStrings_f
 =================
*/
static void SV_ConfigStrings_f (void){

	int		start;

	Com_DPrintf("SV_ConfigStrings_f() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED){
		Com_DPrintf("SV_ConfigStrings_f() not valid. Already spawned\n");
		return;
	}

	// Handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_ConfigStrings_f() from different level\n");
		SV_New_f();
		return;
	}
	
	start = atoi(Cmd_Argv(2));
	if (start < 0)
		start = 0;
	else if (start > MAX_CONFIGSTRINGS)
		start = MAX_CONFIGSTRINGS;

	// Write a packet full of data
	while (sv_client->netChan.message.curSize < MAX_MSGLEN/2 && start < MAX_CONFIGSTRINGS){
		if (sv.configStrings[start][0]){
			MSG_WriteByte(&sv_client->netChan.message, SVC_CONFIGSTRING);
			MSG_WriteShort(&sv_client->netChan.message, start);
			MSG_WriteString(&sv_client->netChan.message, sv.configStrings[start]);
		}
		start++;
	}

	// Send next command
	if (start == MAX_CONFIGSTRINGS){
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, va("cmd baselines %i 0\n", svs.spawnCount));
	}
	else {
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, va("cmd configstrings %i %i\n", svs.spawnCount, start));
	}
}

/*
 =================
 SV_BaseLines_f
 =================
*/
static void SV_BaseLines_f (void){

	int				start;
	entity_state_t	*base, nullState;

	Com_DPrintf("SV_BaseLines_f() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED){
		Com_DPrintf("SV_BaseLines_f() not valid. Already spawned\n");
		return;
	}
	
	// Handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_BaseLines_f() from different level\n");
		SV_New_f();
		return;
	}
	
	start = atoi(Cmd_Argv(2));
	if (start < 0)
		start = 0;
	else if (start > MAX_EDICTS)
		start = MAX_EDICTS;

	memset(&nullState, 0, sizeof(nullState));

	// Write a packet full of data
	while (sv_client->netChan.message.curSize < MAX_MSGLEN/2 && start < MAX_EDICTS){
		base = &sv.baselines[start];
		if (base->modelindex || base->sound || base->effects){
			MSG_WriteByte(&sv_client->netChan.message, SVC_SPAWNBASELINE);
			MSG_WriteDeltaEntity(&sv_client->netChan.message, &nullState, base, true, true);
		}
		start++;
	}

	// Send next command
	if (start == MAX_EDICTS){
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, va("precache %i\n", svs.spawnCount));
	}
	else {
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, va("cmd baselines %i %i\n", svs.spawnCount, start));
	}
}

/*
 =================
 SV_Begin_f
 =================
*/
static void SV_Begin_f (void){

	Com_DPrintf("SV_Begin_f() from %s\n", sv_client->name);

	// Handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_Begin_f() from different level\n");
		SV_New_f();
		return;
	}

	sv_client->state = CS_SPAWNED;

	// Call the game begin function
	ge->ClientBegin(sv_player);

	Cbuf_InsertFromDefer();
}

/*
 =================
 SV_NextServer_f

 A cinematic has completed or been aborted by a client, so move to the 
 next server
 =================
*/
static void SV_NextServer_f (void){

	Com_DPrintf("SV_Nextserver_f() from %s\n", sv_client->name);

	if (atoi(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_Nextserver_f() from different level\n");
		return;		// Leftover from last server
	}

	SV_NextServer();
}

/*
 =================
 SV_Disconnect_f

 The client is going to disconnect, so remove the connection immediately
 =================
*/
static void SV_Disconnect_f (void){

	SV_DropClient(sv_client);	
}

/*
 =================
 SV_NextDownload_f
 =================
*/
static void SV_NextDownload_f (void){

	byte	data[1024];
	int		len, percent;

	if (!sv_client->downloadFile)
		return;

	len = sv_client->downloadSize - sv_client->downloadOffset;
	if (len > sizeof(data))
		len = sizeof(data);

	len = FS_Read(data, len, sv_client->downloadFile);

	sv_client->downloadOffset += len;
	percent = sv_client->downloadOffset * 100/sv_client->downloadSize;

	MSG_WriteByte(&sv_client->netChan.message, SVC_DOWNLOAD);
	MSG_WriteShort(&sv_client->netChan.message, len);
	MSG_WriteByte(&sv_client->netChan.message, percent);
	MSG_Write(&sv_client->netChan.message, data, len);

	if (sv_client->downloadOffset == sv_client->downloadSize){
		FS_FCloseFile(sv_client->downloadFile);
		sv_client->downloadFile = 0;
	}
}

/*
 =================
 SV_Download_f
 =================
*/
static void SV_Download_f (void){

	char	*name;
	int		offset = 0;

	name = Cmd_Argv(1);

	if (Cmd_Argc() > 2)
		offset = atoi(Cmd_Argv(2));	// Downloaded offset

	if (strstr(name, "..") || !strstr(name, "/") || name[0] == '.' || name[0] == '/' || !sv_allowDownload->integer){
		MSG_WriteByte(&sv_client->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort(&sv_client->netChan.message, -1);
		MSG_WriteByte(&sv_client->netChan.message, 0);
		return;
	}

	if (sv_client->downloadFile){
		FS_FCloseFile(sv_client->downloadFile);
		sv_client->downloadFile = 0;
	}

	sv_client->downloadSize = FS_FOpenFile(name, &sv_client->downloadFile, FS_READ);
	
	if (!sv_client->downloadFile || !sv_client->downloadSize){
		Com_DPrintf("Couldn't download %s to %s\n", name, sv_client->name);

		if (sv_client->downloadFile){
			FS_FCloseFile(sv_client->downloadFile);
			sv_client->downloadFile = 0;
		}

		MSG_WriteByte(&sv_client->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort(&sv_client->netChan.message, -1);
		MSG_WriteByte(&sv_client->netChan.message, 0);
		return;
	}

	sv_client->downloadOffset = offset;
	if (sv_client->downloadOffset > sv_client->downloadSize)
		sv_client->downloadOffset = sv_client->downloadSize;

	SV_NextDownload_f();

	Com_DPrintf("Downloading %s to %s...\n", name, sv_client->name);
}

/*
 =================
 SV_Info_f

 Dumps the server info string
 =================
*/
static void SV_Info_f (void){

	Com_Printf("Server info settings:\n");
	Com_Printf("---------------------\n");
	Info_Print(Cvar_ServerInfo());
}


/*
 =======================================================================

 USER CMD EXECUTION

 =======================================================================
*/

typedef struct {
	char	*name;
	void	(*func)(void);
} ucmd_t;

static ucmd_t	sv_userCmds[] = {
	{"new",				SV_New_f},
	{"configstrings",	SV_ConfigStrings_f},
	{"baselines",		SV_BaseLines_f},
	{"begin",			SV_Begin_f},
	{"nextserver",		SV_NextServer_f},
	{"disconnect",		SV_Disconnect_f},
	{"nextdl",			SV_NextDownload_f},
	{"download",		SV_Download_f},
	{"info",			SV_Info_f},
	{NULL,				NULL}
};


/*
 =================
 SV_ExecuteUserCommand
 =================
*/
static void SV_ExecuteUserCommand (char *s){

	ucmd_t	*ucmd;

	Cmd_TokenizeString(s);

	for (ucmd = sv_userCmds; ucmd->name; ucmd++){
		if (!Q_stricmp(Cmd_Argv(0), ucmd->name)){
			ucmd->func();
			break;
		}
	}

	if (!ucmd->name && sv.state == SS_GAME)
		ge->ClientCommand(sv_player);
}

/*
 =================
 SV_ClientThink
 =================
*/
static void SV_ClientThink (client_t *cl, usercmd_t *cmd){

	cl->commandMsec -= cmd->msec;

	if (cl->commandMsec < 0 && sv_enforceTime->integer){
		Com_DPrintf("SV_ClientThink: commandMsec underflow from %s\n", cl->name);
		return;
	}

	ge->ClientThink(cl->edict, cmd);
}

/*
 =================
 SV_ParseClientMessage

 The current net_message is parsed for the given client
 =================
*/
void SV_ParseClientMessage (client_t *cl){

#define	MAX_STRINGCMDS	8

	int			c;
	char		*s;
	usercmd_t	oldestCmd, oldCmd, newCmd, nullCmd;
	int			checksum, calculatedChecksum, checksumIndex;
	qboolean	moveIssued;
	int			stringCmdCount;
	int			dropped, lastFrame;

	sv_client = cl;
	sv_player = sv_client->edict;

	// Only allow one move command
	moveIssued = false;
	stringCmdCount = 0;

	while (1){
		if (net_message.readCount > net_message.curSize){
			Com_Printf(S_COLOR_RED "SV_ParseClientMessage: bad client message\n");
			SV_DropClient(cl);
			return;
		}	

		c = MSG_ReadByte(&net_message);
		if (c == -1)
			break;

		switch (c){
		case CLC_NOP:

			break;
		case CLC_USERINFO:
			s = MSG_ReadString(&net_message);
			Q_strncpyz(cl->userInfo, s, sizeof(cl->userInfo));
			SV_UserInfoChanged(cl);

			break;
		case CLC_MOVE:
			if (moveIssued)
				return;		// Someone is trying to cheat...
			moveIssued = true;

			checksumIndex = net_message.readCount;
			checksum = MSG_ReadByte(&net_message);
			lastFrame = MSG_ReadLong(&net_message);
			if (lastFrame != cl->lastFrame){
				cl->lastFrame = lastFrame;
				if (cl->lastFrame > 0)
					cl->frameLatency[cl->lastFrame & (LATENCY_COUNTS-1)] = svs.realTime - cl->frames[cl->lastFrame & UPDATE_MASK].sentTime;
			}

			memset(&nullCmd, 0, sizeof(nullCmd));
			MSG_ReadDeltaUserCmd(&net_message, &nullCmd, &oldestCmd);
			MSG_ReadDeltaUserCmd(&net_message, &oldestCmd, &oldCmd);
			MSG_ReadDeltaUserCmd(&net_message, &oldCmd, &newCmd);

			if (cl->state != CS_SPAWNED){
				cl->lastFrame = -1;
				break;
			}

			// If the checksum fails, ignore the rest of the packet
			calculatedChecksum = Com_BlockSequenceCRCByte(net_message.data + checksumIndex + 1, net_message.readCount - checksumIndex - 1, cl->netChan.incomingSequence);
			if (calculatedChecksum != checksum){
				Com_DPrintf(S_COLOR_RED "Failed command checksum for %s (%i != %i)\n", cl->name, calculatedChecksum, checksum);
				return;
			}

			if (!paused->integer){
				dropped = cl->netChan.dropped;
				if (dropped < 20){
					while (dropped > 2){
						SV_ClientThink(cl, &cl->lastCmd);
						dropped--;
					}

					if (dropped > 1)
						SV_ClientThink(cl, &oldestCmd);

					if (dropped > 0)
						SV_ClientThink(cl, &oldCmd);

				}

				SV_ClientThink(cl, &newCmd);
			}

			cl->lastCmd = newCmd;

			break;
		case CLC_STRINGCMD:	
			s = MSG_ReadString(&net_message);

			// Malicious users may try using too many string commands
			if (++stringCmdCount < MAX_STRINGCMDS)
				SV_ExecuteUserCommand(s);

			if (cl->state == CS_ZOMBIE)
				return;		// Disconnect command

			break;
		default:
			Com_Printf(S_COLOR_RED "SV_ParseClientMessage: illegible client message\n");
			SV_DropClient(cl);

			return;
		}
	}
}
