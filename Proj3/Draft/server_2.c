#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/shm.h>
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

cache_entry *cache; //pointer to  access the cache with
int CACHE_KEY = 100; //Cache Key

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
  /* for(n = 0; n+sizeof(cache_entry); n == sizeof(cache_entry)*100){
	pointer = cache[n];
	if(pointer->request == request){
		return n/sizeof(cache_entry)
		
	}
	
  }
  return -1
  */
  // return the index if the request is present in the cache
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  // It should add the request at an index according to the cache replacement policy
  // Make sure to allocate/free memeory when adding or replacing cache entries
}

// clear the memory allocated to the cache
void deleteCache(){
  int shmdt ((void *) cache); //shared memory is deleted
  // De-allocate/free the cache memory
}

// Function to initialize the cache
void initCache(){
  
  
  shmid = shmget (CACHE_KEY, sizeof (buffer), 0600 | IPC_CREAT); 
  cache = (cache_entry *) shmat (shmid, 0, 0); // Cache is initialized and attached
  
  // Allocating memory and initializing the cache array
}


// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
int readFromDisk(/*necessary arguments*/) {
  // Open and read the contents of file given the request
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

   while (1) {

    // Start recording time

    // Get the request from the queue

    // Get the data from the disk or the cache

    // Stop recording the time

    // Log the request into the file and terminal

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
     int port, num_dispatcher, num_workers​, cache_size;

     // Get the input args
     port = atoi(*&argv[1]);
     num_dispatcher = atoi(*&argv[3]);
     num_workers​ = atoi(*&argv[4]);
     cache_size = atoi(*&argv[7]);
     // Perform error checks on the input arguments
     if (port < 1025 | port > 65535)
     {
          perror ("Port number has to be form 1025 to 65535");
     }
     //*********NEED TO CHECK OTHER ARGUMENTS**********//
     // Change the current working directory to server root directory

     // Start the server and initialize cache
     init (atoi(*&argv[1]));
     // Create dispatcher and worker threads

     // Clean up
     return 0;
}
