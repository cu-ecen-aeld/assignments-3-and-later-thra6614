#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
     struct thread_data * threading = (struct thread_data * )thread_param;

    threading->thread_complete_success = false;
    usleep(threading->wait_get * 1000);
    int rc = pthread_mutex_lock(threading->mutex);
    if(rc != 0){
    	printf("pthread_mutex_lock failed with %d\n", rc);
    	return NULL;
    }
    usleep(threading->wait_release * 1000);
    rc = pthread_mutex_unlock(threading->mutex);
    if(rc != 0){
    	printf("pthread_mutex_unlock failed with %d\n", rc);
    	return NULL;
    }
    threading->thread_complete_success = true;
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
     struct thread_data * threading_struct = (struct thread_data *)malloc(sizeof(struct thread_data));
     
     if(threading_struct == NULL)
     {
     	printf("malloc failed\n");
     	return false;
     }
     memset(threading_struct, 0, sizeof(struct thread_data));
     threading_struct->mutex = mutex;
     threading_struct->wait_get = wait_to_obtain_ms;
     threading_struct->wait_release = wait_to_release_ms;
     threading_struct->thread_complete_success = false;
     
     int rc = pthread_create(thread, NULL, threadfunc, threading_struct);
	//bool output = threading_struct->thread_complete_success;
	//
     if(rc != 0){
    	printf("pthread_create failed with %d\n", rc);
    	// add free
    	free(threading_struct);
    	return false;
     }
     pthread_join(*thread, (void *)threading_struct);
     printf("%d\n", threading_struct->thread_complete_success);
     
     return false;
}

