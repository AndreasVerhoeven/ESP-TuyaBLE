#include "CryptoHelper.h"

#include <MD5Builder.h>
#include <CryptoAES_CBC.h>
#include <AES.h>
#include <CBC.h>
#include "bootloader_random.h"

static CBC<AES128> cbcaes128;
static CBC<AES256> cbcaes256;

Buffer CryptoHelper::md5(const uint8_t* data, size_t length) {
    MD5Builder md5builder;
    md5builder.begin();
    md5builder.add(const_cast<uint8_t*>(data), static_cast<uint16_t>(length));
    uint8_t digest[16] = {0};
    md5builder.calculate();
    md5builder.getBytes(digest);

    return Buffer(digest, sizeof(digest));
}

Buffer CryptoHelper::aesCbc128Decrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* cipherText, size_t length) {
    Buffer output(length);

    cbcaes128.setIV(iv, 16);
    cbcaes128.setKey(key, 16);
    cbcaes128.decrypt(output.data(), cipherText, length);

    return output;
}

Buffer CryptoHelper::aesCbc128Encrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* plainText, size_t length) {
   Buffer output(length);

    cbcaes128.setIV(iv, 16);
    cbcaes128.setKey(key, 16);
    cbcaes128.encrypt(output.data(), plainText, length);

    return output;
}

Buffer CryptoHelper::aesCbc256Encrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* plainText, size_t length) {
   Buffer output(length);

    cbcaes256.setIV(iv, 16);
    cbcaes256.setKey(key, 16);
    cbcaes256.encrypt(output.data(), plainText, length);

    return output;
}

Buffer CryptoHelper::aesCbc256Decrypt(const uint8_t* key, const uint8_t* iv, const uint8_t* cipherText, size_t length) {
   Buffer output(length);

    cbcaes256.setIV(iv, 16);
    cbcaes256.setKey(key, 16);
    cbcaes256.decrypt(output.data(), cipherText, length);

    return output;
}

Buffer CryptoHelper::iv(size_t length) {
    Buffer output(length);
    esp_fill_random(output.data(), length);
    return output;
}

uint16_t CryptoHelper::crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for(size_t i = 0; i < length; i++) {
        crc ^= static_cast<uint16_t>(data[i]);

        for(size_t b = 0; b < 8; b++) {
            uint16_t tmp = crc & 0x1;
            crc >>= 1;
            if(tmp != 0) {
                crc ^= 0xA001;
            }
        }
    }

    return crc;
}