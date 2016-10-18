#include <stdio.h>
#include <getopt.h>

int main(int argc, char *argv[])
{
    int         opt;                                                        /* holds the current command line argument */
    const char  *short_opt = "hs:l:t:";                                     /* valid args */
    struct      option long_opt[] = { { "help", no_argument, NULL, 'h' } }; /* valid non-char args */
    int         num_slaves = 5;                                             /* max number of slave processes spawned */
    char        *filename = "log.txt";                                      /* log file */
    int         timeout = 20;                                               /* time before master should kill itself */

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

    printf("num slaves: %d\nfilename: %s\ntimeout: %d\n", num_slaves, filename, timeout);
}
