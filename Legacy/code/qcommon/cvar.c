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


// cvar.c -- dynamic variable tracking


#include "qcommon.h"


#define CVARS_HASHSIZE	128

cvar_t		*cvar_varsHash[CVARS_HASHSIZE];
cvar_t		*cvar_vars;

qboolean	cvar_allowCheats = true;
qboolean	cvar_userInfoModified = false;


/*
 =================
 Cvar_WriteVariables

 Appends lines containing "seta variable value" for all variables with
 the archive flag set to true
 =================
*/
void Cvar_WriteVariables (fileHandle_t f){

	cvar_t	*var;

	for (var = cvar_vars; var; var = var->next){
		if (!(var->flags & CVAR_ARCHIVE))
			continue;

		if (!var->latchedString)
			FS_Printf(f, "seta %s \"%s\"\r\n", var->name, var->string);
		else
			FS_Printf(f, "seta %s \"%s\"\r\n", var->name, var->latchedString);
	}
}

/*
 =================
 Cvar_CompleteVariable
 =================
*/
void Cvar_CompleteVariable (const char *partial, void (*callback)(const char *found)){

	cvar_t	*var;
	int		len;

	len = strlen(partial);

	// Find matching variables
	for (var = cvar_vars; var; var = var->next){
		if (Q_strnicmp(var->name, partial, len))
			continue;

		callback(var->name);
	}
}

/*
 =================
 Cvar_FindVar
 =================
*/
cvar_t *Cvar_FindVar (const char *name){

	cvar_t		*var;
	unsigned	hashKey;

	hashKey = Com_HashKey(name, CVARS_HASHSIZE);

	for (var = cvar_varsHash[hashKey]; var; var = var->nextHash){
		if (!Q_stricmp(var->name, name))
			return var;
	}

	return NULL;
}

/*
 =================
 Cvar_VariableString
 =================
*/
char *Cvar_VariableString (const char *name){

	cvar_t *var;
	
	var = Cvar_FindVar(name);
	if (!var)
		return "";

	return var->string;
}

/*
 =================
 Cvar_VariableValue
 =================
*/
float Cvar_VariableValue (const char *name){

	cvar_t *var;
	
	var = Cvar_FindVar(name);
	if (!var)
		return 0;

	return atof(var->string);
}

/*
 =================
 Cvar_VariableInteger
 =================
*/
int Cvar_VariableInteger (const char *name){

	cvar_t	*var;

	var = Cvar_FindVar(name);
	if (!var)
		return 0;

	return atoi(var->string);
}

/*
 =================
 Cvar_Get

 If the variable already exists, the value will not be set.
 The flags will be OR'ed in if the variable exists.
 =================
*/
cvar_t *Cvar_Get (const char *name, const char *value, int flags){

	cvar_t		*var;
	unsigned	hashKey;
	int			i;

	var = Cvar_FindVar(name);
	if (var){
		var->flags |= flags;
		var->modified = true;

		// Update latched cvars
		if (var->latchedString){
			FreeString(var->string);

			var->string = var->latchedString;
			var->latchedString = NULL;
			var->value = atof(var->string);
			var->integer = atoi(var->string);
		}

		if (value){
			// Reset string should always use values set internally
			FreeString(var->resetString);
			var->resetString = CopyString(value);

			// Read only cvars should always use values set internally
			if (var->flags & CVAR_ROM){
				FreeString(var->string);
				var->string = CopyString(value);
			}
		}

		return var;
	}

	if (!value)
		return NULL;

	// Check for invalid name
	for (i = 0; name[i]; i++){
		if (name[i] >= 'a' && name[i] <= 'z')
			continue;
		if (name[i] >= 'A' && name[i] <= 'Z')
			continue;
		if (name[i] >= '0' && name[i] <= '9')
			continue;
		if (name[i] == '_')
			continue;

		Com_Printf("Invalid cvar name '%s'\n", name);
		return NULL;
	}

	// Check for cmd override
	if (Cmd_Exists(name)){
		Com_Printf("'%s' already defined as a command\n", name);
		return NULL;
	}

	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO)){
		if (strchr(value, '\\') || strchr(value, '\"') || strchr(value, ';')){
			Com_Printf("Invalid info cvar value\n");
			return NULL;
		}
	}
	
	var = Z_MallocSmall(sizeof(cvar_t));
	var->name = CopyString(name);
	var->string = CopyString(value);
	var->resetString = CopyString(value);
	var->flags = flags;
	var->modified = true;
	var->value = atof(var->string);
	var->integer = atoi(var->string);
	var->next = cvar_vars;
	cvar_vars = var;

	// Add to hash table
	hashKey = Com_HashKey(name, CVARS_HASHSIZE);

	var->nextHash = cvar_varsHash[hashKey];
	cvar_varsHash[hashKey] = var;

	return var;
}

/*
 =================
 Cvar_Set2
 =================
*/
cvar_t *Cvar_Set2 (const char *name, const char *value, int flags, qboolean force){

	cvar_t	*var;

	Com_DPrintf("Cvar_Set2: %s \"%s\"\n", name, value);

	var = Cvar_FindVar(name);
	if (!var)	// Create it
		return Cvar_Get(name, value, flags);

	var->flags |= flags;

	if (var->flags & CVAR_ARCHIVE)
		com_configModified = true;		// Save at next opportunity

	if (var->flags & CVAR_USERINFO)
		cvar_userInfoModified = true;	// Transmit at next opportunity

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO)){
		if (strchr(value, '\\') || strchr(value, '\"') || strchr(value, ';')){
			Com_Printf("Invalid info cvar value\n");
			return var;
		}
	}

	if (!force){
		if (var->flags & CVAR_INIT){
			Com_Printf("%s is write protected\n", name);
			return var;
		}

		if (var->flags & CVAR_ROM){
			Com_Printf("%s is read only\n", name);
			return var;
		}

		if (var->flags & CVAR_CHEAT && !cvar_allowCheats){
			Com_Printf("%s is cheat protected\n", name);
			return var;
		}

		if (var->flags & CVAR_LATCH){
			if (var->latchedString){
				if (!Q_stricmp(var->latchedString, value))
					return var;

				FreeString(var->latchedString);
				var->latchedString = NULL;
			}

			if (!Q_stricmp(var->string, value))
				return var;

			var->latchedString = CopyString(value);

			Com_Printf("%s will be changed upon restarting\n", name);
			return var;
		}
	}
	else {
		if (var->latchedString){
			FreeString(var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!Q_stricmp(var->string, value))
		return var;		// Not changed

	FreeString(var->string);

	var->string = CopyString(value);
	var->modified = true;
	var->value = atof(var->string);
	var->integer = atoi(var->string);

	// Stupid hack to workaround our changes to cvar_t
	if (var->flags & CVAR_GAMEVAR)
		Cvar_GameUpdateOrCreate(var);

	return var;
}

/*
 =================
 Cvar_Set
 =================
*/
cvar_t *Cvar_Set (const char *name, const char *value){

	return Cvar_Set2(name, value, 0, false);
}

/*
 =================
 Cvar_SetValue
 =================
*/
cvar_t *Cvar_SetValue (const char *name, float value){

	return Cvar_Set2(name, va("%f", value), 0, false);
}

/*
 =================
 Cvar_SetInteger
 =================
*/
cvar_t *Cvar_SetInteger (const char *name, int integer){

	return Cvar_Set2(name, va("%i", integer), 0, false);
}

/*
 =================
 Cvar_ForceSet
 =================
*/
cvar_t *Cvar_ForceSet (const char *name, const char *value){

	return Cvar_Set2(name, value, 0, true);
}

/*
 =================
 Cvar_FixCheatVars
 =================
*/
void Cvar_FixCheatVars (qboolean allowCheats){

	cvar_t	*var;

	if (cvar_allowCheats == allowCheats)
		return;
	cvar_allowCheats = allowCheats;

	if (cvar_allowCheats)
		return;

	for (var = cvar_vars; var; var = var->next){
		if (!(var->flags & CVAR_CHEAT))
			continue;

		if (!Q_stricmp(var->string, var->resetString))
			continue;

		Cvar_Set2(var->name, var->resetString, 0, true);
	}
}

/*
 =================
 Cvar_Command

 Handles variable inspection and changing from the console
 =================
*/
qboolean Cvar_Command (void){

	cvar_t *var;

	// Check variables
	var = Cvar_FindVar(Cmd_Argv(0));
	if (!var)
		return false;
		
	// Perform a variable print or set
	if (Cmd_Argc() == 1){
		Com_Printf("\"%s\" is: \"%s" S_COLOR_WHITE "\" default: \"%s" S_COLOR_WHITE "\"", var->name, var->string, var->resetString);
		if (var->latchedString)
			Com_Printf(" latched: \"%s" S_COLOR_WHITE "\"", var->latchedString);
		Com_Printf("\n");

		return true;
	}

	Cvar_Set2(var->name, Cmd_Argv(1), 0, false);

	return true;
}

/*
 =================
 Cvar_BitInfo
 =================
*/
char *Cvar_BitInfo (int bit){

	static char	info[MAX_INFO_STRING];
	char		value[MAX_INFO_VALUE];
	cvar_t		*var;

	info[0] = 0;

	for (var = cvar_vars; var; var = var->next){
		if (!(var->flags & bit))
			continue;

		if (Q_PrintStrlen(var->string) != strlen(var->string)){
			Q_snprintfz(value, sizeof(value), "%s%s", var->string, S_COLOR_WHITE);
			Info_SetValueForKey(info, var->name, value);
		}
		else
			Info_SetValueForKey(info, var->name, var->string);
	}

	return info;
}

/*
 =================
 Cvar_UserInfo

 Returns an info string containing all the CVAR_USERINFO cvars
 =================
*/
char *Cvar_UserInfo (void){

	return Cvar_BitInfo(CVAR_USERINFO);
}

/*
 =================
 Cvar_ServerInfo

 Returns an info string containing all the CVAR_SERVERINFO cvars
 =================
*/
char *Cvar_ServerInfo (void){

	return Cvar_BitInfo(CVAR_SERVERINFO);
}

/*
 =================
 Cvar_Get_f
 =================
*/
void Cvar_Get_f (void){

	cvar_t *var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: get <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Com_Printf("\"%s\" is: \"%s" S_COLOR_WHITE "\" default: \"%s" S_COLOR_WHITE "\"", var->name, var->string, var->resetString);
	if (var->latchedString)
		Com_Printf(" latched: \"%s" S_COLOR_WHITE "\"", var->latchedString);
	Com_Printf("\n");
}

/*
 =================
 Cvar_Set_f

 Allows setting and defining of arbitrary cvars from console
 =================
*/
void Cvar_Set_f (void){

	if (Cmd_Argc() < 3 || Cmd_Argc() > 4){
		Com_Printf("Usage: set <variable> <value> [a / u / s]\n");
		return;
	}

	if (Cmd_Argc() == 4){
		if (!Q_stricmp(Cmd_Argv(3), "a"))
			Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), CVAR_ARCHIVE, false);
		else if (!Q_stricmp(Cmd_Argv(3), "u"))
			Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), CVAR_USERINFO, false);
		else if (!Q_stricmp(Cmd_Argv(3), "s"))
			Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), CVAR_SERVERINFO, false);
		else {
			Com_Printf("Flags can only be 'a', 'u' or 's'\n");
			return;
		}
	}
	else
		Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), 0, false);
}

/*
 =================
 Cvar_Seta_f
 =================
*/
void Cvar_Seta_f (void){

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: seta <variable> <value>\n");
		return;
	}

	Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), CVAR_ARCHIVE, false);
}

/*
 =================
 Cvar_Setu_f
 =================
*/
void Cvar_Setu_f (void){

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: setu <variable> <value>\n");
		return;
	}

	Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), CVAR_USERINFO, false);
}

/*
 =================
 Cvar_Sets_f
 =================
*/
void Cvar_Sets_f (void){

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: sets <variable> <value>\n");
		return;
	}

	Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), CVAR_SERVERINFO, false);
}

/*
 =================
 Cvar_Add_f
 =================
*/
void Cvar_Add_f (void){

	cvar_t	*var;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: add <variable> <value>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set2(var->name, va("%f", var->value + atof(Cmd_Argv(2))), 0, false);
}

/*
 =================
 Cvar_Sub_f
 =================
*/
void Cvar_Sub_f (void){

	cvar_t	*var;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: sub <variable> <value>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set2(var->name, va("%f", var->value - atof(Cmd_Argv(2))), 0, false);
}

/*
 =================
 Cvar_Inc_f
 =================
*/
void Cvar_Inc_f (void){

	cvar_t	*var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: inc <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set2(var->name, va("%i", var->integer + 1), 0, false);
}

/*
 =================
 Cvar_Dec_f
 =================
*/
void Cvar_Dec_f (void){

	cvar_t	*var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: dec <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set2(var->name, va("%i", var->integer - 1), 0, false);
}

/*
 =================
 Cvar_Toggle_f
 =================
*/	
void Cvar_Toggle_f (void){

	cvar_t	*var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: toggle <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set2(var->name, va("%i", !var->integer), 0, false);
}

/*
 =================
 Cvar_Reset_f
 =================
*/
void Cvar_Reset_f (void){

	cvar_t *var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: reset <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set2(var->name, var->resetString, 0, false);
}

/*
 =================
 Cvar_Restart_f
 =================
*/
void Cvar_Restart_f (void){

	cvar_t *var;

	for (var = cvar_vars; var; var = var->next){
		if (var->flags & (CVAR_INIT | CVAR_ROM))
			continue;

		Cvar_Set2(var->name, var->resetString, 0, false);
	}
}

/*
 =================
 Cvar_List_f
 =================
*/
void Cvar_List_f (void){

	cvar_t	*var;
	int		total = 0, found = 0;
	char	*pattern = NULL;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: cvarlist [filter]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		pattern = Cmd_Argv(1);

	for (var = cvar_vars; var; var = var->next, total++){
		if (pattern){
			if (!Q_GlobMatch(pattern, var->name, false))
				continue;
		}

		found++;
		
		if (var->flags & CVAR_USERINFO)
			Com_Printf("U");
		else
			Com_Printf(" ");

		if (var->flags & CVAR_SERVERINFO)
			Com_Printf("S");
		else
			Com_Printf(" ");

		if (var->flags & CVAR_INIT)
			Com_Printf("I");
		else
			Com_Printf(" ");

		if (var->flags & CVAR_ROM)
			Com_Printf("R");
		else
			Com_Printf(" ");

		if (var->flags & CVAR_ARCHIVE)
			Com_Printf("A");
		else
			Com_Printf(" ");

		if (var->flags & CVAR_LATCH)
			Com_Printf("L");
		else
			Com_Printf(" ");

		if (var->flags & CVAR_CHEAT)
			Com_Printf("C");
		else
			Com_Printf(" ");

		Com_Printf(" %s \"%s\"\n", var->name, var->string);
	}

	Com_Printf("%i cvars found (%i total cvars)\n", found, total);
}

/*
 =================
 Cvar_Init
 =================
*/
void Cvar_Init (void){

	Cmd_AddCommand("get", Cvar_Get_f);
	Cmd_AddCommand("set", Cvar_Set_f);
	Cmd_AddCommand("seta", Cvar_Seta_f);
	Cmd_AddCommand("setu", Cvar_Setu_f);
	Cmd_AddCommand("sets", Cvar_Sets_f);
	Cmd_AddCommand("add", Cvar_Add_f);
	Cmd_AddCommand("sub", Cvar_Sub_f);
	Cmd_AddCommand("inc", Cvar_Inc_f);
	Cmd_AddCommand("dec", Cvar_Dec_f);
	Cmd_AddCommand("toggle", Cvar_Toggle_f);
	Cmd_AddCommand("reset", Cvar_Reset_f);
	Cmd_AddCommand("cvar_restart", Cvar_Restart_f);
	Cmd_AddCommand("cvarlist", Cvar_List_f);
}

/*
 =================
 Cvar_Shutdown
 =================
*/
void Cvar_Shutdown (void){

	Cmd_RemoveCommand("get");
	Cmd_RemoveCommand("set");
	Cmd_RemoveCommand("seta");
	Cmd_RemoveCommand("setu");
	Cmd_RemoveCommand("sets");
	Cmd_RemoveCommand("add");
	Cmd_RemoveCommand("sub");
	Cmd_RemoveCommand("inc");
	Cmd_RemoveCommand("dec");
	Cmd_RemoveCommand("toggle");
	Cmd_RemoveCommand("reset");
	Cmd_RemoveCommand("cvar_restart");
	Cmd_RemoveCommand("cvarlist");
}


// =====================================================================

gamecvar_t	*game_vars;


/*
 =================
 Cvar_GameFindVar
 =================
*/
gamecvar_t *Cvar_GameFindVar (char *name){

	gamecvar_t	*var;

	for (var = game_vars; var; var = var->next){
		if (!Q_stricmp(var->name, name))
			return var;
	}

	return NULL;
}

/*
 =================
 Cvar_GameUpdateOrCreate
 =================
*/
gamecvar_t *Cvar_GameUpdateOrCreate (cvar_t *var){

	gamecvar_t	*gameVar;

	gameVar = Cvar_GameFindVar(var->name);
	if (gameVar){
		if (gameVar->string)
			FreeString(gameVar->string);
		if (gameVar->latched_string)
			FreeString(gameVar->latched_string);

		gameVar->name = CopyString(var->name);
		gameVar->string = CopyString(var->string);
		gameVar->latched_string = (var->latchedString) ? CopyString(var->latchedString) : NULL;
		gameVar->flags = var->flags;
		gameVar->modified = var->modified;
		gameVar->value = var->value;

		return gameVar;
	}

	gameVar = Z_MallocSmall(sizeof(gamecvar_t));
	gameVar->name = CopyString(var->name);
	gameVar->string = CopyString(var->string);
	gameVar->latched_string = (var->latchedString) ? CopyString(var->latchedString) : NULL;
	gameVar->flags = var->flags;
	gameVar->modified = var->modified;
	gameVar->value = var->value;

	gameVar->next = game_vars;
	game_vars = gameVar;

	return gameVar;
}

/*
 =================
 Cvar_GameGet
 =================
*/
gamecvar_t *Cvar_GameGet (char *name, char *value, int flags){

	cvar_t	*var;

	// HACK: the game library may want to know the game dir
	if (!Q_stricmp(name, "game"))
		name = "fs_game";

	var = Cvar_Get(name, value, flags | CVAR_GAMEVAR);
	if (!var)
		return NULL;

	return Cvar_GameUpdateOrCreate(var);
}

/*
 =================
 Cvar_GameSet
 =================
*/
gamecvar_t *Cvar_GameSet (char *name, char *value){

	cvar_t	*var;

	var = Cvar_Set2(name, value, CVAR_GAMEVAR, true);
	if (!var)
		return NULL;

	return Cvar_GameUpdateOrCreate(var);
}

/*
 =================
 Cvar_GameForceSet
 =================
*/
gamecvar_t *Cvar_GameForceSet (char *name, char *value){

	cvar_t	*var;

	var = Cvar_Set2(name, value, CVAR_GAMEVAR, true);
	if (!var)
		return NULL;

	return Cvar_GameUpdateOrCreate(var);
}
