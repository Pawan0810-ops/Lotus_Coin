#include "RestApiServer.hpp"
#include "CryptoUtils.hpp"
#include <iostream>
#include <thread>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

RestApiServer::RestApiServer(int p, LotusNetwork& node) : serverFd(-1), port(p), isRunning(false), networkNode(node) {}

void RestApiServer::start() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cout << "[REST API ERROR] Port " << port << " occupied" << std::endl;
        return;
    }

    listen(serverFd, 10);
    isRunning = true;
    std::cout << "[HTTP REST API] Active on http://127.0.0.1:" << port << std::endl;

    std::thread([this]() {
        while (isRunning) {
            sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            int clientSocket = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket >= 0) {
                char buffer[2048] = {0};
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

                if (bytesRead > 0) {
                    std::string req(buffer);
                    std::string body = handleRoute(req) + "\n";

                    std::string httpResponse = 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n"
                        "Content-Length: " + std::to_string(body.length()) + "\r\n\r\n" +
                        body;

                    send(clientSocket, httpResponse.c_str(), httpResponse.length(), 0);
                }
                close(clientSocket);
            }
        }
    }).detach();
}

std::string RestApiServer::handleRoute(const std::string& req) {
    if (req.find("GET /api/status") != std::string::npos) {
        return "{\"status\":\"ONLINE\",\"chain_length\":" + std::to_string(networkNode.getChainLength()) + 
               ",\"mempool_pending_txs\":" + std::to_string(networkNode.getMempoolSize()) +
               ",\"latest_block_hash\":\"" + networkNode.getLatestBlockHash() + "\"" +
               ",\"treasury_pool\":" + std::to_string(networkNode.getTreasuryPool()) + 
               ",\"staked_amount\":" + std::to_string(networkNode.getStake("LOTUS_HTTP_MINER_WALLET")) + "}";
    } 
    else if (req.find("GET /api/mine") != std::string::npos) {
        std::string minedHash = networkNode.mineBlock("LOTUS_HTTP_MINER_WALLET");
        return "{\"message\":\"Block Mined with High Priority Mempool Txs & Broadcasted\",\"mined_hash\":\"" + minedHash + "\"}";
    }
    else if (req.find("GET /api/wallet/generate") != std::string::npos) {
        std::string mnemonic = CryptoUtils::generateMnemonic();
        auto [priv, pub] = CryptoUtils::generateKeyPair();
        std::string addr = "LOTUS_" + CryptoUtils::sha256(pub + mnemonic).substr(0, 16);
        
        networkNode.setBalance(addr, 500.0);
        CryptoUtils::saveWalletToFile("wallet.json", addr, mnemonic, priv, pub);

        return "{\"status\":\"WALLET_CREATED\",\"address\":\"" + addr + 
               "\",\"mnemonic\":\"" + mnemonic + 
               "\",\"private_key\":\"" + priv + 
               "\",\"file\":\"wallet.json saved in build folder\"}";
    }
    else if (req.find("/api/send") != std::string::npos) {
        auto [testPriv, testPub] = CryptoUtils::generateKeyPair();
        std::string testAddr = "LOTUS_" + CryptoUtils::sha256(testPub).substr(0, 16);
        networkNode.setBalance(testAddr, 100.0);

        double dynamicGasFee = 2.0 + (rand() % 9);
        Transaction testTx{testPub, "LOTUS_BOB_WALLET_999", 25.0, dynamicGasFee, ""};
        testTx.signature = CryptoUtils::signMessage(testPriv, testTx.getPayloadHash());

        bool success = networkNode.submitToMempool(testTx);
        if (success) {
            return "{\"status\":\"QUEUED_IN_MEMPOOL\",\"gas_fee\":" + std::to_string(dynamicGasFee) + ",\"tx_hash\":\"" + testTx.getPayloadHash() + "\",\"message\":\"Transaction prioritized in Mempool queue\"}";
        } else {
            return "{\"status\":\"REJECTED\",\"message\":\"Transaction rejected\"}";
        }
    }
    else if (req.find("/api/stake") != std::string::npos) {
        networkNode.setBalance("LOTUS_HTTP_MINER_WALLET", 100.0);
        bool success = networkNode.stakeTokens("LOTUS_HTTP_MINER_WALLET", 25.0);
        if (success) {
            return "{\"status\":\"SUCCESS\",\"message\":\"25.0 LOTUS Staked to Network Validator Node!\"}";
        } else {
            return "{\"status\":\"FAILED\",\"message\":\"Insufficient balance to stake\"}";
        }
    }
    else if (req.find("/api/unstake") != std::string::npos) {
        bool success = networkNode.unstakeTokens("LOTUS_HTTP_MINER_WALLET", 25.0);
        if (success) {
            return "{\"status\":\"SUCCESS\",\"message\":\"25.0 LOTUS Unstaked and returned to wallet!\"}";
        } else {
            return "{\"status\":\"FAILED\",\"message\":\"No active stake found\"}";
        }
    }
    else if (req.find("GET /api/blocks") != std::string::npos) {
        return networkNode.getChainJSON();
    }

    return "{\"error\":\"Endpoint Not Found\"}";
}