#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>


int tcpServer() {
   
    //Socket creation
    int server_fd {socket(AF_INET, SOCK_STREAM, 0)};
    if (server_fd < 0) {perror("socket"); return 1;}

    //Address information
    sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    addr.sin_addr.s_addr = INADDR_ANY;

    //bind socket and addr
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {perror("bind"); return 1;}

    //start listening
    if (listen(server_fd, 5) < 0) {perror("listen"); return 1;}

    //create socket for the accepted client communication
    int client_fd {accept(server_fd, nullptr, nullptr)};
    if(client_fd < 0) {perror("accept"); return 1;}

    //receive data
    char buffer[1024] {};
    int bytes {recv(client_fd, buffer, sizeof(buffer), 0)};

    std::cout << "Client says: " << std::string(buffer, bytes) << std::endl;

    //close connection
    close(client_fd);
    close(server_fd);

    return 0;
}

int main() {
    return tcpServer();
}
