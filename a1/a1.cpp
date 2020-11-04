#include <iostream>
#include <sys/resource.h> // for getrlimit and setrlimit
#include <sys/times.h>    // for times
#include <unistd.h>       // for fork, vfork, sleep, getpid
#include <string>
#include <signal.h>
#include <stdlib.h> // atoi()
#include <iomanip>
#include <sys/wait.h> //waitpid()

#include "JobSpawner.h"
#include "CommandFactory.h"
#include "Command.h"

int main()
{
    // get the pid of the current a1jobs process
    const pid_t pid = getpid();

    // set up timing structs and get the starting time
    clock_t startTime, endTime;
    struct tms startCPUTime, endCPUTime;

    if ((startTime = times(&startCPUTime)) == -1)
    {
        cout << "Error getting start time" << endl;
    }

    // set a time limit on the CPU time for processes to 10 minutes
    rlim_t timeLimit = CPU_LIMIT_IN_SECS;
    const struct rlimit CPULimit = {timeLimit, RLIM_INFINITY};

    setrlimit(RLIMIT_CPU, &CPULimit);

    string input;
    string command;
    string args;

    JobSpawner js;
    CommandFactory cf;
    Command *c;

    // main loop
    // gets an input and parses it for a command
    // if it is a valid command takes the corresponding action
    while (true)
    {
        cout << "a1jobs[" << pid << "]: ";
        getline(cin, input);

        int endOfFirstWord = input.find(" ");
        string command;
        if (endOfFirstWord != std::string::npos)
        {
            command = input.substr(0, endOfFirstWord);
            args = input.substr(endOfFirstWord + 1, input.length() - endOfFirstWord + 1);
        }
        else
        {
            command = input;
        }

        if (cf.createCommand(c, command, js, args))
        {
            if (!c->execute())
            {
                break;
            }
            delete c;
        }

    } // end while

    // gets the end time and prints the results
    if ((endTime = times(&endCPUTime)) == -1)
    {
        cout << "Error getting end time" << endl;
    }

    static long clockTick = 0;
    if ((clockTick = sysconf(_SC_CLK_TCK)) < 0)
    {
        cout << "sysconf error" << endl;
    }

    cout << "  real:         " << ((endTime - startTime) / (double)clockTick) << endl;
    cout << "  user:         " << fixed << setprecision(2) << ((endCPUTime.tms_utime - startCPUTime.tms_utime) / (double)clockTick) << endl;
    cout << "  sys:          " << fixed << setprecision(2) << ((endCPUTime.tms_stime - startCPUTime.tms_stime) / (double)clockTick) << endl;
    cout << "  child user:   " << fixed << setprecision(2) << ((endCPUTime.tms_cutime - startCPUTime.tms_cutime) / (double)clockTick) << endl;
    cout << "  child sys:    " << fixed << setprecision(2) << ((endCPUTime.tms_cstime - startCPUTime.tms_cstime) / (double)clockTick) << endl;

    return 0;
} // end main