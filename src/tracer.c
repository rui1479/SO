// Compile with: gcc tracer.c -o tracer
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#define PIPE_NAME "/tmp/tracer_pipe"
#define MAX_BUFFER_SIZE 256

long long current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void execute_program(const char *cmd_line) {
    char *cmd_args[MAX_BUFFER_SIZE];
    char *token;
    int i = 0;

    token = strtok(cmd_line, " ");
    while (token != NULL && i < MAX_BUFFER_SIZE - 1) {
        cmd_args[i++] = token;
        token = strtok(NULL, " ");
    }
    cmd_args[i] = NULL;

    execute_program_with_args(cmd_args[0], cmd_args);
}

void execute_program_with_args(const char *cmd, char *const argv[]) {
    int pid = fork();

    if (pid < 0) {
        perror("Erro ao criar processo");
        exit(1);
    }

    if (pid == 0) {
        if (execvp(cmd, argv) < 0) {
            perror("Erro ao executar programa");
            exit(1);
        }
    } else {
        printf("Executando programa com PID: %d\n", pid);

        long long start_time = current_time_ms();
        int pipe_fd = open(PIPE_NAME, O_WRONLY);
        if (pipe_fd < 0) {
            perror("Erro ao abrir pipe");
            exit(1);
        }

        char buffer[MAX_BUFFER_SIZE];
        snprintf(buffer, MAX_BUFFER_SIZE, "%d %s %lld", pid, cmd, start_time);
        write(pipe_fd, buffer, strlen(buffer));
        close(pipe_fd);

        int status;
        waitpid(pid, &status, 0);
        long long end_time = current_time_ms();

        pipe_fd = open(PIPE_NAME, O_WRONLY);
        if (pipe_fd < 0) {
            perror("Erro ao abrir pipe");
            exit(1);
        }

        snprintf(buffer, MAX_BUFFER_SIZE, "%d %lld", pid, end_time);
        write(pipe_fd, buffer, strlen(buffer));
        close(pipe_fd);

        printf("Programa finalizado. Tempo de execução: %lld ms\n", end_time - start_time);
    }
}

void query_running_programs() {
    int pipe_fd = open(PIPE_NAME, O_WRONLY);
    if (pipe_fd < 0) {
        perror("Erro ao abrir pipe");
        exit(1);
    }

    write(pipe_fd, "status", strlen("status"));
    close(pipe_fd);

    pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd < 0) {
        perror("Erro ao abrir pipe");
        exit(1);
    }

    char buffer[MAX_BUFFER_SIZE];
    while (1) {
        ssize_t num_read = read(pipe_fd, buffer, MAX_BUFFER_SIZE - 1);
        if (num_read > 0) {
            buffer[num_read] = '\0';
            printf("%s", buffer);
        } else if (num_read == 0) {
            break;
        } else {
            if (errno != EAGAIN) {
                perror("Erro ao ler do pipe");
                break;
            }
        }
    }

    close(pipe_fd);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <execute|status> [-u] [cmd] [args]\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "execute") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Uso: %s execute -u [cmd] [args]\n", argv[0]);
            exit(1);
        }
        if (strcmp(argv[2], "-u") == 0) {
            execute_program(argv[3]);
        } else {
            fprintf(stderr, "Opção inválida. Use -u para executar um programa.\n");
            exit(1);
        }
    } else if (strcmp(argv[1], "status") == 0) {
        query_running_programs();
    } else {
        fprintf(stderr, "Comando inválido. Use 'execute' para executar um programa ou 'status' para consultar o status.\n");
        exit(1);
    }

    return 0;
}
