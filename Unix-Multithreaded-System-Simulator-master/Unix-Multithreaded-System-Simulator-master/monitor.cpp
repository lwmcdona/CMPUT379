// the monitor.cpp file, this file handles the monitor class
// and all methods relating to the monitor

#include "includes.hpp"
#include "shared.hpp"
#include "monitor.hpp"
#include "task.hpp"

// adds a pointer to a task to the monitor
void Monitor::add_task(class Task* task){
    this->tasks.push_back(task);
}

// sets the monitor time
void Monitor::set_monitor_time(int monitor_time){
    this->monitor_time = monitor_time;
}

// prints all tasks in a given state at a given time
void Monitor::print_state(enum State state){
    printf("monitor: [%s] ", state_to_string(state).c_str());
    for (int i = 0; i < this->tasks.size(); i++){
        class Task task = *this->tasks[i];

        if (task.state == state){
            printf("%s ", task.name.c_str());
        }
    }
    printf("\n");
}

// prints all tasks in all states then sleeps for the monitor tim
void *Monitor::print(void *arg){
    // cast the monitor pointer to the monitor class
    class Monitor *monitor = (Monitor *) arg;

    // safely prints all states through the use of
    // the printing mutex
    while (true){
        mutexes.lock_mutex(&mutexes.printing_mutex);
        monitor->print_state(WAIT);
        monitor->print_state(RUN);
        monitor->print_state(IDLE);
        mutexes.unlock_mutex(&mutexes.printing_mutex);
        usleep(monitor->monitor_time * 1000);
    }
}

// Create the thread which will run for the monitor, pass the thread
// the print method, as this method will manage the thread once
// its created, also pass that method the pointer to this class.
// Also performs error checking
void Monitor::execute(){
    int rval = pthread_create(&this->tid, NULL, this->print, this);
    if (rval) {
          perror("pthread_create failed");
          exit(1);
    }
}