#ifndef CRYPTO_UTILS_HPP
#define CRYPTO_UTILS_HPP

#include <string>
#include <utility>

class CryptoUtils {
public:
    static std::string sha256(const std::string& input);
    static std::string doubleSHA256(const std::string& input);
    static std::pair<std::string, std::string> generateKeyPair();
    static std::string signMessage(const std::string& privateKeyHex, const std::string& message);
    static bool verifySignature(const std::string& publicKeyHex, const std::string& message, const std::string& signatureHex);
    
    // BIP-39 Mnemonic & Wallet File Engine
    static std::string generateMnemonic();
    static bool saveWalletToFile(const std::string& filename, const std::string& address, const std::string& mnemonic, const std::string& privKey, const std::string& pubKey);
};

#endif