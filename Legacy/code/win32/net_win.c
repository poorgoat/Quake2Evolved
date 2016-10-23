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


#include "winquake.h"
#include "../qcommon/qcommon.h"


#define	MAX_LOOPBACK	4

typedef struct {
	byte		data[MAX_MSGLEN];
	int			dataLen;
} loopMsg_t;

typedef struct {
	loopMsg_t	msgs[MAX_LOOPBACK];
	int			get;
	int			send;
} loopback_t;

static loopback_t	net_loopbacks[2];
static int			net_sockets[2];

cvar_t	*net_ip;
cvar_t	*net_port;
cvar_t	*net_clientPort;


/*
 =================
 NET_ErrorString
 =================
*/
static const char *NET_ErrorString (void){

	int		code;

	code = WSAGetLastError();

	switch (code){
	case WSAEINTR:				return "WSAEINTR";
	case WSAEBADF:				return "WSAEBADF";
	case WSAEACCES:				return "WSAEACCES";
	case WSAEDISCON:			return "WSAEDISCON";
	case WSAEFAULT:				return "WSAEFAULT";
	case WSAEINVAL:				return "WSAEINVAL";
	case WSAEMFILE:				return "WSAEMFILE";
	case WSAEWOULDBLOCK:		return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS:		return "WSAEINPROGRESS";
	case WSAEALREADY:			return "WSAEALREADY";
	case WSAENOTSOCK:			return "WSAENOTSOCK";
	case WSAEDESTADDRREQ:		return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE:			return "WSAEMSGSIZE";
	case WSAEPROTOTYPE:			return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT:		return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT:	return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT:	return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP:			return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT:		return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT:		return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE:			return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL:		return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN:			return "WSAENETDOWN";
	case WSAENETUNREACH:		return "WSAENETUNREACH";
	case WSAENETRESET:			return "WSAENETRESET";
	case WSAECONNABORTED:		return "WSAECONNABORTED";
	case WSAECONNRESET:			return "WSAECONNRESET";
	case WSAENOBUFS:			return "WSAENOBUFS";
	case WSAEISCONN:			return "WSAEISCONN";
	case WSAENOTCONN:			return "WSAENOTCONN";
	case WSAESHUTDOWN:			return "WSAESHUTDOWN";
	case WSAETOOMANYREFS:		return "WSAETOOMANYREFS";
	case WSAETIMEDOUT:			return "WSAETIMEDOUT";
	case WSAECONNREFUSED:		return "WSAECONNREFUSED";
	case WSAELOOP:				return "WSAELOOP";
	case WSAENAMETOOLONG:		return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN:			return "WSAEHOSTDOWN";
	case WSASYSNOTREADY:		return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED:	return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED:		return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND:		return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN:			return "WSATRY_AGAIN";
	case WSANO_RECOVERY:		return "WSANO_RECOVERY";
	case WSANO_DATA:			return "WSANO_DATA";
	default:					return "UNKNOWN ERROR";
	}
}

/*
 =================
 NET_NetadrToSockadr
 =================
*/
static void NET_NetadrToSockadr (const netAdr_t *adr, struct sockaddr *s){

	memset(s, 0, sizeof(*s));

	if (adr->type == NA_BROADCAST){
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = adr->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (adr->type == NA_IP){
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&adr->ip;
		((struct sockaddr_in *)s)->sin_port = adr->port;
	}
}

/*
 =================
 NET_SockadrToNetadr
 =================
*/
static void NET_SockadrToNetadr (const struct sockaddr *s, netAdr_t *adr){

	if (s->sa_family == AF_INET){
		adr->type = NA_IP;
		*(int *)&adr->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		adr->port = ((struct sockaddr_in *)s)->sin_port;
	}
}

/*
 =================
 NET_StringToSockaddr

 localhost
 idnewt
 idnewt:28000
 192.246.40.70
 192.246.40.70:28000
 =================
*/
static qboolean NET_StringToSockaddr (const char *string, struct sockaddr *s){

	struct hostent	*h;
	char			*colon;
	char			copy[128];
	
	memset(s, 0, sizeof(*s));

	((struct sockaddr_in *)s)->sin_family = AF_INET;
	((struct sockaddr_in *)s)->sin_port = 0;

	Q_strncpyz(copy, string, sizeof(copy));

	// Strip off a trailing :port if present
	for (colon = copy; *colon; colon++){
		if (*colon == ':'){
			*colon = 0;
			((struct sockaddr_in *)s)->sin_port = htons((short)atoi(colon+1));	
		}
	}
		
	if (copy[0] >= '0' && copy[0] <= '9')
		*(int *)&((struct sockaddr_in *)s)->sin_addr = inet_addr(copy);
	else {
		if (!(h = gethostbyname(copy)))
			return false;

		*(int *)&((struct sockaddr_in *)s)->sin_addr = *(int *)h->h_addr_list[0];
	}
	
	return true;
}

/*
 =================
 NET_CompareAdr
 =================
*/
qboolean NET_CompareAdr (const netAdr_t a, const netAdr_t b){

	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;
	else if (a.type == NA_IP){
		if (!memcmp(a.ip, b.ip, 4) && a.port == b.port)
			return true;

		return false;
	}
	else {
		Com_Printf(S_COLOR_RED "NET_CompareAdr: bad address type\n");
		return false;
	}
}

/*
 =================
 NET_CompareBaseAdr

 Compare without the port
 =================
*/
qboolean NET_CompareBaseAdr (const netAdr_t a, const netAdr_t b){

	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;
	else if (a.type == NA_IP){
		if (!memcmp(a.ip, b.ip, 4))
			return true;

		return false;
	}
	else {
		Com_Printf(S_COLOR_RED "NET_CompareBaseAdr: bad address type\n");
		return false;
	}
}

/*
 =================
 NET_IsLocalAddress
 =================
*/
qboolean NET_IsLocalAddress (const netAdr_t adr){

	return (adr.type == NA_LOOPBACK);
}

/*
 =================
 NET_AdrToString
 =================
*/
char *NET_AdrToString (const netAdr_t adr){

	static char	string[64];

	if (adr.type == NA_LOOPBACK)
		Q_snprintfz(string, sizeof(string), "loopback");
	else if (adr.type == NA_IP)
		Q_snprintfz(string, sizeof(string), "%i.%i.%i.%i:%i", adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], ntohs(adr.port));
	else
		string[0] = 0;

	return string;
}

/*
 =================
 NET_StringToAdr

 localhost
 idnewt
 idnewt:28000
 192.246.40.70
 192.246.40.70:28000
 =================
*/
qboolean NET_StringToAdr (const char *string, netAdr_t *adr){

	struct sockaddr s;
	
	if (!Q_stricmp(string, "localhost")){
		memset(adr, 0, sizeof(netAdr_t));
		adr->type = NA_LOOPBACK;
		return true;
	}

	if (!NET_StringToSockaddr(string, &s))
		return false;
	
	NET_SockadrToNetadr(&s, adr);

	return true;
}


/*
 =======================================================================

 LOOPBACK BUFFERS FOR LOCAL PLAYER

 =======================================================================
*/


/*
 =================
 NET_GetLoopPacket
 =================
*/
static qboolean NET_GetLoopPacket (netSrc_t sock, netAdr_t *from, msg_t *msg){

	int			i;
	loopback_t	*loop;

	loop = &net_loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy(msg->data, loop->msgs[i].data, loop->msgs[i].dataLen);
	msg->curSize = loop->msgs[i].dataLen;

	memset(from, 0, sizeof(netAdr_t));
	from->type = NA_LOOPBACK;

	return true;
}

/*
 =================
 NET_SendLoopPacket
 =================
*/
static qboolean NET_SendLoopPacket (netSrc_t sock, const netAdr_t to, const void *data, int length){

	int			i;
	loopback_t	*loop;

	if (to.type != NA_LOOPBACK)
		return false;

	loop = &net_loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].dataLen = length;

	return true;
}


// =====================================================================


/*
 =================
 NET_GetPacket
 =================
*/
qboolean NET_GetPacket (netSrc_t sock, netAdr_t *from, msg_t *msg){

	int 			ret, err;
	struct sockaddr	addr;
	int				addrLen;
	int				net_socket;

	if (NET_GetLoopPacket(sock, from, msg))
		return true;

	net_socket = net_sockets[sock];
	if (!net_socket)
		return false;

	addrLen = sizeof(addr);
	ret = recvfrom(net_socket, msg->data, msg->maxSize, 0, (struct sockaddr *)&addr, &addrLen);

	NET_SockadrToNetadr(&addr, from);

	if (ret == SOCKET_ERROR){
		err = WSAGetLastError();

		// WSAEWOULDBLOCK and WSAECONNRESET are silent
		if (err == WSAEWOULDBLOCK || err == WSAECONNRESET)
			return false;

		Com_Printf(S_COLOR_RED "NET_GetPacket: %s from %s\n", NET_ErrorString(), NET_AdrToString(*from));
		return false;
	}

	if (ret == msg->maxSize){
		Com_Printf(S_COLOR_RED "NET_GetPacket: oversize packet from %s\n", NET_AdrToString(*from));
		return false;
	}

	msg->curSize = ret;
	return true;
}

/*
 =================
 NET_SendPacket
 =================
*/
void NET_SendPacket (netSrc_t sock, const netAdr_t to, const void *data, int length){

	int				ret, err;
	struct sockaddr	addr;
	int				net_socket;

	if (NET_SendLoopPacket(sock, to, data, length))
		return;

	if (to.type != NA_BROADCAST && to.type != NA_IP)
		Com_Error(ERR_FATAL, "NET_SendPacket: bad address type");

	net_socket = net_sockets[sock];
	if (!net_socket)
		return;

	NET_NetadrToSockadr(&to, &addr);

	ret = sendto(net_socket, data, length, 0, &addr, sizeof(addr));

	if (ret == SOCKET_ERROR){
		err = WSAGetLastError();

		// WSAEWOULDBLOCK is silent
		if (err == WSAEWOULDBLOCK)
			return;

		// Some PPP links don't allow broadcasts
		if (err == WSAEADDRNOTAVAIL && to.type == NA_BROADCAST)
			return;

		Com_Printf(S_COLOR_RED "NET_SendPacket: %s to %s\n", NET_ErrorString(), NET_AdrToString(to));
	}
}


// =====================================================================


/*
 =================
 NET_UDPSocket
 =================
*/
static int NET_UDPSocket (const char *netInterface, int port){

	int					err;
	struct sockaddr_in	addr;
	qboolean			_true = 1;
	int					net_socket;

	Com_DPrintf("NET_UDPSocket( %s, %i )\n", netInterface, port);

	if ((net_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR){
		err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
			Com_DPrintf(S_COLOR_YELLOW "NET_UDPSocket: socket = %s\n", NET_ErrorString());

		return 0;
	}

	// Make it non-blocking
	if (ioctlsocket(net_socket, FIONBIO, &_true) == SOCKET_ERROR){
		Com_DPrintf(S_COLOR_YELLOW "NET_UDPSocket: ioctlsocket FIONBIO = %s\n", NET_ErrorString());
		closesocket(net_socket);
		return 0;
	}

	// Make it broadcast capable
	if (setsockopt(net_socket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof(_true)) == SOCKET_ERROR){
		Com_DPrintf(S_COLOR_YELLOW "NET_UDPSocket: setsockopt SO_BROADCAST = %s\n", NET_ErrorString());
		closesocket(net_socket);
		return 0;
	}

	if (!netInterface[0] || !Q_stricmp(netInterface, "localhost"))
		addr.sin_addr.s_addr = INADDR_ANY;
	else
		NET_StringToSockaddr(netInterface, (struct sockaddr *)&addr);

	if (port == PORT_ANY)
		addr.sin_port = 0;
	else
		addr.sin_port = htons((short)port);

	addr.sin_family = AF_INET;

	if (bind(net_socket, (void *)&addr, sizeof(addr)) == SOCKET_ERROR){
		Com_DPrintf(S_COLOR_YELLOW "NET_UDPSocket: bind = %s\n", NET_ErrorString());
		closesocket(net_socket);
		return 0;
	}

	return net_socket;
}

/*
 =================
 NET_OpenUDP
 =================
*/
static void NET_OpenUDP (void){

	net_sockets[NS_SERVER] = NET_UDPSocket(net_ip->string, net_port->integer);
	if (!net_sockets[NS_SERVER])
		Com_Printf(S_COLOR_YELLOW "WARNING: failed to open server UDP socket\n");

	// Dedicated servers don't need client ports
	if (dedicated->integer)
		return;

	net_sockets[NS_CLIENT] = NET_UDPSocket(net_ip->string, net_clientPort->integer);
	if (!net_sockets[NS_CLIENT]){
		net_sockets[NS_CLIENT] = NET_UDPSocket(net_ip->string, PORT_ANY);
		if (net_sockets[NS_CLIENT])
			Com_Printf(S_COLOR_YELLOW "WARNING: failed to open client UDP socket\n");
	}
}

/*
 =================
 NET_CloseUDP
 =================
*/
static void NET_CloseUDP (void){

	if (net_sockets[NS_SERVER]){
		closesocket(net_sockets[NS_SERVER]);
		net_sockets[NS_SERVER] = 0;
	}

	if (net_sockets[NS_CLIENT]){
		closesocket(net_sockets[NS_CLIENT]);
		net_sockets[NS_CLIENT] = 0;
	}
}

/*
 =================
 NET_ShowIP_f
 =================
*/
void NET_ShowIP_f (void){

	char			s[256];
	int				i;
	struct hostent	*h;
	struct in_addr	in;

	gethostname(s, sizeof(s));
	if (!(h = gethostbyname(s))){
		Com_Printf("Can't get host\n");
		return;
	}

	Com_Printf("HostName: %s\n", h->h_name);

	for (i = 0; h->h_addr_list[i]; i++){
		in.s_addr = *(int *)h->h_addr_list[i];
		Com_Printf("IP: %s\n", inet_ntoa(in));
	}
}

/*
 =================
 NET_Restart_f
 =================
*/
void NET_Restart_f (void){

	NET_Shutdown();
	NET_Init();
}

/*
 =================
 NET_Init
 =================
*/
void NET_Init (void){

	WSADATA	wsaData;
	int		err;

	// Register our cvars and commands
	net_ip = Cvar_Get("net_ip", "localhost", CVAR_LATCH);
	net_port = Cvar_Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH);
	net_clientPort = Cvar_Get("net_clientPort", va("%i", PORT_CLIENT), CVAR_LATCH);

	Cmd_AddCommand("showip", NET_ShowIP_f);
	Cmd_AddCommand("net_restart", NET_Restart_f);

	// Initialize Winsock
	if ((err = WSAStartup(MAKEWORD(1, 1), &wsaData)) != 0){
		Com_Printf(S_COLOR_YELLOW "WARNING: Winsock initialization failed, returned %i\n", err);
		return;
	}

	Com_Printf("Winsock Initialized\n");

	// Open sockets
	NET_OpenUDP();

	NET_ShowIP_f();
}

/*
 =================
 NET_Shutdown
 =================
*/
void NET_Shutdown (void){

	Cmd_RemoveCommand("showip");
	Cmd_RemoveCommand("net_restart");

	// Close sockets
	NET_CloseUDP();

	// Shutdown Winsock
	WSACleanup();
}
