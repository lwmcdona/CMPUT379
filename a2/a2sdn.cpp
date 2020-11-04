/* 
Code written by Logan McDonald

Acknowledgements: 
Experiments with sending and receiving formatted messages on CMPUT 379 website
for packet code development
*/

#include <iostream>
#include <stdlib.h> //atoi()
#include <string>
#include <poll.h> // poll()
#include <unistd.h> //STDIN_FILENO, STDOUT_FILENO
#include <signal.h> //SIGUSR1, SIG_ERR
#include <map>
#include <vector>
#include <queue>
#include <set>
#include <sstream>
#include <iterator>
#include <stdio.h> // fopen, fdopen, fread, fwrite
#include <fcntl.h> //open

using namespace std;

#define MAXLINE 1000
#define MAX_NSW 7
#define MAXIP 1000
#define MINPRI 4 // not tested in assignment, important when controller issues overlapping rules

#define MAX_FLOWTABLE_SIZE 100

// action type values
enum action {DROP, FORWARD};
string ACTIONNAME[2] = {"DROP", "FORWARD"};

// possible packet types
enum packetType {OPEN, ACK, QUERY, ADD, RELAY, ADMIT, RELAYIN, RELAYOUT, QUEUEDQUERY, QUEUEDRELAY, EXIT};
string PACKETNAME[11] = {"OPEN", "ACK", "QUERY", "ADDRULE", "RELAY", "ADMIT", "RELAYIN", "RELAYOUT", "QUEUEDQUERY", "QUEUEDRELAY", "EXIT"};

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

// global variables
queue<packet> packetQueue;              // queue comprised of queryRelayMessage type packets
vector<message> connectionInfo;         // controller: switch table comprised of openMessage types
                                        // switch: flow table comprised of flowTableEntry types
                                         
packetStats pktStats;                   // tracks the number of packets sent and received
set< pair<bool, int> > pendingQuerySet; // used in switches to avoid sending duplicate queries

bool isSwitch;                          // used in printing and signal handling
bool acknowledged = false;              // if a switch has been added to the network
// end global variables

// function headers
void processPacketQueue(int currSwitchNumber, int port1Switch = -2, int port2Switch = -2);
bool sendPacket(int sender, int receiver, packet outPacket);
// end function headers

// print message functions
void printMessage(queryRelayMessage msg)
{
    cout << "srcIP= " << msg.srcIP << ", destIP= " << msg.destIP << endl;
}

void printMessage(openMessage msg)
{
    openMessage sw = msg;
    cout << "[sw" << sw.switchNumber << "] port1= " << sw.port1Switch <<
                                        ", port2= " << sw.port2Switch <<
                                        ", port3= " << sw.ipLow << "-" << sw.ipHigh << endl;
}

void printMessage(flowTableEntry msg)
{
    flowTableEntry ft = msg;
    cout <<                 "(srcIP= "    << ft.srcIPLo  << "-" << ft.srcIPHi  <<
                            ", destIP= "   << ft.destIPLo << "-" << ft.destIPHi << 
                            ", action= "   << ACTIONNAME[ft.actionType] << ":" << ft.actionVal <<
                            ", pri= "      << ft.pri <<
                            ", pktCount= " << ft.pktCount << ")" << endl;
}
// end print message functions

// prints information in connectionInfo for the controller
// the connected switch information
void listControllerInfo()
{

    cout << "Switch Information: " << endl;
    for (vector<message>::iterator it = connectionInfo.begin(); it != connectionInfo.end(); ++it)
    {
        openMessage sw = it->oMessage;
        cout << "[sw" << sw.switchNumber << "] port1= " << sw.port1Switch <<
                                            ", port2= " << sw.port2Switch <<
                                            ", port3= " << sw.ipLow << "-" << sw.ipHigh << endl;
    }
    cout << endl; 
}

// prints information in connectionInfo for the switch
// the flow table rules
void listSwitchInfo()
{
    int counter = 0;
    cout << "Flow Table: " << endl;
    for (vector<message>::iterator it = connectionInfo.begin(); it != connectionInfo.end(); ++it)
    {
        flowTableEntry ft = it->aMessage;
        cout << "[" << counter << "] (srcIP= "    << ft.srcIPLo  << "-" << ft.srcIPHi  <<
                                   ", destIP= "   << ft.destIPLo << "-" << ft.destIPHi << 
                                   ", action= "   << ACTIONNAME[ft.actionType] << ":" << ft.actionVal <<
                                   ", pri= "      << ft.pri <<
                                   ", pktCount= " << ft.pktCount << ")" << endl;
        counter += 1;
    }
    cout << endl; 
}

// list received and transmitted packet information
void listPacketStats()
{
    bool firstIteration = true;
    cout << "Packet Stats: " << endl;
    cout << "   Received:    ";
    for (map<packetType, int>::iterator it = pktStats.received.begin(); it != pktStats.received.end(); ++it)
    {   
        packetType type = it->first;
        int number = it->second;

        if (firstIteration) 
        {
            cout << PACKETNAME[type] << ":" << number;
            firstIteration = false; 
        }
        else 
        {
            cout << ", " << PACKETNAME[type] << ":" << number; 
        }
    }
    firstIteration = true;
    cout << endl << "   Transmitted: ";
    for (map<packetType, int>::iterator it = pktStats.transmitted.begin(); it != pktStats.transmitted.end(); ++it)
    {   
        packetType type = it->first;
        int number = it->second;

        if (firstIteration) 
        {
            cout << PACKETNAME[type] << ":" << number;
            firstIteration = false; 
        }
        else 
        {
            cout << ", " << PACKETNAME[type] << ":" << number; 
        }
    }
    cout << endl; 
}

// list the information for either the switch or the controller
void listInfo() 
{
    cout << endl;
    if (isSwitch) 
    {
        listSwitchInfo();
    }
    else
    {
        // controller
        listControllerInfo();
    }
    listPacketStats();
}

// the SIGUSR1 signal handler for both switch and controller
void user1SignalHandler(int signo) 
{
    if (signo == SIGUSR1) 
    {
        listInfo();
    }
    else
    {
        cout << "Received signal: " << signo << ". Unable to handle this signal." << endl;
    }
}

// initialize the packet stats for the controller
void initializeControllerPacketStats()
{
    pktStats.received.insert(pair<packetType, int>(OPEN, 0));
    pktStats.received.insert(pair<packetType, int>(QUERY, 0));

    pktStats.transmitted.insert(pair<packetType, int>(ACK, 0));
    pktStats.transmitted.insert(pair<packetType, int>(ADD, 0));
    pktStats.transmitted.insert(pair<packetType, int>(EXIT, 0));
}

// initialize the packet stats for the switches
void initializeSwitchPacketStats()
{
    pktStats.received.insert(pair<packetType, int>(ADMIT, 0));
    pktStats.received.insert(pair<packetType, int>(ACK, 0));
    pktStats.received.insert(pair<packetType, int>(ADD, 0));
    pktStats.received.insert(pair<packetType, int>(RELAYIN, 0));
    pktStats.received.insert(pair<packetType, int>(EXIT, 0));

    pktStats.transmitted.insert(pair<packetType, int>(OPEN, 0));
    pktStats.transmitted.insert(pair<packetType, int>(QUERY, 0));
    pktStats.transmitted.insert(pair<packetType, int>(RELAYOUT, 0));
}

// close the open fifos and exit the program
void exitFunction(struct pollfd FIFOS[], int numFIFOS) 
{   
    for (int i = 0; i < numFIFOS; ++i)
    {
        close(FIFOS[i].fd);
    }
    exit(0);
}

// polls the user input for either the list or the exit command
void pollUserInput(struct pollfd FIFOS[], int numFIFOS)
{
    string userInput;
    struct pollfd keyboard[1];
    keyboard[0].fd = STDIN_FILENO;
    keyboard[0].events = POLLIN | POLLPRI;
    if (poll(keyboard, 1, 0) > 0) 
    {
        // received some form of input
        getline(cin, userInput);

        // poll the keyboard for user input

        if ((userInput.compare("list") == 0) || (userInput.compare("exit") == 0)) 
        {   
            listInfo();
            if (userInput.compare("exit") == 0)
            {
                cout << "Exiting..." << endl << endl;
                exitFunction(FIFOS, numFIFOS);
            }
        }
        else 
        {
            cout << "Unrecognized user input." << endl;
        }
    }
}

// controller check to avoid duplicate switches
bool switchNumberNotInUse(int swNumber)
{   
    bool validNumber = true;
    for (vector<message>::iterator it = connectionInfo.begin(); it != connectionInfo.end(); ++it)
    {
        if (it->oMessage.switchNumber == swNumber) 
        {
            validNumber = false;

            // shut down the switch
            cout << "An invalid switch has attempted to connect. It was rejected." << endl;
            packet outPacket;
            outPacket.type = EXIT;
            sendPacket(0, it->oMessage.switchNumber, outPacket);
            pktStats.transmitted[EXIT] += 1;

            break;
        }
    }
    return validNumber;
}

// controller adds a switch to the network, orders it to simplify forwarding later
void addSwitch(message msg)
{
    bool success = true;
    vector<message>::iterator it;
    openMessage swNew = msg.oMessage;
    if (connectionInfo.empty())
    {
        // there are no switches currently in the network
        connectionInfo.push_back(msg);
    }
    else
    {   
        // there is at least one switch in the network
        if (swNew.port1Switch == -1 && swNew.port2Switch == -1)
        {   
            // fatal error: both ports on new switch are null
            success = false;
        }
        else if (swNew.port1Switch == -1 && swNew.port2Switch > 0)
        {
            // port1 on the new switch is null
            if (connectionInfo[0].oMessage.port1Switch == -1) 
            {
                // fatal error: there are two switches with a null port1
                success = false;
            }
            else 
            {
                // no switch with null port1 yet, add new switch to the beginning
                connectionInfo.insert(connectionInfo.begin(), msg);
            }
        }
        else if (swNew.port1Switch > 0 && swNew.port2Switch == -1)
        {
            // port2 on the new switch is null
            if (connectionInfo[connectionInfo.size()-1].oMessage.port2Switch == -1) 
            {
                // fatal error: there are two switches with a null port2
                success = false;
            }
            else 
            {
                // no switch with null port2 yet, add new switch to the end
                connectionInfo.push_back(msg);
            }
        }
        else if (swNew.port1Switch > 0 && swNew.port2Switch > 0)
        {
            // our new switch has two valid ports, try to place it
            for (int i = 0; i < connectionInfo.size(); ++i)
            {
                openMessage sw = connectionInfo[i].oMessage;
                if (sw.port1Switch == -1 && sw.port2Switch == -1) 
                {
                    // The existing switch cannot connect to the new switch
                    success = false;
                }
                else if (swNew.switchNumber == sw.port1Switch)
                {
                    if (swNew.port2Switch == sw.switchNumber)
                    {
                        // the two switches match, place the new switch before the current switch
                        connectionInfo.insert(connectionInfo.begin() + i, msg);
                        break;
                    }
                    else 
                    {
                        // fatal error: the switches should connect but don't
                        success = false;
                    }
                } 
                else if (swNew.switchNumber == sw.port2Switch)
                {
                    if (swNew.port1Switch == sw.switchNumber) 
                    {
                        // the two switches match, place the new switch after the current switch
                        connectionInfo.insert(connectionInfo.begin() + i + 1, msg);
                        break;
                    }
                    else 
                    {
                        // fatal error: the switches should connect but don't
                        success = false;
                    }
                }
                else if (i == connectionInfo.size()-1)
                {
                    // switch could not be placed anywhere, so just place based on switch number and validate later
                    for (int j = 0; j < connectionInfo.size() - 1; ++j)
                    {
                        openMessage sw2 = connectionInfo[i].oMessage;
                        if (swNew.switchNumber > connectionInfo[i].oMessage.switchNumber && swNew.switchNumber < connectionInfo[i+1].oMessage.switchNumber)
                        {    
                            connectionInfo.insert(connectionInfo.begin() + i + 1, msg);
                            break;
                        }
                    }
                }
            }
        }
    }
    if (!success)
    {
        cout << "Invalid ports for new switch. Nowhere to place. Network failure." << endl;
        packet outPacket;
        outPacket.type = EXIT;
        // shut down the network
        for (int i = 0; i < connectionInfo.size(); ++i)
        {
            sendPacket(0, connectionInfo[i].oMessage.switchNumber, outPacket);
            pktStats.transmitted[EXIT] += 1;
        }
        sendPacket(0, swNew.switchNumber, outPacket);
        pktStats.transmitted[EXIT] += 1;
        listInfo();
        cout << "Exiting..." << endl;
        exit(0);
    }
}

// controller calls this once the correct number of switches have joined the network
// steps through the network and verifies the order of switches and ports
// for correctness
void verifyNetwork()
{   
    bool valid = true;
    for (int i = 0; i < connectionInfo.size(); ++i)
    {
 
        if (i == connectionInfo.size()-1)
        {
            if (connectionInfo[i].oMessage.port2Switch != -1) 
            {
                cout << "Only one switch" << endl;
                valid = false;
            }
        }
        else
        {
            if (i == 0) 
            {
                if (connectionInfo[i].oMessage.port1Switch != -1) 
                {
                    valid = false;
                }
            }
            if (connectionInfo[i].oMessage.switchNumber != connectionInfo[i+1].oMessage.port1Switch || connectionInfo[i].oMessage.port2Switch != connectionInfo[i+1].oMessage.switchNumber)
            {
                valid = false;
            }
        }
    }
    if (!valid)
    {
        cout << "WARNING: Switch numbering scheme is skewed. Network may be incorrect." << endl;
    }
}

// creates a fifo name for a sender to send to a receiver
string determineFIFOName(int sender, int receiver)
{
    //cout << "Sender: " << sender << ", Receiver: " << receiver << endl;
    stringstream ss;
    ss << sender;
    string fifo = "fifo--";
    fifo.insert(5, ss.str());
    
    ss.str("");
    ss << receiver;
    fifo.insert(7, ss.str());

    return fifo;
}

// sends a packet to a receiver
// retries 3 times to connect to a given fifo
bool sendPacket(int sender, int receiver, packet outPacket)
{
    string fifo = determineFIFOName(sender, receiver);

    int fd;

    if ((fd = open(fifo.c_str(), O_WRONLY)) < 0)
    {
        cout << "Unable to open fifo " << fifo << " for write." << endl;
        return false;
    }

    ssize_t numBytes;
    if ((numBytes = write(fd, (char *) &outPacket, sizeof(outPacket))) < 0)
    {
        cout << "Unable to write packet to " << fifo << endl;
        return false;
    }
    else 
    {
        cout << "Sent packet of type " << PACKETNAME[outPacket.type] << " from " << sender << " to " << receiver << endl;
    }
    
    close(fd);
    return true;
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

// create a packet with the given type and a queryRelayMessage
packet createQRMessagePacket(packetType type, int srcIP, int destIP)
{
    message msg;
    msg.qrMessage.srcIP = srcIP;
    msg.qrMessage.destIP = destIP;

    packet outPacket = {.type = type, .msg = msg};
    return outPacket;
}

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

// controller processes a query packet in the network and return a corresponding ADD packet
// to sent to a switch as a new rule
packet processQueryPacket(queryRelayMessage qrMessage, int switchNumber)
{
    // ip addresses are disjoint
    int ipLow;
    int ipHigh;
    int forwardPort;
    bool destBeforeSwitch;
    bool foundDest = false;
    int srcIP = qrMessage.srcIP;
    int destIP = qrMessage.destIP;

    // ip address is greater than 1000 so drop the packet
    if (srcIP > MAXIP)
    {
        return createAMessagePacket(ADD, srcIP, srcIP, destIP, destIP, DROP, 0, MINPRI, 0);
    }

    for (vector<message>::iterator it = connectionInfo.begin(); it != connectionInfo.end(); ++it)
    {
        if (it->oMessage.switchNumber == switchNumber)
        {   
            destBeforeSwitch = true;
        }
        else if (destIP >= it->oMessage.ipLow && destIP <= it->oMessage.ipHigh) 
        {
            // the destination IP is in the range, this is the switch to send to
            destBeforeSwitch = false; 
            foundDest = true;
            ipLow = it->oMessage.ipLow; 
            ipHigh = it->oMessage.ipHigh;          
        }
    }
    if (destBeforeSwitch) 
    {
        forwardPort = 1;
    }
    else 
    {
        forwardPort = 2;
    }

    if (foundDest)
    {
        return createAMessagePacket(ADD, 0, MAXIP, destIP, destIP, FORWARD, forwardPort, MINPRI, 0);
    }
    return createAMessagePacket(ADD, 0, MAXIP, destIP, destIP, DROP, 0, MINPRI, 0);
}

// switches search their respective flow table for a valid rule and set the outPort
// value
bool processRelayPacket(queryRelayMessage qrMessage, int &outPort)
{
    int foundIndex = -1;
    for (int i = 0; i < connectionInfo.size(); ++i)
    {
        int pri = MINPRI; 
        int srcIPLo = connectionInfo[i].aMessage.srcIPLo;
        int srcIPHi = connectionInfo[i].aMessage.srcIPHi;
        int destIPLo = connectionInfo[i].aMessage.destIPLo;
        int destIPHi = connectionInfo[i].aMessage.destIPHi;
        if (qrMessage.srcIP >= srcIPLo && qrMessage.srcIP <= srcIPHi && qrMessage.destIP >= destIPLo && qrMessage.destIP <= destIPHi)
        {
            // found a matching rule, use the last one in the case of matching priority 
            if (connectionInfo[i].aMessage.pri >= pri)
            {
                pri = connectionInfo[i].aMessage.pri;
                foundIndex = i;
            }
        }
    }
    if (foundIndex > -1) 
    {
        connectionInfo[foundIndex].aMessage.pktCount += 1;
        outPort = connectionInfo[foundIndex].aMessage.actionVal;
        return true;
    }
    outPort = 0;
    return false;
}

// the main function for processing all types of packets for both the controller and switches
bool processPacket(packet inPacket, int currSwitchNumber, int port1Switch = -2, int port2Switch = -2, int sendingSwitchNumber = -2, int numSwitches = 0)
{   
    cout << endl;
    packet outPacket;
    int outPort;
    bool status = true;

    packetType type = inPacket.type;
    message msg = inPacket.msg;
    if (type != QUEUEDQUERY && type != QUEUEDRELAY)
    {
        cout << "Packet received";
        if (sendingSwitchNumber == -2)
        {
            cout << ": ";
        }
        else if (sendingSwitchNumber == 0)
        {
            cout << " from controller: ";
        }
        else 
        {
            cout << " from sw" << sendingSwitchNumber << ": ";
        }
        cout << PACKETNAME[inPacket.type] << endl;
    }

    switch(inPacket.type) 
    {
        // controller packets
        case OPEN:
            printMessage(msg.oMessage);
            pktStats.received[OPEN] += 1;
            if (switchNumberNotInUse(msg.oMessage.switchNumber))
            {   
                if (connectionInfo.size() < numSwitches) 
                {
                    addSwitch(msg);
                }
                else
                {
                    cout << "Received an open packet for a new switch when max number of switches are already in use. The new switch was ignored." << endl;
                    outPacket.type = EXIT;
                    status = sendPacket(currSwitchNumber, msg.oMessage.switchNumber, outPacket);
                    pktStats.transmitted[EXIT] += 1;
                    return false;
                }
                outPacket.type = ACK;
                status = sendPacket(currSwitchNumber, msg.oMessage.switchNumber, outPacket);
                pktStats.transmitted[ACK] += 1;
                if (connectionInfo.size() == numSwitches)
                {
                    // all switches have connected, we can process all queries that have come in
                    verifyNetwork();
                    cout << "All switches have connected. Processing query queue..." << endl;
                    processPacketQueue(currSwitchNumber, -1, -1);
                }
            }
            break;
        case QUERY:
            printMessage(msg.qrMessage);
            pktStats.received[QUERY] += 1;
            if (connectionInfo.size() < numSwitches)
            {
                // all switches have not connected yet, add the packet to the queue
                inPacket.type = QUEUEDQUERY;
                inPacket.msg.qrMessage.sendingSwitchNumber = sendingSwitchNumber;
                packetQueue.push(inPacket);
                cout << "Waiting for additional switches to connect. Packet was added to the queue." << endl;
                cout << "Number of packets in queue: " << packetQueue.size() << endl;
                return false;
            }
            // all switches have connected
            // process the query, send back the new rule
            outPacket = processQueryPacket(msg.qrMessage, sendingSwitchNumber);
            status = sendPacket(currSwitchNumber, sendingSwitchNumber, outPacket);
            pktStats.transmitted[ADD] += 1;
            break;  
        case QUEUEDQUERY:
            cout << "Processing packet: ";
            printMessage(msg.qrMessage);
            outPacket = processQueryPacket(msg.qrMessage, sendingSwitchNumber);
            status = sendPacket(currSwitchNumber, sendingSwitchNumber, outPacket);
            pktStats.transmitted[ADD] += 1;
            break; 

        // switch packets
        case ACK:
            pktStats.received[ACK] += 1;
            acknowledged = true;
            break;
        case ADD:
            printMessage(msg.aMessage);
            pktStats.received[ADD] += 1;
            // add the packet message to connection info as a new rule
            cout << "New rule added to flow table. Processing queue with new rule..." << endl;
            connectionInfo.push_back(msg);

            // process the waiting RELAY and ADMIT packet queue
            processPacketQueue(currSwitchNumber, port1Switch, port2Switch);
            break;
        case RELAY:
            printMessage(msg.qrMessage);
            pktStats.received[RELAYIN] += 1;
            if (!processRelayPacket(msg.qrMessage, outPort)) // rule was not found
            {
                cout << "No rule found. Adding to queue." << endl;
                // change type and add to queue
                inPacket.type = QUEUEDRELAY;
                packetQueue.push(inPacket);
                cout << "Number of packets in queue: " << packetQueue.size() << endl;
                
                // send query packet to controller if necessary
                bool ltsrcIP;
                if (msg.qrMessage.srcIP > MAXIP) 
                    ltsrcIP = false;
                else
                    ltsrcIP = true;
                if ((pendingQuerySet.count(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP))) == 0)
                {
                    pendingQuerySet.insert(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP));
                    inPacket.type = QUERY;
                    status = sendPacket(currSwitchNumber, 0, inPacket);
                    pktStats.transmitted[QUERY] += 1;
                    return status;
                }
                cout << "Waiting for duplicate query. No packet was sent." << endl;
                return true;
            }
            // follow the rule on packet
            if (outPort == 1)
            {
                inPacket.type = RELAY;
                status = sendPacket(currSwitchNumber, port1Switch, inPacket);
                pktStats.transmitted[RELAYOUT] += 1;
            }
            else if (outPort == 2)
            {
                inPacket.type = RELAY;
                status = sendPacket(currSwitchNumber, port2Switch, inPacket);
                pktStats.transmitted[RELAYOUT] += 1;
            }
            else if (outPort == 3) 
            {
                cout << "Packet delivered." << endl;
            }
            else if (outPort == 0) 
            {
                cout << "Packet dropped." << endl;
            }
            break;
        case ADMIT:
            printMessage(msg.qrMessage);
            pktStats.received[ADMIT] += 1;
            if (!processRelayPacket(msg.qrMessage, outPort)) // rule was not found
            {
                cout << "No rule found. Adding to queue." << endl;
                // change type and add to queue
                inPacket.type = QUEUEDRELAY;
                packetQueue.push(inPacket);
                cout << "Number of packets in queue: " << packetQueue.size() << endl;

                // send query packet to controller if necessary
                bool ltsrcIP;
                if (msg.qrMessage.srcIP > MAXIP) 
                    ltsrcIP = false;
                else
                    ltsrcIP = true;
                if ((pendingQuerySet.count(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP))) == 0)
                {
                    pendingQuerySet.insert(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP));
                    inPacket.type = QUERY;
                    status = sendPacket(currSwitchNumber, 0, inPacket);
                    pktStats.transmitted[QUERY] += 1;
                    return status;
                }
                cout << "Waiting for duplicate query. No packet was sent." << endl;
                return true;
            }
            // follow the rule on packet
            if (outPort == 1)
            {
                inPacket.type = RELAY;
                status = sendPacket(currSwitchNumber, port1Switch, inPacket);
                pktStats.transmitted[RELAYOUT] += 1;
            }
            else if (outPort == 2)
            {
                inPacket.type = RELAY;
                status = sendPacket(currSwitchNumber, port2Switch, inPacket);
                pktStats.transmitted[RELAYOUT] += 1;
            }
            else if (outPort == 3) 
            {
                cout << "Packet delivered." << endl;
            }
            else if (outPort == 0) 
            {
                cout << "Packet dropped." << endl;
            }
            break;
        case QUEUEDRELAY:
            cout << "Processing packet: ";
            printMessage(msg.qrMessage);
            if (!processRelayPacket(msg.qrMessage, outPort)) // rule was not found
            {
                cout << "No rule found. Packet returned to queue." << endl;
                // put the packet back on the queue
                packetQueue.push(inPacket);
                return false;
            }
            cout << "Rule found. Delivering packet." << endl;
            // remove from the pending query set
            bool ltsrcIP;
            if (msg.qrMessage.srcIP > MAXIP) 
                ltsrcIP = false;
            else
                ltsrcIP = true;
            pendingQuerySet.erase(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP));
            // follow the rule on packet
            if (outPort == 1)
            {
                inPacket.type = RELAY;
                status = sendPacket(currSwitchNumber, port1Switch, inPacket);
                pktStats.transmitted[RELAYOUT] += 1;
            }
            else if (outPort == 2)
            {
                inPacket.type = RELAY;
                status = sendPacket(currSwitchNumber, port2Switch, inPacket);
                pktStats.transmitted[RELAYOUT] += 1;
            }
            else if (outPort == 3) 
            {
                cout << "Packet delivered." << endl;
            }
            else if (outPort == 0) 
            {
                cout << "Packet dropped." << endl;
            }
            break;
        case EXIT:
            pktStats.received[EXIT] += 1;
            cout << "Network connection problem. Controller forced an exit." << endl;
            listInfo();
            cout << "Exiting..." << endl << endl;
            exit(0);
            break;
        default:
            cout << "Could not handle packet type" << endl;
            status = false;
            break;
    }
    return status;
}

// pop off packets from the queue and process them
// they will be added back if not successful
void processPacketQueue(int currSwitchNumber, int port1Switch, int port2Switch)
{
    int count = packetQueue.size();
    if (count == 0)
    {
        cout << "Query queue is empty." << endl;
    }
    for (int i = 0; i < count; ++i)
    {
        packet inPacket = packetQueue.front();
        packetQueue.pop();
        processPacket(inPacket, currSwitchNumber, port1Switch, port2Switch, inPacket.msg.qrMessage.sendingSwitchNumber);    
    }
    cout << endl << "Finished processing queue. Number of packets still in queue: " << packetQueue.size() << endl;
}

// Checks the name of a switch and returns the switch number if valid
bool checkSwitchName(string name, int &switchNumber) 
{
    bool status = true;
    int testNumber;
    if (name.length() == 3 && name.find("sw") >= 0) 
    {
        testNumber = (int)name[2] - 48;
        if (testNumber > 0 && testNumber < 8)
        {
            switchNumber = testNumber;
        }
        else
        {
            cout << "Invalid switch number provided: " << testNumber << endl;
            return false;
        }
    }
    else if (name.compare("null") == 0)
    {
        switchNumber = -1;
    }   
    else 
    {
        cout << "Invalid switch name: " << name << endl;
        return false;
    }
    return status;   
}

void controllerMainLoop(int numSwitches) 
{   
    // initialize controller global variables
    isSwitch = false;
    acknowledged = true;
    connectionInfo.clear();
    initializeControllerPacketStats();
   
    // set up signal handler for USER1
    if (signal(SIGUSR1, user1SignalHandler) == SIG_ERR)
    {
        cout << "Problem setting signal handler for USER1" << endl;
        return;
    } 

    // open each fifo for reading
    struct pollfd contFIFOS[MAX_NSW];
    for (int i = 0; i < MAX_NSW; ++i)
    {   
        int numRetries = 1;
        string fifo = determineFIFOName(i+1, 0);
        int fd;
        if ((fd = open(fifo.c_str(), O_RDONLY | O_NONBLOCK)) < 0)
        {
            cout << "Unable to open fifo " << fifo << " for read." << endl;
        }
        contFIFOS[i].fd = fd;
        contFIFOS[i].events = POLLIN | POLLPRI;
    }

    ssize_t numberBytes = -1;
    packet inPacket;

    while (true)
    {
        // poll the user input
        pollUserInput(contFIFOS, MAX_NSW);       

        //poll all the FIFOs for packets
        if (poll(contFIFOS, MAX_NSW, 0) > 0)
        {
            // an event occurred in one of the fifos
            for (int i = 0; i < MAX_NSW; ++i)
            {
                if ((contFIFOS[i].revents & (POLLIN | POLLPRI)) > 0)
                {
                    // a packet was received on fifo-i-0
                    numberBytes = read(contFIFOS[i].fd, (char*) &inPacket, sizeof(packet));
                    processPacket(inPacket, 0, -1, -1, i+1, numSwitches);
                }
            }
        }
    } // end while(true)
}

void switchMainLoop(flowTableEntry firstEntry, int switchNumber, string trafficFile, int port1Switch, int port2Switch) 
{   
    // initialize the switch
    isSwitch = true;
    int numInFIFOS = 3;
    bool finished = false;
    vector<int> connectedNumbers;
    connectedNumbers.clear();
    
    if (port1Switch == -1)
    {
        numInFIFOS -= 1;
    }
    if (port2Switch == -1)
    {
        numInFIFOS -= 1;
    }

    initializeSwitchPacketStats();
    connectionInfo.clear();
    message entry;
    entry.aMessage = firstEntry;
    connectionInfo.push_back(entry);

    // set up signal handler for USER1
    if (signal(SIGUSR1, user1SignalHandler) == SIG_ERR)
    {
        cout << "Problem setting signal handler for USER1" << endl;
        return;
    }  

    // open FIFOS for reading
    struct pollfd swFIFOS[numInFIFOS];
    for (int i = 0; i < numInFIFOS; ++i)
    {   
        int numRetries = 1;
        string fifo;
        int fd;

        if (i == 1) 
        {   
            if (port1Switch == -1) 
            {
                fifo = determineFIFOName(port2Switch, switchNumber);
                connectedNumbers.push_back(port2Switch);
            }
            else 
            {
                fifo = determineFIFOName(port1Switch, switchNumber);
                connectedNumbers.push_back(port1Switch);
            }
        }
        else if (i == 2)
        {
            fifo = determineFIFOName(port2Switch, switchNumber);
            connectedNumbers.push_back(port2Switch);
        }
        else 
        {
            fifo = determineFIFOName(i, switchNumber);
            connectedNumbers.push_back(i);
        }
        if ((fd = open(fifo.c_str(), O_RDONLY | O_NONBLOCK)) < 0)
        {
            cout << "Unable to open fifo " << fifo << " for read." << endl;
        }
        swFIFOS[i].fd = fd;
        swFIFOS[i].events = POLLIN | POLLPRI;
    }

    // send OPEN packet to controller
    packet outPacket = createOMessagePacket(OPEN, switchNumber, port1Switch, port2Switch, firstEntry.destIPLo, firstEntry.destIPHi);
    sendPacket(switchNumber, 0, outPacket);
    pktStats.transmitted[OPEN] += 1;

    ssize_t numberBytes = -1;
    packet inPacket;

    FILE *fp;
    // open the traffic file
    if ((fp = fopen(trafficFile.c_str(), "r")) == NULL)
    {
        cout << "Provided traffic file is invalid: " << trafficFile << endl;
        return;
    }

    while (true) 
    {   
        if (acknowledged && !finished)
        {
            // read a valid line from traffic file
            char buf[MAXLINE];
            while (fgets(buf, MAXLINE, fp) != NULL)
            {
                string line(buf);
                istringstream wordstream(line);
                vector<string> words((istream_iterator<string>(wordstream)),
                                                istream_iterator<string>());
                if (words.size() == 0 || words[0][0] == '#')
                {
                    // comment line or a blank line
                    continue;
                }
                else
                {   
                    int trafficSwitchNumber;
                    checkSwitchName(words[0], trafficSwitchNumber);
                    if (trafficSwitchNumber != switchNumber) 
                    {
                        //ignore it and get the next line
                        continue;
                    }
                    else 
                    {
                        // admit the header
                        int srcIP = atoi(words[1].c_str());
                        int destIP = atoi(words[2].c_str());
                        processPacket(createQRMessagePacket(ADMIT, srcIP, destIP), switchNumber, port1Switch, port2Switch);
                        break; 
                    }
                }
            }
            if (feof(fp)) 
            {
                fclose(fp);
                finished = true;
                cout << "Finished processing traffic file." << endl;
            }
        }

        // poll the user
        pollUserInput(swFIFOS, numInFIFOS);

        // poll the open fifos
        if (poll(swFIFOS, numInFIFOS, 0) > 0)
        {   
            // poll only the controller until an ACK packet is received
            if (acknowledged)
            {
                // an event occurred in one of the fifos
                for (int i = 0; i < numInFIFOS; ++i)
                {
                    if ((swFIFOS[i].revents & (POLLIN | POLLPRI)) > 0)
                    {
                        // a packet was received on fifo-i-0
                        numberBytes = read(swFIFOS[i].fd, (char*) &inPacket, sizeof(packet));
                        processPacket(inPacket, switchNumber, port1Switch, port2Switch, connectedNumbers[i]);
                    }
                }
            }
            else 
            {
                if ((swFIFOS[0].revents & (POLLIN | POLLPRI)) > 0)
                {
                    // a packet was received from the controller
                    numberBytes = read(swFIFOS[0].fd, (char*) &inPacket, sizeof(packet));
                    processPacket(inPacket, switchNumber, port1Switch, port2Switch, connectedNumbers[0]);
                }
            }
        }


    } // end while(true)
}

int main(int argc, char *argv[])
{   
    string switchType;
    bool status;
    
    if (argc == 3) 
    {
        // correct number of arguments for controller

        // check for "cont" word
        switchType = argv[1];
        if (switchType.compare("cont") != 0) {
            cout << "Invalid reserved word: " << switchType << endl;
            return 0;
        }

        // verify for valid number of switches
        int numSwitches = atoi(argv[2]);
        if (numSwitches > MAX_NSW || numSwitches <= 0)
        {
            cout << "Invalid number of switches specified." << endl;
            return 0;
        }
        
        // begin controller main loop
        controllerMainLoop(numSwitches);
    }
    else if (argc == 6) 
    {
        // correct number of arguments for switch
        int switchNumber;
        int port1Switch;
        int port2Switch;

        // check for valid switchType and name
        switchType = argv[1];
        status = checkSwitchName(switchType, switchNumber);
        if (!status) 
        {
            return 1;
        }

        string trafficFile = argv[2];

        // check for valid port names
        string swPort1 = argv[3];
        status = checkSwitchName(swPort1, port1Switch);
        if (!status) 
        {
            return 1;
        }
        
        string swPort2 = argv[4];
        status = checkSwitchName(swPort2, port2Switch);
        if (!status) 
        {
            return 1;
        }

        // get the IP range for the switch
        string ipRange = argv[5];
        int hyphenIndex = ipRange.find("-");
        int ipLow = atoi(ipRange.substr(0, hyphenIndex).c_str());
        int ipHigh = atoi(ipRange.substr(hyphenIndex + 1, ipRange.length() - hyphenIndex).c_str());

        flowTableEntry firstEntry = {.srcIPLo = 0, 
                                     .srcIPHi = MAXIP, 
                                     .destIPLo = ipLow, 
                                     .destIPHi = ipHigh, 
                                     .actionType = FORWARD, 
                                     .actionVal = 3, 
                                     .pri = MINPRI, 
                                     .pktCount = 0};

        // begin the switch main loop
        switchMainLoop(firstEntry, switchNumber, trafficFile, port1Switch, port2Switch);
    } 
    else 
    {
        cout << "Incorrect number of arguments specified" << endl;
    }
    
    return 0;
}

//fileno(fp)
//fdopen(fd)