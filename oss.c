#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>

typedef struct { int sec, nsec; } tick_t;                               /* represents a clock containing the time in seconds and nanoseconds */
typedef struct { long mtype; int sec; int nsec; int pid; } msgbuf_t;    /* represents a message to be sent / received */
typedef struct { long mtype; } prcmsgbuf_t;

const int       NSECS_IN_SECS           = 1000000000;   /* 10^9 nanoseconds per second */
const int       MAX_SLAVES              = 100;          /* max number of slaves to spawn before terminating the program */
volatile        sig_atomic_t term_flag  = 0;            /* set this flag once the program has reached its timeout value */
volatile        sig_atomic_t int_flag   = 0;            /* set this flag when ctrl+c (interrupt) is thrown */

static void sig_handler(int signo)
{
    switch (signo)
    {
        case SIGINT:
            int_flag = 1;
            break;
        case SIGALRM:
            term_flag = 1;
            break;
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sig_handler);    /* init our signal handler */
    signal(SIGCHLD, SIG_IGN);        /* ignore the dead children so we can make new ones */

    /*initialize our variables */
    int         opt;															/* holds the current command line argument */
    const char  *short_opt			= "hs:l:t:";								/* valid args */
    struct      option long_opt[]	= { { "help", no_argument, NULL, 'h' } };	/* valid non-char args */
    int         num_slaves			= 5;                                        /* max number of slave processes spawned */
    char        *filename			= (char *) "log.txt";                       /* log file */
    int         timeout				= 20;                                       /* time before master should kill itself */
    int         i;																/* iterator */
    pid_t       slaves[15];                                             /* array of slaves by pid */
    for (i = 0; i < 15; i++) slaves[i] = 0;
    FILE *fp;

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
                exit(EXIT_SUCCESS);
                break;
            case 's':
                fprintf(stderr, "maximum allowed number of slaves is 15. setting num_slaves to 15\n");
                num_slaves = (atoi(optarg) > 15) ? 15 : atoi(optarg);
                break;
            case 'l':
                filename = optarg;
                break;
            case 't':
                timeout = atoi(optarg);
                break;
        }
    }

    /* clear out any old content of the file */
    fp = fopen(filename, "w");
    fclose(fp);


  	/* initialize some shared memory vars */
  	int key_clock = ftok("/tmp", 44);
  	int shmid_clock = shmget(key_clock, sizeof(tick_t), 0666 | IPC_CREAT);
  	tick_t *shm_clock = (tick_t *)shmat(shmid_clock, NULL, 0);
  	(*shm_clock).nsec = 0;
    (*shm_clock).sec = 0;

    key_t key_msg = ftok("/tmp", 97);
    msgbuf_t msgbuf;
    int msqid = msgget(key_msg, IPC_CREAT | 0666);
    msgbuf.mtype = 0;
    msgbuf.nsec = 0;
    msgbuf.sec = 0;
    msgbuf.pid = getpid();

    key_msg = ftok("/tmp", 11);
    prcmsgbuf_t pmb;
    int pmbqid = msgget(key_msg, IPC_CREAT | 0666);

    /* spawn some slaves */
    for (i = 0; i < num_slaves; i++)
    {
        if ((slaves[i] = fork()) < 0)
        {
            perror("fork error");
            shmdt(shm_clock);
            shmctl(shmid_clock, IPC_RMID, NULL);
            msgctl(msqid, IPC_RMID, NULL);
            msgctl(pmbqid, IPC_RMID, NULL);
            exit(EXIT_FAILURE);
        }
        else if (slaves[i] == 0)
            execl("./user", "user", NULL);
    }

    /* initialize our alarm signal handler */
    alarm(timeout);
    signal(SIGALRM, sig_handler);

    /* send the first message saying we're ready for messages */
    pmb.mtype = 1;
    if (msgsnd(pmbqid, &pmb, sizeof(pmb), 0) != 0)
    {
        perror("msgsnd");
        shmdt(shm_clock);
        shmctl(shmid_clock, IPC_RMID, NULL);
        msgctl(msqid, IPC_RMID, NULL);
        msgctl(pmbqid, IPC_RMID, NULL);
	  }

	  while ((*shm_clock).sec < 2 && num_slaves <= 100)
	  {
        /* interrupt was thrown; we need to exit */
        if (term_flag)
        {
            fprintf(stderr, "\nMaster timed out. Terminating now.\n");

            shmdt(shm_clock);
            shmctl(shmid_clock, IPC_RMID, NULL);
            msgctl(msqid, IPC_RMID, NULL);
            msgctl(pmbqid, IPC_RMID, NULL);

            exit(EXIT_SUCCESS);
        }
        else if (int_flag)
        {
            fprintf(stderr, "\nInterrupt thrown. Terminating now.\n");

            shmdt(shm_clock);
            shmctl(shmid_clock, IPC_RMID, NULL);
            msgctl(msqid, IPC_RMID, NULL);
            msgctl(pmbqid, IPC_RMID, NULL);

            exit(EXIT_SUCCESS);
        }
		
		    /* increment our process clock */
        (*shm_clock).nsec++;
		    if ((*shm_clock).nsec % NSECS_IN_SECS == 0)
		    {
		        (*shm_clock).sec++;
		        (*shm_clock).nsec = 0;
            //fprintf(stderr, "The current elapsed time is %ds:%dns\n", (*shm_clock).sec, (*shm_clock).nsec);
		    }

        if (msgrcv(msqid, &msgbuf, sizeof(msgbuf), 2, IPC_NOWAIT) != -1)
        {
            /* write our received message to file */
            fp = fopen(filename, "a");
            fprintf(fp, "Master: Child %d is terminating at my time %d:%d because it reached %d:%d\n", msgbuf.pid, (*shm_clock).sec, (*shm_clock).nsec, msgbuf.sec, msgbuf.nsec);
            fclose(fp);

            /* spawn another slave */
            num_slaves++;
            int escape = 0; /* used to signal to break out of the loop */
            for (i = 0; i < 15; i++)
            {
                if (escape) break;
                else if (slaves[i] == msgbuf.pid)
                {
                    escape = 1;
                    kill(slaves[i], SIGINT);

                    if ((slaves[i] = fork()) < 0)
                    {
                        perror("fork error");
                        shmdt(shm_clock);
                    	shmctl(shmid_clock, IPC_RMID, NULL);
                        msgctl(msqid, IPC_RMID, NULL);
                        msgctl(pmbqid, IPC_RMID, NULL);
					          }
                    else if (slaves[i] == 0)
                        execl("./user", "user", NULL);
                }
            }
        }
    }
	  fprintf(stderr, "Made it to the end successfully\n");

	  int status;	
	  for (i = 0; i < 15; i++)
	  {
		    if (kill(slaves[i], 0) == 0) /* process is still alive */
		    {
			    kill(slaves[i], SIGINT);
			    //wait(&status); THIS SHOULD BE HERE BUT IT CAUSES THE PROGRAM TO HANG
		    }
	  }

	  /* cleanup all of our shared memory */
	  shmdt(shm_clock);
	  shmctl(shmid_clock, IPC_RMID, NULL);
    msgctl(msqid, IPC_RMID, NULL);
    msgctl(pmbqid, IPC_RMID, NULL);

	  /* program exited successfully */
    exit(EXIT_SUCCESS);
}
