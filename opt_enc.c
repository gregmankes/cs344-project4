#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

/*******************************************************************************
 * struct addrinfo * create_address_info(char*)
 * 
 * creates a pointer to an address info linked list with port
 * Args: two strings: the address and port number
 * Returns: An address info linked list
 ******************************************************************************/
struct addrinfo * create_address_info(char * port){
	int status;
	struct addrinfo hints;
	struct addrinfo * res;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(NULL, port, &hints, &res)) != 0){
		fprintf(stderr,
				"getaddrinfo error: %s\nDid you enter the correct IP/Port?\n",
				gai_strerror(status));
		exit(1);
	}
	
	return res;
}

/*******************************************************************************
 * int create_socket(struct addrinfo *)
 * 
 * Creates a socket from an address info linked list
 * Args: The address info linked list
 * Returns: a socket file descriptor
 ******************************************************************************/
int create_socket(struct addrinfo * res){
	int sockfd;
	if ((sockfd = socket((struct addrinfo *)(res)->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
		fprintf(stderr, "Error in creating socket\n");
		exit(1);
	}
	return sockfd;
}

/*******************************************************************************
 * void connect_socket(int, struct addrinfo *)
 * 
 * Connects the socket to the address specified in the address info linked list
 * Args: a socket file descriptor and an address info linked list
 ******************************************************************************/
void connect_socket(int sockfd, struct addrinfo * res){
	int status;
	if ((status = connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1){
		fprintf(stderr, "Error in connecting socket\n");
		exit(1);
	}
}



void send_file(int new_fd, char * message, int message_length){
	int nwrote = 0;
	int i = 0;
	printf("Sending file\n");
	for (; i < message_length; i+=nwrote){
		nwrote = write(new_fd, message, message + i);
		if(nwrote < 0){
			fprintf(stderr, "Error in writing to socket\n");
			_Exit(2);
		}
	}
	char buff[100];
	memset(buff, 0, sizeof(buff));
	recv(new_fd, buff, sizeof(buff), 0);
}

/* int handshake(int new_fd){ */
/* 	char buffer[100]; */
/* 	memset(buffer, 0, sizeof(buffer)); */
/* 	printf("Verifying the client\n"); */
/* 	recv(new_fd, buffer, sizeof(buffer),0); */
/* 	if(strcmp(buffer, "opt_enc") == 0){ */
/* 		return 1; */
/* 	} */
/* 	return 0; */
/* } */

int handshake(int sockfd){
	printf("Verifying identity with daemon\n");
	char * identity = "opt_enc";
	send(sockfd, identity, strlen(identity),0);
	// get the response from the server
	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	printf("Receiving response from daemon\n");
	recv(sockfd, buffer, sizeof(buffer),0);
	int to_return = 0;
	if(strcmp(buffer, "Valid") == 0){
		to_return = 1;
	}
	return to_return;
}

char * recv_file(int new_fd, int message_length){
	char * to_receive = malloc(message_length * sizeof(char));
	int nread = 0;
	int i = 0;
	printf("Receiving the file\n");
	for(; i< message_length; i+= nread){
		nread = read(new_fd, to_receive + i, message_length -1);
		if(nread < 0){
			fprintf(stderr, "Error in receiving file\n");
			_Exit(2);
		}
	}
	// echo finished response
	char * finished = "opt_enc_d f";
	send(new_fd, finished, strlen(finished),0);
	return to_receive;
}


/* /\******************************************************************************* */
/*  * void handle_request(int) */
/*  *  */
/*  * Handles the request from the client */
/*  * Args: the newly created socket from the request */
/*  ******************************************************************************\/ */
/* void handle_request(int new_fd){ */
/* 	int correct_client = handshake(new_fd); */
/* 	if (!correct_client){ */
/* 		fprintf(stderr, "Invalid Client\n"); */
/* 		char invalid[] = "Invalid"; */
/* 		send(new_fd, invalid, strlen(invalid),0); */
/* 		_Exit(2); */
/* 	} */
/* 	char valid[] = "Valid"; */
/* 	send(new_fd, valid, strlen(valid), 0); */
/* 	// get the length of how long the file is */
/* 	char buffer[1000]; */
/* 	memset(buffer, 0, sizeof(buffer)); */
/* 	printf("getting the length of the file\n"); */
/* 	recv(new_fd, buffer, sizeof(buffer), 0); */
/* 	int message_length = atoi(buffer); */
/* 	// send the length of the file back */
/* 	printf("sending back the length of the file\n"); */
/* 	send(new_fd, buffer, strlen(buffer),0); */
/* 	// get the length of the key */
/* 	memset(buffer, 0, sizeof(buffer)); */
/* 	printf("getting the length of the key\n"); */
/* 	recv(new_fd, buffer, sizeof(buffer), 0); */
/* 	int key_length = atoi(buffer); */
/* 	// send the length of the key back */
/* 	printf("Sending the length of the key back"); */
/* 	send(new_fd, buffer, strlen(buffer),0); */
/* 	// get the message */
/* 	char * message = recv_file(new_fd, message_length); */
/* 	// get the key */
/* 	char * key = recv_file(new_fd, key_length); */
/* 	encrypt(message, key, message_length); */
/* 	// send back the file */
/* 	send_file(new_fd, message, message_length); */
/* 	// free the key and message */
/* 	free(key); */
/* 	free(message); */
/* 	_Exit(0); */
/* } */

int check_file_and_get_length(int fd){
	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	while(read(fd, buffer, 1) != 0){
		if(((buffer[0] < 'A' || buffer[0] > 'Z') &&
			buffer[0] != ' ') && buffer[0] != '\n'){
			fprintf(stderr, "File contains invalid characters\n");
			exit(1);
		}
	}
	return lseek(fd, 0, SEEK_END);
}

void handle_request(int sockfd, char * filename, char * keyname){
	// begin by verifying identity
	int is_valid = handshake(sockfd);
	if(!is_valid){
		fprintf(stderr,"Daemon did not accept client\n");
		exit(1);
	}
	// open the files
	int file_fd = open(filename, O_RDONLY);
	int key_fd = open(keyname, O_RDONLY);
	if (key_fd < 1){
		fprintf(stderr, "Error opening key\n");
		exit(1);
	}
	if (file_fd < 1){
		fprintf(stderr, "Error opening file\n");
		exit(1);
	}
	printf("Getting file and key length\n");
	int file_length = check_file_and_get_length(file_fd);
	int key_length = check_file_and_get_length(key_fd);
	if(file_length > key_length){
		fprintf(stderr, "Error: Key is too short\n");
		exit(1);
	}
	close(key_fd);
	close(file_fd);
	// tell the daemon the size of the strings
	char file_length_s[20];
	memset(file_length_s, 0, sizeof(file_length_s));
	char key_length_s[20];
	memset(key_length_s, 0, sizeof(key_length_s));
	sprintf(file_length_s, "%d", file_length);
	sprintf(key_length_s, "%d", key_length);
	printf("Sending and receiving lengths of file and key to daemon\n");
	// Sending the length of the file and echoing back
	send(sockfd, file_length_s, strlen(file_length_s), 0);
	recv(sockfd, file_length_s, sizeof(file_length_s), 0);
	// sending the length of the key and echoing back
	send(sockfd, key_length_s, strlen(key_length_s), 0);
	recv(sockfd, key_length_s, sizeof(key_length_s), 0);
	// create the key and file strings
	char * message = malloc((file_length+1) * sizeof(char)+1);
	char * key = malloc((key_length+1) * sizeof(char));
	memset(message, 0, sizeof(message));
	memset(key, 0, sizeof(key));
	printf("Loading the file into memory\n");
	FILE * message_fd = fopen(filename, "r");
	fread(message, 1, file_length, message_fd);
	fclose(message_fd);
	printf("Loading the key into memory\n");
	FILE * key_file = fopen(keyname, "r");
	fread(key, 1, key_length, key_file);
	fclose(key_file);
	send_file(sockfd, message, file_length);
	send_file(sockfd, key, key_length);
	printf("Receiving the encrypted file\n");
	char * encrypted = recv_file(sockfd, file_length);
	printf("%s", encrypted);
	free(encrypted);
	free(message);
	free(key);
}

/*******************************************************************************
 * int main(int, char*)
 * 
 * main method. sets up server socket, command line args and calls 
 * handle_request
 * Args: the command lin args
 ******************************************************************************/
int main(int argc, char *argv[]){
	if(argc < 4){
		fprintf(stderr, "Invalid number of arguments\n");
		fprintf(stderr, "Usage: opt_enc filename keyname portnumber\n");
		exit(1);
	}
	struct addrinfo * res = create_address_info(argv[4]);
	int sockfd = create_socket(res);
	connect_socket(sockfd, res);
	handle_request(sockfd, argv[1], argv[2]);
	freeaddrinfo(res);
	close(sockfd);
}

