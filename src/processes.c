#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "processes.h"

// See processes.h for description of each function.

typedef struct process_
{
	int process_id;
	char* filename;
	char state[8];
	struct process_* next;
} process;


void add_process(process_tracker* processes, int process_id, const char* filename)
{
	process* new_process_ptr = malloc(sizeof(process)); //freed on removal
	if (new_process_ptr == NULL)
	{
		perror("malloc() failed");
		exit(1);
	}
	new_process_ptr->process_id = process_id;
	
	new_process_ptr->filename = malloc(strlen(filename) + 1); //freed on removal
	if (new_process_ptr->filename == NULL)
	{
		perror("malloc() failed");
		exit(1);
	}
	strcpy(new_process_ptr->filename, filename);
	
	strcpy(new_process_ptr->state, "RUNNING");
	
	new_process_ptr->next = processes->head;
	processes->head = new_process_ptr;
}


int remove_process(process_tracker* processes, int process_id)
{
	process* curr = processes->head;
	
	if (curr == NULL)
	{
		return -1;
	}
	
	if (curr->process_id == process_id)
	{
		processes->head = curr->next;
		free(curr->filename);
		free(curr);
		return 0;
	}
	
	process* prev = curr;
	curr = curr->next;
	while (curr != NULL)
	{
		if (curr->process_id == process_id)
		{
			prev->next = curr->next;
			free(curr->filename);
			free(curr);
			return 0;
		}
		
		prev = curr;
		curr = curr->next;
	}
	
	return -1;
}


int set_process_state(process_tracker* processes, int process_id, const char* state)
{
	if (strlen(state) > 7)
	{
		//only have 8 bytes to hold state
		return -1;
	}
	process* curr = processes->head;
	while (curr != NULL)
	{
		if (curr->process_id == process_id)
		{
			strcpy(curr->state, state);
			return 0;
		}
		
		curr = curr->next;
	}
	
	//couldn't find the process id
	return -1;
}


void print_processes(const process_tracker* processes)
{
	int num_processes = 0;
	process* curr = processes->head;
	while (curr != NULL)
	{
		printf("%d: %s (%s)\n", curr->process_id, curr->filename, curr->state);
		num_processes++;
		curr = curr->next;
	}
	printf("Total background jobs: %d\n", num_processes);
}

/*
 * Utility function to read the first line from a file.
 */
int read_file_line(char* output, size_t output_len, const char* filename)
{
	FILE* fp;
	fp = fopen(filename, "r");
	if (fp != NULL)
	{
		fgets(output, output_len, fp);
	}
	else
	{
		printf("File %s failed to open.\n", filename);
		return -1;
	}
	fclose(fp);
	
	return 0;
}


int print_process_stats(const process_tracker* processes, int process_id)
{
	process* curr = processes->head;
	while (curr != NULL)
	{
		if (curr->process_id == process_id)
		{
			printf("\nStatistics for process %d:\n", curr->process_id);
			
			char pcb_directory[32];
			snprintf(pcb_directory, sizeof(pcb_directory), "/proc/%d", curr->process_id);
			
			char comm_filename[64];		
			snprintf(comm_filename, sizeof(comm_filename), "%s/comm", pcb_directory);
			char comm_value[64];
			read_file_line(comm_value, sizeof(comm_value), comm_filename);
			printf("comm: %s", comm_value);
			
			// Much of the info is found in /proc/[pid]/stat
			char stats_filename[64];
			snprintf(stats_filename, sizeof(stats_filename), "%s/stat", pcb_directory);
			char stats[1024];
			
			read_file_line(stats, sizeof(stats), stats_filename);
			
			char* curr_value;
			curr_value = strtok(stats, " ");
			int i;
			// State is the 3rd space-separated value in stats
			for (i = 0; i < 2; i++)
			{
				curr_value = strtok(NULL, " ");
			}
			printf("state: %s\n", curr_value);

			// Continue iterating: utime is the 14th space-separated value in stats
			for (i = 0; i < 11; i++)
			{
				curr_value = strtok(NULL, " ");			
			}
			float utime_s = atof(curr_value) / sysconf(_SC_CLK_TCK);
			printf("utime: %.2fs\n", utime_s);
			
			// stime is the 15th space-separated value in stats
			curr_value = strtok(NULL, " ");
			float stime_s = atof(curr_value) / sysconf(_SC_CLK_TCK);
			printf("stime: %.2fs\n", stime_s);
			
			// rss is the 24th space-separated value in stats
			for (i = 0; i < 9; i++)
			{
				curr_value = strtok(NULL, " ");			
			}
			printf("rss: %s\n", curr_value);
			
			// Context switching info is found in /proc/[pid]/status
			char status_filename[64];		
			snprintf(status_filename, sizeof(status_filename), "%s/status", pcb_directory);
			FILE* status_fp;
			status_fp = fopen(status_filename, "r");
			if (status_fp != NULL)
			{
				char curr_line[64];
				while (fgets(curr_line, sizeof(curr_line), status_fp) != NULL)
				{
					if (strncmp(curr_line, "voluntary_ctxt_switches", 23) == 0)
					{
						printf("%s", curr_line);
					}
					else if (strncmp(curr_line, "nonvoluntary_ctxt_switches", 25) == 0)
					{
						printf("%s", curr_line);
					}
				}
			}
			
			printf("\n");
			
			return 0;
		}
		curr = curr->next;
	}
	
	printf("Error: Process %d does not exist.\n", process_id);
	return -1;
}