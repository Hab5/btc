#include "OpEnum.hpp"
#include "PrivateKey.hpp"
#include "PublicKey.hpp"
#include "Script.hpp"
#include "Tx.hpp"
#include "base58.hpp"
#include "utils.hpp"

#include <bits/stdint-uintn.h>
#include <secp256k1.h>
#include "json.hpp" // nlohmann

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <bitset>
#include <curl/curl.h>

using json = nlohmann::json;
std::string GetRequest(const std::string &url);

int main() {
    PrivateKey private_key("cVaBTStH5t5JEStnnJ9rgXUtVBKDq23uBMvTTi7TiswwA4B7xuJR");
    PublicKey  public_key = private_key.GenPublicKey();

    std::cout << "Private Key        : " << private_key << std::endl;
    std::cout << "Private Key WIF    : " << private_key.WalletImportFormat() << std::endl;
    std::cout << "Public Key         : " << public_key << std::endl;
    std::cout << "Public Key Address : " << public_key.Address() << std::endl;

    std::vector<uint8_t> hash      = hash256(hex2bytes("the truth is out there"));
    std::vector<uint8_t> signature = private_key.Sign(hash);

    std::cout << "Hash               : " << toHex(hash) << std::endl;
    std::cout << "Signature          : " << toHex(signature) << std::endl;

    std::cout << std::endl;

    std::cout << (public_key.Verify(hash, signature) ?
        "Succesfully verified signature!" :
        "Failed to verify signature!")
    << std::endl;

    std::cout << std::endl;

    Script script;

    script  << signature
            << public_key.GetBytes()
            << OP_DUP
            << OP_HASH160
            << hash160(public_key.GetBytes())
            << OP_EQUALVERIFY;

    std::cout << "Raw Script Bytes:" << std::endl;
    std::cout << script << std::endl << std::endl;

    std::cout << "Script Flow:" << std::endl;
    auto result = script.Run();

    // std::cout << std::endl << "Script Output: " << std::endl;
    // std::cout << toHex(result) << std::endl;

    // Tx tx(hex2bytes(
    //     "0200000002b9a8333d8a3b87c4ecd838c6a711018c"
    //     "d08e60aa13d642fc8361ef1856ed25bd010000006a"
    //     "47304402205236848c5ea3f9311bcc3837cfebe409"
    //     "87e2090e0e63f567bd7057aa4a6a05d702200260f8"
    //     "4e946bb0a60756452a586dc3cfcf1787419ad4c41d"
    //     "72dbf1c0389337e3012102eea850e7059a991e8605"
    //     "20b9a3dd1ba4f8a77e9756b16e54da149245104890"
    //     "55fdffffffc333a3881a28f68583a26b418b919d92"
    //     "909677fd58cd907f3c5cc0953e197dd9000000006a"
    //     "47304402203149b1bc5261cc756a0c95058fa42ebe"
    //     "c3ec074417e1d438eba4e5612adba54f02205b7e6e"
    //     "7245d975b19915a060415e30b11e362a45a5748898"
    //     "170893e24e949c8c012102eea850e7059a991e8605"
    //     "20b9a3dd1ba4f8a77e9756b16e54da149245104890"
    //     "55fdffffff0210270000000000001976a914690958"
    //     "e252b3e8d791e8318b8baa78d21e26be0688ac209a"
    //     "1d00000000001976a914b01cd846a4c2751bd89b3c"
    //     "752c77680d7fe05d1188acc2141e00")
    // );

    // Sender address
    auto address = "mq6LSejCxiQPXHhGqdmaT8xrGXuC7LyzzK";

    // Amount (Satoshis)
    auto amount  = 100000;

    // Get sender UTXOs
    std::string utxo_url = "https://blockstream.info/testnet/api/address/";
    auto res_utxo = GetRequest(utxo_url + address + "/utxo");
    nlohmann::json UTXOs;
    try { UTXOs = json::parse(res_utxo); }
    catch (const std::exception& e) { throw std::runtime_error(res_utxo); }

    // CHECK/CHOOSE BEST UTXOs TO USE HERE

    std::string txid     = UTXOs[0]["txid"];
    size_t      txid_idx = UTXOs[0]["vout"];
    uint64_t    value    = UTXOs[0]["value"];

    // TxIn()

    // Get utxo Tx
    std::string tx_url = "https://blockstream.info/testnet/api/tx/";
    auto res_tx = GetRequest(tx_url + txid + "/hex");
    if (res_tx == "Invalid hex string")
        throw std::runtime_error("Failed to fecth transaction");

    Tx tmp(hex2bytes(res_tx));
    Script locking_script = tmp.outputs[txid_idx].GetScript();
    auto in_amount = tmp.outputs[txid_idx].GetSatoshis();

    // Recipient hash160
    auto recipient_address_bytes = Base58::Decode(address);
    recipient_address_bytes.erase(recipient_address_bytes.begin()); // strip network prefix
    recipient_address_bytes.erase(recipient_address_bytes.end()-4,
                                  recipient_address_bytes.end());   // strip checksum

    // Locking Script
    Script p2pkh;
    p2pkh << OP_DUP
          << OP_HASH160
          << recipient_address_bytes
          << OP_EQUALVERIFY
          << OP_CHECKSIG;

    TxOut change(in_amount - amount - 1000, p2pkh);
    TxOut dest(amount, p2pkh);

    return 0;
}

std::string GetRequest(const std::string& url) {
    CURL*       curl;
    CURLcode    curl_result;
    std::string curl_output;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        // Write callback, curl output -> string
        auto curl_write_cb = [](void* contents, size_t size, size_t nmemb, void* output) -> size_t {
            ((std::string*)output)->append((char*)contents, size*nmemb);
            return size*nmemb;
        };
        // Curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_output);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +curl_write_cb);
        // Get results
        curl_result = curl_easy_perform(curl);
        if (curl_result != CURLE_OK) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error("GetRequest: failed to fetch json");
        }
        curl_easy_cleanup(curl);
    } else throw std::runtime_error("GetRequest: failed to initialize curl");
    curl_global_cleanup();
    return curl_output;
}
