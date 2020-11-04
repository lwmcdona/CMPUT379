#ifndef LINKED_LIST_INCLUDE
#define LINKED_LIST_INCLUDE

#include "includes.h"

// This node class contains the pid and cmd of a proccess, also it contains a pointer to the next element.
class node{
    public:
    pid_t pid;
    string cmd;
    node *next;
    bool terminated;

    node(pid_t pid, string cmd, bool terminated, node *next);
    };

// This linked_list class contains a collection of nodes, with the functionality of adding, removing, and printing nodes.
class linked_list{
    public:
    node *head;
    linked_list();
    void add(pid_t pid, string cmd);
    bool remove(pid_t pid);
    pid_t getpid(int idx);
    void print();
    int getsize();
};

// This function is used to test the linked list and node classes.
void test_linked_list();

#endif
