#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>

class User{
    private:
        static int m_next_user_id;
        int m_user_id{};
        std::string m_name{};
        std::vector<int> m_followers{};
        std::vector<int> m_following{};

    public:
        User() = default;
        User(std::string name) : m_user_id{++m_next_user_id}, m_name{name} {}

        int getUserId() const {return m_user_id;}
        void follow(int follow_id) {m_following.push_back(follow_id);}  
        void unfollow(int unfollow_id) {}               //INCOMPLETE
};

int User::m_next_user_id{0};

class Post {
    private: 
        static int m_next_post_id;
        int m_post_id{};
        int m_likes_count{};
        std::string m_content{};
        int m_author_id{};

    public:
        Post() = default;
        Post(int user_id, std::string content) : m_post_id{++m_next_post_id}, m_likes_count{0}, m_content{content}, m_author_id{user_id} {}

        int getPostId() const {return m_post_id;}
        void like() {++m_likes_count;}
        std::string getContent() const {return m_content;}
        int getAuthorId() const {return m_author_id;}
};

int Post::m_next_post_id{0};

std::unordered_map<int, User> users{};
std::unordered_map<int, Post> posts{};
std::unordered_map<int, std::vector<int>> followers{};

void createUser()
{
    std::cout << "Name: ";
    std::string name;
    std::cin >> name;

    User u{name};
    users[u.getUserId()] = u;

    std::cout << "User created: " << name << " (id=" << u.getUserId() << ")\n"; 
}

void createPost()
{
    int uid{};
    std::string content{};

    std::cout << "User ID: ";
    std::cin >> uid;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << std::endl;
    std::cout << "Post Contents: ";
    std::getline(std::cin, content);

    Post p{uid, content};
    posts[p.getPostId()] = p;

    std::cout << "Post created by user " << uid << std::endl;

}

void showAllPosts()
{
    std::cout << "\nALL POSTS\n\n";

    for (const auto& [id, post]: posts)
    {
        std::cout << "Post (id=" << id << ") by user " << post.getAuthorId() << ": \n" << post.getContent() << "\n"; 
    }
}

int main()
{
   

    std::cout << "WELCOME TO INSTACLI!\n";
    
    int command{};

    while (true)
    {
        std::cout << "\nChoose: \n1)Create User \n2)Create Post \n3)Show All Posts \n4)Exit\n\n";
        std::cin >> command;

        switch(command)
        {
            case 1: 
                createUser();
                break;

            case 2:
                createPost();
                break;

            case 3: 
                showAllPosts();
                break;

            case 4:
                return 0;
            
            default: return 0;
        }
            
    };
    return 0;
}
