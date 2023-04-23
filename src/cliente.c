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

void execute(char *command, ProcessInfo *info) {
    int fd;

    strcpy(info->command, command);

    fd = open(FIFO_FILE, O_WRONLY);
    write(fd, info, sizeof(ProcessInfo));
    close(fd);

    fd = open(FIFO_FILE, O_RDONLY);
    read(fd, info, sizeof(ProcessInfo));
    close(fd);


    int fd_out = open("output.txt", O_RDONLY);
    char buffer[BUFFER_SIZE];
    int n;
    while ((n = read(fd_out, buffer, BUFFER_SIZE)) > 0) {
        write(STDOUT_FILENO, buffer, n);
    }
    close(fd_out);

}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: ./cliente -u [comando]\n");
        exit(0);
    }

    if (strcmp(argv[1], "-u") != 0) {
        printf("Opção inválida. Use -u para executar um comando.\n");
        exit(0);
    }

    ProcessInfo info;
    execute(argv[2], &info);

    return 0;
}
