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


// msg.c -- message I/O functions


#include "qcommon.h"


/*
 =================
 MSG_Init
 =================
*/
void MSG_Init (msg_t *msg, byte *data, int maxSize, qboolean allowOverflow){

	msg->allowOverflow = allowOverflow;
	msg->overflowed = false;
	msg->data = data;
	msg->maxSize = maxSize;
	msg->curSize = 0;
	msg->readCount = 0;
}

/*
 =================
 MSG_Clear
 =================
*/
void MSG_Clear (msg_t *msg){

	msg->overflowed = false;
	msg->curSize = 0;
}

/*
 =================
 MSG_GetSpace
 =================
*/
byte *MSG_GetSpace (msg_t *msg, int length){

	byte	*data;

	if (msg->curSize + length > msg->maxSize){
		if (!msg->allowOverflow)
			Com_Error(ERR_FATAL, "MSG_GetSpace: overflow without allowOverflow set");

		if (length > msg->maxSize)
			Com_Error(ERR_FATAL, "MSG_GetSpace: %i is > full buffer size", length);

		Com_Printf("MSG_GetSpace: overflow\n");

		msg->overflowed = true;
		msg->curSize = 0;
	}

	data = msg->data + msg->curSize;
	msg->curSize += length;

	return data;
}

/*
 =================
 MSG_Write
 =================
*/
void MSG_Write (msg_t *msg, const void *data, int length){

	memcpy(MSG_GetSpace(msg, length), data, length);
}

/*
 =================
 MSG_Print
 =================
*/
void MSG_Print (msg_t *msg, const char *data){

	int		length;

	length = strlen(data) + 1;

	if (msg->curSize){
		if (msg->data[msg->curSize-1])	// No trailing 0
			memcpy(MSG_GetSpace(msg, length), data, length);
		else							// Write over trailing 0
			memcpy(MSG_GetSpace(msg, length-1)-1, data, length);
	}
	else
		memcpy(MSG_GetSpace(msg, length), data, length);
}


// =====================================================================


/*
 =================
 MSG_WriteChar
 =================
*/
void MSG_WriteChar (msg_t *msg, int c){

	byte	*buf;
	
	buf = MSG_GetSpace(msg, 1);
	buf[0] = c;
}

/*
 =================
 MSG_WriteByte
 =================
*/
void MSG_WriteByte (msg_t *msg, int b){

	byte	*buf;
	
	buf = MSG_GetSpace(msg, 1);
	buf[0] = b;
}

/*
 =================
 MSG_WriteShort
 =================
*/
void MSG_WriteShort (msg_t *msg, int s){

	byte	*buf;
	
	buf = MSG_GetSpace(msg, 2);
	buf[0] = s & 255;
	buf[1] = (s >> 8);
}

/*
 =================
 MSG_WriteLong
 =================
*/
void MSG_WriteLong (msg_t *msg, int l){

	byte	*buf;
	
	buf = MSG_GetSpace(msg, 4);
	buf[0] = l & 255;
	buf[1] = (l >> 8) & 255;
	buf[2] = (l >> 16) & 255;
	buf[3] = (l >> 24);
}

/*
 =================
 MSG_WriteFloat
 =================
*/
void MSG_WriteFloat (msg_t *msg, float f){

	union {
		byte	b[4];
		float	f;
	} dat;
	byte	*buf;
	
	dat.f = LittleFloat(f);
	
	buf = MSG_GetSpace(msg, 4);
	buf[0] = dat.b[0];
	buf[1] = dat.b[1];
	buf[2] = dat.b[2];
	buf[3] = dat.b[3];
}

/*
 =================
 MSG_WriteString
 =================
*/
void MSG_WriteString (msg_t *msg, const char *s){

	int		i, l;
	byte	*buf;

	if (s)
		l = strlen(s);
	else
		l = 0;

	buf = MSG_GetSpace(msg, l+1);
	for (i = 0; i < l; i++)
		buf[i] = s[i];
	buf[i] = 0;
}

/*
 =================
 MSG_WriteCoord
 =================
*/
void MSG_WriteCoord (msg_t *msg, float c){

	MSG_WriteShort(msg, (int)(c * 8));
}

/*
 =================
 MSG_WritePos
 =================
*/
void MSG_WritePos (msg_t *msg, const vec3_t pos){

	MSG_WriteShort(msg, (int)(pos[0] * 8));
	MSG_WriteShort(msg, (int)(pos[1] * 8));
	MSG_WriteShort(msg, (int)(pos[2] * 8));
}

/*
 =================
 MSG_WriteAngle
 =================
*/
void MSG_WriteAngle (msg_t *msg, float a){

	MSG_WriteByte(msg, (int)(a * 256.0/360) & 255);
}

/*
 =================
 MSG_WriteAngle16
 =================
*/
void MSG_WriteAngle16 (msg_t *msg, float a){

	MSG_WriteShort(msg, ANGLE2SHORT(a));
}

/*
 =================
 MSG_WriteDir
 =================
*/
void MSG_WriteDir (msg_t *msg, const vec3_t dir){

	int		best;

	best = DirToByte(dir);
	MSG_WriteByte(msg, best);
}

/*
 =================
 MSG_WriteDeltaUserCmd
 =================
*/
void MSG_WriteDeltaUserCmd (msg_t *msg, const usercmd_t *from, const usercmd_t *to){

	int		bits;

	// Send the movement message
	bits = 0;

	if (to->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (to->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (to->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;
	if (to->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (to->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (to->upmove != from->upmove)
		bits |= CM_UP;
	if (to->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (to->impulse != from->impulse)
		bits |= CM_IMPULSE;

    MSG_WriteByte(msg, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteShort(msg, to->angles[0]);
	if (bits & CM_ANGLE2)
		MSG_WriteShort(msg, to->angles[1]);
	if (bits & CM_ANGLE3)
		MSG_WriteShort(msg, to->angles[2]);
	
	if (bits & CM_FORWARD)
		MSG_WriteShort(msg, to->forwardmove);
	if (bits & CM_SIDE)
	  	MSG_WriteShort(msg, to->sidemove);
	if (bits & CM_UP)
		MSG_WriteShort(msg, to->upmove);

 	if (bits & CM_BUTTONS)
	  	MSG_WriteByte(msg, to->buttons);
 	if (bits & CM_IMPULSE)
	    MSG_WriteByte(msg, to->impulse);

    MSG_WriteByte(msg, to->msec);
	MSG_WriteByte(msg, to->lightlevel);
}

/*
 =================
 MSG_WriteDeltaEntity

 Writes part of a packet entities message.
 Can delta from either a baseline or a previous packet entity.
 =================
*/
void MSG_WriteDeltaEntity (msg_t *msg, const entity_state_t *from, const entity_state_t *to, qboolean force, qboolean newEntity){

	int		bits;

	if (!to->number)
		Com_Error(ERR_DROP, "MSG_WriteDeltaEntity: unset entity number");
	if (to->number >= MAX_EDICTS)
		Com_Error(ERR_DROP, "MSG_WriteDeltaEntity: number >= MAX_EDICTS");

	// Send an update
	bits = 0;

	if (to->number >= 256)
		bits |= U_NUMBER16;		// U_NUMBER8 is implicit otherwise

	if (to->origin[0] != from->origin[0])
		bits |= U_ORIGIN1;
	if (to->origin[1] != from->origin[1])
		bits |= U_ORIGIN2;
	if (to->origin[2] != from->origin[2])
		bits |= U_ORIGIN3;

	if (to->angles[0] != from->angles[0])
		bits |= U_ANGLE1;		
	if (to->angles[1] != from->angles[1])
		bits |= U_ANGLE2;
	if (to->angles[2] != from->angles[2])
		bits |= U_ANGLE3;

	if (to->skinnum != from->skinnum){
		if ((unsigned)to->skinnum < 0x100)
			bits |= U_SKIN8;
		else if ((unsigned)to->skinnum < 0x10000)
			bits |= U_SKIN16;
		else
			bits |= (U_SKIN8|U_SKIN16);
	}

	if (to->frame != from->frame){
		if (to->frame < 256)
			bits |= U_FRAME8;
		else
			bits |= U_FRAME16;
	}

	if (to->effects != from->effects){
		if (to->effects < 0x100)
			bits |= U_EFFECTS8;
		else if (to->effects < 0x8000)
			bits |= U_EFFECTS16;
		else
			bits |= U_EFFECTS8|U_EFFECTS16;
	}

	if (to->renderfx != from->renderfx){
		if (to->renderfx < 0x100)
			bits |= U_RENDERFX8;
		else if (to->renderfx < 0x8000)
			bits |= U_RENDERFX16;
		else
			bits |= U_RENDERFX8|U_RENDERFX16;
	}

	if (to->solid != from->solid)
		bits |= U_SOLID;

	// Event is not delta compressed, just 0 compressed
	if (to->event)
		bits |= U_EVENT;

	if (to->modelindex != from->modelindex)
		bits |= U_MODEL;
	if (to->modelindex2 != from->modelindex2)
		bits |= U_MODEL2;
	if (to->modelindex3 != from->modelindex3)
		bits |= U_MODEL3;
	if (to->modelindex4 != from->modelindex4)
		bits |= U_MODEL4;

	if (to->sound != from->sound)
		bits |= U_SOUND;

	if (newEntity || (to->renderfx & RF_BEAM))
		bits |= U_OLDORIGIN;

	// Write the message
	if (!bits && !force)
		return;		// Nothing to send!

	if (bits & 0xFF000000)
		bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x00FF0000)
		bits |= U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x0000FF00)
		bits |= U_MOREBITS1;

	MSG_WriteByte(msg, bits & 255);

	if (bits & 0xFF000000){
		MSG_WriteByte(msg, (bits >> 8) & 255);
		MSG_WriteByte(msg, (bits >> 16) & 255);
		MSG_WriteByte(msg, (bits >> 24) & 255);
	}
	else if (bits & 0x00FF0000){
		MSG_WriteByte(msg, (bits >> 8) & 255);
		MSG_WriteByte(msg, (bits >> 16) & 255);
	}
	else if (bits & 0x0000FF00)
		MSG_WriteByte(msg, (bits >> 8) & 255);

	if (bits & U_NUMBER16)
		MSG_WriteShort(msg, to->number);
	else
		MSG_WriteByte(msg, to->number);

	if (bits & U_MODEL)
		MSG_WriteByte(msg, to->modelindex);
	if (bits & U_MODEL2)
		MSG_WriteByte(msg, to->modelindex2);
	if (bits & U_MODEL3)
		MSG_WriteByte(msg, to->modelindex3);
	if (bits & U_MODEL4)
		MSG_WriteByte(msg, to->modelindex4);

	if (bits & U_FRAME8)
		MSG_WriteByte(msg, to->frame);
	if (bits & U_FRAME16)
		MSG_WriteShort(msg, to->frame);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))	// Used for laser colors
		MSG_WriteLong(msg, to->skinnum);
	else if (bits & U_SKIN8)
		MSG_WriteByte(msg, to->skinnum);
	else if (bits & U_SKIN16)
		MSG_WriteShort(msg, to->skinnum);

	if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))
		MSG_WriteLong(msg, to->effects);
	else if (bits & U_EFFECTS8)
		MSG_WriteByte(msg, to->effects);
	else if (bits & U_EFFECTS16)
		MSG_WriteShort(msg, to->effects);

	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
		MSG_WriteLong(msg, to->renderfx);
	else if (bits & U_RENDERFX8)
		MSG_WriteByte(msg, to->renderfx);
	else if (bits & U_RENDERFX16)
		MSG_WriteShort(msg, to->renderfx);

	if (bits & U_ORIGIN1)
		MSG_WriteCoord(msg, to->origin[0]);		
	if (bits & U_ORIGIN2)
		MSG_WriteCoord(msg, to->origin[1]);
	if (bits & U_ORIGIN3)
		MSG_WriteCoord(msg, to->origin[2]);

	if (bits & U_ANGLE1)
		MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle(msg, to->angles[2]);

	if (bits & U_OLDORIGIN){
		MSG_WriteCoord(msg, to->old_origin[0]);
		MSG_WriteCoord(msg, to->old_origin[1]);
		MSG_WriteCoord(msg, to->old_origin[2]);
	}

	if (bits & U_SOUND)
		MSG_WriteByte(msg, to->sound);
	if (bits & U_EVENT)
		MSG_WriteByte(msg, to->event);
	if (bits & U_SOLID)
		MSG_WriteShort(msg, to->solid);
}


// =====================================================================


/*
 =================
 MSG_BeginReading
 =================
*/
void MSG_BeginReading (msg_t *msg){

	msg->readCount = 0;
}

/*
 =================
 MSG_ReadChar
 =================
*/
int MSG_ReadChar (msg_t *msg){

	int	c;
	
	if (msg->readCount + 1 > msg->curSize)
		c = -1;
	else
		c = (char)msg->data[msg->readCount];

	msg->readCount += 1;
	
	return c;
}

/*
 =================
 MSG_ReadByte
 =================
*/
int MSG_ReadByte (msg_t *msg){

	int	b;
	
	if (msg->readCount + 1 > msg->curSize)
		b = -1;
	else
		b = (byte)msg->data[msg->readCount];
	
	msg->readCount += 1;
	
	return b;
}

/*
 =================
 MSG_ReadShort
 =================
*/
int MSG_ReadShort (msg_t *msg){

	int	s;
	
	if (msg->readCount + 2 > msg->curSize)
		s = -1;
	else		
		s = (short)(msg->data[msg->readCount] + (msg->data[msg->readCount+1] << 8));
	
	msg->readCount += 2;
	
	return s;
}

/*
 =================
 MSG_ReadLong
 =================
*/
int MSG_ReadLong (msg_t *msg){

	int	l;
	
	if (msg->readCount + 4 > msg->curSize)
		l = -1;
	else
		l = (int)msg->data[msg->readCount] + (msg->data[msg->readCount+1] << 8) + (msg->data[msg->readCount+2] << 16) + (msg->data[msg->readCount+3] << 24);
	
	msg->readCount += 4;
	
	return l;
}

/*
 =================
 MSG_ReadFloat
 =================
*/
float MSG_ReadFloat (msg_t *msg){

	union {
		byte	b[4];
		float	f;
	} dat;
	
	if (msg->readCount+4 > msg->curSize)
		dat.f = -1;
	else {
		dat.b[0] = msg->data[msg->readCount];
		dat.b[1] = msg->data[msg->readCount+1];
		dat.b[2] = msg->data[msg->readCount+2];
		dat.b[3] = msg->data[msg->readCount+3];
	}
	
	msg->readCount += 4;
	
	return LittleFloat(dat.f);
}

/*
 =================
 MSG_ReadString
 =================
*/
char *MSG_ReadString (msg_t *msg){

	static char	string[2048];
	int			l, c;

	l = 0;
	do {
		if (msg->readCount + 1 > msg->curSize)
			c = -1;
		else
			c = (char)msg->data[msg->readCount];

		msg->readCount += 1;

		if (c == -1 || c == 0)
			break;

		string[l++] = c;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

/*
 =================
 MSG_ReadStringLine
 =================
*/
char *MSG_ReadStringLine (msg_t *msg){

	static char	string[2048];
	int			l, c;
	
	l = 0;
	do {
		if (msg->readCount + 1 > msg->curSize)
			c = -1;
		else
			c = (char)msg->data[msg->readCount];

		msg->readCount += 1;

		if (c == -1 || c == 0 || c == '\n')
			break;

		string[l++] = c;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

/*
 =================
 MSG_ReadCoord
 =================
*/
float MSG_ReadCoord (msg_t *msg){

	return MSG_ReadShort(msg) * 0.125;
}

/*
 =================
 MSG_ReadPos
 =================
*/
void MSG_ReadPos (msg_t *msg, vec3_t pos){

	pos[0] = MSG_ReadShort(msg) * 0.125;
	pos[1] = MSG_ReadShort(msg) * 0.125;
	pos[2] = MSG_ReadShort(msg) * 0.125;
}

/*
 =================
 MSG_ReadAngle
 =================
*/
float MSG_ReadAngle (msg_t *msg){

	return MSG_ReadChar(msg) * (360.0/256);
}

/*
 =================
 MSG_ReadAngle16
 =================
*/
float MSG_ReadAngle16 (msg_t *msg){

	return SHORT2ANGLE(MSG_ReadShort(msg));
}

/*
 =================
 MSG_ReadDir
 =================
*/
void MSG_ReadDir (msg_t *msg, vec3_t dir){

	int		b;

	b = MSG_ReadByte(msg);
	ByteToDir(b, dir);
}

/*
 =================
 MSG_ReadDeltaUserCmd
 =================
*/
void MSG_ReadDeltaUserCmd (msg_t *msg, const usercmd_t *from, usercmd_t *to){

	int bits;

	memcpy(to, from, sizeof(*to));

	bits = MSG_ReadByte(msg);

	// Read current angles
	if (bits & CM_ANGLE1)
		to->angles[0] = MSG_ReadShort(msg);
	if (bits & CM_ANGLE2)
		to->angles[1] = MSG_ReadShort(msg);
	if (bits & CM_ANGLE3)
		to->angles[2] = MSG_ReadShort(msg);

	// Read movement
	if (bits & CM_FORWARD)
		to->forwardmove = MSG_ReadShort(msg);
	if (bits & CM_SIDE)
		to->sidemove = MSG_ReadShort(msg);
	if (bits & CM_UP)
		to->upmove = MSG_ReadShort(msg);

	// Read buttons
	if (bits & CM_BUTTONS)
		to->buttons = MSG_ReadByte(msg);

	if (bits & CM_IMPULSE)
		to->impulse = MSG_ReadByte(msg);

	// Read time to run command
	to->msec = MSG_ReadByte(msg);

	// Read the light level
	to->lightlevel = MSG_ReadByte(msg);
}

/*
 =================
 MSG_ReadData
 =================
*/
void MSG_ReadData (msg_t *msg, void *buffer, int size){

	int		i;

	for (i = 0; i < size; i++)
		((byte *)buffer)[i] = MSG_ReadByte(msg);
}
