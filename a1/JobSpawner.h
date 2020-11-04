#ifndef JOBSPAWNER_H
#define JOBSPAWNER_H

#include <iostream>
// #include <sys/resource.h> // for getrlimit and setrlimit
// #include <sys/times.h>    // for times
#include <unistd.h> // for fork, vfork, sleep, getpid
#include <string>
// #include <signal.h>
// #include <stdlib.h> // atoi()
// #include <iomanip>
#include <sys/wait.h> //waitpid()

// #include "Command.h"

#define MAXJOBS 32
#define CPU_LIMIT_IN_SECS 600

using namespace std;

class JobSpawner
{
private:
    struct admittedJob
    {
        int index;
        pid_t headPid;
        std::string command;
        bool terminated;
    };

    admittedJob jobs_[MAXJOBS];
    int numJobs_;

    int parseInput(string runCommands[], string input);

public:
    JobSpawner();
    void listJobs();
    void runJob(string args);
    void suspendJob(string args);
    void resumeJob(string args);
    void terminateJob(string args);
    void terminateAllJobs();
    void addJob();

    // Command createCommand(string command);
    // void runCommand(string command, string args);
};

#endif