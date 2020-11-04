// the shared.cpp file, this file handles contains shared methods
// used throughout the program, also defines the mutex struct
// which holds all mutexes
#include "includes.hpp"
#include "shared.hpp"

// get the current time in milliseconds
long int get_current_time(){
    struct timeval t;
    gettimeofday(&t, NULL);
    long int milli = t.tv_sec * 1000 + t.tv_usec / 1000;
    return milli;
}

// converts the state of a task to a string
string state_to_string(enum State s){
    if (s == WAIT){
        return "WAIT";
    }
    if (s == RUN){
        return "RUN";
    }
    if (s == IDLE){
        return "IDLE";
    }
    return "";
}

// Initializes all mutexes
void Mutexes::initialize_mutexes(){
    this->initalize_mutex(&this->printing_mutex);
    this->initalize_mutex(&this->resources_mutex);    
}

// ***the next 3 methods were influenced by resources on eclass***

// Initialize a mutex and perform error checking
void Mutexes::initalize_mutex(pthread_mutex_t* mutex){
    int rval = pthread_mutex_init(mutex, NULL);
    if (rval){
        perror("mutex initiliazation failed");
        exit(1); 
    }
}

// lock a mutex and perform error checking
void Mutexes::lock_mutex(pthread_mutex_t* mutex){
    int rval = pthread_mutex_lock(mutex);
    if (rval){
        perror("mutex locking failed");
        exit(1); 
    }
}    

// unlock a mutex and perform error checking
void Mutexes::unlock_mutex(pthread_mutex_t* mutex){
    int rval = pthread_mutex_unlock(mutex);
    if (rval){
        perror("mutex unlocking failed");
        exit(1); 
    }
}