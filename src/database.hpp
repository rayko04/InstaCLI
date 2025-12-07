#include <string>
#include <vector>
#include <algorithm>


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


class Post {
    private:
        int m_post_id{};
        User m_author{};
        std::string m_captions{};
        std::string m_mediaPath{};
        std::string m_timestamp{};
        static int m_next_post_id;

    public:
        Post() = default;
        Post(User user, std::string cap, std::string media, std::string time) 
            : m_author{user}, m_captions{cap}, m_mediaPath{media}, m_timestamp{time} {}

        int getPostId() const {return m_post_id;}
        bool hasMedia() const {return !m_mediaPath.empty();}
        std::string getAuthorName() const {return m_author.getUserName();}
        std::string getCaptions() const {return m_captions;}
        std::string getMediaPath() const {return m_mediaPath;}
        std::string getTimestamp() const {return m_timestamp;}

};

int Post::m_next_post_id{0};

class Message {
    private:
        int m_message_id{};
        std::string m_sender{};
        std::string m_receiver{};
        std::string m_content{};
        std::string m_timestamp{};
        static int m_next_message_id;
    public:
        Message() = default;
        Message(std::string sender, std::string receiver, std::string content, std::string timestamp)
            : m_message_id{++m_next_message_id}, m_sender{sender}, m_receiver{receiver}, 
              m_content{content}, m_timestamp{timestamp} {}
        
        int getMessageId() const {return m_message_id;}
        std::string getSender() const {return m_sender;}
        std::string getReceiver() const {return m_receiver;}
        std::string getContent() const {return m_content;}
        std::string getTimestamp() const {return m_timestamp;}
};

int Message::m_next_message_id{0};

class Database {
    private:
        std::vector<User> registered_users{};
        std::vector<Post> registered_posts{};
        std::vector<Message> message_history{};

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

        bool createPost(std::string username, std::string captions, std::string mediaPath, std::string timestamp) {
            
            for (User& user: registered_users) {
                if(user.getUserName() == username) {

                    Post p(user, captions, mediaPath, timestamp);
                    registered_posts.push_back(p);
                    return true;
                }
            }
            return false;
        }

        std::string showAllPosts() {
           
            if (registered_posts.empty()) return "EMPTY";
    
            std::string feed{};
            for (const Post& post : registered_posts) {
                
                feed += post.getAuthorName() + "|" + 
                post.getCaptions() + "|" + 
                post.getMediaPath() + "|" + 
                post.getTimestamp() + "\n";
            }
    
            return feed;
        }

        const std::vector<Post>& getAllPosts() const {
            return registered_posts;
        }

        const Post* getPostById(int postId) const {
            for (const Post& post : registered_posts) {
                if (post.getPostId() == postId) {
                    return &post;
                }
            }
            return nullptr;
        }

        int getPostCount() const {
            return registered_posts.size();
        }


        bool storeMessage(std::string sender, std::string receiver, std::string content, std::string timestamp) {
            Message msg(sender, receiver, content, timestamp);
            message_history.push_back(msg);
            return true;
        }
        
        std::string getConversation(std::string user1, std::string user2) {
            if (message_history.empty()) return "EMPTY";
            
            std::string conversation{};
            for (const Message& msg : message_history) {
                // Check if message is between these two users (in either direction)
                if ((msg.getSender() == user1 && msg.getReceiver() == user2) ||
                    (msg.getSender() == user2 && msg.getReceiver() == user1)) {
                    
                    conversation += msg.getSender() + "|" +
                                   msg.getReceiver() + "|" +
                                   msg.getContent() + "|" +
                                   msg.getTimestamp() + "\n";
                }
            }
            
            return conversation.empty() ? "EMPTY" : conversation;
        }


        std::string getChatList(std::string username) {
            std::vector<std::string> chatUsers;
            
            for (const Message& msg : message_history) {
                std::string otherUser;
                if (msg.getSender() == username) {
                    otherUser = msg.getReceiver();
                } else if (msg.getReceiver() == username) {
                    otherUser = msg.getSender();
                }
                
                // Add to list if not already present
                if (!otherUser.empty() && 
                    std::find(chatUsers.begin(), chatUsers.end(), otherUser) == chatUsers.end()) {
                    chatUsers.push_back(otherUser);
                }
            }
            
            if (chatUsers.empty()) return "EMPTY";
            
            std::string result{};
            for (const std::string& user : chatUsers) {
                result += user + "\n";
            }
            return result;
        }
};
