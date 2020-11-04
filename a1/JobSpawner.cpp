#include "JobSpawner.h"

JobSpawner::JobSpawner()
{
    numJobs_ = 0;
}

void JobSpawner::addJob()
{
    numJobs_ += 1;
}

void JobSpawner::listJobs()
{
    if (numJobs_ <= 0)
    {
        cout << "No jobs." << endl;
    }
    for (int i = 0; i < numJobs_; ++i)
    {
        cout << jobs_[i].index << ": (pid= " << jobs_[i].headPid << ", cmd= " << jobs_[i].command << ")" << endl;
    }
}

void JobSpawner::runJob(string args)
{
    // cannot store a job at index 32 or greater so just return
    if (numJobs_ >= MAXJOBS)
    {
        cout << "Process failed to run. Job cache is full." << endl;
        return;
    }

    // parse the input into separate arguments to be passed to execlp()
    string runCommands[5];
    int numArgs = parseInput(runCommands, args);

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
        admittedJob job = {.index = numJobs_, .headPid = pid, .command = args, .terminated = false};
        jobs_[numJobs_] = job;
        numJobs_ += 1;
    }
}

void JobSpawner::suspendJob(string args)
{
    int index = atoi(args.c_str());
    kill(jobs_[index].headPid, SIGSTOP);
}

void JobSpawner::resumeJob(string args)
{
    int index = atoi(args.c_str());
    kill(jobs_[index].headPid, SIGCONT);
}

void JobSpawner::terminateJob(string args)
{
    int index = atoi(args.c_str());
    if (!jobs_[index].terminated)
    {
        kill(jobs_[index].headPid, SIGKILL);
        // wait for the job to get its user and system time
        waitpid(jobs_[index].headPid, NULL, 0);
        jobs_[index].terminated = true;
        cout << "    job " << jobs_[index].headPid << " terminated" << endl;
    }
    cout << endl;
}

void JobSpawner::terminateAllJobs()
{
    for (int index = 0; index < numJobs_; ++index)
    {
        if (!jobs_[index].terminated)
        {
            kill(jobs_[index].headPid, SIGKILL);
            // wait for the job to get its user and system time
            waitpid(jobs_[index].headPid, NULL, 0);
            jobs_[index].terminated = true;
            cout << "    job " << jobs_[index].headPid << " terminated" << endl;
        }
    }
    cout << endl;
}

// parses the string input for the run command
int JobSpawner::parseInput(string runCommands[], string input)
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

// Command JobSpawner::createCommand(string command)
// {
// }

// void JobSpawner::runCommand(string command, string args)
// {
//     Command c = createCommand(command);
//     c.execute(args);
// }