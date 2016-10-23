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


#define	CON_TIMES		4
#define	CON_LINES		1024
#define CON_LINELEN		80
#define CON_MAXINPUT	256
#define CON_INPUTLINES	32
#define CON_MAXCMDS		4096

typedef struct {
	qboolean	initialized;
	
	// Text buffer
	char		text[CON_LINES][CON_LINELEN];
	int			offset;			// Offset in current line for next print
	int 		lineWidth;		// Characters across screen
	int			currentLine;	// Line where next message will be printed
	int			displayLine;	// Bottom of console displays this line
	int			lastColorIndex;	// Last color index for next line

	// Transparent notify lines
	int			notifyTimes[CON_TIMES];

	// Drawing
	float		height;
	float		fraction;
	int			visibleLines;

	// Input
	char		inputLines[CON_INPUTLINES][CON_MAXINPUT];
	int			historyLine;
	int			editLine;
	int			editLen;
	int			editPos;

	// Chat
	qboolean	chatActive;
	qboolean	chatTeam;
	char		chatBuffer[CON_MAXINPUT];
	int			chatLen;
	int			chatPos;

	// Command auto-completion
	char		*cmds[CON_MAXCMDS];
	int			numCmds;
	int			curCmd;
	char		args[CON_MAXINPUT];
} console_t;

static console_t	con;

cvar_t	*con_notifyTime;
cvar_t	*con_toggleSpeed;
cvar_t	*con_showClock;
cvar_t	*con_noPrint;


/*
 =================
 Con_ClearNotify
 =================
*/
void Con_ClearNotify (void){

	int		i;

	for (i = 0; i < CON_TIMES; i++)
		con.notifyTimes[i] = 0;
}

/*
 =================
 Con_ClearTyping
 =================
*/
void Con_ClearTyping (void){

	int		i;

	con.inputLines[con.editLine][0] = 0;
	con.editLen = 0;
	con.editPos = 0;

	// Free the list
	for (i = 0; i < con.numCmds; i++)
		FreeString(con.cmds[i]);

	con.numCmds = 0;
}

/*
 =================
 Con_ClearChat
 =================
*/
void Con_ClearChat (void){

	con.chatActive = false;
	con.chatTeam = false;
	con.chatBuffer[0] = 0;
	con.chatLen = 0;
	con.chatPos = 0;
}

/*
 =================
 Con_Clear_f
 =================
*/
void Con_Clear_f (void){

	memset(con.text, 0, CON_LINES*CON_LINELEN);
}

/*
 =================
 Con_Dump_f

 Save the console contents out to a file
 =================
*/
void Con_Dump_f (void){

	fileHandle_t	f;
	char			name[MAX_QPATH];
	int				l;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: condump <filename>\n");
		return;
	}

	Q_strncpyz(name, Cmd_Argv(1), sizeof(name));
	Com_DefaultExtension(name, sizeof(name), ".txt");

	FS_FOpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Couldn't open %s\n", name);
		return;
	}

	Com_Printf("Dumped console text to %s\n", name);

	// Skip empty lines
	for (l = con.currentLine - CON_LINES + 1; l <= con.currentLine; l++){
		if (con.text[l % CON_LINES][0])
			break;
	}

	// Write the remaining lines
	for ( ; l <= con.currentLine; l++)
		FS_Printf(f, "%s\r\n", con.text[l % CON_LINES]);

	FS_FCloseFile(f);
}

/*
 =================
 Con_ToggleConsole_f
 =================
*/
void Con_ToggleConsole_f (void){

	Con_ClearTyping();
	Con_ClearNotify();

	if (Key_GetKeyDest() == KEY_CONSOLE){
		if (UI_IsVisible())
			Key_SetKeyDest(KEY_MENU);
		else if (con.chatActive)
			Key_SetKeyDest(KEY_MESSAGE);
		else
			Key_SetKeyDest(KEY_GAME);
	}
	else
		Key_SetKeyDest(KEY_CONSOLE);
}

/*
 =================
 Con_MessageMode_f
 =================
*/
void Con_MessageMode_f (void){

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle || cl.demoPlayback)
		return;

	con.chatActive = true;
	con.chatTeam = false;

	Key_SetKeyDest(KEY_MESSAGE);
}

/*
 =================
 Con_MessageMode2_f
 =================
*/
void Con_MessageMode2_f (void){

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle || cl.demoPlayback)
		return;

	con.chatActive = true;
	con.chatTeam = true;

	Key_SetKeyDest(KEY_MESSAGE);
}

/*
 =================
 Con_Print

 Handles cursor positioning, line wrapping, etc...
 =================
*/
void Con_Print (const char *text){

	int		len;

	if (!con.initialized)
		return;

	if (con_noPrint->integer)
		return;

	// Convert the mask to color escape
	if (text[0] == 1 || text[0] == 2){
		text++;
		Con_Print(S_COLOR_GREEN);
	}

	while (*text){
		// Count word length
		for (len = 0; len < con.lineWidth; len++){
			if (text[len] <= ' ')
				break;
		}

		// Word wrap
		if (len != con.lineWidth && con.offset + len >= con.lineWidth)
			con.offset = 0;

		if (!con.offset){
			// Linefeed
			if (con.displayLine == con.currentLine)
				con.displayLine++;
	
			con.currentLine++;
			memset(con.text[con.currentLine % CON_LINES], 0, CON_LINELEN);

			if (con.displayLine < con.currentLine - CON_LINES + 1)
				con.displayLine = con.currentLine - CON_LINES + 1;
			if (con.displayLine < CON_LINES)
				con.displayLine = CON_LINES;

			// Add last color index from previous line
			if (con.lastColorIndex != -1){
				con.text[con.currentLine % CON_LINES][con.offset++] = Q_COLOR_ESCAPE;
				con.text[con.currentLine % CON_LINES][con.offset++] = '0' + con.lastColorIndex;
			}

			// Mark time for transparent overlay
			con.notifyTimes[con.currentLine % CON_TIMES] = cls.realTime;
		}

		switch (*text){
		case '\n':
			// Reset cursor and color
			con.offset = 0;
			con.lastColorIndex = -1;

			break;
		default:
			// Display character and advance
			con.text[con.currentLine % CON_LINES][con.offset++] = *text;
			if (con.offset >= con.lineWidth)
				con.offset = 0;

			// Save last color index for next line
			if (Q_IsColorString(text))
				con.lastColorIndex = Q_ColorIndex(*(text+1));

			break;
		}

		text++;
	}
}

/*
 =================
 Con_CloseConsole

 Forces the console off
 =================
*/
void Con_CloseConsole (void){

	con.fraction = 0;

	Con_ClearChat();
	Con_ClearTyping();
	Con_ClearNotify();

	Key_SetKeyDest(KEY_GAME);
}

/*
 =================
 Con_RunConsole
 =================
*/
void Con_RunConsole (void){

	float	height;

	// Decide on the height of the console
	if (Key_GetKeyDest() == KEY_CONSOLE)
		height = con.height;
	else
		height = 0.0;

	if (height < con.fraction){
		con.fraction -= con_toggleSpeed->value * cls.frameTime;
		if (con.fraction < height)
			con.fraction = height;
	}
	else {
		con.fraction += con_toggleSpeed->value * cls.frameTime;
		if (con.fraction > height)
			con.fraction = height;
	}
}


/*
 =======================================================================

 LINE TYPING

 =======================================================================
*/


/*
 =================
 Con_CompleteCommandCallback
 =================
*/
static void Con_CompleteCommandCallback (const char *found){

	if (con.numCmds >= CON_MAXCMDS)
		return;

	con.cmds[con.numCmds++] = CopyString(found);
}
	
/*
 =================
 Con_CompleteCommand
 =================
*/
static void Con_CompleteCommand (void){

	cvar_t	*var;
	char	partial[CON_MAXINPUT], args[CON_MAXINPUT];
	char	*in, *out, *first, *last;
	int		len = 0, i;

	in = con.inputLines[con.editLine];

	// Skip backslash
	while (*in && (*in == '\\' || *in == '/'))
		in++;

	// Copy partial command
	out = partial;
	while (*in && (*in != ' '))
		*out++ = *in++;
	*out = 0;

	// Copy the rest of the line, if available
	out = args;
	while (*in++)
		*out++ = *in;
	*out = 0;

	if (!partial[0])
		return;		// Nothing to search for

	// Free the list
	for (i = 0; i < con.numCmds; i++)
		FreeString(con.cmds[i]);

	con.numCmds = 0;

	// Find matching commands and variables
	Cmd_CompleteCommand(partial, Con_CompleteCommandCallback);
	Cvar_CompleteVariable(partial, Con_CompleteCommandCallback);

	if (!con.numCmds)
		return;		// Nothing found

	if (con.numCmds == 1){
		// Only one was found, so copy it to the edit line
		if (args[0])
			Q_snprintfz(con.inputLines[con.editLine], sizeof(con.inputLines[con.editLine]), "\\%s %s", con.cmds[0], args);
		else
			Q_snprintfz(con.inputLines[con.editLine], sizeof(con.inputLines[con.editLine]), "\\%s ", con.cmds[0]);

		con.editLen = strlen(con.inputLines[con.editLine]);
		con.editPos = con.editLen;
	}
	else {
		// Display the list of matching cmds and cvars
		Com_Printf("]\\%s\n", partial);

		qsort(con.cmds, con.numCmds, sizeof(char *), Q_SortStrcmp);

		// Find the number of matching characters between the first and
		// the last element in the list and copy it
		first = con.cmds[0];
		last = con.cmds[con.numCmds-1];

		while (*first && *last && *first == *last){
			first++;
			last++;

			partial[len] = con.cmds[0][len];
			len++;
		}
		partial[len] = 0;

		// Copy the match to the edit line
		if (args[0])
			Q_snprintfz(con.inputLines[con.editLine], sizeof(con.inputLines[con.editLine]), "\\%s %s", partial, args);
		else
			Q_snprintfz(con.inputLines[con.editLine], sizeof(con.inputLines[con.editLine]), "\\%s", partial);

		con.editLen = strlen(con.inputLines[con.editLine]);
		con.editPos = len+1;

		// List
		for (i = 0; i < con.numCmds; i++){
			var = Cvar_FindVar(con.cmds[i]);
			if (var)
				Com_Printf("   %s \"%s\"\n", con.cmds[i], var->string);
			else
				Com_Printf("   %s\n", con.cmds[i]);
		}

		con.curCmd = -1;
		Q_strncpyz(con.args, args, sizeof(con.args));
	}
}

/*
 =================
 Con_KeyDown

 Interactive line editing and console scrollback
 =================
*/
void Con_KeyDown (int key){

	char	*str, *s;
	int		i;

	switch (key){
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_STAR:
		key = '*';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	case K_KP_ENTER:
		key = K_ENTER;
		break;
	}

	// Auto-complete command
	if (key == K_TAB){
		if (con.numCmds <= 1)
			Con_CompleteCommand();
		else {
			// Cycle through commands
			if (Key_IsDown(K_SHIFT)){
				con.curCmd--;
				if (con.curCmd <= -1)
					con.curCmd = con.numCmds - 1;
			}
			else {
				con.curCmd++;
				if (con.curCmd >= con.numCmds)
					con.curCmd = 0;
			}
			
			if (con.args[0])
				Q_snprintfz(con.inputLines[con.editLine], sizeof(con.inputLines[con.editLine]), "\\%s %s", con.cmds[con.curCmd], con.args);
			else
				Q_snprintfz(con.inputLines[con.editLine], sizeof(con.inputLines[con.editLine]), "\\%s", con.cmds[con.curCmd]);

			con.editLen = strlen(con.inputLines[con.editLine]);
			con.editPos = strlen(con.cmds[con.curCmd])+1;
		}

		return;
	}

	// Any other typing frees the list
	for (i = 0; i < con.numCmds; i++)
		FreeString(con.cmds[i]);

	con.numCmds = 0;

	// Clipboard paste
	if ((key == 'v' && Key_IsDown(K_CTRL)) || (key == K_INS && Key_IsDown(K_SHIFT))){
		str = Sys_GetClipboardText();
		if (!str)
			return;

		// Insert at the current position
		s = str;
		while (*s){
			if (con.editLen + 1 == CON_MAXINPUT)
				break;		// All full

			if (*s < 32 || *s > 126){
				s++;
				continue;	// Non printable
			}

			for (i = con.editLen + 1; i > con.editPos; i--)
				con.inputLines[con.editLine][i] = con.inputLines[con.editLine][i-1];

			con.inputLines[con.editLine][con.editPos++] = *s++;
			con.editLen++;
		}

		FreeString(str);
		return;
	}

	// Toggle between full and half screen console
	if (key == 'f' && Key_IsDown(K_CTRL)){
		if (con.height != 1.0)
			con.height = 1.0;
		else
			con.height = 0.5;

		return;
	}

	// Shift console up
	if ((key == K_UPARROW || key == K_PGUP) && Key_IsDown(K_SHIFT)){
		if (key == K_UPARROW)
			con.height -= 0.05;
		else
			con.height -= 0.1;

		if (con.height < 0.1)
			con.height = 0.1;

		return;
	}

	// Shift console down
	if ((key == K_DOWNARROW || key == K_PGDN) && Key_IsDown(K_SHIFT)){
		if (key == K_DOWNARROW)
			con.height += 0.05;
		else
			con.height += 0.1;

		if (con.height > 1.0)
			con.height = 1.0;

		return;
	}

	// Clear input
	if (key == 'c' && Key_IsDown(K_CTRL)){
		Con_ClearTyping();
		return;
	}

	// Clear buffer
	if (key == 'l' && Key_IsDown(K_CTRL)){
		Con_Clear_f();
		return;
	}

	// Scroll buffer up
	if (key == K_PGUP || key == K_MWHEELUP){
		if (Key_IsDown(K_CTRL))
			con.displayLine -= 6;
		else
			con.displayLine -= 2;

		if (con.displayLine < con.currentLine - CON_LINES + 1)
			con.displayLine = con.currentLine - CON_LINES + 1;
		if (con.displayLine < CON_LINES)
			con.displayLine = CON_LINES;

		return;
	}

	// Scroll buffer down
	if (key == K_PGDN || key == K_MWHEELDOWN){
		if (Key_IsDown(K_CTRL))
			con.displayLine += 6;
		else
			con.displayLine += 2;

		if (con.displayLine > con.currentLine)
			con.displayLine = con.currentLine;

		return;
	}

	// Buffer home
	if (key == K_HOME && Key_IsDown(K_CTRL)){
		con.displayLine = con.currentLine - CON_LINES + 1;
		if (con.displayLine < CON_LINES)
			con.displayLine = CON_LINES;
		
		return;
	}

	// Buffer end
	if (key == K_END && Key_IsDown(K_CTRL)){
		con.displayLine = con.currentLine;
		return;
	}

	// Execute a command
	if (key == K_ENTER){
		str = con.inputLines[con.editLine];
		while (*str && (*str == '\\' || *str == '/'))
			str++;

		Cbuf_AddText(str);
		Cbuf_AddText("\n");

		Com_Printf("]%s\n", con.inputLines[con.editLine]);

		con.editLine = (con.editLine + 1) & (CON_INPUTLINES-1);
		con.historyLine = con.editLine;

		con.inputLines[con.editLine][0] = 0;
		con.editLen = 0;
		con.editPos = 0;

		return;
	}

	// Previous history line
	if (key == K_UPARROW || (key == 'p' && Key_IsDown(K_CTRL))){
		do {
			con.historyLine = (con.historyLine - 1) & (CON_INPUTLINES-1);
		} while (con.historyLine != con.editLine && !con.inputLines[con.historyLine][0]);
		
		if (con.historyLine == con.editLine)
			con.historyLine = (con.historyLine - con.editLine) & (CON_INPUTLINES-1);
			
		Q_strncpyz(con.inputLines[con.editLine], con.inputLines[con.historyLine], sizeof(con.inputLines[con.editLine]));
		con.editLen = strlen(con.inputLines[con.editLine]);
		con.editPos = con.editLen;
		
		return;
	}

	// Next history line
	if (key == K_DOWNARROW || (key == 'n' && Key_IsDown(K_CTRL))){
		if (con.historyLine == con.editLine)
			return;
		
		do {
			con.historyLine = (con.historyLine + 1) & (CON_INPUTLINES-1);
		} while (con.historyLine != con.editLine && !con.inputLines[con.historyLine][0]);
		
		if (con.historyLine == con.editLine){
			con.inputLines[con.editLine][0] = 0;
			con.editLen = 0;
			con.editPos = 0;
		}
		else {
			Q_strncpyz(con.inputLines[con.editLine], con.inputLines[con.historyLine], sizeof(con.inputLines[con.editLine]));
			con.editLen = strlen(con.inputLines[con.editLine]);
			con.editPos = con.editLen;
		}
		
		return;
	}

	// Previous character
	if (key == K_LEFTARROW){
		if (con.editPos)
			con.editPos--;

		return;
	}

	// Next character
	if (key == K_RIGHTARROW){
		if (con.editPos < con.editLen)
			con.editPos++;

		return;
	}

	// First character
	if (key == K_HOME || (key == 'a' && Key_IsDown(K_CTRL))){
		con.editPos = 0;
		return;
	}

	// Last character
	if (key == K_END || (key == 'e' && Key_IsDown(K_CTRL))){
		con.editPos = con.editLen;
		return;
	}

	// Delete previous character
	if (key == K_BACKSPACE || (key == 'h' && Key_IsDown(K_CTRL))){
		if (!con.editPos)
			return;

		con.editPos--;
		for (i = con.editPos; i <= con.editLen; i++)
			con.inputLines[con.editLine][i] = con.inputLines[con.editLine][i+1];
		con.inputLines[con.editLine][con.editLen--] = 0;

		return;
	}

	// Delete next character
	if (key == K_DEL){
		if (con.editPos == con.editLen)
			return;

		for (i = con.editPos; i <= con.editLen; i++)
			con.inputLines[con.editLine][i] = con.inputLines[con.editLine][i+1];
		con.inputLines[con.editLine][con.editLen--] = 0;

		return;
	}

	// Normal typing
	if (con.editLen + 1 == CON_MAXINPUT)
		return;		// All full

	if (key < 32 || key > 126)
		return;		// Non printable

	for (i = con.editLen + 1; i > con.editPos; i--)
		con.inputLines[con.editLine][i] = con.inputLines[con.editLine][i-1];

	con.inputLines[con.editLine][con.editPos++] = key;
	con.editLen++;
}

/*
 =================
 Con_KeyMessage
 =================
*/
void Con_KeyMessage (int key){

	char	*str, *s;
	int		i;

	// Execute chat message
	if (key == K_ENTER){
		if (con.chatLen){
			if (con.chatTeam){
				Cbuf_AddText("say_team \"");
				Cbuf_AddText(con.chatBuffer);
				Cbuf_AddText("\"\n");
			}
			else {
				Cbuf_AddText("say \"");
				Cbuf_AddText(con.chatBuffer);
				Cbuf_AddText("\"\n");
			}
		}

		Con_ClearChat();

		Key_SetKeyDest(KEY_GAME);
		return;
	}

	// Cancel chat
	if (key == K_ESCAPE){
		Con_ClearChat();

		Key_SetKeyDest(KEY_GAME);
		return;
	}

	// Clipboard paste
	if ((key == 'v' && Key_IsDown(K_CTRL)) || (key == K_INS && Key_IsDown(K_SHIFT))){
		str = Sys_GetClipboardText();
		if (!str)
			return;

		// Insert at the current position
		s = str;
		while (*s){
			if (con.chatLen + 1 == CON_MAXINPUT)
				break;		// All full

			if (*s < 32 || *s > 126){
				s++;
				continue;	// Non printable
			}

			for (i = con.chatLen + 1; i > con.chatPos; i--)
				con.chatBuffer[i] = con.chatBuffer[i-1];

			con.chatBuffer[con.chatPos++] = *s++;
			con.chatLen++;
		}

		FreeString(str);
		return;
	}

	// Previous character
	if (key == K_LEFTARROW){
		if (con.chatPos)
			con.chatPos--;

		return;
	}

	// Next character
	if (key == K_RIGHTARROW){
		if (con.chatPos < con.chatLen)
			con.chatPos++;

		return;
	}

	// First character
	if (key == K_HOME){
		con.chatPos = 0;
		return;
	}

	// Last character
	if (key == K_END){
		con.chatPos = con.chatLen;
		return;
	}

	// Delete previous character
	if (key == K_BACKSPACE){
		if (!con.chatPos)
			return;

		con.chatPos--;
		for (i = con.chatPos; i <= con.chatLen; i++)
			con.chatBuffer[i] = con.chatBuffer[i+1];
		con.chatBuffer[con.chatLen--] = 0;
		
		return;
	}

	// Delete next character
	if (key == K_DEL){
		if (con.chatPos == con.chatLen)
			return;

		for (i = con.chatPos; i <= con.chatLen; i++)
			con.chatBuffer[i] = con.chatBuffer[i+1];
		con.chatBuffer[con.chatLen--] = 0;

		return;
	}

	// Normal typing
	if (con.chatLen + 1 == CON_MAXINPUT)
		return;		// All full

	if (key < 32 || key > 126)
		return;		// Non printable

	for (i = con.chatLen + 1; i > con.chatPos; i--)
		con.chatBuffer[i] = con.chatBuffer[i-1];

	con.chatBuffer[con.chatPos++] = key;
	con.chatLen++;
}


/*
 =======================================================================

 DRAWING

 =======================================================================
*/


/*
 =================
 Con_DrawString
 =================
*/
static void Con_DrawString (int x, int y, const char *string, const color_t color){

	color_t	modulate;
	int		ch;
	float	col, row;

	*(unsigned *)modulate = *(unsigned *)color;

	while (*string){
		if (Q_IsColorString(string)){
			*(unsigned *)modulate = *(unsigned *)colorTable[Q_ColorIndex(*(string+1))];
			string += 2;
			continue;
		}

		ch = *string++;

		ch &= 255;
		if (ch != ' '){
			col = (ch & 15) * 0.0625;
			row = (ch >> 4) * 0.0625;

			R_DrawStretchPic(x+1, y+2, 8, 16, col, row, col + 0.0625, row + 0.0625, colorBlack, cls.media.charsetShader);
			R_DrawStretchPic(x, y, 8, 16, col, row, col + 0.0625, row + 0.0625, modulate, cls.media.charsetShader);
		}

		x += 8;
	}
}

/*
 =================
 Con_DrawInput

 The input line scrolls horizontally if typing goes beyond the right 
 edge
 =================
*/
static void Con_DrawInput (void){

	char	*text = con.inputLines[con.editLine];
	int		scroll = 0, width = con.lineWidth - 2;
	int		cursor;

	// Prompt char
	Con_DrawString(8, con.visibleLines-32, "]", colorWhite);

	// Prestep if horizontally scrolling
	while (Q_PrintStrlen(text+scroll) + 2 > width)
		scroll++;

	// Draw it
	Con_DrawString(16, con.visibleLines-32, text+scroll, colorWhite);

	// Add the cursor frame
	if ((cls.realTime >> 8) & 1){
		// Find cursor position
		width -= 2;

		if (scroll)
			cursor = width - (Q_PrintStrlen(text) - con.editPos);
		else
			cursor = con.editPos;

		cursor -= (strlen(text) - Q_PrintStrlen(text));
		if (cursor > width)
			cursor = width;
		if (cursor < 0)
			cursor = 0;

		// Draw cursor
		Con_DrawString(16+(cursor*8), con.visibleLines-32, "_", colorWhite);
	}
}

/*
 =================
 Con_DrawNotify

 Draws the last few lines of output transparently over the game top.
 Also draws the chat line.
 =================
*/
static void Con_DrawNotify (void){

	int		y = 0;
	char	*text;
	int		scroll, width;
	int		cursor, skip;
	int		i, time;

	for (i = con.currentLine - CON_TIMES+1; i <= con.currentLine; i++){
		time = con.notifyTimes[i % CON_TIMES];
		if (time == 0)
			continue;

		time = cls.realTime - time;
		if (time > con_notifyTime->value * 1000)
			continue;

		text = con.text[i % CON_LINES];
		Con_DrawString(8, y, text, colorWhite);

		y += 16;
	}

	if (Key_GetKeyDest() == KEY_MESSAGE){
		// Prompt text
		if (con.chatTeam){
			Con_DrawString(8, y, "say_team: ", colorWhite);
			skip = 11;
		}
		else {
			Con_DrawString(8, y, "say: ", colorWhite);
			skip = 6;
		}

		width = con.lineWidth - skip;
		text = con.chatBuffer;
		scroll = 0;

		// Prestep if horizontally scrolling
		while (Q_PrintStrlen(text+scroll) + 2 > width)
			scroll++;

		// Draw it
		Con_DrawString(skip*8, y, text+scroll, colorWhite);

		// Add the cursor frame
		if ((cls.realTime >> 8) & 1){
			// Find cursor position
			width -= 2;

			if (scroll)
				cursor = width - (Q_PrintStrlen(text) - con.chatPos);
			else
				cursor = con.chatPos;

			cursor -= (strlen(text) - Q_PrintStrlen(text));
			if (cursor > width)
				cursor = width;
			if (cursor < 0)
				cursor = 0;

			// Draw cursor
			Con_DrawString((skip*8)+(cursor*8), y, "_", colorWhite);
		}
	}
}

/*
 =================
 Con_DrawConsole

 Draws the console
 =================
*/
void Con_DrawConsole (void){

	int		i, x, y;
	int		rows, row;
	char	*text, str[32];
	color_t	color = {0, 76, 127, 255};

	if (!con.fraction){
		// Only draw notify lines in game
		if ((cls.state != CA_ACTIVE || cls.loading) || cls.cinematicHandle)
			return;

		if (Key_GetKeyDest() == KEY_GAME || Key_GetKeyDest() == KEY_MESSAGE)
			Con_DrawNotify();

		return;
	}

	con.visibleLines = cls.glConfig.videoHeight * con.fraction;

	// Draw the background
	R_DrawStretchPic(0, 0, cls.glConfig.videoWidth, con.visibleLines, 0, 0, 1, 1, colorWhite, cls.media.consoleShader);
	R_DrawStretchPic(0, con.visibleLines-3, cls.glConfig.videoWidth, 3, 0, 0, 1, 1, colorBlack, cls.media.whiteShader);

	// Draw the version string
	Con_DrawString(cls.glConfig.videoWidth-8-(strlen(Q2E_VERSION)*8), con.visibleLines-20, Q2E_VERSION, color);

	// Draw the current time
	if (con_showClock->integer){
		_strtime(str);
		Con_DrawString(cls.glConfig.videoWidth-8-(strlen(str)*8), con.visibleLines-36, str, color);
	}

	// Draw the text
	rows = (con.visibleLines - 48) >> 3;		// Rows of text to draw
	y = con.visibleLines - 48;

	// Draw arrows to show the buffer is backscrolled
	if (con.displayLine != con.currentLine){
		for (x = 0; x < con.lineWidth; x += 4)
			Con_DrawString((x+1) << 3, y, "^", color);

		rows--;
		y -= 16;
	}

	// Draw from the top down
	y -= (rows * 16);
	row = con.displayLine - rows;

	for (i = 0; i <= rows; i++, y += 16, row++){
		if (row < 0 || con.currentLine - row >= CON_LINES)
			continue;
		if (y <= -16)
			continue;

		text = con.text[row % CON_LINES];
		Con_DrawString(8, y, text, colorWhite);
	}

	// Draw the input prompt, user text, and cursor if desired
	Con_DrawInput();
}


// =====================================================================


/*
 =================
 Con_Init
 =================
*/
void Con_Init (void){

	// Register our cvars and commands
	con_notifyTime = Cvar_Get("con_notifyTime", "3", 0);
	con_toggleSpeed = Cvar_Get("con_toggleSpeed", "3", 0);
	con_showClock = Cvar_Get("con_showClock", "1", 0);
	con_noPrint = Cvar_Get("con_noPrint", "0", 0);

	Cmd_AddCommand("clear", Con_Clear_f);
	Cmd_AddCommand("condump", Con_Dump_f);
	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);

	con.lineWidth = CON_LINELEN - 2;
	con.currentLine = CON_LINES - 1;
	con.displayLine = con.currentLine;
	con.lastColorIndex = -1;
	con.height = 0.5;

	con.initialized = true;

	Com_Printf("Console Initialized\n");
}

/*
 =================
 Con_Shutdown
 =================
*/
void Con_Shutdown (void){

	int		i;

	if (!con.initialized)
		return;

	Cmd_RemoveCommand("clear");
	Cmd_RemoveCommand("condump");
	Cmd_RemoveCommand("toggleconsole");
	Cmd_RemoveCommand("messagemode");
	Cmd_RemoveCommand("messagemode2");

	// Free the list
	for (i = 0; i < con.numCmds; i++)
		FreeString(con.cmds[i]);

	memset(&con, 0, sizeof(console_t));
}
