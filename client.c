#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <fcntl.h> 
#include <errno.h>

#define MAXLINE 1024 

void connectTOopponent(unsigned short OpponentPort);
void playGameAsClient(int connclient);
void playGameAsServer(int connclient);

int port = 1103;
char ipAddress[20] = "127.0.0.1";
char usernameLocal[20] = "";
char portString[20] = "";
char mapFile[20] = "";

int max(int x, int y) 
{ 
    if (x > y) 
        return x; 
    else
        return y; 
} 

int main(int argc, char *argv[]) { 
	printf("Enter your port: ");
	scanf("%d", &port);
	printf("Enter your username: ");
	scanf("%s", usernameLocal);
	printf("Enter your map's file name: ");
	scanf("%s", mapFile);
	snprintf(portString, 20, "%d", port);
	int connectSpecificUser = 0;
	char OpponentUsername[20] = "";
	int yes=1;
	int sockfd; 
	int sockfdsend;
	int sockfdlisten;
	char buffer[MAXLINE]; 
	char heartBeatMsgAnswer[50] = "";
	strcat(heartBeatMsgAnswer, ipAddress);
	strcat(heartBeatMsgAnswer, " ");
	strcat(heartBeatMsgAnswer, portString);
	strcat(heartBeatMsgAnswer, " ");
	strcat(heartBeatMsgAnswer, usernameLocal);
	char *hello = "Hello from client"; 
	struct sockaddr_in	 servBroadcastaddr, servaddr, listenaddr;
	unsigned short OpponentPort;
	unsigned short ServerBroadcastPort;
	unsigned short ClientBroadcastPort; 

	fd_set read_fds;  
	fd_set write_fds;
	fd_set listen_fds;	
	int fdmax;

	ServerBroadcastPort = atoi(argv[1]);
	ClientBroadcastPort = atoi(argv[2]);

	if(argc == 4){
		connectSpecificUser = 1;
		strcpy(OpponentUsername, argv[3]);
		strcat(heartBeatMsgAnswer, " ");
		strcat(heartBeatMsgAnswer, OpponentUsername);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&listen_fds);

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	if ( (sockfdsend = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	}

	memset(&servBroadcastaddr, 0, sizeof(servBroadcastaddr));
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&listen_fds, 0, sizeof(listen_fds));

	listenaddr.sin_family = AF_INET; 
	listenaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	listenaddr.sin_port = htons(port); 
	
	servBroadcastaddr.sin_family = AF_INET; 
	servBroadcastaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servBroadcastaddr.sin_port = htons(ServerBroadcastPort); 


	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
    }

	if ( bind(sockfd, (const struct sockaddr *)&servBroadcastaddr, sizeof(servBroadcastaddr)) < 0 ) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}
	
	int n, len;
	int m; 
	len = sizeof(servaddr);

	socklen_t* len2;

	fdmax = sockfd + sockfdsend ;

	int waitingForGame = 0;

	int firstConnectioToServer = 1;

	int connclient;


	for(;;){

		struct timeval tv = {2, 0};

		FD_SET(sockfd, &read_fds);


		if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
			perror("select");
        		exit(4);
        }


		if (FD_ISSET(sockfd, &read_fds) && firstConnectioToServer) {
			
			n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, (struct sockaddr *) &servBroadcastaddr, (socklen_t*)sizeof(servBroadcastaddr));
			buffer[n] = '\0'; 
			printf("Server HeartBeat : %s\n", buffer);

			char ServerPort[30];
			char temp[30];
			strcpy(temp, buffer);
			int index = 0;
			while(1){
				if(temp[index] == ' '){
					temp[index] = '\0';
					strcpy(ServerPort, &temp[index+1]);
					break;
				}
				index++;
			}

			servaddr.sin_family = AF_INET; 
			servaddr.sin_port = htons(atoi(ServerPort)); 
			servaddr.sin_addr.s_addr = INADDR_ANY;

			if (connect(sockfdsend, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) { 
        		printf("connection with the server failed...\n"); 
        		exit(0); 
    		} 
    		int t = send(sockfdsend, (const char *)heartBeatMsgAnswer, strlen(heartBeatMsgAnswer), 0);
			printf("HeartBeat answer sent.\n");

			firstConnectioToServer--;


			waitingForGame = 1;

		}

		if(waitingForGame && connectSpecificUser){
			int gamedone = 0;
			if ( (sockfdlisten = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
				perror("socket creation failed"); 
				exit(EXIT_FAILURE); 
			}
			if (setsockopt(sockfdlisten, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		        perror("setsockopt");
		        exit(1);
		    }
			if ( bind(sockfdlisten, (const struct sockaddr *)&listenaddr, sizeof(listenaddr)) < 0 ) { 
				perror("bind failed"); 
				exit(EXIT_FAILURE); 
			} 
			if ((listen(sockfdlisten, 5)) != 0) { 
		        printf("Listen failed...\n"); 
		        exit(0); 
		    }
		    fdmax = sockfdlisten;
		    int notonline = 0;
		    for(;;){
			    FD_SET(sockfdlisten, &listen_fds);
			    if (select(fdmax+1, &listen_fds, NULL, NULL, NULL) == -1) {
			        perror("select");
			        exit(4);
			    }
			    if (FD_ISSET(sockfdlisten, &listen_fds)) {
					connclient = accept(sockfdlisten, (struct sockaddr*)&listenaddr, &len);
			    	bzero(buffer, sizeof(buffer));
			    	if(notonline == 0){
						int n = recv(connclient, buffer, MAXLINE, 0);
						if(strcmp(buffer, "User is not online") == 0){
							printf("Server : %s\n", buffer);
							notonline = 1;
							close(connclient);
							continue;
						}
						
					}
					if(notonline){
						n = recv(connclient, buffer, MAXLINE, 0);
						if(strcmp(buffer, "Start Game") == 0){
							printf("opponent Client : %s", buffer);
		                	playGameAsServer(connclient);
		                }
		                gamedone = 1;
						break;
					}
					printf("Server : Opponents Port is %s", buffer);
					OpponentPort = atoi(buffer);
					connectTOopponent(OpponentPort);
					break;
				}
			}
			close(sockfdlisten);
			close(connclient);
			break;
		}

		if(firstConnectioToServer == 1){
			//Server is not online
			int yes=1;
			int broadcast = 1;
			int sockfdsend;
			int sockfdrecv;
			int sockfdrecvTCP;
			int connfd;
			fd_set read_fds;
			char msgRequest[30] = "";
			strcpy(msgRequest, heartBeatMsgAnswer);
			char buffer[30];
			struct sockaddr_in recvaddr;
			struct sockaddr_in recvaddrTCP;
			struct sockaddr_in sendaddr;
			FD_ZERO(&read_fds);
			if ( (sockfdrecvTCP = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
				perror("socket creation failed"); 
				exit(EXIT_FAILURE); 
			}
			if ( (sockfdsend = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
				perror("socket creation failed"); 
				exit(EXIT_FAILURE); 
			}
			if ( (sockfdrecv = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
				perror("socket creation failed"); 
				exit(EXIT_FAILURE); 
			}
			if (setsockopt(sockfdrecvTCP, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		        perror("setsockopt");
		        exit(1);
		    }
			if (setsockopt(sockfdsend, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
	        	perror("setsockopt (SO_BROADCAST)");
	        	exit(1);
		    }
		    if (setsockopt(sockfdsend, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		        perror("setsockopt");
		        exit(1);
		    }
		    if (setsockopt(sockfdrecv, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
	        	perror("setsockopt (SO_BROADCAST)");
	        	exit(1);
		    } 
		    if (setsockopt(sockfdrecv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		        perror("setsockopt");
		        exit(1);
		    } 
		    memset(&recvaddrTCP, 0, sizeof(recvaddrTCP));
		    memset(&recvaddr, 0, sizeof(recvaddr));
		    memset(&sendaddr, 0, sizeof(sendaddr));
		    recvaddrTCP.sin_family = AF_INET; // IPv4 
			recvaddrTCP.sin_addr.s_addr = htonl(INADDR_ANY);
			recvaddrTCP.sin_port = htons(port);
		    sendaddr.sin_family = AF_INET; // IPv4 
			sendaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
			sendaddr.sin_port = htons(ClientBroadcastPort);
			recvaddr.sin_family = AF_INET; // IPv4 
			recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			recvaddr.sin_port = htons(ClientBroadcastPort);
			if ( bind(sockfdrecv, (const struct sockaddr *)&recvaddr, sizeof(recvaddr)) < 0 ) 
			{ 
				perror("bind failed"); 
				exit(EXIT_FAILURE); 
			}
			if ( bind(sockfdrecvTCP, (const struct sockaddr *)&recvaddrTCP, sizeof(recvaddrTCP)) < 0 ) 
			{ 
				perror("bind failed"); 
				exit(EXIT_FAILURE); 
			}
			listen(sockfdrecvTCP, 10); 
			int len; 
			len = sizeof(sendaddr);
			fdmax = max(max(sockfdsend, sockfdrecv), sockfdrecvTCP) ;
			sendto(sockfdsend, (const char *)msgRequest, strlen(msgRequest), 0, (const struct sockaddr *) &sendaddr, len);
			int gamedone = 0;
			while(1){
				FD_SET(sockfdrecv, &read_fds);
				FD_SET(sockfdrecvTCP, &read_fds);
				struct timeval tv = {1, 0};
				int flag;
				if (flag = select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			        perror("select");
			        exit(4);
			    }
			    if (FD_ISSET(sockfdrecv, &read_fds)) {
			    	int n = recvfrom(sockfdrecv, (char *)buffer, 30, 0, (struct sockaddr *) &recvaddr, &len);
					buffer[n] = '\0';
					if(strcmp(buffer, msgRequest) == 0){
						continue;
					} 
					printf("opponent client : %s\n", buffer);
					int i=0;
					while(1){
						if(buffer[i] == ' '){
							break;
						}
						i++;
					}
					int j=0;
					while(1){
						if(buffer[j] == ' '){
							break;
						}
						j++;
					}
					buffer[j] = '\0';
					OpponentPort = atoi(&buffer[i+1]);

					connectTOopponent(OpponentPort);

					close(sockfdrecv);
					gamedone = 1;
		            break;

			    }
			    if(FD_ISSET(sockfdrecvTCP, &read_fds)){
		            connfd = accept(sockfdrecvTCP, (struct sockaddr*)&recvaddrTCP, &len); 
	
	                bzero(buffer, sizeof(buffer)); 
	      
	                recv(connfd, buffer, sizeof(buffer), 0); 
	                printf("opponent client : %s", buffer);
	                if(strcmp(buffer, "Start Game") == 0){
	                	close(sockfdrecvTCP);
	                	playGameAsServer(connfd);
	                }

		            close(connfd);
		            gamedone = 1;
		            break;
			    }

			    if(gamedone)
			    	break;


			}

			if(gamedone)
			    break;


		    firstConnectioToServer--;
		}


		if(waitingForGame){
			if ( (sockfdlisten = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
				perror("socket creation failed"); 
				exit(EXIT_FAILURE); 
			}
			if (setsockopt(sockfdlisten, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		        perror("setsockopt");
		        exit(1);
		    }
			if ( bind(sockfdlisten, (const struct sockaddr *)&listenaddr, sizeof(listenaddr)) < 0 ) { 
				perror("bind failed"); 
				exit(EXIT_FAILURE); 
			} 
			if ((listen(sockfdlisten, 5)) != 0) { 
		        printf("Listen failed...\n"); 
		        exit(0); 
		    }
		    FD_SET(sockfdlisten, &listen_fds);
		    fdmax = sockfdlisten;
		    if (select(fdmax+1, &listen_fds, NULL, NULL, NULL) == -1) {
		        perror("select");
		        exit(4);
		    }
		    if (FD_ISSET(sockfdlisten, &listen_fds)) {

				connclient = accept(sockfdlisten, (struct sockaddr*)&listenaddr, &len);
		    	close(sockfdlisten);
		    	bzero(buffer, sizeof(buffer));
				m = recv(connclient, buffer, 80, 0);
				
				if(strcmp(buffer, "Start Game") == 0){
					printf("opponent Client : %s\n", buffer);
					playGameAsServer(connclient);
				}
				else if(buffer[0] == '0' || buffer[0] == '1' || buffer[0] == '2' || buffer[0] == '3' || buffer[0] == '4' || buffer[0] == '5' || buffer[0] == '6' || buffer[0] == '7' || buffer[0] == '8' || buffer[0] == '9'){
					printf("Server : opponent port is %s\n", buffer);
					OpponentPort = atoi(buffer);
					connectTOopponent(OpponentPort);
				}
				close(connclient);

				break;
		    }

			waitingForGame = 0;
		}
		

	}

	
	close(sockfd); 
	return 0; 
} 


void connectTOopponent(unsigned short OpponentPort){

	int sockfd;
	int connfd;
	struct sockaddr_in	 opponentAddr;
	char buffer[MAXLINE]; 
	char *startGame = "Start Game";
	fd_set read_fds;
	FD_ZERO(&read_fds);
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	}
	memset(&opponentAddr, 0, sizeof(opponentAddr));  
	opponentAddr.sin_family = AF_INET; 
	opponentAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	opponentAddr.sin_port = htons(OpponentPort);

	if (connect(sockfd, (struct sockaddr*)&opponentAddr, sizeof(opponentAddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
	} 
	int t = send(sockfd, (const char *)startGame, strlen(startGame), 0);
	playGameAsClient(sockfd);

   
}


int isGameLost(char map[10][10]){
	for(int i=0; i<10; i++){
		for(int j=0; j<10; j++){
			if(map[i][j] == '1'){
				return 0;
			}
		}
	}
	return 1;
}

char sendMove(int connclient){
	int x,y;
    char buffer[5];
    printf("Enter the coordinates(zero based) to attack : ");
	scanf("%d %d", &x, &y);
	char coordinates[4];
	coordinates[0] = (char)(x + '0');
	coordinates[1] = ' ';
	coordinates[2] = (char)(y + '0');
	coordinates[3] = '\0';
	send(connclient, (const char *)coordinates, strlen(coordinates), 0);
	int n = recv(connclient, buffer, 5, 0);
	buffer[n] = '\0';
	if(strcmp(buffer, "Hit") == 0){
		printf("You Hit\n");
		return 'H';
	}
	if(strcmp(buffer, "Miss") == 0){
		printf("You Miss\n");
		return 'M';
	}
	if(strcmp(buffer, "Win") == 0){
		return 'W';
	}
}

char recvMove(int connclient, char map[10][10]){
	char retvalue[5];
    char buffer[5];
    recv(connclient, buffer, 4, 0);
	int coordinates[2];
	coordinates[0] = (int)(buffer[0] - '0');
	coordinates[1] = (int)(buffer[2] - '0');
	if(map[coordinates[0]][coordinates[1]] == '1'){
		map[coordinates[0]][coordinates[1]] = '0';
		if(isGameLost(map)){
			strcpy(retvalue, "Win");
			retvalue[3] = '\0';
		}
		else{
			strcpy(retvalue, "Hit");
			retvalue[3] = '\0';
			printf("Opponent Hit\n");
		}
	}
	else if(map[coordinates[0]][coordinates[1]] == '0'){
		strcpy(retvalue, "Miss");
		retvalue[4] = '\0';
		printf("Opponent Miss\n");
	}
	send(connclient, (const char *)retvalue, strlen(retvalue), 0);
	if(retvalue[0] == 'M')
		return 'H';
	if(retvalue[0] == 'H')
		return 'M';
	if(retvalue[0] == 'W')
		return 'L';
}


void playGameAsClient(int connclient){
	char map[10][10];
	char *c = (char *) calloc(200, sizeof(char));
	int fd = open(mapFile, O_RDONLY);
	if (fd == -1) 
    { 
        printf("Error Number % d\n", errno);        
        perror("Program");                  
    } 
    int sz = read(fd, c, 200);
    c[sz] = '\0'; 
    int index = 0;
    for(int i=0; i<10; i++){
    	for(int j=0; j<10; j++){
    		map[i][j] = c[index];
    		index++;
    		index++;
    	}
    }
    if (close(fd) < 0)  
    { 
        perror("c1"); 
        exit(1); 
    }
//----------------------------------------------------
    char status = 'M';
    int isNotFinished = 1;
    while(isNotFinished == 1){
    	while(status == 'M'){
	    	status = recvMove(connclient, map);
		}
	    while(status == 'H'){
	    	status = sendMove(connclient);
		}
		if(status == 'W' || status == 'L'){
			break;
		}
	}
	if(status == 'W'){
		printf("You Won!!!\n");
	}
	if(status == 'L'){
		printf("You Lost!!!\n");	
	}
}

void playGameAsServer(int connclient){
	char map[10][10];
	char *c = (char *) calloc(200, sizeof(char));
	int fd = open(mapFile, O_RDONLY);
	if (fd == -1) 
    { 
        printf("Error Number % d\n", errno);        
        perror("Program");                  
    } 
    int sz = read(fd, c, 200);
    c[sz] = '\0'; 
    int index = 0;
    for(int i=0; i<10; i++){
    	for(int j=0; j<10; j++){
    		map[i][j] = c[index];
    		index++;
    		index++;
    	}
    }
    if (close(fd) < 0)  
    { 
        perror("c1"); 
        exit(1); 
    }
//----------------------------------------------------
    char status = 'H';
    int isNotFinished = 1;
    while(isNotFinished == 1){
	    while(status == 'H'){
	    	status = sendMove(connclient);
		}
		while(status == 'M'){
	    	status = recvMove(connclient, map);
		}
		if(status == 'W' || status == 'L'){
			break;
		}
	}
	if(status == 'W'){
		printf("You Won!!!\n");
	}
	if(status == 'L'){
		printf("You Lost!!!\n");	
	}
}
