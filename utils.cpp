#include "utils.hpp"

std::string toHex(const std::vector<uint8_t>& input) {
    std::stringstream bytes;
    for (auto& b: input)
        bytes << std::hex << std::setfill('0') << std::setw(2) << +b;
    return std::string(bytes.str());
}

std::vector<uint8_t> hex2bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byte_string = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byte_string.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}
