

int add_user(int idx, USER * user_list, int pid, char * user_id, int pipe_to_child, int pipe_to_parent)
{
	for i in range(0, len(user_list)):
    if (user_list[i].status == empty):
      user_list[i].m_pid = pid;
      user_list[i].m_fd_to_user = pipe_to_child;
      user_list[i].m_fd_to_server = pipe_to_parent;
      user_list[i].m_user_id[MAX_USER_ID] = user_id;
      return i;
	// populate the user_list structure with the arguments passed to this function
	// return the index of user added
	return -1;
}

void kill_user(int idx, USER * user_list) {
  int k_pid = user_list[idx].m_pid;
  kill(k_pid);

	// kill a user (specified by idx) by using the systemcall kill()
	// then call waitpid on the user
}
//////////////////////////////////////////////////////////////////////////////
  server -> child -> user/client


// THIS IS THE SERVER.c!
while(1) {
  /* ------------------------YOUR CODE FOR MAIN--------------------------------*/

  // Handling a new connection using get_connection
  int pipe_SERVER_reading_from_child[2];
  int pipe_SERVER_writing_to_child[2];
  char user_id[MAX_USER_ID];

  //I ADDED THE CODE BELOW


  if (get_connection(user_id, pipe_child_writing_to_user, pipe_child_reading_from_user) == 0) //a user is connected
  {
    int pipe_child_writing_to_user[2];
    int pipe_child_reading_from_user[2];

    printf ("%s\n", "Connected");

    if (pipe(pipe_SERVER_writing_to_child) < 0 || pipe(pipe_SERVER_reading_from_child) < 0)
    {
      perror("Failed to create pipe\n");
      return -1;
    }
    int pid = fork();
    if (pid == -1)
      perror ("Failed to fork");
    else if (pid == 0)//child
      printf ("%s\n", "Child here");
      // Child process: poli users and SERVER
      //assumed that the pipes from child is connected to USER/CLIENT
      //polling while loop check input from user and input from server
      // Child will have 4 pipes total:
      //    pipe_child_writing_to_user
      //    pipe_child_reading_from_user
      //    pipe_SERVER_reading_from_child
      //    pipe_SERVER_writing_from_child
      //    while polling check to see if user is dead (learn how to check if a process with pid given is still active)
    else //parent (Server)
      // Server process: Add a new user information into an empty slot
      //add user to user_list
      // poll child processes and handle user commands
      // Poll stdin (input from the terminal) and handle admnistrative command
      printf ("%s\n", "Parent here");
  }
  // Check max user and same user id



  /* ------------------------YOUR CODE FOR MAIN--------------------------------*/
}
}


#Oguz:
- Now working on kill/kick/add/cleanup user


#Yahya & Khai:
- Warm-up
- Read message from terminal, store it as a string, perform the command
- read(0, buf, MAX_MSG); //0 = stdin, this will read stdin and store to buf
