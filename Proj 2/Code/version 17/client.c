#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "comm.h"

#define POLLING_DELAY 10000

/* ------------------------------- Initialize Functions ------------------------------------*/
void print_prompt(char * name);

/* -----------------------------END OF Initialize Functions---------------------------------*/


/* -------------------------------------signal handler--------------------------------------*/
void SigHandler (int sig)
{
	if(kill(getpid(), SIGKILL) == -1)
	{
		perror("ERROR: ");
	}
}
/* ------------------------------END OF signal handler-------------------------------------*/


/* ----------------------------Main function for the client -------------------------------*/
void main(int argc, char * argv[]) {

	signal(SIGINT, SigHandler);

	int pipe_user_reading_from_child[2], pipe_user_writing_to_child[2];
	char buf[MAX_MSG];

	if(connect_to_server("WE_DESERVE_100_ON_THIS_PROJECT", argv[1], pipe_user_reading_from_child, pipe_user_writing_to_child) == -1) {
		exit(-1);
	}
	else
	{
		print_prompt(argv[1]);
	}

	/* --------------------- YOUR CODE STARTS HERE -----------------------------------*/

	// poll pipe retrieved and print it to sdiout
	if (fcntl(pipe_user_reading_from_child[0], F_SETFL, O_NONBLOCK) < 0 || fcntl(0, F_SETFL, O_NONBLOCK)){
		perror("O_NONBLOCK ERROR: \n");
	}
	close(pipe_user_reading_from_child[1]);
	close(pipe_user_writing_to_child[0]);
	// Poll stdin (input from the terminal) and send it to server (child process) via pipe
	//write (pipe_user_writing_to_child[1], (int *) getpid(), sizeof(int));

	int check;
	while (1)
	{
		/* --------------------- READING FROM USER --------------------------------*/
		if(read(0, buf, MAX_MSG) != -1)
		{
			strtok(buf, "\n");
			if (strcmp("\\exit", buf) == 0)
			{
				close(pipe_user_reading_from_child[0]);
				close(pipe_user_writing_to_child[1]);
				if(kill(getpid(), SIGKILL) == -1)
				{
					perror("ERROR: ");
				}
			}
			else if (strcmp("\\seg", buf) == 0)
			{
				char *n = NULL;
				*n = 1;
			}
			else
			{
				write(pipe_user_writing_to_child[1], buf, MAX_MSG);
				memset(buf,'\0', MAX_MSG);
				print_prompt(argv[1]);
			}
		}
		/* ------------------------- END OF READING FROM USER ----------------------------*/


		/* ------------------------- READING FROM SERVER ----------------------------*/
		check = read(pipe_user_reading_from_child[0], buf, MAX_MSG);
		if(check == MAX_MSG){
			printf("%s\n", buf);
			memset(buf,'\0', MAX_MSG);
			print_prompt(argv[1]);
		}
		else if (check == 0)
		{
			printf ("This user is signing out\n");
			close(pipe_user_reading_from_child[0]);
			close(pipe_user_writing_to_child[1]);
			if(kill(getpid(), SIGKILL) == -1)
			{
				perror("ERROR: ");
			}
		}
		/* ----------------------- END OF READING FROM SERVER -----------------------*/
		usleep(POLLING_DELAY);
	}

	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/
