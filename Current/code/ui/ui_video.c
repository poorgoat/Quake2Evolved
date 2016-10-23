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
#define ART_BANNER				"ui/banners/video_t"
#define ART_TEXT1				"ui/text/video_text_p1"
#define ART_TEXT2				"ui/text/video_text_p2"
#define ART_ADVANCED			"ui/buttons/advanced_b"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_TEXT1			2
#define ID_TEXT2			3
#define ID_TEXTSHADOW1		4
#define ID_TEXTSHADOW2		5

#define ID_CANCEL			6
#define ID_ADVANCED			7
#define ID_APPLY			8

#define ID_VIDEOMODE		9
#define ID_FULLSCREEN		10
#define ID_DISPLAYFREQUENCY	11
#define ID_GAMMA			12
#define ID_TEXTUREDETAIL	13
#define ID_TEXTUREFILTER	14

static const char	*uiVideoYesNo[] = {
	"False",
	"True"
};
static const char	*uiVideoModes[] = {
	"Custom",
	"320 x 240",
	"400 x 300",
	"512 x 384",
	"640 x 480",
	"800 x 600",
	"960 x 720",
	"1024 x 768",
	"1152 x 864",
	"1280 x 1024",
	"1600 x 1200",
	"2048 x 1536",
	"856 x 480 (wide)",
	"1920 x 1200 (wide)"
};
static const char	*uiVideoTextureFilters[] = {
	"Bilinear",
	"Trilinear"
};

typedef struct {
	float	videoMode;
	float	fullScreen;
	float	displayFrequency;
	float	gamma;
	float	textureDetail;
	float	textureFilter;
} uiVideoValues_t;

static uiVideoValues_t		uiVideoInitial;

typedef struct {
	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			textShadow1;
	menuBitmap_s			textShadow2;
	menuBitmap_s			text1;
	menuBitmap_s			text2;

	menuBitmap_s			cancel;
	menuBitmap_s			advanced;
	menuBitmap_s			apply;

	menuSpinControl_s		videoMode;
	menuSpinControl_s		fullScreen;
	menuField_s				displayFrequency;
	menuSpinControl_s		gamma;
	menuSpinControl_s		textureDetail;
	menuSpinControl_s		textureFilter;
} uiVideo_t;

static uiVideo_t			uiVideo;


/*
 =================
 UI_Video_GetConfig
 =================
*/
static void UI_Video_GetConfig (void){

	uiVideo.videoMode.curValue = Cvar_GetInteger("r_mode");

	if (Cvar_GetInteger("r_fullscreen"))
		uiVideo.fullScreen.curValue = 1;

	Q_snprintfz(uiVideo.displayFrequency.buffer, sizeof(uiVideo.displayFrequency.buffer), "%i", Cvar_GetInteger("r_displayRefresh"));

	uiVideo.gamma.curValue = Cvar_GetFloat("r_gamma");

	if (Cvar_GetInteger("r_downSizeTextures"))
		uiVideo.textureDetail.curValue = 1;

	if (!Q_stricmp(Cvar_GetString("r_textureFilter"), "GL_LINEAR_MIPMAP_LINEAR"))
		uiVideo.textureFilter.curValue = 1;

	// Save initial values
	uiVideoInitial.videoMode = uiVideo.videoMode.curValue;
	uiVideoInitial.fullScreen = uiVideo.fullScreen.curValue;
	uiVideoInitial.displayFrequency = atoi(uiVideo.displayFrequency.buffer);
	uiVideoInitial.gamma = uiVideo.gamma.curValue;
	uiVideoInitial.textureDetail = uiVideo.textureDetail.curValue;
	uiVideoInitial.textureFilter = uiVideo.textureFilter.curValue;
}

/*
 =================
 UI_Video_SetConfig
 =================
*/
static void UI_Video_SetConfig (void){

	Cvar_SetInteger("r_mode", (int)uiVideo.videoMode.curValue);

	Cvar_SetInteger("r_fullscreen", (int)uiVideo.fullScreen.curValue);

	Cvar_SetInteger("r_displayRefresh", atoi(uiVideo.displayFrequency.buffer));

	Cvar_SetFloat("r_gamma", uiVideo.gamma.curValue);

	Cvar_SetInteger("r_downSizeTextures", (int)uiVideo.textureDetail.curValue);

	if ((int)uiVideo.textureFilter.curValue == 1)
		Cvar_SetString("r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR");
	else
		Cvar_SetString("r_textureFilter", "GL_LINEAR_MIPMAP_NEAREST");

	// Restart video subsystem
	Cbuf_ExecuteText(EXEC_NOW, "vid_restart\n");
}

/*
 =================
 UI_Video_UpdateConfig
 =================
*/
static void UI_Video_UpdateConfig (void){

	static char	gammaText[8];

	uiVideo.videoMode.generic.name = uiVideoModes[(int)uiVideo.videoMode.curValue + 1];

	uiVideo.fullScreen.generic.name = uiVideoYesNo[(int)uiVideo.fullScreen.curValue];

	Q_snprintfz(gammaText, sizeof(gammaText), "%.2f", uiVideo.gamma.curValue);
	uiVideo.gamma.generic.name = gammaText;

	uiVideo.textureDetail.generic.name = uiVideoYesNo[(int)uiVideo.textureDetail.curValue];

	uiVideo.textureFilter.generic.name = uiVideoTextureFilters[(int)uiVideo.textureFilter.curValue];

	// Some settings can be updated here
	if (uiStatic.glConfig.deviceSupportsGamma)
		Cvar_SetFloat("r_gamma", uiVideo.gamma.curValue);

	if ((int)uiVideo.textureFilter.curValue == 1)
		Cvar_SetString("r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR");
	else
		Cvar_SetString("r_textureFilter", "GL_LINEAR_MIPMAP_NEAREST");

	// See if the apply button should be enabled or disabled
	uiVideo.apply.generic.flags |= QMF_GRAYED;

	if (uiVideoInitial.videoMode != uiVideo.videoMode.curValue){
		uiVideo.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}
	if (uiVideoInitial.fullScreen != uiVideo.fullScreen.curValue){
		uiVideo.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}
	if (uiVideoInitial.displayFrequency != atoi(uiVideo.displayFrequency.buffer)){
		uiVideo.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}
	if (uiVideoInitial.gamma != uiVideo.gamma.curValue){
		uiVideo.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}
	if (uiVideoInitial.textureDetail != uiVideo.textureDetail.curValue){
		uiVideo.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}
}

/*
 =================
 UI_Video_Callback
 =================
*/
static void UI_Video_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event == QM_CHANGED){
		UI_Video_UpdateConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_ADVANCED:
		UI_Advanced_Menu();
		break;
	case ID_APPLY:
		UI_Video_SetConfig();
		break;
	}
}

/*
 =================
 UI_Video_Ownerdraw
 =================
*/
static void UI_Video_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (uiVideo.menu.items[uiVideo.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

	if (item->flags & QMF_GRAYED)
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorDkGrey, ((menuBitmap_s *)self)->pic);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
}

/*
 =================
 UI_Video_Init
 =================
*/
static void UI_Video_Init (void){

	memset(&uiVideo, 0, sizeof(uiVideo_t));

	uiVideo.background.generic.id				= ID_BACKGROUND;
	uiVideo.background.generic.type				= QMTYPE_BITMAP;
	uiVideo.background.generic.flags			= QMF_INACTIVE;
	uiVideo.background.generic.x				= 0;
	uiVideo.background.generic.y				= 0;
	uiVideo.background.generic.width			= 1024;
	uiVideo.background.generic.height			= 768;
	uiVideo.background.pic						= ART_BACKGROUND;

	uiVideo.banner.generic.id					= ID_BANNER;
	uiVideo.banner.generic.type					= QMTYPE_BITMAP;
	uiVideo.banner.generic.flags				= QMF_INACTIVE;
	uiVideo.banner.generic.x					= 0;
	uiVideo.banner.generic.y					= 66;
	uiVideo.banner.generic.width				= 1024;
	uiVideo.banner.generic.height				= 46;
	uiVideo.banner.pic							= ART_BANNER;

	uiVideo.textShadow1.generic.id				= ID_TEXTSHADOW1;
	uiVideo.textShadow1.generic.type			= QMTYPE_BITMAP;
	uiVideo.textShadow1.generic.flags			= QMF_INACTIVE;
	uiVideo.textShadow1.generic.x				= 182;
	uiVideo.textShadow1.generic.y				= 170;
	uiVideo.textShadow1.generic.width			= 256;
	uiVideo.textShadow1.generic.height			= 256;
	uiVideo.textShadow1.generic.color			= uiColorBlack;
	uiVideo.textShadow1.pic						= ART_TEXT1;

	uiVideo.textShadow2.generic.id				= ID_TEXTSHADOW2;
	uiVideo.textShadow2.generic.type			= QMTYPE_BITMAP;
	uiVideo.textShadow2.generic.flags			= QMF_INACTIVE;
	uiVideo.textShadow2.generic.x				= 182;
	uiVideo.textShadow2.generic.y				= 426;
	uiVideo.textShadow2.generic.width			= 256;
	uiVideo.textShadow2.generic.height			= 256;
	uiVideo.textShadow2.generic.color			= uiColorBlack;
	uiVideo.textShadow2.pic						= ART_TEXT2;

	uiVideo.text1.generic.id					= ID_TEXT1;
	uiVideo.text1.generic.type					= QMTYPE_BITMAP;
	uiVideo.text1.generic.flags					= QMF_INACTIVE;
	uiVideo.text1.generic.x						= 180;
	uiVideo.text1.generic.y						= 168;
	uiVideo.text1.generic.width					= 256;
	uiVideo.text1.generic.height				= 256;
	uiVideo.text1.pic							= ART_TEXT1;

	uiVideo.text2.generic.id					= ID_TEXT2;
	uiVideo.text2.generic.type					= QMTYPE_BITMAP;
	uiVideo.text2.generic.flags					= QMF_INACTIVE;
	uiVideo.text2.generic.x						= 180;
	uiVideo.text2.generic.y						= 424;
	uiVideo.text2.generic.width					= 256;
	uiVideo.text2.generic.height				= 256;
	uiVideo.text2.pic							= ART_TEXT2;

	uiVideo.cancel.generic.id					= ID_CANCEL;
	uiVideo.cancel.generic.type					= QMTYPE_BITMAP;
	uiVideo.cancel.generic.x					= 206;
	uiVideo.cancel.generic.y					= 656;
	uiVideo.cancel.generic.width				= 198;
	uiVideo.cancel.generic.height				= 38;
	uiVideo.cancel.generic.callback				= UI_Video_Callback;
	uiVideo.cancel.generic.ownerdraw			= UI_Video_Ownerdraw;
	uiVideo.cancel.pic							= UI_CANCELBUTTON;

	uiVideo.advanced.generic.id					= ID_ADVANCED;
	uiVideo.advanced.generic.type				= QMTYPE_BITMAP;
	uiVideo.advanced.generic.x					= 413;
	uiVideo.advanced.generic.y					= 656;
	uiVideo.advanced.generic.width				= 198;
	uiVideo.advanced.generic.height				= 38;
	uiVideo.advanced.generic.callback			= UI_Video_Callback;
	uiVideo.advanced.generic.ownerdraw			= UI_Video_Ownerdraw;
	uiVideo.advanced.pic						= ART_ADVANCED;

	uiVideo.apply.generic.id					= ID_APPLY;
	uiVideo.apply.generic.type					= QMTYPE_BITMAP;
	uiVideo.apply.generic.x						= 620;
	uiVideo.apply.generic.y						= 656;
	uiVideo.apply.generic.width					= 198;
	uiVideo.apply.generic.height				= 38;
	uiVideo.apply.generic.callback				= UI_Video_Callback;
	uiVideo.apply.generic.ownerdraw				= UI_Video_Ownerdraw;
	uiVideo.apply.pic							= UI_APPLYBUTTON;

	uiVideo.videoMode.generic.id				= ID_VIDEOMODE;
	uiVideo.videoMode.generic.type				= QMTYPE_SPINCONTROL;
	uiVideo.videoMode.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiVideo.videoMode.generic.x					= 580;
	uiVideo.videoMode.generic.y					= 224;
	uiVideo.videoMode.generic.width				= 198;
	uiVideo.videoMode.generic.height			= 30;
	uiVideo.videoMode.generic.callback			= UI_Video_Callback;
	uiVideo.videoMode.generic.statusText		= "Set your game resolution";
	uiVideo.videoMode.minValue					= -1;
	uiVideo.videoMode.maxValue					= 12;
	uiVideo.videoMode.range						= 1;
	
	uiVideo.fullScreen.generic.id				= ID_FULLSCREEN;
	uiVideo.fullScreen.generic.type				= QMTYPE_SPINCONTROL;
	uiVideo.fullScreen.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiVideo.fullScreen.generic.x				= 580;
	uiVideo.fullScreen.generic.y				= 256;
	uiVideo.fullScreen.generic.width			= 198;
	uiVideo.fullScreen.generic.height			= 30;
	uiVideo.fullScreen.generic.callback			= UI_Video_Callback;
	uiVideo.fullScreen.generic.statusText		= "Switch between fullscreen and windowed mode";
	uiVideo.fullScreen.minValue					= 0;
	uiVideo.fullScreen.maxValue					= 1;
	uiVideo.fullScreen.range					= 1;

	uiVideo.displayFrequency.generic.id			= ID_DISPLAYFREQUENCY;
	uiVideo.displayFrequency.generic.type		= QMTYPE_FIELD;
	uiVideo.displayFrequency.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY;
	uiVideo.displayFrequency.generic.x			= 580;
	uiVideo.displayFrequency.generic.y			= 288;
	uiVideo.displayFrequency.generic.width		= 198;
	uiVideo.displayFrequency.generic.height		= 30;
	uiVideo.displayFrequency.generic.callback	= UI_Video_Callback;
	uiVideo.displayFrequency.generic.statusText	= "Set your monitor refresh rate (0 = Disabled)";
	uiVideo.displayFrequency.maxLenght			= 3;

	uiVideo.gamma.generic.id					= ID_GAMMA;
	uiVideo.gamma.generic.type					= QMTYPE_SPINCONTROL;
	uiVideo.gamma.generic.flags					= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiVideo.gamma.generic.x						= 580;
	uiVideo.gamma.generic.y						= 384;
	uiVideo.gamma.generic.width					= 198;
	uiVideo.gamma.generic.height				= 30;
	uiVideo.gamma.generic.callback				= UI_Video_Callback;
	uiVideo.gamma.generic.statusText			= "Set display device gamma level";
	uiVideo.gamma.minValue						= 0.5;
	uiVideo.gamma.maxValue						= 3.0;
	uiVideo.gamma.range							= 0.25;

	uiVideo.textureDetail.generic.id			= ID_TEXTUREDETAIL;
	uiVideo.textureDetail.generic.type			= QMTYPE_SPINCONTROL;
	uiVideo.textureDetail.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiVideo.textureDetail.generic.x				= 580;
	uiVideo.textureDetail.generic.y				= 480;
	uiVideo.textureDetail.generic.width			= 198;
	uiVideo.textureDetail.generic.height		= 30;
	uiVideo.textureDetail.generic.callback		= UI_Video_Callback;
	uiVideo.textureDetail.generic.statusText	= "Set texture detail level";
	uiVideo.textureDetail.minValue				= 0;
	uiVideo.textureDetail.maxValue				= 1;
	uiVideo.textureDetail.range					= 1;

	uiVideo.textureFilter.generic.id			= ID_TEXTUREFILTER;
	uiVideo.textureFilter.generic.type			= QMTYPE_SPINCONTROL;
	uiVideo.textureFilter.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiVideo.textureFilter.generic.x				= 580;
	uiVideo.textureFilter.generic.y				= 544;
	uiVideo.textureFilter.generic.width			= 198;
	uiVideo.textureFilter.generic.height		= 30;
	uiVideo.textureFilter.generic.callback		= UI_Video_Callback;
	uiVideo.textureFilter.generic.statusText	= "Set texture filter (Trilinear = Better quality)";
	uiVideo.textureFilter.minValue				= 0;
	uiVideo.textureFilter.maxValue				= 1;
	uiVideo.textureFilter.range					= 1;

	UI_Video_GetConfig();

	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.background);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.banner);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.textShadow1);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.textShadow2);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.text1);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.text2);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.cancel);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.advanced);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.apply);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.videoMode);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.fullScreen);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.displayFrequency);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.gamma);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.textureDetail);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.textureFilter);
}

/*
 =================
 UI_Video_Precache
 =================
*/
void UI_Video_Precache (void){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
	R_RegisterMaterialNoMip(ART_ADVANCED);
}

/*
 =================
 UI_Video_Menu
 =================
*/
void UI_Video_Menu (void){

	UI_Video_Precache();
	UI_Video_Init();

	UI_Video_UpdateConfig();

	UI_PushMenu(&uiVideo.menu);
}
