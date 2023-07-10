#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

    if (argc>1)
        sleep(atoi(argv[1]));
    else
        printf("Time to sleep required.\n");
    exit(0);
}