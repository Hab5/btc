#include "TxIn.hpp"

TxIn::TxIn(const std::vector<uint8_t>& serialized) {
    // Transaction ID
    __offset += 32;
    if (serialized.size() < __offset)
        throw std::runtime_error("TxIn: invalid serialization (txid)");
    txid = std::vector<uint8_t>(serialized.begin(), serialized.begin()+__offset);

    // Transaction ID index.
    if (serialized.size() < __offset + 4)
        throw std::runtime_error("TxIn: invalid serialization (txid index)");
    std::memcpy(&txid_idx, serialized.data()+__offset, 4);
    __offset += 4;

    // ScriptSig Length
    CompactSize script_length(std::vector<uint8_t>(serialized.begin()+__offset,
                                                   serialized.end()));
    __offset += script_length.GetBytes().size();

    // ScriptSig
    if (serialized.size() < __offset + script_length.GetInt())
        throw std::runtime_error("TxIn: invalid serialization (script size)");
    unlock_script = Script(std::vector<uint8_t>(serialized.begin()+__offset,
                                             serialized.begin()+__offset+
                                             script_length.GetInt()));
    __offset += script_length.GetInt();

    // Sequence
    if (serialized.size() < __offset + 4)
        throw std::runtime_error("TxIn: invalid serialization (sequence)");
    std::memcpy(&sequence, serialized.data()+__offset, 4);
    __offset += 4;
}

std::vector<uint8_t> TxIn::Serialize(bool ignore_script) const {
    std::vector<uint8_t> serialized;
    std::vector<uint8_t> script_ser;

    auto txid_idx_bytes = int2bytes_LE(txid_idx, 4);
    if (!ignore_script) script_ser = std::vector<uint8_t>(unlock_script.GetBytes());
    auto sz_bytes = CompactSize(script_ser.size()).GetBytes();
    auto seq_bytes = int2bytes_LE(sequence, 4);

    serialized.reserve(txid.size() +
                       txid_idx_bytes.size() +
                       sz_bytes.size() +
                       script_ser.size() +
                       seq_bytes.size());

    serialized.insert(serialized.end(),
                      txid.begin(),
                      txid.end());
    serialized.insert(serialized.end(),
                      std::move_iterator(txid_idx_bytes.begin()),
                      std::move_iterator(txid_idx_bytes.end()));
    serialized.insert(serialized.end(),
                      std::move_iterator(sz_bytes.begin()),
                      std::move_iterator(sz_bytes.end()));
    serialized.insert(serialized.end(),
                      std::move_iterator(script_ser.begin()),
                      std::move_iterator(script_ser.end()));
    serialized.insert(serialized.end(),
                      std::move_iterator(seq_bytes.begin()),
                      std::move_iterator(seq_bytes.end()));

    return serialized;
}
