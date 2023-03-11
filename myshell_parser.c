#include "myshell_parser.h"
#include "stddef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

//Lexing Tokens
#define AMPERSAND "&"
#define PIPE "|"
#define REDIR_IN "<"
#define REDIR_OUT ">"
#define WHITESPACE " \t\n"
#define SPECIAL_CHARS "&|<>"

//Selector: Determines what tokens are present
bool selector(char* line, char* chars)
{
    if(strstr(line, chars))
    {
		return true;
	}
    else
    {
		return false;
	}
}

//Structure for pipeline commands; Represents a single command in a pipeline
struct pipeline_command* initialize_pipeline_command_structure()
{
    struct pipeline_command* pipeline_command_structure = malloc(sizeof(struct pipeline_command));

    if (!pipeline_command_structure) //Failure message for malloc()
    {
        printf("ERROR: Pipeline Command Structure failed to initialize\n");
	}
    else
    {
        //Making all fields in the structure NULL as default
        for(int i = 0; i < MAX_ARGV_LENGTH; i++)
        {
            pipeline_command_structure -> command_args[i] = NULL;
        }
        pipeline_command_structure -> redirect_in_path = NULL;
        pipeline_command_structure -> redirect_out_path = NULL;
        pipeline_command_structure -> next = NULL;
    }
    return pipeline_command_structure;
}

//Structure for pipeline; Represents a collection of commands that are connected through a pipeline
struct pipeline* initialize_pipeline()
{
	struct pipeline_command* pipeline_command_structure = initialize_pipeline_command_structure(); /* Pointer to the first command in the pipeline*/
    
    struct pipeline* pipeline = malloc(sizeof(struct pipeline)); // Initialize pipeline

    if (!pipeline)//Failure message for malloc()
    {
        printf("ERROR: Pipeline failed to initialize\n");
    }
    else
    {
        pipeline -> commands = pipeline_command_structure; //Fills commands with appropriate structure from initialize_pipeline_command_structure()
        pipeline -> is_background = false; // True if this pipeline should execute in the background; False by default
    }
    
    return pipeline;
}

//Bool for is_background
bool background_check(char* line)
{
    if(selector(line, AMPERSAND))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//The function replaces all occurrences of oldWord in the original string with newWord.
char* replace_occurences(char* str, char* oldWord, char* newWord, int c, int oldWordLength, int newWordLength)
{
    char* result;
    int i = 0;
    int j = 0;

    result = (char*)malloc(strlen(str) + c * (newWordLength - oldWordLength) + 1);

    while (*str)
    {
        if (strstr(str, oldWord) == str)
        {
            strcpy(&result[i], newWord);
            i += newWordLength;
            str += oldWordLength;
        }
        else
        {
            result[i] = *str;
            i++;
            str++;
        }
    }

    result[i] = '\0';

    return result;
}

//counts the number of times oldWord is in the string
int count_occurrences(char* str, char* oldWord)
{
    int count = 0;
    int oldWordLength = strlen(oldWord);

    while ((str = strstr(str, oldWord)))
    {
        count++;
        str += oldWordLength;
    }

    return count;
}

//Handles all the '>' and '<' commands
struct pipeline_command* parse_command(char* cline, struct pipeline_command* p_command)
{
    int position_pointer = 0;
    int c = 0;
    int newWordLength = 0;
    int oldWordLength = 0;
    char* tokens;
    char* tok;
    char* line;
    char* result;
    char* temp;

//Handles the redir_in '<' command
    if (selector(cline, REDIR_IN))
    {
        if (token != NULL)
        {
            line = strdup(cline);
            tokens = strtok(line, REDIR_IN);
            tokens = strtok(NULL, REDIR_IN);
            tokens = strtok(tokens, WHITESPACE);
            tokens = strtok(tokens, SPECIAL_CHARS);
        }
        p_command -> redirect_in_path = tokens;
        temp = strdup(cline);
        oldWordLength = strlen(tokens);
        c = count_occurrences(temp, tokens);
        newWordLength = strlen("");
        result = replace_occurences(temp, tokens, "", c, oldWordLength, newWordLength);
        free(temp);
        cline = strdup(result);
        free(result);
        free(line);
    }

//Handles the redir_out '>' command
    if (selector(cline, REDIR_OUT))
    {
        if (token != NULL)
        {
            line = strdup(cline);
            tokens = strtok(line, REDIR_IN);
            tokens = strtok(NULL, REDIR_IN);
            tokens = strtok(tokens, WHITESPACE);
            tokens = strtok(tokens, SPECIAL_CHARS);
        }
        p_command -> redirect_out_path = tokens;
        temp = strdup(cline);
        oldWordLength = strlen(tokens);
        c = count_occurrences(temp, tokens);
        newWordLength = strlen("");
        result = replace_occurences(temp, tokens, "", c, oldWordLength, newWordLength);
        free(temp);
        cline = strdup(result);
        free(result);
        free(line);
    }

//Splits the command line into WHITESPACE and then SPECIAL_CHARS (Parsing?)
    char* tokens_WHITESPACE = strtok_r(cline, WHITESPACE, &cline);

    while (tokens_WHITESPACE != NULL)
    {
        // Parse each token using strtok_r()
        char* tok_SPECIAL_CHARS = strtok_r(tokens_WHITESPACE, SPECIAL_CHARS, &tokens_WHITESPACE);

        while (tok_SPECIAL_CHARS != NULL)
        {
            // Store each token in an array of command arguments
            p_command -> command_args[position_pointer] = tok_SPECIAL_CHARS;
            position_pointer++;

            // Parse the next token using strtok_r()
            tok_SPECIAL_CHARS = strtok_r(tokens_WHITESPACE, SPECIAL_CHARS, &tokens_WHITESPACE);
        }

        // Parse the next command argument using strtok_r()
        tokens_WHITESPACE = strtok_r(cline, WHITESPACE, &cline);
    }

    return p_command;
}

//Creates pipeline of commands to be executed in a sequence
void pipelines(char* pipe_tokens, struct pipeline* pipe)
{
    //Allocate memory for a new pipeline command and link it to the given pipe tokens
    struct pipeline_command* new_command = parse_command(pipe_tokens, initialize_pipeline_command_structure());

    //Check if the pipeline has any commands yet
    if (pipe -> commands == NULL)
    {
        pipe -> commands = new_command;
    }
    else
    {
        //Find the last command in the pipeline and link the new command to it
        struct pipeline_command* last_command = pipe -> commands;
        while (last_command -> next != NULL)
        {
            last_command = last_command->next;
        }

        last_command->next = new_command;
    }
}

//Create a pipeline structure that represents the given command line
struct pipeline *pipeline_build(const char *command_line)
{
	struct pipeline* pipeline_pipeline = initialize_pipeline();
	char* cline = strdup(command_line);
	char* pipe_tokens;

    //Check for ampersand & to run in background
    bool is_background = background_check(cline);
    if (is_background)
    {
        char *line = strdup(cline);
        line = strtok(line, AMPERSAND);
        pipeline_pipeline -> is_background = true;
    }

    //Check for pipe characters
    if(selector(cline, PIPE))
    {
        //Tokenizing cline with pipe | as delimiter
		while((pipe_tokens = strtok_r(cline, PIPE, &cline)))
        {
			pipelines(pipe_tokens, pipeline_pipeline);
		}

	}
    else
    {
        //Tokenizing cline with only one token without pipe |
		while((pipe_tokens = strtok_r(cline, PIPE, &cline)))
        {
			pipelines(pipe_tokens, pipeline_pipeline);
		}
	}

	return pipeline_pipeline;
}

//Frees a pipeline structure that was created with pipeline_build()
void pipeline_free(struct pipeline *pipeline)
{
	struct pipeline_command *command = initialize_pipeline_command_structure();
	struct pipeline_command *next_command = initialize_pipeline_command_structure();

	command = pipeline -> commands;
	
	while(command != NULL)
    {
		next_command = command -> next;
		free(command);
		command = next_command;
	}
	free(pipeline);
}
