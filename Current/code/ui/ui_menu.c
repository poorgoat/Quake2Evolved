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


// ui_menu.c -- main menu interface


#include "ui_local.h"


cvar_t			*ui_precache;
cvar_t			*ui_sensitivity;
cvar_t			*ui_singlePlayerSkill;

uiStatic_t		uiStatic;

const char		*uiSoundIn		= "misc/menu1.wav";
const char		*uiSoundMove	= "misc/menu2.wav";
const char		*uiSoundOut		= "misc/menu3.wav";
const char		*uiSoundBuzz	= "misc/menu4.wav";
const char		*uiSoundNull	= "";

color_t			uiColorWhite	= {255, 255, 255, 255};
color_t			uiColorLtGrey	= {192, 192, 192, 255};
color_t			uiColorMdGrey	= {127, 127, 127, 255};
color_t			uiColorDkGrey	= { 64,  64,  64, 255};
color_t			uiColorBlack	= {  0,   0,   0, 255};
color_t			uiColorRed		= {255,   0,   0, 255};
color_t			uiColorGreen	= {  0, 255,   0, 255};
color_t			uiColorBlue		= {  0,   0, 255, 255};
color_t			uiColorYellow	= {255, 255,   0, 255};
color_t			uiColorCyan		= {  0, 255, 255, 255};
color_t			uiColorMagenta	= {255,   0, 255, 255};


/*
 =================
 UI_ScaleCoords

 Any parameter can be NULL if you don't want it
 =================
*/
void UI_ScaleCoords (int *x, int *y, int *w, int *h){

	if (x)
		*x *= uiStatic.scaleX;
	if (y)
		*y *= uiStatic.scaleY;
	if (w)
		*w *= uiStatic.scaleX;
	if (h)
		*h *= uiStatic.scaleY;
}

/*
 =================
 UI_CursorInRect
 =================
*/
qboolean UI_CursorInRect (int x, int y, int w, int h){

	if (uiStatic.cursorX < x)
		return false;
	if (uiStatic.cursorX > x+w)
		return false;
	if (uiStatic.cursorY < y)
		return false;
	if (uiStatic.cursorY > y+h)
		return false;

	return true;
}

/*
 =================
 UI_DrawPic
 =================
*/
void UI_DrawPic (int x, int y, int w, int h, const color_t color, const char *pic){

	struct material_s	*material;

	if (!pic || !pic[0])
		return;

	material = R_RegisterMaterialNoMip(pic);
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, material);
}

/*
 =================
 UI_FillRect
 =================
*/
void UI_FillRect (int x, int y, int w, int h, const color_t color){

	struct material_s	*material;

	material = R_RegisterMaterialNoMip("white");
	R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, color, material);
}

/*
 =================
 UI_DrawString
 =================
*/
void UI_DrawString (int x, int y, int w, int h, const char *string, const color_t color, qboolean forceColor, int charW, int charH, int justify, qboolean shadow){

	color_t				modulate, shadowModulate;
	char				line[1024], *l;
	int					xx, yy, ofsX, ofsY;
	int					i, len, ch;
	float				col, row;
	struct material_s	*material;

	if (!string || !string[0])
		return;

	material = R_RegisterMaterialNoMip("charset");

	// Vertically centered
	if (!strchr(string, '\n') && !strchr(string, '\r'))
		y = y + ((h - charH) / 2);

	if (shadow){
		MakeRGBA(shadowModulate, 0, 0, 0, color[3]);

		ofsX = charW / 8;
		ofsY = charH / 8;
	}

	*(unsigned *)modulate = *(unsigned *)color;

	yy = y;
	while (*string){
		// Get a line of text
		len = 0;
		while (*string){
			if (*string == '\n' || *string == '\r'){
				string++;
				break;
			}
			else if (*string == '\t'){
				for (i = 0; i < 4; i++){
					line[len++] = ' ';

					if (len == sizeof(line)-1)
						break;
				}

				string++;
			}
			else
				line[len++] = *string++;

			if (len == sizeof(line)-1)
				break;
		}
		line[len] = 0;

		if (!len){
			yy += h;
			continue;
		}

		// Align the text as appropriate
		if (justify == 0)
			xx = x;
		if (justify == 1)
			xx = x + ((w - (Q_PrintStrlen(line) * charW)) / 2);
		if (justify == 2)
			xx = x + (w - (Q_PrintStrlen(line) * charW));

		// Draw it
		l = line;
		while (*l){
			if (Q_IsColorString(l)){
				if (!forceColor){
					*(unsigned *)modulate = *(unsigned *)colorTable[Q_ColorIndex(*(l+1))];
					modulate[3] = color[3];
				}

				l += 2;
				continue;
			}

			ch = *l++;

			ch &= 255;
			if (ch != ' '){
				col = (ch & 15) * 0.0625;
				row = (ch >> 4) * 0.0625;

				if (shadow)
					R_DrawStretchPic(xx + ofsX, yy + ofsY, charW, charH, col, row, col + 0.0625, row + 0.0625, shadowModulate, material);

				R_DrawStretchPic(xx, yy, charW, charH, col, row, col + 0.0625, row + 0.0625, modulate, material);
			}

			xx += charW;
		}

		yy += charH;
	}
}

/*
 =================
 UI_DrawMouseCursor
 =================
*/
void UI_DrawMouseCursor (void){

	struct material_s	*material = NULL;
	menuCommon_s		*item;
	int					i;
	int					w = UI_CURSOR_SIZE, h = UI_CURSOR_SIZE;

	UI_ScaleCoords(NULL, NULL, &w, &h);

	for (i = 0; i < uiStatic.menuActive->numItems; i++){
		item = (menuCommon_s *)uiStatic.menuActive->items[i];

		if (item->flags & (QMF_INACTIVE | QMF_HIDDEN))
			continue;

		if (!UI_CursorInRect(item->x, item->y, item->width, item->height))
			continue;

		if (item->flags & QMF_GRAYED)
			material = R_RegisterMaterialNoMip(UI_CURSOR_DISABLED);
		else {
			if (item->type == QMTYPE_FIELD)
				material = R_RegisterMaterialNoMip(UI_CURSOR_TYPING);
		}

		break;
	}

	if (!material)
		material = R_RegisterMaterialNoMip(UI_CURSOR_NORMAL);

	R_DrawStretchPic(uiStatic.cursorX, uiStatic.cursorY, w, h, 0, 0, 1, 1, colorTable[COLOR_WHITE], material);
}

/*
 =================
 UI_StartSound
 =================
*/
void UI_StartSound (const char *sound){

	struct sfx_s	*sfx;

	sfx = S_RegisterSound(sound);
	S_StartLocalSound(sfx);
}


// =====================================================================


/*
 =================
 UI_AddItem
 =================
*/
void UI_AddItem (menuFramework_s *menu, void *item){

	menuCommon_s	*generic = (menuCommon_s *)item;

	if (menu->numItems >= UI_MAX_MENUITEMS)
		Com_Error(ERR_FATAL, "UI_AddItem: UI_MAX_MENUITEMS hit");

	menu->items[menu->numItems] = item;
	((menuCommon_s *)menu->items[menu->numItems])->parent = menu;
	((menuCommon_s *)menu->items[menu->numItems])->flags &= ~QMF_HASMOUSEFOCUS;
	menu->numItems++;

	switch (generic->type){
	case QMTYPE_SCROLLLIST:
		UI_ScrollList_Init((menuScrollList_s *)item);
		break;
	case QMTYPE_SPINCONTROL:
		UI_SpinControl_Init((menuSpinControl_s *)item);
		break;
	case QMTYPE_FIELD:
		UI_Field_Init((menuField_s *)item);
		break;
	case QMTYPE_ACTION:
		UI_Action_Init((menuAction_s *)item);
		break;
	case QMTYPE_BITMAP:
		UI_Bitmap_Init((menuBitmap_s *)item);
		break;
	default:
		Com_Error(ERR_FATAL, "UI_AddItem: unknown item type (%i)\n", generic->type);
	}
}

/*
 =================
 UI_CursorMoved
 =================
*/
void UI_CursorMoved (menuFramework_s *menu){

	void (*callback) (void *self, int event);

	if (menu->cursor == menu->cursorPrev)
		return;

	if (menu->cursorPrev >= 0 && menu->cursorPrev < menu->numItems){
		callback = ((menuCommon_s *)menu->items[menu->cursorPrev])->callback;
		if (callback)
			callback(menu->items[menu->cursorPrev], QM_LOSTFOCUS);
	}

	if (menu->cursor >= 0 && menu->cursor < menu->numItems){
		callback = ((menuCommon_s *)menu->items[menu->cursor])->callback;
		if (callback)
			callback(menu->items[menu->cursor], QM_GOTFOCUS);
	}
}

/*
 =================
 UI_SetCursor
 =================
*/
void UI_SetCursor (menuFramework_s *menu, int cursor){

	if (((menuCommon_s *)(menu->items[cursor]))->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN))
		return;

	menu->cursorPrev = menu->cursor;
	menu->cursor = cursor;

	UI_CursorMoved(menu);
}

/*
 =================
 UI_SetCursorToItem
 =================
*/
void UI_SetCursorToItem (menuFramework_s *menu, void *item){

	int i;

	for (i = 0; i < menu->numItems; i++){
		if (menu->items[i] == item){
			UI_SetCursor(menu, i);
			return;
		}
	}
}

/*
 =================
 UI_ItemAtCursor
 =================
*/
void *UI_ItemAtCursor (menuFramework_s *menu){

	if (menu->cursor < 0 || menu->cursor >= menu->numItems)
		return 0;

	return menu->items[menu->cursor];
}

/*
 =================
 UI_AdjustCursor

 This functiont takes the given menu, the direction, and attempts to
 adjust the menu's cursor so that it's at the next available slot
 =================
*/
void UI_AdjustCursor (menuFramework_s *menu, int dir){

	menuCommon_s	*item;
	qboolean		wrapped = false;

wrap:
	while (menu->cursor >= 0 && menu->cursor < menu->numItems){
		item = (menuCommon_s *)menu->items[menu->cursor];
		if (item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN | QMF_MOUSEONLY))
			menu->cursor += dir;
		else
			break;
	}

	if (dir == 1){
		if (menu->cursor >= menu->numItems){
			if (wrapped){
				menu->cursor = menu->cursorPrev;
				return;
			}
			menu->cursor = 0;
			wrapped = true;
			goto wrap;
		}
	}
	else if (dir == -1){
		if (menu->cursor < 0){
			if (wrapped){
				menu->cursor = menu->cursorPrev;
				return;
			}
			menu->cursor = menu->numItems - 1;
			wrapped = true;
			goto wrap;
		}
	}
}

/*
 =================
 UI_DrawMenu
 =================
*/
void UI_DrawMenu (menuFramework_s *menu){

	static int			statusFadeTime;
	static menuCommon_s	*lastItem;
	color_t				color = {255, 255, 255, 255};
	int					i;
	menuCommon_s		*item;

	// Draw contents
	for (i = 0; i < menu->numItems; i++){
		item = (menuCommon_s *)menu->items[i];

		if (item->flags & QMF_HIDDEN)
			continue;

		if (item->ownerdraw){
			// Total subclassing, owner draws everything
			item->ownerdraw(item);
			continue;
		}

		switch (item->type){
		case QMTYPE_SCROLLLIST:
			UI_ScrollList_Draw((menuScrollList_s *)item);
			break;
		case QMTYPE_SPINCONTROL:
			UI_SpinControl_Draw((menuSpinControl_s *)item);
			break;
		case QMTYPE_FIELD:
			UI_Field_Draw((menuField_s *)item);
			break;
		case QMTYPE_ACTION:
			UI_Action_Draw((menuAction_s *)item);
			break;
		case QMTYPE_BITMAP:
			UI_Bitmap_Draw((menuBitmap_s *)item);
			break;
		}
	}

	// Draw status bar
	item = UI_ItemAtCursor(menu);
	if (item != lastItem){
		statusFadeTime = uiStatic.realTime;
		lastItem = item;
	}

	if (item && (item->flags & QMF_HASMOUSEFOCUS) && (item->statusText != NULL)){
		// Fade it in, but wait a second
		color[3] = Clamp((float)((uiStatic.realTime - statusFadeTime) - 1000) / 1000, 0.0, 1.0) * 255;

		UI_DrawString(0, 720*uiStatic.scaleY, 1024*uiStatic.scaleX, 28*uiStatic.scaleY, item->statusText, color, true, UI_SMALL_CHAR_WIDTH*uiStatic.scaleX, UI_SMALL_CHAR_HEIGHT*uiStatic.scaleY, 1, true);
	}
	else
		statusFadeTime = uiStatic.realTime;
}

/*
 =================
 UI_DefaultKey
 =================
*/
const char *UI_DefaultKey (menuFramework_s *menu, int key){

	const char		*sound = 0;
	menuCommon_s	*item;
	int				cursorPrev;

	// Menu system key
	if (key == K_ESCAPE || key == K_MOUSE2){
		UI_PopMenu();
		return uiSoundOut;
	}

	if (!menu || !menu->numItems)
		return 0;

	item = UI_ItemAtCursor(menu);
	if (item && !(item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN))){
		switch (item->type){
		case QMTYPE_SCROLLLIST:
			sound = UI_ScrollList_Key((menuScrollList_s *)item, key);
			break;
		case QMTYPE_SPINCONTROL:
			sound = UI_SpinControl_Key((menuSpinControl_s *)item, key);
			break;
		case QMTYPE_FIELD:
			sound = UI_Field_Key((menuField_s *)item, key);
			break;
		case QMTYPE_ACTION:
			sound = UI_Action_Key((menuAction_s *)item, key);
			break;
		case QMTYPE_BITMAP:
			sound = UI_Bitmap_Key((menuBitmap_s *)item, key);
			break;
		}

		if (sound)
			return sound;	// Key was handled
	}

	// Default handling
	switch (key){
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		cursorPrev = menu->cursor;
		menu->cursorPrev = menu->cursor;
		menu->cursor--;

		UI_AdjustCursor(menu, -1);
		if (cursorPrev != menu->cursor){
			UI_CursorMoved(menu);
			if (!(((menuCommon_s *)menu->items[menu->cursor])->flags & QMF_SILENT))
				sound = uiSoundMove;
		}

		break;
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
	case K_TAB:
		cursorPrev = menu->cursor;
		menu->cursorPrev = menu->cursor;
		menu->cursor++;

		UI_AdjustCursor(menu, 1);
		if (cursorPrev != menu->cursor){
			UI_CursorMoved(menu);
			if (!(((menuCommon_s *)menu->items[menu->cursor])->flags & QMF_SILENT))
				sound = uiSoundMove;
		}

		break;
	case K_MOUSE1:
		if (item){
			if ((item->flags & QMF_HASMOUSEFOCUS) && !(item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN)))
				return UI_ActivateItem(menu, item);
		}

		break;
	case K_ENTER:
	case K_KP_ENTER:
		if (item){
			if (!(item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN | QMF_MOUSEONLY)))
				return UI_ActivateItem(menu, item);
		}

		break;
	}

	return sound;
}

/*
 =================
 UI_DefaultChar
 =================
*/
const char *UI_DefaultChar (menuFramework_s *menu, int ch){

	menuCommon_s	*item;

	if (!menu || !menu->numItems)
		return 0;

	item = UI_ItemAtCursor(menu);
	if (item && !(item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN))){
		if (item->type == QMTYPE_FIELD)
			return UI_Field_Char((menuField_s *)item, ch);
	}

	return 0;
}		

/*
 =================
 UI_ActivateItem
 =================
*/
const char *UI_ActivateItem (menuFramework_s *menu, menuCommon_s *item){

	if (item->callback){
		item->callback(item, QM_ACTIVATED);

		if (!(item->flags & QMF_SILENT))
			return uiSoundMove;
	}

	return 0;
}

/*
 =================
 UI_RefreshServerList
 =================
*/
void UI_RefreshServerList (void){

	int		i;

	for (i = 0; i < UI_MAX_SERVERS; i++){
		memset(&uiStatic.serverAddresses[i], 0, sizeof(netAdr_t));
		Q_strncpyz(uiStatic.serverNames[i], "<no server>", sizeof(uiStatic.serverNames[i]));
	}

	uiStatic.numServers = 0;

	Cbuf_ExecuteText(EXEC_APPEND, "localServers\npingServers\n");
}


// =====================================================================


/*
 =================
 UI_CloseMenu
 =================
*/
void UI_CloseMenu (void){

	uiStatic.menuActive = NULL;
	uiStatic.menuDepth = 0;
	uiStatic.visible = false;

	CL_ClearKeyStates();
	IN_ClearStates();

	CL_SetKeyDest(KEY_GAME);

	Cvar_ForceSet("paused", "0");
}

/*
 =================
 UI_PushMenu
 =================
*/
void UI_PushMenu (menuFramework_s *menu){

	int				i;
	menuCommon_s	*item;

	// Never pause in multiplayer
	if (!Com_ServerState() || Cvar_GetInteger("maxclients") > 1)
		Cvar_ForceSet("paused", "0");
	else
		Cvar_ForceSet("paused", "1");

	// If this menu is already present, drop back to that level to avoid
	// stacking menus by hotkeys
	for (i = 0; i < uiStatic.menuDepth; i++){
		if (uiStatic.menuStack[i] == menu){
			uiStatic.menuDepth = i;
			break;
		}
	}

	if (i == uiStatic.menuDepth){
		if (uiStatic.menuDepth >= UI_MAX_MENUDEPTH)
			Com_Error(ERR_FATAL, "UI_PushMenu: menu stack overflow");

		uiStatic.menuStack[uiStatic.menuDepth++] = menu;
	}

	uiStatic.menuActive = menu;
	uiStatic.firstDraw = true;
	uiStatic.enterSound = true;
	uiStatic.visible = true;

	CL_SetKeyDest(KEY_MENU);

	menu->cursor = 0;
	menu->cursorPrev = 0;

	// Force first available item to have focus
	for (i = 0; i < menu->numItems; i++){
		item = (menuCommon_s *)menu->items[i];

		if (item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN | QMF_MOUSEONLY))
			continue;

		menu->cursorPrev = -1;
		UI_SetCursor(menu, i);
		break;
	}
}

/*
 =================
 UI_PopMenu
 =================
*/
void UI_PopMenu (void){

	UI_StartSound(uiSoundOut);

	uiStatic.menuDepth--;

	if (uiStatic.menuDepth < 0)
		Com_Error(ERR_FATAL, "UI_PopMenu: menu stack underflow");

	if (uiStatic.menuDepth){
		uiStatic.menuActive = uiStatic.menuStack[uiStatic.menuDepth-1];
		uiStatic.firstDraw = true;
	}
	else
		UI_CloseMenu();;
}


// =====================================================================


/*
 =================
 UI_KeyDownEvent
 =================
*/
void UI_KeyDownEvent (int key){

	const char	*sound;

	if (!uiStatic.initialized)
		return;

	if (!uiStatic.visible)
		return;

	if (!uiStatic.menuActive)
		return;

	if (uiStatic.menuActive->keyFunc)
		sound = uiStatic.menuActive->keyFunc(key);
	else
		sound = UI_DefaultKey(uiStatic.menuActive, key);

	if (sound && sound != uiSoundNull)
		UI_StartSound(sound);
}

/*
 =================
 UI_CharEvent
 =================
*/
void UI_CharEvent (int ch){

	const char	*sound;

	if (!uiStatic.initialized)
		return;

	if (!uiStatic.visible)
		return;

	if (!uiStatic.menuActive)
		return;

	sound = UI_DefaultChar(uiStatic.menuActive, ch);

	if (sound && sound != uiSoundNull)
		UI_StartSound(sound);
}

/*
 =================
 UI_MouseMove
 =================
*/
void UI_MouseMove (int x, int y){

	int				i;
	menuCommon_s	*item;

	if (!uiStatic.initialized)
		return;

	if (!uiStatic.visible)
		return;

	if (!uiStatic.menuActive)
		return;

	x *= ui_sensitivity->floatValue;
	y *= ui_sensitivity->floatValue;

	if (!x && !y)
		return;

	uiStatic.cursorX += x;
	if (uiStatic.cursorX < 0)
		uiStatic.cursorX = 0;
	else if (uiStatic.cursorX > uiStatic.glConfig.videoWidth)
		uiStatic.cursorX = uiStatic.glConfig.videoWidth;

	uiStatic.cursorY += y;
	if (uiStatic.cursorY < 0)
		uiStatic.cursorY = 0;
	else if (uiStatic.cursorY > uiStatic.glConfig.videoHeight)
		uiStatic.cursorY = uiStatic.glConfig.videoHeight;

	// Region test the active menu items
	for (i = 0; i < uiStatic.menuActive->numItems; i++){
		item = (menuCommon_s *)uiStatic.menuActive->items[i];

		if (item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN))
			continue;

		if (!UI_CursorInRect(item->x, item->y, item->width, item->height))
			continue;

		// Set focus to item at cursor
		if (uiStatic.menuActive->cursor != i){
			UI_SetCursor(uiStatic.menuActive, i);
			((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursorPrev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if (!(((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags & QMF_SILENT))
				UI_StartSound(uiSoundMove);
		}

		((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}

	// Out of any region
	if (uiStatic.menuActive->numItems){
		((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;

		// A mouse only item restores focus to the previous item
		if (((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags & QMF_MOUSEONLY){
			if (uiStatic.menuActive->cursorPrev != -1)
				uiStatic.menuActive->cursor = uiStatic.menuActive->cursorPrev;
		}
	}
}

/*
 =================
 UI_UpdateMenu
 =================
*/
void UI_UpdateMenu (int realTime){

	if (!uiStatic.initialized)
		return;

	if (!uiStatic.visible)
		return;

	if (!uiStatic.menuActive)
		return;

	uiStatic.realTime = realTime;

	// Draw menu
	if (uiStatic.menuActive->drawFunc)
		uiStatic.menuActive->drawFunc();
	else
		UI_DrawMenu(uiStatic.menuActive);

	if (uiStatic.firstDraw){
		UI_MouseMove(0, 0);
		uiStatic.firstDraw = false;
	}

	// Draw cursor
	UI_DrawMouseCursor();

	// Delay playing the enter sound until after the menu has been
	// drawn, to avoid delay while caching images
	if (uiStatic.enterSound){
		UI_StartSound(uiSoundIn);
		uiStatic.enterSound = false;
	}
}

/*
 =================
 UI_SetActiveMenu
 =================
*/
void UI_SetActiveMenu (uiActiveMenu_t activeMenu){

	if (!uiStatic.initialized)
		return;

	switch (activeMenu){
	case UI_CLOSEMENU:
		UI_CloseMenu();
		break;
	case UI_MAINMENU:
		UI_Main_Menu();
		break;
	case UI_INGAMEMENU:
		UI_InGame_Menu();
		break;
	default:
		Com_Error(ERR_FATAL, "UI_SetActiveMenu: bad activeMenu (%i)", activeMenu);
	}
}

/*
 =================
 UI_AddServerToList
 =================
*/
void UI_AddServerToList (netAdr_t adr, const char *info){

	int		i;

	if (!uiStatic.initialized)
		return;

	if (uiStatic.numServers == UI_MAX_SERVERS)
		return;		// Full

	while (*info == ' ')
		info++;

	// Ignore if duplicated
	for (i = 0; i < uiStatic.numServers; i++){
		if (!Q_stricmp(uiStatic.serverNames[i], info))
			return;
	}

	// Add it to the list
	uiStatic.serverAddresses[uiStatic.numServers] = adr;
	Q_strncpyz(uiStatic.serverNames[uiStatic.numServers], info, sizeof(uiStatic.serverNames[uiStatic.numServers]));
	uiStatic.numServers++;
}

/*
 =================
 UI_IsVisible
 =================
*/
qboolean UI_IsVisible (void){

	if (!uiStatic.initialized)
		return false;

	return uiStatic.visible;
}

/*
 =================
 UI_IsFullscreen
 =================
*/
qboolean UI_IsFullscreen (void){

	if (!uiStatic.initialized)
		return false;

	if (!uiStatic.visible)
		return false;

	if (!uiStatic.menuActive)
		return false;

	return !uiStatic.menuActive->translucent;
}

/*
 =================
 UI_Precache
 =================
*/
void UI_Precache (void){

	if (!uiStatic.initialized)
		return;

	if (!ui_precache->integerValue)
		return;

	S_RegisterSound(uiSoundIn);
	S_RegisterSound(uiSoundMove);
	S_RegisterSound(uiSoundOut);
	S_RegisterSound(uiSoundBuzz);

	R_RegisterMaterialNoMip(UI_CURSOR_NORMAL);
	R_RegisterMaterialNoMip(UI_CURSOR_DISABLED);
	R_RegisterMaterialNoMip(UI_CURSOR_TYPING);
	R_RegisterMaterialNoMip(UI_LEFTARROW);
	R_RegisterMaterialNoMip(UI_LEFTARROWFOCUS);
	R_RegisterMaterialNoMip(UI_RIGHTARROW);
	R_RegisterMaterialNoMip(UI_RIGHTARROWFOCUS);
	R_RegisterMaterialNoMip(UI_UPARROW);
	R_RegisterMaterialNoMip(UI_UPARROWFOCUS);
	R_RegisterMaterialNoMip(UI_DOWNARROW);
	R_RegisterMaterialNoMip(UI_DOWNARROWFOCUS);
	R_RegisterMaterialNoMip(UI_BACKGROUNDLISTBOX);
	R_RegisterMaterialNoMip(UI_SELECTIONBOX);
	R_RegisterMaterialNoMip(UI_BACKGROUNDBOX);
	R_RegisterMaterialNoMip(UI_MOVEBOX);
	R_RegisterMaterialNoMip(UI_MOVEBOXFOCUS);
	R_RegisterMaterialNoMip(UI_BACKBUTTON);
	R_RegisterMaterialNoMip(UI_LOADBUTTON);
	R_RegisterMaterialNoMip(UI_SAVEBUTTON);
	R_RegisterMaterialNoMip(UI_DELETEBUTTON);
	R_RegisterMaterialNoMip(UI_CANCELBUTTON);
	R_RegisterMaterialNoMip(UI_APPLYBUTTON);
	R_RegisterMaterialNoMip(UI_ACCEPTBUTTON);
	R_RegisterMaterialNoMip(UI_PLAYBUTTON);
	R_RegisterMaterialNoMip(UI_STARTBUTTON);
	R_RegisterMaterialNoMip(UI_NEWGAMEBUTTON);

	if (ui_precache->integerValue == 1)
		return;

	UI_Main_Precache();
	UI_InGame_Precache();
	UI_SinglePlayer_Precache();
	UI_LoadGame_Precache();
	UI_SaveGame_Precache();
	UI_MultiPlayer_Precache();
	UI_Options_Precache();
	UI_PlayerSetup_Precache();
	UI_Controls_Precache();
	UI_GameOptions_Precache();
	UI_Audio_Precache();
	UI_Video_Precache();
	UI_Advanced_Precache();
	UI_Performance_Precache();
	UI_Network_Precache();
	UI_Defaults_Precache();
	UI_Cinematics_Precache();
	UI_Demos_Precache();
	UI_Mods_Precache();
	UI_Quit_Precache();
	UI_Credits_Precache();
	UI_GoToSite_Precache();
}

/*
 =================
 UI_Init
 =================
*/
void UI_Init (void){

	// Register our variables and commands
	ui_precache = Cvar_Get("ui_precache", "0", CVAR_ARCHIVE, "Precache UI files");
	ui_sensitivity = Cvar_Get("ui_sensitivity", "1", CVAR_ARCHIVE, "Mouse sensitivity scale factor");
	ui_singlePlayerSkill = Cvar_Get("ui_singlePlayerSkill", "1", CVAR_ARCHIVE, "Single player skill");

	Cmd_AddCommand("menu_main", UI_Main_Menu, NULL);
	Cmd_AddCommand("menu_ingame", UI_InGame_Menu, NULL);
	Cmd_AddCommand("menu_singleplayer", UI_SinglePlayer_Menu, NULL);
	Cmd_AddCommand("menu_loadgame", UI_LoadGame_Menu, NULL);
	Cmd_AddCommand("menu_savegame", UI_SaveGame_Menu, NULL);
	Cmd_AddCommand("menu_multiplayer", UI_MultiPlayer_Menu, NULL);
	Cmd_AddCommand("menu_options", UI_Options_Menu, NULL);
	Cmd_AddCommand("menu_playersetup", UI_PlayerSetup_Menu, NULL);
	Cmd_AddCommand("menu_controls", UI_Controls_Menu, NULL);
	Cmd_AddCommand("menu_gameoptions", UI_GameOptions_Menu, NULL);
	Cmd_AddCommand("menu_audio", UI_Audio_Menu, NULL);
	Cmd_AddCommand("menu_video", UI_Video_Menu, NULL);
	Cmd_AddCommand("menu_advanced", UI_Advanced_Menu, NULL);
	Cmd_AddCommand("menu_performance", UI_Performance_Menu, NULL);
	Cmd_AddCommand("menu_network", UI_Network_Menu, NULL);
	Cmd_AddCommand("menu_defaults", UI_Defaults_Menu, NULL);
	Cmd_AddCommand("menu_cinematics", UI_Cinematics_Menu, NULL);
	Cmd_AddCommand("menu_demos", UI_Demos_Menu, NULL);
	Cmd_AddCommand("menu_mods", UI_Mods_Menu, NULL);
	Cmd_AddCommand("menu_quit", UI_Quit_Menu, NULL);
	Cmd_AddCommand("menu_credits", UI_Credits_Menu, NULL);

	R_GetGLConfig(&uiStatic.glConfig);
	S_GetALConfig(&uiStatic.alConfig);

	uiStatic.scaleX = 640.0 / 1024;
	uiStatic.scaleY = 480.0 / 768;

	uiStatic.initialized = true;

	Com_Printf("UI Initialized\n");
}

/*
 =================
 UI_Shutdown
 =================
*/
void UI_Shutdown (void){

	if (!uiStatic.initialized)
		return;

	Cmd_RemoveCommand("menu_main");
	Cmd_RemoveCommand("menu_ingame");
	Cmd_RemoveCommand("menu_singleplayer");
	Cmd_RemoveCommand("menu_loadgame");
	Cmd_RemoveCommand("menu_savegame");
	Cmd_RemoveCommand("menu_multiplayer");
	Cmd_RemoveCommand("menu_options");
	Cmd_RemoveCommand("menu_playersetup");
	Cmd_RemoveCommand("menu_controls");
	Cmd_RemoveCommand("menu_gameoptions");
	Cmd_RemoveCommand("menu_audio");
	Cmd_RemoveCommand("menu_video");
	Cmd_RemoveCommand("menu_advanced");
	Cmd_RemoveCommand("menu_performance");
	Cmd_RemoveCommand("menu_network");
	Cmd_RemoveCommand("menu_defaults");
	Cmd_RemoveCommand("menu_cinematics");
	Cmd_RemoveCommand("menu_demos");
	Cmd_RemoveCommand("menu_mods");
	Cmd_RemoveCommand("menu_quit");
	Cmd_RemoveCommand("menu_credits");

	// Make sure we stop a playing cinematic
	if (uiStatic.cinematicPlaying)
		CIN_StopCinematic();

	memset(&uiStatic, 0, sizeof(uiStatic_t));
}
