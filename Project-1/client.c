#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#define SERVER "server"
#define MAXSIZE 1000
#define BUFFER 100

volatile sig_atomic_t myFlag = 0;
static void myHandler(int);
struct sigaction newAction, oldAction;

void start(int);
int writeToSharedFile(char, char*, int);
void readFromSharedFile();

int main() {
	/*Initializing signals*/
	sigset_t zeromask;
	sigemptyset(&newAction.sa_mask);
	newAction.sa_flags = 0; // no arguments are passed to the signal handler
	newAction.sa_handler = myHandler; // set signal handler

	/*Opening the shared file*/
	/*Clear the shared file data.*/
        FILE *fp0 = fopen("sharedfile", "w");
        fclose(fp0);
	
	/*Creating a Server Process*/
	pid_t childpid; /* variable to store the child's pid */

        /* now create a new process */
        childpid = fork();

        if (childpid < 0) /* fork returns -1 on failure */
        {
                perror("fork"); /* display error message */
                exit(1);
        }

        if (childpid == 0) /* fork() returns 0 to the child process */
        {
                execl(SERVER,SERVER, NULL);
                /* If execv returns, it must have failed. */
                printf("ERROR 1: ommand\n");
                exit(2);
        }
	
		
	sigemptyset (&oldAction.sa_mask);
	sigemptyset(&zeromask);
	sigaddset (&newAction.sa_mask, SIGUSR1); // additional signal to Block

	if ( sigaction(SIGUSR1, &newAction, &oldAction)) // install signal handler
	{
		printf("error\n");
		exit(-1);
	}

	sigprocmask (SIG_BLOCK, &newAction.sa_mask, &oldAction.sa_mask); // block SIGUSR1
	for ( ; ; ) {
		while(myFlag == 0) {

			// atomically sleep + reset the sig mask to the arg value,i.e., all signals are unblocked.
			sigsuspend (&zeromask); 
		}
		
		/* signal has occurred, process it.. */
		readFromSharedFile();
		start(childpid);
		myFlag = 0; // SIGUSR1 is still blocked

	}
	sigprocmask (SIG_SETMASK, &oldAction.sa_mask, NULL); // reset to the old mask
	exit(0);
}


/* Signal Handler */
static void myHandler(int sigNo) {
	myFlag = 1;
	sigprocmask (SIG_BLOCK, &newAction.sa_mask, &oldAction.sa_mask); // block SIGUSR1
	return;
}

void start(int child_pid) {
	/* Initial Message */
	printf("\n **************************** \n");
	printf("Please Use following 3 commands\n");
	printf("1. Read : r filename\n2. Delete : d filename\n3. Exit : e\n");
	printf("\n **************************** \n");
	int check = -1;
	char ch = 0;
        char str[BUFFER] = { 0 };
	char input[BUFFER] = { 0 };

	while (check ==  -1) {
		printf("Please Enter your command: ");
		fgets(str, sizeof(str), stdin);
		sscanf(str, "%c %s", &ch, str);
		
		switch(ch) {
			case 'r' :
			case 'd' : 

				check = writeToSharedFile(ch, str, child_pid);
				if (check == 1) {
					check = -1;
					continue;			
				} 
				break;
			case 'e' :
				/* cleaning up the server process. */
				kill(child_pid, SIGKILL);
				printf("Good Bye From Server\n");
				system("rm sharedfile");
				/* terminating client process */
				printf("Good Bye From Client\n");
				exit(0);
				break;
			default  :
				printf(" ERROR : Invalid command\n" ); 	
		}
	}
}

/* Non_exit commands i.e. read and delete write to the shared file */
int writeToSharedFile(char ch, char* str, int child_pid) {
	char buff[BUFFER] = { 0 };
	sprintf(buff, "%c %s", ch, str);
	if (strlen(str) < 0 ){
		printf("Error : Invalid Command format\n");
		return 1;
	}
	FILE *fp;
        fp = fopen("sharedfile", "w");
	fputs(buff, fp);
	fclose(fp);
        //printf ("Client --- > Server Signal \n") ;
	kill(child_pid, SIGUSR1);
	return 0;
}

/* This Function reads from the shared file and display the result on the standard output*/
void readFromSharedFile() {
	char buff[MAXSIZE] = { 0 };
	FILE *fp;
	char a;
	fp = fopen("sharedfile", "r");
	if (fp == NULL) {
		return;
	}
	/* display result on stdout */
	while((a = fgetc(fp)) != EOF) {
		putc(a, stdout);
	}		
	fclose(fp);
}
