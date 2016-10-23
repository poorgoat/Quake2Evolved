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


#include "server.h"


#define MAX_MASTERS		5

static netAdr_t	sv_masterAdr[MAX_MASTERS];	// Address of group servers

server_t		sv;							// Local server
serverStatic_t	svs;						// Persistant server info

client_t		*sv_client;					// Current client
edict_t			*sv_player;					// Current player

cvar_t	*sv_cheats;
cvar_t	*sv_maxClients;
cvar_t	*sv_hostName;
cvar_t	*sv_nextServer;
cvar_t	*sv_master1;
cvar_t	*sv_master2;
cvar_t	*sv_master3;
cvar_t	*sv_master4;
cvar_t	*sv_master5;
cvar_t	*sv_timeOut;
cvar_t	*sv_zombieTime;
cvar_t	*sv_reconnectLimit;
cvar_t	*sv_noReload;
cvar_t	*sv_airAccelerate;
cvar_t	*sv_enforceTime;
cvar_t	*sv_allowDownload;
cvar_t	*sv_publicServer;
cvar_t	*sv_rconPassword;


/*
 =================
 SV_NextServer
 =================
*/
void SV_NextServer (void){

	if (sv.state == SS_GAME)
		return;			// Can't nextserver while playing a normal game

	svs.spawnCount++;	// Make sure another doesn't sneak in

	if (!sv_nextServer->value[0])
		Cbuf_AddText("killServer\n");
	else {
		Cbuf_AddText(sv_nextServer->value);
		Cbuf_AddText("\n");

		Cvar_SetString("sv_nextServer", "");
	}
}

/*
 =================
 SV_DropClient

 Called when the player is totally leaving the server, either willingly
 or unwillingly. This is NOT called if the entire server is quiting or 
 crashing.
 =================
*/
void SV_DropClient (client_t *cl){

	// Add the disconnect
	MSG_WriteByte(&cl->netChan.message, SVC_DISCONNECT);

	// Call the game function for removing a client.
	// This will remove the body, among other things.
	if (cl->state == CS_SPAWNED)
		ge->ClientDisconnect(cl->edict);

	// Close download
	if (cl->downloadFile){
		FS_CloseFile(cl->downloadFile);
		cl->downloadFile = 0;
	}

	cl->state = CS_ZOMBIE;		// Become free in a few seconds
	cl->name[0] = 0;
}

/*
 =================
 SV_UserInfoChanged

 Pull specific info from a newly changed user info string into a more C
 friendly form
 =================
*/
void SV_UserInfoChanged (client_t *cl){

	char	*val;

	// Call game code to allow overrides
	ge->ClientUserinfoChanged(cl->edict, cl->userInfo);

	// Name for C code
	Q_strncpyz(cl->name, Info_ValueForKey(cl->userInfo, "name"), sizeof(cl->name));

	// Rate command
	val = Info_ValueForKey(cl->userInfo, "rate");
	if (val[0])
		cl->rate = Clamp(atoi(val), 100, 25000);
	else
		cl->rate = 5000;

	// Msg command
	val = Info_ValueForKey(cl->userInfo, "msg");
	if (val[0])
		cl->messageLevel = atoi(val);
}

/*
 =================
 SV_StatusString

 Builds the string that is sent as heartbeats and status replies
 =================
*/
static char *SV_StatusString (void){

	static char	status[MAX_MSGLEN - 16];
	char		player[64];
	int			statusLen;
	int			playerLen;
	int			i;
	client_t	*cl;

	Q_snprintfz(status, sizeof(status), "%s\n", Cvar_ServerInfo());
	statusLen = strlen(status);

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state == CS_CONNECTED || cl->state == CS_SPAWNED){
			Q_snprintfz(player, sizeof(player), "%i %i \"%s\"\n", cl->edict->client->ps.stats[STAT_FRAGS], cl->ping, cl->name);
			playerLen = strlen(player);

			if (statusLen + playerLen >= sizeof(status))
				break;		// Can't hold any more

			Q_strncatz(status, player, sizeof(status));
			statusLen += playerLen;
		}
	}

	return status;
}

/*
 =================
 SV_Status

 Responds with all the info that qplug or qspy can see
 =================
*/
static void SV_Status (void){

	NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\n%s", SV_StatusString());
}

/*
 =================
 SV_Ack
 =================
*/
static void SV_Ack (void){

	Com_Printf("Ping acknowledge from %s\n", NET_AdrToString(net_from));
}

/*
 =================
 SV_Info

 Responds with short info for broadcast scans.
 The second parameter should be the current protocol version number.
 =================
*/
static void SV_Info (void){

	char	string[64];
	int		version, count = 0;
	int		i;

	if (sv_maxClients->integerValue == 1)
		return;		// Ignore in singleplayer

	version = atoi(Cmd_Argv(1));

	if (version != PROTOCOL_VERSION)
		Q_snprintfz(string, sizeof(string), "%s: wrong version\n", sv_hostName->value, sizeof(string));
	else {
		for (i = 0; i < sv_maxClients->integerValue; i++){
			if (svs.clients[i].state >= CS_CONNECTED)
				count++;
		}

		Q_snprintfz(string, sizeof(string), "%16s %8s %2i/%2i\n", sv_hostName->value, sv.name, count, sv_maxClients->integerValue);
	}

	NetChan_OutOfBandPrint(NS_SERVER, net_from, "info\n%s", string);
}

/*
 =================
 SV_Ping

 Just responds with an acknowledgement
 =================
*/
static void SV_Ping (void){

	NetChan_OutOfBandPrint(NS_SERVER, net_from, "ack");
}

/*
 =================
 SV_GetChallenge

 Returns a challenge number that can be used in a subsequent 
 client_connect command.
 We do this to prevent denial of service attacks that flood the server 
 with invalid connection IPs. With a challenge, they must give a valid 
 IP address.
 =================
*/
static void SV_GetChallenge (void){

	int		i, oldest, oldestTime;

	oldest = 0;
	oldestTime = 0x7FFFFFFF;

	// See if we already have a challenge for this IP
	for (i = 0; i < MAX_CHALLENGES; i++){
		if (NET_CompareBaseAdr(net_from, svs.challenges[i].adr))
			break;

		if (svs.challenges[i].time < oldestTime){
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES){
		// Overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7FFF;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = Sys_Milliseconds();
		i = oldest;
	}

	// Send it back
	NetChan_OutOfBandPrint(NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge);
}

/*
 =================
 SV_DirectConnect

 A connection request that did not come from the master
 =================
*/
static void SV_DirectConnect (void){

	int			i;
	client_t	*cl, *newCL = NULL;
	char		userInfo[MAX_INFO_STRING];
	edict_t		*ent;
	int			version, qport, challenge;

	version = atoi(Cmd_Argv(1));
	if (version != PROTOCOL_VERSION){
		NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\nServer uses protocol version %i\n", PROTOCOL_VERSION);
		Com_DPrintf("Rejected connect from version %i\n", version);
		return;
	}

	qport = atoi(Cmd_Argv(2));
	challenge = atoi(Cmd_Argv(3));
	Q_strncpyz(userInfo, Cmd_Argv(4), sizeof(userInfo));

	// Force the IP key/value pair so the game can filter based on IP
	Info_SetValueForKey(userInfo, "ip", NET_AdrToString(net_from));

	// attractLoop servers are ONLY for local clients
	if (sv.attractLoop){
		if (!NET_IsLocalAddress(net_from)){
			NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\nConnection refused\n");
			Com_DPrintf("Rejected connect in attract loop\n");
			return;
		}
	}

	// See if the challenge is valid
	if (!NET_IsLocalAddress(net_from)){
		for (i = 0; i < MAX_CHALLENGES; i++){
			if (NET_CompareBaseAdr(net_from, svs.challenges[i].adr)){
				if (challenge == svs.challenges[i].challenge)
					break;		// Good

				NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\nBad challenge\n");
				Com_DPrintf("%s: bad challenge\n", NET_AdrToString(net_from));
				return;
			}
		}

		if (i == MAX_CHALLENGES){
			NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\nNo challenge for address\n");
			Com_DPrintf("%s: no challenge\n", NET_AdrToString(net_from));
			return;
		}
	}

	// If there is already a slot for this IP, reuse it
	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state == CS_FREE)
			continue;

		if (NET_CompareBaseAdr(net_from, cl->netChan.remoteAddress) && (cl->netChan.qport == qport || net_from.port == cl->netChan.remoteAddress.port)){
			if (!NET_IsLocalAddress(net_from) && (svs.realTime - cl->lastConnect) < SEC2MS(sv_reconnectLimit->floatValue)){
				Com_DPrintf("%s: reconnect rejected : too soon\n", NET_AdrToString(net_from));
				return;
			}

			Com_DPrintf("%s: reconnect\n", NET_AdrToString(net_from));

			newCL = cl;
			break;
		}
	}

	// Find a client slot
	if (!newCL){
		for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
			if (cl->state == CS_FREE){
				newCL = cl;
				break;
			}
		}

		if (!newCL){
			NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\nServer is full\n");
			Com_DPrintf("Rejected a connection\n");
			return;
		}
	}

	// Build a new connection.
	// Accept the new client.
	// This is the only place a client_t is ever initialized.
	memset(newCL, 0, sizeof(client_t));

	i = newCL - svs.clients;
	ent = EDICT_NUM(i+1);
	newCL->edict = ent;
	newCL->challenge = challenge;		// Save challenge for checksumming

	// Get the game a chance to reject this connection or modify the
	// user info
	if (!(ge->ClientConnect(ent, userInfo))){
		if (*Info_ValueForKey(userInfo, "rejmsg"))
			NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\n%s\n", Info_ValueForKey(userInfo, "rejmsg"));
		else
			NetChan_OutOfBandPrint(NS_SERVER, net_from, "print\nConnection refused\n");

		Com_DPrintf("Game rejected a connection\n");
		return;
	}

	// Parse some info from the info strings
	Q_strncpyz(newCL->userInfo, userInfo, sizeof(newCL->userInfo));
	SV_UserInfoChanged(newCL);

	NetChan_Setup(NS_SERVER, &newCL->netChan, net_from, qport);

	MSG_Init(&newCL->datagram, newCL->datagramBuffer, sizeof(newCL->datagramBuffer), true);

	newCL->lastMessage = svs.realTime;	// Don't time-out
	newCL->lastConnect = svs.realTime;
	newCL->state = CS_CONNECTED;

	// Send the connect packet to the client
	NetChan_OutOfBandPrint(NS_SERVER, net_from, "client_connect");
}

/*
 =================
 SV_RconValidate
 =================
*/
static qboolean SV_RconValidate (void){

	if (!sv_rconPassword->value[0])
		return false;

	if (Q_strcmp(sv_rconPassword->value, Cmd_Argv(1)))
		return false;

	return true;
}

/*
 =================
 SV_RemoteCommand

 A client issued a rcon command.
 Shift down the remaining args.
 Redirect all printfs.
 =================
*/
static void SV_RemoteCommand (void){

	int		i;
	char	remaining[1024];

	if (!SV_RconValidate())
		Com_Printf("Bad rcon from %s:\n%s\n", NET_AdrToString(net_from), net_message.data+4);
	else
		Com_Printf("Rcon from %s:\n%s\n", NET_AdrToString(net_from), net_message.data+4);

	Com_BeginRedirect(RD_PACKET, sv_outputBuf, OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!SV_RconValidate())
		Com_Printf("Bad \"rconPassword\"\n");
	else {
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++){
			Q_strncatz(remaining, Cmd_Argv(i), sizeof(remaining));
			Q_strncatz(remaining, " ", sizeof(remaining));
		}

		Cbuf_ExecuteText(EXEC_NOW, remaining);
	}

	Com_EndRedirect();
}

/*
 =================
 SV_ConnectionlessPacket

 A connectionless packet has four leading 0xFF characters to distinguish 
 it from a game channel.
 Clients that are in the game can still send connectionless packets.
 =================
*/
static void SV_ConnectionlessPacket (void){

	char	*s, *c;

	MSG_BeginReading(&net_message);
	MSG_ReadLong(&net_message);		// Skip the -1 marker

	s = MSG_ReadStringLine(&net_message);
	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);
	Com_DPrintf("SV packet %s: %s\n", NET_AdrToString(net_from), c);

	if (!Q_stricmp(c, "status"))
		SV_Status();
	else if (!Q_stricmp(c, "ack"))
		SV_Ack();
	else if (!Q_stricmp(c, "info"))
		SV_Info();
	else if (!Q_stricmp(c, "ping"))
		SV_Ping();
	else if (!Q_stricmp(c, "getchallenge"))
		SV_GetChallenge();
	else if (!Q_stricmp(c, "connect"))
		SV_DirectConnect();
	else if (!Q_stricmp(c, "rcon"))
		SV_RemoteCommand();
	else
		Com_Printf(S_COLOR_YELLOW "Bad connectionless packet from %s:\n%s\n", NET_AdrToString(net_from), s);
}

/*
 =================
 SV_ReadPackets
 =================
*/
static void SV_ReadPackets (void){

	int			i;
	client_t	*cl;
	int			qport;

	while (NET_GetPacket(NS_SERVER, &net_from, &net_message)){
		// Check for connectionless packet first
		if (*(int *)net_message.data == -1){
			SV_ConnectionlessPacket();
			continue;
		}

		// Read the qport out of the message so we can fix up stupid 
		// address translating routers
		MSG_BeginReading(&net_message);
		MSG_ReadLong(&net_message);		// Sequence number
		MSG_ReadLong(&net_message);		// Sequence number
		qport = MSG_ReadShort(&net_message) & 0xFFFF;

		// Check for packets from connected clients
		for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
			if (cl->state == CS_FREE)
				continue;

			if (!NET_CompareBaseAdr(net_from, cl->netChan.remoteAddress))
				continue;

			if (cl->netChan.qport != qport)
				continue;

			if (cl->netChan.remoteAddress.port != net_from.port){
				Com_Printf(S_COLOR_YELLOW "SV_ReadPackets: fixing up a translated port\n");
				cl->netChan.remoteAddress.port = net_from.port;
			}

			if (NetChan_Process(&cl->netChan, &net_message)){
				// This is a valid sequenced packet, so process it
				if (cl->state != CS_ZOMBIE){
					cl->lastMessage = svs.realTime;	// Don't time-out

					SV_ParseClientMessage(cl);
				}
			}

			break;
		}
	}
}

/*
 =================
 SV_MasterHeartbeat

 Send a message to the master every few minutes to let it know we are 
 alive, and log information
 =================
*/
static void SV_MasterHeartbeat (void){

	cvar_t		*master;
	netAdr_t	adr;
	char		*string;
	int			i;

	if (!com_dedicated->integerValue)
		return;		// Only dedicated servers send heartbeats

	if (!sv_publicServer->integerValue)
		return;		// A private dedicated game

	// Check for time wrapping
	if (svs.lastHeartbeat > svs.realTime)
		svs.lastHeartbeat = svs.realTime;

	if (svs.realTime - svs.lastHeartbeat < 300000)
		return;		// Not time to send yet

	svs.lastHeartbeat = svs.realTime;

	// Send the same string that we would give for a status OOB command
	string = SV_StatusString();

	// Send to group master
	for (i = 0; i < MAX_MASTERS; i++){
		master = Cvar_Get(va("sv_master%i", i+1), "", 0, NULL);

		// If the variable has been modified, we need to resolve a new
		// master server address
		if (master->modified){
			master->modified = false;

			// Inform the previous master that this server is going down
			if (sv_masterAdr[i].port){
				Com_Printf("Sending heartbeat to %s...\n", NET_AdrToString(sv_masterAdr[i]));
				NetChan_OutOfBandPrint(NS_SERVER, sv_masterAdr[i], "shutdown");

				memset(&sv_masterAdr[i], 0, sizeof(netAdr_t));
			}

			if (!master->value[0])
				continue;		// Empty slot

			// Resolve the new master address
			if (!NET_StringToAdr(master->value, &adr)){
				Com_Printf("Bad master address: %s\n", master->value);
				continue;
			}

			if (adr.port == 0)
				adr.port = BigShort(PORT_MASTER);

			// Send an initial ping
			memcpy(&sv_masterAdr[i], &adr, sizeof(netAdr_t));

			Com_Printf("Sending heartbeat to %s...\n", NET_AdrToString(sv_masterAdr[i]));
			NetChan_OutOfBandPrint(NS_SERVER, sv_masterAdr[i], "ping");

			continue;
		}

		if (!sv_masterAdr[i].port)
			continue;

		Com_Printf("Sending heartbeat to %s...\n", NET_AdrToString(sv_masterAdr[i]));
		NetChan_OutOfBandPrint(NS_SERVER, sv_masterAdr[i], "heartbeat\n%s", string);
	}
}

/*
 =================
 SV_MasterShutdown

 Informs all masters that this server is going down
 =================
*/
static void SV_MasterShutdown (void){

	int		i;

	if (!com_dedicated->integerValue)
		return;		// Only dedicated servers send heartbeats

	if (!sv_publicServer->integerValue)
		return;		// A private dedicated game

	// Send to group master
	for (i = 0; i < MAX_MASTERS; i++){
		if (!sv_masterAdr[i].port)
			continue;

		Com_Printf("Sending heartbeat to %s...\n", NET_AdrToString(sv_masterAdr[i]));
		NetChan_OutOfBandPrint(NS_SERVER, sv_masterAdr[i], "shutdown");
	}
}

/*
 =================
 SV_CalcPings

 Updates the cl->ping variables
 =================
*/
static void SV_CalcPings (void){

	int			i, j;
	client_t	*cl;
	int			total, count;

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state != CS_SPAWNED)
			continue;

		total = 0;
		count = 0;

		for (j = 0; j < LATENCY_COUNTS; j++){
			if (cl->frameLatency[j] > 0){
				total += cl->frameLatency[j];
				count++;
			}
		}

		if (!count)
			cl->ping = 0;
		else
			cl->ping = total / count;

		// Let the game library know about the ping
		cl->edict->client->ping = cl->ping;
	}
}

/*
 =================
 SV_GiveMsec

 Every few frames, give all clients an allotment of milliseconds for
 their command moves. If they exceed it, assume cheating.
 =================
*/
static void SV_GiveMsec (void){

	int			i;
	client_t	*cl;

	if (sv.frameNum & 15)
		return;

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state == CS_FREE)
			continue;

		cl->commandMsec = 1800;		// 1600 + some slop
	}
}

/*
 =================
 SV_CheckTimeOuts

 If a packet has not been received from a client for sv_timeOut seconds,
 drop the connection. Server frames are used instead of real-time to
 avoid dropping the local client while debugging.

 When a client is normally dropped, the client goes into a zombie state
 for a few seconds to make sure any final reliable message gets resent
 if necessary.
 =================
*/
static void SV_CheckTimeOuts (void){

	int			i;
	client_t	*cl;
	int			dropPoint, zombiePoint;

	dropPoint = svs.realTime - SEC2MS(sv_timeOut->floatValue);
	zombiePoint = svs.realTime - SEC2MS(sv_zombieTime->floatValue);

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		// Message times may be wrong across a level change
		if (cl->lastMessage > svs.realTime)
			cl->lastMessage = svs.realTime;

		if (cl->state == CS_ZOMBIE && cl->lastMessage < zombiePoint){
			cl->state = CS_FREE;	// Can now be reused
			continue;
		}

		if ((cl->state == CS_CONNECTED || cl->state == CS_SPAWNED) && cl->lastMessage < dropPoint){
			SV_BroadcastPrintf(PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient(cl); 

			cl->state = CS_FREE;	// Don't bother with zombie state
		}
	}
}

/*
 =================
 SV_ClearGameEvents
 =================
*/
static void SV_ClearGameEvents (void){

	edict_t	*ent;
	int		i;

	// Events only last for a single message
	for (i = 0; i < ge->num_edicts; i++, ent++){
		ent = EDICT_NUM(i);
		ent->s.event = 0;
	}
}

/*
 =================
 SV_RunGameFrame
 =================
*/
static void SV_RunGameFrame (void){

	// Don't run if paused
	if (com_paused->integerValue && sv_maxClients->integerValue == 1)
		return;

	sv.frameNum++;
	sv.time = sv.frameNum * 100;

	if (com_speeds->integerValue)
		com_timeBeforeGame = Sys_Milliseconds();

	ge->RunFrame();

	if (com_speeds->integerValue)
		com_timeAfterGame = Sys_Milliseconds();

	// Never get more than one tic behind
	if (sv.time < svs.realTime)
		svs.realTime = sv.time;
}

/*
 =================
 SV_AllowCheats
 =================
*/
qboolean SV_AllowCheats (void){

	// Allow cheats if not running a game
	if (!svs.initialized || sv.state != SS_GAME)
		return true;

	// Allow cheats if playing a singleplayer game
	if (sv_maxClients->integerValue == 1)
		return true;

	// Allow cheats if desired
	if (sv_cheats->integerValue)
		return true;

	// Otherwise don't allow cheats at all
	return false;
}


// =====================================================================


/*
 =================
 SV_Frame
 =================
*/
void SV_Frame (int msec){

	// If server is not active, do nothing
	if (!svs.initialized)
		return;

    svs.realTime += msec;

	// Keep the random time dependent
	rand();

	// Check time-outs
	SV_CheckTimeOuts();

	// Get packets from clients
	SV_ReadPackets();

	// Move autonomous things around if enough time has passed
	if (!com_timeDemo->integerValue && svs.realTime < sv.time){
		// Never let the time get too far off
		if (sv.time - svs.realTime > 100)
			svs.realTime = sv.time - 100;

		return;
	}

	// Update ping based on the last known frame from all clients
	SV_CalcPings();

	// Give the clients some time slices
	SV_GiveMsec();

	// Let everything in the world think and move
	SV_RunGameFrame();

	// Send messages back to the clients that had packets read this 
	// frame
	SV_SendClientMessages();

	// Save the entire world state if recording a serverdemo
	SV_RecordDemoMessage();

	// Clear game events for next frame
	SV_ClearGameEvents();

	// Send a heartbeat to the masters if needed
	SV_MasterHeartbeat();
}

/*
 =================
 SV_Init

 Only called at startup, not for each game
 =================
*/
void SV_Init (void){

	// Register our variables and commands
	Cvar_Get("skill", "1", CVAR_SERVERINFO | CVAR_LATCH, "Game skill setting");
	Cvar_Get("coop", "0", CVAR_SERVERINFO | CVAR_LATCH, "Cooperative game mode");
	Cvar_Get("deathmatch", "0", CVAR_SERVERINFO | CVAR_LATCH, "Deathmatch game mode");
	Cvar_Get("dmflags", va("%i", DF_INSTANT_ITEMS), CVAR_SERVERINFO, "Deathmatch flags");
	Cvar_Get("fraglimit", "0", CVAR_SERVERINFO, "Frag limit");
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO, "Time limit");
	Cvar_Get("sv_mapName", "", CVAR_SERVERINFO | CVAR_ROM, "Current map name");
	Cvar_Get("sv_mapChecksum", "", CVAR_ROM, "Current map checksum");
	Cvar_Get("sv_protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM, "Server protocol version");
	sv_cheats = Cvar_Get("sv_cheats", "0", CVAR_SERVERINFO | CVAR_LATCH, "Allow cheats in the game");
	sv_maxClients = Cvar_Get("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH, "Maximum number of clients allowed in the server");
	sv_hostName = Cvar_Get("sv_hostName", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE, "Host name");
	sv_nextServer = Cvar_Get("sv_nextServer", "", 0, "Next server map");
	sv_master1 = Cvar_Get("sv_master1", "", CVAR_ARCHIVE, "Master server address");
	sv_master2 = Cvar_Get("sv_master2", "", CVAR_ARCHIVE, "Master server address");
	sv_master3 = Cvar_Get("sv_master3", "", CVAR_ARCHIVE, "Master server address");
	sv_master4 = Cvar_Get("sv_master4", "", CVAR_ARCHIVE, "Master server address");
	sv_master5 = Cvar_Get("sv_master5", "", CVAR_ARCHIVE, "Master server address");
	sv_timeOut = Cvar_Get("sv_timeOut", "120", 0, "Connection time out time in seconds");
	sv_zombieTime = Cvar_Get("sv_zombieTime", "2", 0, "Zombie time in seconds");
	sv_reconnectLimit = Cvar_Get("sv_reconnectLimit", "3", 0, "Reconnect count limit");
	sv_noReload = Cvar_Get("sv_noReload", "0", 0, "Don't reload a saved game");
	sv_airAccelerate = Cvar_Get("sv_airAccelerate", "0", CVAR_SERVERINFO | CVAR_LATCH, "Air acceleration");
	sv_enforceTime = Cvar_Get("sv_enforceTime", "0", 0, "Enforce time");
	sv_allowDownload = Cvar_Get("sv_allowDownload", "1", CVAR_ARCHIVE, "Allow file downloads to clients");
	sv_publicServer = Cvar_Get("sv_publicServer", "1", 0, "Public server");
	sv_rconPassword = Cvar_Get("rconPassword", "", 0, "Remote console password");

	Cmd_AddCommand("loadGame", SV_LoadGame_f, "Load a game");
	Cmd_AddCommand("saveGame", SV_SaveGame_f, "Save a game");
	Cmd_AddCommand("gameMap", SV_GameMap_f, "Load a game map");
	Cmd_AddCommand("map", SV_Map_f, "Load a map");
	Cmd_AddCommand("demo", SV_Demo_f, "Run a demo");
	Cmd_AddCommand("kick", SV_Kick_f, "Kick a player from the server");
	Cmd_AddCommand("status", SV_Status_f, "Show server status");
	Cmd_AddCommand("heartbeat", SV_Heartbeat_f, "Send a heartbeat to master servers");
	Cmd_AddCommand("serverInfo", SV_ServerInfo_f, "Show server information");
	Cmd_AddCommand("dumpUser", SV_DumpUser_f, "Show user information from a player");
	Cmd_AddCommand("serverRecord", SV_ServerRecord_f, "Record a server demo");
	Cmd_AddCommand("serverStopRecord", SV_ServerStopRecord_f, "Stop recording a server demo");
	Cmd_AddCommand("killServer", SV_KillServer_f, "Kill the server");
	Cmd_AddCommand("sv", SV_ServerCommand_f, "Execute a game server command");

	if (com_dedicated->integerValue)
		Cmd_AddCommand("say", SV_ConSay_f, "Send a chat message to everyone");
}

/*
 =================
 SV_FinalMessage

 Used by SV_Shutdown to send a final message to all connected clients 
 before the server goes down. The messages are sent immediately, not 
 just stuck on the outgoing message list, because the server is going
 to totally exit after returning from this function.
 =================
*/
static void SV_FinalMessage (const char *message, qboolean reconnect){

	int			i;
	client_t	*cl;
	char		buffer[MAX_MSGLEN];
	msg_t		msg;

	if (!svs.clients)
		return;

	MSG_Init(&msg, buffer, sizeof(buffer), false);
	MSG_WriteByte(&msg, SVC_PRINT);
	MSG_WriteByte(&msg, PRINT_HIGH);
	MSG_WriteString(&msg, message);

	if (reconnect)
		MSG_WriteByte(&msg, SVC_RECONNECT);
	else
		MSG_WriteByte(&msg, SVC_DISCONNECT);

	// Send it twice.
	// Stagger the packets to crutch operating system limited buffers.
	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state >= CS_CONNECTED)
			NetChan_Transmit(&cl->netChan, msg.data, msg.curSize);
	}

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state >= CS_CONNECTED)
			NetChan_Transmit(&cl->netChan, msg.data, msg.curSize);
	}
}

/*
 =================
 SV_Shutdown

 Called when each game quits, before Sys_Quit or Sys_Error
 =================
*/
void SV_Shutdown (const char *message, qboolean reconnect){

	int			i;
	client_t	*cl;

	if (!svs.initialized)
		return;

	Com_Printf("------- Server Shutdown -------\n");

	// Send a final message
	SV_FinalMessage(message, reconnect);

	// Inform all masters that this server is going down
	SV_MasterShutdown();

	// Shutdown game
	SV_ShutdownGameProgs();

	// Free server data
	if (sv.demoFile)
		FS_CloseFile(sv.demoFile);

	memset(&sv, 0, sizeof(server_t));

	// Free server static data
	if (svs.demoFile)
		FS_CloseFile(svs.demoFile);

	if (svs.clients){
		for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
			if (cl->downloadFile)
				FS_CloseFile(cl->downloadFile);
		}

		Z_Free(svs.clients);
	}

	if (svs.clientEntities)
		Z_Free(svs.clientEntities);

	memset(&svs, 0, sizeof(serverStatic_t));

	// Free current level
	CM_UnloadMap();

	// Set server state
	Com_SetServerState(sv.state);

	Com_Printf("-------------------------------\n");
}
