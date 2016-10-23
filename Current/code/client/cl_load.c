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

	Sys_GetEvents();
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
	char	skyName[MAX_OSPATH];
	float	skyRotate;
	vec3_t	skyAxis;
	char	name[MAX_OSPATH];

	// Load the map
	CL_UpdateLoading("MAP");

	Q_snprintfz(skyName, sizeof(skyName), "env/%s",cl.configStrings[CS_SKY]);
	skyRotate = atof(cl.configStrings[CS_SKYROTATE]);
#ifdef SECURE
	sscanf_s(cl.configStrings[CS_SKYAXIS], "%f %f %f", &skyAxis[0], &skyAxis[1], &skyAxis[2]);
#else
	sscanf(cl.configStrings[CS_SKYAXIS], "%f %f %f", &skyAxis[0], &skyAxis[1], &skyAxis[2]);
#endif

	R_LoadWorldMap(cl.configStrings[CS_MODELS+1], skyName, skyRotate, skyAxis);

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
			Com_FileExtension(cl.configStrings[CS_MODELS+i], name, sizeof(name));
			if (!Q_stricmp(name, ".sp2")){
				cl.media.gameModels[i] = NULL;
				continue;
			}

			cl.media.gameModels[i] = R_RegisterModel(cl.configStrings[CS_MODELS+i]);

			if (cl.configStrings[CS_MODELS+i][0] == '*')
				cl.media.gameCModels[i] = CM_InlineModel(cl.configStrings[CS_MODELS+i]);
			else
				cl.media.gameCModels[i] = NULL;
		}
	}

	// Register materials
	CL_UpdateLoading("MATERIALS");

	cl.media.lagometerMaterial = R_RegisterMaterialNoMip("lagometer");
	cl.media.disconnectedMaterial = R_RegisterMaterialNoMip("disconnected");
	cl.media.pauseMaterial = R_RegisterMaterialNoMip("pause");

	for (i = 0; i < NUM_CROSSHAIRS; i++)
		cl.media.crosshairMaterials[i] = R_RegisterMaterialNoMip(va("pics/crosshair%i", i+1));

	for (i = 0; i < 11; i++){
		if (i != 10){
			cl.media.hudNumberMaterials[0][i] = R_RegisterMaterialNoMip(va("pics/num_%i", i));
			cl.media.hudNumberMaterials[1][i] = R_RegisterMaterialNoMip(va("pics/anum_%i", i));
		}
		else {
			cl.media.hudNumberMaterials[0][i] = R_RegisterMaterialNoMip("pics/num_minus");
			cl.media.hudNumberMaterials[1][i] = R_RegisterMaterialNoMip("pics/anum_minus");
		}
	}

	cl.media.fireScreenMaterial = R_RegisterMaterialNoMip("fireScreen");
	cl.media.waterBlurMaterial = R_RegisterMaterialNoMip("waterBlur");
	cl.media.doubleVisionMaterial = R_RegisterMaterialNoMip("doubleVision");
	cl.media.underWaterVisionMaterial = R_RegisterMaterialNoMip("underWaterVision");
	cl.media.irGogglesMaterial = R_RegisterMaterialNoMip("irGoggles");

	cl.media.rocketExplosionMaterial = R_RegisterMaterial("rocketExplosion", false);
	cl.media.rocketExplosionWaterMaterial = R_RegisterMaterial("rocketExplosionWater", false);
	cl.media.grenadeExplosionMaterial = R_RegisterMaterial("grenadeExplosion", false);
	cl.media.grenadeExplosionWaterMaterial = R_RegisterMaterial("grenadeExplosionWater", false);
	cl.media.bfgExplosionMaterial = R_RegisterMaterial("bfgExplosion", false);
	cl.media.bfgBallMaterial = R_RegisterMaterial("bfgBall", false);
	cl.media.plasmaBallMaterial = R_RegisterMaterial("plasmaBall", false);
	cl.media.waterPlumeMaterial = R_RegisterMaterial("waterPlume", false);
	cl.media.waterSprayMaterial = R_RegisterMaterial("waterSpray", false);
	cl.media.waterWakeMaterial = R_RegisterMaterial("waterWake", false);
	cl.media.nukeShockwaveMaterial = R_RegisterMaterial("nukeShockwave", false);
	cl.media.bloodBlendMaterial = R_RegisterMaterial("bloodBlend", false);
	cl.media.bloodSplatMaterial[0] = R_RegisterMaterial("bloodSplat", false);
	cl.media.bloodSplatMaterial[1] = R_RegisterMaterial("greenBloodSplat", false);
	cl.media.bloodCloudMaterial[0] = R_RegisterMaterial("bloodCloud", false);
	cl.media.bloodCloudMaterial[1] = R_RegisterMaterial("greenBloodCloud", false);

	cl.media.powerScreenShellMaterial = R_RegisterMaterial("shells/powerScreen", false);
	cl.media.invulnerabilityShellMaterial = R_RegisterMaterial("shells/invulnerability", false);
	cl.media.quadDamageShellMaterial = R_RegisterMaterial("shells/quadDamage", false);
	cl.media.doubleDamageShellMaterial = R_RegisterMaterial("shells/doubleDamage", false);
	cl.media.halfDamageShellMaterial = R_RegisterMaterial("shells/halfDamage", false);
	cl.media.genericShellMaterial = R_RegisterMaterial("shells/generic", false);

	cl.media.laserBeamMaterial = R_RegisterMaterial("beams/laser", false);
	cl.media.grappleBeamMaterial = R_RegisterMaterial("beams/grapple", false);
	cl.media.lightningBeamMaterial = R_RegisterMaterial("beams/lightning", false);
	cl.media.heatBeamMaterial = R_RegisterMaterial("beams/heat", false);

	cl.media.energyParticleMaterial = R_RegisterMaterial("particles/energy", false);
	cl.media.glowParticleMaterial = R_RegisterMaterial("particles/glow", false);
	cl.media.flameParticleMaterial = R_RegisterMaterial("particles/flame", false);
	cl.media.smokeParticleMaterial = R_RegisterMaterial("particles/smoke", false);
	cl.media.liteSmokeParticleMaterial = R_RegisterMaterial("particles/liteSmoke", false);
	cl.media.bubbleParticleMaterial = R_RegisterMaterial("particles/bubble", false);
	cl.media.dropletParticleMaterial = R_RegisterMaterial("particles/droplet", false);
	cl.media.steamParticleMaterial = R_RegisterMaterial("particles/steam", false);
	cl.media.sparkParticleMaterial = R_RegisterMaterial("particles/spark", false);
	cl.media.impactSparkParticleMaterial = R_RegisterMaterial("particles/impactSpark", false);
	cl.media.trackerParticleMaterial = R_RegisterMaterial("particles/tracker", false);
	cl.media.flyParticleMaterial = R_RegisterMaterial("particles/fly", false);

	cl.media.energyMarkMaterial = R_RegisterMaterial("decals/energyMark", false);
	cl.media.bulletMarkMaterial = R_RegisterMaterial("decals/bulletMark", false);
	cl.media.burnMarkMaterial = R_RegisterMaterial("decals/burnMark", false);
	cl.media.bloodMarkMaterials[0][0] = R_RegisterMaterial("decals/bloodMark1", false);
	cl.media.bloodMarkMaterials[0][1] = R_RegisterMaterial("decals/bloodMark2", false);
	cl.media.bloodMarkMaterials[0][2] = R_RegisterMaterial("decals/bloodMark3", false);
	cl.media.bloodMarkMaterials[0][3] = R_RegisterMaterial("decals/bloodMark4", false);
	cl.media.bloodMarkMaterials[0][4] = R_RegisterMaterial("decals/bloodMark5", false);
	cl.media.bloodMarkMaterials[0][5] = R_RegisterMaterial("decals/bloodMark6", false);
	cl.media.bloodMarkMaterials[1][0] = R_RegisterMaterial("decals/greenBloodMark1", false);
	cl.media.bloodMarkMaterials[1][1] = R_RegisterMaterial("decals/greenBloodMark2", false);
	cl.media.bloodMarkMaterials[1][2] = R_RegisterMaterial("decals/greenBloodMark3", false);
	cl.media.bloodMarkMaterials[1][3] = R_RegisterMaterial("decals/greenBloodMark4", false);
	cl.media.bloodMarkMaterials[1][4] = R_RegisterMaterial("decals/greenBloodMark5", false);
	cl.media.bloodMarkMaterials[1][5] = R_RegisterMaterial("decals/greenBloodMark6", false);

	CL_LoadHUD();

	R_RegisterMaterialNoMip("pics/w_machinegun");
	R_RegisterMaterialNoMip("pics/a_bullets");
	R_RegisterMaterialNoMip("pics/i_health");
	R_RegisterMaterialNoMip("pics/a_grenades");

	// Register the materials that the server references
	CL_UpdateLoading("GAME MATERIALS");

	for (i = 1; i < MAX_IMAGES; i++){
		if (!cl.configStrings[CS_IMAGES+i][0])
			break;

		if (!strchr(cl.configStrings[CS_IMAGES+i], '/'))
			Q_snprintfz(name, sizeof(name), "pics/%s", cl.configStrings[CS_IMAGES+i]);
		else
			Com_StripExtension(cl.configStrings[CS_IMAGES+i], name, sizeof(name));

		cl.media.gameMaterials[i] = R_RegisterMaterialNoMip(name);
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

	// If playing a cinematic, stop it
	CL_StopCinematic();

	// Make sure CD audio and sounds aren't playing
	CDAudio_Stop();
	S_StopAllSounds(false);

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

	switch (cls.state){
	case CA_CONNECTING:
		// Awaiting connection
		CL_DrawPicByName(0, 0, 640, 480, colorTable[COLOR_WHITE], "ui/title_screen/title_backg");
		CL_DrawPicByName(0, 0, 640, 160, colorTable[COLOR_WHITE], "ui/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			Q_snprintfz(str, sizeof(str), "Starting up...");
			CL_DrawString(0, 424, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			if (cls.serverMessage[0])
				CL_DrawString(0, 360, 16, 16, 640, cls.serverMessage, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);

			Q_snprintfz(str, sizeof(str), "Connecting to %s\nAwaiting connection... %i", cls.serverName, cls.connectCount);
			CL_DrawString(0, 408, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		break;
	case CA_CHALLENGING:
		// Awaiting challenge
		CL_DrawPicByName(0, 0, 640, 480, colorTable[COLOR_WHITE], "ui/title_screen/title_backg");
		CL_DrawPicByName(0, 0, 640, 160, colorTable[COLOR_WHITE], "ui/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			Q_snprintfz(str, sizeof(str), "Starting up...");
			CL_DrawString(0, 424, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			if (cls.serverMessage[0])
				CL_DrawString(0, 360, 16, 16, 640, cls.serverMessage, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);

			Q_snprintfz(str, sizeof(str), "Connecting to %s\nAwaiting challenge... %i", cls.serverName, cls.connectCount);
			CL_DrawString(0, 408, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		break;
	case CA_CONNECTED:
		if (cls.downloadFile){
			// Downloading file from server
			CL_DrawPicByName(0, 0, 640, 480, colorTable[COLOR_WHITE], "ui/title_screen/title_backg");
			CL_DrawPicByName(0, 0, 640, 160, colorTable[COLOR_WHITE], "ui/title_screen/q2e_logo");

			if (cls.downloadStart != cls.realTime)
				speed = (float)(cls.downloadBytes / 1024) / ((cls.realTime - cls.downloadStart) / 1000);
			else
				speed = 0;

			if (Com_ServerState()){
				Q_snprintfz(str, sizeof(str), "Downloading %s... (%i%% @ %.2f KB/sec)", cls.downloadName, cls.downloadPercent, speed);
				CL_DrawString(0, 424, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
			}
			else {
				Q_snprintfz(str, sizeof(str), "Connecting to %s\nDownloading %s... (%i%% @ %.2f KB/sec)", cls.serverName, cls.downloadName, cls.downloadPercent, speed);
				CL_DrawString(0, 408, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
			}

			percent = Clamp(cls.downloadPercent - (cls.downloadPercent % 5), 5, 100);
			if (percent){
				CL_DrawPicByName(240, 160, 160, 160, colorTable[COLOR_WHITE], "ui/loading/load_main2");
				CL_DrawPicByName(240, 160, 160, 160, colorTable[COLOR_WHITE], va("ui/loading/percent/load_%i", percent));
				CL_DrawPicByName(240, 160, 160, 160, colorTable[COLOR_WHITE], "ui/loading/load_main");
			}

			break;
		}

		// Awaiting game state
		CL_DrawPicByName(0, 0, 640, 480, colorTable[COLOR_WHITE], "ui/title_screen/title_backg");
		CL_DrawPicByName(0, 0, 640, 160, colorTable[COLOR_WHITE], "ui/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			Q_snprintfz(str, sizeof(str), "Starting up...");
			CL_DrawString(0, 424, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			Q_snprintfz(str, sizeof(str), "Connecting to %s\nAwaiting game state...", cls.serverName);
			CL_DrawString(0, 408, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		break;
	case CA_LOADING:
		// Loading level data
		CL_DrawPic(0, 0, 640, 480, colorTable[COLOR_WHITE], cl.media.levelshot);
		CL_DrawPic(0, 0, 640, 480, colorTable[COLOR_WHITE], cl.media.levelshotDetail);
		CL_DrawPic(0, 0, 640, 160, colorTable[COLOR_WHITE], cl.media.loadingLogo);

		if (NET_IsLocalAddress(cls.serverAddress)){
			Q_snprintfz(str, sizeof(str), "Loading %s\n\"%s\"\n\n\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.loadingInfo.string);
			CL_DrawString(0, 360, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}
		else {
			Q_snprintfz(str, sizeof(str), "Loading %s\n\"%s\"\n\nConnecting to %s\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.serverName, cls.loadingInfo.string);
			CL_DrawString(0, 360, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);
		}

		percent = Clamp((cls.loadingInfo.percent / 5) - 1, 0, 19);
		if (percent){
			CL_DrawPic(240, 160, 160, 160, colorTable[COLOR_WHITE], cl.media.loadingDetail[0]);
			CL_DrawPic(240, 160, 160, 160, colorTable[COLOR_WHITE], cl.media.loadingPercent[percent]);
			CL_DrawPic(240, 160, 160, 160, colorTable[COLOR_WHITE], cl.media.loadingDetail[1]);
		}

		break;
	case CA_PRIMED:
		// Awaiting frame
		CL_DrawPic(0, 0, 640, 480, colorTable[COLOR_WHITE], cl.media.levelshot);
		CL_DrawPic(0, 0, 640, 480, colorTable[COLOR_WHITE], cl.media.levelshotDetail);
		CL_DrawPic(0, 0, 640, 160, colorTable[COLOR_WHITE], cl.media.loadingLogo);

		Q_snprintfz(str, sizeof(str), "Awaiting frame...");
		CL_DrawString(0, 424, 16, 16, 640, str, colorTable[COLOR_WHITE], cls.media.charsetMaterial, DSF_FORCECOLOR|DSF_DROPSHADOW|DSF_CENTER|DSF_UPPERCASE);

		break;
	}
}

/*
 =================
 CL_LoadClientInfo
 =================
*/
void CL_LoadClientInfo (clientInfo_t *ci, const char *string){

	char	model[MAX_OSPATH], skin[MAX_OSPATH], name[MAX_OSPATH];
	char	checkMD3[MAX_OSPATH], checkMD2[MAX_OSPATH];
	char	checkTGA[MAX_OSPATH], checkPCX[MAX_OSPATH];
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

	if (cl_noSkins->integerValue || *string == 0){
		// No custom skins or bad info string, so just use male/grunt
		ci->model = R_RegisterModel("players/male/tris.md2");
		ci->skin = R_RegisterMaterial("players/male/grunt", true);
		ci->icon = R_RegisterMaterialNoMip("players/male/grunt_i");
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
	if (!FS_FileExists(checkMD3) && !FS_FileExists(checkMD2))
		Q_strncpyz(model, "male", sizeof(model));

	// If the skin doesn't exist, default to male/grunt
	Q_snprintfz(checkTGA, sizeof(checkTGA), "players/%s/%s.tga", model, skin);
	Q_snprintfz(checkPCX, sizeof(checkPCX), "players/%s/%s.pcx", model, skin);
	if (!FS_FileExists(checkTGA) && !FS_FileExists(checkPCX)){
		Q_strncpyz(model, "male", sizeof(model));
		Q_strncpyz(skin, "grunt", sizeof(skin));
	}

	// If the icon doesn't exist, default to male/grunt
	Q_snprintfz(checkTGA, sizeof(checkTGA), "players/%s/%s_i.tga", model, skin);
	Q_snprintfz(checkPCX, sizeof(checkPCX), "players/%s/%s_i.pcx", model, skin);
	if (!FS_FileExists(checkTGA) && !FS_FileExists(checkPCX)){
		Q_strncpyz(model, "male", sizeof(model));
		Q_strncpyz(skin, "grunt", sizeof(skin));
	}

	// If a weapon model doesn't exist, default to male/grunt
	for (i = 0; i < cl.numWeaponModels; i++){
		Q_snprintfz(checkMD3, sizeof(checkMD3), "players/%s/%s.md3", model, cl.weaponModels[i]);
		Q_snprintfz(checkMD2, sizeof(checkMD2), "players/%s/%s.md2", model, cl.weaponModels[i]);
		if (!FS_FileExists(checkMD3) && !FS_FileExists(checkMD2)){
			Q_strncpyz(model, "male", sizeof(model));
			Q_strncpyz(skin, "grunt", sizeof(skin));
			break;
		}
	}

	// We can now load everything
	Q_snprintfz(name, sizeof(name), "players/%s/tris.md2", model);
	ci->model = R_RegisterModel(name);

	Q_snprintfz(name, sizeof(name), "players/%s/%s", model, skin);
	ci->skin = R_RegisterMaterial(name, true);

	Q_snprintfz(name, sizeof(name), "players/%s/%s_i", model, skin);
	ci->icon = R_RegisterMaterialNoMip(name);

	for (i = 0; i < cl.numWeaponModels; i++){
		Q_snprintfz(name, sizeof(name), "players/%s/%s.md2", model, cl.weaponModels[i]);
		ci->weaponModel[i] = R_RegisterModel(name);
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

	char	levelshot[MAX_OSPATH];
	int		i, n;
	int		time;

	time = Sys_Milliseconds();

	// Need to precache files
	cls.state = CA_LOADING;

	// Clear local effects because they now point to invalid files
	CL_ClearTempEntities();
	CL_ClearLocalEntities();
	CL_ClearDynamicLights();
	CL_ClearParticles();
	CL_ClearBloodBlends();

	// Clear test model
	cl.testGun = false;
	cl.testModel = false;

	// Clear test post-process
	cl.testPostProcess = false;

	// Get map name
	Com_StripExtension(cl.configStrings[CS_MODELS+1]+5, cls.loadingInfo.map, sizeof(cls.loadingInfo.map));
	Q_strncpyz(cls.loadingInfo.name, cl.configStrings[CS_NAME], sizeof(cls.loadingInfo.name));

	// Check if a levelshot for this map exists
	Q_snprintfz(levelshot, sizeof(levelshot), "ui/levelshots/%s.tga", cls.loadingInfo.map);
	if (FS_FileExists(levelshot))
		Q_snprintfz(levelshot, sizeof(levelshot), "ui/levelshots/%s", cls.loadingInfo.map);
	else
		Q_snprintfz(levelshot, sizeof(levelshot), "ui/levelshots/unknownmap");

	// Load a few needed materials for the loading screen
	cl.media.levelshot = R_RegisterMaterialNoMip(levelshot);
	cl.media.levelshotDetail = R_RegisterMaterialNoMip("ui/loading/loading_detail");
	cl.media.loadingLogo = R_RegisterMaterialNoMip("ui/title_screen/q2e_logo");
	cl.media.loadingDetail[0] = R_RegisterMaterialNoMip("ui/loading/load_main2");
	cl.media.loadingDetail[1] = R_RegisterMaterialNoMip("ui/loading/load_main");

	for (i = 0, n = 5; i < 20; i++, n += 5)
		cl.media.loadingPercent[i] = R_RegisterMaterialNoMip(va("ui/loading/percent/load_%i", n));

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

	// Load a few needed materials
	cls.media.cinematicMaterial = R_RegisterMaterialNoMip("cinematic");
	cls.media.whiteMaterial = R_RegisterMaterialNoMip("white");
	cls.media.consoleMaterial = R_RegisterMaterialNoMip("console");
	cls.media.charsetMaterial = R_RegisterMaterialNoMip("charset");

	// Precache UI files if requested
	UI_Precache();
}
