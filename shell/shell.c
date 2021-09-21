/**
 * shell
 * CS 241 - Spring 2021
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

void handle_signal(int sig);
void store_history(char* arg);
void run_file(char* arg);
void execute_multiple_commands(FILE* stream);
int execute_single_command(char* command_line, char*** argv);
int handle_operator(char* command_line);
void get_argc_argv(char* command_line, int* argc, char*** argv);
int built_in_command(int argc, char** argv, char* command);
int is_background(int argc, char** argv);
int fork_exec_wait(int argc, char** argv, char* command);
void add_process_info_vector(int pid, char* command_line);
void update_process_info_vector();
void destroy_argv(char** argv);
void kill_processes();
void destroy_shell();
void exit_shell(int status);

typedef struct process {
    char *command;
    pid_t pid;
} process;

#define PATHMAXSIZE 256

static char* cur_dir = NULL;   //store current working directory
static vector* history_vector = NULL;
static char* history_file_path = NULL;
static vector* process_info_vector = NULL;
static sstring* sstr = NULL; //store the sstr of the command line from stream
static vector* vec = NULL; //store the string of args split by ' ' from sstr
static process* foreground_running_process = NULL; //store the currently running foreground process

static char* buffer = NULL;
static char* first_command = NULL;  //store the string of first command if there is operator
static char* second_command = NULL; //store the string of second command if there is operator
static char** argvalue = NULL; //store the array of arguments of a command
static char** argvalue_temp = NULL; //store store the array of arguments of a command from history_vector
static int command_need_store = 0;  //mark whether to store the command to history_vector
static FILE* dest = 0; //file that need to store the output of a command

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    signal(SIGINT, handle_signal);
    cur_dir = malloc(sizeof(char) * PATHMAXSIZE);
    history_vector = string_vector_create();
    process_info_vector = shallow_vector_create();
    add_process_info_vector(getpid(), *argv);
    int opt;
    if (argc >= 2) {
        while ((opt = getopt(argc, argv, "h:f:")) != -1) {
            switch (opt) {
            case 'h':
                if (!optarg) {
                    print_usage();
                    exit_shell(1);
                }
                store_history(optarg);
                break;
            case 'f':
                if (!optarg) {
                    print_usage();
                    exit_shell(1);
                }
                run_file(optarg);
                break;
            default: /* '?' */
                print_usage();
                exit_shell(1);
            }
        }
    }
    //TODO
    execute_multiple_commands(stdin);
    exit_shell(0);
    return 0;
}

//handle signal
void handle_signal(int sig) {
    //TODO
    if (sig == SIGINT && foreground_running_process) {
        if (kill(foreground_running_process->pid, SIGINT) != -1) {
            print_killed_process(foreground_running_process->pid, foreground_running_process->command);
        } else {
            print_no_process_found(foreground_running_process->pid);
        }
        free(foreground_running_process->command);
        free(foreground_running_process);
        foreground_running_process = NULL;
    }
}

//handle -h
void store_history(char* arg) {
    //Open for reading and appending (writing at end of file).  The file is created if it does not exist.
    //The initial file position for reading is at the beginning of the file, but output is always appended to the end of the file.
    FILE* history_file = fopen(arg, "a+");
    if (history_file == NULL) {
        print_history_file_error();
        exit_shell(1);
    }
	size_t capacity = 0;
	while (getline(&buffer, &capacity, history_file) != -1) {
        if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }
        vector_push_back(history_vector, buffer);
	}
	free(buffer);
    buffer = NULL;
    fclose(history_file);
    //store path to history_file_path
    history_file_path = get_full_path(arg);
}

//handle -f
void run_file(char* arg) {
    FILE* file = fopen(arg, "r");
    if (file == NULL) {
        print_script_file_error();
        exit_shell(1);
    }
    execute_multiple_commands(file);
    fclose(file);
    file = NULL;
    exit_shell(0);
}

void execute_multiple_commands(FILE* stream) {
    //Print a command prompt
    //Read the command from standard input
    //Print the PID of the process executing the command (with the exception of built-in commands), and run the command
    //TODO
    
	size_t capacity = 0;
    if (!getcwd(cur_dir, PATHMAXSIZE)) {
        exit_shell(1);
    }
    print_prompt(cur_dir, getpid());
	while (getline(&buffer, &capacity, stream) != -1) {
        //wait for process with group id equals current process
        waitpid(0, 0, WNOHANG);
        //get the input
        if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }
        execute_single_command(buffer, &argvalue);
        if (!getcwd(cur_dir, PATHMAXSIZE)) {
            exit_shell(1);
        }
        print_prompt(cur_dir, getpid());
	}
}

int execute_single_command(char* command_line, char*** argv) {
    //transform string into argc and argv
    int flag = handle_operator(command_line);
    int argc = 0;
    int res = 0;    //flag for success/fail command
    int save_out = 0;   //saved file descriptor for stdout
    int save_int = 0;
    switch (flag) {
        case 0: //no operator
            get_argc_argv(command_line, &argc, argv);
            if ((res = built_in_command((int)argc, *argv, command_line)) == 2) {
                res = fork_exec_wait((int)argc, *argv, command_line);
            }
            break;
        case 1: //Operator &&
            //exec first command
            get_argc_argv(first_command, &argc, argv);
            res = built_in_command((int)argc, *argv, first_command);
            if (res == 2) {
                //not built-in command, try exec external command
                res = fork_exec_wait((int)argc, *argv, first_command);
            }
            destroy_argv(*argv);
            *argv = NULL;
            //if first command success, exec second command
            if (res == 0) {
                get_argc_argv(second_command, &argc, argv);
                if (built_in_command((int)argc, *argv, second_command) == 2) {
                    fork_exec_wait((int)argc, *argv, second_command);
                }
            }
            break;
        case 2: //Operator ||
            //exec first command
            get_argc_argv(first_command, &argc, argv);
            res = built_in_command((int)argc, *argv, first_command);
            if (res == 2) {
                //not built-in command, try exec external command
                res = fork_exec_wait((int)argc, *argv, first_command);
            }
            destroy_argv(*argv);
            *argv = NULL;
            //if first command fail, exec second command
            if (res == 1) {
                get_argc_argv(second_command, &argc, argv);
                if (built_in_command((int)argc, *argv, second_command) == 2) {
                    fork_exec_wait((int)argc, *argv, second_command);
                }
            }
            break;
        case 3: //Operator ;
            //exec first command
            get_argc_argv(first_command, &argc, argv);
            if (built_in_command((int)argc, *argv, first_command) == 2) {
                fork_exec_wait((int)argc, *argv, first_command);
            }
            destroy_argv(*argv);
            *argv = NULL;
            //exec second command
            get_argc_argv(second_command, &argc, argv);
            if (built_in_command((int)argc, *argv, second_command) == 2) {
                fork_exec_wait((int)argc, *argv, second_command);
            }
            break;
        case 4: //operator >
            save_out = dup(fileno(stdout));
            //get the file name in (*argv)[0]
            get_argc_argv(second_command, &argc, argv);
            //open the file
            dest = fopen((*argv)[0], "w+");
            if (dest == NULL) {
                print_redirection_file_error();
                return 1;
            }
            destroy_argv(*argv);
            *argv = NULL;
            //exec the command
            get_argc_argv(first_command, &argc, argv);
            if (built_in_command((int)argc, *argv, first_command) == 2) {
                fork_exec_wait((int)argc, *argv, first_command);
            }
            fflush(stdout);
            if (dest) { fclose(dest); dest = NULL; }
            dup2(save_out, fileno(stdout));
            close(save_out);
            command_need_store = 1;
            break;
        case 5: //operator >>
            save_out = dup(fileno(stdout));
            //get the file name in (*argv)[0]
            get_argc_argv(second_command, &argc, argv);
            //open the file
            dest = fopen((*argv)[0], "a+");
            if (dest == NULL) {
                print_redirection_file_error();
                return 1;
            }
            destroy_argv(*argv);
            *argv = NULL;
            //exec the command
            get_argc_argv(first_command, &argc, argv);
            if (built_in_command((int)argc, *argv, first_command) == 2) {
                fork_exec_wait((int)argc, *argv, first_command);
            }
            fflush(stdout);
            if (dest) { fclose(dest); dest = NULL; }
            dup2(save_out, fileno(stdout));
            close(save_out);
            command_need_store = 1;
            break;
        case 6: //operator <
            save_int = dup(fileno(stdin));
            //get the file name in (*argv)[0]
            get_argc_argv(second_command, &argc, argv);
            //open the file
	        dest = fopen((*argv)[0], "r");
            if (dest == NULL) {
                print_redirection_file_error();
                return 1;
            }
            destroy_argv(*argv);
            *argv = NULL;
            dup2(fileno(dest), fileno(stdin));
            if (dest) { fclose(dest); dest = NULL; }
            //exec the command
            get_argc_argv(first_command, &argc, argv);
            if (built_in_command((int)argc, *argv, first_command) == 2) {
                fork_exec_wait((int)argc, *argv, first_command);
            }
            dup2(save_int, fileno(stdin));
            close(save_int);
            command_need_store = 1;
            break;
    }
    //store the command to history_vector if needed
    if (command_need_store) vector_push_back(history_vector, command_line);

    if (first_command) { free(first_command); first_command = NULL; }
    if (second_command) { free(second_command); second_command = NULL; }
    destroy_argv(*argv);
    *argv = NULL;
    fflush(stdout);
    return res;
}

//handle operators in command
int handle_operator(char* command_line) {
    size_t i = 0, len = strlen(command_line);
    for (; i < len; i++) {
        if (command_line[i] == '&' && i < len - 1 && command_line[i + 1] == '&') {
            first_command = malloc(sizeof(char) * (i + 1));
            strncpy(first_command, command_line, i + 1);
            first_command[i] = '\0';
            second_command = malloc(sizeof(char) * (len - i - 1));
            strncpy(second_command, command_line + i + 2, len - i - 1);
            second_command[len - i - 2] = '\0';
            return 1;
        }
        if (command_line[i] == '|' && i < len - 1 && command_line[i + 1] == '|') {
            first_command = malloc(sizeof(char) * (i + 1));
            strncpy(first_command, command_line, i + 1);
            first_command[i] = '\0';
            second_command = malloc(sizeof(char) * (len - i - 1));
            strncpy(second_command, command_line + i + 2, len - i - 1);
            second_command[len - i - 2] = '\0';
            return 2;
        }
        if (command_line[i] == ';') {
            first_command = malloc(sizeof(char) * (i + 1));
            strncpy(first_command, command_line, i + 1);
            first_command[i] = '\0';
            second_command = malloc(sizeof(char) * (len - i));
            strncpy(second_command, command_line + i + 1, len - i);
            second_command[len - i - 1] = '\0';
            return 3;
        }
        if (command_line[i] == '>' && i < len - 1 && command_line[i + 1] != '>') {
            first_command = malloc(sizeof(char) * (i + 1));
            strncpy(first_command, command_line, i + 1);
            first_command[i] = '\0';
            second_command = malloc(sizeof(char) * (len - i));
            strncpy(second_command, command_line + i + 1, len - i);
            second_command[len - i - 1] = '\0';
            return 4;
        }
        if (command_line[i] == '>' && i < len - 1 && command_line[i + 1] == '>') {
            first_command = malloc(sizeof(char) * (i + 1));
            strncpy(first_command, command_line, i + 1);
            first_command[i] = '\0';
            second_command = malloc(sizeof(char) * (len - i - 1));
            strncpy(second_command, command_line + i + 2, len - i - 1);
            second_command[len - i - 2] = '\0';
            return 5;
        }
        if (command_line[i] == '<') {
            first_command = malloc(sizeof(char) * (i + 1));
            strncpy(first_command, command_line, i + 1);
            first_command[i] = '\0';
            second_command = malloc(sizeof(char) * (len - i));
            strncpy(second_command, command_line + i + 1, len - i);
            second_command[len - i - 1] = '\0';
            return 6;
        }
    }
    return 0;
}

void get_argc_argv(char* command_line, int* argc, char*** argv) {
    sstr = cstr_to_sstring(command_line);
    vec = sstring_split(sstr, ' ');
    *argv = (char**) calloc(vector_size(vec) + 1, sizeof(char*));
    size_t i = 0, argv_idx = 0;
    for (; i < vector_size(vec); i++) {
        char* cur = vector_get(vec, i);
        if (!strcmp(cur, "")) continue;
        (*argv)[argv_idx] = (char*) calloc(strlen(cur) + 1, sizeof(char));
        strncpy((*argv)[argv_idx], cur, strlen(cur));
        if ((*argv)[argv_idx][strlen(cur) - 1] == '\n') (*argv)[argv_idx][strlen(cur) - 1] = '\0';
        argv_idx++;
    }
    *argc = argv_idx;
    vector_destroy(vec);
    vec = NULL;
    sstring_destroy(sstr);
    sstr = NULL;
}

//handle built-in command
int built_in_command(int argc, char** argv, char* command) {
    //if the command is built-in command and successfully run, return 0
    //if the command is built-in command and failed to run, return 1
    //the command is not built-in, return 2
    if (argc == 0) {
        print_invalid_command(command);
        return 1;
    }
    int is_built_in = 0;    //flag for success exec of built-in command

    //TODO
    if (!strcmp(argv[0], "cd")) {
        command_need_store = 1;
        if (argc <= 1) {
            print_no_directory("");
            return 1;
        }
        if (chdir(argv[1]) == -1) {
            print_no_directory(argv[1]);
            return 1;
        }
    } else if (!strcmp(argv[0], "!history")) {
        //not stored in history
        command_need_store = 0;
        size_t i = 0;
        for (; i < vector_size(history_vector); i++) {
            print_history_line(i, vector_get(history_vector, i));
        }
    } else if (argv[0][0] == '#') {
        //Print out the command before executing if there is a match.
        //The #<n> command itself is not stored in history, but the command being executed (if any) is.
        if (strcmp(argv[0], "#")) {
            size_t idx = atoi(argv[0] + 1);
            if (idx < vector_size(history_vector)) {
                print_command(vector_get(history_vector, idx));
                //try exec
                int res = execute_single_command(vector_get(history_vector, idx), &argvalue_temp);
                destroy_argv(argvalue_temp);
                argvalue_temp = NULL;
                command_need_store = 0;
                return res;
            }
        }
        print_invalid_index();
        command_need_store = 0;
        return 1;
    } else if (argv[0][0] == '!') {
        //Print out the command before executing if there is a match.
        //The !<prefix> command itself is not stored in history, but the command being executed (if any) is.
        int flag_match = 0;
        if (!strcmp(argv[0], "!")) {
            flag_match = 1;
        }
        int idx = vector_size(history_vector) - 1;
        for (; idx >= 0; idx--) {
            char* cur = vector_get(history_vector, idx);
            size_t prefix_i = 1, cur_i = 0;
            for (; prefix_i < strlen(argv[0]); prefix_i++) {
                while (cur_i < strlen(cur) && cur[cur_i] == ' ') cur_i++;
                if (cur_i >= strlen(cur) || argv[0][prefix_i] != cur[cur_i++]) {
                    flag_match = 0;
                    break;
                }
                if (prefix_i == strlen(argv[0]) - 1) {
                    flag_match = 1;
                }
            }
            if (!flag_match) continue;
            print_command(cur);
            //try exec
            int res = execute_single_command(cur, &argvalue_temp);
            destroy_argv(argvalue_temp);
            argvalue_temp = NULL;
            command_need_store = 0;
            return res;
        }
        print_no_history_match();
        command_need_store = 0;
        return 1;
    } else if (!strcmp(argv[0], "ps")) {
        update_process_info_vector();
        print_process_info_header();
        for (size_t i = 0; i < vector_size(process_info_vector); i++) {
            print_process_info(vector_get(process_info_vector, i));
        }
        command_need_store = 1;
    } else if (!strcmp(argv[0], "cont")) {
        if (argc == 1) {
            print_invalid_command(command);
            return 1;
        }
        //update process_info_vector to remove the dead process
        update_process_info_vector();
        //get pid
        int pid = atoi(argv[1]);
        process_info* cur = NULL;
        size_t i = 0;
        for (; i < vector_size(process_info_vector); i++) {
            cur = vector_get(process_info_vector, i);
            if (cur->pid == pid) {
                //DO SOMETHING
                if (kill(pid, SIGCONT) == 0) {
                    print_continued_process(pid, command);
                }
                break;
            }
        }
        if (i == vector_size(process_info_vector)) {
            print_no_process_found(pid);
        }
        command_need_store = 1;
    } else if (!strcmp(argv[0], "stop")) {
        if (argc == 1) {
            print_invalid_command(command);
            return 1;
        }
        //update process_info_vector to remove the dead process
        update_process_info_vector();
        //get pid
        int pid = atoi(argv[1]);
        process_info* cur = NULL;
        size_t i = 0;
        for (; i < vector_size(process_info_vector); i++) {
            cur = vector_get(process_info_vector, i);
            if (cur->pid == pid) {
                //DO SOMETHING
                if (kill(pid, SIGSTOP) == 0) {
                    print_stopped_process(pid, command);
                }
                break;
            }
        }
        if (i == vector_size(process_info_vector)) {
            print_no_process_found(pid);
        }
        command_need_store = 1;
    } else if (!strcmp(argv[0], "kill")) {
        if (argc == 1) {
            print_invalid_command(command);
            return 1;
        }
        //update process_info_vector to remove the dead process
        update_process_info_vector();
        //get pid
        int pid = atoi(argv[1]);
        process_info* cur = NULL;
        size_t i = 0;
        for (; i < vector_size(process_info_vector); i++) {
            cur = vector_get(process_info_vector, i);
            if (cur->pid == pid) {
                //DO SOMETHING
                if (kill(pid, SIGKILL) == 0) {
                    print_killed_process(pid, command);
                }
                break;
            }
        }
        if (i == vector_size(process_info_vector)) {
            print_no_process_found(pid);
        }
        command_need_store = 1;
    } else if (!strcmp(argv[0], "exit")) {
        //not stored in history
        exit_shell(0);
    } else {
        //not built-in command
        is_built_in = 2;
    }
    return is_built_in;
}

int is_background(int argc, char** argv) {
    if (argc > 0 && !strcmp(argv[argc - 1], "&")) {
        //should put to background if is external command
        free(argv[argc - 1]);
        argv[argc - 1] = NULL;
        return 1;
    }
    return 0;
}

//handle external command
int fork_exec_wait(int argc, char** argv, char* command) {
    fflush(stdout);
    int flag_background = is_background(argc, argv);
    pid_t child = fork();
    if (child == -1) {
        print_fork_failed();
        exit_shell(1);
    }
    //add process
    add_process_info_vector(child, command);
    if (child && !flag_background) {
        //store the child process to foreground_running_process
        foreground_running_process = malloc(sizeof(process));
        foreground_running_process->pid = child;
        foreground_running_process->command = malloc((strlen(command) + 1) * sizeof(char));
        strcpy(foreground_running_process->command, command);
    }
    //mark need to store command
    command_need_store = 1;

    int return_status = 0;
    if (child == 0) {
        //is child
        pid_t process_id = getpid();
        //print message
        print_command_executed(process_id);
        //if need redirection
        if (dest) {
            if (dup2(fileno(dest), fileno(stdout)) == -1) {
                print_redirection_file_error();
                fclose(dest);
                dest = NULL;
                return 1;
            }
            fclose(dest);
            dest = NULL;
        }
        //check is background
        if (flag_background) {
            if (setpgid(process_id, getppid()) == -1) {
                print_setpgid_failed();
                exit_shell(1);
            }
        }
        //exec command
        execvp(argv[0], argv);
        print_exec_failed(command);
        destroy_shell();
        exit(1);
    } else {
        //is parent
        //TODO
        int status = 0;
        if (flag_background) {
            waitpid(child, &status, WNOHANG);
            return 0;
        }

        pid_t w = waitpid(child, &status, 0);
        if (w == -1) {
            print_wait_failed();
            exit_shell(1);
        } else if (WIFEXITED(status)) {
            //child process terminated normally
            if (WEXITSTATUS(status) != 0){
                //child process exec fail
                return_status = 1;
            }
        } else if (!WIFSIGNALED(status)) {
            exit_shell(1);
        }
        //free foreground_running_process
        if (foreground_running_process) {
            free(foreground_running_process->command);
            free(foreground_running_process);
            foreground_running_process = NULL;
        }
    }
    return return_status;
}

void add_process_info_vector(int pid, char* command_line) {
    if (pid == 0) return;
    char filename[PATHMAXSIZE];
	sprintf(filename, "/proc/%d/stat", pid);
	FILE *f = fopen(filename, "r");
    if (!f) {
        print_no_process_found(pid);
        return;
    }

    char state = 0;
    unsigned long int utime = 0;
    unsigned long int stime = 0;
    long int nthreads = 0;
    unsigned long long int starttime = 0;
    unsigned long int vsize = 0;

	fscanf(f, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu %*ld %*ld %*ld %*ld %ld %*ld %llu %lu" /*%ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %u %u %llu %lu %ld*/, &state, &utime, &stime, &nthreads, &starttime, &vsize);
	fclose(f);
    
    //get the boottime
    unsigned long long int boottime = 0;
    f = popen("cat /proc/stat | grep btime", "r");
    fscanf(f, "%*s %llu", &boottime);
    fclose(f);

    //for test
    // printf("state = %c\n", state);
	// printf("utime = %lu\n", utime / sysconf(_SC_CLK_TCK));
	// printf("stime = %lu\n", stime / sysconf(_SC_CLK_TCK));
	// printf("nthreads = %ld\n", nthreads);
	// printf("starttime = %llu\n", starttime / sysconf(_SC_CLK_TCK));
    // printf("boottime = %llu\n", boottime);
	// printf("vsize = %lu\n", vsize);

    vsize /= 1024;

    size_t hours = 0, minutes = 0, seconds = 0;
    seconds = starttime / sysconf(_SC_CLK_TCK) + boottime;
    minutes = seconds / 60;
    hours = (minutes / 60) % 24;
    minutes %= 60;
    char* start_str = malloc(256);
    execution_time_to_string(start_str, 256, hours, minutes);
    
    seconds = utime / sysconf(_SC_CLK_TCK) + stime / sysconf(_SC_CLK_TCK);
    minutes = seconds / 60;
    seconds %= 60;
    char* time_str = malloc(256);
    execution_time_to_string(time_str, 256, minutes, seconds);

    char* command = calloc(strlen(command_line) + 1, 1);
    strcpy(command, command_line);
    //remove trailing space at the end of command
    for (int i = strlen(command) - 1; i >= 0; i--) {
        if (command[i] == ' ') {
            command[i] = '\0';
        } else {
            break;
        }
    }

    process_info* processinfo = NULL;
    //search process_info_vector for process with same pid
    for (size_t i = 0; i < vector_size(process_info_vector); i++) {
        if (((process_info*)vector_get(process_info_vector, i))->pid == pid) {
            processinfo = vector_get(process_info_vector, i);
            free(processinfo->start_str);
            free(processinfo->time_str);
            free(processinfo->command);
            break;
        }
    }
    //add processinfo to process_info_vector if no match existing pid
    if (!processinfo) {
        processinfo = malloc(sizeof(process_info));
        vector_push_back(process_info_vector, processinfo);
    }
    
    processinfo->pid = pid;
    processinfo->nthreads = nthreads;
    processinfo->vsize = vsize;
    processinfo->state = state;
    processinfo->start_str = start_str;
    processinfo->time_str = time_str;
    processinfo->command = command;
}

void update_process_info_vector() {
    process_info* cur = NULL;
    size_t i = 0;
    for (; i < vector_size(process_info_vector); i++) {
        cur = vector_get(process_info_vector, i);

        char filename[PATHMAXSIZE];
        sprintf(filename, "/proc/%d/stat", cur->pid);
        FILE *f = fopen(filename, "r");
        if (!f) {
            //need to remove this processinfo
            free(cur->start_str);
            free(cur->time_str);
            free(cur->command);
            free(cur);
            vector_erase(process_info_vector, i);
            i--;
            continue;
        }

        char state = 0;
        unsigned long int utime = 0;
        unsigned long int stime = 0;
        long int nthreads = 0;
        unsigned long int vsize = 0;

        fscanf(f, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu %*ld %*ld %*ld %*ld %ld %*ld %*llu %lu" /*%ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %u %u %llu %lu %ld*/, &state, &utime, &stime, &nthreads, &vsize);
        fclose(f);
        //remove processinfo if the process is dead
        if (state == 'X') {
            free(cur->start_str);
            free(cur->time_str);
            free(cur->command);
            free(cur);
            vector_erase(process_info_vector, i);
            i--;
            continue;
        }
        vsize /= 1024;
        size_t minutes = 0, seconds = 0;
        seconds = utime / CLOCKS_PER_SEC + stime / CLOCKS_PER_SEC;
        minutes = seconds / 60;
        seconds %= 60;
        char* time_str = malloc(256);
        execution_time_to_string(time_str, 256, minutes, seconds);
        //update information
        cur->nthreads = nthreads;
        cur->vsize = vsize;
        cur->state = state;
        free(cur->time_str);
        cur->time_str = time_str;
    }
}

void destroy_argv(char** argv) {
    if (argv) {
        size_t i = 0;
        while (argv[i]) {
            free(argv[i]);
            argv[i] = NULL;
            i++;
        }
        free(argv);
        argv = NULL;
    }
}

void destroy_shell() {
    //kill all processes
    //write history to history_file, close history_file
    //destroy history_vector
    //destroy process_info_vector
    if (process_info_vector) {
        size_t i = 0;
        for (; i < vector_size(process_info_vector); i++) {
            process_info* cur = vector_get(process_info_vector, i);
            //free memory
            free(cur->start_str);
            free(cur->time_str);
            free(cur->command);
            free(cur);
        }
        vector_destroy(process_info_vector);
    }
    if (foreground_running_process) {
        if (kill(foreground_running_process->pid, SIGKILL) != -1) {
            print_killed_process(foreground_running_process->pid, foreground_running_process->command);
        } else {
            print_no_process_found(foreground_running_process->pid);
        }
        free(foreground_running_process->command);
        free(foreground_running_process);
        foreground_running_process = NULL;
    }
    if (history_file_path && history_vector) {
        FILE* history_file = fopen(history_file_path, "w");
        size_t i = 0;
        for (; i < vector_size(history_vector); i++) {
            fprintf(history_file, "%s\n", vector_get(history_vector, i));
        }
        fclose(history_file);
        free(history_file_path);
    }
    if (dest) fclose(dest);
    if (vec) vector_destroy(vec);
    if (sstr) sstring_destroy(sstr);
    if (cur_dir) { free(cur_dir); cur_dir = NULL; }
    if (buffer) { free(buffer); buffer = NULL; }
    if (first_command) { free(first_command); first_command = NULL; }
    if (second_command) { free(second_command); second_command = NULL; }
    destroy_argv(argvalue);
    destroy_argv(argvalue_temp);
    if (history_vector) vector_destroy(history_vector);
}

void kill_processes() {
    if (process_info_vector) {
        size_t i = 0;
        for (; i < vector_size(process_info_vector); i++) {
            process_info* cur = vector_get(process_info_vector, i);
            if (cur->pid != getpid() && cur->state != 'X') {
                kill(cur->pid, SIGKILL);
            }
        }
    }
}

void exit_shell(int status) {
    kill_processes();
    destroy_shell();
    exit(status);
}
