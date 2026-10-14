#ifndef REST_API_SERVER_HPP
#define REST_API_SERVER_HPP

#include <string>
#include "LotusNetwork.hpp"

class RestApiServer {
private:
    int serverFd;
    int port;
    bool isRunning;
    LotusNetwork& networkNode;

public:
    RestApiServer(int p, LotusNetwork& node);
    void start();
    std::string handleRoute(const std::string& req);
};

#endif