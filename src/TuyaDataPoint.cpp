#include "TuyaDataPoint.h"

String TuyaDataPoint::debugDescription() const {
    String output = "dp = " + String(dp()) + ", type = ";

    switch(_type) {
        case TuyaDataPointType::raw:
            output += "RAW, value = " + raw().debugDescription();
        break;

        case TuyaDataPointType::boolean:
            output += "BOOLEAN, value = " + String(boolean() ? "true" : "false");
        break;

        case TuyaDataPointType::value:
            output += "VALUE, value = " + String(value());
        break;

        case TuyaDataPointType::string:
            output += "STRING, value = " + string();
        break;

        case TuyaDataPointType::enumeration:
            output += "ENUM, value = " + String(enumeration());
        break;

        case TuyaDataPointType::bitmap:
            output += "BITMAP, value = " + bitmap().debugDescription();
        break;

        default:
            output += "UNKNOWN (" + String(static_cast<uint16_t>(type())) + ")";
    }

    return output;
}