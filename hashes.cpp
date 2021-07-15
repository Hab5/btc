#include "hashes.hpp"

std::vector<uint8_t> sha256(const std::vector<uint8_t>& input) {
    SHA256_CTX sctx;
    SHA256_Init(&sctx);
    SHA256_Update(&sctx, input.data(), input.size());
    std::vector<uint8_t> md(SHA256_DIGEST_LENGTH);
    SHA256_Final(md.data(), &sctx);
    return md;
}

std::vector<uint8_t> sha1(const std::vector<uint8_t>& input) {
    SHA_CTX sctx;
    SHA1_Init(&sctx);
    SHA1_Update(&sctx, input.data(), input.size());
    std::vector<uint8_t> md(SHA_DIGEST_LENGTH);
    SHA1_Final(md.data(), &sctx);
    return md;
}

std::vector<uint8_t> ripemd160(const std::vector<uint8_t>& input) {
    RIPEMD160_CTX rctx;
    RIPEMD160_Init(&rctx);
    RIPEMD160_Update(&rctx, input.data(), input.size());
    std::vector<uint8_t> md(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160_Final(md.data(), &rctx);
    return md;
}

std::vector<uint8_t> hash256(const std::vector<uint8_t>& input) {
    return sha256(sha256(input));
}

std::vector<uint8_t> hash160(const std::vector<uint8_t>& input) {
    return ripemd160(sha256(input));
}