#ifndef RESOURCE_HEADER
#define RESOURCE_HEADER

#include "includes.hpp"

struct Resource{
    string name;
    int max_available;
    int currently_available;

    Resource(string name, int max_available);
    Resource();
    void print();
};

#endif