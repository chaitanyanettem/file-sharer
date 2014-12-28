file-sharer
===========

P2P file sharing system allowing parallel uploading and downloading among remote peers connected to a central server.

Asynchronous IO and multiple downloads/uploads are made possible by the use of epoll.

The design of the application is like so:

- If a client wants to initiate file sharing, it needs to register itself with the server
- The server then sends a list of all registered clients to all the clients
- The client can then establish a connection with any of the registered clients
- Whenever any new client registers or an old client deregisters/drops connection, the registered client list at every client gets updated
