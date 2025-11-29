#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include "database.hpp"

std::mutex cout_mutex {};
std::string current_prompt = "";


bool checkUserExists(int sock_fd, const std::string& username) {
    
    std::string checkCmd = "CHECK " + username;
    if(send(sock_fd, checkCmd.c_str(), checkCmd.size(), 0) < 0) { perror("send"); return false;}
    
    // Wait for response
    char buffer[1024]{};
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if(bytes <= 0) {
        return false;
    }
    
    std::string response(buffer, bytes);
    return (response == "EXISTS");
}


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

    while (true) {
    
    //LOGIN cmd
         {
            //update current prompt for notifs
            current_prompt = "LOGIN/REGISTER";
        
            //lock mutex
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\nLOGIN/REGISTER: ";
        }

        //take input
        std::string name{};
        std::getline(std::cin, name);
        
        // Query server database to check if user exists
        // And update if it doesn't
        bool userExists = checkUserExists(sock_fd, name);
        std::string loginCmd{};

        if(userExists)
            //send LOGIN cmd
            loginCmd = "LOGIN " + name;
    
        else
            loginCmd = "REGIS " + name;

        
        if(send(sock_fd, loginCmd.c_str(), loginCmd.size(), 0) < 0) {perror("send"); close(sock_fd); return 1;}
        std::cout << "\nWELCOME " << name << std::endl;

        bool logoutReq{false};
    
    //MENU
        while (true) {
            
            current_prompt = "\nChoose an option to navigate\n 1.FEED 2.INBOX 3.LOGOUT\n";

            std::cout << "\n\nChoose an option to navigate\n";
            std::cout << "1.FEED " << "2.INBOX " << "3.LOGOUT\n";
           
            std::string choiceStr{};
            std::getline(std::cin, choiceStr);

            if(choiceStr.empty())   continue;
            char choice = choiceStr[0];

            switch (choice) {
                
                case '1':break;
                case '2': { 
                        
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

                            break;
                          }

                case '3': {
                         std::string logoutCmd{"LOGOUT " + name};
                         if(send(sock_fd, logoutCmd.c_str(), logoutCmd.size(), 0) < 0) 
                            {perror("send"); close(sock_fd); return 1;}
                         logoutReq = true;
                         break;
                          }
                default: std::cout << "\nINVALID OPTION\n"; continue; 
            }
            
            if (logoutReq)
                break;
        }

    }
       
    //close
    close(sock_fd);

    return 0;

}

int main() {
    return tcpClient();
}
