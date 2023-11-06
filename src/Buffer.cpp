#include "Buffer.h"
#include "CryptoHelper.h"
#include <endian.h>

const Buffer Buffer::empty = Buffer();

Buffer Buffer::md5() const {
    return CryptoHelper::md5(data(), size());
}

Buffer Buffer::aesCbc128Decrypt(Buffer key, Buffer iv) const {
    return CryptoHelper::aesCbc128Decrypt(key.data(), iv.data(), data(), size());
}

Buffer Buffer::aesCbc128Encrypt(Buffer key, Buffer iv) const {
    return CryptoHelper::aesCbc128Encrypt(key.data(), iv.data(), data(), size());
}

Buffer Buffer::aesCbc256Encrypt(const Buffer key, Buffer iv) const {
    return CryptoHelper::aesCbc256Encrypt(key.data(), iv.data(), data(), size());
}
Buffer Buffer::aesCbc256Decrypt(Buffer key, Buffer iv) const {
    return CryptoHelper::aesCbc256Decrypt(key.data(), iv.data(), data(), size());
}

uint32_t Buffer::asBigEndianUnsignedInt() const {
    switch(size()) {
        case 0: 
            return 0;

        case 1: 
           return static_cast<uint32_t>(_bytes[0]);
        
        case 2:
         return be16toh(*reinterpret_cast<const uint16_t*>(&_bytes[0]));

        case 3: {
            uint8_t fakeBytes[4] = {0, _bytes[1], _bytes[2], _bytes[3]};
            return be32toh(*reinterpret_cast<const uint32_t*>(&fakeBytes[0]));
        }

        case 4: 
            return be32toh(*reinterpret_cast<const uint32_t*>(&_bytes[0]));

        default:
            return 0;
    }
}
int32_t Buffer::asBigEndianSignedInt() const {
    switch(size()) {
        case 0: 
            return 0;

        case 1: 
            return static_cast<int32_t>(_bytes[0]);
        
        case 2:
         return be16toh(*reinterpret_cast<const int16_t*>(&_bytes[0]));

        case 3: {
            uint8_t fakeBytes[4] = {0, _bytes[1], _bytes[2], _bytes[3]};
            return be32toh(*reinterpret_cast<const int32_t*>(&fakeBytes[0]));
        }

        case 4: 
            return be32toh(*reinterpret_cast<const int32_t*>(&_bytes[0]));

        default:
            return 0;
    }
}

uint16_t Buffer::crc16() const { 
    return CryptoHelper::crc16(data(), size()); 
}

Buffer Buffer::aesInitializationVector() {
    return CryptoHelper::iv(16);
}

void Buffer::append(uint8_t value) {
    _bytes.push_back(value);
}

void Buffer::appendLittleEndian(uint16_t value) {
    uint16_t littleEndianValue = htole16(value);
    append((uint8_t*)&littleEndianValue, 2);
}
void Buffer::appendLittleEndian(uint32_t value) {
    uint32_t littleEndianValue = htole32(value);
    append((uint8_t*)&littleEndianValue, 4);
}

void Buffer::appendLittleEndian(int16_t value) {
    int16_t littleEndianValue = htole16(value);
    append((uint8_t*)&littleEndianValue, 2);
}
void Buffer::appendLittleEndian(int32_t value) {
    int32_t littleEndianValue = htole32(value);
    append((uint8_t*)&littleEndianValue, 4);
}

void Buffer::appendBigEndian(uint16_t value) {
    uint16_t bigEndianValue = htobe16(value);
    append((uint8_t*)&bigEndianValue, 2);
}

void Buffer::appendBigEndian(uint32_t value) {
    uint32_t bigEndianValue = htobe32(value);
    append((uint8_t*)&bigEndianValue, 4);
}

void Buffer::appendBigEndian(int16_t value) {
    int16_t bigEndianValue = htobe16(value);
    append((uint8_t*)&bigEndianValue, 2);
}
void Buffer::appendBigEndian(int32_t value) {
    int32_t bigEndianValue = htobe32(value);
    append((uint8_t*)&bigEndianValue, 4);
}

void Buffer::appendBigEndianWithNumberOfBytes(size_t size, size_t numberOfBytes) {
    if(numberOfBytes == 1) {
        append(static_cast<uint8_t>(size));
    } else if(numberOfBytes == 2) {
        appendBigEndian(static_cast<uint16_t>(size));
    } else if(numberOfBytes == 4) {
        appendBigEndian(static_cast<uint32_t>(size));
    }
}

void Buffer::append(const Buffer& buffer) {
    _bytes.insert(_bytes.end(), buffer._bytes.begin(), buffer._bytes.end());
}

void Buffer::append(const uint8_t* data, size_t length) {
    _bytes.insert(_bytes.end(), data, data + length);
}

void Buffer::append(const Buffer& buffer, size_t start, size_t length) {
    _bytes.insert(_bytes.end(), buffer._bytes.begin() + start, buffer._bytes.begin() + start + length);   
}

void Buffer::append(const String& str) {
    append(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

void Buffer::appendPackedInt(unsigned int value) {
    while(true) {
        uint8_t currentByte = value & 0x7F;
        value >>= 7;
        if(value != 0) {
            currentByte |= 0x80;
        }
        append(currentByte);
        if(value == 0) {
            break;
        }
    }
}

unsigned int Buffer::readPackedInt(size_t& offset) const {
    size_t numberOfBytesRead = 0;
    unsigned int value  = 0;
    while(offset < size()) {
        uint8_t byte = _bytes[offset];
        value |= (byte & 0x7F) << (numberOfBytesRead * 7);
        offset += 1;
        numberOfBytesRead += 1;
        if((byte & 0x80)  == 0) {
            break;
        }
    }

    return value;
}

uint8_t Buffer::readUint8(size_t& offset) const {
    if(offset + 1 > size()) return 0;
    return _bytes[offset++];
}

uint16_t Buffer::readBigEndianUint16(size_t& offset) const {
    if(offset + 2 > size()) return 0;
    const uint16_t* bigEndianValue = reinterpret_cast<const uint16_t*>(&_bytes[offset]);
    offset += 2;
    return be16toh(*bigEndianValue);
}

uint16_t Buffer::readBigEndianUint32(size_t& offset) const {
    if(offset + 4 > size()) return 0;
    const uint32_t* bigEndianValue = reinterpret_cast<const uint32_t*>(&_bytes[offset]);
    offset += 4;
    return be32toh(*bigEndianValue);
}

Buffer Buffer::readBuffer(size_t& offset, size_t length) const {
    if(offset + length > size()) return Buffer();
    
    Buffer result = this->subRangeWithStartAndLength(offset, length);
    offset += length;
    return result;
}

void Buffer::padToNumberOfBytes(size_t length, uint8_t byteValue) {
    size_t numBytesToAdd = length - (size() % length);
    if(numBytesToAdd < length)
        _bytes.insert(_bytes.end(), numBytesToAdd, byteValue);
}

String Buffer::toHexString() const {
  String output;
  for(int i = 0; i < size(); i++) {
    String byteHex = String(_bytes[i], 16);
    if(byteHex.length() == 1)
      output += "0";

    output += byteHex;
  }

  return output;
}

String Buffer::debugDescription() const {
    return toHexString() + " (" +  String(size()) + String(size() == 1 ? " byte)" : " bytes)");
}