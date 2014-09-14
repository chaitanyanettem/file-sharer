/**
 * @conn_list
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
 * This contains code for maintaining a list of connections.
 */

#include "global.h"

class client_connection_list {
	std::vector<connection> connection_list;
public:
	/** 
	 * Add a connection to the connection_list after checking if it exists in
	 * server_connection_list.
	 */
	int add_to_list (connection new_connection, vector<connection> server_connection_list) {
		
		for (int i=0; i<server_connection_list.size(); i++) {

			if (server_connection_list[i].ip == new_connection.ip) {
				connection_list.push_back(new_connection);
				return 0; 
				//0 implies successful addition to this client's list
			}

		}
		return 1; 
		//1 implies unsuccessful addition to this client's list because of absence from server's list.
	}

	/** 
	 * Remove given connection from connection_list
	 */
	int remove_from_list (connection remove_connection) {
		for (int i=0; i<connection_list.size(); i++) {
			if (connection_list[i].ip == remove_connection[i].ip) {
				connection_list.erase(connection_list.begin() + i);
			}
		}
	}
};