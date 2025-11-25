#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>

std::mutex cout_mutex {};
std::string current_prompt = "";


//receive
void receiveMessages(int sock_fd) {
    
    char buffer[1024] {};
    while (true) {
        
        memset(buffer, 0, sizeof(buffer));
        
        int bytes {recv(sock_fd, buffer, sizeof(buffer), 0)};
        if(bytes <= 0) {
            std::cout << "Server Disconnected\n";
            exit(0);
        }

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\n\n--- NOTIFICATION ---\n" << std::string(buffer, bytes) << "\n---------------------\n";
            std::cout << "\n" << current_prompt << ": " << std::flush;
        }
    }
}

int tcpClient() {
    
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

    //seperate thread for receiving msgs
    std::thread receiver(receiveMessages, sock_fd);
    receiver.detach();


    //LOGIN
        {
            current_prompt = "LOGIN";
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\nLOGIN: ";
        }
        std::string login{};
        std::getline(std::cin, login);
        login = "LOGIN " + login;
        if(send(sock_fd, login.c_str(), login.size(), 0) < 0) {perror("send"); close(sock_fd); return 1;}
    
        while (true) {
        
    //TARGET
        {
            current_prompt = "SEND";
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\nSEND: ";
        }
        std::string target{};
        std::getline(std::cin, target);
        target = "SEND " + target;
        if(send(sock_fd, target.c_str(), target.size(), 0) < 0) {perror("send"); break;}

    //MSG
        {
            current_prompt = "MSG";
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\nMSG: ";
        }
        std::string msg{};
        std::getline(std::cin, msg);
        msg = "MSG " + msg;
        if (send(sock_fd, msg.c_str(), msg.size(), 0) < 0) {perror("send"); break;}      
        }    

    //close
    close(sock_fd);

    return 0;

}

int main() {
    return tcpClient();
}
