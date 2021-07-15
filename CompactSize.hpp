#pragma once

#include "utils.hpp"

#include <iostream>
#include <vector>
#include <limits>
#include <cstring>

// Format used to serialize the size of some transaction elements
class CompactSize {
public:
    CompactSize(const uint64_t& size);

    CompactSize(const std::vector<uint8_t>& bytes);

    uint64_t             GetInt() const;
    std::vector<uint8_t> GetBytes();

private:
    std::vector<uint8_t> bytes;
    uint64_t             size_value = 0;

    void Encode(const uint64_t& size);
    void Decode(const std::vector<uint8_t>& input_bytes);

    size_t ExtractLength(const std::vector<uint8_t>& serialized_data) const;
};
