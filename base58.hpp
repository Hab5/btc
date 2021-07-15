#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <exception>

namespace Base58 {
    std::string Encode(std::vector<uint8_t>& input);
    std::vector<uint8_t> Decode(std::string input);
}
