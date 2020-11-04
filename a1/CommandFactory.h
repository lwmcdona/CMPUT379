#include "Command.h"
#include "JobSpawner.h"

class CommandFactory
{
public:
    CommandFactory();
    bool createCommand(Command *&c, string command, JobSpawner &js, string args);
};