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
#define ART_BANNER				"ui/banners/performance_t"
#define ART_TEXT1				"ui/text/performance_text_p1"
#define ART_TEXT2				"ui/text/performance_text_p2"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_TEXT1			2
#define ID_TEXT2			3
#define ID_TEXTSHADOW1		4
#define ID_TEXTSHADOW2		5

#define ID_BACK				6

#define ID_SYNCEVERYFRAME	7
#define ID_CAUSTICS			8
#define ID_PARTICLES		9
#define ID_DECALS			10
#define ID_EJECTINGBRASS	11
#define ID_SHELLEFFECTS		12
#define ID_SCREENBLENDS		13
#define ID_BLOOD			14

static const char	*uiPerformanceYesNo[] = {
	"False",
	"True"
};

typedef struct {
	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			textShadow1;
	menuBitmap_s			textShadow2;
	menuBitmap_s			text1;
	menuBitmap_s			text2;

	menuBitmap_s			back;

	menuSpinControl_s		syncEveryFrame;
	menuSpinControl_s		caustics;
	menuSpinControl_s		particles;
	menuSpinControl_s		decals;
	menuSpinControl_s		ejectingBrass;
	menuSpinControl_s		shellEffects;
	menuSpinControl_s		screenBlends;
	menuSpinControl_s		blood;
} uiPerformance_t;

static uiPerformance_t		uiPerformance;


/*
 =================
 UI_Performance_GetConfig
 =================
*/
static void UI_Performance_GetConfig (void){

	if (Cvar_GetInteger("r_finish"))
		uiPerformance.syncEveryFrame.curValue = 1;

	if (Cvar_GetInteger("r_caustics"))
		uiPerformance.caustics.curValue = 1;

	if (Cvar_GetInteger("cl_particles"))
		uiPerformance.particles.curValue = 1;

	if (Cvar_GetInteger("cl_decals"))
		uiPerformance.decals.curValue = 1;

	if (Cvar_GetInteger("cl_ejectBrass"))
		uiPerformance.ejectingBrass.curValue = 1;

	if (Cvar_GetInteger("cl_shells"))
		uiPerformance.shellEffects.curValue = 1;

	if (Cvar_GetInteger("cl_viewBlend"))
		uiPerformance.screenBlends.curValue = 1;

	if (Cvar_GetInteger("cl_blood"))
		uiPerformance.blood.curValue = 1;
}

/*
 =================
 UI_Performance_SetConfig
 =================
*/
static void UI_Performance_SetConfig (void){

	Cvar_SetInteger("r_finish", (int)uiPerformance.syncEveryFrame.curValue);

	Cvar_SetInteger("r_caustics", (int)uiPerformance.caustics.curValue);

	Cvar_SetInteger("cl_particles", (int)uiPerformance.particles.curValue);

	Cvar_SetInteger("cl_decals", (int)uiPerformance.decals.curValue);

	Cvar_SetInteger("cl_ejectBrass", (int)uiPerformance.ejectingBrass.curValue);

	Cvar_SetInteger("cl_shells", (int)uiPerformance.shellEffects.curValue);

	Cvar_SetInteger("cl_viewBlend", (int)uiPerformance.screenBlends.curValue);

	Cvar_SetInteger("cl_blood", (int)uiPerformance.blood.curValue);
}

/*
 =================
 UI_Performance_UpdateConfig
 =================
*/
static void UI_Performance_UpdateConfig (void){

	static char	sizeText[32];

	uiPerformance.syncEveryFrame.generic.name = uiPerformanceYesNo[(int)uiPerformance.syncEveryFrame.curValue];

	uiPerformance.caustics.generic.name = uiPerformanceYesNo[(int)uiPerformance.caustics.curValue];

	uiPerformance.particles.generic.name = uiPerformanceYesNo[(int)uiPerformance.particles.curValue];

	uiPerformance.decals.generic.name = uiPerformanceYesNo[(int)uiPerformance.decals.curValue];

	uiPerformance.ejectingBrass.generic.name = uiPerformanceYesNo[(int)uiPerformance.ejectingBrass.curValue];

	uiPerformance.shellEffects.generic.name = uiPerformanceYesNo[(int)uiPerformance.shellEffects.curValue];

	uiPerformance.screenBlends.generic.name = uiPerformanceYesNo[(int)uiPerformance.screenBlends.curValue];

	uiPerformance.blood.generic.name = uiPerformanceYesNo[(int)uiPerformance.blood.curValue];
}

/*
 =================
 UI_Performance_Callback
 =================
*/
static void UI_Performance_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event == QM_CHANGED){
		UI_Performance_UpdateConfig();
		UI_Performance_SetConfig();
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
 UI_Performance_Ownerdraw
 =================
*/
static void UI_Performance_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (uiPerformance.menu.items[uiPerformance.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

	UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
}

/*
 =================
 UI_Performance_Init
 =================
*/
static void UI_Performance_Init (void){

	memset(&uiPerformance, 0, sizeof(uiPerformance_t));

	uiPerformance.background.generic.id			= ID_BACKGROUND;
	uiPerformance.background.generic.type		= QMTYPE_BITMAP;
	uiPerformance.background.generic.flags		= QMF_INACTIVE;
	uiPerformance.background.generic.x			= 0;
	uiPerformance.background.generic.y			= 0;
	uiPerformance.background.generic.width		= 1024;
	uiPerformance.background.generic.height		= 768;
	uiPerformance.background.pic				= ART_BACKGROUND;

	uiPerformance.banner.generic.id				= ID_BANNER;
	uiPerformance.banner.generic.type			= QMTYPE_BITMAP;
	uiPerformance.banner.generic.flags			= QMF_INACTIVE;
	uiPerformance.banner.generic.x				= 0;
	uiPerformance.banner.generic.y				= 66;
	uiPerformance.banner.generic.width			= 1024;
	uiPerformance.banner.generic.height			= 46;
	uiPerformance.banner.pic					= ART_BANNER;

	uiPerformance.textShadow1.generic.id		= ID_TEXTSHADOW1;
	uiPerformance.textShadow1.generic.type		= QMTYPE_BITMAP;
	uiPerformance.textShadow1.generic.flags		= QMF_INACTIVE;
	uiPerformance.textShadow1.generic.x			= 182;
	uiPerformance.textShadow1.generic.y			= 170;
	uiPerformance.textShadow1.generic.width		= 256;
	uiPerformance.textShadow1.generic.height	= 256;
	uiPerformance.textShadow1.generic.color		= uiColorBlack;
	uiPerformance.textShadow1.pic				= ART_TEXT1;

	uiPerformance.textShadow2.generic.id		= ID_TEXTSHADOW2;
	uiPerformance.textShadow2.generic.type		= QMTYPE_BITMAP;
	uiPerformance.textShadow2.generic.flags		= QMF_INACTIVE;
	uiPerformance.textShadow2.generic.x			= 182;
	uiPerformance.textShadow2.generic.y			= 426;
	uiPerformance.textShadow2.generic.width		= 256;
	uiPerformance.textShadow2.generic.height	= 256;
	uiPerformance.textShadow2.generic.color		= uiColorBlack;
	uiPerformance.textShadow2.pic				= ART_TEXT2;

	uiPerformance.text1.generic.id				= ID_TEXT1;
	uiPerformance.text1.generic.type			= QMTYPE_BITMAP;
	uiPerformance.text1.generic.flags			= QMF_INACTIVE;
	uiPerformance.text1.generic.x				= 180;
	uiPerformance.text1.generic.y				= 168;
	uiPerformance.text1.generic.width			= 256;
	uiPerformance.text1.generic.height			= 256;
	uiPerformance.text1.pic						= ART_TEXT1;

	uiPerformance.text2.generic.id				= ID_TEXT2;
	uiPerformance.text2.generic.type			= QMTYPE_BITMAP;
	uiPerformance.text2.generic.flags			= QMF_INACTIVE;
	uiPerformance.text2.generic.x				= 180;
	uiPerformance.text2.generic.y				= 424;
	uiPerformance.text2.generic.width			= 256;
	uiPerformance.text2.generic.height			= 256;
	uiPerformance.text2.pic						= ART_TEXT2;

	uiPerformance.back.generic.id				= ID_BACK;
	uiPerformance.back.generic.type				= QMTYPE_BITMAP;
	uiPerformance.back.generic.x				= 413;
	uiPerformance.back.generic.y				= 656;
	uiPerformance.back.generic.width			= 198;
	uiPerformance.back.generic.height			= 38;
	uiPerformance.back.generic.callback			= UI_Performance_Callback;
	uiPerformance.back.generic.ownerdraw		= UI_Performance_Ownerdraw;
	uiPerformance.back.pic						= UI_BACKBUTTON;

	uiPerformance.syncEveryFrame.generic.id		= ID_SYNCEVERYFRAME;
	uiPerformance.syncEveryFrame.generic.type	= QMTYPE_SPINCONTROL;
	uiPerformance.syncEveryFrame.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.syncEveryFrame.generic.x		= 580;
	uiPerformance.syncEveryFrame.generic.y		= 192;
	uiPerformance.syncEveryFrame.generic.width	= 198;
	uiPerformance.syncEveryFrame.generic.height	= 30;
	uiPerformance.syncEveryFrame.generic.callback	= UI_Performance_Callback;
	uiPerformance.syncEveryFrame.generic.statusText	= "Synchronize video frames";
	uiPerformance.syncEveryFrame.minValue		= 0;
	uiPerformance.syncEveryFrame.maxValue		= 1;
	uiPerformance.syncEveryFrame.range			= 1;

	uiPerformance.caustics.generic.id			= ID_CAUSTICS;
	uiPerformance.caustics.generic.type			= QMTYPE_SPINCONTROL;
	uiPerformance.caustics.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.caustics.generic.x			= 580;
	uiPerformance.caustics.generic.y			= 320;
	uiPerformance.caustics.generic.width		= 198;
	uiPerformance.caustics.generic.height		= 30;
	uiPerformance.caustics.generic.callback		= UI_Performance_Callback;
	uiPerformance.caustics.generic.statusText	= "Render caustics on submerged surfaces";
	uiPerformance.caustics.minValue				= 0;
	uiPerformance.caustics.maxValue				= 1;
	uiPerformance.caustics.range				= 1;

	uiPerformance.particles.generic.id			= ID_PARTICLES;
	uiPerformance.particles.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.particles.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.particles.generic.x			= 580;
	uiPerformance.particles.generic.y			= 352;
	uiPerformance.particles.generic.width		= 198;
	uiPerformance.particles.generic.height		= 30;
	uiPerformance.particles.generic.callback	= UI_Performance_Callback;
	uiPerformance.particles.generic.statusText	= "Enable/Disable particle effects";
	uiPerformance.particles.minValue			= 0;
	uiPerformance.particles.maxValue			= 1;
	uiPerformance.particles.range				= 1;

	uiPerformance.decals.generic.id				= ID_DECALS;
	uiPerformance.decals.generic.type			= QMTYPE_SPINCONTROL;
	uiPerformance.decals.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.decals.generic.x				= 580;
	uiPerformance.decals.generic.y				= 384;
	uiPerformance.decals.generic.width			= 198;
	uiPerformance.decals.generic.height			= 30;
	uiPerformance.decals.generic.callback		= UI_Performance_Callback;
	uiPerformance.decals.generic.statusText		= "Enable/Disable marks on walls";
	uiPerformance.decals.minValue				= 0;
	uiPerformance.decals.maxValue				= 1;
	uiPerformance.decals.range					= 1;

	uiPerformance.ejectingBrass.generic.id		= ID_EJECTINGBRASS;
	uiPerformance.ejectingBrass.generic.type	= QMTYPE_SPINCONTROL;
	uiPerformance.ejectingBrass.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.ejectingBrass.generic.x		= 580;
	uiPerformance.ejectingBrass.generic.y		= 416;
	uiPerformance.ejectingBrass.generic.width	= 198;
	uiPerformance.ejectingBrass.generic.height	= 30;
	uiPerformance.ejectingBrass.generic.callback	= UI_Performance_Callback;
	uiPerformance.ejectingBrass.generic.statusText	= "Enable/Disable ejecting bullet casings from weapons";
	uiPerformance.ejectingBrass.minValue		= 0;
	uiPerformance.ejectingBrass.maxValue		= 1;
	uiPerformance.ejectingBrass.range			= 1;

	uiPerformance.shellEffects.generic.id		= ID_SHELLEFFECTS;
	uiPerformance.shellEffects.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.shellEffects.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.shellEffects.generic.x		= 580;
	uiPerformance.shellEffects.generic.y		= 448;
	uiPerformance.shellEffects.generic.width	= 198;
	uiPerformance.shellEffects.generic.height	= 30;
	uiPerformance.shellEffects.generic.callback	= UI_Performance_Callback;
	uiPerformance.shellEffects.generic.statusText	= "Enable/Disable player and weapon shells";
	uiPerformance.shellEffects.minValue			= 0;
	uiPerformance.shellEffects.maxValue			= 1;
	uiPerformance.shellEffects.range			= 1;

	uiPerformance.screenBlends.generic.id		= ID_SCREENBLENDS;
	uiPerformance.screenBlends.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.screenBlends.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.screenBlends.generic.x		= 580;
	uiPerformance.screenBlends.generic.y		= 480;
	uiPerformance.screenBlends.generic.width	= 198;
	uiPerformance.screenBlends.generic.height	= 30;
	uiPerformance.screenBlends.generic.callback	= UI_Performance_Callback;
	uiPerformance.screenBlends.generic.statusText	= "Enable/Disable screen blends";
	uiPerformance.screenBlends.minValue			= 0;
	uiPerformance.screenBlends.maxValue			= 1;
	uiPerformance.screenBlends.range			= 1;

	uiPerformance.blood.generic.id				= ID_BLOOD;
	uiPerformance.blood.generic.type			= QMTYPE_SPINCONTROL;
	uiPerformance.blood.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.blood.generic.x				= 580;
	uiPerformance.blood.generic.y				= 512;
	uiPerformance.blood.generic.width			= 198;
	uiPerformance.blood.generic.height			= 30;
	uiPerformance.blood.generic.callback		= UI_Performance_Callback;
	uiPerformance.blood.generic.statusText		= "Enable/Disable blood effects";
	uiPerformance.blood.minValue				= 0;
	uiPerformance.blood.maxValue				= 1;
	uiPerformance.blood.range					= 1;

	UI_Performance_GetConfig();

	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.background);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.banner);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.textShadow1);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.textShadow2);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.text1);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.text2);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.back);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.syncEveryFrame);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.caustics);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.particles);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.decals);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.ejectingBrass);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.shellEffects);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.screenBlends);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.blood);
}

/*
 =================
 UI_Performance_Precache
 =================
*/
void UI_Performance_Precache (void){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
}

/*
 =================
 UI_Performance_Menu
 =================
*/
void UI_Performance_Menu (void){

	UI_Performance_Precache();
	UI_Performance_Init();

	UI_Performance_UpdateConfig();

	UI_PushMenu(&uiPerformance.menu);
}
