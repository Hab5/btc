#pragma once

#include "Script.hpp"
#include "CompactSize.hpp"

class TxIn {
public:
    TxIn() = delete;

    TxIn(const std::vector<uint8_t>& txid,
         const uint32_t&             txid_idx,
         const Script&               unlock_script,
         const uint32_t&             sequence):
         txid(txid),
         txid_idx(txid_idx),
         unlock_script(unlock_script),
         sequence(sequence) {}

    TxIn(const std::vector<uint8_t>& serialized);

    std::vector<uint8_t> Serialize(bool ignore_script=false) const;

    std::vector<uint8_t> GetTxId()  { return txid; }
    uint32_t             GetIndex() { return txid_idx; }
private:
    std::vector<uint8_t> txid;
    uint32_t             txid_idx;
    Script               unlock_script;
    uint32_t             sequence;

    size_t               __offset = 0; // used during tx deserialization, a little ugly but eh

    friend class Tx;
};
