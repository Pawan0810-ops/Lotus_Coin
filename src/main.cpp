#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "LotusNetwork.hpp"
#include "RestApiServer.hpp"

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cout << "Usage: ./lotus_node <HTTP_PORT> <P2P_PORT> <PEER_TARGET_PORT> <DB_PATH>" << std::endl;
        return 1;
    }

    int httpPort = std::atoi(argv[1]);
    int p2pPort = std::atoi(argv[2]);
    int peerTargetPort = std::atoi(argv[3]);
    std::string dbPath = argv[4];

    std::cout << "==================================================" << std::endl;
    std::cout << "   Lotus Coin Dynamic Multi-Node P2P Network      " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "HTTP Port: " << httpPort << " | P2P Port: " << p2pPort 
              << " | Peer Target: " << peerTargetPort << std::endl;

    LotusNetwork node(p2pPort, peerTargetPort, dbPath);
    RestApiServer apiServer(httpPort, node);

    apiServer.start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}