#pragma once
#include <sys/socket.h>
#include <cstring>
#include <string>
#include "user_store.hpp"
#include "utils.hpp"

struct CommandResult {
    std::string response{};
    bool valid{};
};

class CommandHandler {
    private:
        UserStore& store;
    public:
        CommandHandler(UserStore& s) : store(s) {}

        CommandResult handleLogin(const std::string& username, int fd) {
            store.loginUser(username, fd);
            return {"Logged in as " + username, true};
        }

        CommandResult handleSend(const std::string& target) {
            if (!store.hasUser(target)) {
                return {"ERROR: user " + target + " not found", false};
            }
            
            return {"OK SEND", true};
        }

        CommandResult handleMsg( const std::string& target, const std::string& sender, const std::string& message) {
            if (!store.hasUser(target)) {
                return {"ERROR: target offline", false};
            }

            int fd {store.getFd(target)};
            std::string out {sender + ": " + message};
            send(fd, out.c_str(), out.size(), 0);

            return {"OK MSG", true};
        }
        
};
