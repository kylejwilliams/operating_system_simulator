#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    printf("I am process number %d and I have been spawned!\n", getpid());
}
