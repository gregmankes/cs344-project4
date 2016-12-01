#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char * argv[]){
	if(argc != 2){
		fprintf(stderr, "Incorrect number of arguments\nUsage: keygen <keylength>");
		exit(1);
	}
	int key_length = atoi(argv[1]);
	srand(time(0));
	int i = 0;
	int key;
	for(; i < key_length; i++){
		key = rand() % 27;
		if(key != 26){
			printf("%c", 'A'+(char)key);
		}
		else{
			printf(" ");
		}
	}
	return 0;
}
