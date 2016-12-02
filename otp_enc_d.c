#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

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

/*******************************************************************************
 * void bind_socket(int, struct addrinfo *)
 * 
 * Binds the socket to a port
 * Args: a socket file descriptor and an address info linked list
 ******************************************************************************/
void bind_socket(int sockfd, struct addrinfo * res){
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		close(sockfd);
		fprintf(stderr, "Error in binding socket\n");
		exit(1);
	}
}

/*******************************************************************************
 * void listen_socket(int)
 * 
 * listens on the bound port
 * Args: a socket file descriptor
 ******************************************************************************/
void listen_socket(int sockfd){
	if(listen(sockfd, 5) == -1){
		close(sockfd);
		fprintf(stderr, "Error in listening on socket\n");
		exit(1);
	}
}


void send_file(int new_fd, const char * message, int message_length){
	int nwrote = 0;
	int i = 0;
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

int handshake(int new_fd){
	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	recv(new_fd, buffer, sizeof(buffer),0);
	if(strcmp(buffer, "opt_enc") == 0){
		return 1;
	}
	return 0;
}

void recv_file(int new_fd, int message_length, char * to_receive){
	int nread = 0;
	int i = 0;
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
}

void encrypt_message(char * message, char * key, int message_length){
	int i = 0;
	int message_num;
	int key_num;
	int result_num;
	int alphabet = 27; // because of newline
	for (; i < message_length; i++){
		// ignore newlines
		if (message[i] != '\n'){
			// convert the characters of key[i] and message[i]
			// to the range of 0 to 26
			if(message[i] == ' '){
				message_num = 26;
			}
			else{
				message_num = message[i] -'A';
			}
			if(key[i] == ' '){
				key_num = 26;
			}
			else{
				key_num = key[i] - 'A';
			}
			// once converted, add them and mod by the alphabet
			result_num = (message_num + key_num) % alphabet;
			// place the result in the string.
			if(result_num == 26){
				message[i] = ' ';
			}
			else{
				message[i] = 'A' + (char)result_num;
			}
		}
	}
}

/*******************************************************************************
 * void handle_request(int)
 * 
 * Handles the request from the client
 * Args: the newly created socket from the request
 ******************************************************************************/
void handle_request(int new_fd){
	int correct_client = handshake(new_fd);
	if (!correct_client){
		fprintf(stderr, "Invalid Client\n");
		char invalid[] = "Invalid";
		send(new_fd, invalid, strlen(invalid),0);
		_Exit(2);
	}
	char valid[] = "Valid";
	send(new_fd, valid, strlen(valid), 0);
	// get the length of how long the file is
	char buffer[10];
	memset(buffer, 0, sizeof(buffer));
	recv(new_fd, buffer, sizeof(buffer), 0);
	int message_length = atoi(buffer);
	// send the length of the file back
	send(new_fd, buffer, strlen(buffer),0);
	// get the length of the key
	memset(buffer, 0, sizeof(buffer));
	recv(new_fd, buffer, sizeof(buffer), 0);
	int key_length = atoi(buffer);
	// send the length of the key back
	send(new_fd, buffer, strlen(buffer),0);
	// get the message
	char * message = (char *)malloc((long)message_length);
	memset(message, '\0', sizeof(message));
	recv_file(new_fd, message_length, message);
	// get the key
	char * key = (char *)malloc((long)key_length);
	memset(key, '\0', sizeof(key));
	recv_file(new_fd, key_length, key);
	sleep(1);
	encrypt_message(message, key, message_length);
	// send back the file
	send_file(new_fd, message, message_length);
	// free the key and message
	//free(message);
	//free(key);
	exit(0);
}


/*******************************************************************************
 * void wait_for_connection
 * 
 * waits for a new connection to the server
 * Args: the file descriptor to wait on
 ******************************************************************************/
void wait_for_connection(int sockfd){
	// create a container for the connection
	struct sockaddr_storage their_addr;
	// create a size for the connection
    socklen_t addr_size;
	// create a new file descriptor for the connection
	int new_fd;
	// status variable
	int status;
	// pid variable;
	pid_t pid;
	// run forever
	while(1){
		// get the address size
		addr_size = sizeof(their_addr);
		// accept a new client
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
		// if there is no new client keep waiting
		if(new_fd == -1){
			fprintf(stderr, "Error in accepting connection\n");
			continue;
		}
		// fork to let a new process handle the new socket
		pid = fork();
		// if there was an error, say so
		if(pid == -1){
			fprintf(stderr, "Error in fork\n");
		}
		else if(pid == 0){
			// child process
			close(sockfd);
			handle_request(new_fd);
			close(new_fd);
		}
		else{
			// parent process
			close(new_fd);
			while (pid > 0){
				pid = waitpid(-1, &status, WNOHANG);
			}
		}
	}
}

/*******************************************************************************
 * int main(int, char*)
 * 
 * main method. sets up server socket, command line args and calls 
 * wait_for_connection
 * Args: the command lin args
 ******************************************************************************/
int main(int argc, char *argv[]){
	if(argc != 2){
		fprintf(stderr, "Invalid number of arguments\n");
		exit(1);
	}
	printf("Server open on port %s\n", argv[1]);
	struct addrinfo * res = create_address_info(argv[1]);
	int sockfd = create_socket(res);
	bind_socket(sockfd, res);
	listen_socket(sockfd);
	wait_for_connection(sockfd);
	freeaddrinfo(res);
}

