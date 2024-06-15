#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#define NOSYSTEM system

int tokenize(char *str,char *tokens[],int maxtoken);

void log_command(const char *command) 
{
    int fd=open("log.txt",O_WRONLY | O_APPEND | O_CREAT,0644);
    if (fd==-1) 
	{
        perror("Dosya açılırken hata!!!");
        return;
    }

    struct timeval timenow;
    gettimeofday(&timenow,NULL);
    time_t current_time=timenow.tv_sec;

    char timestamp[50];
    strftime(timestamp,sizeof(timestamp), "%m/%d/%Y %H:%M:%S",localtime(&current_time));     /*Güncel tarihi yazdır*/

    write(fd,timestamp,strlen(timestamp));
    write(fd, "\t", 1);
    write(fd,command,strlen(command));
    write(fd, "\n", 1);

    close(fd);
}

void execute_child(char *full_path,char *newargv[],int pipe_fd) 
{
    
    if (dup2(pipe_fd,STDOUT_FILENO)==-1) 
	{
        perror("Dup2 hatası!!!");
        exit(EXIT_FAILURE);
    }

    close(pipe_fd);

    if (execv(full_path,newargv)==-1) 
	{
        fprintf(stderr,"%s: command not found\n",newargv[0]);      /* Command not found hatasını göster */
        exit(EXIT_FAILURE);
    }

    perror("execv hatası!!!");
    exit(EXIT_FAILURE);
}


void execute_parent(int pipe_fd) 
{
    char buffer[4096];                   /* Child processten gelen çıktıyı okuyup dosyaya yaz */
    ssize_t bytes_read;

    int outfile_fd = open("outfile.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (outfile_fd == -1) {
        perror("Dosya açılırken hata!!!");
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = read(pipe_fd, buffer, sizeof(buffer))) > 0) 
	{
        write(outfile_fd, buffer, bytes_read);
        write(STDOUT_FILENO, buffer, bytes_read); 
    }

    close(outfile_fd);
    close(pipe_fd);
}



void execute_command(char *command) 
{
	if (strncmp(command,"exit",4)==0) 
	{
		log_command(command);               /*Exit komutuyla çık */
    	exit(0);
    }
    char *newargv[10];
    tokenize(command,newargv,10);

    char full_path[256];
    snprintf(full_path,sizeof(full_path), "/usr/bin/%s",newargv[0]);

    int pipe_fd[2];
    if (pipe(pipe_fd)==-1) 
	{
        perror("Pipe hatası!!!");
        exit(EXIT_FAILURE);
    }

    pid_t pid=fork();

    if (pid<0)        
	{
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } 
	else if (pid==0) 
	{
        close(pipe_fd[0]); 
        execute_child(full_path,newargv,pipe_fd[1]);   /* Pid değeri 0 olduğu için child process çalışır  */
        exit(0);
    } 
	else 
	{
        close(pipe_fd[1]); 
        execute_parent(pipe_fd[0]);        /* Pid değeri 0'dan büyük olduğu için parent process çalışır  */
        wait(NULL);
    }

    log_command(command);
}

int tokenize(char *str,char *tokens[],int maxtoken) 
{
    char *delim=" \t\n";
    char *saveptr;
    char *token=strtok_r(str,delim,&saveptr);    /*Karakter dizisi parçalara bölünür */

    tokens[0]=NULL;
    int i=0;
    while (token!=NULL&&i<maxtoken-1) 
	{
        tokens[i]=token;
        i++;
        token=strtok_r(NULL,delim,&saveptr);
    }
    tokens[i]=NULL;
    return i;
}

int main() 
{
    char c = '$';
    char command[256] = {0};


    while (1) 
	{
        write(STDOUT_FILENO, &c, 1);
        write(STDOUT_FILENO, " ", 1);
        int r = read(STDIN_FILENO,&command[0], 256);
    
        if (r > 1 && command[0]!='\n') 
		{
            command[r - 1] = '\0';
            execute_command(command);
        } 
		else 
		{
            write(STDOUT_FILENO, "\n", 1);    /*Komut girilmemişse boş satır yazdır*/
        }

        
    }
		

    return 0;
}

