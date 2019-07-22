// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10

/**
 * Command Input and Processing
 */
char history[HISTORY_DEPTH][COMMAND_LENGTH];
int index = 0;

void historyPrint(){ //prints out history
    char conv[4];
    int start = 0;
    if(index < 10){
        for(int j = 0; j < index; j++){
            start = j + 1;
            sprintf(conv, "%d", start);
            
            write(STDOUT_FILENO, conv, strlen(conv));
            write(STDOUT_FILENO, "\t", strlen("\t"));
            write(STDOUT_FILENO, &history[j][0], strlen(&history[j][0]));
            write(STDOUT_FILENO, "\n", strlen("\n"));
        }
    }
    else{
        int i = (index % 10) + 1; //find start position
        for(int j = 0; j < 10; j++){
            int start = index - 10 + j;
            sprintf(conv, "%d", start);
            
            if(i > 9) (i = i - 10);
            write(STDOUT_FILENO, conv, strlen(conv));
            write(STDOUT_FILENO, "\t", strlen("\t"));
            write(STDOUT_FILENO, &history[i][0], strlen(&history[i][0]));
            write(STDOUT_FILENO, "\n", strlen("\n"));
            i++;
        }
    }
}

void historyAdd(char *buff){
    if(buff == '\0') return;
    int i = (index % 10); //finds where to put new value
    strcpy(&history[i][0], buff);
    
    ++index;
}

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;
    
	// Read input
    int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);
    if ( (length < 0) && (errno !=EINTR) ){
        perror("Unable to read command. Terminating.\n");
        exit(-1);  /* terminate with error */
    }
    
    // Null terminate and strip \n.
    buff[length] = '\0';
    if (buff[strlen(buff) - 1] == '\n') {
        buff[strlen(buff) - 1] = '\0';
    }
    
    // Tokenize (saving original command string)
    int token_count = tokenize_command(buff, tokens);
    if (token_count == 0) {
        return;
    }
    if(strstr(&buff[0], "!") == NULL) historyAdd(buff);
    
    // Extract if running in background:
    if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
        *in_background = true;
        tokens[token_count - 1] = 0;
    }
}
void handle_SIGINT()
{
    historyPrint();
    write(STDOUT_FILENO, getcwd(NULL, COMMAND_LENGTH), strlen(getcwd(NULL, COMMAND_LENGTH)));
    write(STDOUT_FILENO, "> \n", strlen("> \n"));
    exit(0);
}

/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
	char input_buffer[COMMAND_LENGTH]; //use input buffer for history stuff
	char* tokens[NUM_TOKENS];
    char* cwd;
    int status;
    
    struct sigaction handler;
    handler.sa_handler = handle_SIGINT;
    handler.sa_flags = 0;
    sigemptyset(&handler.sa_mask);
    sigaction(SIGINT, &handler, NULL);

	while (true) {

		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
        write(STDOUT_FILENO, getcwd(NULL, COMMAND_LENGTH), strlen(getcwd(NULL, COMMAND_LENGTH)));
		write(STDOUT_FILENO, "> ", strlen("> "));
		_Bool in_background = false;
		read_command(input_buffer, tokens, &in_background);
        
		/* DEBUG: Dump out arguments:
		for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}*/
        if(tokens[0] != NULL){
            if(strcmp(tokens[0], "exit") == 0){
                break;
            }
            if(strcmp(tokens[0], "!!") == 0){
                if(index < 0){
                    write(STDOUT_FILENO, "Invalid entry\n", strlen("Invalid entry\n"));
                    continue;
                }
                else{
                    tokenize_command(&history[(index-1) % 10][0],tokens);
                    write(STDOUT_FILENO, tokens[0], strlen(tokens[0]));
                    historyAdd(&history[(index-1) % 10][0]);
                }
            }
            if(strstr(&tokens[0][0], "!") != NULL){
                int dex = atoi(tokens[0]);
                if(dex < 1 || dex > index || dex < index - 9){
                    write(STDOUT_FILENO, "Invalid entry\n", strlen("Invalid entry\n"));
                    historyAdd(&history[dex][0]);
                    ;
                }
                else{
                    tokenize_command(&history[dex+1][0],tokens);
                    write(STDOUT_FILENO, tokens[0], strlen(tokens[0]));
                    historyAdd(&history[dex+1][0]);
                }
            }
            if(strcmp(tokens[0], "pwd") == 0){
                cwd = getcwd(NULL, COMMAND_LENGTH);
                write(STDOUT_FILENO, "Current directory: %s", strlen("Current directory: "));
                write(STDOUT_FILENO, cwd, strlen(cwd));
            }
            else if(strcmp(tokens[0], "cd") == 0){
                if(!tokens[1]) write(STDOUT_FILENO, "Please enter a direcory\n", strlen("Please enter a direcory\n"));
                else{
                    if(chdir(tokens[1]) == 1){
                        cwd = getcwd(NULL, COMMAND_LENGTH);
                        write(STDOUT_FILENO, "Changing directory to: ", strlen("Changing directory to: "));
                        write(STDOUT_FILENO, cwd, strlen(cwd));
                        write(STDOUT_FILENO, "\n", strlen("\n"));
                    }
                    else{
                        cwd = getcwd(NULL, COMMAND_LENGTH);
                        strcat(cwd, "/");
                        strcat(cwd, tokens[1]);
                        chdir(cwd);
                        continue;
                    }
                }
            }
            else if(strcmp(tokens[0], "history") == 0){
                historyPrint();
                continue;
            }
            else{
                write(STDOUT_FILENO, "Unknown command\n", strlen("Unknown command\n"));
                continue;
            }
            if (in_background) {
                write(STDOUT_FILENO, "Run in background.\n", strlen("Run in background.\n"));
            }

        }//if(tokens[0] != NULL)

        pid_t child = fork();
        
        if(child < 0){
            write(STDOUT_FILENO, "Fork failed\n", strlen("Fork failed\n"));
        }
        if(child == 0){
            if(execvp(tokens[0], tokens) == -1){
                write(STDOUT_FILENO, "Exec failed\n", strlen("Exec failed\n"));
                exit(-1);
            }
        }
        if(!in_background) waitpid(child, &status, 0);
        
        // Cleanup any previously exited background child processes
        // (The zombies)
        while (waitpid(-1, NULL, WNOHANG) > 0){
            ; // do nothing.
        }
            /**
             * Steps For Basic Shell:
             * 1. Fork a child process
             * 2. Child process invokes execvp() using results in token array.
             * 3. If in_background is false, parent waits for
             *    child to finish. Otherwise, parent loops back to
             *    read_command() again immediately.
             */

	}//while
	return 0;
}
