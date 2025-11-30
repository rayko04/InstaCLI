#include <string>
#include <vector>


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
        User m_author{};
        std::string m_captions{};
        std::string m_mediaPath{};
        std::string m_timestamp{};

    public:
        Post() = default;
        Post(User user, std::string cap, std::string media, std::string time) 
            : m_author{user}, m_captions{cap}, m_mediaPath{media}, m_timestamp{time} {}

        std::string getAuthorName() const {return m_author.getUserName();}
        std::string getCaptions() const {return m_captions;}
        std::string getMediaPath() const {return m_mediaPath;}
        std::string getTimestamp() const {return m_timestamp;}

};

class Database {
    private:
        std::vector<User> registered_users{};
        std::vector<Post> registered_posts{};

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
};
