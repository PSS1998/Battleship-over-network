#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/socket.h> 
#include <sys/mman.h>
#include <sys/stat.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <signal.h>
#include <fcntl.h>

#define MAXLINE 1024 

void connectUsers(char* portUser1, char* portUser2);
void sendMsg(char* Msg, char* portUser);

int HeartBeatFlag = 0;

int main(int argc, char *argv[]) { 
	
	fd_set master; 
	fd_set read_fds; 
	int fdmax;      
	int newfd;        

	int yes=1;
	int broadcast = 1;
	int sockfd; 
	int sockfdrecv; 
	char buffer[MAXLINE]; 
	char tempBuffer[MAXLINE];
	char *heartBeatMsg = "127.0.0.1 1101"; 
	struct sockaddr_in servaddr, servBroadcastaddr;
	unsigned short ServerBroadcastPort;
	unsigned short clientBroadcastPort;
	unsigned short port = 1101;

	FD_ZERO(&master);    
	FD_ZERO(&read_fds);

/*
	if (argc != 9) {     /* Test for correct number of arguments*/ 
/*		fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]); 
		exit(1); 
	}

	ServerBroadcastPort = atoi(argv[4]);
	clientBroadcastPort = atoi(argv[8]); 
*/	

	ServerBroadcastPort = atoi(argv[1]);
	clientBroadcastPort = atoi(argv[2]);

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	if ( (sockfdrecv = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
        	perror("setsockopt (SO_BROADCAST)");
        	exit(1);
    }
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&servBroadcastaddr, 0, sizeof(servBroadcastaddr)); 
	
	servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(1101); 

	servBroadcastaddr.sin_family = AF_INET; // IPv4 
	servBroadcastaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	servBroadcastaddr.sin_port = htons(ServerBroadcastPort);

	if (setsockopt(sockfdrecv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
    }
	
	if ( bind(sockfdrecv, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 

	if ((listen(sockfdrecv, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    }

	
	int len, m; 
	len = sizeof(servBroadcastaddr);

    	
    fdmax = sockfd + sockfdrecv ; // so far, it's this one
	
    int flag = 1;


	int n=10;

	char usernames[10][15];
	char userips[10][15];
	char userports[10][15];
	int valid[10];
	int matched[10];

	int numUsers = 0;

	char users_trying_to_connect[10][2][20];
	int matched_users_trying_to_connect[10];
	int num_users_trying_to_connect = 0;

	int desc_ready;

	int connfd;
	int childpid;
	

	for(;;){

		struct timeval tv = {1, 0};
		
		FD_SET(sockfdrecv, &read_fds);
	    if (flag = select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
	        perror("select");
	        exit(4);
	    }	
	    int i;
	    desc_ready = flag;
		if (FD_ISSET(sockfdrecv, &read_fds)) {
			flag = 1;

			connfd = accept(sockfdrecv, (struct sockaddr*)&servaddr, &len);


			bzero(tempBuffer, sizeof(tempBuffer));
			m = recv(connfd, tempBuffer, 80, 0);

			printf("Client : %s\n", tempBuffer);

            int i = 0;
			int j = i;
			while(1){
				if(tempBuffer[i] == ' '){
					tempBuffer[i] = '\0';
					strcpy(userips[numUsers], &tempBuffer[j]);
					break;
				}
				i++;
			}
			i++;
			j = i;
			while(1){
				if(tempBuffer[i] == ' '){
					tempBuffer[i] = '\0';
					strcpy(userports[numUsers], &tempBuffer[j]);
					break;
				}
				i++;
			}
			i++;
			j = i;
			int specificUserReq = 0;
			char userReq[20] = "";
			while(1){
				if(tempBuffer[i] == ' '){
					specificUserReq = 1;
					tempBuffer[i] = '\0';
					strcpy(usernames[numUsers], &tempBuffer[j]);
					break;
				}
				if(tempBuffer[i] == '\0'){
					tempBuffer[i] = '\0';
					strcpy(usernames[numUsers], &tempBuffer[j]);
					break;
				}
				i++;
			}
			i++;
			if(specificUserReq){
				j = i;
				while(1){
					if(tempBuffer[i] == ' ' || tempBuffer[i] == '\0'){
						tempBuffer[i] = '\0';
						strcpy(userReq, &tempBuffer[j]);
						break;
					}
					i++;
				}
			}
			valid[numUsers] = 1;
			matched[numUsers] = 0;

			numUsers++;

			int found = 0;
			int indexofuser;
			for(int i=0; i<numUsers; i++){
				if(strcmp(usernames[numUsers-1], users_trying_to_connect[i][1]) == 0 && matched_users_trying_to_connect[i] == 0){
					found = 1;
					indexofuser = i;
					break;
				}
			}

			if(found){
				matched_users_trying_to_connect[indexofuser] = 1;
				connectUsers(users_trying_to_connect[indexofuser][0], userports[numUsers-1]);
				specificUserReq = 0;
				continue;
			}

			if(specificUserReq){
				found = 0;
				for(int i=0; i<numUsers; i++){
					if(strcmp(usernames[i], userReq) == 0 && matched[i] == 0){
						strcpy(userReq, userports[i]);
						found = 1;
						break;
					}
				}
				if(found){
					connectUsers(userReq, userports[numUsers-1]);
					specificUserReq = 0;
					continue;
				}
				else{
					strcpy(users_trying_to_connect[num_users_trying_to_connect][0], userports[numUsers-1]);
					strcpy(users_trying_to_connect[num_users_trying_to_connect][1], userReq);
					matched_users_trying_to_connect[num_users_trying_to_connect] = 0;
					num_users_trying_to_connect++;
					sendMsg("User is not online", userports[numUsers-1]);	
					specificUserReq = 0;
					continue;
				}
			}

			int count = 0;
			int user[2];
			int matchFlag = 0;
			for(int z=0; z<numUsers; z++){
				if(matched[z] == 0 && valid[z] == 1 && (strcmp(userports[z], userports[numUsers-1]) != 0)){
					user[count] = z;
					count++;
				}
			}
			if(count == 2){
				matchFlag = 1;
			}


            if(matchFlag){
				matched[user[0]] = 1;
				matched[user[1]] = 1;
				connectUsers(userports[user[0]], userports[user[1]]);
				matchFlag = 0;
			}
		}

		if(flag == 0){
			HeartBeatFlag = 1;
			flag = 1;
		}
		
		if(HeartBeatFlag){
				sendto(sockfd, (const char *)heartBeatMsg, strlen(heartBeatMsg), 0, (const struct sockaddr *) &servBroadcastaddr, len);
				printf("heartBeat message sent.\n");
				HeartBeatFlag = 0;
		}
		 
	}
	return 0; 
} 


void connectUsers(char* portUser1, char* portUser2){

	int sockfd; 
    struct sockaddr_in cli; 
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 

    bzero(&cli, sizeof(cli)); 

    cli.sin_family = AF_INET; 
    cli.sin_addr.s_addr = htonl(INADDR_ANY); 
    cli.sin_port = htons(atoi(portUser2)); 
  
    if (connect(sockfd, (struct sockaddr*)&cli, sizeof(cli)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    }
    
    char buff[80] = ""; 
    int n; 
    bzero(buff, sizeof(buff)); 
    n = 0;  
    strcpy(buff, portUser1);
    send(sockfd, buff, sizeof(buff), 0); 
    close(sockfd);


}

void sendMsg(char* Msg, char* portUser){

	int sockfd; 
    struct sockaddr_in cli; 
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 

    bzero(&cli, sizeof(cli)); 

    cli.sin_family = AF_INET; 
    cli.sin_addr.s_addr = htonl(INADDR_ANY); 
    cli.sin_port = htons(atoi(portUser)); 
  
    if (connect(sockfd, (struct sockaddr*)&cli, sizeof(cli)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    }
    
    char buff[80] = ""; 
    int n; 
    bzero(buff, sizeof(buff)); 
    n = 0;  
    strcpy(buff, Msg);
    send(sockfd, buff, sizeof(buff), 0); 
    close(sockfd);

}

