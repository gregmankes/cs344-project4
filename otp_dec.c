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


void sendFile(int fd, int sockfd) {
	// keep track of how many bytes read and wrote
	int nread;
	int nwrite;
	// create a buffer for sending/receiving
	char buffer[1024];
	// send the file
	while (1) {
		// grab data from the file
		nread = read(fd, buffer, sizeof(buffer));
		if (nread == 0) {
			// done, close fd
			close(fd);
			break;
		}
		//send the chunk
		for (int i = 0; i < nread; i += nwrite) {
			//keep sending chunks until all sent
			nwrite = write(sockfd, buffer + i, nread - i);
			if (nwrite < 0) {
				fprintf(stderr, "Error writing to socket\n");
				exit(1);
			}
		}
	}
	memset(buffer, '\0', sizeof(buffer));
	//read the confirmation from daemon
	nread = read(sockfd, buffer, sizeof(buffer));
}

int handshake(int sockfd){
	//	printf("Verifying identity with daemon\n");
	char * identity = "opt_dec";
	send(sockfd, identity, strlen(identity),0);
	// get the response from the server
	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	//printf("Receiving response from daemon\n");
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
	//printf("Receiving the file\n");
	for(; i< message_length; i+= nread){
		nread = read(new_fd, to_receive + i, message_length -i);
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
	//printf("Getting file and key length\n");
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
	// Sending the length of the file and echoing back
	send(sockfd, file_length_s, strlen(file_length_s), 0);
	recv(sockfd, file_length_s, sizeof(file_length_s), 0);
	// sending the length of the key and echoing back
	send(sockfd, key_length_s, strlen(key_length_s), 0);
	recv(sockfd, key_length_s, sizeof(key_length_s), 0);
	int filefd = open(filename, O_RDONLY);
	sendFile(filefd, sockfd);
	int keyfd = open(keyname,O_RDONLY);
	sendFile(keyfd, sockfd);
	close(filefd);
	close(keyfd);
	char * encrypted = recv_file(sockfd, file_length);
	printf("%s", encrypted);
	free(encrypted);
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
	int fd = open(argv[1], O_RDONLY);
	if(fd < 0){
		fprintf(stderr, "There was an error opening %s\n", argv[1]);
		exit(1);
	}
	check_file_and_get_length(fd);
	close(fd);
	fd = open(argv[2], O_RDONLY);
	if(fd < 0){
		fprintf(stderr, "There was an error opening %s\n", argv[2]);
		exit(1);
	}
	check_file_and_get_length(fd);
	close(fd);
	struct addrinfo * res = create_address_info(argv[3]);
	int sockfd = create_socket(res);
	connect_socket(sockfd, res);
	handle_request(sockfd, argv[1], argv[2]);
	freeaddrinfo(res);
	close(sockfd);
	exit(0);
}

