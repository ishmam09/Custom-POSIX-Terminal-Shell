#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#define BUFFER_SIZE 1024
#define HISTORY_SIZE 80
#define MAX_ARGS 100
#define MAX_PATH 4096
char *history[HISTORY_SIZE];
int history_index = 0;
char *commands[100];
char input_buffer[BUFFER_SIZE];
int pipe_count = 0;
pid_t current_pid;
char current_dir[MAX_PATH];
char *arguments[MAX_ARGS];
int input_redirect = 0, output_redirect = 0;
char *input_file, *output_file;
int redirect_flag = 0;
void display_prompt();
void store_command_in_history(char *command);
void show_history();
void signal_handler(int sig);
void execute_command(char **args, int arg_count);
void handle_redirection(char **args, int arg_count, char *input_file, char *output_file, int append);
void execute_with_pipe();
static int run_command(int input, int is_first, int is_last, char *command);
static int split_command(char *command, int input, int is_first, int is_last);
void parse_input_for_redirection(char *command);
void parse_input_for_input_redirect(char *command);
void parse_input_for_output_redirect(char *command);
void change_to_directory();
void go_up_directory();
char* remove_comma(char* str);
int handle_multiple_commands(char *input);
void echo_output(char *command);
void execute_history_with_constants();

void echo_output(char *command){
    char *args[MAX_ARGS];
    int i = 0;
    args[i] = strtok(command, " ");
    while ((args[++i] = strtok(NULL, " ")) != NULL){
    }
    for (int j = 1; j < i; j++){
        printf("%s ", args[j]);
    }
    printf("\n");
}

void display_prompt(){
    printf("ri> ");
    fflush(stdout);
}

void store_command_in_history(char *command){
    if (history_index >= HISTORY_SIZE){
        free(history[0]);
        for (int j = 0; j < HISTORY_SIZE - 1; j++){
            history[j] = history[j + 1];
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    } 
    else{
        history[history_index] = strdup(command);
        history_index++;
    }
}

void show_history(){
    if (history_index == 0){
        printf("No commands in history\n");
        return;
    }
    printf("Command History:\n");
    for (int i = 0; i < history_index; i++){
        printf("%d: %s\n", i + 1, history[i]);
    }
}

void signal_handler(int button){
    if (button == SIGINT){
        if (current_pid > 0){
            kill(current_pid, SIGKILL);
        }
        fflush(stdout);
        display_prompt();
    }
}

void execute_command(char **args, int arg_count){
    current_pid = fork();
    if (current_pid == 0) {
        if (execvp(args[0], args) == -1){
            printf("Error executing command\n");
        }
        exit(EXIT_FAILURE);
    } 
    else if (current_pid > 0){
        wait(NULL);
    } 
    else{
        printf("Error in fork\n");
    }
    char full_command[BUFFER_SIZE] = "";
    for (int i = 0; i < arg_count; i++){
        strcat(full_command, args[i]);
        if (i < arg_count - 1){
            strcat(full_command, " ");
        }
    }
    store_command_in_history(full_command);
}

int handle_multiple_commands(char *input){
    char *command;
    int status = 0;
    store_command_in_history(input);
    command = strtok(input, ";");
    while (command != NULL){
        while (*command == ' ' || *command == '\t') command++;
        int len = strlen(command);
        while (len > 0 && (command[len - 1] == ' ' || command[len - 1] == '\t')){
            command[--len] = '\0';
        }
        if (strcmp(command, "history") == 0){
            show_history();
        } 
        else{
            status = system(command);
        }
        command = strtok(NULL, ";");
    }
    return status;
}

void handle_redirection(char **args, int arg_count, char *input_file, char *output_file, int append){
    current_pid = fork();
    if (current_pid == 0){
        if (input_file){
            int in_fd = open(input_file, O_RDONLY);
            if (in_fd == -1){
                printf("Error opening input file\n");
                exit(EXIT_FAILURE);
            }
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (output_file){
            int out_fd;
            if (append){
                out_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } 
            else{
                out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            if (out_fd == -1){
                printf("Error opening output file\n");
                exit(EXIT_FAILURE);
            }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        if (execvp(args[0], args) == -1){
            printf("Execvp error\n");
            exit(EXIT_FAILURE);
        }
    } 
    else if (current_pid > 0){
        wait(NULL);
    } 
    else{
        printf("Error in fork\n");
    }
    char full_command[BUFFER_SIZE] = "";
    for (int i = 0; i < arg_count; i++){
        strcat(full_command, args[i]);
        if (i < arg_count - 1){
            strcat(full_command, " ");
        }
    }
    if (input_file){
        strcat(full_command, " < ");
        strcat(full_command, input_file);
    }
    if (output_file){
        if (append){
            strcat(full_command, " >> ");
        } 
        else{
            strcat(full_command, " > ");
        }
        strcat(full_command, output_file);
    }

    store_command_in_history(full_command);
}

void execute_with_pipe(){
    int i, n = 1, input, first;
    input = 0;
    first = 1;
    commands[0] = strtok(input_buffer, "|");
    while ((commands[n] = strtok(NULL, "|")) != NULL)
        n++;
    commands[n] = NULL;
    pipe_count = n - 1;
    for (i = 0; i < n - 1; i++){
        input = run_command(input, first, 0, commands[i]);
        first = 0;
    }
    input = run_command(input, first, 1, commands[i]);
    input = 0;
    return;
}

static int run_command(int input, int is_first, int is_last, char *command){
    int pipe_fd[2], ret, input_fd, output_fd;
    ret = pipe(pipe_fd);
    if (ret == -1){
        perror("pipe");
        return 1;
    }
    current_pid = fork();
    if (current_pid == 0){
        if (is_first == 1 && is_last == 0 && input == 0){
            dup2(pipe_fd[1], 1);
        } 
        else if (is_first == 0 && is_last == 0 && input != 0){
            dup2(input, 0);
            dup2(pipe_fd[1], 1);
        } 
        else{
            dup2(input, 0);
        }
        if (strchr(command, '<') && strchr(command, '>')){
            input_redirect = 1;
            output_redirect = 1;
            parse_input_for_redirection(command);
        } 
        else if (strchr(command, '<')){
            input_redirect = 1;
            parse_input_for_input_redirect(command);
        } 
        else if (strchr(command, '>')){
            output_redirect = 1;
            parse_input_for_output_redirect(command);
        }
        if (output_redirect == 1){
            output_fd = creat(output_file, 0644);
            if (output_fd < 0){
                fprintf(stderr, "Failed to open %s for writing\n", output_file);
                return (EXIT_FAILURE);
            }
            dup2(output_fd, 1);
            close(output_fd);
            output_redirect = 0;
        }
        if (input_redirect == 1){
            input_fd = open(input_file, O_RDONLY, 0);
            if (input_fd < 0){
                fprintf(stderr, "Failed to open %s for reading\n", input_file);
                return (EXIT_FAILURE);
            }
            dup2(input_fd, 0);
            close(input_fd);
            input_redirect = 0;
        }
        if (strcmp(arguments[0], "pwd") == 0){
            if (getcwd(current_dir, sizeof(current_dir)) != NULL){
                printf("%s\n", current_dir);
            } 
            else{
                perror("getcwd() error");
            }
            exit(0);
        }

        if (strcmp(arguments[0], "echo") == 0){
            echo_output(command);
        } 
        else if (strcmp(arguments[0], "history") == 0){
            show_history();
            exit(0);
        } 
        else{
            if (execvp(arguments[0], arguments) < 0){
                printf("%s: command not found\n", arguments[0]);
            }
        }
        exit(0);
    } 
    else{
        waitpid(current_pid, 0, 0);
    }
    if (is_last == 1){
        close(pipe_fd[0]);
    }
    if (input != 0){
        close(input);
    }
    close(pipe_fd[1]);
    return pipe_fd[0];
}

static int split_command(char *command, int input, int is_first, int is_last) {
    char *new_command;
    new_command = strdup(command);
    int m = 1;
    arguments[0] = strtok(command, " ");
    while ((arguments[m] = strtok(NULL, " ")) != NULL)
        m++;
    arguments[m] = NULL;
    if (arguments[0] != NULL){
        if (strcmp(arguments[0], "exit") == 0){
            exit(0);
        }
        if (strcmp(arguments[0], "echo") != 0){
            command = remove_comma(new_command);
            int m = 1;
            arguments[0] = strtok(command, " ");
            while ((arguments[m] = strtok(NULL, " ")) != NULL)
                m++;
            arguments[m] = NULL;
        }
        if (strcmp("cd", arguments[0]) == 0){
            change_to_directory();
            return 1;
        } 
        else if (strcmp("pwd", arguments[0]) == 0){
            go_up_directory();
            return 1;
        }
    }
    return run_command(input, is_first, is_last, new_command);
}

void parse_input_for_redirection(char *command){
}
void parse_input_for_input_redirect(char *command){
}
void parse_input_for_output_redirect(char *command){
}
void execute_history_with_constants(){
}
void change_to_directory(){
}
void go_up_directory(){
}
char* remove_comma(char* str){
    return str;
}
int main(){
    char input[BUFFER_SIZE];
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    while (1) {
        display_prompt();
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            printf("\n");
            break;
        }
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            handle_multiple_commands(input);
        }
    }
    for (int i = 0; i < history_index; i++) {
        free(history[i]);
    }
    return 0;
}
