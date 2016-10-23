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


// cl_parse.c  -- parse a message received from the server


#include "client.h"


const char	*svc_strings[256] = {
	"SVC_BAD",
	"SVC_MUZZLEFLASH",
	"SVC_MUZZLEFLASH2",
	"SVC_TEMP_ENTITY",
	"SVC_LAYOUT",
	"SVC_INVENTORY",
	"SVC_NOP",
	"SVC_DISCONNECT",
	"SVC_RECONNECT",
	"SVC_SOUND",
	"SVC_PRINT",
	"SVC_STUFFTEXT",
	"SVC_SERVERDATA",
	"SVC_CONFIGSTRING",
	"SVC_SPAWNBASELINE",	
	"SVC_CENTERPRINT",
	"SVC_DOWNLOAD",
	"SVC_PLAYERINFO",
	"SVC_PACKETENTITIES",
	"SVC_DELTAPACKETENTITIES",
	"SVC_FRAME"
};


/*
 =================
 CL_ParsePrint
 =================
*/
static void CL_ParsePrint (void){

	const char	*s;
	char		*ch, name[64];
	int			i;

	i = MSG_ReadByte(&net_message);
	s = MSG_ReadString(&net_message);

	if (i == PRINT_CHAT){
		S_StartLocalSound(S_RegisterSound("misc/talk.wav"));

		ch = strchr(s, ':');
		if (ch){
			Q_strncpyz(name, s, sizeof(name));
			name[ch-s] = 0;
			Com_Printf("%s:" S_COLOR_GREEN "%s", name, ch+1);
		}
		else
			Com_Printf(S_COLOR_GREEN "%s", s);
	}
	else
		Com_Printf("%s", s);
}

/*
 =================
 CL_ParseCenterPrint

 Called for important messages that should stay in the center of the 
 screen for a few moments
 =================
*/
static void CL_ParseCenterPrint (void){

	const char	*s;

	s = MSG_ReadString(&net_message);

	Q_strncpyz(cl.centerPrint, s, sizeof(cl.centerPrint));
	cl.centerPrintTime = cl.time;
}

/*
 =================
 CL_ParseDownload

 A download message has been received from the server
 =================
*/
static void CL_ParseDownload (void){

	int		size, percent;

	// Read the data
	size = MSG_ReadShort(&net_message);
	percent = MSG_ReadByte(&net_message);

	if (!cls.downloadFile){
		Com_Printf("Unwanted download received. Ignored\n");

		net_message.readCount += size;
		return;
	}

	if (size == -1){
		Com_Printf("Server does not have this file\n");

		// If here, we tried to resume a file but the server said no
		FS_CloseFile(cls.downloadFile);
		cls.downloadFile = 0;

		// Get another file if needed
		CL_RequestNextDownload();
		return;
	}

	cls.downloadBytes += size;
	cls.downloadPercent = percent;

	// Write the data
	FS_Write(net_message.data + net_message.readCount, size, cls.downloadFile);
	net_message.readCount += size;

	if (percent == 100){
		// Finished download
		FS_CloseFile(cls.downloadFile);
		cls.downloadFile = 0;

		// Rename the temp file to its final name
		FS_RenameFile(cls.downloadTempName, cls.downloadName);

		// Get another file if needed
		CL_RequestNextDownload();
		return;
	}

	// Request next block
	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print(&cls.netChan.message, "nextdl");
}

/*
 =================
 CL_ParseServerData
 =================
*/
static void CL_ParseServerData (void){

	char	*str;

	Com_DPrintf("Server data packet received\n");

	// Make sure any stuffed commands are done
	Cbuf_Execute();

	// Clear the client state
	CL_ClearState();

	// Parse protocol version number
	cl.serverProtocol = MSG_ReadLong(&net_message);

	// BIG HACK to let old demos continue to work
	if (Com_ServerState() && PROTOCOL_VERSION == 34)
		;
	else if (cl.serverProtocol != PROTOCOL_VERSION)
		Com_Error(ERR_DROP, "Server returned version %i, not %i", cl.serverProtocol, PROTOCOL_VERSION);

	cl.serverCount = MSG_ReadLong(&net_message);
	cl.demoPlaying = MSG_ReadByte(&net_message);

	// Parse game directory
	str = MSG_ReadString(&net_message);
	if (str[0]){
		if (!Q_stricmp(str, "baseq2") || !Q_stricmp(str, "ctf") || !Q_stricmp(str, "rogue") || !Q_stricmp(str, "xatrix"))
			cl.gameMod = false;
		else
			cl.gameMod = true;

		Q_strncpyz(cl.gameDir, str, sizeof(cl.gameDir));
	}
	else{
		cl.gameMod = false;
		Q_strncpyz(cl.gameDir, "baseq2", sizeof(cl.gameDir));
	}

	// Parse client entity number
	cl.clientNum = MSG_ReadShort(&net_message) + 1;

	// Parse the full level name
	str = MSG_ReadString(&net_message);

	if (!cl.clientNum){
		// Playing a cinematic or showing a pic, not a level
		CL_Restart();

		cls.state = CA_ACTIVE;
		cls.loading = false;

		CL_PlayCinematic(str);
	}
}

/*
 =================
 CL_ParseConfigString
 =================
*/
static void CL_ParseConfigString (void){

	int			index;
	const char	*s;
	char		name[MAX_OSPATH];

	index = MSG_ReadShort(&net_message);
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error(ERR_DROP, "CL_ParseConfigstring: index = %i", index);

	s = MSG_ReadString(&net_message);
#ifdef SECURE
	strcpy_s(cl.configStrings[index], sizeof(cl.configStrings[index]), s);
#else
	strcpy(cl.configStrings[index], s);
#endif

	// Do something appropriate 
	if (index == CS_MAXCLIENTS)
		cl.multiPlayer = (cl.configStrings[index][0] && Q_stricmp(cl.configStrings[index], "1"));
	else if (index >= CS_LIGHTS && index < CS_LIGHTS+MAX_LIGHTSTYLES)
		CL_SetLightStyle(index-CS_LIGHTS);
	else if (index == CS_CDTRACK){
		if (cls.state <= CA_LOADING)
			return;

		CL_PlayBackgroundTrack();
	} else if (index >= CS_MODELS && index < CS_MODELS+MAX_MODELS) {
		if (cls.state <= CA_LOADING)
			return;

		Com_FileExtension(cl.configStrings[index], name, sizeof(name));
		if (!Q_stricmp(name, ".sp2")){
			cl.media.gameModels[index-CS_MODELS] = NULL;
			return;
		}

		cl.media.gameModels[index-CS_MODELS] = R_RegisterModel(cl.configStrings[index]);

		if (cl.configStrings[index][0] == '*')
			cl.media.gameCModels[index-CS_MODELS] = CM_InlineModel(cl.configStrings[index]);
		else
			cl.media.gameCModels[index-CS_MODELS] = NULL;
	}
	else if (index >= CS_SOUNDS && index < CS_SOUNDS+MAX_SOUNDS){
		if (cls.state <= CA_LOADING)
			return;

		cl.media.gameSounds[index-CS_SOUNDS] = S_RegisterSound(cl.configStrings[index]);
	}
	else if (index >= CS_IMAGES && index < CS_IMAGES+MAX_IMAGES){
		if (cls.state <= CA_LOADING)
			return;

		if (!strchr(cl.configStrings[index], '/'))
			Q_snprintfz(name, sizeof(name), "pics/%s", cl.configStrings[index]);
		else
			Com_StripExtension(cl.configStrings[index], name, sizeof(name));

		cl.media.gameMaterials[index-CS_IMAGES] = R_RegisterMaterialNoMip(name);
	}
	else if (index >= CS_PLAYERSKINS && index < CS_PLAYERSKINS+MAX_CLIENTS){
		if (cls.state <= CA_LOADING)
			return;

		CL_LoadClientInfo(&cl.clientInfo[index-CS_PLAYERSKINS], cl.configStrings[index]);
	}
}

/*
 =================
 CL_ParseLayout
 =================
*/
static void CL_ParseLayout (void){

	const char	*s;

	s = MSG_ReadString(&net_message);
	Q_strncpyz(cl.layout, s, sizeof(cl.layout));
}

/*
 =================
 CL_ParseInventory
 =================
*/
static void CL_ParseInventory (void){

	int		i;

	for (i = 0; i < MAX_ITEMS; i++)
		cl.inventory[i] = MSG_ReadShort(&net_message);
}

/*
 =================
 CL_ParseStartSound
 =================
*/
static void CL_ParseStartSound (void){

    vec3_t  originVec;
	float	*origin;
	int		flags, sound;
	int		entNum, entChannel, timeOfs;
    float 	volume, attenuation;

	flags = MSG_ReadByte(&net_message);
	sound = MSG_ReadByte(&net_message);

    if (flags & SND_VOLUME)
		volume = MSG_ReadByte(&net_message) / 255.0;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte(&net_message) / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;	

    if (flags & SND_OFFSET)
		timeOfs = MSG_ReadByte(&net_message);
	else
		timeOfs = 0;

	if (flags & SND_ENT){
		// Entity relative
		entChannel = MSG_ReadShort(&net_message); 
		entNum = entChannel >> 3;
		if (entNum < 0 || entNum >= MAX_EDICTS)
			Com_Error(ERR_DROP, "CL_ParseStartSound: entNum = %i", entNum);

		entChannel &= 7;
	}
	else {
		entNum = 0;
		entChannel = 0;
	}

	if (flags & SND_POS){
		// Positioned in space
		MSG_ReadPos(&net_message, originVec);
 		origin = originVec;
	}
	else	// Use entity number
		origin = NULL;

	if (!cl.media.gameSounds[sound])
		return;

	S_StartSound(origin, entNum, entChannel, cl.media.gameSounds[sound], volume, attenuation, timeOfs);
}       

/*
 =================
 CL_UpdateLagometer

 A new packet was just parsed
 =================
*/
static void CL_UpdateLagometer (void){

	lagometer_t	*lagometer = &cl.lagometer;
	int			current;

	if (cls.state != CA_ACTIVE)
		return;

	current = lagometer->current & (LAG_SAMPLES-1);

	lagometer->samples[current].dropped = cls.netChan.dropped;
	lagometer->samples[current].suppressed = cl.suppressCount;
	lagometer->samples[current].ping = cls.realTime - cl.cmdTime[cls.netChan.incomingAcknowledged & CMD_MASK];

	lagometer->current++;
}

/*
 =================
 CL_ShowNet
 =================
*/
void CL_ShowNet (int level, const char *fmt, ...){

	char	string[1024];
	va_list	argPtr;

	if (cl_showNet->integerValue < level)
		return;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), 1024, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	Com_Printf("%s\n", string);
}

/*
 =================
 CL_ParseServerMessage
 =================
*/
void CL_ParseServerMessage (void){

	int		cmd;
	char	*s;

	CL_ShowNet(2, "------------------\n");
	CL_ShowNet(1, "%i", net_message.curSize);

	// Parse the message
	while (1){
		if (net_message.readCount > net_message.curSize)
			Com_Error(ERR_DROP, "CL_ParseServerMessage: bad server message");

		cmd = MSG_ReadByte(&net_message);
		if (cmd == -1){
			CL_ShowNet(2, "END OF MESSAGE");
			break;
		}

		if (!svc_strings[cmd])
			CL_ShowNet(2, "%3i: BAD CMD %i", net_message.readCount-1, cmd);
		else
			CL_ShowNet(2, "%3i: %s", net_message.readCount-1, svc_strings[cmd]);

		// Other commands
		switch (cmd){
		case SVC_NOP:

			break;
		case SVC_DISCONNECT:
			Com_Error(ERR_DISCONNECT, "Server disconnected");

			break;
		case SVC_RECONNECT:
			// Close download
			if (cls.downloadFile){
				FS_CloseFile(cls.downloadFile);
				cls.downloadFile = 0;
			}

			Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

			// CL_CheckForResend will fire immediately
			cls.state = CA_CONNECTING;
			cls.connectTime = -99999;
			cls.connectCount = 0;

			Com_Printf("Server disconnected, reconnecting...\n");

			break;
		case SVC_STUFFTEXT:
			s = MSG_ReadString(&net_message);
			Cbuf_AddText(s);

			Com_DPrintf("StuffText: %s\n", s);

			break;
		case SVC_PRINT:
			CL_ParsePrint();

			break;
		case SVC_CENTERPRINT:
			CL_ParseCenterPrint();

			break;
		case SVC_DOWNLOAD:
			CL_ParseDownload();

			break;
		case SVC_SERVERDATA:
			CL_ParseServerData();

			break;
		case SVC_CONFIGSTRING:
			CL_ParseConfigString();

			break;
		case SVC_LAYOUT:
			CL_ParseLayout();

			break;
		case SVC_INVENTORY:
			CL_ParseInventory();

			break;
		case SVC_SOUND:
			CL_ParseStartSound();

			break;
		case SVC_MUZZLEFLASH:
			CL_ParsePlayerMuzzleFlash();

			break;
		case SVC_MUZZLEFLASH2:
			CL_ParseMonsterMuzzleFlash();

			break;
		case SVC_TEMP_ENTITY:
			CL_ParseTempEntity();

			break;
		case SVC_SPAWNBASELINE:
			CL_ParseBaseLine();

			break;
		case SVC_FRAME:
			CL_ParseFrame();

			break;
		case SVC_PLAYERINFO:
		case SVC_PACKETENTITIES:
		case SVC_DELTAPACKETENTITIES:
			Com_Error(ERR_DROP, "CL_ParseServerMessage: out of place frame data");

			break;
		default:
			Com_Error(ERR_DROP, "CL_ParseServerMessage: illegible server message");

			break;
		}
	}

	// Update lagometer info
	CL_UpdateLagometer();

	// We don't know if it is ok to save a demo message until after we
	// have parsed the frame
	CL_WriteDemoMessage();
}
