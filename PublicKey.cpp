#include "PublicKey.hpp"

PublicKey::PublicKey(const std::vector<uint8_t>& key_, bool compressed, bool testnet)
    : key(key_), compressed(compressed), testnet(testnet) {
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
}

PublicKey::PublicKey(const PublicKey& key_) {
    key        = key_.key;
    ctx        = key_.ctx;
    compressed = key_.compressed;
    testnet    = key_.testnet;
}

PublicKey::~PublicKey() {
    if (ctx) secp256k1_context_destroy(ctx);
}

bool PublicKey::Verify(const std::vector<uint8_t>& hash, 
  const std::vector<uint8_t>& signature) const {
    secp256k1_pubkey pubkey;
    if(!secp256k1_ec_pubkey_parse(ctx, &pubkey, key.data(), key.size()))
        return false;
    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ecdsa_signature_parse_der(ctx, &sig, signature.data(), signature.size()))
        return false;
    secp256k1_ecdsa_signature_normalize(ctx, &sig, &sig);
    return secp256k1_ecdsa_verify(ctx, &sig, hash.data(), &pubkey);
}

std::string PublicKey::Address() const {
    // ripemd160(sha256()) of raw public key.
    auto address = hash160(key);
    // Prepend network prefix (0x00 = mainnet | 0x6f = testnet).
    address.insert(address.begin(), (testnet) ? 0x6f : 0x00);
    // Append checksum (first 4 bytes of sha256(sha256())).
    auto hash = hash256(address);
    auto checksum = std::vector<uint8_t>(hash.begin(), hash.begin()+4);
    address.insert(address.end(), checksum.begin(), checksum.end());
    // Encode as Base58.
    return Base58::Encode(address);
}
