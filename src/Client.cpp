#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <chrono>
#include "encryption.hpp"

std::mutex cout_mutex {};
std::string current_prompt = "";

long getFileSize(const std::string& filePath) {
    
    //predefined struct in sys/stat.h 
    struct stat stat_buf{};
    int rc{stat(filePath.c_str(), &stat_buf)};
    return rc == 0 ? stat_buf.st_size : -1;
}

//send file over socket
bool sendFile(int sock_fd, const std::string& filePath) {
    
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "\nERROR: Could not open file " << filePath << "\n";
        return false;
    }
    
    long fileSize {getFileSize(filePath)};
    if (fileSize < 0) {
        std::cout << "\nError: Could not get file size\n";
        return false;
    }

    //extracting fileName
    size_t pos{filePath.find_last_of("/")};
    std::string fileName {(pos != std::string::npos) ? filePath.substr(pos+1) : filePath};
    
    //send header first: "FILE filename|fileSize"
    std::string fileHeader{"FILE " + fileName + "|" + std::to_string(fileSize)};
    std::string encryptedHeader = Encryption::encrypt(fileHeader);
    if (send(sock_fd, encryptedHeader.c_str(), encryptedHeader.size(), 0) < 0) { 
        perror("send file header"); 
        return false;
    }

    //acknowlegment for header
    char ack[4] {};
    if (recv(sock_fd, ack, sizeof(ack), 0) <= 0) {
        std::cout << "\nERROR: no Ack received\n";
        return false;
    }
    std::string decryptedAck = Encryption::decrypt(std::string(ack, 3));


    //send file in chunks of data
    const size_t CHUNK_SIZE{4096};
    char buffer[CHUNK_SIZE]{};
    long totalSent{0};

    //gcount(): num of characters in last input operation
    while (file.read(buffer, CHUNK_SIZE) || file.gcount() > 0) {
        
        size_t bytesRead{file.gcount()};
        std::string chunk(buffer, bytesRead);
        std::string encryptedChunk = Encryption::encrypt(chunk);
        
        ssize_t bytesSent{send(sock_fd, encryptedChunk.c_str(), encryptedChunk.size(), 0)};
        if (bytesSent < 0) {
            perror("send file data");
            return false;
        }
        totalSent += bytesSent;    
    }

    file.close();

    std::cout << "\nFile sent: " << fileName << " (" << totalSent << " bytes)\n";
    return true;
}

//download file from server
bool downloadFile(int sock_fd, const std::string& serverPath, const std::string& localDir = "downloads") {
    
    struct stat st{};
    if (stat(localDir.c_str(), &st) == -1) {
        mkdir(localDir.c_str(), 0755);
    }
    
    std::string downloadCmd = "DOWNLOAD " + serverPath;
    std::string encryptedCmd = Encryption::encrypt(downloadCmd);
    if (send(sock_fd, encryptedCmd.c_str(), encryptedCmd.size(), 0) < 0) {
        perror("send download request");
        return false;
    }

    char headerBuffer[1024]{};
    int headerBytes = recv(sock_fd, headerBuffer, sizeof(headerBuffer), 0);
    if (headerBytes <= 0) {
        std::cout << "\nError: No response from server\n";
        return false;
    }

    std::string encryptedHeader(headerBuffer, headerBytes);
    std::string header = Encryption::decrypt(encryptedHeader);
    
    if (header.rfind("ERROR:", 0) == 0) {
        std::string errorMsg = header.substr(6);
        std::cout << "\nDownload failed: " << errorMsg << "\n";
        return false;
    }
    
    if (header.rfind("DOWNLOADACK ", 0) != 0) {
        std::cout << "\nError: Invalid server response\n";
        return false;
    }
    
    std::string fileInfo = header.substr(12);
    size_t pos = fileInfo.find('|');
    if (pos == std::string::npos) {
        std::cout << "\nError: Invalid file info format\n";
        return false;
    }
    
    std::string fileName = fileInfo.substr(0, pos);
    long fileSize = std::stol(fileInfo.substr(pos + 1));
   
    std::string ack = "ACK";
    std::string encryptedAck = Encryption::encrypt(ack);
    if (send(sock_fd, encryptedAck.c_str(), encryptedAck.size(), 0) < 0) {
        perror("send ACK");
        return false;
    }    
    std::string localPath = localDir + "/" + fileName;
    std::ofstream outFile(localPath, std::ios::binary);
    
    if (!outFile.is_open()) {
        std::cout << "\nError: Could not create file " << localPath << "\n";
        return false;
    }
    
    const size_t CHUNK_SIZE = 4096;
    char buffer[CHUNK_SIZE];
    long totalReceived = 0;
    
    while (totalReceived < fileSize) {
        long remaining = fileSize - totalReceived;
        size_t toReceive = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        
        ssize_t bytes = recv(sock_fd, buffer, toReceive, 0);
        if (bytes <= 0) {
            std::cout << "\nError receiving file data\n";
            outFile.close();
            remove(localPath.c_str());
            return false;
        }
        std::string encryptedChunk(buffer, bytes);
        std::string decryptedChunk = Encryption::decrypt(encryptedChunk);
        outFile.write(decryptedChunk.c_str(), decryptedChunk.size());
        totalReceived += bytes;    
    }
    
    outFile.close();
    std::cout << "\nFile downloaded successfully: " << localPath << " (" << totalReceived << " bytes)\n";
    return true;
}

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
    std::string encryptedCmd = Encryption::encrypt(checkCmd);
    if(send(sock_fd, encryptedCmd.c_str(), encryptedCmd.size(), 0) < 0) { 
        perror("send"); 
        return false;
    }

    // Wait for response
    char buffer[1024]{};
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if(bytes <= 0) {
        return false;
    }
    
    std::string encryptedResponse(buffer, bytes);
    std::string response = Encryption::decrypt(encryptedResponse);
    
    if (response.rfind("RESPONSE:", 0) == 0) {
        response = response.substr(9); 
    }

    return (response == "EXISTS");
}

bool post(int sock_fd, const std::string& username, const std::string& captions, const std::string& media) {
   
    std::string savedMediaPath{};
    if (!media.empty()) {
        
        if (!sendFile(sock_fd, media)) {
            std::cout << "\nFailed to send media file\n";
            return false;
        }

        //receive the path to media saved in server
        char pathBuffer[1024]{};
        int pathBytes = recv(sock_fd, pathBuffer, sizeof(pathBuffer), 0);
        if (pathBytes <= 0) {
            std::cout << "\nFailed to receive saved media path\n";
            return false;
        }

        //path to media in server
        std::string encryptedPath(pathBuffer, pathBytes);
        savedMediaPath = Encryption::decrypt(encryptedPath);
    
        if (savedMediaPath.rfind("RESPONSE:", 0) == 0) {
            savedMediaPath = savedMediaPath.substr(9);
        }
        
        if (savedMediaPath == "ERROR") {
            std::cout << "\nServer failed to save media file\n";
            return false;
        }
    }

    std::string postCmd{"POST " + username + "|" + captions + "|" + savedMediaPath + "|" + getCurrentTimestamp()};
    std::string encryptedCmd = Encryption::encrypt(postCmd);
    if(send(sock_fd, encryptedCmd.c_str(), encryptedCmd.size(), 0) < 0) { 
        perror("send"); 
        return false;
    }

    //response
    char buffer[1024]{};
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)     return false;

    std::string encryptedResponse(buffer, bytes);
    std::string response = Encryption::decrypt(encryptedResponse);

    if (response.rfind("RESPONSE:", 0) == 0) {
        response = response.substr(9);
    }

    return (response== "OK");

}

struct DisplayPost {
    int postNum;
    std::string author;
    std::string captions;
    std::string mediaPath;
    std::string timestamp;
};

void showFeed(int sock_fd) {
    
    // Request feed from server
    std::string feedCmd = "FEED";
    std::string encryptedCmd = Encryption::encrypt(feedCmd);
    if (send(sock_fd, encryptedCmd.c_str(), encryptedCmd.size(), 0) < 0) {
        perror("send");
        return;
    }
    
    // Receive feed 
    char buffer[4096]{};  // Larger buffer for feed
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
        std::cout << "\nFailed to receive feed\n";
        return;
    }

    std::string encryptedFeed(buffer, bytes);
    std::string feedData = Encryption::decrypt(encryptedFeed);
    
    if (feedData.rfind("RESPONSE:", 0) == 0) {
        feedData = feedData.substr(9);
    }
        
    if (feedData == "EMPTY") {
        std::cout << "\n=== FEED ===\n";
        std::cout << "No posts yet. Be the first to post!\n";
        std::cout << "============\n";
        return;
    }
    
    // Parse into display structs
     std::vector<DisplayPost> displayPosts;
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
         
            DisplayPost dp;
            dp.postNum = postNum++;
            dp.author = postLine.substr(0, pos1);
            dp.captions = postLine.substr(pos1 + 1, pos2 - pos1 - 1);
            dp.mediaPath = postLine.substr(pos2 + 1, pos3 - pos2 - 1);
            dp.timestamp = postLine.substr(pos3 + 1);
            displayPosts.push_back(dp);
        }
    }

    // Display all posts
    std::cout << "\n=== FEED ===\n";
    for (const auto& dp : displayPosts) {
        std::cout << "\n[Post #" << dp.postNum << "]\n";
        std::cout << "Author: " << dp.author << "\n";
        std::cout << "Time: " << dp.timestamp << "\n";
        std::cout << "Caption: " << dp.captions << "\n";
        if (!dp.mediaPath.empty()) {
            std::cout << "Media: " << dp.mediaPath << "\n";
        }
        std::cout << "-------------------\n";
    }
    std::cout << "============\n";

    // Download option
    if (!displayPosts.empty()) {
        std::cout << "\nDo you want to download any media? (y/n): ";
        std::string downloadChoice;
        std::getline(std::cin, downloadChoice);
        
        if (downloadChoice == "y" || downloadChoice == "Y") {
            std::cout << "Enter post number (1-" << displayPosts.size() << ") or 0 to cancel: ";
            std::string postNumStr;
            std::getline(std::cin, postNumStr);
            
            try {
                int selectedPost = std::stoi(postNumStr);
                if (selectedPost > 0 && selectedPost <= static_cast<int>(displayPosts.size())) {
                    const DisplayPost& dp = displayPosts[selectedPost - 1];
                    if (!dp.mediaPath.empty()) {
                        std::cout << "\nDownloading media from post #" << selectedPost << "...\n";
                        downloadFile(sock_fd, dp.mediaPath);
                    } else {
                        std::cout << "\nThis post has no media to download.\n";
                    }
                } else if (selectedPost != 0) {
                    std::cout << "\nInvalid post number.\n";
                }
            } catch (...) {
                std::cout << "\nInvalid input.\n";
            }
        }
    }
}

std::string getChatList(int sock_fd) {

    std::string chatListCmd = "CHATLIST";
    std::string encryptedCmd = Encryption::encrypt(chatListCmd);
    if (send(sock_fd, encryptedCmd.c_str(), encryptedCmd.size(), 0) < 0) {
        perror("send");
        return "";
    }

    char buffer[4096]{};
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
        std::cout << "\nFailed to receive chat list\n";
        return "";
    }
    
    std::string encryptedData(buffer, bytes);
    std::string chatListData = Encryption::decrypt(encryptedData);
        
    if (chatListData.rfind("RESPONSE:", 0) == 0) {
        chatListData = chatListData.substr(9);
    }
    
    return chatListData;
}


void showChatList(int sock_fd) {
    std::string chatListData = getChatList(sock_fd);
    
    if (chatListData == "EMPTY" || chatListData.empty()) {
        std::cout << "\n=== INBOX ===\n";
        std::cout << "No conversations yet. Send a message to start chatting!\n";
        std::cout << "=============\n";
        return;
    }
    
    std::vector<std::string> chatUsers;
    size_t pos = 0;
    while ((pos = chatListData.find('\n')) != std::string::npos) {
        std::string user = chatListData.substr(0, pos);
        if (!user.empty()) {
            chatUsers.push_back(user);
        }
        chatListData.erase(0, pos + 1);
    }
    
    std::cout << "\n=== INBOX ===\n";
    std::cout << "Your conversations:\n\n";
    for (size_t i = 0; i < chatUsers.size(); i++) {
        std::cout << (i + 1) << ". " << chatUsers[i] << "\n";
    }
    std::cout << "=============\n";
}


void showChatHistory(int sock_fd, const std::string& currentUser, const std::string& otherUser) {

    std::string historyCmd = "HISTORY " + otherUser;
    std::string encryptedCmd = Encryption::encrypt(historyCmd);
    if (send(sock_fd, encryptedCmd.c_str(), encryptedCmd.size(), 0) < 0) {
        perror("send");
        return;
    }
    
    
    char buffer[4096]{};
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
        std::cout << "\nFailed to receive chat history\n";
        return;
    }
    
    std::string encryptedData(buffer, bytes);
    std::string historyData = Encryption::decrypt(encryptedData);
   
    if (historyData.rfind("RESPONSE:", 0) == 0) {
        historyData = historyData.substr(9);
    }
    
    if (historyData == "EMPTY") {
        std::cout << "\n=== CHAT WITH " << otherUser << " ===\n";
        std::cout << "No messages yet. Start the conversation!\n";
        std::cout << "========================\n";
        return;
    }
    
    std::cout << "\n=== CHAT WITH " << otherUser << " ===\n";
    size_t pos = 0;
    while ((pos = historyData.find('\n')) != std::string::npos) {
        std::string msgLine = historyData.substr(0, pos);
        historyData.erase(0, pos + 1);
        
        // Parse as sender|receiver|content|timestamp
        size_t pos1 = msgLine.find('|');
        size_t pos2 = msgLine.find('|', pos1 + 1);
        size_t pos3 = msgLine.find('|', pos2 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
            std::string sender = msgLine.substr(0, pos1);
            std::string receiver = msgLine.substr(pos1 + 1, pos2 - pos1 - 1);
            std::string content = msgLine.substr(pos2 + 1, pos3 - pos2 - 1);
            std::string timestamp = msgLine.substr(pos3 + 1);
            
            // Show sender name only if it's the other person
            if (sender == currentUser) {
                std::cout << "[" << timestamp << "] You: " << content << "\n";
            } else {
                std::cout << "[" << timestamp << "] " << sender << ": " << content << "\n";
            }
        }
    }
    std::cout << "========================\n";
}

//receive NOTIF
void receiveMessages(int sock_fd) {
    

    // Set socket timeout to prevent infinite blocking
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buffer[1024] {};
    while (true) {
        
        memset(buffer, 0, sizeof(buffer));
        
        int bytes {recv(sock_fd, buffer, sizeof(buffer), MSG_PEEK)};
        
        if(bytes > 0) {
            
            std::string encryptedMsg(buffer, bytes);
            std::string message = Encryption::decrypt(encryptedMsg);
                        
            // Only process messages with NOTIF: prefix
            if (message.rfind("NOTIF:", 0) == 0) {
                
                //now actually recv and remove from queue
                recv(sock_fd, buffer, sizeof(buffer), 0);
                
                std::string actualMsg = message.substr(6); // Remove "NOTIF:" prefix
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "\n\n--- NEW MESSAGE ---\n" 
                          << actualMsg 
                          << "\n-------------------\n"
                          << "Press Enter to continue..." << std::flush;
            }

           else {
                // if any other msg leave it and sleep briefly to avoid busy-waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(100));        
           }
        }
        else if (bytes == 0) {
            std::cout << "\nServer Disconnected\n";
            exit(0);
        }
        // bytes < 0 with timeout is normal, just continue   
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

    std::cout << "Connected to encrypted server\n";

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

        
        std::string encryptedCmd = Encryption::encrypt(loginCmd);
        if(send(sock_fd, encryptedCmd.c_str(), encryptedCmd.size(), 0) < 0) {
            perror("send"); 
            close(sock_fd); 
            return 1;
        }

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


               /* case '3': {
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
                          } */

                case '3': {
                    
                            // Show chat list first
                            showChatList(sock_fd);
                    
                        std::cout << "\nOptions:\n";
                        std::cout << "1. View chat history\n";
                        std::cout << "2. Send new message\n";
                        std::cout << "3. Back to menu\n";
                        std::cout << "Choose: ";
                    
                        std::string inboxChoice;
                        std::getline(std::cin, inboxChoice);
                    
                        if (inboxChoice == "1") {
                            std::cout << "\nEnter username to view chat: ";
                            std::string chatUser;
                            std::getline(std::cin, chatUser);
                            
                            if (!chatUser.empty()) {
                                showChatHistory(sock_fd, name, chatUser);
                            
                                // Option to send message after viewing
                                std::cout << "\nSend a message? (y/n): ";
                                std::string sendChoice;
                                std::getline(std::cin, sendChoice);
                                
                                if (sendChoice == "y" || sendChoice == "Y") {
                                    std::string sendCmd = "SEND " + chatUser;
                                    std::string encSend = Encryption::encrypt(sendCmd);
                                    if(send(sock_fd, encSend.c_str(), encSend.size(), 0) < 0) {
                                        perror("send"); 
                                        break;
                                    }

                                    std::cout << "\nMSG: ";
                                    std::string msg;
                                    std::getline(std::cin, msg);
                                    msg = "MSG " + msg;
                                    std::string encMsg = Encryption::encrypt(msg);
                                    if (send(sock_fd, encMsg.c_str(), encMsg.size(), 0) < 0) {
                                        perror("send");
                                        break;
                                    }                   
                                    
                                    std::cout << "\nMessage sent!\n";
                                }
                            }

                        } 
                        else if (inboxChoice == "2") {
                            std::cout << "\nSEND: ";
                            std::string target;
                            std::getline(std::cin, target);
                            target = "SEND " + target;
                            
                            std::string encTarget = Encryption::encrypt(target);
                            if(send(sock_fd, encTarget.c_str(), encTarget.size(), 0) < 0) {
                                perror("send");
                                break;
                            }
                            
                            std::cout << "\nMSG: ";
                            std::string msg;
                            std::getline(std::cin, msg);
                            msg = "MSG " + msg;
                            
                            std::string encMsg = Encryption::encrypt(msg);
                            if (send(sock_fd, encMsg.c_str(), encMsg.size(), 0) < 0) {
                                perror("send");
                                break;
                            }
                            
                            std::cout << "\nMessage sent!\n";
                        }

                        break;
                    }

                case '4': {
                         std::string logoutCmd{"LOGOUT " + name};
                         std::string encLogout = Encryption::encrypt(logoutCmd);
                        if(send(sock_fd, encLogout.c_str(), encLogout.size(), 0) < 0) {
                            perror("send"); 
                            close(sock_fd); 
                            return 1;
                        }

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
