//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name network.c	-	The network. */
//
//	(c) Copyright 2000-2002 by Lutz Sammer, Andreas Arens.
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

//@{

//----------------------------------------------------------------------------
//	Documentation
//----------------------------------------------------------------------------

/**
**      @page NetworkModule Module - Network
**
**      @section Basics How does it work.
**
**	FreeCraft uses an UDP peer to peer protocol (p2p). The default port
**	is 6660.
**
**	@subsection udp_vs_tcp UDP vs. TCP
**
**	UDP is a connectionless protocol. This means it does not perform
**	retransmission of data and therefore provides very few error recovery
**	services. UDP instead offers a direct way to send and receive
**	datagrams (packets) over the network; it is used primarily for
**	broadcasting messages.
**
**	TCP, on the other hand, provides a connection-based, reliable data
**	stream.  TCP guarantees delivery of data and also guarantees that
**	packets will be delivered in the same order in which they were sent.
**
**	TCP is a simple and effective way of transmitting data. For making sure
**	that a client and server can talk to each other it is very good.
**	However, it carries with it a lot of overhead and extra network lag.
**
**	UDP needs less overhead and has a smaller lag. Which is very important
**	for real time games. The disadvantages includes:
**
**	@li You won't have an individual socket for each client.
**	@li Given that clients don't need to open a unique socket in order to
**		transmit data there is the very real possibility that a client
**		who is not logged into the game will start sending all kinds of
**		garbage to your server in some kind of attack. It becomes much
**		more difficult to stop them at this point.
**	@li Likewise, you won't have a clear disconnect/leave game message
**		unless you write one yourself.
**	@li Some data may not reach the other machine, so you may have to send
**		important stuff many times.
**	@li Some data may arrive in the wrong order. Imagine that you get
**		package 3 before package 1. Even a package can come duplicate.
**	@li UDP is connectionless and therefore has problems with firewalls.
**
**	I have choosen UDP. Additional support for the TCP protocol is welcome.
**
**	@subsection sc_vs_p2p server/client vs. peer to peer
**
**	@li server to client
**
**	The player input is send to the server. The server collects the input
**	of all players and than send the commands to all clients.
**
**	@li peer to peer (p2p)
**
**	The player input is direct send to all others clients in game.
**
**	p2p has the advantage of a smaller lag, but needs a higher bandwidth
**	by the clients.
**
**	I have choosen p2p. Additional support for a server to client protocol
**	is welcome.
**
**	@subsection bandwidth bandwidth
**
**	I wanted to support up to 8 players with 28.8kbit modems.
**
**	Most modems have a bandwidth of 28.8K bits/second (both directions) to
**	56K bits/second (33.6K uplink) It takes actually 10 bits to send 1 byte.
**	This makes calculating how many bytes you are sending easy however, as
**	you just need to divide 28800 bits/second by 10 and end up with 2880
**	bytes per second.
**
**	We want to send many packets, more updated pro second and big packets,
**	less protocol overhead.
**
**	If we do an update 6 times per second, leaving approximately 480 bytes
**	per update in an ideal environment.
**
**	For the TCP/IP protocol we need following:
**	IP  Header 20 bytes
**	UDP Header 8  bytes
**
**	With 10 bytes per command and 4 commands this are 68 (20+8+4*10) bytes
**	pro packet.  Sending it to 7 other players, gives 476 bytes pro update.
**	This means we could do 6 updates (each 166ms) pro second.
**
**	@subsection a_packet Network packet
**
**	@li [IP  Header - 20 bytes]
**	@li [UDP Header -  8 bytes]
**	@li [Type 1 byte][Cycle 1 byte][Data 8 bytes] - Slot 0
**	@li [Type 1 byte][Cycle 1 byte][Data 8 bytes] - Slot 1
**	@li [Type 1 byte][Cycle 1 byte][Data 8 bytes] - Slot 2
**	@li [Type 1 byte][Cycle 1 byte][Data 8 bytes] - Slot 3
**
**	@subsection internals Putting it together
**
**	All computers in play must run absolute syncron. Only user commands
**	are send over the network to the other computers. The command needs
**	some time to reach the other clients (lag), so the command is not
**	executed immediatly on the local computer, it is stored in a delay
**	queue and send to all other clients. After a delay of ::NetworkLag
**	game cycles the commands of the other players are received and executed
**	together with the local command. Each ::NetworkUpdates game cycles there
**	must a package send, to keep the clients in sync, if there is no user
**	command, a dummy sync package is send.
**	To avoid too much trouble with lost packages, a package contains 
**	::NetworkDups commands, the latest and ::NetworkDups-1 old.
**	If there are missing packages, the game is paused and old commands
**	are resend to all clients.
**
**	@section missing What features are missing
**
**	@li A possible improvement is to compress the sync message, this allows
**	    to lose more packets.
**
**	@li The recover from lost packets can be improved, if the server knows
**	    which packets the clients have received.
**
**	@li The UDP protocol isn't good for firewalls, we need also support
**	    for the TCP protocol.
**
**	@li Add a server / client protocol, which allows more players pro
**	    game.
**
**	@li Lag (latency) and bandwidth are set over the commandline. This
**	    should be automatic detected during game setup and later during
**	    game automatic adapted.
**
**	@li Also it would be nice, if we support viewing clients. This means
**	    other people can view the game in progress. 
**
**	@li The current protocol only uses single cast, for local LAN we
**	    should also support broadcast and multicast.
**
**      @li Proxy and relays should be supported, to improve the playable
**	    over the internet.
**
**	@li Currently the 4 slots are filled with package in sequence, but
**	    dropouts are normal short and then hits the same package. If we
**	    interleave the package, we are more resistent against short
**	    dropouts.
**
**	@li If we move groups, the commands are send for each unit separated.
**	    It would be better to transfer the group first and than only a
**	    single command for the complete group.
**
**	@li The game cycles is transfered for each slot, this is not needed. We
**	    can save some bytes if we compress this.
**
**	@li We can sort the command by importants, currently all commands are
**	    send in order, only chat messages are send if there are free slots.
**
**	@li password protection the login process (optional), to prevent that
**	    the wrong player join an open network game.
**
**	@li add meta server support, i have planned to use bnetd and its
**	    protocol.
**
**      @section api API How should it be used.
**
**	::InitNetwork1()
**
**	::InitNetwork2()
**
**	::ExitNetwork1()
**
**	::NetworkSendCommand()
**
**	::NetworkEvent()
**
**	::NetworkQuit()
**
**	::NetworkChatMessage()
**
**	::NetworkEvent()
**
**	::NetworkRecover()
**
**	::NetworkCommands()
**
**	::NetworkFildes
**
**	::NetworkInSync
**
**	@todo FIXME: continue docu
*/

// FIXME: should split the next into small modules!
// FIXME: I (Johns) leave this for other people (this means you!)

#define __COMPRESS_SYNC

//----------------------------------------------------------------------------
//	Includes
//----------------------------------------------------------------------------

#include <stdio.h>

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "freecraft.h"

#include "etlib/dllist.h"
#include "net_lowlevel.h"
#include "unit.h"
#include "map.h"
#include "actions.h"
#include "player.h"
#include "network.h"
#include "netconnect.h"
#include "commands.h"
#include "interface.h"

#define BASE_OF(type, elem, p) ((type *)((char *)(p) - offsetof(type, elem)))

//----------------------------------------------------------------------------
//	Declaration
//----------------------------------------------------------------------------

/**
**	Network command input/output queue.
*/
typedef struct _network_command_queue_ {
    struct dl_node	List[1];	/// double linked list
    int			Time;		/// time to execute
    NetworkCommand	Data;		/// command content
} NetworkCommandQueue;

//----------------------------------------------------------------------------
//	Variables
//----------------------------------------------------------------------------

global int NetworkNumInterfaces;	/// Network number of interfaces
global int NetworkFildes = -1;		/// Network file descriptor
global int NetworkInSync = 1;		/// Network is in sync
global int NetworkUpdates = 5;		/// Network update each # game cycles
global int NetworkLag = 5;		/// Network lag in # game cycles
global int NetworkStatus[PlayerMax];	/// Network status

local char NetMsgBuf[128][PlayerMax];	/// Chat message buffers
local int NetMsgBufLen[PlayerMax];	/// Stored chat message length
IfDebug(
global unsigned long MyHost;		/// My host number.
global int MyPort;			/// My port number.
);
local unsigned long NetworkDelay;	/// Delay counter for recover.
local int NetworkSyncSeeds[256];	/// Network sync seeds.
local int NetworkSyncHashs[256];	/// Network sync hashs.
local NetworkCommandQueue NetworkIn[256][PlayerMax]; /// Per-player network packet input queue
local DL_LIST(CommandsIn);		/// Network command input queue
local DL_LIST(CommandsOut);		/// Network command output queue

#ifdef DEBUG
local int NetworkReceivedPackets;	/// Packets received packets
local int NetworkReceivedEarly;		/// Packets received too early
local int NetworkReceivedLate;		/// Packets received too late
local int NetworkReceivedDups;		/// Packets received as duplicates
local int NetworkReceivedLost;		/// Packets received packet lost

local int NetworkSendPackets;		/// Packets send packets
local int NetworkSendResend;		/// Packets send to resend
#endif

/**@name api */
//@{

//----------------------------------------------------------------------------
//	Mid-Level api functions
//----------------------------------------------------------------------------

/**
**	Send message to all clients.
**
**	@param buf	Buffer of outgoing message.
**	@param len	Buffer length.
**
**	@todo FIXME: should support multicast and proxy clients/server.
*/
global void NetworkBroadcast(const void *buf, int len)
{
    int i;
#if 0
    //
    //	Can be enabled to simulate network delays.
    //
#define DELAY 5
    static char delay_buf[DELAY][1024];
    static int delay_len[DELAY];
    static int index;

    if (index >= DELAY) {
	// Send to all clients.
	for (i = 0; i < HostsCount; ++i) {
	    NetSendUDP(NetworkFildes, Hosts[i].Host, Hosts[i].Port,
		delay_buf[index % DELAY], delay_len[index % DELAY]);
	}
    }
    memcpy(delay_buf[index % DELAY], buf, len);
    delay_len[index % DELAY] = len;
    ++index;
#else

    // Send to all clients.
    for (i = 0; i < HostsCount; ++i) {
	int n;

	n = NetSendUDP(NetworkFildes, Hosts[i].Host, Hosts[i].Port, buf, len);
	DebugLevel3Fn("Sending %d to %d.%d.%d.%d:%d\n",
		n, NIPQUAD(ntohl(Hosts[i].Host)), ntohs(Hosts[i].Port));
    }
#endif
}

/**
**	Network send packet. Build it from queue and broadcast.
**
**	@param ncq	Outgoing network queue start.
*/
local void NetworkSendPacket(const NetworkCommandQueue *ncq)
{
    NetworkPacket packet;
    int i;

    IfDebug( ++NetworkSendPackets );

    DebugLevel3Fn("In cycle %lu sending: ",GameCycle);

    //
    //	Build packet of 4 messages.
    //
    for (i = 0; i < NetworkDups; ++i) {
	DebugLevel3("%d %d,",ncq->Data.Type,ncq->Time);
	packet.Commands[i] = ncq->Data;
	if (ncq->List->next->next) {
	    ncq = (NetworkCommandQueue *)(ncq->List->next);
	}
    }
    DebugLevel3("\n");

    // if (0 || !(rand() & 15))
	 NetworkBroadcast(&packet, sizeof(packet));

    if( HostsCount<3 ) {		// enough bandwidth to send twice :)
	 NetworkBroadcast(&packet, sizeof(packet));
    }
}

//----------------------------------------------------------------------------
//	API init..
//----------------------------------------------------------------------------

/**
**	Initialise network part 1.
*/
global void InitNetwork1(void)
{
    int i, port;

    DebugLevel0Fn("\n");

    DebugLevel3Fn("Packet %d\n", sizeof(NetworkCommand));
    DebugLevel3Fn("Packet %d\n", sizeof(NetworkChat));

    NetworkFildes = -1;
    NetworkInSync = 1;
    NetworkNumInterfaces = 0;

    NetInit();			// machine dependend setup

    for (i = 0; i < PlayerMax; i++) {
	NetMsgBufLen[i] = 0;
    }

    if (NetworkUpdates <= 0) {
	NetworkUpdates = 1;
    }
    // Lag must be multiple of Updates?
    NetworkLag /= NetworkUpdates;
    NetworkLag *= NetworkUpdates;

    // Our communication port
    port = NetworkPort;
    for( i=0; i<10; ++i ) {
	NetworkFildes = NetOpenUDP(port+i);
	if (NetworkFildes != -1) {
	    break;
	}
	if( i==9 ) {
	    fprintf(stderr,"NETWORK: No free ports %d-%d available, aborting\n",
		    port, port+i);
	    NetExit();		// machine dependend network exit
	    return;
	}
    }

#ifdef NEW_NETMENUS
    NetworkNumInterfaces = NetSocketAddr(NetworkFildes);
    if (NetworkNumInterfaces) {
	DebugLevel0Fn("Num IP: %d\n", NetworkNumInterfaces);
	for (i = 0; i < NetworkNumInterfaces; i++) {
	    DebugLevel0Fn("IP: %d.%d.%d.%d\n", NIPQUAD(ntohl(NetLocalAddrs[i])));
	}
    } else {
	fprintf(stderr, "NETWORK: Not connected to any external IPV4-network, aborting\n");
	ExitNetwork1();
	return;
    }
#endif

    IfDebug({
	char buf[128];

	gethostname(buf, sizeof(buf));
	DebugLevel0Fn("%s\n", buf);
	MyHost = NetResolveHost(buf);
	MyPort = NetLastPort;
	DebugLevel0Fn("My host:port %d.%d.%d.%d:%d\n",
		NIPQUAD(ntohl(MyHost)), ntohs(MyPort));
    });

    dl_init(CommandsIn);
    dl_init(CommandsOut);
}

/**
**	Cleanup network part 1. (to be called _AFTER_ part 2 :)
*/
global void ExitNetwork1(void)
{
    if (NetworkFildes == -1) {	// No network running
	return;
    }
#ifdef DEBUG
    DebugLevel0("Received: %d packets, %d early, %d late, %d dups, %d lost.\n",
	    NetworkReceivedPackets, NetworkReceivedEarly, NetworkReceivedLate,
	    NetworkReceivedDups,NetworkReceivedLost);
    DebugLevel0("Send: %d packets, %d resend\n",
	    NetworkSendPackets, NetworkSendResend);
#endif
    NetCloseUDP(NetworkFildes);

    NetExit();			// machine dependend setup
    NetworkFildes = -1;
    NetworkInSync = 1;
    NetPlayers = 0;
    HostsCount = 0;
}

/**
**	Initialise network part 2.
*/
global void InitNetwork2(void)
{
    int i, n;

    //
    //	Prepare first time without syncs.
    //
    for (i = 0; i <= NetworkLag; i += NetworkUpdates) {
	for (n = 0; n < HostsCount; ++n) {
	    NetworkIn[i][Hosts[n].PlyNr].Time = i;
	    NetworkIn[i][Hosts[n].PlyNr].Data.Cycle = i;
	    NetworkIn[i][Hosts[n].PlyNr].Data.Type = MessageSync;
	}
    }
}

//----------------------------------------------------------------------------
//	Commands input
//----------------------------------------------------------------------------

/**
**	Prepare send of command message.
**
**	Convert arguments into network format and place it into output queue.
**
**	@param command	Command (Move,Attack,...).
**	@param unit	Unit that receive the command.
**	@param x	optional X map position.
**	@param y	optional y map position.
**	@param dest	optional destination unit.
**	@param type	optional unit-type argument.
**	@param status	Append command or flush old commands.
**
**	@warning
**		Destination and unit-type shares the same network slot.
*/
global void NetworkSendCommand(int command, const Unit *unit, int x, int y,
	const Unit *dest, const UnitType *type, int status)
{
    NetworkCommandQueue *ncq;

    DebugLevel3Fn("%d,%d,(%d,%d),%d,%s,%s\n",
	command, unit->Slot, x, y, dest ? dest->Slot : -1,
	type ? type->Ident : "-", status ? "flush" : "append");

    //
    //	FIXME: look if we can ignore this command.
    //		Duplicate commands can be ignored.
    //
    ncq = malloc(sizeof(NetworkCommandQueue));
    dl_insert_first(CommandsIn, ncq->List);

    ncq->Time = GameCycle;
    ncq->Data.Type = command;
    if (status) {
	ncq->Data.Type |= 0x80;
    }
    ncq->Data.Unit = htons(unit->Slot);
    ncq->Data.X = htons(x);
    ncq->Data.Y = htons(y);
    DebugCheck( dest && type );		// Both together isn't allowed
    if (dest) {
	ncq->Data.Dest = htons(dest->Slot);
    } else if (type) {
	ncq->Data.Dest = htons(type->Type);
    } else {
	ncq->Data.Dest = htons(-1);
    }
}

/**
**	Called if message for the network is ready.
**	(by WaitEventsOneFrame)
**
**	@todo
**		NetworkReceivedEarly NetworkReceivedLate NetworkReceivedDups
**		Must be calculated.
*/
global void NetworkEvent(void)
{
    char buf[1024];
    NetworkPacket* packet;
    int player, i, n;

    //
    //	Read the packet.
    //
    if( (n = NetRecvUDP(NetworkFildes, &buf, sizeof(buf))) < 0) {
	//
	//	Server or client gone?
	//
	DebugLevel0("Server/Client gone.\n");
	Exit(0);
    }
    packet = (NetworkPacket *)buf;
    IfDebug( NetworkReceivedPackets++ );

    if (packet->Commands[0].Type <= MessageInitConfig) {
	NetworkParseSetupEvent(buf, n);
	return;
    }
    //
    //	Minimal checks for good/correct packet.
    //
    if (n != sizeof(NetworkPacket) && packet->Commands[0].Type != MessageQuit) {
	DebugLevel0Fn("Bad packet\n");
	return;
    }
    for (i = 0; i < HostsCount; ++i) {
	if (Hosts[i].Host == NetLastHost && Hosts[i].Port == NetLastPort) {
	    break;
	}
    }
    if (i == HostsCount) {
	DebugLevel0Fn("Not a host in play\n");
	return;
    }
    player = Hosts[i].PlyNr;

    //
    //	Parse the packet commands.
    //
    for (i = 0; i < NetworkDups; ++i) {
	const NetworkCommand *nc;

	nc = &packet->Commands[i];

	//
	//	Handle some messages.
	//
	if (nc->Type == MessageQuit) {
	    DebugLevel0("Got quit from network.\n");
	    Exit(0);
	}

	if (nc->Type == MessageResend) {
	    const NetworkCommandQueue *ncq;

	    // Destination cycle (time to execute).
	    n = ((GameCycle + 128) & ~0xFF) | nc->Cycle;
	    if (n > GameCycle + 128) {
		DebugLevel3Fn("+128 needed!\n");
		n -= 0x100;
	    }

	    // FIXME: not neccessary to send this packet multiple!!!!
	    //	other side send re-send until its gets an answer.

	    DebugLevel2Fn("Resend for %d got\n", n);
	    //
	    //	Find the commands to resend
	    //
#if 0
	    // Both directions are same fast/slow
	    ncq = (NetworkCommandQueue *)(CommandsOut->last);
	    while (ncq->List->prev) {
		DebugLevel3Fn("resend %d? %d\n", ncq->Time, n);
		if (ncq->Time == n) {
		    NetworkSendPacket(ncq);
		    break;
		}

		ncq = (NetworkCommandQueue *)(ncq->List->prev);
	    }
	    if (!ncq->List->prev) {
		DebugLevel2Fn("no packets for resend\n");
	    }
#else
	    ncq = (NetworkCommandQueue *)(CommandsOut->first);
	    while (ncq->List->next) {
		DebugLevel3Fn("resend %d? %d\n", ncq->Time, n);
		if (ncq->Time == n) {
		    NetworkSendPacket(ncq);
		    break;
		}

		ncq = (NetworkCommandQueue *)(ncq->List->next);
	    }
	    if (!ncq->List->next) {
		DebugLevel3Fn("no packets for resend\n");
	    }
#endif
	    continue;
	}

	// Destination cycle (time to execute).
	n = ((GameCycle + 128) & ~0xFF) | nc->Cycle;
	if (n > GameCycle + 128) {
	    DebugLevel3Fn("+128 needed!\n");
	    n -= 0x100;
	}

	if (nc->Type == MessageSync) {
	    // FIXME: must support compressed sync slots.
	}

	if (NetworkIn[nc->Cycle][player].Time != n) {
	    DebugLevel3Fn("Command %3d for %8d(%02X) got\n",
		    nc->Type, n, nc->Cycle);
	}

	// Receive statistic
	if( n>NetworkStatus[player] ) {
	    NetworkStatus[player] = n;
	}
#ifdef DEBUG
	if( i ) {
	    // Not in first slot and not got packet lost!
	    // FIXME: if more than 1 in sequence is lost, I count one to much!
	    if( NetworkIn[nc->Cycle][player].Time != n ) {
		++NetworkReceivedLost;
	    }
	} else {
	    // FIXME: more network statistic
	    if( NetworkIn[nc->Cycle][player].Time == n ) {
		++NetworkReceivedDups;
	    }
	}
#endif

	// Place in network in
	NetworkIn[nc->Cycle][player].Time = n;
	NetworkIn[nc->Cycle][player].Data = *nc;
    }

    //
    //	Waiting for this
    //
    if (!NetworkInSync) {
	NetworkInSync = 1;
	n = ((GameCycle) / NetworkUpdates) * NetworkUpdates + NetworkUpdates;
	DebugLevel3Fn("wait for %d - ", n);
	for (player = 0; player < HostsCount; ++player) {
	    if (NetworkIn[n & 0xFF][Hosts[player].PlyNr].Time != n) {
		NetworkInSync = 0;
		break;
	    }
	}
	DebugLevel3("%lu in sync %d\n", GameCycle, NetworkInSync);
    }
}

/**
**	Quit the game.
*/
global void NetworkQuit(void)
{
    NetworkCommand nc;

    nc.Type = MessageQuit;
    nc.Cycle = GameCycle & 0xFF;
    NetworkBroadcast(&nc, sizeof(NetworkCommand));

    // FIXME: if lost? Need an acknowledge for QuitMessages.
}

/**
**	Send chat message. (Message is send with low priority)
**
**	@param msg	Text message to send.
*/
global void NetworkChatMessage(const char *msg)
{
    NetworkCommandQueue *ncq;
    NetworkChat *ncm;
    const char *cp;
    int n;

    if (NetworkFildes != -1) {
	cp = msg;
	n = strlen(msg);
	while (n >= sizeof(ncm->Text)) {
	    ncq = malloc(sizeof(NetworkCommandQueue));
	    dl_insert_last(CommandsIn, ncq->List);
	    ncq->Data.Type = MessageChat;
	    ncm = (NetworkChat *)(&ncq->Data);
	    ncm->Player = ThisPlayer->Player;
	    memcpy(ncm->Text, cp, sizeof(ncm->Text));
	    cp += sizeof(ncm->Text);
	    n -= sizeof(ncm->Text);
	}
	ncq = malloc(sizeof(NetworkCommandQueue));
	dl_insert_last(CommandsIn, ncq->List);
	ncq->Data.Type = MessageChatTerm;
	ncm = (NetworkChat *)(&ncq->Data);
	ncm->Player = ThisPlayer->Player;
	memcpy(ncm->Text, cp, n + 1);		// see >= above :)
    }
}

/**
**	Parse a network command.
**
**	@param ncq	Network command from queue
*/
local void ParseNetworkCommand(const NetworkCommandQueue *ncq)
{
    int ply;
    const NetworkChat *ncm;

    switch (ncq->Data.Type) {
	case MessageSync:
	    ply = ntohs(ncq->Data.X) << 16;
	    ply |= ntohs(ncq->Data.Y);
	    if (ply != NetworkSyncSeeds[GameCycle & 0xFF]
		    || ntohs(ncq->Data.Unit)
			!= NetworkSyncHashs[GameCycle & 0xFF]) {
		DebugLevel0Fn("\n\aNetwork out of sync!\n\n");
	    }
	    return;
	case MessageChat:
	case MessageChatTerm:
	    ncm = (NetworkChat *)(&ncq->Data);
	    ply = ncm->Player;
	    if (NetMsgBufLen[ply] + sizeof(ncm->Text) < 128) {
		memcpy(((char *)NetMsgBuf[ply]) + NetMsgBufLen[ply], ncm->Text,
			sizeof(ncm->Text));
	    }
	    NetMsgBufLen[ply] += sizeof(ncm->Text);
	    if (ncq->Data.Type == MessageChatTerm) {
		NetMsgBuf[127][ply] = '\0';
		SetMessage(NetMsgBuf[ply]);
		NetMsgBufLen[ply] = 0;
	    }
	    break;
	default:
	    ParseCommand(ncq->Data.Type, ntohs(ncq->Data.Unit),
		    ntohs(ncq->Data.X), ntohs(ncq->Data.Y),
		    ntohs(ncq->Data.Dest));
	    break;
    }
}

/**
**	Network resend commands, we have a missing packet send to all clients
**	what packet we are missing.
**
**	@todo
**		We need only send to the clients, that have not delivered the
**		packet. I'm not sure that the extra packets I send with this
**		packet are useful.
*/
local void NetworkResendCommands(void)
{
    NetworkPacket packet;
    const NetworkCommandQueue *ncq;
    int i;

    IfDebug( ++NetworkSendResend );

    //
    //	Build packet of 4 messages.
    //
    packet.Commands[0].Type = MessageResend;
    packet.Commands[0].Cycle =
	    (GameCycle / NetworkUpdates) * NetworkUpdates + NetworkUpdates;

    DebugLevel2Fn("In cycle %lu for cycle %lu(%x):", GameCycle,
	    (GameCycle / NetworkUpdates) * NetworkUpdates + NetworkUpdates,
	    packet.Commands[0].Cycle);

    ncq = (NetworkCommandQueue *)(CommandsOut->last);

    for (i = 1; i < NetworkDups; ++i) {
	DebugLevel2("%d %d,", ncq->Data.Type, ncq->Time);
	packet.Commands[i] = ncq->Data;
	if (ncq->List->prev->prev) {
	    ncq = (NetworkCommandQueue *)(ncq->List->prev);
	}
    }
    DebugLevel2("<%d %d\n", ncq->Data.Type, ncq->Time);

    // if(0 || !(rand() & 15))
	NetworkBroadcast(&packet, sizeof(packet));
}

/**
**	Network send commands.
*/
local void NetworkSendCommands(void)
{
    NetworkCommandQueue *ncq;

    //
    //	No command available, send sync.
    //
    if (dl_empty(CommandsIn)) {
	ncq = malloc(sizeof(NetworkCommandQueue));
	ncq->Data.Type = MessageSync;
	ncq->Data.Unit = htons(SyncHash&0xFFFF);
	ncq->Data.X = htons(SyncRandSeed>>16);
	ncq->Data.Y = htons(SyncRandSeed&0xFFFF);
	// FIXME: can compress sync-messages.
    } else {
	DebugLevel3Fn("command in remove\n");
	ncq = (NetworkCommandQueue *)CommandsIn->first;
	// ncq = BASE_OF(NetworkCommandQueue,List[0], CommandsIn->first);

	IfDebug(
	if (ncq->Data.Type != MessageChat && ncq->Data.Type != MessageChatTerm) {
	    // FIXME: we can send destoyed units over network :(
	    if (UnitSlots[ntohs(ncq->Data.Unit)]->Destroyed) {
		DebugLevel0Fn("Sending destroyed unit %d over network!!!!!!\n",
			ntohs(ncq->Data.Unit));
	    }
	}
	);
	dl_remove_first(CommandsIn);
    }

    //	Insert in output queue.
    dl_insert_first(CommandsOut, ncq->List);

    //	Fill in the time
    ncq->Time = GameCycle + NetworkLag;
    ncq->Data.Cycle = ncq->Time & 0xFF;
    DebugLevel3Fn("sending for %d\n", ncq->Time);

    NetworkSendPacket(ncq);

    NetworkSyncSeeds[ncq->Time & 0xFF] = SyncRandSeed;
    NetworkSyncHashs[ncq->Time & 0xFF] = SyncHash & 0xFFFF;	// FIXME: 32bit later
}

/**
**	Network excecute commands.
*/
local void NetworkExecCommands(void)
{
    NetworkCommandQueue *ncq;
    int i;

    //
    //	Must execute commands on all computers in the same order.
    //
    for (i = 0; i < NumPlayers; ++i) {
	if (i == ThisPlayer->Player) {
	    //
	    //	Remove outdated commands from queue
	    //
	    while (!dl_empty(CommandsOut)) {
		ncq = (NetworkCommandQueue *)(CommandsOut->last);
		// FIXME: how many packets must be kept exactly?
		// if (ncq->Time + NetworkLag + NetworkUpdates >= GameCycle)
		// THIS is too much if (ncq->Time >= GameCycle)
		if (ncq->Time + NetworkLag > GameCycle) {
		    break;
		}
		DebugLevel3Fn("remove %lu,%d\n", GameCycle, ncq->Time);
		dl_remove_last(CommandsOut);
		free(ncq);
	    }
	    //
	    //	Execute local commands from queue
	    //
	    ncq = (NetworkCommandQueue *)(CommandsOut->last);
	    while (ncq->List->prev) {
		if (ncq->Time == GameCycle) {
		    DebugLevel3Fn("execute loc %lu,%d\n", GameCycle, ncq->Time);
		    ParseNetworkCommand(ncq);
		    break;
		}
		ncq = (NetworkCommandQueue *)(ncq->List->prev);
	    }
	} else {
	    //
	    //	Remove external commands.
	    //
	    ncq = &NetworkIn[GameCycle & 0xFF][i];
	    if (ncq->Time) {
		DebugLevel3Fn("execute net %lu,%d(%lx),%d\n",
			GameCycle, i, GameCycle & 0xFF, ncq->Time);
		if (ncq->Time != GameCycle) {
		    DebugLevel2Fn("cycle %lu idx %lu time %d\n",
			    GameCycle, GameCycle & 0xFF, ncq->Time);
		    DebugCheck(ncq->Time != GameCycle);
		}
		ParseNetworkCommand(ncq);
	    }
	}
    }
}

/**
**	Network synchronize commands.
*/
local void NetworkSyncCommands(void)
{
    const NetworkCommandQueue *ncq;
    int i;
    int n;

    //
    //	Check if all next messages are available.
    //
    NetworkInSync = 1;
    n = GameCycle + NetworkUpdates;
    for (i = 0; i < HostsCount; ++i) {
	DebugLevel3Fn("sync %d\n", Hosts[i].PlyNr);
	ncq = &NetworkIn[n & 0xFF][Hosts[i].PlyNr];
	DebugLevel3Fn("sync %d==%d\n", ncq->Time, n);
	if (ncq->Time != n) {
	    NetworkInSync = 0;
	    NetworkDelay = FrameCounter + NetworkUpdates;
	    // FIXME: should send a resent request.
	    DebugLevel3Fn("%lu not in sync %d\n", GameCycle, n);
	    break;
	}
    }
}

/**
**	Handle network commands.
*/
global void NetworkCommands(void)
{
    if (NetworkFildes != -1) {
	//
	//	Send messages to all clients (other players)
	//
	if (!(GameCycle % NetworkUpdates)) {
	    DebugLevel3Fn("Update %lu\n", GameCycle);

	    NetworkSendCommands();
	    NetworkExecCommands();
	    NetworkSyncCommands();
	}
    }
}

/**
**	Recover network.
*/
global void NetworkRecover(void)
{
    // Got no message just resent our oldest messages
    if (FrameCounter > NetworkDelay) {
	NetworkDelay += NetworkUpdates;
	if (!dl_empty(CommandsOut)) {
	    DebugLevel3Fn("cycle %lu vi %d\n", GameCycle, VideoInterrupts);
	    NetworkResendCommands();
	}
    }
}

//@}

//@}
