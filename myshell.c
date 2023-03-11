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

    if (pipe(file_descriptor) < 0)
    {
        perror("ERROR: Failed to create pipe.\n");
        exit(1);
    }

    child_pid = fork();
    if (child_pid < 0)
    {
        perror("ERROR: Failed to fork.\n");
        exit(1);
    }

    //Child
    if (child_pid == 0)
    {
        close(file_descriptor[0]);

        if (background)
        {
            close(0);
            dup(input);
            close(input);
        }
        else
        {
            if (input != 0)
            {
                dup2(input, 0);
                close(input);
            }
        }

        if (last == 1)
        {
            if (the_pipeline->commands->redirect_out_path)
            {
                int fd_out = creat(the_pipeline->commands->redirect_out_path, 0644);

                if (fd_out < 0)
                {
                    perror("ERROR: Failed to create output file.\n");
                    exit(1);
                }

                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            else
            {
                dup2(file_descriptor[1], STDOUT_FILENO);
            }
        }
        else if (first == 1)
        {
            if (the_pipeline->commands->redirect_in_path)
            {
                int fd_in = open(the_pipeline->commands->redirect_in_path, O_RDONLY);

                if (fd_in < 0)
                {
                    perror("ERROR: Failed to open input file.\n");
                    exit(1);
                }

                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            else
            {
                dup2(file_descriptor[1], STDOUT_FILENO);
            }
        }
        else
        {
            dup2(input, STDIN_FILENO);
            dup2(file_descriptor[1], STDOUT_FILENO);
        }

        if (execvp(the_pipeline->commands->command_args[0], the_pipeline->commands->command_args) == -1)
        {
            perror("ERROR: Failed to execute command.\n");
            exit(1);
        }

        close(file_descriptor[1]);
    }
    //Parent
    else
    {
        if (!background)
        {
            waitpid(child_pid, &status, 0);
        }

        close(file_descriptor[1]);
    }

    if (input != 0)
    {
        close(input);
    }

    if (last == 1)
    {
        return file_descriptor[0];
    }
    else
    {
        return file_descriptor[1];
    }
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
