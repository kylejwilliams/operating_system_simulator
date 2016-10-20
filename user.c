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

typedef struct { int sec, nsec; } tick_t;                               /* represents a clock containing the time in seconds and nanoseconds */
typedef struct { long mtype; int sec; int nsec; int pid; } msgbuf_t;    /* represents a message to be sent / received */
typedef struct { long mtype; } prcmsgbuf_t;

int main(int argc, char *argv[])
{
    int sleep_time = 0;
    int start_time = 0;
    int end_time = 0;
	/* get all of the shared memory variables */
	key_t key_clock = ftok("/tmp", 44);
	int shmid_clock = shmget(key_clock, sizeof(int), IPC_CREAT | 0666);
	tick_t *shm_clock = (tick_t *)shmat(shmid_clock, NULL, 0);

    key_t   key_msg = ftok("/tmp", 97);
    msgbuf_t msg;
    int     msqid = msgget(key_msg, 0666);

    key_msg = ftok("/tmp", 11);
    prcmsgbuf_t pmb;
    int pmbqid = msgget(key_msg, IPC_CREAT | 0666);

    msgrcv(pmbqid, &pmb, sizeof(pmb), 1, 0);

    // <<< critical section >>>

    /* let other processes know cs is occupied */
    //msgsnd(pmbqid, &pmb, sizeof(pmb), 2);

    /* wait for a bit of time  */
    sleep_time = rand() % 10;
    start_time = ((*shm_clock).sec * 10) + (*shm_clock).nsec;
    end_time = start_time + sleep_time;
    while ((((*shm_clock).sec * 10) + (*shm_clock).nsec) < end_time) {  }

    /* send our message to oss */
    msg.nsec = (*shm_clock).nsec;
    msg.sec = (*shm_clock).sec;
    msg.pid = getpid();
    msg.mtype = 2;
    if (msgsnd(msqid, &msg, sizeof(msg), 0) != 0)
        perror("msgsnd");

    // <<< end critical section >>>



    exit(EXIT_SUCCESS);
}

//void P(int i)
//{
//  message msg;
//
//  while (true)
//  {
//      receive(box, msg);
//      /* critical section */
//      send(box, msg);
//      /* remainder */
//  }
//}
