#include "ScriptNum.hpp"

ScriptNum::ScriptNum(const int64_t& num) {
    num_value = num;
}

ScriptNum::ScriptNum(const std::vector<uint8_t>& bytes) {
    if (bytes.size() > SCRIPT_NUM_MAX_BYTES)
        throw std::invalid_argument("ScriptNum(): number > 4 bytes (overflow)");
    
    if (bytes.size() > 0)
        if ((bytes.back() & 0x7f) == 0)
            if (bytes.size() <= 1 || (bytes[bytes.size() - 2] & 0x80) == 0)
                throw std::invalid_argument("non-minimally encoded script number");

    num_value = FromBytes(bytes);
}

std::vector<uint8_t> ScriptNum::Serialize(const int64_t& value) {
    if (value == 0)
        return std::vector<uint8_t>();
    std::vector<uint8_t> result;
    const bool neg = value < 0;
    uint64_t abs_value = neg ? -value : value;
    while (abs_value) {
        result.push_back(abs_value & 0xff);
        abs_value >>= 8;
    }
    if (result.back() & 0x80)
        result.push_back(neg ? 0x80 : 0x00);
    else if (neg)
        result.back() |= 0x80;
    return result;
}

int64_t ScriptNum::FromBytes(const std::vector<uint8_t>& bytes) {
    if (bytes.empty())
        return 0;
    int64_t result = 0;
    for (size_t i = 0; i != bytes.size(); i++)
        result |= static_cast<int64_t>(bytes[i]) << 8*i;
    if (bytes.back() & 0x80)
        return -((int64_t)(result & ~(0x80ULL << (8 * (bytes.size() - 1)))));
    return result;
}
