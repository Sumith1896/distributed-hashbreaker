#include <iostream>
#include <string>
#include <set>
#include <queue>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "structs.h"

#define BACKLOG 5

using namespace std;

string getpass(string str) { // strips string of leading '-'
	for(int i = 0; i < 8; i++) {
		if(str[i] != '-') {
			return str.substr(i);
		}
	}
	return "Error";
}

inline bool operator<(const workdata& lhs, const workdata& rhs) 
{
  return true;
}

set<workdata> distribute(userdata udata, int noofworkers) { 
	// Decides distribution of work among noofworkers workers
	// Returns a set - each element of which contains fork for a single worker

	set<workdata> result;
	string iteratorstr = "";
	string lowercase = "abcdefghijklmnopqrstuvwxyz";
	string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string digits = "0123456789";
	if(udata.lowercase) {
		iteratorstr += lowercase;
	}
	if(udata.uppercase) {
		iteratorstr += uppercase;
	}
	if(udata.digits) {
		iteratorstr += digits;
	}
	int len = iteratorstr.length();
	int approximate = len/noofworkers; // Approximate amout of work assigned to each worker
	int j = 0;
	for(int i = 0; i < noofworkers; i++) {
		workdata temp;
		temp.hash_info = udata;
		temp.start = iteratorstr[j];
		if(i != noofworkers - 1)
			j = j + approximate -1;
		else
			j = len - 1;
		temp.end = iteratorstr[j];
		j++;
		result.insert(temp);
	}
	return result;
}

int main(int argc, const char *argv[]) {

	struct sockaddr_in my_addr, user_addr;
	int my_port;

	fd_set master;
	set<int> users;
	set<int> workers;
	fd_set read_fds;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	int fdmax;

	int noofworkers = 0;

	if(argc != 2) {
		perror("Invalid number of arguments");
		return -1;
	} else {
		my_port = stoi(argv[1]);
	}

	my_addr.sin_family = AF_INET; // host byte order works for this
	my_addr.sin_port = htons(my_port); // convert to network byte order, short
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

	int my_sockfd = socket(AF_INET, SOCK_STREAM, 0); // setting up socket fd
	if (my_sockfd < 0) {
		perror("Error on opening socket");
		return -1;
	}

	int yes = 1; // remove the address already in use
	if (setsockopt(my_sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		return -1;
	}

	if(bind(my_sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) { // binding socket and address
		perror("Could not bind the socket");
		return -1;
	}

	if(listen(my_sockfd, BACKLOG) < 0) { //listening
		perror("Error on listening");
		return -1;
	}

	FD_SET(my_sockfd, &master);
	fdmax = my_sockfd;

	char buf[256];
	set<int> working;
	int working_sockfd = 0;
	queue<request> requests;
	int nbytes;

	for(;;) {
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) { // listen on all read_fds sockets
			perror("select");
			return -1;
		}
		// run through the existing connections looking for data to read
		for(int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // we got one!!
				if (i == my_sockfd) {
					// handle new connections
					socklen_t addrlen = sizeof(user_addr);
					int newfd;
					if ((newfd = accept(my_sockfd, (struct sockaddr *) &user_addr, &addrlen)) == -1) {
						perror("accept");
					} else {
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax) {
							// keep track of the maximum
							fdmax = newfd;
						}
						printf("server: new connection from %s on socket %d\n", inet_ntoa(user_addr.sin_addr), newfd);
					}
				} else {
					// handle data from a client
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
						// got error or connection closed by client
						if (nbytes == 0) {
							// connection closed
							printf("server: socket %d hung up\n", i);
							if(workers.find(i) != workers.end())
							{
								// printf("Worker hung up\n");
								workers.erase(i);
								noofworkers--;
							}
							if(users.find(i) != users.end())
							{
								// printf("User hung up\n");
								users.erase(i);

								if((i==working_sockfd) and !working.empty())
								{
									working.clear();
								}
							}
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
					} else {
						buf[nbytes] = '\0';
						string str(buf);
						if(str == "user") { // handshake
							users.insert(i);
							continue;
						} else if(str == "work") { // handshake
							workers.insert(i);
							noofworkers++;
							continue;
						}

						if(users.find(i) != users.end()) {
							request newreq;
							newreq.udata = deserializeu(str);
							newreq.sockfd = i;
							requests.push(newreq); //Add request to queue
						} else if(workers.find(i) != workers.end()) { 
							if(working.find(i) != working.end()) { // handle unexpected connection loss
								if(str == "--------") {
									working.erase(i);
									if(working.empty())
									{
										string ans = "Not Found";
										char *data = new char[ans.length()+1];
										strcpy(data,ans.c_str()); data[ans.length()] = '\0';
										int len = strlen(data);
										send(working_sockfd, data, len, 0);
										working.clear();
									}
								} else {
									string ans = getpass(str);
									char *data = new char[ans.length()+1];
									strcpy(data,ans.c_str()); data[ans.length()] = '\0';
									int len = strlen(data);
									send(working_sockfd, data, len, 0);
									working.clear();
								}
							}
						}

					}
				}
			}
		}
		if(working.empty()) {
			if(!requests.empty() and !workers.empty()) { 
				// Look at the first request in the queue and assign corresponding work to workers
				request nextjob = requests.front();
				requests.pop();
				working_sockfd = nextjob.sockfd;
				set<workdata> distributed = distribute(nextjob.udata, noofworkers); // distribute work
				set<workdata>::iterator it = distributed.begin();
				for(set<int>::iterator i = workers.begin(); i != workers.end(); i++) {
					if(it != distributed.end()) {
						string ser = serialize(*it);
						char *data = new char[ser.length()+1];
						strcpy(data,ser.c_str()); 
						data[ser.length()] = '\0';
						int len = strlen(data);
						send(*i, data, len, 0); // send assigned distributed work
						working.insert(*i); // maintaing set of working workers
						it++;
					}
				}
			}
		}
	}
	return 0;
}