What works:
	int broadcast_msg(USER * user_list, char *buf, char *sender)

		Send a message from SERVER to ALL USERS

		Need to and more functions

	add_user()
		Almost done.

		Used pipe_SERVER_writing_to_child[1], pipe_SERVER_reading_from_child[0]
		as argument
	
	void execCommand (int cmd, USER * user_list)
		Called in main, in the SERVER section.
		
		This function takes in a command types, and then execute it

What changed:
	sleep() to usleep(), make it more responsive.
	