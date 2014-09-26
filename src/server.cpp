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

//returns the ip address of the host machine
//copied most of the code from here http://jhshi.me/2013/11/02/how-to-get-hosts-ip-address/
char * server_operations::my_ip (char *ip_addr){
	char ip[256];

	//DNS server
	char target_name[8] = "8.8.8.8";
	// DNS port
	char target_port[3] = "53";
	char port[20];
	strcpy(ip_addr,"error");
	/* get peer server */
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo* info;
	int ret = 0;
	if ((ret = getaddrinfo(target_name, target_port, &hints, &info)) != 0) {
		printf("[ERROR]: getaddrinfo error: %s\n", gai_strerror(ret));
		return ip_addr;
	}

	if (info->ai_family == AF_INET6) {
		printf("[ERROR]: do not support IPv6 yet.\n");
		return ip_addr;
	}

	/* create socket */
	int sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (sock <= 0) {
		perror("socket");
		return ip_addr;
	}

	/* connect to server */
	if (connect(sock, info->ai_addr, info->ai_addrlen) < 0) {
		perror("connect");
		close(sock);
		return ip_addr;
	}

	/* get local socket info */
	struct sockaddr_storage local_addr;
	socklen_t addr_len = sizeof(local_addr);
	if (getsockname(sock, (struct sockaddr*)&local_addr, &addr_len) < 0) {
		perror("getsockname");
		close(sock);
		return ip_addr;
	}

	/* get peer ip addr */
	if (local_addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&local_addr;

		inet_ntop(AF_INET, &s->sin_addr, ip, sizeof ip);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&local_addr;

		inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof ip);
	}
	strcpy(ip_addr,ip);

	return ip_addr;
}

//Makes entry of given socket to event fd.
//After this entry events on his socket will be notified and thus can be handled.
void server_operations::make_entry_to_epoll(int sfd, int eventid){
	int s;
	struct epoll_event event;
	event.data.fd = sfd;
	event.events = EPOLLIN;
	s = epoll_ctl (eventid, EPOLL_CTL_ADD, sfd, &event);
	if (s == -1)
	{
		perror ("epoll_ctl");
		abort ();
	}
	/* Buffer where events are returned */
}

/*Epoll waiting loop
 *
 *
 * Referred to https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/
 * on how to use Epoll and have copied part of the code from this tutorial
 */
void inline server_operations::wait_for_event(int eventfd, struct epoll_event* event_array,int sfd){
	while (1)
	{
		char buf[MAXMSGSIZE];
		int n, i;
		int infd;
		//epoll wait until no event
		n = epoll_wait (eventfd, event_array, maxPeers, -1);
		for (i = 0; i < n; i++)
		{
			if(event_array[i].events & EPOLLRDHUP){

				//This flag means remote has closed the connection.

				//This flag EPOLLRDHUP is supported at highgate but not at other servers so had to handle closing of connections at multiple places.
				strcpy(buf,remove_from_peer_list(event_array[i].data.fd, buf));
				fprintf(stderr,"\n%s closed the connection.\n",buf);
				close (event_array[i].data.fd);

				send_peer_list();

				continue;

			}
			else if ((event_array[i].events & EPOLLERR) || (event_array[i].events & EPOLLHUP) || (!(event_array[i].events & EPOLLIN)))
			{
				/* An error has occured on this fd, or the socket is not
		                 ready for reading (why were we notified then?) */
				strcpy(buf,remove_from_peer_list(event_array[i].data.fd, buf));
				fprintf(stderr,"\n%s closed the connection.\n",buf);
				close (event_array[i].data.fd);

				send_peer_list();

				continue;
			}
			else if (sfd == event_array[i].data.fd)
				//means event on our own server socket for incoming connections

				while (1)
				{
					struct sockaddr_in in_addr;
					socklen_t in_len;

					//used for error checking
					int s;

					//fd returned from accept
					int infd;

					//Used to get data from getnameinfo
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

					in_len = sizeof in_addr;
					infd = accept (sfd, (struct sockaddr *)&in_addr, &in_len);
					if (infd == -1)
					{
						if ((errno == EAGAIN) ||
								(errno == EWOULDBLOCK))
						{
							/* We have processed all incoming
				                             connections. */
							break;
						}
						else
						{
							perror ("accept");
							break;
						}
					}

					s=getnameinfo((struct sockaddr *)&in_addr, sizeof in_addr, hbuf, sizeof hbuf, sbuf, sizeof sbuf, 0);
					if (s == 0)
					{
						printf("Accepted connection on descriptor %d "
								"(host=%s, port=%s)\n", infd, hbuf, sbuf);
					}
					//make this socket non blocking
					make_socket_non_blocking(infd);
					//make this socket's entry to epoll
					make_entry_to_epoll(infd,eventfd);
				}


			else if (event_array[i].data.fd==0){
				//this means data to be read on stdin
				recv_stdin_client(eventfd);

			}

			else
			{
				/* We have data on the fd waiting to be read.
				 * May be peer's sending their requests to connect?
				 */

				recv_requests_server(event_array[i].data.fd);

			}
		}
	}
}

//used to reap the dead processes
//copied from http://beej.us/guide/bgnet/output/html/multipage/clientserver.html

void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

//sendall function copied from beej.us to avoid partial sends.
int server_operations::sendall(int s, unsigned char *buf, int len)
{
	int total = 0;        // how many bytes we've sent
	int bytesleft = len; // how many we have left to send
	int n;

	while(total < len) {
		n = send(s, buf+total, bytesleft, 0);
		if (n == -1) {

			if (errno == EBADF || ECONNRESET || EDESTADDRREQ || ENOTCONN || ENOTSOCK || EPIPE){
				break;
			}
		}
		if(n>0){
			total += n;
			bytesleft -= n;
		}
	}

	len = total; // return number actually sent here

	return n==-1?-1:0; // return -1 on failure, 0 on success
}

//makes the socket non blocking
void server_operations::make_socket_non_blocking (int sfd)
{
	int flags, s;

	flags = fcntl (sfd, F_GETFL, 0);
	if (flags == -1)
	{
		perror ("fcntl");

	}

	flags |= O_NONBLOCK;
	s = fcntl (sfd, F_SETFL, flags);
	if (s == -1)
	{
		perror ("fcntl");
	}
}

//makes the socket back to blocking mode
//while sending a large file send functions gives an error send: resource not available may be due to congestion over the network
//So, in that case it makes sense make the socket blocking until everything is sent.
void server_operations::make_socket_blocking (int sfd)
{
	int flags, s;

	flags = fcntl (sfd, F_GETFL, 0);
	if (flags == -1)
	{
		perror ("fcntl");

	}

	flags &= (~O_NONBLOCK);
	if(flags & O_NONBLOCK){
		std::cout<<"blocking failed";
	}
	s = fcntl (sfd, F_SETFL, flags);
	if (s == -1)
	{
		perror ("fcntl");
	}
}
