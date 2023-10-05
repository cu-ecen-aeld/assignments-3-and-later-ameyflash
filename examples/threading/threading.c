#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("threading DEBUG: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    int ret;
    command_status_t command_status;
    // to extract for thread parameters
    struct thread_data* thread_func_args = (struct thread_data* ) thread_param;
    
    command_status.success = true;
    do
    {
	    // wait for thread->wait_to_obtain seconds
	    ret = usleep(thread_func_args->wait_to_obtain_ms*1000);
	    if(ret != 0)
	    {
		ERROR_LOG("threadfunc(): usleep obtain seconds failed\n");
		command_status.success = false;
		break;
	    }
	    
	    // obtain mutex and check for error obtaining mutex
	    ret = pthread_mutex_lock(thread_func_args->mutex);
	    if(ret != 0)
	    {
		ERROR_LOG("threadfunc(): mutex lock failed\n");
		command_status.success = false;
		break;
	    }
	    
	    // wait for thread->wait_to_release seconds
	    usleep(thread_func_args->wait_to_release_ms*1000);    
	    if(ret != 0)
	    {
		ERROR_LOG("threadfunc(): usleep release seconds failed\n");
		command_status.success = false;
		// don't break so that mutex unlock is performed 
		// even in case of usleep failure
	    }
	    
	    // release mutex and check for error releasing mutex
	    ret = pthread_mutex_unlock(thread_func_args->mutex);
	    if(ret != 0)
	    {
		ERROR_LOG("threadfunc(): mutex unlock failed\n");
		command_status.success = false;
		break;
	    }
    }while(0);
    
    if(command_status.success == true)
    {
    	thread_func_args->thread_complete_success = true;
    }
    else
    {
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
    int pt_ret;
    
    // malloc thread_data and check for success
    struct thread_data* start_thread_data = (struct thread_data*)malloc(sizeof(struct thread_data));
    if(start_thread_data == NULL)
    {
        ERROR_LOG("start_thread_obtaining_mutex(): malloc failed\n");
        return false;
    }

    start_thread_data->mutex = mutex;
    start_thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    start_thread_data->wait_to_release_ms = wait_to_release_ms;
    start_thread_data->thread_complete_success = false;

    pt_ret = pthread_create(thread, NULL, threadfunc, start_thread_data);
    if (pt_ret != 0)
    {
        ERROR_LOG("start_thread_obtaining_mutex(): pthread create failed\n");
        free(start_thread_data);
        return false;
    }

    return true;
}
