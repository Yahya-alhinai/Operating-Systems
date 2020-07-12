#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>

#define MAX_THREADS 100
#define MAX_queue_len 100
#define MAX_CE 100
#define INVALID -1
#define BUFF_SIZE 1024

pthread_mutex_t lock =  PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CV_IN = PTHREAD_COND_INITIALIZER;
pthread_cond_t CV_OUT = PTHREAD_COND_INITIALIZER;

FILE *fptr;
int nextIN = 0, nextOUT = 0, numItem = 0;
int cache_size;

/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGESSTION. FEEL FREE TO MODIFY AS NEEDED
*/

// structs:
typedef struct request_queue{
    int fd;
    void *request;
} request_t;

request_t boundedBuffer[MAX_queue_len];

typedef struct cache_entry{
    int len;
    char *request;
    char *content;
} cache_entry_t;

struct cache_entry cache[MAX_CE]; // the cache

cache_entry_t current_struct;

/* ************************ Dynamic Pool Code ***********************************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg){
    while(1){
        // Run at regular intervals
        // Increase / decrease dynamically based on your policy
    }
}
/**********************************************************************************/

/* ************************************ Cache Code ********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
    /// return the index if the request is present in the cache
    for(int n = 0; n < cache_size; n++){
        // return the index if the request is present in the cache
      	if(cache[n].request == request)
            return n;

    }

    return -1;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size, char *request){
    // It should add the request at an index according to the cache replacement policy
    // Make sure to allocate/free memeory when adding or replacing cache entries
    for(int n = 0; n < cache_size; n++){
        if(cache[n].len == 0){
            cache[n].content = malloc(memory_size);
            cache[n].content = memory;
            cache[n].request = request;
            cache[n].len = memory_size;
            break;
        }

    }
}

// clear the memory allocated to the cache
void deleteCache(){
    // De-allocate/free the cache memory
    for(int n=0; n < cache_size; n++){
        cache[n].len = NULL;
        free(cache[n].content);
        free(cache[n].request);
    }
}

// Function to initialize the cache
void initCache(){
    // Allocating memory and initializing the cache array
    for(int n=0; n < cache_size; n++){
        cache[n].request = malloc(sizeof(char *));
        cache[n].len = 0;
    }
}

// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
int readFromDisk(/*necessary arguments*/) {
    // Open and read the contents of file given the request
    return -1;
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char * mybuf){
    // Should return the content type based on the file type in the request
    // (See Section 5 in Project description for more details)
}

// This function returns the current time in milliseconds
int getCurrentTimeInMills(){
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    return curr_time.tv_usec;
}

/**********************************************************************************/
void logMe(int threadId, int reqNum, int fd, char *Request_string, int bytes_error, int time, char *cache_stat){
    fprintf(fptr, "[%d][%d][%d][%s][%d][%dms][%s]\n", threadId, reqNum, fd, Request_string, bytes_error, time, cache_stat);
    printf("[%d][%d][%d][%s][%d][%dms][%s]\n", threadId, reqNum, fd, Request_string, bytes_error, time, cache_stat);
}

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg){
    int fd;
    int queue_length = (int*) arg;

    printf("queue_length: %d\n", queue_length);
    char *filename;
    while (1) {

        // Accept client connection deleteCache (cache_size, array);
        fclose(fptr);
        fd = accept_connection();
        // Get request from the client
        if (fd >= 0){
            if (get_request(fd, filename) != 0){ //error
                /*After an error, you
                must NOT use a return_request or return_error function for that
                specific 'connection'.*/
            }

            // Add the request into the queue
            pthread_mutex_lock(&lock);
            while(numItem == queue_length) //queue full
                pthread_cond_wait(&CV_IN, &lock);

            boundedBuffer[nextIN].fd = fd;
            //boundedBuffer[nextIN].request = request;

            numItem++;
            nextIN = (nextIN + 1)%queue_length;

            //send signal(CV) to let worker knows that item is in queue
            pthread_cond_signal(&CV_OUT);
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg){
    int timeStart, timeEnd, cacheIndex, reqNum, fd, bytes_error;
    int queue_length = (int)arg;
    char *request;
    //########################WHERE DO I PUT THE LOCK???? ###########################//
    while (1){
        //************* Start recording time
        timeStart = getCurrentTimeInMills();

        //************* Get the request from the queue
        while (numItem == 0) //no item in queue
            pthread_cond_wait(&CV_OUT, &lock);

        fd = boundedBuffer[nextOUT].fd;
        //request = boundedBuffer[nextOUT].request;
        numItem--;
        nextOUT = (nextOUT + 1)%queue_length;
        //send signal(CV) to let dispatcher knows that there is a free space in queue
        pthread_cond_signal(&CV_IN);
        //************ Get the data from the disk or the cache
        if ((cacheIndex = getCacheIndex((char*)arg)) == -1)
        readFromDisk(/*necessary arguments*/);

        //************ Stop recording the time
        timeEnd = getCurrentTimeInMills();
        //************ Log the request into the file and terminal
        if (cacheIndex == -1)
            logMe(pthread_self(), reqNum, fd, request, bytes_error, timeEnd-timeStart, "MISS");
        else
            logMe(pthread_self(), reqNum, fd, request, bytes_error, timeEnd-timeStart, "HIT");

        // return the result
        //return_result (int fd, char *content_type, char *buf, int numbytes);
        //return_error(int fd, char *buf);
    }

  return NULL;
}

/**********************************************************************************/

int main(int argc, char **argv) {
    // Error check on number of arguments
    if(argc != 8){
        printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
        return -1;
    }

    int port, num_dispatcher, num_workers, queue_length, dynamic_flag;
    char *path;

    fptr = fopen ("webserver_log", "w");
    if (fptr == NULL){
        printf("File does not exists \n");
    }

    // Get the input args
    port = atoi(argv[1]);
    path = argv[2];
    num_dispatcher = atoi(argv[3]);
    num_workers = atoi(argv[4]);
    dynamic_flag = atoi(argv[5]);
    queue_length = atoi(argv[6]);
    cache_size = atoi(argv[7]);

    //TEST INPUTS
    printf("port:\t\t%d\n", port);
    printf("path:\t\t%s\n", path);
    printf("num_dispatcher:\t%d\n", num_dispatcher);
    printf("num_workers:\t%d\n", num_workers);
    printf("dynamic_flag:\t%d\n", dynamic_flag);
    printf("queue_length:\t%d\n", queue_length);
    printf("cache_size:\t%d\n\n", cache_size);

    // Perform error checks on the input arguments
    if (!(port > 1024 & port < 65536)){
        printf("ERROR: Port number has to be form 1025 to 65535\n");
        exit(1);
    }

    if (dynamic_flag == 1){
        //make thread dynamic
    }

    if (num_dispatcher > MAX_THREADS | num_workers > MAX_THREADS){
        printf("ERROR: Maximum thread is %d\n", MAX_THREADS);
        exit(-1);
    }

    if (queue_length > MAX_queue_len){
        printf ("ERROR: Maximum queue length is %d\n", MAX_queue_len);
        exit(-1);
    }

    if (cache_size > MAX_CE){
        printf ("ERROR: Maximum cache size is %d\n", MAX_CE);
        exit(-1);
    }

    if (chdir(path) != 0)
        perror(path);


    //TEST
    logMe(8,1,5,"/image/jpg/30.jpg", 17772, 200, "MISS");
    logMe(9,1,5,"/image/jpg/30.jpg", 17772, 3, "HIT");

    // Change the current working directory to server root directory

    // Start the server
    init (port);

    // initialize cache
    initCache();

    // Create dispatcher and worker threads
    pthread_t dispatchID[num_dispatcher], workerID[num_workers];

    for (int i = 0; i < num_dispatcher; i++)
        pthread_create(&dispatchID[i], NULL, dispatch, (int*) queue_length);

    for (int i = 0; i < num_workers; i++)
        pthread_create(&workerID[i], NULL, worker, (int*) queue_length);

    for (int i = 0; i < num_dispatcher; i++)
        pthread_join(dispatchID[i], NULL);

    for (int i = 0; i < num_workers; i++)
        pthread_join(dispatchID[i], NULL);

    // Clean up
    deleteCache();
    fclose(fptr);

    return 0;
}
