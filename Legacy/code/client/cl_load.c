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


/*
 =================
 CL_UpdateLoading
 =================
*/
static void CL_UpdateLoading (const char *string){

	Q_strncpyz(cls.loadingInfo.string, string, sizeof(cls.loadingInfo.string));
	cls.loadingInfo.percent += 10;

	Sys_PumpMessages();
	CL_UpdateScreen();
}

/*
 =================
 CL_RegisterCollisionMap
 =================
*/
static void CL_RegisterCollisionMap (void){

	unsigned	checksum;

	CL_UpdateLoading("COLLISION MAP");

	CM_LoadMap(cl.configStrings[CS_MODELS+1], true, &checksum);

	if (checksum != atoi(cl.configStrings[CS_MAPCHECKSUM]))
		Com_Error(ERR_DROP, "Local map version differs from server: %i != %s", checksum, cl.configStrings[CS_MAPCHECKSUM]);
}

/*
 =================
 CL_RegisterSounds
 =================
*/
static void CL_RegisterSounds (void){

	int		i;

	// Register sounds
	CL_UpdateLoading("SOUNDS");

	cl.media.sfxRichotecs[0] = S_RegisterSound("world/ric1.wav");
	cl.media.sfxRichotecs[1] = S_RegisterSound("world/ric2.wav");
	cl.media.sfxRichotecs[2] = S_RegisterSound("world/ric3.wav");
	cl.media.sfxSparks[0] = S_RegisterSound("world/spark5.wav");
	cl.media.sfxSparks[1] = S_RegisterSound("world/spark6.wav");
	cl.media.sfxSparks[2] = S_RegisterSound("world/spark7.wav");
	cl.media.sfxFootSteps[0] = S_RegisterSound("player/step1.wav");
	cl.media.sfxFootSteps[1] = S_RegisterSound("player/step2.wav");
	cl.media.sfxFootSteps[2] = S_RegisterSound("player/step3.wav");
	cl.media.sfxFootSteps[3] = S_RegisterSound("player/step4.wav");
	cl.media.sfxLaserHit = S_RegisterSound("weapons/lashit.wav");
	cl.media.sfxRailgun = S_RegisterSound("weapons/railgf1a.wav");
	cl.media.sfxRocketExplosion = S_RegisterSound("weapons/rocklx1a.wav");
	cl.media.sfxGrenadeExplosion = S_RegisterSound("weapons/grenlx1a.wav");
	cl.media.sfxWaterExplosion = S_RegisterSound("weapons/xpld_wat.wav");
	cl.media.sfxMachinegunBrass = S_RegisterSound("weapons/brass_bullet.wav");
	cl.media.sfxShotgunBrass = S_RegisterSound("weapons/brass_shell.wav");

	if (!Q_stricmp(cl.gameDir, "rogue")){
		cl.media.sfxLightning = S_RegisterSound("weapons/tesla.wav");
		cl.media.sfxDisruptorExplosion = S_RegisterSound("weapons/disrupthit.wav");
	}

	S_RegisterSound("player/land1.wav");
	S_RegisterSound("player/fall2.wav");
	S_RegisterSound("player/fall1.wav");

	// Register the sounds that the server references
	CL_UpdateLoading("GAME SOUNDS");

	for (i = 1; i < MAX_SOUNDS; i++){
		if (!cl.configStrings[CS_SOUNDS+i][0])
			break;
	
		cl.media.gameSounds[i] = S_RegisterSound(cl.configStrings[CS_SOUNDS+i]);
	}
}

/*
 =================
 CL_RegisterGraphics
 =================
*/
static void CL_RegisterGraphics (void){

	int		i;
	float	skyRotate;
	vec3_t	skyAxis;
	char	name[MAX_QPATH];

	// Load the map
	CL_UpdateLoading("MAP");

	skyRotate = atof(cl.configStrings[CS_SKYROTATE]);
	sscanf(cl.configStrings[CS_SKYAXIS], "%f %f %f", &skyAxis[0], &skyAxis[1], &skyAxis[2]);

	R_LoadWorldMap(cl.configStrings[CS_MODELS+1], cl.configStrings[CS_SKY], skyRotate, skyAxis);

	// Register models
	CL_UpdateLoading("MODELS");

	cl.media.modParasiteBeam = R_RegisterModel("models/monsters/parasite/segment/tris.md2");
	cl.media.modPowerScreenShell = R_RegisterModel("models/items/armor/effect/tris.md2");
	cl.media.modMachinegunBrass = R_RegisterModel("models/misc/b_shell/tris.md2");
	cl.media.modShotgunBrass = R_RegisterModel("models/misc/s_shell/tris.md2");

	R_RegisterModel("models/objects/laser/tris.md2");
	R_RegisterModel("models/objects/grenade2/tris.md2");
	R_RegisterModel("models/weapons/v_machn/tris.md2");
	R_RegisterModel("models/weapons/v_handgr/tris.md2");
	R_RegisterModel("models/weapons/v_shotg2/tris.md2");
	R_RegisterModel("models/objects/gibs/bone/tris.md2");
	R_RegisterModel("models/objects/gibs/sm_meat/tris.md2");
	R_RegisterModel("models/objects/gibs/bone2/tris.md2");

	// Register the models that the server references
	CL_UpdateLoading("GAME MODELS");

	Q_strncpyz(cl.weaponModels[0], "weapon", sizeof(cl.weaponModels[0]));
	cl.numWeaponModels = 1;

	for (i = 1; i < MAX_MODELS; i++){
		if (!cl.configStrings[CS_MODELS+i][0])
			break;

		if (cl.configStrings[CS_MODELS+i][0] == '#'){
			// Special player weapon model
			if (cl.numWeaponModels < MAX_CLIENTWEAPONMODELS){
				Com_StripExtension(cl.configStrings[CS_MODELS+i]+1, cl.weaponModels[cl.numWeaponModels], sizeof(cl.weaponModels[cl.numWeaponModels]));
				cl.numWeaponModels++;
			}
		} 
		else {
			cl.media.gameModels[i] = R_RegisterModel(cl.configStrings[CS_MODELS+i]);

			if (cl.configStrings[CS_MODELS+i][0] == '*')
				cl.media.gameCModels[i] = CM_InlineModel(cl.configStrings[CS_MODELS+i]);
			else
				cl.media.gameCModels[i] = NULL;
		}
	}

	// Register shaders
	CL_UpdateLoading("SHADERS");

	cl.media.lagometerShader = R_RegisterShaderNoMip("lagometer");
	cl.media.disconnectedShader = R_RegisterShaderNoMip("disconnected");
	cl.media.backTileShader = R_RegisterShaderNoMip("backTile");
	cl.media.pauseShader = R_RegisterShaderNoMip("pause");

	for (i = 0; i < NUM_CROSSHAIRS; i++)
		cl.media.crosshairShaders[i] = R_RegisterShaderNoMip(va("pics/crosshair%i", i+1));

	for (i = 0; i < 11; i++){
		if (i != 10){
			cl.media.hudNumberShaders[0][i] = R_RegisterShaderNoMip(va("pics/num_%i", i));
			cl.media.hudNumberShaders[1][i] = R_RegisterShaderNoMip(va("pics/anum_%i", i));
		}
		else {
			cl.media.hudNumberShaders[0][i] = R_RegisterShaderNoMip("pics/num_minus");
			cl.media.hudNumberShaders[1][i] = R_RegisterShaderNoMip("pics/anum_minus");
		}
	}

	cl.media.viewBloodBlend = R_RegisterShaderNoMip("viewBloodBlend");
	cl.media.viewFireBlend = R_RegisterShaderNoMip("viewFireBlend");
	cl.media.viewIrGoggles = R_RegisterShaderNoMip("viewIrGoggles");

	cl.media.rocketExplosionShader = R_RegisterShader("rocketExplosion");
	cl.media.rocketExplosionWaterShader = R_RegisterShader("rocketExplosionWater");
	cl.media.grenadeExplosionShader = R_RegisterShader("grenadeExplosion");
	cl.media.grenadeExplosionWaterShader = R_RegisterShader("grenadeExplosionWater");
	cl.media.bfgExplosionShader = R_RegisterShader("bfgExplosion");
	cl.media.bfgBallShader = R_RegisterShader("bfgBall");
	cl.media.plasmaBallShader = R_RegisterShader("plasmaBall");
	cl.media.waterPlumeShader = R_RegisterShader("waterPlume");
	cl.media.waterSprayShader = R_RegisterShader("waterSpray");
	cl.media.waterWakeShader = R_RegisterShader("waterWake");
	cl.media.nukeShockwaveShader = R_RegisterShader("nukeShockwave");
	cl.media.bloodSplatShader[0] = R_RegisterShader("bloodSplat");
	cl.media.bloodSplatShader[1] = R_RegisterShader("greenBloodSplat");
	cl.media.bloodCloudShader[0] = R_RegisterShader("bloodCloud");
	cl.media.bloodCloudShader[1] = R_RegisterShader("greenBloodCloud");

	cl.media.powerScreenShellShader = R_RegisterShaderSkin("shells/powerScreen");
	cl.media.invulnerabilityShellShader = R_RegisterShaderSkin("shells/invulnerability");
	cl.media.quadDamageShellShader = R_RegisterShaderSkin("shells/quadDamage");
	cl.media.doubleDamageShellShader = R_RegisterShaderSkin("shells/doubleDamage");
	cl.media.halfDamageShellShader = R_RegisterShaderSkin("shells/halfDamage");
	cl.media.genericShellShader = R_RegisterShaderSkin("shells/generic");

	cl.media.laserBeamShader = R_RegisterShader("beams/laser");
	cl.media.grappleBeamShader = R_RegisterShader("beams/grapple");
	cl.media.lightningBeamShader = R_RegisterShader("beams/lightning");
	cl.media.heatBeamShader = R_RegisterShader("beams/heat");

	cl.media.energyParticleShader = R_RegisterShader("particles/energy");
	cl.media.glowParticleShader = R_RegisterShader("particles/glow");
	cl.media.flameParticleShader = R_RegisterShader("particles/flame");
	cl.media.smokeParticleShader = R_RegisterShader("particles/smoke");
	cl.media.liteSmokeParticleShader = R_RegisterShader("particles/liteSmoke");
	cl.media.bubbleParticleShader = R_RegisterShader("particles/bubble");
	cl.media.dropletParticleShader = R_RegisterShader("particles/droplet");
	cl.media.steamParticleShader = R_RegisterShader("particles/steam");
	cl.media.sparkParticleShader = R_RegisterShader("particles/spark");
	cl.media.impactSparkParticleShader = R_RegisterShader("particles/impactSpark");
	cl.media.trackerParticleShader = R_RegisterShader("particles/tracker");
	cl.media.flyParticleShader = R_RegisterShader("particles/fly");

	cl.media.energyMarkShader = R_RegisterShader("decals/energyMark");
	cl.media.bulletMarkShader = R_RegisterShader("decals/bulletMark");
	cl.media.burnMarkShader = R_RegisterShader("decals/burnMark");
	cl.media.bloodMarkShaders[0][0] = R_RegisterShader("decals/bloodMark1");
	cl.media.bloodMarkShaders[0][1] = R_RegisterShader("decals/bloodMark2");
	cl.media.bloodMarkShaders[0][2] = R_RegisterShader("decals/bloodMark3");
	cl.media.bloodMarkShaders[0][3] = R_RegisterShader("decals/bloodMark4");
	cl.media.bloodMarkShaders[0][4] = R_RegisterShader("decals/bloodMark5");
	cl.media.bloodMarkShaders[0][5] = R_RegisterShader("decals/bloodMark6");
	cl.media.bloodMarkShaders[1][0] = R_RegisterShader("decals/greenBloodMark1");
	cl.media.bloodMarkShaders[1][1] = R_RegisterShader("decals/greenBloodMark2");
	cl.media.bloodMarkShaders[1][2] = R_RegisterShader("decals/greenBloodMark3");
	cl.media.bloodMarkShaders[1][3] = R_RegisterShader("decals/greenBloodMark4");
	cl.media.bloodMarkShaders[1][4] = R_RegisterShader("decals/greenBloodMark5");
	cl.media.bloodMarkShaders[1][5] = R_RegisterShader("decals/greenBloodMark6");

	CL_LoadHUD();

	R_RegisterShaderNoMip("pics/w_machinegun");
	R_RegisterShaderNoMip("pics/a_bullets");
	R_RegisterShaderNoMip("pics/i_health");
	R_RegisterShaderNoMip("pics/a_grenades");

	// Register the shaders that the server references
	CL_UpdateLoading("GAME SHADERS");

	for (i = 1; i < MAX_IMAGES; i++){
		if (!cl.configStrings[CS_IMAGES+i][0])
			break;

		if (!strchr(cl.configStrings[CS_IMAGES+i], '/'))
			Q_snprintfz(name, sizeof(name), "pics/%s", cl.configStrings[CS_IMAGES+i]);
		else
			Com_StripExtension(cl.configStrings[CS_IMAGES+i], name, sizeof(name));

		cl.media.gameShaders[i] = R_RegisterShaderNoMip(name);
	}
}

/*
 =================
 CL_RegisterClients
 =================
*/
static void CL_RegisterClients (void){

	int		i;

	// Register all the clients in the server
	CL_UpdateLoading("CLIENTS");

	CL_LoadClientInfo(&cl.baseClientInfo, "unnamed\\male/grunt");

	for (i = 0; i < MAX_CLIENTS; i++){
		if (!cl.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		CL_LoadClientInfo(&cl.clientInfo[i], cl.configStrings[CS_PLAYERSKINS+i]);
	}
}

/*
 =================
 CL_Loading
 =================
*/
void CL_Loading (void){

	if (cls.loading)
		return;

	cls.loading = true;
	memset(&cls.loadingInfo, 0, sizeof(loadingInfo_t));

	// Make sure CD audio and sounds aren't playing
	CDAudio_Stop();
	S_StopAllSounds();

	// Force menu and console off
	UI_SetActiveMenu(UI_CLOSEMENU);
	Con_CloseConsole();
}

/*
 =================
 CL_DrawLoading
 =================
*/
void CL_DrawLoading (void){

	char	str[1024];
	float	speed;
	int		percent;

	if (!cls.loading)
		return;

	switch (cls.state){
	case CA_DISCONNECTED:
		// Still disconnected, but local server is initializing
		CL_DrawPicByName(0, 0, 640, 480, colorWhite, "ui/title_screen/title_backg");
		CL_DrawPicByName(0, 0, 640, 160, colorWhite, "ui/title_screen/q2e_logo");

		break;
	case CA_CONNECTING:
		// Awaiting connection
		CL_DrawPicByName(0, 0, 640, 480, colorWhite, "ui/title_screen/title_backg");
		CL_DrawPicByName(0, 0, 640, 160, colorWhite, "ui/title_screen/q2e_logo");

		if (Com_ServerState()){
			Q_snprintfz(str, sizeof(str), "Starting up...");
			CL_DrawString(0, 424, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			if (cls.serverMessage[0])
				CL_DrawString(0, 360, 16, 16, 0, 0, 640, cls.serverMessage, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);

			Q_snprintfz(str, sizeof(str), "Connecting to %s\nAwaiting connection... %i", cls.serverName, cls.connectCount);
			CL_DrawString(0, 408, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		break;
	case CA_CHALLENGING:
		// Awaiting challenge
		CL_DrawPicByName(0, 0, 640, 480, colorWhite, "ui/title_screen/title_backg");
		CL_DrawPicByName(0, 0, 640, 160, colorWhite, "ui/title_screen/q2e_logo");

		if (Com_ServerState()){
			Q_snprintfz(str, sizeof(str), "Starting up...");
			CL_DrawString(0, 424, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			if (cls.serverMessage[0])
				CL_DrawString(0, 360, 16, 16, 0, 0, 640, cls.serverMessage, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);

			Q_snprintfz(str, sizeof(str), "Connecting to %s\nAwaiting challenge... %i", cls.serverName, cls.connectCount);
			CL_DrawString(0, 408, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		break;
	case CA_CONNECTED:
		if (cls.downloadFile){
			// Downloading file from server
			CL_DrawPicByName(0, 0, 640, 480, colorWhite, "ui/title_screen/title_backg");
			CL_DrawPicByName(0, 0, 640, 160, colorWhite, "ui/title_screen/q2e_logo");

			if (cls.downloadStart != cls.realTime)
				speed = (float)(cls.downloadBytes / 1024) / ((cls.realTime - cls.downloadStart) / 1000);
			else
				speed = 0;

			if (Com_ServerState()){
				Q_snprintfz(str, sizeof(str), "Downloading %s... (%.2f KB/sec)", cls.downloadName, speed);
				CL_DrawString(0, 424, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
			}
			else {
				Q_snprintfz(str, sizeof(str), "Connecting to %s\nDownloading %s... (%.2f KB/sec)", cls.serverName, cls.downloadName, speed);
				CL_DrawString(0, 408, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
			}

			percent = Clamp(cls.downloadPercent - (cls.downloadPercent % 5), 5, 100);
			if (percent){
				CL_DrawPicByName(240, 160, 160, 160, colorWhite, "ui/loading/load_main2");
				CL_DrawPicByName(240, 160, 160, 160, colorWhite, va("ui/loading/percent/load_%i", percent));
				CL_DrawPicByName(240, 160, 160, 160, colorWhite, "ui/loading/load_main");
			}

			break;
		}

		// Awaiting game state
		CL_DrawPicByName(0, 0, 640, 480, colorWhite, "ui/title_screen/title_backg");
		CL_DrawPicByName(0, 0, 640, 160, colorWhite, "ui/title_screen/q2e_logo");

		if (Com_ServerState()){
			Q_snprintfz(str, sizeof(str), "Starting up...");
			CL_DrawString(0, 424, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			Q_snprintfz(str, sizeof(str), "Connecting to %s\nAwaiting game state...", cls.serverName);
			CL_DrawString(0, 408, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		break;
	case CA_LOADING:
		// Loading level data
		CL_DrawPic(0, 0, 640, 480, colorWhite, cl.media.levelshot);
		CL_DrawPic(0, 0, 640, 480, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0, 0, 640, 160, colorWhite, cl.media.loadingLogo);

		if (Com_ServerState()){
			Q_snprintfz(str, sizeof(str), "Loading %s\n\"%s\"\n\n\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.loadingInfo.string);
			CL_DrawString(0, 360, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			Q_snprintfz(str, sizeof(str), "Loading %s\n\"%s\"\n\nConnecting to %s\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.serverName, cls.loadingInfo.string);
			CL_DrawString(0, 360, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		percent = Clamp((cls.loadingInfo.percent / 5) - 1, 0, 19);
		if (percent){
			CL_DrawPic(240, 160, 160, 160, colorWhite, cl.media.loadingDetail[0]);
			CL_DrawPic(240, 160, 160, 160, colorWhite, cl.media.loadingPercent[percent]);
			CL_DrawPic(240, 160, 160, 160, colorWhite, cl.media.loadingDetail[1]);
		}

		break;
	case CA_PRIMED:
		// Awaiting frame
		CL_DrawPic(0, 0, 640, 480, colorWhite, cl.media.levelshot);
		CL_DrawPic(0, 0, 640, 480, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0, 0, 640, 160, colorWhite, cl.media.loadingLogo);

		Q_snprintfz(str, sizeof(str), "Awaiting frame...");
		CL_DrawString(0, 424, 16, 16, 0, 0, 640, str, colorWhite, cls.media.charsetShader, true, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);

		break;
	case CA_ACTIVE:
		// Still active, but local server is changing levels
		CL_FillRect(0, 0, 640, 480, colorBlack);

		break;
	}
}

/*
 =================
 CL_LoadClientInfo
 =================
*/
void CL_LoadClientInfo (clientInfo_t *ci, const char *string){

	char	model[MAX_QPATH], skin[MAX_QPATH], name[MAX_QPATH];
	char	checkMD3[MAX_QPATH], checkMD2[MAX_QPATH];
	char	checkTGA[MAX_QPATH], checkJPG[MAX_QPATH], checkPCX[MAX_QPATH];
	char	*ch;
	int		i;

	memset(ci, 0, sizeof(clientInfo_t));

	// Isolate the player name
	Q_strncpyz(ci->name, string, sizeof(ci->name));
	ch = strchr(string, '\\');
	if (ch){
		ci->name[ch-string] = 0;
		string = ch+1;
	}

	if (cl_noSkins->integer || *string == 0){
		// No custom skins or bad info string, so just use male/grunt
		ci->model = R_RegisterModel("players/male/tris.md2");
		ci->skin = R_RegisterShaderSkin("players/male/grunt");
		ci->icon = R_RegisterShaderNoMip("players/male/grunt_i");
		ci->weaponModel[0] = R_RegisterModel("players/male/weapon.md2");

		// Save model/skin in the info string
		Q_snprintfz(ci->info, sizeof(ci->info), "male/grunt");

		ci->valid = true;
		return;
	}

	// Isolate model and skin name
	Q_strncpyz(model, string, sizeof(model));
	ch = strchr(model, '/');
	if (!ch)
		ch = strchr(model, '\\');
	if (ch){
		*ch++ = 0;
		Q_strncpyz(skin, ch, sizeof(skin));
	}
	else
		skin[0] = 0;

	// If the model doesn't exist, default to male
	Q_snprintfz(checkMD3, sizeof(checkMD3), "players/%s/tris.md3", model);
	Q_snprintfz(checkMD2, sizeof(checkMD2), "players/%s/tris.md2", model);
	if (FS_LoadFile(checkMD3, NULL) == -1 && FS_LoadFile(checkMD2, NULL) == -1)
		Q_strncpyz(model, "male", sizeof(model));

	// If the skin doesn't exist, default to male/grunt
	Q_snprintfz(checkTGA, sizeof(checkTGA), "players/%s/%s.tga", model, skin);
	Q_snprintfz(checkJPG, sizeof(checkJPG), "players/%s/%s.jpg", model, skin);
	Q_snprintfz(checkPCX, sizeof(checkPCX), "players/%s/%s.pcx", model, skin);
	if (FS_LoadFile(checkTGA, NULL) == -1 && FS_LoadFile(checkJPG, NULL) == -1 && FS_LoadFile(checkPCX, NULL) == -1){
		Q_strncpyz(model, "male", sizeof(model));
		Q_strncpyz(skin, "grunt", sizeof(skin));
	}

	// If the icon doesn't exist, default to male/grunt
	Q_snprintfz(checkTGA, sizeof(checkTGA), "players/%s/%s_i.tga", model, skin);
	Q_snprintfz(checkJPG, sizeof(checkJPG), "players/%s/%s_i.jpg", model, skin);
	Q_snprintfz(checkPCX, sizeof(checkPCX), "players/%s/%s_i.pcx", model, skin);
	if (FS_LoadFile(checkTGA, NULL) == -1 && FS_LoadFile(checkJPG, NULL) == -1 && FS_LoadFile(checkPCX, NULL) == -1){
		Q_strncpyz(model, "male", sizeof(model));
		Q_strncpyz(skin, "grunt", sizeof(skin));
	}

	// If a weapon model doesn't exist, default to male/grunt
	for (i = 0; i < cl.numWeaponModels; i++){
		Q_snprintfz(checkMD3, sizeof(checkMD3), "players/%s/%s.md3", model, cl.weaponModels[i]);
		Q_snprintfz(checkMD2, sizeof(checkMD2), "players/%s/%s.md2", model, cl.weaponModels[i]);
		if (FS_LoadFile(checkMD3, NULL) == -1 && FS_LoadFile(checkMD2, NULL) == -1){
			Q_strncpyz(model, "male", sizeof(model));
			Q_strncpyz(skin, "grunt", sizeof(skin));
			break;
		}

		if (!cl_visibleWeapons->integer)
			break;		// Only one if no visible weapons
	}

	// We can now load everything
	Q_snprintfz(name, sizeof(name), "players/%s/tris.md2", model);
	ci->model = R_RegisterModel(name);

	Q_snprintfz(name, sizeof(name), "players/%s/%s", model, skin);
	ci->skin = R_RegisterShaderSkin(name);

	Q_snprintfz(name, sizeof(name), "players/%s/%s_i", model, skin);
	ci->icon = R_RegisterShaderNoMip(name);

	for (i = 0; i < cl.numWeaponModels; i++){
		Q_snprintfz(name, sizeof(name), "players/%s/%s.md2", model, cl.weaponModels[i]);
		ci->weaponModel[i] = R_RegisterModel(name);

		if (!cl_visibleWeapons->integer)
			break;		// Only one if no visible weapons
	}

	// Save model/skin in the info string
	Q_snprintfz(ci->info, sizeof(ci->info), "%s/%s", model, skin);

	ci->valid = true;
}

/*
 =================
 CL_LoadGameMedia
 =================
*/
void CL_LoadGameMedia (void){

	char	checkTGA[MAX_QPATH], checkJPG[MAX_QPATH];
	char	levelshot[MAX_QPATH];
	int		i, n;
	int		time;

	time = Sys_Milliseconds();

	// Need to precache files
	cls.state = CA_LOADING;

	// Clear all local effects (except for light styles) because they
	// now point to invalid files
	CL_ClearTempEntities();
	CL_ClearLocalEntities();
	CL_ClearDynamicLights();
	CL_ClearMarks();
	CL_ClearParticles();

	// Get map name
	Com_StripExtension(cl.configStrings[CS_MODELS+1]+5, cls.loadingInfo.map, sizeof(cls.loadingInfo.map));
	Q_strncpyz(cls.loadingInfo.name, cl.configStrings[CS_NAME], sizeof(cls.loadingInfo.name));

	// Check if a levelshot for this map exists
	Q_snprintfz(checkTGA, sizeof(checkTGA), "ui/levelshots/%s.tga", cls.loadingInfo.map);
	Q_snprintfz(checkJPG, sizeof(checkJPG), "ui/levelshots/%s.jpg", cls.loadingInfo.map);
	if (FS_LoadFile(checkTGA, NULL) == -1 && FS_LoadFile(checkJPG, NULL) == -1)
		Q_snprintfz(levelshot, sizeof(levelshot), "ui/levelshots/unknownmap");
	else
		Q_snprintfz(levelshot, sizeof(levelshot), "ui/levelshots/%s", cls.loadingInfo.map);

	// Load a few needed shaders for the loading screen
	cl.media.levelshot = R_RegisterShaderNoMip(levelshot);
	cl.media.levelshotDetail = R_RegisterShaderNoMip("ui/loading/loading_detail");
	cl.media.loadingLogo = R_RegisterShaderNoMip("ui/title_screen/q2e_logo");
	cl.media.loadingDetail[0] = R_RegisterShaderNoMip("ui/loading/load_main2");
	cl.media.loadingDetail[1] = R_RegisterShaderNoMip("ui/loading/load_main");

	for (i = 0, n = 5; i < 20; i++, n += 5)
		cl.media.loadingPercent[i] = R_RegisterShaderNoMip(va("ui/loading/percent/load_%i", n));

	// Register all the files for this level
	CL_RegisterCollisionMap();
	CL_RegisterSounds();
	CL_RegisterGraphics();
	CL_RegisterClients();

	// Start the background track
	CL_PlayBackgroundTrack();

	// All precaches are now complete
	cls.state = CA_PRIMED;

	Com_Printf("CL_LoadGameMedia: %.2f seconds\n", (float)(Sys_Milliseconds() - time) / 1000.0);

	CL_UpdateLoading("");

	// Touch all the memory used for this level
	Com_TouchMemory();

	// Force menu and console off
	UI_SetActiveMenu(UI_CLOSEMENU);
	Con_CloseConsole();
}

/*
 =================
 CL_LoadLocalMedia
 =================
*/
void CL_LoadLocalMedia (void){

	// Get GL/AL information
	R_GetGLConfig(&cls.glConfig);
	S_GetALConfig(&cls.alConfig);

	// Set screen scales
	cls.screenScaleX = cls.glConfig.videoWidth / 640.0;
	cls.screenScaleY = cls.glConfig.videoHeight / 480.0;

	// Load a few needed shaders
	cls.media.whiteShader = R_RegisterShaderNoMip("white");
	cls.media.consoleShader = R_RegisterShaderNoMip("console");
	cls.media.charsetShader = R_RegisterShaderNoMip("charset");

	// Precache UI files if requested
	UI_Precache();
}
