#include "CompactSize.hpp"

#include "utils.hpp"

CompactSize::CompactSize(const uint64_t& size)
    : size_value(size) {
    Encode(size);
}

CompactSize::CompactSize(const std::vector<uint8_t>& bytes) {
    Decode(bytes);
}

std::vector<uint8_t> CompactSize::GetBytes() {
    if (bytes.empty())
        Encode(size_value);
    return bytes;
}

uint64_t CompactSize::GetInt() const {
    return size_value;
}

void CompactSize::Encode(const uint64_t& size) {
    if (size_value < 0xfd)
        bytes.push_back((uint8_t)size);
    else if (size_value <= std::numeric_limits<uint16_t>::max()) {
        bytes = int2bytes_LE((uint16_t)size, 2);
        bytes.insert(bytes.begin(), 0xfd);
    }
    else if (size_value <= std::numeric_limits<uint32_t>::max()) {
        bytes = int2bytes_LE((uint32_t)size, 4);
        bytes.insert(bytes.begin(), 0xfe);
    }
    else {
        bytes = int2bytes_LE((uint64_t)size, 8);
        bytes.insert(bytes.begin(), 0xff);
    }
}

void CompactSize::Decode(const std::vector<uint8_t>& input_bytes) {
    size_t length = ExtractLength(input_bytes);
    bool prefix = (length > 1) ? true : false;
    if (input_bytes.size() < length+prefix)
        throw std::runtime_error("Decode: invalid input");
    std::memcpy(&size_value, input_bytes.data()+prefix, length);
    if (!bytes.empty())
        bytes = std::vector<uint8_t>(input_bytes.begin(),
            input_bytes.begin()+length+prefix);
}

size_t CompactSize::ExtractLength(const std::vector<uint8_t>& serialized_data) const {
    uint8_t prefix = serialized_data[0];

    if      (prefix <  0xfd) return 1;
    else if (prefix == 0xfd) return 2;
    else if (prefix == 0xfe) return 4;
    else                     return 8;
}
