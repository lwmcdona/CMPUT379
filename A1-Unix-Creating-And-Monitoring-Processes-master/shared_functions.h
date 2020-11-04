#ifndef CPU_LIMIT_HEADER
#define CPU_LIMIT_HEADER

#include "includes.h"

using namespace std;

// Set the CPU limit in seconds.
void setCPU_limit(int seconds);

// Convert the string input, into a vector of strings, split on spaces
void get_vector_input(vector<string> *split_input, string input);

#endif