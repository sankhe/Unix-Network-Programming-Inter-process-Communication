#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define PORT 5000 // the port client will be connecting to
#define MAX_BUFF_SIZE 1024 // max number of bytes we can get at once
#define TEMP_BUFF_SIZE 128

void tcp_server();
void udp_server();
void start(); 
int main(int argc, char *argv[]) {
	start();

        if(argc < 2) {
                printf("Error : ./server [tcp|udp] \n");
                exit(1);
        }
        if(strcmp(argv[1], "tcp") == 0) {
                tcp_server();
        }
        if(strcmp(argv[1], "udp") == 0) {
                udp_server();
        }
        return 0;
}

/* prints the welcome message when the server starts*/
void start() {
	char host_name[TEMP_BUFF_SIZE] = { 0 };
	gethostname(host_name, TEMP_BUFF_SIZE - 1);
	system("clear");
	printf("*********************************************************\n\n");
	printf("Server's Side\n");
	printf("\nConnection oriented(TCP) and Connectionless(UDP) Sockets have been implemented. \n");
	printf("Server's hostname is %s\n",host_name);
	printf("\n*******************************************************\n\n");
}

/* connection oriented : tcp_server */
void tcp_server() {
	printf("TCP server has started....\n");
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; 
	int cmd_id = 0;
	ssize_t n = 0;
	char sendBuff[MAX_BUFF_SIZE + 1];
	char recvBuff[MAX_BUFF_SIZE];

	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(sendBuff, 0, sizeof(sendBuff));
	memset(recvBuff, 0, sizeof(recvBuff));

	// creating tcp socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0) {
		printf("Error : socket Failed --> %s\n", strerror(errno));
		return;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT); 
	int yes = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if((bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){
		printf("bind error - %s\n", strerror(errno));
		return; 
	}
	// waiting for client
	if((listen(listenfd, 10)) < 0) {
		printf("listen error - %s\n", strerror(errno));
		return;
	}

	if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) < 0) {
		printf("accept error - %s\n", strerror(errno));
		return;
	}
	memset(recvBuff, 0,sizeof(recvBuff));
	if((n = recv(connfd, recvBuff, sizeof(recvBuff)-1,0)) < 0 ) {

		printf("recv error - %s\n", strerror(errno));
		return;
	}
	cmd_id = recvBuff[n-1] - '0';
	recvBuff[n-1] = '\0';
	FILE *fp;

	while(cmd_id == 1 || cmd_id == 2) {
		memset(sendBuff, 0, sizeof(sendBuff));
		if (cmd_id == 1) {
			if((fp = fopen(recvBuff, "r")) == NULL) {
				printf("Unknown file name\n");
				sprintf(sendBuff, "%s : %s", recvBuff, strerror(errno));
				if((send(connfd, sendBuff, strlen(sendBuff), 0)) < 0) {
					printf("send error - %s\n", strerror(errno));
					return;	
				}
			}
			else {
				/* fopen succeeded */
				printf("Reading from %s\n", recvBuff);
				fseek(fp, 0L, SEEK_END);
				int sz = ftell(fp);
				if(sz >= MAX_BUFF_SIZE) {
					fseek(fp, 0L, SEEK_SET);
					while(sz >= MAX_BUFF_SIZE) {
						memset(sendBuff, 0, sizeof(sendBuff));
						fread(sendBuff, MAX_BUFF_SIZE,1,fp);
						sz = sz - MAX_BUFF_SIZE;
						if((send(connfd, sendBuff, strlen(sendBuff), 0)) < 0) {
							printf("send error - %s\n", strerror(errno));
							return;
						}
					}
					if (sz > 0) {
						memset(sendBuff, 0, sizeof(sendBuff));
						fread(sendBuff, sz, 1,fp);
						if(sendBuff[strlen(sendBuff)-1] == '\n')
							sendBuff[strlen(sendBuff)-1] = '\0';
						if((send(connfd, sendBuff, strlen(sendBuff), 0)) < 0) {
							printf("send error - %s\n", strerror(errno));
							return;
						}
					}
				}
				else {
					fseek(fp, 0L, SEEK_SET);
					fread(sendBuff, sz, 1, fp);
					if(sendBuff[strlen(sendBuff)-1] == '\n')
						sendBuff[strlen(sendBuff)-1] = '\0';
					if((send(connfd, sendBuff, strlen(sendBuff), 0)) < 0) {
						printf("send error - %s\n", strerror(errno));
						return;
					}
				}
				fclose(fp);
			}
		}
		if(cmd_id == 2) {
			int del_status = remove(recvBuff);
			memset(sendBuff,0, sizeof(sendBuff));
			if (del_status == 0) {
				printf("Deleting %s\n", recvBuff);
				sprintf(sendBuff, "%s : deleted Successfully", recvBuff);
			}
			else {
				printf("Unknown file name\n");
				sprintf(sendBuff, "%s : %s", recvBuff, strerror(errno));
			}
			if((send(connfd, sendBuff, strlen(sendBuff), 0)) < 0) {
				printf("send error : %s\n", strerror(errno));
			}
		}
		sleep(1);
		memset(sendBuff,0, sizeof(sendBuff));
		strcpy(sendBuff, "najukaEND\0");
		if((send(connfd, sendBuff, strlen(sendBuff), 0)) < 0) {
			printf("send error - %s\n", strerror(errno));
			return;
		}

		/* Read next message from the socket */
		memset(recvBuff, 0, sizeof(recvBuff));
		if ((n = recv(connfd, recvBuff, sizeof(recvBuff) - 1, 0)) < 0){
			printf("recv error - %s\n", strerror(errno));
			return;
		}
		cmd_id = recvBuff[n-1] - '0';
		recvBuff[n-1] = '\0';
	}
	/* EXIT command */
	if(cmd_id == 3) {
		memset(sendBuff, 0, sizeof(sendBuff));
		strcpy(sendBuff, "server is exiting...\n");
		if((send(connfd, sendBuff, strlen(sendBuff),0)) < 0) {
			printf("send error - %s\n", strerror(errno));
			return;
		}
		printf("Goodbye from the TCP Sever !!!\n");
		close(connfd);
	}
}	


void udp_server() {
	printf("UDP server has started.....\n");
	int connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;

	int len = sizeof(struct sockaddr_in);
	char sendBuff[MAX_BUFF_SIZE + 1];
	char recvBuff[MAX_BUFF_SIZE];

	FILE *fp;
	ssize_t n = 0;

	int cmd_id = 0;
	/* create socket */
	connfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(connfd < 0) {
		printf("socket error - %s\n", strerror(errno));
		return;
	}

	memset(sendBuff, 0, sizeof(sendBuff));
	memset(recvBuff, 0, sizeof(recvBuff));

	/* initialize server addr */
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	/* bind to server port */
	if(bind(connfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("bind error - %s\n", strerror(errno));
		return;
	}
	/* wait for connection request from client */
	if((n = recvfrom(connfd, recvBuff, sizeof(recvBuff), 0, (struct sockaddr *)&client_addr, &len)) < 0) {
		printf("recvfrom error - %s\n", strerror(errno));
		return;
	}
	n = strlen(recvBuff);
	cmd_id = recvBuff[n-1]-'0';
	recvBuff[n-1] = '\0';

	/* Read Message from the sokcet */
	while( cmd_id == 1 || cmd_id == 2) {
		memset(sendBuff, 0, sizeof(sendBuff));
		if(cmd_id == 1) {
			fp = fopen(recvBuff, "r");
			if(fp == NULL) {
				printf("Unknown file name\n");
				sprintf(sendBuff, "%s : %s", recvBuff, strerror(errno));			if(sendto(connfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
					printf("sendto error - %s\n", strerror(errno));
					return;
				}
			}
			else {
				/* fopen succeeded */
				printf("Reading from %s\n", recvBuff);
				fseek(fp, 0L, SEEK_END);
				int sz = ftell(fp);
				if(sz >= MAX_BUFF_SIZE) {
					fseek(fp, 0L, SEEK_SET);
					while(sz >= MAX_BUFF_SIZE) {
						memset(sendBuff, 0, sizeof(sendBuff));
						fread(sendBuff, MAX_BUFF_SIZE,1,fp);
						sz = sz - MAX_BUFF_SIZE;
						if(sendto(connfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
							printf("sendto error - %s\n", strerror(errno));
							return;
						}
					}
					if (sz > 0) {
						memset(sendBuff, 0, sizeof(sendBuff));
						fread(sendBuff, sz, 1,fp);
						if(sendBuff[strlen(sendBuff)-1] == '\n')
							sendBuff[strlen(sendBuff)-1] = '\0';
						if(sendto(connfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
							printf("sendto error - %s\n", strerror(errno));
							return;
						}
					}
				}
				else {
					fseek(fp, 0L, SEEK_SET);
					fread(sendBuff, sz, 1, fp);
					if(sendBuff[strlen(sendBuff)-1] == '\n')
						sendBuff[strlen(sendBuff)-1] = '\0';
					if(sendto(connfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
						printf("sendto error - %s\n", strerror(errno));
						return;
					}
				}
				fclose(fp);
			}
		}
		if(cmd_id == 2) {
			int del_status = remove(recvBuff);
			memset(sendBuff,0, sizeof(sendBuff));
			if (del_status == 0) {
				printf("Deleting %s\n", recvBuff);
				sprintf(sendBuff, "%s : deleted Successfully", recvBuff);
			}
			else {
				printf("Unknown file name\n");
				sprintf(sendBuff, "%s : %s", recvBuff, strerror(errno));
			}
			if(sendto(connfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
				printf("sendto error - %s\n", strerror(errno));
				return;
			}
		}
		sleep(1);
		memset(sendBuff,0, sizeof(sendBuff));
		strcpy(sendBuff, "najukaEND\0");
		if(sendto(connfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
			printf("sendto error - %s\n", strerror(errno));
			return;
		}
		/* Read next message from the socket */
		if((n = recvfrom(connfd, recvBuff, sizeof(recvBuff), 0, (struct sockaddr *)&client_addr, &len)) < 0) {
			printf("recvfrom error - %s\n", strerror(errno));
			return;
		}
		n = strlen(recvBuff);
		cmd_id = recvBuff[n-1]-'0';
		recvBuff[n-1] = '\0';
	}
	
	//EXIT command
	if(cmd_id == 3) {
		memset(sendBuff, 0, sizeof(sendBuff));
                strcpy(sendBuff, "server is exiting...\n");
		if(sendto(connfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
			printf("sendto error - %s\n", strerror(errno));
                        return;
		}
		printf("Goodbye from the UDP server\n");
                close(connfd);
	}
}	
