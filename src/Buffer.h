#ifndef BUFFER_1234
#define BUFFER_1234

#include <Arduino.h>
#include <stdint.h>
#include <vector>

/// This is a class that holds  
class Buffer {
private:
    std::vector<uint8_t> _bytes;

public:
    Buffer(const size_t length) {
        _bytes = std::vector<uint8_t>(length, 0);
    }

    Buffer(const void* ptr, const size_t length) {
        const uint8_t* bytesPtr = static_cast<const uint8_t*>(ptr);
        _bytes = std::vector<uint8_t>(bytesPtr, bytesPtr + length);
    }

    Buffer(const std::string& binaryString) {
        const uint8_t* bytesPtr = reinterpret_cast<const uint8_t*>(binaryString.data());
        _bytes = std::vector<uint8_t>(bytesPtr, bytesPtr + binaryString.size());        
    }

    Buffer(std::initializer_list<uint8_t> bytes) {
        _bytes = bytes;
    }

    Buffer() {}

    static const Buffer empty;

    uint8_t& operator[](size_t index) {
        return _bytes[index];
    }

    uint8_t operator[](size_t index) const {
        return _bytes[index];
    }

    Buffer operator+(const Buffer& other) {
        Buffer output = *this;
        output.append(other);
        return output;
    }

    // MARK: - get data
    uint8_t* data() const { return const_cast<uint8_t*>(&_bytes.at(0)); }
    const size_t size() const { return _bytes.size(); }

    // MARK: - Slicing
    Buffer subRangeWithStartAndLength(size_t start, size_t length) const {
        return Buffer(data() + start, length);
    }

    Buffer suffixFrom(size_t offset) const {
        return subRangeWithStartAndLength(offset, size() - offset);
    }

    Buffer prefixUntil(size_t length) const {
        return subRangeWithStartAndLength(0, length);
    }

    // MARK: - Appending
    void append(uint8_t value);
    void appendLittleEndian(uint16_t value);
    void appendLittleEndian(uint32_t value);
    void appendLittleEndian(int16_t value);
    void appendLittleEndian(int32_t value);
    void appendBigEndian(uint16_t value);
    void appendBigEndian(uint32_t value);
    void appendBigEndian(int16_t value);
    void appendBigEndian(int32_t value);
    void appendBigEndianWithNumberOfBytes(size_t size, size_t numberOfBytes); //supports 1, 2, 4
    void append(const Buffer& buffer);
    void append(const uint8_t* data, size_t length);
    void append(const Buffer& buffer, size_t start, size_t length);
    void append(const String& str);

    // MARK: - Packed Int

    /// packs an int: we shave of the bottom 7 bits of the value
    /// and store them in a byte. If there are remaining 1 bits
    /// in the value, we set the high-bit of the byte indicating
    /// more data is coming. Next up, we shave the next 7 bits
    /// of the value and store them in the next byte. The high-bit
    ///  is set if there are remaning 1 bits in the value, etc...
    void appendPackedInt(unsigned int value);

    // reading
    uint8_t readUint8(size_t& offset) const;
    uint16_t readBigEndianUint16(size_t& offset) const;
    uint16_t readBigEndianUint32(size_t& offset) const;
    unsigned int readPackedInt(size_t& offset) const;
    Buffer readBuffer(size_t& offset, size_t length) const;

    // MARK: - Converting
    Buffer md5() const;
    Buffer aesCbc128Decrypt(Buffer key, Buffer iv) const;
    Buffer aesCbc128Encrypt(Buffer key, Buffer iv) const;
    Buffer aesCbc256Encrypt(Buffer key, Buffer iv) const;
    Buffer aesCbc256Decrypt(Buffer key, Buffer iv) const;

    uint32_t asBigEndianUnsignedInt() const;
    int32_t asBigEndianSignedInt() const;
    String asString() const { return String(data(), static_cast<unsigned int>(size())); }

    String toHexString() const;
    String debugDescription() const;

    static Buffer aesInitializationVector();

    // crc
    uint16_t crc16() const;

    // padding
    void padToNumberOfBytes(size_t length, uint8_t byteValue = 0);
};

#endif//BUFFER_1234