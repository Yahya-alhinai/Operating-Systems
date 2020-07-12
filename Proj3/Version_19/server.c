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
#include <unistd.h>
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

pthread_mutex_t lock_1=  PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_2 =  PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_3 =  PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CV_IN = PTHREAD_COND_INITIALIZER;
pthread_cond_t CV_OUT = PTHREAD_COND_INITIALIZER;

/* -------------------------------------global variables------------------------------------*/
FILE *fptr;
int nextIN = 0, nextOUT = 0;
int cache_size, queueSize, num_dispatcher, num_workers;
char *path;
/* --------------------------------END OF global variables----------------------------------*/


/* ------------------------------- Initialize Functions ------------------------------------*/
void thread_init();
int thread_add(pthread_t ti, int index);
void thread_kill();

void * dynamic_pool_size_update(void *arg);

void initCache();
void addIntoCache(char *request, char *memory , int memory_size);
int getCacheIndex(char *request);
void deleteCache(int idx);

void logMe(int threadId, int reqNum, int fd, char *Request_string, char* bytes_error, int time, char *cache_stat, int size);
int readFromDisk(char *request, char **content);
char* getContentType(char *request);

void * dispatch(void *arg);
void * worker(void *arg);
/* -----------------------------END OF Initialize Functions---------------------------------*/


/* -------------------------------------typedef struct--------------------------------------*/
typedef struct request_queue{
    int fd;
    char request[BUFF_SIZE];
} request_t;

typedef struct cache_entry{
    int len;
    char *request;
    char *content;
} cache_entry_t;

typedef struct thread_tracker{
    int exists;
    pthread_t pthread_id;
} thread_t;

request_t boundedBuffer[MAX_queue_len+1];
cache_entry_t cache[MAX_CE];
thread_t threadArray[2*MAX_THREADS];
int cacheStat[MAX_CE];
/* ---------------------------------END OF typedef struct-----------------------------------*/


/* -------------------------------------signal handler--------------------------------------*/
void SigHandler (int sig){
    printf("\n");
    thread_kill();
    deleteCache(-1);
    fclose(fptr);
    exit(1);
}
/* ----------------------------------END OF signal handler----------------------------------*/


/* -----------------------------------Thread Tracking--------------------------------------*/
//Initializes the thread struct list to set all the vlues of exist to 0, effectivelhy making it an empty list
void thread_init(){
    for(int i = 0; i < MAX_THREADS; i++)
        threadArray[i].exists = 0;
}

//add a thread to the thread list
int thread_add(pthread_t ti, int index){
    if (threadArray[index].exists == 0)
    {
        threadArray[index].pthread_id = ti;
        threadArray[index].exists = 1;
        fprintf(fptr, "Thread [%d] is created!\n", index);
        fprintf(stderr, "Thread [%d] is created!\n", index);
        return 0;
    }
    return -1;
}

//kill all threads in the list
void thread_kill(){
    for(int i = 0; i < 2*MAX_THREADS; i++){
        if (threadArray[i].exists == 1){
            pthread_cancel(threadArray[i].pthread_id);
            fprintf(fptr, "Thread [%d] is killed!\n", i);
            fprintf(stderr, "Thread [%d] is killed!\n", i);
      	    threadArray[i].exists = 0;
	    }
    }
}
/* --------------------------------END OF Thread Tracking---------------------------------*/


/* -------------------------------------Dynamic Pool--------------------------------------*/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg){
    while(1){
        // Run at regular intervals
        // Increase / decrease dynamically based on your policy
    }
}
/* ------------------------------END OF Dynamic Pool-------------------------------------*/


/* ----------------------------------Cache Code------------------------------------------*/
// Function to initialize the cache
void initCache(){
    // Allocating memory and initializing the cache array
    for(int i = 0; i < cache_size; i++){
        cache[i].content = NULL;
        cache[i].request = NULL;
        cache[i].len = -1;
        cacheStat[i] = 0;
    }
}

// Function to add the request and its file content into the cache
void addIntoCache(char *request, char *memory , int memory_size){
    // It should add the request at an index according to the cache replacement policy
    // Make sure to allocate/free memeory when adding or replacing cache entries
    fprintf(stderr, "INSIDE addIntoCache\n");
    for(int i = 0; i < cache_size; i++){
        if(cache[i].len == -1){
            //array[i] = (char *)malloc(stringsize+1);
            cache[i].content = malloc(memory_size);
            cache[i].request = malloc(strlen(request));
            strcpy(cache[i].content, memory);
            strcpy(cache[i].request, request);
            cache[i].len = memory_size;
            cacheStat[i] = 1;
            return;
        }
    }

    //LFU replacing an index
    int min = 0;
    for(int i = 0; i < cache_size -1; i++)
    {
        if (cacheStat[min] < cacheStat[i+1])
            min = i;
        else
            min = i + 1;
    }

    fprintf(stderr, "AFTER LOOP\n");
    fprintf (fptr, "\nRequest [%s] is being replaced with request [%s]\n", cache[min].request, request);
    deleteCache(min);
    cache[min].content = malloc(memory_size);
    cache[min].request = malloc(strlen(request));
    strcpy(cache[min].content, memory);
    strcpy(cache[min].request, request);
    cache[min].len = memory_size;
    cacheStat[min] = 1;

    return;
}

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
    /// return the index if the request is present in the cache
    for(int i = 0; i < cache_size; i++)
        if (cache[i].len != -1 && strcmp(cache[i].request, request) == 0)
        {
            cacheStat[i] = cacheStat[i] + 1;
            return i;
        }

    return -1;
}

// clear the memory allocated to the cache
void deleteCache(int idx){
    //De-allocate/free the cache memory
    //If index != -1, delete cache[index]
    //If index == -1, delete everything

    if (idx == -1){
        for(int i = 0; i < cache_size; i++){
            if(cache[i].len != -1 && cacheStat[i] != 0)
            {
                cache[i].len = -1;
                //free(cache[i].request);
                //free(cache[i].content);
                cacheStat[i] = 0;
            }
        }
    }
    else if(cache[idx].len != -1 && cacheStat[idx] != 0)
    {
        cache[idx].len = -1;
        free(cache[idx].request);
        free(cache[idx].content);
        cacheStat[idx] = 0;
    }
    else{
        fprintf(stderr, "ERROR: Replace index[%d] by deleteCache had not succeeded\n", idx);
    }
}
/* ------------------------------END OF Cache Code-------------------------------------*/


/* ------------------------------- Utilities ------------------------------------------*/
void logMe(int threadId, int reqNum, int fd, char *Request_string, char* bytes_error, int time, char *cache_stat, int size){
    if (size != -1){
        fprintf(fptr, "[%d][%d][%d][%s][%d][%dus][%s]\n", threadId, reqNum, fd, Request_string, size, time, cache_stat);
        printf("[%d][%d][%d][%s][%d][%dus][%s]\n", threadId, reqNum, fd, Request_string, size, time, cache_stat);
    } else{
        fprintf(fptr, "[%d][%d][%d][%s][%s][%dus][%s]\n", threadId, reqNum, fd, Request_string, bytes_error, time, cache_stat);
        printf("[%d][%d][%d][%s][%s][%dus][%s]\n", threadId, reqNum, fd, Request_string, bytes_error, time, cache_stat);
    }
}

// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
int readFromDisk(char *request, char **content) {
    // Open and read the contents of file given the request
    size_t size;
  	int fd;
    char dest[1024];
  	errno = 0;
  	printf("request: %s \n", request);
    strcpy(dest, path);
    strcat(dest, request);

  	fd = open(dest, O_RDONLY);
  	if (errno != 0)
        perror("ERROR");

    struct stat st;
    stat(dest, &st);
    size = st.st_size;
    fprintf(stderr, "Size is %ld\n", size);
    if (size != 0)
        *content = malloc(size);
    //fprintf(stderr, "Size of content: %ld\n", sizeof (content));
  	// fread reads one block of size "size" from the file located at request
    int bytes_error = read(fd, *content, size);
  	if (bytes_error != size) {
        printf("Read %d out of %ld\n", bytes_error, size);
        close(fd);
        return size;
    }
    //fprintf(stderr, "CONTENT: %s\n", *content);
  	close(fd);
    return size;
}


// Function to get the content type from the request
char* getContentType(char *request){
    // Should return the content type based on the file type in the request
    // (See Section 5 in Project description for more details)
	if(strstr(request, ".gif") != NULL)
        return "image/gif";
	else if(strstr(request, ".jpg") != NULL)
        return "image/jpeg";
	else if(strstr(request, ".html") != NULL || strstr(request, ".htm"))
        return "text/html";

    return "text/plain";
}

// This function returns the current time in milliseconds
long getCurrentTimeInMicro() {
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    return curr_time.tv_sec * 1000000 + curr_time.tv_usec;
}
/* -----------------------------END OF Utilities---------------------------------------*/


/* ------------------------------- Threads --------------------------------------------*/
// Function to receive the request from the client and add to the queue
void * dispatch(void *arg){
    pthread_detach(pthread_self());
    int index = *((int *) arg);

    //Needed because we don't want multiple thread have access to the log file at the sametime
    pthread_mutex_lock(&lock_3);
        thread_add(pthread_self(), index);
    pthread_mutex_unlock(&lock_3);

    int fd;
    char filename[BUFF_SIZE];
    while (1) {

        //test if queue is full
        pthread_mutex_lock(&lock_1);
        while((nextIN + 1)%queueSize == nextOUT)
            pthread_cond_wait(&CV_IN, &lock_1);

        // Accept client connection
        fd = accept_connection();

        if (fd < 0 || get_request(fd, filename) != 0){
            perror("connection has not been established");
            pthread_mutex_unlock(&lock_1);
            continue;
        }

        fprintf(stderr, "|||||||||||||||||||||||||||||||||||||DISPATCH-START-[%ld]||||||||||||\n", pthread_self());

        boundedBuffer[nextIN].fd = fd;
        strcpy (boundedBuffer[nextIN].request, filename);
        fprintf(stderr, "DISPATCH: FD = %d | request = %s | numItem = %d | save at index[%d]\n", boundedBuffer[nextIN].fd, boundedBuffer[nextIN].request,  ((nextOUT > nextIN)? queueSize-nextOUT+nextIN: nextIN-nextOUT), nextIN);

        nextIN = (nextIN + 1)%queueSize;

        fprintf(stderr, "|||||||||||||||||||||||||||||||||||||DISPATCH-END---[%ld]||||||||||||\n\n", pthread_self());
        pthread_mutex_unlock(&lock_1);
        pthread_cond_signal(&CV_OUT); //send signal(CV) to let worker knows that item is in queue
    }

    pthread_exit(NULL);
}


int test = 0;
// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg){
    pthread_detach(pthread_self());
    // while(!test){
    //     sleep(10);
    //     test = 1;
    // }

    int index = *((int *) arg);

    pthread_mutex_lock(&lock_3);//Needed because we don't want multiple thread have access to the log file at the sametime
        thread_add(pthread_self(), index);
    pthread_mutex_unlock(&lock_3);

    int timeStart, timeEnd, cacheIndex, fd, fileStat, size, reqNum = 0;
    char request[BUFF_SIZE];
    char *content;

    while (1){

        // Get the request from the queue
        pthread_mutex_lock(&lock_2);
        while(nextOUT == nextIN)
            pthread_cond_wait(&CV_OUT, &lock_2);

        fprintf(stderr, "************************************************************************************\n");
        fprintf(stderr, "|||||||||||||||||WORKER-START-[%ld]|||||||||||||||||||||||||||||||||||||\n", pthread_self());
        // Start recording time
        timeStart = getCurrentTimeInMicro();

        fd = boundedBuffer[nextOUT].fd;
        strcpy (request, boundedBuffer[nextOUT].request);
        fprintf(stderr, "WORKER: FD = %d | request = %s | numItem = %d | read index[%d]\n", fd, request, ((nextOUT > nextIN)? queueSize-nextOUT+nextIN: nextIN-nextOUT), nextOUT);

        nextOUT = (nextOUT + 1)%queueSize;

        //Get the data from the disk or the cache
        cacheIndex = getCacheIndex(request);
        if (cacheIndex == -1) //not in cache
        {
            size = readFromDisk(request, &content);
            if (size == 0)//not in disk
            {
                fileStat = -1;
            }
            else //found in disk, put it and cache
            {
                addIntoCache(request, content, size); //need to pass in the correct argument.
                fileStat = 0;
            }
        }
        else
        {
            fileStat = 0;
        }

        //Stop recording the time
        timeEnd = getCurrentTimeInMicro();

        // Log the request into the file and terminal
        if (fileStat != -1){  //in disk/cache
            // return the result
            logMe(index, ++reqNum, fd, request, "", timeEnd-timeStart, (cacheIndex == -1)? "MISS" : "HIT", (cacheIndex == -1)? size : cache[cacheIndex].len);
            test = return_result(fd, (cacheIndex == -1)?  getContentType(request) : getContentType(cache[cacheIndex].request), (cacheIndex == -1)? content : cache[cacheIndex].content, (cacheIndex == -1)? size : cache[cacheIndex].len);
            if (test != 0)
                exit(1);
        }
        else {
            logMe(index, ++reqNum, fd, request, "File not found", timeEnd-timeStart, (cacheIndex == -1)? "MISS" : "HIT", -1);
            return_error(fd, "File not found!\n");
        }

        fprintf(stderr, "|||||||||||||||||WORKER-END---[%ld]|||||||||||||||||||||||||||||||||||||\n", pthread_self());
        fprintf(stderr, "************************************************************************************\n\n");
        pthread_mutex_unlock(&lock_2);
	    pthread_cond_signal(&CV_IN);
        //return_result (int fd, char *content_type, char *buf, int numbytes);
    }

  pthread_exit(NULL);
}
/* ------------------------------- END OF Threads -------------------------------------*/


int main(int argc, char **argv) {

    signal(SIGINT, SigHandler);
    fptr = fopen ("webserver_log", "w");
    if (fptr == NULL){
        printf("Failed to open/create LOG file\n");
        exit(1);
    }

    // Error check on number of arguments
    if(argc != 8){
        printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
        return -1;
    }

    int port, queue_length, dynamic_flag;

    // Get the input args
    port = atoi(argv[1]);
    path = argv[2];
    num_dispatcher = atoi(argv[3]);
    num_workers = atoi(argv[4]);
    dynamic_flag = atoi(argv[5]);
    queueSize = atoi(argv[6]);
    cache_size = atoi(argv[7]);

    //TEST INPUTS
    printf("port:\t\t%d\n", port);
    printf("path:\t\t%s\n", path);
    printf("num_dispatcher:\t%d\n", num_dispatcher);
    printf("num_workers:\t%d\n", num_workers);
    printf("dynamic_flag:\t%d\n", dynamic_flag);
    printf("queue_length:\t%d\n", queueSize);
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

    if (queueSize > MAX_queue_len){
        printf ("ERROR: Maximum queue length is %d\n", MAX_queue_len);
        exit(-1);
    }
    queueSize++;

    if (cache_size > MAX_CE){
        printf ("ERROR: Maximum cache size should be within %d\n", MAX_CE);
        exit(-1);
    }

    DIR* dir = opendir(path);
    if (ENOENT == errno)
    {
        perror(path);
	    exit (-1);
    }
    free(dir);

    // Change the current working directory to server root directory
    //chdir(path);
    // Start the server
    init(port);
    // initialize cache
    initCache();
    // initialize threads thread array
    thread_init();
    // Create dispatcher and worker threads
    pthread_t dispatchID[num_dispatcher], workerID[num_workers];
    int threadSet[2*MAX_THREADS];

    for (int i = 0; i < num_dispatcher; i++)
    {
        threadSet[i] = i;
        pthread_create(&dispatchID[i], NULL, dispatch, &threadSet[i]);
    }

    for (int i = 0; i < num_workers; i++)
    {
        threadSet[i+num_dispatcher] = i+num_dispatcher;
        pthread_create(&workerID[i], NULL, worker, &threadSet[i+num_dispatcher]);
    }

    while(1);

    return 0;
}
