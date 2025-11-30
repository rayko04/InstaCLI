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

void displayChatInterface(const std::string& current_user, const std::string& chat_with, const std::vector<std::string>& messages) {
    
    clearScreen();
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Chat with: " << chat_with;
    
    // Padding
    int padding = 54 - chat_with.length();
    for(int i = 0; i < padding; i++) 
        std::cout << " ";
    
    std::cout << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    
    // Display last 15 messages
    int start = messages.size() > 15 ? messages.size() - 15 : 0;
    
    for(size_t i = start; i < messages.size(); i++) {
        std::cout << "║ " << messages[i];
        
        // Padding to fill line
        int msg_len = messages[i].length();
        int msg_padding = 58 - msg_len;
        
        if(msg_padding < 0) 
            msg_padding = 0;
        
        for(int j = 0; j < msg_padding; j++) 
            std::cout << " ";

        std::cout << "║\n";
    }
    
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\nType message (or 'back' to return): ";
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
        
        std::string msg_data(buffer, bytes);
        
        if(msg_data.rfind("NOTIFY|", 0) == 0) {
            
            std::string notification = msg_data.substr(7);
            
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\n\n🔔 NEW MESSAGE 🔔\n" << notification << "\n";
            std::cout << current_prompt << std::flush;
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
    
                            // Request contacts list
                            std::string contactsCmd = "CONTACTS";
                            send(sock_fd, contactsCmd.c_str(), contactsCmd.size(), 0);

                             // Receive contacts
                            char buffer[1024]{};
                            memset(buffer, 0, sizeof(buffer));
                            recv(sock_fd, buffer, sizeof(buffer), 0);

                            std::string contacts_data(buffer);

                            // Parse contacts
                            std::vector<std::string> contacts;
                            if(contacts_data.rfind("CONTACTS_LIST|", 0) == 0) {
                                std::string contact_list = contacts_data.substr(14);
                                size_t pos = 0;
                                while((pos = contact_list.find(',')) != std::string::npos) {
                                    std::string contact = contact_list.substr(0, pos);
                                    if(!contact.empty())
                                        contacts.push_back(contact);
                                    contact_list.erase(0, pos + 1);
                                }
                            }

                            std::string chat_target;

                            if(contacts.empty()) {
                                std::cout << "\nNo conversations yet. Start a new chat!\n";
                                std::cout << "Enter username to chat with: ";
                                std::getline(std::cin, chat_target);
                            }
                            
                            else {
                            
                                // Display contacts
                                std::cout << "\n=== Your Conversations ===\n";
                                for(size_t i = 0; i < contacts.size(); i++) {
                                    std::cout << (i+1) << ". " << contacts[i] << "\n";
                                }
                                
                                std::cout << (contacts.size()+1) << ". New Chat\n";
                                std::cout << "\nSelect conversation: ";
                                
                                std::string selection;
                                std::getline(std::cin, selection);
                                
                                if(selection.empty()) break;
                                
                                int sel_idx = std::stoi(selection) - 1;
                                if(sel_idx >= (int)contacts.size()) {
                                    
                                    // New chat
                                    std::cout << "Enter username: ";
                                    std::getline(std::cin, chat_target);
                                }
                                
                                else if(sel_idx >= 0) {
                                    chat_target = contacts[sel_idx];
                                }
                                
                                else {
                                    break;
                                }
                            }
                            
                            if(chat_target.empty()) break;
                            
                            // Request chat history
                            std::string historyCmd = "HISTORY " + chat_target;
                            send(sock_fd, historyCmd.c_str(), historyCmd.size(), 0);
                            
                            // Receive and display history
                            std::vector<std::string> chat_messages;
                            
                            while(true) {
                                memset(buffer, 0, sizeof(buffer));
                                int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
                                if(bytes <= 0) break;
                                
                                std::string msg_data(buffer, bytes);
                                
                                if(msg_data == "HISTORY_END") break;
                                
                                if(msg_data.rfind("HISTORY_MSG|", 0) == 0) {
                                    // Parse: HISTORY_MSG|sender|timestamp|content
                                    size_t pos1 = msg_data.find('|', 12);
                                    size_t pos2 = msg_data.find('|', pos1 + 1);
                                    
                                    std::string sender = msg_data.substr(12, pos1 - 12);
                                    std::string timestamp = msg_data.substr(pos1 + 1, pos2 - pos1 - 1);
                                    std::string content = msg_data.substr(pos2 + 1);
                                    
                                    std::string formatted = "[" + timestamp + "] " + sender + ": " + content;
                                    chat_messages.push_back(formatted);
                                }
                            }
                            
                            // Chat interface loop
                            while(true) {
                                displayChatInterface(name, chat_target, chat_messages);
                                
                                std::string message;
                                std::getline(std::cin, message);
                                
                                if(message == "back") break;
                                
                                if(message.empty()) continue;
                                
                                // Send target first
                                std::string targetCmd = "SEND " + chat_target;
                                send(sock_fd, targetCmd.c_str(), targetCmd.size(), 0);
                                
                                // Send message
                                std::string msgCmd = "MSG " + message;
                                send(sock_fd, msgCmd.c_str(), msgCmd.size(), 0);
                                
                                // Wait for confirmation
                                memset(buffer, 0, sizeof(buffer));
                                recv(sock_fd, buffer, sizeof(buffer), 0);
                                
                                // Add to local display
                                std::string formatted = "[" + getCurrentTimestamp() + "] You: " + message;
                                chat_messages.push_back(formatted);
                            }
            
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
