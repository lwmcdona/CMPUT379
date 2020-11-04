#ifndef PACKETS_H
#define PACKETS_H

#include "libraries.h"


// action type values
enum action {DROP, FORWARD};
const string ACTIONNAME[2] = {"DROP", "FORWARD"};

// packet types
enum packetType {OPEN, ACK, QUERY, ADD, RELAY, ADMIT, RELAYIN, RELAYOUT, QUEUEDQUERY, QUEUEDRELAY, EXIT};
const string PACKETNAME[11] = {"OPEN", "ACK", "QUERY", "ADDRULE", "RELAY", "ADMIT", "RELAYIN", "RELAYOUT", "QUEUEDQUERY", "QUEUEDRELAY", "EXIT"};

// the packet stats struct used for storing number of sent and received packets for both switch and controller
struct packetStats
{
    map<packetType, int> received;
    map<packetType, int> transmitted;
};

/* PACKET DECLARATIONS
Note: 
-ACK packet has no message
-RELAY, ADMIT, QUERY, QUEUEDQUERY, QUEUEDRELAY are all of type queryRelayMessage
-ADD packet message is a flowTableEntry
-OPEN is of type openMessage
*/
struct flowTableEntry
{
    int srcIPLo;
    int srcIPHi;
    int destIPLo;
    int destIPHi;
    action actionType;
    int actionVal;
    int pri;
    int pktCount; 
};

// switch sends this on its creation
struct openMessage
{
    int switchNumber;
    int port1Switch;
    int port2Switch;
    int ipLow;
    int ipHigh;
};

struct queryRelayMessage
{
    int sendingSwitchNumber; // optional parameter used only for QUEUEDQUERY types
    int srcIP;
    int destIP;    
};

union message
{
    openMessage oMessage;
    queryRelayMessage qrMessage;
    flowTableEntry aMessage;
};

struct packet
{
    packetType type;
    message msg; 
};
// end packet declarations

// print message function declarations
void printMessage(queryRelayMessage msg);

void printMessage(openMessage msg);

void printMessage(flowTableEntry msg);

void printPacketMessage(int source, int destination, packet printPacket, bool transmitted = false);
// end print message function declarations

// create packet function declarations
packet createAMessagePacket(packetType type, int srcIPLo, int srcIPHi, int destIPLo, int destIPHi, action actionType, int actionVal, int pri, int pktCount);

packet createQRMessagePacket(packetType type, int srcIP, int destIP);

packet createOMessagePacket(packetType type, int switchNumber, int port1Switch, int port2Switch, int ipLow, int ipHigh);
// end create packet function declarations


#endif