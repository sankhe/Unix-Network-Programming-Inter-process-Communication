#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 5000 // the port client will be connecting to 
#define MAX_BUFF_SIZE 1024 // max number of bytes we can get at once
#define TEMP_BUFF_SIZE 100

typedef struct command
{
        int id;          /* Command id */
        char name[TEMP_BUFF_SIZE]; /* This can be IPC mechanism or file name*/
}command;

command cmd;

void getCommand();
void tcp_client(char* host_name);
void udp_client(char* host_name);
void start();

int main(int argc, char *argv[]) {
	start();
	memset(&cmd, 0, sizeof(cmd));

	if(argc < 3) {
		printf("Error : ./client [tcp|udp] <hostname>\n");
		exit(1);
	}
	if(strcmp(argv[1], "tcp") == 0) {
		tcp_client(argv[2]);
	} 
	if(strcmp(argv[1], "udp") == 0) {
		udp_client(argv[2]);
	}
	return 0;
}

/* This function runs connection oriented tcp_client */
void tcp_client(char* host_name) {
	printf("TCP Client connecting.....\n");
	int sockfd = 0, n = 0;
	char recvBuff[MAX_BUFF_SIZE];
	char sendBuff[MAX_BUFF_SIZE];
	struct sockaddr_in serv_addr; 
	struct hostent *hp;

	memset(recvBuff, 0,sizeof(recvBuff));
	memset(sendBuff, 0, sizeof(sendBuff));
	memset(&serv_addr, 0, sizeof(serv_addr));
	// creates tcp socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Error : Could not create socket \n");
		return;
	} 

	// get server's IP address
	if((hp = gethostbyname(host_name)) == NULL) {
		printf("gethostbyname error - %s\n", strerror(errno));
		return;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	bcopy(hp->h_addr, &(serv_addr.sin_addr.s_addr), hp->h_length);

	//connect to the server
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\n Error : Connect Failed \n");
		return;
	} 

	getCommand();

	/* READ or DELETE command */
	while(cmd.id == 1 || cmd.id == 2) {
		ssize_t n = 0;
		/* file name */
		memset(sendBuff, 0, sizeof(sendBuff));
		strcpy(sendBuff, cmd.name);
		int len = strlen(sendBuff);
		sendBuff[len] = cmd.id + '0';
		sendBuff[len+1] = '\0';

		/* write message to socket */
		if ((send(sockfd, sendBuff, strlen(sendBuff), 0)) < 0) {
			printf("\n Error : Send Failed --> %s\n", strerror(errno));
			return;
		}
		/* Read from the sockets and display result on stdout */
		int flag = 0;
		while( flag == 0) {
			memset(recvBuff, 0,sizeof(recvBuff));
			if((n = recv(sockfd, recvBuff, sizeof(recvBuff) - 1, 0)) < 0) {
				printf("\n Error : recv Failed --> %s\n", strerror(errno));
				return;
			}
			if(strncmp(recvBuff,"najukaEND", 9) == 0) {
				flag = 1;
			}
			if(flag == 0) { 
				recvBuff[n] = '\0';
				printf("%s", recvBuff);
			}
		}
		if(recvBuff[n-1] == '\n') {
			recvBuff[n-1] = '\0';
                }
		printf("\n");
		// next command as user input
		getCommand();
	}
	// EXIT command
	if (cmd.id == 3) {
		memset(sendBuff, 0, sizeof(sendBuff));
		strcpy(sendBuff, "exit");
		int len = strlen(sendBuff);
		sendBuff[len] = cmd.id + '0';
		sendBuff[len+1] = '\0';

		if((send(sockfd, sendBuff, strlen(sendBuff), 0)) < 0) {
			printf("\n Error : send Failed --> %s\n", strerror(errno));
			return;
		}
		memset(recvBuff, 0,sizeof(recvBuff));
		if ((n = recv(sockfd, recvBuff, sizeof(recvBuff)-1,0)) < 0 ) {
			printf("\n Error : recv Failed --> %s\n", strerror(errno));
			return;
		}
		if(recvBuff[n-1] == '\n') {
			recvBuff[n-1] = '\0';
		}
		recvBuff[n] = '\0';
		printf("%s\n", recvBuff);
		printf("TCP client exiting.....\nGoodbye from the TCP Client !!!\n");
	}
}

/* This function runs connectionless udp_client */
void udp_client(char* host_name) {
	printf("UDP Client connecting ....\n");
	int sockfd = 0;
	int n = 0;
	char sendBuff[MAX_BUFF_SIZE];
	char recvBuff[MAX_BUFF_SIZE];
	struct sockaddr_in serv_addr;
	struct hostent *hp;
	int len = sizeof(struct sockaddr_in);

	/* Read from socket and display results of stdout */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return;
	}

	if((hp = gethostbyname(host_name)) == NULL) {
		printf("gethostbyname error - %s\n", strerror(errno));
		return;
	}

	memset(sendBuff, 0, sizeof(sendBuff));
	memset(recvBuff, 0, sizeof(recvBuff));
	/* fill in the server's address and data */
	memset((char*)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	/* put the host's address into the server address structure */
	bcopy(hp->h_addr, &(serv_addr.sin_addr.s_addr), hp->h_length);

	getCommand();

	while((cmd.id == 1) || (cmd.id == 2)) {
		ssize_t n = 0;
		memset(sendBuff, 0, sizeof(sendBuff));

		strcpy(sendBuff, cmd.name);
		int i = strlen(sendBuff);
		sendBuff[i] = cmd.id + '0';
		sendBuff[i+1] = '\0';

		if(sendto(sockfd, sendBuff, sizeof(sendBuff), 0,  (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("sendto error - %s\n", strerror(errno));
			return;
		}
		/* read from the socket and display result to stdout */
		int flag = 0;
		while( flag == 0) {

			memset(recvBuff, 0, sizeof(recvBuff));
			if((n = recvfrom(sockfd, recvBuff, sizeof(recvBuff), 0, (struct sockaddr *)&serv_addr, &len)) < 0) {
				printf("recvfrom error - %s\n", strerror(errno));
				return;
			}
			if(strncmp(recvBuff,"najukaEND", 9) == 0) {
				flag = 1;
			}
			if(flag == 0) {
				recvBuff[n] = '\0';
				printf("%s", recvBuff);
			}
		}
		if(recvBuff[n-1] == '\n') {
			recvBuff[n-1] = '\0';
		}
		printf("\n");
		// get next command from user
		getCommand();
	}
	if(cmd.id == 3) {
		memset(sendBuff, 0, sizeof(sendBuff));
		strcpy(sendBuff, "exit");
		int i = strlen(sendBuff);
		sendBuff[i] = cmd.id + '0';
		sendBuff[i+1] = '\0';

		if(sendto(sockfd, sendBuff, sizeof(sendBuff), 0,  (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("sendto error - %s\n", strerror(errno));
			return;
		}
		memset(recvBuff, 0, sizeof(recvBuff));
		if((n = recvfrom(sockfd, recvBuff, sizeof(recvBuff), 0, (struct sockaddr *)&serv_addr, &len)) < 0) {
			printf("recvfrom error - %s\n", strerror(errno));
			return;
		}
		n = strlen(recvBuff);
		if(recvBuff[n-1] == '\n') {
			recvBuff[n-1] = '\0';
		}
		recvBuff[n] = '\0';
		printf("%s\n", recvBuff);
		printf("UDP client exiting.....\nGoodbye from the UDP Client !!!\n");
	}
}

/* This function displays the initial Welcome message on the screen */
void start() {
	system("clear");
	printf("***********************************************************\n");
	printf("Client's Side\n");
	printf("\nConnection oriented(TCP) and Connectionless(UDP) Sockets have been implemented. \n");
	printf("The commands that can be used are :\n1. READ <filename>\n2. DELETE <filename>\n3. EXIT\n");
	printf("\n**********************************************************\n\n");
}

/* This function takes the input commands from the user */
void getCommand() {
	printf("\nPlease enter your command\n");
	char str[TEMP_BUFF_SIZE] = { 0 };
	char str1[TEMP_BUFF_SIZE] = { 0 };
	memset(&cmd, 0, sizeof(cmd));

	if(fgets(str1, sizeof(str1), stdin) == NULL)
		printf("%s\n", strerror(errno));
	sscanf(str1, "%s %s", str, cmd.name);

	if(strcmp(str, "READ") == 0) {
		if(strlen(cmd.name) > 0) {
			cmd.id = 1;
		}
		else {
			printf("Error : <filename> is not specified\n");				cmd.id = 0;
			getCommand();
		}
	}
	else if (strcmp(str, "DELETE") == 0) {
		if(strlen(cmd.name) > 0) {
			cmd.id = 2;
		}
		else {
			printf("Error : <filename> is not specified\n");
			cmd.id = 0;
			getCommand();
		}
	}
	else if (strcmp(str, "EXIT") == 0) {
		cmd.id = 3;
	}
	else {
		printf(" Error : Invalid Command\n");
		cmd.id = 0;
		getCommand();
	}
}
