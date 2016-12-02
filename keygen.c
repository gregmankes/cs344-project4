/******************************************************************************
* keygen.c
*
* Author: Greg Mankes
* Generates a key of specified length given over command line
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/******************************************************************************
* int main(int, char *)
*
* Main method
* args: command line arguments
*******************************************************************************/
int main(int argc, char * argv[]){
	// check the number of args
	if(argc != 2){
		fprintf(stderr, "Incorrect number of arguments\nUsage: keygen <keylength>");
		exit(1);
	}
	// get the key length
	int key_length = atoi(argv[1]);
	// seed random number generator
	srand(time(0));
	// set up loop var and key
	int i = 0;
	int key;
	// begin generating keys
	for(; i < key_length; i++){
		// get a random number
		key = rand() % 27;
		// if the number is not 26, mapp it to a char
		if(key != 26){
			printf("%c", 'A'+(char)key);
		}
		else{
			// if it is 26, it is a space
			printf(" ");
		}
	}
	// print a newline
	printf("\n");
	return 0;
}
