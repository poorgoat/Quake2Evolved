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


#define PROGRAMS_HASHSIZE	128

static program_t	*r_programsHash[PROGRAMS_HASHSIZE];
static program_t	*r_programs[MAX_PROGRAMS];
static int			r_numPrograms;

program_t			*r_defaultVertexProgram;
program_t			*r_defaultFragmentProgram;


/*
 =================
 R_ProgramList_f
 =================
*/
void R_ProgramList_f (void){

	program_t	*program;
	int			i;

	Com_Printf("\n");
	Com_Printf("-----------------------------------\n");

	for (i = 0; i < r_numPrograms; i++){
		program = r_programs[i];

		Com_Printf("%3i: ", i);

		switch (program->uploadTarget){
		case GL_VERTEX_PROGRAM_ARB:
			Com_Printf("VP ");
			break;
		case GL_FRAGMENT_PROGRAM_ARB:
			Com_Printf("FP ");
			break;
		default:
			Com_Printf("?? ");
			break;
		}

		Com_Printf("%s\n", program->name);
	}

	Com_Printf("-----------------------------------\n");
	Com_Printf("%i total programs\n", r_numPrograms);
	Com_Printf("\n");
}

/*
 =================
 R_UploadProgram
 =================
*/
static qboolean R_UploadProgram (const char *name, unsigned target, const char *string, int length, unsigned *progNum){

	const char	*errString;
	int			errPosition;

	qglGenProgramsARB(1, progNum);
	qglBindProgramARB(target, *progNum);
	qglProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, length, string);

	errString = qglGetString(GL_PROGRAM_ERROR_STRING_ARB);
	qglGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPosition);

	if (errPosition == -1)
		return true;

	switch (target){
	case GL_VERTEX_PROGRAM_ARB:
		Com_Printf(S_COLOR_RED "Error in vertex program '%s' at char %i: %s\n", name, errPosition, errString);
		break;
	case GL_FRAGMENT_PROGRAM_ARB:
		Com_Printf(S_COLOR_RED "Error in fragment program '%s' at char %i: %s\n", name, errPosition, errString);
		break;
	}

	qglDeleteProgramsARB(1, progNum);

	return false;
}

/*
 =================
 R_LoadProgram
 =================
*/
program_t *R_LoadProgram (const char *name, unsigned target, const char *string, int length){

	program_t	*program;
	unsigned	progNum;
	unsigned	hashKey;

	if (!R_UploadProgram(name, target, string, length, &progNum)){
		switch (target){
		case GL_VERTEX_PROGRAM_ARB:
			return r_defaultVertexProgram;
		case GL_FRAGMENT_PROGRAM_ARB:
			return r_defaultFragmentProgram;
		}
	}

	if (r_numPrograms == MAX_PROGRAMS)
		Com_Error(ERR_DROP, "R_LoadProgram: MAX_PROGRAMS hit");

	r_programs[r_numPrograms++] = program = Hunk_Alloc(sizeof(program_t));

	// Fill it in
	Q_strncpyz(program->name, name, sizeof(program->name));
	program->uploadTarget = target;
	program->progNum = progNum;

	// Add to hash table
	hashKey = Com_HashKey(name, PROGRAMS_HASHSIZE);

	program->nextHash = r_programsHash[hashKey];
	r_programsHash[hashKey] = program;

	return program;
}

/*
 =================
 R_FindProgram
 =================
*/
program_t *R_FindProgram (const char *name, unsigned target){

	program_t	*program;
	char		*string;
	int			length;
	unsigned	hashKey;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindProgram: NULL name");

	if (strlen(name) >= MAX_QPATH)
		Com_Error(ERR_DROP, "R_FindProgram: program name exceeds MAX_QPATH");

	// See if already loaded
	hashKey = Com_HashKey(name, PROGRAMS_HASHSIZE);

	for (program = r_programsHash[hashKey]; program; program = program->nextHash){
		if (!Q_stricmp(program->name, name)){
			if (program->uploadTarget == target)
				return program;
		}
	}

	// Load it from disk
	length = FS_LoadFile(name, (void **)&string);
	if (string){
		program = R_LoadProgram(name, target, string, length);
		FS_FreeFile(string);
		return program;
	}

	// Not found
	return NULL;
}

/*
 =================
 R_InitPrograms
 =================
*/
void R_InitPrograms (void){

	const char	*stringVP = 
		"!!ARBvp1.0"
		"PARAM mvpMatrix[4] = { state.matrix.mvp };"
		"PARAM texMatrix[4] = { state.matrix.texture[0] };"
		"DP4 result.position.x, mvpMatrix[0], vertex.position;"
		"DP4 result.position.y, mvpMatrix[1], vertex.position;"
		"DP4 result.position.z, mvpMatrix[2], vertex.position;"
		"DP4 result.position.w, mvpMatrix[3], vertex.position;"
		"MOV result.color, vertex.color;"
		"DP4 result.texcoord[0].x, texMatrix[0], vertex.texcoord[0];"
		"DP4 result.texcoord[0].y, texMatrix[1], vertex.texcoord[0];"
		"DP4 result.texcoord[0].z, texMatrix[2], vertex.texcoord[0];"
		"DP4 result.texcoord[0].w, texMatrix[3], vertex.texcoord[0];"
		"END";

	const char	*stringFP = 
		"!!ARBfp1.0"
		"TEMP tmp;"
		"TEX tmp, fragment.texcoord[0], texture[0], 2D;"
		"MUL result.color, tmp, fragment.color;"
		"END";

	if (glConfig.vertexProgram)
		r_defaultVertexProgram = R_LoadProgram("<defaultVP>", GL_VERTEX_PROGRAM_ARB, stringVP, strlen(stringVP));

	if (glConfig.fragmentProgram)
		r_defaultFragmentProgram = R_LoadProgram("<defaultFP>", GL_FRAGMENT_PROGRAM_ARB, stringFP, strlen(stringFP));
}

/*
 =================
 R_ShutdownPrograms
 =================
*/
void R_ShutdownPrograms (void){

	program_t	*program;
	int			i;

	for (i = 0; i < r_numPrograms; i++){
		program = r_programs[i];

		qglDeleteProgramsARB(1, &program->progNum);
	}

	memset(r_programsHash, 0, sizeof(r_programsHash));
	memset(r_programs, 0, sizeof(r_programs));

	r_numPrograms = 0;
}
