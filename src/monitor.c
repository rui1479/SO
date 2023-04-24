// Compile with: gcc monitor.c -o monitor -lpthread
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>


#define PIPE_NAME "/tmp/tracer_pipe"
#define MAX_BUFFER_SIZE 256
#define MAX_COMMAND_SIZE 128

typedef struct ProcessInfo {
    int pid;
    char cmd[MAX_COMMAND_SIZE];
    long long start_time;
    long long end_time;
    struct ProcessInfo *next;
} ProcessInfo;

ProcessInfo *head = NULL;
pthread_mutex_t lock;

void add_process(ProcessInfo **head, int pid, const char *cmd, long long start_time) {
    ProcessInfo *new_process = (ProcessInfo *)malloc(sizeof(ProcessInfo));
    new_process->pid = pid;
    strncpy(new_process->cmd, cmd, MAX_COMMAND_SIZE - 1);
    new_process->cmd[MAX_COMMAND_SIZE - 1] = '\0';
    new_process->start_time = start_time;
    new_process->end_time = 0;
    new_process->next = *head;
    *head = new_process;
}

void update_process_end_time(ProcessInfo *head, int pid, long long end_time) {
    ProcessInfo *current = head;
    while (current != NULL) {
        if (current->pid == pid) {
            current->end_time = end_time;
            return;
        }
        current = current->next;
    }
}

void print_running_processes(ProcessInfo *head, int client_fd) {
    ProcessInfo *current = head;
    char buffer[MAX_BUFFER_SIZE];
    while (current != NULL) {
        if (current->end_time == 0) {
            long long duration = (long long)(time(NULL) - current->start_time) * 1000;
            snprintf(buffer, MAX_BUFFER_SIZE, "PID: %d, CMD: %s, EXECUTION_TIME: %lld ms\n", current->pid, current->cmd, duration);
            write(client_fd, buffer, strlen(buffer));
        }
        current = current->next;
    }
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    char buffer[MAX_BUFFER_SIZE];
    int num_read = read(client_fd, buffer, MAX_BUFFER_SIZE - 1);
    if (num_read > 0) {
        buffer[num_read] = '\0';

        if (strcmp(buffer, "status") == 0) {
            pthread_mutex_lock(&lock);
            print_running_processes(head, client_fd);
            pthread_mutex_unlock(&lock);
        } else {
            int pid;
            char cmd[MAX_COMMAND_SIZE];
            long long start_time;
            sscanf(buffer, "%d %[^|] %lld", &pid, cmd, &start_time);

            pthread_mutex_lock(&lock);
            add_process(&head, pid, cmd, start_time);
            pthread_mutex_unlock(&lock);
        }
    }

    close(client_fd);
    return NULL;
}

int main() {
    mkfifo(PIPE_NAME, 0666);

    int pipe_fd = open(PIPE_NAME, O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        perror("Erro ao abrir pipe");
        return 1;
    }

    pthread_mutex_init(&lock, NULL);

    while (1) {
        int client_fd = open(PIPE_NAME,O_WRONLY | O_NONBLOCK);
    if (client_fd != -1) {
        pthread_t thread;
        int *arg = malloc(sizeof(int));
        *arg = client_fd;
        pthread_create(&thread, NULL, handle_client, arg);
        pthread_detach(thread);
    } else {
        if (errno != EAGAIN) {
            perror("Erro ao abrir pipe do cliente");
        break;
        }
    }
        char buffer[MAX_BUFFER_SIZE];
    int num_read = read(pipe_fd, buffer, MAX_BUFFER_SIZE - 1);
    if (num_read > 0) {
        buffer[num_read] = '\0';

        int pid;
        char cmd[MAX_COMMAND_SIZE];
        long long start_time;
        sscanf(buffer, "%d %[^|] %lld", &pid, cmd, &start_time);

        pthread_mutex_lock(&lock);
        add_process(&head, pid, cmd, start_time);
        pthread_mutex_unlock(&lock);
    } else {
        if (errno != EAGAIN) {
            perror("Erro ao ler do pipe");
            break;
        }
    }

    sleep(1);
    }

    close(pipe_fd);
    unlink(PIPE_NAME);

    pthread_mutex_destroy(&lock);

    ProcessInfo *current = head;
    while (current != NULL) {
        printf("PID: %d, CMD: %s, START_TIME: %lld, END_TIME: %lld\n", current->pid, current->cmd, current->start_time, current->end_time);
        ProcessInfo *temp = current;
        current = current->next;
        free(temp);
    }

return 0;
}
