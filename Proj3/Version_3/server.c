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
FILE *fptr;
/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGESSTION. FEEL FREE TO MODIFY AS NEEDED
*/

// structs:
typedef struct request_queue {
   int fd;
   void *request;
} request_t;

typedef struct cache_entry {
    int len;
    char *request;
    char *content;
} cache_entry_t;


struct cache_entry cache[MAX_CE]; // the cache

cache_entry_t current_struct;

/* ************************ Dynamic Pool Code ***********************************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg) {
  while(1) {
    // Run at regular intervals
    // Increase / decrease dynamically based on your policy
  }
}
/**********************************************************************************/

/* ************************************ Cache Code ********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
  int n = 0;
  for(n; n < 100; n++){
	current_struct = cache[n];
	if(current_struct.request == request){
		return n;
		
	}
	
  }
  return -1;
  
  // return the index if the request is present in the cache
}

// Function to add the request and its file content into the cache
void addIntoCache(char *request, char *memory , int memory_size){
  int n = 0;
  for(n; n < 100; n++){
	current_struct = cache[n];
	if(current_struct.len == 0){
		current_struct.content = malloc(memory_size);
        	current_struct.content = memory;
		current_struct.request = request;
		current_struct.len = memory_size;
		break;
	}
	
  }
  // It should add the request at an index according to the cache replacement policy
  // Make sure to allocate/free memeory when adding or replacing cache entries
}

// clear the memory allocated to the cache
void deleteCache(){
	int n = 0;
    for(n; n < 100; n++){
	  current_struct = cache[n];
	  if(current_struct.len == 0){
		current_struct.len = 0;
		free(current_struct.content);
		free(current_struct.request);
	}
	
  }
  // De-allocate/free the cache memory
}

// Function to initialize the cache
void initCache(){
  int n = 0;
    for(n; n < 100; n++){
	  current_struct = cache[n];
	  if(current_struct.len == 0){
		current_struct.request = malloc(sizeof(char *));
		current_struct.len = 0;
	}
	
  }
  // Allocating memory and initializing the cache array
}


// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
int readFromDisk(/*Arguments here*/) {
  //file_p = fopen(filename, "r");
  //fseeks then ftell to find the size of the file
  //fscanf for reading from the file
  // Open and read the contents of file given the request
  return -1;
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char * mybuf) {
  // Should return the content type based on the file type in the request
  // (See Section 5 in Project description for more details)
}

// This function returns the current time in milliseconds
int getCurrentTimeInMills() {
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  return curr_time.tv_usec;
}

/**********************************************************************************/
void logMe(int threadId, int reqNum, int fd, char *Request_string, int bytes_error, int time, char *cache_stat)
{
     fprintf (fptr, "[%d][%d][%d][%s][%d][%dms][%s]\n", threadId, reqNum, fd, Request_string, bytes_error, time, cache_stat);
}

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg) {
     int fd;
     char *filename;
     while (1) {

     // Accept client connection
     fd = accept_connection();
     // Get request from the client
     if (fd >= 0)
          {
               if (get_request(fd, filename) != 0) //error
               {
                    /*After an error, you
                    must NOT use a return_request or return_error function for that
                    specific 'connection'.*/
               }
               // Add the request into the queue
          }
     }
     return NULL;
}

/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  int timeStart, timeEnd, cacheIndex, reqNum, fd, bytes_error;
  char *request;
   while (1) {

    // Start recording time
    timeStart = getCurrentTimeInMills();
    // Get the request from the queue

    // Get the data from the disk or the cache
    if ((cacheIndex = getCacheIndex((char*)arg)) == -1)
      readFromDisk(/*necessary arguments*/);
    // Stop recording the time
    timeEnd = getCurrentTimeInMills();
    // Log the request into the file and terminal
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
     int port, num_dispatcher, num_workers, cache_size, queue_length, dynamic_flag;
     char *path;
     fptr = fopen ("webserver_log", "w");
     if (fptr == NULL)
     {
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
     // Perform error checks on the input arguments

     if (port < 1025 | port > 65535)
     {
          perror ("Port number has to be form 1025 to 65535");
     }
     if (dynamic_flag == 1)
     {
          //make thread dynamic
     }
     logMe(8,1,5,"/image/jpg/30.jpg", 17772, 200, "MISS");
     logMe(9,1,5,"/image/jpg/30.jpg", 17772, 3, "HIT");
     //*********NEED TO CHECK OTHER ARGUMENTS**********//
     // Change the current working directory to server root directory

     // Start the server and initialize cache
     init (port);
     char **array;
     initCache(cache_size, array);
     // Create dispatcher and worker threads

     // Clean up
     deleteCache (cache_size, array);
     fclose(fptr);
     return 0;
}
