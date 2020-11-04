#ifndef COMMAND_H
#define COMMAND_H

#include "JobSpawner.h"

class Command
{
protected:
    JobSpawner &jobSpawner_;
    string args_;

public:
    Command(JobSpawner &jobSpawner, string args);
    virtual bool execute() = 0;
};

#endif

#ifndef LIST_COMMAND_H
#define LIST_COMMAND_H

class ListCommand : public Command
{
public:
    ListCommand(JobSpawner &jobSpawner, string args);
    bool execute();
};

#endif

#ifndef RUN_COMMAND_H
#define RUN_COMMAND_H

class RunCommand : public Command
{
public:
    RunCommand(JobSpawner &jobSpawner, string args);
    bool execute();
};

#endif

#ifndef SUSPEND_COMMAND_H
#define SUSPEND_COMMAND_H

class SuspendCommand : public Command
{
public:
    SuspendCommand(JobSpawner &jobSpawner, string args);
    bool execute();
};

#endif

#ifndef RESUME_COMMAND_H
#define RESUME_COMMAND_H

class ResumeCommand : public Command
{
public:
    ResumeCommand(JobSpawner &jobSpawner, string args);
    bool execute();
};

#endif

#ifndef TERMINATE_COMMAND_H
#define TERMINATE_COMMAND_H

class TerminateCommand : public Command
{
public:
    TerminateCommand(JobSpawner &jobSpawner, string args);
    bool execute();
};

#endif

#ifndef EXIT_COMMAND_H
#define EXIT_COMMAND_H

class ExitCommand : public Command
{
public:
    ExitCommand(JobSpawner &jobSpawner, string args);
    bool execute();
};

#endif

#ifndef QUIT_COMMAND_H
#define QUIT_COMMAND_H

class QuitCommand : public Command
{
public:
    QuitCommand(JobSpawner &jobSpawner, string args);
    bool execute();
};

#endif