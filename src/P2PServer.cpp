#include "P2PServer.hpp"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

P2PServer::P2PServer(int p) : serverFd(-1), port(p), isRunning(false) {}

void P2PServer::start(std::function<void(const std::string&)> onMessage) {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cout << "[P2P ERROR] P2P Port " << port << " failed to bind" << std::endl;
        return;
    }

    listen(serverFd, 5);
    isRunning = true;
    std::cout << "[P2P LISTENER] Listening for Peer Nodes on Port " << port << std::endl;

    std::thread([this, onMessage]() {
        while (isRunning) {
            sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            int clientSocket = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket >= 0) {
                char buffer[2048] = {0};
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead > 0) {
                    onMessage(std::string(buffer));
                }
                close(clientSocket);
            }
        }
    }).detach();
}

void P2PServer::sendToPeer(int peerPort, const std::string& packet) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(peerPort);
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) >= 0) {
        send(sock, packet.c_str(), packet.length(), 0);
        std::cout << "\n[P2P BROADCAST SENT] Packet -> 127.0.0.1:" << peerPort << std::endl;
    } else {
        std::cout << "\n[P2P NOTICE] Peer 127.0.0.1:" << peerPort << " unreachable." << std::endl;
    }
    close(sock);
}