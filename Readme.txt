CSC 360 Assignment 1
Cailan St Martin
Sept 2017

To compile this code, simply run "make" within the code directory.
To run the code, simply run "./PMan" (after compiling) within the code directory.

PMan is a basic process management tool. Its commands are:
	
	bg [executable] [arguments]
		Begins a process on the program specified by "executable", with the
		desired "arguments". This accepts either a path to an executable or a 
		executable in the environment path.
		
	bglist
		Lists all the processes currently managed by PMan.
		
	bgkill [pid]
		Kills the process with process ID "pid", if it was started by PMan. After
		it is killed, the process is no longer managed by PMan.
		
	bgstop [pid]
		Kills the process with process ID "pid", if it was started by PMan and
		is currently running. It can be restarted with bgstart.
		
	bgstart [pid]
		Starts the process with process ID "pid", if it was started by PMan and
		is currently stopped.
		
	pstat [pid]
		Displays various information about the process.
		
When a process managed by PMan terminates, it will be cleaned up and the user will
be notified that the process terminated. This may not occur until the next command is
entered by the user, since the main program is blocked as it waits for user input.
