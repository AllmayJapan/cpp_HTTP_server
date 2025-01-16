#ifndef SERVER_HPP
#define SERVER_HPP

class Server {
    public:
        Server(int port);
        void run();

    private:
        int port;
};

#endif // SERVER_HPP
