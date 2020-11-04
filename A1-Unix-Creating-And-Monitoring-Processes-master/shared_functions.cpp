#include "includes.h"
#include "shared_functions.h"


// Set the CPU limit in seconds.
void setCPU_limit(int seconds){
    struct rlimit cpu_limiter;     
    cpu_limiter.rlim_cur = seconds;
    setrlimit (RLIMIT_CPU, &cpu_limiter); 
}

// Convert the string input, into a vector of strings, split on spaces
void get_vector_input(vector<string> *split_input, string input){
    istringstream iss(input);
    for(input; iss >> input; ){
        (*split_input).push_back(input);
    }
}