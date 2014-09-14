#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128

#define MAX_PARALLEL_DOWNLOAD 3

#include <string.h>

struct connection {
	int id;
	std::string hostname;
	std::string ip;
	int port;
};

#endif