#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



int main(int argc, char** argv)
{
    if(argc != 2)
    {
    
        perror("Eroare: Lipsa argumente!\n");
        exit(-1);
    }

    struct stat stat_buff;

    if(lstat(argv[1],&stat_buff) == -1)
    {
        perror("lstat");
        exit(-2);
    }

    if(S_ISDIR(stat_buff.st_mode))
    {
        printf("Este director\n");

    }



    return 0;
}