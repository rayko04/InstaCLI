#pragma once
#include "user_store.hpp"
#include "commands.hpp"
#include "utils.hpp" 

UserStore store{};

void handleClient(int client_fd) {
    CommandHandler handler(store);

    char buffer[1024]{};
    std::string target{};
    std::string username{};

    while (true) {
        memset(buffer, 0, sizeof(buffer));
    
    //receive data
        int bytes {recv(client_fd, buffer, sizeof(buffer), 0)};
        if(bytes <= 0) {
            store.removeUser(username);
            break;
        }
        
        std::string cmd {trim(std::string (buffer, bytes))};
    
    //LOGIN command
        if (cmd.rfind("LOGIN", 0) == 0) {
            
            //trim "LOGIN "
            username = trim(cmd.substr(6));

            CommandResult res{handler.handleLogin(username, client_fd)};
            send(client_fd, res.response.c_str(), res.response.size(), 0);

            if(!res.valid)  username.clear();
        }

    //SEND command
        else if (cmd.rfind("SEND", 0) == 0) {
            
            //trim "SEND "
            target = trim(cmd.substr(5));

            CommandResult res{handler.handleSend(target)};
            
            

            if(!res.valid) { 
                send(client_fd, res.response.c_str(), res.response.size(), 0);
                target.clear();
            }
        }

    //MSG command
        else if (cmd.rfind("MSG", 0) == 0) {
            
            //trim "MSG "
            std::string msg{ trim(cmd.substr(4)) };

            CommandResult res{handler.handleMsg(target, username, msg)};
            send(client_fd, res.response.c_str(), res.response.size(), 0);
        }

    }

    close(client_fd);
}

