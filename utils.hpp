#pragma once

#include <iostream>
#include <iomanip>
#include <vector>
#include <stack>

std::string toHex(const std::vector<uint8_t>& input);
std::vector<uint8_t> hex2bytes(const std::string& hex);

template <typename T>
std::vector<uint8_t> int2bytes_LE(const T& input, size_t input_bytes_size) {
    std::vector<uint8_t> bytes(input_bytes_size);
    for (size_t i = 0; i < input_bytes_size; i++)
        bytes[i] = input >> (i * 8);
    return bytes;
}
