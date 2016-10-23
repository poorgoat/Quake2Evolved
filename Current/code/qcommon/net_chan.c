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


/*
 =======================================================================

 PACKET HEADER
 -------------
 31	sequence
 1	does this message contain a reliable payload
 31	acknowledge sequence
 1	acknowledge receipt of even/odd message
 16	qport

 The remote connection never knows if it missed a reliable message, the
 local side detects that it has been dropped by seeing a sequence
 acknowledge higher than the last reliable sequence, but without the
 correct even/odd bit for the reliable set.

 If the sender notices that a reliable message has been dropped, it will
 be retransmitted. It will not be retransmitted again until a message
 after the retransmit has been acknowledged and the reliable still
 failed to get there.

 If the sequence number is -1, the packet should be handled without a
 net connection.

 The reliable message can be added to at any time by doing
 MSG_Write* (&netChan->message, <data>).

 If the message buffer is overflowed, either by a single message, or by
 multiple frames worth piling up while the last reliable transmit goes
 unacknowledged, the netChan signals a fatal error.

 Reliable messages are always placed first in a packet, then the
 unreliable message is included if there is sufficient room.

 To the receiver, there is no distinction between the reliable and
 unreliable parts of the message, they are just processed out as a
 single larger message.

 Illogical packet sequence numbers cause the packet to be dropped, but
 do not kill the connection. This, combined with the tight window of
 valid reliable acknowledgement numbers provides protection against
 malicious address spoofing.

 The qport field is a workaround for bad address translating routers
 that sometimes remap the client's source port on a packet during
 gameplay.

 If the base part of the net address matches and the qport matches, then
 the channel matches even if the IP port differs. The IP port should be
 updated to the new value before sending out any replies.

 If there is no information that needs to be transfered on a given
 frame, such as during the connection stage while waiting for the client
 to load, then a packet only needs to be delivered if there is something
 in the unacknowledged reliable.

 =======================================================================
*/


netAdr_t	net_from;
msg_t		net_message;
byte		net_messageBuffer[MAX_MSGLEN];

cvar_t		*net_qport;
cvar_t		*net_showPackets;
cvar_t		*net_showDrop;


/*
 =================
 NetChan_Init
 =================
*/
void NetChan_Init (void){

	int		port;

	// Pick a port value that should be nice and random
	port = Sys_Milliseconds() & 0xFFFF;

	net_qport = Cvar_Get("net_qport", va("%i", port), CVAR_ROM, "Quake port");
	net_showPackets = Cvar_Get("net_showPackets", "0", 0, "Report sent/received packet information");
	net_showDrop = Cvar_Get("net_showDrop", "0", 0, "Report dropped packets");

	MSG_Init(&net_message, net_messageBuffer, sizeof(net_messageBuffer), false);
}

/*
 =================
 NetChan_OutOfBand

 Sends an out-of-band datagram
 =================
*/
void NetChan_OutOfBand (netSrc_t sock, const netAdr_t adr, const void *data, int length){

	byte	buffer[MAX_MSGLEN];
	msg_t	msg;

	// Write the packet header
	MSG_Init(&msg, buffer, sizeof(buffer), false);

	MSG_WriteLong(&msg, -1);		// -1 sequence means out of band
	MSG_Write(&msg, data, length);

	// Send the datagram
	NET_SendPacket(sock, adr, msg.data, msg.curSize);
}

/*
 =================
 NetChan_OutOfBandPrint

 Sends a text message in an out-of-band datagram
 =================
*/
void NetChan_OutOfBandPrint (netSrc_t sock, const netAdr_t adr, const char *fmt, ...){

	char	string[MAX_MSGLEN - 4];
	va_list	argPtr;

	va_start(argPtr, fmt);
#ifdef SECURE
	vsnprintf_s(string, sizeof(string), MAX_MSGLEN - 4, fmt, argPtr);
#else
	vsnprintf(string, sizeof(string), fmt, argPtr);
#endif
	va_end(argPtr);

	NetChan_OutOfBand(sock, adr, string, strlen(string));
}

/*
 =================
 NetChan_Setup

 Called to open a channel to a remote system
 =================
*/
void NetChan_Setup (netSrc_t sock, netChan_t *chan, const netAdr_t adr, int qport){

	memset(chan, 0, sizeof(netChan_t));

	chan->sock = sock;
	chan->remoteAddress = adr;
	chan->qport = qport;
	chan->lastReceived = Sys_Milliseconds();
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;

	MSG_Init(&chan->message, chan->messageBuffer, sizeof(chan->messageBuffer), true);
}

/*
 =================
 NetChan_Transmit

 Tries to send an unreliable message to a connection, and handles the
 transmition / retransmition of the reliable messages.

 A 0 length will still generate a packet and deal with the reliable 
 messages.
 =================
*/
void NetChan_Transmit (netChan_t *chan, const void *data, int length){

	byte		buffer[MAX_MSGLEN];
	msg_t		msg;
	qboolean	sendReliable = false;
	unsigned	w1, w2;

	// Check for message overflow
	if (chan->message.overflowed){
		Com_Printf(S_COLOR_YELLOW "%s: outgoing message overflow\n", NET_AdrToString(chan->remoteAddress));
		return;
	}

	// If the remote side dropped the last reliable message, resend it
	if (chan->incomingAcknowledged > chan->lastReliableSequence && chan->incomingReliableAcknowledged != chan->reliableSequence)
		sendReliable = true;

	// If the reliable transmit buffer is empty, copy the current 
	// message out
	if (!chan->reliableLength && chan->message.curSize)
		sendReliable = true;

	if (!chan->reliableLength && chan->message.curSize){
		memcpy(chan->reliableBuffer, chan->messageBuffer, chan->message.curSize);
		chan->reliableLength = chan->message.curSize;
		chan->message.curSize = 0;
		chan->reliableSequence ^= 1;
	}

	// Write the packet header
	MSG_Init(&msg, buffer, sizeof(buffer), false);

	w1 = (chan->outgoingSequence & ~(1<<31)) | (sendReliable<<31);
	w2 = (chan->incomingSequence & ~(1<<31)) | (chan->incomingReliableSequence<<31);

	chan->outgoingSequence++;
	chan->lastSent = Sys_Milliseconds();

	MSG_WriteLong(&msg, w1);
	MSG_WriteLong(&msg, w2);

	// Send the qport if we are a client
	if (chan->sock == NS_CLIENT)
		MSG_WriteShort(&msg, net_qport->integerValue);

	// Copy the reliable message to the packet first
	if (sendReliable){
		MSG_Write(&msg, chan->reliableBuffer, chan->reliableLength);
		chan->lastReliableSequence = chan->outgoingSequence;
	}

	// Add the unreliable part if space is available
	if (msg.maxSize - msg.curSize >= length)
		MSG_Write(&msg, data, length);
	else
		Com_Printf(S_COLOR_YELLOW "%s: dumped unreliable\n", NET_AdrToString(chan->remoteAddress));

	// Send the datagram
	NET_SendPacket(chan->sock, chan->remoteAddress, msg.data, msg.curSize);

	if (net_showPackets->integerValue){
		if (chan->sock == NS_CLIENT)
			Com_Printf("CL ");
		else if (chan->sock == NS_SERVER)
			Com_Printf("SV ");

		if (sendReliable)
			Com_Printf("send %4i : s=%i reliable=%i ack=%i rack=%i\n", msg.curSize, chan->outgoingSequence - 1, chan->reliableSequence, chan->incomingSequence, chan->incomingReliableSequence);
		else
			Com_Printf("send %4i : s=%i ack=%i rack=%i\n", msg.curSize, chan->outgoingSequence - 1, chan->incomingSequence, chan->incomingReliableSequence);
	}
}

/*
 =================
 NetChan_Process

 Called when the current net_message is from remoteAddress. Modifies 
 net_message so that it points to the packet payload.
 =================
*/
qboolean NetChan_Process (netChan_t *chan, msg_t *msg){

	unsigned	sequence, sequenceAck;
	unsigned	reliableAck, reliableMessage;
	int			qport;

	MSG_BeginReading(msg);

	// Get sequence numbers
	sequence = MSG_ReadLong(msg);
	sequenceAck = MSG_ReadLong(msg);

	// Read the qport if we are a server
	if (chan->sock == NS_SERVER)
		qport = MSG_ReadShort(msg);

	reliableMessage = sequence >> 31;
	reliableAck = sequenceAck >> 31;

	sequence &= ~(1<<31);
	sequenceAck &= ~(1<<31);	

	if (net_showPackets->integerValue){
		if (chan->sock == NS_CLIENT)
			Com_Printf("CL ");
		else if (chan->sock == NS_SERVER)
			Com_Printf("SV ");

		if (reliableMessage)
			Com_Printf("recv %4i : s=%i reliable=%i ack=%i rack=%i\n", msg->curSize, sequence, chan->incomingReliableSequence ^ 1, sequenceAck, reliableAck);
		else
			Com_Printf("recv %4i : s=%i ack=%i rack=%i\n", msg->curSize, sequence, sequenceAck, reliableAck);
	}

	// Discard stale or duplicated packets
	if (sequence <= chan->incomingSequence){
		if (net_showDrop->integerValue)
			Com_Printf("%s: out of order packet %i at %i\n", NET_AdrToString(chan->remoteAddress), sequence, chan->incomingSequence);

		return false;
	}

	// Dropped packets don't keep the message from being used
	chan->dropped = sequence - (chan->incomingSequence+1);
	if (chan->dropped > 0){
		if (net_showDrop->integerValue)
			Com_Printf("%s: dropped %i packets at %i\n", NET_AdrToString(chan->remoteAddress), chan->dropped, sequence);
	}

	// If the current outgoing reliable message has been acknowledged
	// clear the buffer to make way for the next
	if (reliableAck == chan->reliableSequence)
		chan->reliableLength = 0;	// It has been received
	
	// If this message contains a reliable message, bump 
	// incomingReliableSequence 
	chan->incomingSequence = sequence;
	chan->incomingAcknowledged = sequenceAck;
	chan->incomingReliableAcknowledged = reliableAck;

	if (reliableMessage)
		chan->incomingReliableSequence ^= 1;

	// The message can now be read from the current message pointer
	chan->lastReceived = Sys_Milliseconds();

	return true;
}
