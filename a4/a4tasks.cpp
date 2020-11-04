#include "libraries.h"

#define NRES_TYPES 10
#define NTASKS 25
#define MAXLINE 32

using namespace std;

enum state {WAIT, RUN, IDLE};
const string STATENAME[3] = {"WAIT", "RUN", "IDLE"};

struct rsrc 
{
    string name;
    int total;
    int held;
};

struct taskParameters
{
    string name;
    state taskState;
    int busyTime;
    int idleTime;
    pthread_t ntid;
    vector<rsrc> taskResources;
    int numberIterations;
    int waitTime;
};

taskParameters taskList[NTASKS]; 
map<string, int> resources;

int NITER;
int numTasks = 0;
struct timeval startTime;
pthread_mutex_t resourceMutex;
pthread_rwlock_t monitorLock;

// -------------------------------------------
// mutex functions borrowed from "Experiments involving race conditions in multithreaded programs"
// located on the course website
// all mutex-like functions used are similar to these
void mutex_init(pthread_mutex_t* mutex)
{
    int rval= pthread_mutex_init(mutex, NULL);
    if (rval) {perror("Problem initializing mutex"); exit(1); }
}    

void mutex_lock(pthread_mutex_t* mutex)
{
    int rval= pthread_mutex_lock(mutex);
    if (rval) {perror("Lock error for mutex"); exit(1); }
}    

void mutex_unlock(pthread_mutex_t* mutex)
{
    int rval= pthread_mutex_unlock(mutex);
    if (rval) {perror("Unlock error for mutex"); exit(1); }
}
// -------------------------------------------

// -------------------------------------------
// read-write lock functions
void rwlock_init(pthread_rwlock_t* rwlock)
{
    int rval = pthread_rwlock_init(rwlock, NULL);
    if (rval) {perror("Problem initializing rwlock"); exit(1); }
}

void rwlock_rdlock(pthread_rwlock_t* rwlock)
{
    int rval = pthread_rwlock_rdlock(rwlock);
    if (rval) {perror("Read lock error for rwlock"); exit(1); }
}

void rwlock_wrlock(pthread_rwlock_t* rwlock)
{
    int rval = pthread_rwlock_wrlock(rwlock);
    if (rval) {perror("Write lock error for rwlock"); exit(1); }
}

void rwlock_unlock(pthread_rwlock_t* rwlock)
{
    int rval = pthread_rwlock_unlock(rwlock);
    if (rval) {perror("Unlock error for rwlock"); exit(1); }
}
// -------------------------------------------

// enables or disables the cancel response for the monitor
void setCancelState(int state) 
{
    int rval= pthread_setcancelstate(state, NULL);
    if (rval) {perror("Monitor cancel state change error"); exit(1); }
}

// prints the states of all tasks for the monitor thread
// monitor must acquire the monitor lock in write mode before executing
void printTaskStates(map<state, vector<string> > taskListPerState)
{
    cout << endl << "monitor: " << "[WAIT] ";
    for (vector<string>::iterator it = taskListPerState[WAIT].begin(); it != taskListPerState[WAIT].end(); ++it)
    {
        cout << *it << " ";
    }
    cout << endl << "         [RUN]  ";
    for (vector<string>::iterator it = taskListPerState[RUN].begin(); it != taskListPerState[RUN].end(); ++it)
    {
        cout << *it << " ";
    }
    cout << endl << "         [IDLE] ";
    for (vector<string>::iterator it = taskListPerState[IDLE].begin(); it != taskListPerState[IDLE].end(); ++it)
    {
        cout << *it << " ";
    }
    cout << endl << endl;
}

// prints the task status after every iteration for a task thread
// task must acquire the monitor lock in write mode before executing
void printTaskStatus(int threadIndex)
{
    struct timeval sinceStart;
    gettimeofday(&sinceStart, NULL);
    long long waitTime = ((sinceStart.tv_sec * 1000000 + sinceStart.tv_usec) - (startTime.tv_sec * 1000000 + startTime.tv_usec)) / 1000;
 
    cout << "task: "   << taskList[threadIndex].name << 
            "(tid= "   << taskList[threadIndex].ntid <<
            ", iter= " << taskList[threadIndex].numberIterations <<
            ", time= " << waitTime << " msec)" << endl;
}

// prints the system resources at the end of the program
void printSystemResources(map<string, int> maxResources) 
{
    cout << endl << "System Resources: " << endl;
    for (map<string, int>::iterator it = maxResources.begin(); it != maxResources.end(); ++it)
    {
        cout << "       "       << it->first << 
                ": (maxAvail= " << it->second <<
                ", held= "      << it->second - resources[it->first] << ")" << endl;
    }
}

// prints the task results at the end of the program
void printSystemTasks()
{
    int count = 0;
    cout << endl << "System Tasks:" << endl;
    for (int i = 0; i < numTasks; ++i)
    {
        cout << "[" << count << "] " << taskList[i].name << " (" << 
                STATENAME[taskList[i].taskState] << 
                ", runTime= "   << taskList[i].busyTime << " msec" <<
                ", idleTime= "  << taskList[i].idleTime << " msec):" << endl <<
                "       (tid= " << taskList[i].ntid << ")" << endl;

        for (int j = 0; j < taskList[i].taskResources.size(); ++j)
        {
            cout << "        "    << taskList[i].taskResources[j].name << 
                    ": (needed= " << taskList[i].taskResources[j].total <<
                    ", held= "    << taskList[i].taskResources[j].held << ")" << endl;
        }

        cout << "       (RUN: "   << taskList[i].numberIterations << " times" << 
                        ", WAIT: " << taskList[i].waitTime << " msec)" << endl << endl;
        count += 1;
    }
}

// checks the resource pool to determine if there are enough resources to run a task
// task must obtain the resource mutex before calling this function
bool canAcquireResources(int threadIndex)
{
    vector<rsrc> acquiredResources = taskList[threadIndex].taskResources;
    for (int i = 0; i < acquiredResources.size(); ++i)
    {
        string resource = acquiredResources[i].name;
        int needed = acquiredResources[i].total;
        // if there is not enough of a certain resource, return false
        if (resources[resource] < needed)
        {
            return false;
        }
    }
    // there are enough resources to run this task
    return true;
}

// creates a resource
rsrc createResource(string name, int total, int held)
{
    rsrc r = {.name = name, .total = total, .held = held};
    return r;
}

// the thread for the monitor
void *monitorThread(void *arg)
{
    int monitorTime = *((int *) arg);
    map<state, vector<string> > taskListPerState;

    // convert to microseconds
    monitorTime = monitorTime * 1000;
    while (true)
    {
        taskListPerState.clear();
        usleep(monitorTime);

        // prevent the monitor from being cancelled while holding the rwlock
        setCancelState(PTHREAD_CANCEL_DISABLE);
        rwlock_wrlock(&monitorLock);

        // print the list of tasks based upon their state
        for (int i = 0; i < numTasks; ++i)
        {
            taskListPerState[taskList[i].taskState].push_back(taskList[i].name);
        }
        printTaskStates(taskListPerState);

        rwlock_unlock(&monitorLock);
        // re-enable the cancel response
        setCancelState(PTHREAD_CANCEL_ENABLE);
    }

}

// the main task thread 
void *taskThread(void *arg)
{
    int* threadIndexP =  (int *) arg;
    int threadIndex = *threadIndexP;
    delete threadIndexP;

    struct timeval startWaitTime, endWaitTime;
    vector<rsrc> acquiredResources;

    while (taskList[threadIndex].numberIterations < NITER) 
    {
        // attempt to acquire all resources
        rwlock_rdlock(&monitorLock);
        taskList[threadIndex].taskState = WAIT;
        rwlock_unlock(&monitorLock);

        gettimeofday(&startWaitTime, NULL);
        while (true)
        {   
            if (taskList[threadIndex].taskResources.size() == 0)
            {
                // task doesn't need any resources
                break;
            }
            bool success = false;
            mutex_lock(&resourceMutex);

            if (canAcquireResources(threadIndex))
            {
                acquiredResources = taskList[threadIndex].taskResources;
                for (int i = 0; i < acquiredResources.size(); ++i)
                {
                    string resource = acquiredResources[i].name;
                    int needed = acquiredResources[i].total;
                    resources[resource] -= needed;
                    acquiredResources[i].held = needed;
                } 
                // commit the changes to the resources
                taskList[threadIndex].taskResources = acquiredResources;
                success = true;         
            }

            mutex_unlock(&resourceMutex);
            if (success) 
            {
                break;
            }
            usleep(10*1000);
        }
        // resources successfully obtained
        
        // calculate the time spent waiting
        gettimeofday(&endWaitTime, NULL);
        long long waitTime = ((endWaitTime.tv_sec * 1000000 + endWaitTime.tv_usec) - (startWaitTime.tv_sec * 1000000 + startWaitTime.tv_usec)) / 1000;
        taskList[threadIndex].waitTime += waitTime;

        // hold the resources for busyTime
        rwlock_rdlock(&monitorLock);
        taskList[threadIndex].taskState = RUN;
        rwlock_unlock(&monitorLock);

        usleep(taskList[threadIndex].busyTime * 1000);
        
        // release all resources
        mutex_lock(&resourceMutex);
        acquiredResources = taskList[threadIndex].taskResources;
        for (int i = 0; i < acquiredResources.size(); ++i)
        {
            string resource = acquiredResources[i].name;
            int needed = acquiredResources[i].total;

            resources[resource] += needed;
            acquiredResources[i].held = 0;
        }
        taskList[threadIndex].taskResources = acquiredResources;
        mutex_unlock(&resourceMutex);

        // enter idle state 
        rwlock_rdlock(&monitorLock);
        taskList[threadIndex].taskState = IDLE;
        rwlock_unlock(&monitorLock);

        usleep(taskList[threadIndex].idleTime * 1000);

        // iteration complete
        taskList[threadIndex].numberIterations += 1;

        // print iteration complete message for task
        rwlock_wrlock(&monitorLock);
        printTaskStatus(threadIndex);
        rwlock_unlock(&monitorLock);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // specify the program start time
    struct timeval endTime;
    gettimeofday(&startTime, NULL);
    if (argc == 4) 
    {
        // initialize mutexes and locks
        mutex_init(&resourceMutex);
        rwlock_init(&monitorLock);

        int rval;
        map<string, int> maxResources;

        // initialize global variables
        resources.clear();
        maxResources.clear();

        // process the input
        string inputFile;
        int monitorTime;
        inputFile = argv[1];
        monitorTime = atoi(argv[2]);
        NITER = atoi(argv[3]);

        FILE *fp;
        // open the input file
        if ((fp = fopen(inputFile.c_str(), "r")) == NULL)
        {               
            rwlock_wrlock(&monitorLock);
            cout << "Provided input file is invalid: " << inputFile << endl;
            rwlock_unlock(&monitorLock); 
            return 1;
        }

        // start the monitoring thread
        pthread_t monitor_tid;
        rval = pthread_create(&monitor_tid, NULL, monitorThread, (void *) &monitorTime);
        if (rval) 
        {
            rwlock_wrlock(&monitorLock);
            perror("Error creating monitor thread");
            rwlock_unlock(&monitorLock); 
            exit(1); 
        }

        // read the input file and take appropriate actions
        char buf[MAXLINE];
        while (fgets(buf, MAXLINE, fp) != NULL)
        {
            string line(buf);
            istringstream wordstream(line);
            vector<string> words((istream_iterator<string>(wordstream)),
                                            istream_iterator<string>());
            if (words.size() == 0 || words[0][0] == '#')
            {
                // comment line or a blank line
                continue;
            }
            else
            {   
                // look for a resources or task line
                if (words[0].compare("resources") == 0) 
                {
                    if (resources.size() > 1)
                    {   // resources already declared
                        rwlock_wrlock(&monitorLock);
                        cout << "ERROR: Two resource lines specified" << endl;
                        rwlock_unlock(&monitorLock);
                        exit(1);
                    }
                    int numberResources = words.size() - 1;
                    if (numberResources > NRES_TYPES)
                    {   // can only have at most 10 resource types
                        numberResources = NRES_TYPES;
                    }
                    for (int i = 1; i < numberResources + 1; ++i)
                    {
                        // store a map of the resource keys and totals
                        // store another map for the currently available resources
                        int colonIndex = words[i].find(":");
                        string name = words[i].substr(0, colonIndex);
                        int value = atoi(words[i].substr(colonIndex+1, words[i].length() - colonIndex).c_str());
                        maxResources.insert(pair<string, int>(name, value));
                        resources.insert(pair<string, int>(name, value));
                    }
                }
                else if (words[0].compare("task") == 0)
                {
                    if (resources.size() < 1) 
                    {   // no resources exist
                        rwlock_wrlock(&monitorLock);
                        cout << "ERROR: No resources declared before task" << endl;
                        rwlock_unlock(&monitorLock);
                        exit(1);
                    }
                    if (numTasks >= 25) 
                    {   // no more tasks allowed
                        rwlock_wrlock(&monitorLock);
                        cout << "Max number of tasks started" << endl;
                        rwlock_unlock(&monitorLock);
                        break;
                    }

                    // create a new task
                    taskParameters task;
                    task.name = words[1];
                    task.busyTime = atoi(words[2].c_str());
                    task.idleTime = atoi(words[3].c_str());
                    task.taskState = WAIT; // set the initial state to wait
                    task.numberIterations = 0;
                    task.waitTime = 0;
                    task.ntid = 0;

                    // set the new task's required resources
                    for (int i = 4; i < words.size(); ++i)
                    {
                        int colonIndex = words[i].find(":");
                        string name = words[i].substr(0, colonIndex);
                        int value = atoi(words[i].substr(colonIndex+1, words[i].length() - colonIndex).c_str());
                        if (maxResources.find(name) == resources.end())
                        {   // no matching resource
                            rwlock_wrlock(&monitorLock);
                            cout << "Task " << task.name << " requires resources that don't exist" << endl;
                            rwlock_unlock(&monitorLock);
                            exit(1);
                        }
                        else if (maxResources[name] < value) 
                        {   // too many resources required
                            rwlock_wrlock(&monitorLock);
                            cout << "Task " << task.name << " requires more of resource " << name << " then are available" << endl;
                            rwlock_unlock(&monitorLock);
                            exit(1);
                        }
                        task.taskResources.push_back(createResource(name, value, 0));
                    }

                    // allocate memory for the index of the new task
                    int *idxPointer = new int;
                    *idxPointer = numTasks;

                    taskList[numTasks] = task;

                    // start the new task, pass it the pointer to its index in the taskList
                    rval = pthread_create(&taskList[numTasks].ntid, NULL, taskThread, (void *) idxPointer);
                    if (rval) 
                    {
                        perror("Error creating task thread"); 
                        exit(1); 
                    }
                    
                    // update the number of tasks, make sure the monitor is not printing when we do this
                    rwlock_wrlock(&monitorLock);
                    numTasks++;
                    rwlock_unlock(&monitorLock);              
                }
            }
        }

        fclose(fp);

        // wait for all threads to finish
        for (int i = 0; i < numTasks; ++i)
        {
            pthread_join(taskList[i].ntid, NULL);
        }

        // all threads have finished, exit the monitor thread and display output
        pthread_cancel(monitor_tid);
        pthread_join(monitor_tid, NULL);

        printSystemResources(maxResources);
        printSystemTasks();
        gettimeofday(&endTime, NULL);
        long long runTime = ((endTime.tv_sec * 1000000 + endTime.tv_usec) - (startTime.tv_sec * 1000000 + startTime.tv_usec)) / 1000;
        cout << "Running time= " << runTime << " msec" << endl;
    }
    else
    {
        cout << "Incorrect number of arguments specified" << endl;
    }
    return 0;
} 

 