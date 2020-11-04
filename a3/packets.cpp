#include "packets.h"
#include "constants.h"

// prints a queryRelayMessage type message
void printMessage(queryRelayMessage msg)
{
    cout << "(srcIP= " << msg.srcIP << ", destIP= " << msg.destIP << ")" << endl;
}

// prints an openMessage type message
void printMessage(openMessage msg)
{
    openMessage sw = msg;
    cout << "(port0= cont, port1= " << sw.port1Switch <<
                        ", port2= " << sw.port2Switch <<
                        ", port3= " << sw.ipLow << "-" << sw.ipHigh << ")" << endl;
}

// prints a flowTableEntry type message
void printMessage(flowTableEntry msg)
{
    flowTableEntry ft = msg;
    cout <<                 "(srcIP= "    << ft.srcIPLo  << "-" << ft.srcIPHi  <<
                            ", destIP= "   << ft.destIPLo << "-" << ft.destIPHi << 
                            ", action= "   << ACTIONNAME[ft.actionType] << ":" << ft.actionVal <<
                            ", pri= "      << ft.pri <<
                            ", pktCount= " << ft.pktCount << ")" << endl;
}

// prints a packet message based on whether it is being received or transmitted
void printPacketMessage(int source, int destination, packet printPacket, bool transmitted)
{
    string src;
    string dest;
    stringstream ss;

    // determine who the message came from based upon the source 
    if (source == 0) 
    {
        src = "cont";
    }
    else if (source == FILEPORT)
    {
        src = "file";
    }
    else
    {
        ss << source;
        src = "sw" + ss.str();
    }

    // determine who to send the message to based upon the destination
    ss.str("");
    if (destination == 0)
    {
        dest = "cont";
    }
    else if (destination == NETPORT)
    {
        dest = "net";
    }
    else 
    {
        ss << destination;
        dest = "sw" + ss.str();
    }

    // determine if the message is being received or transimitted
    if (transmitted)
    {
        cout << "Transmitted ";
    }
    else
    {
        cout << "Received ";
    }

    // print the packet information based on its type
    cout << "(src= " << src << ", dest= " << dest << ") [" << PACKETNAME[printPacket.type] << "]";
    if (printPacket.type != ACK && printPacket.type != EXIT)
    {
        cout << ": " << endl << "      ";
        if (printPacket.type == ADD)
        {
            printMessage(printPacket.msg.aMessage);
        }
        else if (printPacket.type == OPEN)
        {
            printMessage(printPacket.msg.oMessage);
        }
        else 
        {
            printMessage(printPacket.msg.qrMessage);
        }
    }
    else 
    {
        cout << endl;
    }
}
// end print message functions

// create packet functions
// create a packet with the given type and a flowTableEntry/addMessage
packet createAMessagePacket(packetType type, int srcIPLo, int srcIPHi, int destIPLo, int destIPHi, action actionType, int actionVal, int pri, int pktCount)
{
    message msg;
    msg.aMessage.srcIPLo = srcIPLo;
    msg.aMessage.srcIPHi = srcIPHi;
    msg.aMessage.destIPLo = destIPLo;
    msg.aMessage.destIPHi = destIPHi;
    msg.aMessage.actionType = actionType;
    msg.aMessage.actionVal = actionVal;
    msg.aMessage.pri = pri;
    msg.aMessage.pktCount = pktCount;

    packet outPacket = {.type = type, .msg = msg};
    return outPacket;
}

// create a packet with the given type and a queryRelayMessage
packet createQRMessagePacket(packetType type, int srcIP, int destIP)
{
    message msg;
    msg.qrMessage.srcIP = srcIP;
    msg.qrMessage.destIP = destIP;

    packet outPacket = {.type = type, .msg = msg};
    return outPacket;
}

// create a packet with the given type and an openMessage
packet createOMessagePacket(packetType type, int switchNumber, int port1Switch, int port2Switch, int ipLow, int ipHigh)
{
    message msg;
    msg.oMessage.switchNumber = switchNumber;
    msg.oMessage.port1Switch = port1Switch;
    msg.oMessage.port2Switch = port2Switch;
    msg.oMessage.ipLow = ipLow;
    msg.oMessage.ipHigh = ipHigh;

    packet outPacket = {.type = type, .msg = msg};
    return outPacket;
}
// end create packet functions