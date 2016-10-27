#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <wait.h>

typedef struct { int sec, nsec; } tick_t;                               /* represents a clock containing the time in seconds and nanoseconds */
typedef struct { long mtype; int sec; int nsec; int pid; } msgbuf_t;    /* represents a message to be sent / received */
typedef struct { long mtype; } prcmsgbuf_t;

volatile sig_atomic_t stop;
void end(int sig) { stop = 1; }

int main(int argc, char *argv[])
{
    /* variables to handle waiting inside the critical section */
    int sleep_time = 0;
    int start_time = 0;
    int end_time = 0;

    /* get all of the shared memory variables */
    key_t key_clock = ftok("/tmp", 86);
    int shmid_clock = shmget(key_clock, sizeof(int), 0666);
    tick_t *shm_clock = (tick_t *)shmat(shmid_clock, NULL, 0);

    key_t   key_msg = ftok("/tmp", 17);
    msgbuf_t msg;
    int     msqid = msgget(key_msg, 0666);

    key_msg = ftok("/tmp", 78);
    prcmsgbuf_t pmb;
    int pmbqid = msgget(key_msg, 0666);
    
    /* wait until it's our turn to enter the critical section */
    msgrcv(pmbqid, &pmb, sizeof(pmb), 1, 0);
    
    // <<< begin critical section >>>
			
		/* wait for a bit of time  while inside the critical section */
		sleep_time = rand() % 100000;
		start_time = ((*shm_clock).sec * 1000000000) + (*shm_clock).nsec;
		end_time = start_time + sleep_time;
		while ((((*shm_clock).sec * 1000000000) + (*shm_clock).nsec) < end_time) {  }

		/* send our message to oss */
		msg.nsec = (*shm_clock).nsec;
		msg.sec = (*shm_clock).sec;
		msg.pid = getpid();
		msg.mtype = 2;
		msgsnd(msqid, &msg, sizeof(msg), 0);

		// <<< end critical section >>>
	
    /* let the other processes know we're finished and terminate */
	  pmb.mtype = 1;
	  msgsnd(pmbqid, &pmb, sizeof(pmb), 0);
			
	  exit(EXIT_SUCCESS);
}
