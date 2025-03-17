/*
 * Hamilton-Wright, Andrew (2025)
 *
 * This small program is provided for contrast with process_children.c
 * in this same diretory.  This demonstrates the creation, destruction
 * and cleanup of POSIX threads (Pthreads for short).
 * 
 * There is no attempt to communicate between the threads here, so there
 * are no synchronization/IPC tools used.  Please see semaphore_pthreads_demo.c
 * in the parent directory for an example of that.
 * 
 * This example is provided specifically for comparison with the process
 * based creation, destruction and cleanup in the companion example.
 *
 * For all the nitty gritty on Pthreads and Pthread semaphores under Linux,
 * read the following man pages. pthread_create, pthread_attr_init,
 *   pthread_join, pthread_exit and sem_wait.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*
 * Define the number of threads and number of messages each prints.
 */
#define	NTHREADS	3
#define	NMESSAGES	5

/*
 * Define the function we will call to perform the thread work
 */
void * thread_activity(void *userdata);

/*
 * The main program creates the Pthreads and then simply waits for
 * them to complete.
 */
int
main()
{
	int i;
	pthread_t threadid[NTHREADS];
	pthread_attr_t pthread_attr;

	/*
	 * Set the Pthread attributes to the defaults, which are fine for
	 * most situations.
	 */
	pthread_attr_init(&pthread_attr);


	fprintf(stderr, "main: creating threads\n");

	/*
	 * Create the Pthreads.
	 */
	for (i = 0; i < NTHREADS; i++) {
		pthread_create(
			&threadid[i], /* location where thread identity is stored */
			&pthread_attr, /* setup attributes */
			thread_activity, /* function thread will perform */
		       	(void *)&i); /* thread data arg (here an integer name) */
		fprintf(stderr, "main: created child thread %d\n", i);
	}

	fprintf(stderr, "main: waiting for threads to terminate\n");

	/*
	 * Wait for each thread to terminate.
	 *
	 * Because we are passing the threadid value, the first return
	 * from pthread_join() will not occur until the thread we created
	 * first above actually completes, even though other threads may
	 * have completed first and be hanging around "in limbo" waiting
	 * to be cleaned up.
	 */
	for (i = 0; i < NTHREADS; i++) {
		pthread_join(threadid[i], NULL);
		fprintf(stderr, "main: joined child thread %d\n", i);
	}

	printf("\n");

	fprintf(stderr, "main: thread cleanup complete\n");

	return (0);
}

/*
 * This function is executed by each thread.
 *
 * The void * argument is the pointer to the thread data we passed as
 * the data argument above.  This design pattern is referred to as a
 * "user data argument", usually seen in a "callback".  Here we technically
 * do not have a "callback" because that would imply code calling back into
 * the *same* thread.
 */
void *
thread_activity(void *userdata)
{
	int *thread_id_number;
	int i;
	

	thread_id_number = (int *)userdata;

	/*
	 * Just loop the indicated number of times, placing strings on
	 * the terminal.
	 */
	for (i = 0; i < NMESSAGES; i++) {
		fprintf(stderr,"child: thread %d printing, iteration %d\n",
			*thread_id_number, i);
	}

	/*
	 * This call to pthread_exit() terminates this thread.
	 */
	pthread_exit(0);

	/*
	 * Never gets here, because the thread terminates
	 * just above us at the pthread_exit().
	 */
}

