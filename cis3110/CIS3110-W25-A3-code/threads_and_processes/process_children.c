/*
 * Hamilton-Wright, Andrew (2025)
 *
 * This small program is provided for contrast with thread_children.c
 * in this same diretory.  This demonstrates the creation, destruction
 * and cleanup of child processes.
 * 
 * There is no attempt to communicate between the processes here, so there
 * are no synchronization/IPC tools used.  Please see semaphore_process_demo.c
 * in the parent directory for an example of that.
 * 
 * This example is provided specifically for comparison with the thread
 * based creation, destruction and cleanup in the companion example.
 *
 * For all the nitty gritty on process management under Linux,
 * read the following man pages: fork, wait, and exit.
 * You may additionally be interested in the man page for exec, though
 * that tool is not used here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * Define the number of processes and number of messages each prints.
 */
#define	NCHILDREN	3
#define	NMESSAGES	5

/*
 * Define the function we will call to perform the process work
 */
void process_activity(int userdata);

/*
 * The main program creates the processes and then simply waits for
 * them to complete.
 */
int
main()
{
	int i, waitstatus;
	pid_t processid[NCHILDREN];


	fprintf(stderr, "main: creating processes\n");

	/*
	 * Create the child processes.
	 */
	for (i = 0; i < NCHILDREN; i++) {

	       	/* location where process identity is stored */
		processid[i] = fork();

		if (processid[i] < 0) {
			perror("fork() failed");
			exit (-1);

		} else if (processid[i] == 0) {
			/* we are in the new child, so simply call our function */
			process_activity(i);

			/*
			 * child must exit when complete  -- this is a no-op
			 * because the function above calls exit within
			 * it.  This is provided here to make clear that
			 * the child process should not proceed past this
			 * point
			 */
			exit(-1);

		} else {
			fprintf(stderr, "main: created child %d at pid %d\n",
				i, processid[i]);
		}
	}

	fprintf(stderr, "main: waiting for processes to terminate\n");

	/*
	 * Wait for each process to terminate.
	 *
	 * Because we are passing the process ID value, the first return
	 * from pthread_join() will not occur until the process we created
	 * first above actually completes, even though other children may
	 * have completed first and be hanging around "in limbo" (also known
	 * as "zombie status") waiting to be cleaned up.
	 */
	for (i = 0; i < NCHILDREN; i++) {
		waitpid(processid[i], &waitstatus, 0);
		if (WIFEXITED(waitstatus)) {
			fprintf(stderr, "main: child %d (pid %d) exitted with status %d\n",
					i, processid[i], WEXITSTATUS(waitstatus));
		} else {
			fprintf(stderr, "main: child %d (pid %d) crashed\n",
					i, processid[i]);
		}
	}

	printf("\n");

	fprintf(stderr, "main: process cleanup complete\n");

	return (0);
}

/*
 * This function is executed by each process.
 *
 * The int argument is the pointer to the process data we passed as
 * the data argument above.  This design pattern is referred to as a
 * "user data argument", usually seen in a "callback".  Here we technically
 * do not have a "callback" because that would imply code calling back into
 * the *same* process.
 */
void
process_activity(int userdata)
{
	int child_id_number;
	int i;
	

	child_id_number = userdata;

	/*
	 * Just loop the indicated number of times, placing strings on
	 * the terminal.
	 */
	for (i = 0; i < NMESSAGES; i++) {
		fprintf(stderr,"child: child %d (pid %d) printing, iteration %d\n",
			child_id_number, getpid(), i);
	}

	/*
	 * This call to exit() terminates this child process.
	 *
	 * We pass the unusual value 7 here so we can see where it appears
	 * through the call to waidpid()
	 */
	exit(7);

	/*
	 * Never gets here, because the process terminates
	 * just above us at exit().
	 */
}

