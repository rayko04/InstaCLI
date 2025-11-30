#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <ctime>

// Message structure
struct Message {
    std::string sender;
    std::string receiver;
    std::string content;
    std::string timestamp;
};


class User {
    private:
        int m_user_id{};
        std::string m_name{};
        static int m_next_user_id;
    protected:
        bool m_online_status{true};
    public:
        User() = default;
        User(std::string name) : m_user_id{++m_next_user_id}, m_name{name} {}
        int getUserId() const {return m_user_id;}
        std::string getUserName() const {return m_name;}
        std::string getUserNameFromId(int id) const {return m_name;}
        bool getUserStatus() const {return m_online_status;}
        void setUserStatus(bool val) {m_online_status = val;}
};
int User::m_next_user_id{0};


class Database {
    private:
        std::vector<User> registered_users{};
        // <conversation key, queue<message>>
        std::unordered_map<std::string, std::deque<Message>> message_history{};
        
        // Helper to create conversation key
        std::string getConversationKey(const std::string& user1, const std::string& user2) const {
            if (user1 < user2)
                return user1 + ":" + user2;
            return user2 + ":" + user1;
        }
        
    public:
        bool login(std::string name) {
            for(User& user: registered_users) {
                if(user.getUserName() == name)
                    return true;    //return true if login
            }
            registered_users.push_back(User(name));
            return false;       //return false if registration
        }
        
        void logout(std::string name) {
            for(User& user: registered_users) {
                if(user.getUserName() == name) {
                    user.setUserStatus(false);
                    break;
                }
            }
        }
        
        // Add message to history
        void addMessage(const std::string& sender, const std::string& receiver, 
                       const std::string& content, const std::string& timestamp) {
            std::string conv_key = getConversationKey(sender, receiver);
            message_history[conv_key].push_back({sender, receiver, content, timestamp});
            
            // Keep only last 100 messages per conversation
            if(message_history[conv_key].size() > 100)
                message_history[conv_key].pop_front();
        }
        
        // Get chat history between two users
        std::vector<Message> getChatHistory(const std::string& user1, const std::string& user2) const {
            std::string conv_key = getConversationKey(user1, user2);
            
            if(message_history.find(conv_key) != message_history.end()) {
                const auto& deque = message_history.at(conv_key);
                return std::vector<Message>(deque.begin(), deque.end());
            }
            
            return std::vector<Message>();  // Empty vector if no history
        }
        
        // Get list of contacts (users you've chatted with)
        std::vector<std::string> getContacts(const std::string& username) const {
            std::vector<std::string> contacts;
            
            for(const auto& [conv_key, messages] : message_history) {
                if(messages.empty()) continue;
                
                // Extract users from conversation key
                size_t pos = conv_key.find(':');
                std::string user1 = conv_key.substr(0, pos);
                std::string user2 = conv_key.substr(pos + 1);
                
                // Add the other user to contacts
                if(user1 == username)
                    contacts.push_back(user2);
                else if(user2 == username)
                    contacts.push_back(user1);
            }
            
            return contacts;
        }
        
        // Check if a conversation exists
        bool hasConversation(const std::string& user1, const std::string& user2) const {
            std::string conv_key = getConversationKey(user1, user2);
            return message_history.find(conv_key) != message_history.end() 
                   && !message_history.at(conv_key).empty();
        }
};
