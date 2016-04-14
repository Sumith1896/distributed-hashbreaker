#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

struct userdata {
	char hash[14];
	bool lowercase, uppercase, digits;
	int len;
};

struct workdata {
	userdata hash_info;
	char start, end;
};

struct request {
	userdata udata;
	int sockfd;
};

userdata deserializeu(string s) //Decodes a string to return a userdata object
{
	userdata x;
	strcpy(x.hash, s.substr(0,13).c_str()); //first 13 characters of string form hash
	x.hash[13] = '\0'; 
	//next 3 characters are the flags
	x.lowercase = (s[13]=='1');
	x.uppercase = (s[14]=='1');
	x.digits = (s[15]=='1');
	//next character = length of password
	x.len = stoi(s.substr(16,1));
	return x;
}

string serialize(workdata dat) 
{
	//Encodes a workdata object into a string : <hash><flags><length><start><end>

	string r(dat.hash_info.hash);
	r = r+ ((dat.hash_info.lowercase)?'1':'0');
	r = r+ ((dat.hash_info.uppercase)?'1':'0');
	r = r+ ((dat.hash_info.digits)?'1':'0');
	if(dat.hash_info.len>9)
	{
		perror("Length is too long");
		r = r+ "1";
	}
	r = r+ (char)(dat.hash_info.len + (int)'0');
	r = r+ dat.start;
	r = r+ dat.end;
	return r;
}

string serialize(userdata dat) // serialize, convert to string, the userdata to send over the network
{
	string r(dat.hash);
	r = r+ ((dat.lowercase)?'1':'0');
	r = r+ ((dat.uppercase)?'1':'0');
	r = r+ ((dat.digits)?'1':'0');
	if(dat.len>9)
	{
		perror("Length is too long");
		return r + "1";
	}
	r = r+ (char)(dat.len + (int)'0');
	return r;
}

inline bool isInteger(const std::string & s) // checks if a string is an integer
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtol(s.c_str(), &p, 10) ;

   return (*p == 0) ;
}


workdata deserializew(string s) //Decodes a string to return a workdata object
{
	userdata x;
	workdata y;
	strcpy(x.hash, s.substr(0,13).c_str()); //first 13 characters of string form hash
	x.hash[13] = '\0';
	//next 3 characters are the flags
	x.lowercase = (s[13]=='1');
	x.uppercase = (s[14]=='1');
	x.digits = (s[15]=='1');
	x.len = stoi(s.substr(16,1)); // next character represents the length
	// next 2 characters represent the work to be done by this worker. (The first character of the possible passwords to be checked lie between start and end)
	y.start = s[17];
	y.end = s[18];
	y.hash_info = x;
	return y;
}
