#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    pthread_mutex_t* mutex = thread_func_args->mutex;
    int wait_to_obtain_ms = thread_func_args->wait_to_obtain_ms;
    int wait_to_release_ms = thread_func_args->wait_to_release_ms;

    // wait
    if (usleep(wait_to_obtain_ms*1000) != 0){
        thread_func_args->thread_complete_success = false;
    }

    // obtain mutex
    if (pthread_mutex_lock(mutex) != 0){
        thread_func_args->thread_complete_success = false;
    }

    // wait
    if (usleep(wait_to_release_ms*1000) != 0){
        thread_func_args->thread_complete_success = false;
    }
    // release mutex
    if (pthread_mutex_unlock(mutex) != 0){
        thread_func_args->thread_complete_success = false;
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

    // setup thread attribute as joinable
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // allocate memory for thread_data
    struct thread_data* thread_param = (struct thread_data*)malloc(sizeof(struct thread_data));
    if (thread_param == (struct thread_data*)NULL){
        // memory allocation error
        return false;
    }

    // setup mutex and wait arguments
    thread_param->mutex = mutex;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;
    thread_param->thread_complete_success = true;

    // pass thread_data to created thread using threadfunc() as entry point
    if (pthread_create(thread, &attr, threadfunc, (void *)thread_param) == 0) {
        return true;
    }

    return false;
}

