/** This program acts as a shell when ran through linux terminal.
 *  1. Will be a able to change directory
 *  2. Execute a simple linux commands
 *  3. Allow for redirecting I/O using >, >>, >
 *  4. Allow for the user to single pipe cat "cat filename | grep hello"
 *  5. Commands running in the background using &
 *
 * Errors with program: redirecting when piping does not work
 *
**/

#include <stdio.h>
#include <stdlib.h> //exit
#include <sys/types.h> // pid_t
#include <sys/wait.h> //wait
#include <unistd.h> //fork, execlp, pipe
#include <string.h>
#include <fcntl.h>

/** Takes in a string and a char array of pointers, the method
 *  tokenizes the string by each space or \n in the string and
 *  will place it in the array. Also checks whether a piping is
 *  happening or &. It will return a postive value for where the
 *  pipe is at, return a negative where the & is at, and 0 if
 *  non of the above.
 * 
 * @param *array[] char array of pointers
 * @param str[] char array
 * @return piping integer
**/
int split_command(char *array[], char str[]) {
	char *path;
	int i = 0;
	int piping = 0;
	path = strtok(str, "\n ");
   	while( path != NULL ) {
		if(strcmp(path, "|") == 0)
			piping = i;
		else if (strcmp(path, "&") == 0)
			piping =  i*(-1);
		array[i++] = path;
		path = strtok(NULL, "\n ");
   	}
	return piping;
}

/** Takes input from the by using fgets
 * @param input[] char array
**/

void input_prompt(char input[]) {
	printf("\n:>");
	fgets(input, 2048, stdin);
}

/** Called when the user wants to change directory
 * in the shell. Takes a char array of pointers
 * to find the path that the user wants to switch to.
 * 
 * @param *command[] char array of pointers
**/

void cd_process(char *command[]) {
	int i;
	char path[64];
	for(i = 1; command[i] != NULL; i++) {
		if( i > 1) {
			strcat(path, " ");
			}
		strcat(path, command[i]);
	}
	if(chdir(path) != 0)
		printf("Failed to change to %s\n", path);
}

/** Will handle a execvp call by taking in commands 
 *  that were tokenized by forking, 
 *  no special types of command such as piping, &, 
 *  redirecting, and indirecting.
 *
 * @param char array of pointers
**/

void execvp_call(char *argv[]) {
	pid_t child = fork();
	if(child == 0)
		execvp(argv[0], argv);
	else
		wait(0);
}

/** Will be called when user wants to redirect command
 *  to a file that is specified. Forks the process to
 *  handle redirection to file that is done by replacing
 *  the stdout buffer with the file that was opened.
 *
 *  Creates new file if not existing or will overwrite
 *  the file
 * 
 * @param filename is the file used over the stdout
 * @param argv is the list of commands to execute
**/

void overrite_file(char filename[], char *argv[]) {
	pid_t child = fork();
	if(child == 0) {
		int saved_stdout = dup(1);
		int file_id = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		close(1); // close std output  stream
		dup(file_id);
		execvp(argv[0], argv);
		fflush(stdout);
		dup2(saved_stdout, 1);
	} else
		wait(0);
}


/** Will be called when user wants to redirect command
 *  to a file that is specified. Forks the process to
 *  handle redirection to file that is done by replacing
 *  the stdout buffer with the file that was opened.
 *
 *  Creates new file if not existing or will append to
 *  the file
 * 
 * @param filename is the file used over the stdout
 * @param argv is the list of commands to execute
**/

void append_file(char filename[], char *argv[]) {
	pid_t child = fork();
	if(child == 0) {
		int saved_stdout = dup(1);
		int file_id = open(filename, O_WRONLY | O_APPEND, 0666);
		close(1); // close std output  stream
		dup(file_id);
		execvp(argv[0], argv);
		fflush(stdout);
		dup2(saved_stdout, 1);
	} else
		wait(0);

}

/** Will direct a file into a command specified by the user
 *  by forking the process and replacing the stdin in the
 *  buffer to the file that was given for the command.
 *  
 *  Read only file
 * 
 * @param filename is the file used over the stdout
 * @param argv is the list of commands to execute
**/

void reading_file(char filename[], char *argv[]) {
	pid_t child = fork();
	if(child == 0) {
		int saved_stdin = dup(0);
		int file_id = open(filename, O_RDONLY);
		close(0);
		dup(file_id);
		execvp(argv[0], argv);
		dup2(saved_stdin, 0);
	} else
		wait(0);
}

/** This will be called when the system has recgonize
 *  that a pipe will be happening. The following code
 *  will split the given parameter commands by the
 *  piping integer where the integer represents the
 *  index that the piping is happening (piping == |).
 *  After creating two argumnets , one for the inner
 *  command and one for the outer command it will pipe
 *  and then fork() hte two process that need to be 
 *  executed.
**/

void pipe_command(char *commands[], int piping) {
	char *inner[4];
	char *outer[4];
	int i;
	for(i = 0; i < piping; i++)
		inner[i] = commands[i];
	inner[i] = NULL;

	int j;
	for(j = i+1 , i = 0; commands[j] != NULL; j++, i++)
		outer[i] = commands[j];
	outer[i] = NULL;

	int fd[2];
	pipe(fd);

	if(fork() == 0) {
		close(fd[0]); //close the read end of pipe
		dup2(fd[1], 1); // connects write end of pipe to stdout
		if(execvp(inner[0], inner) == -1 ) printf("%s\n","command not found");

	} else if (fork() == 0) {
		close(fd[1]); // close right end of pipe
		dup2(fd[0], 0); // connects write end of pipe stdout
		if(execvp(outer[0], outer) == -1 ) printf("%s\n","command not found");

	}

	close(fd[0]);
	close(fd[1]);
	wait(NULL);
	wait(NULL);
}

/** This will process the command when it has a & at the
 *  end of it. It will run a process and not have the
 *  parent of that process wait for the child. This 
 *  allows a process to be executed and run more during
 *  the execution.
 *
 * @param commands
 * @param piping
**/
void mulit_process (char *commands[], int piping) {
	char *argv[5];
	int i;
	for(i = 0; i < piping; i++)
		argv[i] = commands[i];
	argv[i] = NULL;
	pid_t child = fork();
	if(child == 0) {
		execvp(argv[0], argv);
		exit(0);
	}
}

/** This method will check what kind of process the user
 *  wants done. Weather it being a pipe command, &, a 
 *  redirect (>) , an overrite redirect (>>), an indirect
 *  (<), or a regular process and calls the appropriate
 *  method to do so.
 *
 * @param commands 
 * @param piping
**/
void proccess_command(char *commands[], int piping) {
	if(piping > 0 )
		pipe_command(commands, piping);
	else if (piping < 0)
		mulit_process(commands, (piping *(-1)));
	else if(commands[1] != NULL && strcmp(commands[1], ">>") == 0) {
		char *argv[] = {commands[0], NULL};
		append_file(commands[2], argv);
	}
	
	else if (commands[2] != NULL && strcmp(commands[2], ">>") == 0) {
		char *argv[] = {commands[0], commands[1], NULL};
		append_file(commands[3], argv);
	}

	else if(commands[1] != NULL && strcmp(commands[1], ">") == 0) {
		char *argv[] = {commands[0], NULL};
		overrite_file(commands[2], argv);
	}

	else if (commands[2] != NULL && strcmp(commands[2], ">") == 0) {
		char *argv[] = {commands[0], commands[1], NULL};
		overrite_file(commands[3], argv);
	}

	else if(commands[2] != NULL && strcmp(commands[2], "<") == 0) {
		char *argv[] = {commands[0], commands[1], NULL};
		reading_file(commands[3], argv);
	}

	else if(commands[1] != NULL && strcmp(commands[1], "<") == 0) {
		char *argv[] = {commands[0], NULL};
		reading_file(commands[2], argv);
	}

	else
		execvp_call(commands);
} 

/** The main program that runs the whole system.
 *  Declaring input and commands variables and
 *  keeps the user in a while loop unless the 
 *  types in "exit".
**/
int main() {

	printf("%s", "Matt Joyce's Shell\n");
	char input[2048];
	char *commands[6];
	do {
		memset(&commands[0], '\0', sizeof(commands));
		input_prompt(input);
		int piping = split_command(commands, input);
		if(strcmp(input, "exit") == 0)
			break;
		else  if(strcmp(commands[0], "cd") == 0)
			cd_process(commands);
		else
			proccess_command(commands, piping);
	}
	while(1);
	
}

