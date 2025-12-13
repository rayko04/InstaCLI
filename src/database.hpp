#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

class User {
    private:
        int m_user_id{};
        std::string m_name{};

    protected:
        bool m_online_status{true};

    public:
        static int m_next_user_id;
        User() = default;
        User(std::string name) : m_user_id{++m_next_user_id}, m_name{name} {}
        User(int id, std::string name, bool status) 
            : m_user_id{id}, m_name{name}, m_online_status{status} {}


        int getUserId() const {return m_user_id;}
        std::string getUserName() const {return m_name;}
        std::string getUserNameFromId(int id) const {return m_name;}
        bool getUserStatus() const {return m_online_status;}
        void setUserStatus(bool val) {m_online_status = val;}


        //serialize to store info in file
        std::string serialize() const {
            return std::to_string(m_user_id) + "|" + m_name; 
        }
        
        //deserialize input from files
        static User deserialize(const std::string& data) {
            std::istringstream ss(data);
            std::string id_str, name, status_str;
            
            std::getline(ss, id_str, '|');
            std::getline(ss, name, '|');
            std::getline(ss, status_str, '|');
            
            int id = std::stoi(id_str);
            bool status = (status_str == "1");
            
            return User(id, name, false); //always offline status at server start
        }
};

int User::m_next_user_id{0};


class Post {
    private:
        int m_post_id{};
        User m_author{};
        std::string m_captions{};
        std::string m_mediaPath{};
        std::string m_timestamp{};
        
    public:
        static int m_next_post_id;
        Post() = default;
        Post(User user, std::string cap, std::string media, std::string time) 
            : m_author{user}, m_captions{cap}, m_mediaPath{media}, m_timestamp{time} {}
        Post(int id, User user, std::string cap, std::string media, std::string time)
            : m_post_id{id}, m_author{user}, m_captions{cap}, 
              m_mediaPath{media}, m_timestamp{time} {}


        int getPostId() const {return m_post_id;}
        bool hasMedia() const {return !m_mediaPath.empty();}
        std::string getAuthorName() const {return m_author.getUserName();}
        std::string getCaptions() const {return m_captions;}
        std::string getMediaPath() const {return m_mediaPath;}
        std::string getTimestamp() const {return m_timestamp;}

        //serilize for file storage
        std::string serialize() const {
            return std::to_string(m_post_id) + "|" + m_author.getUserName() + "|" + 
                   m_captions + "|" + m_mediaPath + "|" + m_timestamp;
        }

        //deserialize(parse) info from file
        static Post deserialize(const std::string& data, const std::vector<User>& users) {
            std::istringstream ss(data);
            std::string id_str, author_name, captions, media, timestamp;
            
            std::getline(ss, id_str, '|');
            std::getline(ss, author_name, '|');
            std::getline(ss, captions, '|');
            std::getline(ss, media, '|');
            std::getline(ss, timestamp, '|');
            
            int id = std::stoi(id_str);
            
            // Find user by name
            User author;
            for (const User& u : users) {
                if (u.getUserName() == author_name) {
                    author = u;
                    break;
                }
            }
            
            return Post(id, author, captions, media, timestamp);
        }

};

int Post::m_next_post_id{0};

class Message {
    private:
        int m_message_id{};
        std::string m_sender{};
        std::string m_receiver{};
        std::string m_content{};
        std::string m_timestamp{};

    public:
        static int m_next_message_id;
        Message() = default;
        Message(std::string sender, std::string receiver, std::string content, std::string timestamp)
            : m_message_id{++m_next_message_id}, m_sender{sender}, m_receiver{receiver}, 
              m_content{content}, m_timestamp{timestamp} {}
        Message(int id, std::string sender, std::string receiver, std::string content, std::string timestamp)
            : m_message_id{id}, m_sender{sender}, m_receiver{receiver}, 
              m_content{content}, m_timestamp{timestamp} {}
        
        int getMessageId() const {return m_message_id;}
        std::string getSender() const {return m_sender;}
        std::string getReceiver() const {return m_receiver;}
        std::string getContent() const {return m_content;}
        std::string getTimestamp() const {return m_timestamp;}


        std::string serialize() const {
            return std::to_string(m_message_id) + "|" + m_sender + "|" + 
                   m_receiver + "|" + m_content + "|" + m_timestamp;
        }
        
        static Message deserialize(const std::string& data) {
            std::istringstream ss(data);
            std::string id_str, sender, receiver, content, timestamp;
            
            std::getline(ss, id_str, '|');
            std::getline(ss, sender, '|');
            std::getline(ss, receiver, '|');
            std::getline(ss, content, '|');
            std::getline(ss, timestamp, '|');
            
            int id = std::stoi(id_str);
            
            return Message(id, sender, receiver, content, timestamp);
        }
};

int Message::m_next_message_id{0};

class Database {
    private:
        std::vector<User> registered_users{};
        std::vector<Post> registered_posts{};
        std::vector<Message> message_history{};


        const std::string USERS_FILE = "data/users.txt";
        const std::string POSTS_FILE = "data/posts.txt";
        const std::string MESSAGES_FILE = "data/messages.txt";
        
        void ensureDataDirectory() {
            struct stat st{};
            if (stat("data", &st) == -1) {
                mkdir("data", 0755);
            }
        }

    public:

        Database() {
            ensureDataDirectory();
            loadFromFiles();
        }
        
        ~Database() {
            saveToFiles();
        }

        void loadFromFiles() {
            loadUsers();
            loadPosts();
            loadMessages();
        }
        
        void saveToFiles() {
            saveUsers();
            savePosts();
            saveMessages();
        }

        void loadUsers() {
            std::ifstream file(USERS_FILE);
            if (!file.is_open()) return;
            
            std::string line;
            int max_id = 0;
            
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                User user = User::deserialize(line);
                registered_users.push_back(user);
                
                if (user.getUserId() > max_id) {
                    max_id = user.getUserId();
                }
            }
            
            User::m_next_user_id = max_id;
            file.close();
        }

        void saveUsers() {
            std::ofstream file(USERS_FILE);
            if (!file.is_open()) return;
            
            for (const User& user : registered_users) {
                file << user.serialize() << "\n";
            }
            
            file.close();
        }

        void loadPosts() {
            std::ifstream file(POSTS_FILE);
            if (!file.is_open()) return;
            
            std::string line;
            int max_id = 0;
            
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                Post post = Post::deserialize(line, registered_users);
                registered_posts.push_back(post);
                
                if (post.getPostId() > max_id) {
                    max_id = post.getPostId();
                }
            }
            
            Post::m_next_post_id = max_id;
            file.close();
        }

        void savePosts() {
            std::ofstream file(POSTS_FILE);
            if (!file.is_open()) return;
            
            for (const Post& post : registered_posts) {
                file << post.serialize() << "\n";
            }
            
            file.close();
        }

        void loadMessages() {
            std::ifstream file(MESSAGES_FILE);
            if (!file.is_open()) return;
            
            std::string line;
            int max_id = 0;
            
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                Message msg = Message::deserialize(line);
                message_history.push_back(msg);
                
                if (msg.getMessageId() > max_id) {
                    max_id = msg.getMessageId();
                }
            }
            
            Message::m_next_message_id = max_id;
            file.close();
        }
        
        void saveMessages() {
            std::ofstream file(MESSAGES_FILE);
            if (!file.is_open()) return;
            
            for (const Message& msg : message_history) {
                file << msg.serialize() << "\n";
            }
            
            file.close();
        }

        bool login(std::string name) {
            
            for(User& user: registered_users) {
                if(user.getUserName() == name)
                    return true;    //return true if login
            }

            User newUser(name);
            newUser.setUserStatus(true);
            registered_users.push_back(newUser);
            saveUsers();

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
                    savePosts();
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
            saveMessages();
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
