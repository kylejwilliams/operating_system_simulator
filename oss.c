#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[])
{
    int         opt;                                                        /* holds the current command line argument */
    const char  *short_opt = "hs:l:t:";                                     /* valid args */
    struct      option long_opt[] = { { "help", no_argument, NULL, 'h' } }; /* valid non-char args */
    int         num_slaves = 5;                                             /* max number of slave processes spawned */
    char        *filename = "log.txt";                                      /* log file */
    int         timeout = 20;                                               /* time before master should kill itself */
    int         i;                                                          /* iterator */

    while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
    {
        switch (opt)
        {
            case -1:
            case 0:
                break;
            case 'h':
                printf("Usage: %s [OPTIONS]\n", argv[0]);
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

    while (num_slaves > 0)
    {
        slave = wait(&status);
        printf("Child with PID %d has died\n", slave);
        --num_slaves;
    }
    return (0);
}
