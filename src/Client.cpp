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

std::string getCurrentTimestamp() {
 
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    return std::string(buf);
}

void clearScreen() {
    std::cout << "\033[2J\033[1;1H";
}


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

bool post(int sock_fd, const std::string& username, const std::string& captions, const std::string& media) {
    
    std::string postCmd{"POST " + username + "|" + captions + "|" + media + "|" + getCurrentTimestamp()};
    if(send(sock_fd, postCmd.c_str(), postCmd.size(), 0) < 0) { perror("send"); return false;}

    //response
    char buffer[1024] {};
    int bytes {recv(sock_fd, buffer, sizeof(buffer), 0)};
    if (bytes <= 0)     return false;

    std::string response(buffer, bytes);
    return (response== "OK");

}

void showFeed(int sock_fd) {
    
    // Request feed from server
    std::string feedCmd = "FEED";
    if (send(sock_fd, feedCmd.c_str(), feedCmd.size(), 0) < 0) {perror("send");return;}
    
    // Receive feed 
    char buffer[4096]{};  // Larger buffer for feed
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
        std::cout << "\nFailed to receive feed\n";
        return;
    }
    
    std::string feedData(buffer, bytes);
    
    if (feedData == "EMPTY") {
        std::cout << "\n=== FEED ===\n";
        std::cout << "No posts yet. Be the first to post!\n";
        std::cout << "============\n";
        return;
    }
    
    // Parse and display feed
    std::cout << "\n=== FEED ===\n";
    size_t pos = 0;
    int postNum = 1;

    while ((pos = feedData.find('\n')) != std::string::npos) {
        
        std::string postLine = feedData.substr(0, pos);
        feedData.erase(0, pos + 1);
        
        // Parse: author|captions|mediaPath|timestamp
        size_t pos1 = postLine.find('|');
        size_t pos2 = postLine.find('|', pos1 + 1);
        size_t pos3 = postLine.find('|', pos2 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
            
            std::string author = postLine.substr(0, pos1);
            std::string captions = postLine.substr(pos1 + 1, pos2 - pos1 - 1);
            std::string mediaPath = postLine.substr(pos2 + 1, pos3 - pos2 - 1);
            std::string timestamp = postLine.substr(pos3 + 1);
            
            std::cout << "\n[Post #" << postNum++ << "]\n";
            std::cout << "Author: " << author << "\n";
            std::cout << "Time: " << timestamp << "\n";
            std::cout << "Caption: " << captions << "\n";
            if (!mediaPath.empty()) {
                std::cout << "Media: " << mediaPath << "\n";
            }
            std::cout << "-------------------\n";
        }
    }
    std::cout << "============\n";
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
            
            current_prompt = "\nChoose an option to navigate\n 1.FEED 2.POST 3.INBOX 4.LOGOUT\n";

            std::cout << "\n\nChoose an option to navigate\n";
            std::cout << "1.FEED " << "2.POST " << "3.INBOX " << "4.LOGOUT\n";
           
            std::string choiceStr{};
            std::getline(std::cin, choiceStr);

            if(choiceStr.empty())   continue;
            char choice = choiceStr[0];

            switch (choice) {
                
                case '1': {
                              //clearScreen();
                              std::cout << "\nWELCOME TO THE FEED\n";
                              showFeed(sock_fd);
                              break;
                            }
                                    
                case '2': {
                              std::string caps{}, mediaPath{};
                              std::cout << "\nCREATING POST\n";
                       
                              std::cout << "Enter captions: ";
                              std::getline(std::cin, caps);

                              std::cout << "\nGive Path to Media(leave empty if none): ";
                              std::getline(std::cin, mediaPath);

                              if (post(sock_fd, name, caps, mediaPath))
                                  std::cout << "\nPost Created\n";
                              else
                                  std::cout << "\nPost not Created\n";
                          }

                        break;


                case '3': {
                              //clearScreen();
                        
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

                case '4': {
                         std::string logoutCmd{"LOGOUT " + name};
                         if(send(sock_fd, logoutCmd.c_str(), logoutCmd.size(), 0) < 0) 
                            {perror("send"); close(sock_fd); return 1;}
                         logoutReq = true;
                         break;
                          }
                default: std::cout << "\nINVALID OPTION\n"; continue; 
            }
            
            if (logoutReq) {
                //clearScreen(); 
                break; 
            }
        }

    }
       
    //close
    close(sock_fd);

    return 0;

}

int main() {
    return tcpClient();
}
