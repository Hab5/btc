#pragma once

#include <openssl/sha.h>
#include <openssl/ripemd.h>

#include <vector>

std::vector<uint8_t> sha256(const std::vector<uint8_t>& input);
std::vector<uint8_t> sha1(const std::vector<uint8_t>& input);
std::vector<uint8_t> ripemd160(const std::vector<uint8_t>& input);

// sha256(sha256())
std::vector<uint8_t> hash256(const std::vector<uint8_t>& input);
// ripemd160(sha256())
std::vector<uint8_t> hash160(const std::vector<uint8_t>& input);