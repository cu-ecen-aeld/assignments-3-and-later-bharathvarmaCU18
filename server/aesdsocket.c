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

#define PORT_ADDRESS 9000
#define STATIC_BUFFER_SIZE 1024
#define TMP_FILE_PATH "/var/tmp/aesdsocketdata"



int server_fd, client_fd;

//Signal hanlder for SIGINT and SIGTERM
void aesdsocket_signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
    	if (client_fd > 0)
    	{
            close(client_fd);
    	}
    	if (server_fd > 0)
    	{
            close(server_fd);
    	}

    	remove(TMP_FILE_PATH);
    	closelog();
    	exit(EXIT_SUCCESS);
    }
}

//Appends received data to tmp file
void append_to_tmp_file(const char *data, size_t size)
{
    int fd = open(TMP_FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
        return;
    }
    
    ssize_t bytes_written = write(fd, data, size);
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Failed to write to file: %s", strerror(errno));
    }
    
    close(fd);
}

//Sends data from file to the client
void send_file_data_to_client(int client_fd)
{
    int fd = open(TMP_FILE_PATH, O_RDONLY);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open file for reading: %s", strerror(errno));
        return;
    }

    char buffer[STATIC_BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        ssize_t bytes_sent = send(client_fd, buffer, bytes_read, 0);
        if (bytes_sent == -1) {
            syslog(LOG_ERR, "Failed to send data: %s", strerror(errno));
            break;
        }
    }

    if (bytes_read == -1) {
        syslog(LOG_ERR, "Failed to read from file: %s", strerror(errno));
    }

    close(fd);
}


int main(int argc, char *argv[])
{

    int daemon_mode = 0;

    // Check for -d argument
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }
    
    struct sockaddr_in sock_address;
    socklen_t  addr_len = sizeof(sock_address);

    openlog("aesdsocket", LOG_PID, LOG_USER);

    //register signals
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


    // Fork if daemon mode is enabled
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

        // Redirect standard file descriptors to /dev/null
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

    }

    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&sock_address, &addr_len);
        if (client_fd < 0)
        {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        syslog(LOG_INFO, "accepted client connection from %s", inet_ntoa(sock_address.sin_addr));

        char buffer[STATIC_BUFFER_SIZE];
        ssize_t bytes_received;
        while ((bytes_received = recv(client_fd, buffer, STATIC_BUFFER_SIZE - 1, 0)) > 0)
        {
            char *newline = memchr(buffer, '\n', bytes_received);
            if (newline)
            {
                size_t line_length = newline - buffer + 1;
                append_to_tmp_file(buffer, line_length);
                send_file_data_to_client(client_fd);
                break;
            }
            else
            {
                append_to_tmp_file(buffer, bytes_received);
            }
        }

        if (bytes_received == -1) {
            syslog(LOG_ERR, "Receive failed: %s", strerror(errno));
        }

        bytes_received = 0;
        close(client_fd);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(sock_address.sin_addr));
    }


    return 0;
}
