#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#define BUFFER 100

volatile sig_atomic_t myFlag = 0;
static void myHandler(int);
struct sigaction newAction, oldAction;


void readFromSharedFile();
void readFromFile(char*);
void writeToSharedFile(char*, int);
void deleteFile(char*);

int main() {
	sigset_t zeromask;
	/* Intialization of Signals */	
	sigemptyset(&newAction.sa_mask);
	newAction.sa_flags = 0; // no arguments are passed to the signal handler
	newAction.sa_handler = myHandler; // set signal handler

	sigemptyset (&oldAction.sa_mask);
	sigemptyset(&zeromask);
	sigaddset (&newAction.sa_mask, SIGUSR1); // additional signal to Block
	
	if ( sigaction(SIGUSR1, &newAction, &oldAction)) // install signal handler
	{
		printf("error\n");
		exit(-1);
	}
	// first signal from server to client 
	kill(getppid(), SIGUSR1);
	sigprocmask (SIG_BLOCK, &newAction.sa_mask, &oldAction.sa_mask); // block SIGUSR1
	for ( ; ; ) {
		while(myFlag == 0) {
			//atomically sleep + reset the sig mask to the arg value,i.e., all signals are unblocked		
			sigsuspend (&zeromask);
		}
		/* signal has occurred, process it.. */
		readFromSharedFile();
		myFlag = 0; // SIGUSR1 is still blocked
	}
	sigprocmask (SIG_SETMASK, &oldAction.sa_mask, NULL); // reset to the old mask
	exit(0);
}
/* Signal Handler*/
static void myHandler(int sigNo) {
	myFlag = 1;
	sigprocmask (SIG_BLOCK, &newAction.sa_mask, &oldAction.sa_mask); // block SIGUSR1
	return;
}

/* This function reads from the shared file*/
void readFromSharedFile() {
	FILE* fp;
	fp = fopen("sharedfile", "r");
	char ch = 0;
	char str[BUFFER] = {0};
	char str2[] = "Error : Wrong command \n\0";
	if( fp == NULL ) {
		printf("Error : File Doesn't exists\n");
		exit(4);
	}
	fscanf(fp, "%c %s", &ch, str);
	if ((ch == 0) || strlen(str) <= 0) {
		char str1[] = "Error : Wrong command format\n\0";
		writeToSharedFile(str1, 1);
	}
	fclose(fp);
	switch(ch) {
		case 'r' :
			readFromFile(str);
			break;
		case 'd' :
			deleteFile(str);
			break;
		default  :
			writeToSharedFile(str2, 1);		 
	}
}

/* This function process the "delete" command*/
void deleteFile(char *filename) {

	int status;
	char str[BUFFER] = { 0 };
	strcpy (str,filename);
	status = remove(filename);
	if( status == 0 )
		strcat(str, " File deleted successfully.\0\n");
	else {
		strcat(str, " ERROR: Unable to delete the file. May be file doesn't exist.\0\n");
	}
	writeToSharedFile(str, 1);
}

/* This function reads the content from the file specified by "read" command*/
void readFromFile(char* filename) {
	FILE *fp;
        fp = fopen(filename, "r");
        if (fp == NULL) {
                if (errno == 2) {
                        char str1[] = "Error : File doesn't exist\n\0";
                        writeToSharedFile(str1, 1);
                }
                if (errno == 13) {
                        char str2[] = "Error : Permission Denied\n\0";
                        writeToSharedFile(str2, 1);
                }
        }
        else {
                writeToSharedFile(filename, 2);
        }
}

/* This function write to the shared file */
void writeToSharedFile(char* str, int check) {
	FILE *fp1, *fp2;
	char a;
	fp2 = fopen("sharedfile", "w");
	if (fp2 == NULL) {
		printf("Error : Shared File Doesn't Exists");
		exit(6);
	}
	if (check == 2) {
		fp1 = fopen(str, "r");
		do {
			a = fgetc(fp1);
			fputc(a, fp2);
		} while (a != EOF);

	}
	else if (check == 1) {
		fputs(str, fp2);		
	}
	fcloseall();
	//printf ("Server --- > CLient  Signal \n") ;        
	kill(getppid(), SIGUSR1);
} 
