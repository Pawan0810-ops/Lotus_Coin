#ifndef LOTUS_NETWORK_HPP
#define LOTUS_NETWORK_HPP

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>
#include "Types.hpp"
#include "Storage.hpp"
#include "P2PServer.hpp"

class LotusNetwork {
private:
    std::vector<Block> chain;
    std::unordered_map<std::string, double> balances;
    std::unordered_map<std::string, double> stakes;
    std::unordered_set<std::string> slashedValidators;
    std::vector<Transaction> mempool;
    double treasuryPool;
    double minStakeRequired;
    uint32_t difficulty; // Dynamic Difficulty Target (Prefix count of '0's)
    LevelDBStorage dbEngine;
    P2PServer p2pListener;
    int targetPeerPort;
    std::mutex mtx;

    void slashMaliciousActorInternal(const std::string& validatorAddress, double penaltyAmount, const std::string& reason);
    void adjustDifficulty();

public:
    LotusNetwork(int p2pPort, int peerPort, const std::string& dbPath);
    void handleIncomingP2PPacket(const std::string& packet);
    bool submitToMempool(Transaction tx, bool broadcast = true);
    std::string mineBlock(const std::string& minerAddress, bool broadcast = true);

    // Staking & Slashing Engine Methods
    bool stakeTokens(const std::string& validator, double amount);
    bool unstakeTokens(const std::string& validator, double amount);
    double getStake(const std::string& validator);
    bool isSlashed(const std::string& validator);
    void slashMaliciousActor(const std::string& validatorAddress, double penaltyAmount, const std::string& reason);

    // Fork Resolution & Difficulty Methods
    bool isChainValid(const std::vector<Block>& targetChain);
    bool replaceChain(const std::vector<Block>& incomingChain);
    uint32_t getDifficulty();

    size_t getMempoolSize();
    std::string fetchKeyFromDB(const std::string& key);
    void setBalance(const std::string& address, double amount);
    double getBalance(const std::string& address);
    double getTreasuryPool();
    size_t getChainLength();
    std::string getLatestBlockHash();
    std::string getChainJSON();
    const std::vector<Block>& getChainVector();
};

#endif