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
global int NetLocalPlayerNumber;	/// Network menu: Player number of local client
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
//	Functions
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
    "ccs_synced",		// client is in sync with server
    "ccs_async",		// server user has changed selection
    "ccs_changed",		// client user has made menu selection
    "ccs_detaching",		// client user wants to detach
    "ccs_disconnected",		// client has detached
    "ccs_unreachable",		// server is unreachable
    "ccs_usercanceled",		// user canceled game
    "ccs_nofreeslots",		// server has no more free slots
    "ccs_serverquits",		// server quits
    "ccs_goahead",		// server wants to start game
    "ccs_started",		// server has started game
    "ccs_incompatibleengine",	// incompatible engine version
    "ccs_incompatiblenetwork",	// incompatible netowrk version
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

    "Go",			// Client is ready to run
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
	DebugLevel0Fn("Sending Init Message (%s:%d): %d:%d(%d) %d.%d.%d.%d:%d\n" _C_
		ncconstatenames[NetLocalState] _C_ NetStateMsgCnt _C_ msg->Type _C_
		msg->SubType _C_ n _C_
		NIPQUAD(ntohl(NetworkServerIP)) _C_ NetworkServerPort);
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

    DebugLevel1Fn("SELECTED SERVER: %s (%d.%d.%d.%d)\n" _C_ serveraddr _C_
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
    // HostsCount = 0;
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

    DebugLevel3Fn("Waiting for %d client(s)\n" _C_ NetPlayers - 1);
    // Cannot use NetPlayers here, as map change might modify the number!!
    for (i = 0; i < PlayerMax; ++i) {
	NetStates[i].State = ccs_unused;
	Hosts[i].Host = 0;
	Hosts[i].Port = 0;
	Hosts[i].PlyNr = 0;		// slotnr until final cfg msg
	memset(Hosts[i].PlyName, 0, 16);
    }

    // HostsCount = 0;
    // preset the server (initially always slot 0)
    memcpy(Hosts[0].PlyName, NetworkName, 16);
    // HostsCount++;
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
    for (h = 1; h < PlayerMax-1; ++h) {
	// Spew out 5 and trust in God that they arrive
	if (Hosts[h].PlyNr) {
	    for (i = 0; i < 5; i++) {
		n = NetworkSendICMessage(Hosts[h].Host, Hosts[h].Port, &message);
		DebugLevel0Fn("Sending InitReply Message ServerQuit: (%d) to %d.%d.%d.%d:%d\n" _C_
						n _C_ NIPQUAD(ntohl(Hosts[h].Host)) _C_ ntohs(Hosts[h].Port));
	    }
	}
    }

    NetworkInitServerConnect();	// Reset Hosts slots

    NetConnectRunning = 0;
}

/**
**	Notify state change by menu user to connected clients
*/
global void NetworkServerResyncClients(void)
{
    int i;

    if (NetConnectRunning) {
	for (i = 1; i < PlayerMax-1; ++i) {
	    if (Hosts[i].PlyNr && NetStates[i].State == ccs_synced) {
		NetStates[i].State = ccs_async;
	    }
	}
    }
}

/**
**	Server user has finally hit the start game button
*/
global void NetworkServerStartGame(void)
{
    int h, i, j, n;
    int num[PlayerMax], org[PlayerMax], rev[PlayerMax];
    char buf[1024];
    InitMessage *msg;
    InitMessage message, statemsg;

    DebugCheck(ServerSetupState.CompOpt[0] != 0);

    // save it first..
    LocalSetupState = ServerSetupState;

    // Make a list of the available player slots.
    for (h = i = 0; i < PlayerMax; i++) {
	if (ScenSelectPudInfo->PlayerType[i] == PlayerPerson) {
	    rev[i] = h;
	    num[h++] = i;
	    DebugLevel0Fn("Slot %d is available for an interactive player (%d)\n" _C_ i _C_ rev[i]);
	}
    }
    // Make a list of the available computer slots.
    n = h;
    for (i = 0; i < PlayerMax; i++) {
	if (ScenSelectPudInfo->PlayerType[i] == PlayerComputer) {
	    rev[i] = n++;
	    DebugLevel0Fn("Slot %d is available for an ai computer player (%d)\n" _C_ i _C_ rev[i]);
	}
    }
    // Make a list of the remaining slots.
    for (i = 0; i < PlayerMax; i++) {
	if (ScenSelectPudInfo->PlayerType[i] != PlayerPerson &&
				ScenSelectPudInfo->PlayerType[i] != PlayerComputer) {
	    rev[i] = n++;
	    // PlayerNobody - not available to anything..
	}
    }

#if 0
    printf("INITIAL ServerSetupState:\n");
    for (i = 0; i < PlayerMax-1; i++) {
	printf("%02d: CO: %d   Race: %d   Host: ", i, ServerSetupState.CompOpt[i], ServerSetupState.Race[i]);
	if (ServerSetupState.CompOpt[i] == 0) {
	    printf(" %d.%d.%d.%d:%d %s", NIPQUAD(ntohl(Hosts[i].Host)),
			 ntohs(Hosts[i].Port), Hosts[i].PlyName);
	}
	printf("\n");
    }
#endif

    // Reverse to assign slots to menu setup state positions.
    for (i = 0; i < PlayerMax; i++) {
	org[i] = -1;
	for (j = 0; j < PlayerMax; j++) {
	    if (rev[j] == i) {
		org[i] = j;
		break;
	    }
	}
    }

    // Compact host list.. (account for computer/closed slots in the middle..)
    for (i = 1; i < h; i++) {
	if (Hosts[i].PlyNr == 0) {
	    for (j = i + 1; j < PlayerMax - 1; j++) {
		if (Hosts[j].PlyNr) {
		    DebugLevel0Fn("Compact: Hosts %d -> Hosts %d\n" _C_ j _C_ i);
		    Hosts[i] = Hosts[j];
		    Hosts[j].PlyNr = Hosts[j].Host = Hosts[j].Port = 0;
		    n = LocalSetupState.CompOpt[i];
		    LocalSetupState.CompOpt[i] = LocalSetupState.CompOpt[j];
		    LocalSetupState.CompOpt[j] = n;
		    n = LocalSetupState.Race[i];
		    LocalSetupState.Race[i] = LocalSetupState.Race[j];
		    LocalSetupState.Race[j] = n;
		    n = LocalSetupState.LastFrame[i];
		    LocalSetupState.LastFrame[i] = LocalSetupState.LastFrame[j];
		    LocalSetupState.LastFrame[j] = n;
		    break;
		}
	    }
	    if (j == PlayerMax - 1) {
		break;
	    }
	}
    }

    // Randomize the position.
    j = h;
    for (i = 0; i < NetPlayers; i++) {
	if (j > 0) {
	    int k, o, chosen = MyRand() % j;

	    n = num[chosen];
	    Hosts[i].PlyNr = n;
	    k = org[i];
	    if (k != n) {
		for (o = 0; o < PlayerMax; o++) {
		    if (org[o] == n) {
			org[o] = k;
			break;
		    }
		}
		org[i] = n;
	    }
	    DebugLevel0Fn("Assigning player %d to slot %d (%d)\n" _C_ i _C_ n _C_ org[i]);

	    num[chosen] = num[--j];
	} else {
	    DebugCheck(1 == 1);
#if 0
	    // ARI: is this code path really executed? (initially h >= NetPlayers..)
	    Hosts[i].PlyNr = num[0];
	    DebugLevel0Fn("Hosts[%d].PlyNr = %i\n", i, num[0]);
#endif
	}
    }

    // Complete all setup states for the assigned slots.
    for (i = 0; i < PlayerMax; i++) {
	num[i] = 1;
	n = org[i];
	ServerSetupState.CompOpt[n] = LocalSetupState.CompOpt[i];
	ServerSetupState.Race[n] = LocalSetupState.Race[i];
	ServerSetupState.LastFrame[n] = LocalSetupState.LastFrame[i];
    }

    /* NOW we have NetPlayers in Hosts array, with ServerSetupState shuffled up to match it.. */

    //
    //	Send all clients host:ports to all clients.
    //  Slot 0 is the server!
    //
    NetLocalPlayerNumber = Hosts[0].PlyNr;
    HostsCount = NetPlayers - 1;

    //
    // Move ourselves (server slot 0) to the end of the list
    //
    Hosts[PlayerMax - 1] = Hosts[0];		// last slot unused (special), use as tempstore..
    Hosts[0] = Hosts[HostsCount];
    Hosts[HostsCount] = Hosts[PlayerMax - 1];
    Hosts[PlayerMax - 1].PlyNr = 0;

    // Prepare the final config message:
    message.Type = MessageInitReply;
    message.SubType = ICMConfig;
    message.HostsCount = NetPlayers;
    message.MapUID = htonl(ScenSelectPudInfo->MapUID);
    for (i = 0; i < NetPlayers; ++i) {
	message.u.Hosts[i].Host = Hosts[i].Host;
	message.u.Hosts[i].Port = Hosts[i].Port;
	memcpy(message.u.Hosts[i].PlyName, Hosts[i].PlyName, 16);
	message.u.Hosts[i].PlyNr = htons(Hosts[i].PlyNr);
    }

    // Prepare the final state message:
    statemsg.Type = MessageInitReply;
    statemsg.SubType = ICMState;
    statemsg.HostsCount = NetPlayers;
    statemsg.u.State = ServerSetupState;
    statemsg.MapUID = htonl(ScenSelectPudInfo->MapUID);

    msg = (InitMessage *)buf;
    DebugLevel1Fn("Ready, sending InitConfig to %d host(s)\n" _C_ HostsCount);
    //
    //	Send all clients host:ports to all clients.
    //
    for (j = HostsCount; j;) {

breakout:
	// Send to all clients.
	for (i = 0; i < HostsCount; ++i) {
	    unsigned long host;
	    int port;

	    if (num[Hosts[i].PlyNr] == 1) {		// not acknowledged yet
		host = message.u.Hosts[i].Host;
		port = message.u.Hosts[i].Port;
		message.u.Hosts[i].Host = message.u.Hosts[i].Port = 0;
		n = NetworkSendICMessage(host, port, &message);
		DebugLevel0Fn("Sending InitConfig Message Config (%d) to %d.%d.%d.%d:%d\n" _C_
			n _C_ NIPQUAD(ntohl(host)) _C_ ntohs(port));
		message.u.Hosts[i].Host = host;
		message.u.Hosts[i].Port = port;
	    } else if (num[Hosts[i].PlyNr] == 2) {
		host = message.u.Hosts[i].Host;
		port = message.u.Hosts[i].Port;
		n = NetworkSendICMessage(host, port, &statemsg);
		DebugLevel0Fn("Sending InitReply Message State: (%d) to %d.%d.%d.%d:%d\n" _C_
			n _C_ NIPQUAD(ntohl(host)) _C_ ntohs(port));
	    }
	}

	// Wait for acknowledge
	while (j && NetSocketReady(NetworkFildes, 1000)) {
	    if ((n = NetRecvUDP(NetworkFildes, &buf, sizeof(buf))) < 0) {
		DebugLevel0Fn("*Receive ack failed: (%d) from %d.%d.%d.%d:%d\n" _C_
			n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
		continue;
	    }

	    if (msg->Type == MessageInitHello && n == sizeof(*msg)) {
		switch (msg->SubType) {

		    case ICMConfig:
			DebugLevel0Fn("Got ack for InitConfig: (%d) from %d.%d.%d.%d:%d\n" _C_
				n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));

			for (i = 0; i < HostsCount; ++i) {
			    if (NetLastHost == Hosts[i].Host && NetLastPort == Hosts[i].Port) {
				if (num[Hosts[i].PlyNr] == 1) {
				    num[Hosts[i].PlyNr]++;
				}
				goto breakout;
			    }
			}
			break;

		    case ICMGo:
			DebugLevel0Fn("Got ack for InitState: (%d) from %d.%d.%d.%d:%d\n" _C_
				n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));

			for (i = 0; i < HostsCount; ++i) {
			    if (NetLastHost == Hosts[i].Host && NetLastPort == Hosts[i].Port) {
				if (num[Hosts[i].PlyNr] == 2) {
				    num[Hosts[i].PlyNr] = 0;
				    j--;
				    DebugLevel0Fn("Removing host %d from waiting list\n" _C_ j);
				}
				break;
			    }
			}
			break;

		    default:
			DebugLevel0Fn("Server: Config ACK: Unhandled subtype %d\n" _C_ msg->SubType);
			break;
		}
	    } else {
		DebugLevel0Fn("Unexpected Message Type %d while waiting for Config ACK\n" _C_ msg->Type);
	    }
	}
    }

    DebugLevel0Fn("DONE: All configs acked - Now starting..\n");
}

/**
**	Assign player slots and names in a network game..
*/
global void NetworkConnectSetupGame(void)
{
    int i;

    ThisPlayer = &Players[NetLocalPlayerNumber];
    PlayerSetName(ThisPlayer, NetworkName);
    for (i = 0; i < HostsCount; ++i) {
	PlayerSetName(&Players[Hosts[i].PlyNr], Hosts[i].PlyName);
    }

    // FIXME: evaluate ServerSetupState .....
    // FIXME: must be done earlier during player creation!!!
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
	    NetLocalState = ccs_usercanceled;
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
		DebugLevel0Fn("ccs_detaching: Above message limit %d\n" _C_ NetStateMsgCnt);
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
		DebugLevel0Fn("ccs_connecting: Above message limit %d\n" _C_ NetStateMsgCnt);
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
		DebugLevel0Fn("ccs_connected: Above message limit %d\n" _C_ NetStateMsgCnt);
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
		message.MapUID = htonl(ScenSelectPudInfo->MapUID);
		NetworkSendRateLimitedClientMessage(&message, 450);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_changed: Above message limit %d\n" _C_ NetStateMsgCnt);
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
		DebugLevel0Fn("ccs_async: Above message limit %d\n" _C_ NetStateMsgCnt);
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
		DebugLevel0Fn("ccs_mapinfo: Above message limit %d\n" _C_ NetStateMsgCnt);
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
		DebugLevel0Fn("ccs_badmap: Above message limit %d\n" _C_ NetStateMsgCnt);
	    }
	    break;
	case ccs_goahead:
	    if (NetStateMsgCnt < 50) {	// 50 retries
		message.Type = MessageInitHello;
		message.SubType = ICMConfig;
		NetworkSendRateLimitedClientMessage(&message, 250);
	    } else {
		NetLocalState = ccs_unreachable;
		NetConnectRunning = 0;	// End the menu..
		DebugLevel0Fn("ccs_goahead: Above message limit %d\n" _C_ NetStateMsgCnt);
	    }
	case ccs_started:
	    if (NetStateMsgCnt < 20) {	// 20 retries
		message.Type = MessageInitHello;
		message.SubType = ICMGo;
		NetworkSendRateLimitedClientMessage(&message, 250);
	    } else {
		DebugLevel3Fn("ccs_started: Final state enough ICMGo sent - starting\n");
		NetConnectRunning = 0;	// End the menu..
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
    int i, h, n, k;
    InitMessage message;

    if (msg->Type > MessageInitConfig || size != sizeof(*msg)) {
	DebugLevel0Fn("Wrong message\n");
	return;
    }
    DebugLevel0Fn("Received %s Init Message %d:%d (%d) from %d.%d.%d.%d:%d (%ld)\n" _C_
	    icmsgsubtypenames[msg->SubType] _C_ msg->Type _C_ msg->SubType _C_ size _C_ NIPQUAD(ntohl(NetLastHost)) _C_
	    ntohs(NetLastPort) _C_ FrameCounter);

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
		    DebugLevel0Fn("ccs_disconnected: Server sending GoodBye dups %d\n" _C_ msg->SubType);
		    break;

		case ccs_detaching:
		    switch(msg->SubType) {

			case ICMGoodBye:	// Server has let us go
			    DebugLevel3Fn("ccs_detaching: Server GoodBye subtype %d received - byebye\n" _C_ msg->SubType);
			    NetLocalState = ccs_disconnected;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_detaching: Unhandled subtype %d\n" _C_ msg->SubType);
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
			    NetLocalHostsSlot = ntohs(msg->u.Hosts[0].PlyNr);
			    memcpy(Hosts[0].PlyName, msg->u.Hosts[0].PlyName, 16); // Name of server player
			    NetworkLag= ntohl(msg->Lag);
			    NetworkUpdates= ntohl(msg->Updates);

			    Hosts[0].Host = NetworkServerIP;
			    Hosts[0].Port = htons(NetworkServerPort);
			    for (i = 1; i < PlayerMax; i++) {
				if (i != NetLocalHostsSlot) {
				    Hosts[i].Host = msg->u.Hosts[i].Host;
				    Hosts[i].Port = msg->u.Hosts[i].Port;
				    Hosts[i].PlyNr = ntohs(msg->u.Hosts[i].PlyNr);
				    if (Hosts[i].PlyNr) {
					memcpy(Hosts[i].PlyName, msg->u.Hosts[i].PlyName, 16);
				    }
				} else {
				    Hosts[i].PlyNr = i;
				    memcpy(Hosts[i].PlyName, NetworkName, 16);
				}
			    }
			    break;

			default:
			    DebugLevel0Fn("ccs_connecting: Unhandled subtype %d\n" _C_ msg->SubType);
			    break;

		    }
		    break;

		case ccs_connected:
		    switch(msg->SubType) {
			int pathlen;

			case ICMMap:		// Server has sent us new map info
			    sprintf(ScenSelectFullPath, "%s/", FreeCraftLibPath);
			    pathlen = strlen(ScenSelectFullPath);
			    memcpy(ScenSelectFullPath+pathlen, msg->u.MapPath, 256);
			    ScenSelectFullPath[pathlen+255] = 0;
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
			    DebugLevel3Fn("ccs_connected: DUP Welcome subtype %d\n" _C_ msg->SubType);
			    break;

			default:
			    DebugLevel0Fn("ccs_connected: Unhandled subtype %d\n" _C_ msg->SubType);
			    break;
		    }
		    break;

		case ccs_mapinfo:
		    switch(msg->SubType) {

			case ICMState:		// Server has sent us first state info
			    DebugLevel3Fn("ccs_mapinfo: Initial State subtype %d received - going sync\n" _C_ msg->SubType);
			    ServerSetupState = msg->u.State;
			    NetClientUpdateState();
			    NetLocalState = ccs_synced;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_mapinfo: Unhandled subtype %d\n" _C_ msg->SubType);
			    break;
		    }
		    break;

		case ccs_changed:
		case ccs_synced:
		    switch(msg->SubType) {

			case ICMState:		// Server has sent us new state info
			    DebugLevel3Fn("ccs_synced: New State subtype %d received - resynching\n" _C_ msg->SubType);
			    ServerSetupState = msg->u.State;
			    NetClientUpdateState();
			    NetLocalState = ccs_async;
			    NetStateMsgCnt = 0;
			    break;

			case ICMConfig:		// Server gives the go ahead..
			    DebugLevel0Fn("ccs_synced: Config subtype %d received - starting\n" _C_ msg->SubType);
			    HostsCount = 0;
			    for (i = 0; i < msg->HostsCount - 1; ++i) {
				if (msg->u.Hosts[i].Host || msg->u.Hosts[i].Port) {
				    Hosts[HostsCount].Host = msg->u.Hosts[i].Host;
				    Hosts[HostsCount].Port = msg->u.Hosts[i].Port;
				    Hosts[HostsCount].PlyNr = ntohs(msg->u.Hosts[i].PlyNr);
				    memcpy(Hosts[HostsCount].PlyName, msg->u.Hosts[i].PlyName, 16);
				    HostsCount++;
				    DebugLevel0Fn("Client %d = %d.%d.%d.%d:%d [%s]\n" _C_
					    ntohs(ntohs(msg->u.Hosts[i].PlyNr)) _C_ NIPQUAD(ntohl(msg->u.Hosts[i].Host)) _C_
					    ntohs(msg->u.Hosts[i].Port) _C_ msg->u.Hosts[i].PlyName);
				} else {			// Own client
				    NetLocalPlayerNumber = ntohs(msg->u.Hosts[i].PlyNr);
				    DebugLevel0Fn("SELF %d [%s]\n" _C_ ntohs(msg->u.Hosts[i].PlyNr) _C_
					    msg->u.Hosts[i].PlyName);
				}
			    }
			    // server is last:
			    Hosts[HostsCount].Host = NetLastHost;
			    Hosts[HostsCount].Port = NetLastPort;
			    Hosts[HostsCount].PlyNr = ntohs(msg->u.Hosts[i].PlyNr);
			    memcpy(Hosts[HostsCount].PlyName, msg->u.Hosts[i].PlyName, 16);
			    HostsCount++;
			    NetPlayers = HostsCount + 1;
			    DebugLevel0Fn("Server %d = %d.%d.%d.%d:%d [%s]\n" _C_
				    ntohs(msg->u.Hosts[i].PlyNr) _C_ NIPQUAD(ntohl(NetLastHost)) _C_
				    ntohs(NetLastPort) _C_ msg->u.Hosts[i].PlyName);

			    // put ourselves to the end, like on the server..
			    Hosts[HostsCount].Host = 0;
			    Hosts[HostsCount].Port = 0;
			    Hosts[HostsCount].PlyNr = NetLocalPlayerNumber;
			    memcpy(Hosts[HostsCount].PlyName, NetworkName, 16);

			    NetLocalState = ccs_goahead;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_synced: Unhandled subtype %d\n" _C_ msg->SubType);
			    break;
		    }
		    break;

		case ccs_async:
		    switch(msg->SubType) {

			case ICMResync:		// Server has resynced with us and sends resync data
			    for (i = 1; i < PlayerMax-1; i++) {
				if (i != NetLocalHostsSlot) {
				    Hosts[i].Host = msg->u.Hosts[i].Host;
				    Hosts[i].Port = msg->u.Hosts[i].Port;
				    Hosts[i].PlyNr = ntohs(msg->u.Hosts[i].PlyNr);
				    if (Hosts[i].PlyNr) {
					memcpy(Hosts[i].PlyName, msg->u.Hosts[i].PlyName, 16);
					DebugLevel3Fn("Other client %d: %s\n" _C_ Hosts[i].PlyNr _C_ Hosts[i].PlyName);
				    }
				} else {
				    Hosts[i].PlyNr = ntohs(msg->u.Hosts[i].PlyNr);
				    memcpy(Hosts[i].PlyName, NetworkName, 16);
				}
			    }
			    NetClientUpdateState();
			    NetLocalState = ccs_synced;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_async: Unhandled subtype %d\n" _C_ msg->SubType);
			    break;
		    }
		    break;

		case ccs_goahead:
		    switch(msg->SubType) {

			case ICMConfig:		// Server go ahead dup - ignore..
			    DebugLevel3Fn("ccs_goahead: DUP Config subtype %d\n" _C_ msg->SubType);
			    break;

			case ICMState:		// Server has sent final state info
			    DebugLevel3Fn("ccs_goahead: Final State subtype %d received - starting\n" _C_ msg->SubType);
			    ServerSetupState = msg->u.State;
			    NetLocalState = ccs_started;
			    NetStateMsgCnt = 0;
			    break;

			default:
			    DebugLevel0Fn("ccs_goahead: Unhandled subtype %d\n" _C_ msg->SubType);
			    break;
		    }
		    break;

		default:
		    DebugLevel0Fn("Client: Unhandled state %d\n" _C_ NetLocalState);
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
	    DebugLevel0Fn("Sending InitReply Message EngineMismatch: (%d) to %d.%d.%d.%d:%d\n" _C_
			n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
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
	    DebugLevel0Fn("Sending InitReply Message ProtocolMismatch: (%d) to %d.%d.%d.%d:%d\n" _C_
			n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
	    return;
	}

	switch(msg->SubType) {
	    case ICMHello:		// a client has arrived
		// first look up, if client host is already known.
		k = 0;
		for (h = 1; h < PlayerMax-1; ++h) {	// last Player slot (15) is special - avoid!
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			k = h;
			break;
		    }
		}
		if (k == 0) {	// it is a new client
		    for (n = i = 1; i < PlayerMax-1; i++) {
			DebugLevel3Fn("SSS.CO[%d] = %d, Hosts[%d].PlyNr = %d\n" _C_ i _C_
					 ServerSetupState.CompOpt[i] _C_ i _C_ Hosts[i].PlyNr);
			// occupy first available slot
			if (ServerSetupState.CompOpt[i] == 0) {
			    n++;					// n = total available slots
			    if (k == 0 && Hosts[i].PlyNr == 0) {
				k = i;
			    }
			}
		    }
		    if (k) {
			Hosts[k].Host = NetLastHost;
			Hosts[k].Port = NetLastPort;
			Hosts[k].PlyNr = k;
			memcpy(Hosts[k].PlyName, msg->u.Hosts[0].PlyName, 16);
			DebugLevel0Fn("New client %d.%d.%d.%d:%d [%s]\n" _C_
			    NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort) _C_ Hosts[k].PlyName);
			NetStates[k].State = ccs_connecting;
			NetStates[k].MsgCnt = 0;
		    } else {
			message.Type = MessageInitReply;
			message.SubType = ICMGameFull;	// Game is full - reject connnection
			message.MapUID = 0L;
			n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
			DebugLevel0Fn("Sending InitReply Message GameFull: (%d) to %d.%d.%d.%d:%d\n" _C_
				    n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
			return;
		    }
		}
		// this code path happens until client sends waiting (= has received this message)
		message.Type = MessageInitReply;
		message.SubType = ICMWelcome;				// Acknowledge: Client is welcome
		message.u.Hosts[0].PlyNr = htons(k);			// Host array slot number
		memcpy(message.u.Hosts[0].PlyName, NetworkName, 16);	// Name of server player
		message.MapUID = 0L;
		for (i = 1; i < PlayerMax-1; i++) {			// Info about other clients
		    if (i != k) {
			if (Hosts[i].PlyNr) {
			    message.u.Hosts[i].Host = Hosts[i].Host;
			    message.u.Hosts[i].Port = Hosts[i].Port;
			    message.u.Hosts[i].PlyNr = htons(Hosts[i].PlyNr);
			    memcpy(message.u.Hosts[i].PlyName, Hosts[i].PlyName, 16);
			} else {
			    message.u.Hosts[i].Host = 0;
			    message.u.Hosts[i].Port = 0;
			    message.u.Hosts[i].PlyNr = 0;
			}
		    }
		}
		n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
		DebugLevel0Fn("Sending InitReply Message Welcome: (%d) [PlyNr: %d] to %d.%d.%d.%d:%d\n" _C_
			    n _C_ k _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
		NetStates[h].MsgCnt++;
		if (NetStates[h].MsgCnt > 50) {
		    // FIXME: Client sends hellos, but doesn't receive our welcome acks....
		    ;
		}
		NetConnectForceDisplayUpdate();
		return;

	    case ICMResync:
		// look up the host
		for (h = 0; h < PlayerMax-1; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort) {
			switch (NetStates[h].State) {
			    case ccs_mapinfo:
				// a delayed ack - fall through..
			    case ccs_async:
				// client has recvd welcome and is waiting for info
				NetStates[h].State = ccs_synced;
				NetStates[h].MsgCnt = 0;
				/* Fall through */
			    case ccs_synced:
				// this code path happens until client falls back to ICMWaiting
				// (indicating Resync has completed)
				message.Type = MessageInitReply;
				message.SubType = ICMResync;
				for (i = 1; i < PlayerMax-1; i++) {	// Info about other clients
				    if (i != h) {
					if (Hosts[i].PlyNr) {
					    message.u.Hosts[i].Host = Hosts[i].Host;
					    message.u.Hosts[i].Port = Hosts[i].Port;
					    message.u.Hosts[i].PlyNr = htons(Hosts[i].PlyNr);
					    memcpy(message.u.Hosts[i].PlyName, Hosts[i].PlyName, 16);
					} else {
					    message.u.Hosts[i].Host = 0;
					    message.u.Hosts[i].Port = 0;
					    message.u.Hosts[i].PlyNr = 0;
					}
				    }
				}
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message Resync: (%d) to %d.%d.%d.%d:%d\n" _C_
					    n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends resync, but doesn't receive our resync ack....
				    ;
				}
				break;

			    default:
				DebugLevel0Fn("Server: ICMResync: Unhandled state %d Host %d\n" _C_
								 NetStates[h].State _C_ h);
				break;
			}
			break;
		    }
		}
		break;
		DebugLevel0Fn("Server: Unhandled subtype %d\n" _C_ msg->SubType);
		break;

	    case ICMWaiting:
		// look up the host
		for (h = 0; h < PlayerMax-1; ++h) {
		    if (Hosts[h].Host == NetLastHost && Hosts[h].Port == NetLastPort && Hosts[h].PlyNr) {
			int pathlen;
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
				pathlen = strlen(FreeCraftLibPath) + 1;
				memcpy(message.u.MapPath, ScenSelectFullPath+pathlen, 256);
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message Map: (%d) to %d.%d.%d.%d:%d\n" _C_
					    n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends waiting, but doesn't receive our map....
				    ;
				}
				break;
			    case ccs_mapinfo:
				NetStates[h].State = ccs_synced;
				NetStates[h].MsgCnt = 0;
				for (i = 1; i < PlayerMax-1; i++) {
				    if (i != h && Hosts[i].PlyNr) {
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
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message State: (%d) to %d.%d.%d.%d:%d\n" _C_
					    n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends waiting, but doesn't receive our state info....
				    ;
				}
				break;

			    default:
				DebugLevel0Fn("Server: ICMWaiting: Unhandled state %d Host %d\n" _C_
								 NetStates[h].State _C_ h);
				break;
			}
			break;
		    }
		}
		break;

	    case ICMMap:
		// look up the host
		for (h = 0; h < PlayerMax-1; ++h) {
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
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message State: (%d) to %d.%d.%d.%d:%d\n" _C_
					    n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends mapinfo, but doesn't receive our state info....
				    ;
				}
				break;
			    default:
				DebugLevel0Fn("Server: ICMMap: Unhandled state %d Host %d\n" _C_
								 NetStates[h].State _C_ h);
				break;
			}
			break;
		    }
		}
		break;

	    case ICMState:
		// look up the host
		for (h = 0; h < PlayerMax-1; ++h) {
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
				DebugLevel3Fn("Server: ICMState: Client[%d]: Ready: %d Race: %d\n" _C_
						 h _C_ ServerSetupState.Ready[h] _C_ ServerSetupState.Race[h]);
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
				message.MapUID = htonl(ScenSelectPudInfo->MapUID);
				n = NetworkSendICMessage(NetLastHost, NetLastPort, &message);
				DebugLevel0Fn("Sending InitReply Message State: (%d) to %d.%d.%d.%d:%d\n" _C_
					    n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
				NetStates[h].MsgCnt++;
				if (NetStates[h].MsgCnt > 50) {
				    // FIXME: Client sends State, but doesn't receive our state info....
				    ;
				}
				break;
			    default:
				DebugLevel0Fn("Server: ICMState: Unhandled state %d Host %d\n" _C_
								 NetStates[h].State _C_ h);
				break;
			}
			break;
		    }
		}
		break;

	    case ICMGoodBye:
		// look up the host
		for (h = 0; h < PlayerMax-1; ++h) {
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
				DebugLevel0Fn("Sending InitReply Message GoodBye: (%d) to %d.%d.%d.%d:%d\n" _C_
					    n _C_ NIPQUAD(ntohl(NetLastHost)) _C_ ntohs(NetLastPort));
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
		for (h = 0; h < PlayerMax-1; ++h) {
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

				// Resync other clients
				for (i = 1; i < PlayerMax-1; i++) {
				    if (i != h && Hosts[i].PlyNr) {
					NetStates[i].State = ccs_async;
				    }
				}
				NetConnectForceDisplayUpdate();
				break;

			    default:
				DebugLevel0Fn("Server: ICMSeeYou: Unhandled state %d Host %d\n" _C_
								 NetStates[h].State _C_ h);
				break;
			}
			break;
		     }
		}
		break;

	    default:
		DebugLevel0Fn("Server: Unhandled subtype %d\n" _C_ msg->SubType);
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
	DebugLevel0Fn("Sending config ack (%d)\n" _C_ size);
	return;
    }
    if (packet->Commands[0].Type == MessageInitReply) {
	DebugLevel0Fn("late init reply\n");
	return;
    }
}

//@}

//@}
