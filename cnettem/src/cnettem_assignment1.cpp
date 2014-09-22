/**
 * @cnettem_assignment1
 * @author  Chaitanya Choudhary Nettem <cnettem@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include "../include/global.h"

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */


int main(int argc, char **argv)
{
	/*Start Here*/

	return 0;
}

void creator() {
	std::cout<<"Chaitanya Choudhary Nettem\ncnettem\ncnettem@buffalo.edu";
	std::cout<<"I have read and understood the course academic integrity policy located at\
	 http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity";
}

void myip() {
	//TO-DO: Find way to get external IP.
	std::cout<<"This isn't the ip address you are looking for.";
}

void myport() {
	//TO-DO: save port number somewhere and show it here.
	std::cout<<"This isn't the port number you are looking for.";
}

void help(int server_or_client) {
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
