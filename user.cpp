#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include "structs.h"

#define ARGC 6
#define HASH_SIZE 13

using namespace std;

int main(int argc, char const *argv[]) {
	const char *dest_ip;
	int dest_port;

	userdata hash_info;
	if(argc != ARGC) { // check the number of arguments, it should be equal to defined ARGC
		perror("Invalid number of arguments");
		return -1;
	} else { // the following code parses of command line arguments and populates the struct
		dest_ip = argv[1];
		if(isInteger(argv[2]))
		{
			dest_port = stoi(argv[2]);
		} else
		{
			perror("Port must be an Integer\n");
			return -1;
		}
		if(strlen(argv[3]) != HASH_SIZE) {
			perror("Invalid length of hash");
			return -1;
		} else {
			strcpy(hash_info.hash, argv[3]); hash_info.hash[13] = '\0';
		}
		if(isInteger(argv[4]))
		{
			hash_info.len = stoi(argv[4]);
		} else
		{
			perror("Length must be an Integer\n");
			return -1;
		}
		if(strlen(argv[5]) != 3) {
			perror("Invalid number of flags");
			return -1;
		} else {
			if(argv[5][0] == '1') {
				hash_info.lowercase = true;
			} else if(argv[5][0] == '0') {
				hash_info.lowercase = false;
			} else {
				perror("Invalid flag");
				return -1;
			}
			if(argv[5][1] == '1') {
				hash_info.uppercase = true;
			} else if(argv[5][1] == '0') {
				hash_info.uppercase = false;
			} else {
				perror("Invalid flag");
				return -1;
			}
			if(argv[5][2] == '1') {
				hash_info.digits = true;
			} else if(argv[5][2] == '0') {
				hash_info.digits = false;
			} else {
				perror("Invalid flag");
				return -1;
			}
		}		
	}

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET; // host byte order works for this
	dest_addr.sin_port = htons(dest_port); // convert to network byte order, short
	if(inet_aton(dest_ip, &(dest_addr.sin_addr)) == 0) { // can also pick up ip directly from machine: Beej pg 14
		perror("Error on assigning IP address to socket");
      	return -1;
	}
	memset(&(dest_addr.sin_zero), '\0', 8); // zero the rest of the struct

	int my_sockfd = socket(AF_INET, SOCK_STREAM, 0); // setting up socket fd
	if (my_sockfd < 0) {
		perror("Error on opening socket");
		return -1;
	}

	if(connect(my_sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) < 0) { // binding socket and address 
		perror("Could not connect to destination");
		return -1;
	}
	int yes = 1;
	if (setsockopt(my_sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) { // remove the address already in use
		perror("setsockopt");
		return -1;
	}

	// send "user" to the server as first message to ground its identity
	string str = "user";
	char *data = new char[str.length()+1];
	strcpy(data,str.c_str()); data[str.length()] = '\0';
	int len = strlen(data);
	int bsent = send(my_sockfd, data, len, 0) ;
	if(bsent != len)
	{
		perror("Sorry, the whole message wasn't sent!");
		return -1;
	}

	// sleep for a minor time, to prevent the above message being sent 
	// together with the hash information
	unsigned int microseconds = 1000;
	usleep(microseconds);

	// serialize and send the hash informations
	str = serialize(hash_info);
	data = new char[str.length()+1];
	strcpy(data,str.c_str()); 
	data[str.length()] = '\0';
	len = strlen(data);

	// start the timer and send the data
	chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	if(send(my_sockfd, data, len, 0) < len){
		perror("Sorry, the whole message wasn't sent!");
		return -1;
	}

	// receive the result server sends
	char buf[256];
	int nbytes = recv(my_sockfd, buf, sizeof(buf), 0);
	buf[nbytes] = '\0';
	//strop the timer
	chrono::steady_clock::time_point end = chrono::steady_clock::now();
	double elapsed_secs = (chrono::duration_cast<chrono::microseconds>(end - start).count())/1000000.0;
	// print the result followed by time in separate lines
	// prints "Not Found" if unsuccessful
	cout<<buf<<endl<<elapsed_secs<<endl;
	return 0;
}
