# Notes


## Functions

### Client:
1 Connects with a server.
2 Send data.
3 Receive response.

### Server:
1 Waits for Clients.
2 Accepts connection.
3 Reads Data.
4 Handles Multiple clients.

## Basic Functions Of A TCP Server
1) socket(): 
    Create a "communication endpoint".
2) bind():
    Bind the server/socket with an ip and port.
3) listen():
    Listen for incomming connections.
4) accept():
    Wait for the client to connect.
5) recv():
    Receive Bytes fo data from client.

### socket()
    socket(AF, socketType, Protocol): returns a socket
    AF_INET: IPv4 addr family
    SOCK_STREAM: Stream socket
    SOCK_DGRAM: Datagram socket
    0: Use default Protocol for given socketType(TCP for stream, UDP for datagram).

### sockaddr_in
    Structure that defines IP and Port for a socket. memset(destination, value, length) fills the destination with size length wiprovided value. 
    Contains:
        sin_family: Defines addr family.
        sin_port: specifies port number. htons(): host to network short. Converts incoming byte order to network byte order(Big Endian)
        sin_addr: a structure holding IP addr 32bit unsigned. s_addr: specifies that addr is stores as a one 4-byte integer.
### bind()
        bind(socketDescriptor, addr, sizeof(addr))

### listen()
        listen(socketDescriptor, backlog)
        backlog: maximum length of queue for pending connections.

### accept()
    Extracts first pending connection request. Creates and returns new socket descriptor for this client. Original descriptor remains open and listens for incomming requests. The specific client socket is used for communication. Client socket has same type as original one.
    accept(originalSocketDescriptor, addr, addrlen)

### recv() 
    recv(socketDescriptor, buffer, sizeof(bufffer), optionalFlags)
    On success recv() returns number of bytes received. 0 if connection closed. SOCKET_ERROR in case of error.


## Basic Functions of A Client
1) Create socket
2) server address resolution/setup
3) connect()
4) send()
5) close()

### connect()
    Syscall used for establishing connection with server.
    connect(sock_fd, pointerToSockAddr, sizeof(sockAddr))
    return 0 on success, -1 on failiure

### send()
    send(sock_fd, buffer, bufferLength, optionalFlags)
    on success returns number of bytes sent. on failure -1
