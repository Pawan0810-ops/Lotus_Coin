#ifndef LOTUS_ENGINE_HPP
#define LOTUS_ENGINE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdint>
#include <fstream>
#include <thread>
#include <mutex>
#include <cstring>
#include <functional>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <leveldb/db.h>

class CryptoUtils {
public:
    static std::string sha256(const std::string& input) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    static std::string doubleSHA256(const std::string& input) {
        return sha256(sha256(input));
    }

    static std::pair<std::string, std::string> generateKeyPair() {
        EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!ecKey) return {"", ""};
        EC_KEY_generate_key(ecKey);

        const BIGNUM* privBn = EC_KEY_get0_private_key(ecKey);
        char* privHexChar = BN_bn2hex(privBn);
        std::string privateKeyHex = privHexChar ? privHexChar : "";
        if (privHexChar) OPENSSL_free(privHexChar);

        const EC_POINT* pubPoint = EC_KEY_get0_public_key(ecKey);
        const EC_GROUP* group = EC_KEY_get0_group(ecKey);
        char* pubHexChar = EC_POINT_point2hex(group, pubPoint, POINT_CONVERSION_UNCOMPRESSED, nullptr);
        std::string publicKeyHex = pubHexChar ? pubHexChar : "";
        if (pubHexChar) OPENSSL_free(pubHexChar);

        EC_KEY_free(ecKey);
        return {privateKeyHex, publicKeyHex};
    }

    static std::string signMessage(const std::string& privateKeyHex, const std::string& message) {
        if (privateKeyHex.empty()) return "";
        std::string msgHash = doubleSHA256(message);

        EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!ecKey) return "";

        BIGNUM* privBn = nullptr;
        if (!BN_hex2bn(&privBn, privateKeyHex.c_str()) || !privBn) {
            EC_KEY_free(ecKey);
            return "";
        }

        EC_KEY_set_private_key(ecKey, privBn);

        const EC_GROUP* group = EC_KEY_get0_group(ecKey);
        EC_POINT* pubPoint = EC_POINT_new(group);
        if (!pubPoint) {
            BN_free(privBn);
            EC_KEY_free(ecKey);
            return "";
        }

        EC_POINT_mul(group, pubPoint, privBn, nullptr, nullptr, nullptr);
        EC_KEY_set_public_key(ecKey, pubPoint);

        unsigned char sig[128];
        unsigned int sigLen = 0;
        ECDSA_sign(0, reinterpret_cast<const unsigned char*>(msgHash.c_str()), msgHash.length(), sig, &sigLen, ecKey);

        std::stringstream ss;
        for (unsigned int i = 0; i < sigLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)sig[i];
        }

        EC_POINT_free(pubPoint);
        BN_free(privBn);
        EC_KEY_free(ecKey);
        return ss.str();
    }

    static bool verifySignature(const std::string& publicKeyHex, const std::string& message, const std::string& signatureHex) {
        if (signatureHex.empty() || publicKeyHex.empty()) return false;

        std::string msgHash = doubleSHA256(message);

        EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!ecKey) return false;

        const EC_GROUP* group = EC_KEY_get0_group(ecKey);
        EC_POINT* pubPoint = EC_POINT_new(group);
        
        if (!pubPoint || !EC_POINT_hex2point(group, publicKeyHex.c_str(), pubPoint, nullptr)) {
            if (pubPoint) EC_POINT_free(pubPoint);
            EC_KEY_free(ecKey);
            return false;
        }

        EC_KEY_set_public_key(ecKey, pubPoint);

        std::vector<unsigned char> sigBytes;
        for (size_t i = 0; i < signatureHex.length(); i += 2) {
            std::string byteString = signatureHex.substr(i, 2);
            unsigned char byte = (unsigned char) strtol(byteString.c_str(), nullptr, 16);
            sigBytes.push_back(byte);
        }

        int result = ECDSA_verify(0, reinterpret_cast<const unsigned char*>(msgHash.c_str()), msgHash.length(), sigBytes.data(), sigBytes.size(), ecKey);

        EC_POINT_free(pubPoint);
        EC_KEY_free(ecKey);

        return (result == 1);
    }
};

class LevelDBStorage {
private:
    leveldb::DB* db;

public:
    LevelDBStorage(const std::string& dbPath) : db(nullptr) {
        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(options, dbPath, &db);
        if (!status.ok()) {
            db = nullptr;
        } else {
            std::cout << "[LEVELDB STORAGE] DB active at: '" << dbPath << "'" << std::endl;
        }
    }

    ~LevelDBStorage() {
        if (db) delete db;
    }

    bool put(const std::string& key, const std::string& value) {
        if (!db) return false;
        return db->Put(leveldb::WriteOptions(), key, value).ok();
    }

    std::string get(const std::string& key) {
        if (!db) return "NOT_FOUND";
        std::string val;
        leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &val);
        return s.ok() ? val : "NOT_FOUND";
    }
};

struct Transaction {
    std::string senderPublicKey;
    std::string recipient;
    double amount;
    double gasFee;
    std::string signature;

    std::string getPayloadHash() const {
        return CryptoUtils::doubleSHA256(senderPublicKey + recipient + std::to_string(amount) + std::to_string(gasFee));
    }
};

class Block {
public:
    uint32_t index;
    uint64_t timestamp;
    std::string previousHash;
    std::string blockHash;
    uint64_t nonce;
    std::vector<Transaction> transactions;

    Block(uint32_t idx, std::string prevHash, std::vector<Transaction> txs)
        : index(idx), previousHash(prevHash), transactions(txs), nonce(0) {
        timestamp = std::time(nullptr);
        blockHash = calculateHash();
    }

    std::string calculateHash() const {
        std::string txData = "";
        for (const auto& tx : transactions) {
            txData += tx.getPayloadHash();
        }
        std::string header = std::to_string(index) + std::to_string(timestamp) + previousHash + txData + std::to_string(nonce);
        return CryptoUtils::doubleSHA256(header);
    }
};

class P2PServer {
private:
    int serverFd;
    int port;
    bool isRunning;

public:
    P2PServer(int p) : port(p), isRunning(false) {}

    void start(std::function<void(const std::string&)> onMessage) {
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

    static void sendToPeer(int peerPort, const std::string& packet) {
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
};

class LotusNetwork {
private:
    std::vector<Block> chain;
    std::unordered_map<std::string, double> balances;
    std::vector<Transaction> mempool; // Pending Transaction Holding Queue
    double treasuryPool;
    LevelDBStorage dbEngine;
    P2PServer p2pListener;
    int targetPeerPort;
    std::mutex mtx;

public:
    LotusNetwork(int p2pPort, int peerPort, const std::string& dbPath)
        : treasuryPool(0.0), dbEngine(dbPath), p2pListener(p2pPort), targetPeerPort(peerPort) {

        Transaction genesisTx{"GENESIS_PUBKEY", "LOTUS_SYSTEM_TREASURY", 21000000.0, 0.0, "GENESIS_SIG"};
        Block genesisBlock(0, "0000000000000000000000000000000000000000000000000000000000000000", {genesisTx});
        chain.push_back(genesisBlock);
        balances["LOTUS_SYSTEM_TREASURY"] = 21000000.0;

        dbEngine.put("block_0", genesisBlock.blockHash);
        dbEngine.put("bal_LOTUS_SYSTEM_TREASURY", "21000000.0");

        p2pListener.start([this](const std::string& packet) {
            std::cout << "\n[P2P PACKET RECEIVED] " << packet << std::endl;
            this->handleIncomingP2PPacket(packet);
        });
    }

    void handleIncomingP2PPacket(const std::string& packet) {
        std::lock_guard<std::mutex> lock(mtx);
        
        if (packet.rfind("NEW_BLOCK", 0) == 0) {
            size_t idxPos = packet.find("INDEX=");
            size_t hashPos = packet.find("HASH=");

            if (idxPos != std::string::npos && hashPos != std::string::npos) {
                std::string idxStr = packet.substr(idxPos + 6, hashPos - (idxPos + 7));
                std::string blockHash = packet.substr(hashPos + 5);

                uint32_t blockIdx = std::stoi(idxStr);

                Transaction p2pTx{"NETWORK_SYNC", "LOTUS_P2P_SYNC_MINER", 50.0, 0.0, "P2P_SYNC_SIG"};
                Block syncedBlock(blockIdx, chain.back().blockHash, {p2pTx});
                syncedBlock.blockHash = blockHash;
                
                chain.push_back(syncedBlock);

                dbEngine.put("block_" + idxStr, blockHash);
                dbEngine.put("latest_block_hash", blockHash);

                std::cout << ">>> [P2P LEDGER SYNC] Block #" << blockIdx << " Synced into RAM & LevelDB! (Hash: " << blockHash.substr(0, 16) << "...)" << std::endl;
            }
        }
        else if (packet.rfind("NEW_TX", 0) == 0) {
            std::cout << ">>> [P2P MEMPOOL SYNC] Pending TX synced into local Mempool Queue." << std::endl;
        }
    }

    // Add TX to Mempool & Sort by Highest Gas Fee First
    bool submitToMempool(Transaction tx, bool broadcast = true) {
        std::lock_guard<std::mutex> lock(mtx);
        std::string senderAddr = "LOTUS_" + CryptoUtils::sha256(tx.senderPublicKey).substr(0, 16);

        if (tx.amount <= 0.0 || balances[senderAddr] < (tx.amount + tx.gasFee)) {
            return false;
        }

        bool isSigValid = CryptoUtils::verifySignature(tx.senderPublicKey, tx.getPayloadHash(), tx.signature);
        if (!isSigValid) return false;

        mempool.push_back(tx);

        // Sort Mempool Descending: High Gas Fee Transactions First
        std::sort(mempool.begin(), mempool.end(), [](const Transaction& a, const Transaction& b) {
            return a.gasFee > b.gasFee;
        });

        std::cout << "[MEMPOOL ADDED] Tx Hash: " << tx.getPayloadHash().substr(0, 12) 
                  << "... | Gas Fee: " << tx.gasFee << " LOTUS | Mempool Size: " << mempool.size() << std::endl;

        if (broadcast) {
            std::string pkt = "NEW_TX:" + tx.getPayloadHash() + ":FEE=" + std::to_string(tx.gasFee);
            P2PServer::sendToPeer(targetPeerPort, pkt);
        }

        return true;
    }

    // Mine Block with Top High-Priority Transactions from Mempool
    std::string mineBlock(const std::string& minerAddress, bool broadcast = true) {
        std::lock_guard<std::mutex> lock(mtx);
        
        std::vector<Transaction> blockTxs;
        double totalBlockFees = 0.0;

        // Pick top 5 transactions with highest gas fees from Mempool
        size_t txsToPick = std::min(mempool.size(), static_cast<size_t>(5));
        for (size_t i = 0; i < txsToPick; ++i) {
            Transaction tx = mempool[i];
            std::string senderAddr = "LOTUS_" + CryptoUtils::sha256(tx.senderPublicKey).substr(0, 16);

            if (balances[senderAddr] >= (tx.amount + tx.gasFee)) {
                balances[senderAddr] -= (tx.amount + tx.gasFee);
                balances[tx.recipient] += tx.amount;
                totalBlockFees += tx.gasFee;
                blockTxs.push_back(tx);

                dbEngine.put("bal_" + senderAddr, std::to_string(balances[senderAddr]));
                dbEngine.put("bal_" + tx.recipient, std::to_string(balances[tx.recipient]));
            }
        }

        // Erase picked transactions from Mempool
        mempool.erase(mempool.begin(), mempool.begin() + txsToPick);

        // Add Miner Reward + Total Collected Gas Fees
        double totalMinerReward = 50.0 + totalBlockFees;
        Transaction rewardTx{"NETWORK_REWARD", minerAddress, totalMinerReward, 0.0, "REWARD_SIG"};
        blockTxs.push_back(rewardTx);

        Block newBlock(chain.size(), chain.back().blockHash, blockTxs);
        
        while (newBlock.calculateHash().substr(0, 4) != "0000") {
            newBlock.nonce++;
        }
        newBlock.blockHash = newBlock.calculateHash();
        chain.push_back(newBlock);

        balances[minerAddress] += totalMinerReward;
        treasuryPool += totalBlockFees;

        dbEngine.put("block_" + std::to_string(newBlock.index), newBlock.blockHash);
        dbEngine.put("bal_" + minerAddress, std::to_string(balances[minerAddress]));
        dbEngine.put("treasury_pool", std::to_string(treasuryPool));

        if (broadcast) {
            std::string pkt = "NEW_BLOCK:INDEX=" + std::to_string(newBlock.index) + ":HASH=" + newBlock.blockHash;
            P2PServer::sendToPeer(targetPeerPort, pkt);
        }

        return newBlock.blockHash;
    }

    size_t getMempoolSize() { std::lock_guard<std::mutex> lock(mtx); return mempool.size(); }
    std::string fetchKeyFromDB(const std::string& key) { return dbEngine.get(key); }
    void setBalance(const std::string& address, double amount) { 
        std::lock_guard<std::mutex> lock(mtx);
        balances[address] = amount; 
        dbEngine.put("bal_" + address, std::to_string(amount));
    }
    double getBalance(const std::string& address) { std::lock_guard<std::mutex> lock(mtx); return balances[address]; }
    double getTreasuryPool() { std::lock_guard<std::mutex> lock(mtx); return treasuryPool; }
    size_t getChainLength() { std::lock_guard<std::mutex> lock(mtx); return chain.size(); }
    std::string getLatestBlockHash() { std::lock_guard<std::mutex> lock(mtx); return chain.back().blockHash; }
};

class RestApiServer {
private:
    int serverFd;
    int port;
    bool isRunning;
    LotusNetwork& networkNode;

public:
    RestApiServer(int p, LotusNetwork& node) : port(p), isRunning(false), networkNode(node) {}

    void start() {
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

    std::string handleRoute(const std::string& req) {
        if (req.find("GET /api/status") != std::string::npos) {
            return "{\"status\":\"ONLINE\",\"chain_length\":" + std::to_string(networkNode.getChainLength()) + 
                   ",\"mempool_pending_txs\":" + std::to_string(networkNode.getMempoolSize()) +
                   ",\"latest_block_hash\":\"" + networkNode.getLatestBlockHash() + "\"" +
                   ",\"treasury_pool\":" + std::to_string(networkNode.getTreasuryPool()) + "}";
        } 
        else if (req.find("GET /api/mine") != std::string::npos) {
            std::string minedHash = networkNode.mineBlock("LOTUS_HTTP_MINER_WALLET");
            return "{\"message\":\"Block Mined with High Priority Mempool Txs & Broadcasted\",\"mined_hash\":\"" + minedHash + "\"}";
        }
        else if (req.find("GET /api/wallet/generate") != std::string::npos) {
            auto [priv, pub] = CryptoUtils::generateKeyPair();
            std::string addr = "LOTUS_" + CryptoUtils::sha256(pub).substr(0, 16);
            networkNode.setBalance(addr, 200.0);
            return "{\"private_key\":\"" + priv + "\",\"public_key\":\"" + pub + "\",\"address\":\"" + addr + "\",\"balance\":200.0}";
        }
        else if (req.find("/api/send") != std::string::npos) {
            auto [testPriv, testPub] = CryptoUtils::generateKeyPair();
            std::string testAddr = "LOTUS_" + CryptoUtils::sha256(testPub).substr(0, 16);
            networkNode.setBalance(testAddr, 100.0);

            // Generate Tx with Random Dynamic Gas Fee between 2.0 and 10.0 LOTUS
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

        return "{\"error\":\"Endpoint Not Found\"}";
    }
};

#endif