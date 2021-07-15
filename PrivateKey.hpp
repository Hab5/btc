#pragma once

#include "PublicKey.hpp"
#include "utils.hpp"

#include <bits/stdint-uintn.h>
#include <secp256k1.h>

#include <vector>
#include <iomanip>
#include <climits>
#include <algorithm>
#include <functional>
#include <random>

static const size_t PRIVATE_KEY_SIZE        = 32;
static const size_t COMPRESSED_PKEY_SIZE    = 33;
static const size_t UNCOMPRESSED_PKEY_SIZE  = 65;
static const size_t SIGNATURE_SIZE          = 72;

enum class KEY_FLAGS : unsigned {
    NONE,
    COMPRESS_PUBLIC_KEY,
    TESTNET
};

template<class T> inline T  operator~  (T a) { return (T)~(int)a; }
template<class T> inline T  operator|  (T a, T b) { return (T)((int)a | (int)b); }
template<class T> inline T  operator&  (T a, T b) { return (T)((int)a & (int)b); }
template<class T> inline T  operator^  (T a, T b) { return (T)((int)a ^ (int)b); }
template<class T> inline T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
template<class T> inline T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
template<class T> inline T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

class PrivateKey {
public:
    PrivateKey(KEY_FLAGS flags = KEY_FLAGS::NONE);
    PrivateKey(const std::string& wallet_import_format);
    ~PrivateKey();

    PublicKey            GenPublicKey()                         const;
    std::string          WalletImportFormat()                   const;
    std::vector<uint8_t> Sign(const std::vector<uint8_t>& hash) const;

    std::vector<uint8_t> GetBytes() const { return key; }

    friend std::ostream& operator<< (std::ostream& out, const PrivateKey& data) {
        out << toHex(data.key);
        return out;
    }

private:
    secp256k1_context*   ctx;
    std::vector<uint8_t> key;
    bool                 comp_pub_key;
    bool                 testnet;
};

