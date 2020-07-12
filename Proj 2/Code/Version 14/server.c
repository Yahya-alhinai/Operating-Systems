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

char* concat(const char *str1, const char *str2)
{
	char *string = malloc(strlen(str1) + strlen(str2) + 3);
	strcpy(string, str1);
	strcat(string, ": ");
	strcat(string, str2);

	return string;
}

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
 	strncpy(s, "---connected user list---\n", strlen("---connected user list---\n"));
 	s += strlen("---connected user list---\n");
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
 		printf("%s", buf);
 		printf("\n");
 	} else {
 		/* write to the given pipe fd */
 		if (write(user_list[idx].m_fd_to_user, buf, MAX_MSG) < 0)
 			perror("writing to server shell");
 	}

 	return 0;
}


void kill_user(int idx, USER * user_list) {
	int pid = user_list[idx].m_pid; //get pid of the child
	kill (pid, SIGKILL);//kill the child
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
	//reset this user
	user_list[idx].m_pid = -1;
	memset(user_list[idx].m_user_id, '\0', sizeof(user_list[idx].m_user_id[MAX_USER_ID]));
	close(user_list[idx].m_fd_to_user);
	close(user_list[idx].m_fd_to_server);
	user_list[idx].m_fd_to_user = -1;
	user_list[idx].m_fd_to_server = -1;
	user_list[idx].m_status = SLOT_EMPTY;

	// m_pid should be set back to -1
	// m_user_id should be set to zero, using memset()
	// close all the fd
	// set the value of all fd back to -1
	// set the status back to empty
}


int add_user(USER * user_list, pid_t pid, char * user_id, int pipe_to_child, int pipe_to_parent, int Dop)
{
	int idx = find_empty_slot(user_list);
	if (Dop == 2)//full
	{
		return -1;
	}
	else if (Dop == 1)//duplicate name
	{
		return -1;
	}
	else
	{
		user_list[idx].m_status = SLOT_FULL;
		user_list[idx].m_pid = pid;
		user_list[idx].m_fd_to_user = pipe_to_child;
		user_list[idx].m_fd_to_server = pipe_to_parent;
		strcpy (user_list[idx].m_user_id, user_id);
	}

	return idx;
}

/*
 * Kills the user and performs cleanup
 */
void kick_user(int idx, USER * user_list)
{
	printf("USER '%s' is getting kicked\n", user_list[idx].m_user_id, user_list[idx].m_pid);
	kill_user(idx, user_list);
	cleanup_user(idx, user_list);
}

/*
 * broadcast message to all users
 */
void broadcast_msg(USER * user_list, char *buf, char *sender)
{
	for(int i=0;i<MAX_USER;i++) //go through all users
	{
		if(user_list[i].m_status != SLOT_EMPTY && strcmp(user_list[i].m_user_id, sender) != 0) //if there is a user at that slot
		{
			if (strcmp(sender, "SERVER") == 0)
			{
				write(user_list[i].m_fd_to_user, concat("NOTICE", buf), MAX_MSG);
			}
			else
			{
				write(user_list[i].m_fd_to_user, concat(sender, buf), MAX_MSG);
			}
		}
	}

	//iterate over the user_list and if a slot is full, and the user is not the sender itself,
	//then send the message to that user
	//return zero on success
}

/*
 * Cleanup user chat boxes
 */
void cleanup_users(USER * user_list)
{
	//iterate over user list
	for (int i = 0; i < MAX_USER; i++)
	{
		//chekc to see if a user occupies a slot in the list
		if(user_list[i].m_status == SLOT_FULL)
		{
			//kick the user (Kick also does cleanup)
			kick_user(i, user_list);
		}
	}
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
    if(token_cnt >= 2)
	{
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
	if(token_cnt >= 3)
	{
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
	char user_name[MAX_USER_ID];
	extract_name(buf, user_name);
	int idx_target = find_user_index(user_list, user_name);

	char text[MAX_MSG];
	extract_text(buf, text);

	//Extract the recipient name from string buf
	if (idx_target == -1)
	{
		write(user_list[idx].m_fd_to_user, "ERROR: User cannot be Found", MAX_MSG);
	}
	else if (extract_text(buf, text) == -1)
	{
		write(user_list[idx].m_fd_to_user, "ERROR: There is no message to be sent", MAX_MSG);
	}
	else if (idx == idx_target)
	{
		write(user_list[idx].m_fd_to_user, "ERROR: You can't connect to yourself this way. Maybe try meditation or a walk in the woods instead?", MAX_MSG);
	}
	else
	{
		write(user_list[idx_target].m_fd_to_user, concat(user_list[idx].m_user_id, text), MAX_MSG);
	}
	memset(buf,'\0', MAX_MSG);
	memset(text,'\0', MAX_MSG);

	// get the target user by name using extract_name() function
	// find the user id using find_user_index()
	// if user not found, write back to the original user "User not found", using the write()function on pipes.
	// if the user is found then write the message that the user wants to send to that user.
}

//takes in the filename of the file being executed, //send message to userand prints an error message stating the commands and their usage
void show_error_message(char *filename)
{



}

void exit_server(USER * user_list)
{
	cleanup_users(user_list);
	exit(1);
}
/*
 * Populates the user list initially
 */
void init_user_list(USER * user_list)
{
	//iterate over the MAX_USER
	//memset() all m_user_id to zero
	//set all fd to -1
	//set the status to be EMPTY
	for(int i=0; i<MAX_USER; i++) {
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
	close(pipe_SERVER_writing_to_child[1]);
	close(pipe_SERVER_reading_from_child[0]);
	close(pipe_child_reading_from_user[1]);
	close(pipe_child_writing_to_user[0]);
	int check;
    while(1)
    {
        	//read from user and send to server
		check = read(pipe_child_reading_from_user[0], buf, MAX_MSG);
		if (check == 0)
		{
			printf ("Lost connection to a user, cleaning up...\n");
			print_prompt("admin");
			close(pipe_SERVER_writing_to_child[0]);
			close(pipe_SERVER_reading_from_child[1]);
			close(pipe_child_reading_from_user[0]);
			close(pipe_child_writing_to_user[1]);
		}
		else if (check == MAX_MSG)
		{
			write (pipe_SERVER_reading_from_child[1], buf, MAX_MSG);
			memset(buf,'\0', MAX_MSG);
		}
        	//read from server and send to user
		check = read(pipe_SERVER_writing_to_child[0], buf, MAX_MSG);
		if (check == 0)
		{
			close(pipe_SERVER_writing_to_child[0]);
			close(pipe_SERVER_reading_from_child[1]);
			close(pipe_child_reading_from_user[0]);
			close(pipe_child_writing_to_user[1]);
		}
		else if (check == MAX_MSG)
		{
			write(pipe_child_writing_to_user[1], buf, MAX_MSG);
			memset(buf,'\0', MAX_MSG);
		}
        usleep(100);
    }
}

void execCommand (int cmd, char *buf, USER * user_list, int i)
{
	if (cmd == LIST)
	{
		list_users(i, user_list);
	}
	else if (cmd == KICK)
	{
		char user_name[MAX_USER_ID];
		int user_name_idx = extract_name(buf, user_name);
		if (user_name_idx == -1 || find_user_index(user_list, user_name) == -1)
		{
			printf("ERROR: User is not found\n");
		}
		else
		{
			kick_user(find_user_index(user_list, user_name), user_list);
		}
	}
	else if (cmd == P2P)
	{
		send_p2p_msg(i, user_list, buf);
	}
	else if (cmd == SEG)
	{

	}
	else if (cmd == EXIT)
	{
		exit_server(user_list); // Call exit and close off all fd's/kill all children/kill the server
	}
	else if (cmd == BROADCAST)
	{
		if (i == -1) //if it is from SERVER
		{
			broadcast_msg(user_list, buf, "SERVER"); //send message to all user
		}
		else //from user
		{
			broadcast_msg(user_list, buf, user_list[i].m_user_id); //send message to all user
		}
		memset(buf,'\0', MAX_MSG);
	}
}
void SigHandler (int sig)
{
	printf("\n");
	if(kill(getpid(), SIGKILL) == -1)
	{
		perror("ERROR: ");
	}
}
//return 1 if duplicate name
//return 2 if server full
int DupOflow (char * user_name, USER * user_list)
{
	if (find_user_index(user_list, user_name) != -1)
	{
		//Name taken
		return 1;
	}
	else if (find_empty_slot(user_list) == -1)
	{
		//No empty slot
		return 2;
	}
	return 0;
}
/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[])
{
	signal (SIGINT, SigHandler);
	int nbytes;
	setup_connection("YOUR_UNIQUE_ID"); // Specifies the connection point as argument.

	USER user_list[MAX_USER];
	init_user_list(user_list);   // Initialize user list

	char buf[MAX_MSG];
	fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK);
	print_prompt("admin");
	int Dop;
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
				Dop = DupOflow (user_id, user_list);
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
					runChild(pipe_child_writing_to_user, pipe_child_reading_from_user,
		    				pipe_SERVER_reading_from_child, pipe_SERVER_writing_to_child,
					    	buf, user_id);
				}
				else //parent
				{
					//Close ends that server dont use
					close (pipe_SERVER_writing_to_child[0]);
					close (pipe_SERVER_reading_from_child[1]);
					close (pipe_child_writing_to_user[1]);
					close (pipe_child_reading_from_user[1]);
					close (pipe_child_writing_to_user[0]);
					close (pipe_child_reading_from_user[0]);
					//Server process: Add a new user information into an empty slot
					if (add_user(user_list, pid, user_id, pipe_SERVER_writing_to_child[1], pipe_SERVER_reading_from_child[0], Dop) == -1)
					{
						if (Dop == 2)//full
						{
							write(pipe_SERVER_writing_to_child[1], "ERROR: You have reached the maximum number of users!", MAX_MSG);
							printf("ERROR: You have reached the maximum number of users!\n");
							usleep(1000);
						}
						else if (Dop == 1)//duplicate name
						{
							write(pipe_SERVER_writing_to_child[1], "ERROR: Name taken, pick another one!", MAX_MSG);
							printf("ERROR: Name taken, pick another one!\n");
							usleep(1000);
						}
						kill(pid, SIGKILL);
						print_prompt("admin");
						printf("User '%s' connection has been terminated!\n", user_id);
					}
					else
					{
						printf ("User '%s' with CHILD_PID '%d' joined the server\n", user_id, pid);
					}

					print_prompt("admin");
				}
				// Check max user and same user id
				// poll child processes and handle user commands
				// Poll stdin (input from the terminal) and handle admnistrative command
			/* ------------------------YOUR CODE FOR MAIN--------------------------------*/
			}//if

			/* ------------------------SERVER--------------------------------*/
			if(read(0, buf, MAX_MSG) != -1) //SERVER terminal
			{
				strtok(buf, "\n"); //remove '\n' from buf
				execCommand(get_command_type(buf), buf, user_list, -1); //run the command
				memset(buf,'\0', MAX_MSG);
				print_prompt("admin");
			}
			//Poll all child
			int check;
			for(int i=0; i<MAX_USER; i++)
			{
			    if(user_list[i].m_status != SLOT_EMPTY)
				{
					check = read(user_list[i].m_fd_to_server, buf, MAX_MSG);
					if(check != -1 && check != 0)
		        		{
		        			printf ("%s: %s\n", user_list[i].m_user_id, buf);
						execCommand(get_command_type(buf), buf, user_list, i);
						memset(buf,'\0', MAX_MSG);
						print_prompt("admin");
		        		}
					else if (check == 0)
					{
						kick_user(i, user_list);
						print_prompt ("admin");
					}
				}
			}

        usleep(100);
		}//while
}//main

/* --------------------End of the main function ----------------------------------------*/
