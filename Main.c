#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/limits.h>

int debug_mode = 0;


void printToFile(int fd, const char *str)
{
    if (write(fd, str, strlen(str)) == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void parcurgeDirector(const char *numeDirector, int snapshot_fd)
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;

    if (!(dir = opendir(numeDirector)))
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", numeDirector, entry->d_name);
        
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (lstat(path, &fileStat) < 0)
        {
            perror("stat");
            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(fileStat.st_mode))
        {
            char dir_msg[PATH_MAX];
            snprintf(dir_msg, sizeof(dir_msg), "(Director)  %s\n", path);
            printToFile(snapshot_fd,dir_msg);
        }

        if (S_ISREG(fileStat.st_mode)) 
        {
            char file_msg[PATH_MAX];
            snprintf(file_msg, sizeof(file_msg), "(Fisier)    %s\n", path);
            printToFile(snapshot_fd,file_msg);
        }

        if (S_ISLNK(fileStat.st_mode)) 
        {
            char link_msg[PATH_MAX];
            snprintf(link_msg, sizeof(link_msg), "(Link)      %s\n", path);
            printToFile(snapshot_fd,link_msg);
        }

        printToFile(snapshot_fd, "Nume: ");                                 // Nume
        printToFile(snapshot_fd, entry->d_name);

        printToFile(snapshot_fd, "\nDimensiune: ");                         // Dimensiune
        char size_str[20];
        snprintf(size_str, sizeof(size_str), "%ld", fileStat.st_size);
        printToFile(snapshot_fd, size_str);

        printToFile(snapshot_fd, " bytes\nI-Node: ");                       // Inode
        char inode_str[20];
        snprintf(inode_str, sizeof(inode_str), "%ld", fileStat.st_ino);
        printToFile(snapshot_fd, inode_str);

        printToFile(snapshot_fd, "\nData ultimei modificări: ");            // Modified Date
        printToFile(snapshot_fd, ctime(&fileStat.st_mtime));
        printToFile(snapshot_fd, "-----------------------------\n");

        if (S_ISDIR(fileStat.st_mode))
        {
            parcurgeDirector(path, snapshot_fd);
        }
    }

    closedir(dir);
}

int isDirector(const char* path)
{
    struct stat fileStat;

    if(lstat(path, &fileStat) < 0)
    {
        printf("problema aici\n\n");
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(fileStat.st_mode))
    {
        return 1;
    }

    return 0;
}

int createSnapshot(const char* snapshot_name, const char* output_name)
{
    char nume_path[100];
    snprintf(nume_path, 100, "%s/%s",output_name, snapshot_name);

    if(debug_mode)
    {
        printf("nume path: %s\n",nume_path);
    }

    int snapshot_fd = open(nume_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (snapshot_fd == -1) 
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return snapshot_fd;
}

int main(int argc, char *argv[]) 
{
    
    printf("DebugMode 1 sau 0: ");
    scanf("%d",&debug_mode);

    if(debug_mode)
    {
        printf("argc = %d\n",argc);
    }

    if (argc < 4 || argc > 13) // minim 1  -  max 10
    {        
        perror("Utilizare incorecta:  ./p -o output_dir arg1 arg2 arg3 ... arg10");
        exit(EXIT_FAILURE);
    }

    for(int i=3;i<argc;i++)
    {   
        if(debug_mode)
            {
               printf("argv[%d] %s\n",i,argv[i]);
            }


        if(isDirector(argv[i]))
        {
            char snapshot_name[20];
            snprintf(snapshot_name, 20, "snapshot_%d", i-2);

            if(debug_mode)
            {
               printf("argv[2] %s\n",argv[2]);
            }
            int snapshot_fd = createSnapshot(snapshot_name,argv[2]);

            if(debug_mode)
            {
                printf("snapshot_name  %s\n",snapshot_name);
            }   

            parcurgeDirector(argv[i], snapshot_fd);

            close(snapshot_fd);
        }
    }


    ///// functia de parcurgere 

    if(debug_mode)
    {
        printf("Success.\n");
    }

    return EXIT_SUCCESS;
}
