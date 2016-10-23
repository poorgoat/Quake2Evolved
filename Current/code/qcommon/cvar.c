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


#define CVARS_HASH_SIZE		1024

#define MAX_LIST_CVARS		4096

cvar_t		*cvar_varsHashTable[CVARS_HASH_SIZE];
cvar_t		*cvar_vars;

qboolean	cvar_allowCheats = true;
int			cvar_modifiedFlags = 0;


/*
 =================
 Cvar_SortListCvars
 =================
*/
static int Cvar_SortListCvars (const cvar_t **cvar1, const cvar_t **cvar2){

	return Q_strcmp((*cvar1)->name, (*cvar2)->name);
}

/*
 =================
 Cvar_WriteVariables

 Appends lines containing "seta variable value" for all variables with
 the archive flag set to true
 =================
*/
void Cvar_WriteVariables (fileHandle_t f){

	cvar_t	*cvar;

	for (cvar = cvar_vars; cvar; cvar = cvar->next){
		if (!(cvar->flags & CVAR_ARCHIVE))
			continue;

		if (!cvar->latchedValue)
			FS_Printf(f, "seta %s \"%s\"\r\n", cvar->name, cvar->value);
		else
			FS_Printf(f, "seta %s \"%s\"\r\n", cvar->name, cvar->latchedValue);
	}
}

/*
 =================
 Cvar_CompleteVariable
 =================
*/
void Cvar_CompleteVariable (const char *partial, void (*callback)(const char *)){

	cvar_t	*cvar;
	int		len;

	len = strlen(partial);

	// Find matching variables
	for (cvar = cvar_vars; cvar; cvar = cvar->next){
		if (Q_strnicmp(cvar->name, partial, len))
			continue;

		callback(cvar->name);
	}
}

/*
 =================
 Cvar_FindVariable
 =================
*/
cvar_t *Cvar_FindVariable (const char *name){

	cvar_t		*cvar;
	unsigned	hash;

	hash = Com_HashKey(name, CVARS_HASH_SIZE);

	for (cvar = cvar_varsHashTable[hash]; cvar; cvar = cvar->nextHash){
		if (!Q_stricmp(cvar->name, name))
			return cvar;
	}

	return NULL;
}

/*
 =================
 Cvar_Get

 If the variable already exists, the value will be set to the latched
 value (if any).
 The flags will be OR'ed in if the variable exists.
 =================
*/
cvar_t *Cvar_Get (const char *name, const char *value, int flags, const char *description){

	cvar_t		*cvar;
	unsigned	hash;

	cvar = Cvar_FindVariable(name);
	if (cvar){
		cvar->flags |= flags;

		// Update latched variables
		if (cvar->latchedValue){
			FreeString(cvar->value);

			cvar->value = cvar->latchedValue;
			cvar->floatValue = atof(cvar->latchedValue);
			cvar->integerValue = atoi(cvar->latchedValue);
			cvar->latchedValue = NULL;
		}

		// Reset value is always set internally
		if (value){
			FreeString(cvar->resetValue);
			cvar->resetValue = CopyString(value);

			// Read only variables always use values set internally
			if (cvar->flags & CVAR_ROM){
				FreeString(cvar->value);

				cvar->value = CopyString(value);
				cvar->floatValue = atof(cvar->value);
				cvar->integerValue = atoi(cvar->value);
			}
		}

		// Description is always set internally
		if (description){
			if (cvar->description)
				FreeString(cvar->description);

			cvar->description = CopyString(description);
		}

		cvar->modified = true;

		return cvar;
	}

	if (!value)
		return NULL;

	// Check for command override
	if (Cmd_Exists(name)){
		Com_Printf("'%s' already defined as a command\n", name);
		return NULL;
	}

	// Check for invalid name
	if (strchr(name, '\\') || strchr(name, '\"') || strchr(name, ';')){
		Com_Printf("Invalid variable name '%s'\n", name);
		return NULL;
	}

	// Check for invalid value
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO)){
		if (strchr(value, '\\') || strchr(value, '\"') || strchr(value, ';')){
			Com_Printf("Invalid info variable value '%s'\n", value);
			return NULL;
		}
	}

	// Allocate a new cvar
	cvar = Z_MallocSmall(sizeof(cvar_t));

	cvar->name = CopyString(name);
	cvar->value = CopyString(value);
	cvar->floatValue = atof(value);
	cvar->integerValue = atoi(value);
	cvar->resetValue = CopyString(value);
	cvar->latchedValue = NULL;
	cvar->flags = flags;
	cvar->description = (description) ? CopyString(description) : NULL;
	cvar->modified = true;

	// Link the variable in
	cvar->next = cvar_vars;
	cvar_vars = cvar;

	// Add to hash table
	hash = Com_HashKey(name, CVARS_HASH_SIZE);

	cvar->nextHash = cvar_varsHashTable[hash];
	cvar_varsHashTable[hash] = cvar;

	return cvar;
}

/*
 =================
 Cvar_Set

 Will create the variable if it doesn't exist
 =================
*/
static cvar_t *Cvar_Set (const char *name, const char *value, int flags, qboolean force){

	cvar_t	*cvar;

	Com_DPrintf("Cvar_Set: %s \"%s\" %i %s\n", name, value, flags, (force) ? "true" : "false");

	cvar = Cvar_FindVariable(name);
	if (!cvar)	// Create it
		return Cvar_Get(name, value, flags, NULL);

	cvar->flags |= flags;

	// Set modified flags
	cvar_modifiedFlags |= cvar->flags;

	if (cvar->flags & CVAR_ARCHIVE)
		com_configModified = true;		// Save at next opportunity

	// Check for invalid value
	if (cvar->flags & (CVAR_USERINFO | CVAR_SERVERINFO)){
		if (strchr(value, '\\') || strchr(value, '\"') || strchr(value, ';')){
			Com_Printf("Invalid info variable value '%s'\n", value);
			return cvar;
		}
	}

	if (!force){
		if (cvar->flags & CVAR_INIT){
			Com_Printf("'%s' is write protected\n", name);
			return cvar;
		}

		if (cvar->flags & CVAR_ROM){
			Com_Printf("'%s' is read only\n", name);
			return cvar;
		}

		if (cvar->flags & CVAR_CHEAT && !cvar_allowCheats){
			Com_Printf("'%s' is cheat protected\n", name);
			return cvar;
		}

		if (cvar->flags & CVAR_LATCH){
			if (cvar->latchedValue){
				if (!Q_stricmp(cvar->latchedValue, value))
					return cvar;

				FreeString(cvar->latchedValue);
				cvar->latchedValue = NULL;
			}

			if (!Q_stricmp(cvar->value, value))
				return cvar;

			cvar->latchedValue = CopyString(value);

			Com_Printf("'%s' will be changed upon restarting\n", name);
			return cvar;
		}
	}
	else {
		if (cvar->latchedValue){
			FreeString(cvar->latchedValue);
			cvar->latchedValue = NULL;
		}
	}

	if (!Q_stricmp(cvar->value, value))
		return cvar;		// Not changed

	FreeString(cvar->value);

	cvar->value = CopyString(value);
	cvar->floatValue = atof(cvar->value);
	cvar->integerValue = atoi(cvar->value);
	cvar->modified = true;

	// Stupid hack to workaround our changes to cvar_t
	if (cvar->flags & CVAR_GAMEVAR)
		Cvar_GameUpdateOrCreate(cvar);

	return cvar;
}

/*
 =================
 Cvar_GetString
 =================
*/
char *Cvar_GetString (const char *name){

	cvar_t	*cvar;
	
	cvar = Cvar_FindVariable(name);
	if (!cvar)
		return "";

	return cvar->value;
}

/*
 =================
 Cvar_GetFloat
 =================
*/
float Cvar_GetFloat (const char *name){

	cvar_t	*cvar;
	
	cvar = Cvar_FindVariable(name);
	if (!cvar)
		return 0.0;

	return cvar->floatValue;
}

/*
 =================
 Cvar_GetInteger
 =================
*/
int Cvar_GetInteger (const char *name){

	cvar_t	*cvar;

	cvar = Cvar_FindVariable(name);
	if (!cvar)
		return 0;

	return cvar->integerValue;
}

/*
 =================
 Cvar_SetString
 =================
*/
cvar_t *Cvar_SetString (const char *name, const char *value){

	return Cvar_Set(name, value, 0, false);
}

/*
 =================
 Cvar_SetFloat
 =================
*/
cvar_t *Cvar_SetFloat (const char *name, float value){

	return Cvar_Set(name, va("%f", value), 0, false);
}

/*
 =================
 Cvar_SetInteger
 =================
*/
cvar_t *Cvar_SetInteger (const char *name, int value){

	return Cvar_Set(name, va("%i", value), 0, false);
}

/*
 =================
 Cvar_ForceSet
 =================
*/
cvar_t *Cvar_ForceSet (const char *name, const char *value){

	return Cvar_Set(name, value, 0, true);
}

/*
 =================
 Cvar_FixCheatVariables
 =================
*/
void Cvar_FixCheatVariables (qboolean allowCheats){

	cvar_t	*cvar;

	if (cvar_allowCheats == allowCheats)
		return;
	cvar_allowCheats = allowCheats;

	if (cvar_allowCheats)
		return;

	for (cvar = cvar_vars; cvar; cvar = cvar->next){
		if (!(cvar->flags & CVAR_CHEAT))
			continue;

		if (!Q_stricmp(cvar->value, cvar->resetValue))
			continue;

		Cvar_Set(cvar->name, cvar->resetValue, 0, true);
	}
}

/*
 =================
 Cvar_Command

 Handles variable inspection and changing from the console
 =================
*/
qboolean Cvar_Command (void){

	cvar_t *cvar;

	// Check variables
	cvar = Cvar_FindVariable(Cmd_Argv(0));
	if (!cvar)
		return false;
		
	// Perform a variable print or set
	if (Cmd_Argc() == 1){
		Com_Printf("\"%s\" is: \"%s" S_COLOR_WHITE "\" default: \"%s" S_COLOR_WHITE "\"", cvar->name, cvar->value, cvar->resetValue);
		if (cvar->latchedValue)
			Com_Printf(" latched: \"%s" S_COLOR_WHITE "\"", cvar->latchedValue);
		Com_Printf("\n");

		if (cvar->description)
			Com_Printf("%s\n", cvar->description);

		return true;
	}

	Cvar_Set(cvar->name, Cmd_Argv(1), 0, false);

	return true;
}

/*
 =================
 Cvar_BitInfo
 =================
*/
static char *Cvar_BitInfo (int bit){

	static char	info[MAX_INFO_STRING];
	char		value[MAX_INFO_VALUE];
	cvar_t		*cvar;

	info[0] = 0;

	for (cvar = cvar_vars; cvar; cvar = cvar->next){
		if (!(cvar->flags & bit))
			continue;

		if (Q_PrintStrlen(cvar->value) != strlen(cvar->value)){
			Q_snprintfz(value, sizeof(value), "%s%s", cvar->value, S_COLOR_WHITE);
			Info_SetValueForKey(info, cvar->name, value);
		}
		else
			Info_SetValueForKey(info, cvar->name, cvar->value);
	}

	return info;
}

/*
 =================
 Cvar_UserInfo

 Returns an info string containing all the CVAR_USERINFO variables
 =================
*/
char *Cvar_UserInfo (void){

	return Cvar_BitInfo(CVAR_USERINFO);
}

/*
 =================
 Cvar_ServerInfo

 Returns an info string containing all the CVAR_SERVERINFO variables
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
static void Cvar_Get_f (void){

	cvar_t *cvar;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: get <variable>\n");
		return;
	}

	cvar = Cvar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Com_Printf("\"%s\" is: \"%s" S_COLOR_WHITE "\" default: \"%s" S_COLOR_WHITE "\"", cvar->name, cvar->value, cvar->resetValue);
	if (cvar->latchedValue)
		Com_Printf(" latched: \"%s" S_COLOR_WHITE "\"", cvar->latchedValue);
	Com_Printf("\n");
}

/*
 =================
 Cvar_Set_f

 Allows setting and defining of arbitrary variables from console
 =================
*/
static void Cvar_Set_f (void){

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: set <variable> <value>\n");
		return;
	}

	Cvar_Set(Cmd_Argv(1), Cmd_Argv(2), 0, false);
}

/*
 =================
 Cvar_Seta_f

 Allows setting and defining of arbitrary variables from console
 =================
*/
static void Cvar_Seta_f (void){

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: seta <variable> <value>\n");
		return;
	}

	Cvar_Set(Cmd_Argv(1), Cmd_Argv(2), CVAR_ARCHIVE, false);
}

/*
 =================
 Cvar_Setu_f

 Allows setting and defining of arbitrary variables from console
 =================
*/
static void Cvar_Setu_f (void){

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: setu <variable> <value>\n");
		return;
	}

	Cvar_Set(Cmd_Argv(1), Cmd_Argv(2), CVAR_USERINFO, false);
}

/*
 =================
 Cvar_Sets_f

 Allows setting and defining of arbitrary variables from console
 =================
*/
static void Cvar_Sets_f (void){

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: sets <variable> <value>\n");
		return;
	}

	Cvar_Set(Cmd_Argv(1), Cmd_Argv(2), CVAR_SERVERINFO, false);
}

/*
 =================
 Cvar_Add_f
 =================
*/
static void Cvar_Add_f (void){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: add <variable> <value>\n");
		return;
	}

	cvar = Cvar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set(cvar->name, va("%f", cvar->floatValue + atof(Cmd_Argv(2))), 0, false);
}

/*
 =================
 Cvar_Sub_f
 =================
*/
static void Cvar_Sub_f (void){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: sub <variable> <value>\n");
		return;
	}

	cvar = Cvar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set(cvar->name, va("%f", cvar->floatValue - atof(Cmd_Argv(2))), 0, false);
}

/*
 =================
 Cvar_Inc_f
 =================
*/
static void Cvar_Inc_f (void){

	cvar_t	*cvar;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: inc <variable>\n");
		return;
	}

	cvar = Cvar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set(cvar->name, va("%i", cvar->integerValue + 1), 0, false);
}

/*
 =================
 Cvar_Dec_f
 =================
*/
static void Cvar_Dec_f (void){

	cvar_t	*cvar;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: dec <variable>\n");
		return;
	}

	cvar = Cvar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set(cvar->name, va("%i", cvar->integerValue - 1), 0, false);
}

/*
 =================
 Cvar_Toggle_f
 =================
*/	
static void Cvar_Toggle_f (void){

	cvar_t	*cvar;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: toggle <variable>\n");
		return;
	}

	cvar = Cvar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set(cvar->name, va("%i", !cvar->integerValue), 0, false);
}

/*
 =================
 Cvar_Reset_f
 =================
*/
static void Cvar_Reset_f (void){

	cvar_t	*cvar;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: reset <variable>\n");
		return;
	}

	cvar = Cvar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set(cvar->name, cvar->resetValue, 0, false);
}

/*
 =================
 Cvar_Restart_f
 =================
*/
static void Cvar_Restart_f (void){

	cvar_t	*cvar;

	for (cvar = cvar_vars; cvar; cvar = cvar->next){
		if (cvar->flags & (CVAR_INIT | CVAR_ROM))
			continue;

		Cvar_Set(cvar->name, cvar->resetValue, 0, false);
	}
}

/*
 =================
 Cvar_ListCvars_f
 =================
*/
static void Cvar_ListCvars_f (void){

	cvar_t		*cvar, *list[MAX_LIST_CVARS];
	int			i, found = 0, total = 0;
	qboolean	help = false, flags = false;
	char		*filter = NULL;

	if (Cmd_Argc() > 3){
		Com_Printf("Usage: listCvars [\"-help\" | \"-flags\"] [filter]\n");
		return;
	}

	if (Cmd_Argc() == 2){
		if (!Q_stricmp("-help", Cmd_Argv(1)))
			help = true;
		else if (!Q_stricmp("-flags", Cmd_Argv(1)))
			flags = true;
		else
			filter = Cmd_Argv(1);
	}
	else if (Cmd_Argc() == 3){
		if (!Q_stricmp("-help", Cmd_Argv(1)))
			help = true;
		else if (!Q_stricmp("-flags", Cmd_Argv(1)))
			flags = true;
		else {
			Com_Printf("Usage: listCvars [\"-help\" | \"-flags\"] [filter]\n");
			return;
		}

		filter = Cmd_Argv(2);
	}

	for (cvar = cvar_vars; cvar; cvar = cvar->next, total++){
		if (filter){
			if (!Q_MatchFilter(cvar->name, filter, false))
				continue;
		}

		list[found++] = cvar;
	}

	if (!found){
		Com_Printf("0 cvars found (%i total cvars)\n", total);
		return;
	}

	qsort(list, found, sizeof(cvar_t *), Cvar_SortListCvars);

	for (i = 0; i < found; i++){
		cvar = list[i];

		if (help){
			if (cvar->description)
				Com_Printf("   %-32s %s\n", cvar->name, cvar->description);
			else
				Com_Printf("   %s\n", cvar->name);

			continue;
		}

		if (flags){
			Com_Printf("   %-32s", cvar->name);

			if (cvar->flags & CVAR_USERINFO)
				Com_Printf(" UI");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_SERVERINFO)
				Com_Printf(" SI");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_INIT)
				Com_Printf(" IN");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_ROM)
				Com_Printf(" RO");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_ARCHIVE)
				Com_Printf(" AR");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_LATCH)
				Com_Printf(" LA");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_CHEAT)
				Com_Printf(" CH");
			else
				Com_Printf("   ");

			Com_Printf("\n");

			continue;
		}

		Com_Printf("   %-32s \"%s\"\n", cvar->name, cvar->value);
	}

	Com_Printf("%i cvars found (%i total cvars)\n", found, total);
}

/*
 =================
 Cvar_Init
 =================
*/
void Cvar_Init (void){

	Cmd_AddCommand("get", Cvar_Get_f, "Get a variable");
	Cmd_AddCommand("set", Cvar_Set_f, "Set a variable");
	Cmd_AddCommand("seta", Cvar_Seta_f, "Set a variable flagged as archive");
	Cmd_AddCommand("setu", Cvar_Setu_f, "Set a variable flagged as user info");
	Cmd_AddCommand("sets", Cvar_Sets_f, "Set a variable flagged as server info");
	Cmd_AddCommand("add", Cvar_Add_f, "Add a value to a variable");
	Cmd_AddCommand("sub", Cvar_Sub_f, "Subtract a value from a variable");
	Cmd_AddCommand("inc", Cvar_Inc_f, "Increment a variable value");
	Cmd_AddCommand("dec", Cvar_Dec_f, "Decrement a variable value");
	Cmd_AddCommand("toggle", Cvar_Toggle_f, "Toggle a variable");
	Cmd_AddCommand("reset", Cvar_Reset_f, "Reset a variable");
	Cmd_AddCommand("cvar_restart", Cvar_Restart_f, "Restart the cvar system");
	Cmd_AddCommand("listCvars", Cvar_ListCvars_f, "List variables");
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
	Cmd_RemoveCommand("listCvars");
}


// =====================================================================

gamecvar_t	*game_vars;


/*
 =================
 Cvar_GameFindVar
 =================
*/
gamecvar_t *Cvar_GameFindVar (char *name){

	gamecvar_t	*cvar;

	for (cvar = game_vars; cvar; cvar = cvar->next){
		if (!Q_stricmp(cvar->name, name))
			return cvar;
	}

	return NULL;
}

/*
 =================
 Cvar_GameUpdateOrCreate
 =================
*/
gamecvar_t *Cvar_GameUpdateOrCreate (cvar_t *cvar){

	gamecvar_t	*gameVar;

	gameVar = Cvar_GameFindVar(cvar->name);
	if (gameVar){
		if (gameVar->string)
			FreeString(gameVar->string);
		if (gameVar->latched_string)
			FreeString(gameVar->latched_string);

		gameVar->name = CopyString(cvar->name);
		gameVar->string = CopyString(cvar->value);
		gameVar->latched_string = (cvar->latchedValue) ? CopyString(cvar->latchedValue) : NULL;
		gameVar->flags = cvar->flags;
		gameVar->modified = cvar->modified;
		gameVar->value = cvar->floatValue;

		return gameVar;
	}

	gameVar = Z_MallocSmall(sizeof(gamecvar_t));
	gameVar->name = CopyString(cvar->name);
	gameVar->string = CopyString(cvar->value);
	gameVar->latched_string = (cvar->latchedValue) ? CopyString(cvar->latchedValue) : NULL;
	gameVar->flags = cvar->flags;
	gameVar->modified = cvar->modified;
	gameVar->value = cvar->floatValue;

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

	cvar_t	*cvar;

	// HACK: the game library may want to know the game dir
	if (!Q_stricmp(name, "game"))
		name = "fs_game";

	cvar = Cvar_Get(name, value, flags | CVAR_GAMEVAR, NULL);
	if (!cvar)
		return NULL;

	return Cvar_GameUpdateOrCreate(cvar);
}

/*
 =================
 Cvar_GameSet
 =================
*/
gamecvar_t *Cvar_GameSet (char *name, char *value){

	cvar_t	*cvar;

	cvar = Cvar_Set(name, value, CVAR_GAMEVAR, true);
	if (!cvar)
		return NULL;

	return Cvar_GameUpdateOrCreate(cvar);
}

/*
 =================
 Cvar_GameForceSet
 =================
*/
gamecvar_t *Cvar_GameForceSet (char *name, char *value){

	cvar_t	*cvar;

	cvar = Cvar_Set(name, value, CVAR_GAMEVAR, true);
	if (!cvar)
		return NULL;

	return Cvar_GameUpdateOrCreate(cvar);
}
