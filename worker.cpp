#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <crypt.h>
#include <unistd.h>
#include "structs.h"

using namespace std;

int server_sockfd;
bool cancel_thread = false; // Indicates to a thread if server has sent new query (Some other workerprobably found a solution)

bool send_server(string st) // send string st to server
{
	char *data = new char[st.length()+1];
	strcpy(data,st.c_str()); data[st.length()] = '\0';
	int len = strlen(data);
	int bsent = send(server_sockfd, data, len, 0);
	if(bsent != len)
	{
		perror("Sorry, the whole message wasn't sent!");
		return false;
	}
	return true;
}

bool send_to_server(string s) // prepends '-' to s to make it of length 8 and then sends it to server
{
	int len = s.length();
	string st = "";
	for (int i = 0; i < 8-len; ++i)
	{
		st += '-';
	}
	st += s;

	return send_server(st);
}

bool next_string(string&curr, bool lower, bool upper, bool dig, char start, char end) { 
	// Modifies curr to the next lexographic string of same length 
	// the flags lower, upper and dig specify the space of characters over which the strings are formed
	// Returns false if the next string's first character doesnt lie between start and end; true otherwise
	string iteratorstr = ""; // contains the space of permissible characters
	string lowercase = "abcdefghijklmnopqrstuvwxyz";
	string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string digits = "0123456789";
	if(lower) {
		iteratorstr += lowercase;
	}
	if(upper) {
		iteratorstr += uppercase;
	}
	if(dig) {
		iteratorstr += digits;
	}
	int itlen = iteratorstr.length();
	int len = curr.length();
	int j = len-1;
	while(true)
	{
		int k = iteratorstr.find(curr[j]);
		if(k<itlen-1)
		{
			if(j == 0 and curr[j] == end)
			{
				// next string's first character doesnt lie between start and end
				return false;
			}
			curr[j] = iteratorstr[k+1];
			for(int i = j+1; i<len; i++)
			{
				curr[i] = iteratorstr[0];
			}
			return true;
		}
		j--;
		if(j < 0)
		{
			// Just an extra check for error handling
			return false;
		}
	}
}

void *hashbreaker(void *attr) // Thread function
{
	workdata *w1 = (workdata*)attr;
	workdata w = *w1;
	bool lower = w.hash_info.lowercase;
	bool upper = w.hash_info.uppercase;
	bool dig = w.hash_info.digits;
	char start = w.start;
	char end = w.end;
	char *salt = new char[3]; // salt = first 2 characters of hash
	salt[0] = w.hash_info.hash[0];
	salt[1] = w.hash_info.hash[1];
	salt[3] = '\0';
	string iteratorstr = ""; // contains the space of permissible characters
	string lowercase = "abcdefghijklmnopqrstuvwxyz";
	string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string digits = "0123456789";
	if(lower) {
		iteratorstr += lowercase;
	}
	if(upper) {
		iteratorstr += uppercase;
	}
	if(dig) {
		iteratorstr += digits;
	}

	string curr;
	//initialize curr string
	curr += w.start; 
	for(int i = 1; i<w.hash_info.len; i++)
	{
		curr += iteratorstr[0];
	}

	if(cancel_thread) //Exit thread if cancel_thread is true
	{
		cancel_thread = false;
		pthread_exit(NULL);
	}
	if(strcmp(crypt(curr.c_str(),salt),w.hash_info.hash)==0)
	{
		// Successfully decrypted hash
		send_to_server(curr); // send hash to server and exit
		cancel_thread = false;
		pthread_exit(NULL);
	}

	while(next_string(curr,lower,upper,dig,start,end))
	{
		if(cancel_thread) //Exit thread if cancel_thread is true
		{
			cancel_thread = false;
			pthread_exit(NULL);
		}
		if(strcmp(crypt(curr.c_str(),salt),w.hash_info.hash)==0)
		{
			// Successfully decrypted hash
			send_to_server(curr); // send hash to server and exit
			cancel_thread = false;
			pthread_exit(NULL);
		}
	}
	// send "not found" sequence ('--------') to server
	send_to_server("--------");
	delete []salt; //deallocate memory and exit
	cancel_thread = false;
	pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
	const char *dest_ip;
	int dest_port;
	if(argc != 3) {
		perror("Invalid number of arguments");
		exit(1);
	}
	dest_ip = argv[1];
	if(isInteger(argv[2]))
	{
		dest_port = stoi(argv[2]);
	} else
	{
		perror("Port must be an Integer\n");
		exit(1);
	}
	
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET; // host byte order works for this
	dest_addr.sin_port = htons(dest_port); // convert to network byte order, short
	if(inet_aton(dest_ip, &(dest_addr.sin_addr)) == 0) { // can also pick up ip directly from machine: Beej pg 14
		perror("Error on assigning IP address to socket");
      	exit(1);
	}
	memset(&(dest_addr.sin_zero), '\0', 8); // zero the rest of the struct

	int my_sockfd = socket(AF_INET, SOCK_STREAM, 0); // setting up socket fd
	if (my_sockfd < 0) {
		perror("Error on opening socket");
		exit(1);
	}

	server_sockfd = my_sockfd;

	if(connect(my_sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) < 0) { // binding socket and address 
		perror("Could not connect to destination");
		exit(1);
	}
	int yes = 1;
	if (setsockopt(my_sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) { // remove the address already in use
		perror("setsockopt: Unable to unbind sockets");
		exit(1);
	}

	if(!send_server("work"))
	{
		perror("Handshake with server uncessful");
		exit(1);
	}

	char buf[256];
	pthread_t thread = 0;
	if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0){
		perror("Error while setting thread cancel state");
	}
	while (true)
	{
		int nbytes = recv(my_sockfd, buf, sizeof(buf), 0);
		//ERROR CHECKING
		if(nbytes == 0)
		{
			perror("Connection Closed!");
			exit(1);
		}
		if(nbytes < 0)
		{
			perror("Error on receiving");
			exit(1);
		}
		buf[nbytes] = '\0';
		string s(buf);
		workdata w = deserializew(s);
		cancel_thread = true;
		
		unsigned int microseconds = 1000;
		usleep(microseconds);

		cancel_thread = false;
		int rc = pthread_create(&thread, NULL, hashbreaker, (void *) &w);
		//ERROR CHECKING
		if(rc!=0)
		{
			perror("Error on creating thread");
		}
	}

	pthread_exit(NULL); //wait for threads to exit
	return 0;
}
