#ifndef SHARED_HEADER
#define SHARED_HEADER

#include "includes.hpp"

enum State {WAIT, RUN, IDLE};

extern long int start_time;
extern struct Mutexes mutexes;

struct Mutexes{
    pthread_mutex_t  printing_mutex;
    pthread_mutex_t  resources_mutex;

    void initialize_mutexes();
    void initalize_mutex(pthread_mutex_t* mutex);
    void lock_mutex(pthread_mutex_t* mutex);
    void unlock_mutex(pthread_mutex_t* mutex);
};

long int get_current_time();

string state_to_string(enum State s);

#endif