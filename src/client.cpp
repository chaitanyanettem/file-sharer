/* Client.cpp file
 * All the functions used by client process.
 */

#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <climits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <ctime>
#include <sys/epoll.h>
#include <fstream>
#include <errno.h>
#include "../include/server.h"
#include "../include/client.h"
#include <iomanip>
#include <cstring>

#define BACKLOG 10     // how many pending connections queue will hold

#define maxPeers 10	   //Maximum number of peers allowed to be connected to server

extern const char* server_port;		//Listening port of the server

#define default_server "timberlake.cse.buffalo.edu"

//Assignmnet operator for class host_info
host_info& host_info:: operator= (const host_info b){
	file_descriptor=b.file_descriptor;
	strcpy(hostname,b.hostname);
	strcpy(ipstr,b.ipstr);
	strcpy(port,b.port);
	return *this;

}

client_operations::client_operations(){
	peer_idx=0;
	peer_list.resize(10);
	connected_list.resize(10);
	connected_list_idx=0;
}

//Extracts and fills info about connected peer in connection_list
void client_operations::add_connection_list(int file_desc, const char* port){

	//Sock Addr
	struct sockaddr_storage addr;
	socklen_t len = sizeof addr;
	//Hostname
	char hostname[MAXMSGSIZE];
	//Ip address
	char ipstr[INET6_ADDRSTRLEN];
	//Service name
	char service[20];

	//Fill addr with info
	//Used examples from beej.us guide on getpeername, getnameinfo functions
	getpeername(file_desc, (struct sockaddr*)&addr, &len);

	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;

		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;

		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	//Fill out the vector conncted_list
	getnameinfo((struct sockaddr*)&addr,sizeof addr,hostname,sizeof hostname, service, sizeof service,0);

	std::cout<<"connected to "<<hostname<<std::endl;

	connected_list.at(connected_list_idx).hostname=new char[strlen(hostname)];
	strcpy(connected_list.at(connected_list_idx).hostname,hostname);

	connected_list.at(connected_list_idx).port=new char[strlen(port)];
	strcpy(connected_list.at(connected_list_idx).port,port);

	strcpy(connected_list.at(connected_list_idx).ipstr,ipstr);

	connected_list.at(connected_list_idx).file_descriptor=file_desc;

	connected_list.at(connected_list_idx).connection_on=false;

	connected_list.at(connected_list_idx).download_st_time=0.0;

	connected_list_idx++;
}

//Sends download command to the given file_desc peer
void client_operations::send_download_command(int file_desc,const char *send_cmd){

	//set connection on with this peer to true
	set_connection_on(file_desc,true);

	//send the command to the peer
	int count=sendall(file_desc,(unsigned char *)send_cmd,MAXMSGSIZE);
	if(count!=0){
		perror("send");

	}
}

//Remove the peer from connection list and fill out it's host name
char* client_operations::remove_from_connected_list(int file_desc,char *host){

	for(int i=0;i<connected_list_idx;i++){
		if(connected_list.at(i).file_descriptor==file_desc){
			strcpy(host,connected_list.at(i).hostname);

			connected_list.at(i)=connected_list.at(--connected_list_idx);
		}
	}
	return (char *)host;
}

//Set connection on to value val for the peer
void client_operations::set_connection_on(int clientfd ,bool val){
	for(int i=0;i<connected_list_idx;i++){
		if(connected_list.at(i).file_descriptor==clientfd){
			connected_list.at(i).connection_on=val;
		}
	}

}

//check if download is on with this peer
bool client_operations::is_download_on(int clientfd){
	for(int i=0;i<connected_list_idx;i++){
		if(connected_list.at(i).file_descriptor==clientfd){
			return connected_list.at(i).connection_on;
		}
	}
	return false;
}

//check if connection exists with the peer to avoid duplicate connections
bool client_operations::is_connection_present(const char* host, const char* port){

	for(int i=0;i<connected_list_idx;i++){
		if(!strcmp(connected_list.at(i).hostname,host) && !strcmp(connected_list.at(i).port,port) ){
			return true;
		}
	}
	return false;
}

//Is given peer a valid peer to connect to checks against peer list received form the server
bool client_operations::is_valid_peer(const char* host){
	char temp_host[46]="::ffff:";
	strcat(temp_host,host);

	for (int i=0;i<peer_idx;i++){

		//for some reason I keep getting some garbage along with hostname for highgate.cse.buffalo.edu e.g. highgate.cse.buffalo.edu1 or !highgate.cse.buffalo.edu
		//so added this strstr check to see if hostname in peer_list is contained within hostname.
		char *str=strstr(peer_list.at(i).hostname,host);
		if((!strcmp(peer_list.at(i).hostname,host)) || (!strcmp(peer_list.at(i).ipstr,host)) || (!strcmp(peer_list.at(i).ipstr,temp_host) || str)){
			return true;
		}
	}
	return false;
}

//connect to host specified by host at port port
//copied from http://beej.us/guide/bgnet/output/html/multipage/clientserver.html
int client_operations::connect_to_port(const char* host,const char * port)
{
	int sockfd, numbytes;
	char hostname[MAXMSGSIZE];
	char service[20];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int error;
	char s[INET6_ADDRSTRLEN];


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//Avoid duplicate connection
	if(is_connection_present(host,port)){
		fprintf(stderr,"Connection is already present between peers");
		return -1;
	}

	if ((rv = getaddrinfo(host,port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 2;
	}

	// loop through all the results and connect to the first we can
	//Usual connect call
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		error=connect(sockfd, p->ai_addr, p->ai_addrlen);
		if(error==-1){
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);

	printf("client: connecting to %s\n", s);

	//add socket to connected_list
	add_connection_list(sockfd,port);
	freeaddrinfo(servinfo); // all done with this structure
	return sockfd;
}


//client's version of list to requets function
//Client overrides the server method due to small peculiarities of the client
//again socket part is copied from beej.us
void client_operations::listen_to_requests(int sfd){

	//Epoll eventfd
	int eventfd;

	//epoll event handler to add events to
	struct epoll_event event;
	struct epoll_event* event_array =new epoll_event[10] ;
	struct sockaddr_storage in_addr;
	struct sigaction sa;

	//listen
	if (listen(sfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	//create epoll eventfd
	eventfd = epoll_create (maxPeers);
	if (eventfd == -1)
	{
		perror ("epoll_create");
		abort ();
	}

	//Listen to stdin events
	make_entry_to_epoll(0,eventfd);

	//listen to own socket
	make_entry_to_epoll(sfd,eventfd);

	printf("client: waiting for connections...\n");
	wait_for_event(eventfd,event_array,sfd);

	delete event_array;

	close (sfd);
}


//Sends any download commands remaining for the given peer
void client_operations::handle_rem_downloads(int clientfd){
	set_connection_on(clientfd,false);
	while(!is_download_on(clientfd) && !(send_cmd_buffer.empty())){

		send_download_command(clientfd,send_cmd_buffer.front().c_str());
		send_cmd_buffer.erase(send_cmd_buffer.begin());

	}
}

//Adds start time of download to connected list.
void  client_operations::add_st_time(int file_desc,double st_time){

	for(int i=0;i<connected_list_idx;i++){
		if(connected_list.at(i).file_descriptor==file_desc){

			connected_list.at(i).download_st_time=st_time;

		}
	}
}

//returns start time from the connected list
double  client_operations::st_time(int file_desc){

	for(int i=0;i<connected_list_idx;i++){
		if(connected_list.at(i).file_descriptor==file_desc){

			return connected_list.at(i).download_st_time;
		}
	}
	return 0.0;
}

//returns index of first occurrence of a char c in the str
int client_operations::return_first_occr(const char* str, char c){

	for (int i=0;i<PACKET_SIZE;i++){
		if(str[i]==c){
			return i;
		}
	}
	return -1;
}

//puts the string into token until char c is encountered
void client_operations:: split_return(const char* str, char c, char *token){

	int i=0;
	int j=0;

	while((str[i]!=c)){
		*(token+j)=str[i++];
		j++;
	}
	token[j]='\0';
}