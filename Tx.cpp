#include "Tx.hpp"

Tx::Tx(const std::vector<uint8_t>& serialized_tx) {

    auto ser_it = serialized_tx.begin();

    // Version
    if (serialized_tx.size() < 4)
        throw std::runtime_error("Transaction: invalid input size");
    std::memcpy(&version, serialized_tx.data(), 4);
    ser_it += 4;

    // Number of inputs
    CompactSize n_inputs_cs(std::vector<uint8_t>(ser_it, serialized_tx.end()));
    auto n_inputs = n_inputs_cs.GetInt();
    ser_it += n_inputs_cs.GetBytes().size();

    // Inputs
    inputs.reserve(n_inputs);
    for (size_t i = 0; i < n_inputs; i++) {
        TxIn in(std::vector<uint8_t>(ser_it, serialized_tx.end()));
        ser_it += in.__offset;
        inputs.push_back(in);
    }

    // Number of outputs
    CompactSize n_outputs_cs(std::vector<uint8_t>(ser_it, serialized_tx.end()));
    auto n_outputs = n_outputs_cs.GetInt();
    ser_it += n_outputs_cs.GetBytes().size();

    // Outputs
    outputs.reserve(n_outputs);
    for (size_t i = 0; i < n_outputs; i++) {
        TxOut out(std::vector<uint8_t>(ser_it, serialized_tx.end()));
        ser_it += out.__offset;
        outputs.push_back(out);
    }

    // Locktime
    if (std::vector<uint8_t>(ser_it, serialized_tx.end()).size() < 4)
        throw std::runtime_error("Transaction: invalid input size");
    std::memcpy(&locktime, std::vector<uint8_t>(ser_it, serialized_tx.end()).data(), 4);
    ser_it += 4;

    std::cout << "Transaction:" << std::endl;
    std::cout << std::endl;
    std::cout << "Version: " << version << std::endl << std::endl;
    std::cout << "Inputs: " << n_inputs << std::endl;
    for (auto& in: inputs) {
        std::cout << "    txid          : " << toHex(in.txid) << std::endl;
        std::cout << "    txid_idx      : " << in.txid_idx << std::endl;
        std::cout << "    script_length : " << in.unlock_script.GetBytes().size() << std::endl;
        std::cout << "    script bytes  : " << toHex(in.unlock_script.GetBytes()) << std::endl;
        std::cout << "    sequence      : " << toHex(int2bytes_LE(in.sequence, 4)) << std::endl;
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Outputs: " << n_outputs << std::endl;
    for (auto& out: outputs) {
        std::cout << "    satoshis   : " << out.satoshis << std::endl;
        std::cout << "    script_len : " << out.locking_script.GetBytes().size() << std::endl;
        std::cout << "    script     : " << toHex(out.locking_script.GetBytes()) << std::endl;
        std::cout << std::endl;
    }
    std::cout << "Locktime: " << locktime << std::endl;

}

std::vector<uint8_t> Tx::Serialize() {
    std::vector<uint8_t> serialized;

    // Version
    auto version_bytes = int2bytes_LE(version, 4);
    serialized.insert(serialized.end(),
                      std::move_iterator(version_bytes.begin()),
                      std::move_iterator(version_bytes.end()));

    // Inputs
    auto n_inputs = CompactSize(inputs.size()).GetBytes();
    serialized.insert(serialized.end(),
                      std::move_iterator(n_inputs.begin()),
                      std::move_iterator(n_inputs.end()));
    for (auto& in: inputs) {
        std::vector<uint8_t> in_ser = in.Serialize();
        serialized.insert(serialized.end(),
                          std::move_iterator(in_ser.begin()),
                          std::move_iterator(in_ser.end()));
    }

    // Outputs
    auto n_outputs = CompactSize(outputs.size()).GetBytes();
    serialized.insert(serialized.end(),
                      std::move_iterator(n_outputs.begin()),
                      std::move_iterator(n_outputs.end()));
    for (auto& out: outputs) {
        std::vector<uint8_t> out_ser = out.Serialize();
        serialized.insert(serialized.end(),
                          std::move_iterator(out_ser.begin()),
                          std::move_iterator(out_ser.end()));
    }

    // Locktime
    auto locktime_bytes = int2bytes_LE(locktime, 4);
    serialized.insert(serialized.end(),
                      std::move_iterator(locktime_bytes.begin()),
                      std::move_iterator(locktime_bytes.end()));

    return serialized;
}
