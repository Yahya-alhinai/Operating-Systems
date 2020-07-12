/* CSci4061 F2018 Assignment 1
* login: cselabs login name (login used to submit)
* date: mm/dd/yy
* name: full name1, full name2, full name3 (for partner(s))
* id: id for first name, id for second name, id for third name */

// This is the main file for the code
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

/*-------------------------------------------------------HELPER FUNCTIONS PROTOTYPES---------------------------------*/
void show_error_message(char * ExecName);
void show_targets(target_t targets[], int nTargetCount);

int compile(target_t targets[], int nTarget);
void Fexec(char *args[], char *TargetName);
void build(target_t targets[], int nTarget, int nTargetCount);
void build_Make(target_t targets[], int nTargetCount);
void show_targets(target_t targets[], int nTargetCount);
/*-------------------------------------------------------END OF HELPER FUNCTIONS PROTOTYPES--------------------------*/


/*-------------------------------------------------------GLOBALE VARIABLES-------------------------------------------*/
int up_to_date = 0;
char *tokens[256];
/*-------------------------------------------------------END OF GLOBALE VARIABLES------------------------------------*/


/*-------------------------------------------------------HELPER FUNCTIONS--------------------------------------------*/

//This is the function for writing an error to the stream
//It prints the same message in all the cases
void show_error_message(char * ExecName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", ExecName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	exit(0);
}

int compile(target_t targets[], int nTarget) // 1 = compile -- 0 = Don't
{
	if (does_file_exist(targets[nTarget].TargetName) == -1)
		return 1; // The file have not been created yet, therefore it should be compiled!
	else
	{
		for (int i = 0; i < targets[nTarget].DependencyCount; i++)
		{
			if (compare_modification_time(targets[nTarget].TargetName, targets[nTarget].DependencyNames[i]) == 2 )
			{
				return 1; //if the time of those two file are not idintial --> compile!
			}
		}
	}
	return 0;
}

void Fexec(char *args[], char *TargetName)
{
	up_to_date += 1;
	int wstatus;
    pid_t childpid = fork();

    if(childpid==-1){
        perror("Failed to fork");
    }
    //child code
    if(childpid == 0){
        execvp(args[0],args);
        perror("child failed to exec all_ids");
    }
    else
	{
		wait(&wstatus);
		if (WEXITSTATUS(wstatus) != 0) {
			printf("Makefile: recipe for target '%s' failed\n", TargetName);
			printf("make4061: *** [%s] Error 1\n", TargetName);
		exit(-1);
		}
    }
}

void build(target_t targets[], int nTarget, int nTargetCount)
{
	char cmd[256];
	if (targets[nTarget].DependencyCount == 0 && targets[nTarget].Status == 0) //if the target doesn't have any dependency, just run the commands
	{
		strcpy(cmd, targets[nTarget].Command);
		targets[nTarget].Status = 1;
		printf("%s\n", targets[nTarget].Command);
		parse_into_tokens(cmd, tokens, " ");
		Fexec(tokens, targets[nTarget].TargetName);
	}
	for(int j = 0; j < targets[nTarget].DependencyCount; j++) //visit all the dependencies of the target
	{
		strcpy(cmd, targets[nTarget].Command);
		int idx_depend = find_target(targets[nTarget].DependencyNames[j], targets, nTargetCount);
		if (idx_depend == -1) // The dependency is not a target
		{
			if(does_file_exist(targets[nTarget].DependencyNames[j]) == -1) //file does not  exist!
			{
				printf("make4061: *** No rule to make target '%s', needed by '%s'.  Stop.\n", targets[nTarget].DependencyNames[j], targets[nTarget].TargetName);
				exit(-1);
			}
			else if(j == targets[nTarget].DependencyCount-1 && compile(targets, nTarget) && parse_into_tokens(cmd, tokens, " ")) //if it is the last dependency of the target, means that we have checked all the dependency
			{
				printf("%s\n", targets[nTarget].Command);
				Fexec(tokens, targets[nTarget].TargetName);
			}
		}
		else
		{
			build(targets, idx_depend, nTargetCount); // first build the dependencies of the target
			if(j == targets[nTarget].DependencyCount-1 && compile(targets, nTarget) && parse_into_tokens(cmd, tokens, " ")) // compile the command once you check all the Dependencies
			{
				printf("%s\n", targets[nTarget].Command);
				Fexec(tokens, targets[nTarget].TargetName);
			}
		}
	}
}

void build_Make(target_t targets[], int nTargetCount)
{
	build(targets, 0, nTargetCount);
	if (up_to_date == 0)
		printf("make4061: '%s' is up to date.\n", targets[0].TargetName);
}


//Phase1: Warmup phase for parsing the structure here. Do it as per the PDF (Writeup)
void show_targets(target_t targets[], int nTargetCount){
	printf("THIS IS ME!\n");
	printf("%d\n", nTargetCount);
	for (int i = 0; i < nTargetCount; i++){
		printf("[1]: %s\n", targets[i].TargetName);
		printf("[2]: %d\n", targets[i].DependencyCount);
		printf("[3]: ");
		for (int j = 0; j < targets[i].DependencyCount; j++){
			printf("%s", targets[i].DependencyNames[j]);
			printf((j == targets[i].DependencyCount-1)? "" : ", ");
		}
		printf("\n");
		printf("[4]: %s\n\n", targets[i].Command);
	}
}

/*-------------------------------------------------------END OF HELPER FUNCTIONS-------------------------------------*/


/*-------------------------------------------------------MAIN PROGRAM------------------------------------------------*/
//Main commencement
int main(int argc, char *argv[])
{
  target_t targets[MAX_NODES];
  int nTargetCount = 0;

  /* Variables you'll want to use */
  char Makefile[64] = "Makefile";
  char TargetName[64];

  /* Declarations for getopt. For better understanding of the function use the man command i.e. "man getopt" */
  extern int optind;   		// It is the index of the next element of the argv[] that is going to be processed
  extern char * optarg;		// It points to the option argument
  int ch;
  char *format = "f:h";
  char *temp;

  //Getopt function is used to access the command line arguments. However there can be arguments which may or may not need the parameters after the command
  //Example -f <filename> needs a finename, and therefore we need to give a colon after that sort of argument
  //Ex. f: for h there won't be any argument hence we are not going to do the same for h, hence "f:h"
  while((ch = getopt(argc, argv, format)) != -1){
	  switch(ch){
	  	  case 'f':
	  		  temp = strdup(optarg);
	  		  strcpy(Makefile, temp);  // here the strdup returns a string and that is later copied using the strcpy
	  		  free(temp);	//need to manually free the pointer
	  		  break;

	  	  case 'h':
	  	  default:
	  		  show_error_message(argv[0]);
	  		  exit(1);
	  }

  }

  argc -= optind;
  if(argc > 1)   //Means that we are giving more than 1 target which is not accepted
  {
	  show_error_message(argv[0]);
	  return -1;   //This line is not needed
  }

  /* Init Targets */
  memset(targets, 0, sizeof(targets));   //initialize all the nodes first, just to avoid the valgrind checks

  /* Parse graph file or die, This is the main function to perform the toplogical sort and hence populate the structure */
  if((nTargetCount = parse(Makefile, targets)) == -1)  //here the parser returns the starting address of the array of the structure. Here we gave the makefile and then it just does the parsing of the makefile and then it has created array of the nodes
	  return -1;


  //Phase1: Warmup-----------------------------------------------------------------------------------------------------
  //Parse the structure elements and print them as mentioned in the Project Writeup

  /* Comment out the following line before Phase2 */

  //show_targets(targets, nTargetCount);

  //End of Warmup------------------------------------------------------------------------------------------------------

  /*
   * Set Targetname
   * If target is not set, set it to default (first target from makefile)
   */
  if(argc == 1)
		strcpy(TargetName, argv[optind]);    // here we have the given target, this acts as a method to begin the building
  else
  	strcpy(TargetName, targets[0].TargetName);  // default part is the first target

  /*
   * Now, the file has been parsed and the targets have been named.
   * You'll now want to check all dependencies (whether they are
   * available targets or files) and then execute the target that
   * was specified on the command line, along with their dependencies,
   * etc. Else if no target is mentioned then build the first target
   * found in Makefile.
   */

  //Phase2: Begins ----------------------------------------------------------------------------------------------------
  /*Your code begins here*/

  build_Make(targets, nTargetCount);

  /*End of your code*/
  //End of Phase2------------------------------------------------------------------------------------------------------

	return 0;
}
/*-------------------------------------------------------END OF MAIN PROGRAM------------------------------------------*/
