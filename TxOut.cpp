#include "TxOut.hpp"

TxOut::TxOut(const std::vector<uint8_t>& serialized) {
    // Amount
    __offset += 8;
    if (serialized.size() < __offset)
        throw std::runtime_error("TxOut: invalid serialization (amount)");
    std::memcpy(&satoshis, serialized.data(), 8);

    // Script Length
    CompactSize script_length(std::vector<uint8_t>(serialized.begin()+__offset,
                                                   serialized.end()));
    __offset += script_length.GetBytes().size();

    // PubkeyScript
    if (serialized.size() < __offset + script_length.GetInt())
        throw std::runtime_error("TxOut: invalid serialization (script size)");
    locking_script = Script(std::vector<uint8_t>(serialized.begin()+__offset,
                                                serialized.begin()+__offset+
                                                script_length.GetInt()));
    __offset += script_length.GetInt();
}

std::vector<uint8_t> TxOut::Serialize() const {
    std::vector<uint8_t> serialized;

    auto satoshis_bytes = int2bytes_LE(satoshis, 8);
    auto script_ser = locking_script.GetBytes();
    auto sz_bytes = CompactSize(script_ser.size()).GetBytes();

    serialized.reserve(satoshis_bytes.size() +
                       script_ser.size() +
                       sz_bytes.size());

    serialized.insert(serialized.end(),
                      std::move_iterator(satoshis_bytes.begin()),
                      std::move_iterator(satoshis_bytes.end()));
    serialized.insert(serialized.end(),
                      std::move_iterator(sz_bytes.begin()),
                      std::move_iterator(sz_bytes.end()));
    serialized.insert(serialized.end(),
                      std::move_iterator(script_ser.begin()),
                      std::move_iterator(script_ser.end()));

    return serialized;
}
