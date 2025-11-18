/**
 * @file Memory.cpp
 * @brief Implementation of the Memory class for 6502 emulator
 */

#include "Memory.h"
#include <cstring>

namespace M6502 {

    Memory::Memory() {
        Initialize();
    }

    void Memory::Initialize() {
        // Clear all memory to zero (simulates power-on state)
        data.fill(0);
    }

    Byte Memory::ReadByte(Address address, Cycles& cycles) {
        // Reading from memory takes 1 cycle
        cycles++;
        return data[address];
    }

    Byte Memory::ReadByteNoCycles(Address address) const {
        return data[address];
    }

    void Memory::WriteByte(Address address, Byte value, Cycles& cycles) {
        // Writing to memory takes 1 cycle
        cycles++;
        data[address] = value;
    }

    Word Memory::ReadWord(Address address, Cycles& cycles) {
        // Read low byte (LSB)
        Byte lowByte = ReadByte(address, cycles);
        
        // Read high byte (MSB)
        Byte highByte = ReadByte(address + 1, cycles);
        
        // Combine into 16-bit word (little-endian)
        // Example: lowByte = 0x34, highByte = 0x12 -> result = 0x1234
        Word word = (static_cast<Word>(highByte) << 8) | lowByte;
        
        return word;
    }

    void Memory::WriteWord(Address address, Word value, Cycles& cycles) {
        // Extract low byte (bits 0-7)
        Byte lowByte = value & 0xFF;
        
        // Extract high byte (bits 8-15)
        Byte highByte = (value >> 8) & 0xFF;
        
        // Write low byte first (little-endian)
        WriteByte(address, lowByte, cycles);
        
        // Write high byte
        WriteByte(address + 1, highByte, cycles);
    }

    Byte& Memory::operator[](Address address) {
        return data[address];
    }

    const Byte& Memory::operator[](Address address) const {
        return data[address];
    }

} // namespace M6502
