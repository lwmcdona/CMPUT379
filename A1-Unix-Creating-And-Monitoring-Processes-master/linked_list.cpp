#include "linked_list.h"

// Initialize node element
node::node(pid_t pid, string cmd, bool terminated, node *next){
    this->pid = pid;
    this->cmd = cmd;
    this->next = next;
    this->terminated = terminated;
}

// Initialize linked list
linked_list::linked_list(){
    this->head = new node(-1, "", 1, NULL);
}

// Add an element to the list
void linked_list::add(pid_t pid, string cmd){
    node *temp = new node(-1, "", 0, NULL);
    temp = head;
    while(temp->next != NULL){
        temp = temp->next;
    }

    temp->next = new node(pid, cmd, 0, NULL);
}

// Remove an element from the list
bool linked_list::remove(pid_t pid){
    node *temp = new node(-1, "", 1, NULL);
    temp = head;
    while(temp->next != NULL){
        if(temp->next->pid == pid){
            temp->next->terminated=1;
            return 1; // remove successful
        }
        temp = temp->next;
    }
    return 0; // remove unsuccsessful
}

// Get the pid of a node given it's index
pid_t linked_list::getpid(int idx){
    node *temp = new node(-1, "", 1, NULL);
    temp = head;
    for(int i=0; i<=idx; i++){
        temp = temp->next;
        if(temp == NULL){
            return -1;
        }
    }
    if (temp->terminated){
        return -1;
    }
    return temp->pid;
}

// Print the linked list
void linked_list::print(){
    node *temp = new node(-1, "", 1, NULL);
    temp = head;

    int n = 0;
    int i = 0;
    while(temp->next != NULL){
        temp = temp->next;
        if (!temp->terminated){
            cout << i << ": (pid = " << temp->pid << ", cmd = " << temp->cmd << ")" << endl;
            n++;
        }
        i++;
    }

    if (n == 0){
        cout << "Empty List" << endl;
    }
}

// Get the amount of non-terminated processes
int linked_list::getsize(){
    int size = 0;
    node *temp = new node(-1, "", 0, NULL);
    temp = head;
    while(temp->next != NULL){
        if(!temp->next->terminated){
            size++;
        }
        temp = temp->next;
    }
    return size;
}

// Test the linked list and node classes
void test_linked_list(){
    linked_list *l = new linked_list();

    l->add(7,"test1");
    l->add(9,"test2");
    l->add(91,"test3");
    l->print();
    cout << endl;
    l->remove(7);
    l->remove(91);
    l->remove(9);
    l->remove(76);
    l->add(13, "test4");
    l->print();
}