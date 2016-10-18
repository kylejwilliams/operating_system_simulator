#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct { int sec, nsec; } tick_t; /* represents a clock containing the time in seconds and nanoseconds */
const int NSECS_IN_SECS = 1000000000; /* 10^9 nanoseconds per second */

int main(int argc, char *argv[])
{
    int         opt;															/* holds the current command line argument */
    const char  *short_opt			= "hs:l:t:";								/* valid args */
    struct      option long_opt[]	= { { "help", no_argument, NULL, 'h' } };	/* valid non-char args */
    int         num_slaves			= 5;                                        /* max number of slave processes spawned */
    char        *filename			= "log.txt";                                /* log file */
    int         timeout				= 20;                                       /* time before master should kill itself */
    int         i;																/* iterator */
	tick_t		clock				= { 0, 0 };									/* time elapsed since execution began */

    while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
    {
        switch (opt)
        {
            case -1:
				break;
			case ':':
			case '?':
				abort();
			case 'h':
                printf("Usage: %s [OPTIONS]\n", argv[0]);
				printf("\t-h, --help\tprints this help message and exits\n");
				printf("\t-s x\t\tmaximum number of slave processes to spawn\n");
				printf("\t-l filename\tname of the log file to output to\n");
				printf("\t-t z\t\tamount of time (in seconds) before the master process should terminate\n");
                return 0;
                break;
            case 's':
                num_slaves = atoi(optarg);
                break;
            case 'l':
                filename = optarg;
                break;
            case 't':
                timeout = atoi(optarg);
                break;
        }
    }
	
	/* initialize some shared memory vars */
	key_t key_clock = ftok("/tmp", 44);
	int shmid_clock = shmget(key_clock, sizeof(tick_t), IPC_CREAT | 0666);
	tick_t *shm_clock = shmat(shmid_clock, NULL, 0);
	*shm_clock = clock;

    /* spawn some slaves */
    pid_t   slaves[num_slaves]; /* array of slaves by pid */
    int     status;             /* returned message from spawned process */
    pid_t   slave;              /* represents a single slave process */

    for (i = 0; i < num_slaves; i++)
    {
        if ((slaves[i] = fork()) < 0)
        {
            perror("fork error");
            abort();
        }
        else if (slaves[i] == 0)
            execl("./user", "user", NULL);
    }
	
	while (((*shm_clock).sec < timeout) && num_slaves > 0)
	{
		(*shm_clock).nsec++;
		if ((*shm_clock).nsec % NSECS_IN_SECS == 0)
		{
			(*shm_clock).sec++;
			(*shm_clock).nsec = 0;
			printf("The current elapsed time is %ds:%dns\n", (*shm_clock).sec, (*shm_clock).nsec);
		}
	}

    while (num_slaves > 0)
    {
        slave = wait(&status);
        printf("Child with PID %d has died\n", slave);
        --num_slaves;
    }


	/* cleanup all of our shared memory */
	shmdt(shm_clock);
	shmctl(shmid_clock, IPC_RMID, NULL);

	/* program exited successfully */
    exit(0);
}
