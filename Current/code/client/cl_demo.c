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


// cl_demo.c -- demo recording


#include "client.h"


/*
 =================
 CL_Record_f

 Begins recording a demo from the current position
 =================
*/
void CL_Record_f (void){

	char			name[MAX_OSPATH];
	char			data[MAX_MSGLEN];
	msg_t			msg;
	int				i, len;
	entity_state_t	*ent, nullState;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: record [demoName]\n");
		return;
	}

	if (cls.demoFile){
		Com_Printf("Already recording\n");
		return;
	}

	if (cls.state != CA_ACTIVE || cls.cinematicPlaying || cl.demoPlaying){
		Com_Printf("You must be in a level to record\n");
		return;
	}

	if (Cmd_Argc() == 1){
		// Find a file name to record to
		for (i = 0; i <= 9999; i++){
			Q_snprintfz(name, sizeof(name), "demos/demo%04i.dm2", i);
			if (!FS_FileExists(name))
				break;	// File doesn't exist
		}

		if (i == 10000){
			Com_Printf("Demos directory is full!\n");
			return;
		}
	}
	else {
		Q_snprintfz(name, sizeof(name), "demos/%s", Cmd_Argv(1));
		Com_DefaultExtension(name, sizeof(name), ".dm2");
	}

	// Open the demo file
	FS_OpenFile(name, &cls.demoFile, FS_WRITE);
	if (!cls.demoFile){
		Com_Printf("Couldn't open %s\n", name);
		return;
	}

	Com_Printf("Recording to %s\n", name);

	Q_strncpyz(cls.demoName, name, sizeof(cls.demoName));

	// Don't start saving messages until a non-delta compressed message 
	// is received
	cls.demoWaiting = true;

	// Write out messages to hold the startup information
	MSG_Init(&msg, data, sizeof(data), false);

	// Send the server data
	MSG_WriteByte(&msg, SVC_SERVERDATA);
	MSG_WriteLong(&msg, PROTOCOL_VERSION);
	MSG_WriteLong(&msg, 0x10000 + cl.serverCount);
	MSG_WriteByte(&msg, 1);			// Demos are always attract loops
	MSG_WriteString(&msg, cl.gameDir);
	MSG_WriteShort(&msg, cl.clientNum - 1);
	MSG_WriteString(&msg, cl.configStrings[CS_NAME]);

	// Configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++){
		if (!cl.configStrings[i][0])
			continue;

		if (msg.curSize + strlen(cl.configStrings[i]) + 32 > msg.maxSize){
			// Write it out
			len = LittleLong(msg.curSize);
			FS_Write(&len, sizeof(len), cls.demoFile);
			FS_Write(msg.data, msg.curSize, cls.demoFile);

			msg.curSize = 0;
		}

		MSG_WriteByte(&msg, SVC_CONFIGSTRING);
		MSG_WriteShort(&msg, i);
		MSG_WriteString(&msg, cl.configStrings[i]);
	}

	// Baselines
	memset(&nullState, 0, sizeof(entity_state_t));
	for (i = 0; i < MAX_EDICTS; i++){
		ent = &cl.entities[i].baseline;
		if (!ent->modelindex)
			continue;

		if (msg.curSize + sizeof(entity_state_t) + 32 > msg.maxSize){
			// Write it out
			len = LittleLong(msg.curSize);
			FS_Write(&len, sizeof(len), cls.demoFile);
			FS_Write(msg.data, msg.curSize, cls.demoFile);

			msg.curSize = 0;
		}

		MSG_WriteByte(&msg, SVC_SPAWNBASELINE);
		MSG_WriteDeltaEntity(&msg, &nullState, ent, true, true);
	}

	MSG_WriteByte(&msg, SVC_STUFFTEXT);
	MSG_WriteString(&msg, "precache\n");

	// Write it to the demo file
	len = LittleLong(msg.curSize);
	FS_Write(&len, sizeof(len), cls.demoFile);
	FS_Write(msg.data, msg.curSize, cls.demoFile);

	// The rest of the demo file will be individual frames
}

/*
 =================
 CL_StopRecord_f

 Stop recording a demo
 =================
*/
void CL_StopRecord_f (void){

	int		end;

	if (!cls.demoFile){
		Com_Printf("Not recording a demo\n");
		return;
	}

	// Finish up
	end = LittleLong(-1);

	FS_Write(&end, sizeof(end), cls.demoFile);
	FS_CloseFile(cls.demoFile);
	cls.demoFile = 0;

	Com_Printf("Stopped recording\n");
}

/*
 =================
 CL_WriteDemoMessage

 Dumps the current net message, prefixed by the length
 =================
*/
void CL_WriteDemoMessage (void){

	int		len, swLen;

	if (!cls.demoFile || cls.demoWaiting)
		return;

	// The first eight bytes are just packet sequencing stuff
	len = net_message.curSize-8;
	swLen = LittleLong(len);

	if (!swLen)
		return;

	FS_Write(&swLen, sizeof(swLen), cls.demoFile);
	FS_Write(net_message.data+8, len, cls.demoFile);
}
