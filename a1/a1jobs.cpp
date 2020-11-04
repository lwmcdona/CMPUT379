#include <iostream>
#include <sys/resource.h> // for getrlimit and setrlimit
#include <sys/times.h>    // for times
#include <unistd.h>       // for fork, vfork, sleep, getpid
#include <string>
#include <signal.h>
#include <stdlib.h> // atoi()
#include <iomanip>
#include <sys/wait.h> //waitpid()

#define MAXJOBS 32
#define CPU_LIMIT_IN_SECS 600

using namespace std;

struct admittedJob
{
    int index;
    pid_t headPid;
    std::string command;
    bool terminated;
};

// parses the string input for the run command
int parseInput(string runCommands[], string input)
{
    int numArgs = 0;
    int prevSpace = -1;
    int nextSpace = input.find(" ");
    while (nextSpace >= 0 && numArgs <= 4)
    {
        runCommands[numArgs] = input.substr(prevSpace + 1, nextSpace - (prevSpace + 1));
        numArgs += 1;
        prevSpace = nextSpace;
        nextSpace = input.find(" ", prevSpace + 1);
    }
    if (numArgs > 4)
    {
        cout << "Too many arguments specified." << endl;
        return -1;
    }
    runCommands[numArgs] = input.substr(prevSpace + 1, input.length() - (prevSpace + 1));
    numArgs += 1;
    return numArgs;
}

// list all jobs in the job array
void list(admittedJob jobs[], int numJobs)
{
    if (numJobs <= 0)
    {
        cout << "No jobs." << endl;
    }
    for (int i = 0; i < numJobs; ++i)
    {
        cout << jobs[i].index << ": (pid= " << jobs[i].headPid << ", cmd= " << jobs[i].command << ")" << endl;
    }
}

// runs a specified process with up to four specified arguments
void run(string input, admittedJob jobs[], int &numJobs)
{
    // cannot store a job at index 32 or greater so just return
    if (numJobs >= MAXJOBS)
    {
        cout << "Process failed to run. Job cache is full." << endl;
        return;
    }

    // parse the input into separate arguments to be passed to execlp()
    string runCommands[5];
    int numArgs = parseInput(runCommands, input);

    // nothing to run
    if (numArgs == -1)
    {
        return;
    }

    // fork a child to run the specified process and get the parent to store it in
    // the admitted job array
    pid_t pid;
    if ((pid = fork()) < 0)
    {
        cout << "fork error" << endl;
    }
    else if (pid == 0) // child execution
    {
        int error;
        switch (numArgs)
        {
        case 1:
            error = execlp(runCommands[0].c_str(), runCommands[0].c_str(), NULL);
            break;
        case 2:
            error = execlp(runCommands[0].c_str(), runCommands[0].c_str(), runCommands[1].c_str(), NULL);
            break;
        case 3:
            error = execlp(runCommands[0].c_str(), runCommands[0].c_str(), runCommands[1].c_str(), runCommands[2].c_str(), (char *)0);
            break;
        case 4:
            error = execlp(runCommands[0].c_str(), runCommands[0].c_str(), runCommands[1].c_str(), runCommands[2].c_str(), runCommands[3].c_str(), (char *)0);
            break;
        case 5:
            error = execlp(runCommands[0].c_str(), runCommands[0].c_str(), runCommands[1].c_str(), runCommands[2].c_str(), runCommands[3].c_str(), runCommands[4].c_str(), (char *)0);
            break;
        default:
            cout << "Too many arguments to command: " << numArgs << endl;
        }
        // exit the child process on error
        if (error == -1)
        {
            cout << "Process failed to run" << endl;
            exit(0);
        }
    }
    else // parent execution
    {
        // NOTE: assume execution of pgm by forked process is successful
        admittedJob job = {.index = numJobs, .headPid = pid, .command = input, .terminated = false};
        jobs[numJobs] = job;
        numJobs += 1;
    }
}

// suspends a specific job at the index given by input
void suspend(string input, admittedJob jobs[])
{
    int index = atoi(input.c_str());
    kill(jobs[index].headPid, SIGSTOP);
}

// resumes a specific job at the index given by input
void resume(string input, admittedJob jobs[])
{
    int index = atoi(input.c_str());
    kill(jobs[index].headPid, SIGCONT);
}

// terminates a specific job at the index given by input
void terminate(string input, admittedJob jobs[])
{
    int index = atoi(input.c_str());
    if (!jobs[index].terminated)
    {
        kill(jobs[index].headPid, SIGKILL);
        // wait for the job to get its user and system time
        waitpid(jobs[index].headPid, NULL, 0);
        jobs[index].terminated = true;
        cout << "    job " << jobs[index].headPid << " terminated" << endl;
    }
    cout << endl;
}

// terminates all jobs
void exitFunction(admittedJob jobs[], int numJobs)
{
    for (int index = 0; index < numJobs; ++index)
    {
        if (!jobs[index].terminated)
        {
            kill(jobs[index].headPid, SIGKILL);
            // wait for the job to get its user and system time
            waitpid(jobs[index].headPid, NULL, 0);
            jobs[index].terminated = true;
            cout << "    job " << jobs[index].headPid << " terminated" << endl;
        }
    }
    cout << endl;
}

int main()
{
    // get the pid of the current a1jobs process
    const pid_t pid = getpid();

    // create the job structure array
    struct admittedJob jobs[MAXJOBS];

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
    int numJobs = 0;
    bool continueLoop = true;

    // main loop
    // gets an input and parses it for a command
    // if it is a valid command takes the corresponding action
    while (true)
    {
        char str[100];

        cout << "a1jobs[" << pid << "]: ";
        getline(cin, input);

        int endOfFirstWord = input.find(" ");
        string command;
        if (endOfFirstWord != std::string::npos)
        {
            command = input.substr(0, endOfFirstWord);
            input = input.substr(endOfFirstWord + 1, input.length() - endOfFirstWord + 1);
        }
        else
        {
            command = input;
        }

        if (command.compare("list") == 0)
        {
            list(jobs, numJobs);
        }
        else if (command.compare("run") == 0)
        {
            run(input, jobs, numJobs);
        }
        else if (command.compare("suspend") == 0)
        {
            suspend(input, jobs);
        }
        else if (command.compare("resume") == 0)
        {
            resume(input, jobs);
        }
        else if (command.compare("terminate") == 0)
        {
            terminate(input, jobs);
        }
        else if (command.compare("exit") == 0)
        {
            exitFunction(jobs, numJobs);
            break;
        }
        else if (command.compare("quit") == 0)
        {
            break;
        }
        else
        {
            cout << "Invalid command." << endl;
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
