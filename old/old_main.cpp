// #include <iostream>
// #include <memory>

// #include <openssl/crypto.h>
// #include <openssl/ecdsa.h>
// #include <openssl/ec.h>
// #include <openssl/evp.h>
// #include <openssl/bio.h>
// #include <openssl/obj_mac.h>
// #include <openssl/err.h>
// #include <openssl/pem.h>

// template<typename T, typename D>
// std::unique_ptr<T, D> make_handle(T* handle, D deleter) {
//     return std::unique_ptr<T, D>{handle, deleter};
// }

// bool openssl_error(const std::string& function) {
//     char buffer[1024];
//     ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
//     std::cerr << "openssl error: " << function
//               << " failed with " << buffer << std::endl;
//     return false;
// }

// bool generate_private_key() {
    
//     // Create context for the key generation
//     auto kctx = make_handle(EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr), EVP_PKEY_CTX_free);
//     if (!kctx) return openssl_error("EVP_PKEY_CTX_new()");
    
//     // Generate the key using the secp256k1 curve
//     if (EVP_PKEY_keygen_init(kctx.get()) != 1) 
//         return openssl_error("EVP_PKEY_keygen_init()");
//     if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(kctx.get(), NID_secp256k1) != 1)
//         return openssl_error("EVP_PKEY_CTX_set_ec_paramgen_curve_nid()");
//     EVP_PKEY* pkey_ = nullptr;
//     if (EVP_PKEY_keygen(kctx.get(), &pkey_) != 1)
//         return openssl_error("EVP_PKEY_keygen()");
    
//     // Write out to file
//     auto pkey = make_handle(pkey_, EVP_PKEY_free);
    
//     // Secret key
//     auto sk_file = make_handle(BIO_new_file("private_key.pem", "w"), BIO_free);
//     if (!sk_file) return openssl_error("BIO_new_file()");
//     if (!PEM_write_bio_PrivateKey(sk_file.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr))
//         return openssl_error("PEM_write_bio_PrivateKey()");

//     // Public key
//     auto pk_file = make_handle(BIO_new_file("public_key.pem", "w"), BIO_free);
//     if (!pk_file) return openssl_error("BIO_new_file()");
//     if (!PEM_write_bio_PUBKEY(pk_file.get(), pkey.get()))
//         return openssl_error("PEM_write_bio_PUBKEY()");
    
//     return true;
// }

// int main() {
    // if (!generate_private_key())
        // std::cerr << "Failed to generate private key\n";
//     return 0;
// }

#include <iostream>
#include <iomanip>

#include "cryptopp/eccrypto.h"
#include "cryptopp/osrng.h"
#include "cryptopp/oids.h"
#include "cryptopp/files.h"
#include "cryptopp/hex.h"
#include "cryptopp/base64.h"

using namespace CryptoPP;

// void save_key_as_hex(const std::string& filename, const PrivateKey& key)
// {
//     ByteQueue queue;
//     key.Save(queue);

//     SaveHex(filename, queue);
// }

std::string PublicKeyToHex(const PublicKey& key)
{   
    ByteQueue queue;
    key.Save(queue);
    HexEncoder encoder(NULL, false);
    queue.CopyTo(encoder);
    encoder.MessageEnd();
    std::string compressed_sec;
    StringSink ss(compressed_sec);
    encoder.TransferTo(ss);
    return compressed_sec;
}

std::string PrivateKeyToHex(const PrivateKey& key)
{   
    ByteQueue queue;
    key.Save(queue);
    HexEncoder encoder(NULL, false);
    queue.CopyTo(encoder);
    encoder.MessageEnd();
    std::string compressed_sec;
    StringSink ss(compressed_sec);
    encoder.TransferTo(ss);
    return compressed_sec;
}

namespace bitcoin {

class KeyPair {
public:
    KeyPair() {}
    
    void GenerateRandom(bool compress_public_key=true);
    // GenerateFromSeed(seed) {} // Seeded generation
    // LoadFromString(string) {} // load from string
    // LoadFromFile(file) {} // load from file
    std::string Sign(const std::string& message);
    bool Verify(const std::string& message, const std::string& signature);
private:
    ECDSA<ECP, SHA256>::PrivateKey privateKey;
    ECDSA<ECP, SHA256>::PublicKey publicKey;
};

void KeyPair::GenerateRandom(bool compress_public_key) {
    AutoSeededRandomPool prng;
    privateKey.Initialize(prng, ASN1::secp256k1());
    privateKey.MakePublicKey(publicKey);
    if (compress_public_key)
        publicKey.AccessGroupParameters().SetPointCompression(true);
}

std::string KeyPair::Sign(const std::string& message) {
    AutoSeededRandomPool prng;
    ECDSA<ECP, SHA256>::Signer signer(privateKey);
    size_t siglen = signer.MaxSignatureLength();
    std::string signature(siglen, 0x00);
    siglen = signer.SignMessage(prng, (const byte*)&message[0], message.size(), (byte*)&signature[0]);
    signature.resize(siglen);
    return signature;
}

bool KeyPair::Verify(const std::string& message, const std::string& signature) {
    ECDSA<ECP, SHA256>::Verifier verifier(publicKey);
    return verifier.VerifyMessage((const byte*)&message[0], message.size(), 
                                  (const byte*)&signature[0], signature.size());
}

}

int main() 
{   

    std::string message("to sign");

    bitcoin::KeyPair keyPair;
    keyPair.GenerateRandom(true);
    std::string signature = keyPair.Sign(message);
    std::cout << (keyPair.Verify(message, signature) ? "success" : "failure") << std::endl;

    // AutoSeededRandomPool prng;

    // // Create private key
    // ECDSA<ECP, SHA256>::PrivateKey privateKey;
    // privateKey.Initialize(prng, ASN1::secp256k1());

    // // Get public key
    // ECDSA<ECP, SHA256>::PublicKey publicKey;
    // privateKey.MakePublicKey(publicKey);
    // publicKey.AccessGroupParameters().SetPointCompression(true);

    // std::string message = "Do or do not. There is no try.";

    // // Sign message using the private key
    // ECDSA<ECP, SHA256>::Signer signer(privateKey);
    // size_t siglen = signer.MaxSignatureLength();
    // std::string signature(siglen, 0x00);
    // siglen = signer.SignMessage(prng, (const byte*)&message[0], message.size(), (byte*)&signature[0]);
    // signature.resize(siglen);

    // // Verify signature using the public key
    // ECDSA<ECP, SHA256>::Verifier verifier(publicKey);
    // bool result = verifier.VerifyMessage((const byte*)&message[0], message.size(), 
    //                                      (const byte*)&signature[0], signature.size());

    // // Check result                                         
    // std::cout << ((result) ? "success" : "failure") << std::endl;

    // ByteQueue pk;
    // publicKey.Save(pk);

    // FileSink fs("key.dat", true);
    // pk.TransferTo(fs);

    // std::cout << "sk: " << PrivateKeyToHex(privateKey) << std::endl;
    // std::cout << "pk: " << PublicKeyToHex(publicKey) << std::endl;

    return 0;
}