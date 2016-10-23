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


// cmd.c -- script command processing module


#include "qcommon.h"


#define ALIAS_HASHSIZE		32

#define	MAX_ALIAS_COUNT		16

typedef struct cmdAlias_s {
	char				*name;
	char				*command;
	struct cmdAlias_s	*next;
	struct cmdAlias_s	*nextHash;
} cmdAlias_t;

static cmdAlias_t	*cmd_aliasHash[ALIAS_HASHSIZE];
static cmdAlias_t	*cmd_alias;

static int			cmd_aliasCount;		// For detecting runaway loops

static int			cmd_wait;


/*
 =======================================================================

 COMMAND BUFFER

 =======================================================================
*/

#define CBUF_SIZE		65536

typedef struct {
	char	buffer[CBUF_SIZE];
	int		size;
} cmdBuffer_t;

static cmdBuffer_t	cmd_text;
static char			cmd_deferTextBuffer[CBUF_SIZE];


/*
 =================
 Cbuf_AddText

 Adds command text at the end of the buffer
 =================
*/
void Cbuf_AddText (const char *text){

	int		len;
	
	len = strlen(text);

	if (cmd_text.size + len >= CBUF_SIZE){
		Com_Printf("Cbuf_AddText: overflow\n");
		return;
	}

	memcpy(cmd_text.buffer + cmd_text.size, text, len);
	cmd_text.size += len;
}

/*
 =================
 Cbuf_InsertText

 Inserts command text at the beginning of the buffer
 =================
*/
void Cbuf_InsertText (const char *text){

	char	temp[CBUF_SIZE];
	int		tempLen;
	int		len;

	len = strlen(text);

	if (cmd_text.size + len >= CBUF_SIZE){
		Com_Printf("Cbuf_InsertText: overflow\n");
		return;
	}

	// Copy off any commands still remaining in the exec buffer
	tempLen = cmd_text.size;
	if (tempLen)
		memcpy(temp, cmd_text.buffer, cmd_text.size);
		
	// Add the entire text
	memcpy(cmd_text.buffer, text, len);
	cmd_text.size = len;

	// Add the copied off data
	if (tempLen){
		memcpy(cmd_text.buffer + cmd_text.size, temp, tempLen);
		cmd_text.size += tempLen;
	}
}

/*
 =================
 Cbuf_ExecuteText
 =================
*/
void Cbuf_ExecuteText (cbufExec_t execWhen, const char *text){

	switch (execWhen){
	case EXEC_APPEND:
		Cbuf_AddText(text);
		break;
	case EXEC_INSERT:
		Cbuf_InsertText(text);
		break;
	case EXEC_NOW:
		Cmd_ExecuteString(text);
		break;
	default:
		Com_Error(ERR_FATAL, "Cbuf_ExecuteText: bad execWhen");
	}
}

/*
 =================
 Cbuf_Execute
 =================
*/
void Cbuf_Execute (void){

	char	*text;
	char	line[MAX_STRING_CHARS];
	int		i, quotes;

	cmd_aliasCount = 0;		// Don't allow infinite alias loops

	while (cmd_text.size){
		if (cmd_wait){
			// Skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// Find a \n or ; line break
		text = cmd_text.buffer;

		quotes = 0;
		for (i = 0; i < cmd_text.size; i++){
			if (text[i] == '"')
				quotes++;
			if (text[i] == ';' && !(quotes & 1))
				break;	// Don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		if (i > sizeof(line)-1)
			i = sizeof(line)-1;

		memcpy(line, text, i);
		line[i] = 0;

		// Delete the text from the command buffer and move remaining 
		// commands down. This is necessary because commands (exec, 
		// alias) can insert data at the beginning of the text buffer
		if (i == cmd_text.size)
			cmd_text.size = 0;
		else {
			i++;
			cmd_text.size -= i;
			memmove(text, text+i, cmd_text.size);
		}

		// Execute the command line
		Cmd_ExecuteString(line);
	}
}

/*
 =================
 Cbuf_CopyToDefer
 =================
*/
void Cbuf_CopyToDefer (void){

	memcpy(cmd_deferTextBuffer, cmd_text.buffer, cmd_text.size);
	cmd_deferTextBuffer[cmd_text.size] = 0;
	cmd_text.size = 0;
}

/*
 =================
 Cbuf_InsertFromDefer
 =================
*/
void Cbuf_InsertFromDefer (void){

	Cbuf_InsertText(cmd_deferTextBuffer);
	cmd_deferTextBuffer[0] = 0;
}


/*
 =======================================================================

 COMMAND EXECUTION

 =======================================================================
*/

#define CMDS_HASHSIZE	128

typedef struct cmd_s {
	char			*name;
	void			(*function)(void);
	struct cmd_s	*next;
	struct cmd_s	*nextHash;
} cmd_t;

static cmd_t	*cmd_functionsHash[CMDS_HASHSIZE];
static cmd_t	*cmd_functions;

static int		cmd_argc;
static char		*cmd_argv[MAX_STRING_TOKENS];
static char		cmd_args[MAX_STRING_CHARS];


/*
 =================
 Cmd_Argc
 =================
*/
int	Cmd_Argc (void){

	return cmd_argc;
}

/*
 =================
 Cmd_Argv
 =================
*/
char *Cmd_Argv (int arg){

	if (arg < 0 || arg >= cmd_argc)
		return "";

	return cmd_argv[arg];	
}

/*
 =================
 Cmd_Args
 =================
*/
char *Cmd_Args (void){

	return cmd_args;
}

/*
 =================
 Cmd_MacroExpandString

 $Cvars will be expanded unless they are in a quoted token
 =================
*/
static char *Cmd_MacroExpandString (const char *text){

	static char	expanded[MAX_STRING_CHARS];
	char		temporary[MAX_STRING_CHARS];
	char		*scan, *start, *token;
	int			i, len, count;
	qboolean	inQuote = false;

	len = strlen(text);
	if (len >= MAX_STRING_CHARS){
		Com_Printf("Line exceeded %i chars, discarded\n", MAX_STRING_CHARS);
		return NULL;
	}

	scan = (char *)text;
	count = 0;

	for (i = 0; i < len; i++){
		if (scan[i] == '"')
			inQuote ^= 1;
		if (inQuote)
			continue;	// Don't expand inside quotes
		if (scan[i] != '$')
			continue;

		// Scan out the complete macro
		start = scan + (i+1);
		token = Com_Parse(&start);
		if (!start)
			continue;

		token = Cvar_VariableString(token);

		len += strlen(token);
		if (len >= MAX_STRING_CHARS){
			Com_Printf("Expanded line exceeded %i chars, discarded\n", MAX_STRING_CHARS);
			return NULL;
		}

		Q_strncpyz(temporary, scan, i+1);
		Q_strncatz(temporary, token, sizeof(temporary));
		Q_strncatz(temporary, start, sizeof(temporary));

		Q_strncpyz(expanded, temporary, sizeof(expanded));
		scan = expanded;
		i--;

		if (++count == 100){
			Com_Printf("Macro expansion loop, discarded\n");
			return NULL;
		}
	}

	if (inQuote){
		Com_Printf("Line has unmatched quote, discarded\n");
		return NULL;
	}

	return scan;
}

/*
 =================
 Cmd_TokenizeString

 Parses the given string into command line tokens
 =================
*/
void Cmd_TokenizeString (const char *text){

	int		i;
	char	*txt, *token;

	// Clear the args from the last string
	for (i = 0; i < cmd_argc; i++)
		FreeString(cmd_argv[i]);

	cmd_argc = 0;
	cmd_args[0] = 0;

	if (!text)
		return;

	txt = (char *)text;
	while (1){
		// Skip whitespace up to a \n
		while (*txt && *txt <= ' ' && *txt != '\n')
			txt++;

		// A newline separates commands in the buffer
		if (*txt == '\n'){
			txt++;
			break;
		}

		if (!*txt)
			break;

		token = Com_Parse(&txt);
		if (!txt)
			break;

		if (cmd_argc < MAX_STRING_TOKENS){
			cmd_argv[cmd_argc] = CopyString(token);
			cmd_argc++;
		}
	}

	// Set cmd_args to everything after the first arg
	for (i = 1; i < cmd_argc; i++){
		Q_strncatz(cmd_args, cmd_argv[i], sizeof(cmd_args));
		if (i != cmd_argc - 1)
			Q_strncatz(cmd_args, " ", sizeof(cmd_args));
	}
}

/*
 =================
 Cmd_ExecuteString

 A complete command line has been parsed, so try to execute it
 =================
*/
void Cmd_ExecuteString (const char *text){

	cmd_t		*cmd;
	cmdAlias_t	*alias;
	unsigned	hashKey;

	// Macro expand the text
	text = Cmd_MacroExpandString(text);

	// Tokenize
	Cmd_TokenizeString(text);

	// Execute the command line
	if (!cmd_argc)
		return;		// No tokens

	// Check functions
	hashKey = Com_HashKey(cmd_argv[0], CMDS_HASHSIZE);

	for (cmd = cmd_functionsHash[hashKey]; cmd; cmd = cmd->nextHash){
		if (!Q_stricmp(cmd->name, cmd_argv[0])){
			if (cmd->function)
				cmd->function();
			else
				Cmd_ExecuteString(va("cmd %s", text));	// Forward to server command

			return;
		}
	}

	// Check alias
	hashKey = Com_HashKey(cmd_argv[0], ALIAS_HASHSIZE);

	for (alias = cmd_aliasHash[hashKey]; alias; alias = alias->nextHash){
		if (!Q_stricmp(alias->name, cmd_argv[0])){
			if (++cmd_aliasCount == MAX_ALIAS_COUNT){
				Com_Printf("MAX_ALIAS_COUNT hit\n");
				return;
			}

			Cbuf_InsertText(alias->command);
			return;
		}
	}

	// Check cvars
	if (Cvar_Command())
		return;

	// Send it as a server command if we are connected
	CL_ForwardToServer();
}

/*
 =================
 Cmd_Exists
 =================
*/
qboolean Cmd_Exists (const char *name){

	cmd_t		*cmd;
	unsigned	hashKey;

	hashKey = Com_HashKey(name, CMDS_HASHSIZE);

	for (cmd = cmd_functionsHash[hashKey]; cmd; cmd = cmd->nextHash){
		if (!Q_stricmp(cmd->name, name))
			return true;
	}

	return false;
}

/*
 =================
 Cmd_CompleteCommand
 =================
*/
void Cmd_CompleteCommand (const char *partial, void (*callback)(const char *found)){

	cmd_t		*cmd;
	cmdAlias_t	*alias;
	int			len;

	len = strlen(partial);

	// Find matching commands
	for (cmd = cmd_functions; cmd; cmd = cmd->next){
		if (Q_strnicmp(cmd->name, partial, len))
			continue;

		callback(cmd->name);
	}

	// Find matching aliases
	for (alias = cmd_alias; alias; alias = alias->next){
		if (Q_strnicmp(alias->name, partial, len))
			continue;

		callback(alias->name);
	}
}

/*
 =================
 Cmd_AddCommand
 =================
*/
void Cmd_AddCommand (const char *name, void (*function)(void)){

	cmd_t		*cmd;
	unsigned	hashKey;

	// Fail if the command is a variable name
	if (Cvar_FindVar(name)){
		Com_DPrintf("Cmd_AddCommand: '%s' already defined as a cvar\n", name);
		return;
	}
	
	// Fail if the command already exists
	hashKey = Com_HashKey(name, CMDS_HASHSIZE);

	for (cmd = cmd_functionsHash[hashKey]; cmd; cmd = cmd->nextHash){
		if (!Q_stricmp(cmd->name, name)){
			Com_DPrintf("Cmd_AddCommand: '%s' already defined\n", name);
			return;
		}
	}

	cmd = Z_MallocSmall(sizeof(cmd_t));
	cmd->name = CopyString(name);
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;

	// Add to hash table
	cmd->nextHash = cmd_functionsHash[hashKey];
	cmd_functionsHash[hashKey] = cmd;
}

/*
 =================
 Cmd_RemoveCommand
 =================
*/
void Cmd_RemoveCommand (const char *name){

	cmd_t		*cmd, **back, **backHash;
	unsigned	hashKey;

	hashKey = Com_HashKey(name, CMDS_HASHSIZE);

	backHash = &cmd_functionsHash[hashKey];
	while (1){
		cmd = *backHash;
		if (!cmd){
			Com_DPrintf("Cmd_RemoveCommand: '%s' not added\n", name);
			return;
		}

		if (!Q_stricmp(cmd->name, name)){
			*backHash = cmd->nextHash;

			back = &cmd_functions;
			while (1){
				cmd = *back;
				if (!cmd)
					break;

				if (!Q_stricmp(cmd->name, name)){
					*back = cmd->next;

					FreeString(cmd->name);
					Z_Free(cmd);
					break;
				}

				back = &cmd->next;
			}

			return;
		}

		backHash = &cmd->nextHash;
	}
}

/*
 =================
 Cmd_Wait_f

 Causes execution of the remainder of the command buffer to be delayed 
 during the given number of frames. This allows commands like:
 bind g "+attack ; wait ; -attack"
 =================
*/
void Cmd_Wait_f (void){

	if (Cmd_Argc() == 1)
		cmd_wait += 1;
	else
		cmd_wait += atoi(Cmd_Argv(1));

	if (cmd_wait < 0)
		cmd_wait = 0;
}

/*
 =================
 Cmd_Exec_f
 =================
*/
void Cmd_Exec_f (void){

	char	name[MAX_QPATH];
	char	*buffer;

	if (Cmd_Argc() != 2){
		Com_Printf("exec <filename> : execute a script file\n");
		return;
	}

	Q_strncpyz(name, Cmd_Argv(1), sizeof(name));
	Com_DefaultExtension(name, sizeof(name), ".cfg");

	// Load the file
	FS_LoadFile(name, (void **)&buffer);
	if (!buffer){
		Com_Printf("Couldn't exec %s\n", name);
		return;
	}

	Com_Printf("Execing %s\n", name);

	Cbuf_InsertText(buffer);

	FS_FreeFile(buffer);
}

/*
 =================
 Cmd_Echo_f

 Just prints the rest of the line to the console
 =================
*/
void Cmd_Echo_f (void){

	int		i;
	
	for (i = 1; i < Cmd_Argc(); i++)
		Com_Printf("%s ", Cmd_Argv(i));
	
	Com_Printf("\n");
}

/*
 =================
 Cmd_Alias_f

 Creates a new command that executes a command string (possibly ;
 separated)
 =================
*/
void Cmd_Alias_f (void){

	cmdAlias_t	*alias;
	unsigned	hashKey;
	char		*name, command[MAX_STRING_CHARS];
	int			i;

	if (Cmd_Argc() == 1){
		Com_Printf("Current alias commands:\n");

		for (alias = cmd_alias; alias; alias = alias->next)
			Com_Printf("%s : %s", alias->name, alias->command);

		return;
	}

	name = Cmd_Argv(1);

	// If the alias already exists, reuse it
	hashKey = Com_HashKey(name, ALIAS_HASHSIZE);

	for (alias = cmd_aliasHash[hashKey]; alias; alias = alias->next){
		if (!Q_stricmp(alias->name, name)){
			FreeString(alias->command);
			break;
		}
	}

	if (!alias){
		// Create a new alias
		alias = Z_MallocSmall(sizeof(cmdAlias_t));
		alias->name = CopyString(name);
		alias->next = cmd_alias;
		cmd_alias = alias;

		// Add to hash table
		alias->nextHash = cmd_aliasHash[hashKey];
		cmd_aliasHash[hashKey] = alias;
	}

	// Copy the rest of the command line
	command[0] = 0;
	for (i = 2; i < Cmd_Argc(); i++){
		Q_strncatz(command, Cmd_Argv(i), sizeof(command));
		if (i != Cmd_Argc() - 1)
			Q_strncatz(command, " ", sizeof(command));
	}
	Q_strncatz(command, "\n", sizeof(command));

	alias->command = CopyString(command);
}

/*
 =================
 Cmd_List_f
 =================
*/
void Cmd_List_f (void){

	cmd_t	*cmd;
	int		total = 0, found = 0;
	char	*pattern = NULL;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: cmdlist [filter]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		pattern = Cmd_Argv(1);

	for (cmd = cmd_functions; cmd; cmd = cmd->next, total++){
		if (pattern){
			if (!Q_GlobMatch(pattern, cmd->name, false))
				continue;
		}

		found++;

		Com_Printf("%s\n", cmd->name);
	}

	Com_Printf("%i cmds found (%i total cmds)\n", found, total);
}

/*
 =================
 Cmd_Init
 =================
*/
void Cmd_Init (void){

	Cmd_AddCommand("wait", Cmd_Wait_f);
	Cmd_AddCommand("exec", Cmd_Exec_f);
	Cmd_AddCommand("echo", Cmd_Echo_f);
	Cmd_AddCommand("alias", Cmd_Alias_f);
	Cmd_AddCommand("cmdlist", Cmd_List_f);
}

/*
 =================
 Cmd_Shutdown
 =================
*/
void Cmd_Shutdown (void){

	Cmd_RemoveCommand("wait");
	Cmd_RemoveCommand("exec");
	Cmd_RemoveCommand("echo");
	Cmd_RemoveCommand("alias");
	Cmd_RemoveCommand("cmdlist");
}
