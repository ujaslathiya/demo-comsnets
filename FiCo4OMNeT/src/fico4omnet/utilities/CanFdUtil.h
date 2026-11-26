#pragma once
#include "fico4omnet/linklayer/can/messages/CanDataFrame_m.h"
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace FiCo4OMNeT {

class CanFdUtil {
public:
  static int dlcToLength(int dlc) {
    if (dlc <= 8)
      return dlc;
    switch (dlc) {
    case 9:
      return 12;
    case 10:
      return 16;
    case 11:
      return 20;
    case 12:
      return 24;
    case 13:
      return 32;
    case 14:
      return 48;
    case 15:
      return 64;
    default:
      throw std::runtime_error("Invalid CAN-FD DLC");
    }
  }

  static int lengthToDlc(int len) {
    if (len <= 8)
      return len;
    if (len <= 12)
      return 9;
    if (len <= 16)
      return 10;
    if (len <= 20)
      return 11;
    if (len <= 24)
      return 12;
    if (len <= 32)
      return 13;
    if (len <= 48)
      return 14;
    if (len <= 64)
      return 15;
    throw std::runtime_error("Invalid CAN-FD payload length");
  }

  static std::vector<uint8_t> serializeForCRC(CanDataFrame *df) {
    std::vector<uint8_t> bits;

    // 1. SOF (Start of Frame) - 1 dominant bit (0)
    bits.push_back(0);

    // 2. Arbitration Field (Simplified for CRC input)
    // Standard ID (11 bits)
    uint32_t id = df->getCanID();
    for (int i = 10; i >= 0; --i)
      bits.push_back((id >> i) & 1);

    if (df->getExtendedID()) {
      // SRR (1), IDE (1), Extended ID (18 bits)
      bits.push_back(1);
      bits.push_back(1);
      // Assuming ID field holds full 29 bits if extended
      for (int i = 17; i >= 0; --i)
        bits.push_back((id >> i) & 1);
      bits.push_back(df->getRtr() ? 1 : 0); // RTR/RRS
    } else {
      bits.push_back(df->getRtr() ? 1 : 0); // RTR
    }

    // 3. Control Field
    bits.push_back(df->isCanFd() ? 0 : 0); // IDE/Reserved (Simplified)
    bits.push_back(df->isCanFd() ? 1 : 0); // FDF

    if (df->isCanFd()) {
      bits.push_back(0);                    // Reserved
      bits.push_back(df->getBrs() ? 1 : 0); // BRS
      bits.push_back(df->getEsi() ? 1 : 0); // ESI
    }

    // DLC (4 bits)
    int dlc = df->getDlc();
    for (int i = 3; i >= 0; --i)
      bits.push_back((dlc >> i) & 1);

    // 4. Data Field
    int payloadLen = df->getPayloadArraySize();
    for (int i = 0; i < payloadLen; ++i) {
      uint8_t byte = df->getPayload(i);
      for (int b = 7; b >= 0; --b)
        bits.push_back((byte >> b) & 1);
    }

    return bits;
  }

  static double calculateTransmissionTime(CanDataFrame *df, double nominalBw,
                                          double dataBw) {
    if (!df->isCanFd() || !df->getBrs()) {
      return (double)calculateStuffedBitLength(df) / nominalBw;
    }

    std::vector<uint8_t> bits = serializeForCRC(df);
    int arbBits = 0;
    int dataBits = 0;
    int consecutive = 1;
    int lastBit = -1;

    // BRS Position: Standard (index 16), Extended (index 35)
    int brsIndex = df->getExtendedID() ? 35 : 16;

    for (size_t i = 0; i < bits.size(); ++i) {
      bool isArb = ((int)i <= brsIndex);

      if (isArb)
        arbBits++;
      else
        dataBits++;

      if (bits[i] == lastBit) {
        consecutive++;
        if (consecutive == 5) {
          if (isArb)
            arbBits++;
          else
            dataBits++;
          consecutive = 1;
          lastBit = (bits[i] == 0) ? 1 : 0;
        }
      } else {
        consecutive = 1;
        lastBit = bits[i];
      }
    }

    // Fixed fields
    // CRC (Data Phase)
    // 17 bits for payload < 16, 21 bits otherwise
    int crcLen = (df->getPayloadArraySize() < 16) ? 17 : 21;
    dataBits += crcLen;

    // CRC Delim (Data Phase)
    dataBits += 1;

    // ACK, ACK Delim, EOF (Arb Phase)
    arbBits += 9;

    return (double)arbBits / nominalBw + (double)dataBits / dataBw;
  }

  static int calculateStuffedBitLength(CanDataFrame *df) {
    std::vector<uint8_t> bits = serializeForCRC(df);
    int stuffedLength = 0;
    int consecutive = 1;
    int lastBit = -1;

    for (uint8_t bit : bits) {
      stuffedLength++;
      if (bit == lastBit) {
        consecutive++;
        if (consecutive == 5) {
          stuffedLength++;
          consecutive = 1;
          lastBit = (bit == 0) ? 1 : 0;
        }
      } else {
        consecutive = 1;
        lastBit = bit;
      }
    }

    int crcLen =
        df->isCanFd() ? (df->getPayloadArraySize() < 16 ? 17 : 21) : 15;
    stuffedLength += crcLen;
    stuffedLength += 10; // Footer

    return stuffedLength;
  }
};

} // namespace FiCo4OMNeT
