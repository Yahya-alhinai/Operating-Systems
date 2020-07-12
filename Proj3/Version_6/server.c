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

#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <dirent.h>
#include <errno.h>

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
    for(int i = 0; i < cache_size; i++)
        if (cache[i].len != 0 && strcmp(cache[i].request, request) == 0)
            return i;

    return -1;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *request, char *memory , int memory_size){
    // It should add the request at an index according to the cache replacement policy
    // Make sure to allocate/free memeory when adding or replacing cache entries

    for(int i = 0; i < cache_size; i++)
        if(cache[i].len == 0){
            //array[i] = (char *)malloc(stringsize+1);
            cache[i].content = malloc(sizeof(memory));
            cache[i].request = malloc(sizeof(request));
            strcpy(cache[i].content, memory);
            strcpy(cache[i].request, request);
            cache[i].len = 1;
            break;
        }
}

// clear the memory allocated to the cache
void deleteCache(int idx){
    //De-allocate/free the cache memory
    //If index != -1, delete cache[index]
    //If index == -1, delete everything
    if (idx == -1){
        for(int i = 0; i < cache_size; i++){
            if(cache[i].len != 0){
                cache[i].len = 0;
                free(cache[i].request);
                free(cache[i].content);
            }
        }
    }
    else {
        if(cache[idx].len != 0){
            fprintf(stderr, "Deleteing %d\n", idx);
            cache[idx].len = 0;
            free(cache[idx].request);
            free(cache[idx].content);
        }
        else {
            printf("ERROR: Cannot delete Cache[%d]\n", idx);
        }
    }
}

// Function to initialize the cache
void initCache(){
    // Allocating memory and initializing the cache array
    for(int i = 0; i < cache_size; i++){
        cache[i].content = NULL;
        cache[i].request = NULL;
        cache[i].len = 0;
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
    int queueSize = *((int *) arg);

    printf("queue_length: %d\n", queueSize);
    char *filename;
    char *request;
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
            while(numItem == queueSize) //queue full
                pthread_cond_wait(&CV_IN, &lock);


            boundedBuffer[nextIN].fd = fd;
            boundedBuffer[nextIN].request = request;

            numItem++;
            nextIN = (nextIN + 1)%queueSize;

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
    int queueSize = *((int *) arg);
    char *request;
    //########################WHERE DO I PUT THE LOCK???? ###########################//
    while (1){

        //************* Get the request from the queue
        pthread_mutex_lock(&lock);
        while (numItem == 0) //no item in queue
            pthread_cond_wait(&CV_OUT, &lock);

        //************* Start recording time
        timeStart = getCurrentTimeInMills();

        fd = boundedBuffer[nextOUT].fd;
        request = boundedBuffer[nextOUT].request;
        numItem--;
        nextOUT = (nextOUT + 1)%queueSize;
        //send signal(CV) to let dispatcher knows that there is a free space in queue
        pthread_cond_signal(&CV_IN);
        //************ Get the data from the disk or the cache
        if (cacheIndex = getCacheIndex(request) == -1)
          readFromDisk(/*necessary arguments*/);

        //************ Stop recording the time
        timeEnd = getCurrentTimeInMills();
        ///
        //************ Log the request into the file and terminal

        logMe(pthread_self(), reqNum, fd, request, bytes_error, timeEnd-timeStart, (cacheIndex == -1)? "MISS" : "HIT");

        // return the result
        //return_result (int fd, char *content_type, char *buf, int numbytes);
        //return_error(int fd, char *buf);
    }

  return NULL;
}

void SigHandler (int sig){
    deleteCache(-1);
    fclose(fptr);
    printf("\n");

    exit(1);
}

/**********************************************************************************/
int main(int argc, char **argv) {
    signal(SIGINT, SigHandler);
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
        printf ("ERROR: Maximum cache size should be within %d\n", MAX_CE);
        exit(-1);
    }

    DIR* dir = opendir(path);
    if (ENOENT == errno)
        perror(path);

    //TEST
    logMe(8,1,5,"/image/jpg/30.jpg", 17772, 200, "MISS");
    logMe(9,1,5,"/image/jpg/30.jpg", 17772, 3, "HIT");

    // Change the current working directory to server root directory

    // Start the server
    init (port);
    // initialize cache
    initCache();

    /////////////////////////_TEST_//////////////////////////
    addIntoCache("HELLO", "HELLO" , sizeof("HELLO"));

    int k = getCacheIndex("HELLO");
    fprintf(stderr, "%s ---> K is %d\n", cache[k].content, k);

    addIntoCache("HELLO1", "HELLO1" , sizeof("HELLO1"));
    k = getCacheIndex("HELLO1");
    fprintf(stderr, "%s ---> K is %d\n", cache[k].content, k);

    printf ("K is: %d\n", getCacheIndex("HELLO"));
    printf ("K is: %d\n", getCacheIndex("HELLO2"));

    deleteCache(1);
    printf ("K is: %d\n", getCacheIndex("HELLO1"));
    //////////////////////////////////////////

    // Create dispatcher and worker threads
    pthread_t dispatchID[num_dispatcher], workerID[num_workers];

    for (int i = 0; i < num_dispatcher; i++)
        pthread_create(&dispatchID[i], NULL, dispatch, &queue_length);

    for (int i = 0; i < num_workers; i++)
        pthread_create(&workerID[i], NULL, worker, &queue_length);

    // for (int i = 0; i < num_dispatcher; i++)
    //     pthread_cancel(dispatchID[i]);
    //
    // for (int i = 0; i < num_workers; i++)
    //     pthread_cancel(workerID[i]);

    while(1);

    return 0;
}