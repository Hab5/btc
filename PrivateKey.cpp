#include "PrivateKey.hpp"

PrivateKey::PrivateKey(KEY_FLAGS flags) 
    : comp_pub_key((flags & KEY_FLAGS::COMPRESS_PUBLIC_KEY) 
                         == KEY_FLAGS::COMPRESS_PUBLIC_KEY),
      testnet((flags & KEY_FLAGS::TESTNET) == KEY_FLAGS::TESTNET) {
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    key = std::vector<uint8_t>(PRIVATE_KEY_SIZE);
    std::independent_bits_engine<std::random_device, CHAR_BIT, uint8_t> rbe;
    do std::generate(begin(key), end(key), std::ref(rbe));
    while (!secp256k1_ec_seckey_verify(ctx, key.data()));
}

PrivateKey::PrivateKey(const std::string& wallet_import_format) {
    // Decode Base58.
    key = Base58::Decode(wallet_import_format);
    // Check checksum.
    std::vector<uint8_t> checksum(key.end()-4, key.end());
    key.resize(key.size()-4);
    auto hash = hash256(key);
    if (std::vector<uint8_t>(hash.begin(), hash.begin()+4) != checksum)
        throw std::runtime_error("Invalid checksum");
    // Check prefix/suffix.
    if (key.front() != 0xef && key.front() != 0x80)
        throw std::runtime_error("Invalid network prefix");
    if (key.back() != 0x01 && key.back() != 0x00)
        throw std::runtime_error("Invalid compression suffix");
    // Get and strip prefix.
    testnet = (key.front() == 0xef) ? true : false;
    comp_pub_key = (key.back() == 0x01) ? true : false;
    key.erase(key.begin());
    key.pop_back();
    
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
}

PrivateKey::~PrivateKey() {
    if (ctx) secp256k1_context_destroy(ctx);
    key.clear();
}

PublicKey PrivateKey::GenPublicKey() const {
    secp256k1_pubkey pub_key;
    if (!secp256k1_ec_pubkey_create(ctx, &pub_key, key.data())) 
        throw std::runtime_error("Failed to create public key");
    std::vector<uint8_t> pub_key_bytes(comp_pub_key ? 
        COMPRESSED_PKEY_SIZE : UNCOMPRESSED_PKEY_SIZE);
    size_t len = pub_key_bytes.size();
    secp256k1_ec_pubkey_serialize(ctx, pub_key_bytes.data(), &len, &pub_key, 
        comp_pub_key ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    pub_key_bytes.resize(len);
    return PublicKey(pub_key_bytes, comp_pub_key, testnet);
}

std::vector<uint8_t> PrivateKey::Sign(const std::vector<uint8_t>& hash) const {
    std::vector<uint8_t> signature(SIGNATURE_SIZE);
    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ecdsa_sign(ctx, &sig, hash.data(), key.data(), 
      secp256k1_nonce_function_rfc6979, nullptr))
        throw std::runtime_error("Failed to sign");
    size_t sig_size = signature.size();
    if(!secp256k1_ecdsa_signature_serialize_der(ctx, 
      (uint8_t*)signature.data(), &sig_size, &sig))
        throw std::runtime_error("Failed to serialize signature");
    signature.resize(sig_size);
    return signature;
}

std::string PrivateKey::WalletImportFormat() const {
    auto wif = key;
    // Network Prefix
    wif.insert(wif.begin(), testnet ? 0xef : 0x80);
    // Public Key compression suffix
    if (comp_pub_key)
        wif.push_back(0x01);
    // Checksum & Base58
    auto hash = hash256(wif);
    auto checksum = std::vector<uint8_t>(hash.begin(), hash.begin()+4);
    wif.insert(wif.end(), checksum.begin(), checksum.end());
    return Base58::Encode(wif);
}
