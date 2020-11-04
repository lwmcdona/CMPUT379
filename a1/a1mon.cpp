#include <iostream>
#include <sstream>
#include <iterator>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unistd.h> // sleep, getpid
#include <sys/resource.h>
#include <signal.h> //kill

using namespace std;

// displays a list of all running processes
void displayMonitoredProcessList(map<pid_t, string> processCommand)
{
    int processCount = 0;
    cout << "-----------------------------------" << endl
         << "List of monitored processes:" << endl;
    for (map<pid_t, string>::iterator it = processCommand.begin(); it != processCommand.end(); ++it)
    {
        if (processCount > 0)
        {
            cout << ", " << endl;
        }
        else
        {
            cout << "[" << endl;
        }
        cout << "   " << processCount << ":[" << it->first << "," << it->second << "]";
        processCount++;
    }
    cout << endl
         << "]" << endl
         << endl
         << endl
         << endl;
}

// recursively kill child processes of the target process that are stored in childProcesses
// update childProcesses by referring to parentChild, a map from a parent process to a set of its children
void killAllChildProcesses(pid_t targetPid, map<pid_t, set<pid_t>> parentChild, map<pid_t, string> processCommand, set<pid_t> childProcesses)
{
    //find target pid in the map and kill all the children
    for (set<pid_t>::iterator it = childProcesses.begin(); it != childProcesses.end(); ++it)
    {
        cout << "Terminating process: [" << *it << ", " << processCommand[*it] << "]" << endl;
        kill(*it, SIGKILL);
        // kill all the children of each child in turn
        killAllChildProcesses(*it, parentChild, processCommand, parentChild[*it]);
    }
}

// main monitoring function
// prints out the status of all user processes
// stores the information
// determines if the target process is still running
bool checkTargetProcess(pid_t targetPid, pid_t pid, int interval)
{
    FILE *fp;
    map<pid_t, set<pid_t>> parentChild;
    map<pid_t, string> processCommand;
    set<pid_t> targetProcessChildren;
    int counter = 0;
    while (true)
    {
        // keep track of the children of the target process so that
        // we know which processes to terminate once the target is terminated
        // if the target is terminated on the first iteration, we will have no information on its children
        // since the parent of all its child processes will be set to 1 on the initial call to ps
        if (counter > 0)
        {
            targetProcessChildren = parentChild[targetPid];
        }

        // print out the header
        counter += 1;
        cout << "===================================" << endl
             << "a1mon ["
             << "counter= " << counter << ", "
             << "pid= " << pid << ", "
             << "target_pid= " << targetPid << ", "
             << "interval= " << interval << " sec]: " << endl
             << endl;

        // get the status of the user processes
        if ((fp = popen("ps -u $USER -o user,pid,ppid,state,start,cmd --sort start", "r")) == NULL)
        {
            cout << "No target process ID found." << endl;
            return 1;
        }

        // empty the current maps
        parentChild = map<pid_t, set<pid_t>>();
        processCommand = map<pid_t, string>();

        // assume the process is terminated until locate it in the output from ps
        bool processTerminated = true;

        // parse the statuses into the maps
        int MAXLINE = 1000;
        char buf[MAXLINE];
        int loopCounter = 0;

        while (fgets(buf, MAXLINE, fp) != NULL)
        {
            cout << buf;
            // ignore the first iteration as this is only headers
            if (loopCounter < 1)
            {
                loopCounter++;
                continue;
            }

            // split the line from ps on spaces and place each word into a vector
            string line(buf);
            istringstream wordstream(line);
            vector<string> words((istream_iterator<string>(wordstream)),
                                 istream_iterator<string>());

            pid_t childPid = atoi(words[1].c_str());
            pid_t parentPid = atoi(words[2].c_str());

            // if the child process id matches the target process id it is still running
            if (childPid == targetPid)
            {
                processTerminated = false;
            }

            // map the parent to its child
            parentChild[parentPid].insert(childPid);

            // map the child to its command for display purposes
            for (int i = 5; i < words.size(); ++i)
            {
                if (i == words.size() - 1)
                {
                    processCommand[childPid].append(words[i]);
                }
                else
                {
                    processCommand[childPid].append(words[i]);
                    processCommand[childPid].append(" ");
                }
            }
        } // end while

        pclose(fp);

        // display the list of processes we stored
        displayMonitoredProcessList(processCommand);

        // kill all the child processes of the target process if it has been terminated
        if (processTerminated)
        {
            cout << "a1mon: target appears to have terminated; cleaning up" << endl;
            killAllChildProcesses(targetPid, parentChild, processCommand, targetProcessChildren);

            cout << "exiting a1mon" << endl;
            return processTerminated;
        }
        sleep(interval);
    } // end while

    // should never get here
    return -1;
} // end getTargetProcess()

int main(int argc, char *argv[])
{
    // get the pid of the current process
    pid_t pid = getpid();

    // set a limit on the CPU time
    rlim_t timeLimit = 600;
    const struct rlimit CPULimit = {timeLimit, RLIM_INFINITY};

    setrlimit(RLIMIT_CPU, &CPULimit);

    // get the user input arguments
    int interval;
    pid_t targetPid;
    if (argc <= 1)
    {
        cout << "Missing argument: Target process ID" << endl;
        return 1;
    }
    else if (argc >= 2 || argc <= 3)
    {
        targetPid = atoi(argv[1]);
        if (argc == 3)
        {
            interval = atoi(argv[2]);
        }
        else
        {
            interval = 3;
        }
    }
    else
    {
        cout << "Too many arguments specified." << endl;
        return 1;
    }

    // monitor the target's status
    if (checkTargetProcess(targetPid, pid, interval) == -1)
    {
        cout << "An error occurred in monitoring target process" << endl;
    }
    return 0;
}