#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


#define msleep(x) usleep(x * 1000) 

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    struct thread_data* args = (struct thread_data *) thread_param;
    
    msleep(args->wait_to_obtain_ms); // wait
    int ret = pthread_mutex_lock(args->mutex); // obtain mutex
    if(ret != 0) {
        perror("pthread_mutex_lock");
        args->thread_complete_success = false;
    } else {
        msleep(args->wait_to_release_ms); // wait
        pthread_mutex_unlock (args->mutex); //release mutex
        args->thread_complete_success = true;
    }

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

    struct thread_data *args = (struct thread_data*)malloc(sizeof(struct thread_data));
    if(args == NULL) {
        perror("thread_data malloc");
        return false;
    }

    args->mutex = mutex;
    args->wait_to_obtain_ms = wait_to_obtain_ms;
    args->wait_to_release_ms = wait_to_release_ms;
    args->thread_complete_success = false;
    
    int ret;
    ret = pthread_create (thread, NULL, threadfunc, (void*)args);
    if (ret != 0) {
        free(args);
        perror("pthread_create");
        return false;
    }

    return true;
}

