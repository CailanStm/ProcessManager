#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "processes.h"

/*
 * Handles the input of a bg command:
 * Starts a new child process using full_command as arguments, and adds it
 * to the supplied process_tracker.
 *
 * Returns: 0 on success, -1 on failure
 */
int handle_bg(const char* full_command, process_tracker* processes)
{
	char command_temp[1024];
	strcpy(command_temp, full_command);
	
	char* command_args[256];
	int i = 0;
	command_args[i] = strtok(command_temp, " ");
	while (command_args[i] != NULL)
	{
		if (i >= 256)
		{
			printf("The command you have entered is too long. Please try again.\n");
			return -1;
		}
		i++;
		command_args[i] = strtok(NULL, " ");
	}
	
	if (access(command_args[0], X_OK) != 0)
	{
		printf("Cannot run %s: does not exist, or insufficient permission.\n", command_args[0]);
		return -1;
	}
	
	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork() failed");
		exit(1);
	} 
	else if (pid == 0)
	{
		// Child process
		if (execvp(command_args[0], command_args) == -1)
		{
			if (errno == ENOENT)
			{
				printf("Cannot execute %s: no such file or directory. Please supply a path to the executable.\n", command_args[0]);
			}
			else
			{
				perror("execvp() failed");
			}
			
			exit(1); // just kill the child process, not the parent
		}
	}
	else
	{
		// Parent process
		if (waitpid(pid, NULL, WNOHANG) == -1)
		{
			perror("waitpid() failed");
		}
		char fullpath[128];
		realpath(command_args[0], fullpath);
		add_process(processes, pid, fullpath);
	}
	
	// If we reach this, all is well
	return 0;
}

/*
 * Handles the bgkill, bgstop and bgstart commands.
 * Calls the system call kill() with the appropriate signal. Handles errors
 * if an improper process id is supplied.
 *
 * Returns: 0 on success, -1 on failure
 */
int handle_bgsignal(const char* pid_str, process_tracker* processes, int signal)
{
	int pid = atoi(pid_str);
	if (pid == 0)
	{
		printf("Invalid pid supplied. Usage: bgkill [pid]\n");
		return -1;
	}
	else if (kill(pid, signal) == -1)
	{
		if (errno == ESRCH)
		{
			printf("This process does not exist.\n");
		}
		else if (errno == EPERM)
		{
			printf("You do not have permission to modify this process.\n");
		}
		else
		{
			perror("kill() failed");
			exit(1);
		}
		return -1;
	}
	else
	{
		if (signal == SIGTERM)
		{
			if (remove_process(processes, pid) == -1)
			{
				printf("This process is not managed by PMan.\n");
			}
			else
			{
				printf("Killed process [%d]\n", pid);	
			}
		}
		else if (signal == SIGSTOP)
		{
			if (set_process_state(processes, pid, "STOPPED") == -1)
			{
				printf("This process is not managed by PMan.\n");
			}
			else
			{
				printf("Stopped process [%d]\n", pid);
			}
		}
		else if (signal == SIGCONT)
		{
			if (set_process_state(processes, pid, "RUNNING") == -1)
			{
				printf("This process is not managed by PMan.\n");
			}
			else
			{
				printf("Continued process [%d]\n", pid);
			}
		}
		return 0;
	}
}

/*
 * Calls a processes.h function to handle a pstat input.
 *
 * Returns: 0 on success, -1 on failure (if user input is invalid)
 */
int handle_pstat(const char* pid_str, process_tracker* processes)
{
	int pid = atoi(pid_str);
	if (pid == 0)
	{
		printf("Invalid pid supplied. Usage: pstat [pid]\n");
		return -1;
	}
	
	print_process_stats(processes, pid);
	return 0;
}

/*
 * Handles user input, including calling the correct handler function and 
 * handling invalid commands.
 *
 * Returns: 0 on success, -1 on failure (user input too long, command not found).
 */
int handle_input(const char* user_input, size_t max_input_len, process_tracker* processes)
{
	size_t command_len = strcspn(user_input, " ");
	if (command_len > max_input_len) {
		printf("%s: user input too long\n", user_input);
		return -1;
	}
	
	char command[command_len + 1];
	command[command_len + 1] = '\0';
	memcpy(command, user_input, command_len);
	
	char argument[max_input_len];
	strcpy(argument, user_input + command_len + 1);
	
	if (strcmp(command, "bg") == 0)
	{
		handle_bg(argument, processes);
	}
	else if (strcmp(command, "bglist") == 0)
	{
		print_processes(processes);
	}
	else if (strcmp(command, "bgkill") == 0)
	{
		handle_bgsignal(argument, processes, SIGTERM);
	}
	else if (strcmp(command, "bgstop") == 0)
	{
		handle_bgsignal(argument, processes, SIGSTOP);
	}
	else if (strcmp(command, "bgstart") == 0)
	{
		handle_bgsignal(argument, processes, SIGCONT);
	}
	else if (strcmp(command, "pstat") == 0)
	{
		handle_pstat(argument, processes);
	}
	else
	{
		printf("PMan: >%s: command not found\n", command);
		return -1;
	}
	
	return 0;
}

/*
 * Removes any zombie processes that were managed by PMan.
 * Also, notifies the user if any processes have been terminated.
 */
void reap_zombie_processes(process_tracker* processes)
{
	int reaped_process;
	do {
		reaped_process = waitpid(-1, NULL, WNOHANG);		
		if (reaped_process == -1)
		{
			// If error is ECHILD, that just means that there were no zombie
			// processes, which is okay
			if (errno != ECHILD)
			{
				perror("waitpid failed");
				exit(1);
			}			
		}
		else if (reaped_process > 0)
		{
			if (remove_process(processes, reaped_process) == 0)
			{
				// Only do this if it was actually removed here (if not,
				// then the user must have killed it manually, so we don't
				// need to notify them again)
				printf("Process %d has terminated.\n", reaped_process);
			}
		}
	} while (reaped_process > 0);
}

int main()
{
	process_tracker processes = {NULL};
	
	while(1)
	{		
		printf("PMan: >");
		
		int max_input_len = 1024;
		char user_input[max_input_len];
		fgets(user_input, max_input_len, stdin);
		user_input[strcspn(user_input, "\n")] = '\0'; //remove trailing newline
		
		reap_zombie_processes(&processes);
		
		handle_input(user_input, max_input_len, &processes);
	}
}