#ifndef P2P_SERVER_HPP
#define P2P_SERVER_HPP

#include <string>
#include <functional>

class P2PServer {
private:
    int serverFd;
    int port;
    bool isRunning;

public:
    P2PServer(int p);
    void start(std::function<void(const std::string&)> onMessage);
    static void sendToPeer(int peerPort, const std::string& packet);
};

#endif