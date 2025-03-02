#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT_ADDRESS 9000
#define STATIC_BUFFER_SIZE 1024
#define TMP_FILE_PATH "/var/tmp/aesdsocketdata"

volatile sig_atomic_t keep_running = 1;
int server_fd;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Linked list structure for thread management
struct thread_node {
    pthread_t thread;
    int client_fd;
    struct thread_node *next;
};

struct thread_node *thread_list = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

void aesdsocket_signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        keep_running = 0;
        if (server_fd > 0) {
            close(server_fd);
        }
    }
}

void append_to_tmp_file(const char *data, size_t size)
{
    int fd = open(TMP_FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
    {
        syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
    } else 
    {
        write(fd, data, size);
        close(fd);
    }
}

void send_file_data_to_client(int client_fd)
{
    int fd = open(TMP_FILE_PATH, O_RDONLY);
    if (fd == -1)
    {
        syslog(LOG_ERR, "Failed to open file for reading: %s", strerror(errno));
    } else
    {
        char buffer[STATIC_BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
        {
            send(client_fd, buffer, bytes_read, 0);
        }
        close(fd);
    }
}


void *handle_client(void *c_fd)
{
    int client_fd = *((int *)c_fd);
    free(c_fd);  // Free the allocated memory

    char buffer[STATIC_BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(client_fd, buffer, STATIC_BUFFER_SIZE - 1, 0)) > 0) {
        pthread_mutex_lock(&file_mutex);
        append_to_tmp_file(buffer, bytes_received);
        if (memchr(buffer, '\n', bytes_received)) {
            send_file_data_to_client(client_fd);
            pthread_mutex_unlock(&file_mutex);
            break;
        }
        pthread_mutex_unlock(&file_mutex);
    }

    //close(client_fd);
    return NULL;
}



void *timestamp_thread(void *arg) {
    while (keep_running) {
        // Sleep in 1-second chunks to allow quick exit
        for(int i = 0; i < 10 && keep_running; i++) {
            sleep(1);
        }
        
        if(!keep_running) break;
        
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", tm_info);
        
        pthread_mutex_lock(&file_mutex);
        append_to_tmp_file(timestamp, strlen(timestamp));
        pthread_mutex_unlock(&file_mutex);
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    int daemon_mode = 0;
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        daemon_mode = 1;
    }

    struct sockaddr_in sock_address;
    socklen_t addr_len = sizeof(sock_address);
    
    openlog("aesdsocket", LOG_PID, LOG_USER);
    signal(SIGINT, aesdsocket_signal_handler);
    signal(SIGTERM, aesdsocket_signal_handler);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        return -1;
    }

    sock_address.sin_family = AF_INET;
    sock_address.sin_addr.s_addr = INADDR_ANY;
    sock_address.sin_port = htons(PORT_ADDRESS);


    int opt = 1;
    //If address already in use
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        return -1;
    }

    if (bind(server_fd, (struct sockaddr *)&sock_address, sizeof(sock_address)) < 0)
    {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        return -1;
    }

    if (listen(server_fd, 15) < 0)
    {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        return -1;
    }

    if (daemon_mode)
    {
        pid_t pid = fork();
        if (pid < 0) 
        {
            syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
            return -1;
        }
        if (pid > 0) 
        {
            // Parent process
            exit(EXIT_SUCCESS);
        }
        // Child process becomes daemon
        setsid();
        chdir("/");
        // Close standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        // Open /dev/null
        int fd = open("/dev/null", O_RDWR);
        if (fd == -1)
        {
            syslog(LOG_ERR, "Failed to open /dev/null: %s", strerror(errno));
            return -1;
        }
    }

    pthread_t timestamp_thread_id;
    pthread_create(&timestamp_thread_id, NULL, timestamp_thread, NULL);

    while (keep_running)
    {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&sock_address, &addr_len);
    
        if (*client_fd < 0)
        {
            free(client_fd);  // Free the allocated memory
            if (!keep_running) break;
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(sock_address.sin_addr));

        int fd_value = *client_fd;
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_fd);

        pthread_mutex_lock(&list_mutex);
        struct thread_node *new_node = malloc(sizeof(struct thread_node));
        new_node->thread = thread;
        new_node->client_fd = fd_value;
        new_node->next = thread_list;
        thread_list = new_node;
        pthread_mutex_unlock(&list_mutex);
    }


    // Cleanup and join threads
    pthread_join(timestamp_thread_id, NULL);

    //pthread_mutex_lock(&list_mutex);
    while (thread_list != NULL) {
        struct thread_node *node = thread_list;
        pthread_join(node->thread, NULL);
        close(node->client_fd);
        thread_list = node->next;
        free(node);
    }
    //pthread_mutex_unlock(&list_mutex);

    remove(TMP_FILE_PATH);
    closelog();
    return 0;
}
