#ifndef CRYPTO_HELPER_1234
#define CRYPTO_HELPER_1234

#include "Buffer.h"

class CryptoHelper {
public:
    // hashing
    static Buffer md5(const uint8_t* data, size_t length);

    // aes cbc encryption
    static Buffer aesCbc128Decrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* cipherText, size_t length);
    static Buffer aesCbc128Encrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* plainText, size_t length);
    static Buffer aesCbc256Encrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* plainText, size_t length);
    static Buffer aesCbc256Decrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* cipherText, size_t length);

    // random bytes
    static Buffer iv(size_t length = 16);

    // crc
    static uint16_t crc16(const uint8_t* data, size_t length);
};

#endif//CRYPTO_HELPER_1234