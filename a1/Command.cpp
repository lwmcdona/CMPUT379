#include "Command.h"
// : jobSpawner_(jobSpawner), args_(args)
Command::Command(JobSpawner &jobSpawner, string args) : jobSpawner_(jobSpawner), args_(args)
{
}

ListCommand::ListCommand(JobSpawner &jobSpawner, string args) : Command(jobSpawner, args)
{
}

bool ListCommand::execute()
{
    jobSpawner_.listJobs();
    return true;
}

RunCommand::RunCommand(JobSpawner &jobSpawner, string args) : Command(jobSpawner, args)
{
}

bool RunCommand::execute()
{
    jobSpawner_.runJob(args_);
    return true;
}

SuspendCommand::SuspendCommand(JobSpawner &jobSpawner, string args) : Command(jobSpawner, args)
{
}

bool SuspendCommand::execute()
{
    jobSpawner_.suspendJob(args_);
    return true;
}

ResumeCommand::ResumeCommand(JobSpawner &jobSpawner, string args) : Command(jobSpawner, args)
{
}

bool ResumeCommand::execute()
{
    jobSpawner_.resumeJob(args_);
    return true;
}

TerminateCommand::TerminateCommand(JobSpawner &jobSpawner, string args) : Command(jobSpawner, args)
{
}

bool TerminateCommand::execute()
{
    jobSpawner_.terminateJob(args_);
    return true;
}

ExitCommand::ExitCommand(JobSpawner &jobSpawner, string args) : Command(jobSpawner, args)
{
}

bool ExitCommand::execute()
{
    jobSpawner_.terminateAllJobs();
    return false;
}

QuitCommand::QuitCommand(JobSpawner &jobSpawner, string args) : Command(jobSpawner, args)
{
}

bool QuitCommand::execute()
{
    return false;
}