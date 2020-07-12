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

pthread_mutex_t lock =  PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_1 =  PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_2 =  PTHREAD_MUTEX_INITIALIZER;
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

typedef struct thread_tracker{
    int exists;
    pthread_t pthread_id;
} thread_t;

thread_t threadArray[MAX_THREADS];

request_t boundedBuffer[MAX_queue_len];

typedef struct cache_entry{
    int len;
    char *request;
    char *content;
} cache_entry_t;

struct cache_entry cache[MAX_CE]; // the cache
/* ************************ Thread Tracking Code ***********************************/
//Initializes the thread struct list to set all the vlues of exist to 0, effectivelhy making it an empty list

void thread_init(){
    for(int i = 0; i < MAX_THREADS; i++) {
	    threadArray[i].exists = 0;
    }
}

//add a thread to the thread list
int thread_add(pthread_t ti){
    for(int i = 0; i < MAX_THREADS; i++) {
        if (threadArray[i].exists == 0){
            threadArray[i].pthread_id = ti;
	    threadArray[i].exists = 1;
	    return 0;
	}
    }
    return -1;
}

//remove thread from the thread list
int thread_remove(pthread_t ti){
    for(int i = 0; i < MAX_THREADS; i++) {
        if (threadArray[i].pthread_id == ti){
	    threadArray[i].exists = 0;
	    return 0;
	}
    }
    return -1;
}

//kill all threads in the list
void thread_kill(){
    for(int i = 0; i < MAX_THREADS; i++) {
        if (threadArray[i].exists == 1){
	    pthread_kill(threadArray[i].pthread_id, SIGTERM);
	    threadArray[i].exists = 0;
	}
    }
}



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
            cache[i].len = memory_size;
            return;
        }

    printf("ERROR: Cache has reached its maximum size\n");
}

void deleteQueue(int idx){
    boundedBuffer[idx].request = NULL;
    boundedBuffer[idx].fd = 0;
}


// clear the memory allocated to the cache
void deleteCache(int idx){
    //De-allocate/free the cache memory
    //If index != -1, delete cache[index]
    //If index == -1, delete everything
    if (idx == -1){
        for(int i = 0; i < cache_size; i++)
            if(cache[i].len != 0){
                cache[i].len = 0;
                free(cache[i].request);
                free(cache[i].content);
            }
    }
    else if(!(idx > 0 && idx < cache_size))
        printf("ERROR: The cache index [%d] is out of the range\n", idx);
    else if(cache[idx].len != 0){
            fprintf(stderr, "Deleteing %d\n", idx);
            cache[idx].len = 0;
            free(cache[idx].request);
            free(cache[idx].content);
    }
    else {
        printf("ERROR: Cannot delete Cache[%d]\n", idx);
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
int readFromDisk(char *request, char *content) {
	long size;
	FILE *req_ptr;
	errno = 0;
	printf("request: %s \n", request);

	//open file

	req_ptr = fopen(request, "rb");

	//error checking

	if (errno != 0){
	perror("ERROR!\n");
	return -1;
	}


	fseek(req_ptr, 0, SEEK_END);
	size = ftell(req_ptr);
	rewind(req_ptr);

	content = malloc(size+1);

	// fread reads one block of size "size" from the file located at request
	if (fread(content, size, 1, req_ptr) != 1) {

	printf("Cannot read from file");
	fclose(req_ptr);
	return -1;

	}

	fclose(req_ptr);
	return 0;
    // Open and read the contents of file given the request
    	
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *request){
	if(strstr(request, ".gif") != NULL){
	return "image/gif";
	}
	else if(strstr(request, ".jpg") != NULL){
	return "image/jpeg";
	}
	else if(strstr(request, ".html") != NULL || strstr(request, ".htm")){
	return "text/html";
	}
	else{
	return "text/plain";
	}

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
    //printf("[%d][%d][%d][%s][%d][%dms][%s]\n", threadId, reqNum, fd, Request_string, bytes_error, time, cache_stat);
}

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg){
    int fd;
    int queueSize = *((int *) arg);
    thread_add(pthread_self());
    char filename[BUFF_SIZE];
    char *request;
    while (1) {

        // Accept client connection
        fd = accept_connection();

        // Get request from the client
        pthread_mutex_lock(&lock);
        while(numItem >= queueSize) //queue full
            pthread_cond_wait(&CV_IN, &lock);

        // fprintf(stderr, "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
        // fprintf(stderr, "|||||||||||||||||||||||||||||||||START-DISPATCH||||||||||||||||||||||||||||||||||\n");

        if (fd >= 0){   // Get request from the client
            if (get_request(fd, filename) != 0){ //error
                /*After an error, you
                must NOT use a return_request or return_error function for that
                specific 'connection'.*/
                fprintf(stderr, "ERROR when get request\n");
            }

            // fprintf(stderr, "DISPATCH:\tFD is: %d\n", fd);
            // fprintf(stderr, "DISPATCH:\tfilename is: %s\n", filename);
            pthread_mutex_lock(&lock_2);
            numItem++;
            pthread_mutex_unlock(&lock_2);
            boundedBuffer[nextIN].fd = fd;
            boundedBuffer[nextIN].request = filename;
            // fprintf(stderr, "DISPATCH:\tFD = %d - request = %s - queue at index [%d]\n", boundedBuffer[nextIN].fd, (char *)boundedBuffer[nextIN].request, nextIN);

            nextIN = (nextIN + 1)%queueSize;
            //send signal(CV) to let worker knows that item is in queue
            //fprintf(stderr, "WORKER:\tnumItem --> %d\n", numItem);
            // fprintf(stderr, "|||||||||||||||||||||||||||||||||END-DISPATCH||||||||||||||||||||||||||||||||||||\n");
            // fprintf(stderr, "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n\n");
            pthread_mutex_unlock(&lock);
            pthread_cond_signal(&CV_OUT);
        }
    }
    thread_remove(pthread_self());
    return NULL;
}

/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg){
    int timeStart, timeEnd, cacheIndex, reqNum, fd, bytes_error;
    int queueSize = *((int *) arg);
    char *request;
    thread_add(pthread_self());
    //########################WHERE DO I PUT THE LOCK???? ###########################//
    while (1){
        //************* Get the request from the queue
        fprintf(stderr, "WORKER:\tnumItem --> %d\n", numItem);
        pthread_mutex_lock(&lock_1);
        while (numItem <= 0)
            pthread_cond_wait(&CV_OUT, &lock_1);

        fprintf(stderr, "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
        fprintf(stderr, "|||||||||||||||||||||||||||||||||START-WORKER|||||||||||||||||||||||||||||||||||\n");

        //************* Start recording time
        timeStart = getCurrentTimeInMills();

        fd = boundedBuffer[nextOUT].fd;
        request = boundedBuffer[nextOUT].request;
        fprintf (stderr, "WORKER:\tFD = %d, request = %s, were read at index %d\n", fd, request, nextOUT);
        printf("");
        nextOUT = (nextOUT + 1)%queueSize;
	char *content;
        //************ Get the data from the disk or the cache
        if (cacheIndex = getCacheIndex(request) == -1){
          readFromDisk(request, content);
	}
	printf("readsuccess");
        //************ Stop recording the time
        timeEnd = getCurrentTimeInMills();

        //************ Log the request into the file and terminal
        logMe(pthread_self(), reqNum, fd, request, bytes_error, timeEnd-timeStart, (cacheIndex == -1)? "HIT" : "MISS");

        // return the result
        //return_result (int fd, char *content_type, char *buf, int numbytes);

        // char buf [256];
        // while(1){
        //     if(read(0, buf, 256) != -1){ //SERVER terminal
        //         if (strcmp(buf, "-\n") == 0){
        //             memset(buf,'\0', 256);
        //             break;
        //         }
        //     }
        //
        // }
        sleep(10);
        //send signal(CV) to let dispatcher knows that there is a free space in queue
        pthread_mutex_lock(&lock_2);
        numItem--;
        pthread_mutex_unlock(&lock_2);
        fprintf (stderr,"return_error num: %d\n", return_error(fd, "TESTING ERROR"));
        fprintf(stderr, "|||||||||||||||||||||||||||||||||END-WORKER||||||||||||||||||||||||||||||||||||\n");
        fprintf(stderr, "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n\n");
        pthread_mutex_unlock(&lock_1);
        pthread_cond_signal(&CV_IN);
    }
  thread_remove(pthread_self());
  return NULL;
}

void SigHandler (int sig){
    deleteCache(-1);
    fclose(fptr);
    printf("\n");
    //NEED TO CLOSE THREADS
    thread_kill();
    exit(1);
}

/**********************************************************************************/
int main(int argc, char **argv) {
    signal(SIGINT, SigHandler);
    //fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK);
    // Error check on number of arguments
    if(argc != 8){
        printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
        return -1;
    }
    thread_init();

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
    {
        perror(path);
	exit (-1);
    }
    //TEST
    //logMe(8,1,5,"/image/jpg/30.jpg", 17772, 200, "MISS");
    //logMe(9,1,5,"/image/jpg/30.jpg", 17772, 3, "HIT");

    // Change the current working directory to server root directory
    chdir(path);
    // Start the server
    init(port);
    // initialize cache
    initCache();

    /////////////////////////_TEST_//////////////////////////
    /*
    nextIN = 0;
    nextOUT = 0;
    numItem = 0;

    int queueSize = queue_length;
    while(numItem == queueSize) //queue full
    {}
    boundedBuffer[nextIN].fd = 0;
    boundedBuffer[nextIN].request = "request 0";
    numItem++;
    nextIN = (nextIN + 1)%queueSize;

    while(numItem == queueSize) //queue full
    {}
    boundedBuffer[nextIN].fd = 1;
    boundedBuffer[nextIN].request = "request 1";
    numItem++;
    nextIN = (nextIN + 1)%queueSize;

    while(numItem == queueSize) //queue full
    {
    	printf ("Queue is full\n");
	break;
    }


    for (int y = 0; y < queueSize; y++)
    {
	fprintf (stderr, "Index: %d\nFD: %d\nRequest: %s\n\n", y, boundedBuffer[y].fd, (char*)boundedBuffer[y].request);
    } */
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
