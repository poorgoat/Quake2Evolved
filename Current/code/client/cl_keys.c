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


typedef struct {
	qboolean	down;				// True if currently down
	int			repeats;			// If > 1, it is auto-repeating
	char		*binding;			// Bound command text
} key_t;

typedef struct {
	char		*name;
	int			keyNum;
} keyName_t;

static key_t		cl_keys[256];

static int			cl_anyKeyDown;

static keyDest_t	cl_keyDest;

static qboolean		cl_keyEditMode;

static keyName_t	cl_keyNames[] = {
	{"TAB",				K_TAB},
	{"ENTER",			K_ENTER},
	{"ESCAPE",			K_ESCAPE},
	{"SPACE",			K_SPACE},
	{"BACKSPACE",		K_BACKSPACE},
	{"UPARROW",			K_UPARROW},
	{"DOWNARROW",		K_DOWNARROW},
	{"LEFTARROW",		K_LEFTARROW},
	{"RIGHTARROW",		K_RIGHTARROW},

	{"ALT",				K_ALT},
	{"CTRL",			K_CTRL},
	{"SHIFT",			K_SHIFT},
	
	{"F1",				K_F1},
	{"F2",				K_F2},
	{"F3",				K_F3},
	{"F4",				K_F4},
	{"F5",				K_F5},
	{"F6",				K_F6},
	{"F7",				K_F7},
	{"F8",				K_F8},
	{"F9",				K_F9},
	{"F10",				K_F10},
	{"F11",				K_F11},
	{"F12",				K_F12},

	{"INS",				K_INS},
	{"DEL",				K_DEL},
	{"PGDN",			K_PGDN},
	{"PGUP",			K_PGUP},
	{"HOME",			K_HOME},
	{"END",				K_END},

	{"MOUSE1",			K_MOUSE1},
	{"MOUSE2",			K_MOUSE2},
	{"MOUSE3",			K_MOUSE3},
	{"MOUSE4",			K_MOUSE4},
	{"MOUSE5",			K_MOUSE5},

	{"KP_HOME",			K_KP_HOME},
	{"KP_UPARROW",		K_KP_UPARROW},
	{"KP_PGUP",			K_KP_PGUP},
	{"KP_LEFTARROW",	K_KP_LEFTARROW},
	{"KP_5",			K_KP_5},
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW},
	{"KP_END",			K_KP_END},
	{"KP_DOWNARROW",	K_KP_DOWNARROW},
	{"KP_PGDN",			K_KP_PGDN},
	{"KP_ENTER",		K_KP_ENTER},
	{"KP_INS",			K_KP_INS},
	{"KP_DEL",			K_KP_DEL},
	{"KP_SLASH",		K_KP_SLASH},
	{"KP_STAR",			K_KP_STAR},
	{"KP_MINUS",		K_KP_MINUS},
	{"KP_PLUS",			K_KP_PLUS},
	{"KP_NUMLOCK",		K_KP_NUMLOCK},

	{"MWHEELUP",		K_MWHEELUP},
	{"MWHEELDOWN",		K_MWHEELDOWN},

	{"PAUSE",			K_PAUSE},
	{"CAPSLOCK",		K_CAPSLOCK},

	{"SEMICOLON",		';'},	// A raw semicolon separates commands

	{NULL,				0}
};


/*
 =================
 CL_StringToKeyNum

 Returns a key number to be used to index a key binding by looking at
 the given string. Single ASCII characters return themselves, while the
 K_* names are matched up.
 =================
*/
int CL_StringToKeyNum (const char *string){

	keyName_t	*kn;

	if (!string || !string[0])
		return -1;

	if (!string[1])
		return string[0];

	for (kn = cl_keyNames; kn->name; kn++){
		if (!Q_stricmp(kn->name, string))
			return kn->keyNum;
	}

	return -1;
}

/*
 =================
 CL_KeyNumToString

 Returns a string (either a single ASCII char, or a K_* name) for the
 given key number
 =================
*/
char *CL_KeyNumToString (int keyNum){

	static char	string[2];
	keyName_t	*kn;

	if (keyNum == -1)
		return "<KEY NOT FOUND>";

	if (keyNum < 0 || keyNum > 255)
		return "<OUT OF RANGE>";

	if (keyNum > 32 && keyNum < 127){
		// Printable ASCII
		string[0] = keyNum;
		string[1] = 0;
		return string;
	}

	for (kn = cl_keyNames; kn->name; kn++){
		if (kn->keyNum == keyNum)
			return kn->name;
	}

	return "<UNKNOWN KEYNUM>";
}

/*
 =================
 CL_SetKeyBinding
 =================
*/
void CL_SetKeyBinding (int keyNum, const char *binding){

	if (keyNum < 0 || keyNum > 255)
		return;

	com_configModified = true;		// Save at next opportunity

	// Free old binding
	if (cl_keys[keyNum].binding){
		FreeString(cl_keys[keyNum].binding);
		cl_keys[keyNum].binding = NULL;
	}

	// Allocate memory for new binding
	cl_keys[keyNum].binding = CopyString(binding);
}

/*
 =================
 CL_GetKeyBinding
 =================
*/
char *CL_GetKeyBinding (int keyNum){

	if (keyNum < 0 || keyNum > 255)
		return "";

	if (!cl_keys[keyNum].binding)
		return "";

	return cl_keys[keyNum].binding;
}

/*
 =================
 CL_WriteKeyBindings

 Writes lines containing "bind key command"
 =================
*/
void CL_WriteKeyBindings (fileHandle_t f){

	int		i;

	FS_Printf(f, "unbindall\r\n");

	for (i = 0; i < 256; i++){
		if (!cl_keys[i].binding || !cl_keys[i].binding[0])
			continue;

		FS_Printf(f, "bind %s \"%s\"\r\n", CL_KeyNumToString(i), cl_keys[i].binding);
	}
}

/*
 =================
 CL_GetKeyDest
 =================
*/
keyDest_t CL_GetKeyDest (void){

	return cl_keyDest;
}

/*
 =================
 CL_SetKeyDest
 =================
*/
void CL_SetKeyDest (keyDest_t dest){

	cl_keyDest = dest;
}

/*
 =================
 CL_SetKeyEditMode
 =================
*/
void CL_SetKeyEditMode (qboolean editMode){

	cl_keyEditMode = editMode;
}

/*
 =================
 CL_KeyIsDown
 =================
*/
qboolean CL_KeyIsDown (int key){

	if (key < 0 || key > 255)
		return false;

	return cl_keys[key].down;
}

/*
 =================
 CL_AnyKeyIsDown
 =================
*/
qboolean CL_AnyKeyIsDown (void){

	if (cl_keyDest != KEY_GAME)
		return false;

	return (cl_anyKeyDown != 0);
}

/*
 =================
 CL_ClearKeyStates
 =================
*/
void CL_ClearKeyStates (void){

	int		i;

	for (i = 0; i < 256; i++){
		if (cl_keys[i].down || cl_keys[i].repeats)
			CL_KeyEvent(i, false, 0);

		cl_keys[i].down = false;
		cl_keys[i].repeats = 0;
	}

	cl_anyKeyDown = 0;
}

/*
 =================
 CL_KeyEvent
 
 Called by the system for both key up and key down events
 =================
*/
void CL_KeyEvent (int key, qboolean down, unsigned time){

	char	*kb;
	char	cmd[1024];

	// Update auto-repeat status and BUTTON_ANY status
	cl_keys[key].down = down;

	if (down){
		cl_keys[key].repeats++;
		if (cl_keys[key].repeats == 1)
			cl_anyKeyDown++;

		if (cl_keyDest == KEY_GAME){
			if (cl_keys[key].repeats == 100 && !cl_keys[key].binding)
				Com_Printf("'%s' is unbound, use controls menu to set\n", CL_KeyNumToString(key));

			if (cl_keys[key].repeats > 1)
				return;		// Ignore most auto-repeats
		}
	}
	else {
		cl_keys[key].repeats = 0;

		cl_anyKeyDown--;
		if (cl_anyKeyDown < 0)
			cl_anyKeyDown = 0;
	}

	// Console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~'){
		if (!down)
			return;

		Con_ToggleConsole_f();
		return;
	}

	// Any key pressed during cinematic playback will finish it and
	// advance to the next server
	if ((cls.state == CA_ACTIVE && cls.cinematicPlaying) && (cl_keyDest == KEY_GAME && (key < K_F1 || key > K_F12))){
		if (!down)
			return;

		CL_StopCinematic();
		CL_FinishCinematic();
		return;
	}

	// Any key pressed during demo playback will disconnect
	if ((cls.state == CA_ACTIVE && cl.demoPlaying) && (cl_keyDest == KEY_GAME && (key < K_F1 || key > K_F12))){
		if (!down)
			return;

		Cbuf_AddText("disconnect\n");
		return;
	}

	// Escape key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE){
		if (!down)
			return;

		// If playing a cinematic, stop and bring up the main menu
		if (cls.cinematicPlaying && cl_keyDest != KEY_CONSOLE){
			CL_StopCinematic();
			CL_FinishCinematic();
			return;
		}

		// If connecting or loading, disconnect
		if ((cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE) && cl_keyDest != KEY_CONSOLE){
			Cbuf_AddText("disconnect\n");
			return;
		}

		// If the help computer / inventory is on, put away
		if ((cls.state == CA_ACTIVE && cl.frame.playerState.stats[STAT_LAYOUTS]) && cl_keyDest == KEY_GAME){
			Cbuf_AddText("cmd putaway\n");
			return;
		}

		switch (cl_keyDest){
		case KEY_GAME:
			if (cls.state == CA_ACTIVE)
				UI_SetActiveMenu(UI_INGAMEMENU);
			else
				UI_SetActiveMenu(UI_MAINMENU);

			break;
		case KEY_CONSOLE:
			Con_KeyDownEvent(key);
			break;
		case KEY_MESSAGE:
			Con_MessageKeyDownEvent(key);
			break;
		case KEY_MENU:
			UI_KeyDownEvent(key);
			break;
		default:
			Com_Error(ERR_FATAL, "CL_KeyEvent: bad cl_keyDest");
		}

		return;
	}

	// Key up events only generate commands if the game key binding is
	// a button command (leading + sign). These will occur even in 
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	// Button commands include the key number as a parameter, so
	// multiple downs can be matched with ups.
	if (!down){
		kb = cl_keys[key].binding;
		if (kb && kb[0] == '+'){
			Q_snprintfz(cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
			Cbuf_AddText(cmd);
		}

		return;
	}

	// Send the key down event to the appropriate handler
	switch (cl_keyDest){
	case KEY_GAME:
		// If the light editor is active, try to edit a light
		if (cl_keyEditMode && (key == K_MOUSE1)){
			if (R_EditLight())
				break;
		}

		// Generate a game command
		kb = cl_keys[key].binding;
		if (kb){
			if (kb[0] == '+'){
				Q_snprintfz(cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
				Cbuf_AddText(cmd);
			}
			else {
				Q_snprintfz(cmd, sizeof(cmd), "%s\n", kb);
				Cbuf_AddText(cmd);
			}
		}

		break;
	case KEY_CONSOLE:
		Con_KeyDownEvent(key);
		break;
	case KEY_MESSAGE:
		Con_MessageKeyDownEvent(key);
		break;
	case KEY_MENU:
		UI_KeyDownEvent(key);
		break;
	default:
		Com_Error(ERR_FATAL, "CL_KeyEvent: bad cl_keyDest");
	}
}

/*
 =================
 CL_CharEvent

 Called by the system for char events
 =================
*/
void CL_CharEvent (int ch){

	// The console key should never be used as a char
	if (ch == '`' || ch == '~')
		return;

	// Send the char event to the appropriate handler
	switch (cl_keyDest){
	case KEY_GAME:
		break;
	case KEY_CONSOLE:
		Con_CharEvent(ch);
		break;
	case KEY_MESSAGE:
		Con_MessageCharEvent(ch);
		break;
	case KEY_MENU:
		UI_CharEvent(ch);
		break;
	default:
		Com_Error(ERR_FATAL, "CL_CharEvent: bad cl_keyDest");
	}
}

/*
 =================
 CL_Bind_f
 =================
*/
static void CL_Bind_f (void){

	int		i, keyNum;
	char	cmd[1024];

	if (Cmd_Argc() < 2){
		Com_Printf("bind <key> [command]\n");
		return;
	}

	keyNum = CL_StringToKeyNum(Cmd_Argv(1));
	if (keyNum == -1){
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (Cmd_Argc() == 2){
		if (cl_keys[keyNum].binding)
			Com_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), cl_keys[keyNum].binding);
		else
			Com_Printf("\"%s\" is not bound\n", Cmd_Argv(1));

		return;
	}

	// Copy the rest of the command line
	cmd[0] = 0;
	for (i = 2; i < Cmd_Argc(); i++){
		Q_strncatz(cmd, Cmd_Argv(i), sizeof(cmd));
		if (i != Cmd_Argc() - 1)
			Q_strncatz(cmd, " ", sizeof(cmd));
	}

	CL_SetKeyBinding(keyNum, cmd);
}

/*
 =================
 CL_Unbind_f
 =================
*/
static void CL_Unbind_f (void){

	int		keyNum;

	if (Cmd_Argc() != 2){
		Com_Printf("unbind <key>\n");
		return;
	}

	keyNum = CL_StringToKeyNum(Cmd_Argv(1));
	if (keyNum == -1){
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	CL_SetKeyBinding(keyNum, "");
}

/*
 =================
 CL_UnbindAll_f
 =================
*/
static void CL_UnbindAll_f (void){

	int		i;

	for (i = 0; i < 256; i++){
		if (!cl_keys[i].binding)
			continue;

		CL_SetKeyBinding(i, "");
	}
}

/*
 =================
 CL_ListBinds_f
 =================
*/
static void CL_ListBinds_f (void){

	int		i;

	for (i = 0; i < 256; i++){
		if (!cl_keys[i].binding || !cl_keys[i].binding[0])
			continue;

		Com_Printf("   %-20s \"%s\"\n", CL_KeyNumToString(i), cl_keys[i].binding);
	}
}

/*
 =================
 CL_InitKeys
 =================
*/
void CL_InitKeys (void){

	Cmd_AddCommand("bind", CL_Bind_f, "Bind a command to a key");
	Cmd_AddCommand("unbind", CL_Unbind_f, "Unbind any command from a key");
	Cmd_AddCommand("unbindAll", CL_UnbindAll_f, "Unbind any commands from all keys");
	Cmd_AddCommand("listBinds", CL_ListBinds_f, "List bound keys");
}

/*
 =================
 CL_ShutdownKeys
 =================
*/
void CL_ShutdownKeys (void){

	int		i;

	Cmd_RemoveCommand("bind");
	Cmd_RemoveCommand("unbind");
	Cmd_RemoveCommand("unbindAll");
	Cmd_RemoveCommand("listBinds");

	for (i = 0; i < 256; i++){
		if (cl_keys[i].binding)
			FreeString(cl_keys[i].binding);
	}

	memset(cl_keys, 0, sizeof(cl_keys));
}
