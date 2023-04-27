#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {

    mkfifo("tracer_fifo", 0666);
    int pipe_fd = open("tracer_fifo", O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        perror("Falha ao abrir pipe");
        exit(EXIT_FAILURE);
    }

    if (argc < 3) {
        printf("Uso: %s execute -u <programa> [argumentos]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "execute") == 0 && strcmp(argv[2], "-u") == 0) {
        int pipe_fd = open("monitor_fifo", O_WRONLY);
        if (pipe_fd == -1) {
            perror("Falha ao abrir pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("Falha ao criar processo filho");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            close(pipe_fd);
            execvp(argv[3], &argv[3]);
            perror("Falha ao executar o programa");
            exit(EXIT_FAILURE);
        } else {
            struct timeval start_time, end_time;
            gettimeofday(&start_time, NULL);

            printf("PID do programa: %d\n", pid);
            write(pipe_fd, &pid, sizeof(pid_t));
            write(pipe_fd, argv[3], strlen(argv[3]) + 1);
            write(pipe_fd, &start_time, sizeof(struct timeval));

            int status;
            waitpid(pid, &status, 0);
            gettimeofday(&end_time, NULL);

            long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
            int elapsed_time_ms = elapsed_time / 10;
            printf("Programa executado com sucesso\n");
            printf("Tempo de execução: %i ms\n", elapsed_time_ms);

            // Criar a pasta "PIDS-folder" caso não exista
            mkdir("PIDS-folder", 0777);

            // Salvar informações em um arquivo com o nome do PID do programa na pasta "PIDS-folder"
            char filename[256];
            snprintf(filename, sizeof(filename), "PIDS-folder/%d.txt", pid);
            FILE *file = fopen(filename, "w");
            if (file == NULL) {
                perror("Erro ao abrir o arquivo");
                exit(EXIT_FAILURE);
            }
            fprintf(file, "PID do programa: %d\n", pid);
            fprintf(file, "Comando usado: %s", argv[3]);
            for (int i = 4; i < argc; i++) {
                fprintf(file, " %s", argv[i]);
            }
            fprintf(file, "\nTempo de execução: %i milissegundos\n", elapsed_time_ms);
            fclose(file);
            close(pipe_fd);
        }
    } else {
        printf("Opção inválida\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
