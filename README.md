# InstaCLI

InstaCLI is a command-line, multi-threaded client-server application that mimics a simplified version of social media platforms like Instagram. It is built in C++ and demonstrates core networking concepts, including TCP sockets, multi-client handling, and basic data persistence. All communication between the client and server is encrypted using a simple XOR cipher.

## Features

*   **User System**: Register a new username or log in with an existing one.
*   **Feed**: View a chronological feed of all posts from every user.
*   **Posting**: Create text-based posts with captions.
*   **Media Sharing**: Attach and upload media files (images, videos, etc.) to your posts.
*   **Direct Messaging**: Send and receive private messages between users. The system supports offline messaging by storing messages on the server.
*   **Inbox**: View your conversation list and browse chat history with other users.
*   **File Downloads**: Download media files from posts directly to a local `downloads` directory.
*   **Encrypted Communication**: All data exchanged between the client and server is encrypted.
*   **Concurrent Users**: The server uses `std::thread` to handle multiple clients simultaneously. The client uses a separate thread to receive real-time message notifications without blocking the main user interface.
*   **Data Persistency**: A cutom database class that handles User, Post, Message persistency. Only the Database class is accesible to the Server, hiding implementation details.

## Architecture

The application follows a classic client-server model over TCP.

### Server
The server is responsible for managing the application's state and data.
*   **Multi-Client Handling**: Listens for incoming connections and spawns a new thread for each connected client.
*   **User Management**: Maintains a map of currently online users and their associated socket descriptors for real-time communication.
*   **Command Processing**: Parses and executes commands received from clients, such as `LOGIN`, `POST`, `FEED`, `SEND`, and `DOWNLOAD`.
*   **Database**: Interacts with a simple file-based database (`database.hpp`) to store and retrieve user information, posts, and message history. Data is saved in the `data/` directory.
*   **File Storage**: Manages file uploads from clients, saving them with unique names in the `uploads/` directory.

### Client
The client provides the user interface to interact with the system.
*   **Connection**: Establishes a TCP connection with the server.
*   **User Interface**: A menu-driven command-line interface allows users to navigate features like viewing the feed, posting, and messaging.
*   **Asynchronous Receiver**: A dedicated thread runs in the background to listen for incoming messages and notifications from the server, displaying them to the user without interrupting their current action.
*   **File Transfer**: Implements logic to send local files to the server when creating a post and to receive and save files when downloading from the feed.

### Data Persistence
The server uses a simple file-based system to persist data:
*   `data/users.txt`: Stores registered usernames.
*   `data/posts.txt`: Stores all posts, including author, caption, media path, and timestamp.
*   `data/messages.txt`: Stores the entire message history for all users.

The server creates the `data/` directory if it does not exist. Similarly, it creates `uploads/` for storing media, and the client creates `downloads/` for saved media.

## Getting Started

### Prerequisites
*   A C++ compiler that supports C++11 (e.g., g++).
*   Standard C++ libraries and POSIX libraries for networking (`sys/socket`, `netinet/in`, etc.).

### Compilation

Open your terminal and compile the server and client programs separately.

1.  **Compile the Server:**
    ```bash
    g++ -std=c++11 -pthread src/Server.cpp -o Server
    ```

2.  **Compile the Client:**
    ```bash
    g++ -std=c++11 -pthread src/Client.cpp -o Client
    ```

### Running the Application

1.  **Start the Server**:
    Run the server executable in a terminal window. It will start listening for connections on port 5555.
    ```bash
    ./Server
    ```
    You will see the output: `Encrypted Server listening on port 5555`.

2.  **Run the Client(s)**:
    Open one or more new terminal windows and run the client executable. Each instance will act as a separate user.
    ```bash
    ./Client
    ```
    You will be prompted to log in or register a new username. From there, you can navigate the application using the on-screen menu.


## What I Learnt

* Basics of Network Programming
* Threading and Synchronization handling
* Client-Server Architecture
