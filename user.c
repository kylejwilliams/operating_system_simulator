#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct { int sec, nsec; } tick_t;

int main(int argc, char *argv[])
{
	/* get all of the shared memory variables */
	key_t key_clock = ftok("/tmp", 44);
	int shmid_clock = shmget(key_clock, sizeof(int), IPC_CREAT | 0666);
	tick_t *shm_clock = shmat(shmid_clock, NULL, 0);
	
	printf("I am child number %d and the timeout variable has a value of %ds:%dns\n", getpid(), (*shm_clock).sec, (*shm_clock).nsec);

	/* program exited successfully */
	exit(0);
}
