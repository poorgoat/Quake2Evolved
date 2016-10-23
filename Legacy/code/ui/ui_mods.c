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


#define ART_BACKGROUND			"ui/misc/ui_sub_modifications"
#define ART_BANNER				"ui/banners/mods_t"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_BACK				2
#define ID_LOAD				3

#define ID_MODLIST			4

#define MAX_MODS			128
#define MAX_MODDESC			48

typedef struct {
	char					modsDir[MAX_MODS][MAX_QPATH];
	char					modsDescription[MAX_MODS][MAX_MODDESC];
	char					*modsDescriptionPtr[MAX_MODS];

	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			back;
	menuBitmap_s			load;

	menuScrollList_s		modList;
} uiMods_t;

static uiMods_t				uiMods;


/*
 =================
 UI_Mods_GetModList
 =================
*/
static void UI_Mods_GetModList (void){

	char	dirFiles[0x4000], *dirPtr, *descPtr;
	int		dirNum, dirLen, i;
	int		count = 0;

	// Always start off with baseq2
	Q_strncpyz(uiMods.modsDir[count], "baseq2", sizeof(uiMods.modsDir[count]));
	Q_strncpyz(uiMods.modsDescription[count], "Quake II", sizeof(uiMods.modsDescription[count]));
	count++;

	dirNum = FS_GetModList(dirFiles, sizeof(dirFiles));

	dirNum /= 2;
	if (dirNum > MAX_MODS)
		dirNum = MAX_MODS;

	for (i = 0, dirPtr = dirFiles; i < dirNum; i++){
		dirLen = strlen(dirPtr) + 1;
		descPtr = dirPtr + dirLen;

		Q_strncpyz(uiMods.modsDir[count], dirPtr, sizeof(uiMods.modsDir[count]));

		// Special check for CTF and mission packs
		if (!Q_stricmp(dirPtr, "ctf"))
			Q_strncpyz(uiMods.modsDescription[count], "Quake II: Capture The Flag", sizeof(uiMods.modsDescription[count]));
		else if (!Q_stricmp(dirPtr, "xatrix"))
			Q_strncpyz(uiMods.modsDescription[count], "Quake II: The Reckoning", sizeof(uiMods.modsDescription[count]));
		else if (!Q_stricmp(dirPtr, "rogue"))
			Q_strncpyz(uiMods.modsDescription[count], "Quake II: Ground Zero", sizeof(uiMods.modsDescription[count]));
		else
			Q_strncpyz(uiMods.modsDescription[count], descPtr, sizeof(uiMods.modsDescription[count]));

		count++;

		dirPtr += (dirLen + strlen(descPtr) + 1);
	}

	for (i = 0; i < count; i++)
		uiMods.modsDescriptionPtr[i] = uiMods.modsDescription[i];
	for ( ; i < MAX_MODS; i++)
		uiMods.modsDescriptionPtr[i] = NULL;

	uiMods.modList.itemNames = uiMods.modsDescriptionPtr;

	// See if the load button should be grayed
	if (!Q_stricmp(Cvar_VariableString("fs_game"), uiMods.modsDir[0]))
		uiMods.load.generic.flags |= QMF_GRAYED;
}

/*
 =================
 UI_Mods_Callback
 =================
*/
static void UI_Mods_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event == QM_CHANGED){
		// See if the load button should be grayed
		if (!Q_stricmp(Cvar_VariableString("fs_game"), uiMods.modsDir[uiMods.modList.curItem]))
			uiMods.load.generic.flags |= QMF_GRAYED;
		else
			uiMods.load.generic.flags &= ~QMF_GRAYED;

		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_LOAD:
		if (cls.state == CA_ACTIVE)
			break;		// Don't fuck up the game

		Cvar_ForceSet("fs_game", uiMods.modsDir[uiMods.modList.curItem]);

		// Restart file system
		FS_Restart();

		// Flush all data so it will be forced to reload
		Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");

		break;
	}
}

/*
 =================
 UI_Mods_Ownerdraw
 =================
*/
static void UI_Mods_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (uiMods.menu.items[uiMods.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

	UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
}

/*
 =================
 UI_Mods_Init
 =================
*/
static void UI_Mods_Init (void){

	memset(&uiMods, 0, sizeof(uiMods_t));

	uiMods.background.generic.id				= ID_BACKGROUND;
	uiMods.background.generic.type				= QMTYPE_BITMAP;
	uiMods.background.generic.flags				= QMF_INACTIVE;
	uiMods.background.generic.x					= 0;
	uiMods.background.generic.y					= 0;
	uiMods.background.generic.width				= 1024;
	uiMods.background.generic.height			= 768;
	uiMods.background.pic						= ART_BACKGROUND;

	uiMods.banner.generic.id					= ID_BANNER;
	uiMods.banner.generic.type					= QMTYPE_BITMAP;
	uiMods.banner.generic.flags					= QMF_INACTIVE;
	uiMods.banner.generic.x						= 0;
	uiMods.banner.generic.y						= 66;
	uiMods.banner.generic.width					= 1024;
	uiMods.banner.generic.height				= 46;
	uiMods.banner.pic							= ART_BANNER;

	uiMods.back.generic.id						= ID_BACK;
	uiMods.back.generic.type					= QMTYPE_BITMAP;
	uiMods.back.generic.x						= 310;
	uiMods.back.generic.y						= 656;
	uiMods.back.generic.width					= 198;
	uiMods.back.generic.height					= 38;
	uiMods.back.generic.callback				= UI_Mods_Callback;
	uiMods.back.generic.ownerdraw				= UI_Mods_Ownerdraw;
	uiMods.back.pic								= UI_BACKBUTTON;

	uiMods.load.generic.id						= ID_LOAD;
	uiMods.load.generic.type					= QMTYPE_BITMAP;
	uiMods.load.generic.x						= 516;
	uiMods.load.generic.y						= 656;
	uiMods.load.generic.width					= 198;
	uiMods.load.generic.height					= 38;
	uiMods.load.generic.callback				= UI_Mods_Callback;
	uiMods.load.generic.ownerdraw				= UI_Mods_Ownerdraw;
	uiMods.load.pic								= UI_LOADBUTTON;

	uiMods.modList.generic.id					= ID_MODLIST;
	uiMods.modList.generic.type					= QMTYPE_SCROLLLIST;
	uiMods.modList.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiMods.modList.generic.x					= 256;
	uiMods.modList.generic.y					= 208;
	uiMods.modList.generic.width				= 512;
	uiMods.modList.generic.height				= 352;
	uiMods.modList.generic.callback				= UI_Mods_Callback;

	UI_Mods_GetModList();

	UI_AddItem(&uiMods.menu, (void *)&uiMods.background);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.banner);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.back);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.load);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.modList);
}

/*
 =================
 UI_Mods_Precache
 =================
*/
void UI_Mods_Precache (void){

	R_RegisterShaderNoMip(ART_BACKGROUND);
	R_RegisterShaderNoMip(ART_BANNER);
}

/*
 =================
 UI_Mods_Menu
 =================
*/
void UI_Mods_Menu (void){

	UI_Mods_Precache();
	UI_Mods_Init();

	UI_PushMenu(&uiMods.menu);
}
