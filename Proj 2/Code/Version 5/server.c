#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "comm.h"
#include "util.h"
#include <signal.h>

/* -----------Functions that implement server functionality -------------------------*/

/*
 * Returns the empty slot on success, or -1 on failure
 */
int find_empty_slot(USER * user_list) {
	// iterate through the user_list and check m_status to see if any slot is EMPTY
	// return the index of the empty slot
	for(int i=0;i<MAX_USER;i++) {
    	if(user_list[i].m_status == SLOT_EMPTY) {
			return i;
		}
	}
	return -1;
}

/*
 * list the existing users on the server shell
 */
 int list_users(int idx, USER * user_list)
 {
 	// iterate through the user list
 	// if you find any slot which is not empty, print that m_user_id
 	// if every slot is empty, print "<no users>""
 	// If the function is called by the server (that is, idx is -1), then printf the list
 	// If the function is called by the user, then send the list to the user using write() and passing m_fd_to_user
 	// return 0 on success
 	int i, flag = 0;
 	char buf[MAX_MSG] = {}, *s = NULL;

 	/* construct a list of user names */
 	s = buf;
 	strncpy(s, "---connecetd user list---\n", strlen("---connecetd user list---\n"));
 	s += strlen("---connecetd user list---\n");
 	for (i = 0; i < MAX_USER; i++) {
 		if (user_list[i].m_status == SLOT_EMPTY)
 			continue;
 		flag = 1;
 		strncpy(s, user_list[i].m_user_id, strlen(user_list[i].m_user_id));
 		s = s + strlen(user_list[i].m_user_id);
 		strncpy(s, "\n", 1);
 		s++;
 	}
 	if (flag == 0) {
 		strcpy(buf, "<no users>\n");
 	} else {
 		s--;
 		strncpy(s, "\0", 1);
 	}

 	if(idx < 0) {
 		printf(buf);
 		printf("\n");
 	} else {
 		/* write to the given pipe fd */
 		if (write(user_list[idx].m_fd_to_user, buf, strlen(buf) + 1) < 0)
 			perror("writing to server shell");
 	}

 	return 0;
 }


int add_user(int idx, USER * user_list, int pid, char * user_id, int pipe_to_child, int pipe_to_parent)
{
  //m_status = 1 means slot is available
  idx = find_empty_slot(user_list);
  user_list[idx].m_status = 0;
  user_list[idx].m_pid = pid;
  user_list[idx].m_fd_to_user = pipe_to_child;
  user_list[idx].m_fd_to_server = pipe_to_parent;
  strcpy (user_list[idx].m_user_id, user_id);
  printf ("User added: %s\n", user_list[idx].m_user_id);
  printf ("User PID: %d\n", user_list[idx].m_pid);
  return idx;
	// populate the user_list structure with the arguments passed to this function
	// return the index of user added
}

void kill_user(int idx, USER * user_list) {
	int pid = user_list[idx].m_pid;
	kill (pid, SIGSTOP);
  int status;
	waitpid(pid, &status, 0);
  if (WEXITSTATUS(status) != 0)
  {
    printf("child exited with error code=%d\n", WEXITSTATUS(status));
    exit(-1);
  }
	// kill a user (specified by idx) by using the systemcall kill()
	// then call waitpid on the user
}


/*
 * Perform cleanup actions after the used has been killed
 */
void cleanup_user(int idx, USER * user_list)
{

	user_list[idx].m_pid = -1;
	memset(user_list[idx].m_user_id, '\0', sizeof(user_list[idx].m_user_id[MAX_USER_ID]));
	close(user_list[idx].m_fd_to_user);
  close(user_list[idx].m_fd_to_server);
	user_list[idx].m_fd_to_user = -1;
  user_list[idx].m_fd_to_server = -1;
	user_list[idx].m_status = 1;
	// m_pid should be set back to -1
	// m_user_id should be set to zero, using memset()
	// close all the fd
	// set the value of all fd back to -1
	// set the status back to empty
}

/*
 * Kills the user and performs cleanup
 */
void kick_user(int idx, USER * user_list) {
	// should kill_user()
	kill_user(idx, user_list);
	cleanup_user(idx, user_list);
}

/*
 * broadcast message to all users
 */
int broadcast_msg(USER * user_list, char *buf, char *sender)
{
	for(int i=0;i<MAX_USER;i++)
	{
    if(user_list[i].m_status != SLOT_EMPTY)
		{
			if (strcmp (user_list[i].m_user_id, sender) != 0)
				{
					printf ("BROADCASTING Message: %s\n", buf);
					write(user_list[i].m_fd_to_user, buf, MAX_MSG);
				}
			//return i;
		}
	}

	//iterate over the user_list and if a slot is full, and the user is not the sender itself,
	//then send the message to that user
	//return zero on success
	return 0;
}

/*
 * Cleanup user chat boxes
 */
void cleanup_users(USER * user_list)
{
	// go over the user list and check for any empty slots
	// call cleanup user for each of those users.
}

/*
 * find user index for given user name
 */
 int find_user_index(USER * user_list, char * user_id)
 {
 	// go over the  user list to return the index of the user which matches the argument user_id
 	// return -1 if not found
 	int i, user_idx = -1;

 	if (user_id == NULL) {
 		fprintf(stderr, "NULL name passed.\n");
 		return user_idx;
 	}
 	for (i=0;i<MAX_USER;i++) {
 		if (user_list[i].m_status == SLOT_EMPTY)
 			continue;
 		if (strcmp(user_list[i].m_user_id, user_id) == 0) {
 			return i;
 		}
 	}

 	return -1;
 }

/*
 * given a command's input buffer, extract name
 */
int extract_name(char * buf, char * user_name)
{
	char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 2) {
        strcpy(user_name, tokens[1]);
        return 0;
    }

    return -1;
}

int extract_text(char *buf, char * text)
{
    char inbuf[MAX_MSG];
    char * tokens[16];
    char * s = NULL;
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 3) {
        //Find " "
        s = strchr(buf, ' ');
        s = strchr(s+1, ' ');

        strcpy(text, s+1);
        return 0;
    }

    return -1;
}

/*
 * send personal message
 */
void send_p2p_msg(int idx, USER * user_list, char *buf)
{
	int send_idx;
	char *user_name;
	extract_name(buf, user_name);
	send_idx = find_user_index(user_list, user_name);
	if (send_idx == -1){
		write(user_list[idx].m_fd_to_user, "User Not Found", MAX_MSG);	
	}
	else{
		write(user_list[idx].m_fd_to_user, buf, MAX_MSG);
	}
	// get the target user by name using extract_name() function
	// find the user id using find_user_index()
	// if user not found, write back to the original user "User not found", using the write()function on pipes.
	// if the user is found then write the message that the user wants to send to that user.
}

//takes in the filename of the file being executed, //send message to userand prints an error message stating the commands and their usage
void show_error_message(char *filename)
{
}


/*
 * Populates the user list initially
 */
void init_user_list(USER * user_list) {

	//iterate over the MAX_USER
	//memset() all m_user_id to zero
	//set all fd to -1
	//set the status to be EMPTY
	int i=0;
	for(i=0;i<MAX_USER;i++) {
		user_list[i].m_pid = -1;
		memset(user_list[i].m_user_id, '\0', MAX_USER_ID);
		user_list[i].m_fd_to_user = -1;
		user_list[i].m_fd_to_server = -1;
		user_list[i].m_status = SLOT_EMPTY;
	}
}

/* ---------------------End of the functions that implementServer functionality -----------------*/


/* ---------------------------------- These are added funcion ------------__---------------------*/
void runChild(int pipe_child_writing_to_user[2], int pipe_child_reading_from_user[2], int pipe_SERVER_reading_from_child[2], int pipe_SERVER_writing_to_child[2], char * buf, char * user_id)
{
	//Close ends that child dont use
	close (pipe_SERVER_writing_to_child[1]);
	close (pipe_SERVER_reading_from_child[0]);
    while(1)
    {
        //read from user and send to server
        if (read(pipe_child_reading_from_user[0], buf, MAX_MSG) != -1)
        {
            printf("Child reading message from User: %s\n", buf);
            write (pipe_SERVER_reading_from_child[1], buf, MAX_MSG);
            //clean the buffer
            memset(buf,'\0', MAX_MSG);
            print_prompt("admin");
        }

        //read from server and send to user
        if (read(pipe_SERVER_writing_to_child[0], buf, MAX_MSG) != -1)
        {
            printf("Child reading message from Server: %s\n", buf);
            write(pipe_child_writing_to_user[1], buf, MAX_MSG);
            //clean the buffer
            memset(buf,'\0', MAX_MSG);
            print_prompt("admin");
        }
        sleep(1);
    }
}

void execCommand (int cmd, USER * user_list)
{
	if (cmd == 0)//LIST
	{
		printf ("In execCommand()\n");
		list_users(-1, user_list);
	}
	else if (cmd == 1)//KICK
	{

	}
	else if (cmd == 2)//P2P
	{

	}
	else if (cmd == 3)//SEG
	{

	}
	else if (cmd == 4)//EXIT
	{
		//close all users and child appropriately and exit
		exit(1);
	}
	return;
}

/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[])
{
	int nbytes;
	setup_connection("YOUR_UNIQUE_ID"); // Specifies the connection point as argument.

	USER user_list[MAX_USER];
	init_user_list(user_list);   // Initialize user list

	char buf[MAX_MSG];
	fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK);
	print_prompt("admin");

    while(1)
    {
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/

		// Handling a new connection using get_connection
        int pipe_SERVER_reading_from_child[2];
        int pipe_SERVER_writing_to_child[2];
        char user_id[MAX_USER_ID];

        int pipe_child_writing_to_user[2];
        int pipe_child_reading_from_user[2];

      if (get_connection(user_id, pipe_child_writing_to_user, pipe_child_reading_from_user) == 0)
      {
          printf ("\nServer connected to user: %s\n", user_id);
          print_prompt("admin");

          // make non-blocking
          pipe(pipe_SERVER_reading_from_child);
          pipe(pipe_SERVER_writing_to_child);

          if (fcntl(pipe_SERVER_reading_from_child[0], F_SETFL, O_NONBLOCK) < 0 || fcntl(pipe_SERVER_writing_to_child[0], F_SETFL, O_NONBLOCK) < 0){
              perror("O_NONBLOCK error 1\n");
          }
          if (fcntl(pipe_child_reading_from_user[0], F_SETFL, O_NONBLOCK) < 0 || fcntl(pipe_child_writing_to_user[0], F_SETFL, O_NONBLOCK) < 0){
              perror("O_NONBLOCK error 2\n");
          }

          pid_t pid = fork();
          if (pid == 0)
          {
							// Child process: poli users and SERVER
              runChild(pipe_child_writing_to_user, pipe_child_reading_from_user,
                       pipe_SERVER_reading_from_child, pipe_SERVER_writing_to_child,
                       buf, user_id);
          }
          else //parent
          {
						//Close ends that server dont use
							close (pipe_SERVER_writing_to_child[0]);
							close (pipe_SERVER_reading_from_child[1]);
              printf ("PIPE id: %d\n", pipe_SERVER_writing_to_child[1]);
              //Server process: Add a new user information into an empty slot
              //NEED TO DETERMINE THE VALUE OF INDEX AND PIES FOR add_user
              add_user(0, user_list, pid, user_id, pipe_SERVER_writing_to_child[1], pipe_SERVER_reading_from_child[0]);
							print_prompt("admin");
              //list_users(-1, user_list);
          }
    		// Check max user and same user id
    		// poll child processes and handle user commands
    		// Poll stdin (input from the terminal) and handle admnistrative command

    		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/
      }//if
				/* ------------------------SERVER--------------------------------*/
        if(read(0, buf, MAX_MSG) != -1)
        {
					strtok(buf, "\n"); //remove \n from buf
					//LIST=0, KICK=1, EXIT=4, BROADCAST=5
					int cmd = get_command_type(buf);
					if (cmd == 5)//BROADCAST
					{
						broadcast_msg(user_list, buf, "SERVER"); //send message to all user
						memset(buf,'\0', MAX_MSG);
						/*
						printf ("Server sending message to child: %s\n", buf);
	        	write(pipe_SERVER_writing_to_child[1], buf, MAX_MSG);
	        	memset(buf,'\0', MAX_MSG);
						*/
					}
					else
						execCommand (cmd, user_list); //run the command
        	print_prompt("admin");
        }
        if(read(pipe_SERVER_reading_from_child[0], buf, MAX_MSG) != -1)
        {
          printf ("Server reading message from child: %s\n", buf);
        	memset(buf,'\0', MAX_MSG);
        	print_prompt("admin");
        }
        usleep(100);
   }//while
}//main

/* --------------------End of the main function ----------------------------------------*/