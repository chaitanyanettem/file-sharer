#include "../include/global.h"

void shell::creator() {
	std::cout<<"Chaitanya Choudhary Nettem\ncnettem\ncnettem@buffalo.edu";
	std::cout<<"I have read and understood the course academic integrity policy located at\
	 http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity";
}

void shell::help(int server_or_client) {
	//TO-DO: Complete the help messages.
	switch(server_or_client) {
		
		case 0: // Server
			std::cout<<"CREATOR - Gives creator's information\nHELP - Displays this message\nMYIP\
			 - Displays the IP of the process\nMYPORT - Displays the port number of this process\
			 \n";
			break;
		
		case 1: //Client
			std::cout<<"CREATOR";
	}
}

void shell::myip() {
	//TO-DO: Find way to get external IP.
	std::cout<<"This isn't the ip address you are looking for.";
}

void shell::myport() {
	//TO-DO: save port number somewhere and show it here.
	std::cout<<"This isn't the port number you are looking for.";
}