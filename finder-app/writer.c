#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* args[])
{

    int write_fd;

    /* open the connection to syslog */
    openlog(NULL, 0, LOG_USER);
    if(argc != 3)
    {
        syslog(LOG_ERR, "Wrong Arguments : %d! Specify filepath in the first argument and string in the second",argc);
	closelog();
	return 1;
    }

    const char *write_str = args[2];
    const char *write_file = args[1];

    write_fd = open(write_file, O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);

    /* File to write could not be opened */
    if(write_fd == -1)
    {
        syslog(LOG_ERR, "Specified file:%s couldn't be opened!",write_file);
        closelog();
        return 1;
    }

    ssize_t write_bytes;
    write_bytes = write(write_fd, write_str, strlen(write_str));

    if(write_bytes == -1)
    {
        syslog(LOG_ERR, "Specified file:%s couldn't be written!",write_file);
        closelog();
	close(write_fd);
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", write_str, write_file);
    closelog();
    close(write_fd);

    return 0;
}

