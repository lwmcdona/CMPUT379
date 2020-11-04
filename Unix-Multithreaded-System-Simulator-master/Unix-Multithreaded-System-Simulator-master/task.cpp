// the task.cpp file, this file handles the task class
// and all methods relating to tasks

#include "includes.hpp"
#include "task.hpp"
#include "shared.hpp"
#include "resources.hpp"

// Task constructor, assigns the state, name, all times, and number of iterations
// tid is initialized to -1, however, this will change when the thread is created
Task::Task(string name, int busy_time, int idle_time, int total_number_of_iterations){
    this->state = WAIT;
    this->name = name;
    this->busy_time = busy_time;
    this->wait_time = 0;
    this->idle_time = idle_time;
    this->total_number_of_iterations = total_number_of_iterations;
    this->iterations_completed = 0;
    this->tid = -1;
}

// Create the thread which will run for this task, pass the thread
// the task_handler method, as this method will manage the thread once
// its created, also pass that method the pointer to this class so the
// thread knows which task to run as. Also performs error checking
void Task::execute(){
    int rval = pthread_create(&this->tid, NULL, this->task_handler, this);
    if (rval) {
          perror("pthread_create failed");
          exit(1);
    }
}

// Determines if the task can aquire all of the resources that it needs,
// this method will be called inside of a critical section protected by
// a mutex lock
bool Task::can_aquire_resources_needed(){
    for (int i = 0; i < needed_resources.size(); i++){
        struct Needed_Resource *needed_resource = &needed_resources[i];
        struct Resource *resource = needed_resource->resource;
        if (resource->currently_available < needed_resource->amount_needed){
            return false;
        }
    }
    return true;
}

// Aquires all of the resources that the task needs, this method will be 
// called inside of a critical section protected by a mutex lock
void Task::aquire_resources_needed(){
    for (int i = 0; i < needed_resources.size(); i++){
        struct Needed_Resource *needed_resource = &needed_resources[i];
        struct Resource *resource = needed_resource->resource;
        if (resource->currently_available < needed_resource->amount_needed){
            cout << "Error: Resource vanished unexpectidley" << endl;
            exit(0);
        }
        needed_resource->amount_held += needed_resource->amount_needed;
        resource->currently_available -= needed_resource->amount_needed;
    }
}

// Releases all of the resources that the task has, this method will be 
// called inside of a critical section protected by a mutex lock
void Task::release_resources(){
    for (int i = 0; i < needed_resources.size(); i++){
        struct Needed_Resource *needed_resource = &needed_resources[i];
        struct Resource *resource = needed_resource->resource;
        resource->currently_available += needed_resource->amount_held;
        needed_resource->amount_held = 0;
    }
}

// Performs the RUN action. The task does this by sleeping
// for the busy_time and then releasing the resources that the
// task has and then changing the state to idle
void Task::run(){
    // sleep for the busy time
    usleep(this->busy_time * 1000);

    // Releases all resources the task has
    // this is done inside of a mutex lock
    // to protect the resource from being written
    // to and read from at the same time
    mutexes.lock_mutex(&mutexes.resources_mutex);
    this->release_resources();
    mutexes.unlock_mutex(&mutexes.resources_mutex);

    // Changes the state to IDLE
    // this is done inside of a mutex lock so the
    // monitor thread doesn't print the state of
    // this task twice
    mutexes.lock_mutex(&mutexes.printing_mutex);
    this->state = IDLE;
    mutexes.unlock_mutex(&mutexes.printing_mutex);
}

// the task thread main controller
// ensures that the thread changes states,
// and handles resources and printing
// properly and safely
void *Task::task_handler(void *arg){
    // Get pointer to task that this thread
    // will run
    class Task *task = (Task *) arg;
    while (true){
        // safely change state to WAIT
        mutexes.lock_mutex(&mutexes.printing_mutex);
        task->state = WAIT;
        mutexes.unlock_mutex(&mutexes.printing_mutex);

        // safely determine if we can aquire the needed
        // resources, if we can't, release the lock and
        // wait for 10 ms
        mutexes.lock_mutex(&mutexes.resources_mutex);
        if (!task->can_aquire_resources_needed()){
            mutexes.unlock_mutex(&mutexes.resources_mutex);

            task->wait_time += 10;
            usleep(10 * 1000); // 10ms
            continue;
        }
        
        // if we can aquire the needed resources, aquire
        // them and then unlock the mutex
        task->aquire_resources_needed();
        mutexes.unlock_mutex(&mutexes.resources_mutex);

        // then we change the state to run and run the
        // task
        mutexes.lock_mutex(&mutexes.printing_mutex);
        task->state = RUN;
        mutexes.unlock_mutex(&mutexes.printing_mutex);

        task->run();

        // after running we sleep for the idle time
        usleep(task->idle_time * 1000);

        // then we print all information and
        // incriment iterations_completed
        mutexes.lock_mutex(&mutexes.printing_mutex);
        task->print_after_iteration();
        mutexes.unlock_mutex(&mutexes.printing_mutex);

        task->iterations_completed++;

        // finally if the task has completed all needed iterations we exit
        // the thread
        if (task->iterations_completed >= task->total_number_of_iterations){
            void * value_pointer;
            pthread_exit(value_pointer);
        }
    }
}

// used at the end of the program
// prints information about the task
void Task::print_final(){
    printf("%s (%s, runTime= %d msec, idleTime= %d msec):\n",
    this->name.c_str(), state_to_string(this->state).c_str(), this->busy_time, this->idle_time);
    cout << "\t(tid= " << this->tid << ")" << endl;

    for (int i = 0; i < needed_resources.size(); i++){
        cout << "\t";
        needed_resources[i].print();
    }

    printf("\t(RUN: %d times, WAIT: %d msec)\n", this->iterations_completed, this->wait_time);
}

// used after each iteration of the task
// prints information about the task
void Task::print_after_iteration(){
    printf("task: %s (tid= ",this->name.c_str());
    cout << this->tid;
    printf(", iter= %d, time= %d msec)\n", this->iterations_completed, int(get_current_time() - start_time));

}

// add a needed resource to a task
void Task::add_needed_resource(struct Needed_Resource needed_resource){
    this->needed_resources.push_back(needed_resource);
}

// constructor for the needed resource struct, initalizes by 
// setting the resource pointer, amount needed, and amount held
Needed_Resource::Needed_Resource(struct Resource *resource, int amount_needed){
    this->resource = resource;
    this->amount_needed = amount_needed;
    this->amount_held = 0;
}

// another constructor for the needed resource struct, initalizes by 
// setting the resource pointer, amount needed, and amount held
Needed_Resource::Needed_Resource(){
    this->resource = new Resource();
    this->amount_needed = -1;
    this->amount_held = 0;
}

// prints all of the information for a needed resource
void Needed_Resource::print(){
    printf("%s: (needed= %d, held= %d)\n",
    this->resource->name.c_str(), this->amount_needed, this->amount_held);
}