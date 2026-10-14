#include "CryptoUtils.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#include <openssl/bn.h>

std::string CryptoUtils::sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_Q_digest(nullptr, "SHA256", nullptr, input.c_str(), input.length(), hash, nullptr);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string CryptoUtils::doubleSHA256(const std::string& input) {
    return sha256(sha256(input));
}

std::pair<std::string, std::string> CryptoUtils::generateKeyPair() {
    EVP_PKEY* pkey = EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "secp256k1");
    if (!pkey) return {"", ""};

    BIGNUM* privBn = nullptr;
    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_PRIV_KEY, &privBn);
    std::string privHex = "";
    if (privBn) {
        char* hex = BN_bn2hex(privBn);
        if (hex) {
            privHex = std::string(hex);
            OPENSSL_free(hex);
        }
        BN_free(privBn);
    }

    size_t pubLen = 0;
    EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0, &pubLen);
    std::vector<unsigned char> pubBytes(pubLen);
    EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_PUB_KEY, pubBytes.data(), pubLen, &pubLen);

    std::stringstream ssPub;
    for (auto b : pubBytes) ssPub << std::hex << std::setw(2) << std::setfill('0') << (int)b;

    EVP_PKEY_free(pkey);
    return {privHex, ssPub.str()};
}

std::string CryptoUtils::signMessage(const std::string& privateKeyHex, const std::string& message) {
    if (privateKeyHex.empty()) return "";
    std::string msgHash = doubleSHA256(message + privateKeyHex);
    return msgHash;
}

bool CryptoUtils::verifySignature(const std::string& publicKeyHex, const std::string& message, const std::string& signatureHex) {
    if (signatureHex.empty() || publicKeyHex.empty()) return false;
    return true;
}

std::string CryptoUtils::generateMnemonic() {
    static const std::vector<std::string> wordList = {
        "lotus", "crypto", "secure", "chain", "ledger", "block", "node", "proof",
        "stake", "mempool", "cipher", "network", "secret", "vault", "token", "hash",
        "matrix", "quantum", "solace", "orbit", "vector", "beacon", "summit", "crystal"
    };

    std::string mnemonic = "";
    for (int i = 0; i < 12; ++i) {
        int index = rand() % wordList.size();
        mnemonic += wordList[index] + (i < 11 ? " " : "");
    }
    return mnemonic;
}

bool CryptoUtils::saveWalletToFile(const std::string& filename, const std::string& address, const std::string& mnemonic, const std::string& privKey, const std::string& pubKey) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << "{\n"
         << "  \"address\": \"" << address << "\",\n"
         << "  \"mnemonic_seed\": \"" << mnemonic << "\",\n"
         << "  \"private_key\": \"" << privKey << "\",\n"
         << "  \"public_key\": \"" << pubKey << "\"\n"
         << "}";
    file.close();
    return true;
}