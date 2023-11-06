#ifndef TUYA_DATAPOINT_123
#define TUYA_DATAPOINT_123

#include "Buffer.h"

enum class TuyaDataPointType: uint8_t {
    raw = 0,
    boolean = 1,
    value = 2,
    string = 3,
    enumeration = 4,
    bitmap = 5,
};

class String;

class TuyaDataPoint {
private:
    uint8_t _dp = 0;
    TuyaDataPointType _type = TuyaDataPointType::raw;

    Buffer _buffer;
    int32_t _value;
    uint8_t _enumeration;
    String _string;

public:
    TuyaDataPoint(uint8_t dp, TuyaDataPointType type): _dp(dp), _type(type) {}

    static TuyaDataPoint raw(uint8_t dp, const Buffer& value) {
        TuyaDataPoint dataPoint(dp, TuyaDataPointType::raw);
        dataPoint.setRaw(value);
        return dataPoint;
    }

    static TuyaDataPoint boolean(uint8_t dp, boolean value) {
        TuyaDataPoint dataPoint(dp, TuyaDataPointType::boolean);
        dataPoint.setBoolean(value);
        return dataPoint;
    }

    static TuyaDataPoint value(uint8_t dp, int32_t value) {
        TuyaDataPoint dataPoint(dp, TuyaDataPointType::value);
        dataPoint.setValue(value);
        return dataPoint;
    }

    static TuyaDataPoint string(uint8_t dp, const String& value) {
        TuyaDataPoint dataPoint(dp, TuyaDataPointType::string);
        dataPoint.setString(value);
        return dataPoint;
    }

    static TuyaDataPoint enumeration(uint8_t dp, uint8_t value) {
        TuyaDataPoint dataPoint(dp, TuyaDataPointType::enumeration);
        dataPoint.setEnumeration(value);
        return dataPoint;
    }

    static TuyaDataPoint bitmap(uint8_t dp, const Buffer& value) {
        TuyaDataPoint dataPoint(dp, TuyaDataPointType::bitmap);
        dataPoint.setBitmap(value);
        return dataPoint;
    }
    uint8_t dp() const { return _dp; }
    TuyaDataPointType type() const { return _type; }

    const Buffer& raw() const { return _buffer; }
    bool boolean() const  { return _value != 0; }
    int32_t value() const  { return _value; }
    const String& string() const  { return _string; }
    uint8_t enumeration() const  { return static_cast<uint8_t>(_value); }
    const Buffer& bitmap() const { return _buffer; }

    void setRaw(const Buffer& data) { 
        _buffer = data; 
        _string = "";
        _value = 0;
    }

    void setBoolean(bool value) {
        setValue(value ? 0 : 1);
    }

    void setValue(int32_t value) {
        _value = value;
        _buffer = Buffer();
        _string = "";
    }

    void setString(const String& string) {
        _string = string;
        _buffer = Buffer();
        _value = 0;
    }

    void setEnumeration(int32_t enumeration) {
        setValue(enumeration);
    }

    void setBitmap(const Buffer& bitmap) {
        setRaw(bitmap);
    }

    String debugDescription() const;
};

#endif//TUYA_DATAPOINT_123