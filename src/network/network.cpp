//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name network.cpp - The network. */
//
//      (c) Copyright 2000-2008 by Lutz Sammer, Andreas Arens, and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

//@{

//----------------------------------------------------------------------------
// Documentation
//----------------------------------------------------------------------------

/**
** @page NetworkModule Module - Network
**
** @section Basics How does it work.
**
** Stratagus uses an UDP peer to peer protocol (p2p). The default port
** is 6660.
**
** @subsection udp_vs_tcp UDP vs. TCP
**
** UDP is a connectionless protocol. This means it does not perform
** retransmission of data and therefore provides very few error recovery
** services. UDP instead offers a direct way to send and receive
** datagrams (packets) over the network; it is used primarily for
** broadcasting messages.
**
** TCP, on the other hand, provides a connection-based, reliable data
** stream. TCP guarantees delivery of data and also guarantees that
** packets will be delivered in the same order in which they were sent.
**
** TCP is a simple and effective way of transmitting data. For making sure
** that a client and server can talk to each other it is very good.
** However, it carries with it a lot of overhead and extra network lag.
**
** UDP needs less overhead and has a smaller lag. Which is very important
** for real time games. The disadvantages includes:
**
** @li You won't have an individual socket for each client.
** @li Given that clients don't need to open a unique socket in order to
** transmit data there is the very real possibility that a client
** who is not logged into the game will start sending all kinds of
** garbage to your server in some kind of attack. It becomes much
** more difficult to stop them at this point.
** @li Likewise, you won't have a clear disconnect/leave game message
** unless you write one yourself.
** @li Some data may not reach the other machine, so you may have to send
** important stuff many times.
** @li Some data may arrive in the wrong order. Imagine that you get
** package 3 before package 1. Even a package can come duplicate.
** @li UDP is connectionless and therefore has problems with firewalls.
**
** I have choosen UDP. Additional support for the TCP protocol is welcome.
**
** @subsection sc_vs_p2p server/client vs. peer to peer
**
** @li server to client
**
** The player input is send to the server. The server collects the input
** of all players and than send the commands to all clients.
**
** @li peer to peer (p2p)
**
** The player input is direct send to all others clients in game.
**
** p2p has the advantage of a smaller lag, but needs a higher bandwidth
** by the clients.
**
** p2p has been choosen for in-game.
** s/c for the preparing room.
**
** @subsection bandwidth bandwidth
**
** I wanted to support up to 8 players with 28.8kbit modems.
**
** Most modems have a bandwidth of 28.8K bits/second (both directions) to
** 56K bits/second (33.6K uplink) It takes actually 10 bits to send 1 byte.
** This makes calculating how many bytes you are sending easy however, as
** you just need to divide 28800 bits/second by 10 and end up with 2880
** bytes per second.
**
** We want to send many packets, more updated per second and big packets,
** less protocol overhead.
**
** If we do an update 6 times per second, leaving approximately 480 bytes
** per update in an ideal environment.
**
** For the TCP/IP protocol we need following:
** IP  Header 20 bytes
** UDP Header 8  bytes
**
** With ~9 bytes per command and up to N(=9) commands there are 20+8+1+9*N(=120) bytes
** per packet. Sending it to 7 other players, gives 840 bytes per update.
** This means we could do 6 updates (each 166ms) per second (6*840=5040 bytes/s).
**
** @subsection a_packet Network packet
**
** @li [IP  Header - 20 bytes]
** @li [UDP Header -  8 bytes]
** @li [Header Data:Type - 1 byte]
** if Type is one of the InitConfigMessage
** @li [Header Data:SubType - 1 byte]
** @li [Data - depend of subtype (may be 0 byte)]
** else
** @li [Header Data:Types - N-1 bytes] (for N commands)
** @li [Header Data:Cycle - 1 byte]
** @li [Data:Commands - Sum of Xi bytes for the N Commands]
**
**
** @subsection internals Putting it together
**
** All computers in play must run absolute syncron. Only user commands
** are send over the network to the other computers. The command needs
** some time to reach the other clients (lag), so the command is not
** executed immediatly on the local computer, it is stored in a delay
** queue and send to all other clients. After a delay of ::NetworkLag
** game cycles the commands of the other players are received and executed
** together with the local command. Each ::NetworkUpdates game cycles there
** must a package send, to keep the clients in sync, if there is no user
** command, a dummy sync package is send.
** If there are missing packages, the game is paused and old commands
** are resend to all clients.
**
** @section missing What features are missing
**
** @li The recover from lost packets can be improved, if the server knows
** which packets the clients have received.
**
** @li The UDP protocol isn't good for firewalls, we need also support
** for the TCP protocol.
**
** @li Add a server / client protocol, which allows more players per game.
**
** @li Lag (latency) and bandwidth are set over the commandline. This
** should be automatic detected during game setup and later during
** game automatic adapted.
**
** @li Also it would be nice, if we support viewing clients. This means
** other people can view the game in progress.
**
** @li The current protocol only uses single cast, for local LAN we
** should also support broadcast and multicast.
**
** @li Proxy and relays should be supported, to improve the playable
** over the internet.
**
** @li We can sort the command by importants, currently all commands are
** send in order, only chat messages are send if there are free slots.
**
** @li password protection the login process (optional), to prevent that
** the wrong player join an open network game.
**
** @li add meta server support, i have planned to use bnetd and its protocol.
**
** @section api API How should it be used.
**
** ::InitNetwork1()
**
** ::InitNetwork2()
**
** ::ExitNetwork1()
**
** ::NetworkSendCommand()
**
** ::NetworkSendExtendedCommand()
**
** ::NetworkEvent()
**
** ::NetworkQuit()
**
** ::NetworkChatMessage()
**
** ::NetworkEvent()
**
** ::NetworkRecover()
**
** ::NetworkCommands()
**
** ::NetworkFildes
**
** ::NetworkInSync
**
** @todo FIXME: continue docu
*/

//----------------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------------

#include "stratagus.h"

#include <stddef.h>
#include <list>

#include "network.h"

#include "actions.h"
#include "commands.h"
#include "interface.h"
#include "map.h"
#include "net_lowlevel.h"
#include "net_message.h"
#include "netconnect.h"
#include "parameters.h"
#include "player.h"
#include "replay.h"
#include "sound.h"
#include "translate.h"
#include "unit.h"
#include "unit_manager.h"
#include "unittype.h"
#include "video.h"

#include <deque>

//----------------------------------------------------------------------------
//  Declaration
//----------------------------------------------------------------------------

/**
**  Network command input/output queue.
*/
class CNetworkCommandQueue
{
public:
	CNetworkCommandQueue() : Time(0), Type(0) {}
	void Clear() { this->Time = this->Type = 0; Data.clear(); }

	bool operator == (const CNetworkCommandQueue &rhs) const
	{
		return Time == rhs.Time && Type == rhs.Type && Data == rhs.Data;
	}
	bool operator != (const CNetworkCommandQueue &rhs) const { return !(*this == rhs); }
public:
	unsigned long Time;    /// time to execute
	unsigned char Type;    /// Command Type
	std::vector<unsigned char> Data;  /// command content (network format)
};

//----------------------------------------------------------------------------
//  Variables
//----------------------------------------------------------------------------

/* static */ CNetworkParameter CNetworkParameter::Instance;

CNetworkParameter::CNetworkParameter()
{
	localHost = "127.0.0.1";
	localPort = defaultPort;
	NetworkUpdates = 5;
	NetworkLag = 2;
	timeoutInS = 45;
}

void CNetworkParameter::FixValues()
{
	NetworkUpdates = std::max(NetworkUpdates, 1u);
	NetworkLag = std::max(NetworkLag, 2u);
}

bool NetworkInSync = true;                 /// Network is in sync

CUDPSocket NetworkFildes;                  /// Network file descriptor

static unsigned long NetworkLastFrame[PlayerMax]; /// Last frame received packet

static unsigned long NetworkDelay;         /// Delay counter for recover.
static int NetworkSyncSeeds[256];          /// Network sync seeds.
static int NetworkSyncHashs[256];          /// Network sync hashs.
static CNetworkCommandQueue NetworkIn[256][PlayerMax][MaxNetworkCommands]; /// Per-player network packet input queue
static std::deque<CNetworkCommandQueue> CommandsIn;    /// Network command input queue
static std::deque<CNetworkCommandQueue> MsgCommandsIn; /// Network message input queue

#ifdef DEBUG
class CNetworkStat
{
public:
	CNetworkStat() :
		resentPacketCount(0)
	{}

	void print() const
	{
		DebugPrint("resent: %d packets\n" _C_ resentPacketCount);
	}

public:
	unsigned int resentPacketCount;
};

static void printStatistic(const CUDPSocket::CStatistic &statistic)
{
	DebugPrint("Sent: %d packets %d bytes (max %d bytes).\n"
			   _C_ statistic.sentPacketsCount _C_ statistic.sentBytesCount
			   _C_ statistic.biggestSentPacketSize);
	DebugPrint("Received: %d packets %d bytes (max %d bytes).\n" _C_
			   statistic.receivedPacketsCount _C_ statistic.receivedBytesCount
			   _C_ statistic.biggestReceivedPacketSize);
	DebugPrint("Received: %d error(s).\n" _C_ statistic.receivedErrorCount);
}

static CNetworkStat NetworkStat;
#endif

static int PlayerQuit[PlayerMax];          /// Player quit

//----------------------------------------------------------------------------
//  Mid-Level api functions
//----------------------------------------------------------------------------

/**
**  Send message to all clients.
**
**  @param packet       Packet to send.
**  @param numcommands  Number of commands.
*/
static void NetworkBroadcast(const CNetworkPacket &packet, int numcommands)
{
	const unsigned int size = packet.Size(numcommands);
	unsigned char *buf = new unsigned char[size];
	packet.Serialize(buf, numcommands);

	// Send to all clients.
	for (int i = 0; i < HostsCount; ++i) {
		const CHost host(Hosts[i].Host, Hosts[i].Port);
		NetworkFildes.Send(host, buf, size);
	}
	delete[] buf;
}

/**
**  Network send packet. Build it from queue and broadcast.
**
**  @param ncq  Outgoing network queue start.
*/
static void NetworkSendPacket(const CNetworkCommandQueue (&ncq)[MaxNetworkCommands])
{
	CNetworkPacket packet;

	// Build packet of up to MaxNetworkCommands messages.
	int numcommands = 0;
	packet.Header.Cycle = ncq[0].Time & 0xFF;
	int i;
	for (i = 0; i < MaxNetworkCommands && ncq[i].Type != MessageNone; ++i) {
		packet.Header.Type[i] = ncq[i].Type;
		packet.Command[i] = ncq[i].Data;
		++numcommands;
	}
	for (; i < MaxNetworkCommands; ++i) {
		packet.Header.Type[i] = MessageNone;
	}
	NetworkBroadcast(packet, numcommands);
}

//----------------------------------------------------------------------------
//  API init..
//----------------------------------------------------------------------------

/**
**  Initialize network part 1.
*/
void InitNetwork1()
{
	CNetworkParameter::Instance.FixValues();

	CommandsIn.clear();
	MsgCommandsIn.clear();

	NetworkFildes.Close();
	NetworkInSync = true;

	NetInit(); // machine dependent setup

	// Our communication port
	int port = CNetworkParameter::Instance.localPort;
	for (int i = 0; i < 10; ++i) {
		NetworkFildes.Open(CHost(CNetworkParameter::Instance.localHost.c_str(), port + i));
		if (NetworkFildes.IsValid()) {
			port = port + i;
			break;
		}
	}
	if (NetworkFildes.IsValid() == false) {
		fprintf(stderr, "NETWORK: No free ports %d-%d available, aborting\n", port, port + 10);
		NetExit(); // machine dependent network exit
		return;
	}
#ifdef DEBUG
	{
		char buf[128];

		gethostname(buf, sizeof(buf));
		DebugPrint("%s\n" _C_ buf);
		const std::string hostStr = CHost(buf, port).toString();
		DebugPrint("My host:port %s\n" _C_ hostStr.c_str());
	}
#endif

#if 0 // FIXME: need a working interface check
	unsigned long ips[10];
	int networkNumInterfaces = NetSocketAddr(NetworkFildes, ips, 10);
	if (networkNumInterfaces) {
		DebugPrint("Num IP: %d\n" _C_ networkNumInterfaces);
		for (int i = 0; i < networkNumInterfaces; ++i) {
			DebugPrint("IP: %d.%d.%d.%d\n" _C_ NIPQUAD(ntohl(ips[i])));
		}
	} else {
		fprintf(stderr, "NETWORK: Not connected to any external IPV4-network, aborting\n");
		ExitNetwork1();
		return;
	}
#endif
}

/**
**  Cleanup network part 1. (to be called _AFTER_ part 2 :)
*/
void ExitNetwork1()
{
	if (!IsNetworkGame()) { // No network running
		return;
	}

#ifdef DEBUG
	printStatistic(NetworkFildes.getStatistic());
	NetworkFildes.clearStatistic();
	NetworkStat.print();
#endif

	NetworkFildes.Close();
	NetExit(); // machine dependent setup

	NetworkInSync = true;
	NetPlayers = 0;
	HostsCount = 0;
}

/**
**  Initialize network part 2.
*/
void InitNetwork2()
{
	ThisPlayer->SetName(Parameters::Instance.LocalPlayerName);
	for (int i = 0; i < HostsCount; ++i) {
		Players[Hosts[i].PlyNr].SetName(Hosts[i].PlyName);
	}
	DebugPrint("Updates %d, Lag %d, Hosts %d\n" _C_
			   CNetworkParameter::Instance.NetworkUpdates _C_
			   CNetworkParameter::Instance.NetworkLag _C_ HostsCount);
	// Prepare first time without syncs.
	for (int i = 0; i != 256; ++i) {
		for (int p = 0; p != PlayerMax; ++p) {
			for (int j = 0; j != MaxNetworkCommands; ++j) {
				NetworkIn[i][p][j].Clear();
			}
		}
	}
	CNetworkCommandSync nc;
	//nc.syncHash = SyncHash;
	//nc.syncSeed = SyncRandSeed;

	for (unsigned int i = 0; i <= CNetworkParameter::Instance.NetworkLag; ++i) {
		for (int n = 0; n < HostsCount; ++n) {
			CNetworkCommandQueue (&ncqs)[MaxNetworkCommands] = NetworkIn[i][Hosts[n].PlyNr];

			ncqs[0].Time = i * CNetworkParameter::Instance.NetworkUpdates;
			ncqs[0].Type = MessageSync;
			ncqs[0].Data.resize(nc.Size());
			nc.Serialize(&ncqs[0].Data[0]);
			ncqs[1].Time = i * CNetworkParameter::Instance.NetworkUpdates;
			ncqs[1].Type = MessageNone;
		}
	}
	memset(NetworkSyncSeeds, 0, sizeof(NetworkSyncSeeds));
	memset(NetworkSyncHashs, 0, sizeof(NetworkSyncHashs));
	memset(PlayerQuit, 0, sizeof(PlayerQuit));
	memset(NetworkLastFrame, 0, sizeof(NetworkLastFrame));
}

//----------------------------------------------------------------------------
//  Commands input
//----------------------------------------------------------------------------

/**
**  Prepare send of command message.
**
**  Convert arguments into network format and place it into output queue.
**
**  @param command  Command (Move,Attack,...).
**  @param unit     Unit that receive the command.
**  @param x        optional X map position.
**  @param y        optional y map position.
**  @param dest     optional destination unit.
**  @param type     optional unit-type argument.
**  @param status   Append command or flush old commands.
**
**  @warning  Destination and unit-type shares the same network slot.
*/
void NetworkSendCommand(int command, const CUnit &unit, int x, int y,
						const CUnit *dest, const CUnitType *type, int status)
{
	CNetworkCommandQueue ncq;

	ncq.Time = GameCycle;
	ncq.Type = command;
	if (status) {
		ncq.Type |= 0x80;
	}
	CNetworkCommand nc;
	nc.Unit = UnitNumber(unit);
	nc.X = x;
	nc.Y = y;
	Assert(!dest || !type); // Both together isn't allowed
	if (dest) {
		nc.Dest = UnitNumber(*dest);
	} else if (type) {
		nc.Dest = type->Slot;
	} else {
		nc.Dest = 0xFFFF; // -1
	}
	ncq.Data.resize(nc.Size());
	nc.Serialize(&ncq.Data[0]);
	// Check for duplicate command in queue
	if (std::find(CommandsIn.begin(), CommandsIn.end(), ncq) != CommandsIn.end()) {
		return;
	}
	CommandsIn.push_back(ncq);
}

/**
**  Prepare send of extended command message.
**
**  Convert arguments into network format and place it into output queue.
**
**  @param command  Command (Move,Attack,...).
**  @param arg1     optional argument #1
**  @param arg2     optional argument #2
**  @param arg3     optional argument #3
**  @param arg4     optional argument #4
**  @param status   Append command or flush old commands.
*/
void NetworkSendExtendedCommand(int command, int arg1, int arg2, int arg3,
								int arg4, int status)
{
	CNetworkCommandQueue ncq;

	ncq.Time = GameCycle;
	ncq.Type = MessageExtendedCommand;
	if (status) {
		ncq.Type |= 0x80;
	}
	CNetworkExtendedCommand nec;
	nec.ExtendedType = command;
	nec.Arg1 = arg1;
	nec.Arg2 = arg2;
	nec.Arg3 = arg3;
	nec.Arg4 = arg4;
	ncq.Data.resize(nec.Size());
	nec.Serialize(&ncq.Data[0]);
	CommandsIn.push_back(ncq);
}

/**
**  Sends my selections to teammates
**
**  @param units  Units to send
**  @param count  Number of units to send
*/
void NetworkSendSelection(CUnit **units, int count)
{
	// Check if we have any teammates to send to
	bool hasteammates = false;
	for (int i = 0; i < HostsCount; ++i) {
		if (Players[Hosts[i].PlyNr].Team == ThisPlayer->Team) {
			hasteammates = true;
			break;
		}
	}
	if (!hasteammates) {
		return;
	}
	// Build and send packets to cover all units.
	CNetworkSelection ns;

	for (int i = 0; i != count; ++i) {
		ns.Units.push_back(UnitNumber(*units[i]));
	}
	CNetworkCommandQueue ncq;
	ncq.Time = GameCycle;
	ncq.Type = MessageSelection;

	ncq.Data.resize(ns.Size());
	ns.Serialize(&ncq.Data[0]);
	CommandsIn.push_back(ncq);
}

/**
**  Process Received Unit Selection
**
**  @param ncq  Network Packet to Process
*/
static void ParseNetworkCommand_Selection(const CNetworkCommandQueue &ncq)
{
	Assert((ncq.Type & 0x7F) == MessageSelection);

	CNetworkSelection ns;

	ns.Deserialize(&ncq.Data[0]);
	if (Players[ns.player].Team != ThisPlayer->Team) {
		return;
	}
	std::vector<CUnit *> units;

	for (size_t i = 0; i != ns.Units.size(); ++i) {
		units.push_back(&UnitManager.GetSlotUnit(ns.Units[i]));
	}
	ChangeTeamSelectedUnits(Players[ns.player], units);
}

/**
**  Remove a player from the game.
**
**  @param player  Player number
*/
static void NetworkRemovePlayer(int player)
{
	// Remove player from Hosts and clear NetworkIn
	for (int i = 0; i < HostsCount; ++i) {
		if (Hosts[i].PlyNr == player) {
			Hosts[i] = Hosts[HostsCount - 1];
			--HostsCount;
			break;
		}
	}
	for (int i = 0; i < 256; ++i) {
		for (int c = 0; c < MaxNetworkCommands; ++c) {
			NetworkIn[i][player][c].Time = 0;
		}
	}
}

static bool IsNetworkCommandReady(unsigned long gameNetCycle)
{
	// Check if all next messages are available.
	const CNetworkCommandQueue (&ncqs)[PlayerMax][MaxNetworkCommands] = NetworkIn[gameNetCycle & 0xFF];
	for (int i = 0; i < HostsCount; ++i) {
		const CNetworkCommandQueue *ncq = ncqs[Hosts[i].PlyNr];

		if (ncq[0].Time != gameNetCycle * CNetworkParameter::Instance.NetworkUpdates) {
			return false;
		}
	}
	return true;
}

static void ParseResendCommand(const CNetworkPacket &packet)
{
	// Destination cycle (time to execute).
	unsigned long n = ((GameCycle + 128) & ~0xFF) | packet.Header.Cycle;
	if (n > GameCycle + 128) {
		n -= 0x100;
	}
	const unsigned long gameNetCycle = n / CNetworkParameter::Instance.NetworkUpdates;
	// FIXME: not necessary to send this packet multiple times!!!!
	// other side sends re-send until it gets an answer.
	if (n != NetworkIn[gameNetCycle & 0xFF][ThisPlayer->Index][0].Time) {
		// Asking for a cycle we haven't gotten to yet, ignore for now
		return;
	}
	NetworkSendPacket(NetworkIn[gameNetCycle & 0xFF][ThisPlayer->Index]);
	// Check if a player quit this cycle
	for (int j = 0; j < HostsCount; ++j) {
		for (int c = 0; c < MaxNetworkCommands; ++c) {
			const CNetworkCommandQueue *ncq;
			ncq = &NetworkIn[gameNetCycle & 0xFF][Hosts[j].PlyNr][c];
			if (ncq->Time && ncq->Type == MessageQuit) {
				CNetworkPacket np;
				np.Header.Cycle = ncq->Time & 0xFF;
				np.Header.Type[0] = ncq->Type;
				np.Command[0] = ncq->Data;
				for (int k = 1; k < MaxNetworkCommands; ++k) {
					np.Header.Type[k] = MessageNone;
				}
				// FIXME : BUG? : order may differ when send alone
				NetworkBroadcast(np, 1);
			}
		}
	}
}

static bool IsAValidCommand(const CNetworkPacket &packet, int index)
{
	const int player = Hosts[index].PlyNr;

	switch (packet.Header.Type[index] & 0x7F) {
		case MessageExtendedCommand: // FIXME: ensure the sender is part of the command
			return true;
		case MessageSync: // Sync does not matter
			return true;
		case MessageSelection:
			return true;
		case MessageQuit:
		case MessageResend:
		case MessageChat:
			// FIXME: ensure it's from the right player
			return true;
		case MessageCommandDismiss: {
			CNetworkCommand nc;
			nc.Deserialize(&packet.Command[index][0]);
			const unsigned int slot = nc.Unit;
			const CUnit *unit = slot < UnitManager.GetUsedSlotCount() ? &UnitManager.GetSlotUnit(slot) : NULL;

			if (unit && unit->Type->ClicksToExplode) {
				return true;
			}
		}
		// Fall through!
		default: {
			CNetworkCommand nc;
			nc.Deserialize(&packet.Command[index][0]);
			const unsigned int slot = nc.Unit;
			const CUnit *unit = slot < UnitManager.GetUsedSlotCount() ? &UnitManager.GetSlotUnit(slot) : NULL;

			if (unit && (unit->Player->Index == player
						 || Players[player].IsTeamed(*unit))) {
				return true;
			} else {
				return false;
			}
		}
	}
	// FIXME: not all values in nc have been validated
}

static void NetworkParseInGameEvent(const unsigned char *buf, int len, const CHost &host)
{
	const int index = FindHostIndexBy(host);
	if (index == -1 || PlayerQuit[Hosts[index].PlyNr]) {
#ifdef DEBUG
		const std::string hostStr = host.toString();
		DebugPrint("Not a host in play: %s\n" _C_ hostStr.c_str());
#endif
		return;
	}
	const int player = Hosts[index].PlyNr;

	CNetworkPacket packet;
	int commands;
	packet.Deserialize(buf, len, &commands);
	if (commands < 0) {
		DebugPrint("Bad packet read\n");
		return;
	}
	// Parse the packet commands.
	for (int i = 0; i != commands; ++i) {
		// Handle some messages.
		if (packet.Header.Type[i] == MessageQuit) {
			CNetworkCommandQuit nc;
			nc.Deserialize(&packet.Command[i][0]);
			const int playerNum = nc.player;

			if (playerNum >= 0 && playerNum < NumPlayers) {
				PlayerQuit[playerNum] = 1;
			}
		}
		if (packet.Header.Type[i] == MessageResend) {
			ParseResendCommand(packet);
			return;
		}
		// Receive statistic
		NetworkLastFrame[player] = FrameCounter;

		bool validCommand = IsAValidCommand(packet, i);
		// Place in network in
		if (validCommand) {
			// Destination cycle (time to execute).
			unsigned long n = ((GameCycle + 128) & ~0xFF) | packet.Header.Cycle;
			if (n > GameCycle + 128) {
				n -= 0x100;
			}
			const unsigned long gameNetCycle = n / CNetworkParameter::Instance.NetworkUpdates;
			NetworkIn[gameNetCycle & 0xFF][player][i].Time = n;
			NetworkIn[gameNetCycle & 0xFF][player][i].Type = packet.Header.Type[i];
			NetworkIn[gameNetCycle & 0xFF][player][i].Data = packet.Command[i];
		} else {
			SetMessage(_("%s sent bad command"), Players[player].Name.c_str());
			DebugPrint("%s sent bad command: 0x%x\n" _C_ Players[player].Name.c_str()
					   _C_ packet.Header.Type[i] & 0x7F);
		}
	}
	for (int i = commands; i != MaxNetworkCommands; ++i) {
		NetworkIn[packet.Header.Cycle][player][i].Time = 0;
	}
	// Waiting for this time slot
	if (!NetworkInSync) {
		const int networkUpdates = CNetworkParameter::Instance.NetworkUpdates;
		const unsigned long nextGameNetCycle = (GameCycle / networkUpdates) + 1;
		if (IsNetworkCommandReady(nextGameNetCycle) == true) {
			NetworkInSync = true;
		}
	}
}

/**
**  Called if message for the network is ready.
**  (by WaitEventsOneFrame)
**
**  @todo
**  NetworkReceivedEarly NetworkReceivedLate NetworkReceivedDups
**  Must be calculated.
*/
void NetworkEvent()
{
	if (!IsNetworkGame()) {
		NetworkInSync = true;
		return;
	}
	// Read the packet.
	unsigned char buf[1024];
	CHost host;
	int len = NetworkFildes.Recv(&buf, sizeof(buf), &host);
	if (len < 0) {
		DebugPrint("Server/Client gone?\n");
		// just hope for an automatic recover right now..
		NetworkInSync = false;
		return;
	}

	// Setup messages
	if (NetConnectRunning) {
		if (NetworkParseSetupEvent(buf, len, host)) {
			return;
		}
	}
	const unsigned char msgtype = buf[0];
	if (msgtype == MessageInit_FromClient || msgtype == MessageInit_FromServer) {
		return;
	}
	NetworkParseInGameEvent(buf, len, host);
}

/**
**  Quit the game.
*/
void NetworkQuit()
{
	if (!ThisPlayer) {
		return;
	}
	const int NetworkUpdates = CNetworkParameter::Instance.NetworkUpdates;
	const int NetworkLag = CNetworkParameter::Instance.NetworkLag;
	const int gameNetCycle = GameCycle / NetworkUpdates;
	const int n = gameNetCycle + 1 + NetworkLag;
	CNetworkCommandQueue (&ncqs)[MaxNetworkCommands] = NetworkIn[n & 0xFF][ThisPlayer->Index];
	CNetworkCommandQuit nc;
	nc.player = ThisPlayer->Index;
	ncqs[0].Type = MessageQuit;
	ncqs[0].Time = n * CNetworkParameter::Instance.NetworkUpdates;
	ncqs[0].Data.resize(nc.Size());
	nc.Serialize(&ncqs[0].Data[0]);
	for (int i = 1; i < MaxNetworkCommands; ++i) {
		ncqs[i].Type = MessageNone;
		ncqs[i].Data.clear();
	}
	NetworkSendPacket(ncqs);
}

/**
**  Send chat message. (Message is sent with low priority)
**
**  @param msg  Text message to send.
*/
void NetworkChatMessage(const std::string &msg)
{
	if (!IsNetworkGame()) {
		return;
	}
	CNetworkChat nc;
	nc.Text = msg;
	CNetworkCommandQueue ncq;
	ncq.Type = MessageChat;
	ncq.Data.resize(nc.Size());
	nc.Serialize(&ncq.Data[0]);
	MsgCommandsIn.push_back(ncq);
}

static void ParseNetworkCommand_Sync(const CNetworkCommandQueue &ncq)
{
	Assert((ncq.Type & 0x7F) == MessageSync);

	CNetworkCommandSync nc;
	nc.Deserialize(&ncq.Data[0]);
	const unsigned long gameNetCycle = GameCycle / CNetworkParameter::Instance.NetworkUpdates;
	const int syncSeed = nc.syncSeed;
	const int syncHash = nc.syncHash;

	if (syncSeed != NetworkSyncSeeds[gameNetCycle & 0xFF]
		|| syncHash != NetworkSyncHashs[gameNetCycle & 0xFF]) {
		SetMessage("%s", _("Network out of sync"));
		DebugPrint("\nNetwork out of sync %x!=%x! %d!=%d! Cycle %lu\n\n" _C_
				   syncSeed _C_ NetworkSyncSeeds[gameNetCycle & 0xFF] _C_
				   syncHash _C_ NetworkSyncHashs[gameNetCycle & 0xFF] _C_ GameCycle);
	}
}

static void ParseNetworkCommand_Chat(const CNetworkCommandQueue &ncq)
{
	Assert((ncq.Type & 0x7F) == MessageChat);

	CNetworkChat nc;
	nc.Deserialize(&ncq.Data[0]);

	SetMessage("%s", nc.Text.c_str());
	PlayGameSound(GameSounds.ChatMessage.Sound, MaxSampleVolume);
	CommandLog("chat", NoUnitP, FlushCommands, -1, -1, NoUnitP, nc.Text.c_str(), -1);
}

/**
**  Parse a network command.
**
**  @param ncq  Network command from queue
*/
static void ParseNetworkCommand(const CNetworkCommandQueue &ncq)
{
	switch (ncq.Type & 0x7F) {
		case MessageSync: ParseNetworkCommand_Sync(ncq); break;
		case MessageSelection: ParseNetworkCommand_Selection(ncq); break;
		case MessageChat: ParseNetworkCommand_Chat(ncq); break;

		case MessageQuit: {
			CNetworkCommandQuit nc;
			nc.Deserialize(&ncq.Data[0]);
			NetworkRemovePlayer(nc.player);
			CommandLog("quit", NoUnitP, FlushCommands, nc.player, -1, NoUnitP, NULL, -1);
			CommandQuit(nc.player);
			break;
		}
		case MessageExtendedCommand: {
			CNetworkExtendedCommand nec;
			nec.Deserialize(&ncq.Data[0]);
			ParseExtendedCommand(nec.ExtendedType, (ncq.Type & 0x80) >> 7,
								 nec.Arg1, nec.Arg2, nec.Arg3, nec.Arg4);
		}
		break;
		case MessageNone:
			// Nothing to Do, This Message Should Never be Executed
			Assert(0);
			break;
		default: {
			CNetworkCommand nc;
			nc.Deserialize(&ncq.Data[0]);
			ParseCommand(ncq.Type, nc.Unit, nc.X, nc.Y, nc.Dest);
			break;
		}
	}
}

/**
**  Network send commands.
*/
static void NetworkSendCommands(unsigned long gameNetCycle)
{
	// No command available, send sync.
	int numcommands = 0;
	CNetworkCommandQueue (&ncq)[MaxNetworkCommands] = NetworkIn[gameNetCycle & 0xFF][ThisPlayer->Index];
	ncq[0].Clear();
	if (CommandsIn.empty() && MsgCommandsIn.empty()) {
		CNetworkCommandSync nc;
		ncq[0].Type = MessageSync;
		nc.syncHash = SyncHash;
		nc.syncSeed = SyncRandSeed;
		ncq[0].Data.resize(nc.Size());
		nc.Serialize(&ncq[0].Data[0]);
		ncq[0].Time = gameNetCycle * CNetworkParameter::Instance.NetworkUpdates;
		numcommands = 1;
	} else {
		while (!CommandsIn.empty() && numcommands < MaxNetworkCommands) {
			const CNetworkCommandQueue &incommand = CommandsIn.front();
#ifdef DEBUG
			if (incommand.Type != MessageExtendedCommand) {
				CNetworkCommand nc;
				nc.Deserialize(&incommand.Data[0]);

				const CUnit &unit = UnitManager.GetSlotUnit(nc.Unit);
				// FIXME: we can send destoyed units over network :(
				if (unit.Destroyed) {
					DebugPrint("Sending destroyed unit %d over network!!!!!!\n" _C_ nc.Unit);
				}
			}
#endif
			ncq[numcommands] = incommand;
			ncq[numcommands].Time = gameNetCycle * CNetworkParameter::Instance.NetworkUpdates;
			++numcommands;
			CommandsIn.pop_front();
		}
		while (!MsgCommandsIn.empty() && numcommands < MaxNetworkCommands) {
			const CNetworkCommandQueue &incommand = MsgCommandsIn.front();
			ncq[numcommands] = incommand;
			ncq[numcommands].Time = gameNetCycle * CNetworkParameter::Instance.NetworkUpdates;
			++numcommands;
			MsgCommandsIn.pop_front();
		}
	}
	if (numcommands != MaxNetworkCommands) {
		ncq[numcommands].Type = MessageNone;
	}
	NetworkSendPacket(ncq);

	NetworkSyncSeeds[gameNetCycle & 0xFF] = SyncRandSeed;
	NetworkSyncHashs[gameNetCycle & 0xFF] = SyncHash;
}

/**
**  Network excecute commands.
*/
static void NetworkExecCommands(unsigned long gameNetCycle)
{
	// Must execute commands on all computers in the same order.
	for (int i = 0; i < NumPlayers; ++i) {
		// Remove commands.
		const CNetworkCommandQueue *ncqs = NetworkIn[gameNetCycle & 0xFF][i];
		for (int c = 0; c < MaxNetworkCommands; ++c) {
			const CNetworkCommandQueue &ncq = ncqs[c];
			if (ncq.Type == MessageNone) {
				break;
			}
			if (ncq.Time) {
				Assert(ncq.Time == gameNetCycle * CNetworkParameter::Instance.NetworkUpdates);
				ParseNetworkCommand(ncq);
			}
		}
	}
}

/**
**  Handle network commands.
*/
void NetworkCommands()
{
	if (!IsNetworkGame()) {
		return;
	}
	if ((GameCycle % CNetworkParameter::Instance.NetworkUpdates) != 0) {
		return;
	}
	const unsigned long gameNetCycle = GameCycle / CNetworkParameter::Instance.NetworkUpdates;
	// Send messages to all clients (other players)
	NetworkSendCommands(gameNetCycle + CNetworkParameter::Instance.NetworkLag);
	NetworkExecCommands(gameNetCycle);
	if (IsNetworkCommandReady(gameNetCycle + 1) == false) {
		NetworkInSync = false;
		NetworkDelay = FrameCounter + CNetworkParameter::Instance.NetworkUpdates;
		// FIXME: should send a resend request.
	}
}

/**
**  Network resend commands, we have a missing packet send to all clients
**  what packet we are missing.
**
**  @todo
**  We need only send to the clients, that have not delivered the packet.
*/
static void NetworkResendCommands()
{
#ifdef DEBUG
	++NetworkStat.resentPacketCount;
#endif

	const int networkUpdates = CNetworkParameter::Instance.NetworkUpdates;
	const int nextGameCycle = ((GameCycle / networkUpdates) + 1) * networkUpdates;
	// Build packet
	CNetworkPacket packet;
	packet.Header.Type[0] = MessageResend;
	packet.Header.Type[1] = MessageNone;
	packet.Header.Cycle = uint8_t(nextGameCycle & 0xFF);

	NetworkBroadcast(packet, 1);
}

/**
**  Recover network.
*/
void NetworkRecover()
{
	if (HostsCount == 0) {
		NetworkInSync = true;
		return;
	}
	if (FrameCounter <= NetworkDelay) {
		return;
	}
	NetworkDelay += CNetworkParameter::Instance.NetworkUpdates;

	// Check for players that timed out
	for (int i = 0; i < HostsCount; ++i) {
		const int playerIndex = Hosts[i].PlyNr;
		const unsigned long lastFrame = NetworkLastFrame[playerIndex];
		if (!lastFrame) {
			continue;
		}
		const int framesPerSecond = FRAMES_PER_SECOND * VideoSyncSpeed / 100;
		const int secs = (FrameCounter - lastFrame) / framesPerSecond;
		// FIXME: display a menu while we wait
		const int timeoutInS = CNetworkParameter::Instance.timeoutInS;
		if (3 <= secs && secs < timeoutInS) {
			if (FrameCounter % framesPerSecond == 0) {
				SetMessage(_("Waiting for player \"%s\": %d:%02d"), Hosts[i].PlyName,
						   (timeoutInS - secs) / 60, (timeoutInS - secs) % 60);
			}
		}
		if (secs >= timeoutInS) {
			const unsigned int nextGameNetCycle = GameCycle / CNetworkParameter::Instance.NetworkUpdates + 1;
			CNetworkCommandQuit nc;
			nc.player = playerIndex;
			CNetworkCommandQueue *ncq = &NetworkIn[nextGameNetCycle & 0xFF][playerIndex][0];
			ncq->Time = nextGameNetCycle * CNetworkParameter::Instance.NetworkUpdates;
			ncq->Type = MessageQuit;
			ncq->Data.resize(nc.Size());
			nc.Serialize(&ncq->Data[0]);
			PlayerQuit[playerIndex] = 1;
			SetMessage("%s", _("Timed out"));

			CNetworkPacket np;
			np.Header.Cycle = ncq->Time & 0xFF;
			np.Header.Type[0] = ncq->Type;
			np.Header.Type[1] = MessageNone;

			NetworkBroadcast(np, 1);

			if (IsNetworkCommandReady(nextGameNetCycle) == false) {
				NetworkInSync = false;
				NetworkDelay = FrameCounter + CNetworkParameter::Instance.NetworkUpdates;
				// FIXME: should send a resend request.
			}
		}
	}
	// Resend old commands
	NetworkResendCommands();
}

//@}
