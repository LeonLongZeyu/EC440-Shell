#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "myshell_parser.h"

#define MAX_BUF 1024
#define SUPPRESS "-n"

//Child signal handler
void signalHandler()
{
    pid_t pid;
    int status;

    //Infinite loop
    while(1)
    {
        pid = waitpid(-1, &status, 0x00000001);

        if(pid <= 0)
        {
            return;
        }
    }
}

//Function that executes the command line
int execute_command(struct pipeline* the_pipeline, int input, int first, int last, bool background)
{
    pid_t child_pid;
    int status;
    int file_descriptor[2];

    //Forking
    child_pid = fork();

    //Creating a pipe
    pipe(file_descriptor);
    
    //Child
    if (child_pid == 0)
    {



        if(background)
        {
            close(file_descriptor[1]);
        }

        if (the_pipeline -> commands -> redirect_out_path)
        {
            int fd_out = creat(the_pipeline -> commands -> redirect_out_path, 0644);

            if (fd_out == -1)
            {
                perror("ERROR: Failed to open output file.\n");
                exit(0);
            }
            
            file_descriptor[1] = fd_out;
        }

        if (the_pipeline -> commands -> redirect_in_path)
        {
            int input_fd = open(the_pipeline -> commands -> redirect_in_path, 0x0000);

            if (input_fd < 0)
            {
                perror("ERROR: Failed to open input file.\n");
                exit(0);
            }

            if (dup2(input_fd, 0) < 0)
            {
                perror("ERROR: Failed to redirect input.\n");
                exit(0);
            }
            close(input_fd);

            file_descriptor[0] = input_fd;
        }
        
        if(first == 1 && last == 0 && input == 0)
        {
            dup2(file_descriptor[1], 1);
        }
        else if(first == 0 && last == 0 && input != 0)
        {
            dup2(input, 0);
            dup2(file_descriptor[1], 1);
        }
        else
        {
            if(the_pipeline -> commands -> redirect_out_path)
            {
                dup2(file_descriptor[1], 1);
            }

            if(the_pipeline -> commands -> redirect_in_path)
            {
                dup2(file_descriptor[1], 0);
            }
            dup2(input, 0);
        }

        //for fork()
        if(execvp(the_pipeline -> commands -> command_args[0], the_pipeline -> commands -> command_args) == -1)
        {
            perror("ERROR: Failed to fork.\n");
            exit(1);
        }

        close(file_descriptor[0]);
        close(file_descriptor[1]);
        close(0);
        dup(0);        
    }
    //Parent
    else
    {
        //Waiting for the child processes not in background
        if(!background)
        {
            wait(&status);
        }

        //Closing the write end of the pipe
        close(file_descriptor[1]);
    }
    
    //Closing the input
    if(input != 0)
    {
        close(input);
    }

    //Closing the read end of the pipe
    if(last == 1)
    {
        close(file_descriptor[0]);
    }
    
    //Returning the read end to the next pipe as an input
    return file_descriptor[0];
}



int main(int argc, char* argv[])
{
    char buf[MAX_BUF];
    struct pipeline* the_pipeline;
    bool suppress;

    //Check for -n character that suppresses the my_shell$ prompt
    if((argc > 1) && selector(argv[1], SUPPRESS))
    {
        suppress = 1;
    }   

    //Waiting for the child processes
    signal(SIGCHLD, signalHandler);

    //Infinite loop for constantly printing my_shell$, except when CRTL^D
    while(1)
    {
        const char *command_line;

        //Suppress prompt
        if(suppress)
        {
            //DO NOT print promt my_shell$
        }
        else
        {
            printf("my_shell$ ");
        }

        //Ctrl^D command to exit the shell
        if((command_line = fgets(buf, MAX_LINE_LENGTH, stdin)) == NULL)
        {
            break;
        }
        else
        {
            the_pipeline = pipeline_build(command_line);

            //Variables for control
            int input = 0;
            int first = 1;

            //Executing a pipeline
            while(the_pipeline -> commands -> next != NULL)
            {
                input = execute_command(the_pipeline, input, first, 0, the_pipeline -> is_background);
                first = 0;
                the_pipeline -> commands = the_pipeline -> commands -> next;
            }
            
            input = execute_command(the_pipeline, input, first, 1, the_pipeline -> is_background);
        }
    }

    //Freeing the memory that is taken by the pipeline
    pipeline_free(the_pipeline);
    
    return 0;
}
