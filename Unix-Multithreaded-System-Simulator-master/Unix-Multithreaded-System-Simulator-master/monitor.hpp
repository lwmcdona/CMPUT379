#ifndef MONITOR_HEADER
#define MONITOR_HEADER

#include "includes.hpp"
#include "shared.hpp"

class Monitor{
    vector<class Task*> tasks;
    pthread_t tid;
    int monitor_time;

    public:
    void set_monitor_time(int monitor_time);
    void add_task(class Task* task);
    static void *print(void *arg);
    void print_state(enum State state);
    void execute();
};

#endif