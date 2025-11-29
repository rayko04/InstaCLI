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


class Database {
    private:
        std::vector<User> registered_users{};

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
};
