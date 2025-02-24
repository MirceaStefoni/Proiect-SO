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
#include <sys/wait.h>
#include <bits/waitflags.h>

int debug_mode = 0;
int contor_fisier_malecios = 0;

void printToFile(int fd, const char *str)      // Functie custom pentru Write
{
    if (write(fd, str, strlen(str)) == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void createSnapshot(const char *numeDirector, int snapshot_fd, const char* path_carantina)    // Functie care creaza un snapshot sau lanzeaz un proces in caz de fisier fara drepturi
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

        if (S_ISREG(fileStat.st_mode))    // verifica daca e fisier
        {
            if ((fileStat.st_mode & S_IRWXU) == 0 && (fileStat.st_mode & S_IRWXG) == 0 && (fileStat.st_mode & S_IRWXO) == 0) // verifica daca nu are drepturi
            {
                if(debug_mode)
                {
                    printf("fisierul %s nu are drepturi\n", path);
                }
                

                int pfd[2];
                FILE *stream;
                char string[50];

                int wstatus;
                pid_t cpid, w;

                if(pipe(pfd)<0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                cpid = fork();
                if (cpid == -1)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                if (cpid == 0) // Code Executed by child
                {
                    close(pfd[0]);  // Inchide capatul de citire

                    dup2(pfd[1],1); // Redirectare iesire standard spre pipe

                    execlp("./script_cautare.bash", "script_cautare.bash", path, NULL);  // Executa script verificare

                    perror("Eroare execlp");

                    exit(EXIT_FAILURE);
                }
                // Code Executed by parent

                close(pfd[1]); // Inchide capatul de scriere
                
                stream=fdopen(pfd[0],"r");  // Deschide stream-ul pentru citire

                fscanf(stream, "%s",string);

                close(pfd[0]);

                // string are rezultatul

                if (strcmp("RISK", string) == 0)  // Daca este Risk
                {
                    if (debug_mode)
                    {
                        printf("RISK\n");

                        printf("Path %s\n", path);

                        printf("Carantina %s\n", path_carantina);
                    }

                    contor_fisier_malecios = contor_fisier_malecios +1;  // Numara fisierele compromise

                    char path_carantina_fisier[1060];

                    snprintf(path_carantina_fisier, sizeof(path_carantina_fisier), "%s/%s", path_carantina, entry->d_name);  // Creaza outputul_carantina

                    if (debug_mode)
                    {
                        printf("Carantina iesire %s\n", path_carantina_fisier);
                    }

                    if (rename(path,path_carantina_fisier) != 0)   // Mutare
                    {
                        perror("rename");
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }

                w = wait(&wstatus);
                if (w == -1)
                {
                    perror("wait");
                    exit(EXIT_FAILURE);
                }
            }
             
            char file_msg[PATH_MAX];
            snprintf(file_msg, sizeof(file_msg), "(Fisier)    %s\n", path);
            printToFile(snapshot_fd, file_msg);

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
            createSnapshot(path, snapshot_fd, path_carantina);
        }
    }

    closedir(dir);
}

int isDirector(const char* path)   // Verifica daca este director
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

int snapshotExist(const char* nume_path)  // Verifica daca exista deja un snapshot cu acelasi nume
{

    struct stat stat_buffer;

    if(lstat(nume_path , &stat_buffer) == -1)
    {
        return 0;
    }

    return 1;
}

void genereazaNumePath(const char* nume,const char* folder_curent, const char* output, char* nume_path)   // Genereaza numele iesirii
{

    struct stat fileStat;

    if (lstat(folder_curent, &fileStat) < 0)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    char inode_str[20];
    snprintf(inode_str, sizeof(inode_str), "%ld", fileStat.st_ino);

    char snapshot_name[50];
    snprintf(snapshot_name, 50, "%s_%s",nume, inode_str);

    snprintf(nume_path, 100, "%s/%s",output, snapshot_name);

}

int openSnapshot(const char* nume_path)   // Deschide snapshot si il initializeaza
{
    int snapshot_fd = open(nume_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if (snapshot_fd == -1)
    {
        perror("open snapshot_fd");
        exit(-1);
    }
    return snapshot_fd;
}

int comparare_snapshot(const char *file1, const char *file2)  // Compara doua snapshoturi binar
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

        if (bytes_read1 != bytes_read2)  // Verifica daca exista diferente de dimensiune
        {
            if(debug_mode)
            {
                printf("Fișierele %s și %s au dimensiuni diferite.\n", file1, file2);
            }
            close(fd1);
            close(fd2);
            return 0;
        }

        if (memcmp(buf1, buf2, bytes_read1) != 0)  // Verifica daca sunt identice
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

    int pozitie_output;
    int pozitie_izolare;

    if (argc < 6 || argc > 15) // minim 1  -  max 10 Argumente
    {        
        perror("Utilizare incorecta\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++)  // Verifica daca exista duplicare de argumente
    {
        for (int j = i + 1; j < argc; j++)
        {
            if (strcmp(argv[i], argv[j]) == 0)
            {
                perror("Duplicare de argumente");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 1; i < argc; i++)  // Ia pozitia output-ului si a output-ului carantina
    {
        if(strcmp(argv[i],"-o") == 0)
        {
            pozitie_output = i;
        }

        if(strcmp(argv[i],"-x") == 0)
        {
            pozitie_izolare = i;
        }
    }

    int wstatus;
    pid_t cpid, w;

    for (int i = 1; i < argc; i++)
    {

        if (debug_mode)
        {
            printf("argv[%d] %s\n", i, argv[i]);
        }
       
        if(i == pozitie_output || i == pozitie_output + 1 || i == pozitie_izolare || i == pozitie_izolare + 1)  // sare peste output-uri si peste tag-uri
        {
            continue;
        }


        if (isDirector(argv[i]))
        {
            contor_fisier_malecios = 0;
            cpid = fork();
            if (cpid == -1)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (cpid == 0) // Code executed by child
            {
                if (debug_mode)
                {
                    printf("Procesul copil rulează...\n");
                }

                if (debug_mode)
                {
                    printf("argv[%d] %s\n", i, argv[i]);
                }

                char nume_path[100];
                genereazaNumePath("snapshot", argv[i], argv[pozitie_output + 1 ], nume_path);  // Genereaza nume snapshot

                if (debug_mode)
                {
                    printf("nume_path %s\n", nume_path);
                }

                if (snapshotExist(nume_path))  // Verifica daca exista
                { // DA exista

                    if (debug_mode)
                    {
                        printf("Snapshotul exista: \n");
                    }

                    char nume_path_aux[100];
                    genereazaNumePath("snapshot_aux", argv[i], argv[pozitie_output + 1], nume_path_aux);  // Genereaza nume snapshot auxiliar

                    int snapshot_fd_aux = openSnapshot(nume_path_aux);

                    createSnapshot(argv[i], snapshot_fd_aux, argv[pozitie_izolare + 1]); // Creaza snapshot

                    close(snapshot_fd_aux);

                    if (comparare_snapshot(nume_path, nume_path_aux))  // Compara continutul
                    { // Sunt identice
                        unlink(nume_path_aux);
                    }
                    else
                    { // Nu sunt identice

                        clonareSnapshot(nume_path, nume_path_aux);
                        unlink(nume_path_aux);
                    }
                }
                else
                { // NU exista

                    if (debug_mode)
                    {
                        printf("Snapshotul nu exista: \n");
                    }

                    int snapshot_fd = openSnapshot(nume_path);

                    createSnapshot(argv[i], snapshot_fd, argv[pozitie_izolare + 1]);  // Creaza Snpashot

                    close(snapshot_fd);
                }

                if (debug_mode)
                {
                    printf("Procesul copil s-a încheiat.\n");
                }

                exit(contor_fisier_malecios);
            }
            else // Code executed by parent
            {
                do
                {
                   
                    w = wait(&wstatus);
                    if (w == -1)
                    {
                        perror("wait");
                        exit(EXIT_FAILURE);
                    }

                    if (WIFEXITED(wstatus))
                    {
                        char mesaj[200];
                        snprintf(mesaj,sizeof(mesaj),"Procesul cu pid: %d. Directorul: %s, a avut %d fisiere periculoase\n",cpid, argv[i], WEXITSTATUS(wstatus));  // Genereza Mesaj
                        printToFile(1,mesaj);  // Scrie la stdo

                        if (debug_mode)
                        {
                            printf("exited, status=%d\n", WEXITSTATUS(wstatus));
                        }
                    }
                    else
                    {
                        if (WIFSIGNALED(wstatus))
                        {
                            if (debug_mode)
                            {
                                printf("killed by signal %d\n", WTERMSIG(wstatus));
                            }
                        }
                        else
                        {
                            if (WIFSTOPPED(wstatus))
                            {
                                if (debug_mode)
                                {
                                    printf("stopped by signal %d\n", WSTOPSIG(wstatus));
                                }
                            }
                            else
                            {
                                if (WIFCONTINUED(wstatus))
                                {
                                    if (debug_mode)
                                    {
                                        printf("continued\n");
                                    }
                                }
                            }
                        }
                    }

                } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
            }

        }
    }

    if (debug_mode)
    {
        printf("Success.\n");
    }

    return EXIT_SUCCESS;
}