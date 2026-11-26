#include "fico4omnet/utilities/CanCRC.h"

namespace FiCo4OMNeT {

uint16_t CanCRC::calculateCRC15(const std::vector<uint8_t>& bitStream) {
    uint16_t crc = 0; // Initialization 0 for Standard CAN
    uint16_t poly = POLY_CRC15;

    for (uint8_t bit : bitStream) {
        bool bitVal = bit != 0;
        bool crcNext = (crc >> 14) & 1; // 15th bit (index 14)
        crc <<= 1;
        if (bitVal ^ crcNext) {
            crc ^= poly;
        }
        crc &= 0x7FFF; // Keep to 15 bits
    }
    return crc;
}

uint32_t CanCRC::calculateCRC17(const std::vector<uint8_t>& bitStream) {
    uint32_t crc = 0x10000; // Initialization (1 at x^16) per ISO 11898-1 2015
    uint32_t poly = POLY_CRC17;

    for (uint8_t bit : bitStream) {
        bool bitVal = bit != 0;
        bool crcNext = (crc >> 16) & 1;
        crc <<= 1;
        if (bitVal ^ crcNext) {
            crc ^= poly;
        }
        crc &= 0x1FFFF; // Keep to 17 bits
    }
    return crc;
}

uint32_t CanCRC::calculateCRC21(const std::vector<uint8_t>& bitStream) {
    uint32_t crc = 0x100000; // Initialization (1 at x^20) per ISO 11898-1 2015
    uint32_t poly = POLY_CRC21;

    for (uint8_t bit : bitStream) {
        bool bitVal = bit != 0;
        bool crcNext = (crc >> 20) & 1;
        crc <<= 1;
        if (bitVal ^ crcNext) {
            crc ^= poly;
        }
        crc &= 0x1FFFFF; // Keep to 21 bits
    }
    return crc;
}

}
