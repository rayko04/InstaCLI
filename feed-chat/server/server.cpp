#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include "client_handler.hpp"

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

    // Accept and store connections with multiple clients
    std::vector<int> clients {};
    while (true) {

        //create socket for the accepted client communication
        int client_fd { accept(server_fd, nullptr, nullptr) };
        if(client_fd < 0) {perror("accept"); break;}

        clients.push_back(client_fd);

        std::thread t(handleClient, client_fd);
        t.detach();
    }

     //close connection
    for (int client_fd: clients) {
        close(client_fd);
    }
    close(server_fd);

    return 0;
}

int main() {
    return tcpServer();
}
