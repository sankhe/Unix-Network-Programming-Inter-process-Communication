#include "header.h"

#define FIFO1   "/tmp/fifo.1"
#define FIFO2   "/tmp/fifo.2"
#define SIZE 20
#define BUF_SIZE 100
#define SERVER "server"

void start();
void getCommandId();
void processCommand();
int getIpc();
void runSelectedIpc(int);
void runPipe();
void runFifo();
void runPosix();
void runSys(); 

typedef struct command
{
	int id;		 /* Command id */
        char name[SIZE]; /* This can be IPC mechanism or file name*/
}command;

command cmd;

pid_t childpid;
int pipe1[2] = {-1, -1};
int pipe2[2] = {-1, -1};
/* fifo */
int readfd = -1;
int  writefd = -1;
/* msg queue system v */
int readid = -1;
int writeid = -1;
/* msg queue posix descriptor */
mqd_t readfd_p = -1;
mqd_t writefd_p = -1;

/* remembers the previously selected ipc mechanism for READ and DELETE */
int oldOption = -1;

int main() {

	start();
	memset(&cmd, 0, sizeof(cmd));
	getCommandId();

	return 0;
}

/* This function cleans the IPC conncetion while switching ipc mechanism */

void clean(int oldoption) {
	int status = -1;
	switch(oldoption) {
		case 1:
			/* close client's read and write pipes after execution is over */
			printf("Exiting from PIPE........\n");
			close(pipe1[1]);
			close(pipe2[0]);
			status = kill(childpid, SIGKILL);
			wait(&status);
			break;
		case 2:
			printf("Exiting from FIFO.........\n");
			close(readfd);
                	close(writefd);
			/* unlink the FIFO1 and FIFO2 */
			if(unlink(FIFO1))
				perror("unlink error");
			if(unlink(FIFO2))
				perror("unlink error");
			status = kill(childpid, SIGKILL);
			wait(&status);
			break;
		case 3:
			printf("Exiting from SYSTEM V MQ........\n");
			/* now we can delete the queues */
			msgctl(readid, IPC_RMID, NULL);
			msgctl(writeid, IPC_RMID, NULL);
			status = kill(childpid, SIGKILL);
			wait(&status);
			break;
		case 4:
			printf("Exiting from POSIX MQ............\n");
			/* Done with queues, so close it */
			mq_close(readfd_p);
			mq_close(writefd_p);
			/* destroy the queues */
			if (mq_unlink(MQ_KEY3))
				perror("mq_unlink error");
			if(mq_unlink(MQ_KEY4))	
				perror("mq_unlink error");
			status = kill(childpid, SIGKILL);
			wait(&status);
			break;
	}
}

/* This function processes the user commands */
void processCommand() {
	int newOption = -1;
	switch(cmd.id) {
		case 1:
			newOption = getIpc();
			if (newOption > 0) {
				if( oldOption > 0 ) {
					clean(oldOption);	
				}
			oldOption = newOption;
			runSelectedIpc(newOption);
			}
			else {
				printf("ERROR : Please enter SWITCH SELECT PIPE or SWITCH FIFO or SWITCH SYSMQ or SWITCH POMQ.\n");
                		getCommandId();
			}	
			break;
		case 2:
		case 3:
			if (oldOption > 0) {
				//printf("we are using previously selected ipc mechanism: %d\n", oldOption);
				runSelectedIpc(oldOption);
			}
			break;
		
		case 4:
			if (oldOption > 0) {
				clean(oldOption);
			}
			printf("Good Bye  !!!\n");
			exit(0);
			break;
		default:
			printf("Error: Invalid Command\n");
		break;
	}	
}

/* This function initiate the selected ipc mechanism */
void runSelectedIpc(int op) {
	switch(op) {
		case 1:	
			runPipe();
			break;
		case 2:
			runFifo();
			break;
		case 3:
			runSys();
			break;
		case 4:
			runPosix();
			break;
	}
}

/* This function writes and reads to pipe/fifo and displays the result */
void clientPipeFifo(int readfd, int writefd)
{

	Mesg msg;
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.mesg_data,cmd.name);                       

	msg.mesg_len = strlen(msg.mesg_data);
	msg.mesg_type = cmd.id;

	/*Write message to IPC channel */
	if(write(writefd, &msg, MAXBUF) < 0)
		perror("client : Error write to pipe");
	if(read(readfd, &msg, MAXBUF) < 0)
		perror("client : Error read from pipe");
	if(msg.mesg_data[msg.mesg_len - 1] == '\n') {
		msg.mesg_data[msg.mesg_len - 1] = 0;
	}
	printf("%s\n", msg.mesg_data);
}



/* Initialization and Execution of PIPE IPC mechanism */
void runPipe() {
	if (cmd.id == 1) {
		printf("**** switchig to PIPEs ****\n");

		pipe(pipe1);                                    //Create two pipes
		pipe(pipe2);

		/*For sending server read and write pipes as command line arguments*/
		char read[20];
		char write[20];
		sprintf(read, "%d", pipe1[0]);           //convert to char* datatype
		sprintf(write, "%d", pipe2[1]);         //convert to char* datatype

		char *arglist[] = {"pipe",read,write,NULL};     //populate arglist to send via execv
		if((childpid = fork()) == 0)
		{
			execv(SERVER, arglist);
			printf("Error : %s\n", strerror(errno));
			exit(1);
		} 
	}
	else if (cmd.id == 2 || cmd.id == 3)
	{
		/*Close pipes which would be used by server for read and write.*/
		close(pipe1[0]);
		close(pipe2[1]);
		/*client readfd and writefd are send for communication to the clien()*/
		clientPipeFifo(pipe2[0], pipe1[1]);
	}
}

/* Initialization and Execution of PIPE IPC mechanism */
void runFifo() {
	if (cmd.id == 1) {
		printf("**** switchig to FIFO ****\n");

		/*Create two FIFOs; OK if they already exist */
		if ((mkfifo(FIFO1, FILE_MODE) < 0) && (errno != EEXIST))
			printf("can't create %s\n", FIFO1);
		if ((mkfifo(FIFO2, FILE_MODE) < 0) && (errno != EEXIST)) {
			unlink(FIFO1);
			printf("can't create %s\n", FIFO2);
		}

		char *arglist[] = {"fifo",NULL};  
		if ((childpid = fork()) == 0) {
			execv(SERVER, arglist);
			printf("Error : %s\n", strerror(errno));
			exit(5);
		}
	}
	else if (cmd.id == 2 || cmd.id == 3) {
		writefd = open(FIFO1, O_WRONLY, 0);
		readfd = open(FIFO2, O_RDONLY, 0);
		clientPipeFifo(readfd, writefd);
	}	
}


/* System V MQ: client and server reads, writes and displays the results */
void clientMQ1(int readfd, int writefd) {
	Mesg msg;
	memset(&msg, 0, sizeof(msg));
	strncpy(msg.mesg_data, cmd.name, strlen(cmd.name));

	msg.mesg_len = strlen(msg.mesg_data);
	msg.mesg_type = cmd.id;

	/*Write message to IPC channel */
	if(msgsnd(writefd, &(msg.mesg_type), msg.mesg_len, 0) < 0) {
		 perror("client msgsend");
	}
	int readbytes = 0;
	if((readbytes = msgrcv(readfd, &(msg.mesg_type), MAXBUF, 8, MSG_NOERROR)) < 0) {
		perror("client msgrcv");
	}
	msg.mesg_data[readbytes] = '\0';
	printf("%s\n", msg.mesg_data);

}

/* Initialization and Execution of PIPE IPC mechanism */
void runSys() {

	if (cmd.id == 1) {
		printf("**** switchig to System V ****\n");
		readid = -1; writeid = -1;
		/* Create the queues */
		writeid = msgget(MQ_KEY2, IPC_CREAT | SVMSG_MODE);
		if (writeid < 0)
			perror("msgget writeid error:");
		readid = msgget(MQ_KEY1, IPC_CREAT | SVMSG_MODE);
		if (readid < 0)
                        perror("msgget readid error:");

		char *arglist[] = {"sysmq", NULL};     //populate arglist to send via execv
		if((childpid = fork()) == 0)
		{
			execv(SERVER, arglist);
			printf("Error : %s\n", strerror(errno));
			exit(4);
		}
	}
	else if(cmd.id == 2 || cmd.id == 3) {

		clientMQ1(readid, writeid);
	}		
}

/* POSIX MQ :client and server reads, writes and displays the results */
void clientMQ2(mqd_t readfd, mqd_t writefd) {
	Mesg msg;
	int priority = 0;
	int status = 0;
	ssize_t nbytes = 0;

	memset(&msg, 0, sizeof(msg));
	strcpy(msg.mesg_data,cmd.name);

	msg.mesg_len = strlen(msg.mesg_data);
	msg.mesg_type = cmd.id;

	/*Write message to IPC channel */
	status = mq_send(writefd,  (char *) &msg, MAXBUF, priority);
	if (status == -1)
        	perror("client mq_send failure on writefd");
	
	nbytes = mq_receive(readfd,(char*)  &msg, MAXBUF, &priority);
	if (nbytes == -1)
        	perror("client mq_receive failure on readfd");
	if(msg.mesg_data[msg.mesg_len - 1] == '\n') {
		msg.mesg_data[msg.mesg_len - 1] = 0;
	}
	printf("%s\n", msg.mesg_data);
}

/* Initialization and Execution of PIPE IPC mechanism */
void runPosix() {
	struct mq_attr attr;

	if (cmd.id == 1) {
		printf("**** switchig to POSIX ****\n");

		/* Fill in attributes for message queue */
		attr.mq_maxmsg = 10;
		attr.mq_msgsize = MAXBUF;
		attr.mq_flags   = 0;

		/* Create the queues */
		readfd_p = mq_open(MQ_KEY3, O_RDWR | O_CREAT, FILE_MODE, &attr);
		if (readfd_p == -1) 
			perror ("client1 POMQ CreateError");
		writefd_p = mq_open(MQ_KEY4, O_RDWR | O_CREAT, FILE_MODE, &attr);
		if (writefd_p == -1) 
			perror ("client2 POMQ CreateError");

		char *arglist[] = {"pomq", NULL};     //populate arglist to send via execv
		if((childpid = fork()) == 0) {
			execv(SERVER, arglist);
			printf("Error : %s\n", strerror(errno));
			exit(4);
		}
	}
	else if(cmd.id == 2 || cmd.id == 3) {
		clientMQ2(readfd_p, writefd_p);
	}
}

/* This function takes the input commands from the user */
void getCommandId() {
	int check = -1;
	do {
		printf("\nPlease enter you command\n");
		char str[BUF_SIZE] = { 0 };
		char str1[BUF_SIZE] = { 0 };
		memset(&cmd, 0, sizeof(cmd));
                fflush(stdin);
		if (fgets(str1, sizeof(str1), stdin) == NULL)
			printf("%s\n", strerror(errno));
		sscanf(str1, "%s %s", str, cmd.name);
		if (strcmp(str, "SWITCH") == 0) {
			if( strlen(cmd.name) > 0) {
				cmd.id = 1;
				check = 1;
				processCommand();
			}
			else {
				printf("Error : IPC mechanism name is not specified\n");
				cmd.id = 5;
				continue;
			}
		}
		else if (strcmp(str, "READ") == 0) {
			if (strlen(cmd.name) > 0) {
				if ( check == 1) {
					cmd.id = 2;
					processCommand();
				}
				else {
					printf("Error : You have not selected IPC mechanism\n");
					cmd.id = 5;
					continue;
				}
			}
			else {
				printf("Error : file name is not specified\n");
				cmd.id = 5;
				continue;
			}

		}
		else if (strcmp(str, "DELETE") == 0) {
			if (strlen(cmd.name) > 0) {
				if ( check == 1) {
					cmd.id = 3;
					processCommand();
				}
				else {
					printf("Error : You have not selected IPC mechanism\n");
					cmd.id = 5;
					continue;	
				}
			}
			else {
				printf("Error : file name is not specified\n");
				cmd.id = 5;
				continue;
			}

		}
		else if (strcmp(str, "EXIT") == 0) {
			cmd.id = 4;
			processCommand();
		}
		else {
			printf(" Error : Invalid Command\n");
			cmd.id = 5;
		}
	} while (cmd.id <= 5 && cmd.id >=1);		
}


/* This function selects user entered ipc mechanism */
int getIpc() {

	if (strcmp(cmd.name, "PIPE") == 0) {
		return 1;
	}
	else if (strcmp(cmd.name, "FIFO") == 0) {
		return 2;
	}
	else if (strcmp(cmd.name, "SYSMQ") == 0) {
		return 3;
	}
	else if (strcmp(cmd.name, "POMQ") == 0) {
		return 4;
	}
	return -1;
}

/* This function displays the initial Welcome message on the screen */
void start() {
	printf("\n ****************************************************************************\n");
	printf("COMMANDS are as follows:\n");
	printf("1. SWITCH ipc_mechanism\n2. READ filename \n3. DELETE filename \n4. EXIT\n\n");
	printf("Options for ipc_mechanisms :\n1. PIPE \n2. FIFO \n3. SYSMQ \n4. POMQ \n");
	printf("for example SWITCH command you can write : SWITCH PIPE or SWITCH FIFO etc. \n");
	printf("\n ****************************************************************************\n");
}
