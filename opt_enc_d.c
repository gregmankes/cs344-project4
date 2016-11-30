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
 * struct addrinfo * create_address_info_with_ip(char*, char*)
 * 
 * creates a pointer to an address info linked list with a address and port
 * Args: two strings: the address and port number
 * Returns: An address info linked list
 ******************************************************************************/
struct addrinfo * create_address_info_with_ip(char * ip_address, char * port){
	int status;
	struct addrinfo hints;
	struct addrinfo * res;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(ip_address, port, &hints, &res)) != 0){
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

/*******************************************************************************
 * char ** create_string_array(int)
 * 
 * Creates a string array on the heap used for files in directory
 * Args: an integer number of files
 ******************************************************************************/
char ** create_string_array(int size){
	char ** array = malloc(size*sizeof(char *));
	int i = 0;
	for(; i < size; i++){
		array[i] = malloc(100*sizeof(char));
		memset(array[i],0,sizeof(array[i]));
	}
	return array;
}

/*******************************************************************************
 * void delete_string_array(char**, int)
 * 
 * deletes the string array on the heap
 * Args: the string array and how many files it had
 ******************************************************************************/
void delete_string_array(char ** array, int size){
	int i = 0;
	for (; i < size; i++){
		free(array[i]);
	}
	free(array);
}


/*******************************************************************************
 * int get_available files(char **)
 * 
 * Counts how many files are in the directory and places them into the string
 * array. Source is below
 * http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program 
 * Args: the string array 
 ******************************************************************************/
int get_available_files(char ** files){
	DIR * d;
	struct dirent * dir;
	d = opendir(".");
	int i = 0;
	if (d){
		while ((dir = readdir(d)) != NULL){
			if (dir->d_type == DT_REG){
				strcpy(files[i], dir->d_name);
				i++;
			}
		}
		closedir(d);
	}
	return i;
}

/*******************************************************************************
 * int does_file_exist(char **, int, char *)
 * 
 * Checks if file exists in file array
 * Args: the file array, the number of files, and the filename
 ******************************************************************************/
int does_file_exist(char ** files, int num_files, char * filename){
	int file_exists = 0;
	int i = 0;
	// loop through files, if the filename we want is in there, set the file_exists
	// to be 1
	for (; i < num_files; i++){
		if(strcmp(files[i], filename) == 0){
			file_exists = 1;
		}
	}
	return file_exists;
}

/******************************************************************************
 * http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
 ******************************************************************************/
/* void send_file(int data_socket,  char * filename){ */
/* 	// create a buffer for file contents */
/* 	char buffer[1000]; */
/* 	// make sure its clean */
/* 	memset(buffer, 0, sizeof(buffer)); */
/* 	// open the file */
/* 	int fd = open(filename, O_RDONLY); */
/* 	while (1) { */
/* 		// Read data into buffer.  We may not have enough to fill up buffer, so we */
/* 		// store how many bytes were actually read in bytes_read. */
/* 		int bytes_read = read(fd, buffer, sizeof(buffer)-1); */
/* 		if (bytes_read == 0) // We're done reading from the file */
/* 			break; */

/* 		if (bytes_read < 0) { */
/* 			fprintf(stderr, "Error reading file\n"); */
/* 			return; */
/* 		} */

/* 		// You need a loop for the write, because not all of the data may be written */
/* 		// in one call; write will return how many bytes were written. p keeps */
/* 		// track of where in the buffer we are, while we decrement bytes_read */
/* 		// to keep track of how many bytes are left to write. */
/* 		void *p = buffer; */
/* 		while (bytes_read > 0) { */
/* 			int bytes_written = send(data_socket, p, sizeof(buffer),0); */
/* 			if (bytes_written < 0) { */
/* 				fprintf(stderr, "Error writing to socket\n"); */
/* 				return; */
/* 			} */
/* 			bytes_read -= bytes_written; */
/* 			p += bytes_written; */
/* 		} */
/* 		// clear out the buffer just in case */
/* 		memset(buffer, 0, sizeof(buffer)); */
/* 	} */
/* 	// clear out the buffer and send the done message */
/* 	memset(buffer, 0, sizeof(buffer)); */
/* 	strcpy(buffer, "__done__"); */
/* 	send(data_socket, buffer, sizeof(buffer),0); */
/* 	// close the socket and free the address info */
/* 	close(data_socket); */
/* } */

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
}

int handshake(int new_fd){
	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	printf("Verifying the client\n");
	recv(new_fd, buffer, sizeof(buffer),0);
	if(strcmp(buffer, "opt_enc") == 0){
		return 1;
	}
	return 0;
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

void encrypt(char * message, char * key, int message_length){
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
	char buffer[1000];
	memset(buffer, 0, sizeof(buffer));
	printf("getting the length of the file\n");
	recv(new_fd, buffer, sizeof(buffer), 0);
	int message_length = atoi(buffer);
	// send the length of the file back
	printf("sending back the length of the file\n");
	send(new_fd, buffer, strlen(buffer),0);
	// get the length of the key
	memset(buffer, 0, sizeof(buffer));
	printf("getting the length of the key\n");
	recv(new_fd, buffer, sizeof(buffer), 0);
	int key_length = atoi(buffer);
	// send the length of the key back
	printf("Sending the length of the key back");
	send(new_fd, buffer, strlen(buffer),0);
	// get the message
	char * message = recv_file(new_fd, message_length);
	// get the key
	char * key = recv_file(new_fd, key_length);
	encrypt(message, key, message_length);
	// free the key and message
	free(key);
	free(message);
	_Exit(0);
}
/* void handle_request(int new_fd){ */
/* 	// get the port number the client is expecting for a data connection */
/* 	char * ok_message = "ok"; */
/* 	char * bad_message = "bad"; */
/* 	char port[100]; */
/* 	memset(port, 0, sizeof(port)); */
/* 	recv(new_fd, port, sizeof(port)-1, 0); */
/* 	// send ok message to fix bug with python sending soo much info */
/* 	send(new_fd, ok_message, strlen(ok_message),0); */
/* 	// get the command from the client */
/* 	char command[100]; */
/* 	memset(command,0,sizeof(command)); */
/* 	recv(new_fd, command, sizeof(command)-1, 0); */
/* 	// send ok message to fix bug with python sending soo much info */
/* 	send(new_fd, ok_message, strlen(ok_message),0); */
/* 	// receive the ip of the client */
/* 	char ip_address[100]; */
/* 	memset(ip_address,0,sizeof(ip_address)); */
/* 	recv(new_fd, ip_address, sizeof(ip_address)-1,0); */
/* 	// print that we have a connection */
/* 	printf("Incoming connection from %s\n", ip_address); */
/* 	// if the command was -l: */
/* 	if(strcmp(command,"l") == 0){ */
/* 		// send an ok message to let the client know that the command was ok */
/* 		send(new_fd, ok_message, strlen(ok_message),0); */
/* 		printf("File list requested on port %s\n", port); */
/* 		printf("Sending file list to %s on port %s\n", ip_address, port); */
/* 		// create a string array for the files in the dir */
/* 		char ** files = create_string_array(100); */
/* 		// get those files */
/* 		int num_files = get_available_files(files); */
/* 		// send the contents of the file array to the client */
/* 		send_directory(ip_address, port, files, num_files); */
/* 		// free up the space from the file array */
/* 		delete_string_array(files,100); */
/* 	} */
/* 	else if(strcmp(command, "g") == 0){ */
/* 		// else if the command was -g: */
/* 		// send an ok message to let the client know that the command was ok */
/* 		send(new_fd, ok_message, strlen(ok_message),0); */
/* 		// get the file name from the client */
/* 		char filename[100]; */
/* 		memset(filename, 0, sizeof(filename)); */
/* 		recv(new_fd, filename, sizeof(filename)-1,0); */
/* 		printf("File: %s requested on port %s\n", filename, port); */
/* 		// create a list of files and check the file exists */
/* 		char ** files = create_string_array(100); */
/* 		int num_files = get_available_files(files); */
/* 		int file_exists = does_file_exist(files, num_files, filename); */
/* 		if(file_exists){ */
/* 			// if the file exists, let the user and client know */
/* 			printf("File found, sending %s to client\n", filename); */
/* 			char * file_found = "File found"; */
/* 			send(new_fd, file_found, strlen(file_found),0); */
/* 			// create a new filename based on current location */
/* 			char new_filename[100]; */
/* 			memset(new_filename,0,sizeof(new_filename)); */
/* 			strcpy(new_filename, "./"); */
/* 			char * end = new_filename + strlen(new_filename); */
/* 			end += sprintf(end, "%s", filename); */
/* 			// send that file to the client */
/* 			send_file(ip_address, port, new_filename); */
/* 		} */
/* 		else{ */
/* 			// else the file was not found, send that to the client */
/* 			printf("File not found, sending error message to client\n"); */
/* 			char * file_not_found = "File not found"; */
/* 			send(new_fd, file_not_found, 100, 0); */
/* 		} */
/* 		delete_string_array(files, 100); */
/* 	} */
/* 	else{// else the command wasn't found, send that to the client */
/* 		send(new_fd, bad_message, strlen(bad_message), 0); */
/* 		printf("Invalid command sent\n"); */
/* 	} */
/* 	// print this to notify user that we have finished processing request */
/* 	printf("Continuing to wait for incoming connections\n"); */
/* } */

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
		new_fd = accept(sockfd, (struct addrinfo *)&their_addr, &addr_size);
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

