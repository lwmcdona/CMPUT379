// a4tasks.cpp is the main file, it is what handles the user input,
// reading from the file, and managing all threads

#include "includes.hpp"
#include "task.hpp"
#include "resources.hpp"
#include "shared.hpp"
#include "monitor.hpp"

vector<struct Resource> resources;
vector<class Task> tasks;
class Monitor monitor;
long int start_time;
struct Mutexes mutexes;

// Convert the string input, into a vector of strings, split on spaces
void get_vector_input(vector<string> *split_input, string input){
    (*split_input).clear();
    istringstream iss(input);
    for(input; iss >> input; ){
        (*split_input).push_back(input);
    }
}

// Process the resource line, instantiates an instance of the resource struct
// for each resource and provides it all the information stored in the file
void process_resource_line(vector<string> split_line){
    if (split_line.size() < 2){
        cout << "Error: Resource line is empty" << endl;
        exit(0);
    }

    string resource;
    int pos_of_colon; // used to split the resource name from the max available
    string resource_name;
    int max_available;
    for (int i = 1; i < split_line.size(); i++){
        resource = split_line[i];
        pos_of_colon = resource.find(':');
        resource_name = resource.substr(0, pos_of_colon);
        max_available = atoi(resource.substr(pos_of_colon + 1, resource.size() - pos_of_colon).c_str());

        // a resource must have atleast 1 max available
        if (max_available <= 0){
            printf("Error: Resource %s has an invalid maximum available value\n", resource_name.c_str());
            exit(0);
        }

        // create resource and add it resources vector
        struct Resource r = Resource(resource_name, max_available);
        resources.push_back(r);
    }
}

// given a resource name return a pointer to the resource
// also takes in a pointer to an int which is set to 1 if 
// the resource is found or is unchanged if not found
struct Resource* get_resource(string resource_name, int * found){
    struct Resource *resource;
    for (int i = 0; i < resources.size(); i++){
        resource = &resources[i];
        if (strcmp((*resource).name.c_str(), resource_name.c_str()) == 0){
            *found = 1;
            break;
        }
    }
    return resource;
}

// process a task line from the input file
// performs error checking to ensure the task line
// is valid. Instantiates an instance of the task
// class for each task and provides it all the information stored on the line
void process_task_line(vector<string> split_line, int number_of_iterations){
    if (split_line.size() < 4){
        cout << "Error: Task line is incorrect" << endl;
        exit(0);
    }

    string task_name = split_line[1];
    int busy_time = atoi(split_line[2].c_str());
    int idle_time = atoi(split_line[3].c_str());

    if (busy_time < 0 || idle_time < 0){
        cout << "Error: Task provided invalid times" << endl;
        exit(0);
    }

    // Instantiate an instance of the Task class
    class Task task = Task(task_name, busy_time, idle_time, number_of_iterations);
    string resource_string;
    int pos_of_colon; // used to split the resource name from the max available
    string resource_name;
    int needed;

    // for each resourse, add it to the task's needed resources
    for (int i = 4; i < split_line.size(); i++){
        resource_string = split_line[i];
        pos_of_colon = resource_string.find(':');
        resource_name = resource_string.substr(0, pos_of_colon);
        needed = atoi(resource_string.substr(pos_of_colon + 1, resource_string.size() - pos_of_colon).c_str());

        if (needed < 0){
            cout << "Error: Negative resources needed" << endl;
            exit(0);
        }

        int found = 0;
        struct Resource *resource = get_resource(resource_name, &found);

        if (!found){
            printf("Error: Task %s wants to use resource %s. However, that resource is not defined.\n",
            task_name.c_str(), resource_name.c_str());
            exit(0);
        }

        struct Needed_Resource needed_resource = Needed_Resource(resource, needed);
        task.add_needed_resource(needed_resource);
    }

    // add the task to the tasks vector
    tasks.push_back(task);
} 

// Processes the input file, skip over comments and empty lines
// also detects errors
void process_input_file(string input_file_name, int number_of_iterations){
    string line;
    vector<string> split_line;
    ifstream input_file;
    input_file.open(input_file_name.c_str());

    int error_checker = -1;
    int line_no = 0;
    while(getline(input_file, line)) {

        if (line == ""){
            continue;
        }

        if (line.at(0) == '#'){
            line_no++;
            continue;
        }

        get_vector_input(&split_line, line);

        if (split_line.size() == 0){
            line_no++;
            continue;
        }

        if (strcmp(split_line[0].c_str(),"resources") == 0){ // resources line
            process_resource_line(split_line);
        }
        else if (strcmp(split_line[0].c_str(),"task") == 0){ // task line
            process_task_line(split_line, number_of_iterations);
        }
        else{
            printf("Error, line #%d in input file not recognized\n", line_no);
            exit(0);
        }

        line_no++;
        error_checker++;
    }

    if (error_checker == -1){
        cout << "Error in processing input file" << endl;
        exit(0);
    }

}

// set the monitors monitor time, and provide it
// valid pointers to all of the tasks.
void set_monitor(int monitor_time){
    monitor.set_monitor_time(monitor_time);

    for (int i = 0; i < tasks.size(); i++){
        class Task *task = &tasks[i];
        monitor.add_task(task);
    }
}

// execute all of the threads,
// this includes the monitor thread
// and all of the task threads
void create_threads(){
    monitor.execute();
    for (int i = 0; i < tasks.size(); i++){
        tasks[i].execute();
    }
}

// determines if all of the tasks have ran to completion
bool is_finished(){
    for (int i = 0; i < tasks.size(); i++){
        // a task is completed if it has completed all of its iterations
        if (tasks[i].iterations_completed != tasks[i].total_number_of_iterations){
            return false;
        }
    }
    return true;
}

// Prints all of the information about the resources
// and tasks. Is ran as the final portion of the program
void termination_printing(){
    mutexes.lock_mutex(&mutexes.printing_mutex); // last time we need to print, we do not unlock this mutex
    cout << "\nSystem Resources:" << endl;
    for (int i = 0; i < resources.size(); i++){
        printf("\t");
        resources[i].print();
    }
    cout << "\nSystem Tasks:" << endl;
    for (int i = 0; i < tasks.size(); i++){
        printf("[%d]\t", i);
        tasks[i].print_final();
    }
    printf("Running time= %d msec\n", (int) (get_current_time() - start_time));
}

// the main function, handles error checking for user input
// then proceeds to get the starting time, processes the
// input file, sets the monitor, initializes the mutexes,
// and creates the threads. Once completed does all 
// nessesary termination printing
int main(int argc, char const *argv[]){
    if (argc != 4){
        cout << "Invalid number of arguments." << endl;
        exit(0);
    }
    string input_file_name = argv[1];
    int monitor_time = atoi(argv[2]);
    int number_of_iterations = atoi(argv[3]);
    
    if (monitor_time <= 0){
        cout << "Incorrect Monitor Time Specified." << endl;
        exit(0);
    }

    if (number_of_iterations <= 0){
        cout << "Incorrect Number of Iterations Specified." << endl;
        exit(0);
    }
    
    start_time = get_current_time();
    process_input_file(input_file_name, number_of_iterations);

    set_monitor(monitor_time);

    mutexes.initialize_mutexes();

    create_threads();

    while(!is_finished()){}

    termination_printing();

    return 0;
}