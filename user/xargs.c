#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAXLINE 512
char buf[MAXLINE];

int
main(int argc, char *argv[]) {
    char* argvN[MAXARG] = {0};
    argvN[0] = argv[0];
    int i=0;
    for(;argv[i+1];i++)
        argvN[i] = argv[i+1];

    for(int j=0;j<MAXLINE;j++)
    {
        read(0,&buf[j],1);
        if(buf[j] == '\n')
        {
            buf[j] = '\0';
            j = -1;
            argvN[i] = buf;
            if(!fork())
                exec(argv[1],argvN);
            else 
                wait(0);
            continue;
        } 
        if(buf[j] == '\0')
            break;
    }
    exit(0);
}