#ifndef CANCRC_H
#define CANCRC_H

#include <vector>
#include <cstdint>

namespace FiCo4OMNeT {

class CanCRC {
    public:
        // Polynomials (ISO 11898-1)
        static const uint16_t POLY_CRC15 = 0x4599;
        static const uint32_t POLY_CRC17 = 0x3685B;
        static const uint32_t POLY_CRC21 = 0x302899;

        // Calculate CRC-15 (Standard CAN)
        static uint16_t calculateCRC15(const std::vector<uint8_t>& bitStream);

        // Calculate CRC-17 (CAN FD < 16 bytes)
        static uint32_t calculateCRC17(const std::vector<uint8_t>& bitStream);

        // Calculate CRC-21 (CAN FD >= 16 bytes)
        static uint32_t calculateCRC21(const std::vector<uint8_t>& bitStream);
};

}

#endif // CANCRC_H
