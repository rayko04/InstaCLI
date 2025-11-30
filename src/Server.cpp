#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <unordered_map>
#include <mutex>
#include "database.hpp"

Database database{};
std::unordered_map<std::string, int> user_map{};
std::mutex map_mutex{};

//trim spaces
std::string trim(const std::string& str) {
    size_t start {str.find_first_not_of("\t\n\r")};
    size_t end {str.find_last_not_of("\t\n\r")};

    if (start == std::string::npos || end == std::string::npos)
        return "";

    return str.substr(start, end-start+1);
}

void handleClient(int client_fd) {

    char buffer[1024]{};
    std::string target{};
    std::string username{};

    while (true) {
        memset(buffer, 0, sizeof(buffer));
    
    //receive data
        int bytes {recv(client_fd, buffer, sizeof(buffer), 0)};
        if(bytes <= 0) {
            std::cout << username << " disconnected\n";
            //lock map_mutex before accessing user_map
            std::lock_guard<std::mutex> guard(map_mutex);
            if(!username.empty())   
                user_map.erase(username);
            break;
        }
        

        std::string data(buffer, bytes);
        data = trim(data);
    
    //CHECK command
        if (data.rfind("CHECK", 0) == 0) {
        
        //trim "CHECK "
        std::string check_username = trim(data.substr(6));
        
            if(!check_username.empty()) {
                std::lock_guard<std::mutex> guard(map_mutex);
                bool exists = database.login(check_username);
                
                // Send response back to client
                std::string response = exists ? "EXISTS" : "NOTEXISTS";
                if(send(client_fd, response.c_str(), response.size(), 0) < 0) {perror("send");}
            }
        }
    //LOGIN command
        else if (data.rfind("LOGIN", 0) == 0) {
            
            //trim "LOGIN "
            username = trim(data.substr(6));
            
            if(!username.empty()) {
                std::lock_guard<std::mutex> guard(map_mutex);
                user_map[username] = client_fd;
                std::cout << username << " logged in\n";
            }
        }

        else if (data.rfind("REGIS", 0) == 0) {
            
            //trim "REGIS "
            username = trim(data.substr(6));
            
            if(!username.empty()) {
                std::lock_guard<std::mutex> guard(map_mutex);
                user_map[username] = client_fd;
                std::cout << username << " registered\n";
            }
        }

        else if (data.rfind("LOGOUT", 0) == 0) {
            
            //trim "LOGOUT "
            username = trim(data.substr(7));
            
            if(!username.empty()) {
                std::lock_guard<std::mutex> guard(map_mutex);
                user_map.erase(username);
                std::cout << username << " logged out.\n";
            }
        }

    //SEND command
        else if (data.rfind("SEND", 0) == 0) {
            
            //trim "SEND "
            target = trim(data.substr(5));

            std::lock_guard<std::mutex> guard(map_mutex);
            if(user_map.find(target) == user_map.end()) {
                std::cout << "\nNO USERNAME " << target << " found!\n";
                target.clear();
            }
        }

    //MSG command
        else if (data.rfind("MSG", 0) == 0) {
            
            if(target.empty()) {
                std::cout << "\nERROR: No Target Selected\n";
                continue;
            }

            //trim "MSG "
            std::string msg{ trim(data.substr(4)) };
            
            std::lock_guard<std::mutex> guard(map_mutex);
            if (user_map.find(target) != user_map.end()) {
                std::string send_msg {username + ": " + msg};

                if (send(user_map[target], send_msg.c_str(), send_msg.size(), 0) < 0) {perror("send"); break;}
            }
            else {
                std::cout << "\nERROR: Target " << target << " disconnected\n";
                target.clear();
            }
        }

    //POST command
        else if (data.rfind("POST", 0) == 0) {
    
            //trim "POST "
            std::string postData = trim(data.substr(5));
    
            // Parse: username|captions|mediaPath|timestamp
            size_t pos1 = postData.find('|');
            size_t pos2 = postData.find('|', pos1 + 1);
            size_t pos3 = postData.find('|', pos2 + 1);
    
            if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
                std::string post_username {postData.substr(0, pos1)};
                std::string captions {postData.substr(pos1 + 1, pos2 - pos1 - 1)};
                std::string mediaPath {postData.substr(pos2 + 1, pos3 - pos2 - 1)};
                std::string timestamp {postData.substr(pos3 + 1)};
        
                // Create post in database
                bool success {database.createPost(post_username, captions, mediaPath, timestamp)};
        
                // Send response back to client
                std::string response = success ? "OK" : "NO";
                if (send(client_fd, response.c_str(), response.size(), 0) < 0) {perror("send");}
        
                if (success) 
                    std::cout << post_username << " created a post: \"" << captions << "\"\n";
                else 
                    std::cout << "Failed to create post for " << post_username << "\n";
            } 

            else {
                
                // Invalid format
                std::string response = "NO";
                if (send(client_fd, response.c_str(), response.size(), 0) < 0) {perror("send");}
                std::cout << "Invalid POST format received\n";
                }
        }

    //FEED command
        else if (data.rfind("FEED", 0) == 0) {
    
            std::string feedData = database.showAllPosts();
    
            // Send feed data back to client
            if (send(client_fd, feedData.c_str(), feedData.size(), 0) < 0) {perror("send");}
            
            std::cout << username << " requested feed\n";
        }

    //invalid commands
        else {
            std::cout << "\nUNKNOWN COMMAND\n";
        }
    }

    close(client_fd);
}

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
