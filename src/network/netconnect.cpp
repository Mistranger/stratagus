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
/**@name netconnect.c	-	The network high level connection code. */
//
//	(c) Copyright 2001,2002 by Lutz Sammer, Andreas Arens.
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
//	Includes
//----------------------------------------------------------------------------

#include <stdio.h>

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "freecraft.h"

#include "net_lowlevel.h"
#include "player.h"
#include "map.h"
#include "network.h"
#include "netconnect.h"
#include "interface.h"
#include "menus.h"


//----------------------------------------------------------------------------
//	Declaration
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	Variables
//----------------------------------------------------------------------------

global char *NetworkArg;		/// Network command line argument
global int NetPlayers;			/// How many network players
global int NetworkPort = NetworkDefaultPort;	/// Local network port to use

IfDebug(
extern unsigned long MyHost;		/// My host number.
extern int MyPort;			/// My port number.
);

global int HostsCount;			/// Number of hosts.
global NetworkHost Hosts[PlayerMax];	/// Host and ports of all players.

global NetworkState NetStates[PlayerMax];/// Network menu: Server: Client Host states
global int NetLocalHostsSlot;		/// Network menu: Slot # in Hosts array of local client
global char NetworkName[16];		/// Network menu: Name of local player
global int NetConnectRunning;		/// Network menu: Setup mode active
global unsigned char NetLocalState;	/// Network menu: Local Server/Client connect state;

global int NetStateMsgCnt;		/// Number of consecutive msgs of same type sent
local unsigned char LastStateMsgType;	/// Subtype of last InitConfig message sent
local unsigned long NetLastPacketSent;	/// Tick the last network packet was sent
local unsigned long NetworkServerIP;	/// Network Client: IP of server to join

/// FIXME ARI: The following is a kludge to have some way to override the default port
/// on the server to connect to. Should be selectable by advanced network menus.
/// For now just specify with the -P port command line arg...
local int NetworkServerPort = NetworkDefaultPort; /// Server network port to use


/**@name api */
//@{


//----------------------------------------------------------------------------
//	OLD API init..
//----------------------------------------------------------------------------

/**
**	Setup command line connection parameters
*/
global void NetworkSetupArgs(void)
{
    int i;
    char* s;

    DebugLevel0Fn("%d players\n", NetPlayers);
    DebugLevel0Fn("%s arg\n", NetworkArg);

    if (NetworkPort == NetworkDefaultPort && NetworkArg) {
	i = strtol(NetworkArg, &s, 0);
	if (s != NetworkArg && (*s == ':' || *s == '\0')) {
	    if (*s == ':')
		s++;
	    NetworkArg = s;
	    NetworkPort = i;
	}
    }
}

/**
**	Server Setup.
*/
global void NetworkServerSetup(WorldMap *map)
{
    int i, j, n;
    char buf[1024];
    InitMessage *msg;
    InitMessage message;
    Acknowledge acknowledge;
    int num[PlayerMax];

    // Prepare reply message:
    message.Type = MessageInitConfig;
    message.SubType = ICMGameFull;	// default: reject connnection

    message.FreeCraft = htonl(FreeCraftVersion);
    message.Version = htonl(NetworkProtocolVersion);
    message.Lag = htonl(NetworkLag);
    message.Updates = htonl(NetworkUpdates);
    if (map->Info) {
	message.MapUID = htonl(map->Info->MapUID);
    } else {
	message.MapUID = 0L;
    }

    //
    //	Wait for all clients to connect.
    //
    DebugLevel1Fn("Waiting for %d client(s)\n", NetPlayers - 1);
    acknowledge.Type = MessageInitReply;
    msg = (InitMessage *)buf;
    for (i = 1; i < NetPlayers;) {
	if ((n = NetRecvUDP(NetworkFildes, &buf, sizeof(buf))) < 0) {
	    ExitFatal(-1);
	}

	if (msg->Type != MessageInitHello || n != sizeof(*msg)) {
	    DebugLevel0Fn("Wrong message\n");
	    continue;
	}

	// FIXME: ARI: Ignoring all non-supported subtypes here - add chat, client-quit, etc...
	if (msg->SubType != ICMHello) {
	    DebugLevel0Fn("Unsupported InitHello Subtype %d\n", (int)msg->SubType);
	    continue;
	}

	DebugLevel0Fn("Received InitHello %d:%d(%d) from %d.%d.%d.%d:%d\n",
		msg->Type, msg->SubType, n, NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));

	if (ntohl(msg->FreeCraft) != FreeCraftVersion) {
	    fprintf(stderr, "Incompatible FreeCraft version "
			FreeCraftFormatString " <-> "
			FreeCraftFormatString "\n",
		    FreeCraftFormatArgs((int)ntohl(msg->FreeCraft)),
		    FreeCraftFormatArgs(FreeCraftVersion));
	    message.SubType = ICMEngineMismatch; // FreeCraft engine version doesn't match
	    n = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &message,
			sizeof(message));
	    DebugLevel0Fn("Sending InitConfig Message EngineMismatch: (%d) to %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
	    continue;
	    // ExitFatal(-1);
	}

	if (ntohl(msg->Version) != NetworkProtocolVersion) {
	    fprintf(stderr, "Incompatible network protocol version "
			NetworkProtocolFormatString " <-> "
			NetworkProtocolFormatString "\n",
		    NetworkProtocolFormatArgs((int)ntohl(msg->Version)),
		    NetworkProtocolFormatArgs(NetworkProtocolVersion));
	    message.SubType = ICMProtocolMismatch; // Network protocol version doesn't match
	    n = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &message,
			sizeof(message));
	    DebugLevel0Fn("Sending InitConfig Message ProtocolMismatch: (%d) to %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
	    continue;
	    // ExitFatal(-1);
	}
	n = 0;
	if (map->Info) {
	    if (ntohl(msg->MapUID) != map->Info->MapUID) {
		n = 1;
	    }
	} else {
	    if (ntohl(msg->MapUID) != 0L) {
		n = 1;
	    }
	}
	if (n) {
	    fprintf(stderr,
		    "FreeCraft maps do not match (0x%08x) <-> (0x%08x)\n",
			map->Info ? (unsigned int)map->Info->MapUID : 0,
			(unsigned int)ntohl(msg->MapUID));
	    message.SubType = ICMMapUidMismatch; // MAP Uid  doesn't match
	    n = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &message,
			sizeof(message));
	    DebugLevel0Fn("Sending InitConfig Message MapUIDMismatch: (%d) to %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
	    continue;
	    // ExitFatal(-1);
	}

	DebugLevel0Fn("Version=" FreeCraftFormatString
		    ", Network=" NetworkProtocolFormatString
		    ", Lag=%ld, Updates=%ld\n",
	    FreeCraftFormatArgs((int)ntohl(msg->FreeCraft)),
	    NetworkProtocolFormatArgs((int)ntohl(msg->Version)),
	    (long)ntohl(msg->Lag), (long)ntohl(msg->Updates));

	//
	//	Warning: Server should control it!
	//
	if (ntohl(msg->Lag) != NetworkLag) {
	    fprintf(stderr, "Incompatible network lag %ld-%d\n",
		    (long)ntohl(msg->Lag), NetworkLag);
	}

	if (ntohl(msg->Updates) != NetworkUpdates) {
	    fprintf(stderr, "Incompatible network updates %ld-%d\n",
		(long)ntohl(msg->Updates), NetworkUpdates);
	}

	// Lookup, if host is already known.
	for (n = 0; n < HostsCount; ++n) {
	    if (Hosts[n].Host == NetLastHost && Hosts[n].Port == NetLastPort) {
		break;
	    }
	}

	// A new client
	if (n == HostsCount) {
	    Hosts[HostsCount].Host = NetLastHost;
	    Hosts[HostsCount].Port = NetLastPort;
	    memcpy(Hosts[HostsCount].PlyName, msg->u.Hosts[0].PlyName, 16);
	    DebugLevel0Fn("New client %d.%d.%d.%d:%d [%s]\n",
		    NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort), Hosts[HostsCount].PlyName);
	    HostsCount++;
	    ++i;
	}

	// Acknowledge the packet.
	n = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &acknowledge,
		sizeof(acknowledge));
	DebugLevel0Fn("Sending ACK for InitHello (%d)\n", n);
    }

    //
    //	Assign the players.
    //
    // FIXME: ARI: selectable by 'Position' selector in Network setup menus!

    // Make a list of the player slots.
    for (n = i = 0; i < NumPlayers; ++i) {
	if (Players[i].Type == PlayerPerson) {
	    num[n] = i;
	    DebugLevel3Fn("Player slot %i is available for a person\n", i);
	    n++;
	}
    }
    if (n < NetPlayers) {
	fprintf(stderr, "Not enough person slots\n");
	ExitFatal(-1);
    }

    // Randomize them.
    for(i = 0; i<NetPlayers; ++i)  {
      if(n>0) {
	int chosen=MyRand()%n;

	DebugLevel2Fn("Assigning player %i to slot %i\n", i, chosen);
	Hosts[i].PlyNr=num[chosen];
	num[chosen]=num[n-1];
	--n;
      } else {
	Hosts[i].PlyNr=num[0];
      }
    }

    //
    //	Send all clients host:ports to all clients.
    //

    // Prepare config message:
    message.SubType = ICMConfig;
    message.HostsCount = HostsCount + 1;
    for (i = 0; i < HostsCount; ++i) {
	message.u.Hosts[i].Host = Hosts[i].Host;
	message.u.Hosts[i].Port = Hosts[i].Port;
	memcpy(message.u.Hosts[i].PlyName, Hosts[i].PlyName, 16);
	message.u.Hosts[i].PlyNr = htons(Hosts[i].PlyNr);
	PlayerSetName(&Players[Hosts[i].PlyNr], Hosts[i].PlyName);
    }
    message.u.Hosts[i].Host = 0;
    message.u.Hosts[i].Port = 0;	// marks the server
    memcpy(message.u.Hosts[i].PlyName, NetworkName, 16);
    message.u.Hosts[i].PlyNr = htons(Hosts[i].PlyNr);

    DebugLevel3Fn("Player here %d\n", Hosts[i].PlyNr);
    ThisPlayer = &Players[Hosts[i].PlyNr];
    PlayerSetName(ThisPlayer, NetworkName);

    DebugLevel1Fn("Ready, sending InitConfig to %d host(s)\n", HostsCount);
    //
    //	Send all clients host:ports to all clients.
    //
    for (j = HostsCount; j;) {

	// Send to all clients.
	for (i = 0; i < HostsCount; ++i) {
	    if (num[i] != -1) {		// already acknowledged
		unsigned long host;
		int port;

		host = message.u.Hosts[i].Host;
		port = message.u.Hosts[i].Port;
		message.u.Hosts[i].Host = message.u.Hosts[i].Port = 0;
		n = NetSendUDP(NetworkFildes, host, port, &message,
			sizeof(message));
		DebugLevel0Fn("Sending InitConfig Message Config (%d) to %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(host)), ntohs(port));
		message.u.Hosts[i].Host = host;
		message.u.Hosts[i].Port = port;
	    }
	}

	// Wait for acknowledge
	while (j && NetSocketReady(NetworkFildes, 1000)) {
	    if ((n = NetRecvUDP(NetworkFildes, &buf, sizeof(buf))) < 0) {
		DebugLevel0Fn("*Receive ack failed: (%d) from %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));
		continue;
	    }

	// DebugLevel0Fn("Received ack %d(%d) from %d.%d.%d.%d:%d\n",
	//	    msg->Type,n,
	//	    NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));

	    if (msg->Type == MessageInitHello && n == sizeof(*msg)) {
		DebugLevel0Fn("Acknowledge for InitHello was lost\n");

		// Acknowledge the hello packets.
		acknowledge.Type = MessageInitReply;
		n = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &acknowledge,
			sizeof(acknowledge));
		DebugLevel0Fn("Sending ack for InitHello (%d)\n", n);
	    } else {
		DebugLevel0Fn("Got ack for InitConfig: (%d) from %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));

		for (i = 0; i < HostsCount; ++i) {
		    if (NetLastHost == Hosts[i].Host
			    && NetLastPort == Hosts[i].Port
			    && msg->Type == MessageInitReply
			    && n == 1) {
			if (num[i] != -1) {
			    num[i] = -1;
			    j--;
			    DebugLevel0Fn("Removing host (j = %d)\n", j);
			} else {
			    DebugLevel0Fn("Already removed host\n");
			}
			break;
		    }
		}
	    }
	}
    }
    DebugLevel0Fn("DONE: All configs acked - starting game\n");
}

/**
**	Client Setup.
*/
global void NetworkClientSetup(WorldMap *map)
{
    char buf[1024];
    InitMessage *msg;
    InitMessage message;
    Acknowledge acknowledge;
    unsigned long host;
    int port;
    char *cp;
    int i;
    int n;

    // Parse server address.
    cp = strchr(NetworkArg, ':');
    if (cp) {
	*cp = '\0';
	port = htons(atoi(cp + 1));
	NetworkServerPort = htons(port);
	host = NetResolveHost(NetworkArg);
	*cp = ':';
    } else {
	port = htons(NetworkPort);
	NetworkServerPort = htons(port);
	host = NetResolveHost(NetworkArg);
    }
    if (host == INADDR_NONE) {
	fprintf(stderr, "Can't resolve host %s\n", NetworkArg);
	ExitFatal(-1);
    }
    DebugLevel0Fn("Server host:port %d.%d.%d.%d:%d\n",
	    NIPQUAD(ntohl(host)), ntohs(port));

    //
    //	Connecting to server
    //

    HostsCount = 0;

    message.Type = MessageInitHello;
    message.SubType = ICMHello;
    message.FreeCraft = htonl(FreeCraftVersion);
    message.Version = htonl(NetworkProtocolVersion);
    message.Lag = htonl(NetworkLag);
    message.Updates = htonl(NetworkUpdates);
    memcpy(message.u.Hosts[0].PlyName, NetworkName, 16);
    if (map->Info) {
	message.MapUID = htonl(map->Info->MapUID);
    } else {
	message.MapUID = 0L;
    }
    msg = (InitMessage *)buf;
    for (;;) {
	n = NetSendUDP(NetworkFildes, host, port, &message, sizeof(message));
	DebugLevel0Fn("Sending hello (%d)\n", n);

	// Wait for answer (timeout 1/2s)
	if (NetSocketReady(NetworkFildes, 500)) {
	    if ((n = NetRecvUDP(NetworkFildes, &buf, sizeof(buf))) < 0) {
		ExitFatal(-1);
	    }
	    DebugLevel0Fn("Received reply? %d:%d(%d) %d.%d.%d.%d:%d\n",
		    msg->Type, msg->SubType, n,
		    NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));

	    IfDebug(
		if (NetLastHost == MyHost && NetLastPort == MyPort) {
		    fprintf(stderr, "Network client setup: Talking to myself!\n");
		    ExitFatal(-1);
		}
	    );

	    if (NetLastHost == host && NetLastPort == port) {
		if (msg->Type == MessageInitReply && n == 1) {
		    break;
		}
		if (msg->Type == MessageInitConfig && n == sizeof(InitMessage)) {
		    if (msg->SubType == ICMConfig) {
			// lost ACK - but Config got through!
			break;
		    }
		    switch(msg->SubType) {
			case ICMEngineMismatch:
			    fprintf(stderr, "Incompatible FreeCraft version "
					FreeCraftFormatString " <-> "
					FreeCraftFormatString "\n",
				    FreeCraftFormatArgs((int)ntohl(msg->FreeCraft)),
				    FreeCraftFormatArgs(FreeCraftVersion));
			    ExitFatal(-1);

			case ICMProtocolMismatch:
			    fprintf(stderr, "Incompatible network protocol version "
					NetworkProtocolFormatString " <-> "
					NetworkProtocolFormatString "\n",
				    NetworkProtocolFormatArgs((int)ntohl(msg->Version)),
				    NetworkProtocolFormatArgs(NetworkProtocolVersion));
			    ExitFatal(-1);

			case ICMEngineConfMismatch:	// FIXME: Not Implemented yet
			    ExitFatal(-1);

			case ICMMapUidMismatch:
			    fprintf(stderr, "FreeCraft maps do not match (0x%08x) <-> (0x%08x)\n",
					map->Info ?
					    (unsigned int)map->Info->MapUID : 0,
					    (unsigned int)ntohl(msg->MapUID));
			    ExitFatal(-1);

			case ICMGameFull:
			    fprintf(stderr, "Server is full!\n");
			    ExitFatal(-1);

			case ICMServerQuit:
			    fprintf(stderr, "Server has quit!\n");
			    ExitFatal(-1);
		    }
		}
	    }

	    DebugLevel0Fn("Received wrong packet\n");
	}
    }

    //
    //	Wait for addresses of other clients.
    //
    DebugLevel0Fn("Waiting for ClientConfig\n");
    acknowledge.Type = MessageInitReply;
    for (;;) {
	if ((n = NetRecvUDP(NetworkFildes, &buf, sizeof(buf))) < 0) {
	    ExitFatal(-1);
	}

	DebugLevel0Fn("Received config? %d(%d) %d.%d.%d.%d:%d\n",
		msg->Type,n,
		NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));

	if (NetLastHost != host || NetLastPort != port
		|| msg->Type != MessageInitConfig || n != sizeof(InitMessage)) {
	    DebugLevel0Fn("Received wrong packet\n");
	    continue;
	}

	// FIXME: ARI: add switch over SubType here (ServerQuit, etc)

	DebugLevel0Fn("Received ClientConfig (HostsCount = %d)\n", (int)msg->HostsCount);

	NetworkLag = ntohl(msg->Lag);
	NetworkUpdates = ntohl(msg->Updates);

	for (i = 0; i < msg->HostsCount - 1; ++i) {
	    if (msg->u.Hosts[i].Host || msg->u.Hosts[i].Port) {
		Hosts[HostsCount].Host = msg->u.Hosts[i].Host;
		Hosts[HostsCount].Port = msg->u.Hosts[i].Port;
		Hosts[HostsCount].PlyNr = ntohs(msg->u.Hosts[i].PlyNr);
		memcpy(Hosts[HostsCount].PlyName, msg->u.Hosts[i].PlyName, 16);
		PlayerSetName(&Players[Hosts[HostsCount].PlyNr], Hosts[HostsCount].PlyName);
		HostsCount++;
		DebugLevel0Fn("Client %d = %d.%d.%d.%d:%d [%s]\n",
			ntohs(msg->u.Hosts[i].PlyNr), NIPQUAD(ntohl(msg->u.Hosts[i].Host)),
			ntohs(msg->u.Hosts[i].Port), msg->u.Hosts[i].PlyName);
	    } else {			// Own client
		DebugLevel0Fn("SELF %d [%s]\n", ntohs(msg->u.Hosts[i].PlyNr),
			msg->u.Hosts[i].PlyName);
		ThisPlayer = &Players[ntohs(msg->u.Hosts[i].PlyNr)];
		PlayerSetName(ThisPlayer, NetworkName);
	    }
	}

	Hosts[HostsCount].Host = host;
	Hosts[HostsCount].Port = port;
	DebugLevel0Fn("SERVER %d [%s]\n", ntohs(msg->u.Hosts[i].PlyNr),
		msg->u.Hosts[i].PlyName);
	Hosts[HostsCount].PlyNr = ntohs(msg->u.Hosts[i].PlyNr);
	memcpy(Hosts[HostsCount].PlyName, msg->u.Hosts[i].PlyName, 16);
	PlayerSetName(&Players[Hosts[HostsCount].PlyNr], Hosts[HostsCount].PlyName);
	HostsCount++;

	// Acknowledge the packet.
	n = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &acknowledge,
		sizeof(acknowledge));
	DebugLevel0Fn("Sending ClientConfig ack (%d)\n", n);
	break;
    }

    //
    //	Wait for lost acknowledge (timeout 3s)
    //
    while (NetSocketReady(NetworkFildes, 3000)) {
	if ((n = NetRecvUDP(NetworkFildes, &buf, sizeof(buf))) < 0) {
	    ExitFatal(-1);
	}

	DebugLevel3Fn("Received config? %d(%d) %d.%d.%d.%d:%d\n",
		msg->Type, n,
		NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));

	if (NetLastHost == host && NetLastPort == port
		&& msg->Type == MessageInitConfig && n == sizeof(InitMessage)) {

	    DebugLevel0Fn("Received DUP ClientConfig (SERVER LOST ACK)\n");

	    // Acknowledge the packets.
	    n = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &acknowledge,
		    sizeof(acknowledge));
	    DebugLevel0Fn("Sending DUP ClientConfig ack (%d)\n", n);
	    continue;
	}

	// if (msg->Type > MessageInitConfig && n == sizeof(NetworkPacket))
	if (msg->Type > MessageInitConfig) {
	    DebugLevel0Fn("Game message received - server has started game\n");
	    break;
	}
    }
    DebugLevel3Fn("DONE: All client have received config - starting game\n");
}


//----------------------------------------------------------------------------
//	NEW API
//----------------------------------------------------------------------------

/**
**	Send an InitConfig message across the Network
**
**	@param host	Host to send to (network byte order).
**	@param port	Port of host to send to (network byte order).
**	@param msg	The message to send
*/
local int NetworkSendICMessage(unsigned long host, int port, InitMessage *msg)
{
    msg->FreeCraft = htonl(FreeCraftVersion);
    msg->Version = htonl(NetworkProtocolVersion);
    msg->Lag = htonl(NetworkLag);
    msg->Updates = htonl(NetworkUpdates);
    return NetSendUDP(NetworkFildes, host, port, msg, sizeof(*msg));
}

#ifdef DEBUG
local const char *ncconstatenames[] = {
    "ccs_unused",
    "ccs_connecting",		// new client
    "ccs_connected",		// has received slot info
    "ccs_mapinfo",		// has received matching map-info
    "ccs_badmap",		// has received non-matching map-info
    "ccs_synced",
    "ccs_async",		// server user has changed selection
    "ccs_changed",		// client user has made menu selection
    "ccs_detaching",		// client user wants to detach
    "ccs_disconnected",		// client has detached
    "ccs_unreachable",		// Server is unreachable
    "ccs_usercanceled",		// User canceled game
    "ccs_nofreeslots",		// Server has no more free slots
    "ccs_serverquits",		// Server quits
    "ccs_incompatibleengine",	// Incompatible engine version
    "ccs_incompatiblenetwork",	// Incompatible netowrk version
};

local const char *icmsgsubtypenames[] = {
    "Hello",			// Client Request
    "Config",			// Setup message configure clients

    "EngineMismatch",		// FreeCraft engine version doesn't match
    "ProtocolMismatch",		// Network protocol version doesn't match
    "EngineConfMismatch",	// Engine configuration isn't identical
    "MapUidMismatch",		// MAP UID doesn't match

    "GameFull",			// No player slots available
    "Welcome",			// Acknowledge for new client connections

    "Waiting",			// Client has received Welcome and is waiting for Map/State
    "Map",			// MapInfo (and Mapinfo Ack)
    "State",			// StateInfo
    "Resync",			// Ack StateInfo change

    "ServerQuit",		// Server has quit game
    "GoodBye",			// Client wants to leave game
    "SeeYou",			// Client has left game
};
#endif

/**
**	Send a message to the server, but only if the last packet was a while
**	ago
**
**	@param msg	The message to send
**	@param msecs	microseconds to delay
*/
local void NetworkSendRateLimitedClientMessage(InitMessage *msg, long msecs)
{
    unsigned long now;
    int n;

    now = GetTicks();
    if (now - NetLastPacketSent >= msecs) {
	NetLastPacketSent = now;
	if (msg->SubType == LastStateMsgType) {
	    NetStateMsgCnt++;
	} else {
	    NetStateMsgCnt = 0;
	    LastStateMsgType = msg->SubType;
	}
	n = NetworkSendICMessage(NetworkServerIP, htons(NetworkServerPort), msg);
#ifdef DEBUG
	DebugLevel0Fn("Sending Init Message (%s:%d): %d:%d(%d) %d.%d.%d.%d:%d\n",
		ncconstatenames[NetLocalState], NetStateMsgCnt, msg->Type,
		msg->SubType, n,
		NIPQUAD(ntohl(NetworkServerIP)), NetworkServerPort);
#endif
    }
}

/**
**	Setup the IP-Address of the network server to connect to
**
**	@param serveraddr	the serveraddress the user has entered
**	@param ipbuf		buffer to store the text representation of the IP-address
**
**	@return			True, if error; otherwise false.
*/
global int NetworkSetupServerAddress(const char *serveraddr, char *ipbuf)
{
    unsigned long addr;

    addr = NetResolveHost(serveraddr);
    if (addr == INADDR_NONE) {
	return 1;
    }
    NetworkServerIP = addr;

    DebugLevel1Fn("SELECTED SERVER: %s (%d.%d.%d.%d)\n", serveraddr,
		    NIPQUAD(ntohl(addr)));

    sprintf(ipbuf, "%d.%d.%d.%d", NIPQUAD(ntohl(addr)));
    return 0;
}

/**
**	Setup Network connect state machine for clients
*/
global void NetworkInitClientConnect(void)
{
    int i;

    NetConnectRunning = 2;
    NetLastPacketSent = GetTicks();
    NetLocalState = ccs_connecting;
    NetStateMsgCnt = 0;
    LastStateMsgType = ICMServerQuit;
    for (i = 0; i < PlayerMax; ++i) {
	Hosts[i].Host = 0;
	Hosts[i].Port = 0;
	Hosts[i].PlyNr = 0;
	memset(Hosts[i].PlyName, 0, 16);
    }
    HostsCount = 0;
}

/**
**	Terminate Network connect state machine for clients
*/
global void NetworkExitClientConnect(void)
{
    NetConnectRunning = 0;
    NetPlayers = 0;		// Make single player menus work again!
}

/**
**	Terminate and detach Network connect state machine for the client
*/
global void NetworkDetachFromServer(void)
{
    NetLocalState = ccs_detaching;
    NetStateMsgCnt = 0;
}

/**
**	Setup Network connect state machine for the server
*/
global void NetworkInitServerConnect(void)
{
    int i;

    NetConnectRunning = 1;

    DebugLevel1Fn("Waiting for %d client(s)\n", NetPlayers - 1);

    for (i = 0; i < NetPlayers; ++i) {
	NetStates[i].State = ccs_unused;
	Hosts[i].Host = 0;
	Hosts[i].Port = 0;
	Hosts[i].PlyNr = 0;		// slotnr until final cfg msg
	memset(Hosts[i].PlyName, 0, 16);
    }

    HostsCount = 0;
    // preset the server (always slot 0)
    memcpy(Hosts[HostsCount].PlyName, NetworkName, 16);
    HostsCount++;
}

/**
**	Terminate Network connect state machine for the server
*/
global void NetworkExitServerConnect(void)
{
    int h, i, n;
    InitMessage message;

    message.Type = MessageInitReply;
    message.SubType = ICMServerQuit;
    for (h = 1; h < HostsCount; ++h) {
	// Spew out 5 and trust in God that they arrive
	if (Hosts[h].PlyNr) {
	    for (i = 0; i < 5; i++) {
		n = NetworkSendICMessage(Hosts[h].Host, Hosts[h].Port, &message);
		DebugLevel0Fn("Sending InitReply Message ServerQuit: (%d) to %d.%d.%d.%d:%d\n",
						n, NIPQUAD(ntohl(Hosts[h].Host)),ntohs(Hosts[h].Port));
	    }
	}
    }
    NetConnectRunning = 0;
}

/**
**	Notify state change by menu user to connected clients
*/
global void NetworkServerResyncClients(void)
{
    int i;

    if (NetConnectRunning) {
	for (i = 1; i < NetPlayers; ++i) {
	    if (Hosts[i].PlyNr && NetStates[i].State == ccs_synced) {
		NetStates[i].State = ccs_async;
	    }
	}
    }
}


/**
**	Client Menu Loop: Send out client request messages
*/
global void NetworkProcessClientRequest(void)
{
    InitMessage message;
    int i;

changed:
    switch (NetLocalState) {
	case ccs_disconnected:
	    message.Type = MessageInitHello;
	    message.SubType = ICMSeeYou;
	    // Spew out 5 and trust in God that they arrive
	    for (i = 0; i < 5; i++) {
		NetworkSendICMessage(NetworkServerIP, htons(NetworkServerPort), &message);
	    }
	    NetLocalState = ccs_unreachable;
	    NetConnectRunning = 0;	// End the menu..
	    break;
	case ccs_detaching:
	    if (NetStateMsgCnt < 10) {	// 10 retries = 1 second
		message.Type = MessageInitHello;
		message.SubType = ICMGoodBye;
		NetworkSendRateLimitedClientMessage(&message, 100);
	    } else {
		// Server is ignoring us - break out!
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_detaching: Above message limit %d\n", NetStateMsgCnt);
	    }
	    break;
	case ccs_connecting:		// connect to server
	    if (NetStateMsgCnt < 60) {	// 60 retries = 30 seconds
		message.Type = MessageInitHello;
		message.SubType = ICMHello;
		memcpy(message.u.Hosts[0].PlyName, NetworkName, 16);
		message.MapUID = 0L;
		NetworkSendRateLimitedClientMessage(&message, 500);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_connecting: Above message limit %d\n", NetStateMsgCnt);
	    }
	    break;
	case ccs_connected:
	    if (NetStateMsgCnt < 20) {	// 20 retries
		message.Type = MessageInitHello;
		message.SubType = ICMWaiting;
		NetworkSendRateLimitedClientMessage(&message, 650);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_connected: Above message limit %d\n", NetStateMsgCnt);
	    }
	    break;
	case ccs_synced:
	    NetClientCheckLocalState();
	    if (NetLocalState != ccs_synced) {
		NetStateMsgCnt = 0;
		goto changed;
	    }
	    message.Type = MessageInitHello;
	    message.SubType = ICMWaiting;
	    NetworkSendRateLimitedClientMessage(&message, 850);
	    break;
	case ccs_changed:
	    if (NetStateMsgCnt < 20) {						// 20 retries
		message.Type = MessageInitHello;
		message.SubType = ICMState;
		message.u.State = LocalSetupState;
		message.HostsCount = (char)(NetLocalHostsSlot & 0xff);
		message.MapUID = htonl(ScenSelectPudInfo->MapUID);
		NetworkSendRateLimitedClientMessage(&message, 450);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_changed: Above message limit %d\n", NetStateMsgCnt);
	    }
	    break;
	case ccs_async:
	    if (NetStateMsgCnt < 20) {						// 20 retries
		message.Type = MessageInitHello;
		message.SubType = ICMResync;
		NetworkSendRateLimitedClientMessage(&message, 450);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_async: Above message limit %d\n", NetStateMsgCnt);
	    }
	    break;
	case ccs_mapinfo:
	    if (NetStateMsgCnt < 20 && ScenSelectPudInfo != NULL) {		// 20 retries
		message.Type = MessageInitHello;
		message.SubType = ICMMap;					// ICMMapAck..
		message.MapUID = htonl(ScenSelectPudInfo->MapUID);
		NetworkSendRateLimitedClientMessage(&message, 650);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_mapinfo: Above message limit %d\n", NetStateMsgCnt);
	    }
	case ccs_badmap:
	    if (NetStateMsgCnt < 20) {	// 20 retries
		message.Type = MessageInitHello;
		message.SubType = ICMMapUidMismatch;
		if (ScenSelectPudInfo) {
		    message.MapUID = htonl(ScenSelectPudInfo->MapUID);		// MAP Uid doesn't match
		} else {
		    message.MapUID = 0L;					// Map not found
		}
		NetworkSendRateLimitedClientMessage(&message, 650);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_badmap: Above message limit %d\n", NetStateMsgCnt);
	    }
	    break;
	default:
	    break;
    }
}

/**
**	Parse a network menu packet in client disconneccted state.
**
**	@param msg	message received
*/
local void ParseClientDisconnected(const InitMessage* msg)
{
    // FIXME: Split NetworkParseMenuPacket into small functions.
    DebugLevel0Fn("ccs_disconnected: Server sending GoodBye dups %d\n" _C_
	    msg->SubType);
}

/**
**	Parse a Network menu packet.
**
**	@param msg	message received
**	@param size	size of the received packet.
*/
local void NetworkParseMenuPacket(const InitMessage *msg, int size)
{
    int i, h, n;
    InitMessage message;

    if (msg->Type > MessageInitConfig || size != sizeof(*msg)) {
	DebugLevel0Fn("Wrong message\n");
	return;
    }
    DebugLevel0Fn("Received %s Init Message %d:%d (%d) from %d.%d.%d.%d:%d (%ld)\n",
	    icmsgsubtypenames[msg->SubType], msg->Type, msg->SubType, size, NIPQUAD(ntohl(NetLastHost)),
	    ntohs(NetLastPort), FrameCounter);

    if (NetConnectRunning == 2) {		// client
	if (msg->Type == MessageInitReply) {
	    if (msg->SubType == ICMServerQuit) { // Server user canceled, should work in all states
		NetLocalState = ccs_serverquits;
		NetConnectRunning = 0;	// End the menu..
		// No ack here - Server will spew out a few Quit msgs, which has to be enough
		return;
	    }
	    switch(NetLocalState) {
		case ccs_disconnected:
		    DebugLevel0Fn("ccs_disconnected: Server sending GoodBye dups %d\n",msg->SubType);
		    break;

		case ccs_detaching:
		    switch(msg->SubType) {

			case ICMGoodBye:	// Server has let us go
			    DebugLevel3Fn("ccs_detaching: Server GoodBye subtype %d received - byebye\n",msg->SubType);
			    NetLocalState = ccs_disconnected;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_detaching: Unhandled subtype %d\n",msg->SubType);
			    break;
		    }
		    break;

		case ccs_connecting:
		    switch(msg->SubType) {

			case ICMEngineMismatch: // FreeCraft engine version doesn't match
			    fprintf(stderr, "Incompatible FreeCraft version "
					FreeCraftFormatString " <-> "
					FreeCraftFormatString "\n"
					"from %d.%d.%d.%d:%d\n",
				    FreeCraftFormatArgs((int)ntohl(msg->FreeCraft)),
				    FreeCraftFormatArgs(FreeCraftVersion),
				    NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
			    NetLocalState = ccs_incompatibleengine;
			    NetConnectRunning = 0;	// End the menu..
			    return;

			case ICMProtocolMismatch: // Network protocol version doesn't match
			    fprintf(stderr, "Incompatible network protocol version "
					NetworkProtocolFormatString " <-> "
					NetworkProtocolFormatString "\n"
					"from %d.%d.%d.%d:%d\n",
				    NetworkProtocolFormatArgs((int)ntohl(msg->Version)),
				    NetworkProtocolFormatArgs(NetworkProtocolVersion),
				    NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
			    NetLocalState = ccs_incompatiblenetwork;
			    NetConnectRunning = 0;	// End the menu..
			    return;

			case ICMGameFull:	// Game is full - server rejected connnection
			    fprintf(stderr, "Server at %d.%d.%d.%d:%d is full!\n",
				    NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
			    NetLocalState = ccs_nofreeslots;
			    NetConnectRunning = 0;	// End the menu..
			    return;

			case ICMWelcome:	// Server has accepted us
			    NetLocalState = ccs_connected;
			    NetStateMsgCnt = 0;
			    NetLocalHostsSlot = msg->u.Hosts[0].PlyNr;
			    memcpy(Hosts[0].PlyName, msg->u.Hosts[0].PlyName, 16); // Name of server player
			    Hosts[0].Host = NetworkServerIP;
			    Hosts[0].Port = htons(NetworkServerPort);
			    HostsCount = msg->HostsCount;
			    for (i = 1; i < HostsCount; i++) {
				if (i != NetLocalHostsSlot) {
				    Hosts[i].Host = msg->u.Hosts[i].Host;
				    Hosts[i].Port = msg->u.Hosts[i].Port;
				    Hosts[i].PlyNr = i;
				    memcpy(Hosts[i].PlyName, msg->u.Hosts[i].PlyName, 16);
				} else {
				    Hosts[i].PlyNr = i;
				    memcpy(Hosts[i].PlyName, NetworkName, 16);
				}
			    }
			    break;

			default:
			    DebugLevel0Fn("ccs_connecting: Unhandled subtype %d\n",msg->SubType);
			    break;

		    }
		    break;

		case ccs_connected:
		    switch(msg->SubType) {

			case ICMMap:		// Server has sent us new map info
			    memcpy(ScenSelectFullPath, msg->u.MapPath, 256);
			    ScenSelectFullPath[255] = 0;
			    if (NetClientSelectScenario()) {
				NetLocalState = ccs_badmap;
				break;
			    }
			    if (ntohl(msg->MapUID) != ScenSelectPudInfo->MapUID) {
				NetLocalState = ccs_badmap;
				fprintf(stderr,
				    "FreeCraft maps do not match (0x%08x) <-> (0x%08x)\n",
					    (unsigned int)ScenSelectPudInfo->MapUID,
					    (unsigned int)ntohl(msg->MapUID));
				break;
			    }
			    NetLocalState = ccs_mapinfo;
			    NetStateMsgCnt = 0;
			    NetConnectRunning = 0;	// Kick the menu..
			    break;

			case ICMWelcome:	// Server has accepted us (dup)
			    DebugLevel3Fn("ccs_connected: DUP Welcome subtype %d\n",msg->SubType);
			    break;

			default:
			    DebugLevel0Fn("ccs_connected: Unhandled subtype %d\n",msg->SubType);
			    break;
		    }
		    break;

		case ccs_mapinfo:
		    switch(msg->SubType) {

			case ICMState:		// Server has sent us first state info
			    DebugLevel3Fn("ccs_mapinfo: Initial State subtype %d received - going sync\n",msg->SubType);
			    ServerSetupState = msg->u.State;
			    NetClientUpdateState();
			    NetLocalState = ccs_synced;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_mapinfo: Unhandled subtype %d\n",msg->SubType);
			    break;
		    }
		    break;

		case ccs_changed:
		case ccs_synced:
		    switch(msg->SubType) {

			case ICMState:		// Server has sent us new state info
			    DebugLevel3Fn("ccs_synced: New State subtype %d received - resynching\n",msg->SubType);
			    ServerSetupState = msg->u.State;
			    NetClientUpdateState();
			    NetLocalState = ccs_async;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_synced: Unhandled subtype %d\n",msg->SubType);
			    break;
		    }
		    break;

		case ccs_async:
		    switch(msg->SubType) {

			case ICMResync:		// Server has resynced with us and sends resync data
			    HostsCount = msg->HostsCount;
			    for (i = 1; i < HostsCount; i++) {
				if (i != NetLocalHostsSlot) {
				    Hosts[i].Host = msg->u.Hosts[i].Host;
				    Hosts[i].Port = msg->u.Hosts[i].Port;
				    Hosts[i].PlyNr = msg->u.Hosts[i].PlyNr;
				    memcpy(Hosts[i].PlyName, msg->u.Hosts[i].PlyName, 16);
				    DebugLevel3Fn("Other client %d: %s\n", Hosts[i].PlyNr, Hosts[i].PlyName);
				} else {
				    Hosts[i].PlyNr = i;
				    memcpy(Hosts[i].PlyName, NetworkName, 16);
				}
			    }
			    NetClientUpdateState();
			    NetLocalState = ccs_synced;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_async: Unhandled subtype %d\n",msg->SubType);
			    break;
		    }
		    break;

		default:
		    DebugLevel0Fn("Client: Unhandled state %d\n", NetLocalState);
		    break;
	    }
	}

    } else if (NetConnectRunning == 1) {	// server

	if (ntohl(msg->FreeCraft) != FreeCraftVersion) {
	    fprintf(stderr, "Incompatible FreeCraft version "
			FreeCraftFormatString " <-> "
			FreeCraftFormatString "\n"
			"from %d.%d.%d.%d:%d\n",
		    FreeCraftFormatArgs((int)ntohl(msg->FreeCraft)),
		    FreeCraftFormatArgs(FreeCraftVersion),
		    NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));

	    message.Type = MessageInitReply;
	    message.SubType = ICMEngineMismatch; // FreeCraft engine version doesn't match
	    message.MapUID = 0L;
	    n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
	    DebugLevel0Fn("Sending InitReply Message EngineMismatch: (%d) to %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
	    return;
	}

	if (ntohl(msg->Version) != NetworkProtocolVersion) {
	    fprintf(stderr, "Incompatible network protocol version "
			NetworkProtocolFormatString " <-> "
			NetworkProtocolFormatString "\n"
			"from %d.%d.%d.%d:%d\n",
		    NetworkProtocolFormatArgs((int)ntohl(msg->Version)),
		    NetworkProtocolFormatArgs(NetworkProtocolVersion),
		    NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));

	    message.Type = MessageInitReply;
	    message.SubType = ICMProtocolMismatch; // Network protocol version doesn't match
	    message.MapUID = 0L;
	    n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
	    DebugLevel0Fn("Sending InitReply Message ProtocolMismatch: (%d) to %d.%d.%d.%d:%d\n",
			n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
	    return;
	}

	switch(msg->SubType) {
	    case ICMHello:		// a client has arrived
		// first look up, if host is already known.
		for (h = 1; h < HostsCount; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			break;
		    }
		}
		if (h == HostsCount) {	// it is a new client
		    for (i = 1; i < h; i++) {
			if (Hosts[i].PlyNr == 0) {	// If possible occupy empty (previously freed) slots
			    h = i;
			    break;
			}
		    }
		    if (h < NetPlayers) {
			Hosts[h].Host = NetLastHost;
			Hosts[h].Port = NetLastPort;
			Hosts[h].PlyNr = h;
			memcpy(Hosts[h].PlyName, msg->u.Hosts[0].PlyName, 16);
			DebugLevel0Fn("New client %d.%d.%d.%d:%d [%s]\n",
			    NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort), Hosts[h].PlyName);
			NetStates[h].State = ccs_connecting;
			NetStates[h].MsgCnt = 0;
			if (h == HostsCount) {
			    HostsCount++;
			}
		    } else {
			message.Type = MessageInitReply;
			message.SubType = ICMGameFull;	// Game is full - reject connnection
			message.MapUID = 0L;
			n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
			DebugLevel0Fn("Sending InitReply Message GameFull: (%d) to %d.%d.%d.%d:%d\n",
				    n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
			return;
		    }
		}
		// this code path happens until client sends waiting (= has received this message)
		message.Type = MessageInitReply;
		message.SubType = ICMWelcome;				// Acknowledge: Client is welcome
		message.u.Hosts[0].PlyNr = h;				// Host array slot number
		memcpy(message.u.Hosts[0].PlyName, NetworkName, 16);	// Name of server player
		message.HostsCount = (char)(HostsCount & 0xff);
		message.MapUID = 0L;
		for (i = 1; i < HostsCount; i++) {			// Info about other clients
		    if (i != h) {
			message.u.Hosts[i].Host = Hosts[i].Host;
			message.u.Hosts[i].Port = Hosts[i].Port;
			message.u.Hosts[i].PlyNr = i;
			memcpy(message.u.Hosts[i].PlyName, Hosts[i].PlyName, 16);
		    }
		}
		n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
		DebugLevel0Fn("Sending InitReply Message Welcome: (%d) to %d.%d.%d.%d:%d\n",
			    n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
		NetStates[h].MsgCnt++;
		if (NetStates[h].MsgCnt > 50) {
		    // FIXME: Client sends hellos, but doesn't receive our welcome acks....
		    ;
		}
		NetConnectForceDisplayUpdate();
		return;

	    case ICMResync:
		// look up the host
		for (h = 0; h < HostsCount; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			switch (NetStates[h].State) {
			    // client has recvd welcome and is waiting for info
			    case ccs_async:
				NetStates[h].State = ccs_synced;
				NetStates[h].MsgCnt = 0;
				/* Fall through */
			    case ccs_synced:
				// this code path happens until client falls back to ICMWaiting
				// (indicating Resync has completed)
				message.Type = MessageInitReply;
				message.SubType = ICMResync;
				message.HostsCount = (char)(HostsCount & 0xff);
				for (i = 1; i < HostsCount; i++) {	// Info about other clients
				    if (i != h) {
					message.u.Hosts[i].Host = Hosts[i].Host;
					message.u.Hosts[i].Port = Hosts[i].Port;
					message.u.Hosts[i].PlyNr = Hosts[i].PlyNr;
					memcpy(message.u.Hosts[i].PlyName, Hosts[i].PlyName, 16);
				    }
				}
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message Resync: (%d) to %d.%d.%d.%d:%d\n",
					    n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends resync, but doesn't receive our resync ack....
				    ;
				}
				break;

			    default:
				DebugLevel0Fn("Server: ICMResync: Unhandled state %d Host %d\n",
								 NetStates[h].State, h);
				break;
			}
			break;
		    }
		}
		break;
		DebugLevel0Fn("Server: Unhandled subtype %d\n",msg->SubType);
		break;

	    case ICMWaiting:
		// look up the host
		for (h = 0; h < HostsCount; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			ServerSetupState.LastFrame[h] = FrameCounter;
			NetConnectForceDisplayUpdate();

			switch (NetStates[h].State) {
			    // client has recvd welcome and is waiting for info
			    case ccs_connecting:
				NetStates[h].State = ccs_connected;
				NetStates[h].MsgCnt = 0;
				/* Fall through */
			    case ccs_connected:
				// this code path happens until client acknoledges the map
				message.Type = MessageInitReply;
				message.SubType = ICMMap;			// Send Map info to the client
				// FIXME: Transmit (and receive!) relative to FreeCraftLibPath
				memcpy(message.u.MapPath, ScenSelectFullPath, 256);
				message.HostsCount = (char)(HostsCount & 0xff);
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message Map: (%d) to %d.%d.%d.%d:%d\n",
					    n, NIPQUAD(ntohl(NetLastHost)), ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends waiting, but doesn't receive our map....
				    ;
				}
				break;
			    case ccs_mapinfo:
				NetStates[h].State = ccs_synced;
				NetStates[h].MsgCnt = 0;
				for (i = 1; i < HostsCount; i++) {
				    if (i != h) {
					// Notify other clients
					NetStates[i].State = ccs_async;
				    }
				}
				/* Fall through */
			    case ccs_synced:
				// the wanted state - do nothing.. until start...
				NetStates[h].MsgCnt = 0;
				break;

			    case ccs_async:
				// Server User has changed menu selection. This state is set by MENU code
				// OR we have received a new client/other client has changed data

				// this code path happens until client acknoledges the state change
				// by sending ICMResync
				message.Type = MessageInitReply;
				message.SubType = ICMState;		// Send new state info to the client
				message.u.State = ServerSetupState;
				message.HostsCount = (char)(HostsCount & 0xff);
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message State: (%d) to %d.%d.%d.%d:%d\n",
					    n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends waiting, but doesn't receive our state info....
				    ;
				}
				break;

			    default:
				DebugLevel0Fn("Server: ICMWaiting: Unhandled state %d Host %d\n",
								 NetStates[h].State, h);
				break;
			}
			break;
		    }
		}
		break;

	    case ICMMap:
		// look up the host
		for (h = 0; h < HostsCount; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			switch (NetStates[h].State) {
			    // client has recvd map info waiting for state info
			    case ccs_connected:
				NetStates[h].State = ccs_mapinfo;
				NetStates[h].MsgCnt = 0;
				/* Fall through */
			    case ccs_mapinfo:
				// this code path happens until client acknoledges the state info
				// by falling back to ICMWaiting with prev. State synced
				message.Type = MessageInitReply;
				message.SubType = ICMState;		// Send State info to the client
				message.u.State = ServerSetupState;
				message.HostsCount = (char)(HostsCount & 0xff);
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message State: (%d) to %d.%d.%d.%d:%d\n",
					    n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends mapinfo, but doesn't receive our state info....
				    ;
				}
				break;
			    default:
				DebugLevel0Fn("Server: ICMMap: Unhandled state %d Host %d\n",
								 NetStates[h].State, h);
				break;
			}
			break;
		    }
		}
		break;

	    case ICMState:
		// look up the host
		for (h = 0; h < HostsCount; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			switch (NetStates[h].State) {
			    case ccs_mapinfo:
				// User State Change right after connect - should not happen, but..
				/* Fall through */
			    case ccs_synced:
				// Default case: Client is in sync with us, but notes a local change
				NetStates[h].State = ccs_async;
				NetStates[h].MsgCnt = 0;
				// Use information supplied by the client:
				ServerSetupState.Ready[h] = msg->u.State.Ready[h];
				ServerSetupState.Race[h] = msg->u.State.Race[h];
				DebugLevel3Fn("Server: ICMState: Client[%d]: Ready: %d Race: %d\n",
						 h, ServerSetupState.Ready[h], ServerSetupState.Race[h]);
				// Add additional info usage here!
				ServerSetupState.LastFrame[h] = FrameCounter;
				NetConnectForceDisplayUpdate();
				/* Fall through */
			    case ccs_async:
				// this code path happens until client acknoledges the state change reply
				// by sending ICMResync
				message.Type = MessageInitReply;
				message.SubType = ICMState;		// Send new state info to the client
				message.u.State = ServerSetupState;
				message.HostsCount = (char)(HostsCount & 0xff);
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message State: (%d) to %d.%d.%d.%d:%d\n",
					    n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends State, but doesn't receive our state info....
				    ;
				}
				break;
			    default:
				DebugLevel0Fn("Server: ICMState: Unhandled state %d Host %d\n",
								 NetStates[h].State, h);
				break;
			}
			break;
		    }
		}
		break;

	    case ICMGoodBye:
		// look up the host
		for (h = 0; h < HostsCount; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			switch (NetStates[h].State) {
			    default:
				// We can enter here from _ANY_ state!
				NetStates[h].MsgCnt = 0;
				NetStates[h].State = ccs_detaching;
				/* Fall through */
			    case ccs_detaching:
				// this code path happens until client acknoledges the GoodBye
				// by sending ICMSeeYou;
				message.Type = MessageInitReply;
				message.SubType = ICMGoodBye;
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message GoodBye: (%d) to %d.%d.%d.%d:%d\n",
					    n, NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 10) {
				    // FIXME: Client sends GoodBye, but doesn't receive our GoodBye....
				    ;
				}
				break;
			}
			break;
		     }
		}
		break;

	    case ICMSeeYou:
		// look up the host
		for (h = 0; h < HostsCount; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			switch (NetStates[h].State) {
			    case ccs_detaching:
				NetStates[h].State = ccs_unused;
				Hosts[h].Host = 0;
				Hosts[h].Port = 0;
				Hosts[h].PlyNr = 0;
				memset(Hosts[h].PlyName, 0, 16);
				ServerSetupState.Ready[h] = 0;
				ServerSetupState.Race[h] = 0;

				// FIXME: Resync other clients here!
				NetConnectForceDisplayUpdate();
				break;

			    default:
				DebugLevel0Fn("Server: ICMSeeYou: Unhandled state %d Host %d\n",
								 NetStates[h].State, h);
				break;
			}
			break;
		     }
		}
		break;

	    default:
		DebugLevel0Fn("Server: Unhandled subtype %d\n",msg->SubType);
		break;
	}
    }
}

/**
**	Parse a setup event. (Command type <= MessageInitEvent)
**
**	@param buf	Packet received
**	@param size	size of the received packet.
*/
global void NetworkParseSetupEvent(const char *buf, int size)
{
    NetworkPacket *packet;

    if (InterfaceState == IfaceStateMenu && NetConnectRunning) {
	NetworkParseMenuPacket((const InitMessage *)buf, size);
	return;
    }
    packet = (NetworkPacket *)buf;
    if (packet->Commands[0].Type == MessageInitConfig
	    && size == sizeof(InitMessage)) {
	Acknowledge acknowledge;

	DebugLevel0Fn("Received late clients\n");

	// Acknowledge the packets.
	acknowledge.Type = MessageInitReply;
	size = NetSendUDP(NetworkFildes, NetLastHost, NetLastPort, &acknowledge,
		sizeof(acknowledge));
	DebugLevel0Fn("Sending config ack (%d)\n", size);
	return;
    }
    if (packet->Commands[0].Type == MessageInitReply) {
	DebugLevel0Fn("late init reply\n");
	return;
    }
}

//@}

//@}
