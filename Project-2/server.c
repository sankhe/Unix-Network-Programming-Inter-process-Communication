#include "header.h"
#define FIFO1   "/tmp/fifo.1"
#define FIFO2   "/tmp/fifo.2"
#define TEMPSIZE 100

void pipeFifoServer(int, int);
void serverSysMQ(int, int);
void serverPOMQ( mqd_t, mqd_t);

int main(int argc, char* argv[]) {
	int readfd = -1;
	int  writefd = -1;
	mqd_t readid, writeid;
	if((strcmp(argv[0], "pipe")) == 0) {
		pipeFifoServer(atoi(argv[1]), atoi(argv[2]));
	}
	else if ((strcmp(argv[0], "fifo")) == 0) {
		readfd = open(FIFO1, O_RDONLY, 0);
		writefd = open(FIFO2, O_WRONLY, 0);	
		pipeFifoServer(readfd, writefd);
	}
	else if((strcmp(argv[0], "sysmq")) == 0) {
		if ( (readfd = msgget(MQ_KEY2, 0)) < 0 ) {
			perror ("msgget") ;
		}
		if (( writefd = msgget(MQ_KEY1, 0)) == 0 ) {
			perror("msgget");
		}
		serverSysMQ(readfd, writefd);
	}
	else if((strcmp(argv[0], "pomq")) == 0) {
		struct mq_attr attr;
		/* Fill in attributes for message queue */
		attr.mq_maxmsg = 5;
		attr.mq_msgsize = MAXBUF;
		attr.mq_flags   = 0;
		readid = mq_open(MQ_KEY4,  O_RDWR | O_CREAT, FILE_MODE, &attr);
		if (readid == -1)
			perror ("server1 POMQ CreateError");

		writeid = mq_open(MQ_KEY3, O_RDWR | O_CREAT, FILE_MODE, &attr);
		if (writeid == -1)
			perror ("server2 POMQ CreateError");

		serverPOMQ(readid, writeid);	
	}
	return 0;
}

/* this function handles the SYSTEM MQ read and delete command processing */
void serverPOMQ( mqd_t readfd, mqd_t writefd) {
	Mesg msg;
	FILE *fp;
	int priority = 0;
	ssize_t nbytes = 0;
	int status = 0;
	while (1) {
		nbytes = mq_receive(readfd, (char *) &msg, MAXBUF, &priority);
		if (nbytes == -1)
			perror("server mq_receive failure on readfd");
		int n = strlen(msg.mesg_data);	
		if( msg.mesg_type == 2) {
			fp = fopen(msg.mesg_data, "r"); //read mode
			if( fp == NULL) {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : Error -> %s", strerror(errno));
				msg.mesg_len = strlen(msg.mesg_data);
				status = mq_send(writefd, (char *) &msg, MAXBUF, priority);
				if (status == -1) 
					perror("server mq_senf failure on writefd");
			}
			else{ 
				memset(&msg, 0, sizeof(msg));
				fseek(fp, 0L, SEEK_END);
				int sz = ftell(fp);
				fseek(fp, 0L, SEEK_SET);
				fread(msg.mesg_data, sz, 1, fp);
				msg.mesg_len = strlen(msg.mesg_data);
				if (msg.mesg_data[msg.mesg_len - 1] == '\n') {
					msg.mesg_data[msg.mesg_len - 1] = '\0';
				}
				msg.mesg_len = strlen(msg.mesg_data);
				status = mq_send(writefd, (char *) &msg, MAXBUF, priority);
				if (status == -1)
					perror("server mq_send failure on writefd");
				fclose(fp);
			}
		}
		if (msg.mesg_type == 3) {
			int del_status = remove(msg.mesg_data);
			if (del_status == 0) {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : deleted Successfully");
			}
			else {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : Error -> %s", strerror(errno));
			}
			msg.mesg_len = strlen(msg.mesg_data);
			status = mq_send(writefd, (char *) &msg, MAXBUF, priority);
			if (status == -1)
				perror("server mq_send failure on writefd");
		}
	}
}
/* This function handles the SYSTEM MQ read and delete command processing */
void serverSysMQ(int readfd, int writefd) {
	FILE *fp;
	Mesg msg;
	while (1) {
		int nbytes = 0;
		nbytes = msgrcv(readfd, &(msg.mesg_type), MAXBUF, 0, MSG_NOERROR);
		if(nbytes < 0) {
			 perror("server sgrcv");
		}
		msg.mesg_data[nbytes] = '\0';
		int n = strlen(msg.mesg_data);
		if (msg.mesg_type == 2) {
			fp = fopen(msg.mesg_data, "r"); // read mode
			if (fp == NULL) {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : Error -> %s", strerror(errno));
				msg.mesg_len = strlen(msg.mesg_data);
				if (msg.mesg_data[msg.mesg_len - 1] == '\n') {
                                        msg.mesg_data[msg.mesg_len - 1] = '\0';
                                }
                                msg.mesg_len = strlen(msg.mesg_data);
				msg.mesg_type = 8;
				if (msgsnd(writefd, &(msg.mesg_type), msg.mesg_len, 0) < 0) {
				 	perror("Server msgsend1 error:");
				}
			}
			else {
				memset(&msg, 0, sizeof(msg));
				fseek(fp, 0L, SEEK_END);
				int sz = ftell(fp);
				fseek(fp, 0L, SEEK_SET);
				fread(msg.mesg_data, sz, 1, fp);
				msg.mesg_len = strlen(msg.mesg_data);
				if (msg.mesg_data[msg.mesg_len - 1] == '\n') {
					msg.mesg_data[msg.mesg_len - 1] = '\0';
				}
				msg.mesg_len = strlen(msg.mesg_data);
				msg.mesg_type = 8;
				if(msgsnd(writefd, &(msg.mesg_type), msg.mesg_len, 0) < 0){
					perror("Server msgsend3 error:");
				}
				fclose(fp);
			}
		}
		if (msg.mesg_type == 3) {
			int status = remove(msg.mesg_data);
			if (status == 0) {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : deleted Successfully");
			}
			else {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : Error -> %s", strerror(errno));
			}
			msg.mesg_len = strlen(msg.mesg_data);
			msg.mesg_type = 8;
			if(msgsnd(writefd, &(msg.mesg_type), msg.mesg_len, 0) < 0) {
				perror("Server msgsend3 error:");
			}
		}
	}	
}

/* Handles Pipe and Fifo read and delete command processing*/
void pipeFifoServer(int readfd, int writefd) {
	FILE *fp;
	Mesg msg;
	while (1) {
		/*Read message from IPC*/ 
		if (read(readfd, &msg, MAXBUF) < 0)
			perror("server read error:");
		int n = msg.mesg_len;
		if (msg.mesg_type == 2) {
			fp = fopen(msg.mesg_data ,"r"); // read mode
			if (fp == NULL ) {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : Error -> %s", strerror(errno));
				msg.mesg_len = strlen(msg.mesg_data);
				write(writefd, &msg, MAXBUF);

			} else {
				memset(&msg, 0, sizeof(msg));
				fseek(fp, 0L, SEEK_END);
				int sz = ftell(fp);
				fseek(fp, 0L, SEEK_SET);
				fread(msg.mesg_data, sz, 1, fp);
				msg.mesg_len = strlen(msg.mesg_data);
				if (msg.mesg_data[msg.mesg_len - 1] == '\n') {
					msg.mesg_data[msg.mesg_len - 1] = '\0';
				}
				msg.mesg_len = strlen(msg.mesg_data);
				write(writefd, &msg, MAXBUF);
				fclose(fp);
			}
		}
		if (msg.mesg_type == 3) {
			int status = remove(msg.mesg_data);
			if (status == 0) {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : deleted Successfully");
			}
			else {
				snprintf(msg.mesg_data + n, sizeof(msg.mesg_data) - n, " : Error -> %s", strerror(errno));
			}
			msg.mesg_len = strlen(msg.mesg_data);
			write(writefd, &msg, MAXBUF);
		}
	}
}
