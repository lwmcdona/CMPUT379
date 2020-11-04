#include "includes.h"
#include "linked_list.h"
#include "shared_functions.h"

// Initialize the list of jobs as a global variable
linked_list *jobs = new linked_list();

// Run a process, first fork and then get the child to execute the specified process along with it's input parameters.
void run(vector<string> split_input, string input, int num_of_words){
   
    if(jobs->getsize() >= 32){
        cout << "Maximum number of processes already running please terminate a process before running a new one" << endl;
        return;
    }

    int status;
    pid_t child = fork();

    if (child == 0){ // child process
        const char *pgm = split_input[1].c_str(); // convert the pgm to a const char array

        int error_checker=0;

        error_checker = execlp(pgm, pgm, \
        (num_of_words>2) ? split_input[2].c_str(): (char *) NULL, \
        (num_of_words>3) ? split_input[3].c_str(): (char *) NULL, \
        (num_of_words>4) ? split_input[4].c_str(): (char *) NULL, \
        (num_of_words>5) ? split_input[5].c_str(): (char *) NULL, \
        (char *) NULL
        );

        if (error_checker == -1){
            exit(1);
        }
        else{
            exit(0);
        }
    }
    else{
        jobs->add(child, input); // add child to data structure
        return; // Do not exit main loop
    }
}

// determines if a string is strictly numeric (positive numbers only)
bool is_number(string s){

    if(s.size() == 0){
        return 0;
    }

    for (int i=0; i<s.size(); i++){
        if (!isdigit(s[i])){
            return 0;
        }
    }
    return 1;
}

// Send a signal to a process, given the process index. Returns the pid of the proccess.
pid_t sendsignal(vector<string> split_input, int num_of_words, int signo){
        if(num_of_words != 2 || !is_number(split_input[1])){
            cout << "Invalid Syntax" << endl;
            return -1;
        }
        int index;

        istringstream ss(split_input[1]);
        ss >> index;

        pid_t suspention_pid = jobs->getpid(index);

        if(suspention_pid == -1){
            cout << "That process does not exist" << endl;
            return -1;
        }
        kill(suspention_pid, signo);
        return suspention_pid;
}

// Find all proccesses that have not yet been terminated yet, and terminate them. Returns 0 to continue accepting input
// or returns 1 to end the program.
void terminate_all_proccesses(){
    node *temp = new node(-1, "", 0, NULL);
    temp = jobs->head;
    while(temp->next != NULL){
        temp = temp->next;
        if (!temp->terminated){
            temp->terminated = 1;
            kill(temp->pid, SIGKILL);
        }
    }
}

// This function handles the user's input, and routes it accordingly.
bool handle_input(string input){

    // If the input is empty, it is invalid
    if (input == ""){
        cout << "Invalid Syntax" << endl;
        return 0;
    }

    // first we handle one word operations
    if (input == "quit"){
        return 1; // Exit main loop
    }
    else if (input == "exit"){
        terminate_all_proccesses();
        return 1; // Exit main loop
    }
    else if (input == "list"){
        jobs->print();
        return 0; // Do not exit main loop
    }
    
    // now we handle multi word inputs
    // we first split up the input using a vector to store the result
    vector<string> split_input;
    get_vector_input(&split_input, input);
    int num_of_words = split_input.size();

    // If the user wants to run a process
    if(split_input[0] == "run" && num_of_words >= 2 && num_of_words <= 6){
        run(split_input, input, num_of_words);
        return 0; // Do not exit main loop 
    }
    // If the user wants to suspend a process
    else if (split_input[0] == "suspend" && num_of_words == 2){
        sendsignal(split_input, num_of_words, SIGSTOP);
        return 0; // Do not exit main loop
    } 
    // If the user wants to resume a process
    else if (split_input[0] == "resume" && num_of_words == 2){
        sendsignal(split_input, num_of_words, SIGCONT);
        return 0; // Do not exit main loop
    }
    // If the user wants to terminate a process
    else if (split_input[0] == "terminate" && num_of_words == 2){
        pid_t terminate_pid = sendsignal(split_input, num_of_words, SIGKILL);
        jobs->remove(terminate_pid);
        return 0; // Do not exit main loop
    }
    // Otherwise the input has invalid syntax
    else{
        cout << "Invalid Syntax" << endl;
        return 0; // Do not exit main loop
    }
    

    return 0; // Do not exit main loop
}

// This function gets the input from the user and passes it to the input handler function
void run_main_loop(){

    string input;

    while (true) {
        cout << "a1jobs [" << getpid() << "] : ";
        getline(cin,input);

        if(handle_input(input)){
            break;
        }
    }

    return;
}

// Wait on all zombie processes to free up thier resources
void cleanup(){
    int status;
    node *temp = new node(-1, "", 0, NULL);
    temp = jobs->head;
    while(temp->next != NULL){
        temp = temp->next;
        waitpid(temp->pid, &status, WNOHANG);
    }
}

int main(int argc, char *argv[]){
    // set CPU limit to be 10 minutes
    setCPU_limit(600);

    // Get CPU initial time
    struct tms initial_time_struct, final_time_struct;
    clock_t start, end;

    start = times(&initial_time_struct);

    // Issue the main loop
    run_main_loop();

    // Clean up Zombie Processes
    cleanup();

    // Get CPU final time
    end = times(&final_time_struct);

    static long clock_ticker = sysconf(_SC_CLK_TCK);

    // Print CPU Times
    printf("Real Time Difference: %7.2f.\n", (end - start) / (double) clock_ticker);
    printf("User CPU Time Difference: %7.2f.\n", (final_time_struct.tms_utime - initial_time_struct.tms_utime) / (double) clock_ticker);
    printf("System CPU Time Difference: %7.2f.\n", (final_time_struct.tms_stime - initial_time_struct.tms_stime) / (double) clock_ticker);
    printf("User Terminated Children CPU Time Difference: %7.2f.\n", (final_time_struct.tms_cutime - initial_time_struct.tms_cutime) / (double) clock_ticker);
    printf("System Terminated Children CPU Time Difference: %7.2f.\n", (final_time_struct.tms_cstime - initial_time_struct.tms_cstime) / (double) clock_ticker);

    return 0;
}