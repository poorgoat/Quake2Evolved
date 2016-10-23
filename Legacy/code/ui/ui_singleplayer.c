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


#define ART_BACKGROUND			"ui/misc/ui_sub_singleplayer"
#define ART_BANNER				"ui/banners/singleplayer_t"
#define ART_LEVELSHOTBLUR		"ui/segments/sp_mapshot"
#define ART_LISTBLUR			"ui/segments/sp_mapload"
#define ART_EASY				"ui/buttons/sp_easy"
#define ART_MEDIUM				"ui/buttons/sp_medium"
#define ART_HARD				"ui/buttons/sp_hard"
#define ART_HARDPLUS			"ui/buttons/sp_hard+"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_BACK				2
#define ID_START			3

#define ID_LEVELSHOT		4
#define ID_MISSIONLIST		5

#define ID_SKILL			6
#define ID_LOADGAME			7
#define ID_SAVEGAME			8

#define MAX_MISSIONS		32
#define MAX_LEVELSHOTS		16

typedef struct {
	char	mission[MAX_QPATH];
	char	name[80];
	char	objectives[1024];
	char	levelShots[MAX_LEVELSHOTS][MAX_QPATH];
	int		numLevelShots;
} missionInfo_t;

typedef struct {
	missionInfo_t			missions[MAX_MISSIONS];
	int						numMissions;
	char					*missionsPtr[MAX_MISSIONS];

	int						currentLevelShot;
	int						fadeTime;

	int						currentSkill;

	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			back;
	menuBitmap_s			start;

	menuBitmap_s			levelShot;
	menuScrollList_s		missionList;

	menuBitmap_s			skill;
	menuBitmap_s			loadGame;
	menuBitmap_s			saveGame;
} uiSinglePlayer_t;

static uiSinglePlayer_t		uiSinglePlayer;


/*
 =================
 UI_SinglePlayer_GetMissionList
 =================
*/
static void UI_SinglePlayer_GetMissionList (void){

	int		i;
	char	name[MAX_QPATH];
	char	*buffer, *buf, *tok;

	Q_snprintfz(name, sizeof(name), "scripts/ui/mission_%s.txt", Cvar_VariableString("fs_game"));
	FS_LoadFile(name, (void **)&buffer);

	if (!buffer){
		for (i = 0; i < MAX_MISSIONS; i++)
			uiSinglePlayer.missionsPtr[i] = NULL;

		uiSinglePlayer.missionList.itemNames = uiSinglePlayer.missionsPtr;
		return;
	}

	// Parse infos
	buf = buffer;
	while (1){
		// Parse name
		tok = Com_ParseExt(&buf, true);
		if (!tok[0])
			break;		// End of data

		Q_strncpyz(uiSinglePlayer.missions[uiSinglePlayer.numMissions].mission, tok, sizeof(uiSinglePlayer.missions[uiSinglePlayer.numMissions].mission));

		tok = Com_ParseExt(&buf, true);
		if (Q_stricmp(tok, "{"))
			break;

		while (1){
			tok = Com_ParseExt(&buf, true);
			if (!tok[0])
				break;	// End of data

			if (!Q_stricmp(tok, "name")){
				tok = Com_ParseExt(&buf, false);
				Q_strncpyz(uiSinglePlayer.missions[uiSinglePlayer.numMissions].name, tok, sizeof(uiSinglePlayer.missions[uiSinglePlayer.numMissions].name));
			}
			else if (!Q_stricmp(tok, "objective")){
				char	str[1024];
				int		len = 0;

				tok = Com_ParseExt(&buf, false);
				Q_strncpyz(str, tok, sizeof(str));

				// Remove non-printable chars, except newlines
				for (i = 0; str[i]; i++){
					if ((str[i] >= ' ' && str[i] <= 126) || str[i] == '\n')
						uiSinglePlayer.missions[uiSinglePlayer.numMissions].objectives[len++] = str[i];
				}

				uiSinglePlayer.missions[uiSinglePlayer.numMissions].objectives[len] = 0;
			}
			else if (!Q_stricmp(tok, "levelshots")){
				char	*levelshot;

				for (i = 0; i < MAX_LEVELSHOTS; i++){
					tok = Com_ParseExt(&buf, false);
					if (!tok[0])
						break;

					levelshot = uiSinglePlayer.missions[uiSinglePlayer.numMissions].levelShots[uiSinglePlayer.missions[uiSinglePlayer.numMissions].numLevelShots];
					Q_strncpyz(levelshot, tok, MAX_QPATH);
					uiSinglePlayer.missions[uiSinglePlayer.numMissions].numLevelShots++;
				}
			}
			else if (!Q_stricmp(tok, "}")){
				uiSinglePlayer.numMissions++;
				break;
			}
			else
				break;	// Invalid
		}
	}

	FS_FreeFile(buffer);

	for (i = 0; i < uiSinglePlayer.numMissions; i++)
		uiSinglePlayer.missionsPtr[i] = uiSinglePlayer.missions[i].name;
	for ( ; i < MAX_MISSIONS; i++)
		uiSinglePlayer.missionsPtr[i] = NULL;

	uiSinglePlayer.missionList.itemNames = uiSinglePlayer.missionsPtr;}

/*
 =================
 UI_SinglePlayer_StartGame
 =================
*/
static void UI_SinglePlayer_StartGame (void){

	char	*game = Cvar_VariableString("fs_game");
	char	text[256];

	Cvar_SetInteger("skill", uiSinglePlayer.currentSkill);
	Cvar_SetInteger("deathmatch", 0);
	Cvar_SetInteger("coop", 0);

	if (uiSinglePlayer.numMissions)
		Q_snprintfz(text, sizeof(text), "killserver ; wait ; exec %s.cfg\n", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].mission);
	else {
		if (!Q_stricmp(game, "baseq2"))
			Q_snprintfz(text, sizeof(text), "killserver ; wait ; gamemap *ntro.cin+base1\n");
		else if (!Q_stricmp(game, "xatrix"))
			Q_snprintfz(text, sizeof(text), "killserver ; wait ; gamemap *xin.cin+xswamp\n");
		else if (!Q_stricmp(game, "rogue"))
			Q_snprintfz(text, sizeof(text), "killserver ; wait ; gamemap *rintro.cin+rmine1\n");
		else
			Q_snprintfz(text, sizeof(text), "killserver ; wait ; newgame\n");
	}

	Cbuf_ExecuteText(EXEC_APPEND, text);
}

/*
 =================
 UI_SinglePlayer_Callback
 =================
*/
static void UI_SinglePlayer_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;
	if (event == QM_CHANGED){
		// Restart levelshot animation
		uiSinglePlayer.currentLevelShot = 0;
		uiSinglePlayer.fadeTime = uiStatic.realTime;
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_START:
		UI_SinglePlayer_StartGame();
		break;
	case ID_SKILL:
		uiSinglePlayer.currentSkill++;
		if (uiSinglePlayer.currentSkill > 3)
			uiSinglePlayer.currentSkill = 0;

		Cvar_SetInteger("ui_singlePlayerSkill", uiSinglePlayer.currentSkill);

		switch (uiSinglePlayer.currentSkill){
		case 0:
			uiSinglePlayer.skill.pic = ART_EASY;
			break;
		case 1:
			uiSinglePlayer.skill.pic = ART_MEDIUM;
			break;
		case 2:
			uiSinglePlayer.skill.pic = ART_HARD;
			break;
		case 3:
			uiSinglePlayer.skill.pic = ART_HARDPLUS;
			break;
		}
		break;
	case ID_LOADGAME:
		UI_LoadGame_Menu();
		break;
	case ID_SAVEGAME:
		UI_SaveGame_Menu();
		break;
	}
}

/*
 =================
 UI_SinglePlayer_Ownerdraw
 =================
*/
static void UI_SinglePlayer_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (item->id == ID_LEVELSHOT){
		int			x, y, w, h, cw, ch;
		char		str[2048];
		int			prev;
		color_t		color = {255, 255, 255, 255};
		
		// Draw the levelshot
		x = 66;
		y = 134;
		w = 410;
		h = 202;
		
		UI_ScaleCoords(&x, &y, &w, &h);

		if (uiSinglePlayer.numMissions && uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].numLevelShots){
			if (uiStatic.realTime - uiSinglePlayer.fadeTime >= 3000){
				uiSinglePlayer.fadeTime = uiStatic.realTime;

				uiSinglePlayer.currentLevelShot++;
				if (uiSinglePlayer.currentLevelShot == uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].numLevelShots)
					uiSinglePlayer.currentLevelShot = 0;
			}

			prev = uiSinglePlayer.currentLevelShot - 1;
			if (prev < 0)
				prev = uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].numLevelShots - 1;

			color[3] = Clamp((float)(uiStatic.realTime - uiSinglePlayer.fadeTime) / 1000, 0.0, 1.0) * 255;

			UI_DrawPic(x, y, w, h, uiColorWhite, va("ui/menu_levelshots/%s", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].levelShots[prev]));
			UI_DrawPic(x, y, w, h, color, va("ui/menu_levelshots/%s", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].levelShots[uiSinglePlayer.currentLevelShot]));
		}
		else
			UI_DrawPic(x, y, w, h, uiColorWhite, "ui/menu_levelshots/unknownmap");

		// Draw the blurred frame
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);

		// Draw the info text
		x = 512;
		y = 134;
		w = 448;
		h = 202;

		cw = 10;
		ch = 20;

		UI_ScaleCoords(&x, &y, &w, &h);
		UI_ScaleCoords(NULL, NULL, &cw, &ch);

		MakeRGBA(color, 0, 76, 127, 255);

		if (uiSinglePlayer.numMissions){
			Q_snprintfz(str, sizeof(str), "MISSION: ^7%s\n", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].name);
			UI_DrawString(x, y, w, h, str, color, false, cw, ch, 0, true);
		
			y += ch;
			Q_snprintfz(str, sizeof(str), "OBJECTIVES:\n");
			UI_DrawString(x, y, w, h, str, color, false, cw, ch, 0, true);

			y += ch;
			Q_snprintfz(str, sizeof(str), "%s\n", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].objectives);
			UI_DrawString(x, y, w, h, str, uiColorWhite, true, cw, ch, 0, true);
		}
		else
			UI_DrawString(x, y, w, h, "NO MISSION DATA AVAILABLE", color, true, cw, ch, 1, true);
	}
	else {
		if (uiSinglePlayer.menu.items[uiSinglePlayer.menu.cursor] == self)
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
	}
}

/*
 =================
 UI_SinglePlayer_Init
 =================
*/
static void UI_SinglePlayer_Init (void){

	memset(&uiSinglePlayer, 0, sizeof(uiSinglePlayer_t));

	uiSinglePlayer.fadeTime = uiStatic.realTime;

	uiSinglePlayer.background.generic.id		= ID_BACKGROUND;
	uiSinglePlayer.background.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.background.generic.flags		= QMF_INACTIVE;
	uiSinglePlayer.background.generic.x			= 0;
	uiSinglePlayer.background.generic.y			= 0;
	uiSinglePlayer.background.generic.width		= 1024;
	uiSinglePlayer.background.generic.height	= 768;
	uiSinglePlayer.background.pic				= ART_BACKGROUND;

	uiSinglePlayer.banner.generic.id			= ID_BANNER;
	uiSinglePlayer.banner.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.banner.generic.flags			= QMF_INACTIVE;
	uiSinglePlayer.banner.generic.x				= 0;
	uiSinglePlayer.banner.generic.y				= 66;
	uiSinglePlayer.banner.generic.width			= 1024;
	uiSinglePlayer.banner.generic.height		= 46;
	uiSinglePlayer.banner.pic					= ART_BANNER;

	uiSinglePlayer.back.generic.id				= ID_BACK;
	uiSinglePlayer.back.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.back.generic.x				= 310;
	uiSinglePlayer.back.generic.y				= 656;
	uiSinglePlayer.back.generic.width			= 198;
	uiSinglePlayer.back.generic.height			= 38;
	uiSinglePlayer.back.generic.callback		= UI_SinglePlayer_Callback;
	uiSinglePlayer.back.generic.ownerdraw		= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.back.pic						= UI_BACKBUTTON;

	uiSinglePlayer.start.generic.id				= ID_START;
	uiSinglePlayer.start.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.start.generic.x				= 516;
	uiSinglePlayer.start.generic.y				= 656;
	uiSinglePlayer.start.generic.width			= 198;
	uiSinglePlayer.start.generic.height			= 38;
	uiSinglePlayer.start.generic.callback		= UI_SinglePlayer_Callback;
	uiSinglePlayer.start.generic.ownerdraw		= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.start.pic					= UI_STARTBUTTON;

	uiSinglePlayer.levelShot.generic.id			= ID_LEVELSHOT;
	uiSinglePlayer.levelShot.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.levelShot.generic.flags		= QMF_INACTIVE;
	uiSinglePlayer.levelShot.generic.x			= 64;
	uiSinglePlayer.levelShot.generic.y			= 132;
	uiSinglePlayer.levelShot.generic.width		= 414;
	uiSinglePlayer.levelShot.generic.height		= 206;
	uiSinglePlayer.levelShot.generic.ownerdraw	= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.levelShot.pic				= ART_LEVELSHOTBLUR;

	uiSinglePlayer.missionList.generic.id		= ID_MISSIONLIST;
	uiSinglePlayer.missionList.generic.type		= QMTYPE_SCROLLLIST;
	uiSinglePlayer.missionList.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiSinglePlayer.missionList.generic.x		= 64;
	uiSinglePlayer.missionList.generic.y		= 416;
	uiSinglePlayer.missionList.generic.width	= 414;
	uiSinglePlayer.missionList.generic.height	= 140;
	uiSinglePlayer.missionList.generic.callback	= UI_SinglePlayer_Callback;
	uiSinglePlayer.missionList.background		= ART_LISTBLUR;

	uiSinglePlayer.skill.generic.id				= ID_SKILL;
	uiSinglePlayer.skill.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.skill.generic.x				= 676;
	uiSinglePlayer.skill.generic.y				= 420;
	uiSinglePlayer.skill.generic.width			= 198;
	uiSinglePlayer.skill.generic.height			= 38;
	uiSinglePlayer.skill.generic.callback		= UI_SinglePlayer_Callback;
	uiSinglePlayer.skill.generic.ownerdraw		= UI_SinglePlayer_Ownerdraw;
	
	uiSinglePlayer.loadGame.generic.id			= ID_LOADGAME;
	uiSinglePlayer.loadGame.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.loadGame.generic.x			= 676;
	uiSinglePlayer.loadGame.generic.y			= 468;
	uiSinglePlayer.loadGame.generic.width		= 198;
	uiSinglePlayer.loadGame.generic.height		= 38;
	uiSinglePlayer.loadGame.generic.callback	= UI_SinglePlayer_Callback;
	uiSinglePlayer.loadGame.generic.ownerdraw	= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.loadGame.pic					= UI_LOADBUTTON;

	uiSinglePlayer.saveGame.generic.id			= ID_SAVEGAME;
	uiSinglePlayer.saveGame.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.saveGame.generic.x			= 676;
	uiSinglePlayer.saveGame.generic.y			= 516;
	uiSinglePlayer.saveGame.generic.width		= 198;
	uiSinglePlayer.saveGame.generic.height		= 38;
	uiSinglePlayer.saveGame.generic.callback	= UI_SinglePlayer_Callback;
	uiSinglePlayer.saveGame.generic.ownerdraw	= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.saveGame.pic					= UI_SAVEBUTTON;

	UI_SinglePlayer_GetMissionList();

	uiSinglePlayer.currentSkill = ui_singlePlayerSkill->integer;
	if (uiSinglePlayer.currentSkill > 3)
		uiSinglePlayer.currentSkill = 3;
	else if (uiSinglePlayer.currentSkill < 0)
		uiSinglePlayer.currentSkill = 0;

	switch (uiSinglePlayer.currentSkill){
	case 0:
		uiSinglePlayer.skill.pic = ART_EASY;
		break;
	case 1:
		uiSinglePlayer.skill.pic = ART_MEDIUM;
		break;
	case 2:
		uiSinglePlayer.skill.pic = ART_HARD;
		break;
	case 3:
		uiSinglePlayer.skill.pic = ART_HARDPLUS;
		break;
	}

	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.background);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.banner);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.back);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.start);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.levelShot);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.missionList);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.skill);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.loadGame);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.saveGame);
}

/*
 =================
 UI_SinglePlayer_Precache
 =================
*/
void UI_SinglePlayer_Precache (void){

	R_RegisterShaderNoMip(ART_BACKGROUND);
	R_RegisterShaderNoMip(ART_BANNER);
	R_RegisterShaderNoMip(ART_LEVELSHOTBLUR);
	R_RegisterShaderNoMip(ART_LISTBLUR);
	R_RegisterShaderNoMip(ART_EASY);
	R_RegisterShaderNoMip(ART_MEDIUM);
	R_RegisterShaderNoMip(ART_HARD);
	R_RegisterShaderNoMip(ART_HARDPLUS);
}

/*
 =================
 UI_SinglePlayer_Menu
 =================
*/
void UI_SinglePlayer_Menu (void){

	UI_SinglePlayer_Precache();
	UI_SinglePlayer_Init();

	UI_PushMenu(&uiSinglePlayer.menu);
}
