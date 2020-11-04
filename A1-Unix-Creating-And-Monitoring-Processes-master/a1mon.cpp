#include "includes.h"
#include "shared_functions.h"

map<pid_t, pid_t> pid_to_parent_map;
map<pid_t, string> pid_cmd_map;

// get a process's parent pid, using the pid_to_parent_map global map
pid_t get_parent_pid(pid_t key){
    if (pid_to_parent_map.count(key) > 0){
        return pid_to_parent_map.at(key);
    }
    else{
        return -1;
    }
}

// terminate a process given it's pid
void terminate_process(pid_t pid){
    printf("Terminating [%d, %s]\n", pid, pid_cmd_map.at(pid).c_str());
    kill(pid, SIGKILL);
    return;
}

// Terminate all children of a process and then recursivley kill all of thier children
void terminate_children(pid_t root){
    for (map<pid_t, pid_t>::const_iterator it = pid_to_parent_map.begin(); it != pid_to_parent_map.end(); ++it){
        if (it->second == root){
            terminate_process(it->first);

            // should never happen but just incase, to prevent an infinite recursive call
            if (it->first == it->second){
                continue;
            }

            terminate_children(it->first);            
        }
    }
    return;
}

// Print all the processes, and record them, if the target PID is no longer there
// terminate, all child processes
bool process_and_print_ps(pid_t target_pid){
    // used for reading ps results
    FILE *ps_out;
    char buffer [100];
    string line;
    
    // determine if target is in the results
    bool target_found = 0;

    // used to store in maps
    pid_t pid;
    pid_t parent_pid;
    string cmd;

    vector<string> split_input;

    // call ps
    ps_out = popen("ps -u $USER -o user,pid,ppid,state,start,cmd --sort start", "r");

    // iterate through the results, store and print them
    while (!feof(ps_out)){
       if (fgets(buffer , 100 , ps_out) == NULL){
           break;
       }
        line = string(buffer);

        get_vector_input(&split_input, line);

        pid = (pid_t) atoi(split_input[1].c_str());
        parent_pid = (pid_t) atoi(split_input[2].c_str());
        cmd = split_input[5];

        cout << line;

        if (pid == 0){
            split_input.clear();
            continue;
        }
        else if(pid == target_pid){
            target_found = 1;
        }
        
        if (pid_to_parent_map.count(pid) == 0){
            pid_to_parent_map.insert(map<pid_t, pid_t>::value_type(pid, parent_pid));
        }
        if (pid_cmd_map.count(pid) == 0){
            pid_cmd_map.insert(map<pid_t, string>::value_type(pid, cmd));
        }

        split_input.clear();
    }

    cout << endl << endl;

    // close popen
    pclose(ps_out);

    // if the target is not in the results we terminate all processes that can be traced back to 
    // the target process (e.g. children, grandchildren)
    if (target_found == 0){
            cout << "a1mon: target appears to have terminated; cleaning up" << endl;
            terminate_children(target_pid);
            return 1;
    }

    return 0;
}


// On a loop display the header, then call ps and print the results. 
// If the target PID is not in the results terminate all children of the target process
// Finally, we sleep for the interval
void run_main_loop(pid_t target_pid, int interval){
    unsigned int counter = 0;

    while (true){
        printf("a1mon [counter = %d, pid = %d, target_pid = %d, interval = %d sec]:\n", counter, getpid(), target_pid, interval);
        
        // Returns 1 if completed
        if (process_and_print_ps(target_pid)){
            cout << "exiting a1mon" << endl;
            return;
        }

        sleep(interval);
        counter++;
    }

}

// Sets CPU limit, gets interval and target pid and then passes them to the main loop
int main(int argc, char const *argv[]){
    // set CPU limit to be 10 minutes
    setCPU_limit(600);

    // must be given either 2 or 3 arguments
    if (argc < 2 || argc > 3){
        cout << "Invalid number of arguments" << endl;
        return 1;
    }

    // get target pid, and exit if it is invalid
    pid_t target_pid = (pid_t) atoi(argv[1]);

    if (target_pid <= 0){
        cout << "Invalid pid" << endl;
        return 1;
    }

    // get interval if there is one, otherwise default it to 3 seconds. Exit if given invalid interval
    int interval = 3;
    
    if (argc == 3){
        interval = atoi(argv[2]);
        if (interval <= 0){
            cout << "Invalid interval" << endl;
            return 1;
        }
    }

    // Begin the main loop
    run_main_loop(target_pid, interval);

    return 0;
}
