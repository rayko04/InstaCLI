#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

class UserStore {
    private:
        //username to client socket descriptor map
        std::unordered_map<std::string, int> users{};
        std::mutex mtx{};

    public:
        void loginUser(const std::string& name, int fd) {
            std::lock_guard<std::mutex> lock(mtx);
            users[name] = fd;
        }

        void removeUser(const std::string& name) {
            std::lock_guard<std::mutex> lock(mtx);
            users.erase(name);
        }

        bool hasUser(const std::string& name) {
           std::lock_guard<std::mutex> lock(mtx);
           return users.find(name) != users.end();
        }

        int getFd(const std::string& name) {
            std::lock_guard<std::mutex> lock(mtx);
            return users[name];
        }
};
