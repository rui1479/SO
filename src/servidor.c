#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256
#define FIFO_FILE "/tmp/myfifo"

typedef struct {
    int pid;
    char command[BUFFER_SIZE];
    int status;
} ProcessInfo;

void execute(ProcessInfo *info) {
    pid_t pid;
    int status;

    printf("Executando comando: %s\n", info->command);

    pid = fork();

    if (pid == 0) {
        execlp(info->command, info->command, (char *) NULL);
        exit(0);
    } else {
        wait(&status);
        info->pid = pid;
        info->status = WEXITSTATUS(status);

        printf("Comando executado. PID: %d, Status: %d\n", info->pid, info->status);

        int fd = open(FIFO_FILE, O_WRONLY);
        
        close(fd);
    }
}

int main() {
    int fd;
    ProcessInfo info;

    mkfifo(FIFO_FILE, 0666);

    printf("Servidor iniciado. Aguardando comandos...\n");

    while (1) {
        fd = open(FIFO_FILE, O_RDONLY);
        read(fd, &info, sizeof(ProcessInfo));
        close(fd);

        execute(&info);
    }

    return 0;
}
