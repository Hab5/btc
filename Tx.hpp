#pragma once

#include "Script.hpp"
#include "CompactSize.hpp"
#include "utils.hpp"

#include "TxIn.hpp"
#include "TxOut.hpp"

#include <iostream>
#include <vector>

class Tx {
public:
    int32_t            version;
    std::vector<TxIn>  inputs;
    std::vector<TxOut> outputs;
    uint32_t           locktime;

    Tx() = delete;

    Tx(const int32_t&            version,
                const std::vector<TxIn>&  inputs,
                const std::vector<TxOut>& outputs,
                const uint32_t&           locktime):
                version(version),
                inputs(inputs),
                outputs(outputs),
                locktime(locktime) {}

    Tx(const std::vector<uint8_t>& serialized_tx);

    std::vector<uint8_t> Serialize();
};
