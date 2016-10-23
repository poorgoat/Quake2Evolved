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


// r_material.c -- material script parsing and loading


#include "r_local.h"


typedef struct mtrScript_s {
	char				name[MAX_OSPATH];
	materialType_t		type;
	surfaceParm_t		surfaceParm;

	char				source[MAX_OSPATH];
	int					line;

	char				*buffer;
	int					size;

	struct mtrScript_s	*nextHash;
} mtrScript_t;

static material_t		r_parseMaterial;
static stage_t			r_parseMaterialStages[MAX_STAGES];
static expOp_t			r_parseMaterialOps[MAX_EXPRESSION_OPS];
static float			r_parseMaterialExpressionRegisters[MAX_EXPRESSION_REGISTERS];

static table_t			*r_tablesHashTable[TABLES_HASH_SIZE];
static material_t		*r_materialsHashTable[MATERIALS_HASH_SIZE];
static mtrScript_t		*r_materialScriptsHashTable[MATERIALS_HASH_SIZE];

static table_t			*r_tables[MAX_TABLES];
static int				r_numTables;

static material_t		*r_materials[MAX_MATERIALS];
static int				r_numMaterials;


/*
 =======================================================================

 TABLE PARSING & LOADING

 =======================================================================
*/


/*
 =================
 R_LoadTable
 =================
*/
static void R_LoadTable (const char *name, qboolean clamp, qboolean snap, int size, float *values){

	table_t		*table;
	unsigned	hash;

	if (r_numTables == MAX_TABLES)
		Com_Error(ERR_DROP, "R_LoadTable: MAX_TABLES hit");

	r_tables[r_numTables++] = table = Z_Malloc(sizeof(table_t));

	// Fill it in
	Q_strncpyz(table->name, name, sizeof(table->name));
	table->index = r_numTables - 1;
	table->clamp = clamp;
	table->snap = snap;
	table->size = size;

	table->values = Hunk_Alloc(size * sizeof(float));
	memcpy(table->values, values, size * sizeof(float));

	// Add to hash table
	hash = Com_HashKey(table->name, TABLES_HASH_SIZE);

	table->nextHash = r_tablesHashTable[hash];
	r_tablesHashTable[hash] = table;
}

/*
 =================
 R_FindTable
 =================
*/
static table_t *R_FindTable (const char *name){

	table_t		*table;
	unsigned	hash;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindTable: NULL table name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_FindTable: table name exceeds MAX_OSPATH");

	// See if already loaded
	hash = Com_HashKey(name, TABLES_HASH_SIZE);

	for (table = r_tablesHashTable[hash]; table; table = table->nextHash){
		if (!Q_stricmp(table->name, name))
			return table;
	}

	// Not found
	return NULL;
}

/*
 =================
 R_ParseTable
 =================
*/
static qboolean R_ParseTable (script_t *script){

	token_t		token;
	char		name[MAX_OSPATH];
	qboolean	clamp = false, snap = false;
	int			size = 0;
	float		values[MAX_TABLE_SIZE];

	if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'table'\n");
		return false;
	}

	Q_strncpyz(name, token.string, sizeof(name));

	PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token);
	if (Q_stricmp(token.string, "{")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in table '%s'\n", token.string, name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for table '%s'\n", name);
			return false;
		}

		if (!Q_stricmp(token.string, "clamp"))
			clamp = true;
		else if (!Q_stricmp(token.string, "snap"))
			snap = true;
		else {
			if (Q_stricmp(token.string, "{")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in table '%s'\n", token.string, name);
				return false;
			}

			while (1){
				if (size != 0){
					PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token);
					if (!Q_stricmp(token.string, "}"))
						break;

					if (Q_stricmp(token.string, ",")){
						Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in table '%s'\n", token.string, name);
						return false;
					}
				}

				if (size == MAX_TABLE_SIZE){
					Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TABLE_SIZE hit in table '%s'\n", name);
					return false;
				}

				if (!PS_ReadFloat(script, PSF_ALLOW_NEWLINES, &values[size])){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for table '%s'\n", name);
					return false;
				}

				size++;
			}

			break;
		}
	}

	PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token);
	if (Q_stricmp(token.string, "}")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '}', found '%s' instead in table '%s'\n", token.string, name);
		return false;
	}

	R_LoadTable(name, clamp, snap, size, values);

	return true;
}

/*
 =================
 R_LookupTable
 =================
*/
static float R_LookupTable (int tableIndex, float index){

	table_t		*table;
	float		frac, value;
	unsigned	curIndex, oldIndex;

	if (tableIndex < 0 || tableIndex >= r_numTables)
		Com_Error(ERR_DROP, "R_LookupTable: out of range");

	table = r_tables[tableIndex];

	index *= table->size;
	frac = index - floor(index);

	curIndex = (unsigned)index + 1;
	oldIndex = (unsigned)index;

	if (table->clamp){
		curIndex = Clamp(curIndex, 0, table->size - 1);
		oldIndex = Clamp(oldIndex, 0, table->size - 1);
	}
	else {
		curIndex %= table->size;
		oldIndex %= table->size;
	}

	if (table->snap)
		value = table->values[oldIndex];
	else
		value = table->values[oldIndex] + (table->values[curIndex] - table->values[oldIndex]) * frac;

	return value;
}


/*
 =======================================================================

 MATERIAL EXPRESSION PARSING

 =======================================================================
*/

#define MAX_EXPRESSION_VALUES		64
#define MAX_EXPRESSION_OPERATORS	64

typedef struct expValue_s {
	int						expressionRegister;

	int						brackets;
	int						parentheses;

	struct expValue_s		*prev;
	struct expValue_s		*next;
} expValue_t;

typedef struct expOperator_s {
	opType_t				opType;
	int						priority;

	int						brackets;
	int						parentheses;

	struct expOperator_s	*prev;
	struct expOperator_s	*next;
} expOperator_t;

typedef struct {
	int						numValues;
	expValue_t				values[MAX_EXPRESSION_VALUES];
	expValue_t				*firstValue;
	expValue_t				*lastValue;

	int						numOperators;
	expOperator_t			operators[MAX_EXPRESSION_OPERATORS];
	expOperator_t			*firstOperator;
	expOperator_t			*lastOperator;

	int						brackets;
	int						parentheses[MAX_EXPRESSION_VALUES + MAX_EXPRESSION_OPERATORS];

	int						resultRegister;
} materialExpression_t;

static materialExpression_t	r_materialExpression;

static qboolean	R_ParseExpressionValue (script_t *script, material_t *material);
static qboolean	R_ParseExpressionOperator (script_t *script, material_t *material);


/*
 =================
 R_GetExpressionConstant
 =================
*/
static qboolean R_GetExpressionConstant (float value, material_t *material, int *expressionRegister){

	if (material->numRegisters == MAX_EXPRESSION_REGISTERS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_REGISTERS hit in material '%s'\n", material->name);
		return false;
	}

	material->expressionRegisters[material->numRegisters] = value;

	*expressionRegister = r_materialExpression.resultRegister = material->numRegisters++;

	return true;
}

/*
 =================
 R_GetExpressionTemporary
 =================
*/
static qboolean R_GetExpressionTemporary (material_t *material, int *expressionRegister){

	if (material->numRegisters == MAX_EXPRESSION_REGISTERS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_REGISTERS hit in material '%s'\n", material->name);
		return false;
	}

	material->expressionRegisters[material->numRegisters] = 0.0;

	*expressionRegister = r_materialExpression.resultRegister = material->numRegisters++;

	return true;
}

/*
 =================
 R_EmitExpressionOp
 =================
*/
static qboolean R_EmitExpressionOp (opType_t opType, int a, int b, int c, material_t *material){

	expOp_t	*op;

	if (material->numOps == MAX_EXPRESSION_OPS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_OPS hit in material '%s'\n", material->name);
		return false;
	}

	op = &material->ops[material->numOps++];

	op->opType = opType;
	op->a = a;
	op->b = b;
	op->c = c;

	return true;
}

/*
 =================
 R_AddExpressionValue
 =================
*/
static qboolean R_AddExpressionValue (int expressionRegister, material_t *material){

	expValue_t		*v;

	if (r_materialExpression.numValues == MAX_EXPRESSION_VALUES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_VALUES hit for expression in material '%s'\n", material->name);
		return false;
	}

	v = &r_materialExpression.values[r_materialExpression.numValues++];

	v->expressionRegister = expressionRegister;
	v->brackets = r_materialExpression.brackets;
	v->parentheses = r_materialExpression.parentheses[r_materialExpression.brackets];
	v->next = NULL;
	v->prev = r_materialExpression.lastValue;

	if (r_materialExpression.lastValue)
		r_materialExpression.lastValue->next = v;
	else
		r_materialExpression.firstValue = v;

	r_materialExpression.lastValue = v;

	return true;
}

/*
 =================
 R_AddExpressionOperator
 =================
*/
static qboolean R_AddExpressionOperator (opType_t opType, int priority, material_t *material){

	expOperator_t	*o;

	if (r_materialExpression.numOperators == MAX_EXPRESSION_OPERATORS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_OPERATORS hit for expression in material '%s'\n", material->name);
		return false;
	}

	o = &r_materialExpression.operators[r_materialExpression.numOperators++];

	o->opType = opType;
	o->priority = priority;
	o->brackets = r_materialExpression.brackets;
	o->parentheses = r_materialExpression.parentheses[r_materialExpression.brackets];
	o->next = NULL;
	o->prev = r_materialExpression.lastOperator;

	if (r_materialExpression.lastOperator)
		r_materialExpression.lastOperator->next = o;
	else
		r_materialExpression.firstOperator = o;

	r_materialExpression.lastOperator = o;

	return true;
}

/*
 =================
 R_ParseExpressionValue
 =================
*/
static qboolean R_ParseExpressionValue (script_t *script, material_t *material){

	token_t	token;
	table_t	*table;
	int		expressionRegister;

	// A newline separates commands
	if (!PS_ReadToken(script, 0, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: unexpected end of expression in material '%s'\n", material->name);
		return false;
	}

	// A comma separates arguments
	if (!Q_stricmp(token.string, ",")){
		Com_Printf(S_COLOR_YELLOW "WARNING: unexpected end of expression in material '%s'\n", material->name);
		return false;
	}

	// Add a new value
	if (token.type != TT_NUMBER && token.type != TT_NAME && token.type != TT_PUNCTUATION){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
		return false;
	}

	switch (token.type){
	case TT_NUMBER:
		// It's a constant
		if (!R_GetExpressionConstant(token.floatValue, material, &expressionRegister))
			return false;

		if (!R_AddExpressionValue(expressionRegister, material))
			return false;

		break;
	case TT_NAME:
		// Check for a table
		table = R_FindTable(token.string);
		if (table){
			// The next token should be an opening bracket
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, "[")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected '[', found '%s' instead for expression in material '%s'\n", token.string, material->name);
				return false;
			}

			r_materialExpression.brackets++;

			if (!R_AddExpressionValue(table->index, material))
				return false;

			if (!R_AddExpressionOperator(OP_TYPE_TABLE, 0, material))
				return false;

			// We still expect a value
			return R_ParseExpressionValue(script, material);
		}

		// Check for a variable
		if (!Q_stricmp(token.string, "glPrograms")){
			if ((glConfig.vertexProgram && glConfig.fragmentProgram) && r_shaderPrograms->integerValue){
				r_materialExpression.resultRegister = EXP_REGISTER_ONE;

				if (!R_AddExpressionValue(EXP_REGISTER_ONE, material))
					return false;
			}
			else {
				r_materialExpression.resultRegister = EXP_REGISTER_ZERO;

				if (!R_AddExpressionValue(EXP_REGISTER_ZERO, material))
					return false;
			}
		}
		else if (!Q_stricmp(token.string, "time")){
			r_materialExpression.resultRegister = EXP_REGISTER_TIME;

			if (!R_AddExpressionValue(EXP_REGISTER_TIME, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm0")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM0;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM0, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm1")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM1;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM1, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm2")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM2;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM2, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm3")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM3;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM3, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm4")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM4;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM4, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm5")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM5;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM5, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm6")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM6;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM6, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "parm7")){
			r_materialExpression.resultRegister = EXP_REGISTER_PARM7;

			if (!R_AddExpressionValue(EXP_REGISTER_PARM7, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global0")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL0;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL0, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global1")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL1;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL1, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global2")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL2;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL2, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global3")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL3;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL3, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global4")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL4;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL4, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global5")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL5;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL5, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global6")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL6;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL6, material))
				return false;
		}
		else if (!Q_stricmp(token.string, "global7")){
			r_materialExpression.resultRegister = EXP_REGISTER_GLOBAL7;

			if (!R_AddExpressionValue(EXP_REGISTER_GLOBAL7, material))
				return false;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
			return false;
		}

		break;
	case TT_PUNCTUATION:
		// Check for an opening parenthesis
		if (!Q_stricmp(token.string, "(")){
			r_materialExpression.parentheses[r_materialExpression.brackets]++;

			// We still expect a value
			return R_ParseExpressionValue(script, material);
		}

		// Check for a minus operator before a constant
		if (!Q_stricmp(token.string, "-")){
			if (!PS_ReadToken(script, 0, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: unexpected end of expression in material '%s'\n", material->name);
				return false;
			}

			if (token.type != TT_NUMBER){
				Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
				return false;
			}

			if (!R_GetExpressionConstant(-token.floatValue, material, &expressionRegister))
				return false;

			if (!R_AddExpressionValue(expressionRegister, material))
				return false;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
			return false;
		}

		break;
	}

	// We now expect an operator
	return R_ParseExpressionOperator(script, material);
}

/*
 =================
 R_ParseExpressionOperator
 =================
*/
static qboolean R_ParseExpressionOperator (script_t *script, material_t *material){

	token_t	token;

	// A newline separates commands
	if (!PS_ReadToken(script, 0, &token)){
		if (r_materialExpression.brackets){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ']' for expression in material '%s'\n", material->name);
			return false;
		}

		if (r_materialExpression.parentheses[r_materialExpression.brackets]){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ')' for expression in material '%s'\n", material->name);
			return false;
		}

		return true;
	}

	// A comma separates arguments
	if (!Q_stricmp(token.string, ",")){
		if (r_materialExpression.brackets){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ']' for expression in material '%s'\n", material->name);
			return false;
		}

		if (r_materialExpression.parentheses[r_materialExpression.brackets]){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ')' for expression in material '%s'\n", material->name);
			return false;
		}

		// Unread the token, because we'll expect a comma later
		PS_UnreadToken(script, &token);

		return true;
	}

	// Add a new operator
	if (token.type != TT_PUNCTUATION){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid operator '%s' for expression in material '%s'\n", token.string, material->name);
		return false;
	}

	// Check for a closing bracket
	if (!Q_stricmp(token.string, "]")){
		if (r_materialExpression.parentheses[r_materialExpression.brackets]){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ')' for expression in material '%s'\n", material->name);
			return false;
		}

		r_materialExpression.brackets--;
		if (r_materialExpression.brackets < 0){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching '[' for expression in material '%s'\n", material->name);
			return false;
		}

		// We still expect an operator
		return R_ParseExpressionOperator(script, material);
	}

	// Check for a closing parenthesis
	if (!Q_stricmp(token.string, ")")){
		r_materialExpression.parentheses[r_materialExpression.brackets]--;
		if (r_materialExpression.parentheses[r_materialExpression.brackets] < 0){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching '(' for expression in material '%s'\n", material->name);
			return false;
		}

		// We still expect an operator
		return R_ParseExpressionOperator(script, material);
	}

	// Check for an operator
	if (!Q_stricmp(token.string, "*")){
		if (!R_AddExpressionOperator(OP_TYPE_MULTIPLY, 7, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "/")){
		if (!R_AddExpressionOperator(OP_TYPE_DIVIDE, 7, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "%")){
		if (!R_AddExpressionOperator(OP_TYPE_MOD, 6, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "+")){
		if (!R_AddExpressionOperator(OP_TYPE_ADD, 5, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "-")){
		if (!R_AddExpressionOperator(OP_TYPE_SUBTRACT, 5, material))
			return false;
	}
	else if (!Q_stricmp(token.string, ">")){
		if (!R_AddExpressionOperator(OP_TYPE_GREATER, 4, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "<")){
		if (!R_AddExpressionOperator(OP_TYPE_LESS, 4, material))
			return false;
	}
	else if (!Q_stricmp(token.string, ">=")){
		if (!R_AddExpressionOperator(OP_TYPE_GEQUAL, 4, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "<=")){
		if (!R_AddExpressionOperator(OP_TYPE_LEQUAL, 4, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "==")){
		if (!R_AddExpressionOperator(OP_TYPE_EQUAL, 3, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "!=")){
		if (!R_AddExpressionOperator(OP_TYPE_NOTEQUAL, 3, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "&&")){
		if (!R_AddExpressionOperator(OP_TYPE_AND, 2, material))
			return false;
	}
	else if (!Q_stricmp(token.string, "||")){
		if (!R_AddExpressionOperator(OP_TYPE_OR, 1, material))
			return false;
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid operator '%s' for expression in material '%s'\n", token.string, material->name);
		return false;
	}

	// We now expect a value
	return R_ParseExpressionValue(script, material);
}

/*
 =================
 R_ParseExpression
 =================
*/
static qboolean R_ParseExpression (script_t *script, material_t *material, int *expressionRegister){

	expValue_t		*v;
	expOperator_t	*o;
	int				a, b, c;

	// Clear the expression data
	memset(&r_materialExpression, 0, sizeof(materialExpression_t));

	// Parse the expression, starting with a value
	if (!R_ParseExpressionValue(script, material))
		return false;

	// Emit the expression ops, if any
	while (r_materialExpression.firstOperator){
		v = r_materialExpression.firstValue;

		for (o = r_materialExpression.firstOperator; o->next; o = o->next){
			// If the current operator is nested deeper in brackets than
			// the next operator
			if (o->brackets > o->next->brackets)
				break;

			// If the current and next operators are nested equally deep
			// in brackets
			if (o->brackets == o->next->brackets){
				// If the current operator is nested deeper in
				// parentheses than the next operator
				if (o->parentheses > o->next->parentheses)
					break;

				// If the current and next operators are nested equally
				// deep in parentheses
				if (o->parentheses == o->next->parentheses){
					// If the priority of the current operator is equal
					// or higher than the priority of the next operator
					if (o->priority >= o->next->priority)
						break;
				}
			}

			v = v->next;
		}

		// Get the source registers
		a = v->expressionRegister;
		b = v->next->expressionRegister;

		// Get the temporary register
		if (!R_GetExpressionTemporary(material, &c))
			return false;

		// Emit the expression op
		if (!R_EmitExpressionOp(o->opType, a, b, c, material))
			return false;

		// The temporary register for the current operation will be used
		// as a source for the next operation
		v->expressionRegister = c;

		// Remove the second value
		v = v->next;

		if (v->prev)
			v->prev->next = v->next;
		else
			r_materialExpression.firstValue = v->next;

		if (v->next)
			v->next->prev = v->prev;
		else
			r_materialExpression.lastValue = v->prev;

		// Remove the operator
		if (o->prev)
			o->prev->next = o->next;
		else
			r_materialExpression.firstOperator = o->next;

		if (o->next)
			o->next->prev = o->prev;
		else
			r_materialExpression.lastOperator = o->prev;
	}

	// The last temporary register will contain the result after
	// evaluation
	*expressionRegister = r_materialExpression.resultRegister;

	return true;
}


/*
 =======================================================================

 MATERIAL PARSING

 =======================================================================
*/


/*
 =================
 R_ParseGeneralSurfaceParm
 =================
*/
static qboolean R_ParseGeneralSurfaceParm (script_t *script, material_t *material){

	token_t	token;

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'surfaceParm' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, 0, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'surfaceParm' in material '%s'\n", material->name);
		return false;
	}

	if (!Q_stricmp(token.string, "lighting"))
		material->surfaceParm |= SURFACEPARM_LIGHTING;
	else if (!Q_stricmp(token.string, "sky"))
		material->surfaceParm |= SURFACEPARM_SKY;
	else if (!Q_stricmp(token.string, "warp"))
		material->surfaceParm |= SURFACEPARM_WARP;
	else if (!Q_stricmp(token.string, "trans33"))
		material->surfaceParm |= SURFACEPARM_TRANS33;
	else if (!Q_stricmp(token.string, "trans66"))
		material->surfaceParm |= SURFACEPARM_TRANS66;
	else if (!Q_stricmp(token.string, "flowing"))
		material->surfaceParm |= SURFACEPARM_FLOWING;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'surfaceParm' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseGeneralIf
 =================
*/
static qboolean R_ParseGeneralIf (script_t *script, material_t *material){

	if (!R_ParseExpression(script, material, &material->conditionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'if' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseGeneralNoPicMip
 =================
*/
static qboolean R_ParseGeneralNoPicMip (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.flags |= TF_NOPICMIP;

	return true;
}

/*
 =================
 R_ParseGeneralUncompressed
 =================
*/
static qboolean R_ParseGeneralUncompressed (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.flags |= TF_UNCOMPRESSED;

	return true;
}

/*
 =================
 R_ParseGeneralHighQuality
 =================
*/
static qboolean R_ParseGeneralHighQuality (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.flags |= (TF_NOPICMIP | TF_UNCOMPRESSED);

	return true;
}

/*
 =================
 R_ParseGeneralLinear
 =================
*/
static qboolean R_ParseGeneralLinear (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.filter = TF_LINEAR;

	return true;
}

/*
 =================
 R_ParseGeneralNearest
 =================
*/
static qboolean R_ParseGeneralNearest (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.filter = TF_NEAREST;

	return true;
}

/*
 =================
 R_ParseGeneralClamp
 =================
*/
static qboolean R_ParseGeneralClamp (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.wrap = TW_CLAMP;

	return true;
}

/*
 =================
 R_ParseGeneralZeroClamp
 =================
*/
static qboolean R_ParseGeneralZeroClamp (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.wrap = TW_CLAMP_TO_ZERO;

	return true;
}

/*
 =================
 R_ParseGeneralAlphaZeroClamp
 =================
*/
static qboolean R_ParseGeneralAlphaZeroClamp (script_t *script, material_t *material){

	int		i;

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].textureStage.wrap = TW_CLAMP_TO_ZERO_ALPHA;

	return true;
}

/*
 =================
 R_ParseGeneralNoOverlays
 =================
*/
static qboolean R_ParseGeneralNoOverlays (script_t *script, material_t *material){

	material->noOverlays = true;

	return true;
}

/*
 =================
 R_ParseGeneralNoFog
 =================
*/
static qboolean R_ParseGeneralNoFog (script_t *script, material_t *material){

	material->noFog = true;

	return true;
}

/*
 =================
 R_ParseGeneralNoShadows
 =================
*/
static qboolean R_ParseGeneralNoShadows (script_t *script, material_t *material){

	material->noShadows = true;

	return true;
}

/*
 =================
 R_ParseGeneralNoSelfShadow
 =================
*/
static qboolean R_ParseGeneralNoSelfShadow (script_t *script, material_t *material){

	material->noSelfShadow = true;

	return true;
}

/*
 =================
 R_ParseGeneralFogLight
 =================
*/
static qboolean R_ParseGeneralFogLight (script_t *script, material_t *material){

	if (material->type != MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'fogLight' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->blendLight || material->ambientLight){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple light types for material '%s'\n", material->name);
		return false;
	}

	material->fogLight = true;

	return true;
}

/*
 =================
 R_ParseGeneralBlendLight
 =================
*/
static qboolean R_ParseGeneralBlendLight (script_t *script, material_t *material){

	if (material->type != MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'blendLight' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->fogLight || material->ambientLight){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple light types for material '%s'\n", material->name);
		return false;
	}

	material->blendLight = true;

	return true;
}

/*
 =================
 R_ParseGeneralAmbientLight
 =================
*/
static qboolean R_ParseGeneralAmbientLight (script_t *script, material_t *material){

	if (material->type != MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'ambientLight' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->fogLight || material->blendLight){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple light types for material '%s'\n", material->name);
		return false;
	}

	material->ambientLight = true;

	return true;
}

/*
 =================
 R_ParseGeneralSpectrum
 =================
*/
static qboolean R_ParseGeneralSpectrum (script_t *script, material_t *material){

	if (material->type != MT_GENERIC && material->type != MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'spectrum' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadInteger(script, 0, &material->spectrum)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'spectrum' in material '%s'\n", material->name);
		return false;
	}

	if (material->spectrum < 1){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for 'spectrum' in material '%s'\n", material->spectrum, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseGeneralLightFalloffImage
 =================
*/
static qboolean R_ParseGeneralLightFalloffImage (script_t *script, material_t *material){

	token_t	token;
	char	name[MAX_OSPATH];

	if (material->type != MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'lightFalloffImage' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'lightFalloffImage' in material '%s'\n", material->name);
		return false;
	}

	Q_strncpyz(name, token.string, sizeof(name));
	while (1){
		if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token))
			break;

		Q_strncatz(name, " ", sizeof(name));
		Q_strncatz(name, token.string, sizeof(name));
	}

	material->lightFalloffImage = R_FindTexture(name, TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_CLAMP);
	if (!material->lightFalloffImage){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseGeneralSpecularExponent
 =================
*/
static qboolean R_ParseGeneralSpecularExponent (script_t *script, material_t *material){

	float	specularExponent;
	int		i;

	if (!PS_ReadFloat(script, 0, &specularExponent)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'specularExponent' in material '%s'\n", material->name);
		return false;
	}

	if (specularExponent < 0.0){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %f for 'specularExponent' in material '%s'\n", specularExponent, material->name);
		return false;
	}

	for (i = 0; i < MAX_STAGES; i++)
		material->stages[i].specularExponent = specularExponent;

	return true;
}

/*
 =================
 R_ParseGeneralDecalInfo
 =================
*/
static qboolean R_ParseGeneralDecalInfo (script_t *script, material_t *material){

	token_t	token;
	int		i;

	if (!PS_ReadFloat(script, 0, &material->decalInfo.stayTime)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	if (material->decalInfo.stayTime < 0.0){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %f for 'decalInfo' in material '%s'\n", material->decalInfo.stayTime, material->name);
		return false;
	}

	if (!PS_ReadFloat(script, 0, &material->decalInfo.fadeTime)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	if (material->decalInfo.fadeTime < 0.0){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %f for 'decalInfo' in material '%s'\n", material->decalInfo.fadeTime, material->name);
		return false;
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'decalInfo' in material '%s'\n", token.string, material->name);
		return false;
	}

	for (i = 0; i < 4; i++){
		if (!PS_ReadFloat(script, 0, &material->decalInfo.startRGBA[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
			return false;
		}
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'decalInfo' in material '%s'\n", token.string, material->name);
		return false;
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, "(")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'decalInfo' in material '%s'\n", token.string, material->name);
		return false;
	}

	for (i = 0; i < 4; i++){
		if (!PS_ReadFloat(script, 0, &material->decalInfo.endRGBA[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
			return false;
		}
	}

	PS_ReadToken(script, 0, &token);
	if (Q_stricmp(token.string, ")")){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'decalInfo' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseGeneralMirror
 =================
*/
static qboolean R_ParseGeneralMirror (script_t *script, material_t *material){

	if (material->subview != SUBVIEW_NONE && material->subview != SUBVIEW_MIRROR){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple subview types for material '%s'\n", material->name);
		return false;
	}

	material->sort = SORT_SUBVIEW;

	material->subview = SUBVIEW_MIRROR;
	material->subviewWidth = 0;
	material->subviewHeight = 0;

	return true;
}

/*
 =================
 R_ParseGeneralSort
 =================
*/
static qboolean R_ParseGeneralSort (script_t *script, material_t *material){

	token_t	token;

	if (!PS_ReadToken(script, 0, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'sort' in material '%s'\n", material->name);
		return false;
	}

	if (!Q_stricmp(token.string, "subview"))
		material->sort = SORT_SUBVIEW;
	else if (!Q_stricmp(token.string, "opaque"))
		material->sort = SORT_OPAQUE;
	else if (!Q_stricmp(token.string, "sky"))
		material->sort = SORT_SKY;
	else if (!Q_stricmp(token.string, "decal"))
		material->sort = SORT_DECAL;
	else if (!Q_stricmp(token.string, "seeThrough"))
		material->sort = SORT_SEE_THROUGH;
	else if (!Q_stricmp(token.string, "banner"))
		material->sort = SORT_BANNER;
	else if (!Q_stricmp(token.string, "underwater"))
		material->sort = SORT_UNDERWATER;
	else if (!Q_stricmp(token.string, "water"))
		material->sort = SORT_WATER;
	else if (!Q_stricmp(token.string, "farthest"))
		material->sort = SORT_FARTHEST;
	else if (!Q_stricmp(token.string, "far"))
		material->sort = SORT_FAR;
	else if (!Q_stricmp(token.string, "medium"))
		material->sort = SORT_MEDIUM;
	else if (!Q_stricmp(token.string, "close"))
		material->sort = SORT_CLOSE;
	else if (!Q_stricmp(token.string, "additive"))
		material->sort = SORT_ADDITIVE;
	else if (!Q_stricmp(token.string, "almostNearest"))
		material->sort = SORT_ALMOST_NEAREST;
	else if (!Q_stricmp(token.string, "nearest"))
		material->sort = SORT_NEAREST;
	else if (!Q_stricmp(token.string, "postProcess"))
		material->sort = SORT_POST_PROCESS;
	else {
		material->sort = token.integerValue;

		if (material->sort <= SORT_BAD || material->sort > SORT_POST_PROCESS){
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'sort' parameter '%s' in material '%s'\n", token.string, material->name);
			return false;
		}
	}

	return true;
}

/*
 =================
 R_ParseGeneralBackSided
 =================
*/
static qboolean R_ParseGeneralBackSided (script_t *script, material_t *material){

	material->cullType = CT_BACK_SIDED;

	return true;
}

/*
 =================
 R_ParseGeneralTwoSided
 =================
*/
static qboolean R_ParseGeneralTwoSided (script_t *script, material_t *material){

	material->cullType = CT_TWO_SIDED;

	return true;
}

/*
 =================
 R_ParseGeneralPolygonOffset
 =================
*/
static qboolean R_ParseGeneralPolygonOffset (script_t *script, material_t *material){

	material->polygonOffset = true;

	return true;
}

/*
 =================
 R_ParseGeneralDeform
 =================
*/
static qboolean R_ParseGeneralDeform (script_t *script, material_t *material){

	token_t	token;
	int		i;

	if (!PS_ReadToken(script, 0, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'deform' in material '%s'\n", material->name);
		return false;
	}

	if (!Q_stricmp(token.string, "expand")){
		if (!R_ParseExpression(script, material, &material->deformRegisters[0])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'deform expand' in material '%s'\n", material->name);
			return false;
		}

		material->deform = DFRM_EXPAND;
	}
	else if (!Q_stricmp(token.string, "move")){
		for (i = 0; i < 3; i++){
			if (!R_ParseExpression(script, material, &material->deformRegisters[i])){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'deform move' in material '%s'\n", material->name);
				return false;
			}

			if (i < 2){
				PS_ReadToken(script, 0, &token);
				if (Q_stricmp(token.string, ",")){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'deform move' in material '%s'\n", token.string, material->name);
					return false;
				}
			}
		}

		material->deform = DFRM_MOVE;
	}
	else if (!Q_stricmp(token.string, "sprite"))
		material->deform = DFRM_SPRITE;
	else if (!Q_stricmp(token.string, "tube"))
		material->deform = DFRM_TUBE;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'deform' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseGeneralBumpMap
 =================
*/
static qboolean R_ParseGeneralBumpMap (script_t *script, material_t *material){

	stage_t	*stage;
	token_t	token;
	char	name[MAX_OSPATH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'bumpMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->numStages == MAX_STAGES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
		return false;
	}
	stage = &material->stages[material->numStages++];

	stage->textureStage.flags |= TF_NORMALMAP;

	if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'bumpMap' in material '%s'\n", material->name);
		return false;
	}

	Q_strncpyz(name, token.string, sizeof(name));
	while (1){
		if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token))
			break;

		Q_strncatz(name, " ", sizeof(name));
		Q_strncatz(name, token.string, sizeof(name));
	}

	stage->textureStage.texture = R_FindTexture(name, stage->textureStage.flags, stage->textureStage.filter, stage->textureStage.wrap);
	if (!stage->textureStage.texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	stage->lighting = SL_BUMP;

	return true;
}

/*
 =================
 R_ParseGeneralDiffuseMap
 =================
*/
static qboolean R_ParseGeneralDiffuseMap (script_t *script, material_t *material){

	stage_t	*stage;
	token_t	token;
	char	name[MAX_OSPATH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'diffuseMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->numStages == MAX_STAGES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
		return false;
	}
	stage = &material->stages[material->numStages++];

	if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'diffuseMap' in material '%s'\n", material->name);
		return false;
	}

	Q_strncpyz(name, token.string, sizeof(name));
	while (1){
		if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token))
			break;

		Q_strncatz(name, " ", sizeof(name));
		Q_strncatz(name, token.string, sizeof(name));
	}

	stage->textureStage.texture = R_FindTexture(name, stage->textureStage.flags, stage->textureStage.filter, stage->textureStage.wrap);
	if (!stage->textureStage.texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	stage->lighting = SL_DIFFUSE;

	return true;
}

/*
 =================
 R_ParseGeneralSpecularMap
 =================
*/
static qboolean R_ParseGeneralSpecularMap (script_t *script, material_t *material){

	stage_t	*stage;
	token_t	token;
	char	name[MAX_OSPATH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'specularMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->numStages == MAX_STAGES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
		return false;
	}
	stage = &material->stages[material->numStages++];

	if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'specularMap' in material '%s'\n", material->name);
		return false;
	}

	Q_strncpyz(name, token.string, sizeof(name));
	while (1){
		if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token))
			break;

		Q_strncatz(name, " ", sizeof(name));
		Q_strncatz(name, token.string, sizeof(name));
	}

	stage->textureStage.texture = R_FindTexture(name, stage->textureStage.flags, stage->textureStage.filter, stage->textureStage.wrap);
	if (!stage->textureStage.texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	stage->lighting = SL_SPECULAR;

	return true;
}

/*
 =================
 R_ParseStageIf
 =================
*/
static qboolean R_ParseStageIf (script_t *script, material_t *material, stage_t *stage){

	if (!R_ParseExpression(script, material, &stage->conditionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'if' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageNoPicMip
 =================
*/
static qboolean R_ParseStageNoPicMip (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->flags |= TF_NOPICMIP;

	return true;
}

/*
 =================
 R_ParseStageUncompressed
 =================
*/
static qboolean R_ParseStageUncompressed (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->flags |= TF_UNCOMPRESSED;

	return true;
}

/*
 =================
 R_ParseStageHighQuality
 =================
*/
static qboolean R_ParseStageHighQuality (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->flags |= (TF_NOPICMIP | TF_UNCOMPRESSED);

	return true;
}

/*
 =================
 R_ParseStageNearest
 =================
*/
static qboolean R_ParseStageNearest (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->filter = TF_NEAREST;

	return true;
}

/*
 =================
 R_ParseStageLinear
 =================
*/
static qboolean R_ParseStageLinear (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->filter = TF_LINEAR;

	return true;
}

/*
 =================
 R_ParseStageNoClamp
 =================
*/
static qboolean R_ParseStageNoClamp (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->wrap = TW_REPEAT;

	return true;
}

/*
 =================
 R_ParseStageClamp
 =================
*/
static qboolean R_ParseStageClamp (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->wrap = TW_CLAMP;

	return true;
}

/*
 =================
 R_ParseStageZeroClamp
 =================
*/
static qboolean R_ParseStageZeroClamp (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->wrap = TW_CLAMP_TO_ZERO;

	return true;
}

/*
 =================
 R_ParseStageAlphaZeroClamp
 =================
*/
static qboolean R_ParseStageAlphaZeroClamp (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	textureStage->wrap = TW_CLAMP_TO_ZERO_ALPHA;

	return true;
}

/*
 =================
 R_ParseStageMap
 =================
*/
static qboolean R_ParseStageMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	char			name[MAX_OSPATH];

	if (material->type == MT_LIGHT)
		textureStage->flags |= (TF_NOPICMIP | TF_UNCOMPRESSED);
	if (material->type == MT_NOMIP)
		textureStage->flags |= TF_NOPICMIP;

	if (stage->lighting == SL_BUMP)
		textureStage->flags |= TF_NORMALMAP;

	if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'map' in material '%s'\n", material->name);
		return false;
	}

	Q_strncpyz(name, token.string, sizeof(name));
	while (1){
		if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token))
			break;

		Q_strncatz(name, " ", sizeof(name));
		Q_strncatz(name, token.string, sizeof(name));
	}

	textureStage->texture = R_FindTexture(name, textureStage->flags, textureStage->filter, textureStage->wrap);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageCubeMap
 =================
*/
static qboolean R_ParseStageCubeMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'cubeMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindCubeMapTexture(token.string, textureStage->flags, textureStage->filter, TW_CLAMP, false);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageCameraCubeMap
 =================
*/
static qboolean R_ParseStageCameraCubeMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'cameraCubeMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindCubeMapTexture(token.string, textureStage->flags, textureStage->filter, TW_CLAMP, true);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageVideoMap
 =================
*/
static qboolean R_ParseStageVideoMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'videoMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->video = R_PlayVideo(token.string);
	if (!textureStage->video){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find video '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	textureStage->texture = tr.cinematicTexture;

	return true;
}

/*
 =================
 R_ParseStageMirrorRenderMap
 =================
*/
static qboolean R_ParseStageMirrorRenderMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (material->subview != SUBVIEW_NONE && material->subview != SUBVIEW_MIRROR){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple subview types for material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadInteger(script, 0, &material->subviewWidth)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'mirrorRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewWidth < 1 || material->subviewWidth > 640){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid width value of %i for 'mirrorRenderMap' in material '%s'\n", material->subviewWidth, material->name);
		return false;
	}

	if (!PS_ReadInteger(script, 0, &material->subviewHeight)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'mirrorRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewHeight < 1 || material->subviewHeight > 640){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid height value of %i for 'mirrorRenderMap' in material '%s'\n", material->subviewHeight, material->name);
		return false;
	}

	textureStage->texture = tr.mirrorRenderTexture;

	material->subview = SUBVIEW_MIRROR;
	material->subviewWidth = NearestPowerOfTwo(material->subviewWidth, true);
	material->subviewHeight = NearestPowerOfTwo(material->subviewHeight, true);

	return true;
}

/*
 =================
 R_ParseStageRemoteRenderMap
 =================
*/
static qboolean R_ParseStageRemoteRenderMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (material->subview != SUBVIEW_NONE && material->subview != SUBVIEW_REMOTE){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple subview types for material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadInteger(script, 0, &material->subviewWidth)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'remoteRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewWidth < 1 || material->subviewWidth > 640){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid width value of %i for 'remoteRenderMap' in material '%s'\n", material->subviewWidth, material->name);
		return false;
	}

	if (!PS_ReadInteger(script, 0, &material->subviewHeight)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'remoteRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewHeight < 1 || material->subviewHeight > 480){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid height value of %i for 'remoteRenderMap' in material '%s'\n", material->subviewHeight, material->name);
		return false;
	}

	textureStage->texture = tr.remoteRenderTexture;

	material->subview = SUBVIEW_REMOTE;
	material->subviewWidth = NearestPowerOfTwo(material->subviewWidth, true);
	material->subviewHeight = NearestPowerOfTwo(material->subviewHeight, true);

	return true;
}

/*
 =================
 R_ParseStageTexGen
 =================
*/
static qboolean R_ParseStageTexGen (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i, j;

	if (!PS_ReadToken(script, 0, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'texGen' in material '%s'\n", material->name);
		return false;
	}

	if (!Q_stricmp(token.string, "explicit"))
		textureStage->texGen = TG_EXPLICIT;
	else if (!Q_stricmp(token.string, "vector")){
		for (i = 0; i < 2; i++){
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, "(")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead for 'texGen vector' in material '%s'\n", token.string, material->name);
				return false;
			}

			for (j = 0; j < 4; j++){
				if (!PS_ReadFloat(script, 0, &textureStage->texGenVectors[i][j])){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'texGen vector' in material '%s'\n", material->name);
					return false;
				}
			}

			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, ")")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead for 'texGen vector' in material '%s'\n", token.string, material->name);
				return false;
			}
		}

		textureStage->texGen = TG_VECTOR;
	}
	else if (!Q_stricmp(token.string, "reflect"))
		textureStage->texGen = TG_REFLECT;
	else if (!Q_stricmp(token.string, "normal"))
		textureStage->texGen = TG_NORMAL;
	else if (!Q_stricmp(token.string, "skyBox"))
		textureStage->texGen = TG_SKYBOX;
	else if (!Q_stricmp(token.string, "screen"))
		textureStage->texGen = TG_SCREEN;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'texGen' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageTranslate
 =================
*/
static qboolean R_ParseStageTranslate (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexTransforms == MAX_TEXTURE_TRANSFORMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXTURE_TRANSFORMS hit in material '%s'\n", material->name);
		return false;
	}

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texTransformRegisters[textureStage->numTexTransforms][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'translate' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'translate' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->texTransform[textureStage->numTexTransforms++] = TT_TRANSLATE;

	return true;
}

/*
 =================
 R_ParseStageScroll
 =================
*/
static qboolean R_ParseStageScroll (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexTransforms == MAX_TEXTURE_TRANSFORMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXTURE_TRANSFORMS hit in material '%s'\n", material->name);
		return false;
	}

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texTransformRegisters[textureStage->numTexTransforms][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'scroll' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'scroll' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->texTransform[textureStage->numTexTransforms++] = TT_SCROLL;

	return true;
}

/*
 =================
 R_ParseStageScale
 =================
*/
static qboolean R_ParseStageScale (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexTransforms == MAX_TEXTURE_TRANSFORMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXTURE_TRANSFORMS hit in material '%s'\n", material->name);
		return false;
	}

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texTransformRegisters[textureStage->numTexTransforms][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'scale' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'scale' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->texTransform[textureStage->numTexTransforms++] = TT_SCALE;

	return true;
}

/*
 =================
 R_ParseStageCenterScale
 =================
*/
static qboolean R_ParseStageCenterScale (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexTransforms == MAX_TEXTURE_TRANSFORMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXTURE_TRANSFORMS hit in material '%s'\n", material->name);
		return false;
	}

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texTransformRegisters[textureStage->numTexTransforms][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'centerScale' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'centerScale' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->texTransform[textureStage->numTexTransforms++] = TT_CENTERSCALE;

	return true;
}

/*
 =================
 R_ParseStageShear
 =================
*/
static qboolean R_ParseStageShear (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexTransforms == MAX_TEXTURE_TRANSFORMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXTURE_TRANSFORMS hit in material '%s'\n", material->name);
		return false;
	}

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texTransformRegisters[textureStage->numTexTransforms][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'shear' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'shear' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->texTransform[textureStage->numTexTransforms++] = TT_SHEAR;

	return true;
}

/*
 =================
 R_ParseStageRotate
 =================
*/
static qboolean R_ParseStageRotate (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->numTexTransforms == MAX_TEXTURE_TRANSFORMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXTURE_TRANSFORMS hit in material '%s'\n", material->name);
		return false;
	}

	if (!R_ParseExpression(script, material, &textureStage->texTransformRegisters[textureStage->numTexTransforms][0])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'rotate' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texTransform[textureStage->numTexTransforms++] = TT_ROTATE;

	return true;
}

/*
 =================
 R_ParseStageRed
 =================
*/
static qboolean R_ParseStageRed (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[0])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'red' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageGreen
 =================
*/
static qboolean R_ParseStageGreen (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[1])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'green' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageBlue
 =================
*/
static qboolean R_ParseStageBlue (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[2])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'blue' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageAlpha
 =================
*/
static qboolean R_ParseStageAlpha (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[3])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'alpha' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageRGB
 =================
*/
static qboolean R_ParseStageRGB (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;
	int				expressionRegister;

	if (!R_ParseExpression(script, material, &expressionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'rgb' in material '%s'\n", material->name);
		return false;
	}

	colorStage->registers[0] = expressionRegister;
	colorStage->registers[1] = expressionRegister;
	colorStage->registers[2] = expressionRegister;

	return true;
}

/*
 =================
 R_ParseStageRGBA
 =================
*/
static qboolean R_ParseStageRGBA (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;
	int				expressionRegister;

	if (!R_ParseExpression(script, material, &expressionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'rgba' in material '%s'\n", material->name);
		return false;
	}

	colorStage->registers[0] = expressionRegister;
	colorStage->registers[1] = expressionRegister;
	colorStage->registers[2] = expressionRegister;
	colorStage->registers[3] = expressionRegister;

	return true;
}

/*
 =================
 R_ParseStageColor
 =================
*/
static qboolean R_ParseStageColor (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;
	token_t			token;
	int				i;

	for (i = 0; i < 4; i++){
		if (!R_ParseExpression(script, material, &colorStage->registers[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'color' in material '%s'\n", material->name);
			return false;
		}

		if (i < 3){
			PS_ReadToken(script, 0, &token);
			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'color' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	return true;
}

/*
 =================
 R_ParseStageColored
 =================
*/
static qboolean R_ParseStageColored (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	colorStage->registers[0] = EXP_REGISTER_PARM0;
	colorStage->registers[1] = EXP_REGISTER_PARM1;
	colorStage->registers[2] = EXP_REGISTER_PARM2;
	colorStage->registers[3] = EXP_REGISTER_PARM3;

	return true;
}

/*
 =================
 R_ParseStageVertexColor
 =================
*/
static qboolean R_ParseStageVertexColor (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	colorStage->vertexColor = VC_MODULATE;

	return true;
}

/*
 =================
 R_ParseStageInverseVertexColor
 =================
*/
static qboolean R_ParseStageInverseVertexColor (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	colorStage->vertexColor = VC_INVERSE_MODULATE;

	return true;
}

/*
 =================
 R_ParseStageProgram
 =================
*/
static qboolean R_ParseStageProgram (script_t *script, material_t *material, stage_t *stage){

	programStage_t	*programStage = &stage->programStage;
	token_t			token;

	if (material->type == MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'program' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'program' in material '%s'\n", material->name);
		return false;
	}

	if ((!glConfig.vertexProgram || !glConfig.fragmentProgram) || !r_shaderPrograms->integerValue){
		stage->programs = true;
		return true;
	}

	programStage->vertexProgram = R_FindProgram(token.string, GL_VERTEX_PROGRAM_ARB);
	if (!programStage->vertexProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find vertex program '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	programStage->fragmentProgram = R_FindProgram(token.string, GL_FRAGMENT_PROGRAM_ARB);
	if (!programStage->fragmentProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find fragment program '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	stage->programs = true;

	return true;
}

/*
 =================
 R_ParseStageVertexProgram
 =================
*/
static qboolean R_ParseStageVertexProgram (script_t *script, material_t *material, stage_t *stage){

	programStage_t	*programStage = &stage->programStage;
	token_t			token;

	if (material->type == MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'vertexProgram' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'vertexProgram' in material '%s'\n", material->name);
		return false;
	}

	if ((!glConfig.vertexProgram || !glConfig.fragmentProgram) || !r_shaderPrograms->integerValue){
		stage->programs = true;
		return true;
	}

	programStage->vertexProgram = R_FindProgram(token.string, GL_VERTEX_PROGRAM_ARB);
	if (!programStage->vertexProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find vertex program '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	stage->programs = true;

	return true;
}

/*
 =================
 R_ParseStageVertexParm
 =================
*/
static qboolean R_ParseStageVertexParm (script_t *script, material_t *material, stage_t *stage){

	programStage_t	*programStage = &stage->programStage;
	programParm_t	*programParm;
	token_t			token;
	int				i;

	if (material->type == MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'vertexParm' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (programStage->numVertexParms == MAX_PROGRAM_PARMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_PROGRAM_PARMS hit in material '%s'\n", material->name);
		return false;
	}
	programParm = &programStage->vertexParms[programStage->numVertexParms++];

	if (!PS_ReadUnsigned(script, 0, &programParm->index)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'vertexParm' in material '%s'\n", material->name);
		return false;
	}

	if (programParm->index >= MAX_PROGRAM_PARMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid index value of %u for 'vertexParm' in material '%s'\n", programParm->index, material->name);
		return false;
	}

	for (i = 0; i < 4; i++){
		if (i != 0){
			if (!PS_ReadToken(script, 0, &token))
				break;

			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'vertexParm' in material '%s'\n", token.string, material->name);
				return false;
			}
		}

		if (!R_ParseExpression(script, material, &programParm->registers[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'vertexParm' in material '%s'\n", material->name);
			return false;
		}
	}

	if (i != 4){
		switch (i){
		case 1:
			programParm->registers[1] = programParm->registers[0];
			programParm->registers[2] = programParm->registers[0];
			programParm->registers[3] = programParm->registers[0];

			break;
		case 2:
			programParm->registers[2] = EXP_REGISTER_ZERO;
			programParm->registers[3] = EXP_REGISTER_ONE;

			break;
		case 3:
			programParm->registers[3] = EXP_REGISTER_ONE;

			break;
		}
	}

	return true;
}

/*
 =================
 R_ParseStageFragmentProgram
 =================
*/
static qboolean R_ParseStageFragmentProgram (script_t *script, material_t *material, stage_t *stage){

	programStage_t	*programStage = &stage->programStage;
	token_t			token;

	if (material->type == MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'fragmentProgram' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentProgram' in material '%s'\n", material->name);
		return false;
	}

	if ((!glConfig.vertexProgram || !glConfig.fragmentProgram) || !r_shaderPrograms->integerValue){
		stage->programs = true;
		return true;
	}

	programStage->fragmentProgram = R_FindProgram(token.string, GL_FRAGMENT_PROGRAM_ARB);
	if (!programStage->fragmentProgram){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find fragment program '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	stage->programs = true;

	return true;
}

/*
 =================
 R_ParseStageFragmentParm
 =================
*/
static qboolean R_ParseStageFragmentParm (script_t *script, material_t *material, stage_t *stage){

	programStage_t	*programStage = &stage->programStage;
	programParm_t	*programParm;
	token_t			token;
	int				i;

	if (material->type == MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'fragmentParm' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (programStage->numFragmentParms == MAX_PROGRAM_PARMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_PROGRAM_PARMS hit in material '%s'\n", material->name);
		return false;
	}
	programParm = &programStage->fragmentParms[programStage->numFragmentParms++];

	if (!PS_ReadUnsigned(script, 0, &programParm->index)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentParm' in material '%s'\n", material->name);
		return false;
	}

	if (programParm->index >= MAX_PROGRAM_PARMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid index value of %u for 'fragmentParm' in material '%s'\n", programParm->index, material->name);
		return false;
	}

	for (i = 0; i < 4; i++){
		if (i != 0){
			if (!PS_ReadToken(script, 0, &token))
				break;

			if (Q_stricmp(token.string, ",")){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'fragmentParm' in material '%s'\n", token.string, material->name);
				return false;
			}
		}

		if (!R_ParseExpression(script, material, &programParm->registers[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'fragmentParm' in material '%s'\n", material->name);
			return false;
		}
	}

	if (i != 4){
		switch (i){
		case 1:
			programParm->registers[1] = programParm->registers[0];
			programParm->registers[2] = programParm->registers[0];
			programParm->registers[3] = programParm->registers[0];

			break;
		case 2:
			programParm->registers[2] = EXP_REGISTER_ZERO;
			programParm->registers[3] = EXP_REGISTER_ONE;

			break;
		case 3:
			programParm->registers[3] = EXP_REGISTER_ONE;

			break;
		}
	}

	return true;
}

/*
 =================
 R_ParseStageFragmentMap
 =================
*/
static qboolean R_ParseStageFragmentMap (script_t *script, material_t *material, stage_t *stage){

	programStage_t	*programStage = &stage->programStage;
	programMap_t	*programMap;
	textureFlags_t	flags = 0;
	textureFilter_t	filter = TF_DEFAULT;
	textureWrap_t	wrap = TW_REPEAT;
	char			name[MAX_OSPATH];
	token_t			token;

	if (material->type == MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'fragmentMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (programStage->numFragmentMaps == MAX_PROGRAM_MAPS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_PROGRAM_MAPS hit in material '%s'\n", material->name);
		return false;
	}
	programMap = &programStage->fragmentMaps[programStage->numFragmentMaps++];

	if (!PS_ReadUnsigned(script, 0, &programMap->index)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentMap' in material '%s'\n", material->name);
		return false;
	}

	if (programMap->index >= MAX_PROGRAM_MAPS){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid index value of %u for 'fragmentMap' in material '%s'\n", programMap->index, material->name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentMap' in material '%s'\n", material->name);
			return false;
		}

		if (!Q_stricmp(token.string, "bumpMap"))
			flags |= TF_NORMALMAP;
		else if (!Q_stricmp(token.string, "noPicMip"))
			flags |= TF_NOPICMIP;
		else if (!Q_stricmp(token.string, "uncompressed"))
			flags |= TF_UNCOMPRESSED;
		else if (!Q_stricmp(token.string, "highQuality"))
			flags |= (TF_NOPICMIP | TF_UNCOMPRESSED);
		else if (!Q_stricmp(token.string, "nearest"))
			filter = TF_NEAREST;
		else if (!Q_stricmp(token.string, "linear"))
			filter = TF_LINEAR;
		else if (!Q_stricmp(token.string, "noClamp"))
			wrap = TW_REPEAT;
		else if (!Q_stricmp(token.string, "clamp"))
			wrap = TW_CLAMP;
		else if (!Q_stricmp(token.string, "zeroClamp"))
			wrap = TW_CLAMP_TO_ZERO;
		else if (!Q_stricmp(token.string, "alphaZeroClamp"))
			wrap = TW_CLAMP_TO_ZERO_ALPHA;
		else if (!Q_stricmp(token.string, "cubeMap")){
			if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentMap' in material '%s'\n", material->name);
				return false;
			}

			if ((!glConfig.vertexProgram || !glConfig.fragmentProgram) || !r_shaderPrograms->integerValue)
				break;

			programMap->texture = R_FindCubeMapTexture(token.string, flags, filter, TW_CLAMP, false);
			if (!programMap->texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
				return false;
			}

			break;
		}
		else if (!Q_stricmp(token.string, "cameraCubeMap")){
			if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentMap' in material '%s'\n", material->name);
				return false;
			}

			if ((!glConfig.vertexProgram || !glConfig.fragmentProgram) || !r_shaderPrograms->integerValue)
				break;

			programMap->texture = R_FindCubeMapTexture(token.string, flags, filter, TW_CLAMP, true);
			if (!programMap->texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
				return false;
			}

			break;
		}
		else if (!Q_stricmp(token.string, "videoMap")){
			if (!PS_ReadToken(script, PSF_ALLOW_PATHNAMES, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'fragmentMap' in material '%s'\n", material->name);
				return false;
			}

			if ((!glConfig.vertexProgram || !glConfig.fragmentProgram) || !r_shaderPrograms->integerValue)
				break;

			programMap->video = R_PlayVideo(token.string);
			if (!programMap->video){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find video '%s' for material '%s'\n", token.string, material->name);
				return false;
			}

			programMap->texture = tr.cinematicTexture;

			break;
		}
		else {
			Q_strncpyz(name, token.string, sizeof(name));
			while (1){
				if (!PS_ReadToken(script, PSF_PARSE_GENERIC, &token))
					break;

				Q_strncatz(name, " ", sizeof(name));
				Q_strncatz(name, token.string, sizeof(name));
			}

			if ((!glConfig.vertexProgram || !glConfig.fragmentProgram) || !r_shaderPrograms->integerValue)
				break;

			programMap->texture = R_FindTexture(name, flags, filter, wrap);
			if (!programMap->texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
				return false;
			}

			break;
		}
	}

	return true;
}

/*
 =================
 R_ParseStageSpecularExponent
 =================
*/
static qboolean R_ParseStageSpecularExponent (script_t *script, material_t *material, stage_t *stage){

	if (!PS_ReadFloat(script, 0, &stage->specularExponent)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'specularExponent' in material '%s'\n", material->name);
		return false;
	}

	if (stage->specularExponent < 0.0){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %f for 'specularExponent' in material '%s'\n", stage->specularExponent, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseStageShadowDraw
 =================
*/
static qboolean R_ParseStageShadowDraw (script_t *script, material_t *material, stage_t *stage){

	stage->shadowDraw = true;

	return true;
}

/*
 =================
 R_ParseStagePrivatePolygonOffset
 =================
*/
static qboolean R_ParseStagePrivatePolygonOffset (script_t *script, material_t *material, stage_t *stage){

	stage->privatePolygonOffset = true;

	return true;
}

/*
 =================
 R_ParseStageAlphaTest
 =================
*/
static qboolean R_ParseStageAlphaTest (script_t *script, material_t *material, stage_t *stage){

	if (!R_ParseExpression(script, material, &stage->alphaTestRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'alphaTest' in material '%s'\n", material->name);
		return false;
	}

	stage->alphaTest = true;

	return true;
}

/*
 =================
 R_ParseStageIgnoreAlphaTest
 =================
*/
static qboolean R_ParseStageIgnoreAlphaTest (script_t *script, material_t *material, stage_t *stage){

	stage->ignoreAlphaTest = true;

	return true;
}

/*
 =================
 R_ParseStageBlend
 =================
*/
static qboolean R_ParseStageBlend (script_t *script, material_t *material, stage_t *stage){

	token_t	token;

	if (!PS_ReadToken(script, 0, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'blend' in material '%s'\n", material->name);
		return false;
	}

	if (!Q_stricmp(token.string, "bumpMap")){
		if (material->type != MT_GENERIC){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'blend bumpMap' not allowed in material '%s'\n", material->name);
			return false;
		}

		stage->lighting = SL_BUMP;
		return true;
	}
	if (!Q_stricmp(token.string, "diffuseMap")){
		if (material->type != MT_GENERIC){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'blend diffuseMap' not allowed in material '%s'\n", material->name);
			return false;
		}

		stage->lighting = SL_DIFFUSE;
		return true;
	}
	if (!Q_stricmp(token.string, "specularMap")){
		if (material->type != MT_GENERIC){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'blend specularMap' not allowed in material '%s'\n", material->name);
			return false;
		}

		stage->lighting = SL_SPECULAR;
		return true;
	}

	if (!Q_stricmp(token.string, "none")){
		stage->blendSrc = GL_ZERO;
		stage->blendDst = GL_ONE;
	}
	else if (!Q_stricmp(token.string, "add")){
		stage->blendSrc = GL_ONE;
		stage->blendDst = GL_ONE;
	}
	else if (!Q_stricmp(token.string, "blend")){
		stage->blendSrc = GL_SRC_ALPHA;
		stage->blendDst = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp(token.string, "filter") || !Q_stricmp(token.string, "modulate")){
		stage->blendSrc = GL_DST_COLOR;
		stage->blendDst = GL_ZERO;
	}
	else {
		if (!Q_stricmp(token.string, "GL_ZERO"))
			stage->blendSrc = GL_ZERO;
		else if (!Q_stricmp(token.string, "GL_ONE"))
			stage->blendSrc = GL_ONE;
		else if (!Q_stricmp(token.string, "GL_DST_COLOR"))
			stage->blendSrc = GL_DST_COLOR;
		else if (!Q_stricmp(token.string, "GL_ONE_MINUS_DST_COLOR"))
			stage->blendSrc = GL_ONE_MINUS_DST_COLOR;
		else if (!Q_stricmp(token.string, "GL_SRC_ALPHA"))
			stage->blendSrc = GL_SRC_ALPHA;
		else if (!Q_stricmp(token.string, "GL_ONE_MINUS_SRC_ALPHA"))
			stage->blendSrc = GL_ONE_MINUS_SRC_ALPHA;
		else if (!Q_stricmp(token.string, "GL_DST_ALPHA"))
			stage->blendSrc = GL_DST_ALPHA;
		else if (!Q_stricmp(token.string, "GL_ONE_MINUS_DST_ALPHA"))
			stage->blendSrc = GL_ONE_MINUS_DST_ALPHA;
		else if (!Q_stricmp(token.string, "GL_SRC_ALPHA_SATURATE"))
			stage->blendSrc = GL_SRC_ALPHA_SATURATE;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'blend' parameter '%s' in material '%s'\n", token.string, material->name);
			return false;
		}

		PS_ReadToken(script, 0, &token);
		if (Q_stricmp(token.string, ",")){
			Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'blend' in material '%s'\n", token.string, material->name);
			return false;
		}

		if (!PS_ReadToken(script, 0, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'blend' in material '%s'\n", material->name);
			return false;
		}

		if (!Q_stricmp(token.string, "GL_ZERO"))
			stage->blendDst = GL_ZERO;
		else if (!Q_stricmp(token.string, "GL_ONE"))
			stage->blendDst = GL_ONE;
		else if (!Q_stricmp(token.string, "GL_SRC_COLOR"))
			stage->blendDst = GL_SRC_COLOR;
		else if (!Q_stricmp(token.string, "GL_ONE_MINUS_SRC_COLOR"))
			stage->blendDst = GL_ONE_MINUS_SRC_COLOR;
		else if (!Q_stricmp(token.string, "GL_SRC_ALPHA"))
			stage->blendDst = GL_SRC_ALPHA;
		else if (!Q_stricmp(token.string, "GL_ONE_MINUS_SRC_ALPHA"))
			stage->blendDst = GL_ONE_MINUS_SRC_ALPHA;
		else if (!Q_stricmp(token.string, "GL_DST_ALPHA"))
			stage->blendDst = GL_DST_ALPHA;
		else if (!Q_stricmp(token.string, "GL_ONE_MINUS_DST_ALPHA"))
			stage->blendDst = GL_ONE_MINUS_DST_ALPHA;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'blend' parameter '%s' in material '%s'\n", token.string, material->name);
			return false;
		}
	}

	stage->blend = true;

	return true;
}

/*
 =================
 R_ParseStageMaskRed
 =================
*/
static qboolean R_ParseStageMaskRed (script_t *script, material_t *material, stage_t *stage){

	stage->maskRed = true;

	return true;
}

/*
 =================
 R_ParseStageMaskGreen
 =================
*/
static qboolean R_ParseStageMaskGreen (script_t *script, material_t *material, stage_t *stage){

	stage->maskGreen = true;

	return true;
}

/*
 =================
 R_ParseStageMaskBlue
 =================
*/
static qboolean R_ParseStageMaskBlue (script_t *script, material_t *material, stage_t *stage){

	stage->maskBlue = true;

	return true;
}

/*
 =================
 R_ParseStageMaskColor
 =================
*/
static qboolean R_ParseStageMaskColor (script_t *script, material_t *material, stage_t *stage){

	stage->maskRed = true;
	stage->maskGreen = true;
	stage->maskBlue = true;

	return true;
}

/*
 =================
 R_ParseStageMaskAlpha
 =================
*/
static qboolean R_ParseStageMaskAlpha (script_t *script, material_t *material, stage_t *stage){

	stage->maskAlpha = true;

	return true;
}


// =====================================================================

typedef struct {
	const char	*name;
	qboolean	(*parseCmd)(script_t *, material_t *);
} materialGeneralCmd_t;

typedef struct {
	const char	*name;
	qboolean	(*parseCmd)(script_t *, material_t *, stage_t *);
} materialStageCmd_t;

static materialGeneralCmd_t	r_materialGeneralCmds[] = {
	{"surfaceParm",				R_ParseGeneralSurfaceParm},
	{"if",						R_ParseGeneralIf},
	{"noPicMip",				R_ParseGeneralNoPicMip},
	{"uncompressed",			R_ParseGeneralUncompressed},
	{"highQuality",				R_ParseGeneralHighQuality},
	{"linear",					R_ParseGeneralLinear},
	{"nearest",					R_ParseGeneralNearest},
	{"clamp",					R_ParseGeneralClamp},
	{"zeroClamp",				R_ParseGeneralZeroClamp},
	{"alphaZeroClamp",			R_ParseGeneralAlphaZeroClamp},
	{"noOverlays",				R_ParseGeneralNoOverlays},
	{"noFog",					R_ParseGeneralNoFog},
	{"noShadows",				R_ParseGeneralNoShadows},
	{"noSelfShadow",			R_ParseGeneralNoSelfShadow},
	{"fogLight",				R_ParseGeneralFogLight},
	{"blendLight",				R_ParseGeneralBlendLight},
	{"ambientLight",			R_ParseGeneralAmbientLight},
	{"spectrum",				R_ParseGeneralSpectrum},
	{"lightFalloffImage",		R_ParseGeneralLightFalloffImage},
	{"specularExponent",		R_ParseGeneralSpecularExponent},
	{"decalInfo",				R_ParseGeneralDecalInfo},
	{"mirror",					R_ParseGeneralMirror},
	{"sort",					R_ParseGeneralSort},
	{"backSided",				R_ParseGeneralBackSided},
	{"twoSided",				R_ParseGeneralTwoSided},
	{"polygonOffset",			R_ParseGeneralPolygonOffset},
	{"deform",					R_ParseGeneralDeform},
	{"bumpMap",					R_ParseGeneralBumpMap},
	{"diffuseMap",				R_ParseGeneralDiffuseMap},
	{"specularMap",				R_ParseGeneralSpecularMap},
	{NULL,						NULL}
};

static materialStageCmd_t	r_materialStageCmds[] = {
	{"if",						R_ParseStageIf},
	{"noPicMip",				R_ParseStageNoPicMip},
	{"uncompressed",			R_ParseStageUncompressed},
	{"highQuality",				R_ParseStageHighQuality},
	{"nearest",					R_ParseStageNearest},
	{"linear",					R_ParseStageLinear},
	{"noClamp",					R_ParseStageNoClamp},
	{"clamp",					R_ParseStageClamp},
	{"zeroClamp",				R_ParseStageZeroClamp},
	{"alphaZeroClamp",			R_ParseStageAlphaZeroClamp},
	{"map",						R_ParseStageMap},
	{"cubeMap",					R_ParseStageCubeMap},
	{"cameraCubeMap",			R_ParseStageCameraCubeMap},
	{"videoMap",				R_ParseStageVideoMap},
	{"mirrorRenderMap",			R_ParseStageMirrorRenderMap},
	{"remoteRenderMap",			R_ParseStageRemoteRenderMap},
	{"texGen",					R_ParseStageTexGen},
	{"translate",				R_ParseStageTranslate},
	{"scroll",					R_ParseStageScroll},
	{"scale",					R_ParseStageScale},
	{"centerScale",				R_ParseStageCenterScale},
	{"shear",					R_ParseStageShear},
	{"rotate",					R_ParseStageRotate},
	{"red",						R_ParseStageRed},
	{"green",					R_ParseStageGreen},
	{"blue",					R_ParseStageBlue},
	{"alpha",					R_ParseStageAlpha},
	{"rgb",						R_ParseStageRGB},
	{"rgba",					R_ParseStageRGBA},
	{"color",					R_ParseStageColor},
	{"colored",					R_ParseStageColored},
	{"vertexColor",				R_ParseStageVertexColor},
	{"inverseVertexColor",		R_ParseStageInverseVertexColor},
	{"program",					R_ParseStageProgram},
	{"vertexProgram",			R_ParseStageVertexProgram},
	{"vertexParm",				R_ParseStageVertexParm},
	{"fragmentProgram",			R_ParseStageFragmentProgram},
	{"fragmentParm",			R_ParseStageFragmentParm},
	{"fragmentMap",				R_ParseStageFragmentMap},
	{"specularExponent",		R_ParseStageSpecularExponent},
	{"shadowDraw",				R_ParseStageShadowDraw},
	{"privatePolygonOffset",	R_ParseStagePrivatePolygonOffset},
	{"alphaTest",				R_ParseStageAlphaTest},
	{"ignoreAlphaTest",			R_ParseStageIgnoreAlphaTest},
	{"blend",					R_ParseStageBlend},
	{"maskRed",					R_ParseStageMaskRed},
	{"maskGreen",				R_ParseStageMaskGreen},
	{"maskBlue",				R_ParseStageMaskBlue},
	{"maskColor",				R_ParseStageMaskColor},
	{"maskAlpha",				R_ParseStageMaskAlpha},
	{NULL,						NULL}
};


/*
 =================
 R_ParseGeneralCommand
 =================
*/
static qboolean R_ParseGeneralCommand (script_t *script, const char *command, material_t *material){

	materialGeneralCmd_t	*cmd;

	for (cmd = r_materialGeneralCmds; cmd->name; cmd++){
		if (!Q_stricmp(cmd->name, command))
			return cmd->parseCmd(script, material);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown general command '%s' in material '%s'\n", command, material->name);
	return false;
}

/*
 =================
 R_ParseStageCommand
 =================
*/
static qboolean R_ParseStageCommand (script_t *script, const char *command, material_t *material, stage_t *stage){

	materialStageCmd_t		*cmd;

	for (cmd = r_materialStageCmds; cmd->name; cmd++){
		if (!Q_stricmp(cmd->name, command))
			return cmd->parseCmd(script, material, stage);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown stage command '%s' in material '%s'\n", command, material->name);
	return false;
}

/*
 =================
 R_ParseMaterial
 =================
*/
static qboolean R_ParseMaterial (script_t *script, material_t *material){

	stage_t	*stage;
	token_t	token;

	// Parse the material
	if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: material '%s' has an empty script\n", material->name);
		return false;
	}

	if (!Q_stricmp(token.string, "{")){
		while (1){
			if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in material '%s'\n", material->name);
				return false;	// End of data
			}

			if (!Q_stricmp(token.string, "}"))
				break;			// End of material

			// Parse a stage
			if (!Q_stricmp(token.string, "{")){
				// Create a new stage
				if (material->numStages == MAX_STAGES){
					Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
					return false;
				}
				stage = &material->stages[material->numStages++];

				// Parse it
				while (1){
					if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token)){
						Com_Printf(S_COLOR_YELLOW "WARNING: no matching '}' in material '%s'\n", material->name);
						return false;	// End of data
					}

					if (!Q_stricmp(token.string, "}"))
						break;			// End of stage

					// Parse the command
					if (!R_ParseStageCommand(script, token.string, material, stage))
						return false;
				}

				continue;
			}

			// Parse the command
			if (!R_ParseGeneralCommand(script, token.string, material))
				return false;
		}
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 =================
 R_ParseMaterialFile
 =================
*/
static void R_ParseMaterialFile (script_t *script, const char *name){

	mtrScript_t	*mtrScript;
	script_t	*scriptBlock;
	token_t		token;
	char		*buffer, *end;
	int			size;
	unsigned	hash;

	while (1){
		// Parse the name
		if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES | PSF_ALLOW_PATHNAMES, &token))
			break;		// End of data

		// If it's "table", then parse a table
		if (!Q_stricmp(token.string, "table")){
			if (!R_ParseTable(script))
				PS_SkipRestOfLine(script);

			continue;
		}

		// Parse the script
		buffer = script->text;
		PS_SkipBracedSection(script, 0);
		end = script->text;

		if (!buffer)
			buffer = script->buffer;
		if (!end)
			end = script->buffer + script->size;

		size = end - buffer;

		// Create the mtrScript
		mtrScript = Hunk_Alloc(sizeof(mtrScript_t));

		Q_strncpyz(mtrScript->name, token.string, sizeof(mtrScript->name));
		mtrScript->type = -1;
		mtrScript->surfaceParm = 0;

		Q_snprintfz(mtrScript->source, sizeof(mtrScript->source), "materials/%s", name);
		mtrScript->line = token.line;

		mtrScript->buffer = Hunk_Alloc(size + 1);
		memcpy(mtrScript->buffer, buffer, size);
		mtrScript->buffer[size] = 0;

		mtrScript->size = size;

		// Add to hash table
		hash = Com_HashKey(mtrScript->name, MATERIALS_HASH_SIZE);

		mtrScript->nextHash = r_materialScriptsHashTable[hash];
		r_materialScriptsHashTable[hash] = mtrScript;

		// We must parse surfaceParm commands here, because
		// R_FindMaterial needs this for correct material loading.
		// Proper syntax checking will be done when the material is
		// actually loaded.
		scriptBlock = PS_LoadScriptMemory(mtrScript->name, mtrScript->buffer, mtrScript->size);
		if (scriptBlock){
			while (1){
				if (!PS_ReadToken(scriptBlock, PSF_ALLOW_NEWLINES, &token))
					break;		// End of data

				if (!Q_stricmp(token.string, "surfaceParm")){
					if (!PS_ReadToken(scriptBlock, 0, &token))
						continue;

					if (!Q_stricmp(token.string, "lighting"))
						mtrScript->surfaceParm |= SURFACEPARM_LIGHTING;
					else if (!Q_stricmp(token.string, "sky"))
						mtrScript->surfaceParm |= SURFACEPARM_SKY;
					else if (!Q_stricmp(token.string, "warp"))
						mtrScript->surfaceParm |= SURFACEPARM_WARP;
					else if (!Q_stricmp(token.string, "trans33"))
						mtrScript->surfaceParm |= SURFACEPARM_TRANS33;
					else if (!Q_stricmp(token.string, "trans66"))
						mtrScript->surfaceParm |= SURFACEPARM_TRANS66;
					else if (!Q_stricmp(token.string, "flowing"))
						mtrScript->surfaceParm |= SURFACEPARM_FLOWING;
					else
						continue;

					mtrScript->type = MT_GENERIC;
				}
			}

			PS_FreeScript(scriptBlock);
		}
	}
}


/*
 =======================================================================

 MATERIAL INITIALIZATION AND LOADING

 =======================================================================
*/


/*
 =================
 R_NewMaterial
 =================
*/
static material_t *R_NewMaterial (void){

	material_t	*material;

	// Clear the material
	material = &r_parseMaterial;
	memset(material, 0, sizeof(material_t));

	// Clear the stages
	material->stages = r_parseMaterialStages;
	memset(material->stages, 0, MAX_STAGES * sizeof(stage_t));

	// Clear the expression ops
	material->ops = r_parseMaterialOps;
	memset(material->ops, 0, MAX_EXPRESSION_OPS * sizeof(expOp_t));

	// Clear the expression registers
	material->expressionRegisters = r_parseMaterialExpressionRegisters;
	memset(material->expressionRegisters, 0, MAX_EXPRESSION_REGISTERS * sizeof(float));

	return material;
}

/*
 =================
 R_CreateDefaultMaterial
 =================
*/
static material_t *R_CreateDefaultMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm){

	material_t	*material;

	// Create a new material
	material = R_NewMaterial();

	// Fill it in
	Q_strncpyz(material->name, name, sizeof(material->name));
	material->index = r_numMaterials;
	material->defaulted = true;

	material->type = type;
	material->coverage = MC_OPAQUE;
	material->surfaceParm = surfaceParm;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;

	if (type == MT_LIGHT)
		material->stages->textureStage.texture = tr.attenuationTexture;
	else
		material->stages->textureStage.texture = tr.defaultTexture;

	material->numStages++;

	return material;
}

/*
 =================
 R_CreateMaterial
 =================
*/
static material_t *R_CreateMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm, mtrScript_t *mtrScript){

	material_t	*material;
	script_t	*script;
	int			i;

	// Create a new material
	material = R_NewMaterial();

	// Fill it in
	Q_strncpyz(material->name, name, sizeof(material->name));
	material->index = r_numMaterials;

	material->type = type;
	material->coverage = MC_OPAQUE;
	material->surfaceParm = surfaceParm;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;

	// If we have a script, create an external material
	if (mtrScript){
		material->mtrScript = mtrScript;

		// Load the script text
		script = PS_LoadScriptMemory(mtrScript->name, mtrScript->buffer, mtrScript->size);
		if (!script)
			return R_CreateDefaultMaterial(name, type, surfaceParm);

		// Parse it
		if (!R_ParseMaterial(script, material)){
			PS_FreeScript(script);

			return R_CreateDefaultMaterial(name, type, surfaceParm);
		}

		PS_FreeScript(script);

		return material;
	}

	// Otherwise create an internal material
	switch (material->type){
	case MT_GENERIC:
		if (material->surfaceParm & SURFACEPARM_LIGHTING){
			// Bump stage
			material->stages[material->numStages].textureStage.texture = R_FindTexture(va("%s_local.tga", material->name), TF_NORMALMAP, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture)
				material->stages[material->numStages].textureStage.texture = tr.flatTexture;

			material->stages[material->numStages++].lighting = SL_BUMP;

			// Diffuse stage
			material->stages[material->numStages].textureStage.texture = R_FindTexture(va("%s_d.tga", material->name), 0, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture){
				material->stages[material->numStages].textureStage.texture = R_FindTexture(va("%s.tga", material->name), 0, TF_DEFAULT, TW_REPEAT);
				if (!material->stages[material->numStages].textureStage.texture){
					Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default...\n", material->name);
					material->stages[material->numStages].textureStage.texture = tr.whiteTexture;
				}
			}

			material->stages[material->numStages++].lighting = SL_DIFFUSE;

			// Specular stage
			material->stages[material->numStages].textureStage.texture = R_FindTexture(va("%s_s.tga", material->name), 0, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture)
				material->stages[material->numStages].textureStage.texture = tr.blackTexture;

			material->stages[material->numStages++].lighting = SL_SPECULAR;

			// Add flowing if needed
			if (material->surfaceParm & SURFACEPARM_FLOWING){
				for (i = 0; i < material->numStages; i++){
					material->stages[i].textureStage.texTransform[0] = TT_SCROLL;
					material->stages[i].textureStage.texTransformRegisters[0][0] = EXP_REGISTER_GLOBAL5;
					material->stages[i].textureStage.texTransformRegisters[0][1] = EXP_REGISTER_ZERO;
					material->stages[i].textureStage.numTexTransforms++;
				}
			}
		}
		else if (material->surfaceParm & SURFACEPARM_SKY){
			material->noOverlays = true;
			material->noFog = true;
			material->noShadows = true;
			material->noSelfShadow = true;

			material->stages[material->numStages].textureStage.texture = R_FindCubeMapTexture(material->name, 0, TF_DEFAULT, TW_CLAMP, true);
			if (!material->stages[material->numStages].textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default...\n", material->name);
				material->stages[material->numStages].textureStage.texture = tr.defaultTexture;
			}
			else
				material->stages[material->numStages].textureStage.texGen = TG_SKYBOX;

			material->numStages++;
		}
		else {
			material->stages[material->numStages].textureStage.texture = R_FindTexture(va("%s.tga", material->name), 0, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default...\n", material->name);
				material->stages[material->numStages].textureStage.texture = tr.defaultTexture;
			}

			// Make it translucent if needed
			if (material->surfaceParm & (SURFACEPARM_TRANS33 | SURFACEPARM_TRANS66)){
				material->noFog = true;
				material->noShadows = true;
				material->noSelfShadow = true;

				material->stages[material->numStages].colorStage.vertexColor = VC_MODULATE;
				material->stages[material->numStages].ignoreAlphaTest = true;
				material->stages[material->numStages].blend = true;
				material->stages[material->numStages].blendSrc = GL_SRC_ALPHA;
				material->stages[material->numStages].blendDst = GL_ONE_MINUS_SRC_ALPHA;
			}

			// Make it warped if needed
			if (material->surfaceParm & SURFACEPARM_WARP){
				material->noOverlays = true;
				material->noShadows = true;
				material->noSelfShadow = true;

				// Add flowing if needed
				if (material->surfaceParm & SURFACEPARM_FLOWING){
					material->stages[material->numStages].textureStage.texTransform[0] = TT_SCROLL;
					material->stages[material->numStages].textureStage.texTransformRegisters[0][0] = EXP_REGISTER_GLOBAL4;
					material->stages[material->numStages].textureStage.texTransformRegisters[0][1] = EXP_REGISTER_ZERO;
					material->stages[material->numStages].textureStage.numTexTransforms++;
				}
			}
			else {
				// Add flowing if needed
				if (material->surfaceParm & SURFACEPARM_FLOWING){
					material->stages[material->numStages].textureStage.texTransform[0] = TT_SCROLL;
					material->stages[material->numStages].textureStage.texTransformRegisters[0][0] = EXP_REGISTER_GLOBAL5;
					material->stages[material->numStages].textureStage.texTransformRegisters[0][1] = EXP_REGISTER_ZERO;
					material->stages[material->numStages].textureStage.numTexTransforms++;
				}
			}

			material->numStages++;
		}

		break;
	case MT_LIGHT:
		material->lightFalloffImage = tr.falloffTexture;

		material->stages[material->numStages].textureStage.texture = R_FindTexture(va("%s.tga", material->name), TF_NOPICMIP | TF_UNCOMPRESSED, TF_DEFAULT, TW_CLAMP_TO_ZERO);
		if (!material->stages[material->numStages].textureStage.texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default...\n", material->name);
			material->stages[material->numStages].textureStage.texture = tr.attenuationTexture;
		}

		material->stages[material->numStages].colorStage.registers[0] = EXP_REGISTER_PARM0;
		material->stages[material->numStages].colorStage.registers[1] = EXP_REGISTER_PARM1;
		material->stages[material->numStages].colorStage.registers[2] = EXP_REGISTER_PARM2;
		material->stages[material->numStages].colorStage.registers[3] = EXP_REGISTER_PARM3;

		material->numStages++;

		break;
	case MT_NOMIP:
		material->stages[material->numStages].textureStage.texture = R_FindTexture(va("%s.tga", material->name), TF_NOPICMIP, TF_LINEAR, TW_REPEAT);
		if (!material->stages[material->numStages].textureStage.texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default...\n", material->name);
			material->stages[material->numStages].textureStage.texture = tr.defaultTexture;
		}

		material->stages[material->numStages].colorStage.vertexColor = VC_MODULATE;
		material->stages[material->numStages].blend = true;
		material->stages[material->numStages].blendSrc = GL_SRC_ALPHA;
		material->stages[material->numStages].blendDst = GL_ONE_MINUS_SRC_ALPHA;

		material->numStages++;

		break;
	}

	return material;
}

/*
 =================
 R_FinishMaterial
 =================
*/
static void R_FinishMaterial (material_t *material){

	stage_t	*stage;
	int		i, j;

	// 'fogLight', 'blendLight' and 'ambientLight' imply 'noShadows'
	if (material->fogLight || material->blendLight || material->ambientLight)
		material->noShadows = true;

	// Make sure it has a 'lightFalloffImage'
	if (!material->lightFalloffImage){
		if (material->fogLight)
			material->lightFalloffImage = tr.fogEnterTexture;
		else
			material->lightFalloffImage = tr.falloffTexture;
	}

	// Force 2D materials to use 'twoSided'
	if (material->type == MT_NOMIP)
		material->cullType = CT_TWO_SIDED;

	// 'backSided' and 'twoSided' imply 'noShadows'
	if (material->cullType != CT_FRONT_SIDED)
		material->noShadows = true;

	// Check stages
	for (i = 0, stage = material->stages; i < material->numStages; i++, stage++){
		// Check whether it is a program or a texture & color stage
		if (stage->programs){
			// Make sure it is an ambient stage
			if (stage->lighting != SL_AMBIENT){
				Com_Printf(S_COLOR_YELLOW "WARNING: material '%s' has a non-ambient stage with programs!\n", material->name);

				stage->lighting = SL_AMBIENT;
			}

			// Ignore program stages on non-capable hardware
			if (!stage->programStage.vertexProgram || !stage->programStage.fragmentProgram)
				stage->conditionRegister = EXP_REGISTER_ZERO;

			// If 'sort' is unset and the material references
			// _currentRender, force it to use 'sort postProcess'
			if (material->sort == SORT_BAD && stage->conditionRegister != EXP_REGISTER_ZERO){
				for (j = 0; j < stage->programStage.numFragmentMaps; j++){
					if (stage->programStage.fragmentMaps[j].texture == tr.currentRenderTexture)
						break;
				}

				if (j != stage->programStage.numFragmentMaps)
					material->sort = SORT_POST_PROCESS;
			}
		}
		else {
			// Make sure it has a texture
			if (!stage->textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: material '%s' has a stage with no texture!\n", material->name);

				if (material->type == MT_LIGHT){
					if (material->fogLight)
						stage->textureStage.texture = tr.fogTexture;
					else
						stage->textureStage.texture = tr.attenuationTexture;
				}
				else {
					switch (stage->lighting){
					case SL_AMBIENT:
						stage->textureStage.texture = tr.defaultTexture;

						break;
					case SL_BUMP:
						stage->textureStage.texture = tr.flatTexture;

						break;
					case SL_DIFFUSE:
						stage->textureStage.texture = tr.whiteTexture;

						break;
					case SL_SPECULAR:
						stage->textureStage.texture = tr.blackTexture;

						break;
					}
				}
			}

			// If 'sort' is unset and the material references
			// _currentRender, force it to use 'sort postProcess'
			if (material->sort == SORT_BAD && stage->conditionRegister != EXP_REGISTER_ZERO){
				if (stage->textureStage.texture == tr.currentRenderTexture)
					material->sort = SORT_POST_PROCESS;
			}
		}

		// Make sure it has a 'specularExponent'
		if (!stage->specularExponent)
			stage->specularExponent = 16.0;

		// Check if it may have alpha tested holes
		if (stage->alphaTest)
			material->coverage = MC_PERFORATED;

		// Force 2D materials to use 'ignoreAlphaTest'
		if (material->type == MT_NOMIP)
			stage->ignoreAlphaTest = true;

		// Count ambient stages
		if (stage->lighting == SL_AMBIENT)
			material->numAmbientStages++;
	}

	// Set 'sort' if unset
	if (material->sort == SORT_BAD){
		if (material->type == MT_GENERIC && (material->surfaceParm & SURFACEPARM_SKY))
			material->sort = SORT_SKY;
		else {
			if (material->numStages != material->numAmbientStages)
				material->sort = SORT_OPAQUE;
			else {
				if (material->polygonOffset)
					material->sort = SORT_DECAL;
				else {
					if (!material->stages->blend || (material->stages->blendSrc == GL_ONE && material->stages->blendDst == GL_ZERO))
						material->sort = SORT_OPAQUE;
					else {
						if ((material->stages->blendSrc == GL_SRC_ALPHA && material->stages->blendDst == GL_ONE) || (material->stages->blendSrc == GL_ONE && material->stages->blendDst == GL_ONE))
							material->sort = SORT_ADDITIVE;
						else
							material->sort = SORT_MEDIUM;
					}
				}
			}
		}
	}

	// Check if it's translucent
	if (material->sort > SORT_OPAQUE){
		// Set the coverage
		material->coverage = MC_TRANSLUCENT;

		// Force it to use 'noFog'
		material->noFog = true;

		// Forge it to use 'ignoreAlphaTest'
		for (i = 0, stage = material->stages; i < material->numStages; i++, stage++)
			stage->ignoreAlphaTest = true;
	}
}

/*
 =================
 R_OptimizeMaterial
 =================
*/
static void R_OptimizeMaterial (material_t *material){

	float	*registers = material->expressionRegisters;
	expOp_t	*op;
	int		i;

	// Make sure the predefined registers are initialized
	registers[EXP_REGISTER_ONE] = 1.0;
	registers[EXP_REGISTER_ZERO] = 0.0;
	registers[EXP_REGISTER_TIME] = 0.0;
	registers[EXP_REGISTER_PARM0] = 0.0;
	registers[EXP_REGISTER_PARM1] = 0.0;
	registers[EXP_REGISTER_PARM2] = 0.0;
	registers[EXP_REGISTER_PARM3] = 0.0;
	registers[EXP_REGISTER_PARM4] = 0.0;
	registers[EXP_REGISTER_PARM5] = 0.0;
	registers[EXP_REGISTER_PARM6] = 0.0;
	registers[EXP_REGISTER_PARM7] = 0.0;
	registers[EXP_REGISTER_GLOBAL0] = 0.0;
	registers[EXP_REGISTER_GLOBAL1] = 0.0;
	registers[EXP_REGISTER_GLOBAL2] = 0.0;
	registers[EXP_REGISTER_GLOBAL3] = 0.0;
	registers[EXP_REGISTER_GLOBAL4] = 0.0;
	registers[EXP_REGISTER_GLOBAL5] = 0.0;
	registers[EXP_REGISTER_GLOBAL6] = 0.0;
	registers[EXP_REGISTER_GLOBAL7] = 0.0;

	// Check for constant expressions
	if (!material->numOps)
		return;

	for (i = 0, op = material->ops; i < material->numOps; i++, op++){
		if (op->opType != OP_TYPE_TABLE){
			if (op->a < EXP_REGISTER_NUM_PREDEFINED || op->b < EXP_REGISTER_NUM_PREDEFINED)
				break;
		}
		else {
			if (op->b < EXP_REGISTER_NUM_PREDEFINED)
				break;
		}
	}

	if (i != material->numOps)
		return;		// Something references a variable

	// Evaluate all the registers
	for (i = 0, op = material->ops; i < material->numOps; i++, op++){
		switch (op->opType){
		case OP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case OP_TYPE_DIVIDE:
			if (registers[op->b] == 0.0){
				registers[op->c] = 0.0;
				break;
			}

			registers[op->c] = registers[op->a] / registers[op->b];
			break;
		case OP_TYPE_MOD:
			if (registers[op->b] == 0.0){
				registers[op->c] = 0.0;
				break;
			}

			registers[op->c] = (int)registers[op->a] % (int)registers[op->b];
			break;
		case OP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case OP_TYPE_GREATER:
			registers[op->c] = registers[op->a] > registers[op->b];
			break;
		case OP_TYPE_LESS:
			registers[op->c] = registers[op->a] < registers[op->b];
			break;
		case OP_TYPE_GEQUAL:
			registers[op->c] = registers[op->a] >= registers[op->b];
			break;
		case OP_TYPE_LEQUAL:
			registers[op->c] = registers[op->a] <= registers[op->b];
			break;
		case OP_TYPE_EQUAL:
			registers[op->c] = registers[op->a] == registers[op->b];
			break;
		case OP_TYPE_NOTEQUAL:
			registers[op->c] = registers[op->a] != registers[op->b];
			break;
		case OP_TYPE_AND:
			registers[op->c] = registers[op->a] && registers[op->b];
			break;
		case OP_TYPE_OR:
			registers[op->c] = registers[op->a] || registers[op->b];
			break;
		case OP_TYPE_TABLE:
			registers[op->c] = R_LookupTable(op->a, registers[op->b]);
			break;
		}
	}

	// We don't need to evaluate the registers during rendering, except
	// for development purposes
	material->constantExpressions = true;
}

/*
 =================
 R_FixMaterialIndices
 =================
*/
static void R_FixMaterialIndices (mesh_t *meshes, int numMeshes, int index){

	mesh_t			*mesh;
	material_t		*material;
	renderEntity_t	*entity;
	meshType_t		type;
	qboolean		caps;
	int				i, sortedIndex;

	if (!numMeshes)
		return;

	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		sortedIndex = (mesh->sort >> 20) & (MAX_MATERIALS-1);
		if (sortedIndex < index)
			continue;

		R_DecomposeSort(mesh->sort, &material, &entity, &type, &caps);

		mesh->sort = ((sortedIndex + 1) << 20) | (entity->index << 9) | (type << 5) | (caps);
	}
}

/*
 =================
 R_FixRenderCommandList

 This is a nasty issue. Materials can be registered after render views
 are generated but before the frame is rendered. This will, for the
 duration of one frame, cause render views to be rendered with bad
 materials. To fix this, we need to go through all the render commands
 and fix the sorted index.
 =================
*/
static void R_FixRenderCommandList (int index){

	renderCommandList_t	*commandList = &backEnd.commandList;
	const void			*data;

	if (!commandList->used)
		return;

	// Add an end of list command to make sure we won't get stuck into
	// an infinite loop
	*(int *)(commandList->data + commandList->used) = RC_END_OF_LIST;

	// Run through the commands
	data = commandList->data;
	while (1){
		switch (*(const int *)data){
		case RC_RENDER_VIEW:
			{
				const renderViewCommand_t		*renderViewCmd = data;
				light_t							*light;
				int								i;

				R_FixMaterialIndices(renderViewCmd->meshes, renderViewCmd->numMeshes, index);
				R_FixMaterialIndices(renderViewCmd->postProcessMeshes, renderViewCmd->numPostProcessMeshes, index);

				for (i = 0, light = renderViewCmd->lights; i < renderViewCmd->numLights; i++, light++){
					R_FixMaterialIndices(light->shadows[0], light->numShadows[0], index);
					R_FixMaterialIndices(light->shadows[1], light->numShadows[1], index);

					R_FixMaterialIndices(light->interactions[0], light->numInteractions[0], index);
					R_FixMaterialIndices(light->interactions[1], light->numInteractions[1], index);
				}

				for (i = 0, light = renderViewCmd->fogLights; i < renderViewCmd->numFogLights; i++, light++){
					R_FixMaterialIndices(light->shadows[0], light->numShadows[0], index);
					R_FixMaterialIndices(light->interactions[0], light->numInteractions[0], index);
				}

				data = (const void *)(renderViewCmd + 1);
			}

			break;
		case RC_CAPTURE_RENDER:
			{
				const captureRenderCommand_t	*captureRenderCmd = data;
				data = (const void *)(captureRenderCmd + 1);
			}

			break;
		case RC_UPDATE_TEXTURE:
			{
				const updateTextureCommand_t	*updateTextureCmd = data;
				data = (const void *)(updateTextureCmd + 1);
			}

			break;
		case RC_STRETCH_PIC:
			{
				const stretchPicCommand_t		*stretchPicCmd = data;
				data = (const void *)(stretchPicCmd + 1);
			}

			break;
		case RC_SHEARED_PIC:
			{
				const shearedPicCommand_t		*shearedPicCmd = data;
				data = (const void *)(shearedPicCmd + 1);
			}

			break;
		case RC_RENDER_SIZE:
			{
				const renderSizeCommand_t		*renderSizeCmd = data;
				data = (const void *)(renderSizeCmd + 1);
			}

			break;
		case RC_DRAW_BUFFER:
			{
				const drawBufferCommand_t	*drawBufferCmd = data;
				data = (const void *)(drawBufferCmd + 1);
			}

			break;
		case RC_SWAP_BUFFERS:
			{
				const swapBuffersCommand_t		*swapBuffersCmd = data;
				data = (const void *)(swapBuffersCmd + 1);
			}

			break;
		case RC_END_OF_LIST:
		default:
			return;
		}
	}
}

/*
 =================
 R_SortMaterial
 =================
*/
static void R_SortMaterial (material_t *material){

	int		i;

	for (i = r_numMaterials - 2; i >= 0; i--){
		if (tr.sortedMaterials[i]->sort <= material->sort)
			break;

		tr.sortedMaterials[i+1] = tr.sortedMaterials[i];
		tr.sortedMaterials[i+1]->index++;
	}

	R_FixRenderCommandList(i+1);

	material->index = i+1;
	tr.sortedMaterials[i+1] = material;
}

/*
 =================
 R_LoadMaterial
 =================
*/
static material_t *R_LoadMaterial (material_t *newMaterial){

	material_t	*material;
	unsigned	hash;

	if (r_numMaterials == MAX_MATERIALS)
		Com_Error(ERR_DROP, "R_LoadMaterial: MAX_MATERIALS hit");

	r_materials[r_numMaterials++] = material = Z_Malloc(sizeof(material_t));

	// Copy the material
	memcpy(material, newMaterial, sizeof(material_t));

	// Allocate and copy the stages
	material->stages = Hunk_Alloc(material->numStages * sizeof(stage_t));
	memcpy(material->stages, newMaterial->stages, material->numStages * sizeof(stage_t));

	// Allocate and copy the expression ops
	material->ops = Hunk_Alloc(material->numOps * sizeof(expOp_t));
	memcpy(material->ops, newMaterial->ops, material->numOps * sizeof(stage_t));

	// Allocate and copy the expression registers
	material->expressionRegisters = Hunk_Alloc(material->numRegisters * sizeof(float));
	memcpy(material->expressionRegisters, newMaterial->expressionRegisters, material->numRegisters * sizeof(float));

	// Make sure all the parameters are valid
	R_FinishMaterial(material);

	// Check for constant expressions
	R_OptimizeMaterial(material);

	// Sort the material
	R_SortMaterial(material);

	// Add to hash table
	hash = Com_HashKey(material->name, MATERIALS_HASH_SIZE);

	material->nextHash = r_materialsHashTable[hash];
	r_materialsHashTable[hash] = material;

	return material;
}

/*
 =================
 R_FindMaterial
 =================
*/
material_t *R_FindMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm){

	material_t	*material;
	mtrScript_t	*mtrScript;
	unsigned	hash;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "R_FindMaterial: NULL material name");

	if (strlen(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "R_FindMaterial: material name exceeds MAX_OSPATH");

	// See if already loaded
	hash = Com_HashKey(name, MATERIALS_HASH_SIZE);

	for (material = r_materialsHashTable[hash]; material; material = material->nextHash){
		if (material->type != type || material->surfaceParm != surfaceParm)
			continue;

		if (!Q_stricmp(material->name, name))
			return material;
	}

	// Performance evaluation option
	if (r_singleMaterial->integerValue && (type != MT_NOMIP)){
		if (type == MT_LIGHT)
			return tr.defaultLightMaterial;
		else
			return tr.defaultMaterial;
	}

	// See if there's a script for this material
	for (mtrScript = r_materialScriptsHashTable[hash]; mtrScript; mtrScript = mtrScript->nextHash){
		if (!Q_stricmp(mtrScript->name, name)){
			if (mtrScript->type == -1)
				break;

			if (mtrScript->type == type && mtrScript->surfaceParm == surfaceParm)
				break;
		}
	}

	// Create the material
	material = R_CreateMaterial(name, type, surfaceParm, mtrScript);

	// Load it in
	return R_LoadMaterial(material);
}

/*
 =================
 R_RegisterMaterial
 =================
*/
material_t *R_RegisterMaterial (const char *name, qboolean lightingDefault){

	if (lightingDefault)
		return R_FindMaterial(name, MT_GENERIC, SURFACEPARM_LIGHTING);
	else
		return R_FindMaterial(name, MT_GENERIC, 0);
}

/*
 =================
 R_RegisterMaterialLight
 =================
*/
material_t *R_RegisterMaterialLight (const char *name){

	return R_FindMaterial(name, MT_LIGHT, 0);
}

/*
 =================
 R_RegisterMaterialNoMip
 =================
*/
material_t *R_RegisterMaterialNoMip (const char *name){

	return R_FindMaterial(name, MT_NOMIP, 0);
}

/*
 =================
 R_EvaluateRegisters
 =================
*/
void R_EvaluateRegisters (material_t *material, float time, const float *entityParms, const float *globalParms){

	float	*registers = material->expressionRegisters;
	expOp_t	*op;
	int		i;

	if (r_skipExpressions->integerValue)
		return;

	if (material->constantExpressions && !r_skipConstantExpressions->integerValue)
		return;

	// Update the predefined registers
	registers[EXP_REGISTER_ONE] = 1.0;
	registers[EXP_REGISTER_ZERO] = 0.0;
	registers[EXP_REGISTER_TIME] = time;
	registers[EXP_REGISTER_PARM0] = entityParms[0];
	registers[EXP_REGISTER_PARM1] = entityParms[1];
	registers[EXP_REGISTER_PARM2] = entityParms[2];
	registers[EXP_REGISTER_PARM3] = entityParms[3];
	registers[EXP_REGISTER_PARM4] = entityParms[4];
	registers[EXP_REGISTER_PARM5] = entityParms[5];
	registers[EXP_REGISTER_PARM6] = entityParms[6];
	registers[EXP_REGISTER_PARM7] = entityParms[7];
	registers[EXP_REGISTER_GLOBAL0] = globalParms[0];
	registers[EXP_REGISTER_GLOBAL1] = globalParms[1];
	registers[EXP_REGISTER_GLOBAL2] = globalParms[2];
	registers[EXP_REGISTER_GLOBAL3] = globalParms[3];
	registers[EXP_REGISTER_GLOBAL4] = globalParms[4];
	registers[EXP_REGISTER_GLOBAL5] = globalParms[5];
	registers[EXP_REGISTER_GLOBAL6] = globalParms[6];
	registers[EXP_REGISTER_GLOBAL7] = globalParms[7];

	// Evaluate all the registers
	for (i = 0, op = material->ops; i < material->numOps; i++, op++){
		switch (op->opType){
		case OP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case OP_TYPE_DIVIDE:
			if (registers[op->b] == 0.0)
				Com_Error(ERR_DROP, "R_EvaluateRegisters: division by zero");

			registers[op->c] = registers[op->a] / registers[op->b];
			break;
		case OP_TYPE_MOD:
			if (registers[op->b] == 0.0)
				Com_Error(ERR_DROP, "R_EvaluateRegisters: division by zero");

			registers[op->c] = (int)registers[op->a] % (int)registers[op->b];
			break;
		case OP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case OP_TYPE_GREATER:
			registers[op->c] = registers[op->a] > registers[op->b];
			break;
		case OP_TYPE_LESS:
			registers[op->c] = registers[op->a] < registers[op->b];
			break;
		case OP_TYPE_GEQUAL:
			registers[op->c] = registers[op->a] >= registers[op->b];
			break;
		case OP_TYPE_LEQUAL:
			registers[op->c] = registers[op->a] <= registers[op->b];
			break;
		case OP_TYPE_EQUAL:
			registers[op->c] = registers[op->a] == registers[op->b];
			break;
		case OP_TYPE_NOTEQUAL:
			registers[op->c] = registers[op->a] != registers[op->b];
			break;
		case OP_TYPE_AND:
			registers[op->c] = registers[op->a] && registers[op->b];
			break;
		case OP_TYPE_OR:
			registers[op->c] = registers[op->a] || registers[op->b];
			break;
		case OP_TYPE_TABLE:
			registers[op->c] = R_LookupTable(op->a, registers[op->b]);
			break;
		default:
			Com_Error(ERR_DROP, "R_EvaluateRegisters: bad opType (%i)", op->opType);
		}
	}
}

/*
 =================
 R_EnumMaterialScripts

 This is for use by integrated editors
 =================
*/
void R_EnumMaterialScripts (void (*callback)(const char *)){

	mtrScript_t	*mtrScript;
	int			i;

	for (i = 0; i < MATERIALS_HASH_SIZE; i++){
		mtrScript = r_materialScriptsHashTable[i];

		while (mtrScript){
			callback(mtrScript->name);

			mtrScript = mtrScript->nextHash;
		}
	}
}

/*
 =================
 R_MaterialInfo_f
 =================
*/
void R_MaterialInfo_f (void){

	material_t		*material;
	expOp_t			*op;
	char			*name;
	materialType_t	type;
	surfaceParm_t	surfaceParm;
	char			*buffer, *copy;
	int				i, len = 0;

	if (Cmd_Argc() != 2 && Cmd_Argc() != 4){
		Com_Printf("Usage: materialInfo <materialName> [<type> <surfaceParm>]\n");
		return;
	}

	name = Cmd_Argv(1);

	if (Cmd_Argc() == 4){
		type = atoi(Cmd_Argv(2));
		surfaceParm = atoi(Cmd_Argv(3));
	}
	else {
		type = -1;
		surfaceParm = 0;
	}

	// Find the material
	for (i = 0; i < r_numMaterials; i++){
		material = r_materials[i];

		if (type == -1){
			if (!Q_stricmp(material->name, name))
				break;

			continue;
		}

		if (material->type != type || material->surfaceParm != surfaceParm)
			continue;

		if (!Q_stricmp(material->name, name))
			break;
	}

	if (i == r_numMaterials){
		Com_Printf("Material '%s' not found\n", name);
		return;
	}

	Com_Printf("Material: %s\n", material->name);
	Com_Printf("Stages: %i total (%i ambient, %i lighting)\n", material->numStages, material->numAmbientStages, material->numStages - material->numAmbientStages);

	// Print source
	if (material->mtrScript){
		// Reformat the buffer
		copy = Z_Malloc(material->mtrScript->size * 4);

		buffer = material->mtrScript->buffer;
		while (*buffer){
			// Convert \r\n or \n\r to \n
			if ((*buffer == '\r' && buffer[1] == '\n') || (*buffer == '\n' && buffer[1] == '\r')){
				copy[len++] = '\n';

				buffer += 2;
				continue;
			}

			// Copy \n or convert \r to \n
			if (*buffer == '\n' || *buffer == '\r'){
				copy[len++] = '\n';

				buffer += 1;
				continue;
			}

			// Convert \t to 4 spaces
			if (*buffer == '\t'){
				copy[len++] = ' ';
				copy[len++] = ' ';
				copy[len++] = ' ';
				copy[len++] = ' ';

				buffer += 1;
				continue;
			}

			// Ignore non-printable characters
			if (*buffer < ' '){
				buffer += 1;
				continue;
			}

			// Copy the character
			copy[len++] = *buffer++;
		}
		copy[len] = 0;

		Com_Printf("Source: %s (line: %i)\n", material->mtrScript->source, material->mtrScript->line);
		Com_Printf("------------------------------\n");
		Com_Printf("%s%s\n", material->mtrScript->name, copy);
		Com_Printf("------------------------------\n");

		Z_Free(copy);
	}
	else
		Com_Printf("Source: generated internally%s\n", (material->defaulted) ? " (DEFAULTED)" : "");

	// Print expression ops
	if (material->numOps){
		Com_Printf("\n");
		Com_Printf("Ops:\n");
		Com_Printf("----\n");

		for (i = 0, op = material->ops; i < material->numOps; i++, op++){
			Com_Printf("%4i: ", i);

			if (op->opType == OP_TYPE_TABLE){
				Com_Printf("%i = %s[ %i ]\n", op->c, r_tables[op->a]->name, op->b);
				continue;
			}

			switch (op->opType){
			case OP_TYPE_MULTIPLY:
				name = "*";
				break;
			case OP_TYPE_DIVIDE:
				name = "/";
				break;
			case OP_TYPE_MOD:
				name = "%";
				break;
			case OP_TYPE_ADD:
				name = "+";
				break;
			case OP_TYPE_SUBTRACT:
				name = "-";
				break;
			case OP_TYPE_GREATER:
				name = ">";
				break;
			case OP_TYPE_LESS:
				name = "<";
				break;
			case OP_TYPE_GEQUAL:
				name = ">=";
				break;
			case OP_TYPE_LEQUAL:
				name = "<=";
				break;
			case OP_TYPE_EQUAL:
				name = "==";
				break;
			case OP_TYPE_NOTEQUAL:
				name = "!=";
				break;
			case OP_TYPE_AND:
				name = "&&";
				break;
			case OP_TYPE_OR:
				name = "||";
				break;
			default:
				name = "<UNKNOWN OPTYPE>";
				break;
			}

			Com_Printf("%i = %i %s %i\n", op->c, op->a, name, op->b);
		}
	}

	// Print expression registers
	if (material->numRegisters){
		Com_Printf("\n");
		Com_Printf("Registers:\n");
		Com_Printf("----------\n");

		for (i = 0; i < material->numRegisters; i++)
			Com_Printf("%4i: %f\n", i, material->expressionRegisters[i]);
	}
}

/*
 =================
 R_ListTables_f
 =================
*/
void R_ListTables_f (void){

	table_t	*table;
	int		i;

	Com_Printf("\n");
	Com_Printf("      flags size -name--------\n");

	for (i = 0; i < r_numTables; i++){
		table = r_tables[i];

		Com_Printf("%4i: ", i);

		if (table->clamp)
			Com_Printf("cl ");
		else
			Com_Printf("   ");

		if (table->snap)
			Com_Printf("sn ");
		else
			Com_Printf("   ");

		Com_Printf("%4i ", table->size);

		Com_Printf("%s\n", table->name);
	}

	Com_Printf("------------------------------\n");
	Com_Printf("%i total tables\n", r_numTables);
	Com_Printf("\n");
}

/*
 =================
 R_ListMaterials_f
 =================
*/
void R_ListMaterials_f (void){

	material_t	*material;
	int			i;

	Com_Printf("\n");
	Com_Printf("      -stg -amb -ops -reg src -type-- -name--------\n");

	for (i = 0; i < r_numMaterials; i++){
		material = r_materials[i];

		Com_Printf("%4i: ", i);

		Com_Printf("%4i %4i ", material->numStages, material->numAmbientStages);

		Com_Printf("%4i %4i ", material->numOps, material->numRegisters);

		if (material->mtrScript)
			Com_Printf(" E  ");
		else
			Com_Printf(" I  ");

		switch (material->type){
		case MT_GENERIC:
			Com_Printf("GENERIC ");
			break;
		case MT_LIGHT:
			Com_Printf("LIGHT   ");
			break;
		case MT_NOMIP:
			Com_Printf("NOMIP   ");
			break;
		default:
			Com_Printf("??????? ");
			break;
		}

		Com_Printf("%s%s\n", material->name, (material->defaulted) ? " (DEFAULTED)" : "");
	}

	Com_Printf("---------------------------------------------------\n");
	Com_Printf("%i total materials\n", r_numMaterials);
	Com_Printf("\n");
}

/*
 =================
 R_CreateBuiltInMaterials
 =================
*/
static void R_CreateBuiltInMaterials (void){

	material_t	*material;

	// Default material
	material = R_NewMaterial();

	Q_strncpyz(material->name, "_default", sizeof(material->name));
	material->index = r_numMaterials;

	material->type = MT_GENERIC;
	material->coverage = MC_OPAQUE;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;
	material->stages->textureStage.texture = tr.defaultTexture;
	material->stages->colorStage.vertexColor = VC_MODULATE;
	material->numStages++;

	tr.defaultMaterial = R_LoadMaterial(material);

	// Default light material
	material = R_NewMaterial();

	Q_strncpyz(material->name, "_defaultLight", sizeof(material->name));
	material->index = r_numMaterials;

	material->type = MT_LIGHT;
	material->coverage = MC_OPAQUE;
	material->lightFalloffImage = tr.falloffTexture;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;
	material->stages->textureStage.texture = tr.attenuationTexture;
	material->stages->colorStage.registers[0] = EXP_REGISTER_PARM0;
	material->stages->colorStage.registers[1] = EXP_REGISTER_PARM1;
	material->stages->colorStage.registers[2] = EXP_REGISTER_PARM2;
	material->stages->colorStage.registers[3] = EXP_REGISTER_PARM3;
	material->numStages++;

	tr.defaultLightMaterial = R_LoadMaterial(material);

	// No-draw material
	material = R_NewMaterial();

	Q_strncpyz(material->name, "_noDraw", sizeof(material->name));
	material->index = r_numMaterials;

	material->type = MT_GENERIC;
	material->coverage = MC_OPAQUE;
	material->noOverlays = true;
	material->noFog = true;
	material->noShadows = true;
	material->noSelfShadow = true;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;

	tr.noDrawMaterial = R_LoadMaterial(material);
}

/*
 =================
 R_InitMaterials
 =================
*/
void R_InitMaterials (void){

	script_t	*script;
	char		**fileList;
	int			numFiles;
	char		name[MAX_OSPATH];
	int			i;

	Com_Printf("Initializing Materials\n");

	// Load .material files
	fileList = FS_ListFiles("materials", ".mtr", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Load the script file
		Q_snprintfz(name, sizeof(name), "materials/%s", fileList[i]);
		Com_Printf("...loading '%s'", name);

		script = PS_LoadScriptFile(name);
		if (!script){
			Com_Printf(": failed\n");
			continue;
		}
		Com_Printf("\n");

		// Parse it
		R_ParseMaterialFile(script, fileList[i]);

		PS_FreeScript(script);
	}

	FS_FreeFileList(fileList);

	// Create built-in materials
	R_CreateBuiltInMaterials();
}

/*
 =================
 R_ShutdownMaterials
 =================
*/
void R_ShutdownMaterials (void){

	table_t		*table;
	material_t	*material;
	int			i;

	for (i = 0; i < r_numTables; i++){
		table = r_tables[i];

		Z_Free(table);
	}

	for (i = 0; i < r_numMaterials; i++){
		material = r_materials[i];

		Z_Free(material);
	}

	memset(r_tablesHashTable, 0, sizeof(r_tablesHashTable));
	memset(r_materialsHashTable, 0, sizeof(r_materialsHashTable));
	memset(r_materialScriptsHashTable, 0, sizeof(r_materialScriptsHashTable));

	memset(r_tables, 0, sizeof(r_tables));
	memset(r_materials, 0, sizeof(r_materials));

	r_numTables = 0;
	r_numMaterials = 0;
}
