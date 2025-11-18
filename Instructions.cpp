/**
 * @file Instructions.cpp
 * @brief Continued implementation of CPU instructions and execution loop
 */

#include "CPU.h"

namespace M6502 {

    // ====================================================================
    // SHIFT/ROTATE INSTRUCTIONS
    // ====================================================================

    void CPU::ASL_ACC(Cycles& cycles) {
        // Arithmetic Shift Left - Accumulator
        // Shift all bits left, bit 0 becomes 0, bit 7 goes to carry
        
        SetFlag(FLAG_CARRY, (A & 0x80) != 0);  // Bit 7 to carry
        A = A << 1;
        cycles++;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::ASL_MEM(Memory& memory, Cycles& cycles, Address address) {
        // Arithmetic Shift Left - Memory
        Byte value = memory.ReadByte(address, cycles);
        SetFlag(FLAG_CARRY, (value & 0x80) != 0);
        value = value << 1;
        cycles++; // Extra cycle for operation
        memory.WriteByte(address, value, cycles);
        UpdateZeroAndNegativeFlags(value);
    }

    void CPU::LSR_ACC(Cycles& cycles) {
        // Logical Shift Right - Accumulator
        // Shift all bits right, bit 7 becomes 0, bit 0 goes to carry
        
        SetFlag(FLAG_CARRY, (A & 0x01) != 0);  // Bit 0 to carry
        A = A >> 1;
        cycles++;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::LSR_MEM(Memory& memory, Cycles& cycles, Address address) {
        // Logical Shift Right - Memory
        Byte value = memory.ReadByte(address, cycles);
        SetFlag(FLAG_CARRY, (value & 0x01) != 0);
        value = value >> 1;
        cycles++; // Extra cycle for operation
        memory.WriteByte(address, value, cycles);
        UpdateZeroAndNegativeFlags(value);
    }

    void CPU::ROL_ACC(Cycles& cycles) {
        // Rotate Left - Accumulator
        // Rotate all bits left through carry
        // Old bit 7 -> Carry, Carry -> bit 0
        
        bool oldCarry = GetFlag(FLAG_CARRY);
        SetFlag(FLAG_CARRY, (A & 0x80) != 0);
        A = (A << 1) | (oldCarry ? 1 : 0);
        cycles++;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::ROL_MEM(Memory& memory, Cycles& cycles, Address address) {
        // Rotate Left - Memory
        Byte value = memory.ReadByte(address, cycles);
        bool oldCarry = GetFlag(FLAG_CARRY);
        SetFlag(FLAG_CARRY, (value & 0x80) != 0);
        value = (value << 1) | (oldCarry ? 1 : 0);
        cycles++;
        memory.WriteByte(address, value, cycles);
        UpdateZeroAndNegativeFlags(value);
    }

    void CPU::ROR_ACC(Cycles& cycles) {
        // Rotate Right - Accumulator
        // Rotate all bits right through carry
        // Old bit 0 -> Carry, Carry -> bit 7
        
        bool oldCarry = GetFlag(FLAG_CARRY);
        SetFlag(FLAG_CARRY, (A & 0x01) != 0);
        A = (A >> 1) | (oldCarry ? 0x80 : 0);
        cycles++;
        UpdateZeroAndNegativeFlags(A);
    }

    void CPU::ROR_MEM(Memory& memory, Cycles& cycles, Address address) {
        // Rotate Right - Memory
        Byte value = memory.ReadByte(address, cycles);
        bool oldCarry = GetFlag(FLAG_CARRY);
        SetFlag(FLAG_CARRY, (value & 0x01) != 0);
        value = (value >> 1) | (oldCarry ? 0x80 : 0);
        cycles++;
        memory.WriteByte(address, value, cycles);
        UpdateZeroAndNegativeFlags(value);
    }

    // ====================================================================
    // JUMP/BRANCH INSTRUCTIONS
    // ====================================================================

    void CPU::JMP(Word address) {
        // Jump to address
        PC = address;
    }

    void CPU::JSR(Memory& memory, Cycles& cycles, Address address) {
        // Jump to Subroutine
        // Push return address (PC - 1) onto stack
        
        cycles++; // Internal operation
        
        // Push PC - 1 (address of last byte of JSR instruction)
        Word returnAddress = PC - 1;
        PushWordToStack(memory, returnAddress, cycles);
        
        // Jump to target
        PC = address;
    }

    void CPU::RTS(Memory& memory, Cycles& cycles) {
        // Return from Subroutine
        // Pull return address from stack and increment it
        
        cycles += 2; // Internal operations
        
        // Pop return address
        Word returnAddress = PopWordFromStack(memory, cycles);
        
        // Increment to get next instruction
        PC = returnAddress + 1;
        
        cycles++; // Extra cycle
    }

    void CPU::RTI(Memory& memory, Cycles& cycles) {
        // Return from Interrupt
        // Pull processor status, then PC from stack
        
        cycles++; // Internal operation
        
        // Pull status register
        P = PopByteFromStack(memory, cycles);
        SetFlag(FLAG_UNUSED, true); // Ensure unused bit is set
        
        // Pull program counter
        PC = PopWordFromStack(memory, cycles);
    }

    void CPU::BranchIf(Memory& memory, Cycles& cycles, bool condition) {
        // Branch helper function
        // Reads signed offset and branches if condition is true
        
        SignedByte offset = static_cast<SignedByte>(FetchByte(memory, cycles));
        
        if (condition) {
            // Branch taken
            cycles++; // Extra cycle for taken branch
            
            Address oldPC = PC;
            PC += offset;
            
            // Extra cycle if page boundary crossed
            if ((oldPC & 0xFF00) != (PC & 0xFF00)) {
                cycles++;
            }
        }
    }

    // ====================================================================
    // FLAG INSTRUCTIONS
    // ====================================================================

    void CPU::CLC(Cycles& cycles) {
        // Clear Carry
        SetFlag(FLAG_CARRY, false);
        cycles++;
    }

    void CPU::CLD(Cycles& cycles) {
        // Clear Decimal
        SetFlag(FLAG_DECIMAL, false);
        cycles++;
    }

    void CPU::CLI(Cycles& cycles) {
        // Clear Interrupt Disable
        SetFlag(FLAG_INTERRUPT, false);
        cycles++;
    }

    void CPU::CLV(Cycles& cycles) {
        // Clear Overflow
        SetFlag(FLAG_OVERFLOW, false);
        cycles++;
    }

    void CPU::SEC(Cycles& cycles) {
        // Set Carry
        SetFlag(FLAG_CARRY, true);
        cycles++;
    }

    void CPU::SED(Cycles& cycles) {
        // Set Decimal
        SetFlag(FLAG_DECIMAL, true);
        cycles++;
    }

    void CPU::SEI(Cycles& cycles) {
        // Set Interrupt Disable
        SetFlag(FLAG_INTERRUPT, true);
        cycles++;
    }

    // ====================================================================
    // SYSTEM INSTRUCTIONS
    // ====================================================================

    void CPU::BRK(Memory& memory, Cycles& cycles) {
        // Break - Software Interrupt
        
        // Increment PC (BRK is 2 bytes, but we only increment once here)
        PC++;
        
        cycles++; // Internal operation
        
        // Push PC onto stack
        PushWordToStack(memory, PC, cycles);
        
        // Push status register with B flag set
        Byte statusToStore = P | FLAG_BREAK | FLAG_UNUSED;
        PushByteToStack(memory, statusToStore, cycles);
        
        // Set interrupt disable flag
        SetFlag(FLAG_INTERRUPT, true);
        
        // Load PC from IRQ/BRK vector
        PC = memory.ReadWord(VECTOR_IRQ_BRK, cycles);
    }

    void CPU::NOP(Cycles& cycles) {
        // No Operation
        cycles++;
    }

    // ====================================================================
    // MAIN EXECUTION LOOP
    // ====================================================================

    Cycles CPU::Execute(Memory& memory) {
        // Execute a single instruction
        Cycles cyclesUsed = 0;
        
        // Fetch opcode
        Byte opcode = FetchByte(memory, cyclesUsed);
        
        // Decode and execute instruction
        switch (opcode) {
            // ============================================================
            // LDA - Load Accumulator
            // ============================================================
            case INS_LDA_IM:   LDA(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_LDA_ZP:   LDA(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_LDA_ZPX:  LDA(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_LDA_ABS:  LDA(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_LDA_ABSX: LDA(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            case INS_LDA_ABSY: LDA(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            case INS_LDA_INDX: LDA(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_LDA_INDY: LDA(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed)); break;
            
            // ============================================================
            // LDX - Load X Register
            // ============================================================
            case INS_LDX_IM:   LDX(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_LDX_ZP:   LDX(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_LDX_ZPY:  LDX(memory, cyclesUsed, AddrZeroPageY(memory, cyclesUsed)); break;
            case INS_LDX_ABS:  LDX(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_LDX_ABSY: LDX(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            
            // ============================================================
            // LDY - Load Y Register
            // ============================================================
            case INS_LDY_IM:   LDY(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_LDY_ZP:   LDY(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_LDY_ZPX:  LDY(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_LDY_ABS:  LDY(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_LDY_ABSX: LDY(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            
            // ============================================================
            // STA - Store Accumulator
            // ============================================================
            case INS_STA_ZP:   STA(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_STA_ZPX:  STA(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_STA_ABS:  STA(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_STA_ABSX: STA(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed, false)); break;
            case INS_STA_ABSY: STA(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed, false)); break;
            case INS_STA_INDX: STA(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_STA_INDY: STA(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed, false)); break;
            
            // ============================================================
            // STX - Store X Register
            // ============================================================
            case INS_STX_ZP:   STX(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_STX_ZPY:  STX(memory, cyclesUsed, AddrZeroPageY(memory, cyclesUsed)); break;
            case INS_STX_ABS:  STX(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            
            // ============================================================
            // STY - Store Y Register
            // ============================================================
            case INS_STY_ZP:   STY(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_STY_ZPX:  STY(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_STY_ABS:  STY(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            
            // ============================================================
            // Register Transfers
            // ============================================================
            case INS_TAX: TAX(cyclesUsed); break;
            case INS_TAY: TAY(cyclesUsed); break;
            case INS_TXA: TXA(cyclesUsed); break;
            case INS_TYA: TYA(cyclesUsed); break;
            case INS_TSX: TSX(cyclesUsed); break;
            case INS_TXS: TXS(cyclesUsed); break;
            
            // ============================================================
            // Stack Operations
            // ============================================================
            case INS_PHA: PHA(memory, cyclesUsed); break;
            case INS_PHP: PHP(memory, cyclesUsed); break;
            case INS_PLA: PLA(memory, cyclesUsed); break;
            case INS_PLP: PLP(memory, cyclesUsed); break;
            
            // ============================================================
            // Logical Operations - AND
            // ============================================================
            case INS_AND_IM:   AND(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_AND_ZP:   AND(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_AND_ZPX:  AND(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_AND_ABS:  AND(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_AND_ABSX: AND(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            case INS_AND_ABSY: AND(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            case INS_AND_INDX: AND(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_AND_INDY: AND(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed)); break;
            
            // ============================================================
            // Logical Operations - ORA
            // ============================================================
            case INS_ORA_IM:   ORA(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_ORA_ZP:   ORA(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_ORA_ZPX:  ORA(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_ORA_ABS:  ORA(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_ORA_ABSX: ORA(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            case INS_ORA_ABSY: ORA(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            case INS_ORA_INDX: ORA(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_ORA_INDY: ORA(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed)); break;
            
            // ============================================================
            // Logical Operations - EOR
            // ============================================================
            case INS_EOR_IM:   EOR(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_EOR_ZP:   EOR(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_EOR_ZPX:  EOR(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_EOR_ABS:  EOR(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_EOR_ABSX: EOR(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            case INS_EOR_ABSY: EOR(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            case INS_EOR_INDX: EOR(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_EOR_INDY: EOR(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed)); break;
            
            // ============================================================
            // BIT Test
            // ============================================================
            case INS_BIT_ZP:  BIT(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_BIT_ABS: BIT(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            
            // ============================================================
            // Arithmetic - ADC
            // ============================================================
            case INS_ADC_IM:   ADC(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_ADC_ZP:   ADC(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_ADC_ZPX:  ADC(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_ADC_ABS:  ADC(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_ADC_ABSX: ADC(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            case INS_ADC_ABSY: ADC(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            case INS_ADC_INDX: ADC(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_ADC_INDY: ADC(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed)); break;
            
            // ============================================================
            // Arithmetic - SBC
            // ============================================================
            case INS_SBC_IM:   SBC(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_SBC_ZP:   SBC(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_SBC_ZPX:  SBC(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_SBC_ABS:  SBC(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_SBC_ABSX: SBC(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            case INS_SBC_ABSY: SBC(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            case INS_SBC_INDX: SBC(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_SBC_INDY: SBC(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed)); break;
            
            // ============================================================
            // Compare - CMP
            // ============================================================
            case INS_CMP_IM:   CMP(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_CMP_ZP:   CMP(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_CMP_ZPX:  CMP(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_CMP_ABS:  CMP(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_CMP_ABSX: CMP(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed)); break;
            case INS_CMP_ABSY: CMP(memory, cyclesUsed, AddrAbsoluteY(memory, cyclesUsed)); break;
            case INS_CMP_INDX: CMP(memory, cyclesUsed, AddrIndexedIndirect(memory, cyclesUsed)); break;
            case INS_CMP_INDY: CMP(memory, cyclesUsed, AddrIndirectIndexed(memory, cyclesUsed)); break;
            
            // ============================================================
            // Compare - CPX, CPY
            // ============================================================
            case INS_CPX_IM:  CPX(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_CPX_ZP:  CPX(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_CPX_ABS: CPX(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            
            case INS_CPY_IM:  CPY(memory, cyclesUsed, AddrImmediate(memory, cyclesUsed)); break;
            case INS_CPY_ZP:  CPY(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_CPY_ABS: CPY(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            
            // ============================================================
            // Increment/Decrement
            // ============================================================
            case INS_INC_ZP:   INC(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_INC_ZPX:  INC(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_INC_ABS:  INC(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_INC_ABSX: INC(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed, false)); break;
            
            case INS_INX: INX(cyclesUsed); break;
            case INS_INY: INY(cyclesUsed); break;
            
            case INS_DEC_ZP:   DEC(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_DEC_ZPX:  DEC(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_DEC_ABS:  DEC(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_DEC_ABSX: DEC(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed, false)); break;
            
            case INS_DEX: DEX(cyclesUsed); break;
            case INS_DEY: DEY(cyclesUsed); break;
            
            // ============================================================
            // Shifts and Rotates
            // ============================================================
            case INS_ASL_ACC:  ASL_ACC(cyclesUsed); break;
            case INS_ASL_ZP:   ASL_MEM(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_ASL_ZPX:  ASL_MEM(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_ASL_ABS:  ASL_MEM(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_ASL_ABSX: ASL_MEM(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed, false)); break;
            
            case INS_LSR_ACC:  LSR_ACC(cyclesUsed); break;
            case INS_LSR_ZP:   LSR_MEM(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_LSR_ZPX:  LSR_MEM(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_LSR_ABS:  LSR_MEM(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_LSR_ABSX: LSR_MEM(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed, false)); break;
            
            case INS_ROL_ACC:  ROL_ACC(cyclesUsed); break;
            case INS_ROL_ZP:   ROL_MEM(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_ROL_ZPX:  ROL_MEM(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_ROL_ABS:  ROL_MEM(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_ROL_ABSX: ROL_MEM(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed, false)); break;
            
            case INS_ROR_ACC:  ROR_ACC(cyclesUsed); break;
            case INS_ROR_ZP:   ROR_MEM(memory, cyclesUsed, AddrZeroPage(memory, cyclesUsed)); break;
            case INS_ROR_ZPX:  ROR_MEM(memory, cyclesUsed, AddrZeroPageX(memory, cyclesUsed)); break;
            case INS_ROR_ABS:  ROR_MEM(memory, cyclesUsed, AddrAbsolute(memory, cyclesUsed)); break;
            case INS_ROR_ABSX: ROR_MEM(memory, cyclesUsed, AddrAbsoluteX(memory, cyclesUsed, false)); break;
            
            // ============================================================
            // Jumps and Calls
            // ============================================================
            case INS_JMP_ABS: {
                Address addr = AddrAbsolute(memory, cyclesUsed);
                JMP(addr);
                break;
            }
            case INS_JMP_IND: {
                // Indirect addressing for JMP
                Address indirectAddr = FetchWord(memory, cyclesUsed);
                
                // Note: 6502 has a bug with indirect JMP across page boundaries
                // If address is $xxFF, it wraps within the page
                if ((indirectAddr & 0x00FF) == 0x00FF) {
                    // Page boundary bug
                    Byte lowByte = memory.ReadByte(indirectAddr, cyclesUsed);
                    Byte highByte = memory.ReadByte(indirectAddr & 0xFF00, cyclesUsed);
                    Address targetAddr = (static_cast<Address>(highByte) << 8) | lowByte;
                    JMP(targetAddr);
                } else {
                    // Normal case
                    Address targetAddr = memory.ReadWord(indirectAddr, cyclesUsed);
                    JMP(targetAddr);
                }
                break;
            }
            
            case INS_JSR: {
                Address addr = AddrAbsolute(memory, cyclesUsed);
                JSR(memory, cyclesUsed, addr);
                break;
            }
            
            case INS_RTS: RTS(memory, cyclesUsed); break;
            case INS_RTI: RTI(memory, cyclesUsed); break;
            
            // ============================================================
            // Branches
            // ============================================================
            case INS_BCC: BranchIf(memory, cyclesUsed, !GetFlag(FLAG_CARRY)); break;
            case INS_BCS: BranchIf(memory, cyclesUsed, GetFlag(FLAG_CARRY)); break;
            case INS_BEQ: BranchIf(memory, cyclesUsed, GetFlag(FLAG_ZERO)); break;
            case INS_BMI: BranchIf(memory, cyclesUsed, GetFlag(FLAG_NEGATIVE)); break;
            case INS_BNE: BranchIf(memory, cyclesUsed, !GetFlag(FLAG_ZERO)); break;
            case INS_BPL: BranchIf(memory, cyclesUsed, !GetFlag(FLAG_NEGATIVE)); break;
            case INS_BVC: BranchIf(memory, cyclesUsed, !GetFlag(FLAG_OVERFLOW)); break;
            case INS_BVS: BranchIf(memory, cyclesUsed, GetFlag(FLAG_OVERFLOW)); break;
            
            // ============================================================
            // Status Flag Changes
            // ============================================================
            case INS_CLC: CLC(cyclesUsed); break;
            case INS_CLD: CLD(cyclesUsed); break;
            case INS_CLI: CLI(cyclesUsed); break;
            case INS_CLV: CLV(cyclesUsed); break;
            case INS_SEC: SEC(cyclesUsed); break;
            case INS_SED: SED(cyclesUsed); break;
            case INS_SEI: SEI(cyclesUsed); break;
            
            // ============================================================
            // System
            // ============================================================
            case INS_BRK: BRK(memory, cyclesUsed); break;
            case INS_NOP: NOP(cyclesUsed); break;
            
            // ============================================================
            // Unknown Opcode
            // ============================================================
            default:
                // Unknown instruction - could throw exception or halt
                // For now, treat as NOP and increment cycles
                cyclesUsed++;
                break;
        }
        
        // Update total cycle count
        TotalCycles += cyclesUsed;
        
        return cyclesUsed;
    }

    Cycles CPU::Execute(Cycles cycles, Memory& memory) {
        // Execute multiple instructions for specified number of cycles
        Cycles cyclesExecuted = 0;
        
        while (cyclesExecuted < cycles) {
            Cycles instructionCycles = Execute(memory);
            cyclesExecuted += instructionCycles;
        }
        
        return cyclesExecuted;
    }

} // namespace M6502
