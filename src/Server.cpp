#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "database.hpp"
#include "encryption.hpp"


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

//creates an upload directory (if not exists) for files with permission 0755
void ensureUploadsDirectory() {

    struct stat st{};
    if (stat("uploads", &st) == -1) {
        mkdir("uploads", 0755);
    }
}

std::string generateUniqueFilename(const std::string& originalFilename) {
  
    time_t now = time(0);
    return "uploads/" + std::to_string(now) + "_" + originalFilename;
}

//receive file from client and save it in uploads directory
std::string receiveFile(int client_fd, const std::string& filename, long filesize) {
    
    ensureUploadsDirectory();
    
    std::string savedPath = generateUniqueFilename(filename);
    std::ofstream outFile(savedPath, std::ios::binary);
    
    if (!outFile.is_open()) {
        std::cout << "\nError: Could not create file " << savedPath << "\n";
        return "";
    }
    
    // Send ACK
    std::string ack = "ACK";
    std::string encryptedAck = Encryption::encrypt(ack);
    if (send(client_fd, encryptedAck.c_str(), encryptedAck.size(), 0) < 0) {
        perror("send ACK");
        outFile.close();
        return "";
    }

    // Receive file data
    const size_t CHUNK_SIZE = 4096;
    char buffer[CHUNK_SIZE];
    long totalReceived = 0;
    
    while (totalReceived < filesize) {
        long remaining = filesize - totalReceived;
        size_t toReceive = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        
        ssize_t bytes = recv(client_fd, buffer, toReceive, 0);
        if (bytes <= 0) {
            std::cout << "\nError receiving file data\n";
            outFile.close();
            remove(savedPath.c_str());
            return "";
        }

        std::string encryptedChunk(buffer, bytes);
        std::string decryptedChunk = Encryption::decrypt(encryptedChunk);
        outFile.write(decryptedChunk.c_str(), decryptedChunk.size());
        totalReceived += bytes;    
    }
    
    outFile.close();
    std::cout << "\nFile received and saved: " << savedPath << " (" << totalReceived << " bytes)\n";
    
    return savedPath;
}

bool sendFileToClient(int client_fd, const std::string& filePath) {
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "\nError: Could not open file " << filePath << "\n";
        return false;
    }
    
    file.seekg(0, std::ios::end);
    long fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    size_t pos = filePath.find_last_of("/");
    std::string fileName = (pos != std::string::npos) ? filePath.substr(pos + 1) : filePath;
    
    std::string fileHeader = "DOWNLOADACK " + fileName + "|" + std::to_string(fileSize);
    
    std::string encryptedHeader = Encryption::encrypt(fileHeader);
    if (send(client_fd, encryptedHeader.c_str(), encryptedHeader.size(), 0) < 0) {
        perror("send file header");
        return false;
    }  

    char ack[4]{};
    if (recv(client_fd, ack, sizeof(ack), 0) <= 0) {
        std::cout << "\nError: No ACK received from client\n";
        return false;
    }
    std::string decryptedAck = Encryption::decrypt(std::string(ack, 3));
    
    const size_t CHUNK_SIZE = 4096;
    char buffer[CHUNK_SIZE];
    long totalSent = 0;
    
    while (file.read(buffer, CHUNK_SIZE) || file.gcount() > 0) {
        size_t bytesRead = file.gcount();
        std::string chunk(buffer, bytesRead);

        std::string encryptedChunk = Encryption::encrypt(chunk);
        ssize_t bytesSent = send(client_fd, encryptedChunk.c_str(), encryptedChunk.size(), 0);
        if (bytesSent < 0) {
            perror("send file data");
            file.close();
            return false;
        }
        totalSent += bytesSent;
    }

    file.close();
    std::cout << "\nFile sent to client: " << fileName << " (" << totalSent << " bytes)\n";
    return true;
}

std::string getCurrentTimestamp() {
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    return std::string(buf);
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
        

        std::string encryptedData(buffer, bytes);
        std::string data = Encryption::decrypt(encryptedData);
        data = trim(data);    

    //CHECK command
        if (data.rfind("CHECK", 0) == 0) {
        
        //trim "CHECK "
        std::string check_username = trim(data.substr(6));
        
            if(!check_username.empty()) {
                std::lock_guard<std::mutex> guard(map_mutex);
                bool exists = database.login(check_username);
                
                // Send response back to client
                std::string response = exists ? "RESPONSE:EXISTS" : "RESPONSE:NOTEXISTS";
               std::string encryptedResponse = Encryption::encrypt(response);
                if(send(client_fd, encryptedResponse.c_str(), encryptedResponse.size(), 0) < 0) {perror("send");}            
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
    //REGIS command
        else if (data.rfind("REGIS", 0) == 0) {
            
            //trim "REGIS "
            username = trim(data.substr(6));
            
            if(!username.empty()) {
                std::lock_guard<std::mutex> guard(map_mutex);
                user_map[username] = client_fd;
                std::cout << username << " registered\n";
            }
        }
    //LOGOUT command
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
            target = trim(data.substr(5));
            
            bool userExists = false;
            for (const auto& user_pair : user_map) {
                if (user_pair.first == target) {
                    userExists = true;  //if online
                    break;
                }
            }
            
            if (!userExists) {
                userExists = database.userExists(target);  // if offline
            }
            
            if (!userExists) {
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
            
            database.storeMessage(username, target, msg, getCurrentTimestamp());

            //if online send notif
            if (user_map.find(target) != user_map.end()) {
                
                std::string send_msg {"NOTIF:" + username + ": " + msg};
                std::string encryptedMsg = Encryption::encrypt(send_msg);
                if (send(user_map[target], encryptedMsg.c_str(), encryptedMsg.size(), 0) < 0) {
                    perror("send"); 
                }
                std::cout << "Message sent to " << target << " (online)\n";
            } 
            else {
                std::cout << "Message stored for " << target << " (offline)\n";
            }
        }

      //CHATLIST command
        else if (data.rfind("CHATLIST", 0) == 0) {
            
            std::string chatListData = "RESPONSE:" + database.getChatList(username);
            
            std::string encryptedData {Encryption::encrypt(chatListData)};
            if (send(client_fd, encryptedData.c_str(), encryptedData.size(), 0) < 0) {perror("send");}
            
            std::cout << username << " requested chat list\n";
        }

    //HISTORY command
        else if (data.rfind("HISTORY", 0) == 0) {
            
            // trim "HISTORY "
            std::string otherUser = trim(data.substr(8));
            
            if (!otherUser.empty()) {
                std::string historyData = "RESPONSE:" + database.getConversation(username, otherUser);
                
                std::string encryptedData {Encryption::encrypt(historyData)};
                if (send(client_fd, encryptedData.c_str(), encryptedData.size(), 0) < 0) {perror("send");}
                
                std::cout << username << " requested chat history with " << otherUser << "\n";
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
                std::string response = success ? "RESPONSE:OK" : "RESPONSE:NO";
                std::string encryptedResponse = Encryption::encrypt(response);
                if (send(client_fd, encryptedResponse.c_str(), encryptedResponse.size(), 0) < 0) {
                    perror("send");
                }
        
                if (success) 
                    std::cout << post_username << " created a post: \"" << captions << "\"\n";
                else 
                    std::cout << "Failed to create post for " << post_username << "\n";
            } 


            else {
                
                // Invalid format
                std::string response = "NO";
                std::string encryptedResponse = Encryption::encrypt(response);
                if (send(client_fd, encryptedResponse.c_str(), encryptedResponse.size(), 0) < 0) {
                    perror("send");
                }                std::cout << "Invalid POST format received\n";
            }
        }

    //FEED command
        else if (data.rfind("FEED", 0) == 0) {
    
            std::string feedData = "RESPONSE:" + database.showAllPosts();
    
            // Send feed data back to client
            std::string encryptedFeed = Encryption::encrypt(feedData);
            if (send(client_fd, encryptedFeed.c_str(), encryptedFeed.size(), 0) < 0) {
                perror("send");
            }
            
            std::cout << username << " requested feed\n";
        }

    //FILE command
        else if (data.rfind("FILE", 0) == 0) {
    
            // Parse: "FILE filename|filesize"
            std::string fileInfo = trim(data.substr(5));
        
            size_t pos = fileInfo.find('|');
            if (pos != std::string::npos) {
                std::string filename = fileInfo.substr(0, pos);
                long filesize = std::stol(fileInfo.substr(pos + 1));
            
                std::cout << "Receiving file: " << filename << " (" << filesize << " bytes)\n";
            
                // Receive and save the file
                std::string savedPath = receiveFile(client_fd, filename, filesize);
            
                if (!savedPath.empty()) {
                    // Send back the saved path to client
                    std::string response{"RESPONSE:" + savedPath};
                    std::string encryptedResponse = Encryption::encrypt(response);
                    if (send(client_fd, encryptedResponse.c_str(), encryptedResponse.size(), 0) < 0) {
                        perror("send saved path");
                    }
                }

                else {
                    // Send error
                    std::string error = "ERROR";
                    std::string encryptedError = Encryption::encrypt(error);
                    if (send(client_fd, encryptedError.c_str(), encryptedError.size(), 0) < 0) {
                        perror("send error");
                    }                }
            }
        }


    //DOWNLOAD command
        else if (data.rfind("DOWNLOAD", 0) == 0) {
        
            std::string filePath = trim(data.substr(9));
            if (!filePath.empty()) {
                std::cout << username << " requesting download: " << filePath << "\n";
                std::ifstream checkFile(filePath);
            
                if (!checkFile.good()) {
                    std::string error = "ERROR:FILE_NOT_FOUND";
                    std::string encryptedError = Encryption::encrypt(error);
                    if (send(client_fd, encryptedError.c_str(), encryptedError.size(), 0) < 0) {
                        perror("send error");
                    }                std::cout << "File not found: " << filePath << "\n";
                } 

                else {
                    
                    checkFile.close();
                    if (!sendFileToClient(client_fd, filePath)) {
                        std::string error = "ERROR:SEND_FAILED";
                        std::string encryptedError = Encryption::encrypt(error);
                        if (send(client_fd, encryptedError.c_str(), encryptedError.size(), 0) < 0) {
                            perror("send error");
                        }                    
                    }
                }
            } 
            
            else {
                
                std::string error = "ERROR:INVALID_PATH";
                std::string encryptedError = Encryption::encrypt(error);
                if (send(client_fd, encryptedError.c_str(), encryptedError.size(), 0) < 0) {
                    perror("send error");
                }            
            }
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

    std::cout << "Encrypted Server listening on port 5555\n";

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
