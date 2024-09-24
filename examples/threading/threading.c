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

    // Initialize thread_complete_success to false
    thread_func_args->thread_complete_success = false;

    // Wait before attempting to obtain the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000); // Convert milliseconds to microseconds

    // Try to lock the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to obtain mutex\n");
        // Free thread data and return NULL to indicate an error occurred
        free(thread_param);
        return NULL;
    }

    DEBUG_LOG("Mutex obtained by thread\n");

    // Wait before releasing the mutex
    usleep(thread_func_args->wait_to_release_ms * 1000); // Convert milliseconds to microseconds

    // Unlock the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to release mutex\n");
        // Free thread data and return NULL to indicate an error occurred
        free(thread_param);
        return NULL;
    }

    DEBUG_LOG("Mutex released by thread\n");

    // Set thread_complete_success to true if no errors occurred
    thread_func_args->thread_complete_success = true;

    // Return the thread data structure to indicate successful completion
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
      // Allocate memory for the thread_data structure
    struct thread_data* data = (struct thread_data*)malloc(sizeof(struct thread_data));
    if (data == NULL) {
        ERROR_LOG("Failed to allocate memory for thread_data\n");
        return false;
    }

    // Initialize the thread_data structure
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false; // Initial state

    // Create the thread and pass threadfunc as the entry point, along with thread_data
    if (pthread_create(thread, NULL, threadfunc, data) != 0) {
        ERROR_LOG("Failed to create thread\n");
        free(data);
        return false;
    }

    return true;
}

