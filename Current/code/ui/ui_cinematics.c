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


#define ART_BACKGROUND			"ui/misc/ui_sub_cinematics"
#define ART_BANNER				"ui/banners/cinematics_t"
#define ART_PREVIEWBACK			"ui/segments/cinematics"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_BACK				2
#define ID_PLAY				3

#define ID_CINLIST			4
#define ID_CINPREVIEW		5

#define MAX_MENUCINEMATICS	128

typedef struct {
	char					cinematics[MAX_MENUCINEMATICS][MAX_OSPATH];
	char					cinematicFiles[MAX_MENUCINEMATICS][MAX_OSPATH];
	char					*cinematicsPtr[MAX_MENUCINEMATICS];

	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			back;
	menuBitmap_s			play;

	menuScrollList_s		cinList;
	menuBitmap_s			cinPreview;
} uiCinematics_t;

static uiCinematics_t		uiCinematics;


/*
 =================
 UI_Cinematics_GetCinList
 =================
*/
static void UI_Cinematics_GetCinList (void){

	char	**fileList;
	int		numFiles;
	char	extension[16];
	int		i, count = 0;

	fileList = FS_ListFiles("video", NULL, true, &numFiles);

	if (numFiles > MAX_MENUCINEMATICS)
		numFiles = MAX_MENUCINEMATICS;

	for (i = 0; i < numFiles; i++){
		// Only copy .cin and .RoQ files, ignore the rest
		Com_FileExtension(fileList[i], extension, sizeof(extension));
		if (!Q_stricmp(extension, ".cin") || !Q_stricmp(extension, ".RoQ")){
			Q_strncpyz(uiCinematics.cinematics[count], fileList[i], sizeof(uiCinematics.cinematics[count]));
			*strchr(uiCinematics.cinematics[count], '.') = 0;
			Q_snprintfz(uiCinematics.cinematicFiles[count], sizeof(uiCinematics.cinematicFiles[count]), "video/%s", fileList[i]);
			count++;
		}
	}

	FS_FreeFileList(fileList);

	for (i = 0; i < count; i++)
		uiCinematics.cinematicsPtr[i] = uiCinematics.cinematics[i];
	for ( ; i < MAX_MENUCINEMATICS; i++)
		uiCinematics.cinematicsPtr[i] = NULL;

	uiCinematics.cinList.itemNames = uiCinematics.cinematicsPtr;
}

/*
 =================
 UI_Cinematics_KeyFunc
 =================
*/
static const char *UI_Cinematics_KeyFunc (int key){

	switch (key){
	case K_ESCAPE:
	case K_MOUSE2:
		if (uiStatic.cinematicPlaying){
			uiStatic.cinematicPlaying = false;

			CIN_StopCinematic();
		}

		break;
	}

	return UI_DefaultKey(&uiCinematics.menu, key);
}

/*
 =================
 UI_Cinematics_Callback
 =================
*/
static void UI_Cinematics_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event == QM_CHANGED){
		if (uiStatic.cinematicPlaying)
			CIN_StopCinematic();

		uiStatic.cinematicPlaying = CIN_PlayCinematic(uiCinematics.cinematicFiles[uiCinematics.cinList.curItem], CIN_SILENT | CIN_LOOP);
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		if (uiStatic.cinematicPlaying){
			uiStatic.cinematicPlaying = false;

			CIN_StopCinematic();
		}

		UI_PopMenu();
		break;
	case ID_PLAY:
	case ID_CINPREVIEW:
		if (uiStatic.cinematicPlaying){
			uiStatic.cinematicPlaying = false;

			CIN_StopCinematic();
		}

		Cbuf_ExecuteText(EXEC_APPEND, va("cinematic %s\n", uiCinematics.cinematicFiles[uiCinematics.cinList.curItem]));
		break;
	}
}

/*
 =================
 UI_Cinematics_Ownerdraw
 =================
*/
static void UI_Cinematics_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (item->id == ID_CINPREVIEW){
		int		x = 566, y = 210, w = 412, h = 348;

		UI_ScaleCoords(&x, &y, &w, &h);

		// Draw black background, cinematic frame and box
		UI_FillRect(x, y, w, h, uiColorBlack);

		if (uiStatic.cinematicPlaying){
			CIN_RunCinematic();
			CIN_DrawCinematic(x, y, w, h);
		}

		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
	}
	else {
		if (uiCinematics.menu.items[uiCinematics.menu.cursor] == self)
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
	}
}

/*
 =================
 UI_Cinematics_Init
 =================
*/
static void UI_Cinematics_Init (void){

	memset(&uiCinematics, 0, sizeof(uiCinematics_t));

	uiCinematics.menu.keyFunc					= UI_Cinematics_KeyFunc;

	uiCinematics.background.generic.id			= ID_BACKGROUND;
	uiCinematics.background.generic.type		= QMTYPE_BITMAP;
	uiCinematics.background.generic.flags		= QMF_INACTIVE;
	uiCinematics.background.generic.x			= 0;
	uiCinematics.background.generic.y			= 0;
	uiCinematics.background.generic.width		= 1024;
	uiCinematics.background.generic.height		= 768;
	uiCinematics.background.pic					= ART_BACKGROUND;

	uiCinematics.banner.generic.id				= ID_BANNER;
	uiCinematics.banner.generic.type			= QMTYPE_BITMAP;
	uiCinematics.banner.generic.flags			= QMF_INACTIVE;
	uiCinematics.banner.generic.x				= 0;
	uiCinematics.banner.generic.y				= 66;
	uiCinematics.banner.generic.width			= 1024;
	uiCinematics.banner.generic.height			= 46;
	uiCinematics.banner.pic						= ART_BANNER;

	uiCinematics.back.generic.id				= ID_BACK;
	uiCinematics.back.generic.type				= QMTYPE_BITMAP;
	uiCinematics.back.generic.x					= 310;
	uiCinematics.back.generic.y					= 656;
	uiCinematics.back.generic.width				= 198;
	uiCinematics.back.generic.height			= 38;
	uiCinematics.back.generic.callback			= UI_Cinematics_Callback;
	uiCinematics.back.generic.ownerdraw			= UI_Cinematics_Ownerdraw;
	uiCinematics.back.pic						= UI_BACKBUTTON;

	uiCinematics.play.generic.id				= ID_PLAY;
	uiCinematics.play.generic.type				= QMTYPE_BITMAP;
	uiCinematics.play.generic.x					= 516;
	uiCinematics.play.generic.y					= 656;
	uiCinematics.play.generic.width				= 198;
	uiCinematics.play.generic.height			= 38;
	uiCinematics.play.generic.callback			= UI_Cinematics_Callback;
	uiCinematics.play.generic.ownerdraw			= UI_Cinematics_Ownerdraw;
	uiCinematics.play.pic						= UI_PLAYBUTTON;

	uiCinematics.cinList.generic.id				= ID_CINLIST;
	uiCinematics.cinList.generic.type			= QMTYPE_SCROLLLIST;
	uiCinematics.cinList.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiCinematics.cinList.generic.x				= 44;
	uiCinematics.cinList.generic.y				= 208;
	uiCinematics.cinList.generic.width			= 512;
	uiCinematics.cinList.generic.height			= 352;
	uiCinematics.cinList.generic.callback		= UI_Cinematics_Callback;

	uiCinematics.cinPreview.generic.id			= ID_CINPREVIEW;
	uiCinematics.cinPreview.generic.type		= QMTYPE_BITMAP;
	uiCinematics.cinPreview.generic.x			= 564;
	uiCinematics.cinPreview.generic.y			= 208;
	uiCinematics.cinPreview.generic.width		= 416;
	uiCinematics.cinPreview.generic.height		= 352;
	uiCinematics.cinPreview.generic.callback	= UI_Cinematics_Callback;
	uiCinematics.cinPreview.generic.ownerdraw	= UI_Cinematics_Ownerdraw;
	uiCinematics.cinPreview.pic					= ART_PREVIEWBACK;

	UI_Cinematics_GetCinList();

	UI_AddItem(&uiCinematics.menu, (void *)&uiCinematics.background);
	UI_AddItem(&uiCinematics.menu, (void *)&uiCinematics.banner);
	UI_AddItem(&uiCinematics.menu, (void *)&uiCinematics.back);
	UI_AddItem(&uiCinematics.menu, (void *)&uiCinematics.play);
	UI_AddItem(&uiCinematics.menu, (void *)&uiCinematics.cinList);
	UI_AddItem(&uiCinematics.menu, (void *)&uiCinematics.cinPreview);

	// Start playing first cinematic in list
	if (uiCinematics.cinematicFiles[uiCinematics.cinList.curItem][0])
		uiStatic.cinematicPlaying = CIN_PlayCinematic(uiCinematics.cinematicFiles[uiCinematics.cinList.curItem], CIN_SILENT | CIN_LOOP);
}

/*
 =================
 UI_Cinematics_Precache
 =================
*/
void UI_Cinematics_Precache (void){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_PREVIEWBACK);
}

/*
 =================
 UI_Cinematics_Menu
 =================
*/
void UI_Cinematics_Menu (void){

	UI_Cinematics_Precache();
	UI_Cinematics_Init();

	UI_PushMenu(&uiCinematics.menu);
}
