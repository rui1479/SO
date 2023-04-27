#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

int main() {
    printf("Servidor iniciado\n");

    mkfifo("monitor_fifo", 0666);
    int pipe_fd = open("monitor_fifo", O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        perror("Falha ao abrir pipe");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pid_t pid;
        char program_name[256];
        struct timeval timestamp;

        ssize_t pid_read = read(pipe_fd, &pid, sizeof(pid_t));
        ssize_t program_name_read = read(pipe_fd, program_name, sizeof(program_name));
        ssize_t timestamp_read = read(pipe_fd, &timestamp, sizeof(struct timeval));

        if (pid_read > 0 && program_name_read > 0 && timestamp_read > 0) {
            printf("Programa \"%s\" com PID %d iniciou em %ld.%06ld\n", program_name, pid, (long)timestamp.tv_sec, (long)timestamp.tv_usec);
        } else if (pid_read == -1 || program_name_read == -1 || timestamp_read == -1) {
            if (errno != EAGAIN) {
                perror("Erro ao ler dados do pipe");
                exit(EXIT_FAILURE);
            }
        }

        usleep(100000);  // Dorme por 100 ms para evitar consumo excessivo de CPU
    }

    close(pipe_fd);
    unlink("monitor_fifo");
    return 0;
}
