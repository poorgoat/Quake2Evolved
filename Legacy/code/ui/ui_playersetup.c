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


#include "ui_local.h"


#define ART_BACKGROUND			"ui/misc/ui_sub_options"
#define ART_BANNER				"ui/banners/playersetup_t"
#define ART_TEXT1				"ui/text/playersetup_text_p1"
#define ART_TEXT2				"ui/text/playersetup_text_p2"
#define ART_PLAYERVIEW			"ui/segments/player_view"
#define ART_VIEWRAILCORE		"ui/misc/rail_type1_1"
#define ART_VIEWRAILSPIRAL		"ui/misc/rail_type1_2"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_TEXT1			2
#define ID_TEXT2			3
#define ID_TEXTSHADOW1		4
#define ID_TEXTSHADOW2		5

#define ID_BACK				6

#define ID_VIEW				7
#define ID_NAME				8
#define ID_MODEL			9
#define ID_SKIN				10
#define ID_RAILTRAIL		12
#define ID_VIEWRAILCORE		13
#define ID_VIEWRAILSPIRAL	14
#define ID_RAILCORECOLOR	15
#define ID_RAILSPIRALCOLOR	16

#define MAX_PLAYERMODELS	256
#define MAX_PLAYERSKINS		2048

typedef struct {
	char					playerModels[MAX_PLAYERMODELS][MAX_QPATH];
	int						numPlayerModels;
	char					playerSkins[MAX_PLAYERSKINS][MAX_QPATH];
	int						numPlayerSkins;
	char					currentModel[MAX_QPATH];
	char					currentSkin[MAX_QPATH];
	
	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			textShadow1;
	menuBitmap_s			textShadow2;
	menuBitmap_s			text1;
	menuBitmap_s			text2;

	menuBitmap_s			back;

	menuBitmap_s			view;

	menuField_s				name;
	menuSpinControl_s		model;
	menuSpinControl_s		skin;
	menuSpinControl_s		railTrail;
	menuBitmap_s			viewRailCore;
	menuBitmap_s			viewRailSpiral;
	menuSpinControl_s		railCoreColor;
	menuSpinControl_s		railSpiralColor;
} uiPlayerSetup_t;

static uiPlayerSetup_t		uiPlayerSetup;


/*
 =================
 UI_PlayerSetup_CalcFov

 I need this here...
 =================
*/
float UI_PlayerSetup_CalcFov (float fovX, float width, float height){

	float	x, y;

	if (fovX < 1)
		fovX = 1;
	else if (fovX > 179)
		fovX = 179;

	x = width / tan(fovX / 360 * M_PI);
	y = atan(height / x) * 360 / M_PI;

	return y;
}

/*
 =================
 UI_PlayerSetup_FindModels
 =================
*/
static void UI_PlayerSetup_FindModels (void){

	char	pathMD2[MAX_QPATH], pathMD3[MAX_QPATH];
	char	buffer[0x8000], *ptr;
	int		num, len;
	int		i;
	char	list[MAX_PLAYERMODELS][MAX_QPATH];
	int		count = 0;

	uiPlayerSetup.numPlayerModels = 0;

	// Get file list
	num = FS_GetFileList("players", NULL, buffer, sizeof(buffer));
	ptr = buffer;
	for (i = 0; i < num; i++, ptr += len){
		len = strlen(ptr) + 1;

		if (strrchr(ptr, '.'))
			continue;

		if (count == MAX_PLAYERMODELS)
			break;

		Q_strncpyz(list[count], ptr, sizeof(list[count]));
		count++;
	}

	// Build the model list
	for (i = 0; i < count; i++){
		// Make sure a model exists
		Q_snprintfz(pathMD2, sizeof(pathMD2), "players/%s/tris.md2", list[i]);
		Q_snprintfz(pathMD3, sizeof(pathMD3), "players/%s/tris.md3", list[i]);
		if (FS_LoadFile(pathMD2, NULL) == -1 && FS_LoadFile(pathMD3, NULL) == -1)
			continue;

		Q_strncpyz(uiPlayerSetup.playerModels[uiPlayerSetup.numPlayerModels], list[i], sizeof(uiPlayerSetup.playerModels[uiPlayerSetup.numPlayerModels]));
		uiPlayerSetup.numPlayerModels++;
	}
}

/*
 =================
 UI_PlayerSetup_FindSkins
 =================
*/
static void UI_PlayerSetup_FindSkins (const char *model){

	char	pathPCX[MAX_QPATH], pathJPG[MAX_QPATH], pathTGA[MAX_QPATH];
	char	buffer[0x8000], *ptr;
	int		num, len;
	int		i;
	char	list[MAX_PLAYERSKINS][MAX_QPATH];
	int		count = 0;

	uiPlayerSetup.numPlayerSkins = 0;

	// Get file list
	num = FS_GetFileList(va("players/%s", model), NULL, buffer, sizeof(buffer));
	ptr = buffer;
	for (i = 0; i < num; i++, ptr += len){
		len = strlen(ptr) + 1;

		if (!strrchr(ptr, '.'))
			continue;

		if (strstr(ptr, "_i."))
			continue;

		if (!strstr(ptr, ".pcx") && !strstr(ptr, ".jpg") && !strstr(ptr, ".tga"))
			continue;

		if (count == MAX_PLAYERSKINS)
			break;

		Com_StripExtension(ptr, list[count], sizeof(list[count]));
		count++;
	}

	// Build the skin list
	for (i = 0; i < count; i++){
		// Make sure an icon exists
		Q_snprintfz(pathPCX, sizeof(pathPCX), "players/%s/%s_i.pcx", model, list[i]);
		Q_snprintfz(pathJPG, sizeof(pathJPG), "players/%s/%s_i.jpg", model, list[i]);
		Q_snprintfz(pathTGA, sizeof(pathTGA), "players/%s/%s_i.tga", model, list[i]);
		if (FS_LoadFile(pathPCX, NULL) == -1 && FS_LoadFile(pathJPG, NULL) == -1 && FS_LoadFile(pathTGA, NULL) == -1)
			continue;

		Q_strncpyz(uiPlayerSetup.playerSkins[uiPlayerSetup.numPlayerSkins], list[i], sizeof(uiPlayerSetup.playerSkins[uiPlayerSetup.numPlayerSkins]));
		uiPlayerSetup.numPlayerSkins++;
	}
}

/*
 =================
 UI_PlayerSetup_GetConfig
 =================
*/
static void UI_PlayerSetup_GetConfig (void){

	char	model[MAX_QPATH], skin[MAX_QPATH];
	char	*ch;
	int		i;

	Q_strncpyz(uiPlayerSetup.name.buffer, Cvar_VariableString("name"), sizeof(uiPlayerSetup.name.buffer));

	// Get user set skin
	Q_strncpyz(model, Cvar_VariableString("skin"), sizeof(model));
	ch = strchr(model, '/');
	if (!ch)
		ch = strchr(model, '\\');
	if (ch){
		*ch++ = 0;
		Q_strncpyz(skin, ch, sizeof(skin));
	}
	else
		skin[0] = 0;

	// Find models
	UI_PlayerSetup_FindModels();

	// Select current model
	for (i = 0; i < uiPlayerSetup.numPlayerModels; i++){
		if (!Q_stricmp(uiPlayerSetup.playerModels[i], model)){
			uiPlayerSetup.model.curValue = (float)i;
			break;
		}
	}

	Q_strncpyz(uiPlayerSetup.currentModel, uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue], sizeof(uiPlayerSetup.currentModel));
	uiPlayerSetup.model.maxValue = (float)(uiPlayerSetup.numPlayerModels - 1);

	// Find skins for the selected model
	UI_PlayerSetup_FindSkins(uiPlayerSetup.currentModel);

	// Select current skin
	for (i = 0; i < uiPlayerSetup.numPlayerSkins; i++){
		if (!Q_stricmp(uiPlayerSetup.playerSkins[i], skin)){
			uiPlayerSetup.skin.curValue = (float)i;
			break;
		}
	}

	Q_strncpyz(uiPlayerSetup.currentSkin, uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue], sizeof(uiPlayerSetup.currentSkin));
	uiPlayerSetup.skin.maxValue = (float)(uiPlayerSetup.numPlayerSkins - 1);
}

/*
 =================
 UI_PlayerSetup_SetConfig
 =================
*/
static void UI_PlayerSetup_SetConfig (void){

	Cvar_Set("name", uiPlayerSetup.name.buffer);

	Cvar_Set("skin", va("%s/%s", uiPlayerSetup.currentModel, uiPlayerSetup.currentSkin));
}

/*
 =================
 UI_PlayerSetup_UpdateConfig
 =================
*/
static void UI_PlayerSetup_UpdateConfig (void){

	// See if the model has changed
	if (Q_stricmp(uiPlayerSetup.currentModel, uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue])){
		char	*skin, *ch;
		int		i;

		Q_strncpyz(uiPlayerSetup.currentModel, uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue], sizeof(uiPlayerSetup.currentModel));

		// Find skins for the selected model
		UI_PlayerSetup_FindSkins(uiPlayerSetup.currentModel);

		// Select current skin
		skin = Cvar_VariableString("skin");
		ch = strchr(skin, '/');
		if (!ch)
			ch = strchr(skin, '\\');
		if (ch)
			skin = ch + 1;

		for (i = 0; i < uiPlayerSetup.numPlayerSkins; i++){
			if (!Q_stricmp(uiPlayerSetup.playerSkins[i], skin)){
				uiPlayerSetup.skin.curValue = (float)i;
				break;
			}
		}

		if (i == uiPlayerSetup.numPlayerSkins)	// Couldn't find, so select first
			uiPlayerSetup.skin.curValue = 0;

		Q_strncpyz(uiPlayerSetup.currentSkin, uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue], sizeof(uiPlayerSetup.currentSkin));
		uiPlayerSetup.skin.maxValue = (float)(uiPlayerSetup.numPlayerSkins - 1);
	}
	else
		Q_strncpyz(uiPlayerSetup.currentSkin, uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue], sizeof(uiPlayerSetup.currentSkin));

	uiPlayerSetup.model.generic.name = uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue];

	uiPlayerSetup.skin.generic.name = uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue];

	uiPlayerSetup.railTrail.generic.name = "Quake 2";
	uiPlayerSetup.railTrail.generic.flags |= QMF_GRAYED;

	uiPlayerSetup.railCoreColor.generic.name = "White";
	uiPlayerSetup.railCoreColor.generic.flags |= QMF_GRAYED;
	
	uiPlayerSetup.railSpiralColor.generic.name = "Cyan";
	uiPlayerSetup.railSpiralColor.generic.flags |= QMF_GRAYED;
}

/*
 =================
 UI_PlayerSetup_Callback
 =================
*/
static void UI_PlayerSetup_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event == QM_CHANGED){
		UI_PlayerSetup_UpdateConfig();
		UI_PlayerSetup_SetConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

/*
 =================
 UI_PlayerSetup_Ownerdraw
 =================
*/
static void UI_PlayerSetup_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (item->id == ID_VIEW){
		int				x = 630, y = 226, w = 316, h = 316;
		char			path[MAX_QPATH];
		color_t			iconTrans = {255, 255, 255, 127};
		vec3_t			angles;
		entity_t		ent;
		refDef_t		refDef;

		// Draw the background
		Q_snprintfz(path, sizeof(path), "players/%s/%s_i", uiPlayerSetup.currentModel, uiPlayerSetup.currentSkin);
		UI_ScaleCoords(&x, &y, &w, &h);

		UI_DrawPic(x, y, w, h, iconTrans, path);
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, uiPlayerSetup.view.pic);

		R_ClearScene();

		// Draw the player model
		memset(&ent, 0, sizeof(entity_t));
		ent.entityType = ET_MODEL;
		Q_snprintfz(path, sizeof(path), "players/%s/tris.md2", uiPlayerSetup.currentModel);
		ent.model = R_RegisterModel(path);
		Q_snprintfz(path, sizeof(path), "players/%s/%s", uiPlayerSetup.currentModel, uiPlayerSetup.currentSkin);
		ent.customShader = R_RegisterShaderSkin(path);
		MakeRGBA(ent.shaderRGBA, 255, 255, 255, 255);
		VectorSet(ent.origin, 80, 0, 0);
		VectorCopy(ent.origin, ent.oldOrigin);
		VectorSet(angles, 0, (uiStatic.realTime & 4095) * 360 / 4096.0, 0);
		AnglesToAxis(angles, ent.axis);

		R_AddEntityToScene(&ent);

		// Also draw the default weapon model
		memset(&ent, 0, sizeof(entity_t));
		ent.entityType = ET_MODEL;
		Q_snprintfz(path, sizeof(path), "players/%s/weapon.md2", uiPlayerSetup.currentModel);
		ent.model = R_RegisterModel(path);
		Q_snprintfz(path, sizeof(path), "players/%s/weapon", uiPlayerSetup.currentModel);
		ent.customShader = R_RegisterShaderSkin(path);
		MakeRGBA(ent.shaderRGBA, 255, 255, 255, 255);
		VectorSet(ent.origin, 80, 0, 0);
		VectorCopy(ent.origin, ent.oldOrigin);
		VectorSet(angles, 0, (uiStatic.realTime & 4095) * 360 / 4096.0, 0);
		AnglesToAxis(angles, ent.axis);

		R_AddEntityToScene(&ent);

		// Create and render the scene
		memset(&refDef, 0, sizeof(refDef_t));
		refDef.x = item->x + (item->width / 12);
		refDef.y = item->y + (item->height / 12);
		refDef.width = item->width - (item->width / 6);
		refDef.height = item->height - (item->height / 6);
		refDef.fovX = 40;
		refDef.fovY = UI_PlayerSetup_CalcFov(refDef.fovX, refDef.width, refDef.height);
		refDef.rdFlags = RDF_NOWORLDMODEL;
		refDef.time = uiStatic.realTime * 0.001;
		AxisCopy(axisDefault, refDef.viewAxis);

		R_RenderScene(&refDef);
	}
	else {
		if (uiPlayerSetup.menu.items[uiPlayerSetup.menu.cursor] == self)
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
	}
}

/*
 =================
 UI_PlayerSetup_Init
 =================
*/
static void UI_PlayerSetup_Init (void){

	memset(&uiPlayerSetup, 0, sizeof(uiPlayerSetup_t));

	uiPlayerSetup.background.generic.id			= ID_BACKGROUND;
	uiPlayerSetup.background.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.background.generic.flags		= QMF_INACTIVE;
	uiPlayerSetup.background.generic.x			= 0;
	uiPlayerSetup.background.generic.y			= 0;
	uiPlayerSetup.background.generic.width		= 1024;
	uiPlayerSetup.background.generic.height		= 768;
	uiPlayerSetup.background.pic				= ART_BACKGROUND;

	uiPlayerSetup.banner.generic.id				= ID_BANNER;
	uiPlayerSetup.banner.generic.type			= QMTYPE_BITMAP;
	uiPlayerSetup.banner.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.banner.generic.x				= 0;
	uiPlayerSetup.banner.generic.y				= 66;
	uiPlayerSetup.banner.generic.width			= 1024;
	uiPlayerSetup.banner.generic.height			= 46;
	uiPlayerSetup.banner.pic					= ART_BANNER;

	uiPlayerSetup.textShadow1.generic.id		= ID_TEXTSHADOW1;
	uiPlayerSetup.textShadow1.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.textShadow1.generic.flags		= QMF_INACTIVE;
	uiPlayerSetup.textShadow1.generic.x			= 98;
	uiPlayerSetup.textShadow1.generic.y			= 230;
	uiPlayerSetup.textShadow1.generic.width		= 128;
	uiPlayerSetup.textShadow1.generic.height	= 256;
	uiPlayerSetup.textShadow1.generic.color		= uiColorBlack;
	uiPlayerSetup.textShadow1.pic				= ART_TEXT1;

	uiPlayerSetup.textShadow2.generic.id		= ID_TEXTSHADOW2;
	uiPlayerSetup.textShadow2.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.textShadow2.generic.flags		= QMF_INACTIVE;
	uiPlayerSetup.textShadow2.generic.x			= 98;
	uiPlayerSetup.textShadow2.generic.y			= 490;
	uiPlayerSetup.textShadow2.generic.width		= 128;
	uiPlayerSetup.textShadow2.generic.height	= 128;
	uiPlayerSetup.textShadow2.generic.color		= uiColorBlack;
	uiPlayerSetup.textShadow2.pic				= ART_TEXT2;

	uiPlayerSetup.text1.generic.id				= ID_TEXT1;
	uiPlayerSetup.text1.generic.type			= QMTYPE_BITMAP;
	uiPlayerSetup.text1.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.text1.generic.x				= 96;
	uiPlayerSetup.text1.generic.y				= 228;
	uiPlayerSetup.text1.generic.width			= 128;
	uiPlayerSetup.text1.generic.height			= 256;
	uiPlayerSetup.text1.pic						= ART_TEXT1;

	uiPlayerSetup.text2.generic.id				= ID_TEXT2;
	uiPlayerSetup.text2.generic.type			= QMTYPE_BITMAP;
	uiPlayerSetup.text2.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.text2.generic.x				= 96;
	uiPlayerSetup.text2.generic.y				= 488;
	uiPlayerSetup.text2.generic.width			= 128;
	uiPlayerSetup.text2.generic.height			= 128;
	uiPlayerSetup.text2.pic						= ART_TEXT2;

	uiPlayerSetup.back.generic.id				= ID_BACK;
	uiPlayerSetup.back.generic.type				= QMTYPE_BITMAP;
	uiPlayerSetup.back.generic.x				= 413;
	uiPlayerSetup.back.generic.y				= 656;
	uiPlayerSetup.back.generic.width			= 198;
	uiPlayerSetup.back.generic.height			= 38;
	uiPlayerSetup.back.generic.callback			= UI_PlayerSetup_Callback;
	uiPlayerSetup.back.generic.ownerdraw		= UI_PlayerSetup_Ownerdraw;
	uiPlayerSetup.back.pic						= UI_BACKBUTTON;

	uiPlayerSetup.view.generic.id				= ID_VIEW;
	uiPlayerSetup.view.generic.type				= QMTYPE_BITMAP;
	uiPlayerSetup.view.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.view.generic.x				= 628;
	uiPlayerSetup.view.generic.y				= 224;
	uiPlayerSetup.view.generic.width			= 320;
	uiPlayerSetup.view.generic.height			= 320;
	uiPlayerSetup.view.generic.ownerdraw		= UI_PlayerSetup_Ownerdraw;
	uiPlayerSetup.view.pic						= ART_PLAYERVIEW;

	uiPlayerSetup.name.generic.id				= ID_NAME;
	uiPlayerSetup.name.generic.type				= QMTYPE_FIELD;
	uiPlayerSetup.name.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.name.generic.x				= 368;
	uiPlayerSetup.name.generic.y				= 224;
	uiPlayerSetup.name.generic.width			= 198;
	uiPlayerSetup.name.generic.height			= 30;
	uiPlayerSetup.name.generic.callback			= UI_PlayerSetup_Callback;
	uiPlayerSetup.name.generic.statusText		= "Enter your multiplayer display name";
	uiPlayerSetup.name.maxLenght				= 32;

	uiPlayerSetup.model.generic.id				= ID_MODEL;
	uiPlayerSetup.model.generic.type			= QMTYPE_SPINCONTROL;
	uiPlayerSetup.model.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.model.generic.x				= 368;
	uiPlayerSetup.model.generic.y				= 256;
	uiPlayerSetup.model.generic.width			= 198;
	uiPlayerSetup.model.generic.height			= 30;
	uiPlayerSetup.model.generic.callback		= UI_PlayerSetup_Callback;
	uiPlayerSetup.model.generic.statusText		= "Select a model for representation in multiplayer";
	uiPlayerSetup.model.minValue				= 0;
	uiPlayerSetup.model.maxValue				= 1;
	uiPlayerSetup.model.range					= 1;

	uiPlayerSetup.skin.generic.id				= ID_SKIN;
	uiPlayerSetup.skin.generic.type				= QMTYPE_SPINCONTROL;
	uiPlayerSetup.skin.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.skin.generic.x				= 368;
	uiPlayerSetup.skin.generic.y				= 288;
	uiPlayerSetup.skin.generic.width			= 198;
	uiPlayerSetup.skin.generic.height			= 30;
	uiPlayerSetup.skin.generic.callback			= UI_PlayerSetup_Callback;
	uiPlayerSetup.skin.generic.statusText		= "Select a skin for representation in multiplayer";
	uiPlayerSetup.skin.minValue					= 0;
	uiPlayerSetup.skin.maxValue					= 1;
	uiPlayerSetup.skin.range					= 1;

	uiPlayerSetup.railTrail.generic.id			= ID_RAILTRAIL;
	uiPlayerSetup.railTrail.generic.type		= QMTYPE_SPINCONTROL;
	uiPlayerSetup.railTrail.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.railTrail.generic.x			= 368;
	uiPlayerSetup.railTrail.generic.y			= 352;
	uiPlayerSetup.railTrail.generic.width		= 198;
	uiPlayerSetup.railTrail.generic.height		= 30;
	uiPlayerSetup.railTrail.generic.callback	= UI_PlayerSetup_Callback;
	uiPlayerSetup.railTrail.minValue			= 0;
	uiPlayerSetup.railTrail.maxValue			= 1;
	uiPlayerSetup.railTrail.range				= 1;

	uiPlayerSetup.viewRailCore.generic.id		= ID_VIEWRAILCORE;
	uiPlayerSetup.viewRailCore.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.viewRailCore.generic.flags	= QMF_INACTIVE;
	uiPlayerSetup.viewRailCore.generic.x		= 368;
	uiPlayerSetup.viewRailCore.generic.y		= 384;
	uiPlayerSetup.viewRailCore.generic.width	= 198;
	uiPlayerSetup.viewRailCore.generic.height	= 96;
	uiPlayerSetup.viewRailCore.generic.color	= uiColorWhite;
	uiPlayerSetup.viewRailCore.pic				= ART_VIEWRAILCORE;
	
	uiPlayerSetup.viewRailSpiral.generic.id		= ID_VIEWRAILSPIRAL;
	uiPlayerSetup.viewRailSpiral.generic.type	= QMTYPE_BITMAP;
	uiPlayerSetup.viewRailSpiral.generic.flags	= QMF_INACTIVE;
	uiPlayerSetup.viewRailSpiral.generic.x		= 368;
	uiPlayerSetup.viewRailSpiral.generic.y		= 384;
	uiPlayerSetup.viewRailSpiral.generic.width	= 198;
	uiPlayerSetup.viewRailSpiral.generic.height	= 96;
	uiPlayerSetup.viewRailSpiral.generic.color	= uiColorCyan;
	uiPlayerSetup.viewRailSpiral.pic			= ART_VIEWRAILSPIRAL;

	uiPlayerSetup.railCoreColor.generic.id		= ID_RAILCORECOLOR;
	uiPlayerSetup.railCoreColor.generic.type	= QMTYPE_SPINCONTROL;
	uiPlayerSetup.railCoreColor.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.railCoreColor.generic.x		= 368;
	uiPlayerSetup.railCoreColor.generic.y		= 482;
	uiPlayerSetup.railCoreColor.generic.width	= 198;
	uiPlayerSetup.railCoreColor.generic.height	= 30;
	uiPlayerSetup.railCoreColor.generic.callback	= UI_PlayerSetup_Callback;
	uiPlayerSetup.railCoreColor.minValue		= 0;
	uiPlayerSetup.railCoreColor.maxValue		= 1;
	uiPlayerSetup.railCoreColor.range			= 1;

	uiPlayerSetup.railSpiralColor.generic.id	= ID_RAILSPIRALCOLOR;
	uiPlayerSetup.railSpiralColor.generic.type	= QMTYPE_SPINCONTROL;
	uiPlayerSetup.railSpiralColor.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.railSpiralColor.generic.x		= 368;
	uiPlayerSetup.railSpiralColor.generic.y		= 514;
	uiPlayerSetup.railSpiralColor.generic.width	= 198;
	uiPlayerSetup.railSpiralColor.generic.height	= 30;
	uiPlayerSetup.railSpiralColor.generic.callback	= UI_PlayerSetup_Callback;
	uiPlayerSetup.railSpiralColor.minValue		= 0;
	uiPlayerSetup.railSpiralColor.maxValue		= 1;
	uiPlayerSetup.railSpiralColor.range			= 1;

	UI_PlayerSetup_GetConfig();

	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.background);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.banner);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.textShadow1);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.textShadow2);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.text1);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.text2);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.back);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.view);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.name);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.model);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.skin);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.railTrail);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.viewRailCore);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.viewRailSpiral);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.railCoreColor);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.railSpiralColor);
}

/*
 =================
 UI_PlayerSetup_Precache
 =================
*/
void UI_PlayerSetup_Precache (void){

	R_RegisterShaderNoMip(ART_BACKGROUND);
	R_RegisterShaderNoMip(ART_BANNER);
	R_RegisterShaderNoMip(ART_TEXT1);
	R_RegisterShaderNoMip(ART_TEXT2);
	R_RegisterShaderNoMip(ART_PLAYERVIEW);
	R_RegisterShaderNoMip(ART_VIEWRAILCORE);
	R_RegisterShaderNoMip(ART_VIEWRAILSPIRAL);
}

/*
 =================
 UI_PlayerSetup_Menu
 =================
*/
void UI_PlayerSetup_Menu (void){

	UI_PlayerSetup_Precache();
	UI_PlayerSetup_Init();

	UI_PlayerSetup_UpdateConfig();

	UI_PushMenu(&uiPlayerSetup.menu);
}
