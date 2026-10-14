#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <cstdint>

struct Transaction {
    std::string senderPublicKey;
    std::string recipient;
    double amount;
    double gasFee;
    std::string signature;

    std::string getPayloadHash() const;
};

class Block {
public:
    uint32_t index;
    uint64_t timestamp;
    std::string previousHash;
    std::string blockHash;
    uint64_t nonce;
    std::vector<Transaction> transactions;

    Block(uint32_t idx, std::string prevHash, std::vector<Transaction> txs);
    std::string calculateHash() const;
};

#endif