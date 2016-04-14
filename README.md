Sumith | Shubham Goel
140050081 | 140050086

## List of relevant files

`server.cpp` - contains code of the server <br>
`user.cpp` - contains code of the user <br>
`worker.cpp` - contains code of the worker <br>

`structs.h` - contains structs and helper functions used throughout the other files
some structs and functions are used in all three files,
instead of having redundant code, we put them in one file. <br>

`results.jpg` - required image graph of evaluation <br>
`makefile` - makefile of the project, instructions below

## Running instructions

```
$ make
```
The above generates the executables `server`, `user` and `worker` 
for server, user and worker respectively.
```
$ make clean
```
The above cleans the directory and returns it back to the 
original state,

```
$ ./server <port-no>
```
The above starts a server at the specified port number.

```
$ ./worker <server-ip> <port-no>
```
The above starts a worker with destination ip and port specified.

```
$ ./user <server ip/host-name> <server-port> <hash> <passwd-length> <binary-string>
```
The above executes a user with instructions as specified in the problem
statement.

Use `crypt("<string>", "<salt>")` to get the hash, <passwd-length> is the length of
the password string. <binary-string> is a 3 bit binary string, where the three bits 
indicate the use of lower case, upper case and numerical characters in the password. 
For example, 001 indicates the password is made up of only numbers; 111 indicates 
the password can contain lower, upper as well as numerical characters.
