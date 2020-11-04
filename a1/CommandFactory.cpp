#include "CommandFactory.h"

CommandFactory::CommandFactory()
{
}

bool CommandFactory::createCommand(Command *&c, string command, JobSpawner &js, string args)
{
    if (command.compare("list") == 0)
    {
        c = new ListCommand(js, args);
    }
    else if (command.compare("run") == 0)
    {
        c = new RunCommand(js, args);
    }
    else if (command.compare("suspend") == 0)
    {
        c = new SuspendCommand(js, args);
    }
    else if (command.compare("resume") == 0)
    {
        c = new ResumeCommand(js, args);
    }
    else if (command.compare("terminate") == 0)
    {
        c = new TerminateCommand(js, args);
    }
    else if (command.compare("exit") == 0)
    {
        c = new ExitCommand(js, args);
    }
    else if (command.compare("quit") == 0)
    {
        c = new QuitCommand(js, args);
    }
    else
    {
        cout << "Invalid command." << endl;
        return false;
    }
    return true;
}