/**
 * @file CPU.cpp
 * @brief Implementation of the 6502 CPU emulator
 */

#include "CPU.h"
#include <stdexcept>

namespace M6502 {

    CPU::CPU() {
        // Initialize all registers to zero
        A = X = Y = 0;
        PC = 0;
        SP = 0;
        P = 0;
        TotalCycles = 0;
    }

    void CPU::Reset(Memory& memory) {
        // Reset program counter from reset vector
        PC = memory.ReadWord(VECTOR_RESET, TotalCycles);
        
        // Initialize stack pointer to top of stack
        SP = STACK_POINTER_RESET;
        
        // Clear all flags except unused (which is always 1)
        P = FLAG_UNUSED;
        
        // Set interrupt disable flag (interrupts disabled on reset)
        SetFlag(FLAG_INTERRUPT, true);
        
        // Clear registers
        A = X = Y = 0;
        
        // Reset takes 8 cycles on real hardware
        TotalCycles += 6;  // We already consumed 2 cycles reading the vector
    }

    // ====================================================================
    // FLAG OPERATIONS
    // ====================================================================

    void CPU::SetFlag(StatusFlags flag, bool condition) {
        if (condition) {
            P |= flag;  // Set the flag bit
        } else {
            P &= ~flag; // Clear the flag bit
        }
    }

    bool CPU::GetFlag(StatusFlags flag) const {
        return (P & flag) != 0;
    }

    void CPU::UpdateZeroAndNegativeFlags(Byte value) {
        // Zero flag: set if value is 0
        SetFlag(FLAG_ZERO, value == 0);
        
        // Negative flag: set if bit 7 is set (value >= 128)
        SetFlag(FLAG_NEGATIVE, (value & 0x80) != 0);
    }

    // ====================================================================
    // STACK OPERATIONS
    // ====================================================================

    void CPU::PushByteToStack(Memory& memory, Byte value, Cycles& cycles) {
        // Stack is at $0100 + SP
        Address stackAddress = STACK_BASE + SP;
        memory.WriteByte(stackAddress, value, cycles);
        
        // Stack grows downward, so decrement SP
        SP--;
    }

    void CPU::PushWordToStack(Memory& memory, Word value, Cycles& cycles) {
        // Push high byte first (stack grows downward)
        PushByteToStack(memory, (value >> 8) & 0xFF, cycles);
        // Then push low byte
        PushByteToStack(memory, value & 0xFF, cycles);
    }

    Byte CPU::PopByteFromStack(Memory& memory, Cycles& cycles) {
        // Increment SP first (stack grows downward)
        SP++;
        
        // Read from stack
        Address stackAddress = STACK_BASE + SP;
        return memory.ReadByte(stackAddress, cycles);
    }

    Word CPU::PopWordFromStack(Memory& memory, Cycles& cycles) {
        // Pop low byte first
        Byte lowByte = PopByteFromStack(memory, cycles);
        // Then pop high byte
        Byte highByte = PopByteFromStack(memory, cycles);
        
        // Combine into word
        return (static_cast<Word>(highByte) << 8) | lowByte;
    }

    // ====================================================================
    // MEMORY ACCESS HELPERS
    // ====================================================================

    Byte CPU::FetchByte(Memory& memory, Cycles& cycles) {
        Byte value = memory.ReadByte(PC, cycles);
        PC++;
        return value;
    }

    Word CPU::FetchWord(Memory& memory, Cycles& cycles) {
        // 6502 is little-endian
        Word value = memory.ReadWord(PC, cycles);
        PC += 2;
        return value;
    }

    // ====================================================================
    // ADDRESSING MODES
    // ====================================================================

    Address CPU::AddrImmediate(Memory& /* memory */, Cycles& cycles) {
        // Immediate: operand is the next byte after opcode
        // Return PC and increment it
        Address address = PC;
        PC++;
        cycles++; // Fetching immediate value takes 1 cycle
        return address;
    }

    Address CPU::AddrZeroPage(Memory& memory, Cycles& cycles) {
        // Zero page: next byte is address in page 0 ($00XX)
        Byte zpAddress = FetchByte(memory, cycles);
        return static_cast<Address>(zpAddress);
    }

    Address CPU::AddrZeroPageX(Memory& memory, Cycles& cycles) {
        // Zero page indexed by X: wraps within page 0
        Byte zpAddress = FetchByte(memory, cycles);
        
        // Add X register (wraps around in zero page)
        Byte finalAddress = zpAddress + X;
        
        // Extra cycle for adding index
        cycles++;
        
        return static_cast<Address>(finalAddress);
    }

    Address CPU::AddrZeroPageY(Memory& memory, Cycles& cycles) {
        // Zero page indexed by Y: wraps within page 0
        Byte zpAddress = FetchByte(memory, cycles);
        
        // Add Y register (wraps around in zero page)
        Byte finalAddress = zpAddress + Y;
        
        // Extra cycle for adding index
        cycles++;
        
        return static_cast<Address>(finalAddress);
    }

    Address CPU::AddrAbsolute(Memory& memory, Cycles& cycles) {
        // Absolute: next two bytes form 16-bit address
        return FetchWord(memory, cycles);
    }

    Address CPU::AddrAbsoluteX(Memory& memory, Cycles& cycles, bool addCycleOnPageCross) {
        // Absolute indexed by X
        Address baseAddress = FetchWord(memory, cycles);
        Address finalAddress = baseAddress + X;
        
        // Check if page boundary was crossed
        if (addCycleOnPageCross) {
            // Page boundary crossed if high byte changed
            if ((baseAddress & 0xFF00) != (finalAddress & 0xFF00)) {
                cycles++;
            }
        }
        
        return finalAddress;
    }

    Address CPU::AddrAbsoluteY(Memory& memory, Cycles& cycles, bool addCycleOnPageCross) {
        // Absolute indexed by Y
        Address baseAddress = FetchWord(memory, cycles);
        Address finalAddress = baseAddress + Y;
        
        // Check if page boundary was crossed
        if (addCycleOnPageCross) {
            // Page boundary crossed if high byte changed
            if ((baseAddress & 0xFF00) != (finalAddress & 0xFF00)) {
                cycles++;
            }
        }
        
        return finalAddress;
    }

    Address CPU::AddrIndexedIndirect(Memory& memory, Cycles& cycles) {
        // Indexed Indirect: ($ZP,X)
        // Add X to zero page address, then read 16-bit address from there
        
        Byte zpAddress = FetchByte(memory, cycles);
        
        // Add X (wraps in zero page)
        Byte finalZpAddress = zpAddress + X;
        
        // Extra cycle for index calculation
        cycles++;
        
        // Read 16-bit address from zero page (wraps at page boundary)
        Byte lowByte = memory.ReadByte(finalZpAddress, cycles);
        Byte highByte = memory.ReadByte((finalZpAddress + 1) & 0xFF, cycles);
        
        return (static_cast<Address>(highByte) << 8) | lowByte;
    }

    Address CPU::AddrIndirectIndexed(Memory& memory, Cycles& cycles, bool addCycleOnPageCross) {
        // Indirect Indexed: ($ZP),Y
        // Read 16-bit address from zero page, then add Y
        
        Byte zpAddress = FetchByte(memory, cycles);
        
        // Read 16-bit base address from zero page
        Byte lowByte = memory.ReadByte(zpAddress, cycles);
        Byte highByte = memory.ReadByte((zpAddress + 1) & 0xFF, cycles);
        
        Address baseAddress = (static_cast<Address>(highByte) << 8) | lowByte;
        Address finalAddress = baseAddress + Y;
        
        // Check for page boundary crossing
        if (addCycleOnPageCross) {
            if ((baseAddress & 0xFF00) != (finalAddress & 0xFF00)) {
                cycles++;
            }
        }
        
        return finalAddress;
    }

    // ====================================================================
    // LOAD/STORE INSTRUCTIONS
    // ====================================================================

    void CPU::LDA(Memory& memory, Cycles& cycles, Address address) {
        // Load Accumulator from memory
        A = memory.ReadByte(address, cycles);
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::LDX(Memory& memory, Cycles& cycles, Address address) {
        // Load X register from memory
        X = memory.ReadByte(address, cycles);
        UpdateZeroAndNegativeFlags(X);
    }

    void CPU::LDY(Memory& memory, Cycles& cycles, Address address) {
        // Load Y register from memory
        Y = memory.ReadByte(address, cycles);
        UpdateZeroAndNegativeFlags(Y);
    }

    void CPU::STA(Memory& memory, Cycles& cycles, Address address) {
        // Store Accumulator to memory
        memory.WriteByte(address, A, cycles);
    }

    void CPU::STX(Memory& memory, Cycles& cycles, Address address) {
        // Store X register to memory
        memory.WriteByte(address, X, cycles);
    }

    void CPU::STY(Memory& memory, Cycles& cycles, Address address) {
        // Store Y register to memory
        memory.WriteByte(address, Y, cycles);
    }

    // ====================================================================
    // REGISTER TRANSFER INSTRUCTIONS
    // ====================================================================

    void CPU::TAX(Cycles& cycles) {
        // Transfer A to X
        X = A;
        cycles++;
        UpdateZeroAndNegativeFlags(X);
    }

    void CPU::TAY(Cycles& cycles) {
        // Transfer A to Y
        Y = A;
        cycles++;
        UpdateZeroAndNegativeFlags(Y);
    }

    void CPU::TXA(Cycles& cycles) {
        // Transfer X to A
        A = X;
        cycles++;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::TYA(Cycles& cycles) {
        // Transfer Y to A
        A = Y;
        cycles++;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::TSX(Cycles& cycles) {
        // Transfer Stack Pointer to X
        X = SP;
        cycles++;
        UpdateZeroAndNegativeFlags(X);
    }

    void CPU::TXS(Cycles& cycles) {
        // Transfer X to Stack Pointer
        SP = X;
        cycles++;
        // Note: TXS does NOT affect flags
    }

    // ====================================================================
    // STACK INSTRUCTIONS
    // ====================================================================

    void CPU::PHA(Memory& memory, Cycles& cycles) {
        // Push Accumulator onto stack
        cycles++; // Internal operation
        PushByteToStack(memory, A, cycles);
    }

    void CPU::PHP(Memory& memory, Cycles& cycles) {
        // Push Processor Status onto stack
        // Note: B and U flags are set when pushed
        cycles++; // Internal operation
        Byte statusToStore = P | FLAG_BREAK | FLAG_UNUSED;
        PushByteToStack(memory, statusToStore, cycles);
    }

    void CPU::PLA(Memory& memory, Cycles& cycles) {
        // Pull Accumulator from stack
        cycles += 2; // Internal operations
        A = PopByteFromStack(memory, cycles);
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::PLP(Memory& memory, Cycles& cycles) {
        // Pull Processor Status from stack
        cycles += 2; // Internal operations
        P = PopByteFromStack(memory, cycles);
        // Ensure unused flag is always set
        SetFlag(FLAG_UNUSED, true);
    }

    // ====================================================================
    // LOGICAL INSTRUCTIONS
    // ====================================================================

    void CPU::AND(Memory& memory, Cycles& cycles, Address address) {
        // Logical AND with accumulator
        Byte value = memory.ReadByte(address, cycles);
        A &= value;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::ORA(Memory& memory, Cycles& cycles, Address address) {
        // Logical OR with accumulator
        Byte value = memory.ReadByte(address, cycles);
        A |= value;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::EOR(Memory& memory, Cycles& cycles, Address address) {
        // Exclusive OR with accumulator
        Byte value = memory.ReadByte(address, cycles);
        A ^= value;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::BIT(Memory& memory, Cycles& cycles, Address address) {
        // Test bits in memory with accumulator
        Byte value = memory.ReadByte(address, cycles);
        
        // Z flag = !(A & value)
        SetFlag(FLAG_ZERO, (A & value) == 0);
        
        // N flag = bit 7 of memory value
        SetFlag(FLAG_NEGATIVE, (value & 0x80) != 0);
        
        // V flag = bit 6 of memory value
        SetFlag(FLAG_OVERFLOW, (value & 0x40) != 0);
    }

    // ====================================================================
    // ARITHMETIC INSTRUCTIONS
    // ====================================================================

    void CPU::ADC(Memory& memory, Cycles& cycles, Address address) {
        // Add with Carry
        Byte operand = memory.ReadByte(address, cycles);
        
        if (GetFlag(FLAG_DECIMAL)) {
            // BCD (Binary Coded Decimal) mode
            // Each nibble represents 0-9
            
            Word sum = (A & 0x0F) + (operand & 0x0F) + (GetFlag(FLAG_CARRY) ? 1 : 0);
            
            // Adjust low nibble if > 9
            if (sum > 0x09) {
                sum += 0x06;
            }
            
            // Add high nibbles
            sum = (A & 0xF0) + (operand & 0xF0) + (sum > 0x0F ? 0x10 : 0) + (sum & 0x0F);
            
            // Set flags (except C, which is set after adjustment)
            SetFlag(FLAG_NEGATIVE, (sum & 0x80) != 0);
            SetFlag(FLAG_ZERO, (sum & 0xFF) == 0);
            
            // Overflow flag in BCD mode
            bool overflow = ((A ^ sum) & (operand ^ sum) & 0x80) != 0;
            SetFlag(FLAG_OVERFLOW, overflow);
            
            // Adjust high nibble if > 9
            if ((sum & 0xF0) > 0x90) {
                sum += 0x60;
            }
            
            // Set carry if result > 99
            SetFlag(FLAG_CARRY, sum > 0x99);
            
            A = sum & 0xFF;
            
        } else {
            // Binary mode
            Word sum = A + operand + (GetFlag(FLAG_CARRY) ? 1 : 0);
            
            // Carry flag: set if result > 255
            SetFlag(FLAG_CARRY, sum > 0xFF);
            
            // Overflow flag: set if sign bit is incorrect
            // Overflow occurs when: both inputs have same sign AND result has different sign
            bool overflow = ((A ^ sum) & (operand ^ sum) & 0x80) != 0;
            SetFlag(FLAG_OVERFLOW, overflow);
            
            A = sum & 0xFF;
            UpdateZeroAndNegativeFlags(A);
        }
    }

    void CPU::SBC(Memory& memory, Cycles& cycles, Address address) {
        // Subtract with Carry (borrow)
        // SBC is equivalent to ADC with inverted operand
        Byte operand = memory.ReadByte(address, cycles);
        
        if (GetFlag(FLAG_DECIMAL)) {
            // BCD mode subtraction
            Word diff = (A & 0x0F) - (operand & 0x0F) - (GetFlag(FLAG_CARRY) ? 0 : 1);
            
            // Adjust low nibble if negative
            if (diff & 0x10) {
                diff = ((diff - 0x06) & 0x0F) | ((A & 0xF0) - (operand & 0xF0) - 0x10);
            } else {
                diff = (diff & 0x0F) | ((A & 0xF0) - (operand & 0xF0));
            }
            
            // Adjust high nibble if negative
            if (diff & 0x100) {
                diff -= 0x60;
            }
            
            // Set carry (borrow) flag: clear if borrow occurred
            SetFlag(FLAG_CARRY, (diff & 0x100) == 0);
            
            A = diff & 0xFF;
            UpdateZeroAndNegativeFlags(A);
            
        } else {
            // Binary mode: use ADC logic with inverted operand
            Word sum = A + (operand ^ 0xFF) + (GetFlag(FLAG_CARRY) ? 1 : 0);
            
            // Carry is set if NO borrow occurred
            SetFlag(FLAG_CARRY, sum > 0xFF);
            
            // Overflow flag
            bool overflow = ((A ^ sum) & ((operand ^ 0xFF) ^ sum) & 0x80) != 0;
            SetFlag(FLAG_OVERFLOW, overflow);
            
            A = sum & 0xFF;
            UpdateZeroAndNegativeFlags(A);
        }
    }

    void CPU::CompareRegister(Byte regValue, Byte memValue) {
        // Compare helper function used by CMP, CPX, CPY
        Word result = regValue - memValue;
        
        // Carry is set if regValue >= memValue (no borrow)
        SetFlag(FLAG_CARRY, regValue >= memValue);
        
        // Zero is set if values are equal
        SetFlag(FLAG_ZERO, regValue == memValue);
        
        // Negative is set based on bit 7 of result
        SetFlag(FLAG_NEGATIVE, (result & 0x80) != 0);
    }

    void CPU::CMP(Memory& memory, Cycles& cycles, Address address) {
        // Compare Accumulator
        Byte value = memory.ReadByte(address, cycles);
        CompareRegister(A, value);
    }

    void CPU::CPX(Memory& memory, Cycles& cycles, Address address) {
        // Compare X register
        Byte value = memory.ReadByte(address, cycles);
        CompareRegister(X, value);
    }

    void CPU::CPY(Memory& memory, Cycles& cycles, Address address) {
        // Compare Y register
        Byte value = memory.ReadByte(address, cycles);
        CompareRegister(Y, value);
    }

    // ====================================================================
    // INCREMENT/DECREMENT INSTRUCTIONS
    // ====================================================================

    void CPU::INC(Memory& memory, Cycles& cycles, Address address) {
        // Increment memory
        Byte value = memory.ReadByte(address, cycles);
        value++;
        cycles++; // Extra cycle for operation
        memory.WriteByte(address, value, cycles);
        UpdateZeroAndNegativeFlags(value);
    }

    void CPU::INX(Cycles& cycles) {
        // Increment X
        X++;
        cycles++;
        UpdateZeroAndNegativeFlags(X);
    }

    void CPU::INY(Cycles& cycles) {
        // Increment Y
        Y++;
        cycles++;
        UpdateZeroAndNegativeFlags(Y);
    }

    void CPU::DEC(Memory& memory, Cycles& cycles, Address address) {
        // Decrement memory
        Byte value = memory.ReadByte(address, cycles);
        value--;
        cycles++; // Extra cycle for operation
        memory.WriteByte(address, value, cycles);
        UpdateZeroAndNegativeFlags(value);
    }

    void CPU::DEX(Cycles& cycles) {
        // Decrement X
        X--;
        cycles++;
        UpdateZeroAndNegativeFlags(X);
    }

    void CPU::DEY(Cycles& cycles) {
        // Decrement Y
        Y--;
        cycles++;
        UpdateZeroAndNegativeFlags(Y);
    }

    // Continue in next part...

} // namespace M6502
