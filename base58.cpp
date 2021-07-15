#include "base58.hpp"
#include <stdexcept>

namespace Base58 {
// All alphanumeric characters except for "0", "I", "O", and "l".
static const char* BASE58_ALPHABET = \
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string Encode(std::vector<uint8_t>& input)
{
    uint8_t* begin = input.data();
    uint8_t* end   = input.data() + input.size();
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (begin != end && *begin == 0) {
        begin++;
        zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    int size = (end - begin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<uint8_t> b58(size);
    // Process the bytes.
    while (begin != end) {
        int carry = *begin;
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (std::vector<uint8_t>::reverse_iterator it = b58.rbegin(); 
         (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }
        assert(carry == 0);
        length = i;
        begin++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<uint8_t>::iterator it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += BASE58_ALPHABET[*(it++)];
    return str;
}

std::vector<uint8_t>  Decode(std::string input) {
    std::vector<uint8_t> output;
    const char* psz = input.data();
    // Skip leading spaces.
    while (*psz && isspace(*psz)) 
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    int size = strlen(psz) * 733 / 1000 + 1;  // log(58) / log(256), rounded up.
    std::vector<uint8_t> b256(size);
    // Process the characters.
    while (*psz && !isspace(*psz)) {
        // Decode base58 character
        const char *ch = strchr(BASE58_ALPHABET, *psz);
        if (ch == nullptr)
            throw std::runtime_error("Base58::Decode: invalid input");
        // Apply "b256 = b256 * 58 + ch".
        int carry = ch - BASE58_ALPHABET;
        int i = 0;
        for (std::vector<uint8_t>::reverse_iterator it = b256.rbegin();
         (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        psz++;
    }
    // Skip trailing spaces.
    while (isspace(*psz))
        psz++;
    if (*psz != 0)
        throw std::runtime_error("Base58::Decode: invalid input");
    // Skip leading zeroes in b256.
    std::vector<uint8_t>::iterator it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0) it++;
    // Copy result into output vector.
    output.reserve(zeroes + (b256.end() - it));
    output.assign(zeroes, 0x00);
    while (it != b256.end())
        output.push_back(*(it++));
    return output;
}

}
