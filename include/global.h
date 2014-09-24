#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128

#define MAX_PARALLEL_DOWNLOAD 3

#include <string.h>
#include <iostream>
#include <cstdlib>

struct connection {
	int id;
	std::string hostname;
	std::string ip;
	int port;
};

/* This class is inherited by the client and server for their shell commands.*/

class shell {
public:
	void creator();
	void help();
	void myip();
	void myport();
	void register();
	void connect();
	void list(int);
/*	
	TO-DO:	void terminate();
	TO-DO:	void exit();
	TO-DO:	void upload();
	TO-DO:	void download();
	TO-DO:	void statistics();
*/
};

#endif