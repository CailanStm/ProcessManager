/*
 * Holds information for a single process.
 * Private - for use within processes.c only.
 */
typedef struct process_ process;

/*
 * A data structure for keeping track of processes.
 */
typedef struct process_tracker_
{
	process* head;
} process_tracker;

/*
 * Adds a process to the supplied process_tracker.
 */
void add_process(process_tracker* processes, int process_id, const char* filename);

/*
 * Removes a process from the supplied process_tracker.
 *
 * Returns: 0 on success, -1 if the process could not be found.
 */
int remove_process(process_tracker* processes, int process_id);

/*
 * Sets the state of a process in the supplied process_tracker.
 * This allows the user to see at a glance the state of a process when using print_processes().
 * In PMan, the state will either be "RUNNING" or "STOPPED".
 *
 * Returns: 0 on success, -1 on failure (if the process could not be found, or
 * if the supplied state string was too long).
 */
int set_process_state(process_tracker* processes, int process_id, const char* state);

/*
 * Prints basic information about the processes in the supplied process tracker in a
 * human readable format.
 */
void print_processes(const process_tracker* processes);

/*
 * Prints various statistics about a single process in the supplied process_tracker.
 * All this information is obtained from the /proc directory.
 *
 * Returns: 0 on success, -1 if the process could not be found.
 */
int print_process_stats(const process_tracker* processes, int process_id);