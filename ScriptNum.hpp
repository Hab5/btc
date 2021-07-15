#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>

// https://github.com/bitcoin/bitcoin/blob/v0.10.1/src/script/script.h#L180

static const size_t SCRIPT_NUM_MAX_BYTES  = 4;

class ScriptNum {
public:
    explicit ScriptNum(const int64_t& num);
    explicit ScriptNum(const std::vector<uint8_t>& bytes);

    int64_t              GetInt()   const {return num_value;}
    std::vector<uint8_t> GetBytes() const { return Serialize(num_value); };
    static std::vector<uint8_t> Serialize(const int64_t& value);

    inline bool operator==(const int64_t& rhs) const { return num_value == rhs; }
    inline bool operator!=(const int64_t& rhs) const { return num_value != rhs; }
    inline bool operator<=(const int64_t& rhs) const { return num_value <= rhs; }
    inline bool operator< (const int64_t& rhs) const { return num_value <  rhs; }
    inline bool operator>=(const int64_t& rhs) const { return num_value >= rhs; }
    inline bool operator> (const int64_t& rhs) const { return num_value >  rhs; }

    inline bool operator==(const ScriptNum& rhs) const { return operator==(rhs.num_value); }
    inline bool operator!=(const ScriptNum& rhs) const { return operator!=(rhs.num_value); }
    inline bool operator<=(const ScriptNum& rhs) const { return operator<=(rhs.num_value); }
    inline bool operator< (const ScriptNum& rhs) const { return operator< (rhs.num_value); }
    inline bool operator>=(const ScriptNum& rhs) const { return operator>=(rhs.num_value); }
    inline bool operator> (const ScriptNum& rhs) const { return operator> (rhs.num_value); }

    inline ScriptNum operator+(const int64_t& rhs)   const { return ScriptNum(num_value + rhs); }
    inline ScriptNum operator-(const int64_t& rhs)   const { return ScriptNum(num_value - rhs); }
    inline ScriptNum operator+(const ScriptNum& rhs) const { return operator+(rhs.num_value);   }
    inline ScriptNum operator-(const ScriptNum& rhs) const { return operator-(rhs.num_value);   }

    inline ScriptNum& operator+=(const ScriptNum& rhs) { return operator+=(rhs.num_value); }
    inline ScriptNum& operator-=(const ScriptNum& rhs) { return operator-=(rhs.num_value); }

    inline ScriptNum operator-() const {
        assert(num_value != std::numeric_limits<int64_t>::min());
        return ScriptNum(-num_value);
    }

    inline ScriptNum& operator=(const int64_t& rhs) {
        num_value = rhs;
        return *this;
    }

    inline ScriptNum& operator+=(const int64_t& rhs) {
        assert(rhs == 0 || (rhs > 0 && num_value <= std::numeric_limits<int64_t>::max() - rhs) ||
                           (rhs < 0 && num_value >= std::numeric_limits<int64_t>::min() - rhs));
        num_value += rhs;
        return *this;
    }

    inline ScriptNum& operator-=(const int64_t& rhs) {
        assert(rhs == 0 || (rhs > 0 && num_value >= std::numeric_limits<int64_t>::min() + rhs) ||
                           (rhs < 0 && num_value <= std::numeric_limits<int64_t>::max() + rhs));
        num_value -= rhs;
        return *this;
    }

private:
    int64_t num_value;

    static int64_t FromBytes(const std::vector<uint8_t>& bytes);
};
