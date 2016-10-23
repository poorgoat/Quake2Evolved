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
#define ART_BANNER				"ui/banners/advanced_t"
#define ART_TEXT1				"ui/text/advanced_text_p1"
#define ART_TEXT2				"ui/text/advanced_text_p2"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_TEXT1			2
#define ID_TEXT2			3
#define ID_TEXTSHADOW1		4
#define ID_TEXTSHADOW2		5

#define ID_CANCEL			6
#define ID_APPLY			7

#define ID_VBO				8
#define ID_TEXTURECOMPRESS	9
#define ID_MAXANISOTROPY	10

static const char	*uiAdvancedYesNo[] = {
	"False",
	"True"
};

typedef struct {
	float	vbo;
	float	textureCompress;
	float	maxAnisotropy;
} uiAdvancedValues_t;

static uiAdvancedValues_t	uiAdvancedInitial;

typedef struct {
	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			textShadow1;
	menuBitmap_s			textShadow2;
	menuBitmap_s			text1;
	menuBitmap_s			text2;

	menuBitmap_s			cancel;
	menuBitmap_s			apply;

	menuSpinControl_s		vbo;
	menuSpinControl_s		textureCompress;
	menuSpinControl_s		maxAnisotropy;
} uiAdvanced_t;

static uiAdvanced_t			uiAdvanced;


/*
 =================
 UI_Advanced_GetConfig
 =================
*/
static void UI_Advanced_GetConfig (void){

	if (Cvar_GetInteger("r_vertexBuffers"))
		uiAdvanced.vbo.curValue = 1;

	if (Cvar_GetInteger("r_compressTextures"))
		uiAdvanced.textureCompress.curValue = 1;

	uiAdvanced.maxAnisotropy.curValue = Cvar_GetFloat("r_textureAnisotropy");

	// Save initial values
	uiAdvancedInitial.vbo = uiAdvanced.vbo.curValue;
	uiAdvancedInitial.textureCompress = uiAdvanced.textureCompress.curValue;
	uiAdvancedInitial.maxAnisotropy = uiAdvanced.maxAnisotropy.curValue;
}

/*
 =================
 UI_Advanced_SetConfig
 =================
*/
static void UI_Advanced_SetConfig (void){

	Cvar_SetInteger("r_vertexBuffers", (int)uiAdvanced.vbo.curValue);

	Cvar_SetInteger("r_compressTextures", (int)uiAdvanced.textureCompress.curValue);

	Cvar_SetFloat("r_textureAnisotropy", uiAdvanced.maxAnisotropy.curValue);

	// Restart video subsystem
	Cbuf_ExecuteText(EXEC_NOW, "vid_restart\n");
}

/*
 =================
 UI_Advanced_UpdateConfig
 =================
*/
static void UI_Advanced_UpdateConfig (void){

	static char	anisotropyText[8];

	uiAdvanced.vbo.generic.name = uiAdvancedYesNo[(int)uiAdvanced.vbo.curValue];

	uiAdvanced.textureCompress.generic.name = uiAdvancedYesNo[(int)uiAdvanced.textureCompress.curValue];

	if (!uiStatic.glConfig.textureFilterAnisotropic)
		uiAdvanced.maxAnisotropy.curValue = 1.0;
	Q_snprintfz(anisotropyText, sizeof(anisotropyText), "%.1f", uiAdvanced.maxAnisotropy.curValue);
	uiAdvanced.maxAnisotropy.generic.name = anisotropyText;

	// Some settings can be updated here
	Cvar_SetInteger("r_vertexBuffers", (int)uiAdvanced.vbo.curValue);

	Cvar_SetFloat("r_textureAnisotropy", uiAdvanced.maxAnisotropy.curValue);

	// See if the apply button should be enabled or disabled
	uiAdvanced.apply.generic.flags |= QMF_GRAYED;

	if (uiAdvancedInitial.textureCompress != uiAdvanced.textureCompress.curValue){
		uiAdvanced.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}
}

/*
 =================
 UI_Advanced_Callback
 =================
*/
static void UI_Advanced_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event == QM_CHANGED){
		UI_Advanced_UpdateConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_APPLY:
		UI_Advanced_SetConfig();
		break;
	}
}

/*
 =================
 UI_Advanced_Ownerdraw
 =================
*/
static void UI_Advanced_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (uiAdvanced.menu.items[uiAdvanced.menu.cursor] == self)
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
 UI_Advanced_Init
 =================
*/
static void UI_Advanced_Init (void){

	memset(&uiAdvanced, 0, sizeof(uiAdvanced_t));

	uiAdvanced.background.generic.id			= ID_BACKGROUND;
	uiAdvanced.background.generic.type			= QMTYPE_BITMAP;
	uiAdvanced.background.generic.flags			= QMF_INACTIVE;
	uiAdvanced.background.generic.x				= 0;
	uiAdvanced.background.generic.y				= 0;
	uiAdvanced.background.generic.width			= 1024;
	uiAdvanced.background.generic.height		= 768;
	uiAdvanced.background.pic					= ART_BACKGROUND;

	uiAdvanced.banner.generic.id				= ID_BANNER;
	uiAdvanced.banner.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.banner.generic.flags				= QMF_INACTIVE;
	uiAdvanced.banner.generic.x					= 0;
	uiAdvanced.banner.generic.y					= 66;
	uiAdvanced.banner.generic.width				= 1024;
	uiAdvanced.banner.generic.height			= 46;
	uiAdvanced.banner.pic						= ART_BANNER;

	uiAdvanced.textShadow1.generic.id			= ID_TEXTSHADOW1;
	uiAdvanced.textShadow1.generic.type			= QMTYPE_BITMAP;
	uiAdvanced.textShadow1.generic.flags		= QMF_INACTIVE;
	uiAdvanced.textShadow1.generic.x			= 182;
	uiAdvanced.textShadow1.generic.y			= 170;
	uiAdvanced.textShadow1.generic.width		= 256;
	uiAdvanced.textShadow1.generic.height		= 256;
	uiAdvanced.textShadow1.generic.color		= uiColorBlack;
	uiAdvanced.textShadow1.pic					= ART_TEXT1;

	uiAdvanced.textShadow2.generic.id			= ID_TEXTSHADOW2;
	uiAdvanced.textShadow2.generic.type			= QMTYPE_BITMAP;
	uiAdvanced.textShadow2.generic.flags		= QMF_INACTIVE;
	uiAdvanced.textShadow2.generic.x			= 182;
	uiAdvanced.textShadow2.generic.y			= 426;
	uiAdvanced.textShadow2.generic.width		= 256;
	uiAdvanced.textShadow2.generic.height		= 256;
	uiAdvanced.textShadow2.generic.color		= uiColorBlack;
	uiAdvanced.textShadow2.pic					= ART_TEXT2;

	uiAdvanced.text1.generic.id					= ID_TEXT1;
	uiAdvanced.text1.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.text1.generic.flags				= QMF_INACTIVE;
	uiAdvanced.text1.generic.x					= 180;
	uiAdvanced.text1.generic.y					= 168;
	uiAdvanced.text1.generic.width				= 256;
	uiAdvanced.text1.generic.height				= 256;
	uiAdvanced.text1.pic						= ART_TEXT1;

	uiAdvanced.text2.generic.id					= ID_TEXT2;
	uiAdvanced.text2.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.text2.generic.flags				= QMF_INACTIVE;
	uiAdvanced.text2.generic.x					= 180;
	uiAdvanced.text2.generic.y					= 424;
	uiAdvanced.text2.generic.width				= 256;
	uiAdvanced.text2.generic.height				= 256;
	uiAdvanced.text2.pic						= ART_TEXT2;

	uiAdvanced.cancel.generic.id				= ID_CANCEL;
	uiAdvanced.cancel.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.cancel.generic.x					= 310;
	uiAdvanced.cancel.generic.y					= 656;
	uiAdvanced.cancel.generic.width				= 198;
	uiAdvanced.cancel.generic.height			= 38;
	uiAdvanced.cancel.generic.callback			= UI_Advanced_Callback;
	uiAdvanced.cancel.generic.ownerdraw			= UI_Advanced_Ownerdraw;
	uiAdvanced.cancel.pic						= UI_CANCELBUTTON;

	uiAdvanced.apply.generic.id					= ID_APPLY;
	uiAdvanced.apply.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.apply.generic.x					= 516;
	uiAdvanced.apply.generic.y					= 656;
	uiAdvanced.apply.generic.width				= 198;
	uiAdvanced.apply.generic.height				= 38;
	uiAdvanced.apply.generic.callback			= UI_Advanced_Callback;
	uiAdvanced.apply.generic.ownerdraw			= UI_Advanced_Ownerdraw;
	uiAdvanced.apply.pic						= UI_APPLYBUTTON;

	uiAdvanced.vbo.generic.id					= ID_VBO;
	uiAdvanced.vbo.generic.type					= QMTYPE_SPINCONTROL;
	uiAdvanced.vbo.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAdvanced.vbo.generic.x					= 580;
	uiAdvanced.vbo.generic.y					= 192;
	uiAdvanced.vbo.generic.width				= 198;
	uiAdvanced.vbo.generic.height				= 30;
	uiAdvanced.vbo.generic.callback				= UI_Advanced_Callback;
	uiAdvanced.vbo.generic.statusText			= "Allow geometry to be stored in fast video or AGP memory";
	uiAdvanced.vbo.minValue						= 0;
	uiAdvanced.vbo.maxValue						= 1;
	uiAdvanced.vbo.range						= 1;

	uiAdvanced.textureCompress.generic.id		= ID_TEXTURECOMPRESS;
	uiAdvanced.textureCompress.generic.type		= QMTYPE_SPINCONTROL;
	uiAdvanced.textureCompress.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAdvanced.textureCompress.generic.x		= 580;
	uiAdvanced.textureCompress.generic.y		= 448;
	uiAdvanced.textureCompress.generic.width	= 198;
	uiAdvanced.textureCompress.generic.height	= 30;
	uiAdvanced.textureCompress.generic.callback	= UI_Advanced_Callback;
	uiAdvanced.textureCompress.generic.statusText	= "Allow texture compression to save texture memory";
	uiAdvanced.textureCompress.minValue			= 0;
	uiAdvanced.textureCompress.maxValue			= 1;
	uiAdvanced.textureCompress.range			= 1;

	uiAdvanced.maxAnisotropy.generic.id			= ID_MAXANISOTROPY;
	uiAdvanced.maxAnisotropy.generic.type		= QMTYPE_SPINCONTROL;
	uiAdvanced.maxAnisotropy.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAdvanced.maxAnisotropy.generic.x			= 580;
	uiAdvanced.maxAnisotropy.generic.y			= 512;
	uiAdvanced.maxAnisotropy.generic.width		= 198;
	uiAdvanced.maxAnisotropy.generic.height		= 30;
	uiAdvanced.maxAnisotropy.generic.callback	= UI_Advanced_Callback;
	uiAdvanced.maxAnisotropy.generic.statusText	= "Set max anisotropy filter level (Higher values = Better quality)";
	uiAdvanced.maxAnisotropy.minValue			= 1.0;
	uiAdvanced.maxAnisotropy.maxValue			= uiStatic.glConfig.maxTextureMaxAnisotropy;
	uiAdvanced.maxAnisotropy.range				= 1.0;

	UI_Advanced_GetConfig();

	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.background);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.banner);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.textShadow1);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.textShadow2);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.text1);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.text2);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.cancel);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.apply);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.vbo);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.textureCompress);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.maxAnisotropy);
}

/*
 =================
 UI_Advanced_Precache
 =================
*/
void UI_Advanced_Precache (void){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
}

/*
 =================
 UI_Advanced_Menu
 =================
*/
void UI_Advanced_Menu (void){

	UI_Advanced_Precache();
	UI_Advanced_Init();

	UI_Advanced_UpdateConfig();

	UI_PushMenu(&uiAdvanced.menu);
}
