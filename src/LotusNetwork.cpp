#include "LotusNetwork.hpp"
#include "CryptoUtils.hpp"
#include <iostream>
#include <algorithm>

LotusNetwork::LotusNetwork(int p2pPort, int peerPort, const std::string& dbPath)
    : treasuryPool(0.0), minStakeRequired(10.0), difficulty(4), dbEngine(dbPath), p2pListener(p2pPort), targetPeerPort(peerPort) {

    Transaction genesisTx{"GENESIS_PUBKEY", "LOTUS_SYSTEM_TREASURY", 21000000.0, 0.0, "GENESIS_SIG"};
    Block genesisBlock(0, "0000000000000000000000000000000000000000000000000000000000000000", {genesisTx});
    chain.push_back(genesisBlock);
    balances["LOTUS_SYSTEM_TREASURY"] = 21000000.0;
    stakes["LOTUS_SYSTEM_TREASURY"] = 21000000.0;

    dbEngine.put("block_0", genesisBlock.blockHash);
    dbEngine.put("bal_LOTUS_SYSTEM_TREASURY", "21000000.0");

    p2pListener.start([this](const std::string& packet) {
        std::cout << "\n[P2P PACKET RECEIVED] " << packet << std::endl;
        this->handleIncomingP2PPacket(packet);
    });
}

void LotusNetwork::adjustDifficulty() {
    if (chain.size() < 5) return;

    const auto& lastBlock = chain.back();
    const auto& prevAdjustmentBlock = chain[chain.size() - 5];
    uint64_t timeExpected = 5 * 10; // 10 seconds per block ideal target time
    uint64_t timeTaken = lastBlock.timestamp - prevAdjustmentBlock.timestamp;

    if (timeTaken < timeExpected / 2) {
        difficulty++;
        std::cout << ">>> ⚡ [DIFFICULTY ADJUSTMENT] Blocks mined too fast! Increased Difficulty Prefix to: " << difficulty << std::endl;
    } else if (timeTaken > timeExpected * 2 && difficulty > 2) {
        difficulty--;
        std::cout << ">>> 🐢 [DIFFICULTY ADJUSTMENT] Blocks mined too slow! Decreased Difficulty Prefix to: " << difficulty << std::endl;
    }
}

void LotusNetwork::slashMaliciousActorInternal(const std::string& validatorAddress, double penaltyAmount, const std::string& reason) {
    if (stakes[validatorAddress] >= penaltyAmount) {
        stakes[validatorAddress] -= penaltyAmount;
    } else {
        stakes[validatorAddress] = 0.0;
    }
    slashedValidators.insert(validatorAddress);
    treasuryPool += penaltyAmount;

    dbEngine.put("stake_" + validatorAddress, std::to_string(stakes[validatorAddress]));
    dbEngine.put("treasury_pool", std::to_string(treasuryPool));

    std::cout << "\n🚨 [SLASHING ENGINE ACTIVATED] Node: " << validatorAddress 
              << " | Penalized: " << penaltyAmount << " LOTUS | Reason: " << reason 
              << " | Confiscated to Treasury!" << std::endl;
}

bool LotusNetwork::stakeTokens(const std::string& validator, double amount) {
    std::lock_guard<std::mutex> lock(mtx);
    if (balances[validator] < amount || amount <= 0.0) return false;

    balances[validator] -= amount;
    stakes[validator] += amount;

    dbEngine.put("bal_" + validator, std::to_string(balances[validator]));
    dbEngine.put("stake_" + validator, std::to_string(stakes[validator]));
    return true;
}

bool LotusNetwork::unstakeTokens(const std::string& validator, double amount) {
    std::lock_guard<std::mutex> lock(mtx);
    if (stakes[validator] < amount || amount <= 0.0) return false;

    stakes[validator] -= amount;
    balances[validator] += amount;

    dbEngine.put("bal_" + validator, std::to_string(balances[validator]));
    dbEngine.put("stake_" + validator, std::to_string(stakes[validator]));
    return true;
}

double LotusNetwork::getStake(const std::string& validator) {
    std::lock_guard<std::mutex> lock(mtx);
    return stakes[validator];
}

bool LotusNetwork::isSlashed(const std::string& validator) {
    std::lock_guard<std::mutex> lock(mtx);
    return slashedValidators.count(validator) > 0;
}

void LotusNetwork::slashMaliciousActor(const std::string& validatorAddress, double penaltyAmount, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mtx);
    slashMaliciousActorInternal(validatorAddress, penaltyAmount, reason);
}

bool LotusNetwork::isChainValid(const std::vector<Block>& targetChain) {
    if (targetChain.empty() || targetChain[0].index != 0) return false;

    for (size_t i = 1; i < targetChain.size(); ++i) {
        const auto& current = targetChain[i];
        const auto& previous = targetChain[i - 1];

        if (current.previousHash != previous.blockHash) return false;
        if (current.calculateHash() != current.blockHash) return false;
    }
    return true;
}

bool LotusNetwork::replaceChain(const std::vector<Block>& incomingChain) {
    std::lock_guard<std::mutex> lock(mtx);

    if (incomingChain.size() > chain.size() && isChainValid(incomingChain)) {
        chain = incomingChain;

        for (const auto& b : chain) {
            dbEngine.put("block_" + std::to_string(b.index), b.blockHash);
        }
        dbEngine.put("latest_block_hash", chain.back().blockHash);

        std::cout << ">>> 🔀 [FORK RESOLUTION SUCCESS] Local chain replaced with longer valid chain! Height: " 
                  << chain.size() << std::endl;
        return true;
    }
    return false;
}

void LotusNetwork::handleIncomingP2PPacket(const std::string& packet) {
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

bool LotusNetwork::submitToMempool(Transaction tx, bool broadcast) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string senderAddr = "LOTUS_" + CryptoUtils::sha256(tx.senderPublicKey).substr(0, 16);

    if (slashedValidators.count(senderAddr)) {
        std::cout << "[REJECTED] Slashed malicious actor attempted transaction!" << std::endl;
        return false;
    }

    if (tx.amount <= 0.0 || balances[senderAddr] < (tx.amount + tx.gasFee)) {
        if (stakes.count(senderAddr) && stakes[senderAddr] > 0.0) {
            slashMaliciousActorInternal(senderAddr, 15.0, "Attempted invalid/insufficient balance transaction");
        }
        return false;
    }

    bool isSigValid = CryptoUtils::verifySignature(tx.senderPublicKey, tx.getPayloadHash(), tx.signature);
    if (!isSigValid) {
        if (stakes.count(senderAddr) && stakes[senderAddr] > 0.0) {
            slashMaliciousActorInternal(senderAddr, 25.0, "Forged signature detected on transaction");
        }
        return false;
    }

    mempool.push_back(tx);

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

std::string LotusNetwork::mineBlock(const std::string& minerAddress, bool broadcast) {
    std::lock_guard<std::mutex> lock(mtx);

    if (slashedValidators.count(minerAddress)) {
        std::cout << "[MINING REJECTED] Node " << minerAddress << " is slashed!" << std::endl;
        return "REJECTED_SLASHED";
    }

    if (stakes[minerAddress] < minStakeRequired) {
        std::cout << "[MINING REJECTED] Node " << minerAddress << " has insufficient stake!" << std::endl;
        return "REJECTED_LOW_STAKE";
    }

    std::vector<Transaction> blockTxs;
    double totalBlockFees = 0.0;

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

    mempool.erase(mempool.begin(), mempool.begin() + txsToPick);

    double totalMinerReward = 50.0 + totalBlockFees;
    Transaction rewardTx{"NETWORK_REWARD", minerAddress, totalMinerReward, 0.0, "REWARD_SIG"};
    blockTxs.push_back(rewardTx);

    Block newBlock(chain.size(), chain.back().blockHash, blockTxs);

    // Dynamic PoW Target Prefix
    std::string targetPrefix(difficulty, '0');
    while (newBlock.calculateHash().substr(0, difficulty) != targetPrefix) {
        newBlock.nonce++;
    }
    newBlock.blockHash = newBlock.calculateHash();
    chain.push_back(newBlock);

    balances[minerAddress] += totalMinerReward;
    treasuryPool += totalBlockFees;

    dbEngine.put("block_" + std::to_string(newBlock.index), newBlock.blockHash);
    dbEngine.put("bal_" + minerAddress, std::to_string(balances[minerAddress]));
    dbEngine.put("treasury_pool", std::to_string(treasuryPool));

    // Periodically Retarget Dynamic Difficulty
    adjustDifficulty();

    if (broadcast) {
        std::string pkt = "NEW_BLOCK:INDEX=" + std::to_string(newBlock.index) + ":HASH=" + newBlock.blockHash;
        P2PServer::sendToPeer(targetPeerPort, pkt);
    }

    return newBlock.blockHash;
}

uint32_t LotusNetwork::getDifficulty() { std::lock_guard<std::mutex> lock(mtx); return difficulty; }
size_t LotusNetwork::getMempoolSize() { std::lock_guard<std::mutex> lock(mtx); return mempool.size(); }
std::string LotusNetwork::fetchKeyFromDB(const std::string& key) { return dbEngine.get(key); }
void LotusNetwork::setBalance(const std::string& address, double amount) { 
    std::lock_guard<std::mutex> lock(mtx);
    balances[address] = amount; 
    dbEngine.put("bal_" + address, std::to_string(amount));
}
double LotusNetwork::getBalance(const std::string& address) { std::lock_guard<std::mutex> lock(mtx); return balances[address]; }
double LotusNetwork::getTreasuryPool() { std::lock_guard<std::mutex> lock(mtx); return treasuryPool; }
size_t LotusNetwork::getChainLength() { std::lock_guard<std::mutex> lock(mtx); return chain.size(); }
std::string LotusNetwork::getLatestBlockHash() { std::lock_guard<std::mutex> lock(mtx); return chain.back().blockHash; }
const std::vector<Block>& LotusNetwork::getChainVector() { std::lock_guard<std::mutex> lock(mtx); return chain; }

std::string LotusNetwork::getChainJSON() {
    std::lock_guard<std::mutex> lock(mtx);
    std::string json = "[";
    for (size_t i = 0; i < chain.size(); ++i) {
        const auto& b = chain[i];
        json += "{\"index\":" + std::to_string(b.index) +
                ",\"hash\":\"" + b.blockHash + "\"" +
                ",\"prev_hash\":\"" + b.previousHash + "\"" +
                ",\"nonce\":" + std::to_string(b.nonce) +
                ",\"timestamp\":" + std::to_string(b.timestamp) +
                ",\"tx_count\":" + std::to_string(b.transactions.size()) + "}";
        if (i + 1 < chain.size()) json += ",";
    }
    json += "]";
    return json;
}