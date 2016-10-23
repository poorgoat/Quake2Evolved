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


#define	CON_LINES			1024
#define CON_LINELEN			80
#define	CON_TIMES			4
#define CON_INPUT_LINES		32
#define CON_INPUT_LINELEN	256
#define CON_MAX_CMDS		8192

typedef struct {
	qboolean	initialized;
	
	// Text buffer
	short		text[CON_LINES][CON_LINELEN];
	int			pos;				// Offset in current line for next print
	int			currentLine;		// Line where next message will be printed
	int			displayLine;		// Bottom of console displays this line
	int			colorIndex;			// Current color index

	// Transparent notify lines
	int			notifyTimes[CON_TIMES];

	// Drawing
	float		displayFrac;		// Fraction of console to display
	int 		lineWidth;			// Characters across screen
	int			visibleLines;		// Lines across screen

	// Input buffer
	char		inputText[CON_INPUT_LINES][CON_INPUT_LINELEN];
	int			inputPos;
	int			inputLen;
	int			inputLine;
	int			historyLine;

	// Chat buffer
	qboolean	chatActive;
	qboolean	chatTeam;
	char		chatText[CON_INPUT_LINELEN];
	int			chatPos;
	int			chatLen;

	// Command auto-completion
	char		*cmdList[CON_MAX_CMDS];
	int			cmdCount;
	int			cmdIndex;
	char		cmdArgs[CON_INPUT_LINELEN];
} console_t;

static console_t	con;

cvar_t	*con_notifyTime;
cvar_t	*con_speed;
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
 Con_ClearInput
 =================
*/
void Con_ClearInput (void){

	int		i;

	con.inputText[con.inputLine][0] = 0;
	con.inputPos = 0;
	con.inputLen = 0;

	// Free the auto-complete list
	for (i = 0; i < con.cmdCount; i++)
		FreeString(con.cmdList[i]);

	con.cmdCount = 0;
	con.cmdIndex = -1;
	con.cmdArgs[0] = 0;
}

/*
 =================
 Con_ClearChat
 =================
*/
void Con_ClearChat (void){

	con.chatActive = false;
	con.chatTeam = false;
	con.chatText[0] = 0;
	con.chatPos = 0;
	con.chatLen = 0;
}

/*
 =================
 Con_Linefeed
 =================
*/
static void Con_Linefeed (void){

	int		i;

	// Scroll down and advance
	if (con.displayLine == con.currentLine)
		con.displayLine++;

	con.currentLine++;

	if (con.displayLine < con.currentLine - CON_LINES + 1)
		con.displayLine = con.currentLine - CON_LINES + 1;
	if (con.displayLine < CON_LINES)
		con.displayLine = CON_LINES;

	// Clear the line
	for (i = 0; i < CON_LINELEN; i++)
		con.text[con.currentLine % CON_LINES][i] = 0;

	// Mark time for transparent overlay
	con.notifyTimes[con.currentLine % CON_TIMES] = cls.realTime;
}

/*
 =================
 Con_Print

 Handles cursor positioning, line wrapping, etc...
 =================
*/
void Con_Print (const char *text){

	static qboolean	cr;
	int				c, len;

	if (!con.initialized)
		return;

	if (con_noPrint->integerValue)
		return;

	// Set the color index if needed
	if (text[0] == 1 || text[0] == 2){
		con.colorIndex = COLOR_GREEN;
		text++;
	}

	while ((c = *text) != 0){
		// Set the color index if needed
		if (Q_IsColorString(text)){
			con.colorIndex = Q_ColorIndex(*(text+1));
			text += 2;
			continue;
		}

		// Count word length
		for (len = 0; len < con.lineWidth; len++){
			if (text[len] <= ' ')
				break;
		}

		// Word wrap
		if (len != con.lineWidth && con.pos + len >= con.lineWidth)
			con.pos = 0;

		if (cr){
			cr = false;

			con.currentLine--;
		}

		if (!con.pos)
			Con_Linefeed();

		text++;

		switch (c){
		case '\n':
			con.pos = 0;
			con.colorIndex = COLOR_WHITE;

			break;
		case '\r':
			con.pos = 0;
			con.colorIndex = COLOR_WHITE;

			cr = true;

			break;
		default:
			// Display character and advance
			con.text[con.currentLine % CON_LINES][con.pos++] = (con.colorIndex << 8) | c;
			if (con.pos >= con.lineWidth)
				con.pos = 0;

			break;
		}
	}
}

/*
 =================
 Con_CloseConsole

 Forces the console off
 =================
*/
void Con_CloseConsole (void){

	con.displayFrac = 0;

	Con_ClearNotify();
	Con_ClearInput();
	Con_ClearChat();

	CL_SetKeyDest(KEY_GAME);
}

/*
 =================
 Con_RunConsole
 =================
*/
void Con_RunConsole (void){

	float	frac;

	// Decide on the height of the console
	if (CL_GetKeyDest() == KEY_CONSOLE)
		frac = 0.5;
	else
		frac = 0.0;

	if (frac < con.displayFrac){
		con.displayFrac -= con_speed->floatValue * cls.frameTime;
		if (con.displayFrac < frac)
			con.displayFrac = frac;
	}
	else if (frac > con.displayFrac){
		con.displayFrac += con_speed->floatValue * cls.frameTime;
		if (con.displayFrac > frac)
			con.displayFrac = frac;
	}
}


/*
 =======================================================================

 LINE EDITING

 =======================================================================
*/


/*
 =================
 Con_CompleteCommandCallback
 =================
*/
static void Con_CompleteCommandCallback (const char *found){

	if (con.cmdCount == CON_MAX_CMDS)
		return;

	con.cmdList[con.cmdCount++] = CopyString(found);
}
	
/*
 =================
 Con_CompleteCommand
 =================
*/
static void Con_CompleteCommand (void){

	cvar_t	*cvar;
	char	partial[CON_INPUT_LINELEN];
	char	*in, *out, *cmd1, *cmd2;
	int		i, len = 0;

	// Skip backslash
	in = con.inputText[con.inputLine];
	while (*in && (*in == '/' || *in == '\\'))
		in++;

	// Copy partial command
	out = partial;
	while (*in && (*in != ' '))
		*out++ = *in++;
	*out = 0;

	if (!partial[0])
		return;		// Nothing to search for

	// Free the auto-complete list
	for (i = 0; i < con.cmdCount; i++)
		FreeString(con.cmdList[i]);

	con.cmdCount = 0;
	con.cmdIndex = -1;
	con.cmdArgs[0] = 0;

	// Copy the rest of the line, if available
	out = con.cmdArgs;
	while (*in++)
		*out++ = *in;
	*out = 0;

	// Find matching commands and variables
	Cmd_CompleteCommand(partial, Con_CompleteCommandCallback);
	Cvar_CompleteVariable(partial, Con_CompleteCommandCallback);

	if (!con.cmdCount)
		return;		// Nothing was found

	if (con.cmdCount == 1){
		// Only one was found, so copy it to the input line
		if (con.cmdArgs[0])
			Q_snprintfz(con.inputText[con.inputLine], sizeof(con.inputText[con.inputLine]), "\\%s %s", con.cmdList[0], con.cmdArgs);
		else
			Q_snprintfz(con.inputText[con.inputLine], sizeof(con.inputText[con.inputLine]), "\\%s ", con.cmdList[0]);

		con.inputLen = strlen(con.inputText[con.inputLine]);
		con.inputPos = con.inputLen;
	}
	else {
		// Sort the list of matches
		qsort(con.cmdList, con.cmdCount, sizeof(char *), Q_SortStrcmp);

		// Display it
		Com_Printf("]\\%s\n", partial);

		for (i = 0; i < con.cmdCount; i++){
			Com_Printf("   %s", con.cmdList[i]);

			cvar = Cvar_FindVariable(con.cmdList[i]);
			if (cvar)
				Com_Printf(" \"%s\"\n", cvar->value);
			else
				Com_Printf("\n");
		}

		// Find the number of matching characters between the first and
		// the last element in the list and copy it
		cmd1 = con.cmdList[0];
		cmd2 = con.cmdList[con.cmdCount-1];

		while (*cmd1 && *cmd2 && *cmd1 == *cmd2){
			cmd1++;
			cmd2++;

			partial[len] = con.cmdList[0][len];
			len++;
		}
		partial[len] = 0;

		// Copy the match to the input line
		if (con.cmdArgs[0])
			Q_snprintfz(con.inputText[con.inputLine], sizeof(con.inputText[con.inputLine]), "\\%s %s", partial, con.cmdArgs);
		else
			Q_snprintfz(con.inputText[con.inputLine], sizeof(con.inputText[con.inputLine]), "\\%s", partial);

		con.inputLen = strlen(con.inputText[con.inputLine]);
		con.inputPos = len + 1;
	}
}

/*
 =================
 Con_KeyDownEvent
 =================
*/
void Con_KeyDownEvent (int key){

	char	*text, *t;
	int		i;

	// Auto-complete command
	if (key == K_TAB){
		if (con.cmdCount <= 1)
			Con_CompleteCommand();
		else {
			// Cycle through the commands
			con.cmdIndex++;
			if (con.cmdIndex == con.cmdCount)
				con.cmdIndex = 0;

			if (con.cmdArgs[0])
				Q_snprintfz(con.inputText[con.inputLine], sizeof(con.inputText[con.inputLine]), "\\%s %s", con.cmdList[con.cmdIndex], con.cmdArgs);
			else
				Q_snprintfz(con.inputText[con.inputLine], sizeof(con.inputText[con.inputLine]), "\\%s", con.cmdList[con.cmdIndex]);

			con.inputLen = strlen(con.inputText[con.inputLine]);
			con.inputPos = strlen(con.cmdList[con.cmdIndex]) + 1;
		}

		return;
	}

	// Any other typing frees the auto-complete list
	for (i = 0; i < con.cmdCount; i++)
		FreeString(con.cmdList[i]);

	con.cmdCount = 0;
	con.cmdIndex = -1;
	con.cmdArgs[0] = 0;

	// Toggle console off
	if (key == K_ESCAPE){
		for (i = 0; i < CON_TIMES; i++)
			con.notifyTimes[i] = 0;

		con.inputText[con.inputLine][0] = 0;
		con.inputPos = 0;
		con.inputLen = 0;

		if (UI_IsVisible())
			CL_SetKeyDest(KEY_MENU);
		else if (con.chatActive)
			CL_SetKeyDest(KEY_MESSAGE);
		else
			CL_SetKeyDest(KEY_GAME);

		return;
	}

	// Clipboard paste
	if ((key == K_INS || key == K_KP_INS) && CL_KeyIsDown(K_SHIFT)){
		text = Sys_GetClipboardText();
		if (!text)
			return;

		// Insert at the current position
		t = text;
		while (*t){
			if (con.inputLen == CON_INPUT_LINELEN - 1)
				break;		// All full

			if (*t < 32){
				t++;
				continue;	// Non printable
			}

			for (i = con.inputLen + 1; i > con.inputPos; i--)
				con.inputText[con.inputLine][i] = con.inputText[con.inputLine][i-1];

			con.inputText[con.inputLine][con.inputPos++] = *t++;
			con.inputLen++;
		}

		FreeString(text);

		return;
	}

	// Scroll buffer up
	if (key == K_PGUP || key == K_KP_PGUP || key == K_MWHEELUP){
		if (CL_KeyIsDown(K_CTRL))
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
	if (key == K_PGDN || key == K_KP_PGDN || key == K_MWHEELDOWN){
		if (CL_KeyIsDown(K_CTRL))
			con.displayLine += 6;
		else
			con.displayLine += 2;

		if (con.displayLine > con.currentLine)
			con.displayLine = con.currentLine;

		return;
	}

	// Buffer home
	if ((key == K_HOME || key == K_KP_HOME) && CL_KeyIsDown(K_CTRL)){
		con.displayLine = con.currentLine - CON_LINES + 1;
		if (con.displayLine < CON_LINES)
			con.displayLine = CON_LINES;
		
		return;
	}

	// Buffer end
	if ((key == K_END || key == K_KP_END) && CL_KeyIsDown(K_CTRL)){
		con.displayLine = con.currentLine;
		return;
	}

	// Execute a command
	if (key == K_ENTER || key == K_KP_ENTER){
		Com_Printf("]%s\n", con.inputText[con.inputLine]);

		// Add the command text
		text = con.inputText[con.inputLine];
		while (*text && (*text == '/' || *text == '\\'))
			text++;

		if (*text){
			Cbuf_AddText(text);
			Cbuf_AddText("\n");
		}

		// Add to history and clear input
		con.inputLine = (con.inputLine + 1) & (CON_INPUT_LINES-1);
		con.historyLine = con.inputLine;

		con.inputText[con.inputLine][0] = 0;
		con.inputPos = 0;
		con.inputLen = 0;

		// Force an update, because the command may take some time
		if (cls.state == CA_DISCONNECTED)
			CL_UpdateScreen();

		return;
	}

	// Previous history line
	if (key == K_UPARROW || key == K_KP_UPARROW){
		do {
			con.historyLine = (con.historyLine - 1) & (CON_INPUT_LINES-1);
		} while (con.historyLine != con.inputLine && !con.inputText[con.historyLine][0]);
		
		if (con.historyLine == con.inputLine)
			con.historyLine = (con.historyLine - con.inputLine) & (CON_INPUT_LINES-1);
			
		Q_strncpyz(con.inputText[con.inputLine], con.inputText[con.historyLine], sizeof(con.inputText[con.inputLine]));
		con.inputLen = strlen(con.inputText[con.inputLine]);
		con.inputPos = con.inputLen;
		
		return;
	}

	// Next history line
	if (key == K_DOWNARROW || key == K_KP_DOWNARROW){
		if (con.historyLine == con.inputLine)
			return;
		
		do {
			con.historyLine = (con.historyLine + 1) & (CON_INPUT_LINES-1);
		} while (con.historyLine != con.inputLine && !con.inputText[con.historyLine][0]);
		
		if (con.historyLine == con.inputLine){
			con.inputText[con.inputLine][0] = 0;
			con.inputPos = 0;
			con.inputLen = 0;
		}
		else {
			Q_strncpyz(con.inputText[con.inputLine], con.inputText[con.historyLine], sizeof(con.inputText[con.inputLine]));
			con.inputLen = strlen(con.inputText[con.inputLine]);
			con.inputPos = con.inputLen;
		}
		
		return;
	}

	// Previous character
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW){
		if (con.inputPos)
			con.inputPos--;

		return;
	}

	// Next character
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW){
		if (con.inputPos < con.inputLen)
			con.inputPos++;

		return;
	}

	// First character
	if (key == K_HOME || key == K_KP_HOME){
		con.inputPos = 0;

		return;
	}

	// Last character
	if (key == K_END || key == K_KP_END){
		con.inputPos = con.inputLen;

		return;
	}

	// Delete previous character
	if (key == K_BACKSPACE){
		if (!con.inputPos)
			return;

		con.inputPos--;
		for (i = con.inputPos; i <= con.inputLen; i++)
			con.inputText[con.inputLine][i] = con.inputText[con.inputLine][i+1];
		con.inputText[con.inputLine][con.inputLen--] = 0;

		return;
	}

	// Delete next character
	if (key == K_DEL || key == K_KP_DEL){
		if (con.inputPos == con.inputLen)
			return;

		for (i = con.inputPos; i <= con.inputLen; i++)
			con.inputText[con.inputLine][i] = con.inputText[con.inputLine][i+1];
		con.inputText[con.inputLine][con.inputLen--] = 0;

		return;
	}
}

/*
 =================
 Con_CharEvent
 =================
*/
void Con_CharEvent (int ch){

	char	*text, *t;
	int		i, j;

	// Clipboard paste
	if (ch == 'v' - 'a' + 1){
		text = Sys_GetClipboardText();
		if (!text)
			return;

		// Insert at the current position
		t = text;
		while (*t){
			if (con.inputLen == CON_INPUT_LINELEN - 1)
				break;		// All full

			if (*t < 32){
				t++;
				continue;	// Non printable
			}

			for (i = con.inputLen + 1; i > con.inputPos; i--)
				con.inputText[con.inputLine][i] = con.inputText[con.inputLine][i-1];

			con.inputText[con.inputLine][con.inputPos++] = *t++;
			con.inputLen++;
		}

		FreeString(text);

		return;
	}

	// Clear buffer
	if (ch == 'l' - 'a' + 1){
		for (i = 0; i < CON_LINES; i++){
			for (j = 0; j < CON_LINELEN; j++)
				con.text[i][j] = 0;
		}

		con.displayLine = con.currentLine;

		return;
	}

	// Clear input
	if (ch == 'c' - 'a' + 1){
		con.inputText[con.inputLine][0] = 0;
		con.inputPos = 0;
		con.inputLen = 0;

		return;
	}

	// Normal typing
	if (con.inputLen == CON_INPUT_LINELEN - 1)
		return;		// All full

	if (ch < 32)
		return;		// Non printable

	for (i = con.inputLen + 1; i > con.inputPos; i--)
		con.inputText[con.inputLine][i] = con.inputText[con.inputLine][i-1];

	con.inputText[con.inputLine][con.inputPos++] = ch;
	con.inputLen++;
}

/*
 =================
 Con_MessageKeyDownEvent
 =================
*/
void Con_MessageKeyDownEvent (int key){

	char	*text, *t;
	int		i;

	// Execute chat message
	if (key == K_ENTER || key == K_KP_ENTER){
		if (con.chatLen){
			if (con.chatTeam){
				Cbuf_AddText("say_team \"");
				Cbuf_AddText(con.chatText);
				Cbuf_AddText("\"\n");
			}
			else {
				Cbuf_AddText("say \"");
				Cbuf_AddText(con.chatText);
				Cbuf_AddText("\"\n");
			}
		}

		Con_ClearChat();

		CL_SetKeyDest(KEY_GAME);

		return;
	}

	// Cancel chat
	if (key == K_ESCAPE){
		Con_ClearChat();

		CL_SetKeyDest(KEY_GAME);

		return;
	}

	// Clipboard paste
	if ((key == K_INS || key == K_KP_INS) && CL_KeyIsDown(K_SHIFT)){
		text = Sys_GetClipboardText();
		if (!text)
			return;

		// Insert at the current position
		t = text;
		while (*t){
			if (con.chatLen == CON_INPUT_LINELEN - 1)
				break;		// All full

			if (*t < 32){
				t++;
				continue;	// Non printable
			}

			for (i = con.chatLen + 1; i > con.chatPos; i--)
				con.chatText[i] = con.chatText[i-1];

			con.chatText[con.chatPos++] = *t++;
			con.chatLen++;
		}

		FreeString(text);

		return;
	}

	// Previous character
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW){
		if (con.chatPos)
			con.chatPos--;

		return;
	}

	// Next character
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW){
		if (con.chatPos < con.chatLen)
			con.chatPos++;

		return;
	}

	// First character
	if (key == K_HOME || key == K_KP_HOME){
		con.chatPos = 0;

		return;
	}

	// Last character
	if (key == K_END || key == K_KP_END){
		con.chatPos = con.chatLen;

		return;
	}

	// Delete previous character
	if (key == K_BACKSPACE){
		if (!con.chatPos)
			return;

		con.chatPos--;
		for (i = con.chatPos; i <= con.chatLen; i++)
			con.chatText[i] = con.chatText[i+1];
		con.chatText[con.chatLen--] = 0;
		
		return;
	}

	// Delete next character
	if (key == K_DEL || key == K_KP_DEL){
		if (con.chatPos == con.chatLen)
			return;

		for (i = con.chatPos; i <= con.chatLen; i++)
			con.chatText[i] = con.chatText[i+1];
		con.chatText[con.chatLen--] = 0;

		return;
	}
}

/*
 =================
 Con_MessageCharEvent
 =================
*/
void Con_MessageCharEvent (int ch){

	char	*text, *t;
	int		i;

	// Clipboard paste
	if (ch == 'v' - 'a' + 1){
		text = Sys_GetClipboardText();
		if (!text)
			return;

		// Insert at the current position
		t = text;
		while (*t){
			if (con.chatLen == CON_INPUT_LINELEN - 1)
				break;		// All full

			if (*t < 32){
				t++;
				continue;	// Non printable
			}

			for (i = con.chatLen + 1; i > con.chatPos; i--)
				con.chatText[i] = con.chatText[i-1];

			con.chatText[con.chatPos++] = *t++;
			con.chatLen++;
		}

		FreeString(text);

		return;
	}

	// Clear input
	if (ch == 'c' - 'a' + 1){
		con.chatText[0] = 0;
		con.chatPos = 0;
		con.chatLen = 0;

		return;
	}

	// Normal typing
	if (con.chatLen == CON_INPUT_LINELEN - 1)
		return;		// All full

	if (ch < 32)
		return;		// Non printable

	for (i = con.chatLen + 1; i > con.chatPos; i--)
		con.chatText[i] = con.chatText[i-1];

	con.chatText[con.chatPos++] = ch;
	con.chatLen++;
}


/*
 =======================================================================

 DRAWING

 =======================================================================
*/


/*
 =================
 Con_DrawChar
 =================
*/
static void Con_DrawChar (int x, int y, int ch, const color_t color){

	float	col, row;

	ch &= 255;
	if (ch == ' ')
		return;

	col = (ch & 15) * 0.0625;
	row = (ch >> 4) * 0.0625;

	R_DrawStretchPic(x+1, y+2, 8, 16, col, row, col + 0.0625, row + 0.0625, colorTable[COLOR_BLACK], cls.media.charsetMaterial);
	R_DrawStretchPic(x, y, 8, 16, col, row, col + 0.0625, row + 0.0625, color, cls.media.charsetMaterial);
}

/*
 =================
 Con_DrawString
 =================
*/
static void Con_DrawString (int x, int y, const char *string, const color_t color){

	color_t	currentColor;
	int		ch;
	float	col, row;

	*(unsigned *)currentColor = *(unsigned *)color;

	while (*string){
		if (Q_IsColorString(string)){
			*(unsigned *)currentColor = *(unsigned *)colorTable[Q_ColorIndex(*(string+1))];
			string += 2;
			continue;
		}

		ch = *string++;

		ch &= 255;
		if (ch != ' '){
			col = (ch & 15) * 0.0625;
			row = (ch >> 4) * 0.0625;

			R_DrawStretchPic(x+1, y+2, 8, 16, col, row, col + 0.0625, row + 0.0625, colorTable[COLOR_BLACK], cls.media.charsetMaterial);
			R_DrawStretchPic(x, y, 8, 16, col, row, col + 0.0625, row + 0.0625, currentColor, cls.media.charsetMaterial);
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

	char	*inputText = con.inputText[con.inputLine];
	int		scroll = 0, width = con.lineWidth - 2;
	int		cursor;

	// Prompt char
	Con_DrawChar(8, con.visibleLines - 32, ']', colorTable[COLOR_WHITE]);

	// Prestep if horizontally scrolling
	while (Q_PrintStrlen(inputText+scroll) + 2 > width)
		scroll++;

	// Draw it
	Con_DrawString(16, con.visibleLines - 32, inputText+scroll, colorTable[COLOR_WHITE]);

	// Add the cursor frame
	if ((cls.realTime >> 8) & 1){
		// Find cursor position
		width -= 2;

		if (scroll)
			cursor = width - (Q_PrintStrlen(inputText) - con.inputPos);
		else
			cursor = con.inputPos;

		cursor -= (strlen(inputText) - Q_PrintStrlen(inputText));
		if (cursor > width)
			cursor = width;
		if (cursor < 0)
			cursor = 0;

		// Draw cursor
		Con_DrawChar(16 + (cursor * 8), con.visibleLines - 32, '_', colorTable[COLOR_WHITE]);
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

	short	*text;
	char	*chatText;
	int		ch, colorIndex;
	int		scroll, width;
	int		cursor, skip;
	int		x, y = 0;
	int		i, time;

	for (i = con.currentLine - CON_TIMES + 1; i <= con.currentLine; i++){
		time = con.notifyTimes[i % CON_TIMES];
		if (time == 0)
			continue;

		if (cls.realTime - time > SEC2MS(con_notifyTime->floatValue))
			continue;

		text = con.text[i % CON_LINES];

		for (x = 0; x < CON_LINELEN; x++){
			ch = text[x] & 0xFF;
			if (!ch)
				break;

			if (ch == ' ')
				continue;

			colorIndex = (text[x] >> 8) & Q_COLOR_MASK;

			Con_DrawChar((x+1) * 8, y, ch, colorTable[colorIndex]);
		}

		y += 16;
	}

	if (CL_GetKeyDest() == KEY_MESSAGE){
		// Prompt text
		if (con.chatTeam){
			Con_DrawString(8, y, "say_team: ", colorTable[COLOR_WHITE]);
			skip = 11;
		}
		else {
			Con_DrawString(8, y, "say: ", colorTable[COLOR_WHITE]);
			skip = 6;
		}

		chatText = con.chatText;
		scroll = 0;
		width = con.lineWidth - skip;

		// Prestep if horizontally scrolling
		while (Q_PrintStrlen(chatText+scroll) + 2 > width)
			scroll++;

		// Draw it
		Con_DrawString(skip * 8, y, chatText+scroll, colorTable[COLOR_WHITE]);

		// Add the cursor frame
		if ((cls.realTime >> 8) & 1){
			// Find cursor position
			width -= 2;

			if (scroll)
				cursor = width - (Q_PrintStrlen(chatText) - con.chatPos);
			else
				cursor = con.chatPos;

			cursor -= (strlen(chatText) - Q_PrintStrlen(chatText));
			if (cursor > width)
				cursor = width;
			if (cursor < 0)
				cursor = 0;

			// Draw cursor
			Con_DrawChar((skip * 8) + (cursor * 8), y, '_', colorTable[COLOR_WHITE]);
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

	color_t	consoleColor = {0, 76, 127, 255};
	short	*text;
	int		ch, colorIndex;
	int		rows, row;
	int		i, x, y;

	if (!con.displayFrac){
		// Only draw notify lines in game
		if (cls.state != CA_ACTIVE || cls.cinematicPlaying)
			return;

		if (CL_GetKeyDest() == KEY_GAME || CL_GetKeyDest() == KEY_MESSAGE)
			Con_DrawNotify();

		return;
	}

	con.visibleLines = 480 * con.displayFrac;

	// Draw the background
	R_DrawStretchPic(0, 0, 640, con.visibleLines, 0, 0, 1, 1, colorTable[COLOR_WHITE], cls.media.consoleMaterial);
	R_DrawStretchPic(0, con.visibleLines - 3, 640, 3, 0, 0, 1, 1, colorTable[COLOR_BLACK], cls.media.whiteMaterial);

	// Draw the version string
	Con_DrawString(640 - (strlen(Q2E_VERSION)+1) * 8, con.visibleLines - 20, Q2E_VERSION, consoleColor);

	// Draw the text
	rows = (con.visibleLines - 48) / 8;
	y = con.visibleLines - 48;

	// Draw arrows to show the buffer is backscrolled
	if (con.displayLine != con.currentLine){
		for (x = 0; x < con.lineWidth; x += 4)
			Con_DrawChar((x+1) * 8, y, '^', consoleColor);

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

		for (x = 0; x < CON_LINELEN; x++){
			ch = text[x] & 0xFF;
			if (!ch)
				break;

			if (ch == ' ')
				continue;

			colorIndex = (text[x] >> 8) & Q_COLOR_MASK;

			Con_DrawChar((x+1) * 8, y, ch, colorTable[colorIndex]);
		}
	}

	// Draw the input prompt, text, and cursor if desired
	Con_DrawInput();
}


// =====================================================================


/*
 =================
 Con_ToggleConsole_f
 =================
*/
void Con_ToggleConsole_f (void){

	Con_ClearNotify();
	Con_ClearInput();

	if (CL_GetKeyDest() == KEY_CONSOLE){
		if (UI_IsVisible())
			CL_SetKeyDest(KEY_MENU);
		else if (con.chatActive)
			CL_SetKeyDest(KEY_MESSAGE);
		else
			CL_SetKeyDest(KEY_GAME);
	}
	else
		CL_SetKeyDest(KEY_CONSOLE);
}

/*
 =================
 Con_Clear_f
 =================
*/
static void Con_Clear_f (void){

	int		i, j;

	for (i = 0; i < CON_LINES; i++){
		for (j = 0; j < CON_LINELEN; j++)
			con.text[i][j] = 0;
	}

	con.displayLine = con.currentLine;
}

/*
 =================
 Con_Dump_f

 Save the console contents out to a file
 =================
*/
static void Con_Dump_f (void){

	fileHandle_t	f;
	char			name[MAX_OSPATH];
	short			*text;
	char			line[CON_LINELEN];
	int				i, j;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: conDump <fileName>\n");
		return;
	}

	Q_strncpyz(name, Cmd_Argv(1), sizeof(name));
	Com_DefaultExtension(name, sizeof(name), ".txt");

	FS_OpenFile(name, &f, FS_WRITE);
	if (!f){
		Com_Printf("Couldn't open %s\n", name);
		return;
	}

	Com_Printf("Dumped console text to %s\n", name);

	// Skip empty lines
	for (i = con.currentLine - CON_LINES + 1; i <= con.currentLine; i++){
		if (con.text[i % CON_LINES][0] & 0xFF)
			break;
	}

	// Write the remaining lines
	for ( ; i <= con.currentLine; i++){
		text = con.text[i % CON_LINES];

		for (j = 0; j < CON_LINELEN; j++)
			line[j] = text[j] & 0xFF;

		FS_Printf(f, "%s\r\n", line);
	}

	FS_CloseFile(f);
}

/*
 =================
 Con_MessageMode_f
 =================
*/
static void Con_MessageMode_f (void){

	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || cl.demoPlaying)
		return;

	con.chatActive = true;
	con.chatTeam = false;

	CL_SetKeyDest(KEY_MESSAGE);
}

/*
 =================
 Con_MessageMode2_f
 =================
*/
static void Con_MessageMode2_f (void){

	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || cl.demoPlaying)
		return;

	con.chatActive = true;
	con.chatTeam = true;

	CL_SetKeyDest(KEY_MESSAGE);
}

/*
 =================
 Con_Init
 =================
*/
void Con_Init (void){

	// Register our variables and commands
	con_notifyTime = Cvar_Get("con_notifyTime", "3", 0, "Notify message time in seconds");
	con_speed = Cvar_Get("con_speed", "3", 0, "Speed at which the console is toggled in/out in seconds");
	con_noPrint = Cvar_Get("con_noPrint", "0", 0, "Don't print console messages");

	Cmd_AddCommand("toggleConsole", Con_ToggleConsole_f, "Toggle the console on/off");
	Cmd_AddCommand("clear", Con_Clear_f, "Clear the console");
	Cmd_AddCommand("conDump", Con_Dump_f, "Dump the console text to a file");
	Cmd_AddCommand("messageMode", Con_MessageMode_f, "Type a chat message");
	Cmd_AddCommand("messageMode2", Con_MessageMode2_f, "Type a team chat message");

	con.currentLine = CON_LINES - 1;
	con.displayLine = CON_LINES - 1;
	con.colorIndex = COLOR_WHITE;
	con.lineWidth = CON_LINELEN - 2;

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

	Cmd_RemoveCommand("toggleConsole");
	Cmd_RemoveCommand("clear");
	Cmd_RemoveCommand("conDump");
	Cmd_RemoveCommand("messageMode");
	Cmd_RemoveCommand("messageMode2");

	// Free the auto-complete list
	for (i = 0; i < con.cmdCount; i++)
		FreeString(con.cmdList[i]);

	memset(&con, 0, sizeof(console_t));
}
