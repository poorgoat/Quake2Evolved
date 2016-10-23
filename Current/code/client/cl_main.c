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
cvar_t	*cl_footSteps;
cvar_t	*cl_noSkins;
cvar_t	*cl_noRender;
cvar_t	*cl_showNet;
cvar_t	*cl_showMiss;
cvar_t	*cl_showMaterial;
cvar_t	*cl_predict;
cvar_t	*cl_timeOut;
cvar_t	*cl_thirdPerson;
cvar_t	*cl_thirdPersonRange;
cvar_t	*cl_thirdPersonAngle;
cvar_t	*cl_viewBlend;
cvar_t	*cl_particles;
cvar_t	*cl_muzzleFlashes;
cvar_t	*cl_decals;
cvar_t	*cl_ejectBrass;
cvar_t	*cl_blood;
cvar_t	*cl_shells;
cvar_t	*cl_drawGun;
cvar_t	*cl_testGunX;
cvar_t	*cl_testGunY;
cvar_t	*cl_testGunZ;
cvar_t	*cl_testModelAnimate;
cvar_t	*cl_testModelRotate;
cvar_t	*cl_crosshairX;
cvar_t	*cl_crosshairY;
cvar_t	*cl_crosshairSize;
cvar_t	*cl_crosshairColor;
cvar_t	*cl_crosshairAlpha;
cvar_t	*cl_crosshairHealth;
cvar_t	*cl_crosshairNames;
cvar_t	*cl_centerTime;
cvar_t	*cl_draw2D;
cvar_t	*cl_drawCrosshair;
cvar_t	*cl_drawStatus;
cvar_t	*cl_drawIcons;
cvar_t	*cl_drawCenterString;
cvar_t	*cl_drawInventory;
cvar_t	*cl_drawLayout;
cvar_t	*cl_drawLagometer;
cvar_t	*cl_drawDisconnected;
cvar_t	*cl_drawRecording;
cvar_t	*cl_drawFPS;
cvar_t	*cl_drawPause;
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
 CL_ClearState
 =================
*/
void CL_ClearState (void){

	// Clear all local effects
	CL_ClearTempEntities();
	CL_ClearLocalEntities();
	CL_ClearDynamicLights();
	CL_ClearParticles();
	CL_ClearBloodBlends();
	CL_ClearLightStyles();

	// Wipe the entire cl structure
	memset(&cl, 0, sizeof(clientState_t));

	MSG_Clear(&cls.netChan.message);
}

/*
 =================
 CL_Restart

 Called before loading a level or after disconnecting from the server to
 restart the necessary subsystems
 =================
*/
void CL_Restart (void){

	// Make sure CD audio and sounds aren't playing
	CDAudio_Stop();
	S_StopAllSounds(true);

	// Shutdown UI and renderer
	UI_Shutdown();
	R_Shutdown(false);

	// Clear the hunk
	Hunk_Clear();

	// If connected to a remote server, we may need to restart the file
	// system to change the game directory
	if (cls.state >= CA_CONNECTED && !Com_ServerState())
		FS_Restart(cl.gameDir);

	// Initialize renderer and UI
	R_Init(false);
	UI_Init();

	// Load all local media
	CL_LoadLocalMedia();

	// Set menu visibility
	if (cls.state == CA_DISCONNECTED)
		UI_SetActiveMenu(UI_MAINMENU);
	else
		UI_SetActiveMenu(UI_CLOSEMENU);
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
	int		timeDemoMsec = 0, timeDemoFrames = 0;

	if (cls.state == CA_DISCONNECTED)
		return;

	if (com_timeDemo->integerValue){
		if (cls.state == CA_ACTIVE && cl.demoPlaying){
			timeDemoMsec = Sys_Milliseconds() - cl.timeDemoStart;
			timeDemoFrames = cl.timeDemoFrames;
		}
	}

	cls.state = CA_DISCONNECTED;
	cls.loading = false;
	cls.connectTime = 0;
	cls.connectCount = 0;

	// Stop download
	if (cls.downloadFile){
		FS_CloseFile(cls.downloadFile);
		cls.downloadFile = 0;
	}

	// Stop demo recording
	if (cls.demoFile)
		CL_StopRecord_f();

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
		return;		// We're shutting down the client

	// Restart the subsystems
	CL_Restart();

	if (timeDemoMsec != 0 && timeDemoFrames != 0)
		Com_Printf("%i frames, %3.1f seconds: %3.1f FPS\n", timeDemoFrames, MS2SEC(timeDemoMsec), timeDemoFrames * 1000.0 / timeDemoMsec);
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
 CL_MapLoading

 A local server is starting to load a map, so update the screen to let
 the user know about it
 =================
*/
void CL_MapLoading (void){

	if (cls.state == CA_UNINITIALIZED)
		return;

	// If we're already connected to the local server, stay connected
	if (cls.state >= CA_CONNECTED && NET_IsLocalAddress(cls.serverAddress)){
		cls.state = CA_CONNECTED;

		// Draw the loading screen
		CL_Loading();
		CL_UpdateScreen();

		return;
	}

	// Disconnect from server
	CL_Disconnect(false);

	// Connect to the local server
	Q_strncpyz(cls.serverName, "localhost", sizeof(cls.serverName));
	Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

	NET_StringToAdr(cls.serverName, &cls.serverAddress);

	// CL_CheckForResend will fire immediately
	cls.state = CA_CHALLENGING;		// We don't need a challenge on the localhost
	cls.connectTime = -99999;
	cls.connectCount = 0;

	// Draw the loading screen
	CL_Loading();
	CL_UpdateScreen();
}

/*
 =================
 CL_CheckForResend

 Resend a connection request message if the last one has timed out
 =================
*/
static void CL_CheckForResend (void){

	if (cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING)
		return;

	// Draw the loading screen
	CL_Loading();

	// Resend if we haven't gotten a reply yet
	if (cls.realTime - cls.connectTime < 3000)
		return;

	cls.connectTime = cls.realTime;
	cls.connectCount++;

	switch (cls.state){
	case CA_CONNECTING:
		// Send a connect message to the server
		NetChan_OutOfBandPrint(NS_CLIENT, cls.serverAddress, "getchallenge\n");

		break;
	case CA_CHALLENGING:
		// We have gotten a challenge from the server, so try and connect
		cvar_modifiedFlags &= ~CVAR_USERINFO;

		NetChan_OutOfBandPrint(NS_CLIENT, cls.serverAddress, "connect %i %i %i \"%s\"\n", PROTOCOL_VERSION, Cvar_GetInteger("net_qport"), cls.serverChallenge, Cvar_UserInfo());

		break;
	}
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

	NetChan_Setup(NS_CLIENT, &cls.netChan, net_from, Cvar_GetInteger("net_qport"));

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
	if (cls.state >= CA_CONNECTED && cls.realTime - cls.netChan.lastReceived > SEC2MS(cl_timeOut->floatValue)){
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
 CL_AVIFrame
 =================
*/
static void CL_AVIFrame (void){

	if (com_timeDemo->integerValue || com_aviDemo->integerValue <= 0){
		cls.aviFrame = 0;
		return;
	}

	if ((cls.state == CA_ACTIVE && cl.demoPlaying) || com_forceAviDemo->integerValue){
		Cbuf_ExecuteText(EXEC_NOW, va("screenShot -silent q2e_avi%08i.tga\n", cls.aviFrame));

		cls.aviFrame++;
	}
	else
		cls.aviFrame = 0;
}

/*
 =================
 CL_PlayBackgroundTrack
 =================
*/
void CL_PlayBackgroundTrack (void){

	char	name[MAX_OSPATH];
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
	if (FS_FileExists(name))
		S_StartBackgroundTrack(name, name);
	else
		CDAudio_Play(track, true);
}

/*
 =================
 CL_AllowCheats
 =================
*/
qboolean CL_AllowCheats (void){

	// Allow cheats if disconnected
	if (cls.state <= CA_DISCONNECTED)
		return true;

	// Allow cheats if playing a cinematic, demo or singleplayer game
	if (cls.cinematicPlaying || cl.demoPlaying || !cl.multiPlayer)
		return true;

	// Otherwise don't allow cheats at all
	return false;
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

	int		size;

	if (FS_FileExists(name))
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

	size = FS_OpenFile(cls.downloadTempName, &cls.downloadFile, FS_APPEND);
	if (!cls.downloadFile){
		Com_Printf("Failed to create %s\n", cls.downloadTempName);
		return true;
	}

	cls.downloadStart = cls.realTime;
	cls.downloadBytes = 0;
	cls.downloadPercent = 0;

	if (size){
		FS_Seek(cls.downloadFile, 0, FS_SEEK_END);

		Com_Printf("Resuming %s...\n", cls.downloadName);
		MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString(&cls.netChan.message, va("download %s %i", cls.downloadName, size));
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

 TODO: extend to support new file formats
 =================
*/
void CL_RequestNextDownload (void){

	char		name[MAX_OSPATH], *p;
	char		model[MAX_OSPATH], skin[MAX_OSPATH];
	md2Header_t	*md2;
	dheader_t	*header;
	dtexinfo_t	*texInfo;
	int			numTexInfo;
	int			i, n;

	if (cls.state != CA_CONNECTED)
		return;

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

	// Load the level
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
static void CL_Precache_f (void){

	if (cls.state != CA_CONNECTED)
		return;

	if (!cl.configStrings[CS_MODELS+1][0])
		return;

	// Restart the subsystems
	CL_Restart();

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
	cl_precacheMap = NULL;
	cl_precacheTexture = 0;

	if (Com_ServerState() || !cl_allowDownload->integerValue)
		cl_precacheCheck = TEX_CNT + 1;

	CL_RequestNextDownload();
}

/*
 =================
 CL_Changing_f

 The server will send this command as a hint that the client should draw
 the loading screen
 =================
*/
static void CL_Changing_f (void){

	Com_Printf("Changing map...\n");

	// Not active anymore, but not disconnected
	cls.state = CA_CONNECTED;

	// Draw the loading screen
	CL_Loading();
}

/*
 =================
 CL_ForwardToServer_f
 =================
*/
static void CL_ForwardToServer_f (void){

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
static void CL_Connect_f (void){

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
	Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

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

	Com_Printf("%s resolved to %s\n", cls.serverName, NET_AdrToString(cls.serverAddress));

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
static void CL_Reconnect_f (void){

	// If connected, the server is changing levels
	if (cls.state == CA_CONNECTED){
		Com_Printf("Reconnecting...\n");

		// Close download
		if (cls.downloadFile){
			FS_CloseFile(cls.downloadFile);
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

	Q_strncpyz(cls.serverMessage, "", sizeof(cls.serverMessage));

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

	Com_Printf("%s resolved to %s\n", cls.serverName, NET_AdrToString(cls.serverAddress));

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
static void CL_Disconnect_f (void){

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
static void CL_LocalServers_f (void){

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
static void CL_PingServers_f (void){

	int			i;
	netAdr_t	adr;
	char		*adrString;

	Com_Printf("Pinging favorites...\n");

	// Send a packet to each address book entry
	for (i = 0; i < 16; i++){
		adrString = Cvar_GetString(va("server%i", i+1));
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
static void CL_Ping_f (void){

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
static void CL_Rcon_f (void){

	netAdr_t	to;
	char		message[1024];
	int			i;

	if (cls.state >= CA_CONNECTED)
		to = cls.netChan.remoteAddress;
	else {
		if (!cl_rconAddress->value[0]){
			Com_Printf("You must either be connected, or set the \"rconAddress\" variable to issue rcon commands\n");
			return;
		}

		if (!NET_StringToAdr(cl_rconAddress->value, &to)){
			Com_Printf("Bad remote server address\n");
			return;
		}
		if (to.port == 0)
			to.port = BigShort(PORT_SERVER);
	}

	if (!cl_rconPassword->value[0]){
		Com_Printf("You must set the \"rconPassword\" variable to issue rcon commands\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	Q_strncatz(message, "rcon ", sizeof(message));
	Q_strncatz(message, cl_rconPassword->value, sizeof(message));
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
static void CL_UserInfo_f (void){

	Com_Printf("User info settings:\n");
	Com_Printf("-------------------\n");
	Info_Print(Cvar_UserInfo());
}

/*
 =================
 CL_Skins_f

 List and load any custom player skins and models
 =================
*/
static void CL_Skins_f (void){

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
static void CL_ConfigStrings_f (void){

	int		i, index;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: configStrings [index]\n");
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

	index = atoi(Cmd_Argv(1));
	if (index < 0 || index >= MAX_CONFIGSTRINGS){
		Com_Printf("Specified index is out of range\n");
		return;
	}

	Com_Printf("%4i: %s\n", index, cl.configStrings[index]);
}

/*
 =================
 CL_Cinematic_f
 =================
*/
static void CL_Cinematic_f (void){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: cinematic <videoName>\n");
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

	// Register our variables and commands
	Cvar_Get("server1", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server2", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server3", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server4", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server5", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server6", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server7", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server8", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server9", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server10", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server11", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server12", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server13", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server14", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server15", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("server16", "", CVAR_ARCHIVE, "Favorite server address");
	Cvar_Get("name", "Player", CVAR_USERINFO | CVAR_ARCHIVE, "Player name");
	Cvar_Get("skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE, "Player model/skin");
	Cvar_Get("gender", "male", CVAR_USERINFO | CVAR_ARCHIVE, "Player gender");
	Cvar_Get("msg", "1", CVAR_USERINFO | CVAR_ARCHIVE, "Message priority level");
	Cvar_Get("password", "", CVAR_USERINFO, "Server password");
	Cvar_Get("spectator", "0", CVAR_USERINFO, "Spectator mode");
	Cvar_Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE, "Network rate");
	Cvar_Get("fov", "90", CVAR_USERINFO | CVAR_ARCHIVE, "Field of view");
	cl_hand = Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE, "Player handedness");
	cl_zoomFov = Cvar_Get("cl_zoomFov", "25", CVAR_ARCHIVE, "Field of view when zooming");
	cl_footSteps = Cvar_Get("cl_footSteps", "1", CVAR_CHEAT, "Play footstep sound effects");
	cl_noSkins = Cvar_Get("cl_noSkins", "0", 0, "Don't load custom player skins");
	cl_noRender = Cvar_Get("cl_noRender", "0", CVAR_CHEAT, "Don't render game views");
	cl_showNet = Cvar_Get("cl_showNet", "0", 0, "Report network activity");
	cl_showMiss = Cvar_Get("cl_showMiss", "0", 0, "Report prediction misses");
	cl_showMaterial = Cvar_Get("cl_showMaterial", "0", CVAR_CHEAT, "Show material name, flags, and contents under crosshair");
	cl_predict = Cvar_Get("cl_predict", "1", CVAR_ARCHIVE, "Predict player movement");
	cl_timeOut = Cvar_Get("cl_timeOut", "120", 0, "Connection time out time in seconds");
	cl_thirdPerson = Cvar_Get("cl_thirdPerson", "0", CVAR_CHEAT, "Third-person camera mode");
	cl_thirdPersonRange = Cvar_Get("cl_thirdPersonRange", "40", CVAR_CHEAT, "Third-person camera range");
	cl_thirdPersonAngle = Cvar_Get("cl_thirdPersonAngle", "0", CVAR_CHEAT, "Third-person camera angle");
	cl_viewBlend = Cvar_Get("cl_viewBlend", "1", CVAR_ARCHIVE, "Draw blended effects over the game view");
	cl_particles = Cvar_Get("cl_particles", "1", CVAR_ARCHIVE, "Draw particles");
	cl_muzzleFlashes = Cvar_Get("cl_muzzleFlashes", "1", CVAR_ARCHIVE, "Draw muzzle-flashes");
	cl_decals = Cvar_Get("cl_decals", "1", CVAR_ARCHIVE, "Draw decals");
	cl_ejectBrass = Cvar_Get("cl_ejectBrass", "1", CVAR_ARCHIVE, "Draw ejecting brass from weapons");
	cl_blood = Cvar_Get("cl_blood", "1", CVAR_ARCHIVE, "Draw blood");
	cl_shells = Cvar_Get("cl_shells", "1", CVAR_ARCHIVE, "Draw shells");
	cl_drawGun = Cvar_Get("cl_drawGun", "1", CVAR_ARCHIVE, "Draw gun");
	cl_testGunX = Cvar_Get("cl_testGunX", "0", CVAR_CHEAT, "Test gun X offset");
	cl_testGunY = Cvar_Get("cl_testGunY", "0", CVAR_CHEAT, "Test gun Y offset");
	cl_testGunZ = Cvar_Get("cl_testGunZ", "0", CVAR_CHEAT, "Test gun Z offset");
	cl_testModelAnimate = Cvar_Get("cl_testModelAnimate", "0", CVAR_CHEAT, "Test model animation");
	cl_testModelRotate = Cvar_Get("cl_testModelRotate", "0.0", CVAR_CHEAT, "Test model rotation");
	cl_crosshairX = Cvar_Get("cl_crosshairX", "0", CVAR_ARCHIVE, "Crosshair X offset");
	cl_crosshairY = Cvar_Get("cl_crosshairY", "0", CVAR_ARCHIVE, "Crosshair Y offset");
	cl_crosshairSize = Cvar_Get("cl_crosshairSize", "24", CVAR_ARCHIVE, "Crosshair size");
	cl_crosshairColor = Cvar_Get("cl_crosshairColor", "7", CVAR_ARCHIVE, "Crosshair color index");
	cl_crosshairAlpha = Cvar_Get("cl_crosshairAlpha", "1.0", CVAR_ARCHIVE, "Crosshair translucency level");
	cl_crosshairHealth = Cvar_Get("cl_crosshairHealth", "0", CVAR_ARCHIVE, "Color crosshair based on health");
	cl_crosshairNames = Cvar_Get("cl_crosshairNames", "1", CVAR_ARCHIVE, "Draw player names under crosshair");
	cl_centerTime = Cvar_Get("cl_centerTime", "2500", CVAR_ARCHIVE, "Time for centered messages");
	cl_draw2D = Cvar_Get("cl_draw2D", "1", CVAR_ARCHIVE, "Draw 2D elements on screen");
	cl_drawCrosshair = Cvar_Get("cl_drawCrosshair", "0", CVAR_ARCHIVE, "Draw crosshair");
	cl_drawStatus = Cvar_Get("cl_drawStatus", "1", CVAR_ARCHIVE, "Draw status bar");
	cl_drawIcons = Cvar_Get("cl_drawIcons", "1", CVAR_ARCHIVE, "Draw icons in the status bar");
	cl_drawCenterString = Cvar_Get("cl_drawCenterString", "1", CVAR_ARCHIVE, "Draw centered messages");
	cl_drawInventory = Cvar_Get("cl_drawInventory", "1", CVAR_ARCHIVE, "Draw inventory");
	cl_drawLayout = Cvar_Get("cl_drawLayout", "1", CVAR_ARCHIVE, "Draw status layouts");
	cl_drawLagometer = Cvar_Get("cl_drawLagometer", "0", CVAR_ARCHIVE, "Draw lagometer");
	cl_drawDisconnected = Cvar_Get("cl_drawDisconnected", "1", CVAR_ARCHIVE, "Draw connection interrupted warnings");
	cl_drawRecording = Cvar_Get("cl_drawRecording", "1", CVAR_ARCHIVE, "Draw demo recording information");
	cl_drawFPS = Cvar_Get("cl_drawFPS", "0", CVAR_ARCHIVE, "Draw frames per second");
	cl_drawPause = Cvar_Get("cl_drawPause", "1", CVAR_ARCHIVE, "Draw pause");
	cl_newHUD = Cvar_Get("cl_newHUD", "0", CVAR_ARCHIVE, "Use the enhanced HUD");
	cl_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE, "Allow file downloads from server");
	cl_rconPassword = Cvar_Get("rconPassword", "", 0, "Remote console password");
	cl_rconAddress = Cvar_Get("rconAddress", "", 0, "Remote console address");

	Cmd_AddCommand("precache", CL_Precache_f, "Precache files for current map");
	Cmd_AddCommand("changing", CL_Changing_f, "Change map");
	Cmd_AddCommand("cmd", CL_ForwardToServer_f, "Forward to server command");
	Cmd_AddCommand("connect", CL_Connect_f, "Connect to a server");
	Cmd_AddCommand("reconnect", CL_Reconnect_f, "Reconnect to the last server");
	Cmd_AddCommand("disconnect", CL_Disconnect_f, "Disconnect from server");
	Cmd_AddCommand("localServers", CL_LocalServers_f, "Ping local servers");
	Cmd_AddCommand("pingServers", CL_PingServers_f, "Ping favorite servers");
	Cmd_AddCommand("ping", CL_Ping_f, "Ping a server");
	Cmd_AddCommand("rcon", CL_Rcon_f, "Issue a remote console command");
	Cmd_AddCommand("userInfo", CL_UserInfo_f, "Show user information");
	Cmd_AddCommand("skins", CL_Skins_f, "Show player skins");
	Cmd_AddCommand("configStrings", CL_ConfigStrings_f, "Show config strings");
	Cmd_AddCommand("cinematic", CL_Cinematic_f, "Play a cinematic");
	Cmd_AddCommand("record", CL_Record_f, "Record a demo");
	Cmd_AddCommand("stopRecord", CL_StopRecord_f, "Stop recording a demo");
	Cmd_AddCommand("testModel", CL_TestModel_f, "Test a model");
	Cmd_AddCommand("testGun", CL_TestGun_f, "Test a gun model");
	Cmd_AddCommand("testMaterial", CL_TestMaterial_f, "Test a material on the current test model");
	Cmd_AddCommand("testMaterialParm", CL_TestMaterialParm_f, "Test a material parm on the current test model");
	Cmd_AddCommand("nextFrame", CL_NextFrame_f, "Show the next frame on the current test model");
	Cmd_AddCommand("prevFrame", CL_PrevFrame_f, "Show the previous frame on the current test model");
	Cmd_AddCommand("nextSkin", CL_NextSkin_f, "Show the next skin on the current test model");
	Cmd_AddCommand("prevSkin", CL_PrevSkin_f, "Show the previous skin on the current test model");
	Cmd_AddCommand("testPostProcess", CL_TestPostProcess_f, "Test a post-process material");
	Cmd_AddCommand("viewPos", CL_ViewPos_f, "Report the current view position and angle");

	// Forward to server commands.
	// The only thing this does is allow command completion to work. All
	// unknown commands are automatically forwarded to the server.
	Cmd_AddCommand("wave", NULL, NULL);
	Cmd_AddCommand("kill", NULL, NULL);
	Cmd_AddCommand("use", NULL, NULL);
	Cmd_AddCommand("drop", NULL, NULL);
	Cmd_AddCommand("say", NULL, NULL);
	Cmd_AddCommand("say_team", NULL, NULL);
	Cmd_AddCommand("info", NULL, NULL);
	Cmd_AddCommand("give", NULL, NULL);
	Cmd_AddCommand("god", NULL, NULL);
	Cmd_AddCommand("notarget", NULL, NULL);
	Cmd_AddCommand("noclip", NULL, NULL);
	Cmd_AddCommand("inven", NULL, NULL);
	Cmd_AddCommand("invuse", NULL, NULL);
	Cmd_AddCommand("invprev", NULL, NULL);
	Cmd_AddCommand("invnext", NULL, NULL);
	Cmd_AddCommand("invdrop", NULL, NULL);
	Cmd_AddCommand("invprevw", NULL, NULL);
	Cmd_AddCommand("invnextw", NULL, NULL);
	Cmd_AddCommand("invprevp", NULL, NULL);
	Cmd_AddCommand("invnextp", NULL, NULL);
	Cmd_AddCommand("weapnext", NULL, NULL);
	Cmd_AddCommand("weapprev", NULL, NULL);
	Cmd_AddCommand("weaplast", NULL, NULL);
	Cmd_AddCommand("score", NULL, NULL);
	Cmd_AddCommand("help", NULL, NULL);
	Cmd_AddCommand("putaway", NULL, NULL);
	Cmd_AddCommand("players", NULL, NULL);
	Cmd_AddCommand("playerlist", NULL, NULL);
}

/*
 =================
 CL_ShutdownLocal
 =================
*/
static void CL_ShutdownLocal (void){

	memset(&cls, 0, sizeof(clientStatic_t));

	Cmd_RemoveCommand("precache");
	Cmd_RemoveCommand("changing");
	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("connect");
	Cmd_RemoveCommand("reconnect");
	Cmd_RemoveCommand("disconnect");
	Cmd_RemoveCommand("localServers");
	Cmd_RemoveCommand("pingServers");
	Cmd_RemoveCommand("ping");
	Cmd_RemoveCommand("rcon");
	Cmd_RemoveCommand("userInfo");
	Cmd_RemoveCommand("skins");
	Cmd_RemoveCommand("configStrings");
	Cmd_RemoveCommand("cinematic");
	Cmd_RemoveCommand("record");
	Cmd_RemoveCommand("stopRecord");
	Cmd_RemoveCommand("testModel");
	Cmd_RemoveCommand("testGun");
	Cmd_RemoveCommand("testMaterial");
	Cmd_RemoveCommand("testMaterialParm");
	Cmd_RemoveCommand("nextFrame");
	Cmd_RemoveCommand("prevFrame");
	Cmd_RemoveCommand("nextSkin");
	Cmd_RemoveCommand("prevSkin");
	Cmd_RemoveCommand("testPostProcess");
	Cmd_RemoveCommand("viewPos");

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

	if (cls.state == CA_UNINITIALIZED)
		return;

	if (com_timeDemo->integerValue){
		if (cls.state == CA_ACTIVE && cl.demoPlaying){
			if (!cl.timeDemoStart)
				cl.timeDemoStart = Sys_Milliseconds();

			cl.timeDemoFrames++;
		}
	}

	// Decide the simulation time
	cl.time += msec;
	cls.realTime = Sys_Milliseconds();

	cls.frameCount++;
	cls.frameTime = MS2SEC(msec);
	if (cls.frameTime > 0.2)
		cls.frameTime = 0.2;

	// If in the debugger last frame, don't time-out
	if (msec > 5000)
		cls.netChan.lastReceived = cls.realTime;

	// Fetch results from server
	CL_ReadPackets();

	// Get new events
	Sys_GetEvents();

	// Let the mouse activate/deactivate and move
	IN_Frame();

	// Process commands
	Cbuf_Execute();

	// Send intentions now
	CL_SendCmd();

	// Resend a connection request if necessary
	CL_CheckForResend();

	// Predict all unacknowledged movements
	CL_PredictMovement();

	// Update the screen
	if (com_speeds->integerValue)
		com_timeBeforeRef = Sys_Milliseconds();

	CL_UpdateScreen();

	if (com_speeds->integerValue)
		com_timeAfterRef = Sys_Milliseconds();

	// Update audio
	if (com_speeds->integerValue)
		com_timeBeforeSnd = Sys_Milliseconds();

	S_Update(cl.renderView.viewOrigin, vec3_origin, cl.renderView.viewAxis[0], cl.renderView.viewAxis[2], cl.underWater);

	CDAudio_Update();

	if (com_speeds->integerValue)
		com_timeAfterSnd = Sys_Milliseconds();

	// Advance local effects for next frame
	CL_RunLightStyles();

	CL_RunCinematic();

	Con_RunConsole();

	// AVI demo frame dumping
	CL_AVIFrame();
}

/*
 =================
 CL_Init
 =================
*/
void CL_Init (void){

	if (com_dedicated->integerValue)
		return;		// Nothing running on the client

	Com_Printf("------- Client Initialization -------\n");

	CL_InitLocal();
	CL_InitInput();

	Con_Init();	

	VID_Init();
	S_Init();
	IN_Init();
	CDAudio_Init();

	UI_Init();

	CL_LoadLocalMedia();

	UI_SetActiveMenu(UI_MAINMENU);

	Com_Printf("-------------------------------------\n");
}

/*
 =================
 CL_Shutdown
 =================
*/
void CL_Shutdown (void){

	if (cls.state == CA_UNINITIALIZED)
		return;

	Com_Printf("------- Client Shutdown -------\n");

	CL_Disconnect(true);

	UI_Shutdown();

	CDAudio_Shutdown();
	IN_Shutdown();
	S_Shutdown();
	VID_Shutdown();

	Con_Shutdown();

	CL_ShutdownInput();
	CL_ShutdownLocal();

	Com_Printf("-------------------------------\n");
}
