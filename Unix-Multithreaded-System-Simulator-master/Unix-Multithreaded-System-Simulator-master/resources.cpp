// the resources.cpp file, this file handles the resources struct
// and all methods relating to resources

#include "includes.hpp"
#include "resources.hpp"

// Resource constructor, sets name, max available,
// and currently available
Resource::Resource(string name, int max_available){
    this->name = name;
    this->max_available = max_available;
    this->currently_available = max_available;
}

// Another resource constructor, sets name, max available,
// and currently available with empty values
Resource::Resource(){
    this->name = "";
    this->max_available = -1;
    this->currently_available = -1;
}

// prints all of the information for a resource
void Resource::print(){
    printf("%s: (MaxAvail= %d, held= %d)\n",
    this->name.c_str(), this->max_available, this->max_available - this->currently_available);
}