/* 
Code written by Logan McDonald

Acknowledgements: 
Experiments with sending and receiving formatted messages on CMPUT 379 website
for packet code development
*/

#include "libraries.h"
#include "constants.h"
#include "packets.h"

// global variables
queue<packet> packetQueue;              // queue comprised of queryRelayMessage type packets
vector<message> connectionInfo;         // controller: switch table comprised of openMessage types
                                        // switch: flow table comprised of flowTableEntry types
                                         
packetStats pktStats;                   // tracks the number of packets sent and received
set< pair<bool, int> > pendingQuerySet; // used in switches to avoid sending duplicate queries
int socketFileDescriptors[MAX_NSW + 1]; // the socket file descriptors, index 0 is not used in the controller
                                        // controller: index 0 is not used, every other index is set using switch number as index
                                        // switch: only index 0 is used for the controller socket descriptor

bool isSwitch;                          // used in printing and signal handling
bool acknowledged = false;              // if a switch has been acknowledged by the controller
// end global variables

// function headers
void processPacketQueue(int currSwitchNumber, int port1Switch = -2, int port2Switch = -2);
bool sendPacket(int sender, int receiver, packet outPacket);
// end function headers

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

// list received and transmitted packet information contained in pktStats 
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

// list the information and the pktStats for either the switch or the controller
// invoked with the EXIT packet, the list or the exit command
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
    if (isSwitch)
    {
        for (int i = 0; i < numFIFOS; ++i)
        {   
            if (i == 0) 
            {
                shutdown(FIFOS[i].fd, SHUT_RDWR);
            }
            else
            {
                close(FIFOS[i].fd);
            }
        }
    }
    else
    {
        for (int i = 0; i < numFIFOS; ++i)
        {
            shutdown(FIFOS[i].fd, SHUT_RDWR);
        }
    }
    
    exit(0);
}

// polls the user input for either the list or the exit command and takes the appropriate action
void pollUserInput(struct pollfd FIFOS[], int numFIFOS)
{
    string userInput;

    // set up the keyboard for polling
    struct pollfd keyboard[1];
    keyboard[0].fd = STDIN_FILENO;
    keyboard[0].events = POLLIN | POLLPRI;

    // poll the keyboard for user input
    if (poll(keyboard, 1, 0) > 0) 
    {
        // received some form of input
        getline(cin, userInput);

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
            // that switch number is already in use, force the new switch to exit
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
    // the switch number is valid
    return validNumber;
}

// controller adds a switch to the network in its correct place to simplify forwarding later
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
            // fatal error: both ports on new switch are null but we have more than one swithc
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
// steps through the network and verifies the order of switches and ports for correctness
void verifyNetwork()
{   
    bool valid = true;
    for (int i = 0; i < connectionInfo.size(); ++i)
    {
        if (i == connectionInfo.size()-1)
        {
            if (connectionInfo[i].oMessage.port2Switch != -1) 
            {
                // the last switch in the network has a port 2 other than null
                valid = false;
            }
        }
        else
        {
            if (i == 0) 
            {
                if (connectionInfo[i].oMessage.port1Switch != -1) 
                {
                    // the first switch in the network has a port 1 other than null
                    valid = false;
                }
            }
            // compare the two neighbouring switches for a match
            // the network is not valid if they all do not match
            if (connectionInfo[i].oMessage.switchNumber != connectionInfo[i+1].oMessage.port1Switch || connectionInfo[i].oMessage.port2Switch != connectionInfo[i+1].oMessage.switchNumber)
            {
                valid = false;
            }
        }
    }
    if (!valid)
    {
        // there is a problem in the network order but try to make it work
        cout << "WARNING: Switch numbering scheme is skewed. Network may be incorrect." << endl;
    }
}

// creates a fifo name for a sender to send to a receiver
string determineFIFOName(int sender, int receiver)
{
    stringstream ss;
    ss << sender;
    string fifo = "fifo--";

    // insert to make "fifo-s-"
    fifo.insert(5, ss.str());
    
    ss.str("");
    ss << receiver;
    // insert to make "fifo-s-r"
    fifo.insert(7, ss.str());

    return fifo;
}

// sends a packet to a receiver
bool sendPacket(int sender, int receiver, packet outPacket)
{
    int fd;
    ssize_t numBytes;
    if (sender == 0 || receiver == 0)
    {
        // controller sending to switch or switch sending to controller
        // use the socket descripors
        fd = socketFileDescriptors[receiver];
        if ((numBytes = write(fd, (char *) &outPacket, sizeof(outPacket))) < 0)
        {
            cout << "Unable to write packet on socket to " << receiver << endl;
            return false;
        }
    }
    else 
    {
        // switch sending to switch
        string fifo = determineFIFOName(sender, receiver);

        // open the corresponding fifo
        if ((fd = open(fifo.c_str(), O_WRONLY)) < 0)
        {
            cout << "Unable to open fifo " << fifo << " for write." << endl;
            return false;
        }

        // write the packet
        if ((numBytes = write(fd, (char *) &outPacket, sizeof(outPacket))) < 0)
        {
            cout << "Unable to write packet to " << fifo << endl;
            return false;
        }
        
        close(fd);
    }
    // print the transmitted message
    printPacketMessage(sender, receiver, outPacket, true);
    return true;
}

// controller processes a query packet in the network and returns a corresponding ADD packet
// to be sent to a switch as a new rule
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

    // try to find a switch that matches the ip address of the destination contained in qrMessage
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
        // destination switch was found so forward the packet in that direction
        return createAMessagePacket(ADD, 0, MAXIP, ipLow, ipHigh, FORWARD, forwardPort, MINPRI, 0);
    }
    // drop the packet as its destination was not found
    return createAMessagePacket(ADD, 0, MAXIP, destIP, destIP, DROP, 0, MINPRI, 0);
}

// switches search their respective flow table for a valid rule and set the outPort value
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
        // we found a rule in the flow table
        connectionInfo[foundIndex].aMessage.pktCount += 1;
        outPort = connectionInfo[foundIndex].aMessage.actionVal;
        return true;
    }
    outPort = 0;
    return false;
}

// checks to see if a rule received from the controller is already present in the flow table
// returns true if it is and false if it is not
bool ruleExists(message msg) 
{
    for (vector<message>::iterator it = connectionInfo.begin(); it != connectionInfo.end(); ++it)
    {
        if (it->aMessage.srcIPLo == msg.aMessage.srcIPLo &&
            it->aMessage.srcIPHi == msg.aMessage.srcIPHi &&
            it->aMessage.destIPLo == msg.aMessage.destIPLo &&
            it->aMessage.destIPHi == msg.aMessage.destIPHi &&
            it->aMessage.actionType == msg.aMessage.actionType &&
            it->aMessage.actionVal == msg.aMessage.actionVal &&
            it->aMessage.pri == msg.aMessage.pri)
        {   
            return true;
        }
    }
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
        // no need to print queued packets as they have already been printed
        printPacketMessage(sendingSwitchNumber, currSwitchNumber, inPacket);
    }
    switch(inPacket.type) 
    {
        // controller packets
        case OPEN:
            pktStats.received[OPEN] += 1;
            if (switchNumberNotInUse(msg.oMessage.switchNumber))
            {   
                // a valid switch number is attempting to connect
                if (connectionInfo.size() < numSwitches) 
                {
                    // there is still room for the switch, attempt to add it
                    addSwitch(msg);
                }
                else
                {
                    // no more room in the network for the switch, force it to exit
                    cout << "Received an open packet for a new switch when max number of switches are already in use. The new switch was ignored." << endl;
                    outPacket.type = EXIT;
                    status = sendPacket(currSwitchNumber, msg.oMessage.switchNumber, outPacket);
                    pktStats.transmitted[EXIT] += 1;
                    return false;
                }
                // switch was added successfully, send an ACK packet back
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
            else
            {
                return false;
            }
            break;
        case QUERY:
            pktStats.received[QUERY] += 1;
            if (connectionInfo.size() < numSwitches)
            {
                // all switches have not connected yet, add the packet to the queue
                inPacket.type = QUEUEDQUERY;
                // set the sending switch number for later
                inPacket.msg.qrMessage.sendingSwitchNumber = sendingSwitchNumber;
                packetQueue.push(inPacket);
                cout << "Waiting for additional switches to connect. Packet was added to the queue." << endl;
                cout << "Number of packets in queue: " << packetQueue.size() << endl;
                return true;
            }
            // all switches have connected
            // process the query, send back the new rule
            outPacket = processQueryPacket(msg.qrMessage, sendingSwitchNumber);
            status = sendPacket(currSwitchNumber, sendingSwitchNumber, outPacket);
            pktStats.transmitted[ADD] += 1;
            break;  
        case QUEUEDQUERY:
            // all switches have connected so send the new rules from the queue
            cout << "Processing packet: ";
            printMessage(msg.qrMessage);
            cout << "Sending new rule." << endl;
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
            pktStats.received[ADD] += 1;
            if (ruleExists(msg))
            {
                // the rule is already in the flow table
                cout << "Rule already in flow table. No action taken." << endl;
            }
            else
            {
                // add the packet message to connection info as a new rule
                cout << "New rule added to flow table. Processing queue with new rule..." << endl;
                connectionInfo.push_back(msg);

                // process the waiting RELAY and ADMIT packet queue
                processPacketQueue(currSwitchNumber, port1Switch, port2Switch);
            }

            break;
        case RELAY:
            pktStats.received[RELAYIN] += 1;
            // process the packet and set the value of outPort based on the rule
            if (!processRelayPacket(msg.qrMessage, outPort)) 
            {
                // rule was not found
                cout << "No rule found. Adding to queue." << endl;
                // change packet type and add to queue
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
                    // there are not matching pending queries so send one
                    pendingQuerySet.insert(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP));
                    inPacket.type = QUERY;
                    status = sendPacket(currSwitchNumber, 0, inPacket);
                    pktStats.transmitted[QUERY] += 1;
                    return true;
                }
                // a matching query was found so no need to send another
                cout << "Waiting for duplicate query. No packet was sent." << endl;
                return true;
            }
            // rule was found so follow the rule on packet
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
                // transmit the packet to the network
                printPacketMessage(currSwitchNumber, NETPORT, inPacket, true);
            }
            else if (outPort == 0) 
            {
                cout << "Packet dropped." << endl;
            }
            break;
        case ADMIT:
            pktStats.received[ADMIT] += 1;
            // process the packet and set the value of outPort based on the rule
            if (!processRelayPacket(msg.qrMessage, outPort)) 
            {
                // rule was not found
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
                    // there are not matching pending queries so send one
                    pendingQuerySet.insert(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP));
                    inPacket.type = QUERY;
                    status = sendPacket(currSwitchNumber, 0, inPacket);
                    pktStats.transmitted[QUERY] += 1;
                    return true;
                }
                // a matching query was found so no need to send another
                cout << "Waiting for duplicate query. No packet was sent." << endl;
                return true;
            }
            // rule was found so follow the rule on packet
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
                // transmit the packet to the network
                printPacketMessage(currSwitchNumber, NETPORT, inPacket, true);
            }
            else if (outPort == 0) 
            {
                cout << "Packet dropped." << endl;
            }
            break;
        case QUEUEDRELAY:
            cout << "Processing packet: ";
            printMessage(msg.qrMessage);
            if (!processRelayPacket(msg.qrMessage, outPort)) 
            {
                // rule was not found
                cout << "No rule found. Packet returned to queue." << endl;
                // put the packet back on the queue
                packetQueue.push(inPacket);
                return false;
            }
            // rule was found so follow the rule on packet
            cout << "Rule found. Delivering packet." << endl;
            // remove from the pending query set
            bool ltsrcIP;
            if (msg.qrMessage.srcIP > MAXIP) 
                ltsrcIP = false;
            else
                ltsrcIP = true;
            pendingQuerySet.erase(pair<bool, int>(ltsrcIP, msg.qrMessage.destIP));
            // deliver the packet
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
                // transmit the packet to the network
                printPacketMessage(currSwitchNumber, NETPORT, inPacket, true);
            }
            else if (outPort == 0) 
            {
                cout << "Packet dropped." << endl;
            }
            break;
        case EXIT:
            // kill the switch
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

// controller removes a switch from the network in the case where the connection was lost
// this allows the switch to reconnect if it wishes
void removeSwitch(int numConnectedSwitches, struct pollfd contSockets[], int socketSwitchNumbers[], int index)
{
    int switchNumber = socketSwitchNumbers[index];

    // shift the entries in contSockets and socketSwitchNumber back one spot
    for (int i = index; i < numConnectedSwitches-1; ++i)
    {
        contSockets[i] = contSockets[i+1];
        socketSwitchNumbers[i] = socketSwitchNumbers[i+1];
    }

    // erase from connection info (the switch information table)
    for (vector<message>::iterator it = connectionInfo.begin(); it != connectionInfo.end(); ++it)
    {
        if (it->oMessage.switchNumber == switchNumber) 
        {
            connectionInfo.erase(it);
            break;
        }
    }
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

// the main loop for the controller
void controllerMainLoop(int numSwitches, int portNumber) 
{   
    // initialize controller global variables
    isSwitch = false;
    connectionInfo.clear();
    initializeControllerPacketStats();
    int socketSwitchNumbers[numSwitches];
   
    // set up signal handler for USER1
    if (signal(SIGUSR1, user1SignalHandler) == SIG_ERR)
    {
        cout << "Problem setting signal handler for USER1" << endl;
        return;
    } 

    // open manager socket
    struct pollfd contManagerSocket[1];
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "Unable to create a manager socket." << endl;
        return;
    }

    // set socket options to allow reuse of address
    int value = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)))
    {
        cout << "Failed setting socket options." << endl;
        return;
    }

    // bind the manager socket
    struct sockaddr_in sin;
    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET; 
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(portNumber);

    if (bind(fd, (struct sockaddr*) &sin, sizeof(sin)) < 0)
    {
        cout << "Unable to bind manager socket." << endl;
        return;
    }

    // listen for incoming connection requests on the socket
    if (listen(fd, numSwitches) < 0)
    {
        cout << "Error listening on manager socket." << endl;
        return;
    }
    // prepare for polling of the manager socket
    contManagerSocket[0].fd = fd;
    contManagerSocket[0].events = POLLIN | POLLPRI;

    // create a structure for polling from data sockets once they are accepted
    struct pollfd contSockets[MAX_NSW];
    int numConnectedSwitches = 0;

    ssize_t numberBytes = -1;
    packet inPacket;

    while (true)
    {
        // poll for a connect request from a switch
        if (numConnectedSwitches < numSwitches) 
        {
            // controller received a connect request
            if (poll(contManagerSocket, 1, 0) > 0)
            {
                int sockfd;
                if ((sockfd = accept(fd, NULL, NULL)) < 0) 
                {
                    cout << "Error accepting connection request." << endl;
                }
                else
                {
                    // add the file descriptor to the main structure for polling
                    contSockets[numConnectedSwitches].fd = sockfd;
                    contSockets[numConnectedSwitches].events = POLLIN | POLLPRI;
                    numConnectedSwitches += 1;
                }
            }
        }

        // poll the user input
        pollUserInput(contSockets, numConnectedSwitches);       

        //poll all the sockets for packets
        if (poll(contSockets, numConnectedSwitches, 0) > 0)
        {
            // an event occurred in one of the sockets
            for (int i = 0; i < numConnectedSwitches; ++i)
            {
                if ((contSockets[i].revents & (POLLIN | POLLPRI)) > 0)
                {
                    // a packet was received from i
                    if ((numberBytes = read(contSockets[i].fd, (char*) &inPacket, sizeof(packet))) < 0)
                    {
                        cout << "Error occurred during read from switch " << socketSwitchNumbers[i] << endl;
                    }
                    else if (numberBytes == 0)
                    {
                        // the switch disconnected, remove it from the network so it can reconnect
                        cout << "Connection to switch " << socketSwitchNumbers[i] << " was lost." << endl;
                        shutdown(contSockets[i].fd, SHUT_RDWR);
                        contSockets[i].events = 0;
                        cout << "Removing switch " << socketSwitchNumbers[i] << " from the network." << endl;
                        removeSwitch(numConnectedSwitches, contSockets, socketSwitchNumbers, i);
                        numConnectedSwitches -= 1;
                        continue;
                    }

                    int tempDescriptor, tempSwitchNumber;
                    if (inPacket.type == OPEN)
                    {
                        // if the packet is OPEN type, save temporary copies of the 
                        // previous switch in case the controller finds an error
                        tempSwitchNumber = socketSwitchNumbers[i];
                        tempDescriptor = socketFileDescriptors[inPacket.msg.oMessage.switchNumber];

                        socketFileDescriptors[inPacket.msg.oMessage.switchNumber] = contSockets[i].fd;
                        socketSwitchNumbers[i] = inPacket.msg.oMessage.switchNumber;
                    }
                    // process the packet
                    bool success = processPacket(inPacket, 0, -1, -1, socketSwitchNumbers[i], numSwitches);
                    if (inPacket.type == OPEN && !success)
                    {
                        // if there was an error, restore the original switch values
                        socketSwitchNumbers[i] = tempSwitchNumber;
                        socketFileDescriptors[inPacket.msg.oMessage.switchNumber] = tempDescriptor;
                        numConnectedSwitches -= 1;
                    }
                }
            }
        }
    } // end while(true)
}

void switchMainLoop(flowTableEntry firstEntry, int switchNumber, string trafficFile, int port1Switch, int port2Switch, char *serverAddress, int portNumber) 
{   
    // initialize the switch
    isSwitch = true;
    int numInFIFOS = 3;
    bool finished = false;
    bool delayed = false;
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

    // open and connect the TCP socket to the controller
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "Unable to create a manager socket." << endl;
        return;
    }
    
    struct hostent* server;
    struct sockaddr_in sin;

    server = gethostbyname(serverAddress);
    if (server == NULL)
    {
        cout << "Error getting server" << endl;
        return;
    }

    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(portNumber);

    memcpy( (char*) &sin.sin_addr, server->h_addr, server->h_length);

    int count = 0;
    while (connect(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0)
    {
        if (count == 0)
        {
            cout << "Error occurred in connection. Retrying..." << endl;
            count += 1;
        }
        else if (count > 500000)
        {
            
            count = 0;
        }
        else
        {
            count += 1;
        }
    }
    socketFileDescriptors[0] = fd;

    // open the remaining FIFOs for reading and set up all file descriptors for polling
    struct pollfd swFDS[numInFIFOS];
    for (int i = 0; i < numInFIFOS; ++i)
    {   
        string fifo;

        if (i > 0) 
        {
            // open the file descriptors for the FIFOS for the switch
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
            if ((fd = open(fifo.c_str(), O_RDONLY | O_NONBLOCK)) < 0)
            {
                cout << "Unable to open fifo " << fifo << " for read." << endl;
            }
        }
        else 
        {
            // controller file descriptor is already open
            connectedNumbers.push_back(i);
        }
        // prepare for polling
        swFDS[i].fd = fd;
        swFDS[i].events = POLLIN | POLLPRI;
    }

    // send OPEN packet to controller
    packet outPacket = createOMessagePacket(OPEN, switchNumber, port1Switch, port2Switch, firstEntry.destIPLo, firstEntry.destIPHi);
    sendPacket(switchNumber, 0, outPacket);
    pktStats.transmitted[OPEN] += 1;

    ssize_t numberBytes = -1;
    packet inPacket;
    time_t startTime;
    time_t delay;

    FILE *fp;
    // open the traffic file
    if ((fp = fopen(trafficFile.c_str(), "r")) == NULL)
    {
        cout << "Provided traffic file is invalid: " << trafficFile << endl;
        return;
    }

    while (true) 
    {   
        // determine when to end a delay period
        if (delayed && difftime(time(NULL), startTime)*1000 > delay) 
        {
            delayed = false;
            cout << endl << "** Delay period has ended." << endl;
        }

        // if the switch has been acknowledged, is not delayed, and has not finished processing the traffic file
        if (acknowledged && !finished && !delayed)
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
                        if (words[1].compare("delay") == 0) 
                        {
                            delay = atoi(words[2].c_str());
                            delayed = true;
                            startTime = time(NULL);
                            cout << endl << "** Entering a delay period for " << delay << " milliseconds" << endl;
                        }
                        else 
                        {
                            int srcIP = atoi(words[1].c_str());
                            int destIP = atoi(words[2].c_str());
                            processPacket(createQRMessagePacket(ADMIT, srcIP, destIP), switchNumber, port1Switch, port2Switch, FILEPORT);
                        }
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
        pollUserInput(swFDS, numInFIFOS);

        // poll the open socket and fifos
        if (poll(swFDS, numInFIFOS, 0) > 0)
        {   
            // poll only the controller until an ACK packet is received
            if (acknowledged)
            {
                // an event occurred in one of the fds
                for (int i = 0; i < numInFIFOS; ++i)
                {
                    if ((swFDS[i].revents & (POLLIN | POLLPRI)) > 0)
                    {
                                            // a packet was received from the controller
                        if ((numberBytes = read(swFDS[i].fd, (char*) &inPacket, sizeof(packet))) < 0)
                        {
                            cout << "Error occurred during read from " << i << endl;
                        }
                        else if (numberBytes == 0)
                        {
                            if (i == 0) 
                            {
                                // lost connection to the controller
                                cout << "Connection to controller was lost. Exiting..." << endl;
                                listInfo();
                                exitFunction(swFDS, numInFIFOS);
                                shutdown(swFDS[i].fd, SHUT_RDWR);
                                swFDS[i].events = 0;
                                exit(0);
                            }
                            else
                            {
                                // this should never happen as they are fifos not sockets
                                cout << "Connection to switch " << i << " was lost." << endl;
                                close(swFDS[i].fd);
                                swFDS[i].events = 0;
                                continue;
                            }
                        }
                        processPacket(inPacket, switchNumber, port1Switch, port2Switch, connectedNumbers[i]);
                    }
                }
            }
            else 
            {
                if ((swFDS[0].revents & (POLLIN | POLLPRI)) > 0)
                {
                    // a packet was received from the controller
                    if ((numberBytes = read(swFDS[0].fd, (char*) &inPacket, sizeof(packet))) < 0)
                    {
                        cout << "Error occurred during read from controller" << endl;
                    }
                    else if (numberBytes == 0)
                    {
                        // lost connection to the controller
                        cout << "Connection to controller was lost. Exiting..." << endl;
                        listInfo();
                        exitFunction(swFDS, numInFIFOS);
                        shutdown(swFDS[0].fd, SHUT_RDWR);
                        swFDS[0].events = 0;
                        exit(0);
                    }
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
    
    if (argc == 4) 
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

        // used port number 9297
        int portNumber = atoi(argv[3]);
        
        // begin controller main loop
        controllerMainLoop(numSwitches, portNumber);
    }
    else if (argc == 8) 
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
        
        char *serverAddress = argv[6];
        int portNumber = atoi(argv[7]);

        // begin the switch main loop
        switchMainLoop(firstEntry, switchNumber, trafficFile, port1Switch, port2Switch, serverAddress, portNumber);
    } 
    else 
    {
        cout << "Incorrect number of arguments specified" << endl;
    }
    
    return 0;
}