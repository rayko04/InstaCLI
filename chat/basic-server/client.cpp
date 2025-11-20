#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int main() {
   
    //socket creation
    int sock_fd {socket(AF_INET, SOCK_STREAM, 0)};
    if (sock_fd < 0) {perror("socket"); return 1;}

    //socket addr 
    sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    //connect
    if (connect(sock_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {perror("connect"); return 1;}

    char buffer[1024] {};

    while (true) {
        
        //send
        std::cout << "Send msg: ";
        std::string msg{};
        std::getline(std::cin, msg);
        if (send(sock_fd, msg.c_str(), msg.size(), 0) < 0) {perror("send"); break;}      
        
        //receive
        int bytes {recv(sock_fd, buffer, sizeof(buffer), 0)};
        if(bytes <= 0) {
            std::cout << "Server Disconnected\n";
            break;
        }

        std::cout << "Server says: " << std::string(buffer, bytes) << std::endl;
        
    }

    //close
    close(sock_fd);
    
    return 0;
}
