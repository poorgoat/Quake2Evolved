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


// cl_main.c  -- client main loop


#include "client.h"


clientState_t	cl;
clientStatic_t	cls;

cvar_t	*cl_hand;
cvar_t	*cl_zoomFov;
cvar_t	*cl_drawGun;
cvar_t	*cl_drawShells;
cvar_t	*cl_footSteps;
cvar_t	*cl_noSkins;
cvar_t	*cl_predict;
cvar_t	*cl_maxFPS;
cvar_t	*cl_freeLook;
cvar_t	*cl_lookSpring;
cvar_t	*cl_lookStrafe;
cvar_t	*cl_upSpeed;
cvar_t	*cl_forwardSpeed;
cvar_t	*cl_sideSpeed;
cvar_t	*cl_yawSpeed;
cvar_t	*cl_pitchSpeed;
cvar_t	*cl_angleSpeedKey;
cvar_t	*cl_run;
cvar_t	*cl_noDelta;
cvar_t	*cl_showNet;
cvar_t	*cl_showMiss;
cvar_t	*cl_showShader;
cvar_t	*cl_timeOut;
cvar_t	*cl_visibleWeapons;
cvar_t	*cl_thirdPerson;
cvar_t	*cl_thirdPersonRange;
cvar_t	*cl_thirdPersonAngle;
cvar_t	*cl_viewBlend;
cvar_t	*cl_particles;
cvar_t	*cl_particleLOD;
cvar_t	*cl_particleBounce;
cvar_t	*cl_particleFriction;
cvar_t	*cl_particleVertexLight;
cvar_t	*cl_markTime;
cvar_t	*cl_brassTime;
cvar_t	*cl_blood;
cvar_t	*cl_testGunX;
cvar_t	*cl_testGunY;
cvar_t	*cl_testGunZ;
cvar_t	*cl_stereoSeparation;
cvar_t	*cl_drawCrosshair;
cvar_t	*cl_crosshairX;
cvar_t	*cl_crosshairY;
cvar_t	*cl_crosshairSize;
cvar_t	*cl_crosshairColor;
cvar_t	*cl_crosshairAlpha;
cvar_t	*cl_crosshairHealth;
cvar_t	*cl_crosshairNames;
cvar_t	*cl_viewSize;
cvar_t	*cl_centerTime;
cvar_t	*cl_drawCenterString;
cvar_t	*cl_drawPause;
cvar_t	*cl_drawFPS;
cvar_t	*cl_drawLagometer;
cvar_t	*cl_drawDisconnected;
cvar_t	*cl_drawRecording;
cvar_t	*cl_draw2D;
cvar_t	*cl_drawIcons;
cvar_t	*cl_drawStatus;
cvar_t	*cl_drawInventory;
cvar_t	*cl_drawLayout;
cvar_t	*cl_newHUD;
cvar_t	*cl_allowDownload;
cvar_t	*cl_rconPassword;
cvar_t	*cl_rconAddress;


/*
 =================
 CL_ForwardToServer

 Adds the current command line as a CLC_STRINGCMD to the client message.
 Things like godmode, noclip, etc..., are commands directed to the
 server, so when they are typed in at the console, they will need to be
 forwarded.
 =================
*/
void CL_ForwardToServer (void){

	char	*cmd;

	cmd = Cmd_Argv(0);
	if (cls.state != CA_ACTIVE || *cmd == '-' || *cmd == '+'){
		Com_Printf("Unknown command \"%s\"\n", cmd);
		return;
	}

	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print(&cls.netChan.message, cmd);

	if (Cmd_Argc() > 1){
		MSG_Print(&cls.netChan.message, " ");
		MSG_Print(&cls.netChan.message, Cmd_Args());
	}
}

/*
 =================
 CL_SendChallengePacket

 Send a connect message to the server
 =================
*/
static void CL_SendChallengePacket (void){

	NetChan_OutOfBandPrint(NS_CLIENT, cls.serverAddress, "getchallenge\n");
}

/*
 =================
 CL_SendConnectPacket

 We have gotten a challenge from the server, so try and connect
 =================
*/
static void CL_SendConnectPacket (void){

	int		port;

	port = Cvar_VariableInteger("net_qport");
	cvar_userInfoModified = false;

	NetChan_OutOfBandPrint(NS_CLIENT, cls.serverAddress, "connect %i %i %i \"%s\"\n", PROTOCOL_VERSION, port, cls.serverChallenge, Cvar_UserInfo());
}

/*
 =================
 CL_CheckForResend
 =================
*/
static void CL_CheckForResend (void){

	if (cls.state >= CA_CONNECTED)
		return;

	if (cls.state == CA_DISCONNECTED){
		// If the local server is running, then connect
		if (Com_ServerState()){
			Q_strncpyz(cls.serverName, "localhost", sizeof(cls.serverName));

			if (!NET_StringToAdr(cls.serverName, &cls.serverAddress)){
				Com_Printf("Bad server address\n");
				return;
			}
			if (cls.serverAddress.port == 0)
				cls.serverAddress.port = BigShort(PORT_SERVER);

			Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

			// We don't need a challenge on the localhost
			cls.state = CA_CHALLENGING;

			CL_Loading();

			CL_SendConnectPacket();
		}

		return;
	}

	CL_Loading();

	// Resend if we haven't gotten a reply yet
	if (cls.realTime - cls.connectTime < 3000)
		return;

	cls.connectTime = cls.realTime;
	cls.connectCount++;

	switch (cls.state){
	case CA_CONNECTING:
		CL_SendChallengePacket();
		break;
	case CA_CHALLENGING:
		CL_SendConnectPacket();
		break;
	}
}

/*
 =================
 CL_ClearState
 =================
*/
void CL_ClearState (void){

	// Clear all local effects
	CL_ClearTempEntities();
	CL_ClearLocalEntities();
	CL_ClearDynamicLights();
	CL_ClearMarks();
	CL_ClearParticles();
	CL_ClearLightStyles();

	// Wipe the entire cl structure
	memset(&cl, 0, sizeof(cl));

	MSG_Clear(&cls.netChan.message);
}

/*
 =================
 CL_ClearMemory

 This clears all the memory used by the client, but does not
 reinitialize anything
 =================
*/
void CL_ClearMemory (void){

	UI_Shutdown();
	CDAudio_Stop();
	S_FreeSounds();
	R_Shutdown(false);

	// If server is not running, clear the hunk
	if (!Com_ServerState())
		Hunk_Clear();

	// Do not attempt to draw
	cls.screenDisabled = true;
}

/*
 =================
 CL_Startup

 This reinitializes the necessary subsystems after CL_ClearMemory
 =================
*/
void CL_Startup (void){

	R_Init(false);
	UI_Init();

	CL_LoadLocalMedia();

	// Set menu visibility
	if (cls.state == CA_DISCONNECTED)
		UI_SetActiveMenu(UI_MAINMENU);
	else
		UI_SetActiveMenu(UI_CLOSEMENU);

	// Ready to draw again
	cls.screenDisabled = false;
}

/*
 =================
 CL_Disconnect

 Sends a disconnect message to the server, and goes to the main menu
 unless shuttingDown is true.
 This is also called on Com_Error, so it shouldn't cause any errors.
 =================
*/
void CL_Disconnect (qboolean shuttingDown){

	byte	final[32];
	int		timeDemoMsec, timeDemoFrames;

	if (cls.state == CA_DISCONNECTED)
		return;

	if (timedemo->integer){
		timeDemoMsec = Sys_Milliseconds() - cl.timeDemoStart;
		timeDemoFrames = cl.timeDemoFrames;
	}

	cls.state = CA_DISCONNECTED;
	cls.connectTime = 0;
	cls.connectCount = 0;
	cls.loading = false;

	// Stop demo recording
	if (cls.demoFile)
		CL_StopRecord_f();

	// Stop download
	if (cls.downloadFile){
		FS_FCloseFile(cls.downloadFile);
		cls.downloadFile = 0;
	}

	// Stop cinematic
	CL_StopCinematic();

	// Send a disconnect message to the server
	final[0] = CLC_STRINGCMD;
	Q_strncpyz((char *)final+1, "disconnect", sizeof(final)-1);
	NetChan_Transmit(&cls.netChan, final, strlen(final));
	NetChan_Transmit(&cls.netChan, final, strlen(final));
	NetChan_Transmit(&cls.netChan, final, strlen(final));

	// Clear the client state
	CL_ClearState();

	// Free current level
	CM_UnloadMap();

	if (shuttingDown)
		return;

	CL_ClearMemory();

	CL_Startup();

	if (timedemo->integer){
		if (timeDemoMsec > 0)
			Com_Printf("%i frames, %3.1f seconds: %3.1f FPS\n", timeDemoFrames, timeDemoMsec / 1000.0, timeDemoFrames * 1000.0 / timeDemoMsec);
	}
}

/*
 =================
 CL_Drop

 Called after an ERR_DROP was thrown
 =================
*/
void CL_Drop (void){

	if (cls.state == CA_UNINITIALIZED)
		return;
	if (cls.state == CA_DISCONNECTED)
		return;

	CL_Disconnect(false);
}

/*
 =================
 CL_Challenge

 Challenge from the server we are connecting to
 =================
*/
static void CL_Challenge (void){

	if (cls.state != CA_CONNECTING){
		if (cls.state == CA_CHALLENGING)
			Com_Printf("Dup challenge received. Ignored\n");
		else
			Com_Printf("Unwanted challenge received. Ignored\n");

		return;
	}

	if (!NET_CompareAdr(net_from, cls.serverAddress)){
		Com_Printf("Challenge from invalid address received. Ignored\n");
		return;
	}

	Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

	cls.serverChallenge = atoi(Cmd_Argv(1));

	// CL_CheckForResend will fire immediately
	cls.state = CA_CHALLENGING;
	cls.connectTime = -99999;
	cls.connectCount = 0;
}

/*
 =================
 CL_ClientConnect

 Server connection
 =================
*/
static void CL_ClientConnect (void){

	if (cls.state != CA_CHALLENGING){
		if (cls.state == CA_CONNECTED)
			Com_Printf("Dup connect received. Ignored\n");
		else
			Com_Printf("Unwanted connect received. Ignored\n");

		return;
	}

	if (!NET_CompareAdr(net_from, cls.serverAddress)){
		Com_Printf("Connect from invalid address received. Ignored\n");
		return;
	}

	cls.state = CA_CONNECTED;

	NetChan_Setup(NS_CLIENT, &cls.netChan, net_from, Cvar_VariableInteger("net_qport"));

	MSG_WriteChar(&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString(&cls.netChan.message, "new");
}

/*
 =================
 CL_Command

 Remote command from GUI front end
 =================
*/
static void CL_Command (void){

	char	*s;

	if (!NET_IsLocalAddress(net_from)){
		Com_Printf("Command packet from remote host. Ignored\n");
		return;
	}

	s = MSG_ReadString(&net_message);
	Cbuf_AddText(s);
	Cbuf_AddText("\n");
}

/*
 =================
 CL_Info

 Server responding to a status broadcast
 =================
*/
static void CL_Info (void){

	char	*s;

	s = MSG_ReadString(&net_message);
	UI_AddServerToList(net_from, s);

	Com_Printf("%s\n", s);
}

/*
 =================
 CL_Print

 Print command from somewhere
 =================
*/
static void CL_Print (void){

	char	*s;

	s = MSG_ReadString(&net_message);
	Q_strncpyz(cls.serverMessage, s, sizeof(cls.serverMessage));

	Com_Printf("%s", s);
}

/*
 =================
 CL_Ping

 Ping from somewhere
 =================
*/
static void CL_Ping (void){

	NetChan_OutOfBandPrint(NS_CLIENT, net_from, "ack");
}

/*
 =================
 CL_Echo

 Echo request from server
 =================
*/
static void CL_Echo (void){

	NetChan_OutOfBandPrint(NS_CLIENT, net_from, "%s", Cmd_Argv(1));
}

/*
 =================
 CL_ConnectionlessPacket
 =================
*/
static void CL_ConnectionlessPacket (void){

	char	*s, *c;

	MSG_BeginReading(&net_message);
	MSG_ReadLong(&net_message);		// Skip the -1 marker

	s = MSG_ReadStringLine(&net_message);
	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);
	Com_DPrintf("CL packet %s: %s\n", NET_AdrToString(net_from), c);

	if (!Q_stricmp(c, "challenge"))
		CL_Challenge();
	else if (!Q_stricmp(c, "client_connect"))
		CL_ClientConnect();
	else if (!Q_stricmp(c, "cmd"))
		CL_Command();
	else if (!Q_stricmp(c, "info"))
		CL_Info();
	else if (!Q_stricmp(c, "print"))
		CL_Print();
	else if (!Q_stricmp(c, "ping"))
		CL_Ping();
	else if (!Q_stricmp(c, "echo"))
		CL_Echo();
	else
		Com_Printf(S_COLOR_YELLOW "Bad connectionless packet from %s:\n%s\n", NET_AdrToString(net_from), s);
}

/*
 =================
 CL_ReadPackets
 =================
*/
static void CL_ReadPackets (void){

	while (NET_GetPacket(NS_CLIENT, &net_from, &net_message)){
		// Check for connectionless packet first
		if (*(int *)net_message.data == -1){
			CL_ConnectionlessPacket();
			continue;
		}

		// Dump it if not connected
		if (cls.state < CA_CONNECTED)
			continue;

		if (net_message.curSize < 8){
			Com_DPrintf(S_COLOR_YELLOW "%s: runt packet\n", NET_AdrToString(net_from));
			continue;
		}

		// Packet from server
		if (!NET_CompareAdr(net_from, cls.netChan.remoteAddress)){
			Com_DPrintf(S_COLOR_YELLOW "%s: sequenced packet without connection\n", NET_AdrToString(net_from));
			continue;
		}

		if (!NetChan_Process(&cls.netChan, &net_message))
			continue;		// Wasn't accepted for some reason

		CL_ParseServerMessage();
	}

	// Check time-out
	if (cls.state >= CA_CONNECTED && cls.realTime - cls.netChan.lastReceived > cl_timeOut->value*1000){
		if (++cl.timeOutCount > 5){		// timeOutCount saves debugger
			Com_Printf("Server connection timed out\n");
			CL_Disconnect(false);
			return;
		}
	}
	else
		cl.timeOutCount = 0;
}

/*
 =================
 CL_FixCheatVars
 =================
*/
void CL_FixCheatVars (void){

	// Allow cheats if disconnected
	if (cls.state == CA_DISCONNECTED){
		Cvar_FixCheatVars(true);
		return;
	}

	// Allow cheats if playing a cinematic, demo or singleplayer game
	if (cls.cinematicHandle || cl.demoPlayback || !cl.multiPlayer){
		Cvar_FixCheatVars(true);
		return;
	}

	// Otherwise don't allow cheats at all
	Cvar_FixCheatVars(false);
}

/*
 =================
 CL_PlayBackgroundTrack
 =================
*/
void CL_PlayBackgroundTrack (void){

	char	name[MAX_QPATH];
	int		track;

	track = atoi(cl.configStrings[CS_CDTRACK]);

	if (track == 0){
		// Stop any playing track
		CDAudio_Stop();
		S_StopBackgroundTrack();
		return;
	}

	// If an OGG file exists play it, otherwise fall back to CD audio
	Q_snprintfz(name, sizeof(name), "music/track%02i.ogg", track);
	if (FS_LoadFile(name, NULL) != -1)
		S_StartBackgroundTrack(name, name);
	else
		CDAudio_Play(track, true);
}


// =====================================================================

#define PLAYER_MULT		5

#define ENV_CNT			(CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEX_CNT			(ENV_CNT + 6)

static const char	*cl_skySuffix[6] = {"rt", "lf", "bk", "ft", "up", "dn"};

static int			cl_precacheCheck;
static int			cl_precacheSpawnCount;
static byte			*cl_precacheModel;
static int			cl_precacheModelSkin;
static byte			*cl_precacheMap;
static int			cl_precacheTexture;


/*
 =================
 CL_CheckOrDownloadFile

 Returns true if the file exists, otherwise it attempts to start a
 download from the server
 =================
*/
static qboolean CL_CheckOrDownloadFile (const char *name){

	int		len;

	if (FS_LoadFile(name, NULL) != -1)
		return true;	// It exists, no need to download

	if (strstr(name, "..") || !strstr(name, "/") || name[0] == '.' || name[0] == '/'){
		Com_Printf("Refusing to download %s\n", name);
		return true;
	}

	Q_strncpyz(cls.downloadName, name, sizeof(cls.downloadName));

	// Download to a temp name, and only rename to the real name when 
	// done, so if interrupted a runt file wont be left
	Com_StripExtension(cls.downloadName, cls.downloadTempName, sizeof(cls.downloadTempName));
	Com_DefaultExtension(cls.downloadTempName, sizeof(cls.downloadTempName), ".tmp");

	len = FS_FOpenFile(cls.downloadTempName, &cls.downloadFile, FS_APPEND);
	if (!cls.downloadFile){
		Com_Printf("Failed to create %s\n", cls.downloadTempName);
		return true;
	}

	cls.downloadStart = cls.realTime;
	cls.downloadBytes = 0;
	cls.downloadPercent = 0;

	if (len){
		FS_Seek(cls.downloadFile, 0, FS_SEEK_END);

		Com_Printf("Resuming %s...\n", cls.downloadName);
		MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString(&cls.netChan.message, va("download %s %i", cls.downloadName, len));
	} 
	else {
		Com_Printf("Downloading %s...\n", cls.downloadName);
		MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString(&cls.netChan.message, va("download %s", cls.downloadName));
	}

	return false;
}

/*
 =================
 CL_RequestNextDownload

 TODO: extend to support TGA/JPG textures, MD3 models, etc... (PAK/PK2?)
 =================
*/
void CL_RequestNextDownload (void){

	char		name[MAX_QPATH], *p;
	char		model[MAX_QPATH], skin[MAX_QPATH];
	md2Header_t	*md2;
	dheader_t	*header;
	dtexinfo_t	*texInfo;
	int			numTexInfo;
	int			i, n;

	if (cls.state != CA_CONNECTED)
		return;

	if (!cl_allowDownload->integer)
		cl_precacheCheck = TEX_CNT + 1;

	// Check map
	if (cl_precacheCheck == CS_MODELS){
		cl_precacheCheck = CS_MODELS + 2;	// 0 isn't used

		if (!CL_CheckOrDownloadFile(cl.configStrings[CS_MODELS+1]))
			return;		// Started a download
	}

	// Check models
	if (cl_precacheCheck >= CS_MODELS && cl_precacheCheck < CS_MODELS+MAX_MODELS){
		while (cl_precacheCheck < CS_MODELS+MAX_MODELS && cl.configStrings[cl_precacheCheck][0]){
			if (cl.configStrings[cl_precacheCheck][0] == '*' || cl.configStrings[cl_precacheCheck][0] == '#'){
				cl_precacheCheck++;
				continue;
			}

			if (cl_precacheModelSkin == -1){
				if (!CL_CheckOrDownloadFile(cl.configStrings[cl_precacheCheck])){
					cl_precacheModelSkin = 0;
					return;		// Started a download
				}

				cl_precacheModelSkin = 0;
			}

			// Check skins in the model
			if (!cl_precacheModel){
				FS_LoadFile(cl.configStrings[cl_precacheCheck], (void **)&cl_precacheModel);
				if (!cl_precacheModel){
					// Couldn't load it
					cl_precacheModelSkin = -1;

					cl_precacheCheck++;
					continue;
				}

				md2 = (md2Header_t *)cl_precacheModel;

				if (LittleLong(md2->ident) != MD2_IDENT || LittleLong(md2->version) != MD2_VERSION){
					// Not an alias model or wrong version
					FS_FreeFile(cl_precacheModel);
					cl_precacheModel = NULL;
					cl_precacheModelSkin = -1;

					cl_precacheCheck++;
					continue;
				}
			}

			md2 = (md2Header_t *)cl_precacheModel;

			while (cl_precacheModelSkin < LittleLong(md2->numSkins)){
				Q_strncpyz(name, (char *)cl_precacheModel + LittleLong(md2->ofsSkins) + cl_precacheModelSkin * MAX_QPATH, sizeof(name));
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheModelSkin++;
					return;		// Started a download
				}

				cl_precacheModelSkin++;
			}

			FS_FreeFile(cl_precacheModel);
			cl_precacheModel = NULL;
			cl_precacheModelSkin = -1;

			cl_precacheCheck++;
		}

		cl_precacheCheck = CS_SOUNDS;
	}

	// Check sounds
	if (cl_precacheCheck >= CS_SOUNDS && cl_precacheCheck < CS_SOUNDS+MAX_SOUNDS){
		if (cl_precacheCheck == CS_SOUNDS)
			cl_precacheCheck++;		// 0 isn't used

		while (cl_precacheCheck < CS_SOUNDS+MAX_SOUNDS && cl.configStrings[cl_precacheCheck][0]){
			if (cl.configStrings[cl_precacheCheck][0] == '*'){
				cl_precacheCheck++;
				continue;
			}

			Q_snprintfz(name, sizeof(name), "sound/%s", cl.configStrings[cl_precacheCheck++]);
			if (!CL_CheckOrDownloadFile(name))
				return;		// Started a download
		}

		cl_precacheCheck = CS_IMAGES;
	}

	// Check images
	if (cl_precacheCheck >= CS_IMAGES && cl_precacheCheck < CS_IMAGES+MAX_IMAGES){
		if (cl_precacheCheck == CS_IMAGES)
			cl_precacheCheck++;		// 0 isn't used

		while (cl_precacheCheck < CS_IMAGES+MAX_IMAGES && cl.configStrings[cl_precacheCheck][0]){
			Q_snprintfz(name, sizeof(name), "pics/%s.pcx", cl.configStrings[cl_precacheCheck++]);
			if (!CL_CheckOrDownloadFile(name))
				return;		// Started a download
		}

		cl_precacheCheck = CS_PLAYERSKINS;
	}

	// Check player skins
	if (cl_precacheCheck >= CS_PLAYERSKINS && cl_precacheCheck < CS_PLAYERSKINS+MAX_CLIENTS*PLAYER_MULT){
		while (cl_precacheCheck < CS_PLAYERSKINS+MAX_CLIENTS*PLAYER_MULT){
			i = (cl_precacheCheck - CS_PLAYERSKINS) / PLAYER_MULT;
			n = (cl_precacheCheck - CS_PLAYERSKINS) % PLAYER_MULT;

			if (cl.configStrings[CS_PLAYERSKINS+i][0] == 0){
				cl_precacheCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				continue;
			}

			p = strchr(cl.configStrings[CS_PLAYERSKINS+i], '\\');
			if (p)
				p++;
			else
				p = cl.configStrings[CS_PLAYERSKINS+i];

			Q_strncpyz(model, p, sizeof(model));
			p = strchr(model, '/');
			if (!p)
				p = strchr(model, '\\');
			if (p){
				*p++ = 0;
				Q_strncpyz(skin, p, sizeof(skin));
			}
			else
				skin[0] = 0;

			switch (n){
			case 0:		// Model
				Q_snprintfz(name, sizeof(name), "players/%s/tris.md2", model);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 1:		// Weapon model
				Q_snprintfz(name, sizeof(name), "players/%s/weapon.md2", model);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 2:		// Weapon skin
				Q_snprintfz(name, sizeof(name), "players/%s/weapon.pcx", model);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 3:		// Skin
				Q_snprintfz(name, sizeof(name), "players/%s/%s.pcx", model, skin);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 4:		// Icon
				Q_snprintfz(name, sizeof(name), "players/%s/%s_i.pcx", model, skin);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
					return;		// Started a download
				}

				// Move on to next model
				cl_precacheCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
			}
		}

		cl_precacheCheck = ENV_CNT;
	}

	// Check skies
	if (cl_precacheCheck >= ENV_CNT && cl_precacheCheck < TEX_CNT){
		while (cl_precacheCheck < TEX_CNT){
			n = cl_precacheCheck++ - ENV_CNT;
			Q_snprintfz(name, sizeof(name), "env/%s%s.tga", cl.configStrings[CS_SKY], cl_skySuffix[n]);
			if (!CL_CheckOrDownloadFile(name))
				return;		// Started a download
		}

		cl_precacheCheck = TEX_CNT;
	}

	// Check textures
	if (cl_precacheCheck == TEX_CNT){
		while (cl_precacheCheck == TEX_CNT){
			if (!cl_precacheMap){
				FS_LoadFile(cl.configStrings[CS_MODELS+1], (void **)&cl_precacheMap);
				if (!cl_precacheMap){
					// Couldn't load it
					cl_precacheCheck++;
					continue;
				}

				header = (dheader_t *)cl_precacheMap;

				if (LittleLong(header->ident) != BSP_IDENT || LittleLong(header->version) != BSP_VERSION){
					// Not a map or wrong version
					FS_FreeFile(cl_precacheMap);
					cl_precacheMap = NULL;
					cl_precacheTexture = 0;

					cl_precacheCheck++;
					continue;
				}
			}

			header = (dheader_t *)cl_precacheMap;

			texInfo = (dtexinfo_t *)(cl_precacheMap + LittleLong(header->lumps[LUMP_TEXINFO].fileOfs));
			numTexInfo = LittleLong(header->lumps[LUMP_TEXINFO].fileLen) / sizeof(dtexinfo_t);

			while (cl_precacheTexture < numTexInfo){
				Q_snprintfz(name, sizeof(name), "textures/%s.wal", texInfo[cl_precacheTexture++].texture);
				if (!CL_CheckOrDownloadFile(name))
					return;		// Started a download
			}

			FS_FreeFile(cl_precacheMap);
			cl_precacheMap = NULL;
			cl_precacheTexture = 0;

			cl_precacheCheck++;
		}
	}

	// Load level
	CL_LoadGameMedia();

	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString(&cls.netChan.message, va("begin %i\n", cl_precacheSpawnCount));
}


// =====================================================================


/*
 =================
 CL_Precache_f

 The server will send this command right before allowing the client into
 the server
 =================
*/
void CL_Precache_f (void){

	if (cls.state != CA_CONNECTED)
		return;

	// Yet another hack to let old demos work with the old precache 
	// sequence
	if (Cmd_Argc() < 2){
		CL_LoadGameMedia();
		return;
	}

	cl_precacheCheck = CS_MODELS;
	cl_precacheSpawnCount = atoi(Cmd_Argv(1));
	cl_precacheModel = NULL;
	cl_precacheModelSkin = -1;
	cl_precacheTexture = 0;

	CL_RequestNextDownload();
}

/*
 =================
 CL_Changing_f

 The server will send this command as a hint that the client should draw
 the loading screen
 =================
*/
void CL_Changing_f (void){

	Com_Printf("Changing map...\n");

	// Not active anymore, but not disconnected
	cls.state = CA_CONNECTED;

	CL_Loading();
}

/*
 =================
 CL_ForwardToServer_f
 =================
*/
void CL_ForwardToServer_f (void){

	if (cls.state < CA_CONNECTED){
		Com_Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	// Don't forward the first argument
	if (Cmd_Argc() > 1){
		MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
		MSG_Print(&cls.netChan.message, Cmd_Args());
	}
}

/*
 =================
 CL_Connect_f
 =================
*/
void CL_Connect_f (void){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: connect <server>\n");
		return;	
	}

	// If running a local server, kill it and reissue
	SV_Shutdown("Server quit\n", false);

	// Disconnect from server
	CL_Disconnect(false);

	// Resolve server address
	Q_strncpyz(cls.serverName, Cmd_Argv(1), sizeof(cls.serverName));

	if (!NET_StringToAdr(cls.serverName, &cls.serverAddress)){
		Com_Printf("Bad server address\n");
		return;
	}
	if (cls.serverAddress.port == 0)
		cls.serverAddress.port = BigShort(PORT_SERVER);

	if (NET_IsLocalAddress(cls.serverAddress)){
		Com_Printf("Can't connect to localhost\n");
		return;
	}

	Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

	// CL_CheckForResend will fire immediately
	cls.state = CA_CONNECTING;
	cls.connectTime = -99999;
	cls.connectCount = 0;

	Com_Printf("Connecting to %s...\n", cls.serverName);
}

/*
 =================
 CL_Reconnect_f
 =================
*/
void CL_Reconnect_f (void){

	// If connected, the server is changing levels
	if (cls.state == CA_CONNECTED){
		Com_Printf("Reconnecting...\n");

		// Close download
		if (cls.downloadFile){
			FS_FCloseFile(cls.downloadFile);
			cls.downloadFile = 0;
		}

		MSG_WriteChar(&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString(&cls.netChan.message, "new");		
		return;
	}

	// If running a local server, kill it and reissue
	SV_Shutdown("Server quit\n", false);

	// Disconnect from server
	CL_Disconnect(false);

	// Resolve server address
	if (!cls.serverName[0]){
		Com_Printf("No server to reconnect\n");
		return;
	}

	if (!NET_StringToAdr(cls.serverName, &cls.serverAddress)){
		Com_Printf("Bad server address\n");
		return;
	}
	if (cls.serverAddress.port == 0)
		cls.serverAddress.port = BigShort(PORT_SERVER);

	if (NET_IsLocalAddress(cls.serverAddress)){
		Com_Printf("Can't reconnect to localhost\n");
		return;
	}

	Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

	// CL_CheckForResend will fire immediately
	cls.state = CA_CONNECTING;
	cls.connectTime = -99999;
	cls.connectCount = 0;

	Com_Printf("Reconnecting to %s...\n", cls.serverName);
}

/*
 =================
 CL_Disconnect_f
 =================
*/
void CL_Disconnect_f (void){

	if (cls.state == CA_DISCONNECTED){
		Com_Printf("Not connected to a server\n");
		return;
	}

	Com_Error(ERR_DROP, "Disconnected from server");
}

/*
 =================
 CL_LocalServers_f
 =================
*/
void CL_LocalServers_f (void){

	netAdr_t	adr;

	Com_Printf("Scanning for servers on the local network...\n");

	// Send a broadcast packet
	adr.type = NA_BROADCAST;
	adr.port = BigShort(PORT_SERVER);

	NetChan_OutOfBandPrint(NS_CLIENT, adr, "info %i", PROTOCOL_VERSION);
}

/*
 =================
 CL_PingServers_f
 =================
*/
void CL_PingServers_f (void){

	int			i;
	netAdr_t	adr;
	char		*adrString;

	Com_Printf("Pinging favorites...\n");

	// Send a packet to each address book entry
	for (i = 0; i < 16; i++){
		adrString = Cvar_VariableString(va("server%i", i+1));
		if (!adrString[0])
			continue;

		Com_Printf("Pinging %s...\n", adrString);

		if (!NET_StringToAdr(adrString, &adr)){
			Com_Printf("Bad server address\n");
			continue;
		}
		if (adr.port == 0)
			adr.port = BigShort(PORT_SERVER);

		NetChan_OutOfBandPrint(NS_CLIENT, adr, "info %i", PROTOCOL_VERSION);
	}
}

/*
 =================
 CL_Ping_f
 =================
*/
void CL_Ping_f (void){

	netAdr_t	adr;
	char		adrString[64];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: ping <server>\n");
		return;
	}

	// Send a packet to the specified server
	Q_strncpyz(adrString, Cmd_Argv(1), sizeof(adrString));
	Com_Printf("Pinging %s...\n", adrString);

	if (!NET_StringToAdr(adrString, &adr)){
		Com_Printf("Bad server address\n");
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort(PORT_SERVER);

	NetChan_OutOfBandPrint(NS_CLIENT, adr, "info %i", PROTOCOL_VERSION);
}

/*
 =================
 CL_Rcon_f

 Send the rest of the command line over as an unconnected command
 =================
*/
void CL_Rcon_f (void){

	netAdr_t	to;
	char		message[1024];
	int			i;

	if (cls.state >= CA_CONNECTED)
		to = cls.netChan.remoteAddress;
	else {
		if (!cl_rconAddress->string[0]){
			Com_Printf("You must either be connected, or set the \"rconAddress\" cvar to issue rcon commands\n");
			return;
		}

		if (!NET_StringToAdr(cl_rconAddress->string, &to)){
			Com_Printf("Bad remote server address\n");
			return;
		}
		if (to.port == 0)
			to.port = BigShort(PORT_SERVER);
	}

	if (!cl_rconPassword->string[0]){
		Com_Printf("You must set the \"rconPassword\" cvar to issue rcon commands\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	Q_strncatz(message, "rcon ", sizeof(message));
	Q_strncatz(message, cl_rconPassword->string, sizeof(message));
	Q_strncatz(message, " ", sizeof(message));

	for (i = 1; i < Cmd_Argc(); i++){
		Q_strncatz(message, Cmd_Argv(i),sizeof(message));
		Q_strncatz(message, " ", sizeof(message));
	}

	NET_SendPacket(NS_CLIENT, to, message, strlen(message)+1);
}

/*
 =================
 CL_UserInfo_f
 =================
*/
void CL_UserInfo_f (void){

	Com_Printf("User info settings:\n");
	Com_Printf("-------------------\n");
	Info_Print(Cvar_UserInfo());
}

/*
 =================
 CL_Skins_f

 Load or download any custom player skins and models
 =================
*/
void CL_Skins_f (void){

	int		i;

	if (cls.state < CA_CONNECTED){
		Com_Printf("Not connected to a server\n");
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++){
		if (!cl.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		Com_Printf("Client %i: %s\n", i, cl.configStrings[CS_PLAYERSKINS+i]);

		CL_LoadClientInfo(&cl.clientInfo[i], cl.configStrings[CS_PLAYERSKINS+i]);
	}
}

/*
 =================
 CL_ConfigStrings_f
 =================
*/
void CL_ConfigStrings_f (void){

	int		i;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: configstrings [index]\n");
		return;
	}

	if (cls.state < CA_CONNECTED){
		Com_Printf("Not connected to a server\n");
		return;
	}

	if (Cmd_Argc() == 1){
		for (i = 0; i < MAX_CONFIGSTRINGS; i++){
			if (!cl.configStrings[i][0])
				continue;

			Com_Printf("%4i: %s\n", i, cl.configStrings[i]);
		}

		return;
	}

	i = atoi(Cmd_Argv(1));
	if (i < 0 || i >= MAX_CONFIGSTRINGS){
		Com_Printf("Bad configstring index %i\n", i);
		return;
	}

	Com_Printf("%4i: %s\n", i, cl.configStrings[i]);
}

/*
 =================
 CL_Cinematic_f
 =================
*/
void CL_Cinematic_f (void){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: cinematic <videofile>\n");
		return;
	}

	// If running a local server, kill it and reissue
	SV_Shutdown("Server quit\n", false);

	// Disconnect from server
	CL_Disconnect(false);

	CL_PlayCinematic(Cmd_Argv(1));
}


// =====================================================================


/*
 =================
 CL_InitLocal
 =================
*/
static void CL_InitLocal (void){

	cls.state = CA_DISCONNECTED;
	cls.realTime = Sys_Milliseconds();

	// Register our cvars and commands
	Cvar_Get("server1", "", CVAR_ARCHIVE);
	Cvar_Get("server2", "", CVAR_ARCHIVE);
	Cvar_Get("server3", "", CVAR_ARCHIVE);
	Cvar_Get("server4", "", CVAR_ARCHIVE);
	Cvar_Get("server5", "", CVAR_ARCHIVE);
	Cvar_Get("server6", "", CVAR_ARCHIVE);
	Cvar_Get("server7", "", CVAR_ARCHIVE);
	Cvar_Get("server8", "", CVAR_ARCHIVE);
	Cvar_Get("server9", "", CVAR_ARCHIVE);
	Cvar_Get("server10", "", CVAR_ARCHIVE);
	Cvar_Get("server11", "", CVAR_ARCHIVE);
	Cvar_Get("server12", "", CVAR_ARCHIVE);
	Cvar_Get("server13", "", CVAR_ARCHIVE);
	Cvar_Get("server14", "", CVAR_ARCHIVE);
	Cvar_Get("server15", "", CVAR_ARCHIVE);
	Cvar_Get("server16", "", CVAR_ARCHIVE);
	Cvar_Get("name", "Player", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("gender", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("msg", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("spectator", "0", CVAR_USERINFO);
	Cvar_Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("fov", "90", CVAR_USERINFO | CVAR_ARCHIVE);
	cl_hand = Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	cl_zoomFov = Cvar_Get("cl_zoomFov", "25", CVAR_ARCHIVE);
	cl_drawGun = Cvar_Get("cl_drawGun", "1", CVAR_ARCHIVE);
	cl_drawShells = Cvar_Get("cl_drawShells", "1", CVAR_ARCHIVE);
	cl_footSteps = Cvar_Get("cl_footSteps", "1", CVAR_CHEAT);
	cl_noSkins = Cvar_Get("cl_noSkins", "0", 0);
	cl_predict = Cvar_Get("cl_predict", "1", CVAR_ARCHIVE);
	cl_maxFPS = Cvar_Get("cl_maxFPS", "90", CVAR_ARCHIVE);
	cl_freeLook = Cvar_Get("cl_freeLook", "1", CVAR_ARCHIVE);
	cl_lookSpring = Cvar_Get("cl_lookSpring", "0", CVAR_ARCHIVE);
	cl_lookStrafe = Cvar_Get("cl_lookStrafe", "0", CVAR_ARCHIVE);
	cl_upSpeed = Cvar_Get("cl_upSpeed", "200", CVAR_ARCHIVE);
	cl_forwardSpeed = Cvar_Get("cl_forwardSpeed", "200", CVAR_ARCHIVE);
	cl_sideSpeed = Cvar_Get("cl_sideSpeed", "200", CVAR_ARCHIVE);
	cl_yawSpeed = Cvar_Get("cl_yawSpeed", "140", CVAR_ARCHIVE);
	cl_pitchSpeed = Cvar_Get("cl_pitchSpeed", "150", CVAR_ARCHIVE);
	cl_angleSpeedKey = Cvar_Get("cl_angleSpeedKey", "1.5", CVAR_ARCHIVE);
	cl_run = Cvar_Get("cl_run", "0", CVAR_ARCHIVE);
	cl_noDelta = Cvar_Get("cl_noDelta", "0", 0);
	cl_showNet = Cvar_Get("cl_showNet", "0", 0);
	cl_showMiss = Cvar_Get("cl_showMiss", "0", 0);
	cl_showShader = Cvar_Get("cl_showShader", "0", CVAR_CHEAT);
	cl_timeOut = Cvar_Get("cl_timeOut", "120", 0);
	cl_visibleWeapons = Cvar_Get("cl_visibleWeapons", "1", CVAR_ARCHIVE);
	cl_thirdPerson = Cvar_Get("cl_thirdPerson", "0", CVAR_CHEAT);
	cl_thirdPersonRange = Cvar_Get("cl_thirdPersonRange", "40", CVAR_CHEAT);
	cl_thirdPersonAngle = Cvar_Get("cl_thirdPersonAngle", "0", CVAR_CHEAT);
	cl_viewBlend = Cvar_Get("cl_viewBlend", "1", CVAR_ARCHIVE);
	cl_particles = Cvar_Get("cl_particles", "1", CVAR_ARCHIVE);
	cl_particleLOD = Cvar_Get("cl_particleLOD", "0", CVAR_ARCHIVE);
	cl_particleBounce = Cvar_Get("cl_particleBounce", "1", CVAR_ARCHIVE);
	cl_particleFriction = Cvar_Get("cl_particleFriction", "1", CVAR_ARCHIVE);
	cl_particleVertexLight = Cvar_Get("cl_particleVertexLight", "1", CVAR_ARCHIVE);
	cl_markTime = Cvar_Get("cl_markTime", "15000", CVAR_ARCHIVE);
	cl_brassTime = Cvar_Get("cl_brassTime", "2500", CVAR_ARCHIVE);
	cl_blood = Cvar_Get("cl_blood", "1", CVAR_ARCHIVE);
	cl_testGunX = Cvar_Get("cl_testGunX", "0", CVAR_CHEAT);
	cl_testGunY = Cvar_Get("cl_testGunY", "0", CVAR_CHEAT);
	cl_testGunZ = Cvar_Get("cl_testGunZ", "0", CVAR_CHEAT);
	cl_stereoSeparation = Cvar_Get("cl_stereoSeparation", "0.4", CVAR_ARCHIVE);
	cl_drawCrosshair = Cvar_Get("cl_drawCrosshair", "0", CVAR_ARCHIVE);
	cl_crosshairX = Cvar_Get("cl_crosshairX", "0", CVAR_ARCHIVE);
	cl_crosshairY = Cvar_Get("cl_crosshairY", "0", CVAR_ARCHIVE);
	cl_crosshairSize = Cvar_Get("cl_crosshairSize", "24", CVAR_ARCHIVE);
	cl_crosshairColor = Cvar_Get("cl_crosshairColor", "7", CVAR_ARCHIVE);
	cl_crosshairAlpha = Cvar_Get("cl_crosshairAlpha", "1.0", CVAR_ARCHIVE);
	cl_crosshairHealth = Cvar_Get("cl_crosshairHealth", "0", CVAR_ARCHIVE);
	cl_crosshairNames = Cvar_Get("cl_crosshairNames", "1", CVAR_ARCHIVE);
	cl_viewSize = Cvar_Get("cl_viewSize", "100", CVAR_ARCHIVE);
	cl_centerTime = Cvar_Get("cl_centerTime", "2500", CVAR_ARCHIVE);
	cl_drawCenterString = Cvar_Get("cl_drawCenterString", "1", CVAR_ARCHIVE);
	cl_drawPause = Cvar_Get("cl_drawPause", "1", CVAR_ARCHIVE);
	cl_drawFPS = Cvar_Get("cl_drawFPS", "0", CVAR_ARCHIVE);
	cl_drawLagometer = Cvar_Get("cl_drawLagometer", "0", CVAR_ARCHIVE);
	cl_drawDisconnected = Cvar_Get("cl_drawDisconnected", "1", CVAR_ARCHIVE);
	cl_drawRecording = Cvar_Get("cl_drawRecording", "1", CVAR_ARCHIVE);
	cl_draw2D = Cvar_Get("cl_draw2D", "1", CVAR_ARCHIVE);
	cl_drawIcons = Cvar_Get("cl_drawIcons", "1", CVAR_ARCHIVE);
	cl_drawStatus = Cvar_Get("cl_drawStatus", "1", CVAR_ARCHIVE);
	cl_drawInventory = Cvar_Get("cl_drawInventory", "1", CVAR_ARCHIVE);
	cl_drawLayout = Cvar_Get("cl_drawLayout", "1", CVAR_ARCHIVE);
	cl_newHUD = Cvar_Get("cl_newHUD", "1", CVAR_ARCHIVE);
	cl_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE);
	cl_rconPassword = Cvar_Get("rconPassword", "", 0);
	cl_rconAddress = Cvar_Get("rconAddress", "", 0);

	Cmd_AddCommand("precache", CL_Precache_f);
	Cmd_AddCommand("changing", CL_Changing_f);
	Cmd_AddCommand("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand("connect", CL_Connect_f);
	Cmd_AddCommand("reconnect", CL_Reconnect_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("localservers", CL_LocalServers_f);
	Cmd_AddCommand("pingservers", CL_PingServers_f);
	Cmd_AddCommand("ping", CL_Ping_f);
	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_AddCommand("userinfo", CL_UserInfo_f);
	Cmd_AddCommand("skins", CL_Skins_f);
	Cmd_AddCommand("configstrings", CL_ConfigStrings_f);
	Cmd_AddCommand("cinematic", CL_Cinematic_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("stoprecord", CL_StopRecord_f);
	Cmd_AddCommand("testmodel", CL_TestModel_f);
	Cmd_AddCommand("testgun", CL_TestGun_f);
	Cmd_AddCommand("nextframe", CL_NextFrame_f);
	Cmd_AddCommand("prevframe", CL_PrevFrame_f);
	Cmd_AddCommand("nextskin", CL_NextSkin_f);
	Cmd_AddCommand("prevskin", CL_PrevSkin_f);
	Cmd_AddCommand("viewpos", CL_Viewpos_f);
	Cmd_AddCommand("timerefresh", CL_TimeRefresh_f);
	Cmd_AddCommand("sizeup", CL_SizeUp_f);
	Cmd_AddCommand("sizedown", CL_SizeDown_f);
	Cmd_AddCommand("centerview", CL_CenterView_f);
	Cmd_AddCommand("+zoom", CL_ZoomDown_f);
	Cmd_AddCommand("-zoom", CL_ZoomUp_f);
	Cmd_AddCommand("+moveup", CL_UpDown_f);
	Cmd_AddCommand("-moveup", CL_UpUp_f);
	Cmd_AddCommand("+movedown", CL_DownDown_f);
	Cmd_AddCommand("-movedown", CL_DownUp_f);
	Cmd_AddCommand("+left", CL_LeftDown_f);
	Cmd_AddCommand("-left", CL_LeftUp_f);
	Cmd_AddCommand("+right", CL_RightDown_f);
	Cmd_AddCommand("-right", CL_RightUp_f);
	Cmd_AddCommand("+forward", CL_ForwardDown_f);
	Cmd_AddCommand("-forward", CL_ForwardUp_f);
	Cmd_AddCommand("+back", CL_BackDown_f);
	Cmd_AddCommand("-back", CL_BackUp_f);
	Cmd_AddCommand("+lookup", CL_LookUpDown_f);
	Cmd_AddCommand("-lookup", CL_LookUpUp_f);
	Cmd_AddCommand("+lookdown", CL_LookDownDown_f);
	Cmd_AddCommand("-lookdown", CL_LookDownUp_f);
	Cmd_AddCommand("+strafe", CL_StrafeDown_f);
	Cmd_AddCommand("-strafe", CL_StrafeUp_f);
	Cmd_AddCommand("+moveleft", CL_MoveLeftDown_f);
	Cmd_AddCommand("-moveleft", CL_MoveLeftUp_f);
	Cmd_AddCommand("+moveright", CL_MoveRightDown_f);
	Cmd_AddCommand("-moveright", CL_MoveRightUp_f);
	Cmd_AddCommand("+speed", CL_SpeedDown_f);
	Cmd_AddCommand("-speed", CL_SpeedUp_f);
	Cmd_AddCommand("+attack", CL_AttackDown_f);
	Cmd_AddCommand("-attack", CL_AttackUp_f);
	Cmd_AddCommand("+use", CL_UseDown_f);
	Cmd_AddCommand("-use", CL_UseUp_f);
	Cmd_AddCommand("+klook", CL_KLookDown_f);
	Cmd_AddCommand("-klook", CL_KLookUp_f);
	Cmd_AddCommand("impulse", CL_Impulse_f);

	// Forward to server commands.
	// The only thing this does is allow command completion to work. All
	// unknown commands are automatically forwarded to the server.
	Cmd_AddCommand("wave", NULL);
	Cmd_AddCommand("kill", NULL);
	Cmd_AddCommand("use", NULL);
	Cmd_AddCommand("drop", NULL);
	Cmd_AddCommand("say", NULL);
	Cmd_AddCommand("say_team", NULL);
	Cmd_AddCommand("info", NULL);
	Cmd_AddCommand("give", NULL);
	Cmd_AddCommand("god", NULL);
	Cmd_AddCommand("notarget", NULL);
	Cmd_AddCommand("noclip", NULL);
	Cmd_AddCommand("inven", NULL);
	Cmd_AddCommand("invuse", NULL);
	Cmd_AddCommand("invprev", NULL);
	Cmd_AddCommand("invnext", NULL);
	Cmd_AddCommand("invdrop", NULL);
	Cmd_AddCommand("invprevw", NULL);
	Cmd_AddCommand("invnextw", NULL);
	Cmd_AddCommand("invprevp", NULL);
	Cmd_AddCommand("invnextp", NULL);
	Cmd_AddCommand("weapnext", NULL);
	Cmd_AddCommand("weapprev", NULL);
	Cmd_AddCommand("weaplast", NULL);
	Cmd_AddCommand("score", NULL);
	Cmd_AddCommand("help", NULL);
	Cmd_AddCommand("putaway", NULL);
	Cmd_AddCommand("players", NULL);
	Cmd_AddCommand("playerlist", NULL);
}

/*
 =================
 CL_ShutdownLocal
 =================
*/
static void CL_ShutdownLocal (void){

	memset(&cls, 0, sizeof(cls));

	Cmd_RemoveCommand("precache");
	Cmd_RemoveCommand("changing");
	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("connect");
	Cmd_RemoveCommand("reconnect");
	Cmd_RemoveCommand("disconnect");
	Cmd_RemoveCommand("localservers");
	Cmd_RemoveCommand("pingservers");
	Cmd_RemoveCommand("ping");
	Cmd_RemoveCommand("rcon");
	Cmd_RemoveCommand("userinfo");
	Cmd_RemoveCommand("skins");
	Cmd_RemoveCommand("configstrings");
	Cmd_RemoveCommand("cinematic");
	Cmd_RemoveCommand("record");
	Cmd_RemoveCommand("stoprecord");
	Cmd_RemoveCommand("testmodel");
	Cmd_RemoveCommand("testgun");
	Cmd_RemoveCommand("nextframe");
	Cmd_RemoveCommand("prevframe");
	Cmd_RemoveCommand("nextskin");
	Cmd_RemoveCommand("prevskin");
	Cmd_RemoveCommand("viewpos");
	Cmd_RemoveCommand("timerefresh");
	Cmd_RemoveCommand("sizeup");
	Cmd_RemoveCommand("sizedown");
	Cmd_RemoveCommand("centerview");
	Cmd_RemoveCommand("+zoom");
	Cmd_RemoveCommand("-zoom");
	Cmd_RemoveCommand("+moveup");
	Cmd_RemoveCommand("-moveup");
	Cmd_RemoveCommand("+movedown");
	Cmd_RemoveCommand("-movedown");
	Cmd_RemoveCommand("+left");
	Cmd_RemoveCommand("-left");
	Cmd_RemoveCommand("+right");
	Cmd_RemoveCommand("-right");
	Cmd_RemoveCommand("+forward");
	Cmd_RemoveCommand("-forward");
	Cmd_RemoveCommand("+back");
	Cmd_RemoveCommand("-back");
	Cmd_RemoveCommand("+lookup");
	Cmd_RemoveCommand("-lookup");
	Cmd_RemoveCommand("+lookdown");
	Cmd_RemoveCommand("-lookdown");
	Cmd_RemoveCommand("+strafe");
	Cmd_RemoveCommand("-strafe");
	Cmd_RemoveCommand("+moveleft");
	Cmd_RemoveCommand("-moveleft");
	Cmd_RemoveCommand("+moveright");
	Cmd_RemoveCommand("-moveright");
	Cmd_RemoveCommand("+speed");
	Cmd_RemoveCommand("-speed");
	Cmd_RemoveCommand("+attack");
	Cmd_RemoveCommand("-attack");
	Cmd_RemoveCommand("+use");
	Cmd_RemoveCommand("-use");
	Cmd_RemoveCommand("+klook");
	Cmd_RemoveCommand("-klook");
	Cmd_RemoveCommand("impulse");

	// Forward to server commands
	Cmd_RemoveCommand("wave");
	Cmd_RemoveCommand("kill");
	Cmd_RemoveCommand("use");
	Cmd_RemoveCommand("drop");
	Cmd_RemoveCommand("say");
	Cmd_RemoveCommand("say_team");
	Cmd_RemoveCommand("info");
	Cmd_RemoveCommand("give");
	Cmd_RemoveCommand("god");
	Cmd_RemoveCommand("notarget");
	Cmd_RemoveCommand("noclip");
	Cmd_RemoveCommand("inven");
	Cmd_RemoveCommand("invuse");
	Cmd_RemoveCommand("invprev");
	Cmd_RemoveCommand("invnext");
	Cmd_RemoveCommand("invdrop");
	Cmd_RemoveCommand("invprevw");
	Cmd_RemoveCommand("invnextw");
	Cmd_RemoveCommand("invprevp");
	Cmd_RemoveCommand("invnextp");
	Cmd_RemoveCommand("weapnext");
	Cmd_RemoveCommand("weapprev");
	Cmd_RemoveCommand("weaplast");
	Cmd_RemoveCommand("score");
	Cmd_RemoveCommand("help");
	Cmd_RemoveCommand("putaway");
	Cmd_RemoveCommand("players");
	Cmd_RemoveCommand("playerlist");
}


// =====================================================================


/*
 =================
 CL_Frame
 =================
*/
void CL_Frame (int msec){

	static int	extraTime;

	if (cls.state == CA_UNINITIALIZED)
		return;

	extraTime += msec;

	if (!timedemo->integer){
		if (cl_maxFPS->integer > 0){
			if (extraTime < 1000 / cl_maxFPS->integer)
				return;		// Framerate is too high
		}
	}

	// Decide the simulation time
	cl.time += extraTime;
	cls.realTime = Sys_Milliseconds();
	cls.frameTime = extraTime * 0.001;

	extraTime = 0;
	if (cls.frameTime > 0.2)
		cls.frameTime = 0.2;

	// If in the debugger last frame, don't time-out
	if (msec > 5000)
		cls.netChan.lastReceived = Sys_Milliseconds();

	// Fix any cheating cvars
	CL_FixCheatVars();

	// Fetch results from server
	CL_ReadPackets();

	// Pump message loop
	Sys_PumpMessages();

	// Let the mouse activate or deactivate
	IN_Frame();

	// Allow mouse and joystick to add commands
	IN_Commands();

	// Process console commands
	Cbuf_Execute();

	// Send intentions now
	CL_SendCmd();

	// Resend a connection request if necessary
	CL_CheckForResend();

	// Predict all unacknowledged movements
	CL_PredictMovement();

	// Allow video subsystem change
	VID_CheckChanges();

	// Update the screen
	if (com_speeds->integer)
		com_timeBeforeRef = Sys_Milliseconds();
	
	CL_UpdateScreen();
	
	if (com_speeds->integer)
		com_timeAfterRef = Sys_Milliseconds();

	// Update audio
	S_Update(cl.refDef.viewOrigin, vec3_origin, cl.refDef.viewAxis[0], cl.refDef.viewAxis[2]);

	CDAudio_Update();

	// Advance local effects for next frame
	CL_RunLightStyles();

	Con_RunConsole();

	// AVI demo frame dumping
	if (com_aviDemo->integer > 0){
		if ((cls.state == CA_ACTIVE && cl.demoPlayback) || com_forceAviDemo->integer)
			Cbuf_AddText("screenshot silent\n");
	}
}

/*
 =================
 CL_Init
 =================
*/
void CL_Init (void){

	if (dedicated->integer)
		return;		// Nothing running on the client

	Com_Printf("----- Client Initialization -----\n");

	CL_InitLocal();

	Con_Init();	

	VID_Init();
	S_Init();
	IN_Init();
	CDAudio_Init();

	UI_Init();

	CL_LoadLocalMedia();

	UI_SetActiveMenu(UI_MAINMENU);

	Com_Printf("----- Client Initialization Complete -----\n");
}

/*
 =================
 CL_Shutdown
 =================
*/
void CL_Shutdown (void){

	if (cls.state == CA_UNINITIALIZED)
		return;

	Com_Printf("----- CL_Shutdown -----\n");

	CL_Disconnect(true);

	UI_Shutdown();

	CDAudio_Shutdown();
	IN_Shutdown();
	S_Shutdown();
	VID_Shutdown();

	Con_Shutdown();

	CL_ShutdownLocal();

	Com_Printf("-----------------------\n");
}
