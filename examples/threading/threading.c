#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application

#define DEBUG 0

#if DEBUG
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#else
#define DEBUG_LOG(msg,...)
#endif


#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
  

    DEBUG_LOG("Starting the thread\n");
    struct thread_data *thread_args = (struct thread_data *) thread_param;


    long wait_to_obtain_ns = thread_args->wait_to_obtain_ms * 1000000;
    long wait_to_release_ns = thread_args->wait_to_release_ms * 1000000;

    const struct timespec obtain_nanotime = {0, wait_to_obtain_ns};
    const struct timespec release_nanotime = {0, wait_to_release_ns};

    struct timespec remain_nanotime = {0,0};

    remain_nanotime.tv_sec = 0;
    remain_nanotime.tv_nsec = 0;

    thread_args->tid = syscall(SYS_gettid);
    
    //usleep(thread_args->wait_to_obtain_ms * 1000);

    int status = clock_nanosleep(CLOCK_REALTIME, 0, &obtain_nanotime, &remain_nanotime);

    if(status != 0)
    {
        if(status == EINTR)
        {
            status = clock_nanosleep(CLOCK_REALTIME, 0, &remain_nanotime, &remain_nanotime);
            if(status != 0)
            {
                return thread_param;
            }
        }
        else
        {
            return thread_param;
        }

    }

    remain_nanotime.tv_sec = 0;
    remain_nanotime.tv_nsec = 0;    
    
    pthread_mutex_lock(thread_args->mutex);

   // usleep(thread_args->wait_to_release_ms * 1000);


    status = clock_nanosleep(CLOCK_REALTIME, 0, &release_nanotime, &remain_nanotime);

    if(status != 0)
    {
        if(status == EINTR)
        {
            status = clock_nanosleep(CLOCK_REALTIME, 0, &remain_nanotime, &remain_nanotime);
            if(status != 0)
            {
                return thread_param;
            }
        }
        else
        {
            return thread_param;
        }

    }

    pthread_mutex_unlock(thread_args->mutex);

    thread_args->thread_complete_success = true;
    
    DEBUG_LOG("Ending the thread \n");

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */


    DEBUG_LOG("dynamic allocation \n");
    size_t size = sizeof(struct thread_data);

    struct thread_data *thread_param = (struct thread_data*)malloc(size);

    //Dynamic allocation failed
    if(thread_param == NULL)
    {
	return false;
    }


    thread_param->mutex = mutex;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;
    thread_param->thread_complete_success = false;

    
    DEBUG_LOG("Starting to create the thread \n");
    int status;
    status = pthread_create(thread, NULL, threadfunc, thread_param);

    //thread creation failed
    if(status != 0)
    {
	ERROR_LOG("Thread creation failed \n");
        free(thread_param);
        return false;
    }

    return true;
}

