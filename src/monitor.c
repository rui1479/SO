#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#define PATH_MAX 4096

char* pids_folder_path; // Variável global para armazenar o caminho da pasta


void handle_stats_time(char *pid_list)
{
    char *token = strtok(pid_list, " ");
    int total_time = 0;

    while (token)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "/home/username/Desktop/SO/src/PIDS-folder/%s.txt", token);

        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
            perror("Erro ao abrir o arquivo");
            exit(EXIT_FAILURE);
        }

        int exec_time;
        // Pular as duas primeiras linhas
        fseek(file, 0, SEEK_SET);
        fscanf(file, "%*[^\n]\n%*[^\n]\n");
        // Ler a terceira linha para obter o tempo de execução
        fscanf(file, "Tempo de execução: %i milissegundos\n", &exec_time);

        total_time += exec_time;

        fclose(file);

        token = strtok(NULL, " ");
    }

    printf("Tempo total de execução: %i milissegundos\n", total_time);
}

void handle_stats_command(char *pid_list, char *command)
{
    char *token = strtok(pid_list, ",");
    int count = 0;

    while (token)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "/home/username/Desktop/SO/src/PIDS-folder/%s.txt", token);

        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
            perror("Erro ao abrir o arquivo");
            exit(EXIT_FAILURE);
        }

        char exec_command[256];
        // Pular a primeira linha
        fseek(file, 0, SEEK_SET);
        fscanf(file, "%*[^\n]\n");
        // Ler a segunda linha para obter o comando usado
        fscanf(file, "Comando usado: %s", exec_command);

        if (strcmp(exec_command, command) == 0)
        {
            count++;
        }

        fclose(file);

        token = strtok(NULL, ",");
    }

    printf("Número de vezes que o comando %s foi executado: %i\n", command, count);
}

void handle_stats_uniq(char *pid_list)
{
    char *token = strtok(pid_list, ",");
    char unique_commands[256][256];
    int unique_count = 0;

    while (token)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "/home/username/Desktop/SO/src/PIDS-folder/%s.txt", token);

        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
            perror("Erro ao abrir o arquivo");
            exit(EXIT_FAILURE);
        }

        char exec_command[256];
        // Pular a primeira linha
        fseek(file, 0, SEEK_SET);
        fscanf(file, "%*[^\n]\n");
        // Ler a segunda linha para obter o comando usado
        fscanf(file, "Comando usado: %s", exec_command);

        int is_unique = 1;
        for (int i = 0; i < unique_count; i++)
        {
            if (strcmp(unique_commands[i], exec_command) == 0)
            {
                is_unique = 0;
                break;
            }
        }

        if (is_unique)
        {
            strncpy(unique_commands[unique_count], exec_command, sizeof(exec_command));
            unique_count++;
        }

        fclose(file);
        token = strtok(NULL, ",");
    }

    printf("Número de comandos únicos executados: %i\n", unique_count);
    for (int i = 0; i < unique_count; i++)
    {
        printf("Comando único: %s\n", unique_commands[i]);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <caminho_da_pasta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pids_folder_path = argv[1]; // Armazenar o caminho da pasta

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