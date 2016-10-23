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


#include "r_local.h"


static program_t	*r_programsHashTable[PROGRAMS_HASH_SIZE];
static program_t	*r_programs[MAX_PROGRAMS];
static int			r_numPrograms;


/*
 =================
 R_UploadProgram
 =================
*/
static void R_UploadProgram (unsigned target, const char *string, int length, program_t *program){

	int		errPos;

	program->uploadSuccessful = false;
	program->uploadInstructions = 0;
	program->uploadNative = 0;
	program->uploadTarget = target;

	// Bind the program
	qglGenProgramsARB(1, &program->progNum);

	GL_BindProgram(program);

	// Upload the program string
	switch (target){
	case GL_VERTEX_PROGRAM_ARB:
		qglProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, length, string);

		// Check for errors and print any to the console
		qglGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		if (errPos != -1){
			if (errPos == length){
				Com_Printf(S_COLOR_RED "Error at end of vertex program '%s'\n", program->name);

				Com_DPrintf("GL_PROGRAM_ERROR_STRING: %s\n", qglGetString(GL_PROGRAM_ERROR_STRING_ARB));
			}
			else {
				Com_Printf(S_COLOR_RED "Error in vertex program '%s'\n", program->name);

				Com_DPrintf("GL_PROGRAM_ERROR_POSITION: %i\n", errPos);
				Com_DPrintf("GL_PROGRAM_ERROR_STRING: %s\n", qglGetString(GL_PROGRAM_ERROR_STRING_ARB));
			}

			return;
		}

		break;
	case GL_FRAGMENT_PROGRAM_ARB:
		qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, length, string);

		// Check for errors and print any to the console
		qglGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		if (errPos != -1){
			if (errPos == length){
				Com_Printf(S_COLOR_RED "Error at end of fragment program '%s'\n", program->name);

				Com_DPrintf("GL_PROGRAM_ERROR_STRING: %s\n", qglGetString(GL_PROGRAM_ERROR_STRING_ARB));
			}
			else {
				Com_Printf(S_COLOR_RED "Error in fragment program '%s'\n", program->name);

				Com_DPrintf("GL_PROGRAM_ERROR_POSITION: %i\n", errPos);
				Com_DPrintf("GL_PROGRAM_ERROR_STRING: %s\n", qglGetString(GL_PROGRAM_ERROR_STRING_ARB));
			}

			return;
		}

		break;
	default:
		Com_Error(ERR_DROP, "R_UploadProgram: bad program target (%u)", target);
	}

	program->uploadSuccessful = true;

	qglGetProgramivARB(target, GL_PROGRAM_INSTRUCTIONS_ARB, &program->uploadInstructions);
	qglGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &program->uploadNative);
}

/*
 =================
 R_LoadProgram
 =================
*/
static void R_LoadProgram (const char *name, unsigned target, const char *string, int length){

	program_t	*program;
	unsigned	hash;

	if (r_numPrograms == MAX_PROGRAMS)
		Com_Error(ERR_DROP, "R_LoadProgram: MAX_PROGRAMS hit");

	r_programs[r_numPrograms++] = program = Z_Malloc(sizeof(program_t));

	// Fill it in
	Q_strncpyz(program->name, name, sizeof(program->name));

	R_UploadProgram(target, string, length, program);

	// Add to hash table
	hash = Com_HashKey(program->name, PROGRAMS_HASH_SIZE);

	program->nextHash = r_programsHashTable[hash];
	r_programsHashTable[hash] = program;
}

/*
 =================
 R_FindProgram
 =================
*/
program_t *R_FindProgram (const char *name, unsigned target){

	program_t	*program;
	unsigned	hash;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindProgram: NULL program name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_FindProgram: program name exceeds MAX_OSPATH");

	// See if it exists
	hash = Com_HashKey(name, PROGRAMS_HASH_SIZE);

	for (program = r_programsHashTable[hash]; program; program = program->nextHash){
		if (!program->uploadSuccessful || program->uploadTarget != target)
			continue;

		if (!Q_stricmp(program->name, name))
			return program;
	}

	// Not found
	return NULL;
}

/*
 =================
 R_ParseProgramFile
 =================
*/
static void R_ParseProgramFile (script_t *script, const char *name, qboolean loadVP, qboolean loadFP){

	token_t	token;
	char	*string;

	// Try to load a vertex program
	if (loadVP){
		PS_ResetScript(script);

		while (1){
			// Parse the header
			if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES | PSF_PARSE_GENERIC, &token)){
				Com_Printf(S_COLOR_RED "Couldn't find '!!ARBvp1.0' header\n");
				return;
			}

			// Skip program comments
			if (token.string[0] == '#'){
				PS_SkipRestOfLine(script);
				continue;
			}

			// Parse the program string
			if (!Q_stricmp(token.string, "!!ARBvp1.0")){
				string = script->text - 10;

				while (1){
					if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES | PSF_PARSE_GENERIC, &token)){
						Com_Printf(S_COLOR_RED "Couldn't find 'END' token\n");
						return;
					}

					// Skip program comments
					if (token.string[0] == '#'){
						PS_SkipRestOfLine(script);
						continue;
					}

					// Load the program
					if (!Q_stricmp(token.string, "END")){
						if (!glConfig.vertexProgram){
							Com_Printf(S_COLOR_YELLOW "GL_VERTEX_PROGRAM not available\n");
							break;
						}

						R_LoadProgram(name, GL_VERTEX_PROGRAM_ARB, string, script->text - string);

						break;
					}
				}

				break;
			}
		}
	}

	// Try to load a fragment program
	if (loadFP){
		PS_ResetScript(script);

		while (1){
			// Parse the header
			if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES | PSF_PARSE_GENERIC, &token)){
				Com_Printf(S_COLOR_RED "Couldn't find '!!ARBfp1.0' header\n");
				return;
			}

			// Skip program comments
			if (token.string[0] == '#'){
				PS_SkipRestOfLine(script);
				continue;
			}

			// Parse the program string
			if (!Q_stricmp(token.string, "!!ARBfp1.0")){
				string = script->text - 10;

				while (1){
					if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES | PSF_PARSE_GENERIC, &token)){
						Com_Printf(S_COLOR_RED "Couldn't find 'END' token\n");
						return;
					}

					// Skip program comments
					if (token.string[0] == '#'){
						PS_SkipRestOfLine(script);
						continue;
					}

					// Load the program
					if (!Q_stricmp(token.string, "END")){
						if (!glConfig.fragmentProgram){
							Com_Printf(S_COLOR_YELLOW "GL_FRAGMENT_PROGRAM not available\n");
							break;
						}

						R_LoadProgram(name, GL_FRAGMENT_PROGRAM_ARB, string, script->text - string);

						break;
					}
				}

				break;
			}
		}
	}
}

/*
 =================
 R_ListPrograms_f
 =================
*/
void R_ListPrograms_f (void){

	program_t	*program;
	int			i;

	Com_Printf("\n");
	Com_Printf("      -inst native type -name--------\n");

	for (i = 0; i < r_numPrograms; i++){
		program = r_programs[i];

		Com_Printf("%4i: ", i);

		Com_Printf("%5i ", program->uploadInstructions);

		if (program->uploadNative)
			Com_Printf("  yes  ");
		else
			Com_Printf("  no   ");

		switch (program->uploadTarget){
		case GL_VERTEX_PROGRAM_ARB:
			Com_Printf("vert ");
			break;
		case GL_FRAGMENT_PROGRAM_ARB:
			Com_Printf("frag ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		Com_Printf("%s%s\n", program->name, (!program->uploadSuccessful) ? " (INVALID)" : "");
	}

	Com_Printf("-------------------------------------\n");
	Com_Printf("%i total programs\n", r_numPrograms);
	Com_Printf("\n");
}

/*
 =================
 R_InitPrograms
 =================
*/
void R_InitPrograms (void){

	script_t	*script;
	char		**fileList;
	int			numFiles;
	char		name[MAX_OSPATH];
	int			i;

	Com_Printf("Initializing Programs\n");

	// Load .vfp files
	fileList = FS_ListFiles("programs", ".vfp", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Load the script file
		Q_snprintfz(name, sizeof(name), "programs/%s", fileList[i]);
		Com_Printf("...loading '%s'", name);

		script = PS_LoadScriptFile(name);
		if (!script){
			Com_Printf(": failed\n");
			continue;
		}
		Com_Printf("\n");

		// Parse it
		R_ParseProgramFile(script, fileList[i], true, true);

		PS_FreeScript(script);
	}

	FS_FreeFileList(fileList);

	// Load .vp files
	fileList = FS_ListFiles("programs", ".vp", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Load the script file
		Q_snprintfz(name, sizeof(name), "programs/%s", fileList[i]);
		Com_Printf("...loading '%s'", name);

		script = PS_LoadScriptFile(name);
		if (!script){
			Com_Printf(": failed\n");
			continue;
		}
		Com_Printf("\n");

		// Parse it
		R_ParseProgramFile(script, fileList[i], true, false);

		PS_FreeScript(script);
	}

	FS_FreeFileList(fileList);

	// Load .fp files
	fileList = FS_ListFiles("programs", ".fp", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Load the script file
		Q_snprintfz(name, sizeof(name), "programs/%s", fileList[i]);
		Com_Printf("...loading '%s'", name);

		script = PS_LoadScriptFile(name);
		if (!script){
			Com_Printf(": failed\n");
			continue;
		}
		Com_Printf("\n");

		// Parse it
		R_ParseProgramFile(script, fileList[i], false, true);

		PS_FreeScript(script);
	}

	FS_FreeFileList(fileList);
}

/*
 =================
 R_ShutdownPrograms
 =================
*/
void R_ShutdownPrograms (void){

	program_t	*program;
	int			i;

	if (glConfig.vertexProgram)
		qglBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
	if (glConfig.fragmentProgram)
		qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);

	for (i = 0; i < r_numPrograms; i++){
		program = r_programs[i];

		qglDeleteProgramsARB(1, &program->progNum);

		Z_Free(program);
	}

	memset(r_programsHashTable, 0, sizeof(r_programsHashTable));
	memset(r_programs, 0, sizeof(r_programs));

	r_numPrograms = 0;
}
