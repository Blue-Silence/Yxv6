#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pipe_left[2];
  int pipe_right[2];
  
  pipe(pipe_right);


  int isMaster=0;
  int child;
  int prime = 2;

  if((child = fork()))
  {
    close(pipe_right[0]);
    for(int j=3;j<=35;j++)
        write(pipe_right[1],&j,4);
    close(pipe_right[1]);
    while(wait(0)!=child)
        ;
    exit(0);
  } 
  else 
  {
    printf("prime %d\n",prime);
    pipe_left[0] = pipe_right[0];
    close(pipe_right[1]);
  }

  for(int recv;read(pipe_left[0],&recv,4);)  
  {
    if(recv%prime)
    {
        if(isMaster)
            write(pipe_right[1],&recv,4);
        else 
        {
            isMaster = 1;
            pipe(pipe_right);
            if((child=fork()))
                close(pipe_right[0]);
            else 
            {
                close(pipe_right[1]);
                close(pipe_left[0]);
                pipe_left[0] = pipe_right[0];
                isMaster = 0;

                prime = recv;
                printf("prime %d\n",prime);
            }
        }
    }
  }

  close(pipe_left[0]);
  if(isMaster)
  {
    close(pipe_right[1]);
    while(wait(0)!=child)
        ;
  }
    
//printf("Exiting!:PRIME:%d\n",prime);
  exit(0);
}
