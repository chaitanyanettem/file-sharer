/*server.cpp has various functions that are used by both a server and client in the system.*/

#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cctype>
#include <errno.h>
#include <iomanip>
#include "../include/server.h"


#define BACKLOG 10     // how many pending connections queue will hold

#define maxPeers 10	   //Maximum number of peers allowed to be connected to server

extern const char* server_port;		//Listening port of the server

//server object constructor
//initializes the peer list that server holds.
server_operations::server_operations(){
	//the index which is used to keep track of the peers in the peer list
	//Initialized to 0
	peer_idx=0;
	//Initialized the peer list size to maxPeers
	peer_list.resize(maxPeers);
}

//This function serializes the peers list in to a single list
void server_operations::send_peer_list(){
	char list_arr[MAXMSGSIZE];

	//list to be sent over to the peers
	char *list=list_arr;

	strcpy(list,"Peer ");
	for (int i=0;i<peer_idx;i++){
		//packing the peer list
		// e.g. a typical peer list will look like "Peer timberlake.cse.buffalo.edu|127.0.0.1|1300\n"
		strcat(list,peer_list.at(i).hostname);
		strcat(list,"|");
		strcat(list,peer_list.at(i).ipstr);
		strcat(list,"|");
		strcat(list,peer_list.at(i).port);
		strcat(list,"|");

	}

	list[strlen(list)-1]='\n';

	for (int i=0;i<peer_idx;i++){
		//send the list to each and every peer
		sendall(peer_list.at(i).file_descriptor,(unsigned char *)list,strlen(list));

	}
}

//extracts and fills the peer_list vector with relevant information.
//Used examples from beej.us guide on getpeername, getnameinfo functions
void server_operations::peer_info(int peer_fd, const char* port){

	//socket addr
	struct sockaddr_storage addr;
	socklen_t len = sizeof addr;
	char hostname[MAXMSGSIZE];
	char ipstr[INET6_ADDRSTRLEN];
	char service[20];

	getpeername(peer_fd, (struct sockaddr*)&addr, &len);
	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;

		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;

		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	getnameinfo((struct sockaddr*)&addr,sizeof addr,hostname,sizeof hostname, service, sizeof service,0);

	// Add info. to peer_list about the peer.
	peer_list.at(peer_idx).hostname=new char[strlen(hostname)];

	strcpy(peer_list.at(peer_idx).hostname,"");
	strcpy(peer_list.at(peer_idx).hostname,hostname);
	strcpy(peer_list.at(peer_idx).ipstr,ipstr);
	peer_list.at(peer_idx).file_descriptor=peer_fd;
	peer_list.at(peer_idx).port=new char[strlen(port)];
	strcpy(peer_list.at(peer_idx).port,port);
	fprintf(stderr,"\nConnected to %s\n",hostname);
	peer_idx++;
}

//removes given peer from peer_list given its file_desc and writes it's hostname in host
char* server_operations ::remove_from_peer_list(int file_desc,char *host){

	for(int i=0;i<peer_idx;i++){
		if(peer_list.at(i).file_descriptor==file_desc){
			strcpy(host,peer_list.at(i).hostname);

			peer_list.at(i)=peer_list.at(--peer_idx);

		}
	}
	return host;
}


//Function to handle various requests from peers
//Part of it is copied from https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/
void server_operations::recv_requests_server(int clientfd){

	//count the bytes received.
	ssize_t count;
	//Buffer to read from.
	char buf[MAXMSGSIZE];

	// receive from the given clientfd
	count = recv (clientfd, buf, sizeof buf, 0);
	if (count == -1)
	{
		/* If errno == EAGAIN, that means we have read all
			                         data. So go back. */
		if (errno != EAGAIN)
		{
			perror ("read");

		}
		return;
	}
	else if (count == 0)
	{
		/* End of file. The remote has closed the
			                         connection. */
		//remove the peer form the list as connection is closed
		strcpy(buf,remove_from_peer_list(clientfd, buf));
		fprintf(stderr,"\n%s closed the connection.\n",buf);
		close(clientfd);
		send_peer_list();
		return;
	}

	char *token=strtok(buf," \r\n");

	if (!strcmp(token,"REGISTER")){
		//Server received request to register from this peer
		//Add the peer to peer list
		peer_info(clientfd,strtok(NULL," \r\n"));
		send_peer_list();

	}

	else if (!strcmp(token,"MYIP")){
		//Get my ip from my_ip function
		strcpy(buf,my_ip(buf));
		//error handling
		if(strcmp(buf,"error")){
			fprintf(stderr, "MY IP is %s\n",buf);
		}
		else{
			fprintf(stderr, "Error occurred while retrieving IP\n");

		}
	}
	//everything else
	else {
		fprintf(stderr,"The server does not recognize %s command",token);

		close(clientfd);

	}

}


