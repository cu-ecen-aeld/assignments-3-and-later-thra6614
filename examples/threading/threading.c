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
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    thread_func_args->thread_complete_success = false; 				//set output in case of failure
    usleep(thread_func_args->wait_get * 1000); 						//sleep for wait_get milliseconds
    int rc = pthread_mutex_lock(thread_func_args->mutex); 			//obtain mutex
    if(rc != 0){
    	ERROR_LOG("pthread_mutex_lock failed with %d\n", rc);
    	return thread_param;
    }
    usleep(thread_func_args->wait_release * 1000); 					//sleep wait_retreive milliseconds
    rc = pthread_mutex_unlock(thread_func_args->mutex); 			//release mutex
    if(rc != 0){
    	ERROR_LOG("pthread_mutex_unlock failed with %d\n", rc);
    	return thread_param;
    }
    thread_func_args->thread_complete_success = true; 				//successful implementation
    return thread_param; 											//return with success boolean true
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
     struct thread_data * threading_struct = (struct thread_data *)malloc(sizeof(struct thread_data));
     
     if(threading_struct == NULL)
     {
     	ERROR_LOG("malloc failed\n");
     	return false;
     }
     memset(threading_struct, 0, sizeof(struct thread_data)); 	//initialize everything as 0
     threading_struct->mutex = mutex; 							//fill struct
     threading_struct->wait_get = wait_to_obtain_ms;
     threading_struct->wait_release = wait_to_release_ms;
     threading_struct->thread_complete_success = false;
     
     int rc = pthread_create(thread, NULL, threadfunc, threading_struct); //create thread
     if(rc != 0){
    	ERROR_LOG("pthread_create failed with %d\n", rc);
    	free(threading_struct); //free if failing
    	return false;
     }
     
     return true; //successful thread creating
}

