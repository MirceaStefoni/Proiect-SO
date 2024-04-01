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

        if (stat(path, &fileStat) < 0)
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

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    {
        fprintf(stderr, "Folosire: %s nume_director\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int snapshot_fd = open("snapshot", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (snapshot_fd == -1) 
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    parcurgeDirector(argv[1], snapshot_fd);

    close(snapshot_fd);
    printf("Success.\n");

    return EXIT_SUCCESS;
}
