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

void createSnapshot(const char *numeDirector, int snapshot_fd)
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
            createSnapshot(path, snapshot_fd);
        }
    }

    closedir(dir);
   // close(snapshot_fd);

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

int snapshotExist(const char* nume_path)
{

    struct stat stat_buffer;

    if(lstat(nume_path , &stat_buffer) == -1)
    {
        return 0;
    }

    return 1;
}

void genereazaNumePath(int pozitie,const char* nume, const char* output, char* nume_path)
{

    char snapshot_name[20];
    snprintf(snapshot_name, 20, "%s_%d",nume,pozitie-2);

    snprintf(nume_path, 100, "%s/%s",output, snapshot_name);

}

int openSnapshot(const char* nume_path)
{
    int snapshot_fd = open(nume_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if (snapshot_fd == -1)
    {
        perror("open snapshot_fd");
        exit(-1);
    }
    return snapshot_fd;
}

int comparare_snapshot(const char *file1, const char *file2)
{
    int BUF_SIZE = 1024;
    int fd1, fd2;
    ssize_t bytes_read1, bytes_read2;
    char buf1[BUF_SIZE], buf2[BUF_SIZE];

    // Deschide fiecare fișier în modul citire
    fd1 = open(file1, O_RDONLY);
    if (fd1 == -1)
    {
        perror("Eroare la deschiderea fișierului 1");
        return 0;
    }

    fd2 = open(file2, O_RDONLY);
    if (fd2 == -1)
    {
        perror("Eroare la deschiderea fișierului 2");
        close(fd1);
        return 0;
    }

    // Compară conținutul fișierelor
    do {
        bytes_read1 = read(fd1, buf1, BUF_SIZE);
        bytes_read2 = read(fd2, buf2, BUF_SIZE);

        if (bytes_read1 != bytes_read2)
        {
            if(debug_mode)
            {
                printf("Fișierele %s și %s au dimensiuni diferite.\n", file1, file2);
            }
            close(fd1);
            close(fd2);
            return 0;
        }

        if (memcmp(buf1, buf2, bytes_read1) != 0)
        {
            if(debug_mode)
            {
                printf("Conținutul fișierelor %s și %s este diferit.\n", file1, file2);
            }
            close(fd1);
            close(fd2);
            return 0;
        }
    } while (bytes_read1 > 0 && bytes_read2 > 0);

    if(debug_mode)
    {
        printf("Conținutul fișierelor %s și %s este identic.\n", file1, file2);
    }

    // Închide fișierele
    close(fd1);
    close(fd2);

    return 1;
}

int clonareSnapshot( const char *destination_file, const char *source_file)
{
    int BUF_SIZE = 1024;
    int fd_source, fd_dest;
    ssize_t bytes_read, bytes_written;
    char buffer[BUF_SIZE];

    // Deschide fișierul sursă în modul citire
    fd_source = open(source_file, O_RDONLY);
    if (fd_source == -1)
    {
        perror("Eroare la deschiderea fișierului sursă");
        return -1;
    }

    // Creează sau deschide fișierul destinație în modul scriere, cu drepturi de scriere și citire pentru utilizator
    fd_dest = open(destination_file, O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);
    if (fd_dest == -1)
    {
        perror("Eroare la deschiderea sau crearea fișierului destinație");
        close(fd_source);
        return -1;
    }

    // Copiază conținutul din fișierul sursă în fișierul destinație
    while ((bytes_read = read(fd_source, buffer, BUF_SIZE)) > 0)
    {
        bytes_written = write(fd_dest, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            perror("Eroare la scrierea datelor în fișierul destinație");
            close(fd_source);
            close(fd_dest);
            return -1;
        }
    }

    // Închide fișierele
    close(fd_source);
    close(fd_dest);

    return 0;
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
        if(debug_mode)   ////////////////////////////////////////////////////
            {
               printf("argv[%d] %s\n",i,argv[i]);
            }


        if(isDirector(argv[i]))
        {

            char nume_path[100];
            genereazaNumePath(i, "snapshot", argv[2], nume_path);


            if(debug_mode) //////////////////////////////////////////////////
            {
               printf("argv[2] %s\n",argv[2]);
               printf("nume_path %s\n",nume_path);
            }


            if(snapshotExist(nume_path))
            {  //DA

                if(debug_mode)
                {
                    printf("Snapshotul exista: \n");
                }
                char nume_path_aux[100];
                genereazaNumePath(i, "snapshot_aux", argv[2], nume_path_aux);

                int snapshot_fd_aux = openSnapshot(nume_path_aux);

                createSnapshot(argv[i], snapshot_fd_aux);  // se face close la fd in create

                close(snapshot_fd_aux);


                if(comparare_snapshot(nume_path, nume_path_aux))
                {   // Sunt identice
                    unlink(nume_path_aux);
                }
                else
                {   // Nu sunt identice
                  
                    clonareSnapshot(nume_path, nume_path_aux);
                    unlink(nume_path_aux);
                }
                
            }
            else
            { // NU

                if(debug_mode)
                {
                    printf("Snapshotul nu exista: \n");
                }

                int snapshot_fd = openSnapshot(nume_path);

                createSnapshot(argv[i], snapshot_fd);

                close(snapshot_fd);

            }

        }
    }


    if(debug_mode)
    {
        printf("Success.\n");
    }

    return EXIT_SUCCESS;
}

/*
#include <stdint.h>
       #include <stdio.h>
       #include <stdlib.h>
       #include <sys/wait.h>
       #include <unistd.h>

       int
       main(int argc, char *argv[])
       {
           int    wstatus;
           pid_t  cpid, w;

           cpid = fork();
           if (cpid == -1) {
               perror("fork");
               exit(EXIT_FAILURE);
           }

           if (cpid == 0) {            // Code executed by child 
               printf("Child PID is %jd\n", (intmax_t) getpid());
               if (argc == 1)
                   pause();                    // Wait for signals 
               _exit(atoi(argv[1]));

           } else {                    // Code executed by parent 
               do {
                   w = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
                   if (w == -1) {
                       perror("waitpid");
                       exit(EXIT_FAILURE);
                   }

                   if (WIFEXITED(wstatus)) {
                       printf("exited, status=%d\n", WEXITSTATUS(wstatus));
                   } else if (WIFSIGNALED(wstatus)) {
                       printf("killed by signal %d\n", WTERMSIG(wstatus));
                   } else if (WIFSTOPPED(wstatus)) {
                       printf("stopped by signal %d\n", WSTOPSIG(wstatus));
                   } else if (WIFCONTINUED(wstatus)) {
                       printf("continued\n");
                   }
               } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
               exit(EXIT_SUCCESS);
           }
       }
       */