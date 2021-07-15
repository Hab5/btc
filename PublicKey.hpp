#pragma once

#include "hashes.hpp"
#include "base58.hpp"
#include "utils.hpp"

#include <secp256k1.h>

#include <iostream>
#include <vector>


class PublicKey {
public:
    PublicKey(const std::vector<uint8_t>& key, bool compressed, bool testnet);
    PublicKey(const PublicKey& key);
    ~PublicKey();
    
    bool Verify(const std::vector<uint8_t>& hash, 
        const std::vector<uint8_t>& signature) const;

    std::string          Address()    const;
    std::vector<uint8_t> GetBytes()   const { return key ;};
    
    friend std::ostream& operator<< (std::ostream& out, const PublicKey& data) {
        out << toHex(data.key);
        return out;
    }

private:
    secp256k1_context*   ctx;
    std::vector<uint8_t> key;
    bool                 compressed;
    bool                 testnet;

};
