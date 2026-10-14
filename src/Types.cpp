#include "Types.hpp"
#include "CryptoUtils.hpp"
#include <ctime>

std::string Transaction::getPayloadHash() const {
    return CryptoUtils::doubleSHA256(senderPublicKey + recipient + std::to_string(amount) + std::to_string(gasFee));
}

Block::Block(uint32_t idx, std::string prevHash, std::vector<Transaction> txs)
    : index(idx), previousHash(prevHash), transactions(txs), nonce(0) {
    timestamp = std::time(nullptr);
    blockHash = calculateHash();
}

std::string Block::calculateHash() const {
    std::string txData = "";
    for (const auto& tx : transactions) {
        txData += tx.getPayloadHash();
    }
    std::string header = std::to_string(index) + std::to_string(timestamp) + previousHash + txData + std::to_string(nonce);
    return CryptoUtils::doubleSHA256(header);
}