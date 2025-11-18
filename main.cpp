#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <unordered_set>

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
        std::unordered_set<int> m_liked_by{};

    public:
        Post() = default;
        Post(int user_id, std::string content) : m_post_id{++m_next_post_id}, m_likes_count{0}, m_content{content}, m_author_id{user_id} {}

        int getPostId() const {return m_post_id;}
        
        std::string getContent() const {return m_content;}
        int getAuthorId() const {return m_author_id;}
        int getLikesCount() const {return m_likes_count;}
        
        bool like(int userId) {
            if(m_liked_by.count(userId))    return false;
            ++m_likes_count;
            m_liked_by.insert(userId);

            return true;
        }
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

    std::cout << "Post(id=" << p.getPostId()  << ") created by user " << uid << std::endl;

}

void showAllPosts()
{
    std::cout << "\nALL POSTS\n\n";

    for (const auto& [id, post]: posts)
    {
        std::cout << "Post (id=" << id << ") by user " << post.getAuthorId()  << ". Total Likes = " << post.getLikesCount() <<  "\n" << post.getContent() << "\n"; 
    }
}

void likePost()             
{
    int userId{}, postId{};
    std::cout << "User ID: ";
    std::cin >> userId;

    if(users.find(userId) == users.end()) 
    {
        std::cout << "No user by the id "  << userId << std::endl;
        return;
    }

    std::cout << "Post ID: ";
    std::cin >> postId;

    auto it = posts.find(postId);
    if(it != posts.end())
    {
        if(it->second.like(userId))
            std::cout << "User " << userId << " liked post " << postId << " (Total likes: " << it->second.getLikesCount() << ")\n";
        else
            std::cout << "User " << userId << " has already liked the post (id=" << postId << ").\n";

    }
    else
        std::cout << "Post ID not found!/n";
}

void follow()
{
    
}

int main()
{
   

    std::cout << "WELCOME TO INSTACLI!\n";
    
    int command{};

    while (true)
    {
        std::cout << "\nChoose: \n1)Create User \n2)Create Post \n3)Like a Post \n4)Follow \n5)Show all posts \n6)Exit\n\n";
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
                likePost();
                break;
            
            case 4:
                follow();
                break;

            case 5:
                showAllPosts();
                break;

            case 6:
                return 0;
            
            default: return 0;
        }
            
    };
    return 0;
}
