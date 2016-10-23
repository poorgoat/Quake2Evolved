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
	char	*name;
	int		keyNum;
} keyName_t;

static char			*key_bindings[256];
static qboolean		key_down[256];
static int			key_shift[256];			// Key to map to if shift held down in console
static int			key_repeats[256];		// If > 1, it is autorepeating

static int			key_anyKeyDown;

static keyDest_t	key_dest;

static qboolean		key_consoleKeys[256];	// If true, can't be rebound while in console

static keyName_t	key_names[] = {
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

	{"JOY1",			K_JOY1},
	{"JOY2",			K_JOY2},
	{"JOY3",			K_JOY3},
	{"JOY4",			K_JOY4},

	{"AUX1",			K_AUX1},
	{"AUX2",			K_AUX2},
	{"AUX3",			K_AUX3},
	{"AUX4",			K_AUX4},
	{"AUX5",			K_AUX5},
	{"AUX6",			K_AUX6},
	{"AUX7",			K_AUX7},
	{"AUX8",			K_AUX8},
	{"AUX9",			K_AUX9},
	{"AUX10",			K_AUX10},
	{"AUX11",			K_AUX11},
	{"AUX12",			K_AUX12},
	{"AUX13",			K_AUX13},
	{"AUX14",			K_AUX14},
	{"AUX15",			K_AUX15},
	{"AUX16",			K_AUX16},
	{"AUX17",			K_AUX17},
	{"AUX18",			K_AUX18},
	{"AUX19",			K_AUX19},
	{"AUX20",			K_AUX20},
	{"AUX21",			K_AUX21},
	{"AUX22",			K_AUX22},
	{"AUX23",			K_AUX23},
	{"AUX24",			K_AUX24},
	{"AUX25",			K_AUX25},
	{"AUX26",			K_AUX26},
	{"AUX27",			K_AUX27},
	{"AUX28",			K_AUX28},
	{"AUX29",			K_AUX29},
	{"AUX30",			K_AUX30},
	{"AUX31",			K_AUX31},
	{"AUX32",			K_AUX32},

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
	{"APPS",			K_APPS},
	{"CAPSLOCK",		K_CAPSLOCK},

	{"SEMICOLON",		';'},	// A raw semicolon separates commands

	{NULL,				0}
};

static byte			key_scanCodeToKey[128] = {
	0			, K_ESCAPE		, '1'			, '2'			,
	'3'			, '4'			, '5'			, '6'			,
	'7'			, '8'			, '9'			, '0'			,
	'-'			, '='			, K_BACKSPACE	, K_TAB			,
	'q'			, 'w'			, 'e'			, 'r'			,
	't'			, 'y'			, 'u'			, 'i'			,
	'o'			, 'p'			, '['			, ']'			,
	K_ENTER		, K_CTRL		, 'a'			, 's'			,
	'd'			, 'f'			, 'g'			, 'h'			,
	'j'			, 'k'			, 'l'			, ';'			,
	'\''		, '`'			, K_SHIFT		, '\\'			,
	'z'			, 'x'			, 'c'			, 'v'			,
	'b'			, 'n'			, 'm'			, ','			,
	'.'			, '/'			, K_SHIFT		, K_KP_STAR		,
	K_ALT		, ' '			, K_CAPSLOCK	, K_F1			,
	K_F2		, K_F3			, K_F4			, K_F5			,
	K_F6		, K_F7			, K_F8			, K_F9			,
	K_F10		, K_PAUSE		, 0				, K_HOME		,
	K_UPARROW	, K_PGUP		, K_KP_MINUS	, K_LEFTARROW	,
	K_KP_5		, K_RIGHTARROW	, K_KP_PLUS		, K_END			,
	K_DOWNARROW	, K_PGDN		, K_INS			, K_DEL			,
	0			, 0				, 0				, K_F11			,
	K_F12		, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0				,
	0			, 0				, 0				, 0
}; 


/*
 =================
 Key_StringToKeyNum

 Returns a key number to be used to index key_bindings[] by looking at
 the given string. Single ASCII characters return themselves, while the
 K_* names are matched up.
 =================
*/
int Key_StringToKeyNum (const char *string){

	keyName_t	*kn;

	if (!string || !string[0])
		return -1;

	if (!string[1])
		return string[0];

	for (kn = key_names; kn->name; kn++){
		if (!Q_stricmp(kn->name, string))
			return kn->keyNum;
	}

	return -1;
}

/*
 =================
 Key_KeyNumToString

 Returns a string (either a single ASCII char, or a K_* name) for the
 given key number
 =================
*/
char *Key_KeyNumToString (int keyNum){

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

	for (kn = key_names; kn->name; kn++){
		if (kn->keyNum == keyNum)
			return kn->name;
	}

	return "<UNKNOWN KEYNUM>";
}

/*
 =================
 Key_SetBinding
 =================
*/
void Key_SetBinding (int keyNum, const char *binding){

	if (keyNum < 0 || keyNum > 255)
		return;

	com_configModified = true;		// Save at next opportunity

	// Free old binding
	if (key_bindings[keyNum]){
		FreeString(key_bindings[keyNum]);
		key_bindings[keyNum] = NULL;
	}

	// Allocate memory for new binding
	key_bindings[keyNum] = CopyString(binding);
}

/*
 =================
 Key_GetBinding
 =================
*/
char *Key_GetBinding (int keyNum){

	if (keyNum < 0 || keyNum > 255)
		return "";

	if (!key_bindings[keyNum])
		return "";

	return key_bindings[keyNum];
}

/*
 =================
 Key_Bind_f
 =================
*/
void Key_Bind_f (void){

	int		i, keyNum;
	char	cmd[1024];

	if (Cmd_Argc() < 2){
		Com_Printf("bind <key> [command] : attach a command to a key\n");
		return;
	}

	keyNum = Key_StringToKeyNum(Cmd_Argv(1));
	if (keyNum == -1){
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (Cmd_Argc() == 2){
		if (key_bindings[keyNum])
			Com_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), key_bindings[keyNum]);
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

	Key_SetBinding(keyNum, cmd);
}

/*
 =================
 Key_Unbind_f
 =================
*/
void Key_Unbind_f (void){

	int		keyNum;

	if (Cmd_Argc() != 2){
		Com_Printf("unbind <key> : remove commands from a key\n");
		return;
	}

	keyNum = Key_StringToKeyNum(Cmd_Argv(1));
	if (keyNum == -1){
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding(keyNum, "");
}

/*
 =================
 Key_UnbindAll_f
 =================
*/
void Key_UnbindAll_f (void){

	int		i;

	for (i = 0; i < 256; i++){
		if (key_bindings[i])
			Key_SetBinding(i, "");
	}
}

/*
 =================
 Key_BindList_f
 =================
*/
void Key_BindList_f (void){

	int		i;

	for (i = 0; i < 256; i++){
		if (key_bindings[i] && key_bindings[i][0])
			Com_Printf("%s \"%s\"\n", Key_KeyNumToString(i), key_bindings[i]);
	}
}

/*
 =================
 Key_WriteBindings

 Writes lines containing "bind key command"
 =================
*/
void Key_WriteBindings (fileHandle_t f){

	int		i;

	FS_Printf(f, "unbindall\r\n");

	for (i = 0; i < 256; i++){
		if (!key_bindings[i] || !key_bindings[i][0])
			continue;

		FS_Printf(f, "bind %s \"%s\"\r\n", Key_KeyNumToString(i), key_bindings[i]);
	}
}

/*
 =================
 Key_GetKeyDest
 =================
*/
keyDest_t Key_GetKeyDest (void){

	return key_dest;
}

/*
 =================
 Key_SetKeyDest
 =================
*/
void Key_SetKeyDest (keyDest_t dest){

	key_dest = dest;
}

/*
 =================
 Key_IsDown
 =================
*/
qboolean Key_IsDown (int key){

	if (key < 0 || key > 255)
		return false;

	return key_down[key];
}

/*
 =================
 Key_IsAnyKeyDown
 =================
*/
qboolean Key_IsAnyKeyDown (void){

	if (key_dest != KEY_GAME)
		return false;

	return (key_anyKeyDown != 0);
}

/*
 =================
 Key_ClearStates
 =================
*/
void Key_ClearStates (void){

	int		i;

	for (i = 0; i < 256; i++){
		if (key_down[i] || key_repeats[i])
			Key_Event(i, false, 0);

		key_down[i] = false;
		key_repeats[i] = 0;
	}

	key_anyKeyDown = 0;
}

/*
 =================
 Key_Event
 
 Called by the system between frames for both key up and key down 
 events.
 Should NOT be called during an interrupt!
 =================
*/
void Key_Event (int key, qboolean down, unsigned time){

	char	*kb;
	char	cmd[1024];

	if (key < 0 || key > 255)
		Com_Error(ERR_FATAL, "Key_Event: key = %i", key);

	// Update auto-repeat status
	if (down){
		key_repeats[key]++;

		if (key_dest == KEY_GAME){
			if (key_repeats[key] >= 100 && !key_bindings[key])
				Com_Printf("%s is unbound, use controls menu to set\n", Key_KeyNumToString(key));

			if (key_repeats[key] > 1)
				return;		// Ignore most auto-repeats
		}
	}
	else
		key_repeats[key] = 0;

	// Console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~'){
		if (!down)
			return;

		Con_ToggleConsole_f();
		return;
	}

	// Any key pressed during a cinematic server will finish it and
	// advance to the next server
	if ((cls.state == CA_ACTIVE && cls.cinematicHandle) && (key_dest != KEY_CONSOLE && (key < K_F1 || key > K_F12))){
		if (!down)
			return;

		CL_StopCinematic();
		CL_FinishCinematic();
		return;
	}

	// Any key pressed during demo playback will disconnect
	if ((cls.state == CA_ACTIVE && cl.demoPlayback) && (key_dest != KEY_CONSOLE && (key < K_F1 || key > K_F12))){
		if (!down)
			return;

		Cbuf_AddText("disconnect\n");
		return;
	}

	// Menu key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE){
		if (!down)
			return;

		// If playing a cinematic, stop and bring up the main menu
		if (cls.cinematicHandle && key_dest != KEY_CONSOLE){
			CL_StopCinematic();
			CL_FinishCinematic();
			return;
		}

		// If connecting/loading, disconnect
		if ((cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE) && key_dest != KEY_CONSOLE){
			Cbuf_AddText("disconnect\n");
			return;
		}

		// Put away help computer / inventory
		if (cl.frame.playerState.stats[STAT_LAYOUTS] && key_dest == KEY_GAME){
			Cbuf_AddText("cmd putaway\n");
			return;
		}

		switch (key_dest){
		case KEY_GAME:
			UI_SetActiveMenu(UI_INGAMEMENU);
			break;
		case KEY_CONSOLE:
			Con_ToggleConsole_f();
			break;
		case KEY_MESSAGE:
			Con_KeyMessage(key);
			break;
		case KEY_MENU:
			UI_KeyDown(key);
			break;
		default:
			Com_Error(ERR_FATAL, "Key_Event: bad key_dest");
		}

		return;
	}

	// Track if any key is down for BUTTON_ANY
	key_down[key] = down;

	if (down){
		if (key_repeats[key] == 1)
			key_anyKeyDown++;
	}
	else {
		key_anyKeyDown--;
		if (key_anyKeyDown < 0)
			key_anyKeyDown = 0;
	}

	// Key up events only generate commands if the game key binding is
	// a button command (leading + sign). These will occur even in 
	// console mode, to keep the character from continuing an action 
	// started before a console switch.
	// Button commands include the key number as a parameter, so
	// multiple downs can be matched with ups.
	if (!down){
		kb = key_bindings[key];
		if (kb && kb[0] == '+'){
			Q_snprintfz(cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
			Cbuf_AddText(cmd);
		}

		if (key_shift[key] != key){
			kb = key_bindings[key_shift[key]];
			if (kb && kb[0] == '+'){
				Q_snprintfz(cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
				Cbuf_AddText(cmd);
			}
		}

		return;
	}

	// If not a console key, send to the interpreter no matter what mode
	if ((key_dest == KEY_CONSOLE && !key_consoleKeys[key]) || (key_dest == KEY_GAME && (cls.state == CA_ACTIVE || !key_consoleKeys[key]))){
		kb = key_bindings[key];
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

		return;
	}

	if (key_down[K_SHIFT])
		key = key_shift[key];

	switch (key_dest){
	case KEY_GAME:
	case KEY_CONSOLE:
		Con_KeyDown(key);
		break;
	case KEY_MESSAGE:
		Con_KeyMessage(key);
		break;
	case KEY_MENU:
		UI_KeyDown(key);
		break;
	default:
		Com_Error(ERR_FATAL, "Key_Event: bad key_dest");
	}
}

/*	
 =================
 Key_MapKey

 Map from Windows to Quake key numbers
 =================
*/
int Key_MapKey (int key){

	int			result;
	int			scanCode;
	qboolean	isExtended = false;

	scanCode = (key >> 16) & 255;
	if (scanCode > 127)
		return 0;

	if (key & (1 << 24))
		isExtended = true;

	result = key_scanCodeToKey[scanCode];

	if (!isExtended){
		switch (result){
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			return result;
		}
	}
	else {
		if (scanCode == 93)	// Special check for the APPS key
			return K_APPS;

		switch (result){
		case K_ENTER:
			return K_KP_ENTER;
		case '/':
			return K_KP_SLASH;
		case K_PAUSE:
			return K_KP_NUMLOCK;
		default:
			return result;
		}
	}
}

/*
 =================
 Key_Init
 =================
*/
void Key_Init (void){

	int		i;

	// Register our commands
	Cmd_AddCommand("bind", Key_Bind_f);
	Cmd_AddCommand("unbind", Key_Unbind_f);
	Cmd_AddCommand("unbindall", Key_UnbindAll_f);
	Cmd_AddCommand("bindlist", Key_BindList_f);

	for (i = 32; i < 128; i++)
		key_consoleKeys[i] = true;

	key_consoleKeys[K_ENTER] = true;
	key_consoleKeys[K_KP_ENTER] = true;
	key_consoleKeys[K_TAB] = true;
	key_consoleKeys[K_LEFTARROW] = true;
	key_consoleKeys[K_KP_LEFTARROW] = true;
	key_consoleKeys[K_RIGHTARROW] = true;
	key_consoleKeys[K_KP_RIGHTARROW] = true;
	key_consoleKeys[K_UPARROW] = true;
	key_consoleKeys[K_KP_UPARROW] = true;
	key_consoleKeys[K_DOWNARROW] = true;
	key_consoleKeys[K_KP_DOWNARROW] = true;
	key_consoleKeys[K_BACKSPACE] = true;
	key_consoleKeys[K_HOME] = true;
	key_consoleKeys[K_KP_HOME] = true;
	key_consoleKeys[K_END] = true;
	key_consoleKeys[K_KP_END] = true;
	key_consoleKeys[K_PGUP] = true;
	key_consoleKeys[K_KP_PGUP] = true;
	key_consoleKeys[K_PGDN] = true;
	key_consoleKeys[K_KP_PGDN] = true;
	key_consoleKeys[K_SHIFT] = true;
	key_consoleKeys[K_INS] = true;
	key_consoleKeys[K_KP_INS] = true;
	key_consoleKeys[K_DEL] = true;
	key_consoleKeys[K_KP_DEL] = true;
	key_consoleKeys[K_KP_SLASH] = true;
	key_consoleKeys[K_KP_STAR] = true;
	key_consoleKeys[K_KP_MINUS] = true;
	key_consoleKeys[K_KP_PLUS] = true;
	key_consoleKeys[K_KP_5] = true;
	key_consoleKeys[K_MWHEELUP] = true;
	key_consoleKeys[K_MWHEELDOWN] = true;

	key_consoleKeys['`'] = false;
	key_consoleKeys['~'] = false;

	for (i = 0; i < 256; i++)
		key_shift[i] = i;

	for (i = 'a'; i <= 'z'; i++)
		key_shift[i] = i - 'a' + 'A';

	key_shift['1'] = '!';
	key_shift['2'] = '@';
	key_shift['3'] = '#';
	key_shift['4'] = '$';
	key_shift['5'] = '%';
	key_shift['6'] = '^';
	key_shift['7'] = '&';
	key_shift['8'] = '*';
	key_shift['9'] = '(';
	key_shift['0'] = ')';
	key_shift['-'] = '_';
	key_shift['='] = '+';
	key_shift[','] = '<';
	key_shift['.'] = '>';
	key_shift['/'] = '?';
	key_shift[';'] = ':';
	key_shift['\''] = '"';
	key_shift['['] = '{';
	key_shift[']'] = '}';
	key_shift['`'] = '~';
	key_shift['\\'] = '|';
}

/*
 =================
 Key_Shutdown
 =================
*/
void Key_Shutdown (void){

	int		i;

	Cmd_RemoveCommand("bind");
	Cmd_RemoveCommand("unbind");
	Cmd_RemoveCommand("unbindall");
	Cmd_RemoveCommand("bindlist");

	for (i = 0; i < 256; i++){
		if (key_bindings[i])
			FreeString(key_bindings[i]);
	}
}
