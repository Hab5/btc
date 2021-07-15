#pragma once

#include "Script.hpp"
#include "CompactSize.hpp"

class TxOut {
public:
    TxOut() = delete;

    TxOut(int64_t satoshis,
          Script  script_pubkey):
          satoshis(satoshis),
          locking_script(script_pubkey) {}

    TxOut(const std::vector<uint8_t>& serialized);

    std::vector<uint8_t> Serialize() const;

    Script  GetScript()   { return locking_script; }
    int64_t GetSatoshis() { return satoshis; }

private:
    int64_t  satoshis; // Amount in satoshis
    Script   locking_script;
    size_t   __offset = 0;

    friend class Tx;
};
