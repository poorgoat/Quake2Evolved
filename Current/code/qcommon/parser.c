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


#include "qcommon.h"


static punctuation_t	ps_punctuationsTable[] = {
	{">>=",		PT_RSHIFT_ASSIGN},
	{"<<=",		PT_LSHIFT_ASSIGN},
	{"...",		PT_PARAMETERS},
	{"##",		PT_PRECOMPILER_MERGE},
	{"&&",		PT_LOGIC_AND},
	{"||",		PT_LOGIC_OR},
	{">=",		PT_LOGIC_GEQUAL},
	{"<=",		PT_LOGIC_LEQUAL},
	{"==",		PT_LOGIC_EQUAL},
	{"!=",		PT_LOGIC_NOTEQUAL},
	{"*=",		PT_MUL_ASSIGN},
	{"/=",		PT_DIV_ASSIGN},
	{"%=",		PT_MOD_ASSIGN},
	{"+=",		PT_ADD_ASSIGN},
	{"-=",		PT_SUB_ASSIGN},
	{"++",		PT_INCREMENT},
	{"--",		PT_DECREMENT},
	{"&=",		PT_BINARY_AND_ASSIGN},
	{"|=",		PT_BINARY_OR_ASSIGN},
	{"^=",		PT_BINARY_XOR_ASSIGN},
	{">>",		PT_RSHIFT},
	{"<<",		PT_LSHIFT},
	{"->",		PT_POINTER_REFERENCE},
	{"::",		PT_CPP_1},
	{".*",		PT_CPP_2},
	{"*",		PT_MUL},
	{"/",		PT_DIV},
	{"%",		PT_MOD},
	{"+",		PT_ADD},
	{"-",		PT_SUB},
	{"=",		PT_ASSIGN},
	{"&",		PT_BINARY_AND},
	{"|",		PT_BINARY_OR},
	{"^",		PT_BINARY_XOR},
	{"~",		PT_BINARY_NOT},
	{"!",		PT_LOGIC_NOT},
	{">",		PT_LOGIC_GREATER},
	{"<",		PT_LOGIC_LESS},
	{".",		PT_REFERENCE},
	{":",		PT_COLON},
	{",",		PT_COMMA},
	{";",		PT_SEMICOLON},
	{"?",		PT_QUESTION_MARK},
	{"{",		PT_BRACE_OPEN},
	{"}",		PT_BRACE_CLOSE},
	{"[",		PT_BRACKET_OPEN},
	{"]",		PT_BRACKET_CLOSE},
	{"(",		PT_PARENTHESIS_OPEN},
	{")",		PT_PARENTHESIS_CLOSE},
	{"#",		PT_PRECOMPILER},
	{"$",		PT_DOLLAR},
	{"\\",		PT_BACKSLASH},
	{NULL,		0}
};


/*
 =================
 PS_NumberValue
 =================
*/
static void PS_NumberValue (token_t *token){

	char		*string = token->string;
	double		fraction = 0.1, power = 1.0;
	qboolean	negative = false;
	int			i, exponent = 0;

	token->floatValue = 0.0;
	token->integerValue = 0;

	if (token->type != TT_NUMBER)
		return;

	// If a decimal number
	if (token->subType & NT_DECIMAL){
		// If a floating point number
		if (token->subType & NT_FLOAT){
			while (*string && *string != '.' && *string != 'e' && *string != 'E')
				token->floatValue = token->floatValue * 10.0 + (double)(*string++ - '0');

			if (*string == '.'){
				string++;
				while (*string && *string != 'e' && *string != 'E'){
					token->floatValue = token->floatValue + (double)(*string++ - '0') * fraction;
					fraction *= 0.1;
				}
			}

			if (*string == 'e' || *string == 'E'){
				string++;

				if (*string == '+'){
					string++;

					negative = false;
				}
				else if (*string == '-'){
					string++;

					negative = true;
				}

				while (*string)
					exponent = exponent * 10 + (*string++ - '0');

				for (i = 0; i < exponent; i++)
					power *= 10.0;

				if (negative)
					token->floatValue /= power;
				else
					token->floatValue *= power;
			}

			token->integerValue = (unsigned)token->floatValue;
			return;
		}

		// If an integer number
		if (token->subType & NT_INTEGER){
			while (*string)
				token->integerValue = token->integerValue * 10 + (*string++ - '0');

			token->floatValue = (double)token->integerValue;
			return;
		}

		return;
	}

	// If a binary number
	if (token->subType & NT_BINARY){
		string += 2;
		while (*string)
			token->integerValue = (token->integerValue << 1) + (*string++ - '0');

		token->floatValue = (double)token->integerValue;
		return;
	}

	// If an octal number
	if (token->subType & NT_OCTAL){
		string += 1;
		while (*string)
			token->integerValue = (token->integerValue << 3) + (*string++ - '0');

		token->floatValue = (double)token->integerValue;
		return;
	}

	// If a hexadecimal number
	if (token->subType & NT_HEXADECIMAL){
		string += 2;
		while (*string){
			if (*string >= 'a' && *string <= 'f')
				token->integerValue = (token->integerValue << 4) + (*string++ - 'a' + 10);
			else if (*string >= 'A' && *string <= 'F')
				token->integerValue = (token->integerValue << 4) + (*string++ - 'A' + 10);
			else
				token->integerValue = (token->integerValue << 4) + (*string++ - '0');
		}

		token->floatValue = (double)token->integerValue;
		return;
	}
}

/*
 =================
 PS_ReadWhiteSpace
 =================
*/
static qboolean PS_ReadWhiteSpace (script_t *script, unsigned flags){

	char		*text;
	int			line;
	qboolean	hasNewLines = false;

	// Backup text and line
	text = script->text;
	line = script->line;

	while (1){
		// Skip whitespace
		while (*script->text <= ' '){
			if (!*script->text){
				script->text = NULL;
				return false;
			}

			if (*script->text == '\n'){
				script->line++;

				hasNewLines = true;
			}

			script->text++;
		}

		// If newlines are not allowed, restore text and line
		if (hasNewLines && !(flags & PSF_ALLOW_NEWLINES)){
			script->text = text;
			script->line = line;

			return false;
		}

		// Skip // comments
		if (*script->text == '/' && script->text[1] == '/'){
			while (*script->text && *script->text != '\n')
				script->text++;

			continue;
		}

		// Skip /* */ comments
		if (*script->text == '/' && script->text[1] == '*'){
			script->text += 2;

			while (*script->text && (*script->text != '*' || script->text[1] != '/')){
				if (*script->text == '\n')
					script->line++;

				script->text++;
			}

			if (*script->text)
				script->text += 2;

			continue;
		}

		// An actual token
		break;
	}

	return true;
}

/*
 =================
 PS_ReadEscapeChar
 =================
*/
static qboolean PS_ReadEscapeChar (script_t *script, unsigned flags, char *ch){

	int		value;

	script->text++;

	switch (*script->text){
	case 'a':
		*ch = '\a';
		break;
	case 'b':
		*ch = '\b';
		break;
	case 'f':
		*ch = '\f';
		break;
	case 'n':
		*ch = '\n';
		break;
	case 'r':
		*ch = '\r';
		break;
	case 't':
		*ch = '\t';
		break;
	case 'v':
		*ch = '\v';
		break;
	case '\"':
		*ch = '\"';
		break;
	case '\'':
		*ch = '\'';
		break;
	case '\\':
		*ch = '\\';
		break;
	case '\?':
		*ch = '\?';
		break;
	case 'x':
		script->text++;

		for (value = 0; ; script->text++){
			if (*script->text >= 'a' && *script->text <= 'f')
				value = (value << 4) + (*script->text - 'a' + 10);
			else if (*script->text >= 'A' && *script->text <= 'F')
				value = (value << 4) + (*script->text - 'A' + 10);
			else if (*script->text >= '0' && *script->text <= '9')
				value = (value << 4) + (*script->text - '0');
			else
				break;
		}

		script->text--;

		if (value > 0xFF){
			PS_ScriptError(script, flags, "too large value in escape character");
			return false;
		}

		*ch = value;
		break;
	default:
		if (*script->text < '0' || *script->text > '9'){
			PS_ScriptError(script, flags, "unknown escape character");
			return false;
		}

		for (value = 0; ; script->text++){
			if (*script->text >= '0' && *script->text <= '9')
				value = value * 10 + (*script->text - '0');
			else
				break;
		}

		script->text--;

		if (value > 0xFF){
			PS_ScriptError(script, flags, "too large value in escape character");
			return false;
		}

		*ch = value;
		break;
	}

	script->text++;

	return true;
}

/*
 =================
 PS_ReadGeneric
 =================
*/
static qboolean PS_ReadGeneric (script_t *script, unsigned flags, token_t *token){

	token->type = TT_GENERIC;
	token->subType = 0;

	token->line = script->line;

	while (1){
		if (*script->text <= ' ')
			break;

		if (token->length == MAX_TOKEN_LENGTH - 1){
			PS_ScriptError(script, flags, "string longer than MAX_TOKEN_LENGTH");
			return false;
		}
		token->string[token->length++] = *script->text++;
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 =================
 PS_ReadString
 =================
*/
static qboolean PS_ReadString (script_t *script, unsigned flags, token_t *token){

	char	*text;
	int		line;

	token->type = TT_STRING;
	token->subType = 0;

	token->line = script->line;

	script->text++;

	while (1){
		if (!*script->text){
			PS_ScriptError(script, flags, "missing trailing quote");
			return false;
		}

		if (*script->text == '\n'){
			PS_ScriptError(script, flags, "newline inside string");
			return false;
		}

		if (*script->text == '\"'){
			script->text++;

			if (!(flags & PSF_ALLOW_STRINGCONCAT))
				break;

			text = script->text;
			line = script->line;

			if (PS_ReadWhiteSpace(script, flags)){
				if (*script->text == '\"'){
					script->text++;
					continue;
				}
			}

			script->text = text;
			script->line = line;

			break;
		}

		if (token->length == MAX_TOKEN_LENGTH - 1){
			PS_ScriptError(script, flags, "string longer than MAX_TOKEN_LENGTH");
			return false;
		}

		if ((flags & PSF_ALLOW_ESCAPECHARS) && *script->text == '\\'){
			if (!PS_ReadEscapeChar(script, flags, &token->string[token->length]))
				return false;

			token->length++;
			continue;
		}

		token->string[token->length++] = *script->text++;
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 =================
 PS_ReadLiteral
 =================
*/
static qboolean PS_ReadLiteral (script_t *script, unsigned flags, token_t *token){

	char	*text;
	int		line;

	token->type = TT_LITERAL;
	token->subType = 0;

	token->line = script->line;

	script->text++;

	while (1){
		if (!*script->text){
			PS_ScriptError(script, flags, "missing trailing quote");
			return false;
		}

		if (*script->text == '\n'){
			PS_ScriptError(script, flags, "newline inside literal");
			return false;
		}

		if (*script->text == '\''){
			script->text++;

			if (!(flags & PSF_ALLOW_STRINGCONCAT))
				break;

			text = script->text;
			line = script->line;

			if (PS_ReadWhiteSpace(script, flags)){
				if (*script->text == '\''){
					script->text++;
					continue;
				}
			}

			script->text = text;
			script->line = line;

			break;
		}

		if (token->length == MAX_TOKEN_LENGTH - 1){
			PS_ScriptError(script, flags, "literal longer than MAX_TOKEN_LENGTH");
			return false;
		}

		if ((flags & PSF_ALLOW_ESCAPECHARS) && *script->text == '\\'){
			if (!PS_ReadEscapeChar(script, flags, &token->string[token->length]))
				return false;

			token->length++;
			continue;
		}

		token->string[token->length++] = *script->text++;
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 =================
 PS_ReadNumber
 =================
*/
static qboolean PS_ReadNumber (script_t *script, unsigned flags, token_t *token){

	qboolean	hasDot = false;
	int			c;

	token->type = TT_NUMBER;
	token->subType = 0;

	token->line = script->line;

	if (*script->text == '0' && script->text[1] != '.'){
		if (script->text[1] == 'b' || script->text[1] == 'B'){
			token->string[token->length++] = *script->text++;
			token->string[token->length++] = *script->text++;

			while (1){
				c = *script->text;

				if (c < '0' || c > '1')
					break;

				if (token->length == MAX_TOKEN_LENGTH - 1){
					PS_ScriptError(script, flags, "binary number longer than MAX_TOKEN_LENGTH");
					return false;
				}
				token->string[token->length++] = *script->text++;
			}

			token->subType |= (NT_BINARY | NT_INTEGER);
		}
		else if (script->text[1] == 'x' || script->text[1] == 'X'){
			token->string[token->length++] = *script->text++;
			token->string[token->length++] = *script->text++;

			while (1){
				c = *script->text;

				if ((c < 'a' || c > 'f') && (c < 'A' || c > 'F') && (c < '0' || c > '9'))
					break;

				if (token->length == MAX_TOKEN_LENGTH - 1){
					PS_ScriptError(script, flags, "hexadecimal number longer than MAX_TOKEN_LENGTH");
					return false;
				}
				token->string[token->length++] = *script->text++;
			}

			token->subType |= (NT_HEXADECIMAL | NT_INTEGER);
		}
		else {
			token->string[token->length++] = *script->text++;

			while (1){
				c = *script->text;

				if (c < '0' || c > '7')
					break;

				if (token->length == MAX_TOKEN_LENGTH - 1){
					PS_ScriptError(script, flags, "octal number longer than MAX_TOKEN_LENGTH");
					return false;
				}
				token->string[token->length++] = *script->text++;
			}

			token->subType |= (NT_OCTAL | NT_INTEGER);
		}

		token->string[token->length] = 0;

		PS_NumberValue(token);

		return true;
	}

	while (1){
		c = *script->text;

		if (c == '.'){
			if (hasDot)
				break;

			hasDot = true;
		}
		else if (c < '0' || c > '9')
			break;

		if (token->length == MAX_TOKEN_LENGTH - 1){
			PS_ScriptError(script, flags, "number longer than MAX_TOKEN_LENGTH");
			return false;
		}
		token->string[token->length++] = *script->text++;
	}

	if (hasDot || (*script->text == 'e' || *script->text == 'E')){
		token->subType |= (NT_DECIMAL | NT_FLOAT);

		if (*script->text == 'e' || *script->text == 'E'){
			if (token->length == MAX_TOKEN_LENGTH - 1){
				PS_ScriptError(script, flags, "number long than MAX_TOKEN_LENGTH");
				return false;
			}
			token->string[token->length++] = *script->text++;

			if (*script->text == '+' || *script->text == '-'){
				if (token->length == MAX_TOKEN_LENGTH - 1){
					PS_ScriptError(script, flags, "number longer than MAX_TOKEN_LENGTH");
					return false;
				}
				token->string[token->length++] = *script->text++;
			}

			while (1){
				c = *script->text;

				if (c < '0' || c > '9')
					break;

				if (token->length == MAX_TOKEN_LENGTH - 1){
					PS_ScriptError(script, flags, "number longer than MAX_TOKEN_LENGTH");
					return false;
				}
				token->string[token->length++] = *script->text++;
			}
		}

		if (*script->text == 'f' || *script->text == 'F'){
			script->text++;

			token->subType |= NT_SINGLE;
		}
		else if (*script->text == 'l' || *script->text == 'L'){
			script->text++;

			token->subType |= NT_EXTENDED;
		}
		else
			token->subType |= NT_DOUBLE;
	}
	else {
		token->subType |= (NT_DECIMAL | NT_INTEGER);

		if (*script->text == 'u' || *script->text == 'U'){
			script->text++;

			token->subType |= NT_UNSIGNED;

			if (*script->text == 'l' || *script->text == 'L'){
				script->text++;

				token->subType |= NT_LONG;
			}
		}
		else if (*script->text == 'l' || *script->text == 'L'){
			script->text++;

			token->subType |= NT_LONG;

			if (*script->text == 'u' || *script->text == 'U'){
				script->text++;

				token->subType |= NT_UNSIGNED;
			}
		}
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 =================
 PS_ReadName
 =================
*/
static qboolean PS_ReadName (script_t *script, unsigned flags, token_t *token){

	int		c;

	token->type = TT_NAME;
	token->subType = 0;

	token->line = script->line;

	while (1){
		c = *script->text;

		if (flags & PSF_ALLOW_PATHNAMES){
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_' && c != '/' && c != '\\' && c != ':' && c != '.' && c != '+' && c != '-')
				break;
		}
		else {
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_')
				break;
		}

		if (token->length == MAX_TOKEN_LENGTH - 1){
			PS_ScriptError(script, flags, "name longer than MAX_TOKEN_LENGTH");
			return false;
		}
		token->string[token->length++] = *script->text++;
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 =================
 PS_ReadPunctuation
 =================
*/
static qboolean PS_ReadPunctuation (script_t *script, unsigned flags, token_t *token){

	punctuation_t	*punctuation;
	int				i, len;

	for (i = 0; script->punctuations[i].name; i++){
		punctuation = &script->punctuations[i];

		for (len = 0; punctuation->name[len] && script->text[len]; len++){
			if (punctuation->name[len] != script->text[len])
				break;
		}

		if (!punctuation->name[len]){
			script->text += len;

			token->type = TT_PUNCTUATION;
			token->subType = punctuation->type;

			token->line = script->line;

			for (i = 0; i < len; i++){
				if (token->length == MAX_TOKEN_LENGTH - 1){
					PS_ScriptError(script, flags, "punctuation longer than MAX_TOKEN_LENGTH");
					return false;
				}
				token->string[token->length++] = punctuation->name[i];
			}

			token->string[token->length] = 0;

			PS_NumberValue(token);

			return true;
		}
	}

	return false;
}

/*
 =================
 PS_ReadToken

 Reads a token from the script
 =================
*/
qboolean PS_ReadToken (script_t *script, unsigned flags, token_t *token){

	// If there is a token available (from PS_UnreadToken)
	if (script->tokenAvailable){
		script->tokenAvailable = false;

		memcpy(token, &script->token, sizeof(token_t));
		return true;
	}

	// Clear token
	token->type = TT_EMPTY;
	token->subType = 0;
	token->line = 1;
	token->string[0] = 0;
	token->length = 0;
	token->floatValue = 0.0;
	token->integerValue = 0;

	// Make sure incoming data is valid
	if (!script->text)
		return false;

	// Skip whitespace and comments
	if (!PS_ReadWhiteSpace(script, flags))
		return false;

	// If we just want to parse a generic string separated by spaces
	if (flags & PSF_PARSE_GENERIC){
		// If it is a string
		if (*script->text == '\"'){
			if (PS_ReadString(script, flags, token))
				return true;
		}
		// If it is a literal
		else if (*script->text == '\''){
			if (PS_ReadLiteral(script, flags, token))
				return true;
		}
		// Check for a generic string
		else if (PS_ReadGeneric(script, flags, token))
			return true;
	}
	// If it is a string
	else if (*script->text == '\"'){
		if (PS_ReadString(script, flags, token))
			return true;
	}
	// If it is a literal
	else if (*script->text == '\''){
		if (PS_ReadLiteral(script, flags, token))
			return true;
	}
	// If it is a number
	else if ((*script->text >= '0' && *script->text <= '9') || (*script->text == '.' && (script->text[1] >= '0' && script->text[1] <= '9'))){
		if (PS_ReadNumber(script, flags, token))
			return true;
	}
	// If it is a name
	else if ((*script->text >= 'a' && *script->text <= 'z') || (*script->text >= 'A' && *script->text <= 'Z') || *script->text == '_'){
		if (PS_ReadName(script, flags, token))
			return true;
	}
	// Check for a path name if needed
	else if ((flags & PSF_ALLOW_PATHNAMES) && (*script->text == '/' || *script->text == '\\' || *script->text == ':' || *script->text == '.')){
		if (PS_ReadName(script, flags, token))
			return true;
	}
	// Check for a punctuation
	else if (PS_ReadPunctuation(script, flags, token))
		return true;

	// Couldn't parse a token
	token->type = TT_EMPTY;
	token->subType = 0;
	token->line = 1;
	token->string[0] = 0;
	token->length = 0;
	token->floatValue = 0.0;
	token->integerValue = 0;

	PS_ScriptError(script, flags, "couldn't read token");

	return false;
}

/*
 =================
 PS_UnreadToken
 =================
*/
void PS_UnreadToken (script_t *script, token_t *token){

	script->tokenAvailable = true;

	memcpy(&script->token, token, sizeof(token_t));
}

/*
 =================
 PS_ReadDouble
 =================
*/
qboolean PS_ReadDouble (script_t *script, unsigned flags, double *value){

	token_t	token;

	if (!PS_ReadToken(script, flags, &token))
		return false;

	if (token.type == TT_PUNCTUATION && !Q_stricmp(token.string, "-")){
		if (!PS_ReadToken(script, flags, &token))
			return false;

		if (token.type != TT_NUMBER){
			PS_ScriptError(script, flags, "expected float value, found '%s'", token.string);
			return false;
		}

		*value = -token.floatValue;
		return true;
	}

	if (token.type != TT_NUMBER){
		PS_ScriptError(script, flags, "expected float value, found '%s'", token.string);
		return false;
	}

	*value = token.floatValue;
	return true;
}

/*
 =================
 PS_ReadFloat
 =================
*/
qboolean PS_ReadFloat (script_t *script, unsigned flags, float *value){

	token_t	token;

	if (!PS_ReadToken(script, flags, &token))
		return false;

	if (token.type == TT_PUNCTUATION && !Q_stricmp(token.string, "-")){
		if (!PS_ReadToken(script, flags, &token))
			return false;

		if (token.type != TT_NUMBER){
			PS_ScriptError(script, flags, "expected float value, found '%s'", token.string);
			return false;
		}

		*value = -((float)token.floatValue);
		return true;
	}

	if (token.type != TT_NUMBER){
		PS_ScriptError(script, flags, "expected float value, found '%s'", token.string);
		return false;
	}

	*value = (float)token.floatValue;
	return true;
}

/*
 =================
 PS_ReadUnsigned
 =================
*/
qboolean PS_ReadUnsigned (script_t *script, unsigned flags, unsigned *value){

	token_t	token;

	if (!PS_ReadToken(script, flags, &token))
		return false;

	if (token.type != TT_NUMBER || !(token.subType & NT_INTEGER)){
		PS_ScriptError(script, flags, "expected integer value, found '%s'", token.string);
		return false;
	}

	*value = token.integerValue;
	return true;
}

/*
 =================
 PS_ReadInteger
 =================
*/
qboolean PS_ReadInteger (script_t *script, unsigned flags, int *value){

	token_t	token;

	if (!PS_ReadToken(script, flags, &token))
		return false;

	if (token.type == TT_PUNCTUATION && !Q_stricmp(token.string, "-")){
		if (!PS_ReadToken(script, flags, &token))
			return false;

		if (token.type != TT_NUMBER || !(token.subType & NT_INTEGER)){
			PS_ScriptError(script, flags, "expected integer value, found '%s'", token.string);
			return false;
		}

		*value = -((int)token.integerValue);
		return true;
	}

	if (token.type != TT_NUMBER || !(token.subType & NT_INTEGER)){
		PS_ScriptError(script, flags, "expected integer value, found '%s'", token.string);
		return false;
	}

	*value = (int)token.integerValue;
	return true;
}

/*
 =================
 PS_SkipWhiteSpace

 Skips until a printable character is found
 =================
*/
void PS_SkipWhiteSpace (script_t *script){

	// Make sure incoming data is valid
	if (!script->text)
		return;

	while (1){
		// Skip whitespace
		while (*script->text <= ' '){
			if (!*script->text){
				script->text = NULL;
				return;
			}

			if (*script->text == '\n')
				script->line++;

			script->text++;
		}

		// Skip // comments
		if (*script->text == '/' && script->text[1] == '/'){
			while (*script->text && *script->text != '\n')
				script->text++;

			continue;
		}

		// Skip /* */ comments
		if (*script->text == '/' && script->text[1] == '*'){
			script->text += 2;

			while (*script->text && (*script->text != '*' || script->text[1] != '/')){
				if (*script->text == '\n')
					script->line++;

				script->text++;
			}

			if (*script->text)
				script->text += 2;

			continue;
		}

		// An actual token
		break;
	}
}

/*
 =================
 PS_SkipRestOfLine

 Skips until a new line is found
 =================
*/
void PS_SkipRestOfLine (script_t *script){

	token_t	token;

	while (1){
		if (!PS_ReadToken(script, 0, &token))
			break;
	}
}

/*
 =================
 PS_SkipBracedSection

 Skips until a matching close brace is found.
 Internal brace depths are properly skipped.
 =================
*/
void PS_SkipBracedSection (script_t *script, int depth){

	token_t	token;

	do {
		if (!PS_ReadToken(script, PSF_ALLOW_NEWLINES, &token))
			break;

		if (token.type == TT_PUNCTUATION){
			if (!Q_stricmp(token.string, "{"))
				depth++;
			else if (!Q_stricmp(token.string, "}"))
				depth--;
		}
	} while (depth);
}

/*
 =================
 PS_ScriptError
 =================
*/
void PS_ScriptError (script_t *script, unsigned flags, const char *fmt, ...){

	char	string[MAX_PRINTMSG];
	va_list	argPtr;

	if (!(flags & PSF_PRINT_ERRORS))
		return;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), MAX_PRINTMSG, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	Com_Printf(S_COLOR_RED "ERROR: source '%s', line %i: %s\n", script->name, script->line, string);
}

/*
 =================
 PS_ScriptWarning
 =================
*/
void PS_ScriptWarning (script_t *script, unsigned flags, const char *fmt, ...){

	char	string[MAX_PRINTMSG];
	va_list	argPtr;

	if (!(flags & PSF_PRINT_WARNINGS))
		return;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), MAX_PRINTMSG, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	Com_Printf(S_COLOR_YELLOW "WARNING: source '%s', line %i: %s\n", script->name, script->line, string);
}

/*
 =================
 PS_SetPunctuationsTable
 =================
*/
void PS_SetPunctuationsTable (script_t *script, punctuation_t *punctuationsTable){

	if (punctuationsTable)
		script->punctuations = punctuationsTable;
	else
		script->punctuations = ps_punctuationsTable;
}

/*
 =================
 PS_ResetScript
 =================
*/
void PS_ResetScript (script_t *script){

	script->text = script->buffer;
	script->line = 1;

	script->tokenAvailable = false;

	script->token.type = TT_EMPTY;
	script->token.subType = 0;
	script->token.line = 1;
	script->token.string[0] = 0;
	script->token.length = 0;
	script->token.floatValue = 0.0;
	script->token.integerValue = 0;
}

/*
 =================
 PS_EndOfScript
 =================
*/
qboolean PS_EndOfScript (script_t *script){

	if (!script->text)
		return true;

	return false;
}

/*
 =================
 PS_LoadScriptFile
 =================
*/
script_t *PS_LoadScriptFile (const char *name){

	script_t		*script;
	fileHandle_t	f;
	char			*buffer;
	int				size;

	size = FS_OpenFile(name, &f, FS_READ);
	if (!f)
		return NULL;	// Let the caller handle this error

	buffer = Z_Malloc(size + 1);

	FS_Read(buffer, size, f);
	FS_CloseFile(f);

	buffer[size] = 0;

	// Allocate the script_t
	script = Z_Malloc(sizeof(script_t));

	Q_strncpyz(script->name, name, sizeof(script->name));

	script->buffer = buffer;
	script->size = size;
	script->allocated = true;

	script->text = buffer;
	script->line = 1;

	script->punctuations = ps_punctuationsTable;

	script->tokenAvailable = false;

	script->token.type = TT_EMPTY;
	script->token.subType = 0;
	script->token.line = 1;
	script->token.string[0] = 0;
	script->token.length = 0;
	script->token.floatValue = 0.0;
	script->token.integerValue = 0;

	return script;
}

/*
 =================
 PS_LoadScriptMemory
 =================
*/
script_t *PS_LoadScriptMemory (const char *name, const char *buffer, int size){

	script_t	*script;

	if (!buffer || size < 0)
		return NULL;	// Let the caller handle this error

	// Allocate the script_t
	script = Z_Malloc(sizeof(script_t));

	Q_strncpyz(script->name, name, sizeof(script->name));

	script->buffer = (char *)buffer;
	script->size = size;
	script->allocated = false;

	script->text = (char *)buffer;
	script->line = 1;

	script->punctuations = ps_punctuationsTable;

	script->tokenAvailable = false;

	script->token.type = TT_EMPTY;
	script->token.subType = 0;
	script->token.line = 1;
	script->token.string[0] = 0;
	script->token.length = 0;
	script->token.floatValue = 0.0;
	script->token.integerValue = 0;

	return script;
}

/*
 =================
 PS_FreeScript
 =================
*/
void PS_FreeScript (script_t *script){

	if (!script)
		Com_Error(ERR_FATAL, "PS_FreeScript: NULL script");

	if (script->allocated)
		Z_Free(script->buffer);

	Z_Free(script);
}
